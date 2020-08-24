#!/bin/bash
set -e
set -x


# Note: if using MDBX engine uncomment corresponding lines below


cd /etc/munin/plugins/

rm aliases
rm alt_blocks_count
rm block_size
rm blockchain_lmdb_data_file_size
#rm blockchain_mdbx_data_file_size
rm db_map_size
rm db_transactions_count
rm emission
rm grey_peerlist_size
rm hashrate
rm height
rm incoming_connections_count
rm market
rm outgoing_connections_count
rm outs_stat
rm performance_block
rm performance_pool
rm performance_transaction
rm performance_transaction_inp
rm performance_transaction_inp_loop
rm performance_transaction_inp_loop_scan_loop
rm poolstate_data_file_size
rm pos_block_ts_shift_vs_actual
rm pos_dif_to_total_coins
rm pos_difficulty
rm pow_difficulty
rm reward
rm seconds_per_blocks
rm sequence_factor
rm timestamps
rm tx_count
rm tx_daily_count
rm tx_daily_volume
rm tx_per_block
rm tx_pool_size
rm white_peerlist_size


ln -s /home/project/zano_for_munin/utils/munin_plugins/aliases
ln -s /home/project/zano_for_munin/utils/munin_plugins/alt_blocks_count
ln -s /home/project/zano_for_munin/utils/munin_plugins/block_size
ln -s /home/project/zano_for_munin/utils/munin_plugins/blockchain_lmdb_data_file_size
#ln -s /home/project/zano_for_munin/utils/munin_plugins/blockchain_mdbx_data_file_size
ln -s /home/project/zano_for_munin/utils/munin_plugins/db_map_size
ln -s /home/project/zano_for_munin/utils/munin_plugins/db_transactions_count
ln -s /home/project/zano_for_munin/utils/munin_plugins/emission
ln -s /home/project/zano_for_munin/utils/munin_plugins/grey_peerlist_size
ln -s /home/project/zano_for_munin/utils/munin_plugins/hashrate
ln -s /home/project/zano_for_munin/utils/munin_plugins/height
ln -s /home/project/zano_for_munin/utils/munin_plugins/incoming_connections_count
ln -s /home/project/zano_for_munin/utils/munin_plugins/market
ln -s /home/project/zano_for_munin/utils/munin_plugins/outgoing_connections_count
ln -s /home/project/zano_for_munin/utils/munin_plugins/outs_stat
ln -s /home/project/zano_for_munin/utils/munin_plugins/performance_block
ln -s /home/project/zano_for_munin/utils/munin_plugins/performance_pool
ln -s /home/project/zano_for_munin/utils/munin_plugins/performance_transaction
ln -s /home/project/zano_for_munin/utils/munin_plugins/performance_transaction_inp
ln -s /home/project/zano_for_munin/utils/munin_plugins/performance_transaction_inp_loop
ln -s /home/project/zano_for_munin/utils/munin_plugins/performance_transaction_inp_loop_scan_loop
ln -s /home/project/zano_for_munin/utils/munin_plugins/poolstate_data_file_size
ln -s /home/project/zano_for_munin/utils/munin_plugins/pos_block_ts_shift_vs_actual
ln -s /home/project/zano_for_munin/utils/munin_plugins/pos_dif_to_total_coins
ln -s /home/project/zano_for_munin/utils/munin_plugins/pos_difficulty
ln -s /home/project/zano_for_munin/utils/munin_plugins/pow_difficulty
ln -s /home/project/zano_for_munin/utils/munin_plugins/reward
ln -s /home/project/zano_for_munin/utils/munin_plugins/seconds_per_blocks
ln -s /home/project/zano_for_munin/utils/munin_plugins/sequence_factor
ln -s /home/project/zano_for_munin/utils/munin_plugins/timestamps
ln -s /home/project/zano_for_munin/utils/munin_plugins/tx_count
ln -s /home/project/zano_for_munin/utils/munin_plugins/tx_daily_count
ln -s /home/project/zano_for_munin/utils/munin_plugins/tx_daily_volume
ln -s /home/project/zano_for_munin/utils/munin_plugins/tx_per_block
ln -s /home/project/zano_for_munin/utils/munin_plugins/tx_pool_size
ln -s /home/project/zano_for_munin/utils/munin_plugins/white_peerlist_size
