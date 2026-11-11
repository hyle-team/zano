// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#include "offline_wallet_session.h"
#include "currency_core/currency_config.h"
#include "wallet2.h"
#include <new>
#include <utility>

namespace tools { namespace offline
{
  storage_lease::storage_lease(std::shared_ptr<fixture_control> control) noexcept
    : m_control(std::move(control)) {}
  storage_lease::~storage_lease() noexcept
  {
    if (m_control)
    {
      ++m_control->lease_releases;
      m_control->lease_release_sequence = ++m_control->sequence;
    }
  }

  prepared_wallet_material::prepared_wallet_material() noexcept = default;
  prepared_wallet_material::~prepared_wallet_material() noexcept = default;
  prepared_wallet_material::prepared_wallet_material(prepared_wallet_material&&) noexcept = default;
  prepared_wallet_material& prepared_wallet_material::operator=(prepared_wallet_material&&) noexcept = default;

  offline_wallet_session::offline_wallet_session(std::shared_ptr<fixture_control> control) noexcept
    : m_control(std::move(control)) {}

  void offline_wallet_session::destroy_owned() noexcept
  {
    // The guard must outlive member teardown, and the lease must outlive the
    // wallet. A destructor-local guard with automatic member teardown would not.
    epee::log_space::scoped_log_silence quiet;
    m_wallet.reset();
    m_material.reset();
  }

  offline_wallet_session::~offline_wallet_session() noexcept
  {
    if (m_material && m_control) ++m_control->abandonments;
    destroy_owned();
  }

  info_result offline_wallet_session::get_info() const noexcept
  {
    if (m_state != session_state::OpenUnchanged || !m_wallet || !m_material)
      return {};
    return {info_status::Available, m_info};
  }

  release_result offline_wallet_session::release_unchanged() noexcept
  {
    if (m_state == session_state::Released) return release_result::AlreadyReleased;
    if (m_state != session_state::OpenUnchanged || !m_wallet || !m_material)
      return release_result::Unavailable;
    destroy_owned();
    m_info = {};
    m_state = session_state::Released;
    if (m_control) ++m_control->successful_releases;
    return release_result::ReleasedUnchanged;
  }

  session_state offline_wallet_session::state() const noexcept { return m_state; }

  open_result open_prepared(prepared_wallet_material& material,
    std::unique_ptr<offline_wallet_session>& out) noexcept
  {
    if (out) return open_result::OccupiedOutput;
    if (!material.m_payload || !material.m_payload->lease || material.m_payload->path.empty())
      return open_result::InvalidMaterial;
#if defined(TESTNET) || CURRENCY_FORMATION_VERSION != 84
    return open_result::RecoveryRequired;
#else
    epee::log_space::scoped_log_silence quiet;
    try
    {
      auto& input = *material.m_payload;
      if (input.control) input.control->reach(fixture_control::checkpoint::BeforeAllocation);
      std::unique_ptr<offline_wallet_session> candidate(new offline_wallet_session(input.control));
      candidate->m_wallet.reset(new wallet2(wallet2::offline_construction_tag{}, input.control));
      candidate->m_wallet->load_offline_prepared(input.path, input.password);

      const auto& address = candidate->m_wallet->m_account.get_public_address();
      if (address.spend_public_key != input.expected.spend_public_key ||
          address.view_public_key != input.expected.view_public_key ||
          address.flags != input.expected.flags)
        return open_result::IdentityMismatch;
      const auto genesis = candidate->m_wallet->m_chain.get_genesis();
      if (genesis != input.expected.stored_genesis) return open_result::GenesisMismatch;
      if (input.control) input.control->reach(fixture_control::checkpoint::AfterValidation);
      candidate->m_info = {address.spend_public_key, address.view_public_key, address.flags,
        genesis, connectivity::Offline, synchronization::NoSynchronizationEvidence};
      if (input.control) input.control->reach(fixture_control::checkpoint::BeforeCommit);
      candidate->m_state = session_state::OpenUnchanged;
      // Publication is the final, nonthrowing ownership move. No registration,
      // serialization, callback or other initialization follows it.
      candidate->m_material = std::move(material.m_payload);
      out = std::move(candidate);
      return open_result::Opened;
    }
    catch (open_result result) { return result; }
    catch (const error::invalid_password&) { return open_result::WrongPassword; }
    catch (const std::bad_alloc&) { return open_result::Unavailable; }
    catch (...) { return open_result::Unavailable; }
#endif
  }
}}
