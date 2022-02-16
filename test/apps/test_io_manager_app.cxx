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

#include <stdio.h>

int
main(int /*argc*/, char** /*argv[]*/)
{
  dunedaq::rubberdaq::IOManager iom;

  // Int sender
  dunedaq::rubberdaq::ConnectionID cid;
  cid.m_service_type = "foo";
  cid.m_service_name = "bar";
  cid.m_topic = "";

  int msg = 5;
  auto isender = iom.get_sender<int>(cid);
  std::cout << "Type: " << typeid(isender).name() << '\n'; 
  isender->send(msg);
  isender->send(msg);

  // One line send
  iom.get_sender<int>(cid)->send(msg);

  // String sender
  dunedaq::rubberdaq::ConnectionID cid2;
  cid2.m_service_type = "bar";
  cid2.m_service_name = "foo";
  cid2.m_topic = ""; 

  auto ssender = iom.get_sender<std::string>(cid2);
  std::cout << "Type: " << typeid(ssender).name() << '\n';
  std::string asd("asd");
  ssender->send(asd);

  // String receiver
  dunedaq::rubberdaq::ConnectionID cid3;
  cid3.m_service_type = "asd";
  cid3.m_service_name = "dsa";
  cid3.m_topic = "";

  auto receiver = iom.get_receiver<std::string>(cid3);
  std::cout << "Type: " << typeid(receiver).name() << '\n';
  std::string got = receiver->receive();

  // Exit
  TLOG() << "Exiting.";
  return 0;
}
