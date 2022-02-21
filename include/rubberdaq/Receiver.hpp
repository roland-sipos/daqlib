/**
 * @file Receiver.hpp
 */

#ifndef RUBBERDAQ_INCLUDE_RECEIVER_HPP_
#define RUBBERDAQ_INCLUDE_RECEIVER_HPP_

#include "rubberdaq/ConnectionID.hpp"
#include "rubberdaq/GenericCallback.hpp"
#include "utilities/ReusableThread.hpp"

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
  virtual void add_callback(std::function<void(Datatype)> callback, std::atomic<bool>& run_marker) = 0;
  virtual void remove_callback() = 0;
};

// QImpl
template<typename Datatype>
class QueueReceiverModel : public ReceiverConcept<Datatype> {
public:
  explicit QueueReceiverModel(ConnectionID conn_id)
    : m_conn_id(conn_id)
    , m_with_callback{false}
  {
    std::cout << "QueueReceiverModel created with DT! Addr: " << this << '\n';
    // get queue ref from queueregistry based on conn_id
    // std::string sink_name = conn_id to sink_name;
    // m_source = std::make_unique<appfwk::DAQSource<Datatype>>(sink_name);
  }

  Datatype receive() override {
    if (m_with_callback) {
      std::cout << "QueueReceiver model is equipped with callback! Ignoring receive call.\n";
      Datatype dt;
      return dt;
    }
    std::cout << "Hand off data...\n";
    Datatype dt;
    return dt;
    //if (m_queue->write(
  }

  void add_callback(std::function<void(Datatype)> callback, std::atomic<bool>& run_marker) override {
    std::cout << "Registering callback.\n";
    m_callback = callback;
    m_with_callback = true;
    // start event loop (thread that calls when receive happens)
    m_event_loop_runner = std::thread([&]() {
      while (run_marker.load()) {
        std::cout << "Take data from q then invoke callback...\n";
        Datatype dt;
        m_callback(dt);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    });
  }

  void remove_callback() override {
    if (m_event_loop_runner.joinable()) {
      m_event_loop_runner.join();
    } else { 
      std::cout << "Event loop can't be closed!\n";
    }
    // remove function.
  }

  ConnectionID m_conn_id;
  bool m_with_callback;
  std::function<void(Datatype)> m_callback; 
  std::thread m_event_loop_runner;
  // std::unique_ptr<appfwk::DAQSource<Datatype>> m_source;
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

  void add_callback(std::function<void(Datatype)> callback, std::atomic<bool>& run_marker) {
    std::cout << "Registering callback.\n";
    m_callback = callback;
    m_with_callback = true;
    // start event loop (thread that calls when receive happens)
    m_event_loop_runner = std::thread([&]() {
      while (run_marker.load()) {
        std::cout << "Take data from network then invoke callback...\n";
        Datatype dt;
        m_callback(dt);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    });
  }

  void remove_callback() override {
    if (m_event_loop_runner.joinable()) {
      m_event_loop_runner.join();
    } else { 
      std::cout << "Event loop can't be closed!\n";
    }
    // remove function.
  }
 
  ConnectionID m_conn_id;
  bool m_with_callback;
  std::function<void(Datatype)> m_callback;
  std::thread m_event_loop_runner;

};

} // namespace rubberdaq
} // namespace dunedaq

#endif
