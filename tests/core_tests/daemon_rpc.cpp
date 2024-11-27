#include "chaingen.h"
#include "daemon_rpc.h"

fill_tx_rpc_inputs::fill_tx_rpc_inputs()
{
  REGISTER_CALLBACK_METHOD(fill_tx_rpc_inputs, c1);
  REGISTER_CALLBACK_METHOD(fill_tx_rpc_inputs, c2);
  REGISTER_CALLBACK_METHOD(fill_tx_rpc_inputs, c3);
  REGISTER_CALLBACK_METHOD(fill_tx_rpc_inputs, c4);
}

bool fill_tx_rpc_inputs::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure that the function "blockchain_storage::fill_tx_rpc_inputs" works as expected.

  GENERATE_ACCOUNT(miner);
  MAKE_GENESIS_BLOCK(events, blk_0, miner, test_core_time::get_time());

  DO_CALLBACK(events, "configure_core");

  // A transaction in the default state.
  {
    currency::transaction tx{};

    DO_CALLBACK_PARAMS_STR(events, "c1", t_serializable_object_to_blob(tx));
  }

  // A transaction with an input of the type "txin_to_key" in the default state.
  {
    currency::transaction tx{};

    tx.vin.emplace_back(currency::txin_to_key{});
    DO_CALLBACK_PARAMS_STR(events, "c2", t_serializable_object_to_blob(tx));
  }

  // A transaction with several "txin_to_key" inputs those have different .amount values.
  {
    currency::transaction tx{};

    tx.vin.emplace_back(currency::txin_to_key{{}, 0});
    tx.vin.emplace_back(currency::txin_to_key{{}, UINT64_MAX});
    tx.vin.emplace_back(currency::txin_to_key{{}, 1});
    DO_CALLBACK_PARAMS_STR(events, "c3", t_serializable_object_to_blob(tx));
  }

  // A transaction with inputs of all possible types, all inputs are in the default state.
  {
    currency::transaction tx{};

    tx.vin.emplace_back(currency::txin_gen{});
    tx.vin.emplace_back(currency::txin_to_key{});
    tx.vin.emplace_back(currency::txin_htlc{});
    tx.vin.emplace_back(currency::txin_zc_input{});
    tx.vin.emplace_back(currency::txin_multisig{});
    DO_CALLBACK_PARAMS_STR(events, "c4", t_serializable_object_to_blob(tx));
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
      CHECK_AND_ASSERT_EQ(info.ins.back().amount, 1);
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
    CHECK_AND_ASSERT_EQ(info.ins.size(), 5);

    for (int position{}; position < info.ins.size(); ++position)
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
        if (position == 3)
        {
          CHECK_AND_ASSERT_EQ(info.ins.at(position).kimage_or_ms_id, std::string(64, 'c'));
        }

        else
        {
          CHECK_AND_ASSERT_EQ(info.ins.at(position).kimage_or_ms_id, epee::string_tools::pod_to_hex(currency::null_ki));
        }
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
