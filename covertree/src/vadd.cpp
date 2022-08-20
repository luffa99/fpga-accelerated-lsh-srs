// TODO: remove useless includes

// Includes
#include <hls_vector.h>
#include <hls_stream.h>
#include "assert.h"
#include "hls_math.h"
#include <iostream>
#include "math.h"
#include "ap_int.h"

// TODO: move definitions in template file

// Useful definition:
#define dimension 6         // Vector dimension in the data structure -> do NOT change
#define maxchildren 128     // Maximum number of children for every node (fixed vector)
#define n_points 1000       // Number of po#defines to generate at maximum
#define dummy_level 69      // Dummy level for null
#define MAX_VECT_SIZE 10000 // Maximum size of vector
typedef ap_uint<512> ap_uint512_t;
using namespace hls;

// Debug/Performance definitions
#define _minlevel -4
#define _maxlevel 2
#define M 96
#define D 6
#define var_size vector_size //vector_size or M
#define var_amount amount //amount or 1

///////////////////////// Dotproduct/Projection functions //////////////////////////////////////////////

/* Given a stream of type ap_uint512_t fills an array with 16 floats */
void load_chunk (   stream<ap_uint512_t> &pipe,
                    float * array_of_size_16) {

    ap_uint512_t temp = pipe.read();

    for(int j=0; j<16; j++) {
    #pragma HLS unroll
        ap_uint<32> tmp_float = temp.range(512 - 1 - j*32, 512 - 32 - j * 32);
        array_of_size_16[j] = *((float*)&tmp_float);
    }

}

/* Each PE mulitplies with one random vector */
void PE_dotproduct  (   int const vector_size,
                        int const amount,
                        stream<ap_uint512_t> &random_vector, 
                        stream<ap_uint512_t> &origin_vector,
                        stream<float> &out_result) {
    
    // Repeat the same process for different vectors!
    for(int i=0; i<var_amount; i++) {
        float sum = 0.0;

        loop2:
        for (int j=0; j < var_size / 16; j++) { // vector_size is ALWAYS a multiple of 16
            #pragma HLS pipeline II=1

            float temp_rand[16];
            float temp_orig[16];
            load_chunk(random_vector, temp_rand);
            load_chunk(origin_vector, temp_orig);

            sum +=  temp_rand[0] * temp_orig[0]
                +   temp_rand[1] * temp_orig[1]
                +   temp_rand[2] * temp_orig[2]
                +   temp_rand[3] * temp_orig[3]
                +   temp_rand[4] * temp_orig[4]
                +   temp_rand[5] * temp_orig[5]
                +   temp_rand[6] * temp_orig[6]
                +   temp_rand[7] * temp_orig[7]
                +   temp_rand[8] * temp_orig[8]
                +   temp_rand[9] * temp_orig[9]
                +   temp_rand[10] * temp_orig[10]
                +   temp_rand[11] * temp_orig[11]
                +   temp_rand[12] * temp_orig[12]
                +   temp_rand[13] * temp_orig[13]
                +   temp_rand[14] * temp_orig[14]
                +   temp_rand[15] * temp_orig[15];
        }

        // write result to out stream
        out_result.write(sum);
    }
}

/* Read random vectors into memory and put into random pipe */
void ReadMemory_rands ( int const vector_size,
                        int const amount,
                        float const *rands,
                        stream<ap_uint512_t> &random_pipe) {

    // copy rands into BRAM (local array), then use it
    // This is convenient because I access the same array in DRAM many times
    float rands_local[MAX_VECT_SIZE];
    for(int i=0; i<var_size;i++){
        rands_local[i] = rands[i];
    }

    // Now put the date into the stream, in chunks of 512 bits
    ap_uint512_t temp;
    float size = var_size*var_amount*32;  // Size is ALWAYS a multiple of 16 and so a multiple of 512
    int i = 0;
    while (size >= 512) {
        for(int j=0; j<16; j++) {
        #pragma HLS unroll
            temp.range(512 - 1 - j*32, 512 - 32 - j * 32) = *((ap_uint<32>*)&rands_local[i%var_size]);
            i++;
        }
        random_pipe.write(temp);
        size -= 512;
    }
}

