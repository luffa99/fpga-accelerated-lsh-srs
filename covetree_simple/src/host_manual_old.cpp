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

#define dimension 6 // Vector dimension in the data structure -> do NOT change
#define maxchildren 16  // Maximum number of children for every node (fixed vector)
#define n_points 100   // Number of po#defines to generate
#define dummy_level 69

float distance( float const in1[dimension],
                float const in2[dimension]) {

    float ans = 0;
    for (int i = 0; i < dimension; i++) {
        float a = in1[i];
        float b = in2[i];
        ans = ans + ( ( a-b ) * (a-b) ); 
    }
    return sqrt(ans); 
}

int seq_vadd(int const size,
        float const points_coords [n_points][dimension],
        int const points_children [n_points][maxchildren*2],
        float const query[dimension],
        float out[dimension],
        int maxlevel,
        int minlevel) {
    

    float min_distance = 19990102;
    int min_index = -1;
    for(int i=0; i<size;i++) {
        // Only points in set
        float d = distance(points_coords[i], query);
        //std::cout << "P: " << i << " D: " << d << std::endl;
        if(d < min_distance) {
            min_distance = d;
            min_index = i;
        }
    }

    for(int i=0; i<dimension; i++) {
        out[i] = points_coords[min_index][i];
    }

    return min_index;
}

int vadd(int const size,
        float const points_coords [n_points][dimension],
        int const points_children [n_points][maxchildren*2],
        float const query[dimension],
        float out[dimension],
        int maxlevel,
        int minlevel) {

    // Naïve implementation

    // Set of points with distances
    // -1 means "not in set"
    float p_set [n_points] = {-1};  
    for (int i=0; i<n_points; i++){
        p_set[i] = -1;
    }

    // Insert root
    p_set[0] = distance(points_coords[0], query);

    // Iteration on levels
    for(int l=maxlevel; l >= minlevel; l--){
        // std::cout << "Round " << l << std::endl;
        // Inspect all children of points in p_set
        // and compute the distances
        float min_distance = 19990102;
        // for(int i=0; i<size; i++){
        //     if(p_set[i] != -1) {
        //     std::cout << i << std::endl;
        //     }
        // }
        for(int i=0; i<size;i++) {
            // Only points in set
            if(p_set[i] != -1) {

                // std::cout << "|=> " << p_set[i] << std::endl;
                // Get children at some level
                // for(int j=0; j<maxchildren*2; j+=2) {
                //     std::cout << i << ", " << j << " > " << points_children[i][j] << " - " << points_children[i][j+1] << std::endl;
                // }
                for(int j=0; j<maxchildren*2; j+=2) {
                    // Values in list are "tuples of 2"
                    // With (Level, Point)

                    // dummy_level means NULL and list is sorted
                    if(points_children[i][j] == dummy_level) {
                        break;
                    }
                    // List is sorted by level; stop
                    // iterating if find LOWER one
                    if(points_children[i][j] < l) {
                        break;
                    }

                    // We found a child at level l
                    // int chil2d = points_children[i][j+1];
                    // std::cout << i << ", " << j << " > " << points_children[i][j] << " - " << points_children[i][j+1] << std::endl;
                    if(points_children[i][j] == l) {
                        int child = points_children[i][j+1];
                        p_set[child] = distance(points_coords[child],query);

                        // OK for k=1
                        if(p_set[child] < min_distance){
                            min_distance = p_set[child];
                        }
                    }
                }

                // OK for k=1
                if(p_set[i] < min_distance){
                    min_distance = p_set[i];
                }
            }
        }

        // We allow only points with distance <= min_distance + 2^l
        for(int i=0; i<size;i++) {
            // Only points in set
            if(p_set[i] != -1) {


                if(p_set[i] > (min_distance + pow(2,l))) {
                    // Exclude point from set
                    p_set[i] = -1;
                }
            }
        }

        // if(l==-1) {
        //     break;
        // }
    }

    // We visited all levels, now just take the minimum
    float min_distance = 19990102;
    int min_index = -1;
    for(int i=0; i<size;i++) {
        // Only points in set
        if(p_set[i] != -1) {
            if(p_set[i] < min_distance) {
                min_distance = p_set[i];
                min_index = i;
            }
        }
    }

    for(int i=0; i<dimension; i++) {
        out[i] = points_coords[min_index][i];
    }

    return min_index;
}

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
                    float query [dimension],
                    int &maxlevel,
                    int &minlevel) {

    std::string str; 

    // Generate random query point
    for(int i=0; i<dimension; i++) {
        // This will generate a random number from 0.0 to 1.0, inclusive.
        srand(time(NULL));
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
    float result_hw [dimension] = {};
    float result_py [dimension] = {};
    // Utils
    float query [dimension] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
    int n_points_real = n_points;
    int maxlevel = dummy_level;
    int minlevel = dummy_level;

    generateTree(points_coords, points_children, result_py, n_points_real, query, maxlevel, minlevel);

    int res = vadd(n_points_real, points_coords, points_children, query, result_hw, maxlevel, minlevel);


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

    std::cout << "Found Node ID: " << res << std::endl;

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

    // std::cout << "Sequential:" <<std::endl;


    // res = seq_vadd(n_points_real, points_coords, points_children, query, result_hw, maxlevel, minlevel);


    // std::cout << res << std::endl;

    // std::cout << "Query point was:" <<std::endl;
    // for (int i=0; i < dimension; i++){
    //     std::cout << query[i] << std::endl;
    // }


    // std::cout << "Found point is:" <<std::endl;
    // for (int i=0; i < dimension; i++){
    //     std::cout << result_hw[i] << std::endl;
    // }

    std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
