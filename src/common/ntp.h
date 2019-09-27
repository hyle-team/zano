// Copyright (c) 2019 Zano Project

#pragma once
#include <cstdint>
#include <string>

namespace tools
{

  // requests current time via NTP from 'host_hame' using 'timeout_sec'
  // may return zero -- means error
  int64_t get_ntp_time(const std::string& host_name, size_t timeout_sec = 5);

  // request time via predefined NTP servers
  // may return zero -- mean error
  int64_t get_ntp_time();

} // namespace tools
