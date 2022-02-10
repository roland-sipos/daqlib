/**
 * @file IOManager.hpp
 */

#ifndef RUBBERDAQ_INCLUDE_IOMANAGER_HPP_
#define RUBBERDAQ_INCLUDE_IOMANAGER_HPP_

#include "nlohmann/json.hpp"

#include <map>
#include <mutex>
#include <type_traits>

namespace dunedaq {
namespace rubberdaq {

class IOManager : public daqling::utilities::Singleton<IOManager> {

public:
  IOManager() : m_receiver_channels{0}, m_sender_channels{0} {}
  // Custom types
  using SizeStatMap = std::map<unsigned, std::atomic<size_t>>;
  using SubManagerMap = std::map<std::string, std::shared_ptr<ConnectionSubManager>>;
  // Teardown
  bool removeChannel(const std::string &name);
  // Add a channel (sockets and queues)
  bool addSenderChannel(const std::string & /*key*/, const nlohmann::json &j);
  bool addReceiverChannel(const std::string & /*key*/, const nlohmann::json &j);

  // Utilities
  ConnectionSubManager &addSubManager(std::string key);

  // Start/stop socket processors
  bool start(const std::string &name);
  bool stop(const std::string &name);

  std::shared_ptr<daqling::utilities::Resource> getLocalResource(unsigned id);

private:
  // Map of ConnectionSubManagers
  SubManagerMap m_sub_managers;
  size_t m_receiver_channels;
  size_t m_sender_channels;

  std::mutex m_mutex;
  std::mutex m_mtx_cleaning;
};

/*
 * ConnectionSubManager
 * Description: Wrapper class for sockets and SPSC circular buffers.
 *   Makes the communication between DAQ processes easier and scalable.
 * Date: May 2021
 */
class ConnectionSubManager {
  friend class IOManager;

public:
  using SenderMap = std::map<unsigned, std::shared_ptr<Sender>>;
  using ReceiverMap = std::map<unsigned, std::shared_ptr<Receiver>>;

  ConnectionSubManager(std::string name);
  /**
   * @brief Receive binary from receiver channel.
   * @param bin Datatype to receive into. Its current inner data will be cleared before passing it
   * on.
   * @param chn receiver channel id
   * @return true when binary file is successfully passed
   */
  template <class T> bool receive(const unsigned &chn, T &bin) {
    if (bin.size() != 0) {
      bin.clear_inner_data();
    }
    DataTypeWrapper msg(bin);
    bool retval = m_receivers[chn]->receive(msg);
    if (retval) {
      msg.transfer_into(bin);
      return true;
    }
    return false;
  }
  /**
   * @brief Sleep receive binary from receiver channel.
   * @param bin Datatype to receive into. Its current inner data will be cleared before passing it
   * on.
   * @param chn receiver channel id
   * @return true when binary file is successfully passed
   */
  template <class T> bool sleep_receive(const unsigned &chn, T &bin) {
    if (bin.size() != 0) {
      bin.clear_inner_data();
    }
    DataTypeWrapper msg(bin);
    bool retval = m_receivers[chn]->sleep_receive(msg);
    if (retval) {
      msg.transfer_into(bin);
      return true;
    }
    return false;
  }
  /**
   * @brief Send binary to channel
   * @param chn sender channel id
   * @param msgBin DataType with inner data to send.
   * @return true when binary file is successfully passed
   */
  template <class T> bool send(const unsigned &chn, T &msgBin) {
    DataTypeWrapper msg(std::move(msgBin));
    return m_senders[chn]->send(msg);
  }
  /**
   * @brief Sleep send binary to channel
   * @param chn sender channel id
   * @param msgBin DataType with inner data to send.
   * @return true when binary file is successfully passed
   */
  template <class T> bool sleep_send(const unsigned &chn, T &msgBin) {
    DataTypeWrapper msg(std::move(msgBin));
    return m_senders[chn]->sleep_send(msg);
  }

  /**
   * @brief Set sleep duration for receiver
   * @param chn receiver channel id
   * @param ms delay in ms
   */
  void set_receiver_sleep_duration(const unsigned &chn, uint ms);
  /**
   * @brief Set sleep duration for sender
   * @param chn sender channel id
   * @param ms delay in ms
   */
  void set_sender_sleep_duration(const unsigned &chn, uint ms);

  bool removeReceiverChannel(unsigned chn);
  bool removeSenderChannel(unsigned chn);
  // Utilities
  const SenderMap getSenderMap() { return m_senders; }
  const ReceiverMap getReceiverMap() { return m_receivers; }
  const std::string getType() { return m_type; }

private:
  SenderMap m_senders;
  ReceiverMap m_receivers;
  size_t m_receiver_channels{};
  size_t m_sender_channels{};
  std::string m_name;
  std::string m_type;
};

} // namespace rubberdaq
} // namespace dunedaq

#endif // RUBBERDAQ_INCLUDE_IOMANAGER_HPP_
