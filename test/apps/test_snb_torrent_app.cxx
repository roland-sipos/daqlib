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

int
main(int /*argc*/, char** /*argv[]*/)
{

  lt::create_flags_t flags = {};

  // Exit
  TLOG() << "Exiting.";
  return 0;
}
