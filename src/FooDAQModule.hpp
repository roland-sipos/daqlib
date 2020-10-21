/**
 * @file FooDAQModule.hpp
 */

#ifndef APPFWK_DAQLIB_FOODAQMODULE_HPP_
#define APPFWK_DAQLIB_FOODAQMODULE_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"

#include <future>
#include <memory>
#include <string>
#include <vector>

namespace dunedaq::daqlib {

/**
 * @brief FooDAQModule Dummy DAQ Module for testing
 *
 * @author Roland Sipos
 * @date   2020-2021
 *
 */
class FooDAQModule : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief FooDAQModule Constructor
   * @param name Instance name for this FooDAQModule instance
   */
  explicit FooDAQModule(const std::string& name);

  FooDAQModule(const FooDAQModule&) =
    delete; ///< FooDAQModule is not copy-constructible
  FooDAQModule& operator=(const FooDAQModule&) =
    delete; ///< FooDAQModule is not copy-assignable
  FooDAQModule(FooDAQModule&&) =
    delete; ///< FooDAQModule is not move-constructible
  FooDAQModule& operator=(FooDAQModule&&) =
    delete; ///< FooDAQModule is not move-assignable

  void init() override;

private:
  // Commands
  void do_configure(const std::vector<std::string>& args);
  void do_start(const std::vector<std::string>& args);
  void do_stop(const std::vector<std::string>& args);

};

} // namespace daqlib

#endif // APPFWK_DAQLIB_FOODAQMODULE_HPP_
