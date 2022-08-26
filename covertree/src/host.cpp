#include <algorithm>
#include <random>
#include "host.hpp"

#define dimension 6     // (Projected) Vector dimension in the data structure
#define maxchildren 100 // Maximum number of children for every node (fixed vector)
#define n_points 2000   // Number of points to be in the data strucuture
#define dummy_level 69
#define MAX_VECT_SIZE 1024
#define BANK_NAME(n) n | XCL_MEM_TOPOLOGY
// memory topology:  https://www.xilinx.com/html_docs/xilinx2021_1/vitis_doc/optimizingperformance.html#utc1504034308941
// See https://github.com/WenqiJiang/Fast-Vector-Similarity-Search-on-FPGA/blob/main/FPGA-ANNS-local/generalized_attempt_K_1_12_bank_6_PE/src/host.cpp
// <id> | XCL_MEM_TOPOLOGY
// The <id> is determined by looking at the Memory Configuration section in the xxx.xclbin.info file generated next to the xxx.xclbin file.
// In the xxx.xclbin.info file, the global memory (DDR, HBM, PLRAM, etc.) is listed with an index representing the <id>.
const int bank[32] = {
    /* 0 ~ 31 HBM (256MB per channel) */
    BANK_NAME(0), BANK_NAME(1), BANK_NAME(2), BANK_NAME(3), BANK_NAME(4),
    BANK_NAME(5), BANK_NAME(6), BANK_NAME(7), BANK_NAME(8), BANK_NAME(9),
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
void generateTree(std::vector<float, aligned_allocator<float>> &points_coords,
                  std::vector<int, aligned_allocator<int>> &points_children,
                  float *result_py,
                  float *result_py_2,
                  float *result_py_3,
                  float *result_py_4,
                  int &n_points_real,
                  std::vector<float, aligned_allocator<float>> &query,
                  int &maxlevel,
                  int &minlevel,
                  int AMOUNT,
                  int want_output,
                  std::vector<float,aligned_allocator<float>> &pts,
                  int &passedTests) {

    std::string str;
    // Generate random query point and save them to file querys.txt
    std::ofstream myfile("querys.txt");
    if (!myfile.is_open()) {
        std::cerr << "Unable to open file querys.txt\n";
        exit(1);
    }
    
    // Save query points to file
    for (int i = 0; i < dimension * AMOUNT; i++)
    {
        myfile << std::to_string(query[i]) << std::endl;
    }
    myfile.close();

    // Write pts to file:
    std::ofstream myfile2("pts.txt");
    if(myfile2.is_open())
    {
    }
    else {std::cerr<<"Unable to open file pts.txt\n";exit(1);}
    for(int i=0; i<dimension*n_points_real; i+=6) {
        myfile2 << std::to_string(pts[i]) << ',' 
        << std::to_string(pts[i+1]) << ',' 
        << std::to_string(pts[i+2]) << ',' 
        << std::to_string(pts[i+3]) << ',' 
        << std::to_string(pts[i+4]) << ',' 
        << std::to_string(pts[i+5]) 
        << std::endl;
    }
    myfile2.close();

    
    // Call python script to generate tree
    system(("rm generated_tree_" + std::to_string(n_points_real) + ".txt").c_str());
    system(("python src/python/generate_covertree.py " + std::to_string(n_points_real) 
        + " " + std::to_string(maxchildren) + " " + std::to_string(want_output)).c_str());

    // Parse generated tree
    std::ifstream file;
    file.open("generated_tree_" + std::to_string(n_points_real) + ".txt");
    if (!file)
    {                                                     // file couldn't be opened
        std::cerr << "Error: file could not be opened: "; //<< strerror(errno) << std::endl;
        exit(1);
    }
    // Due to construction constraints (such as max. number of children) some nodes may
    // not be able to be inserted in the tree. Python scripts tells us how much nodes
    // Were ignored
    std::getline(file, str);
    int ignored = std::stof(str);
    std::getline(file, str);
    maxlevel = std::stof(str);
    std::getline(file, str);
    minlevel = std::stof(str);
    n_points_real = n_points_real - ignored;

    for (int i = 0; i < n_points_real; i++)
    {
        for (int j = 0; j < dimension; j++)
        {
            std::getline(file, str);
            points_coords[i * dimension + j] = std::stof(str);
        }
        for (int j = 0; j < maxchildren * 2; j++)
        {
            std::getline(file, str);
            points_children[i * maxchildren * 2 + j] = std::stoi(str);
        }
    }

    file.close();

    // Parse results of 1-NN queries done by python
    file.open("results.txt");
    if (!file)
    {                                                     
        std::cerr << "Error: file could not be opened: "; 
        exit(1);
    }
    for (int i = 0; i < dimension * AMOUNT; i++)
    {
        std::getline(file, str);
        result_py[i] = std::stof(str);
    }
    std::getline(file, str);
    passedTests = std::stof(str);
    file.close();

    // Also parse 2,3,4-NN (for analysis)
    file.open("results_2.txt");
    if (!file)
    {                                                     
        std::cerr << "Error: file could not be opened: "; 
        exit(1);
    }
    for (int i = 0; i < dimension * AMOUNT; i++)
    {
        std::getline(file, str);
        result_py_2[i] = std::stof(str);
    }
    file.close();
    file.open("results_3.txt");
    if (!file)
    {                                                    
        std::cerr << "Error: file could not be opened: ";
        exit(1);
    }
    for (int i = 0; i < dimension * AMOUNT; i++)
    {
        std::getline(file, str);
        result_py_3[i] = std::stof(str);
    }
    file.close();
    file.open("results_4.txt");
    if (!file)
    {                                                    
        std::cerr << "Error: file could not be opened: "; 
        exit(1);
    }
    for (int i = 0; i < dimension * AMOUNT; i++)
    {
        std::getline(file, str);
        result_py_4[i] = std::stof(str);
    }
    file.close();
}

// Projection on host side
void host_projection(std::vector<float, aligned_allocator<float>> &rand1,
                     std::vector<float, aligned_allocator<float>> &rand2,
                     std::vector<float, aligned_allocator<float>> &rand3,
                     std::vector<float, aligned_allocator<float>> &rand4,
                     std::vector<float, aligned_allocator<float>> &rand5,
                     std::vector<float, aligned_allocator<float>> &rand6,
                     std::vector<float, aligned_allocator<float>> &orig,
                     std::vector<float, aligned_allocator<float>> &proj_host,
                     int vector_size,
                     int AMOUNT)
{
    for (int a = 0; a < AMOUNT; a++)
    {
        float acc_1 = 0;
        float acc_2 = 0;
        float acc_3 = 0;
        float acc_4 = 0;
        float acc_5 = 0;
        float acc_6 = 0;
        for (int i = 0; i < vector_size; i++)
        {
            acc_1 += rand1[i] * orig[i + vector_size * a];
            acc_2 += rand2[i] * orig[i + vector_size * a];
            acc_3 += rand3[i] * orig[i + vector_size * a];
            acc_4 += rand4[i] * orig[i + vector_size * a];
            acc_5 += rand5[i] * orig[i + vector_size * a];
            acc_6 += rand6[i] * orig[i + vector_size * a];
        }

        proj_host[0 + 6 * a] = acc_1;
        proj_host[1 + 6 * a] = acc_2;
        proj_host[2 + 6 * a] = acc_3;
        proj_host[3 + 6 * a] = acc_4;
        proj_host[4 + 6 * a] = acc_5;
        proj_host[5 + 6 * a] = acc_6;
    }
}

// The distance function (euclidian distance)
float distance(float *in1, float *in2)
{
    float ans = 0;
    for (int i = 0; i < dimension; i++)
    {
        float a = in1[i];
        float b = in2[i];
        ans = ans + ((a - b) * (a - b));
    }
    return sqrt(ans);
}

// Compute average and minimum distance between vectors
void distances(std::vector<std::reference_wrapper<std::vector<float, aligned_allocator<float>>>> &rands, float *res)
{
    float avg = 0;
    float max = 0;
    float min = 999999;
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < i; j++)
        {
            auto d = distance(rands[i].get().data(), rands[j].get().data());
            avg += d;
            max = d > max ? d : max;
            min = d < min ? d : min;
        }
    }
    avg /= 15;
    res[0] = avg;
    res[1] = min;
}

