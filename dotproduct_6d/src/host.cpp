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

#define LOWER_SIZE 6
#define MAX_VECT_SIZE 10000

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File> <Size of vectors> <Amount of vectors to project>" << std::endl;
        return EXIT_FAILURE;
    }
    int vector_size = atoi(argv[2]);  // Size of vectors
    int AMOUNT = atoi(argv[3]);
    if (vector_size%16!=0 || vector_size < 16 || vector_size > MAX_VECT_SIZE) {
        std::cout << "Size of vectors must be > 16 and < " << MAX_VECT_SIZE << "! You gave " << argv[2] << std::endl;
        return EXIT_FAILURE;
    }
    if (AMOUNT < 1) {
        std::cout << "The amount of vectors must be > 0. You gave " << argv[3] << std::endl;
        return EXIT_FAILURE;
    }

    std::string binaryFile = argv[1];
    size_t vector_size_bytes = sizeof(float) * vector_size;
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

    std::vector<float, aligned_allocator<float> > rand1(vector_size);
    std::vector<float, aligned_allocator<float> > rand2(vector_size);
    std::vector<float, aligned_allocator<float> > rand3(vector_size);
    std::vector<float, aligned_allocator<float> > rand4(vector_size);
    std::vector<float, aligned_allocator<float> > rand5(vector_size);
    std::vector<float, aligned_allocator<float> > rand6(vector_size);
    std::vector<float, aligned_allocator<float> > orig(vector_size*AMOUNT);
    std::vector<float, aligned_allocator<float> > proj(LOWER_SIZE*AMOUNT);

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist2 (0,1000);

    std::default_random_engine rng;
    rng.seed(dist2(mt));
    std::uniform_real_distribution<float> dist;
    // Create the test data
    std::for_each(rand1.begin(), rand1.end(), [&](float &i) { i = dist(rng); });
    std::for_each(rand2.begin(), rand2.end(), [&](float &i) { i = dist(rng); });
    std::for_each(rand3.begin(), rand3.end(), [&](float &i) { i = dist(rng); });
    std::for_each(rand4.begin(), rand4.end(), [&](float &i) { i = dist(rng); });
    std::for_each(rand5.begin(), rand5.end(), [&](float &i) { i = dist(rng); });
    std::for_each(rand6.begin(), rand6.end(), [&](float &i) { i = dist(rng); });
    std::for_each(orig.begin(), orig.end(), [&](float &i) { i = dist(rng); });

    // Check generated data
    /*int c = 1;
    std::cout << "Rand1" << std::endl;
    for(int i=0; i<vector_size;i++){
        std::cout << "("<<c++<<") " <<rand1[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "Rand2" << std::endl;
    for(int i=0; i<vector_size;i++){
        std::cout << rand2[i];
    }
    std::cout << std::endl;
    std::cout << "Rand3" << std::endl;
    for(int i=0; i<vector_size;i++){
        std::cout << rand3[i];
    }
    std::cout << std::endl;
    std::cout << "Rand4" << std::endl;
    for(int i=0; i<vector_size;i++){
        std::cout << rand4[i];
    }
    std::cout << std::endl;
    std::cout << "Rand5" << std::endl;
    for(int i=0; i<vector_size;i++){
        std::cout << rand5[i];
    }
    std::cout << std::endl;
    std::cout << "Rand6" << std::endl;
    for(int i=0; i<vector_size;i++){
        std::cout << rand6[i];
    }
    std::cout << std::endl;

    std::cout << "Origs" << std::endl;
    for(int j=0; j<AMOUNT; j++) {
        std::cout << "Orig " << j << std::endl;
        for(int i=0; i<vector_size;i++){
            std::cout << orig[j*vector_size+i];
        }
        std::cout << std::endl;
    }
*/
 
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
    OCL_CHECK(err, cl::Buffer buffer_in1(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, vector_size_bytes,
                                         rand1.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_in2(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, vector_size_bytes,
                                        rand2.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_in3(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, vector_size_bytes,
                                        rand3.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_in4(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, vector_size_bytes,
                                        rand4.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_in5(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, vector_size_bytes,
                                        rand5.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_in6(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, vector_size_bytes,
                                        rand6.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_in7(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, vector_size_bytes*AMOUNT,
                                         orig.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_output(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, vector_size_result_bytes*AMOUNT,
                                            proj.data(), &err));
    int amount = AMOUNT;
    int size = vector_size;
    int arg = 0;
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg++, buffer_in1));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg++, buffer_in2));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg++, buffer_in3));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg++, buffer_in4));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg++, buffer_in5));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg++, buffer_in6));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg++, buffer_in7));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg++, buffer_output));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg++, amount));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg++, size));


    // Copy input data to device global memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_in1, buffer_in2, buffer_in3, buffer_in4, buffer_in5, buffer_in6, buffer_in7}, 0 /* 0 means from host*/));

    // Launch the Kernel
    // For HLS kernels global and local size is always (1,1,1). So, it is
    // recommended
    // to always use enqueueTask() for invoking HLS kernel
    auto start = std::chrono::high_resolution_clock::now();

    OCL_CHECK(err, err = q.enqueueTask(krnl_vector_add));


    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_output}, CL_MIGRATE_MEM_OBJECT_HOST));
    q.finish();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> float_ms_1 = end - start;
    std::cout << "Kernel: " << float_ms_1.count() << " milliseconds" << std::endl;
    // OPENCL HOST CODE AREA END 

    std::vector<float> proj_host(LOWER_SIZE*AMOUNT);

    start = std::chrono::high_resolution_clock::now();
    for(int a=0; a<AMOUNT;a++) {
        float acc_1 = 0;
        float acc_2 = 0;
        float acc_3 = 0;
        float acc_4 = 0;
        float acc_5 = 0;
        float acc_6 = 0;
        for (int i=0; i<vector_size; i++){
            acc_1 += rand1[i] * orig[i+vector_size*a];
            acc_2 += rand2[i] * orig[i+vector_size*a];
            acc_3 += rand3[i] * orig[i+vector_size*a];
            acc_4 += rand4[i] * orig[i+vector_size*a];
            acc_5 += rand5[i] * orig[i+vector_size*a];
            acc_6 += rand6[i] * orig[i+vector_size*a];
        }

        proj_host[0+6*a] = acc_1;
        proj_host[1+6*a] = acc_2;
        proj_host[2+6*a] = acc_3;
        proj_host[3+6*a] = acc_4;
        proj_host[4+6*a] = acc_5;
        proj_host[5+6*a] = acc_6;
    }
    end = std::chrono::high_resolution_clock::now();
     std::chrono::duration<double, std::milli> float_ms = end - start;
    std::cout << "Host:   " << float_ms.count() << " milliseconds" << std::endl;

    std::cout << "Speedup: " << float_ms.count() / float_ms_1.count() << std::endl; 

    // Compare the results of the Device to the simulation
    bool match = true;
    std::cout.precision(7);
    std::cout << "Precision: 2e-5"<<std::endl;
    for (int i = 0; i < LOWER_SIZE*AMOUNT; i++) {

        //std::cout << "i = " << i << "\tCPU result = " << proj_host[i]
        //            << "\tDevice result = " << proj[i] << "\tDiff: " << proj_host[i] - proj[i] << std::endl;

        if (abs(proj_host[i] - proj[i]) >= 2e-5 && abs(proj_host[i] - proj[i])/proj_host[i] >= 2e-5) {
            std::cout << "Error: Result mismatch" << std::endl
            << "Diff: " << proj_host[i] - proj[i] << std::endl;
            match = false;
        }
    }

    std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;

    std::cout << "Resulted  projection (1): ";
    for(int i=0; i<LOWER_SIZE;i++){
        std::cout << proj_host[i] << "\t";
    }
    std::cout << "\n";
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
