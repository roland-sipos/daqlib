/**
 * @file Receiver.hpp
 */

#ifndef RUBBERDAQ_INCLUDE_RECEIVER_HPP_
#define RUBBERDAQ_INCLUDE_RECEIVER_HPP_

#include "rubberdaq/ConnectionID.hpp"

#include <any>
#include <atomic>
#include <mutex>
#include <thread>
#include <iostream>

namespace dunedaq {
namespace rubberdaq {

// Typeless
class Receiver {
public:
  virtual ~Receiver() = default;
};

// Interface
template<typename Datatype>
class ReceiverConcept : public Receiver {
public:
  virtual Datatype receive() = 0;
};

// QImpl
template<typename Datatype>
class QueueReceiverModel : public ReceiverConcept<Datatype> {
public:
  explicit QueueReceiverModel(ConnectionID conn_id)
    : m_conn_id(conn_id)
  {
    std::cout << "QueueReceiverModel created with DT! Addr: " << this << '\n';
    // get queue ref from queueregistry based on conn_id
  }

  Datatype receive() override {
    std::cout << "Hand off data...\n";
    Datatype dt;
    return dt;
    //if (m_queue->write(
  }

  ConnectionID m_conn_id;
};

// NImpl
template<typename Datatype>
class NetworkReceiverModel : public ReceiverConcept<Datatype> {
public:
  explicit NetworkReceiverModel(ConnectionID conn_id)
    : ReceiverConcept<Datatype>(conn_id)
    , m_conn_id(conn_id)
  {
    std::cout << "NetworkReceiverModel created with DT! Addr: " << this << '\n';
    // get network resources
  }

  Datatype receive() override {
    std::cout << "Hand off data...\n";
    Datatype dt;
    return dt;
    //if (m_queue->write(
  }

  ConnectionID m_conn_id;
};

} // namespace rubberdaq
} // namespace dunedaq

#endif
