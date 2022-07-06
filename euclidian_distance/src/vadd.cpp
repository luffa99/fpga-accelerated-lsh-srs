// Includes
#include <hls_vector.h>
#include <hls_stream.h>
#include "assert.h"
#include "math.h"
#include "hls_math.h"

extern "C" {


void vadd(float in1[6],
          float in2[6],
          float out[1],
          int size) {
#pragma HLS INTERFACE m_axi port = in1 bundle = gmem0
#pragma HLS INTERFACE m_axi port = in2 bundle = gmem1
#pragma HLS INTERFACE m_axi port = out bundle = gmem0

    float ans = 0;
    for (int i = 0; i < 6; i++) {
        float a = in1[i];
        float b = in2[i];
        ans = ans + hls::sqrt( ( a+b ) * (a+b) ); 
    }
    out[0] = ans; 
}
}