/* Read vectors to project (orig) and copy in different pipes */
void ReadMemory_orig (  int const vector_size,
                        int const amount,
                        float const *orig,
                        stream<ap_uint512_t> &origin_pipe_1,
                        stream<ap_uint512_t> &origin_pipe_2,
                        stream<ap_uint512_t> &origin_pipe_3,
                        stream<ap_uint512_t> &origin_pipe_4,
                        stream<ap_uint512_t> &origin_pipe_5,
                        stream<ap_uint512_t> &origin_pipe_6) {

    // Broadcast origin vector, into 6 streams, in chunks of 512 bits
    ap_uint512_t temp;
    float size = var_size*var_amount*32;
    int i = 0;
    while (size >= 512) { // Size is ALWAYS a multiple of 16 and so a multiple of 512
        for(int j=0; j<16; j++) {
        #pragma HLS unroll
            temp.range(512 - 1 - j*32, 512 - 32 - j * 32) = *((ap_uint<32>*)&orig[i]);
            i++;
        }
        origin_pipe_1.write(temp);
        origin_pipe_2.write(temp);
        origin_pipe_3.write(temp);
        origin_pipe_4.write(temp);
        origin_pipe_5.write(temp);
        origin_pipe_6.write(temp);
        size -= 512;
    }
}

////////////////////////////////////////////// Covertree functions /////////////////////////////////////


/* Compute distance of two points*/
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

