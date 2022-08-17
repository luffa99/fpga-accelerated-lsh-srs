// !!!! TEST VADD !!!!!

// Includes
#include <hls_vector.h>
#include <hls_stream.h>
#include "assert.h"
#include "math.h"
#include "ap_int.h"

#define var_size vector_size //vector_size or M
#define var_amount amount //amount or 1
#define MAX_VECT_SIZE 10000

typedef ap_uint<512> ap_uint512_t;
using namespace hls;

void load_chunk (   stream<ap_uint512_t> &pipe,
                    float * array_of_size_16) {

    ap_uint512_t temp = pipe.read();

    for(int j=0; j<16; j++) {
    #pragma HLS unroll
        ap_uint<32> tmp_float = temp.range(512 - 1 - j*32, 512 - 32 - j * 32);
        array_of_size_16[j] = *((float*)&tmp_float);
    }

}

// Each PE mulitplies with one random vector
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


// Made 2 readMemory to do loops in parallel!
void ReadMemory_rands ( int const vector_size,
                        int const amount,
                        float const *rands,
                        stream<ap_uint512_t> &random_pipe) {

    // TODO copy rands into BRAM (local array), then use it
    // This is convenient because I access the same array in DRAM many times
    int rands_local[MAX_VECT_SIZE];
    for(int i=0; i<var_size;i++){
        rands_local[i] = rands[i];
    }

    // Now put the date into the stream, in chunks of 512 bits
    ap_uint512_t temp;
    int size = var_size*var_amount*32;  // Size is ALWAYS a multiple of 16 and so a multiple of 512
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


void ReadMemory_orig (  int const vector_size,
                        int const amount,
                        float const *orig,
                        stream<ap_uint512_t> &origin_pipe_1
                        ) {

    // Broadcast origin vector, into 6 streams, in chunks of 512 bits
    ap_uint512_t temp;
    int size = var_size*var_amount*32;
    int i = 0;
    while (size >= 512) { // Size is ALWAYS a multiple of 16 and so a multiple of 512
        for(int j=0; j<16; j++) {
        #pragma HLS unroll
            temp.range(512 - 1 - j*32, 512 - 32 - j * 32) = *((ap_uint<32>*)&orig[i]);
            i++;
        }
        origin_pipe_1.write(temp);
        size -= 512;
    }
}


void WriteMemory    (   int const amount,
                        stream<float> &out_1,
                        float *proj) {

    for(int i=0; i<var_amount; i++) {
        proj[0+i*6] = out_1.read();
    }
}

extern "C" {


    void vadd   (   float const *rand1,
                    float const *rand2,
                    float const *rand3,
                    float const *rand4,
                    float const *rand5,
                    float const *rand6,
                    float const *orig,
                    float *proj,
                    int const amount,
                    int const vector_size) {

        #pragma HLS INTERFACE m_axi port = rand1 bundle=gmem2
        #pragma HLS INTERFACE m_axi port = rand2 bundle=gmem3
        #pragma HLS INTERFACE m_axi port = rand3 bundle=gmem4
        #pragma HLS INTERFACE m_axi port = rand4 bundle=gmem5
        #pragma HLS INTERFACE m_axi port = rand5 bundle=gmem6
        #pragma HLS INTERFACE m_axi port = rand6 bundle=gmem7

        #pragma HLS INTERFACE m_axi port = orig bundle=gmem0
        #pragma HLS INTERFACE m_axi port = proj bundle=gmem1

        #pragma HLS INTERFACE s_axilite port = rand1 
        #pragma HLS INTERFACE s_axilite port = rand2
        #pragma HLS INTERFACE s_axilite port = rand3 
        #pragma HLS INTERFACE s_axilite port = rand4 
        #pragma HLS INTERFACE s_axilite port = rand5 
        #pragma HLS INTERFACE s_axilite port = rand6 
        #pragma HLS INTERFACE s_axilite port = orig 

        #pragma HLS INTERFACE s_axilite port = amount 
        #pragma HLS INTERFACE s_axilite port = vector_size
        #pragma HLS INTERFACE s_axilite port = proj 

        // TODO: use wider datatype: 512
        stream<ap_uint512_t> random_1;
        stream<ap_uint512_t> origin_pipe_1;
        stream<float> out_1;
    
       // As in Example4, we make use of Dataflow
       float const _orig[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};
       float const _rand1[] = {0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,0.10,0.11,0.12,0.13,0.14,0.15,0.16,0.17,0.18,0.19,0.20,0.21,0.22,0.23,0.24,0.25,0.26,0.27,0.28,0.29,0.30,0.31,0.32};
    //    int _amount = 2;
    //    int _vector_size = 32;

       #pragma HLS DATAFLOW

       ReadMemory_orig(vector_size, amount, _orig, origin_pipe_1);
       ReadMemory_rands(vector_size, amount, _rand1, random_1);

       PE_dotproduct(vector_size, amount, random_1, origin_pipe_1, out_1);

       WriteMemory(amount, out_1, proj);

    }
}