int main(int argc, char **argv)
{   
    // Parsing and checking arguments
    if (argc != 6)
    {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>  <Number of points in the tree> "
        << "<Amount of vectors to project+search> <Dimension of vectors> <output 1/0>" << std::endl;
        return EXIT_FAILURE;
    }

    int n_points_real = atoi(argv[2]);  // Number of points we want to have in the tree
    int AMOUNT = atoi(argv[3]);         // Number of queries
    int vector_size = atoi(argv[4]);    // Size of vectors in the original space
    int want_output = atoi(argv[5]);    // For output

    if (n_points_real > n_points  || n_points_real < 0)
    {
        std::cout << "Invalid number of points. Value must be >0 and <" << n_points * 10 
        << ". You gave " << n_points_real << std::endl;
        return EXIT_FAILURE;
    }
    if (vector_size % 16 != 0 || vector_size < 16 || vector_size > MAX_VECT_SIZE)
    {
        std::cout << "Size of vectors must be > 16 and < " << MAX_VECT_SIZE 
        << " and divisible by 16! You gave " << argv[4] << std::endl;
        return EXIT_FAILURE;
    }
    if (AMOUNT < 1)
    {
        std::cout << "The amount of vectors must be > 0. You gave " << argv[3] << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Parameters: AMOUNT=" << AMOUNT << " | n_points_real=" << n_points_real 
    << " | amount=" << AMOUNT << " | dim(orig)=" << vector_size << " | dim(dest)=6" << std::endl;
    std::string binaryFile = argv[1];

    // Declare vectors and parameters
    float result_py[dimension * AMOUNT] = {};
    float result_py_2[dimension * AMOUNT] = {};
    float result_py_3[dimension * AMOUNT] = {};
    float result_py_4[dimension * AMOUNT] = {};
    std::vector<int, aligned_allocator<int>> points_children(n_points_real * maxchildren * 2);
    std::vector<float, aligned_allocator<float>> result_hw(dimension * AMOUNT);
    std::vector<float, aligned_allocator<float>> result_hw_2(dimension * AMOUNT);
    std::vector<float, aligned_allocator<float>> points_coords(n_points_real * dimension);
    std::vector<float, aligned_allocator<float>> rand1(vector_size);
    std::vector<float, aligned_allocator<float>> rand2(vector_size);
    std::vector<float, aligned_allocator<float>> rand3(vector_size);
    std::vector<float, aligned_allocator<float>> rand4(vector_size);
    std::vector<float, aligned_allocator<float>> rand5(vector_size);
    std::vector<float, aligned_allocator<float>> rand6(vector_size);
    std::vector<float, aligned_allocator<float>> orig(vector_size * AMOUNT);
    std::vector<float, aligned_allocator<float>> query(dimension * AMOUNT);
    std::vector<float, aligned_allocator<float> > pts_orig(vector_size*n_points_real);   // Points of the data structure, to be first projected
    std::vector<float, aligned_allocator<float> > pts(dimension*n_points_real);   // Points of the data structure, to be first projected
    int maxlevel = dummy_level;
    int minlevel = dummy_level;
    int passedTests = 0;

    // Sizes
    size_t vector_size_bytes = sizeof(float) * vector_size;
    size_t vector_size_result_bytes = sizeof(float) * dimension;

    // Generate test data: generate random random vectors and original vectors
    // std::vector<std::reference_wrapper<std::vector<float, aligned_allocator<float>>>> 
    //     rands = {rand1, rand2, rand3, rand4, rand5, rand6};
    // srand(time(NULL));
    // float tshold_avg = 1.25;
    // float tshold_min = 1.00;
    // float res[2] = {0.0, 0.0};
    // // Want to ensure that random vectors are "good distributed", pretty distant each other
    // do
    // {
    //     std::for_each(rand1.begin(), rand1.end(), [&](float &i)
    //                   { i = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); });
    //     std::for_each(rand2.begin(), rand2.end(), [&](float &i)
    //                   { i = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); });
    //     std::for_each(rand3.begin(), rand3.end(), [&](float &i)
    //                   { i = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); });
    //     std::for_each(rand4.begin(), rand4.end(), [&](float &i)
    //                   { i = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); });
    //     std::for_each(rand5.begin(), rand5.end(), [&](float &i)
    //                   { i = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); });
    //     std::for_each(rand6.begin(), rand6.end(), [&](float &i)
    //                   { i = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); });
    //     // std::cout << distances(rands) << std::endl;
    //     distances(rands, res);
    // } while (res[0] < tshold_avg || res[1] < tshold_min);
    // std::cout << "D: " << res[0] << "/" << res[1] << std::endl;

    // std::for_each(orig.begin(), orig.end(), [&](float &i)
    //     { i = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) / pow(10, floor(log10(vector_size))); 
    // });
    
    // Trying normal distribution!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::normal_distribution<> d{0,1};
    std::for_each(rand1.begin(), rand1.end(), [&](float &i)
                    { i = d(gen); });
    std::for_each(rand2.begin(), rand2.end(), [&](float &i)
                    { i = d(gen); });
    std::for_each(rand3.begin(), rand3.end(), [&](float &i)
                    { i = d(gen); });
    std::for_each(rand4.begin(), rand4.end(), [&](float &i)
                    { i = d(gen); });
    std::for_each(rand5.begin(), rand5.end(), [&](float &i)
                    { i = d(gen); });
    std::for_each(rand6.begin(), rand6.end(), [&](float &i)
                    { i = d(gen); });
    std::for_each(orig.begin(), orig.end(), [&](float &i)
                    { i = d(gen); });
    std::for_each(pts_orig.begin(), pts_orig.end(), [&](float &i)
                    { i = d(gen); });


    // Execute projection on host and measure time
    float host_projection_ms = 0.0;
    auto start = std::chrono::high_resolution_clock::now();
    host_projection(rand1, rand2, rand3, rand4, rand5, rand6, orig, query, vector_size, AMOUNT);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> duration_host_projection = end - start;
    host_projection_ms = host_projection_ms + duration_host_projection.count();

    // Project data to be inserted into the coverTree
    host_projection(rand1, rand2, rand3, rand4, rand5, rand6, pts_orig, pts, vector_size, n_points_real);    

    // Print generated data
    if (want_output)
    {
        int c = 1;
        std::cout << "Rand1" << std::endl;
        for (int i = 0; i < vector_size; i++)
        {
            std::cout << "(" << c++ << ") " << rand1[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "Rand2" << std::endl;
        for (int i = 0; i < vector_size; i++)
        {
            std::cout << rand2[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "Rand3" << std::endl;
        for (int i = 0; i < vector_size; i++)
        {
            std::cout << rand3[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "Rand4" << std::endl;
        for (int i = 0; i < vector_size; i++)
        {
            std::cout << rand4[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "Rand5" << std::endl;
        for (int i = 0; i < vector_size; i++)
        {
            std::cout << rand5[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "Rand6" << std::endl;
        for (int i = 0; i < vector_size; i++)
        {
            std::cout << rand6[i] << " ";
        }
        std::cout << std::endl;

        std::cout << "Origs" << std::endl;
        for (int j = 0; j < AMOUNT; j++)
        {
            std::cout << "Orig " << j << std::endl;
            for (int i = 0; i < vector_size; i++)
            {
                std::cout << orig[j * vector_size + i] << " ";
            }
            std::cout << std::endl;
        }
    }

    // Generate the try
    int wanted_points = n_points_real;
    generateTree(points_coords, points_children, result_py, result_py_2, result_py_3, result_py_4, 
        n_points_real, query, maxlevel, minlevel, AMOUNT, want_output, pts, passedTests);

    // // Debug: check max children 
    // int maxj = 0;
    // for(int i=0; i<n_points_real; i++){
    //     for(int j=0; j<maxchildren*2; j+=2) {
    //         // std::cout << points_children[i][j] << " ";
    //         if(points_children[i*maxchildren*2+j] == 69){
    //             break;
    //         }
    //         maxj = j > maxj ? j : maxj;
    //     }
    // }
    // maxj = maxj/2 + 1;
    // std::cout << "Max children: " << maxj << std::endl;
    // std::string filename_1("../test_and_performance/maxchildren.csv");
    // std::ofstream file_append_1;
    // file_append_1.open(filename_1, std::ios_base::app);
    // file_append_1 << maxj << std::endl;
    // return 1;

    // PROGRAM DEVICE
    cl_int err;
    cl::Context context;
    cl::Kernel krnl_vector_add;
    cl::CommandQueue q;
    // OPENCL HOST CODE AREA START  ////////////////////////////////////////////////////////////////
    std::vector<cl::Device> devices = get_devices();
    xclbin_file_name = argv[1];
    cl::Program::Binaries vadd_bins = import_binary_file();
    bool valid_device = false;
    auto device = devices[0];
    for (unsigned int i = 0; i < devices.size(); i++)
    {
        device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = cl::Context(device, nullptr, nullptr, nullptr, &err));
        OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
        std::cout << "Trying to program device[" << i << "]: " 
        << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        cl::Program program(context, {device}, vadd_bins, nullptr, &err);
        if (err != CL_SUCCESS)
        {
            std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
        }
        else
        {
            std::cout << "Device[" << i << "]: program successful!\n";
            OCL_CHECK(err, krnl_vector_add = cl::Kernel(program, "vadd", &err));
            valid_device = true;
            break; // we break because we found a valid device
        }
    }
    if (!valid_device)
    {
        std::cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }

    //////////////////////////////   TEMPLATE START  //////////////////////////////
    cl_mem_ext_ptr_t
        buffer_in1Ext,
        buffer_in2Ext,
        buffer_in3Ext,
        buffer_in4Ext,
        buffer_in5Ext,
        buffer_in6Ext,
        buffer_in7Ext,
        buffer_in8Ext,
        buffer_in9Ext,
        buffer_outputExt,
        buffer_output2Ext;
    //////////////////////////////   TEMPLATE END  //////////////////////////////
    //////////////////////////////   TEMPLATE START  //////////////////////////////
    buffer_in1Ext.obj = points_coords.data();
    buffer_in1Ext.param = 0;
    buffer_in1Ext.flags = bank[0];

    buffer_in2Ext.obj = points_children.data();
    buffer_in2Ext.param = 0;
    buffer_in2Ext.flags = bank[1];

    buffer_in3Ext.obj = rand1.data();
    buffer_in3Ext.param = 0;
    buffer_in3Ext.flags = bank[2];

    buffer_in4Ext.obj = rand2.data();
    buffer_in4Ext.param = 0;
    buffer_in4Ext.flags = bank[3];

    buffer_in5Ext.obj = rand3.data();
    buffer_in5Ext.param = 0;
    buffer_in5Ext.flags = bank[4];

    buffer_in6Ext.obj = rand4.data();
    buffer_in6Ext.param = 0;
    buffer_in6Ext.flags = bank[5];

    buffer_in7Ext.obj = rand5.data();
    buffer_in7Ext.param = 0;
    buffer_in7Ext.flags = bank[6];

    buffer_in8Ext.obj = rand6.data();
    buffer_in8Ext.param = 0;
    buffer_in8Ext.flags = bank[7];

    buffer_in9Ext.obj = orig.data();
    buffer_in9Ext.param = 0;
    buffer_in9Ext.flags = bank[8];

    buffer_outputExt.obj = result_hw.data();
    buffer_outputExt.param = 0;
    buffer_outputExt.flags = bank[9];

    buffer_output2Ext.obj = result_hw_2.data();
    buffer_output2Ext.param = 0;
    buffer_output2Ext.flags = bank[10];

    OCL_CHECK(err, cl::Buffer buffer_in1(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY 
            | CL_MEM_EXT_PTR_XILINX, sizeof(float) * n_points_real * dimension, &buffer_in1Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_in2(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY 
            | CL_MEM_EXT_PTR_XILINX, sizeof(int) * n_points_real * maxchildren * 2, &buffer_in2Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_in3(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY 
            | CL_MEM_EXT_PTR_XILINX, vector_size_bytes, &buffer_in3Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_in4(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY 
            | CL_MEM_EXT_PTR_XILINX, vector_size_bytes, &buffer_in4Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_in5(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY 
            | CL_MEM_EXT_PTR_XILINX, vector_size_bytes, &buffer_in5Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_in6(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY 
            | CL_MEM_EXT_PTR_XILINX, vector_size_bytes, &buffer_in6Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_in7(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY 
            | CL_MEM_EXT_PTR_XILINX, vector_size_bytes, &buffer_in7Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_in8(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY 
            | CL_MEM_EXT_PTR_XILINX, vector_size_bytes, &buffer_in8Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_in9(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY 
            | CL_MEM_EXT_PTR_XILINX, vector_size_bytes * AMOUNT, &buffer_in9Ext, &err));

    OCL_CHECK(err, cl::Buffer buffer_output(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY 
            | CL_MEM_EXT_PTR_XILINX, sizeof(float) * dimension * AMOUNT, &buffer_outputExt, &err));
    OCL_CHECK(err, cl::Buffer buffer_output_2(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY 
            | CL_MEM_EXT_PTR_XILINX, sizeof(float) * dimension * AMOUNT, &buffer_output2Ext, &err));

    int narg = 0;
    std::cout << "max/min/n_points_real/AMOUNT/vector_size" << maxlevel << "/"
        << minlevel << "/" << n_points_real << "/" << AMOUNT << "/" << vector_size << std::endl;
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, buffer_in1));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, buffer_in2));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, buffer_in3));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, buffer_in4));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, buffer_in5));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, buffer_in6));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, buffer_in7));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, buffer_in8));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, buffer_in9));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, buffer_output));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, buffer_output_2));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, n_points_real));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, maxlevel));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, minlevel));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, AMOUNT));
    OCL_CHECK(err, err = krnl_vector_add.setArg(narg++, vector_size));

    // Copy input data to device global memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_in1, buffer_in2, buffer_in3, buffer_in4,
                                                     buffer_in5, buffer_in6, buffer_in7, buffer_in8, 
                                                     buffer_in9},
                                                    0 /* 0 means from host*/));

    // Launch the Kernel AND MEASURE TIME
    start = std::chrono::high_resolution_clock::now();
    OCL_CHECK(err, err = q.enqueueTask(krnl_vector_add));
    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_output, buffer_output_2}, CL_MIGRATE_MEM_OBJECT_HOST));
    q.finish();
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> kernel_all_ms = end - start;

    // OPENCL HOST CODE AREA END    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    // Get time of python execution
    std::ifstream file;
    std::string str;
    file.open("time.txt");
    if (!file)
    {                                                     // file couldn't be opened
        std::cerr << "Error: file could not be opened: "; //<< strerror(errno) << std::endl;
        exit(1);
    }
    std::getline(file, str);
    float host_all_ms = host_projection_ms + std::stof(str);

    // Check Projected vectors!
    int nonmproj5 = 0;
    for (int i = 0; i < dimension * AMOUNT; i++)
    {
        if (abs(query[i] - result_hw_2[i]) >= 2e-5 && abs(query[i] - result_hw_2[i]) / query[i] >= 2e-5)
        {
            std::cout << "Error: Result mismatch " << i << std::endl
                      << query[i] << " | " << result_hw_2[i] << " "
                      << "Diff: " << query[i] - result_hw_2[i] << std::endl;
            nonmproj5 += 1;
        }
    }

    // Compare the results of the Device to the host
    bool match = true;
    bool match_2 = true;
    bool match_3 = true;
    bool match_4 = true;
    int nonmatching_1 = 0;
    int nonmatching_4 = 0;
    for (int i = 0; i < dimension * AMOUNT; i++)
    {
        if (result_hw[i] != result_py[i])
        {
            std::cout << "Error: Result mismatch | "
                      << "i = " << i << " CPU result = " << result_py[i]
                      << " Device result = " << result_hw[i] << std::endl;
            match = false;
            nonmatching_1 += 1;
            if (result_hw[i] != result_py_2[i])
            {
                match_2 = false;
                if (result_hw[i] != result_py_3[i])
                {
                    match_3 = false;
                    if (result_hw[i] != result_py_4[i])
                    {
                        match_4 = false;
                        nonmatching_4 += 1;
                    }
                }
            }
        }
    }

    // Printing
    if (want_output)
    {
        for (int j = 0; j < AMOUNT; j++)
        {
            std::cout << "Query point (" << j << ") was:" << std::endl;
            std::cout << "(";
            for (int i = 0; i < dimension; i++)
            {
                if (i < dimension - 1)
                {
                    std::cout << query[i + j * dimension] << ", ";
                }
                else
                {
                    std::cout << query[i + j * dimension] << ")" << std::endl;
                }
            }

            std::cout << "Query point HW(" << j << ") was:" << std::endl;
            std::cout << "(";
            for (int i = 0; i < dimension; i++)
            {
                if (i < dimension - 1)
                {
                    std::cout << result_hw_2[i + j * dimension] << ", ";
                }
                else
                {
                    std::cout << result_hw_2[i + j * dimension] << ")" << std::endl;
                }
            }

            std::cout << "Found point (" << j << ") is:" << std::endl;
            std::cout << "[(";
            for (int i = 0; i < dimension; i++)
            {
                if (i < dimension - 1)
                {
                    std::cout << result_hw[i + j * dimension] << ", ";
                }
                else
                {
                    std::cout << result_hw[i + j * dimension] << ")]" << std::endl;
                }
            }
        }
    }


    // Testing Naive NN:
    start = std::chrono::high_resolution_clock::now();
    std::vector<float> min_orig(AMOUNT*vector_size);
    for(int i=0; i<vector_size*AMOUNT; i += vector_size) {

        std::vector<float> query_vect(vector_size);
        for(int o=0; o<vector_size; o++){
            query_vect[o] = orig[i+o];
        }

        float min = 999999;
        float min_id = -1;

        for(int j=0; j<vector_size*n_points_real; j+= vector_size) {
            std::vector<float> point_vect(vector_size);
            for(int o=0; o<vector_size; o++){
                point_vect[o] = pts_orig[j+o];
            }

            
            float dist = distance(query_vect.data(), point_vect.data());
            min = dist < min ? dist : min;
            min_id = dist <= min ? j : min_id;
        }
        // Save found NN
        for(int j=0; j<vector_size; j++){
            min_orig[i+j] = pts_orig[min_id+j];
        }

    }
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> naive_knn = end - start;


    // Output info
    std::cout << std::endl
              << "HOST TEST (Python algo = Bruteforce) => " 
              << ((passedTests < AMOUNT) ? "FAILED" : "PASSED") << std::endl;
    std::cout << "| Failed: " << AMOUNT - passedTests << std::endl;

    std::cout << "KERNEL(ALL) PROJ TEST => " << (nonmproj5 == 0 ? "PASSED" : "FAILED") << std::endl;
    std::cout << "| Failed: " << nonmproj5 << std::endl;

    std::cout << "KERNEL(ALL) CT TEST => " << (match ? "PASSED" : "FAILED") << std::endl;
    std::cout << "| Failed: " << nonmatching_1 / 6 << std::endl;

    std::cout << "Kernel (ALL) tot time: " << kernel_all_ms.count() << " ms" << std::endl;
    std::cout << "Host tot time: " << host_all_ms << " ms" << std::endl;

    std::cout << "Host Naive tot time: " << naive_knn.count() << " ms" << std::endl;

    std::cout << "Speedup: " << host_all_ms/kernel_all_ms.count() << std::endl;
    std::cout << "Naive vs Host(ALL) speedup: " << naive_knn.count()/host_all_ms << std::endl;
    std::cout << "Naive vs Kernel(ALL) speedup: " << naive_knn.count()/kernel_all_ms.count() << std::endl;
    // Export data to file
    time_t now = time(0);
    char *dt = ctime(&now);
    std::string dts = dt;
    dts.erase(std::remove(dts.begin(), dts.end(), '\n'), dts.cend());
    std::string filename("../test_and_performance/data.csv");
    std::ofstream file_append;
    file_append.open(filename, std::ios_base::app);
    file_append << nonmproj5 << ';'
        << match << ';' << match_2 << ';' << match_3 << ';' << match_4 << ';' 
        << nonmatching_1 / 6 << ';' << nonmatching_4 / 6 << ';'
        << dts << ';' << now << ';'
        << AMOUNT << ';' << n_points_real << ';' << wanted_points <<';' << vector_size << ';' 
        << kernel_all_ms.count() << ';' << host_all_ms << ';' 
        << host_all_ms / kernel_all_ms.count()  << ';'
        << naive_knn.count()/host_all_ms << ';'
        << naive_knn.count()/kernel_all_ms.count() << std::endl;

    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