void query( int const n_points_real,
            int maxlevel,
            int minlevel,
            int n_query,
            int const * points_children_dram,
            float const * points_coords_dram,
            float * outs,
            stream<float> &out_1,
            stream<float> &out_2,
            stream<float> &out_3,
            stream<float> &out_4,
            stream<float> &out_5,
            stream<float> &out_6,
            float * proj
            ) {

    ////////////////// Covertree preconfiguration ////////////////////////////////

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

    for(int q=0; q<n_query;q++) {
        // Select actual query
        float query[dimension];
        // for(int i=0; i<dimension; i++){
        //     query[i] = querys[q*dimension+i];
        // }

        // Gather next vector to search, from the projected ones
        query[0] = out_1.read();
        query[1] = out_2.read();
        query[2] = out_3.read();
        query[3] = out_4.read();
        query[4] = out_5.read();
        query[5] = out_6.read();

        //// For debugging
        proj[0+q*6] = query[0];
        proj[1+q*6] = query[1];
        proj[2+q*6] = query[2];
        proj[3+q*6] = query[3];
        proj[4+q*6] = query[4];
        proj[5+q*6] = query[5];

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

extern "C" { 

void vadd(  float const * points_coords_dram,   // Coordinate of points in the data structure
            int const * points_children_dram,   // Childrens of points in the datastructure
            float const *rand1,                 // Random vectors used for projection
            float const *rand2,
            float const *rand3,
            float const *rand4,
            float const *rand5,
            float const *rand6,
            float const *orig,                  // Vectors of original space size to project and search NN
            float * outs,                       // Found NN
            float * proj, //Deubg: projected vectors
            int const n_points_real,            // Effective number of data points in the data structure
            int const maxlevel,                 // Maximum level in data structure
            int const minlevel,                 // Minimum level of data structure
            int const amount,                   // Number of vectors in orig
            int const vector_size )             // Original dimension (size of vectors in orig)
    
    {   

    // Memory mapping
    #pragma HLS INTERFACE m_axi port = points_coords_dram bundle=gmem0
    #pragma HLS INTERFACE m_axi port = points_children_dram bundle=gmem1
    #pragma HLS INTERFACE m_axi port = outs bundle=gmem3
        #pragma HLS INTERFACE m_axi port = proj bundle=gmem5

    #pragma HLS INTERFACE s_axilite port = points_coords_dram
    #pragma HLS INTERFACE s_axilite port = points_children_dram
    #pragma HLS INTERFACE s_axilite port = outs 
         #pragma HLS INTERFACE s_axilite port = proj 

    #pragma HLS INTERFACE s_axilite port = n_points_real 
    #pragma HLS INTERFACE s_axilite port = maxlevel 
    #pragma HLS INTERFACE s_axilite port = minlevel 

    // ?
    #pragma HLS INTERFACE m_axi port = rand1 bundle=gmem6
    #pragma HLS INTERFACE m_axi port = rand2 bundle=gmem7
    #pragma HLS INTERFACE m_axi port = rand3 bundle=gmem8
    #pragma HLS INTERFACE m_axi port = rand4 bundle=gmem9
    #pragma HLS INTERFACE m_axi port = rand5 bundle=gmem10
    #pragma HLS INTERFACE m_axi port = rand6 bundle=gmem11

    #pragma HLS INTERFACE m_axi port = orig bundle=gmem4

    #pragma HLS INTERFACE s_axilite port = rand1 
    #pragma HLS INTERFACE s_axilite port = rand2
    #pragma HLS INTERFACE s_axilite port = rand3 
    #pragma HLS INTERFACE s_axilite port = rand4 
    #pragma HLS INTERFACE s_axilite port = rand5 
    #pragma HLS INTERFACE s_axilite port = rand6 
    #pragma HLS INTERFACE s_axilite port = orig 

    #pragma HLS INTERFACE s_axilite port = amount 
    #pragma HLS INTERFACE s_axilite port = vector_size

    ///////////////// PROJECTION preconfig ///////////////////////////////
    stream<ap_uint512_t> random_1;
    stream<ap_uint512_t> random_2;
    stream<ap_uint512_t> random_3;
    stream<ap_uint512_t> random_4;
    stream<ap_uint512_t> random_5;
    stream<ap_uint512_t> random_6;
    stream<ap_uint512_t> origin_pipe_1;
    stream<ap_uint512_t> origin_pipe_2;
    stream<ap_uint512_t> origin_pipe_3;
    stream<ap_uint512_t> origin_pipe_4;
    stream<ap_uint512_t> origin_pipe_5;
    stream<ap_uint512_t> origin_pipe_6;
    stream<float> out_1;
    stream<float> out_2;
    stream<float> out_3;
    stream<float> out_4;
    stream<float> out_5;
    stream<float> out_6;

    #pragma HLS DATAFLOW

    ReadMemory_orig(vector_size, amount, orig, origin_pipe_1, origin_pipe_2, origin_pipe_3, origin_pipe_4, origin_pipe_5, origin_pipe_6 );
    ReadMemory_rands(vector_size, amount, rand1, random_1);
    ReadMemory_rands(vector_size, amount, rand2, random_2);
    ReadMemory_rands(vector_size, amount, rand3, random_3);
    ReadMemory_rands(vector_size, amount, rand4, random_4);
    ReadMemory_rands(vector_size, amount, rand5, random_5);
    ReadMemory_rands(vector_size, amount, rand6, random_6);

    PE_dotproduct(vector_size, amount, random_1, origin_pipe_1, out_1);
    PE_dotproduct(vector_size, amount, random_2, origin_pipe_2, out_2);
    PE_dotproduct(vector_size, amount, random_3, origin_pipe_3, out_3);
    PE_dotproduct(vector_size, amount, random_4, origin_pipe_4, out_4);
    PE_dotproduct(vector_size, amount, random_5, origin_pipe_5, out_5);
    PE_dotproduct(vector_size, amount, random_6, origin_pipe_6, out_6);

    query(n_points_real, maxlevel, minlevel, amount, points_children_dram, points_coords_dram, outs, 
        out_1, out_2, out_3, out_4, out_5, out_6, proj);
    // Search 100 query points!
    // TODO: parametrize the loop size
    
        
    }
}