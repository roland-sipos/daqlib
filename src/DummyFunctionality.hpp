/**
 * @file DummyFunctionality.hpp
 */

#ifndef RUBBERDAQ_SRC_DUMMYFUNCTIONALITY_HPP_
#define RUBBERDAQ_SRC_DUMMYFUNCTIONALITY_HPP_

// From appfwk
#include "appfwk/DAQSource.hpp"

namespace dunedaq::rubberdaq {

class DummyFunctionality
{
public:
  explicit DummyFunctionality();
  ~DummyFunctionality();
  DummyFunctionality(const DummyFunctionality&) =
    delete; ///< DummyFunctionality is not copy-constructible
  DummyFunctionality& operator=(const DummyFunctionality&) =
    delete; ///< DummyFunctionality is not copy-assignable
  DummyFunctionality(DummyFunctionality&&) =
    delete; ///< DummyFunctionality is not move-constructible
  DummyFunctionality& operator=(DummyFunctionality&&) =
    delete; ///< DummyFunctionality is not move-assignable

  void do_something();

};

} // namespace dunedaq::rubberdaq

#endif // RUBBERDAQ_SRC_DUMMYFUNCTIONALITY_HPP_
