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
// #include <hls_vector.h> 
// #include <hls_stream.h>
#include "assert.h"
// #include "hls_math.h"

#define dimension 6 // Vector dimension in the data structure -> do NOT change
#define maxchildren 128  // Maximum number of children for every node (fixed vector)
#define n_points 1000   // Number of po#defines to generate
#define dummy_level 69


// #define _minlevel -4
// #define _maxlevel 2


float distance( float const in1[dimension],
                float const in2[dimension]) {

    float ans = 0;
    distance_loop:
    for (int i = 0; i < dimension; i++) {
    #pragma HLS unroll
        float a = in1[i];
        float b = in2[i];
        ans = ans + ( ( a-b ) * (a-b) ); 
    }

    return sqrt(ans); 
}



void vadd2(int const n_points_real,
        const float * points_coords_dram,
        const int * points_children_dram,   
        const float * querys,
        float * outs,
        int maxlevel,
        int minlevel,
        int n_query) {

        std::cout << n_points_real << points_coords_dram << points_children_dram << querys << outs << maxlevel << minlevel << n_query <<std::endl;


    // Memory mapping    
    std::cout << "bmap: "  << std::endl;
    float points_coords [n_points][dimension];
    int points_children [n_points][maxchildren*2];

    // std::cout << "amap: "  << std::endl;

    // Copying data structure in on-chip memory:
    for(int i=0; i<n_points_real; i++){
        for(int j=0; j<dimension; j++){
            points_coords[i][j] = points_coords_dram[i*dimension+j];
            // std::cout << "D1: " << i*dimension+j << std::endl;

        }
        for(int j=0; j<maxchildren*2; j++) {
            points_children[i][j] = points_children_dram[i*maxchildren*2+j];
            // std::cout << "D2: " << i*maxchildren*2+j << std::endl;
        }
    }


    // Debug: check max children 
    // int maxj = 0;
    // for(int i=0; i<n_points_real; i++){
    //     for(int j=0; j<maxchildren*2; j+=2) {
    //         // std::cout << points_children[i][j] << " ";
    //         if(points_children[i][j] == 69){
    //             break;
    //         }
    //         maxj = j > maxj ? j : maxj;
    //     }
    // }
    // maxj = maxj/2 + 1;
    // std::cout << "Max children: " << maxj << std::endl;

    // std::cout << "Points coords is: " <<std::endl;
    // for(int i=0; i< n_points; i++) {
    //     for(int j=0; j< dimension;j++) {
    //         std::cout << points_coords[i][j] << " ";
    //     }
    // }
    int max_qp = 0;
    // Search n_query query points!
    for(int q=0; q<n_query;q++) {

        // Select actual query
        float query[dimension];
        for(int i=0; i<dimension; i++){
            query[i] = querys[q*dimension+i];
            // std::cout << "D3: " << q*dimension+i << std::endl;
        }

        // Distances from query point
        float dists [n_points] = {-1};  
        for (int i=0; i<n_points_real; i++){
            dists[i] = -1;
        }

        // Set of points: use array as queue
        int queue_ptr = 0;              // A pointer to the end of the queue
        int queue [n_points/2] = {-1};
        for (int i=0; i<n_points_real; i++){
            queue[i] = -1;
        }

        // Insert root in queue and save distance
        queue[queue_ptr] = 0;
        queue_ptr++;
        dists[0] = distance(points_coords[0], query);

        // Iteration on levels
        levels_loop:
        for(int l=maxlevel; l >= minlevel; l--){
        // for(int l= _maxlevel; l >= _minlevel; l--){
            std::cout << "L: " << l << std::endl;

            // std::cout << "Round " << l << std::endl;
            // Inspect all children of points in p_set
            // and compute the distances
            float min_distance = 19990102;
            // for(int i=0; i<size; i++){
            //     if(p_set[i] != -1) {
            //     std::cout << i << std::endl;
            //     }
            // }

            // Visit all elements in queue
            std::cout << "CoverSet" << std::endl;
            for(int i=0; i<queue_ptr;i++) {
                std::cout << queue[i] << std::endl;
            }
            int min_distance_id = -1;
            queue_loop:
            for(int i=0; i<queue_ptr;i++) {
                // std::cout << queue[i] << std::endl;
                // Get children at some level
                // for(int j=0; j<maxchildren*2; j+=2) {
                //     std::cout << i << ", " << j << " > " << points_children[i][j] << " - " << points_children[i][j+1] << std::endl;
                // }
                int p = queue[i];
                // std::cout << "P: " << p << " ["<<i<<"]" << std::endl;

                children_loop:
                for(int j=0; j<maxchildren*2; j+=2) {
                    // Values in list are "tuples of 2"
                    // With (Level, Point)

                    // dummy_level means NULL and list is sorted or
                    // List is sorted by level; stop
                    // iterating if find LOWER one

                    /*THIS causes the II to increase from 2 to 19. Is it better to have an II of 19 or to always go throu the 
                    entire loop?
                    */
                    // if(points_children[p][j] == dummy_level || points_children[p][j] < l) {
                    //     break;
                    // }

                    // We found a child at level l
                    // int chil2d = points_children[i][j+1];
                    // std::cout << i << ", " << j << " > " << points_children[i][j] << " - " << points_children[i][j+1] << std::endl;
                    if(points_children[p][j] == l) {
                        int child = points_children[p][j+1];
                        // Add child to queue and save distance
                        // std::cout << "C: "<<child << " @"<<queue_ptr <<std::endl;
                        queue[queue_ptr] = child;
                        queue_ptr++;
                        dists[child] = distance(points_coords[child],query);

                        // OK for k=1
                        if(dists[child] < min_distance){
                            min_distance = dists[child];
                            min_distance_id = child;
                        }
                    }
                }

                // OK for k=1
                if(dists[p] < min_distance){
                    min_distance = dists[p];
                    min_distance_id = p;
                }
                
            }

            std::cout << "CoverSet+Children" << std::endl;
            for(int i=0; i<queue_ptr;i++) {
                std::cout << queue[i] << std::endl;
            }
            std::cout << "C: "<< min_distance_id <<" | " << min_distance << " ! "<< distance(points_coords[min_distance_id],query)<<std::endl;
            std::cout << "(";
            for(int i=0; i<6;i++){
                std::cout << points_coords[min_distance_id][i];
                if(i<5){
                   std::cout << ", ";
                }
            }
            std::cout << ")" << std::endl;

            // We allow only points with distance <= min_distance + 2^l
            int reduced_position = 0;
            filter_loop:
            for(int i=0; i<queue_ptr;i++) {
                int p = queue[i];
                int modify_position = 1;
                if(dists[p] > (min_distance + pow(2,l))) {
                    // Exclude point from queue
                    modify_position = 0;
                }
                queue[reduced_position] = queue[i];
                reduced_position += modify_position;
            }
            
            // std::cout << "B: " << queue_ptr << std::endl;
            max_qp = queue_ptr > max_qp ? queue_ptr : max_qp;
            queue_ptr = reduced_position;

            // std::cout << "A: " << queue_ptr << std::endl;

            // if(l==-1) {
            //     break;
            // }
        }

        std::cout << "Max queue: " << max_qp << std::endl;

        // We visited all levels, now just take the minimum
        float min_distance = 19990102;
        int min_index = -1;
        min_find_loop:
        for(int i=0; i<queue_ptr;i++) {
            int p = queue[i];
            if(dists[p] < min_distance) {
                min_distance = dists[p];
                min_index = p;
            }
        }

        // Save result corresponding to query
        for(int i=0; i<dimension; i++) {
            outs[q*dimension+i] = points_coords[min_index][i];
        }
    }

    
}

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
                    int want_output,
                    int &passedTests) {

    std::string str; 

    // Generate random query point and save them to file querys.txt

    std::ofstream myfile("querys.txt");

    if(myfile.is_open())
    {
       
    }
    else std::cerr<<"Unable to open file";
    for(int i=0; i<dimension*n_query; i++) {
        // // This will generate a random number from 0.0 to 1.0, inclusive.
        srand(time(NULL)+i);
        query[i] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        myfile<<std::to_string(query[i]) << std::endl;
    }
    myfile.close();

    // Write pts to file:
    std::ofstream myfile2("pts.txt");
    if(myfile2.is_open())
    {
    }
    else std::cerr<<"Unable to open file";
    for(int i=0; i<dimension*n_points_real; i+=6) {
        myfile2 << std::to_string(static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) << ',' 
        << std::to_string(static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) << ',' 
        << std::to_string(static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) << ',' 
        << std::to_string(static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) << ',' 
        << std::to_string(static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) << ',' 
        << std::to_string(static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) 
        << std::endl;
    }
    myfile2.close();

    
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
    std::getline(file, str);
    passedTests = std::stof(str);
    file.close();
}

