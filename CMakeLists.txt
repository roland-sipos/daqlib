cmake_minimum_required(VERSION 3.12)
project(rubberdaq VERSION 0.0.0)

find_package(daq-buildtools 1.2.1 REQUIRED )

daq_setup_environment()

find_package(appfwk REQUIRED)
find_package(udaq_readout_deps REQUIRED)

##############################################################################
daq_add_library(*.cpp LINK_LIBRARIES appfwk::appfwk udaq_readout_deps::cmem_rcc)

daq_add_plugin( FooDAQModule duneDAQModule LINK_LIBRARIES rubberdaq )


daq_install()

