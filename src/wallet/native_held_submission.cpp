// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#include "native_held_submission.h"
#include "currency_core/currency_format_utils.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "storages/portable_storage_template_helper.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <vector>
namespace tools { namespace native_submission {
exchange_context::exchange_context(const call_guard& g,const std::atomic<bool>& s)
 :guard_(g),stop_(s),deadline_(std::chrono::steady_clock::now()+exchange_budget){}
bool exchange_context::stopped() const noexcept {return stop_.load() || !guard_.current() || std::chrono::steady_clock::now()>=deadline_;}
std::chrono::milliseconds exchange_context::remaining() const noexcept {
 return stopped()?std::chrono::milliseconds{0}:std::min(guard_.remaining(),std::chrono::duration_cast<std::chrono::milliseconds>(deadline_-std::chrono::steady_clock::now()));
}
bool response_sink::append(const unsigned char* p,std::size_t n) noexcept {
 if(failed_ || context_.stopped() || (!p && n) || n>max_response_bytes-bytes_.size()){failed_=true;return false;}
 if(!n)return true;
 try{bytes_.append(reinterpret_cast<const char*>(p),n);return true;}catch(...){failed_=true;return false;}
}
namespace {
// The actual RPC responses contain only a status and two arrays of strings.
// A closed, non-recursive decoder bounds fields, values and array cardinality
// BEFORE allocation. It refuses duplicate/foreign fields or nested values.
struct reader {
 const std::string& s;std::size_t at{0};const exchange_context& context;
 void space(){while(at<s.size() && (s[at]==' '||s[at]=='\n'||s[at]=='\r'||s[at]=='\t'))++at;if(context.stopped())throw 0;}
 bool take(char c){space();if(at<s.size() && s[at]==c){++at;return true;}return false;}
 void need(char c){if(!take(c))throw 0;}
 static int digit(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
 std::string text(std::size_t limit){
  need('"');std::string v;
  while(at<s.size()){
   if(context.stopped())throw 0;
   unsigned char c=s[at++];if(c=='"')return v;
   if(c=='\\'){
    if(at>=s.size())throw 0;c=s[at++];
    if(c=='u'){unsigned value=0;for(unsigned i=0;i<4;++i){if(at>=s.size())throw 0;int d=digit(s[at++]);if(d<0)throw 0;value=value*16+d;}if(value<32 || value>126)throw 0;c=value;}
    else if(c!='"' && c!='\\' && c!='/')throw 0;
   }
   if(c<32 || c>126 || v.size()>=limit)throw 0;v.push_back(c);
  }throw 0;
 }
 std::vector<std::string> array(std::size_t limit){need('[');std::vector<std::string> v;if(take(']'))return v;v.push_back(text(limit));need(']');return v;}
};
std::string hex(const std::string& bytes){constexpr char digits[]="0123456789abcdef";std::string out;out.reserve(bytes.size()*2);for(unsigned char c:bytes){out+=digits[c>>4];out+=digits[c&15];}return out;}
std::string hash_hex(const crypto::hash& hash){return hex(std::string(reinterpret_cast<const char*>(&hash),sizeof(hash)));}
bool canonical(const std::string& bytes,const crypto::hash& expected){
 if(bytes.empty() || bytes.size()>=max_transaction_bytes || expected==currency::null_hash)return false;
 currency::transaction tx{};return currency::parse_and_validate_tx_from_blob(bytes,tx) && currency::get_transaction_hash(tx)==expected && currency::tx_to_blob(tx)==bytes;
}
struct response {std::string status;std::vector<std::string> transactions,missed;};
response decode(const std::string& bytes,bool lookup,const exchange_context& context){
 reader r{bytes,0,context};response out;unsigned seen=0;r.need('{');
 for(unsigned fields=0;;++fields){
  if(fields>=3)throw 0;const auto key=r.text(16);r.need(':');
  if(key=="status" && !(seen&1)){out.status=r.text(max_status_bytes);seen|=1;}
  else if(lookup && key=="txs_as_hex" && !(seen&2)){out.transactions=r.array(2*max_transaction_bytes);seen|=2;}
  else if(lookup && key=="missed_tx" && !(seen&4)){out.missed=r.array(64);seen|=4;}
  else throw 0;
  if(r.take('}'))break;r.need(',');
 }
 r.space();if(r.at!=bytes.size() || out.status.empty() || (!(seen&1u) || (!lookup && seen!=1u)))throw 0;return out;
}
}
struct executor {
 static result run(route method,const std::string& bytes,const crypto::hash& hash,
                   const call_guard& guard,exchange& port,const std::atomic<bool>& stop) noexcept {
  epee::log_space::scoped_log_silence quiet;
  result out;bool called=false;
#ifndef TESTNET
  out.status=outcome::NotAttempted;return out;
#endif
  try {
   exchange_context context(guard,stop);
   if(context.stopped() || !canonical(bytes,hash)){out.status=outcome::NotAttempted;return out;}
   std::string wire;
   if(method==route::Submit){currency::COMMAND_RPC_SEND_RAW_TX::request req;req.tx_as_hex=hex(bytes);if(!epee::serialization::store_t_to_json(req,wire))return out;}
   else {currency::COMMAND_RPC_GET_TRANSACTIONS::request req;req.txs_hashes.push_back(hash_hex(hash));if(!epee::serialization::store_t_to_json(req,wire))return out;}
   if(wire.empty() || wire.size()>max_request_bytes || context.stopped()){out.status=outcome::NotAttempted;return out;}
   response_sink sink(context);called=true;
   if(!port.exchange_json(method,reinterpret_cast<const unsigned char*>(wire.data()),wire.size(),context,sink) || sink.failed_ || sink.bytes_.empty() || context.stopped())return out;
   const auto reply=decode(sink.bytes_,method==route::Lookup,context);
   if(context.stopped())return out;
   out.daemon_status=reply.status;
   if(method==route::Submit){out.status=reply.status==API_RETURN_CODE_OK?outcome::DaemonAccepted:reply.status=="Not relayed"?outcome::Uncertain:outcome::DaemonRefused;return out;}
   if(reply.status!=API_RETURN_CODE_OK)return out;
   if(reply.transactions.size()==1 && reply.missed.empty() && reply.transactions.front()==hex(bytes)){out.status=outcome::ObservedPoolOrChain;return out;}
   if(reply.transactions.empty() && reply.missed.size()==1 && reply.missed.front()==hash_hex(hash)){out.status=outcome::NotObserved;return out;}
   out.daemon_status.clear();return out;
  }catch(...){out.status=called?outcome::Uncertain:outcome::NotAttempted;return out;}
 }
};
result submit(const std::string& bytes,const crypto::hash& hash,const call_guard& guard,exchange& port,const std::atomic<bool>& stop) noexcept {return executor::run(route::Submit,bytes,hash,guard,port,stop);}
result lookup(const std::string& bytes,const crypto::hash& hash,const call_guard& guard,exchange& port,const std::atomic<bool>& stop) noexcept {return executor::run(route::Lookup,bytes,hash,guard,port,stop);}
} }
