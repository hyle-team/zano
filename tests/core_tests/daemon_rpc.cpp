// Copyright (c) 2025 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "daemon_rpc.h"

fill_tx_rpc_inputs::fill_tx_rpc_inputs()
{
  REGISTER_CALLBACK_METHOD(fill_tx_rpc_inputs, c1);
  REGISTER_CALLBACK_METHOD(fill_tx_rpc_inputs, c2);
  REGISTER_CALLBACK_METHOD(fill_tx_rpc_inputs, c3);
  REGISTER_CALLBACK_METHOD(fill_tx_rpc_inputs, c4);
  REGISTER_CALLBACK_METHOD(fill_tx_rpc_inputs, c5);
  REGISTER_CALLBACK_METHOD(fill_tx_rpc_inputs, c6);
  REGISTER_CALLBACK_METHOD(fill_tx_rpc_inputs, c7);
  REGISTER_CALLBACK_METHOD(fill_tx_rpc_inputs, c8);
}

bool fill_tx_rpc_inputs::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure that the function "blockchain_storage::fill_tx_rpc_inputs" works as expected.

  GENERATE_ACCOUNT(miner);
  MAKE_GENESIS_BLOCK(events, blk_0, miner, test_core_time::get_time());

  DO_CALLBACK(events, "configure_core");

  // A transaction in the default state.
  DO_CALLBACK_PARAMS_STR(events, "c1", t_serializable_object_to_blob(currency::transaction{}));

  // A transaction with an input of the type "txin_to_key" in the default state.
  {
    currency::transaction tx{};

    tx.vin.push_back(std::move(currency::txin_to_key{}));
    DO_CALLBACK_PARAMS_STR(events, "c2", t_serializable_object_to_blob(tx));
  }

  // A transaction with an input of the type "txin_zc_input".
  {
    currency::transaction tx{};

    {
      currency::txin_zc_input input{};

      input.k_image = {};
      tx.vin.push_back(std::move(input));
    }

    DO_CALLBACK_PARAMS_STR(events, "c3", t_serializable_object_to_blob(tx));
  }

  // A transaction with several "txin_to_key" inputs those have different .amount values.
  {
    currency::transaction tx{};

    tx.vin.reserve(3);

    {
      currency::txin_to_key input{};

      CHECK_AND_ASSERT_EQ(input.amount, 0);
      tx.vin.push_back(std::move(input));
    }

    {
      currency::txin_to_key input{};

      input.amount = UINT64_MAX;
      tx.vin.push_back(std::move(input));
    }

    {
      currency::txin_to_key input{};

      input.amount = 16730018105294876523ull;
      tx.vin.push_back(std::move(input));
    }

    DO_CALLBACK_PARAMS_STR(events, "c4", t_serializable_object_to_blob(tx));
  }

  // A transaction with inputs of all possible types.
  {
    currency::transaction tx{};

    tx.vin.reserve(5);
    tx.vin.push_back(std::move(currency::txin_gen{}));
    tx.vin.push_back(std::move(currency::txin_to_key{}));
    tx.vin.push_back(std::move(currency::txin_htlc{}));

    {
      currency::txin_zc_input input{};

      input.k_image = {};
      tx.vin.push_back(std::move(input));
    }

    tx.vin.push_back(std::move(currency::txin_multisig{}));
    
  LOG_PRINT_GREEN("---------> tx_0 miner -> alice: " << obj_to_json_str(tx), LOG_LEVEL_0);
    DO_CALLBACK_PARAMS_STR(events, "c5", t_serializable_object_to_blob(tx));
  }

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  MAKE_TX_FEE(events, tx_0, miner, miner, MK_TEST_COINS(2), TESTS_DEFAULT_FEE, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner, tx_0);

  // A transaction with inputs of all possible types.
  {
    currency::transaction tx{};

    tx.vin.reserve(5);
    tx.vin.push_back(std::move(currency::txin_gen{/* .height = */ 7137406440025745250}));

    {
      currency::txin_to_key input{};

      input.key_offsets.push_back(5350230927837587142ull);
      input.key_offsets.push_back(std::move(currency::ref_by_id{currency::get_transaction_hash(tx_0), 0u}));
      input.amount = 2341818593703234797ull;
      input.k_image = crypto::point_t{{0x62, 0xd9, 0xa0, 0xff, 0xb1, 0x89, 0x06, 0xbe, 0xbd, 0xe9, 0xce, 0xd5, 0xc2, 0x04, 0x5b, 0x6b, 0xb4, 0x5f, 0x9c, 0x3b, 0x31, 0xcc, 0x72, 0xc7, 0x55, 0x25, 0x0f,
        0xa2, 0xb2, 0xbf, 0xb1, 0x0c}}.to_key_image();
      input.etc_details.push_back(std::move(currency::signed_parts{/* .n_outs = */ 2613407258u, /* .n_extras = */ 347754399u}));
      input.etc_details.push_back(std::move(currency::extra_attachment_info{/* .sz = */ 5559118977069213037ull, /* .hsh = */ currency::null_hash, /* cnt = */ 8106306627691316520ull}));
      tx.vin.push_back(std::move(input));
    }

    {
      currency::txin_htlc input{};

      input.key_offsets.push_back(9536097715806449708ull);
      input.key_offsets.push_back(std::move(currency::ref_by_id{/* .tx_id = */ currency::get_transaction_hash(tx_0), /* .n = */ 9u}));
      input.amount = 11357119244607763967ull;
      input.k_image = crypto::point_t{{0xcb, 0xec, 0xfb, 0x36, 0x02, 0x1c, 0xe5, 0x64, 0xee, 0x6a, 0xb8, 0x67, 0xb2, 0x8e, 0xe9, 0xef, 0x80, 0x17, 0x34, 0x6f, 0xa8, 0x67, 0x3e, 0x45, 0x3a, 0xe0, 0xd4,
        0x8b, 0x1a, 0x13, 0x75, 0xe2}}.to_key_image();
      input.etc_details.push_back(std::move(currency::signed_parts{/* .n_outs = */ 517318753u, /* .n_extras = */ 1367922888u}));
      input.etc_details.push_back(std::move(currency::extra_attachment_info{/* .sz = */ 10987762797757012676ull, /* .hsh = */ currency::null_hash, /* .cnt = */ 10767056422067827733ull}));
      input.hltc_origin = "htlc-origin";
      tx.vin.push_back(std::move(input));
    }

    {
      currency::txin_zc_input input{};

      input.key_offsets.push_back(16540286509618649069ull);
      input.key_offsets.push_back(std::move(currency::ref_by_id{/* .tx_id = */ currency::get_transaction_hash(tx_0), /* .n = */ 4}));
      input.k_image = crypto::point_t{{0x53, 0xcf, 0xaf, 0x48, 0x4d, 0xf8, 0xfb, 0x09, 0x4c, 0x01, 0x59, 0x9f, 0xe2, 0x2d, 0x3c, 0x23, 0x96, 0x2d, 0x8c, 0x24, 0x09, 0xf9, 0xd3, 0xe6, 0xf3, 0x27, 0xe8,
        0x7c, 0x7a, 0x90, 0x9c, 0xab}}.to_key_image();
      input.etc_details.push_back(std::move(currency::signed_parts{/* .n_outs = */ 517318753u, /* .n_extras = */ 1367922888u}));
      input.etc_details.push_back(std::move(currency::extra_attachment_info{/* .sz = */ 10987762797757012676ull, /* .hsh = */ currency::null_hash, /* .cnt = */ 10767056422067827733ull}));
      tx.vin.push_back(std::move(input));
    }

    {
      currency::txin_multisig input{};

      input.amount = 14073369620052150183ull;
      input.multisig_out_id = crypto::cn_fast_hash("multisig-out-id", 15);
      input.sigs_count = 3497547654u;
      input.etc_details.push_back(std::move(currency::signed_parts{/* .n_outs = */ 1801772931u, /* .n_extras = */ 167800219u}));
      input.etc_details.push_back(std::move(currency::extra_attachment_info{/* .sz = */ 12438971857615319230ull, /* .hsh = */ currency::null_hash, /* .cnt = */ 13515222808659969031ull}));
      tx.vin.push_back(std::move(input));
    }

    DO_CALLBACK_PARAMS_STR(events, "c6", t_serializable_object_to_blob(tx));
  }

  /* A wrong reference by an object of the type "ref_by_id": a value of the attribute ".n" representing an offset is greater than a length of a container of outputs.The function "fill_tx_rpc_inputs"
  returns false. */
  {
    currency::transaction tx{};

    {
      currency::txin_to_key input{};

      CHECK_AND_ASSERT_GREATER(1968482779, tx_0.vout.size() - 1);
      input.key_offsets.push_back(std::move(currency::ref_by_id{currency::get_transaction_hash(tx_0), 1968482779u}));
      tx.vin.push_back(std::move(input));
    }

    DO_CALLBACK_PARAMS_STR(events, "c7", t_serializable_object_to_blob(tx));
  }

  // A wrong reference by an object of the type "ref_by_id": hashcode of non-existent transaction is specified. The function "fill_tx_rpc_inputs" returns false.
  {
    currency::transaction tx{};

    {
      currency::txin_to_key input{};

      input.key_offsets.push_back(std::move(currency::ref_by_id{currency::null_hash, 0u}));
      tx.vin.push_back(std::move(input));
    }

    DO_CALLBACK_PARAMS_STR(events, "c8", t_serializable_object_to_blob(tx));
  }

  return true;
}

