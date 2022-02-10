cmake_minimum_required(VERSION 3.12)
project(rubberdaq VERSION 0.0.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(appfwk REQUIRED)

##############################################################################
daq_add_library(*.cpp LINK_LIBRARIES appfwk::appfwk)

daq_add_plugin( FooDAQModule duneDAQModule LINK_LIBRARIES rubberdaq )

daq_add_application(rubberdaq_test_io_manager test_io_manager_app.cxx TEST LINK_LIBRARIES rubberdaq)
daq_add_application(rubberdaq_test_serreg test_serreg_app.cxx TEST LINK_LIBRARIES rubberdaq)

daq_install()

