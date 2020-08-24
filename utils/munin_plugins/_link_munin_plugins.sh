#!/bin/bash
set -e
set -x


# Note: if using MDBX engine uncomment corresponding lines below


cd /etc/munin/plugins/

rm -f aliases
rm -f alt_blocks_count
rm -f block_size
rm -f blockchain_lmdb_data_file_size
rm -f blockchain_mdbx_data_file_size
rm -f db_map_size
rm -f db_transactions_count
rm -f emission
rm -f grey_peerlist_size
rm -f hashrate
rm -f height
rm -f incoming_connections_count
rm -f market
rm -f outgoing_connections_count
rm -f outs_stat
rm -f performance_block
rm -f performance_pool
rm -f performance_transaction
rm -f performance_transaction_inp
rm -f performance_transaction_inp_loop
rm -f performance_transaction_inp_loop_scan_loop
rm -f poolstate_lmdb_data_file_size
rm -f poolstate_mdbx_data_file_size
rm -f pos_block_ts_shift_vs_actual
rm -f pos_dif_to_total_coins
rm -f pos_difficulty
rm -f pow_difficulty
rm -f reward
rm -f seconds_per_blocks
rm -f sequence_factor
rm -f timestamps
rm -f tx_count
rm -f tx_daily_count
rm -f tx_daily_volume
rm -f tx_per_block
rm -f tx_pool_size
rm -f white_peerlist_size


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
ln -s /home/project/zano_for_munin/utils/munin_plugins/poolstate_lmdb_data_file_size
#ln -s /home/project/zano_for_munin/utils/munin_plugins/poolstate_mdbx_data_file_size
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
