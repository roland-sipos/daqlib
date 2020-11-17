/**
 * @file DummyFunctionality.cc DummyFunctionality class
 * implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "DummyFunctionality.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace dunedaq {
namespace daqlib {

void
DummyFunctionality::do_something()
{

  auto js = json::parse("{\"happy\": 1}");
  std::ofstream ofs("foo.txt");
  ofs << "asd asd \n";
  ofs << js << '\n';

}

} // namespace daqlib
} // namespace dunedaq

