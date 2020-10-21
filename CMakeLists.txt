cmake_minimum_required(VERSION 3.12)
project(daqlib VERSION 0.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../daq-buildtools/cmake ${CMAKE_MODULE_PATH})
include(DAQ)

daq_setup_environment()

find_package(appfwk REQUIRED)

##############################################################################
daq_point_build_to( src )

add_library(daqlib_FooDAQModule_duneDAQModule src/FooDAQModule.cpp src/DummyFunctionality.cpp)
target_link_libraries(daqlib_FooDAQModule_duneDAQModule appfwk::appfwk)

##############################################################################
daq_point_build_to( test )

#file(COPY test/test.json DESTINATION test)

##############################################################################
daq_point_build_to( unittest )

daq_install(TARGETS daqlib_FooDAQModule_duneDAQModule)

