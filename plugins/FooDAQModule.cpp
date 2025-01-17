/**
 * @file FooDAQModule.cc FooDAQModule class
 * implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "FooDAQModule.hpp"

#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <TRACE/trace.h>
/**
 * @brief Name used by TRACE TLOG calls from this source file
 */
#define TRACE_NAME "FooDAQModule" // NOLINT

namespace dunedaq {
namespace rubberdaq {

FooDAQModule::FooDAQModule(const std::string& name)
  : DAQModule(name)
{
  register_command("configure", &FooDAQModule::do_configure);
  register_command("start", &FooDAQModule::do_start);
  register_command("stop", &FooDAQModule::do_stop);
}

void FooDAQModule::init(std::shared_ptr<appfwk::ModuleConfiguration>)
{

}

void FooDAQModule::do_configure(const data_t & /*args*/)
{

}

void
FooDAQModule::do_start(const data_t & /*args*/)
{

}

void
FooDAQModule::do_stop(const data_t & /*args*/)
{

}

} // namespace rubberdaq
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::rubberdaq::FooDAQModule)
