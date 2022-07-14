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
#define maxchildren 128  // Maximum number of children for every node (fixed vector)
#define n_points 1000   // Number of po#defines to generate
#define dummy_level 69

#define BANK_NAME(n) n | XCL_MEM_TOPOLOGY
// memory topology:  https://www.xilinx.com/html_docs/xilinx2021_1/vitis_doc/optimizingperformance.html#utc1504034308941
// See https://github.com/WenqiJiang/Fast-Vector-Similarity-Search-on-FPGA/blob/main/FPGA-ANNS-local/generalized_attempt_K_1_12_bank_6_PE/src/host.cpp
// <id> | XCL_MEM_TOPOLOGY
// The <id> is determined by looking at the Memory Configuration section in the xxx.xclbin.info file generated next to the xxx.xclbin file. 
// In the xxx.xclbin.info file, the global memory (DDR, HBM, PLRAM, etc.) is listed with an index representing the <id>.

/* for U280 specifically */
const int bank[32] = {
    /* 0 ~ 31 HBM (256MB per channel) */
    BANK_NAME(0),  BANK_NAME(1),  BANK_NAME(2),  BANK_NAME(3),  BANK_NAME(4),
    BANK_NAME(5),  BANK_NAME(6),  BANK_NAME(7),  BANK_NAME(8),  BANK_NAME(9),
    BANK_NAME(10), BANK_NAME(11), BANK_NAME(12), BANK_NAME(13), BANK_NAME(14),
    BANK_NAME(15), BANK_NAME(16), BANK_NAME(17), BANK_NAME(18), BANK_NAME(19),
    BANK_NAME(20), BANK_NAME(21), BANK_NAME(22), BANK_NAME(23), BANK_NAME(24),
    BANK_NAME(25), BANK_NAME(26), BANK_NAME(27), BANK_NAME(28), BANK_NAME(29),
    BANK_NAME(30), BANK_NAME(31)};




