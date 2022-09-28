cmake_minimum_required(VERSION 3.12)
project(rubberdaq VERSION 0.0.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(appfwk REQUIRED)

option(WITH_SNBT_EXAMPLES "Build SNB transfer examples" ON)

##############################################################################
if(WITH_SNBT_EXAMPLES)
  if(NOT DEFINED ENV{LIBTORRENT_LIB} OR NOT DEFINED ENV{LIBTORRENT_INC})
    message(FATAL_ERROR "RUBBERDAQ CMake configuration ERROR! SNB transfer examples are requested but environment variables: LIBTORRENT_LIB and/or LIBTORRENT_INT are not set!")
  endif()

  set(LIBTORRENT_LIBRARIES
    $ENV{LIBTORRENT_LIB}/libtorrent-rasterbar.so
  )
  daq_add_library(*.cpp LINK_LIBRARIES appfwk::appfwk ${LIBTORRENT_LIBRARIES})
  target_include_directories(rubberdaq PUBLIC $ENV{LIBTORRENT_INC})
  daq_add_application(rubberdaq_test_snb_torrent_app test_snb_torrent_app.cxx TEST LINK_LIBRARIES rubberdaq)
else()
  daq_add_library(*.cpp LINK_LIBRARIES appfwk::appfwk)
endif()

daq_add_plugin( FooDAQModule duneDAQModule LINK_LIBRARIES rubberdaq )

daq_add_application(rubberdaq_test_io_manager test_io_manager_app.cxx TEST LINK_LIBRARIES rubberdaq)
daq_add_application(rubberdaq_test_serreg test_serreg_app.cxx TEST LINK_LIBRARIES rubberdaq)

daq_install()

