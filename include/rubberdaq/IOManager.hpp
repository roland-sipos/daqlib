/**
 * @file IOManager.hpp
 */

#ifndef RUBBERDAQ_INCLUDE_IOMANAGER_HPP_
#define RUBBERDAQ_INCLUDE_IOMANAGER_HPP_

#include "nlohmann/json.hpp"

#include "rubberdaq/ConnectionID.hpp"
#include "rubberdaq/SerializerRegistry.hpp"
#include "rubberdaq/Sender.hpp"

#include <map>
#include <memory>

namespace dunedaq {
namespace rubberdaq {

/*
 * IOManager
 * Description: Wrapper class for sockets and SPSC circular buffers.
 *   Makes the communication between DAQ processes easier and scalable.
 */
class IOManager {

public:

  IOManager(){}

  IOManager(const IOManager&) = delete;            ///< IOManager is not copy-constructible
  IOManager& operator=(const IOManager&) = delete; ///< IOManager is not copy-assignable
  IOManager(IOManager&&) = delete;                 ///< IOManager is not move-constructible
  IOManager& operator=(IOManager&&) = delete;      ///< IOManager is not move-assignable

  template<typename Datatype>
  SenderConcept<Datatype>* get_sender(ConnectionID conn_id) {
    if (!m_senders.count(conn_id)) {
      // create from lookup service's factory function
      // based on connID we know if it's queue or network
      if (true) { // if queue
        m_senders[conn_id] = std::make_unique<Sender>(NetworkSenderModel<Datatype>(conn_id));
        return dynamic_cast<NetworkSenderModel<Datatype>*>(m_senders[conn_id].get());       
      }
    }

    return dynamic_cast<SenderConcept<Datatype>*>(m_senders[conn_id].get());
  }

  /*
  template<typename Datatype>
  std::shared_ptr<Receiver>& get_receiver(ConnectionID conn_id) {
    if (m_receivers.count(conn_id)) {
      return m_receivers[conn_id];
    } else {
      // create from lookup service
    }
  }
  */

  using SenderMap = std::map<ConnectionID, std::unique_ptr<Sender>>;
  //using ReceiverMap = std::map<ConnectionID, std::shared_ptr<Receiver>>;

  SenderMap m_senders;
  //ReceiverMap m_receivers;
  SerializerRegistry m_serdes_reg;

};

} // namespace rubberdaq
} // namespace dunedaq

#endif // RUBBERDAQ_INCLUDE_IOMANAGER_HPP_