/*
 * To generate the tree we call a python script
 * https://hunch.net/~jl/projects/cover_tree/cover_tree.html
 * https://github.com/ngeiswei/PyCoverTree
 * 
 * And then we parse the generated Tree + knn search
*/
void generateTree(  std::vector<float,aligned_allocator<float>> &points_coords,
                    std::vector<int,aligned_allocator<int>>  &points_children,
                    float * result_py,
                    int &n_points_real,
                    std::vector<float,aligned_allocator<float>> &query,
                    int &maxlevel,
                    int &minlevel,
                    int n_query,
                    int want_output) {

    std::string str; 

    // Generate random query point and save them to file querys.txt

    std::ofstream myfile("querys.txt");

    if(myfile.is_open())
    {
       
    }
    else std::cerr<<"Unable to open file";
    for(int i=0; i<dimension*n_query; i++) {
        // This will generate a random number from 0.0 to 1.0, inclusive.
        srand(time(NULL)+i);
        query[i] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        myfile<<std::to_string(query[i]) << std::endl;
    }
        
    myfile.close();

    
    // Call python script to generate tree 
    system(("rm generated_tree_"+std::to_string(n_points_real)+".txt").c_str() );
    system(("python src/python/generate_covertree.py "+std::to_string(n_points_real)+" "+std::to_string(maxchildren)+" "+std::to_string(want_output)).c_str() );


    std::ifstream file;
    file.open("generated_tree_"+std::to_string(n_points_real)+".txt");
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

    n_points_real = n_points_real - ignored;

    std::cout << "==== TEST (C++) " << std::endl;
    std::cout << "Considering " << n_points_real << " points!" << std::endl;
    
    for (int i=0; i<n_points_real;i++){
        for(int j=0; j<dimension;j++){
            std::getline(file, str);
            //std::cout << str << std::endl;
            points_coords[i*dimension+j] = std::stof(str);
        }
        for(int j=0; j<maxchildren*2; j++){
            std::getline(file, str);
            points_children[i*maxchildren*2+j] = std::stoi(str);
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
    for (int i=0; i<dimension*n_query;i++){
        std::getline(file, str);
        result_py[i] = std::stof(str);
        // std::cout << result_py[i] << std::endl;
    }
}

int main(int argc, char** argv) {
    int n_query = 10;
    int n_points_real = 100;

    if (argc != 5) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File> <Number of points in the tree> <Number of test querys> <output 1/0>" << std::endl;
        return EXIT_FAILURE;
    }

    n_points_real = atoi(argv[2]);
    n_query = atoi(argv[3]);
    int want_output = atoi(argv[4]);

    if(n_points_real > n_points || n_points_real < 0) {
        std::cout << "Invalid number of points. Value must be >0 and <" << n_points<<". You gave "<<n_points_real<<std::endl;
         return EXIT_FAILURE;
    }
    if(n_query < 0) {
        std::cout << "Invalid number of querys. Value must be >0. You gave "<<n_query<<std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Parameters: n_query="<<n_query<<" | n_points_real=" << n_points_real <<std::endl;


    std::string binaryFile = argv[1];

    // 2D array DATA to pass to the FPGA
    // float points_coords_2 [n_points][dimension] = {};
    // int points_children_2 [n_points][maxchildren*2] = {};
    // Results arrays
    // float result_hw [dimension*100] = {};
    std::vector<float,aligned_allocator<float>> result_hw (dimension*n_query);
    float result_py [dimension*n_query] = {};
    // Utils
    // float query [dimension*100];
    std::vector<float,aligned_allocator<float>> query (dimension*n_query);
    int maxlevel = dummy_level;
    int minlevel = dummy_level; 


    // Convert to 1d arrays...
    // float points_coords [n_points*dimension] = {};
    // int points_children [n_points*maxchildren*2] = {};
    std::vector<float,aligned_allocator<float>> points_coords(n_points_real*dimension);
    std::vector<int,aligned_allocator<int>> points_children(n_points_real*maxchildren*2);

    generateTree(points_coords, points_children, result_py, n_points_real, query, maxlevel, minlevel, n_query, want_output);

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

     //////////////////////////////   TEMPLATE START  //////////////////////////////

        cl_mem_ext_ptr_t 
            buffer_in1Ext,
            buffer_in2Ext,
            buffer_in3Ext,
            buffer_outputExt;
    //////////////////////////////   TEMPLATE END  //////////////////////////////

    //////////////////////////////   TEMPLATE START  //////////////////////////////
        buffer_in1Ext.obj = points_coords.data();
        buffer_in1Ext.param = 0;
        buffer_in1Ext.flags = bank[0];

        buffer_in2Ext.obj = points_children.data();
        buffer_in2Ext.param = 0;
        buffer_in2Ext.flags = bank[1];

        buffer_in3Ext.obj = query.data();
        buffer_in3Ext.param = 0;
        buffer_in3Ext.flags = bank[2];

        buffer_outputExt.obj = result_hw.data();
        buffer_outputExt.param = 0;
        buffer_outputExt.flags = bank[3];

    // Allocate Buffer in Global Memory
    // Buffers are allocated using CL_MEM_USE_HOST_PTR for efficient memory and
    // Device-to-host communication
    OCL_CHECK(err, cl::Buffer buffer_in1(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(float)*n_points_real*dimension,
                                         &buffer_in1Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_in2(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(int)*n_points_real*maxchildren*2,
                                         &buffer_in2Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_in3(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(float)*dimension*n_query,
                                         &buffer_in3Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_output(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(float)*dimension*n_query,
                                            &buffer_outputExt, &err));

   


    OCL_CHECK(err, err = krnl_vector_add.setArg(0, n_points_real));
    OCL_CHECK(err, err = krnl_vector_add.setArg(1, buffer_in1));
    OCL_CHECK(err, err = krnl_vector_add.setArg(2, buffer_in2));
    OCL_CHECK(err, err = krnl_vector_add.setArg(3, buffer_in3));
    OCL_CHECK(err, err = krnl_vector_add.setArg(4, buffer_output));
    OCL_CHECK(err, err = krnl_vector_add.setArg(5, maxlevel));
    OCL_CHECK(err, err = krnl_vector_add.setArg(6, minlevel));
    OCL_CHECK(err, err = krnl_vector_add.setArg(7, n_query));

    // Copy input data to device global memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_in1, buffer_in2, buffer_in3}, 0 /* 0 means from host*/));

    // Launch the Kernel
    // For HLS kernels global and local size is always (1,1,1). So, it is
    // recommended
    // to always use enqueueTask() for invoking HLS kernel
    auto start = std::chrono::high_resolution_clock::now();

    OCL_CHECK(err, err = q.enqueueTask(krnl_vector_add));


    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_output}, CL_MIGRATE_MEM_OBJECT_HOST));
    q.finish();
    // OPENCL HOST CODE AREA END

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> float_ms = end - start;
    std::cout << "Kernel time: " << float_ms.count() << " milliseconds" << std::endl;

    // Get time of python execution
    std::ifstream file;
    std::string str; 
    file.open("time.txt");
    if(!file) { // file couldn't be opened
      std::cerr << "Error: file could not be opened: "; //<< strerror(errno) << std::endl;
      exit(1);
    }

    float time = 0.0;
    std::getline(file, str);
    time = std::stof(str);
    std::cout << "Python time: " << time <<std::endl; 

    // Compare the results of the Device to the simulation
    bool match = true;
    for (int i = 0; i < dimension*n_query; i++) { 
        if (result_hw[i] != result_py[i]) {
            std::cout << "Error: Result mismatch" << std::endl;
            std::cout << "i = " << 0 << " CPU result = " << result_py[i]
                      << " Device result = " << result_hw[i] << std::endl;
            match = false;
        }
    }

    if(want_output) {
        for(int j=0; j<n_query;j++) {
            std::cout << "Query point (" << j << ") was:" <<std::endl;
            std::cout << "(";
            for (int i=0; i < dimension; i++){
                if (i < dimension - 1) {
                    std::cout << query[i+j*dimension] << ", ";
                } else {
                    std::cout << query[i+j*dimension] << ")" << std::endl;
                }
            }


            std::cout << "Found point (" << j << ") is:" <<std::endl;
            std::cout << "[(";
            for (int i=0; i < dimension; i++){
                if (i < dimension - 1) {
                    std::cout << result_hw[i+j*dimension] << ", ";
                } else {
                    std::cout << result_hw[i+j*dimension] << ")]" << std::endl;
                }
            }
        }
    }
    std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
