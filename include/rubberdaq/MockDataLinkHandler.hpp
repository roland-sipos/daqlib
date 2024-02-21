/**
 * @file MockDataLinkHandler.hpp
 *
 * Playground for DLH functionalities
 *
 */

#ifndef RUBBERDAQ_INCLUDE_MOCKDATALINKHANDLER_HPP_
#define RUBBERDAQ_INCLUDE_MOCKDATALINKHANDLER_HPP_

#include "logging/Logging.hpp"
#include "iomanager/IOManager.hpp"

#include "readoutlibs/models/IterableQueueModel.hpp"
#include "readoutlibs/utils/ReusableThread.hpp"

#include <iostream>
#include <functional>
#include <atomic>
#include <memory>

namespace dunedaq {
namespace rubberdaq {

template<typename ROT, typename RECEIVE_TYPE = ROT> // preparing for Alessandro's Capsule idea.
class MockDataLinkHandler {
public:
  MockDataLinkHandler(const MockDataLinkHandler&) = delete;
  MockDataLinkHandler& operator=(const MockDataLinkHandler&) = delete;

  MockDataLinkHandler() { }

  MockDataLinkHandler(int id, bool callbacks, std::atomic<bool>& run_marker, int numa_node, std::size_t capacity)
    : m_id(id)
    , m_callbacks(callbacks)
    , m_run_marker(run_marker)
    , m_latency_buffer(capacity, true, numa_node, false, 0)
    , m_consumer_thread(id)
    , m_cleanup_thread(id)
  {
    std::cout << "New MockDLH with ID[" << m_id << "] is prefilling.\n";
    m_latency_buffer.force_pagefault();

    if (callbacks) {
      m_consume_payload = [&, this](ROT&& payload) {
        m_latency_buffer.write(std::move(payload));
        return true;
      };
      std::cout << "  -> Function pointer/address for consume callback is: " << &m_consume_payload << '\n';
    } else {
      //m_consumer_thread.set_work(&MockDataLinkHandler<ROT>::run_consume, this);
      //m_consumer_thread.set_name("consumer", m_id);
    }

    m_pop_limit_size = m_pop_limit_pct * capacity;
    m_cleanup_thread.set_work(&MockDataLinkHandler<ROT>::periodic_cleanups, this);
    m_cleanup_thread.set_name("cleanup", m_id);
  }

  ~MockDataLinkHandler() {

  }

  void set_receiver(dunedaq::iomanager::ConnectionId& queue_id) {
    m_receiver = m_receiver = dunedaq::iomanager::IOManager::get()->get_receiver<ROT>(queue_id);
  } 

  void start_consumer() {
    m_consumer_thread.set_work(&MockDataLinkHandler<ROT>::run_consume, this);
    m_consumer_thread.set_name("consumer", m_id);
  }

  void run_consume() {
    auto rawq_timeout_count = 0;
    auto num_payloads = 0;
    auto sum_payloads = 0;
    auto num_payloads_overwritten = 0; 
    std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(2000);

    while (m_run_marker.load()) {
      auto opt_payload = m_receiver->try_receive(timeout_ms);
      if (opt_payload) {
        ROT& payload = opt_payload.value();
        if (!m_latency_buffer.write(std::move(payload))) {
          ++num_payloads_overwritten;
        }
        ++num_payloads;
      } else {
        ++rawq_timeout_count;
      } 
    }

    TLOG() << "Consumer[" << m_id << "]"
           << " total payloads: " << num_payloads
           << " timeouts: " << rawq_timeout_count
           << " overwritten: " << num_payloads_overwritten;
  } 

  void periodic_cleanups() {
    while (m_run_marker.load()) {
      //cleanup_check();
      if (m_latency_buffer.occupancy() > m_pop_limit_size) {
        auto size_guess = m_latency_buffer.occupancy();
        if (size_guess > m_pop_limit_size) {
          unsigned to_pop = m_pop_size_pct * m_latency_buffer.occupancy();
          m_latency_buffer.pop(to_pop);
          m_num_popped += to_pop;
        }
        ++m_num_cleanups;
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }
  }

  int m_id;
  bool m_callbacks;
  std::atomic<uint64_t> m_num_cleanups = 0;
  std::atomic<uint64_t> m_num_popped = 0;
  std::atomic<bool>& m_run_marker;
  dunedaq::readoutlibs::IterableQueueModel<ROT> m_latency_buffer;

  // Optional callback
  std::function<void(ROT&&)> m_consume_payload;

  // Optional consumer
  dunedaq::readoutlibs::ReusableThread m_consumer_thread;

  using raw_receiver_ct = iomanager::ReceiverConcept<ROT>;
  std::shared_ptr<raw_receiver_ct> m_receiver;

  // auto cleanup
  float m_pop_limit_pct = 0.5f;     // buffer occupancy percentage to issue a pop request
  float m_pop_size_pct = 0.8f;      // buffer percentage to pop
  unsigned m_pop_limit_size; // pop_limit_pct * buffer_capacity
  dunedaq::readoutlibs::ReusableThread m_cleanup_thread;

};


} // namespace rubberdaq
} // namespace dunedaq

#endif // RUBBERDAQ_INCLUDE_MOCKDATALINKHANDLER_HPP_
