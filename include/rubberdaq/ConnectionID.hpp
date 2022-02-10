/**
 * @file ConnectionID.hpp
 */

#ifndef RUBBERDAQ_INCLUDE_CONNECTIONID_HPP_
#define RUBBERDAQ_INCLUDE_CONNECTIONID_HPP_

#include <string>

namespace dunedaq {
namespace rubberdaq {

struct ConnectionID {

  std::string m_service_type;
  std::string m_service_name;
  std::string m_topic;

};

} // namespace rubberdaq
} // namespace dunedaq

#endif // RUBBERDAQ_INCLUDE_CONNECTIONID_HPP_
