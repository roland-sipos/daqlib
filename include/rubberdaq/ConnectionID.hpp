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

inline
bool operator< (const ConnectionID& l, const ConnectionID &r) {
  return l.m_service_type < r.m_service_type &&
         l.m_service_name < r.m_service_name &&
         l.m_topic < r.m_topic;
}

} // namespace rubberdaq
} // namespace dunedaq

#endif // RUBBERDAQ_INCLUDE_CONNECTIONID_HPP_
