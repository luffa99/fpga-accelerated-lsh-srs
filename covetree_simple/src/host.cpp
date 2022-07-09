/** *
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

// #include "../../common/includes/xcl2/xcl2.hpp"
#include <algorithm>
#include <vector>
#include <algorithm>
#include <iostream>
#include <random>
#include "math.h"
#include <fstream>
#include <string>
#include <iostream>
#include <unistd.h>
#include "host.hpp"

#define dimension 6 // Vector dimension in the data structure -> do NOT change
#define maxchildren 16  // Maximum number of children for every node (fixed vector)
#define n_points 100   // Number of po#defines to generate
#define dummy_level 69

/*
 * To generate the tree we call a python script
 * https://hunch.net/~jl/projects/cover_tree/cover_tree.html
 * https://github.com/ngeiswei/PyCoverTree
 * 
 * And then we parse the generated Tree + knn search
*/
void generateTree(  float points_coords [n_points][dimension],
                    int  points_children [n_points][maxchildren*2],
                    float result_py [dimension],
                    int &n_points_real,
                    float query [dimension*100],
                    int &maxlevel,
                    int &minlevel) {

    std::string str; 

    // Generate random query point
    for(int i=0; i<dimension*100; i++) {
        // This will generate a random number from 0.0 to 1.0, inclusive.
        srand(time(NULL)+i);
        query[i] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    }


    // Call python script to generate tree 
    system(("rm generated_tree_"+std::to_string(n_points)+".txt").c_str() );
    system(("python src/python/generate_covertree.py "+std::to_string(n_points)+" "+std::to_string(maxchildren)
        +" "+std::to_string(query[0])
        +" "+std::to_string(query[1])
        +" "+std::to_string(query[2])
        +" "+std::to_string(query[3])
        +" "+std::to_string(query[4])
        +" "+std::to_string(query[5])
        +" "+std::to_string(query[6])
        ).c_str() );

    std::ifstream file;
    file.open("generated_tree_"+std::to_string(n_points)+".txt");
    if(!file) { // file couldn't be opened
      std::cerr << "Error: file could not be opened: "; //<< strerror(errno) << std::endl;
      exit(1);
    }

    // Due to construction constraints (such as max. number of children) some nodes cannot 
    // be entered the tree. At the moment just ignore them
    std::getline(file, str);
    int ignored = std::stof(str);
    std::getline(file, str);
    maxlevel = std::stof(str);
    std::getline(file, str);
    minlevel = std::stof(str);

    n_points_real = n_points - ignored;

    std::cout << "==== TEST (C++) " << std::endl;
    std::cout << "Considering " << n_points_real << " points!" << std::endl;
    
    for (int i=0; i<n_points_real;i++){
        for(int j=0; j<dimension;j++){
            std::getline(file, str);
            //std::cout << str << std::endl;
            points_coords[i][j] = std::stof(str);
        }
        for(int j=0; j<maxchildren*2; j++){
            std::getline(file, str);
            points_children[i][j] = std::stoi(str);
        }
    }

    // Print parsed nodes for debug
    // for (int i=0; i<n_points_real;i++){
    //     std::cout << "Node " << i << std::endl;
    //     for(int j=0; j<dimension;j++){
    //         std::cout << points_coords[i][j] << " | ";
    //     }
    //     std::cout << std::endl;
    //     for(int j=0; j<maxchildren*2; j+=2){
    //         std::cout << "(" << points_children[i][j] << ", " << points_children[i][j+1] << "), ";
    //     }
    //     std::cout << std::endl << std::endl;
    // }

    file.close();
    file.open("results.txt");
    if(!file) { // file couldn't be opened
      std::cerr << "Error: file could not be opened: "; //<< strerror(errno) << std::endl;
      exit(1);
    }
    for (int i=0; i<dimension;i++){
        std::getline(file, str);
        result_py[i] = std::stof(str);
        // std::cout << result_py[i] << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string binaryFile = argv[1];

    // 2D array DATA to pass to the FPGA
    float points_coords [n_points][dimension] = {};
    int points_children [n_points][maxchildren*2] = {};
    // Results arrays
    float result_hw [dimension*100] = {};
    float result_py [dimension] = {};
    // Utils
    float query [dimension*100];
    int n_points_real = n_points;
    int maxlevel = dummy_level;
    int minlevel = dummy_level; 

    generateTree(points_coords, points_children, result_py, n_points_real, query, maxlevel, minlevel);


    // TODO!!!!!!!!!!!!!!!
    // size_t vector_size_bytes = sizeof(int) * dimension;
    // size_t vector_size_result_bytes = sizeof(double) * 1;
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

    // std::vector<float, aligned_allocator<float> > source_in1(dimension);
    // std::vector<float, aligned_allocator<float> > source_in2(dimension);
    // std::vector<float, aligned_allocator<float> > source_hw_results(1); // <-- let's try to make vectors of size 1 ^^
    // std::vector<float, aligned_allocator<float> > source_sw_results(1)

    // Test on CPU
    // float ans = 0;
    // for (int i = 0; i < dimension; i++) {
    //     ans += std::sqrt(((source_in1[i] + source_in2[i]) * (source_in1[i] + source_in2[i])) );
    // }
    // source_sw_results[0] = ans;
    // source_hw_results[0] = 0;
    
    // OPENCL HOST CODE AREA START
    // get_xil_devices() is a utility API which will find the xilinx
    // platforms and will return list of devices connected to Xilinx platform
    // auto devices = xcl::get_xil_devices();
    std::vector<cl::Device> devices = get_devices();

    // read_binary_file() is a utility API which will load the binaryFile
    // and will return the pointer to file buffer.
    // auto fileBuf = xcl::read_binary_file(binaryFile);
    xclbin_file_name = argv[1];
    cl::Program::Binaries vadd_bins = import_binary_file();


    // cl::Program::Binaries bins{{vadd_bins.data(), vadd_bins.size()}};
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


    // Problems with alignment: try to use aligned vectors:
    // std::vector<std::vector<float, aligned_allocator<float> >, aligned_allocator<float> > _points_coords(n_points);
    // std::vector<std::vector<float, aligned_allocator<float> >, aligned_allocator<float> > _points_children(n_points);
    // std::vector<float, aligned_allocator<float> > _query(dimension);
    // std::vector<float, aligned_allocator<float> > _result_hw(dimension);

    // for(int i=0; i<n_points; i++) {
    //     _points_coords[i] = std::vector<float, aligned_allocator<float> >(dimension);
    //     _points_children[i] = std::vector<float, aligned_allocator<float> >(maxchildren*2);

    //     for(int j=0; j<dimension; j++) {
    //         _points_coords[i][j] = points_coords[i][j];
    //     }

    //     for(int j=0; j<maxchildren*2; j++) {
    //         _points_children[i][j] = points_children[i][j];
    //     }

    // }
    // for(int i=0; i<dimension;i++){
    //     _query[i] = query[i];
    //     _result_hw[i] = result_hw[i];
    // }

    // Allocate Buffer in Global Memory
    // Buffers are allocated using CL_MEM_USE_HOST_PTR for efficient memory and
    // Device-to-host communication
    OCL_CHECK(err, cl::Buffer buffer_in1(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(float)*n_points*dimension,
                                         points_coords, &err));
    OCL_CHECK(err, cl::Buffer buffer_in2(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int)*n_points*maxchildren*2,
                                         points_children, &err));
    OCL_CHECK(err, cl::Buffer buffer_in3(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(float)*dimension,
                                         query, &err));
    OCL_CHECK(err, cl::Buffer buffer_output(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof(float)*dimension*100,
                                            result_hw, &err));

    OCL_CHECK(err, err = krnl_vector_add.setArg(0, n_points_real));
    OCL_CHECK(err, err = krnl_vector_add.setArg(1, buffer_in1));
    OCL_CHECK(err, err = krnl_vector_add.setArg(2, buffer_in2));
    OCL_CHECK(err, err = krnl_vector_add.setArg(3, buffer_in3));
    OCL_CHECK(err, err = krnl_vector_add.setArg(4, buffer_output));
    OCL_CHECK(err, err = krnl_vector_add.setArg(5, maxlevel));
    OCL_CHECK(err, err = krnl_vector_add.setArg(6, minlevel));

    // Copy input data to device global memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_in1, buffer_in2, buffer_in3}, 0 /* 0 means from host*/));

    // Launch the Kernel
    // For HLS kernels global and local size is always (1,1,1). So, it is
    // recommended
    // to always use enqueueTask() for invoking HLS kernel
    OCL_CHECK(err, err = q.enqueueTask(krnl_vector_add));

    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_output}, CL_MIGRATE_MEM_OBJECT_HOST));
    q.finish();
    // OPENCL HOST CODE AREA END

    // Compare the results of the Device to the simulation
    bool match = true;
    for (int i = 0; i < dimension; i++) { 
        if (result_hw[i] != result_py[i]) {
            std::cout << "Error: Result mismatch" << std::endl;
            std::cout << "i = " << 0 << " CPU result = " << result_py[i]
                      << " Device result = " << result_hw[i] << std::endl;
            match = false;
        }
    }

    std::cout << "Query point was:" <<std::endl;
    std::cout << "(";
    for (int i=0; i < dimension; i++){
        if (i < dimension - 1) {
            std::cout << query[i] << ", ";
        } else {
            std::cout << query[i] << ")" << std::endl;
        }
    }


    std::cout << "Found point is:" <<std::endl;
    std::cout << "[(";
    for (int i=0; i < dimension; i++){
         if (i < dimension - 1) {
            std::cout << result_hw[i] << ", ";
        } else {
            std::cout << result_hw[i] << ")]" << std::endl;
        }
    }

    std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
