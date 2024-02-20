// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <eos/portable_archive.hpp>




#define CHECK_PROJECT_NAME()    std::string project_name = CURRENCY_NAME; ar & project_name;  if(!(project_name == CURRENCY_NAME) ) {throw std::runtime_error(std::string("wrong storage file: project name in file: ") + project_name + ", expected: " + CURRENCY_NAME );}



namespace tools
{
  template<class t_object>
  bool serialize_obj_to_file(t_object& obj, const std::string& file_path)
  {
    TRY_ENTRY();
    boost::filesystem::ofstream data_file;
    data_file.open( epee::string_encoding::utf8_to_wstring(file_path) , std::ios_base::binary | std::ios_base::out| std::ios::trunc);
    if(data_file.fail())
      return false;

    boost::archive::binary_oarchive a(data_file);
    a << obj;

    return !data_file.fail();
    CATCH_ENTRY_L0("serialize_obj_to_file: could not serialize into " << file_path, false);
  }


  template<class t_object>
  bool serialize_obj_to_buff(t_object& obj, std::string& buff)
  {
    TRY_ENTRY();
    std::stringstream data_buff;

    boost::archive::binary_oarchive a(data_buff);
    a << obj;
    buff = data_buff.str();
    return !data_buff.fail();
    CATCH_ENTRY_L0("serialize_obj_to_buff", false);
  }

  template<class t_object, class t_stream>
  bool portble_serialize_obj_to_stream(t_object& obj, t_stream& stream)
  {
    TRY_ENTRY();

    eos::portable_oarchive a(stream);
    a << obj;

    return !stream.fail();
    CATCH_ENTRY_L0("portble_serialize_obj_to_stream", false);
  }

  template<class t_object>
  bool unserialize_obj_from_file(t_object& obj, const std::string& file_path)
  {
    TRY_ENTRY();

    boost::filesystem::ifstream data_file;  
    data_file.open( epee::string_encoding::utf8_to_wstring(file_path), std::ios_base::binary | std::ios_base::in);
    if(data_file.fail())
      return false;
    boost::archive::binary_iarchive a(data_file);

    a >> obj;
    return !data_file.fail();
    CATCH_ENTRY_L0("unserialize_obj_from_file: could not load " << file_path, false);
  }

  template<class t_object>
  bool unserialize_obj_from_buff(t_object& obj, const std::string& buff)
  {
    TRY_ENTRY();

    std::stringstream ss(buff);
    boost::archive::binary_iarchive a(ss);

    a >> obj;
    return !ss.fail();
    CATCH_ENTRY_L0("unserialize_obj_from_buff", false);
  }


  template<class t_object, class t_stream>
  bool portable_unserialize_obj_from_stream(t_object& obj, t_stream& stream)
  {
    TRY_ENTRY();

    eos::portable_iarchive a(stream);

    a >> obj;
    return !stream.fail();
    CATCH_ENTRY_L0("portable_unserialize_obj_from_stream", false);
  }
}
