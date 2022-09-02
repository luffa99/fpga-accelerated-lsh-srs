// Includes
#include <hls_vector.h>
#include <hls_stream.h>
#include "assert.h"
#include "hls_math.h"

#define dimension 6 // Vector dimension in the data structure -> do NOT change
#define maxchildren 16  // Maximum number of children for every node (fixed vector)
#define n_points 100   // Number of po#defines to generate
#define dummy_level 69

// For analysis purposes: in practice min/max level is defined by tree generation
#define _maxlevel 2
#define _minlevel -3
#define _size n_points

float distance( float const in1[dimension],
                float const in2[dimension]) {

    float ans = 0;
    eu_distance:
    for (int i = 0; i < dimension; i++) {
        float a = in1[i];
        float b = in2[i];
        ans = ans + ( ( a-b ) * (a-b) ); 
    }
    // return sqrt(ans); 
    return ans;
}

inline float PE_child(int l, int const l_child, float const coord_child[dimension], float const query[dimension], float *p_set_i) {
    
    
    if (l_child == dummy_level || l_child != l) {
        return -1.0;
    }

    return distance(coord_child, query);
}

// void writeToPE_child(int l, int const points_children[maxchildren*2], float const points_coords [n_points][dimension], 
//     float const query[dimension]) {

//         write_to_pe_child:
//         for(int j=0; j<maxchildren*2; j+=2){
//             in_int1 << l;
//             for(int k=0; k<dimension; k++) {
//                 in1 << points_coords[points_children[j]][k];
//             }
//             for(int k=0; k<dimension; k++) {
//                 in1 << query[k];
//             }
//         }

// }

