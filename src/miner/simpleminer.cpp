// Copyright (c) 2014-2017 The The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "common/command_line.h"
#include "misc_log_ex.h"
#include "simpleminer.h"
#include "target_helper.h"
#include "net/http_server_handlers_map2.h"
#include "rpc/mining_protocol_defs.h"
#include "storages/http_abstract_invoke.h"
#include "string_tools.h"
#include "currency_core/account.h"
#include "currency_core/currency_format_utils.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "currency_core/miner_common.h"
#ifndef WIN32
#include <sys/mman.h>
#endif

using namespace epee;
namespace po = boost::program_options;

int main(int argc, char** argv)
{
  string_tools::set_module_name_and_folder(argv[0]);
// 
//   //set up logging options
//   log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
//   log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
//   log_space::log_singletone::add_logger(LOGGER_FILE,
//     log_space::log_singletone::get_default_log_file().c_str(),
//     log_space::log_singletone::get_default_log_folder().c_str());
// 
//   po::options_description desc("Allowed options");
//   command_line::add_arg(desc, command_line::arg_help);
//   mining::simpleminer::init_options(desc);
// 
//   //uint32_t t = mining::get_target_for_difficulty(700000);
// 
//   po::variables_map vm;
//   bool r = command_line::handle_error_helper(desc, [&]()
//   {
//     po::store(po::parse_command_line(argc, argv, desc), vm);
//     po::notify(vm);
// 
//     if (command_line::get_arg(vm, command_line::arg_help))
//     {
//       std::cout << desc << std::endl;
//       return false;
//     }
// 
//     return true;
//   });
//   if (!r)
//     return 1;
// 
// 
//   while (1) {
//     mining::simpleminer miner;
//     r = miner.init(vm);
//     r = r && miner.run(); // Returns on too many failures
//     LOG_PRINT_L0("Excessive failures.  Sleeping 10-ish seconds and restarting...");
//     epee::misc_utils::sleep_no_w(10000 + (crypto::rand<size_t>() % 5000));
//   }

  return 0;
}
// /*
// namespace mining
// {
//   const command_line::arg_descriptor<std::string, true> arg_pool_addr = {"pool-addr", ""};
//   const command_line::arg_descriptor<std::string, true> arg_login = {"login", ""};
//   const command_line::arg_descriptor<std::string, true> arg_pass = {"pass", ""};
//   const command_line::arg_descriptor<uint32_t> arg_mining_threads = { "mining-threads", "Specify mining threads count", 1, true };
//   const command_line::arg_descriptor<std::string, true> arg_scratchpad_url = { "remote-scratchpad", "Specify URL to remote scratchpad"};
//   const command_line::arg_descriptor<std::string> arg_scratchpad_local = { "local-scratchpad", "Specify URL to remote scratchpad ", "", true };
// 
//   static const int attempts_per_loop = 5000;
// 
// 
//   //-----------------------------------------------------------------------------------------------------
//   void simpleminer::init_options(boost::program_options::options_description& desc)
//   {
//     command_line::add_arg(desc, arg_pool_addr);
//     command_line::add_arg(desc, arg_login);
//     command_line::add_arg(desc, arg_pass);
//     command_line::add_arg(desc, arg_mining_threads);
//     command_line::add_arg(desc, arg_scratchpad_url);
//     command_line::add_arg(desc, arg_scratchpad_local);
//   }
//   //-----------------------------------------------------------------------------------------------------
//   bool try_mkdir_chdir(const std::string& dirn)
//   {
//     boost::system::error_code ec;
//     if(boost::filesystem::exists(dirn, ec))
//     {
//       return true;
//     }
// 
//     boost::filesystem::create_directories("/some/path", ec);
//     return !ec;
//   }
//   //-----------------------------------------------------------------------------------------------------
//   std::string get_default_local_cache_path()
//   {
// #if defined(_WIN64) || defined(_WIN32)
//     const char* phome_var_name = "LOCALAPPDATA";
// #else 
//     const char* phome_var_name = "HOME";
// #endif
//     if (!getenv(phome_var_name)) 
//     {
//       LOG_ERROR("Env variable " << phome_var_name << " not set");
//       return "";
//     }
// 
//     std::string default_local_path = getenv(phome_var_name);
// #if !defined(_WIN64) && !defined(_WIN32)
//     default_local_path += "/.cache";
// #else 
//     default_local_path += "/" CURRENCY_NAME;
// #endif
//     if (!try_mkdir_chdir(default_local_path) )
//     {
//       LOG_ERROR("Failed to create a dir " << getenv(phome_var_name));
//       return "";
//     }
//     return default_local_path + "/" SCRATCHPAD_DEFAULT_FILENAME;
//   }
//   //--------------------------------------------------------------------------------------------------------------------------------
//   bool simpleminer::reset_scratchpad()
//   {
//     m_blocks_addendums.clear();
//     m_hi = AUTO_VAL_INIT(m_hi);
//     m_scratchpad.clear();
//     return true;
//   }
//   //--------------------------------------------------------------------------------------------------------------------------------
//   bool simpleminer::store_scratchpad_to_file(const std::string& path)
//   {
//     std::string buff;
//     buff.resize(sizeof(export_scratchpad_file_header) + m_scratchpad.size()*32, 0);
// 
//     uint8_t* pbuff = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(buff.data()));
//     export_scratchpad_file_header* pheader = reinterpret_cast<export_scratchpad_file_header*>(pbuff);
//     pheader->current_hi.height = m_hi.height;
//     pheader->current_hi.prevhash = m_hi.id;
//     pheader->scratchpad_size = m_scratchpad.size()*4;
//     pbuff += sizeof(export_scratchpad_file_header);
//     memcpy(pbuff, &m_scratchpad[0], m_scratchpad.size()*32);
// 
//     if(!file_io_utils::save_string_to_file(path, buff))
//     {
//       LOG_ERROR("Failed to save scratchpad local cache to " << path);
//       return false;
//     }
//     m_last_scratchpad_store_time = time(NULL);
//     return true;
//   }
//   //--------------------------------------------------------------------------------------------------------------------------------
//   bool simpleminer::load_scratchpad_from_file(const std::string& path)
//   {
//     reset_scratchpad();
//     if(!path.size())
//     {
//       LOG_ERROR("Empty scratchpad file path");
//       return false;
//     }
//     boost::system::error_code ec;
//     time_t file_time = boost::filesystem::last_write_time(path, ec);
//     if(ec)
//     {
//       LOG_PRINT_L0("Local scratchpad cache (" << path << ") not found");
//       return false;
//     }
// 
//     if(time(NULL) - file_time > LOCAL_SCRATCHPAD_CACHE_EXPIRATION_INTERVAL ) 
//     {
//       /*scratchpad older than 5 days, better to redownload*/
//       LOG_PRINT_L0("Local scratchpad cache (" << path << ") is older that 5 days, downloading new...");
//       return false;
//     }
// 
// 
//     std::string buff;
//     if(!epee::file_io_utils::load_file_to_string(path, buff))
//     {
//       LOG_PRINT_L0("Local scratchpad cache (" << path << ") not found");
//       return false;
//     }
// 
//     if(buff.size() < sizeof(export_scratchpad_file_header))
//     {
//       LOG_ERROR("Wrong scratchpad file size: " << buff.size() << " in file " << path);
//       return false;
//     }
//     
//     const uint8_t* pbuff = reinterpret_cast<const uint8_t*>(buff.data());
//     const export_scratchpad_file_header* pscr_header = reinterpret_cast<const export_scratchpad_file_header*>(pbuff);
// 
//     if(pscr_header->scratchpad_size*8 != buff.size()-sizeof(export_scratchpad_file_header) || pscr_header->scratchpad_size%4)
//     {
//       LOG_ERROR("File size and export_scratchpad_file_header mismatch, " << path);
//       return false;
//     }
// 
//     /*
//     TODO: refactor copy addendums array
//     for(int i = 0;  i != WILD_KECCAK_ADDENDUMS_ARRAY_SIZE; i++)
//     {
//       m_blocks_addendums.push_back(mining::addendum());
//       m_blocks_addendums.back().hi.block_id = pscr_header->add_arr[i].prev_hi      
//     }*/
// 
//     m_hi.height = pscr_header->current_hi.height;
//     m_hi.id = pscr_header->current_hi.prevhash;
//     m_scratchpad.resize(pscr_header->scratchpad_size/4);
//     
//     pbuff+= sizeof(export_scratchpad_file_header);
//     memcpy(&m_scratchpad[0], pbuff, buff.size()-sizeof(export_scratchpad_file_header));
//     m_last_scratchpad_store_time = file_time;
// 
//     return true;
//   }
//   //--------------------------------------------------------------------------------------------------------------------------------
//   bool simpleminer::init_scratchpad()
//   {
//     //let's try to lookup scratchpad in local cache and then, if it not there - try to fetch it from server
//     if(!load_scratchpad_from_file(m_scratchpad_local_path))
//     {
//       //failed, try to fetch it from server
//       if(!m_scratchpad_url.size())
//       {
//         LOG_ERROR("Local scratchpad chache not found, and scratchpad url not found");
//         return false;
//       }
// 
//       epee::net_utils::http::http_simple_client http_client;
//       const epee::net_utils::http::http_response_info *prespinfo = NULL;
//       LOG_PRINT_L0("Downloading remote scrathcpad from " << m_scratchpad_url << "...");
//       if(!epee::net_utils::http::invoke_request(m_scratchpad_url, http_client, 10000, &prespinfo))
//       {
//         LOG_ERROR("Local scratchpad chache not found, and scratchpad url not found");
//         return false;        
//       }
//       LOG_PRINT_L0("Remote scratchpad downloaded, size " << prespinfo->m_body.size()/1024 << "Kb");
//       if(!file_io_utils::save_string_to_file(m_scratchpad_local_path, prespinfo->m_body))
//       {
//         LOG_ERROR("Failed to store local scratchpad file to path " << m_scratchpad_local_path);
//         return false;
//       }
//      
//       //let's try to load it once again
//       if(!load_scratchpad_from_file(m_scratchpad_local_path))
//       {
//         LOG_ERROR("Local scratchpad chache not found, and scratchpad url not found");
//         return false;        
//       }
//     }
//     update_fast_scratchpad();
//     
//     LOG_PRINT_L0("Scratchpad loaded okay, hashes count: " << m_scratchpad.size());
//     return true;
//   }
//   //--------------------------------------------------------------------------------------------------------------------------------
//   bool simpleminer::init(const boost::program_options::variables_map& vm)
//   {
//     std::string pool_addr = command_line::get_arg(vm, arg_pool_addr);
//     //parse ip and address
//     std::string::size_type p = pool_addr.find(':');
//     CHECK_AND_ASSERT_MES(p != std::string::npos && (p + 1 != pool_addr.size()), false, "Wrong srv address syntax");
//     m_pool_ip = pool_addr.substr(0, p);
//     m_pool_port = pool_addr.substr(p + 1, pool_addr.size());
//     m_login = command_line::get_arg(vm, arg_login);
//     m_threads_total = 1;
//     if(command_line::has_arg(vm, arg_mining_threads))
//     {
//       m_threads_total = command_line::get_arg(vm, arg_mining_threads);
//       LOG_PRINT_L0("Mining with " << m_threads_total << " threads ");
//     }
//     m_pass = command_line::get_arg(vm, arg_pass);
//     m_hi = AUTO_VAL_INIT(m_hi);
//     m_last_job_ticks = 0;
//     m_last_scratchpad_store_time = 0;
//     m_fast_scratchpad_pages = 0;
//     m_fast_scratchpad = NULL;
// 
//     if(command_line::has_arg(vm, arg_scratchpad_url))
//     {
//       m_scratchpad_url = command_line::get_arg(vm, arg_scratchpad_url);
//     }
// 
//     if(command_line::has_arg(vm, arg_scratchpad_local))
//     {
//       m_scratchpad_local_path = command_line::get_arg(vm, arg_scratchpad_local);
//     }else
//     {
//       m_scratchpad_local_path = get_default_local_cache_path();
//     }
//     if(!init_scratchpad())
//     {
//       LOG_ERROR("Failed to init scratchpad");
//       return false;
//     }
//     return true;
//   }
//   //--------------------------------------------------------------------------------------------------------------------------------
//   bool simpleminer::text_height_info_to_native_height_info(const height_info& hi, height_info_native& hi_native)
//   {
//     hi_native.height = hi.height;
//     bool r = string_tools::hex_to_pod(hi.block_id, hi_native.id);
//     CHECK_AND_ASSERT_MES(r, false, "wrong block_id: " << hi.block_id);
//     return true;
//   }
//   //--------------------------------------------------------------------------------------------------------------------------------
//   bool simpleminer::native_height_info_to_text_height_info(height_info& hi, const height_info_native& hi_native)
//   {
//     hi.height = hi_native.height;
//     hi.block_id = string_tools::pod_to_hex(hi_native.id);
//     return true;
//   }
//   //--------------------------------------------------------------------------------------------------------------------------------
//   void simpleminer::worker_thread(uint64_t start_nonce, uint32_t nonce_offset, std::atomic<uint32_t> *result, std::atomic<bool> *do_reset, std::atomic<bool> *done) {
//     //	  printf("Worker thread starting at %lu + %u\n", start_nonce, nonce_offset);
//     currency::blobdata blob = m_job.blob;
//     while (!*do_reset) {
//       m_hashes_done += attempts_per_loop;
//       for (int i = 0; i < attempts_per_loop; i++) {
//         (*reinterpret_cast<uint64_t*>(&blob[1])) = (start_nonce+nonce_offset);
//         crypto::hash h = currency::null_hash;
//         currency::get_blob_longhash(blob, h, m_job.prev_hi.height+1, [&](uint64_t index) -> crypto::hash&
//         {
//           return m_fast_scratchpad[index%m_scratchpad.size()];
//         });
// 
//         if( currency::check_hash(h, m_job.difficulty))
//         {
//           (*result) = nonce_offset;
//           (*done) = true;
//           (*do_reset) = true;
//           m_work_done_cond.notify_one();
//           return;
//         }
//         nonce_offset++;
//       }
//       nonce_offset += ((m_threads_total-1) * attempts_per_loop);
//     }
//   }
// 
//   //--------------------------------------------------------------------------------------------------------------------------------
//   bool simpleminer::run()
//   {
//     m_job = AUTO_VAL_INIT(m_job);
//     uint64_t search_start = epee::misc_utils::get_tick_count();
//     uint64_t hashes_done = 0;
//     uint32_t job_submit_failures = 0;
//     bool re_get_scratchpad = false;
// 
//     while(true)
//     {
//       bool job_received = false;
//       //-----------------
//       if(!m_http_client.is_connected())
//       {
//         LOG_PRINT_L0("Connecting " << m_pool_ip << ":" << m_pool_port << "....");
//         if(!m_http_client.connect(m_pool_ip, m_pool_port, 20000))
//         {
//           LOG_PRINT_L0("Failed to connect " << m_pool_ip << ":" << m_pool_port << ", sleep....");
//           epee::misc_utils::sleep_no_w(1000);
//           continue;
//         }
//         m_http_client.get_socket().set_option(boost::asio::ip::tcp::no_delay(true));
//         //DO AUTH
//         LOG_PRINT_L0("Connected " << m_pool_ip << ":" << m_pool_port << " OK");
//         COMMAND_RPC_LOGIN::request req = AUTO_VAL_INIT(req);
//         req.login = m_login;
//         req.pass = m_pass;
//         req.agent = "simpleminer/0.1";
//         native_height_info_to_text_height_info(req.hi, m_hi);
//         bool job_requested = m_hi.height ? true:false;
//         COMMAND_RPC_LOGIN::response resp = AUTO_VAL_INIT(resp);
//         if(!epee::net_utils::invoke_http_json_rpc<mining::COMMAND_RPC_LOGIN>("/json_rpc", req, resp, m_http_client))
//         {
//           LOG_PRINT_L0("Failed to invoke login " << m_pool_ip << ":" << m_pool_port << ", disconnect and sleep....");
//           m_http_client.disconnect();
//           epee::misc_utils::sleep_no_w(1000);
//           continue;
//         }
//         if(resp.status != CORE_RPC_STATUS_OK || resp.id.empty())
//         {
//           LOG_PRINT_L0("Failed to login " << m_pool_ip << ":" << m_pool_port << ", disconnect and sleep....");
//           m_http_client.disconnect();
//           epee::misc_utils::sleep_no_w(1000);
//           continue;
//         }
// 
//         m_pool_session_id = resp.id;        
//         if (re_get_scratchpad || !m_hi.height || !m_scratchpad.size())
//         {
//           if (!reinit_scratchpad())
//             continue;
//           re_get_scratchpad = false;
//         }
//         else if(!apply_addendums(resp.job.addms))
//         {
//           LOG_PRINT_L0("Failed to apply_addendum, requesting full scratchpad...");
//           if (!reinit_scratchpad())
//             continue;
//         }
// 
//         if (job_requested && resp.job.blob.empty() && resp.job.difficulty.empty() && resp.job.job_id.empty())
//         {
//           LOG_PRINT_L0("Job didn't change");
//           continue;
//         }
//         else if(job_requested && !text_job_details_to_native_job_details(resp.job, m_job))
//         {
//           LOG_PRINT_L0("Failed to text_job_details_to_native_job_details(), disconnect and sleep....");
//           m_http_client.disconnect();
//           epee::misc_utils::sleep_no_w(1000);
//           continue;
//         }
//         if(job_requested)
//         {
//           m_last_job_ticks = epee::misc_utils::get_tick_count();
//           job_received = true;
//         }
// 
//       }
// 
//       if(!job_received)
//       {
//         uint64_t get_job_start_time = epee::misc_utils::get_tick_count();
//         if(!get_job()) /* Next version:  Handle this asynchronously */
//         {
//           continue;
//         }
// 
//         uint64_t get_job_end_time  = epee::misc_utils::get_tick_count();
//         if ((get_job_end_time - get_job_start_time) > 1000) 
//         {
//           LOG_PRINT_L0("slow pool response " << (get_job_end_time - get_job_start_time) << " ms");
//         }
//       }
// 
//       update_fast_scratchpad();
// 
//       uint64_t start_nonce = (*reinterpret_cast<uint64_t*>(&m_job.blob[1])) + 1000000;
//       std::list<boost::thread> threads;
//       std::atomic<uint32_t> results[128];
//       std::atomic<bool> do_reset(false);
//       std::atomic<bool> done(false);
// 
//       if (m_threads_total > 128) { 
//         LOG_PRINT_L0("Sorry - simpleminer does not support more than 128 threads right now");
//         m_threads_total = 128;
//       }
// 
//       m_hashes_done = 0; /* Used to calculate offset to push back into job */
// 
//       uint32_t nonce_offset = 0;
//       for (unsigned int i = 0; i < m_threads_total; i++)
//       {
//         results[i] = 0;
//         threads.push_back(boost::thread(&simpleminer::worker_thread, this, start_nonce, nonce_offset, &results[i], &do_reset, &done));
//         nonce_offset += attempts_per_loop;
//       }
// 
// 
//       while(!done && epee::misc_utils::get_tick_count() - m_last_job_ticks < 20000)
//       {
//         std::unique_lock<std::mutex> lck(m_work_mutex);
//         m_work_done_cond.wait_for(lck, std::chrono::seconds(1));
//       }
// 
//       do_reset = true;
//       BOOST_FOREACH(boost::thread& th, threads)
//       {
//         th.join();
//       }
//       for (unsigned int i = 0; i < m_threads_total; i++) {
//         if (results[i] != 0) {
//           (*reinterpret_cast<uint64_t*>(&m_job.blob[1])) = (start_nonce + results[i]);
//           crypto::hash h = currency::null_hash;
//           currency::get_blob_longhash(m_job.blob, h, m_job.prev_hi.height+1, [&](uint64_t index) -> crypto::hash&
//           {
//             return m_scratchpad[index%m_scratchpad.size()];
//           });
// 
//           if( currency::check_hash(h, m_job.difficulty))
//           {
//             //<< ", id" << currency::get_blob_hash(m_job.blob) << ENDL
//             //found!          
//             COMMAND_RPC_SUBMITSHARE::request submit_request = AUTO_VAL_INIT(submit_request);
//             COMMAND_RPC_SUBMITSHARE::response submit_response = AUTO_VAL_INIT(submit_response);
//             submit_request.id     = m_pool_session_id;
//             submit_request.job_id = m_job.job_id;
//             submit_request.nonce  = (*reinterpret_cast<uint64_t*>(&m_job.blob[1]));
//             submit_request.result = string_tools::buff_to_hex_nodelimer(std::string((char*) &h, HASH_SIZE));
//             LOG_PRINT_GREEN("Share found: nonce=" << submit_request.nonce << " for job=" << m_job.job_id << ", diff: " << m_job.difficulty << ENDL             
//               << ", PoW:" << h << ", height:" << m_job.prev_hi.height+1 << ", submitting...", LOG_LEVEL_0);
// 
//             //LOG_PRINT_L0("Block hashing blob: " << string_tools::buff_to_hex_nodelimer(m_job.blob));
//             //LOG_PRINT_L0("scratch_pad: " << currency::dump_scratchpad(m_scratchpad));
//             if(!epee::net_utils::invoke_http_json_rpc<mining::COMMAND_RPC_SUBMITSHARE>("/json_rpc", submit_request, submit_response, m_http_client))
//             {
//               /* Failed to submit a job.  This can happen because of disconnection,
//               * server failure, or block expiry.  In any event, try to get
//               * a new job.  If the job fetch fails, get_job will disconnect
//               * and sleep for us */
//               LOG_PRINT_L0("Failed to submit share!  Updating job.");
//               job_submit_failures++;
//             }
//             else if(submit_response.status != "OK")
//             {
//               LOG_PRINT_L0("Failed to submit share! (submitted share rejected).  Updating job.");
//               job_submit_failures++;
//             }
//             else
//             {
//               LOG_PRINT_GREEN("Share submitted successfully!", LOG_LEVEL_0);
//               job_submit_failures = 0;
//             }
//             break; /* One submission per job id */
//           } 
//           else 
//           {
//             LOG_PRINT_L0("share did not pass diff revalidation");
//           }
//         }
//       }
// 
//       start_nonce += m_hashes_done;
//       hashes_done += m_hashes_done;
//       m_hashes_done = 0;
// 
//       (*reinterpret_cast<uint64_t*>(&m_job.blob[1])) = start_nonce;
//       if (job_submit_failures == 5)
//       {
//         LOG_PRINT_L0("Too many submission failures.  Something is very wrong.");
//         free_fast_scratchpad();
//         return false;
//       }
//       if (job_submit_failures > 3)
//       {
//         re_get_scratchpad = true;
//         /* note fallthrough into next case to also reconnect */
//       }
//       if (job_submit_failures > 1)
//       {
//         m_http_client.disconnect();
//         epee::misc_utils::sleep_no_w(1000);
//       } else {
//         uint64_t loop_end_time = epee::misc_utils::get_tick_count();
//         uint64_t hash_rate = (hashes_done * 1000) / ((loop_end_time - search_start) + 1);
//         LOG_PRINT_L0("avg hr: " << hash_rate);
//       }
//     }
//     free_fast_scratchpad();
//     return true;
//   }
//   //----------------------------------------------------------------------------------------------------------------------------------
//   bool simpleminer::reinit_scratchpad()
//   {
//     init_scratchpad();
//     return true;
//   }
//   //----------------------------------------------------------------------------------------------------------------------------------
//   void simpleminer::free_fast_scratchpad()
//   {
//     if (m_fast_scratchpad) 
//     {
//       if (m_fast_mmapped)
//       {
// #ifndef WIN32
//         munmap(m_fast_scratchpad, m_fast_scratchpad_pages*4096);
// #endif
//       } else {
//         free(m_fast_scratchpad);
//       }
//       m_fast_scratchpad = NULL;
//     }
//   }
// 
//   void simpleminer::update_fast_scratchpad()
//   {
// 
//     /* Check size - reallocate fast scratch if necessary */
//     size_t cur_scratchpad_size = m_scratchpad.size() * sizeof(crypto::hash);
//     size_t cur_scratchpad_pages = (cur_scratchpad_size / 4096) + 1;
//     if (cur_scratchpad_pages > m_fast_scratchpad_pages)
//     {
//       free_fast_scratchpad();
//       void *addr = nullptr;
//       size_t mapsize = cur_scratchpad_pages * 4096;
// #ifdef MAP_HUGETLB
//       addr = MAP_FAILED;
//       addr = mmap(0, mapsize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_POPULATE, 0, 0);
//       if (addr == MAP_FAILED)
//       {
//         LOG_PRINT_L0("Unable to mmap huge pages.  Enable for faster operation.");
//         addr = nullptr;
//       }
// #endif
//       if (addr == nullptr) {
//         addr = malloc(mapsize);
//       } else {
//         m_fast_mmapped = true;
//       }
//       m_fast_scratchpad_pages = cur_scratchpad_pages;
//       m_fast_scratchpad = (crypto::hash *)addr;
//     }
// 
//     memcpy(m_fast_scratchpad, &m_scratchpad[0], m_scratchpad.size() * sizeof(crypto::hash));
//   }
// 
//   //----------------------------------------------------------------------------------------------------------------------------------
//   bool  simpleminer::pop_addendum(const addendum& add)
//   {
//     std::vector<crypto::hash> add_vec;
//     bool r = currency::hexstr_to_addendum(add.addm, add_vec);
//     CHECK_AND_ASSERT_MES(r, false, "failed to hexstr_to_addendum");
//     CHECK_AND_ASSERT_MES(m_scratchpad.size() > add_vec.size(), false, "error: to big addendum or to small local scratchpad");
// 
//     std::map<uint64_t, crypto::hash> patch;
//     currency::get_scratchpad_patch(m_scratchpad.size() - add_vec.size(),  
//       0, 
//       add_vec.size(), 
//       add_vec, 
//       patch);
//     r = currency::apply_scratchpad_patch(m_scratchpad, patch);
//     CHECK_AND_ASSERT_MES(r, false, "failed to apply patch");
//     m_scratchpad.resize(m_scratchpad.size() - add_vec.size());  
//     return true;
//   }
//   //----------------------------------------------------------------------------------------------------------------------------------
//   bool  simpleminer::push_addendum(const addendum& add)
//   {
//     std::vector<crypto::hash> add_vec;
//     bool r = currency::hexstr_to_addendum(add.addm, add_vec);
//     CHECK_AND_ASSERT_MES(r, false, "failed to hexstr_to_addendum");
// 
//     std::map<uint64_t, crypto::hash> patch;
//     currency::get_scratchpad_patch(m_scratchpad.size(),  
//       0, 
//       add_vec.size(), 
//       add_vec, 
//       patch);
//     r = currency::apply_scratchpad_patch(m_scratchpad, patch);
//     CHECK_AND_ASSERT_MES(r, false, "failed to apply patch");
//     for(const auto& h: add_vec)
//       m_scratchpad.push_back(h);
// 
//     return true;
//   }
//   //----------------------------------------------------------------------------------------------------------------------------------
//   bool simpleminer::apply_addendums(const std::list<addendum>& addms)
//   {
//     if(!addms.size())
//       return true;
//     crypto::hash pid = currency::null_hash;
//     bool r = string_tools::hex_to_pod(addms.begin()->prev_id, pid);
//     CHECK_AND_ASSERT_MES(r, false, "wrong prev_id");
//     if(pid != m_hi.id)
//     {
//       LOG_PRINT_L0("Split detected, looking for patch");
// 
//       auto it = std::find_if(m_blocks_addendums.begin(), m_blocks_addendums.end(),[&](const mining::addendum& a){
//         if(a.hi.block_id == addms.begin()->prev_id)
//           return true;
//         else
//           return false;
//       });
//       if(it == m_blocks_addendums.end())
//         return false;
//       //unpatch
//       size_t count = 0;
//       for(auto it_to_patch = --m_blocks_addendums.end(); it_to_patch != it; --it)
//       {
//         r = pop_addendum(*it_to_patch);
//         CHECK_AND_ASSERT_MES(r, false, "failed to pop_addendum()");
//         ++count;
//       }
// 
//       m_blocks_addendums.erase(++it, m_blocks_addendums.end());
//       LOG_PRINT_L0("Numbers of blocks removed from scratchpad: " << count);
//     }
// 
//     //append scratchpad with new blocks
//     for(const auto& a: addms)
//     {
//       r = push_addendum(a);
//       CHECK_AND_ASSERT_MES(r, false, "failed to push_addendum()");
//     }
//     LOG_PRINT_L0("Numbers of blocks added to scratchpad: " << addms.size());
//     text_height_info_to_native_height_info(addms.back().hi, m_hi);
//     return true;
//   }
//   //----------------------------------------------------------------------------------------------------------------------------------
//   bool simpleminer::get_job()
//   {
//     //get next job
//     COMMAND_RPC_GETJOB::request getjob_request = AUTO_VAL_INIT(getjob_request);
//     COMMAND_RPC_GETJOB::response getjob_response = AUTO_VAL_INIT(getjob_response);
//     getjob_request.id = m_pool_session_id;
//     native_height_info_to_text_height_info(getjob_request.hi, m_hi);
//     LOG_PRINT_L0("Getting next job...");
//     if(!epee::net_utils::invoke_http_json_rpc<mining::COMMAND_RPC_GETJOB>("/json_rpc", getjob_request, getjob_response, m_http_client))
//     {
//       LOG_PRINT_L0("Can't get new job! Disconnect and sleep....");
//       m_http_client.disconnect();
//       epee::misc_utils::sleep_no_w(1000);
//       return false;
//     }
//     if (getjob_response.jd.blob.empty() && getjob_response.jd.difficulty.empty() && getjob_response.jd.job_id.empty())
//     {
//       LOG_PRINT_L0("Job didn't change");
//     }
//     else if(!text_job_details_to_native_job_details(getjob_response.jd, m_job))
//     {
//       LOG_PRINT_L0("Failed to text_job_details_to_native_job_details(), disconnect and sleep....");
//       m_http_client.disconnect();
//       epee::misc_utils::sleep_no_w(1000);
//       return false;
//     }
//     //apply addendum
//     if(!apply_addendums(getjob_response.jd.addms))
//     {
//       LOG_PRINT_L0("Failed to apply_addendum, requesting full scratchpad...");
//       reinit_scratchpad();
//       return true;
//     }
// 
//     if(time(NULL) - m_last_scratchpad_store_time > LOCAL_SCRATCHPAD_CACHE_STORE_INTERVAL)
//     {
//       store_scratchpad_to_file(m_scratchpad_local_path);
//     }
// 
//     m_last_job_ticks = epee::misc_utils::get_tick_count();
// 
//     return true;
//   }
//   //----------------------------------------------------------------------------------------------------------------------------------
// }
