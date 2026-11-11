// QA regression source only. Unsigned synthetic transaction objects and fake
// exchange responses, no wallet keys, real signing, provider or native run.
#include "../../src/wallet/native_held_submission.cpp"
#include <cassert>
using namespace tools::native_submission;
struct Guard:call_guard {bool live{true};bool current()const noexcept override{return live;}};
struct Port:exchange {
 std::size_t calls{0};std::string response{"{\"status\":\"OK\"}"};bool fail{false},revoke{false};Guard* guard{};
 bool exchange_json(route,const unsigned char* request,std::size_t size,const exchange_context& context,response_sink& sink) noexcept override {
  ++calls;assert(request && size && size<=max_request_bytes);if(revoke)guard->live=false;
  return !fail && sink.append(reinterpret_cast<const unsigned char*>(response.data()),response.size()) && !context.stopped();
 }
};
int main(){
 currency::transaction tx{};tx.version=1;const auto bytes=currency::tx_to_blob(tx);const auto hash=currency::get_transaction_hash(tx);
 Guard guard;Port port;port.guard=&guard;std::atomic<bool> stop{false};
 assert(submit(bytes,currency::null_hash,guard,port,stop).status==outcome::NotAttempted && port.calls==0);
 guard.live=false;assert(submit(bytes,hash,guard,port,stop).status==outcome::NotAttempted && port.calls==0);
 guard.live=true;stop=true;assert(submit(bytes,hash,guard,port,stop).status==outcome::NotAttempted && port.calls==0);stop=false;
 // Parsing/tx-hash validation is the actual SDK implementation. This fabricated
 // object carries no spend authority and is not a valid wallet payment.
 assert(canonical(bytes,hash));
 auto r=submit(bytes,hash,guard,port,stop);assert(r.status==outcome::DaemonAccepted && port.calls==1);
 port.response="{\"status\":\"DISCONNECTED\"}";r=submit(bytes,hash,guard,port,stop);assert(r.status==outcome::DaemonRefused);
 port.response="{\"status\":\"Not relayed\"}";r=submit(bytes,hash,guard,port,stop);assert(r.status==outcome::Uncertain && r.daemon_status=="Not relayed");
 port.fail=true;assert(submit(bytes,hash,guard,port,stop).status==outcome::Uncertain);port.fail=false;
 port.revoke=true;assert(submit(bytes,hash,guard,port,stop).status==outcome::Uncertain);port.revoke=false;guard.live=true;
 for(auto bad:{"{\"status\":\"OK\",\"status\":\"OK\"}","{\"status\":{\"nested\":\"OK\"}}","{\"status\":\"OK\"} trailing","{\"foreign\":\"OK\"}"}) {
  port.response=bad;assert(submit(bytes,hash,guard,port,stop).status==outcome::Uncertain);
 }
 // Actual SDK serializes empty lists by omitting their fields.
 port.response="{\"status\":\"OK\",\"txs_as_hex\":[\""+hex(bytes)+"\"]}";
 assert(lookup(bytes,hash,guard,port,stop).status==outcome::ObservedPoolOrChain);
 port.response="{\"status\":\"OK\",\"missed_tx\":[\""+hash_hex(hash)+"\"]}";
 assert(lookup(bytes,hash,guard,port,stop).status==outcome::NotObserved);
 port.response="{\"status\":\"OK\"}";assert(lookup(bytes,hash,guard,port,stop).status==outcome::Uncertain);
 port.response="{\"status\":\"OK\",\"txs_as_hex\":[\"00\"]}";assert(lookup(bytes,hash,guard,port,stop).status==outcome::Uncertain);
 port.response="{\"status\":\"OK\",\"missed_tx\":[\""+hash_hex(hash)+"\",\""+hash_hex(hash)+"\"]}";
 assert(lookup(bytes,hash,guard,port,stop).status==outcome::Uncertain);
 port.response=std::string(max_response_bytes+1,'x');assert(submit(bytes,hash,guard,port,stop).status==outcome::Uncertain);
}
