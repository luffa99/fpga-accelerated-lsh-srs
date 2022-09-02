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

void a (stream<ap_uint512_t> &pipe, stream<float> &out_1, int const vsize, int const amount, float const* orig) {
    ap_uint512_t temp;
    int size = vsize*amount*32;
    int i = 0;
    while (size >= 512) { // Size is ALWAYS a multiple of 16 and so a multiple of 512
        for(int j=0; j<16; j++) {
        #pragma HLS unroll
            temp.range(512 - 1 - j*32, 512 - 32 - j * 32) = *((ap_uint<32>*)&orig[i]);
            i++;
        }
        pipe.write(temp);
        size -= 512;
    }

    float v = 69;
    out_1.write(v);
}

void WriteMemory    (   int const amount,
                        stream<float> &out_1,
                        float *proj,
                        stream<ap_uint512_t> &pipe) {

    ap_uint512_t temp = pipe.read();

    for(int j=0; j<5; j++) {
    #pragma HLS unroll
        ap_uint<32> tmp_float = temp.range(512 - 1 - j*32, 512 - 32 - j * 32);
        proj[j] = *((float*)&tmp_float);
    }
    proj[5] = out_1.read();
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
        stream<ap_uint512_t> origin_pipe_1;
        stream<float> out_1;

       #pragma HLS DATAFLOW

       a(origin_pipe_1, out_1, vector_size, amount, orig);
       WriteMemory(amount, out_1, proj, origin_pipe_1);

    }
}