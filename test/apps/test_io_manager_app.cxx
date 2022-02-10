/**
 * @file test_io_manager_app.cxx Test application for
 * demonstrating IOManager skeleton.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "logging/Logging.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <vector>

int
main(int /*argc*/, char** /*argv[]*/)
{
  int runsecs = 5;

  // Run marker
  std::atomic<bool> marker{ true };

  TLOG() << "Application will terminate in few seconds...";

  // Killswitch that flips the run marker
  auto killswitch = std::thread([&]() {
    std::this_thread::sleep_for(std::chrono::seconds(runsecs));
    marker.store(false);
  });

  // Join local threads
  TLOG() << "Flipping killswitch in order to stop...";
  if (killswitch.joinable()) {
    killswitch.join();
  }

  // Exit
  TLOG() << "Exiting.";
  return 0;
}
