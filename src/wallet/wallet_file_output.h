// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once

#include <ios>
#include <boost/iostreams/categories.hpp>

namespace tools { namespace wallet_file_detail
{
  // Boost's ostream adapter calls sputn directly and may retain a short write
  // in its internal buffer without setting the ostream state. Use the checked
  // ostream operations at the last chain link to observe returned short writes.
  // The filebuf may defer an error until the checked flush or close below.
  template<class FileOutput>
  class checked_file_sink
  {
  public:
    typedef char char_type;
    struct category : boost::iostreams::sink_tag, boost::iostreams::flushable_tag {};
    explicit checked_file_sink(FileOutput& file) : m_file(file) {}
    std::streamsize write(const char* bytes, std::streamsize count)
    {
      if (count < 0)
        throw std::ios_base::failure("invalid wallet output size");
      m_file.write(bytes, count);
      if (!m_file.good())
        throw std::ios_base::failure("wallet file write failed");
      return count;
    }
    bool flush()
    {
      m_file.flush();
      if (!m_file.good())
        throw std::ios_base::failure("wallet file flush failed");
      return true;
    }
  private:
    FileOutput& m_file;
  };

  // Complete the wallet's encryption-filter/checked-file-sink chain before
  // replacement. This observes stream errors, not fsync or crash durability.
  template<class FilteredOutput, class FileOutput>
  void finish_output(FilteredOutput& output, FileOutput& file)
  {
    if (!output.good() || !file.good())
      throw std::ios_base::failure("wallet output failed");
    // strict_sync drains buffered output and checks every link. Calling
    // output.flush first would flush a stateful processor twice. Ordinary
    // pubsync alone can also discard a filter's false flush result.
    if (!output.good() || !output.strict_sync() || !file.good())
      throw std::ios_base::failure("wallet filtered output flush failed");

    // The file is borrowed by the chain. Detach without another implicit flush;
    // no chain destructor may perform output after the checked file close.
    output.set_auto_close(false);
    while (!output.empty())
      output.pop();

    file.flush();
    if (!file.good())
      throw std::ios_base::failure("wallet file flush failed");
    file.close();
    if (file.fail())
      throw std::ios_base::failure("wallet file close failed");
  }
}}
