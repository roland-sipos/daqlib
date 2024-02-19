cmake_minimum_required(VERSION 3.12)
project(rubberdaq VERSION 0.0.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(appfwk REQUIRED)
find_package(hdf5libs REQUIRED)
find_package(readoutlibs REQUIRED)
find_package(fdreadoutlibs REQUIRED)
find_package(CLI11 REQUIRED)
set(BOOST_LIBS Boost::iostreams ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_LIBRARIES})

option(WITH_SNBT_EXAMPLES "Build SNB transfer examples" OFF)

add_compile_definitions(WITH_LIBNUMA_SUPPORT WITH_LIBNUMA_BIND_POLICY=1 WITH_LIBNUMA_STRICT_POLICY=1)

#############################################################################
if(WITH_SNBT_EXAMPLES)
  if(NOT DEFINED ENV{LIBTORRENT_LIB} OR NOT DEFINED ENV{LIBTORRENT_INC})
    message(FATAL_ERROR "RUBBERDAQ CMake configuration ERROR! SNB transfer examples are requested but environment variables: LIBTORRENT_LIB and/or LIBTORRENT_INC are not set!")
  endif()

  set(LIBTORRENT_LIBRARIES
    #$ENV{LIBTORRENT_LIB}/libtorrent-rasterbar.so.2.0.7
    $ENV{LIBTORRENT_LIB}/libtorrent-rasterbar.so.2.0
    #$ENV{LIBTORRENT_LIB}/libtorrent-rasterbar.so
  )
  message(INFO "Torrent lib is: " ${LIBTORRENT_LIBRARIES})
  daq_add_library(*.cpp LINK_LIBRARIES ${LIBTORRENT_LIBRARIES} appfwk::appfwk readoutlibs::readoutlibs fdreadoutlibs::fdreadoutlibs hdf5libs::hdf5libs ${BOOST_LIBS})
  target_include_directories(rubberdaq PUBLIC $ENV{LIBTORRENT_INC})
  daq_add_application(rubberdaq_test_snb_torrent_app test_snb_torrent_app.cxx TEST LINK_LIBRARIES rubberdaq)
  daq_add_application(rubberdaq_test_hdf5_overhead test_hdf5_overhead_app.cxx TEST LINK_LIBRARIES rubberdaq)
else()
  daq_add_library(*.cpp LINK_LIBRARIES appfwk::appfwk readoutlibs::readoutlibs fdreadoutlibs::fdreadoutlibs hdf5libs::hdf5libs ${BOOST_LIBS})
endif()

daq_add_plugin( FooDAQModule duneDAQModule LINK_LIBRARIES rubberdaq )

daq_add_application(rubberdaq_test_io_manager test_io_manager_app.cxx TEST LINK_LIBRARIES rubberdaq)
daq_add_application(rubberdaq_test_serreg test_serreg_app.cxx TEST LINK_LIBRARIES rubberdaq)
daq_add_application(rubberdaq_test_mock_dlh test_mock_dlh_app.cxx TEST LINK_LIBRARIES rubberdaq CLI11::CLI11)


daq_install()