void children(int l, int i, int const points_children [n_points][maxchildren*2],        
    float const points_coords [n_points][dimension],
    float const query[dimension],
    float p_set [n_points]
 ) {

    // Stream<float> in1;
    // Stream<float> in2;
    // Stream<float> in3;
    // Stream<float> in4;
    // Stream<float> in5;
    // Stream<float> in6;
    // Stream<float> in7;
    // Stream<float> in8;
    // Stream<float> in9;
    // Stream<float> in10;
    // Stream<float> in11;
    // Stream<float> in12;
    // Stream<float> in13;
    // Stream<float> in14;
    // Stream<float> in15;
    // Stream<float> in16;

    // Stream<int> in_int1;
    // Stream<int> in_int2;
    // Stream<int> in_int3;
    // Stream<int> in_int4;
    // Stream<int> in_int5;
    // Stream<int> in_int6;
    // Stream<int> in_int7;
    // Stream<int> in_int8;
    // Stream<int> in_int9;
    // Stream<int> in_int10;
    // Stream<int> in_int11;
    // Stream<int> in_int12;
    // Stream<int> in_int13;
    // Stream<int> in_int14;
    // Stream<int> in_int15;
    // Stream<int> in_int16;

    // Stream<float> out1;
    // Stream<float> out2;
    // Stream<float> out3;
    // Stream<float> out4;
    // Stream<float> out5;
    // Stream<float> out6;
    // Stream<float> out7;
    // Stream<float> out8;
    // Stream<float> out9;
    // Stream<float> out10;
    // Stream<float> out11;
    // Stream<float> out12;
    // Stream<float> out13;
    // Stream<float> out14;
    // Stream<float> out15;
    // Stream<float> out16;
    float d1 = -1;
    float d2 = -1;
    float d3 = -1;
    float d4 = -1;
    float d5 = -1;
    float d6 = -1;
    float d7 = -1;
    float d8 = -1;
    float d9 = -1;
    float d10 = -1;
    float d11 = -1;
    float d12 = -1;
    float d13 = -1;
    float d14 = -1;
    float d15 = -1;
    float d16 = -1;

    int l1 = l;
    int l2 = l;
    int l3 = l;
    int l4 = l;
    int l5 = l;
    int l6 = l;
    int l7 = l;
    int l8 = l;
    int l9 = l;
    int l10 = l;
    int l11 = l;
    int l12 = l;
    int l13 = l;
    int l14 = l;
    int l15 = l;
    int l16 = l;

    int pc1 = points_children[i][0];
    int pc2 = points_children[i][2];
    int pc3 = points_children[i][4];
    int pc4 = points_children[i][6];
    int pc5 = points_children[i][8];
    int pc6 = points_children[i][10];
    int pc7 = points_children[i][12];
    int pc8 = points_children[i][14];
    int pc9 = points_children[i][16];
    int pc10 = points_children[i][18];
    int pc11 = points_children[i][20];
    int pc12 = points_children[i][22];
    int pc13 = points_children[i][24];
    int pc14 = points_children[i][26];
    int pc15 = points_children[i][28];
    int pc16 = points_children[i][30];

    float pco1[dimension];
    float pco2[dimension];
    float pco3[dimension];
    float pco4[dimension];
    float pco5[dimension];
    float pco6[dimension];
    float pco7[dimension];
    float pco8[dimension];
    float pco9[dimension];
    float pco10[dimension];
    float pco11[dimension];
    float pco12[dimension];
    float pco13[dimension];
    float pco14[dimension];
    float pco15[dimension];
    float pco16[dimension];

    float q1[dimension];
    float q2[dimension];
    float q3[dimension];
    float q4[dimension];
    float q5[dimension];
    float q6[dimension];
    float q7[dimension];
    float q8[dimension];
    float q9[dimension];
    float q10[dimension];
    float q11[dimension];
    float q12[dimension];
    float q13[dimension];
    float q14[dimension];
    float q15[dimension];
    float q16[dimension];

    for(int j=0; j<dimension;j++) {
        pco1[j] = points_coords[points_children[i][1]][j];
        pco2[j] = points_coords[points_children[i][3]][j];
        pco3[j] = points_coords[points_children[i][5]][j];
        pco4[j] = points_coords[points_children[i][7]][j];
        pco5[j] = points_coords[points_children[i][9]][j];
        pco6[j] = points_coords[points_children[i][11]][j];
        pco7[j] = points_coords[points_children[i][13]][j];
        pco8[j] = points_coords[points_children[i][15]][j];
        pco9[j] = points_coords[points_children[i][17]][j];
        pco10[j] = points_coords[points_children[i][19]][j];
        pco11[j] = points_coords[points_children[i][21]][j];
        pco12[j] = points_coords[points_children[i][23]][j];
        pco13[j] = points_coords[points_children[i][25]][j];
        pco14[j] = points_coords[points_children[i][27]][j];
        pco15[j] = points_coords[points_children[i][29]][j];
        pco16[j] = points_coords[points_children[i][31]][j];
        q1[j] = query[j];
        q2[j] = query[j];
        q3[j] = query[j];
        q4[j] = query[j];
        q5[j] = query[j];
        q6[j] = query[j];
        q7[j] = query[j];
        q8[j] = query[j];
        q9[j] = query[j];
        q10[j] = query[j];
        q11[j] = query[j];
        q12[j] = query[j];
        q13[j] = query[j];
        q14[j] = query[j];
        q15[j] = query[j];
        q16[j] = query[j];
    }


// #pragma HLS dataflow
    // writeToPE_child(l, points_children[i], points_coords, query);
    d1 = PE_child(l1, pc1, pco1,
        q1);
    PE_child(l2, pc2, pco2,
        q2, d2);
    PE_child(l3, pc3, pco3,
        q3, d3);
    PE_child(l4, pc4, pco4,
        q4, d4);
    PE_child(l5, pc5, pco5,
        q5, d5);
    PE_child(l6, pc6, pco6,
        q6, d6);
    PE_child(l7, pc7, pco7,
        q7, d7);
    PE_child(l8, pc8, pco8,
        q8, d8);
    PE_child(l9, pc9, pco9,
        q9, d9);
    PE_child(l10, pc10, pco10,
        q10, d10);
    PE_child(l11, pc11, pco11,
        q11, d11);
    PE_child(l12, pc12, pco12,
        q12, d12);
    PE_child(l13, pc13, pco13,
        q13, d13);
    PE_child(l14, pc14, pco14,
        q14, d14);
    PE_child(l15, pc15, pco15,
        q15, d15);
    PE_child(l16, pc16, pco16,
        q16, d16);

    p_set[points_children[i][0]] = d1;
    p_set[points_children[i][2]] = d2;
    p_set[points_children[i][4]] = d3;
    p_set[points_children[i][6]] = d4;
    p_set[points_children[i][8]] = d5;
    p_set[points_children[i][10]] = d6;
    p_set[points_children[i][12]] = d7;
    p_set[points_children[i][14]] = d8;
    p_set[points_children[i][16]] = d9;
    p_set[points_children[i][18]] = d10;
    p_set[points_children[i][20]] = d11;
    p_set[points_children[i][22]] = d12;
    p_set[points_children[i][24]] = d13;
    p_set[points_children[i][26]] = d14;
    p_set[points_children[i][28]] = d15;
    p_set[points_children[i][30]] = d16;

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
    initialize_set:
    for (int i=0; i<n_points; i++){
        p_set[i] = -1;
    }

    // Insert root
    p_set[0] = distance(points_coords[0], query);

    // Iteration on levels
    it_levels:
    for(int l=_maxlevel; l >= _minlevel; l--){
        // std::cout << "Round " << l << std::endl;
        // Inspect all children of points in p_set
        // and compute the distances
        float min_distance = 19990102;
        // for(int i=0; i<size; i++){
        //     if(p_set[i] != -1) {
        //     std::cout << i << std::endl;
        //     }
        // }
        it_set:
        for(int i=0; i<_size;i++) {
            // Only points in set
            if(p_set[i] != -1) {

                children(l, i, points_children, points_coords, query, p_set);

                // for(int j=0; j<maxchildren*2; j+=2) {
                //     #pragma HLS dataflow
                //     PE_child(&l, &points_children[i][j], &points_coords[points_children[i][j+1]],
                //         query, &p_set[points_children[i][j+1]]);
                // }
                float dist_tmp[2 * maxchildren];
#pragma HLS ARRAY_PARTITION VARIABLE=dist_tmp

                 for(int j=0; j<maxchildren*2; j+=2) {
// #pragma HLS UNROLL
                    // Values in list are "tuples of 2"
                    // With (Level, Point)

                    // dummy_level means NULL and list is sorted
                    if(points_children[i][j] == dummy_level || points_children[i][j] != l)
                    // List is sorted by level; stop
                    // iterating if find LOWER one
                        dist_tmp[j] = -1;
                    }
                    else {

                        // We found a child at level l
                        // int chil2d = points_children[i][j+1];
                        // std::cout << i << ", " << j << " > " << points_children[i][j] << " - " << points_children[i][j+1] << std::endl;
                        // if(points_children[i][j] == l) {
                        int child = points_children[i][j+1];
                        p_set[child] = distance(points_coords[child],query);

                        // OK for k=1
                        if(p_set[child] < min_distance){
                            min_distance = p_set[child];
                        }
                        // }
                    }
                }

                
                // // OK for k=1
                // if(p_set[i] < min_distance){
                //     min_distance = p_set[i];
                // }
            }
        }

        it_min:
        for(int i=0; i<_size;i++) {
            // Only points in set
            if(p_set[i] != -1) {
                if(p_set[i] < min_distance){
                    min_distance = p_set[i];
                }
            }
        }

        // We allow only points with distance <= min_distance + 2^l
        it_filter:
        for(int i=0; i<_size;i++) {
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
    find_result:
    for(int i=0; i<_size;i++) {
        // Only points in set
        if(p_set[i] != -1) {
            if(p_set[i] < min_distance) {
                min_distance = p_set[i];
                min_index = i;
            }
        }
    }

    save_output:
    for(int i=0; i<dimension; i++) {
        out[i] = points_coords[min_index][i];
    }
}
}
