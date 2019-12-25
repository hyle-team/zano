#pragma once

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

#include <boost/filesystem.hpp>

#include "misc_log_ex.h"

namespace fs = boost::filesystem;

std::string exec(const std::string& str)
{
    std::array<char, 1024> buffer;

#if defined(WIN32)
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(str.c_str(), "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(str.c_str(), "r"), pclose);
#endif

    if (!pipe)
        throw std::runtime_error("popen() failed!");


    std::string result;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr)
        result += buffer.data();

    return result;
}

bool try_write_test_file(size_t size_bytes)
{

  static const std::string filename = "test_out_file";
  static const fs::path filename_p = filename;

  try
  {
    fs::ofstream s;
    s.open(filename, std::ios_base::binary | std::ios_base::out| std::ios::trunc);
    if(s.fail())
      return false;

    uint8_t block[32 * 1024] = {};
    crypto::generate_random_bytes(sizeof block, &block);
    size_t size_total = 0;
    for (size_t i = 0; i < size_bytes / (sizeof block); ++i)
    {
      s.write((const char*)&block, sizeof block);
      size_total += sizeof block;
    }

    if (size_bytes > size_total)
      s.write((const char*)&block, size_bytes - size_total);

    s.close();

    size_t size_actual = fs::file_size(filename_p);
    CHECK_AND_ASSERT_MES(size_bytes == size_actual, false, "size_bytes = " << size_bytes << ", size_actual = " << size_actual);

    CHECK_AND_ASSERT_MES(fs::remove(filename_p), false, "remove failed");
  }
  catch (std::exception& e)
  {
    LOG_PRINT_RED("caught: " << e.what(), LOG_LEVEL_0);
    return false;
  }
  catch (...)
  {
    LOG_PRINT_RED("caught unknown exception", LOG_LEVEL_0);
    return false;
  }

  return true;
}

void free_space_check()
{
  static const size_t test_file_size = 1024 * 1024;
  namespace fs = boost::filesystem;

  std::string output;
  bool r = false;
  
#ifdef WIN32
  std::string command = "dir";
#else
  std::string command = "df -h && df -i";
#endif
  output = exec(command);

  LOG_PRINT_L0("test command " << command << ", output:" << std::endl << output);

  r = try_write_test_file(test_file_size);
  LOG_PRINT_L0("test file write: " << (r ? "OK" : "fail"));

  boost::filesystem::path current_path(".");

  size_t counter = 0;
  bool need_backspace = false;
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds( 900 ));
    try
    {
      fs::space_info si = fs::space(current_path);
      if (si.available > 1024)
      {
        // free space is ok
        counter = (counter + 1) % 4;
        if (need_backspace)
          std::cout << '\b';
        std::cout << ( counter == 0 ? '*' : counter == 1 ? '\\' : counter == 2 ? '|' : '/' );
        std::cout << std::flush;
        need_backspace = true;
        continue;
      }
      // free space is not ok!
      LOG_PRINT_YELLOW("1) fs::space() : available: " << si.available << ", free: " << si.free << ", capacity: " << si.capacity, LOG_LEVEL_0);

      output = exec(command);
      LOG_PRINT_YELLOW("executed command: " << command << ", output: " << std::endl << output, LOG_LEVEL_0);

      // try one again asap
      si = fs::space(current_path);
      LOG_PRINT_YELLOW("2) fs::space() : available: " << si.available << ", free: " << si.free << ", capacity: " << si.capacity, LOG_LEVEL_0);

      if (!try_write_test_file(test_file_size))
      {
        LOG_PRINT_YELLOW("try_write_test_file(" << test_file_size << ") failed", LOG_LEVEL_0);
      }
      

      need_backspace = false;
    }
    catch (std::exception& e)
    {
      LOG_ERROR("failed to determine free space: " << e.what());
      need_backspace = false;
    }
    catch (...)
    {
      LOG_ERROR("failed to determine free space: unknown exception");
      need_backspace = false;
    }
  }


}

