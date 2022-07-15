// Includes
#include <hls_vector.h>
#include <hls_stream.h>
#include "assert.h"
#include "math.h"
#include "ap_int.h"

#define M 100
#define D 6
#define var_size vector_size //vector_size or M
#define var_amount amount //amount or 1
#define MAX_VECT_SIZE 10000

using namespace hls;

extern "C" {


    void vadd   (   float const *rand1,
                    // float const *rand2,
                    // float const *rand3,
                    // float const *rand4,
                    // float const *rand5,
                    // float const *rand6,
                    // float const *orig,
                    float *proj,
                    int const amount,
                    int const vector_size) {

        #pragma HLS INTERFACE m_axi port = rand1 bundle=gmem2
        // #pragma HLS INTERFACE m_axi port = rand2 bundle=gmem3
        // #pragma HLS INTERFACE m_axi port = rand3 bundle=gmem4
        // #pragma HLS INTERFACE m_axi port = rand4 bundle=gmem5
        // #pragma HLS INTERFACE m_axi port = rand5 bundle=gmem6
        // #pragma HLS INTERFACE m_axi port = rand6 bundle=gmem7

        // #pragma HLS INTERFACE m_axi port = orig bundle=gmem0
        #pragma HLS INTERFACE m_axi port = proj bundle=gmem1

        #pragma HLS INTERFACE s_axilite port = rand1 
        // #pragma HLS INTERFACE s_axilite port = rand2
        // #pragma HLS INTERFACE s_axilite port = rand3 
        // #pragma HLS INTERFACE s_axilite port = rand4 
        // #pragma HLS INTERFACE s_axilite port = rand5 
        // #pragma HLS INTERFACE s_axilite port = rand6 
        // #pragma HLS INTERFACE s_axilite port = orig 

        #pragma HLS INTERFACE s_axilite port = amount 
        #pragma HLS INTERFACE s_axilite port = vector_size
        #pragma HLS INTERFACE s_axilite port = proj 

        // TODO: use wider datatype: 512

        // stream<ap_uint512_t> random_1;
        // stream<ap_uint512_t> random_2;
        // stream<ap_uint512_t> random_3;
        // stream<ap_uint512_t> random_4;
        // stream<ap_uint512_t> random_5;
        // stream<ap_uint512_t> random_6;
        // stream<ap_uint512_t> origin_pipe_1;
        // stream<ap_uint512_t> origin_pipe_2;
        // stream<ap_uint512_t> origin_pipe_3;
        // stream<ap_uint512_t> origin_pipe_4;
        // stream<ap_uint512_t> origin_pipe_5;
        // stream<ap_uint512_t> origin_pipe_6;

        ap_uint512_t temp;
        // Try "encoding" Random_1
        // i = total bits for rand1
        int size = vector_size*32;
        int i = 0;
        while (size >= 512) {
            for(int j=0; j<16; j++) {
                temp.range(512 - 1 - j*32, 512 - 32 - j * 32) = *((ap_uint<32>*)&rand1[i]);
                i++;
            }
            // random_1.write(temp);
            size -= 512;
        }
        // The rest
        if(size>0) {
            for(int j=0; size > 0; size -= 32) {
                temp.range(512 - 1 - j*32, 512 - 32 - j * 32) = *((ap_uint<32>*)&rand1[i]);
                i++;
                j++;
            }
            // random_1.write(temp);
        }

        // Try "decoding" Rrandom_1
        int sizeb = vector_size*32;
        int ib = 0;
        ap_uint512_t tempb;
        while(sizeb > 0) {
            // tempb = random_1.read();
            for(int j=0; j<16 && sizeb > 0; sizeb -= 32) {
                ap_uint<32> tmp_float = temp.range(512 - 1 - j*32, 512 - 32 - j * 32);
                proj[ib] = *((float*)&tmp_float);
                ib++;
                j++;
            }
        }



    }
}
