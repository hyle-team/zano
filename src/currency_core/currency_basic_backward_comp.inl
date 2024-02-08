// Copyright (c) 2014-2022 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.



class transaction_prefix_v1
{
public:
  // tx version information
  uint64_t   version{};
  //extra
  std::vector<extra_v> extra;
  std::vector<txin_v> vin;
  std::vector<tx_out_bare> vout;//std::vector<tx_out> vout;

  BEGIN_SERIALIZE()
    //VARINT_FIELD(version) <-- this already unserialized
    if (TRANSACTION_VERSION_PRE_HF4 < version) return false;
    FIELD(vin)
    FIELD(vout)
    FIELD(extra)
  END_SERIALIZE()
};
/*
class transaction_prefix_v1_full
{
public:
  // tx version information
  uint64_t   version{};
  //extra
  std::vector<extra_v> extra;
  std::vector<txin_v> vin;
  std::vector<tx_out_old> vout;//std::vector<tx_out> vout;

  BEGIN_SERIALIZE()
    VARINT_FIELD(version)
    if (TRANSACTION_VERSION_PRE_HF4 < version) return false;
    FIELD(vin)
    FIELD(vout)
    FIELD(extra)
  END_SERIALIZE()
};
*/ 

template<typename transaction_prefix_current_t>
bool transition_convert(const transaction_prefix_current_t& from, transaction_prefix_v1& to)
{
  to.extra = from.extra;
  to.vin = from.vin;
  for (const auto& v : from.vout)
  {
    if (v.type() == typeid(tx_out_bare))
    {
      to.vout.push_back(boost::get<tx_out_bare>(v));
    }
    else {
      throw std::runtime_error("Unexpected type in tx_out_v");
    }
  }
  return true;
}
template<typename transaction_prefix_current_t>
bool transition_convert(const transaction_prefix_v1& from, transaction_prefix_current_t& to)
{
  to.extra = from.extra;
  to.vin = from.vin;
  for (const auto& v : from.vout)
  {
    to.vout.push_back(v);
  }
  return true;
}



class transaction_v1
{
public:
  std::vector<std::vector<crypto::signature> > signatures; //count signatures  always the same as inputs count
  std::vector<attachment_v> attachment;

  BEGIN_SERIALIZE_OBJECT()
    FIELD(signatures)
    FIELD(attachment)
  END_SERIALIZE()
};

template<typename transaction_current_t>
bool transition_convert(const transaction_current_t& from, transaction_v1& to)
{
  to.attachment = from.attachment;
  for (const auto& s : from.signatures)
  {
    if (s.type() == typeid(NLSAG_sig))
    {
      to.signatures.push_back(boost::get<NLSAG_sig>(s).s);
    }
    else
    {
      throw std::runtime_error(std::string("Unexpected type in tx.signatures during transition_convert: ") + s.type().name());
    }
  }
  return true;
}

template<typename transaction_current_t>
bool transition_convert(const transaction_v1& from, transaction_current_t& to)
{
  // TODO: consider using move semantic for 'from'
  to.attachment = from.attachment;
  to.signatures.resize(from.signatures.size());
  for (size_t i = 0; i != from.signatures.size(); i++)
  {
    boost::get<NLSAG_sig>(to.signatures[i]).s = from.signatures[i];
  }
  return true;
}
