// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <iostream>
#include <type_traits>

#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include "include_base_utils.h"

namespace command_line
{
  template<typename T, bool required = false>
  struct arg_descriptor;

  template<typename T>
  struct arg_descriptor<T, false>
  {
    typedef T value_type;
    arg_descriptor(const char* _name, const char* _description):
      name(_name),
      description(_description),
      use_default(false),
      default_value(T())
    {}
    arg_descriptor(const char* _name, const char* _description, const T& default_val) :
      name(_name),
      description(_description),
      use_default(true),
      default_value(default_val)
    {}
//     arg_descriptor(const char* _name, const char* _description, const T& default_val, bool not_use_default) :
//       name(_name),
//       description(_description),
//       default_value(default_val),
//       not_use_default(not_use_default)
//     {}



    const char* name;
    const char* description;
    bool use_default;
    T default_value;
  };

  template<typename T>
  struct arg_descriptor<std::vector<T>, false>
  {
    arg_descriptor(const char* _name, const char* _description) :
      name(_name),
      description(_description)
    {}

    typedef std::vector<T> value_type;

    const char* name;
    const char* description;
  };

  template<typename T>
  struct arg_descriptor<T, true>
  {
    static_assert(!std::is_same<T, bool>::value, "Boolean switch can't be required");

    typedef T value_type;

    const char* name;
    const char* description;
  };

  template<typename T>
  boost::program_options::typed_value<T, char>* make_semantic(const arg_descriptor<T, true>& /*arg*/)
  {
    return boost::program_options::value<T>()->required();
  }

  template<typename T>
  boost::program_options::typed_value<T, char>* make_semantic(const arg_descriptor<T, false>& arg)
  {
    auto semantic = boost::program_options::value<T>();
    if (arg.use_default)
      semantic->default_value(arg.default_value);
    return semantic;
  }

  template<typename T>
  boost::program_options::typed_value<T, char>* make_semantic(const arg_descriptor<T, false>& arg, const T& def)
  {
    auto semantic = boost::program_options::value<T>();
    if (arg.use_default)
      semantic->default_value(def);
    return semantic;
  }

  template<typename T>
  boost::program_options::typed_value<std::vector<T>, char>* make_semantic(const arg_descriptor<std::vector<T>, false>& /*arg*/)
  {
    auto semantic = boost::program_options::value< std::vector<T> >();
    semantic->default_value(std::vector<T>(), "");
    return semantic;
  }

  template<typename T, bool required>
  void add_arg(boost::program_options::options_description& description, const arg_descriptor<T, required>& arg, bool unique = true)
  {
    if (0 != description.find_nothrow(arg.name, false))
    {
      CHECK_AND_ASSERT_MES(!unique, void(), "Argument already exists: " << arg.name);
      return;
    }

    description.add_options()(arg.name, make_semantic(arg), arg.description);
  }

  template<typename T>
  void add_arg(boost::program_options::options_description& description, const arg_descriptor<T, false>& arg, const T& def, bool unique = true)
  {
    if (0 != description.find_nothrow(arg.name, false))
    {
      CHECK_AND_ASSERT_MES(!unique, void(), "Argument already exists: " << arg.name);
      return;
    }

    description.add_options()(arg.name, make_semantic(arg, def), arg.description);
  }

  template<>
  inline void add_arg(boost::program_options::options_description& description, const arg_descriptor<bool, false>& arg, bool unique)
  {
    if (0 != description.find_nothrow(arg.name, false))
    {
      CHECK_AND_ASSERT_MES(!unique, void(), "Argument already exists: " << arg.name);
      return;
    }

    description.add_options()(arg.name, boost::program_options::bool_switch(), arg.description);
  }

  template<typename charT>
  boost::program_options::basic_parsed_options<charT> parse_command_line(int argc, const charT* const argv[],
    const boost::program_options::options_description& desc, bool allow_unregistered = false)
  {
    auto parser = boost::program_options::basic_command_line_parser<charT>(argc, argv);
    parser.options(desc);
    if (allow_unregistered)
    {
      parser.allow_unregistered();
    }
    return parser.run();
  }

  template<typename F>
  bool handle_error_helper(const boost::program_options::options_description& desc, std::string& err, F parser)
  {
    try
    {
      return parser();
    }
    catch (std::exception& e)
    {
      err = e.what();
      std::cerr << "Failed to parse arguments: " << e.what() << std::endl;
      std::cerr << desc << std::endl;
      return false;
    }
    catch (...)
    {
      std::cerr << "Failed to parse arguments: unknown exception" << std::endl;
      std::cerr << desc << std::endl;
      return false;
    }
  }

  template<typename F>
  bool handle_error_helper(const boost::program_options::options_description& desc, F parser)
  {
    std::string stub_err;
    return handle_error_helper(desc, stub_err, parser);
  }

  template<typename T, bool required>
  bool has_arg(const boost::program_options::variables_map& vm, const arg_descriptor<T, required>& arg)
  {
    auto value = vm[arg.name];
    return !value.empty();
  }


  template<typename T, bool required>
  T get_arg(const boost::program_options::variables_map& vm, const arg_descriptor<T, required>& arg)
  {
    return vm[arg.name].template as<T>();
  }
 
  template<>
  inline bool has_arg<bool, false>(const boost::program_options::variables_map& vm, const arg_descriptor<bool, false>& arg)
  {
    return get_arg<bool, false>(vm, arg);
  }

#define ARG_DB_ENGINE_LMDB   "lmdb"
#define ARG_DB_ENGINE_MDBX   "mdbx"


  extern const arg_descriptor<bool>        arg_help;
  extern const arg_descriptor<bool>        arg_version;
  extern const arg_descriptor<std::string> arg_data_dir;
  extern const arg_descriptor<int>         arg_stop_after_height;
  extern const arg_descriptor<std::string> arg_config_file;
  extern const arg_descriptor<bool>        arg_os_version;
  extern const arg_descriptor<std::string> arg_log_dir;
  extern const arg_descriptor<std::string> arg_log_file;
  extern const arg_descriptor<int>         arg_log_level;
  extern const arg_descriptor<bool>        arg_console;
  extern const arg_descriptor<bool>        arg_show_details;
  //extern const arg_descriptor<bool>        arg_show_rpc_autodoc;
  extern const arg_descriptor<bool>        arg_disable_upnp;
  extern const arg_descriptor<bool>        arg_disable_ntp;
  extern const arg_descriptor<bool>        arg_disable_stop_if_time_out_of_sync;
  extern const arg_descriptor<bool>        arg_disable_stop_on_low_free_space;
  extern const arg_descriptor<bool>        arg_enable_offers_service;
  extern const arg_descriptor<std::string> arg_db_engine;
  extern const arg_descriptor<bool>        arg_no_predownload;
  extern const arg_descriptor<bool>        arg_force_predownload;
  extern const arg_descriptor<std::string> arg_process_predownload_from_path;
  extern const arg_descriptor<bool>        arg_validate_predownload;
  extern const arg_descriptor<std::string> arg_predownload_link;
  extern const arg_descriptor<std::string> arg_deeplink;
  extern const arg_descriptor<std::string> arg_generate_rpc_autodoc;
  
}
