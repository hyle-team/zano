// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#include "read_only_block_wire.h"
#include "common/crypto_serialization.h"
#include "serialization/variant.h"
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/next_prior.hpp>
#include <cstring>
#include <limits>
#include <type_traits>
namespace tools { namespace read_only { namespace wire {
namespace {
using namespace currency;
using crypto::scalar_t;
class cursor;
template<class T> struct schema; // Deliberately no opaque/unknown fallback.
template<class V, class Begin, class End> struct read_variant;
class cursor {
public:
  cursor(bytes_view input, binary_decode_budget& budget, std::vector<char>& scratch,
    inner_collection_work* work)
    : input(input), budget(budget), scratch(scratch), work(work) {}
  bytes_view input;
  binary_decode_budget& budget;
  std::vector<char>& scratch;
  inner_collection_work* work;
  size_t previous_element = 0;
  bool seen_element = false;
  size_t pos=0, written=0;
  bool emitting=false, failed=false, canonical_invalid=false;
  bool poll() { return budget.charge(0,0,0); }
  bool charge(uint64_t n,uint64_t unit,uint64_t elements=0) {
    return budget.charge(n,unit,elements);
  }
  bool collection(uint64_t count,uint64_t unit,bool keep) {
    // One charge/poll, exactly where the original collection charge occurred.
    // Cancellation and allocation refusal retain their original priority.
    if(!charge(keep?count:0,unit,work?(keep?count:0):count)) return false;
    return !work || work->admit(count,budget.max_elements);
  }
  bool begin_element(size_t& start) {
    start=pos;
    return !work || work->begin_element(pos,previous_element,seen_element);
  }
  bool end_element(size_t start) {
    return !work || work->end_element(start,pos);
  }
  // For the finite grammar, every vector element has a distinct leading
  // byte. A variant consumes its tag even for an empty alternative; a nested
  // vector consumes its length before any child starts, even when empty;
  // scalar/blob/struct fields likewise precede descendant collection items.
  // The strictly increasing start witness plus positive end progress guards
  // all three loops. Across transport blobs, borrowed spans are disjoint and
  // each is decoded once, so whole-reply elements <= encoded body bytes.
  bool emit(const char* p,size_t n) {
    if (!emitting) return true;
    if (canonical_invalid) return true;
    if (written>scratch.size() || n>scratch.size()-written) {canonical_invalid=true;return true;}
    if (n) std::memcpy(scratch.data()+written,p,n);
    written+=n; return true;
  }
  bool take(void* p,size_t n,bool output=true) {
    if (!poll() || pos>input.size || n>input.size-pos) {failed=true;return false;}
    if (p && n) std::memcpy(p,input.data+pos,n);
    if (output && !emit(input.data+pos,n)) return false;
    pos+=n; return true;
  }
  template<class T> bool varint(T& value) {
    static_assert(std::is_unsigned<T>::value,"unsigned wire varint");
    if (!poll()) return false;
    const size_t start=pos;
    constexpr size_t max=(std::numeric_limits<T>::digits+6)/7;
    for(size_t n=0;n<max;++n) {
      if(pos>=input.size) {failed=true;return false;}
      const unsigned char b=static_cast<unsigned char>(input.data[pos++]);
      if(!(b&0x80)) {
        const auto* first=reinterpret_cast<const unsigned char*>(input.data+start);
        if(tools::read_varint(static_cast<const unsigned char*>(first),static_cast<const unsigned char*>(first+(pos-start)),value)!=static_cast<int>(pos-start))
          {failed=true;return false;}
        char encoded[max]; char* end=encoded;
        tools::write_varint(end,value);
        return emit(encoded,static_cast<size_t>(end-encoded));
      }
    }
    failed=true;return false;
  }
  template<class T> bool field(T& value,bool keep) {
    if constexpr (std::is_integral<T>::value) {
      if constexpr (std::is_same<T,bool>::value) {
        unsigned char b=0;
        if(!take(&b,1) || b>1) {failed=true;return false;}
        value=b!=0;return true;
      } else {
        using U=typename std::make_unsigned<T>::type;
        U number=0; char encoded[sizeof(T)];
        if(!take(encoded,sizeof(T),false)) return false;
        for(size_t i=0;i<sizeof(T);++i)
          number|=static_cast<U>(static_cast<unsigned char>(encoded[i]))<<(i*8);
        std::memcpy(&value,&number,sizeof(T));
        // Emit canonical primitive representation from the decoded value.
        for(size_t i=0;i<sizeof(T);++i) encoded[i]=static_cast<char>(number>>(i*8));
        return emit(encoded,sizeof(T));
      }
    } else if constexpr (is_blob_type<T>::type::value) {
      return take(&value,sizeof(T));
    } else {
      return schema<T>::read(*this,value,keep);
    }
  }
  bool field(std::string& value,bool keep) {
    size_t count=0;
    if(!varint(count) || pos>input.size || count>input.size-pos) {failed=true;return false;}
    if(keep) {
      if(!charge(count,1)) return false;
      value.assign(input.data+pos,count);
    }
    return take(nullptr,count);
  }
  template<class T> bool element(T& value,bool keep) {
    if constexpr(std::is_same<T,uint64_t>::value) return varint(value);
    else return field(value,keep);
  }
  template<class T> bool vector(std::vector<T>& value,bool keep,size_t* seen=nullptr) {
    size_t count=0;
    if(!varint(count) || pos>input.size || count>input.size-pos) {failed=true;return false;}
    if(seen) *seen=count;
    if(!collection(count,sizeof(T),keep)) return false;
    if(keep) {value.clear();value.reserve(count);}
    for(size_t i=0;i<count;++i) {
      size_t start=0;
      if(!begin_element(start)) return false;
      if(keep) {
        // Decode directly into final pre-reserved storage. An rvalue can
        // still deep-copy a type that declares copying but no move.
        value.emplace_back();
        if(!element(value.back(),true)) return false;
      } else {
        T item{};
        if(!element(item,false)) return false;
      }
      if(!end_element(start)) return false;
    }
    return true;
  }
  template<class T> bool field(std::vector<T>& value,bool keep) {return vector(value,keep);}
  template<class T> bool field(boost::optional<T>& value,bool keep) {
    bool none=false;
    if(!field(none,keep))return false;
    if(none){value.reset();return true;}
    value.emplace();
    return field(value.value(),keep); // Decode into final inline storage.
  }
  template<class... T> bool field(boost::variant<T...>& value,bool keep) {
    uint8_t tag=0;
    if(!field(tag,keep))return false;
    using V=boost::variant<T...>;
    return read_variant<V,typename boost::mpl::begin<typename V::types>::type,
      typename boost::mpl::end<typename V::types>::type>::read(*this,value,tag,keep);
  }
  template<class Old,class Current> bool transition(Current& value,bool keep) {
    const size_t version_byte = written ? written-1 : 0;
    Old old{};
    if(!field(old,keep))return false;
    // The only generic chain here is asset_descriptor_operation_v1, whose
    // destination adopts strings/containers without new requests.
    if constexpr(std::is_same<Old,asset_descriptor_operation_v1>::value) {
      if(emitting && !canonical_invalid && version_byte<scratch.size())
        scratch[version_byte]=static_cast<char>(old.verion);
      if(keep) return transition_convert_from_temporary(old,value);
      value.version=old.verion;
      return true;
    } else {static_assert(sizeof(Old)==0,"unhandled wire transition");}
  }
  bool inputs(std::vector<txin_v>& value,bool keep,transaction_summary& summary) {
    size_t count=0;
    if(!varint(count) || pos>input.size || count>input.size-pos) {failed=true;return false;}
    if(!collection(count,sizeof(txin_v),keep))return false;
    if(keep){value.clear();value.reserve(count);}
    for(size_t i=0;i<count;++i){
      size_t start=0;
      if(!begin_element(start)) return false;
      txin_v discarded{};
      if(keep)value.emplace_back();
      txin_v& item=keep?value.back():discarded;
      if(!field(item,keep) || !end_element(start))return false;
      if(i==0) {
        const txin_gen* gen=boost::get<txin_gen>(&item);
        summary.first_input_generation=gen!=nullptr;
        if(gen)summary.generation_height=gen->height;
      }
    }
    return true;
  }
};
template<class V,class Begin,class End> struct read_variant {
  static bool read(cursor& c,V& v,uint8_t tag,bool keep) {
    using T=typename boost::mpl::deref<Begin>::type;
    if(variant_serialization_traits<binary_archive<false>,T>::get_tag()==tag) {
      if(!c.poll())return false;
      if(keep) {
        // The temporary is empty. Populated alternatives, including the
        // copy-only txin_zc_input, are never transferred through a copy.
        v=T{};
        return c.field(boost::get<T>(v),true);
      }
      T item{};
      if(!c.field(item,false))return false;
      v=std::move(item);return true; // Discarded containers remain empty.
    }
    return read_variant<V,typename boost::mpl::next<Begin>::type,End>::read(c,v,tag,keep);
  }
};
template<class V,class End> struct read_variant<V,End,End> {
  static bool read(cursor& c,V&,uint8_t,bool) {c.failed=true;return false;}
};
template<> struct schema<asset_descriptor_operation_v1> {
  static bool read(cursor& c,asset_descriptor_operation_v1& v,bool keep) {
    return c.field(v.operation_type,keep) && c.field(v.descriptor,keep) &&
      c.field(v.amount_commitment,keep) && c.field(v.opt_asset_id,keep);
  }
};
// The finite field-order mirror is separated only for readable source review.
#include "read_only_block_wire_schema.inl"

bool prefix(cursor& c,transaction_prefix& value,bool keep,transaction_summary& summary) {
  c.emitting=true;c.written=0;
  if(!c.varint(value.version))return false;
  summary.version=value.version;
  if(value.version==TRANSACTION_VERSION_INITAL || value.version==TRANSACTION_VERSION_PRE_HF4) {
    transaction_prefix_v1 old{};
    if(!c.inputs(old.vin,keep,summary) ||
       !c.vector(old.vout,keep,&summary.outputs) || !c.field(old.extra,keep))return false;
    if(keep) {
      if(!c.charge(summary.outputs,sizeof(tx_out_v)))return false;
      if(!transition_convert_from_temporary(old,value))return false;
    }
  } else {
    if(value.version>CURRENT_TRANSACTION_VERSION || !c.inputs(value.vin,keep,summary) ||
       !c.field(value.extra,keep))return false;
    if(value.version<TRANSACTION_VERSION_POST_HF6) {
      std::vector<tx_out_v_v1> old;
      if(!c.vector(old,keep,&summary.outputs))return false;
      if(keep) {
        if(!c.charge(summary.outputs,sizeof(tx_out_v)))return false;
        if(!transition_convert_from_temporary(old,value.vout))return false;
      }
    } else if(!c.vector(value.vout,keep,&summary.outputs)) return false;
    if(value.version>=TRANSACTION_VERSION_POST_HF5 && !c.field(value.hardfork_id,keep))return false;
  }
  if(!c.poll())return false;
  summary.canonical_prefix_bytes=c.written;
  if(!c.canonical_invalid)
    crypto::cn_fast_hash(c.scratch.data(),c.written,summary.prefix_hash);
  c.emitting=false;
  return c.poll();
}
bool transaction(cursor& c,currency::transaction& value,bool keep,transaction_summary& summary) {
  if(!prefix(c,static_cast<transaction_prefix&>(value),keep,summary))return false;
  if(summary.version==TRANSACTION_VERSION_INITAL || summary.version==TRANSACTION_VERSION_PRE_HF4) {
    transaction_v1 old{};
    if(!c.field(old.signatures,keep) || !c.field(old.attachment,keep))return false;
    if(keep) {
      if(!c.charge(old.signatures.size(),sizeof(signature_v)))return false;
      if(!transition_convert_from_temporary(old,value))return false;
    }
    return true;
  }
  return c.field(value.attachment,keep) && c.field(value.signatures,keep) && c.field(value.proofs,keep);
}
transport_failure outcome(cursor& c,bool parsed) {
  if(c.budget.cancelled)return transport_failure::DecodeDeadline; // Owner remaps stop.
  if(c.budget.exhausted || (c.work && c.work->refused))return transport_failure::LimitExceeded;
  if(c.work && c.work->invalid_progress)return transport_failure::MalformedResponse;
  if(!parsed || c.failed || c.pos!=c.input.size)return transport_failure::MalformedResponse;
  if(c.canonical_invalid)return transport_failure::InvalidResponse;
  return transport_failure::None;
}
} // anonymous
transport_failure decode_transaction(bytes_view input,binary_decode_budget& budget,
  std::vector<char>& scratch,currency::transaction* selected,transaction_summary& summary,inner_collection_work* work) {
  currency::transaction discarded{};
  cursor c(input,budget,scratch,work);
  summary={};
  const bool parsed=transaction(c,selected?*selected:discarded,selected!=nullptr,summary);
  return outcome(c,parsed);
}
transport_failure decode_block(bytes_view input,binary_decode_budget& budget,
  std::vector<char>& scratch,currency::block* selected,block_summary& summary,inner_collection_work* work) {
  currency::block discarded{};
  currency::block& value=selected?*selected:discarded;
  cursor c(input,budget,scratch,work);
  summary={};
  bool parsed=c.field(value.major_version,selected!=nullptr) &&
    value.major_version<=CURRENT_BLOCK_MAJOR_VERSION && c.field(value.nonce,selected!=nullptr) &&
    c.field(value.prev_id,selected!=nullptr) && c.varint(value.minor_version) &&
    c.varint(value.timestamp) && c.field(value.flags,selected!=nullptr);
  if(parsed) {
    summary.header=static_cast<block_header&>(value);
    parsed=transaction(c,value.miner_tx,selected!=nullptr,summary.miner);
  }
  if(parsed) {
    size_t count=0;
    parsed=c.varint(count) && c.pos<=input.size && count<=input.size-c.pos &&
      c.collection(count,sizeof(crypto::hash),selected!=nullptr);
    summary.transaction_count=count;
    const size_t start=c.pos;
    if(selected && parsed){value.tx_hashes.clear();value.tx_hashes.reserve(count);}
    for(size_t i=0;parsed && i<count;++i) {
      size_t element_start=0;
      if(!c.begin_element(element_start)) {parsed=false;break;}
      crypto::hash hash{};
      parsed=c.field(hash,selected!=nullptr) && c.end_element(element_start);
      if(selected && parsed)value.tx_hashes.push_back(hash);
    }
    if(parsed)summary.transaction_hashes={input.data+start,c.pos-start};
  }
  return outcome(c,parsed);
}
}}}
