/**
 * @file test_mock_dlh_app.cxx Test application for DLH brainstorm
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "logging/Logging.hpp"

#include "rubberdaq/MockDataLinkHandler.hpp"

#include "iomanager/IOManager.hpp"

#include "readoutlibs/ReadoutTypes.hpp"
#include "readoutlibs/DataMoveCallbackRegistry.hpp"
#include "readoutlibs/models/IterableQueueModel.hpp"
#include "readoutlibs/utils/RateLimiter.hpp"
#include "readoutlibs/utils/ReusableThread.hpp"
#include "fdreadoutlibs/DUNEWIBEthTypeAdapter.hpp"

#include "CLI/App.hpp"
#include "CLI/Config.hpp"
#include "CLI/Formatter.hpp"

#include <string>
#include <iostream>
#include <sstream>
#include <future>

using namespace dunedaq;
using namespace dunedaq::iomanager;
using namespace dunedaq::rubberdaq;
using namespace dunedaq::readoutlibs;
using namespace dunedaq::fdreadoutlibs::types;

namespace dunedaq {
  DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::DUNEWIBEthTypeAdapter, "WIBEthFrame");
}

namespace {

  template<typename T>
  std::map<int, std::unique_ptr<std::function<void(T&&)>>> CallbackMap = {};

  //std::map<int, GenericCallback> GenericCallbackMap = {};

  int num_streams = 40;
  float prod_rate = 30.5;
  int lb_numa_node = 0;
  std::size_t lb_capacity = 124992; // LB capacity
  int run_for_secs = 60;
  bool consumer_thread_mode = false;
  bool consumer_callback_mode = false;

}

int
main(int argc, char** argv)
{
  CLI::App app{"rubberdaq_mock_dlh"};
  app.add_option("-n", num_streams, "Number of data streams in the test.");
  app.add_option("--rate", prod_rate, "Rate of data producers. [kHz]");
  app.add_option("--lb_numa_node", lb_numa_node, "NUMA node for LBs to allocate on.");
  app.add_option("-c", lb_capacity, "Capacity/size of latency buffer.");
  app.add_option("--run_secs", run_for_secs, "How many seconds the test should run.");
  app.add_flag("--ct", consumer_thread_mode, "Consumer threads mode.");
  app.add_flag("--cb", consumer_callback_mode, "Consume callback mode.");
  CLI11_PARSE(app, argc, argv);

  if (!(consumer_thread_mode || consumer_callback_mode)) {
    std::cout << "Neither threaded or callback consumers are requested. Won't run the test.\n";
    return 0;
  } else if (consumer_thread_mode && consumer_callback_mode) {
    std::cout << "Both thread and callback mode requested. Won't run the test.\n";
    return 0;
  } else if (consumer_thread_mode) {
    std::cout << "Test with consumer threads...\n";
  } else if (consumer_callback_mode) {
    std::cout << "Test with consumer callback...\n";
  }

  // Getting DataMoveCBRegistry
  auto dmcbr = DataMoveCallbackRegistry::get();


  // If consumers, set up IOManager
  if (consumer_thread_mode) {
    setenv("DUNEDAQ_SESSION", "IOManager_t", 0);
    dunedaq::iomanager::Queues_t queues;
    for (unsigned i=0; i<num_streams; ++i) {
      auto qidstr = "queue-" + std::to_string(i);
      ConnectionId queue_id = ConnectionId{ qidstr, "WIBEthFrame" };
      queues.emplace_back(QueueConfig{ queue_id, QueueType::kFollySPSCQueue, 10000 });
    }
    dunedaq::iomanager::Connections_t connections;
    IOManager::get()->configure(queues, connections, false, 1000ms);
  }

  // Run marker
  std::atomic<bool> marker{ true };

  // Create Mock DataLinkHandlers
  std::map<int, std::unique_ptr<MockDataLinkHandler<DUNEWIBEthTypeAdapter>>> dlh_map;
  for (unsigned i=0; i<num_streams; ++i) {
    // Create ith MockDLH
    if (consumer_callback_mode) {
      dlh_map[i] = std::make_unique<MockDataLinkHandler<DUNEWIBEthTypeAdapter>>(i, true, marker, lb_numa_node, lb_capacity);
      // Register consume callback to a map with id i
      //GenericCallbackMap[i] = dlh_map[i]->m_consume_payload;
      CallbackMap<DUNEWIBEthTypeAdapter>[i] = std::make_unique<std::function<void(DUNEWIBEthTypeAdapter&&)>>(dlh_map[i]->m_consume_payload);
      dmcbr->register_callback<DUNEWIBEthTypeAdapter>(std::to_string(i), dlh_map[i]->m_consume_payload);
      std::cout << " Created CallbackMap's function pointer/address is: " << &CallbackMap<DUNEWIBEthTypeAdapter>[i] << '\n';
    } else if (consumer_thread_mode) {
      dlh_map[i] = std::make_unique<MockDataLinkHandler<DUNEWIBEthTypeAdapter>>(i, false, marker, lb_numa_node, lb_capacity);
      auto qidstr = "queue-" + std::to_string(i);
      ConnectionId queue_id = ConnectionId{ qidstr, "WIBEthFrame" };
      dlh_map[i]->set_receiver(queue_id);
      dlh_map[i]->start_consumer();
    }
  }

  // DMCB TEST
  //std::string cbid("asd");
  //dmcbr->register_callback<DUNEWIBEthTypeAdapter>(cbid, dlh_map[0]->m_consume_payload);
  //auto& cb = dmcbr->get_callback<DUNEWIBEthTypeAdapter>(cbid);
  
  //std::function<void(DUNEWIBEthTypeAdapter&&)>* cbtest = dmcbr->get_callback<DUNEWIBEthTypeAdapter>(cbid).get();

  // RateLimiter
  std::cout << "Creating ratelimiter with " << prod_rate << "[kHz]...\n";
  RateLimiter rl(prod_rate);

  // Create data producer threads
  std::map<int, std::thread> producer_map;
  for (unsigned i=0; i<num_streams; ++i) {
    if (consumer_callback_mode) { // go through Callbacks
      auto* cbref = CallbackMap<DUNEWIBEthTypeAdapter>[i].get();
      auto cb = dmcbr->get_callback<DUNEWIBEthTypeAdapter>(std::to_string(i));

      //std::function<void(DUNEWIBEthTypeAdapter&&)>* cbtest = dmcbr->get_callback<DUNEWIBEthTypeAdapter>(std::to_string(i)).get();
      std::shared_ptr<std::function<void(DUNEWIBEthTypeAdapter&&)>> cbtestshared = dmcbr->get_callback<DUNEWIBEthTypeAdapter>(std::to_string(i));

      producer_map[i] = std::thread([&, cbref, cbtestshared]() {
        uint64_t tot_produced = 0;
        uint64_t ts = 0; // NOLINT(build/unsigned)
        while (marker.load()) {
          DUNEWIBEthTypeAdapter pl;
          pl.set_first_timestamp(ts);
          //(*cbref)(std::move(pl));
          (*cbtestshared)(std::move(pl));
          ts += 32;
          ++tot_produced;
          rl.limit();
        }
        TLOG() << "Total produced: " << tot_produced;
      });

    } else if (consumer_thread_mode) { // go through IOManager
      auto qidstr = "queue-" + std::to_string(i);
      ConnectionId queue_id = ConnectionId{ qidstr, "WIBEthFrame" };
      auto sender = dunedaq::iomanager::IOManager::get()->get_sender<DUNEWIBEthTypeAdapter>(queue_id);
      producer_map[i] = std::thread([&, sender]() { 
        uint64_t tot_produced = 0;
        uint64_t ts = 0; // NOLINT(build/unsigned)
        uint64_t dropped = 0;
        while (marker.load()) {
          DUNEWIBEthTypeAdapter pl;
          pl.set_first_timestamp(ts);
          if (!sender->try_send(std::move(pl), iomanager::Sender::s_no_block)) {
            ++dropped;
          }
          ts += 32;
          ++tot_produced;
          rl.limit();
        }
        TLOG() << "Total produced: " << tot_produced << " total dropped: " << dropped;
      });
    }

    std::cout << "Producer [" << i << "] spawned.\n";
  }

  // Name producer threads
  for (auto& [id, prod] : producer_map) {
    char tname[16];
    snprintf(tname, 16, "%s-%d", "producer", id); // NOLINT
    auto handle = prod.native_handle();
    pthread_setname_np(handle, tname);
  }

  // Killswitch that flips the run marker
  auto killswitch = std::thread([&]() {
    std::cout << "Application will terminate in " << run_for_secs << " seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(run_for_secs));
    marker.store(false);
  });

  // Join every threads
  std::cout << "Flipping killswitch that will start the countdown...\n";
  if (killswitch.joinable()) {
    killswitch.join();
  }
  
  // Join producers
  for (auto& [id, prod] : producer_map) {
    if (prod.joinable()) {
      prod.join();
      std::cout << "Producer [" << id << "] joined.\n";
    }
    std::cout << "MockDLH[" << id << "] total cleanups: " << dlh_map[id]->m_num_cleanups 
      << " total elements popped: " << dlh_map[id]->m_num_popped << '\n';
  }

  // Exit
  std::cout << "Exiting.\n";
  return 0;
}
