// Copyright (c) 2019-2022 Zano Project

// Note: class udp_blocking_client is a slightly modified version of an example
// taken from https://www.boost.org/doc/libs/1_53_0/doc/html/boost_asio/example/timeouts/blocking_udp_client.cpp
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <cstdlib>
#include <iostream>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/array.hpp>
#include <epee/include/misc_log_ex.h>
#include <chrono>
#include "ntp.h"

using boost::asio::deadline_timer;
using boost::asio::ip::udp;

//----------------------------------------------------------------------

//
// This class manages socket timeouts by applying the concept of a deadline.
// Each asynchronous operation is given a deadline by which it must complete.
// Deadlines are enforced by an "actor" that persists for the lifetime of the
// client object:
//
//  +----------------+
//  |                |     
//  | check_deadline |<---+
//  |                |    |
//  +----------------+    | async_wait()
//              |         |
//              +---------+
//
// If the actor determines that the deadline has expired, any outstanding
// socket operations are cancelled. The socket operations themselves are
// implemented as transient actors:
//
//   +---------------+
//   |               |
//   |    receive    |
//   |               |
//   +---------------+
//           |
//  async_-  |    +----------------+
// receive() |    |                |
//           +--->| handle_receive |
//                |                |
//                +----------------+
//
// The client object runs the io_service to block thread execution until the
// actor completes.
//
namespace
{
class udp_blocking_client
{
public:
  udp_blocking_client(const udp::endpoint& listen_endpoint, udp::socket& socket, boost::asio::io_service& io_service)
    : socket_(socket),
      io_service_(io_service),
      deadline_(io_service)
  {
    // No deadline is required until the first socket operation is started. We
    // set the deadline to positive infinity so that the actor takes no action
    // until a specific deadline is set.
    deadline_.expires_at(boost::posix_time::pos_infin);

    // Start the persistent actor that checks for deadline expiry.
    check_deadline();
  }

  std::size_t receive(const boost::asio::mutable_buffer& buffer,
      boost::posix_time::time_duration timeout, boost::system::error_code& ec)
  {
    // Set a deadline for the asynchronous operation.
    deadline_.expires_from_now(timeout);

    // Set up the variables that receive the result of the asynchronous
    // operation. The error code is set to would_block to signal that the
    // operation is incomplete. Asio guarantees that its asynchronous
    // operations will never fail with would_block, so any other value in
    // ec indicates completion.
    ec = boost::asio::error::would_block;
    std::size_t length = 0;

    // Start the asynchronous operation itself. The handle_receive function
    // used as a callback will update the ec and length variables.
    socket_.async_receive(boost::asio::buffer(buffer),
        boost::bind(&udp_blocking_client::handle_receive, boost::placeholders::_1, boost::placeholders::_2, &ec, &length));

    // Block until the asynchronous operation has completed.
    do io_service_.run_one(); while (ec == boost::asio::error::would_block);

    return length;
  }

private:
  void check_deadline()
  {
    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (deadline_.expires_at() <= deadline_timer::traits_type::now())
    {
      // The deadline has passed. The outstanding asynchronous operation needs
      // to be cancelled so that the blocked receive() function will return.
      //
      // Please note that cancel() has portability issues on some versions of
      // Microsoft Windows, and it may be necessary to use close() instead.
      // Consult the documentation for cancel() for further information.
      socket_.cancel();

      // There is no longer an active deadline. The expiry is set to positive
      // infinity so that the actor takes no action until a new deadline is set.
      deadline_.expires_at(boost::posix_time::pos_infin);
    }

    // Put the actor back to sleep.
    deadline_.async_wait(boost::bind(&udp_blocking_client::check_deadline, this));
  }

  static void handle_receive(
      const boost::system::error_code& ec, std::size_t length,
      boost::system::error_code* out_ec, std::size_t* out_length)
  {
    *out_ec = ec;
    *out_length = length;
  }

private:
  boost::asio::io_service& io_service_;
  udp::socket& socket_;
  deadline_timer deadline_;
};


#pragma pack(push, 1)
struct ntp_packet
{

  uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
                           // li.   Two bits.   Leap indicator.
                           // vn.   Three bits. Version number of the protocol.
                           // mode. Three bits. Client will pick mode 3 for client.

  uint8_t stratum;         // Eight bits. Stratum level of the local clock.
  uint8_t poll;            // Eight bits. Maximum interval between successive messages.
  uint8_t precision;       // Eight bits. Precision of the local clock.

  uint32_t rootDelay;      // 32 bits. Total round trip delay time.
  uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
  uint32_t refId;          // 32 bits. Reference clock identifier.

  uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
  uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

  uint64_t orig_tm;        // 64 bits. Originate time-stamp (set by client)

  uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
  uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

  uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
  uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.

};                         // Total: 384 bits or 48 bytes.
#pragma pack(pop)

static_assert(sizeof(ntp_packet) == 48, "ntp_packet has invalid size");

} // namespace


namespace tools
{
  int64_t get_ntp_time(const std::string& host_name, size_t timeout_sec)
  {
    try
    {
      boost::asio::io_service io_service;
      boost::asio::ip::udp::resolver resolver(io_service);
      boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), host_name, "ntp");
      boost::asio::ip::udp::endpoint receiver_endpoint = *resolver.resolve(query);
      boost::asio::ip::udp::socket socket(io_service);
      socket.open(boost::asio::ip::udp::v4());

      ntp_packet packet_sent = AUTO_VAL_INIT(packet_sent);
      packet_sent.li_vn_mode = 0x1b;
      auto packet_sent_time = std::chrono::high_resolution_clock::now();
      auto send_res = socket.send_to(boost::asio::buffer(&packet_sent, sizeof packet_sent), receiver_endpoint);
      if (send_res != sizeof packet_sent)
      {
        LOG_PRINT_L3("NTP: get_ntp_time(" << host_name << "): wrong send_res: " << send_res << ", expected sizeof packet_sent = " << sizeof packet_sent);
        return 0;
      }

      ntp_packet packet_received = AUTO_VAL_INIT(packet_received);
      boost::asio::ip::udp::endpoint sender_endpoint;

      udp_blocking_client ubc(sender_endpoint, socket, io_service);
      boost::system::error_code ec;
      ubc.receive(boost::asio::buffer(&packet_received, sizeof packet_received), boost::posix_time::seconds(static_cast<long>(timeout_sec)), ec);
      if (ec)
      {
        LOG_PRINT_L3("NTP: get_ntp_time(" << host_name << "): boost error: " << ec.message());
        return 0;
      }

      auto packet_received_time = std::chrono::high_resolution_clock::now();
      int64_t roundtrip_mcs = std::chrono::duration_cast<std::chrono::microseconds>(packet_received_time - packet_sent_time).count();
      uint64_t roundtrip_s = roundtrip_mcs > 2000000 ? roundtrip_mcs / 2000000 : 0;

      time_t ntp_time = ntohl(packet_received.txTm_s);
      if (ntp_time <= 2208988800U)
      {
        LOG_PRINT_L3("NTP: get_ntp_time(" << host_name << "): wrong txTm_s: " << packet_received.txTm_s);
        return 0;
      }
      // LOG_PRINT_L2("NTP: get_ntp_time(" << host_name << "): RAW time received: " << ntp_time << ", refTm_s: " << ntohl(packet_received.refTm_s) << ", stratum: " << packet_received.stratum << ", round: " << roundtrip_mcs);
      ntp_time -= 2208988800U;  // Unix time starts from 01/01/1970 == 2208988800U
      ntp_time += roundtrip_s;
      return ntp_time;
    }
    catch (const std::exception& e)
    {
      LOG_PRINT_L2("NTP: get_ntp_time(" << host_name << "): exception: " << e.what());
      return 0;
    }
    catch (...)
    {
      LOG_PRINT_L2("NTP: get_ntp_time(" << host_name << "): unknown exception");
      return 0;
    }
  }


  #define TIME_SYNC_NTP_SERVERS            "time1.google.com", "time2.google.com", "time3.google.com", "time4.google.com" /* , "pool.ntp.org" -- we need to request a vender zone before using this pool */
  #define TIME_SYNC_NTP_TIMEOUT_SEC        5
  #define TIME_SYNC_NTP_ATTEMPTS_COUNT     3  // max number of attempts when getting time from NTP server

  int64_t get_ntp_time()
  {
    static const std::vector<std::string> ntp_servers { TIME_SYNC_NTP_SERVERS };

    for (size_t att = 0; att < TIME_SYNC_NTP_ATTEMPTS_COUNT; ++att)
    {
      const std::string& ntp_server = ntp_servers[att % ntp_servers.size()];
      LOG_PRINT_L3("NTP: trying to get time from " << ntp_server);
      int64_t time = get_ntp_time(ntp_server, TIME_SYNC_NTP_TIMEOUT_SEC);
      if (time > 0)
      {
        LOG_PRINT_L3("NTP: " << ntp_server << " responded with " << time << " (" << epee::misc_utils::get_time_str_v2(time) << ")");
        return time;
      }
    }

    LOG_PRINT_RED("NTP: unable to get time from NTP with " << TIME_SYNC_NTP_ATTEMPTS_COUNT << " trials", LOG_LEVEL_2);
    return 0; // smth went wrong
  }



} // namespace tools
