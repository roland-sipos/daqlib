/**
 * @file test_io_manager_app.cxx Test application for
 * demonstrating IOManager skeleton.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "logging/Logging.hpp"

#include "rubberdaq/IOManager.hpp"
#include "rubberdaq/ConnectionID.hpp"
#include "rubberdaq/Sender.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <vector>

int
main(int /*argc*/, char** /*argv[]*/)
{

  dunedaq::rubberdaq::IOManager iom;

  dunedaq::rubberdaq::ConnectionID cid;
  cid.m_service_type = "queue";
  cid.m_service_name = "input1";
  cid.m_topic = "";

  int msg = 5;
  iom.get_sender<int>(cid)->send(msg);

  auto sender = iom.get_sender<std::string>(cid);
  std::cout << "Type: " << typeid(sender).name() << '\n';

  std::string asd("asd");
  sender->send(asd);

  // Exit
  TLOG() << "Exiting.";
  return 0;
}
