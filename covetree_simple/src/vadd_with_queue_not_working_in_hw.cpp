// Includes
#include <hls_vector.h>
#include <hls_stream.h>
#include "assert.h"
#include "hls_math.h"

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

extern "C" {


void vadd(int const size,
        float const points_coords [n_points][dimension],
        int const points_children [n_points][maxchildren*2],
        float const query[dimension],
        float out[dimension],
        int maxlevel,
        int minlevel) {

#pragma HLS INTERFACE m_axi port = points_coords bundle=gmem0
#pragma HLS INTERFACE m_axi port = points_children bundle=gmem1
#pragma HLS INTERFACE m_axi port = query bundle=gmem2

#pragma HLS INTERFACE s_axilite port = points_coords 
#pragma HLS INTERFACE s_axilite port = points_children 
#pragma HLS INTERFACE s_axilite port = query 

#pragma HLS INTERFACE s_axilite port = size 
#pragma HLS INTERFACE s_axilite port = out 
    // Naïve implementation

    // Distances from query point
    float dists [n_points] = {-1};  
    for (int i=0; i<n_points; i++){
        dists[i] = -1;
    }

    // Set of points: use array as queue
    int queue_ptr = 0;              // A pointer to the end of the queue
    int queue [n_points] = {-1};
    for (int i=0; i<n_points; i++){
        queue[i] = -1;
    }

    // Insert root in queue and save distance
    queue[queue_ptr] = 0;
    queue_ptr++;
    dists[0] = distance(points_coords[0], query);

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

        // Visit all elements in queue
        for(int i=0; i<queue_ptr;i++) {
            // std::cout << "|=> " << p_set[i] << std::endl;
            // Get children at some level
            // for(int j=0; j<maxchildren*2; j+=2) {
            //     std::cout << i << ", " << j << " > " << points_children[i][j] << " - " << points_children[i][j+1] << std::endl;
            // }
            int p = queue[i];
            for(int j=0; j<maxchildren*2; j+=2) {
                // Values in list are "tuples of 2"
                // With (Level, Point)

                // dummy_level means NULL and list is sorted
                if(points_children[p][j] == dummy_level) {
                    break;
                }
                // List is sorted by level; stop
                // iterating if find LOWER one
                if(points_children[p][j] < l) {
                    break;
                }

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
        
        queue_ptr = reduced_position;
        // if(l==-1) {
        //     break;
        // }
    }

    // We visited all levels, now just take the minimum
    float min_distance = 19990102;
    int min_index = -1;
    for(int i=0; i<size;i++) {
        int p = queue[i];
        if(dists[p] < min_distance) {
            min_distance = dists[p];
            min_index = p;
        }
    }

    for(int i=0; i<dimension; i++) {
        out[i] = points_coords[min_index][i];
    }
}
}
