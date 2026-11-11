// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
//
// Alias operation v1 of the native file session (native_wallet_session.h).
// Separate wallet translation unit; the existing session unit, scanner and
// payment paths are byte-identical. Legacy request_alias_registration/
// request_alias_update are never called: they broadcast by default (their
// final `false` argument is shuffle) and transfer(..., send_to_network=false)
// still selects, finalizes and signs. Preparation here calls only
// prepare_transaction(); signing only finalize_transaction(..., false, false).
#include "native_wallet_session.h"
#include "native_wallet_alias_pending.h"
#include "native_payment_transport.h"
#include "wallet2.h"
#include "currency_core/account.h"
#include "currency_core/currency_config.h"
#include "currency_core/currency_format_utils.h"
#include <algorithm>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <new>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tools { namespace native_session
{
  namespace
  {
    // Same discipline as the session unit's own busy scope: a callback that
    // re-enters the session observes Busy and changes nothing.
    struct alias_busy_scope
    {
      bool& value;
      explicit alias_busy_scope(bool& busy) : value(busy) { value = true; }
      ~alias_busy_scope() { value = false; }
      alias_busy_scope(const alias_busy_scope&) = delete;
      alias_busy_scope& operator=(const alias_busy_scope&) = delete;
    };

    bool same_alias_keys(const public_identity& left, const public_identity& right) noexcept
    {
      return left.spend_public_key == right.spend_public_key &&
        left.view_public_key == right.view_public_key && left.flags == right.flags;
    }

    // Any value other than a definite Committed/Rejected is Unknown.
    commit_status alias_checked(commit_status value) noexcept
    {
      switch (value)
      {
        case commit_status::Committed:
        case commit_status::Rejected: return value;
        default: return commit_status::Unknown;
      }
    }

    constexpr uint64_t burn_flags = currency::tx_destination_entry_flags::tdef_explicit_native_asset_id |
      currency::tx_destination_entry_flags::tdef_zero_amount_blinding_mask;

    bool reward_account(currency::account_public_address& out) noexcept
    {
      try { out = currency::account_public_address{}; return currency::get_aliases_reward_account(out); }
      catch (...) { return false; }
    }

    // The frozen semantic alias terms; the owner signature is never a term.
    bool same_alias_terms(const currency::extra_alias_entry& entry, const std::string& alias,
      const currency::account_public_address& address, const std::string& comment) noexcept
    {
      return entry.m_alias == alias && entry.m_address == address &&
        entry.m_text_comment == comment && entry.m_view_key.empty();
    }

    void wipe_sources(currency::finalize_tx_param& parameters) noexcept
    {
      for (auto& source : parameters.sources)
      {
        volatile unsigned char* a = reinterpret_cast<volatile unsigned char*>(&source.real_out_amount_blinding_mask);
        volatile unsigned char* b = reinterpret_cast<volatile unsigned char*>(&source.real_out_asset_id_blinding_mask);
        for (std::size_t n = 0; n < sizeof(source.real_out_amount_blinding_mask); ++n) a[n] = 0;
        for (std::size_t n = 0; n < sizeof(source.real_out_asset_id_blinding_mask); ++n) b[n] = 0;
      }
    }
  }

  // One rule (native_wallet_alias_pending.h) for preparation, the signed pending
  // row and wallet2's typed retained-pending reader.
  static_assert(alias_pending::name_min_bytes == min_alias_operation_name_bytes &&
    alias_pending::name_min_bytes == ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, "core short-name bound");
  static_assert(alias_pending::name_max_bytes == max_alias_operation_name_bytes &&
    alias_pending::name_max_bytes == ALIAS_NAME_MAX_LEN, "core name bound");
  static_assert(alias_pending::comment_max_bytes == max_alias_public_comment_bytes &&
    alias_pending::comment_max_bytes == ALIAS_COMMENT_MAX_SIZE_BYTES, "core public comment bound");
  static_assert(alias_pending::tx_type_new == GUI_TX_TYPE_NEW_ALIAS &&
    alias_pending::tx_type_update == GUI_TX_TYPE_UPDATE_ALIAS, "core alias tx types");

  // Strict public comment: RFC 3629 shortest-form UTF-8, at most 400 bytes.
  bool valid_alias_public_comment(const std::string& text) noexcept
  {
    return alias_pending::public_comment(text);
  }

  // Ordinary public names only: the core alphabet and maximum, and at least the
  // public short-name minimum (shorter names need the special authority key).
  bool valid_alias_operation_name(const std::string& alias) noexcept
  {
    if (!alias_pending::name_shape(alias)) return false;
    try { return currency::validate_alias_name(alias); } catch (...) { return false; }
  }

  struct file_session::prepared_alias::data
  {
    currency::finalize_tx_param parameters{};
    public_identity identity{};
    std::string saved, alias, comment, address;
    alias_operation_kind_v1 kind = alias_operation_kind_v1::Register;
    std::vector<held_input> inputs;
    uint64_t generation = 0, revision = 0, burn = 0, fee = 0, total = 0, input_amount = 0;
    uint64_t local_blocks = 0, confidential_decoys = 0, bare_decoys = 0, tx_version = 0, hardfork = 0;
    crypto::hash tip{};
    data() = default;
    data(const data&) = default;
    ~data() noexcept { wipe_sources(parameters); }
  };
  file_session::prepared_alias::prepared_alias(std::unique_ptr<data> d) : m_data(std::move(d)) {}
  file_session::prepared_alias::~prepared_alias() noexcept = default;
  alias_operation_kind_v1 file_session::prepared_alias::kind() const noexcept { return m_data->kind; }
  const std::string& file_session::prepared_alias::alias() const noexcept { return m_data->alias; }
  const std::string& file_session::prepared_alias::public_comment() const noexcept { return m_data->comment; }
  const std::string& file_session::prepared_alias::bound_address() const noexcept { return m_data->address; }
  uint64_t file_session::prepared_alias::protocol_burn() const noexcept { return m_data->burn; }
  uint64_t file_session::prepared_alias::miner_fee() const noexcept { return m_data->fee; }
  uint64_t file_session::prepared_alias::total_native() const noexcept { return m_data->total; }
  uint64_t file_session::prepared_alias::input_count() const noexcept { return m_data->inputs.size(); }
  uint64_t file_session::prepared_alias::input_amount() const noexcept { return m_data->input_amount; }
  uint64_t file_session::prepared_alias::local_blocks() const noexcept { return m_data->local_blocks; }
  const crypto::hash& file_session::prepared_alias::local_tip() const noexcept { return m_data->tip; }
  uint64_t file_session::prepared_alias::tx_version() const noexcept { return m_data->tx_version; }
  uint64_t file_session::prepared_alias::hardfork() const noexcept { return m_data->hardfork; }
  uint64_t file_session::prepared_alias::generation() const noexcept { return m_data->generation; }
  uint64_t file_session::prepared_alias::revision() const noexcept { return m_data->revision; }

  file_session::alias_prepare_result_v1 file_session::prepare_alias_review_v1(
    const alias_operation_request_v1& supplied, const std::string& supplied_saved,
    std::shared_ptr<native_payment::decoy_exchange> exchange, std::atomic<bool>& stop,
    native_payment::construction_detail* construction) noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    alias_prepare_result_v1 out;
    using detail = native_payment::construction_detail;
    if (construction) *construction = detail::None;
    detail first_failure = detail::None;
    detail exception_stage = detail::PreparationException;
    const auto refuse = [&out](sign_status status, alias_detail_v1 why = alias_detail_v1::None) {
      out.status = status; out.detail = why; out.reviewed.reset(); return out;
    };
    const auto failed = [&](detail fallback, alias_detail_v1 why) {
      if (construction) *construction = first_failure == detail::None ? fallback : first_failure;
      return refuse(sign_status::PreparationFailed, why);
    };
    // Replay scratch carries no signing authority (same rule as payments).
    if (m_busy || m_replay_wallet) return refuse(sign_status::Busy);
    switch (state())
    {
      case session_state::OpenUnchanged:
      case session_state::OpenDirty: break;
      case session_state::RecoveryRequired: return refuse(sign_status::RecoveryRequired);
      case session_state::OutcomeUnknown: return refuse(sign_status::OutcomeUnknown);
      default: return refuse(sign_status::Unavailable);
    }
    if (!m_wallet || !m_material) return refuse(sign_status::Unavailable);
    // Bound every caller-owned value before copying it.
    if (supplied.version != 1) return refuse(sign_status::InvalidRequest, alias_detail_v1::InvalidVersion);
    if (supplied.alias.size() > max_alias_operation_name_bytes)
      return refuse(sign_status::InvalidRequest, alias_detail_v1::InvalidName);
    if (supplied.public_comment.size() > max_alias_public_comment_bytes)
      return refuse(sign_status::InvalidRequest, alias_detail_v1::InvalidComment);
    if (supplied.observed_owner_address.size() > max_payment_recipient_bytes)
      return refuse(sign_status::InvalidRequest, alias_detail_v1::InvalidObservation);
    if (supplied_saved.empty() || supplied_saved.size() > 4096) return refuse(sign_status::InvalidRequest);
    try
    {
      const alias_operation_request_v1 request = supplied;
      const std::string saved = supplied_saved;
      if (!authorized() || stop.load()) return refuse(sign_status::Revoked);
      if (m_revision_exhausted || m_revision == std::numeric_limits<uint64_t>::max())
        return refuse(sign_status::Unsupported);
      if (request.expected_generation == 0 || request.expected_generation != m_generation ||
          request.expected_revision != m_revision)
        return refuse(sign_status::StaleObservation);
      const auto& profile = build_profile();
      if (profile.status != profile_status::Available || profile.network != compiled_network::Testnet ||
          profile.formation != 103 || profile.genesis == currency::null_hash)
        return refuse(sign_status::Unsupported, alias_detail_v1::UnsupportedProfile);
      const auto identity_current = [this, &profile]() {
        const auto& address = m_wallet->m_account.get_public_address();
        return same_alias_keys(m_identity, m_material->material().expected) &&
          m_material->material().expected.stored_genesis == m_identity.stored_genesis &&
          m_identity.stored_genesis == profile.genesis && m_wallet->m_chain.get_genesis() == profile.genesis &&
          address.spend_public_key == m_identity.spend_public_key &&
          address.view_public_key == m_identity.view_public_key && address.flags == m_identity.flags;
      };
      if (!identity_current()) return refuse(sign_status::InvalidRequest, alias_detail_v1::UnsupportedWallet);
      if (!m_wallet->m_native_file_mode || m_wallet->is_watch_only() || m_wallet->m_account.is_watch_only() ||
          m_identity.flags != 0 || !m_wallet->native_scan_minimum_supported() || m_wallet->m_read_only_sync_active)
        return refuse(sign_status::Unsupported, alias_detail_v1::UnsupportedWallet);

      // Name, public comment and node observations: typed, never repaired.
      if (!valid_alias_operation_name(request.alias))
      {
        bool alphabet = !request.alias.empty();
        try { alphabet = alphabet && currency::validate_alias_name(request.alias); } catch (...) { alphabet = false; }
        return refuse(sign_status::InvalidRequest, alphabet && request.alias.size() < min_alias_operation_name_bytes
          ? alias_detail_v1::ShortName : alias_detail_v1::InvalidName);
      }
      if (!valid_alias_public_comment(request.public_comment))
        return refuse(sign_status::InvalidRequest, alias_detail_v1::InvalidComment);
      switch (request.kind)
      {
        case alias_operation_kind_v1::Register:
          if (request.observation != alias_node_observation_v1::Unregistered || !request.observed_owner_address.empty())
            return refuse(sign_status::InvalidRequest, alias_detail_v1::InvalidObservation);
          if (request.quoted_burn == 0) return refuse(sign_status::InvalidRequest, alias_detail_v1::InvalidBurn);
          break;
        case alias_operation_kind_v1::Update:
          if (request.observation != alias_node_observation_v1::OwnedByThisWallet)
            return refuse(sign_status::InvalidRequest, alias_detail_v1::InvalidObservation);
          if (request.observed_owner_address != m_address)
            return refuse(sign_status::InvalidRequest, alias_detail_v1::OwnerMismatch);
          if (request.quoted_burn != 0) return refuse(sign_status::InvalidRequest, alias_detail_v1::InvalidBurn);
          break;
        default: return refuse(sign_status::InvalidRequest, alias_detail_v1::InvalidObservation);
      }
      const uint64_t fee = m_wallet->get_default_fee(); // Compiled policy fee, never a node value.
      if (fee == 0 || request.quoted_burn > std::numeric_limits<uint64_t>::max() - fee)
        return refuse(sign_status::InvalidRequest, alias_detail_v1::AmountOverflow);
      const uint64_t total = request.quoted_burn + fee;

      // Same retained-pending conservatism as ordinary payment assessment.
      if (m_wallet->m_unconfirmed_txs.size() > max_snapshot_records ||
          m_wallet->m_unconfirmed_in_transfers.size() > max_snapshot_records ||
          m_wallet->m_unconfirmed_multisig_transfers.size() > max_snapshot_records ||
          m_wallet->m_money_expirations.size() > max_snapshot_records)
        return refuse(sign_status::Unsupported, alias_detail_v1::RetainedPending);
      if (!m_wallet->m_unconfirmed_in_transfers.empty() ||
          !m_wallet->m_unconfirmed_multisig_transfers.empty() || !m_wallet->m_money_expirations.empty())
        return refuse(sign_status::Unsupported, alias_detail_v1::RetainedPending);
      size_t pending_work = 0;
      std::vector<std::string> pending_aliases;
      const auto pending = typed_pending_supported(pending_work, &pending_aliases);
      if (pending == snapshot_status::Revoked) return refuse(sign_status::Revoked);
      if (pending != snapshot_status::Available) return refuse(sign_status::Unsupported, alias_detail_v1::RetainedPending);
      // One unconfirmed operation per name: a second would race its own pending terms.
      if (std::find(pending_aliases.begin(), pending_aliases.end(), request.alias) != pending_aliases.end())
        return refuse(sign_status::Unsupported, alias_detail_v1::RetainedPending);
      // The core extra form is fixed to the post-HF2 extra_alias_entry.
      if (!m_wallet->m_core_runtime_config.is_hardfork_active_for_height(2, m_wallet->get_top_block_height()))
        return refuse(sign_status::Unsupported, alias_detail_v1::HardforkUnsupported);
      const auto clock = m_wallet->get_core_runtime_config().get_core_time;
      if (!clock) return refuse(sign_status::Unavailable);
      const uint64_t captured_at = clock();
      const uint64_t local_blocks = m_wallet->m_chain.get_blockchain_current_size();
      uint64_t capacity = 0, eligible = 0;
      if (m_wallet->native_payment_capacity(captured_at, local_blocks, capacity, eligible, pending_work) !=
          snapshot_status::Available)
        return failed(detail::LocalCoverage, alias_detail_v1::LocalCoverage);
      if (capacity < total) return failed(detail::LocalCoverage, alias_detail_v1::LocalCoverage);
      if (stop.load()) return failed(detail::StoppedBeforeConstruction, alias_detail_v1::Construction);
      if (!exchange) return failed(detail::MissingExchange, alias_detail_v1::Construction);
      if (!m_material->signed_payment_supported() || !m_material->checkpoint_supported())
        return refuse(sign_status::Unsupported);
      if (!authorized()) return refuse(sign_status::Revoked);

      alias_busy_scope guard(m_busy);
      auto proxy = native_payment::make_payment_transport(std::move(exchange), local_blocks, stop, &first_failure);
      if (!proxy) return failed(detail::ProxyCreation, alias_detail_v1::Construction);
      struct proxy_scope {
        std::shared_ptr<i_core_proxy>& slot;
        std::shared_ptr<i_core_proxy> prior;
        proxy_scope(std::shared_ptr<i_core_proxy>& value, std::shared_ptr<i_core_proxy> next)
          : slot(value), prior(value) { slot = std::move(next); }
        ~proxy_scope() { slot = std::move(prior); }
      };
      exception_stage = detail::ConstructionSetupException;
      const currency::account_public_address own = m_wallet->m_account.get_public_address();
      if (currency::get_account_address_as_str(own) != m_address)
        return refuse(sign_status::InvalidRequest, alias_detail_v1::UnsupportedWallet);
      currency::extra_alias_entry entry{};
      entry.m_alias = request.alias;
      entry.m_address = own; // This wallet only: no transfer, no tracking key.
      entry.m_text_comment = request.public_comment;
      currency::account_public_address reward{};
      auto ctp = m_wallet->get_default_construct_tx_param_inital();
      ctp.dsts.clear();
      if (request.kind == alias_operation_kind_v1::Register)
      {
        if (!reward_account(reward)) return failed(detail::ConstructionSetupException, alias_detail_v1::Construction);
        currency::tx_destination_entry burn{};
        burn.addr.push_back(reward);
        burn.amount = request.quoted_burn;
        burn.asset_id = currency::native_coin_asset_id;
        burn.flags |= burn_flags;
        ctp.dsts.push_back(burn);
      }
      ctp.fee = fee;
      ctp.fake_outputs_count = CURRENCY_DEFAULT_DECOY_SET_SIZE;
      ctp.unlock_time = 0;
      ctp.extra.clear(); ctp.attachments.clear();
      ctp.extra.push_back(entry); // Post-HF2 form, as the core helper pushes after HF2.
      ctp.flags = 0; ctp.multisig_id = currency::null_hash;
      ctp.create_utxo_defragmentation_tx = false;
      ctp.ado_sign_thirdparty = false;
      currency::finalize_tx_param ftp{};
      ftp.tx_version = m_wallet->get_current_tx_version_and_hardfork_id(ftp.tx_hardfork_id);
      {
        proxy_scope bounded(m_wallet->m_core_proxy, std::move(proxy));
        exception_stage = detail::UnsignedConstructionException;
        if (!m_wallet->prepare_transaction(ctp, ftp))
          return failed(detail::UnsignedConstructionFalse, alias_detail_v1::Construction);
      }
      exception_stage = detail::PreparedRecordException;
      if (stop.load() || !authorized()) return refuse(sign_status::Revoked);

      // Exact prepared shape: the one unsigned alias entry, no attachment, and
      // destinations that are the burn (register only) plus this wallet's change.
      const auto shape_failed = [&]() { wipe_sources(ftp); return failed(detail::PreparedShape, alias_detail_v1::PreparedShape); };
      if (ftp.sources.empty() || ftp.sources.size() > CURRENCY_TX_MAX_ALLOWED_INPUTS ||
          ftp.selected_transfers.size() != ftp.sources.size() || ftp.prepared_destinations.empty() ||
          ftp.prepared_destinations.size() > CURRENCY_TX_MAX_ALLOWED_OUTS || ftp.extra.size() != 1 ||
          !ftp.attachments.empty() || ftp.unlock_time != 0 || ftp.flags != 0 ||
          ftp.multisig_id != currency::null_hash || ftp.ado_sign_thirdparty)
        return shape_failed();
      const auto* frozen = boost::get<currency::extra_alias_entry>(&ftp.extra.front());
      if (!frozen || !same_alias_terms(*frozen, request.alias, own, request.public_comment) || !frozen->m_sign.empty())
        return shape_failed();
      uint64_t inputs_total = 0, burned = 0, change = 0;
      for (const auto& source : ftp.sources)
      {
        if (source.amount > std::numeric_limits<uint64_t>::max() - inputs_total) return shape_failed();
        inputs_total += source.amount;
      }
      for (const auto& destination : ftp.prepared_destinations)
      {
        if (destination.asset_id != currency::native_coin_asset_id || destination.addr.size() != 1 ||
            destination.unlock_time != 0 || destination.payment_id != 0 ||
            destination.addr.front().type() != typeid(currency::account_public_address))
          return shape_failed();
        const auto& to = boost::get<currency::account_public_address>(destination.addr.front());
        if (request.kind == alias_operation_kind_v1::Register && to == reward)
        {
          if ((destination.flags & burn_flags) != burn_flags ||
              destination.amount > std::numeric_limits<uint64_t>::max() - burned) return shape_failed();
          burned += destination.amount;
        }
        else if (to == own)
        {
          if (destination.amount > std::numeric_limits<uint64_t>::max() - change) return shape_failed();
          change += destination.amount;
        }
        else return shape_failed();
      }
      if (burned != request.quoted_burn || inputs_total < total || change != inputs_total - total)
        return shape_failed();

      std::unique_ptr<prepared_alias::data> built(new prepared_alias::data());
      built->saved = saved;
      built->identity = m_identity;
      built->generation = m_generation; built->revision = m_revision;
      built->kind = request.kind;
      built->alias = request.alias; built->comment = request.public_comment; built->address = m_address;
      built->burn = request.quoted_burn; built->fee = fee; built->total = total;
      built->input_amount = inputs_total;
      built->local_blocks = local_blocks; built->tip = m_wallet->m_chain.get_top_block_id();
      built->confidential_decoys = m_wallet->get_core_runtime_config().hf4_minimum_mixins;
      built->bare_decoys = CURRENCY_DEFAULT_DECOY_SET_SIZE;
      built->tx_version = ftp.tx_version; built->hardfork = ftp.tx_hardfork_id;
      for (const auto index : ftp.selected_transfers)
      {
        const auto it = m_wallet->m_transfers.find(index);
        if (it == m_wallet->m_transfers.end() || !it->second.m_ptx_wallet_info ||
            m_wallet->native_input_held(it->second)) { wipe_sources(ftp); return failed(detail::SelectedInput, alias_detail_v1::Construction); }
        const auto& transfer = it->second;
        held_input input{currency::get_transaction_hash(transfer.m_ptx_wallet_info->m_tx),
          transfer.m_internal_output_index, transfer.m_key_image};
        if (input.source_transaction == currency::null_hash || input.key_image == currency::null_ki)
        { wipe_sources(ftp); return failed(detail::SelectedInputIdentity, alias_detail_v1::Construction); }
        for (const auto& prior : built->inputs)
          if (prior.key_image == input.key_image)
          { wipe_sources(ftp); return failed(detail::DuplicateSelectedInput, alias_detail_v1::Construction); }
        built->inputs.push_back(input);
      }
      built->parameters = std::move(ftp);
      out.reviewed = std::shared_ptr<prepared_alias>(new prepared_alias(std::move(built)));
      out.status = sign_status::Prepared;
      out.detail = alias_detail_v1::None;
      return out;
    }
    catch (...)
    {
      if (m_wallet) m_wallet->m_found_free_amounts.clear();
      return failed(exception_stage, alias_detail_v1::Construction);
    }
  }

  bool file_session::build_alias_pending(const currency::transaction& tx,
    const currency::finalize_tx_param& parameters, const crypto::hash& expected_hash,
    uint64_t burn, uint64_t fee, const std::atomic<bool>& stop,
    wallet_public::wallet_transfer_info& out) const
  {
    // A local description of the actual newly signed alias operation. No
    // pending container, flag, callback, lookup or transport is touched.
    const auto& w = *m_wallet;
    if (!w.m_native_file_mode || w.m_watch_only || w.m_account.is_watch_only() ||
        w.m_read_only_sync_active || w.m_read_only_sync_faulted || stop.load() ||
        fee == 0 || burn > std::numeric_limits<uint64_t>::max() - fee ||
        tx.vin.empty() || tx.vin.size() > CURRENCY_TX_MAX_ALLOWED_INPUTS ||
        tx.vout.empty() || tx.vout.size() > CURRENCY_TX_MAX_ALLOWED_OUTS ||
        parameters.selected_transfers.size() != tx.vin.size() || parameters.sources.size() != tx.vin.size() ||
        parameters.prepared_destinations.empty() ||
        parameters.prepared_destinations.size() > CURRENCY_TX_MAX_ALLOWED_OUTS ||
        parameters.unlock_time != 0 || parameters.expiration_time != 0 || parameters.flags != 0 ||
        parameters.multisig_id != currency::null_hash || parameters.ado_sign_thirdparty ||
        parameters.mode_separate_fee != 0 || !parameters.attachments.empty() ||
        !tx.attachment.empty() || expected_hash == currency::null_hash ||
        currency::get_transaction_hash(tx) != expected_hash ||
        w.m_unconfirmed_txs.size() >= max_snapshot_records ||
        w.m_transfer_history.size() > max_snapshot_records ||
        w.m_unconfirmed_txs.count(expected_hash) || w.m_unconfirmed_in_transfers.count(expected_hash) ||
        w.m_tx_keys.count(expected_hash))
      return false;
    for (const auto& prior : w.m_transfer_history)
      if (stop.load() || prior.tx_hash == expected_hash) return false;
    const uint64_t total = burn + fee;
    uint64_t actual_fee = 0;
    if (!currency::get_tx_fee(tx, actual_fee) || actual_fee != fee) return false;
    const auto add = [](uint64_t& sum, uint64_t value) {
      if (value > std::numeric_limits<uint64_t>::max() - sum) return false;
      sum += value; return true;
    };
    std::unordered_map<crypto::key_image, std::pair<uint64_t, const transfer_details*>> selected;
    uint64_t spent = 0;
    for (const auto index : parameters.selected_transfers)
    {
      if (stop.load()) return false;
      const auto found = w.m_transfers.find(index);
      if (found == w.m_transfers.end() || !found->second.m_ptx_wallet_info ||
          !found->second.is_native_coin() || found->second.m_key_image == currency::null_ki ||
          !selected.emplace(found->second.m_key_image, std::make_pair(index, &found->second)).second ||
          !add(spent, found->second.amount()))
        return false;
    }
    wallet2::process_transaction_context context(tx);
    for (std::size_t index = 0; index != tx.vin.size(); ++index)
    {
      if (stop.load()) return false;
      crypto::key_image image{};
      if (tx.vin[index].type() == typeid(currency::txin_to_key))
        image = boost::get<currency::txin_to_key>(tx.vin[index]).k_image;
      else if (tx.vin[index].type() == typeid(currency::txin_zc_input))
        image = boost::get<currency::txin_zc_input>(tx.vin[index]).k_image;
      else return false;
      const auto found = selected.find(image);
      if (found == selected.end()) return false;
      context.handle_incoming_tx_input(found->second.first, *found->second.second, index);
      selected.erase(found);
    }
    if (!selected.empty() || spent < total) return false;
    const auto& own = w.m_account.get_public_address();
    currency::account_public_address reward{};
    if (burn != 0 && !reward_account(reward)) return false;
    uint64_t destinations = 0, burned = 0;
    for (const auto& destination : parameters.prepared_destinations)
    {
      if (stop.load() || destination.asset_id != currency::native_coin_asset_id ||
          destination.addr.size() != 1 || destination.unlock_time != 0 || destination.payment_id != 0 ||
          destination.addr.front().type() != typeid(currency::account_public_address) ||
          !add(destinations, destination.amount))
        return false;
      const auto& address = boost::get<currency::account_public_address>(destination.addr.front());
      if (burn != 0 && address == reward) { if (!add(burned, destination.amount)) return false; }
      else if (address != own) return false;
    }
    if (destinations != spent - fee || burned != burn) return false;
    crypto::public_key public_key{};
    crypto::key_derivation derivation{};
    std::vector<currency::wallet_out_info> received;
    if (!currency::parse_and_validate_tx_extra(tx, public_key) || public_key == currency::null_pkey ||
        !currency::lookup_acc_outs(w.m_account.get_keys(), tx, public_key, received, derivation))
      return false;
    uint64_t received_amount = 0;
    std::unordered_set<std::size_t> received_indices;
    for (const auto& output : received)
    {
      if (stop.load() || output.index >= tx.vout.size() || !output.is_native_coin() ||
          output.payment_id != 0 || !received_indices.insert(output.index).second ||
          !add(received_amount, output.amount))
        return false;
      const auto& vout = tx.vout[output.index];
      if (vout.type() == typeid(currency::tx_out_bare))
      {
        if (boost::get<currency::tx_out_bare>(vout).target.type() != typeid(currency::txout_to_key)) return false;
      }
      else if (vout.type() != typeid(currency::tx_out_zarcanum)) return false;
      context.handle_incoming_tx_output(output);
    }
    if (received_amount != spent - total || context.total_balance_change.size() != 1 ||
        context.total_balance_change.at(currency::native_coin_asset_id) != -boost::multiprecision::int128_t(total))
      return false;
    wallet_public::wallet_transfer_info pending{};
    pending.tx = tx;
    pending.tx_hash = expected_hash;
    pending.employed_entries = std::move(context.employed_entries);
    pending.selected_indicies = parameters.selected_transfers;
    // No counterparty: the burn account is not a payee and update has none.
    pending.unlock_time = wallet2::get_max_unlock_time_from_receive_indices(tx, pending.employed_entries);
    if (pending.unlock_time != 0) return false;
    const auto bytes = currency::get_tx_real_blobsize(tx);
    if (bytes == 0 || bytes >= CURRENCY_MAX_TRANSACTION_BLOB_SIZE || bytes > std::numeric_limits<uint32_t>::max())
      return false;
    pending.tx_blob_size = static_cast<uint32_t>(bytes);
    currency::load_wallet_transfer_info_flags(pending);
    const uint64_t expected_type = burn != 0 ? GUI_TX_TYPE_NEW_ALIAS : GUI_TX_TYPE_UPDATE_ALIAS;
    if (pending.is_mining || pending.tx_type != expected_type || pending.fee != fee) return false;
    // One net native leg, as core processing records it: burn plus fee out.
    wallet_public::wallet_sub_transfer_info leg{};
    leg.asset_id = currency::native_coin_asset_id;
    leg.amount = total;
    leg.is_income = false;
    pending.access_subtransfers_default_pid().push_back(leg);
    const auto clock = w.m_core_runtime_config.get_core_time;
    if (!clock || stop.load()) return false;
    pending.timestamp = clock();
    if (stop.load()) return false;
    out = std::move(pending);
    return true;
  }

  // The session's view of the one typed retained-pending reader. wallet2's
  // native_signed_pending_supported keeps every ordinary payment check and
  // admits a counterparty-free row only as an actual alias registration/update
  // of this wallet (alias_pending::classify plus the transaction-level proof:
  // hash, bytes, tx key, fee, burn in outputs, authenticated holds disjoint
  // across both kinds, owned change, balance). The same reader gates wallet2's
  // history replay, so a pending alias neither blocks it nor is relabelled.
  // Nothing is set aside, released, replayed or re-described. The names come
  // only from rows that reader has just admitted, re-decoded by the core.
  snapshot_status file_session::typed_pending_supported(std::size_t& nested,
    std::vector<std::string>* pending_aliases) noexcept
  {
    const std::function<bool()> current = [this] { return authorized(); };
    try
    {
      if (!m_wallet) return snapshot_status::Unavailable;
      const auto status = m_wallet->native_signed_pending_supported(nested, current);
      if (status != snapshot_status::Available || !pending_aliases) return status;
      std::vector<std::string> names;
      for (const auto& item : m_wallet->m_unconfirmed_txs)
      {
        if (!current()) return snapshot_status::Revoked;
        if (!item.second.remote_addresses.empty()) continue;
        currency::tx_extra_info parsed{};
        if (!currency::parse_and_validate_tx_extra(item.second.tx, parsed) ||
            !valid_alias_operation_name(parsed.m_alias.m_alias))
          return snapshot_status::InvalidData;
        names.push_back(parsed.m_alias.m_alias);
      }
      if (!current()) return snapshot_status::Revoked;
      *pending_aliases = std::move(names);
      return snapshot_status::Available;
    }
    catch (...) { return snapshot_status::Unavailable; } // No release, payload or optimistic proof.
  }

  file_session::sign_result file_session::sign_reviewed_alias_to_held_v1(
    prepared_alias& reviewed, signing_admission& admission, std::atomic<bool>& stop) noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    sign_result out;
    const auto refuse = [&out](sign_status status) { out.status = status; return out; };
    if (m_busy || m_replay_wallet) return refuse(sign_status::Busy);
    if (reviewed.m_consumed || !reviewed.m_data) return refuse(sign_status::InvalidRequest);
    reviewed.m_consumed = true;
    bool construction_entered = false, publication_boundary_entered = false;
    try
    {
      const auto& terms = *reviewed.m_data;
      crypto::hash saved_hash{};
      crypto::cn_fast_hash(terms.saved.data(), terms.saved.size(), saved_hash);
      if (!m_wallet || !m_material || !authorized() || stop.load() || !admission.current() ||
          admission.saved_draft_hash() != saved_hash || !same_alias_keys(terms.identity, m_identity) ||
          terms.identity.stored_genesis != m_identity.stored_genesis ||
          !same_alias_keys(admission.subject(), m_identity) ||
          admission.subject().stored_genesis != m_identity.stored_genesis || terms.address != m_address)
        return refuse(sign_status::Revoked);
      // A fresh protected reopen deliberately changes generation. Chain, tip,
      // hardfork, inputs, decoys, burn and fee remain exact; nothing is rebuilt.
      size_t hardfork = 0;
      const uint64_t version = m_wallet->get_current_tx_version_and_hardfork_id(hardfork);
      if ((terms.generation == m_generation && terms.revision != m_revision) ||
          terms.local_blocks != m_wallet->m_chain.get_blockchain_current_size() ||
          terms.tip != m_wallet->m_chain.get_top_block_id() || terms.fee != TX_MINIMUM_FEE ||
          terms.confidential_decoys != m_wallet->get_core_runtime_config().hf4_minimum_mixins ||
          terms.bare_decoys != CURRENCY_DEFAULT_DECOY_SET_SIZE ||
          terms.parameters.tx_version != version || terms.parameters.tx_hardfork_id != hardfork ||
          terms.tx_version != version || terms.hardfork != hardfork ||
          !m_wallet->m_core_runtime_config.is_hardfork_active_for_height(2, m_wallet->get_top_block_height()))
        return refuse(sign_status::StaleObservation);
      if (!m_material->signed_payment_supported() || !m_material->checkpoint_supported())
        return refuse(sign_status::Unsupported);
      if (terms.inputs.size() != terms.parameters.selected_transfers.size() ||
          terms.inputs.size() != terms.parameters.sources.size() || terms.parameters.extra.size() != 1)
        return refuse(sign_status::InvalidRequest);
      for (std::size_t n = 0; n < terms.inputs.size(); ++n)
      {
        const auto found = m_wallet->m_transfers.find(terms.parameters.selected_transfers[n]);
        const auto& input = terms.inputs[n];
        if (found == m_wallet->m_transfers.end() || !found->second.m_ptx_wallet_info ||
            currency::get_transaction_hash(found->second.m_ptx_wallet_info->m_tx) != input.source_transaction ||
            found->second.m_internal_output_index != input.output_index || found->second.m_key_image != input.key_image ||
            found->second.m_amount != terms.parameters.sources[n].amount || m_wallet->native_input_held(found->second) ||
            !m_wallet->is_transfer_ready_to_go(found->second,
              found->second.is_zc() ? terms.confidential_decoys : terms.bare_decoys))
          return refuse(sign_status::StaleObservation);
      }
      const currency::account_public_address own = m_wallet->m_account.get_public_address();
      {
        const auto* frozen = boost::get<currency::extra_alias_entry>(&terms.parameters.extra.front());
        if (!frozen || !same_alias_terms(*frozen, terms.alias, own, terms.comment) || !frozen->m_sign.empty())
          return refuse(sign_status::InvalidRequest);
      }
      prepared_alias::data working = terms; // Its destructor wipes copied source masks too.
      currency::finalize_tx_param& ftp = working.parameters;
      if (!admission.current() || !authorized() || !admission.consume()) return refuse(sign_status::Revoked);
      if (m_revision == std::numeric_limits<uint64_t>::max()) return refuse(sign_status::Unsupported);
      alias_busy_scope guard(m_busy);
      std::unique_ptr<signed_payment> built(new signed_payment());
      built->saved_draft = terms.saved; built->identity = terms.identity;
      built->generation = m_generation; built->revision = m_revision;
      built->amount = terms.burn; built->fee = terms.fee; built->inputs = terms.inputs;
      currency::finalized_tx finalized{};
      struct secret_scope {
        crypto::secret_key& value;
        ~secret_scope() {
          volatile unsigned char* p = reinterpret_cast<volatile unsigned char*>(&value);
          for (std::size_t i = 0; i < sizeof(value); ++i) p[i] = 0;
        }
      } wipe_key{finalized.one_time_key};
      if (stop.load() || !admission.current() || !authorized()) return refuse(sign_status::Revoked);
      // No authority escapes from the in-memory construction. Any failure from
      // here discards this session through its sticky recovery path.
      construction_entered = true;
      if (terms.kind == alias_operation_kind_v1::Update)
      {
        // Ownership signature, only now: the reviewed semantic entry is signed in
        // place inside the frozen extra. The signed buffer excludes m_sign, so
        // alias, address, comment, inputs and amounts stay exactly as reviewed.
        auto* entry = boost::get<currency::extra_alias_entry>(&ftp.extra.front());
        const auto& keys = m_wallet->m_account.get_keys();
        if (!entry || !currency::sign_extra_alias_entry(*entry, keys.account_address.spend_public_key, keys.spend_secret_key) ||
            entry->m_sign.size() != 1 || !same_alias_terms(*entry, terms.alias, own, terms.comment) ||
            !crypto::check_signature(currency::get_sign_buff_hash_for_alias_update(*entry),
              keys.account_address.spend_public_key, entry->m_sign.back())) {
          mark_recovery(); return refuse(sign_status::RecoveryRequired);
        }
      }
      // No dispatch, tx-key map insertion or sent-history callback here.
      m_wallet->finalize_transaction(ftp, finalized, false, false);
      built->transaction_blob = currency::tx_to_blob(finalized.tx);
      built->transaction_hash = currency::get_transaction_hash(finalized.tx);
      built->transaction_key = finalized.one_time_key;
      uint64_t actual_fee = 0;
      currency::tx_extra_info parsed{};
      const bool parsed_ok = currency::parse_and_validate_tx_extra(finalized.tx, parsed);
      if (built->transaction_blob.empty() || built->transaction_blob.size() >= CURRENCY_MAX_TRANSACTION_BLOB_SIZE ||
          built->transaction_hash == currency::null_hash ||
          !currency::get_tx_fee(finalized.tx, actual_fee) || actual_fee != built->fee || !parsed_ok ||
          !same_alias_terms(parsed.m_alias, terms.alias, own, terms.comment) ||
          parsed.m_alias.m_sign.size() != (terms.kind == alias_operation_kind_v1::Update ? 1u : 0u) ||
          !currency::check_native_coins_amount_burnt_in_outs(finalized.tx, terms.burn)) {
        mark_recovery(); return refuse(sign_status::RecoveryRequired);
      }
      if (stop.load() || !admission.current() || !authorized()) { mark_recovery(); return refuse(sign_status::Revoked); }
      wallet_public::wallet_transfer_info pending{};
      if (!build_alias_pending(finalized.tx, ftp, built->transaction_hash, terms.burn, terms.fee, stop, pending)) {
        mark_recovery();
        return refuse(stop.load() ? sign_status::Revoked : sign_status::RecoveryRequired);
      }
      if (stop.load() || !admission.current() || !authorized()) { mark_recovery(); return refuse(sign_status::Revoked); }
      // Every following mutation belongs to this same recoverable SignedHeld image.
      m_wallet->m_read_only_sync_dirty = true;
      m_state = session_state::OpenDirty;
      ++m_revision; m_page_capture = 0;
      m_wallet->m_native_held_inputs.insert(m_wallet->m_native_held_inputs.end(),
        built->inputs.begin(), built->inputs.end());
      m_wallet->m_tx_keys.emplace(built->transaction_hash, built->transaction_key);
      m_wallet->mark_transfers_with_flag(ftp.selected_transfers,
        WALLET_TRANSFER_DETAIL_FLAG_COLD_SIG_RESERVATION, "native alias SignedHeld", true);
      if (!m_wallet->m_unconfirmed_txs.emplace(built->transaction_hash, std::move(pending)).second) {
        mark_recovery(); return refuse(sign_status::RecoveryRequired);
      }
      currency::account_public_address expected{};
      expected.spend_public_key = m_identity.spend_public_key;
      expected.view_public_key = m_identity.view_public_key;
      expected.flags = m_identity.flags;
      if (m_wallet->render_native_checkpoint_image(expected, m_identity.stored_genesis,
          m_material->material().password, m_image_limit, built->checkpoint_image) !=
          wallet2::native_image_status::Rendered) {
        mark_recovery(); return refuse(sign_status::RecoveryRequired);
      }
      if (stop.load() || !admission.current() || !authorized()) { mark_recovery(); return refuse(sign_status::Revoked); }
      // The existing lease protocol: Prepared is the first durable publication
      // (actual hash, signed bytes, post-reservation image), then replace, then
      // compact Held. Provider entry can leave durable/uncertain evidence.
      publication_boundary_entered = true;
      const auto prepared = alias_checked(m_material->prepare_signed_payment(*built));
      if (prepared != commit_status::Committed) {
        if (prepared == commit_status::Unknown) m_state = session_state::OutcomeUnknown;
        else mark_recovery();
        return refuse(prepared == commit_status::Unknown ? sign_status::OutcomeUnknown : sign_status::RecoveryRequired);
      }
      const auto replaced = alias_checked(m_material->replace_signed_checkpoint(*built));
      if (replaced != commit_status::Committed) {
        if (replaced == commit_status::Unknown) m_state = session_state::OutcomeUnknown;
        else mark_recovery();
        return refuse(replaced == commit_status::Unknown ? sign_status::OutcomeUnknown : sign_status::RecoveryRequired);
      }
      const auto completed = alias_checked(m_material->complete_signed_payment(*built));
      if (completed != commit_status::Committed) {
        if (completed == commit_status::Unknown) m_state = session_state::OutcomeUnknown;
        else mark_recovery();
        return refuse(completed == commit_status::Unknown ? sign_status::OutcomeUnknown : sign_status::RecoveryRequired);
      }
      m_wallet->m_read_only_sync_dirty = false;
      m_state = session_state::OpenUnchanged;
      if (!admission.current() || !authorized()) return refuse(sign_status::Revoked);
      out.status = sign_status::SignedHeld;
      out.transaction_hash = built->transaction_hash;
      return out;
    }
    catch (...)
    {
      if (!construction_entered && !publication_boundary_entered) return refuse(sign_status::PreparationFailed);
      if (m_wallet) mark_recovery();
      return refuse(sign_status::RecoveryRequired);
    }
  }
} }