bool fill_tx_rpc_inputs::c1(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const
{
  currency::transaction tx{};
  currency::tx_rpc_extended_info info{};

  CHECK_AND_ASSERT_EQ(t_unserializable_object_from_blob(tx, boost::get<const callback_entry>(events.at(event_position)).callback_params), true);
  CHECK_AND_ASSERT_EQ(core.get_blockchain_storage().fill_tx_rpc_inputs(info, tx), true);
  CHECK_AND_ASSERT_EQ(info.blob.empty(), true);
  CHECK_AND_ASSERT_EQ(info.blob_size, 0);
  CHECK_AND_ASSERT_EQ(info.fee, 0);
  CHECK_AND_ASSERT_EQ(info.amount, 0);
  CHECK_AND_ASSERT_EQ(info.timestamp, 0);
  CHECK_AND_ASSERT_EQ(info.keeper_block, 0);
  CHECK_AND_ASSERT_EQ(info.id.empty(), true);
  CHECK_AND_ASSERT_EQ(info.pub_key.empty(), true);
  CHECK_AND_ASSERT_EQ(info.outs.empty(), true);
  CHECK_AND_ASSERT_EQ(info.ins.empty(), true);
  CHECK_AND_ASSERT_EQ(info.id.empty(), true);
  CHECK_AND_ASSERT_EQ(info.extra.empty(), true);
  CHECK_AND_ASSERT_EQ(info.attachments.empty(), true);
  CHECK_AND_ASSERT_EQ(info.object_in_json.empty(), true);

  return true;
}

bool fill_tx_rpc_inputs::c2(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const
{
  currency::transaction tx{};
  currency::tx_rpc_extended_info info{};

  CHECK_AND_ASSERT_EQ(t_unserializable_object_from_blob(tx, boost::get<const callback_entry>(events.at(event_position)).callback_params), true);
  CHECK_AND_ASSERT_EQ(core.get_blockchain_storage().fill_tx_rpc_inputs(info, tx), true);
  CHECK_AND_ASSERT_EQ(info.blob.empty(), true);
  CHECK_AND_ASSERT_EQ(info.blob_size, 0);
  CHECK_AND_ASSERT_EQ(info.fee, 0);
  CHECK_AND_ASSERT_EQ(info.amount, 0);
  CHECK_AND_ASSERT_EQ(info.timestamp, 0);
  CHECK_AND_ASSERT_EQ(info.keeper_block, 0);
  CHECK_AND_ASSERT_EQ(info.id.empty(), true);
  CHECK_AND_ASSERT_EQ(info.pub_key.empty(), true);
  CHECK_AND_ASSERT_EQ(info.outs.empty(), true);

  {
    CHECK_AND_ASSERT_EQ(info.ins.size(), 1);

    {
      CHECK_AND_ASSERT_EQ(info.ins.front().amount, 0);
      CHECK_AND_ASSERT_EQ(info.ins.front().multisig_count, 0);
      CHECK_AND_ASSERT_EQ(info.ins.front().htlc_origin.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.front().kimage_or_ms_id, epee::string_tools::pod_to_hex(currency::null_ki));
      CHECK_AND_ASSERT_EQ(info.ins.front().global_indexes.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.front().etc_options.empty(), true);
    }
  }

  CHECK_AND_ASSERT_EQ(info.id.empty(), true);
  CHECK_AND_ASSERT_EQ(info.extra.empty(), true);
  CHECK_AND_ASSERT_EQ(info.attachments.empty(), true);
  CHECK_AND_ASSERT_EQ(info.object_in_json.empty(), true);

  return true;
}

bool fill_tx_rpc_inputs::c3(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const
{
  currency::transaction tx{};
  currency::tx_rpc_extended_info info{};

  CHECK_AND_ASSERT_EQ(t_unserializable_object_from_blob(tx, boost::get<const callback_entry>(events.at(event_position)).callback_params), true);
  CHECK_AND_ASSERT_EQ(core.get_blockchain_storage().fill_tx_rpc_inputs(info, tx), true);
  CHECK_AND_ASSERT_EQ(info.blob.empty(), true);
  CHECK_AND_ASSERT_EQ(info.blob_size, 0);
  CHECK_AND_ASSERT_EQ(info.fee, 0);
  CHECK_AND_ASSERT_EQ(info.amount, 0);
  CHECK_AND_ASSERT_EQ(info.timestamp, 0);
  CHECK_AND_ASSERT_EQ(info.keeper_block, 0);
  CHECK_AND_ASSERT_EQ(info.id.empty(), true);
  CHECK_AND_ASSERT_EQ(info.pub_key.empty(), true);
  CHECK_AND_ASSERT_EQ(info.outs.empty(), true);

  {
    CHECK_AND_ASSERT_EQ(info.ins.size(), 1);

    {
      CHECK_AND_ASSERT_EQ(info.ins.front().amount, 0);
      CHECK_AND_ASSERT_EQ(info.ins.front().multisig_count, 0);
      CHECK_AND_ASSERT_EQ(info.ins.front().htlc_origin.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.front().kimage_or_ms_id, epee::string_tools::pod_to_hex(currency::null_ki));
      CHECK_AND_ASSERT_EQ(info.ins.front().global_indexes.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.front().etc_options.empty(), true);
    }
  }

  CHECK_AND_ASSERT_EQ(info.id.empty(), true);
  CHECK_AND_ASSERT_EQ(info.extra.empty(), true);
  CHECK_AND_ASSERT_EQ(info.attachments.empty(), true);
  CHECK_AND_ASSERT_EQ(info.object_in_json.empty(), true);

  return true;
}

bool fill_tx_rpc_inputs::c4(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const
{
  currency::transaction tx{};
  currency::tx_rpc_extended_info info{};

  CHECK_AND_ASSERT_EQ(t_unserializable_object_from_blob(tx, boost::get<const callback_entry>(events.at(event_position)).callback_params), true);
  CHECK_AND_ASSERT_EQ(core.get_blockchain_storage().fill_tx_rpc_inputs(info, tx), true);
  CHECK_AND_ASSERT_EQ(info.blob.empty(), true);
  CHECK_AND_ASSERT_EQ(info.blob_size, 0);
  CHECK_AND_ASSERT_EQ(info.fee, 0);
  CHECK_AND_ASSERT_EQ(info.amount, 0);
  CHECK_AND_ASSERT_EQ(info.timestamp, 0);
  CHECK_AND_ASSERT_EQ(info.keeper_block, 0);
  CHECK_AND_ASSERT_EQ(info.id.empty(), true);
  CHECK_AND_ASSERT_EQ(info.pub_key.empty(), true);
  CHECK_AND_ASSERT_EQ(info.outs.empty(), true);

  {
    CHECK_AND_ASSERT_EQ(info.ins.size(), 3);

    {
      CHECK_AND_ASSERT_EQ(info.ins.front().amount, 0);
      CHECK_AND_ASSERT_EQ(info.ins.front().multisig_count, 0);
      CHECK_AND_ASSERT_EQ(info.ins.front().htlc_origin.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.front().kimage_or_ms_id, epee::string_tools::pod_to_hex(currency::null_ki));
      CHECK_AND_ASSERT_EQ(info.ins.front().global_indexes.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.front().etc_options.empty(), true);
    }

    {
      CHECK_AND_ASSERT_EQ(info.ins.at(1).amount, UINT64_MAX);
      CHECK_AND_ASSERT_EQ(info.ins.at(1).multisig_count, 0);
      CHECK_AND_ASSERT_EQ(info.ins.at(1).htlc_origin.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.at(1).kimage_or_ms_id, epee::string_tools::pod_to_hex(currency::null_ki));
      CHECK_AND_ASSERT_EQ(info.ins.at(1).global_indexes.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.at(1).etc_options.empty(), true);
    }

    {
      CHECK_AND_ASSERT_EQ(info.ins.back().amount, 16730018105294876523ull);
      CHECK_AND_ASSERT_EQ(info.ins.back().multisig_count, 0);
      CHECK_AND_ASSERT_EQ(info.ins.back().htlc_origin.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.back().kimage_or_ms_id, epee::string_tools::pod_to_hex(currency::null_ki));
      CHECK_AND_ASSERT_EQ(info.ins.back().global_indexes.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.back().etc_options.empty(), true);
    }
  }

  CHECK_AND_ASSERT_EQ(info.id.empty(), true);
  CHECK_AND_ASSERT_EQ(info.extra.empty(), true);
  CHECK_AND_ASSERT_EQ(info.attachments.empty(), true);
  CHECK_AND_ASSERT_EQ(info.object_in_json.empty(), true);

  return true;
}

bool fill_tx_rpc_inputs::c5(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const
{
  currency::transaction tx{};
  currency::tx_rpc_extended_info info{};

  CHECK_AND_ASSERT_EQ(t_unserializable_object_from_blob(tx, boost::get<const callback_entry>(events.at(event_position)).callback_params), true);
  CHECK_AND_ASSERT_EQ(core.get_blockchain_storage().fill_tx_rpc_inputs(info, tx), true);
  CHECK_AND_ASSERT_EQ(info.blob.empty(), true);
  CHECK_AND_ASSERT_EQ(info.blob_size, 0);
  CHECK_AND_ASSERT_EQ(info.fee, 0);
  CHECK_AND_ASSERT_EQ(info.amount, 0);
  CHECK_AND_ASSERT_EQ(info.timestamp, 0);
  CHECK_AND_ASSERT_EQ(info.keeper_block, 0);
  CHECK_AND_ASSERT_EQ(info.id.empty(), true);
  CHECK_AND_ASSERT_EQ(info.pub_key.empty(), true);
  CHECK_AND_ASSERT_EQ(info.outs.empty(), true);

  {
    CHECK_AND_ASSERT_EQ(info.ins.size(), 5);

    for (size_t position{}; position < info.ins.size(); ++position)
    {
      CHECK_AND_ASSERT_EQ(info.ins.at(position).amount, 0);
      CHECK_AND_ASSERT_EQ(info.ins.at(position).multisig_count, 0);
      CHECK_AND_ASSERT_EQ(info.ins.at(position).htlc_origin.empty(), true);

      if (position == 0)
      {
        CHECK_AND_ASSERT_EQ(info.ins.at(position).kimage_or_ms_id.empty(), true);
      }

      else
      {
        CHECK_AND_ASSERT_EQ(info.ins.at(position).kimage_or_ms_id, epee::string_tools::pod_to_hex(currency::null_ki));
      }

      CHECK_AND_ASSERT_EQ(info.ins.at(position).global_indexes.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.at(position).etc_options.empty(), true);
    }
  }

  CHECK_AND_ASSERT_EQ(info.id.empty(), true);
  CHECK_AND_ASSERT_EQ(info.extra.empty(), true);
  CHECK_AND_ASSERT_EQ(info.attachments.empty(), true);
  CHECK_AND_ASSERT_EQ(info.object_in_json.empty(), true);

  return true;
}

bool fill_tx_rpc_inputs::c6(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const
{
  currency::transaction tx{};
  currency::tx_rpc_extended_info info{};

  CHECK_AND_ASSERT_EQ(t_unserializable_object_from_blob(tx, boost::get<const callback_entry>(events.at(event_position)).callback_params), true);
  CHECK_AND_ASSERT_EQ(core.get_blockchain_storage().fill_tx_rpc_inputs(info, tx), true);
  CHECK_AND_ASSERT_EQ(info.blob.empty(), true);
  CHECK_AND_ASSERT_EQ(info.blob_size, 0);
  CHECK_AND_ASSERT_EQ(info.fee, 0);
  CHECK_AND_ASSERT_EQ(info.amount, 0);
  CHECK_AND_ASSERT_EQ(info.timestamp, 0);
  CHECK_AND_ASSERT_EQ(info.keeper_block, 0);
  CHECK_AND_ASSERT_EQ(info.id.empty(), true);
  CHECK_AND_ASSERT_EQ(info.pub_key.empty(), true);
  CHECK_AND_ASSERT_EQ(info.outs.empty(), true);

  {
    CHECK_AND_ASSERT_EQ(info.ins.size(), 5);

    {
      CHECK_AND_ASSERT_EQ(info.ins.front().amount, 0ull);
      CHECK_AND_ASSERT_EQ(info.ins.front().multisig_count, 0ull);
      CHECK_AND_ASSERT_EQ(info.ins.front().htlc_origin.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.front().kimage_or_ms_id.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.front().global_indexes.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.front().etc_options.empty(), true);
    }

    {
      CHECK_AND_ASSERT_EQ(info.ins.at(1).amount, 2341818593703234797ull);
      CHECK_AND_ASSERT_EQ(info.ins.at(1).multisig_count, 0ull);
      CHECK_AND_ASSERT_EQ(info.ins.at(1).htlc_origin.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.at(1).kimage_or_ms_id, "8172e80b8da3bcbce2ee7df42466627bb3559b80bb504f1fd56b460eedbc2ce9");
      CHECK_AND_ASSERT_EQ(info.ins.at(1).global_indexes.size(), 2ull);

      {
        CHECK_AND_ASSERT_EQ(info.ins.at(1).global_indexes.front(), 5350230927837587142ull);
        CHECK_AND_ASSERT_EQ(info.ins.at(1).global_indexes.back(), 0ull);
      }

      CHECK_AND_ASSERT_EQ(info.ins.at(1).etc_options.size(), 2ull);

      {
        CHECK_AND_ASSERT_EQ(info.ins.at(1).etc_options.front(), "n_outs: 2613407258, n_extras: 347754399");
        CHECK_AND_ASSERT_EQ(info.ins.at(1).etc_options.back(), "cnt: 8106306627691316520, sz: 5559118977069213037, hsh: " + std::string(64, '0'));
      }
    }

    {
      CHECK_AND_ASSERT_EQ(info.ins.at(2).amount, 11357119244607763967ull);
      CHECK_AND_ASSERT_EQ(info.ins.at(2).multisig_count, 0ull);
      CHECK_AND_ASSERT_EQ(info.ins.at(2).htlc_origin, epee::string_tools::buff_to_hex_nodelimer(std::string{"htlc-origin"}));
      CHECK_AND_ASSERT_EQ(info.ins.at(2).kimage_or_ms_id, "f15201980333d6ca8fda90c73814baea0864eb56597e40c38061ec77644585ea");
      CHECK_AND_ASSERT_EQ(info.ins.at(2).global_indexes.size(), 2ull);

      {
        CHECK_AND_ASSERT_EQ(info.ins.at(2).global_indexes.front(), 9536097715806449708ull);
        CHECK_AND_ASSERT_EQ(info.ins.at(2).global_indexes.back(), 0ull);
      }

      CHECK_AND_ASSERT_EQ(info.ins.at(2).etc_options.size(), 2ull);

      {
        CHECK_AND_ASSERT_EQ(info.ins.at(2).etc_options.front(), "n_outs: 517318753, n_extras: 1367922888");
        CHECK_AND_ASSERT_EQ(info.ins.at(2).etc_options.back(), "cnt: 10767056422067827733, sz: 10987762797757012676, hsh: " + std::string(64, '0'));
      }
    }

    {
      CHECK_AND_ASSERT_EQ(info.ins.at(3).amount, 0ull);
      CHECK_AND_ASSERT_EQ(info.ins.at(3).multisig_count, 0ull);
      CHECK_AND_ASSERT_EQ(info.ins.at(3).htlc_origin.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.at(3).kimage_or_ms_id, "c08b36a7f77185f31a570a7f51aa550122026985e5de7941218510a1e973202e");
      CHECK_AND_ASSERT_EQ(info.ins.at(3).global_indexes.size(), 2ull);

      {
        CHECK_AND_ASSERT_EQ(info.ins.at(3).global_indexes.front(), 16540286509618649069ull);
        CHECK_AND_ASSERT_EQ(info.ins.at(3).global_indexes.back(), 0ull);
      }

      CHECK_AND_ASSERT_EQ(info.ins.at(3).etc_options.size(), 2ull);

      {
        CHECK_AND_ASSERT_EQ(info.ins.at(3).etc_options.front(), "n_outs: 517318753, n_extras: 1367922888");
        CHECK_AND_ASSERT_EQ(info.ins.at(3).etc_options.back(), "cnt: 10767056422067827733, sz: 10987762797757012676, hsh: " + std::string(64, '0'));
      }
    }

    {
      CHECK_AND_ASSERT_EQ(info.ins.at(4).amount, 14073369620052150183ull);
      CHECK_AND_ASSERT_EQ(info.ins.at(4).multisig_count, 0ull);
      CHECK_AND_ASSERT_EQ(info.ins.at(4).htlc_origin.empty(), true);
      CHECK_AND_ASSERT_EQ(info.ins.at(4).kimage_or_ms_id, "aacdff6018af0aae84d7a836a7f0b4309b51a28bfc4f566657c67b903a3ccba5");
      CHECK_AND_ASSERT_EQ(info.ins.at(4).global_indexes.size(), 0ull);
      CHECK_AND_ASSERT_EQ(info.ins.at(4).etc_options.size(), 2ull);

      {
        CHECK_AND_ASSERT_EQ(info.ins.at(4).etc_options.front(), "n_outs: 1801772931, n_extras: 167800219");
        CHECK_AND_ASSERT_EQ(info.ins.at(4).etc_options.back(), "cnt: 13515222808659969031, sz: 12438971857615319230, hsh: " + std::string(64, '0'));
      }
    }
  }

  CHECK_AND_ASSERT_EQ(info.id.empty(), true);
  CHECK_AND_ASSERT_EQ(info.extra.empty(), true);
  CHECK_AND_ASSERT_EQ(info.attachments.empty(), true);
  CHECK_AND_ASSERT_EQ(info.object_in_json.empty(), true);

  return true;
}

bool fill_tx_rpc_inputs::c7(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const
{
  currency::transaction tx{};
  currency::tx_rpc_extended_info info{};

  CHECK_AND_ASSERT_EQ(t_unserializable_object_from_blob(tx, boost::get<const callback_entry>(events.at(event_position)).callback_params), true);
  CHECK_AND_ASSERT_EQ(tx.vin.size(), 1);

  {
    const auto& input{boost::get<currency::txin_to_key>(tx.vin.front())};

    CHECK_AND_ASSERT_EQ(input.key_offsets.size(), 1);

    {
      const auto& reference{boost::get<currency::ref_by_id>(input.key_offsets.front())};

      CHECK_AND_ASSERT_EQ(reference.n, 1968482779u);

      {
        const auto pointer_entry{core.get_blockchain_storage().get_tx_chain_entry(reference.tx_id)};

        CHECK_AND_ASSERT(pointer_entry, false);
        CHECK_AND_ASSERT(reference.n >= pointer_entry->m_global_output_indexes.size(), false);
      }
    }
  }

  CHECK_AND_ASSERT_EQ(core.get_blockchain_storage().fill_tx_rpc_inputs(info, tx), false);

  return true;
}

bool fill_tx_rpc_inputs::c8(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const
{
  currency::transaction tx{};
  currency::tx_rpc_extended_info info{};

  CHECK_AND_ASSERT_EQ(t_unserializable_object_from_blob(tx, boost::get<const callback_entry>(events.at(event_position)).callback_params), true);
  CHECK_AND_ASSERT_EQ(tx.vin.size(), 1);

  {
    const auto& input{boost::get<currency::txin_to_key>(tx.vin.front())};

    CHECK_AND_ASSERT_EQ(input.key_offsets.size(), 1);

    {
      const auto& reference{boost::get<currency::ref_by_id>(input.key_offsets.front())};

      CHECK_AND_ASSERT_EQ(reference.tx_id, currency::null_hash);
      CHECK_AND_ASSERT_EQ(reference.n, 0u);

      const auto pointer_entry{core.get_blockchain_storage().get_tx_chain_entry(reference.tx_id)};

      CHECK_AND_ASSERT(pointer_entry == nullptr, false);
    }
  }

  CHECK_AND_ASSERT_EQ(core.get_blockchain_storage().fill_tx_rpc_inputs(info, tx), false);

  return true;
}