void importGeneratedTree (  std::vector<float,aligned_allocator<float>> &points_coords,
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

    std::ifstream file;
    // file.open("../covertree/querys.txt");
    file.open("querys1.txt");
    if(!file) { // file couldn't be opened
      std::cerr << "Error: file could not be opened: "; //<< strerror(errno) << std::endl;
      exit(1);
    }
    for(int i=0; i<n_query*dimension; i++){            
        std::getline(file, str);
        query[i] =  std::stof(str);
    }

    file.close();
    // file.open("../covertree/generated_tree_"+std::to_string(n_points_real)+".txt");
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

    file.close();
    // file.open("../covertree/results.txt");    
    file.open("results1.txt");
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

    std::cout << "C SIMULATION ***********************" << std::endl;

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

    // std::string binaryFile = argv[1];
    // 2D array DATA to pass to the FPGA
    // float points_coords_2 [n_points][dimension] = {};
    // int points_children_2 [n_points][maxchildren*2] = {};
    // Results arrays
    // float result_hw [dimension*100] = {};
    std::vector<float,aligned_allocator<float>> result_hw (dimension*n_query);
    std::vector<float> result_py (dimension*n_query);
    // Utils
    // float query [dimension*100];
    std::vector<float,aligned_allocator<float>> query (dimension*n_query);
    int maxlevel = dummy_level;
    int minlevel = dummy_level; 


    // Convert to 1d arrays...
    // float points_coords [n_points*dimension] = {};
    // int points_children [n_points*maxchildren*2] = {};
    std::vector<float,aligned_allocator<float>> points_coords(n_points*dimension);
    std::vector<int,aligned_allocator<int>> points_children(n_points*maxchildren*2);
    int passedTests = 0;

    generateTree(points_coords, points_children, result_py.data(), n_points_real, query, maxlevel, minlevel, n_query, want_output, passedTests);

    std::cout << "Generation ok" <<std::endl;
    // for(int i=0; i< n_points*dimension; i++) {
    //     std::cout << points_coords[i] << " ";
    // }
    int const a = n_points_real;
    // std::cout << "a ok" <<std::endl;
    const float * b = points_coords.data();
    // std::cout << "b ok" <<std::endl;
    const int * c = points_children.data();
    // std::cout << "c ok" <<std::endl;
    const float * d = query.data();
    // std::cout << "d ok" <<std::endl;
    float * e = result_hw.data();
    // std::cout << "e ok" <<std::endl;
    int f = maxlevel;
    // std::cout << "f ok" <<std::endl;
    int g = minlevel;
    std::cout << a << b << c << d << e << f << g <<n_query<<std::endl;
    vadd2(a,b,c,d,e,f,g,n_query);
    std::cout << "vadd ok" <<std::endl;
    // vadd(n_points_real, points_coords.data(), points_children.data(), query.data(), result_hw.data(), maxlevel,minlevel,n_query);

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
    std::cout << "PY TEST " << ((passedTests < n_query) ? "FAILED" : "PASSED") << std::endl;

    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}

