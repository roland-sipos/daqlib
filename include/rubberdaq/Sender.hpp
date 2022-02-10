/**
 * @file Sender.hpp
 */

#ifndef RUBBERDAQ_INCLUDE_SENDER_HPP_
#define RUBBERDAQ_INCLUDE_SENDER_HPP_

#include "rubberdaq/ConnectionID.hpp"

#include <any>
#include <atomic>
#include <mutex>
#include <thread>

namespace dunedaq {
namespace rubberdaq {

class Sender {
  Sender() {}
  ~Sender() {}
  Sender(const Sender&) = delete;            ///< Sender is not copy-constructible
  Sender& operator=(const Sender&) = delete; ///< Sender is not copy-assginable
  Sender(Sender&&) = delete;                 ///< Sender is not move-constructible
  Sender& operator=(Sender&&) = delete;      ///< Sender is not move-assignable
};

template<typename Datatype>
class SenderConcept : Sender {
public:
  virtual void send(Datatype& data) = 0;
};

template<typename Datatype>
class QueueSenderModel : public SenderConcept<Datatype> {
public:
  explicit QueueSenderModel(ConnectionID conn_id)
    : m_conn_id(conn_id)
  {
    // get queue ref from queueregistry based on conn_id
  }

  void send(Datatype& data) override {
    //if (m_queue->write(
  }

protected:
  ConnectionID m_conn_id;
  //std::unique_ptr<Queue> m_queue;
};

} // namespace rubberdaq
} // namespace dunedaq

#endif
