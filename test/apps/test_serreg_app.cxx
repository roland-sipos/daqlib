/**
 * @file test_serreg_app.cxx Test application for
 * SerializerRegistry usage.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "logging/Logging.hpp"

#include "rubberdaq/SerializerRegistry.hpp"

#include <string>
#include <iostream>
#include <sstream>

//using namespace dunedaq::rubberdaq;

std::string int_to_string(int num) {
  std::cout << "My number is: " << num << '\n';
  std::stringstream ss;
  ss << num;
  return ss.str();
}

int
main(int /*argc*/, char** /*argv[]*/)
{

  std::string test_data("foobar");

  std::function<bool(std::string&)> string_to_cout = [&](std::string& str) {
    std::cout << "My string is: " << str;
    return true;
  };

  dunedaq::rubberdaq::SerializerRegistry serreg;

  serreg.register_serializer<int>(int_to_string);
  std::string as_str = serreg.get_serializer<int>().call<std::string>(5);
  std::cout << "Serialized: " << as_str << '\n';

  //serreg.register_serializer(test_data, string_to_cout);


  // Exit
  TLOG() << "Exiting.";
  return 0;
}
