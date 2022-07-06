// Includes
#include <hls_vector.h>
#include <hls_stream.h>
#include "assert.h"
#include "math.h"
#include "hls_math.h"

extern "C" {

/*
    Vector Addition Kernel

    Arguments:
        in1  (input)  --> Input vector 1
        in2  (input)  --> Input vector 2
        out  (output) --> Output vector
        size (input)  --> Number of elements in vector
*/

void vadd(double* rands,
          double* orig,
          double* proj,
          int size_d,
          int size_m) {
#pragma HLS INTERFACE m_axi port = rands
#pragma HLS INTERFACE m_axi port = orig

#pragma HLS INTERFACE s_axilite port = rands
#pragma HLS INTERFACE s_axilite port = orig

#pragma HLS INTERFACE s_axilite port = size_d
#pragma HLS INTERFACE s_axilite port = size_m
#pragma HLS INTERFACE s_axilite port = proj 

    #pragma HLS UNROLL
    /* Unroll for-loops to create multiple instances of the loop body and its 
    instructions that can then be scheduled independently. */
    // size_m must be constant!
    // TODO: divide in PEs (8) and use streams
    for (int j=0; j<size_m; j++){
        double acc = 0;
        // TODO: adder tree -> check Johannes tutorial
        for (int i=0; i<size_d; i++){
            acc += rands[j*size_d+i] * orig[i];
        }
        proj[j] = acc;
    }

}
}

/*

================================================================
+ Timing: 
    * Summary: 
    +--------+---------+----------+------------+
    |  Clock |  Target | Estimated| Uncertainty|
    +--------+---------+----------+------------+
    |ap_clk  |  7.14 ns|  5.214 ns|     1.93 ns|
    +--------+---------+----------+------------+

+ Latency: 
    * Summary: 
    +---------+---------+-----------+----------+-----+-----+---------+
    |  Latency (cycles) |  Latency (absolute)  |  Interval | Pipeline|
    |   min   |   max   |    min    |    max   | min | max |   Type  |
    +---------+---------+-----------+----------+-----+-----+---------+
    |        7|        ?|  50.001 ns|         ?|    8|    ?|     none|
    +---------+---------+-----------+----------+-----+-----+---------+

*/