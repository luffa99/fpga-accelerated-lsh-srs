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
}
}
