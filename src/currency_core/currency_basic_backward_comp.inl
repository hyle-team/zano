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

struct asset_descriptor_operation_v1
{
  uint8_t                         operation_type = ASSET_DESCRIPTOR_OPERATION_UNDEFINED;
  asset_descriptor_base           descriptor;
  crypto::public_key              amount_commitment = currency::null_pkey;     // premultiplied by 1/8
  boost::optional<crypto::public_key> opt_asset_id;      // target asset_id - for update/emit
  uint8_t verion = ASSET_DESCRIPTOR_OPERATION_HF4_VER;

  BEGIN_SERIALIZE()
    FIELD(operation_type)
    FIELD(descriptor)
    FIELD(amount_commitment)
    //END_VERSION_UNDER(1)
    FIELD(opt_asset_id)
  END_SERIALIZE()
  

  //this map doesn't store internal version member, but it set it by default to val "1", which then transfered via transition_convert() to destination struct
  BEGIN_BOOST_SERIALIZATION()
    BOOST_SERIALIZE(operation_type)
    BOOST_SERIALIZE(descriptor)
    BOOST_SERIALIZE(amount_commitment)
    //BOOST_END_VERSION_UNDER(1)
    BOOST_SERIALIZE(opt_asset_id)
  END_BOOST_SERIALIZATION_TOTAL_FIELDS(5)
};

struct tx_out_zarcanum_v1
{
  tx_out_zarcanum_v1() {}

  // Boost's Assignable concept
  tx_out_zarcanum_v1(const tx_out_zarcanum_v1&) = default;
  tx_out_zarcanum_v1& operator=(const tx_out_zarcanum_v1&) = default;

  crypto::public_key  stealth_address;
  crypto::public_key  concealing_point;  // group element Q, see also Zarcanum paper, premultiplied by 1/8
  crypto::public_key  amount_commitment; // premultiplied by 1/8
  crypto::public_key  blinded_asset_id;  // group element T, premultiplied by 1/8
  uint64_t            encrypted_amount = 0;
  uint8_t             mix_attr = 0;

  BEGIN_SERIALIZE_OBJECT()
    FIELD(stealth_address)
    FIELD(concealing_point)
    FIELD(amount_commitment)
    FIELD(blinded_asset_id)
    FIELD(encrypted_amount)
    FIELD(mix_attr)
  END_SERIALIZE()

};

typedef boost::variant<tx_out_bare, tx_out_zarcanum_v1> tx_out_v_v1;



template<typename asset_descriptor_operation_t>
bool transition_convert(const asset_descriptor_operation_t& from, asset_descriptor_operation_v1& to)
{
  to.verion = from.version;
  to.operation_type = from.operation_type;
  if(from.opt_descriptor.has_value())
  {
    to.descriptor = *from.opt_descriptor;
  }
  else
  {
    throw std::runtime_error(std::string("Unexpected:  missing descriptor in from transaction_current_t"));
  }

  if (from.opt_amount_commitment.has_value())
  {
    to.amount_commitment = *from.opt_amount_commitment;
  }
  else
  {
    //not used over update operations //throw std::runtime_error(std::string("Unexpected:  missing amount_commitment in from transaction_current_t"));
  }

  to.opt_asset_id = from.opt_asset_id;

  if(from.opt_amount.has_value() || from.etc.size())
  {
    throw std::runtime_error(std::string("Unexpected: opt_amount or etc have values during convention, looks like object slicing with information getting lost"));
  }

  return true;
}

template<typename asset_descriptor_operation_t>
bool transition_convert(const asset_descriptor_operation_v1& from, asset_descriptor_operation_t& to)
{
  to.operation_type = from.operation_type;
  to.opt_descriptor = from.descriptor;
  to.opt_amount_commitment = from.amount_commitment;     
  to.opt_asset_id = from.opt_asset_id;      // target asset_id - for update/emit
  to.version = from.verion;
  return true;
}




//template<typename asset_descriptor_operation_t>
inline bool transition_convert(const tx_out_zarcanum& from, tx_out_zarcanum_v1& to)
{
  if (from.encrypted_payment_id != 0)
  {
    throw std::runtime_error(std::string("Unexpected: encrypted_payment_id during tx_out_zarcanum convention, looks like object slicing with information getting lost"));
  }

  if (from.version > 0)
  {
    throw std::runtime_error(std::string("Unexpected: version during tx_out_zarcanum convention, looks like object slicing with information getting lost"));
  }

  to.stealth_address = from.stealth_address;
  to.concealing_point = from.concealing_point;
  to.amount_commitment = from.amount_commitment;
  to.blinded_asset_id = from.blinded_asset_id;
  to.encrypted_amount = from.encrypted_amount;
  to.mix_attr = from.mix_attr;
  return true;
}

//template<typename asset_descriptor_operation_t>
inline bool transition_convert(const tx_out_zarcanum_v1& from, tx_out_zarcanum& to)
{

  to.stealth_address = from.stealth_address;
  to.concealing_point = from.concealing_point;
  to.amount_commitment = from.amount_commitment;
  to.blinded_asset_id = from.blinded_asset_id;
  to.encrypted_amount = from.encrypted_amount;
  to.mix_attr = from.mix_attr;
  to.version = 0;
  to.encrypted_payment_id = 0;

  return true;
}


//template<typename asset_descriptor_operation_t>
inline bool transition_convert(const std::vector<tx_out_v>& from, std::vector<tx_out_v_v1>& to)
{
  to.reserve(from.size());
  for (const auto& v : from)
  {
    if (v.type() == typeid(tx_out_zarcanum))
    {
      tx_out_zarcanum_v1 v1 = AUTO_VAL_INIT(v1);
      transition_convert(boost::get<tx_out_zarcanum>(v), v1);
      to.push_back(std::move(v1));
    }
    else
    {
      to.push_back(boost::get<tx_out_bare>(std::move(v)));
    }
  }

  return true;
}

//template<typename asset_descriptor_operation_t>
inline bool transition_convert(const std::vector<tx_out_v_v1>& from, std::vector<tx_out_v>& to)
{
  to.reserve(from.size());
  for (const auto& v1 : from)
  {
    if (v1.type() == typeid(tx_out_zarcanum_v1))
    {
      tx_out_zarcanum v = AUTO_VAL_INIT(v);
      transition_convert(boost::get<tx_out_zarcanum_v1>(v1), v);
      to.push_back(std::move(v));
    }
    else
    {
      to.push_back(boost::get<tx_out_bare>(std::move(v1)));
    }
  }

  return true;
  return true;
}
