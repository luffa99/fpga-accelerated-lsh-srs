/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/
 
// #include "xcl2.hpp"
#include <algorithm>
#include <vector>
#include <iostream>
#include <random>
#include "math.h"
#include "host.hpp"

#define DATA_SIZE 100 // <<-- vectors of size 6
#define LOWER_SIZE 6

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string binaryFile = argv[1];
    size_t vector_size_bytes = sizeof(float) * DATA_SIZE;
    size_t vector_size_result_bytes = sizeof(float) * LOWER_SIZE;
    cl_int err;
    cl::Context context;
    cl::Kernel krnl_vector_add;
    cl::CommandQueue q;
    // Allocate Memory in Host Memory
    // When creating a buffer with user pointer (CL_MEM_USE_HOST_PTR), under the
    // hood user ptr
    // is used if it is properly aligned. when not aligned, runtime had no choice
    // but to create
    // its own host side buffer. So it is recommended to use this allocator if
    // user wish to
    // create buffer using CL_MEM_USE_HOST_PTR to align user buffer to page
    // boundary. It will
    // ensure that user buffer is used when user create Buffer/Mem object with
    // CL_MEM_USE_HOST_PTR
    int size_d = DATA_SIZE;
    int size_m = LOWER_SIZE;
    std::vector<float, aligned_allocator<float> > rands(DATA_SIZE*LOWER_SIZE);
    std::vector<float, aligned_allocator<float> > orig(DATA_SIZE);
    std::vector<float, aligned_allocator<float> > proj(LOWER_SIZE);

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist2 (0,1000);

    std::default_random_engine rng;
    rng.seed(dist2(mt));
    std::uniform_real_distribution<float> dist;
    // Create the test data
    std::for_each(rands.begin(), rands.end(), [&](float &i) { i = dist(rng); });
    std::for_each(orig.begin(), orig.end(), [&](float &i) { i = dist(rng); });


    // OPENCL HOST CODE AREA START
    std::vector<cl::Device> devices = get_devices();
    xclbin_file_name = argv[1];
    cl::Program::Binaries vadd_bins = import_binary_file();

    // get_xil_devices() is a utility API which will find the xilinx
    // platforms and will return list of devices connected to Xilinx platform
    // auto devices = xcl::get_xil_devices();
    // read_binary_file() is a utility API which will load the binaryFile
    // and will return the pointer to file buffer.
    // auto fileBuf = xcl::read_binary_file(binaryFile);
    // cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    bool valid_device = false;
    for (unsigned int i = 0; i < devices.size(); i++) {
        auto device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = cl::Context(device, nullptr, nullptr, nullptr, &err));
        OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
        std::cout << "Trying to program device[" << i << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        cl::Program program(context, {device}, vadd_bins, nullptr, &err);
        if (err != CL_SUCCESS) {
            std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
        } else {
            std::cout << "Device[" << i << "]: program successful!\n";
            OCL_CHECK(err, krnl_vector_add = cl::Kernel(program, "vadd", &err)); // <<-- define the programm, "vadd"
            valid_device = true;
            break; // we break because we found a valid device
        }
    }
    if (!valid_device) {
        std::cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }

    // Allocate Buffer in Global Memory
    // Buffers are allocated using CL_MEM_USE_HOST_PTR for efficient memory and
    // Device-to-host communication
    OCL_CHECK(err, cl::Buffer buffer_in1(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, vector_size_bytes*LOWER_SIZE,
                                         rands.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_in2(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, vector_size_bytes,
                                         orig.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_output(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, vector_size_result_bytes,
                                            proj.data(), &err));

    int size = DATA_SIZE;
    OCL_CHECK(err, err = krnl_vector_add.setArg(0, buffer_in1));
    OCL_CHECK(err, err = krnl_vector_add.setArg(1, buffer_in2));
    OCL_CHECK(err, err = krnl_vector_add.setArg(2, buffer_output));
    OCL_CHECK(err, err = krnl_vector_add.setArg(3, size));
    OCL_CHECK(err, err = krnl_vector_add.setArg(4, size_m));

    // Copy input data to device global memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_in1, buffer_in2}, 0 /* 0 means from host*/));

    // Launch the Kernel
    // For HLS kernels global and local size is always (1,1,1). So, it is
    // recommended
    // to always use enqueueTask() for invoking HLS kernel
    auto start = std::chrono::high_resolution_clock::now();

    OCL_CHECK(err, err = q.enqueueTask(krnl_vector_add));

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> float_ms = end - start;
    std::cout << "Kernel execution elapsed time is " << float_ms.count() << " milliseconds" << std::endl;


    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_output}, CL_MIGRATE_MEM_OBJECT_HOST));
    q.finish();
    // OPENCL HOST CODE AREA END

    std::vector<float> proj_host(LOWER_SIZE);

    for (int j=0; j<size_m; j++){
        float acc = 0;
        for (int i=0; i<size_d; i++){
            acc += rands[j*size_d+i] * orig[i];
        }
        proj_host[j] = acc;
    }

    // Compare the results of the Device to the simulation
    bool match = true;
    std::cout.precision(7);
    std::cout << "Precision: 1e-5"<<std::endl;
    for (int i = 0; i < size_m; i++) {

        std::cout << "i = " << i << "\tCPU result = " << proj_host[i]
                    << "\tDevice result = " << proj[i] << "\tDiff: " << proj_host[i] - proj[i] <<std::endl;

        if (abs(proj_host[i] - proj[i]) >= 1e-5) {
            std::cout << "Error: Result mismatch" << std::endl
            << "Diff: " << proj_host[i] - proj[i] << std::endl;
            match = false;
        }
    }

    std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;

    std::cout << "Resulted  projection: ";
    for(int i=0; i<size_m;i++){
        std::cout << proj_host[i] << "\t";
    }
    std::cout << "\n";
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}