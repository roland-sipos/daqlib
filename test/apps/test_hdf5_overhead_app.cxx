/**
 * @file test_snbt_tracker_app.cxx Test application for Libtorrent tracker.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "logging/Logging.hpp"

#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/create_torrent.hpp"

#include <datahandlinglibs/ReadoutTypes.hpp>
#include <datahandlinglibs/utils/BufferedFileWriter.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace dunedaq::datahandlinglibs;

int
main(int argc, char* argv[])
{

  if (argc != 2) {
    TLOG() << "usage: app filename" << std::endl;
    exit(1);
  }
  //remove(argv[1]); // NOLINT

  dunedaq::datahandlinglibs::types::DUMMY_FRAME_STRUCT chunk;
  for (uint i = 0; i < sizeof(chunk); ++i) {
    (reinterpret_cast<char*>(&chunk))[i] = static_cast<char>(i); // NOLINT
  }

  // BufferedFileWriter
  std::string filename(argv[1]);
  BufferedFileWriter writer(filename, 8388608);
  std::atomic<int64_t> bytes_written_total = 0;
  std::atomic<int64_t> bytes_written_since_last_statistics = 0;
  std::chrono::steady_clock::time_point time_point_last_statistics = std::chrono::steady_clock::now();

  auto statistics_thread = std::thread([&]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      double time_diff = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() -
                                                                                   time_point_last_statistics)
                           .count();
      TLOG() << "Bytes written: " << bytes_written_total << ", Throughput: "
             << static_cast<double>(bytes_written_since_last_statistics) / ((int64_t)1 << 20) / time_diff << " MiB/s"
             << std::endl;
      time_point_last_statistics = std::chrono::steady_clock::now();
      bytes_written_since_last_statistics = 0;
    }
  });

  // Run marker
  std::atomic<bool> marker{ true };

  // Killswitch that flips the run marker
  auto killswitch = std::thread([&]() {
    TLOG() << "Application will terminate in 5s...";
    std::this_thread::sleep_for(std::chrono::seconds(10));
    marker.store(false);
  });

  while (marker) {
    if (!writer.write(reinterpret_cast<char*>(&chunk), sizeof(chunk))) {
      TLOG() << "Could not write to file" << std::endl;
      exit(1);
    }
    bytes_written_total += sizeof(chunk);
    bytes_written_since_last_statistics += sizeof(chunk);
  }

  // Exit
  TLOG() << "Exiting.";
  return 0;
}
