// Includes
#include <hls_vector.h>
#include <hls_stream.h>
#include "assert.h"
#include "hls_math.h"
#include <iostream>

#define dimension 6 // Vector dimension in the data structure -> do NOT change
#define maxchildren 16  // Maximum number of children for every node (fixed vector)
#define n_points 1000   // Number of po#defines to generate at maximum
#define dummy_level 69

#define _minlevel -4
#define _maxlevel 2

float distance( float const in1[dimension],
                float const in2[dimension]) {

    float ans = 0;
    distance_loop:
    for (int i = 0; i < dimension; i++) {
    // TODO: manually unroll
    #pragma HLS unroll
        float a = in1[i];
        float b = in2[i];
        ans = ans + ( ( a-b ) * (a-b) ); 
    }

    /* Previously I returned sqrt(ans), according to the euclidian distance formula
    This is not necessary since we only need to compare distances, and removing it
    can be an optimization
    */
    return ans; 
}

extern "C" {


void vadd(int const n_points_real,
        const float * points_coords_dram,
        const int * points_children_dram,
        const float * querys,
        float * outs,
        int maxlevel,
        int minlevel,
        int n_query) {

// Memory mapping
#pragma HLS INTERFACE m_axi port = points_coords_dram bundle=gmem0
#pragma HLS INTERFACE m_axi port = points_children_dram bundle=gmem1
#pragma HLS INTERFACE m_axi port = querys bundle=gmem2
#pragma HLS INTERFACE m_axi port = outs bundle=gmem3

#pragma HLS INTERFACE s_axilite port = points_coords_dram
#pragma HLS INTERFACE s_axilite port = points_children_dram
#pragma HLS INTERFACE s_axilite port = querys 
#pragma HLS INTERFACE s_axilite port = outs 

#pragma HLS INTERFACE s_axilite port = n_points_real 
#pragma HLS INTERFACE s_axilite port = maxlevel 
#pragma HLS INTERFACE s_axilite port = minlevel 
#pragma HLS INTERFACE s_axilite port = n_query 

// Let's partition the array -> this should allow optimizations

/* Partitioning of points_coords in 6 parts: so we can access each dimension
in parallel and speed-up the distance computation */
// #pragma HLS array_partition variable=points_coords_dram block factor=6 dim=2
float points_coords [n_points][dimension];
#pragma HLS array_partition variable=points_coords block factor=6 dim=2

/*  Partitioning of points_children in maxchildren part: this may optimize
the children_loop below
*/
// #pragma HLS array_partition variable=points_children_dram block factor=16 dim=2
int points_children [n_points][maxchildren*2];
#pragma HLS array_partition variable=points_children block factor=16 dim=2

    // Copying data structure in on-chip memory:
    for(int i=0; i<n_points_real; i++){
        for(int j=0; j<dimension; j++){
            points_coords[i][j] = points_coords_dram[i*dimension+j];
        }
        for(int j=0; j<maxchildren*2; j++) {
            points_children[i][j] = points_children_dram[i*maxchildren*2+j];
        }
    }

    // Search 100 query points!
    // TODO: parametrize the loop size
    for(int q=0; q<n_query;q++) {

        // Select actual query
        float query[dimension];
        for(int i=0; i<dimension; i++){
            query[i] = querys[q*dimension+i];
        }

        // Distances from query point
        float dists [n_points] = {-1};  
        for (int i=0; i<n_points_real; i++){
            dists[i] = -1;
        }

        // Set of points: use array as queue
        int queue_ptr = 0;              // A pointer to the end of the queue
        int queue [n_points] = {-1};
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
            // std::cout << "L: " << l << std::endl;

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
            queue_loop:
            for(int i=0; i<queue_ptr;i++) {
                // std::cout << "|=> " << p_set[i] << std::endl;
                // Get children at some level
                // for(int j=0; j<maxchildren*2; j+=2) {
                //     std::cout << i << ", " << j << " > " << points_children[i][j] << " - " << points_children[i][j+1] << std::endl;
                // }
                int p = queue[i];
                // std::cout << "P: " << p << std::endl;

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
                        queue[queue_ptr] = child;
                        queue_ptr++;
                        dists[child] = distance(points_coords[child],query);

                        // OK for k=1
                        if(dists[child] < min_distance){
                            min_distance = dists[child];
                        }
                    }
                }

                // OK for k=1
                if(dists[p] < min_distance){
                    min_distance = dists[p];
                }
                
            }

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

            queue_ptr = reduced_position;

            // std::cout << "A: " << queue_ptr << std::endl;

            // if(l==-1) {
            //     break;
            // }
        }

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
}
