#pragma once

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

#include <boost/filesystem.hpp>

#include "misc_log_ex.h"

std::string exec(const char* cmd)
{
    std::array<char, 1024> buffer;

#if defined(WIN32)
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
#endif

    if (!pipe)
        throw std::runtime_error("popen() failed!");


    std::string result;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr)
        result += buffer.data();

    return result;
}

void free_space_check()
{
  namespace fs = boost::filesystem;

  std::string output;
  
#ifdef WIN32
  output = exec("dir");
#else
  output = exec("df -h");
#endif

  LOG_PRINT_L0("test command output:" << std::endl << output);

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
#ifdef WIN32
      output = exec("dir");
#else
      output = exec("df -h");
#endif
      LOG_PRINT_YELLOW(output, LOG_LEVEL_0);

      // try one again asap
      si = fs::space(current_path);
      LOG_PRINT_YELLOW("2) fs::space() : available: " << si.available << ", free: " << si.free << ", capacity: " << si.capacity, LOG_LEVEL_0);

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

