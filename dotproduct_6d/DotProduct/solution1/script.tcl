############################################################
## This file is generated automatically by Vitis HLS.
## Please DO NOT edit it.
## Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
############################################################
open_project DotProduct
set_top vadd
add_files Makefile
add_files src/host.cpp
add_files src/host.hpp
add_files synthesis.tcl
add_files utils.mk
add_files src/vadd.cpp
add_files xrt.ini
add_files -tb host
open_solution "solution1" -flow_target vivado
set_part {xcvu5p-flva2104-1-e}
create_clock -period 10 -name default
#source "./DotProduct/solution1/directives.tcl"
csim_design -ldflags {-L$(XILINX_XRT)/lib -lxilinxopencl -pthread -lrt}
csynth_design
cosim_design -ldflags {-L$(XILINX_XRT)/lib -lxilinxopencl -pthread -lrt}
export_design -format ip_catalog
