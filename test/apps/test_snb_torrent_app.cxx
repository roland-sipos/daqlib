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

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

std::string 
branch_path(std::string const& f)
{
  if (f.empty()) return f;
  if (f == "/") return "";

  auto len = f.size();
  // if the last character is / or \ ignore it
  if (f[len-1] == '/' || f[len-1] == '\\') --len;
  while (len > 0) {
          --len;
          if (f[len] == '/' || f[len] == '\\')
                  break;
  }

  if (f[len] == '/' || f[len] == '\\') ++len;
  return std::string(f.c_str(), len);
}

// do not include files and folders whose
// name starts with a .
bool 
file_filter(std::string const& f)
{
  if (f.empty()) return false;

  char const* first = f.c_str();
  char const* sep = strrchr(first, '/');

  // if there is no parent path, just set 'sep'
  // to point to the filename.
  // if there is a parent path, skip the '/' character
  if (sep == nullptr) sep = first;
  else ++sep;

  // return false if the first character of the filename is a .
  if (sep[0] == '.') return false;

  std::cerr << f << "\n";
  return true;
}

int
main(int /*argc*/, char** /*argv[]*/)
{
  std::vector<std::string> trackers;
  std::vector<std::string> collections;
  int piece_size = 1048576; // 1 MB
  std::string full_path("/nfs/sw/rsipos/tde-frames.bin");

  lt::create_flags_t flags = {};
  TLOG() << "Creating file storage.";
  lt::file_storage fs;
  TLOG() << "Adding file to FS.";
  lt::add_files(fs, full_path, file_filter, flags);
  TLOG() << "Creating torrent.";
  lt::create_torrent torr(fs, piece_size, flags);
  TLOG() << "Adding tracker.";
  torr.add_tracker("udp://10.73.136.67:8888/announce", 0); // tier=0
  
  //TLOG() << "Adding collection.";
  // torr.add_collection...

  auto const num = torr.num_pieces();
  TLOG() << "Setting N=" << num << " pieces."; 
    lt::set_piece_hashes(torr, branch_path(full_path), 
      [num] (lt::piece_index_t const p) { std::cout << "\r" << p << "/" << num; });

  torr.set_creator("rubberdaq");
  torr.set_comment("snbtest");
  
  std::vector<char> torrent;
  lt::bencode(back_inserter(torrent), torr.generate());
  //std::cout.write(torrent.data(), int(torrent.size()));
  std::string outfile("/nfs/sw/rsipos/test.torrent");
  std::fstream out;
  out.exceptions(std::ifstream::failbit);
  out.open(outfile.c_str(), std::ios_base::out | std::ios_base::binary);
  out.write(torrent.data(), int(torrent.size()));

  // Exit
  TLOG() << "Exiting.";
  return 0;
}
