// Includes
#include <hls_vector.h>
#include <hls_stream.h>
#include "assert.h"
#include "math.h"

#define M 100
#define D 6

using namespace hls;

// Each PE mulitplies with one random vector
void PE_dotproduct  (   stream<float> &random_vector, 
                        stream<float> &origin_vector,
                        stream<float> &out_result) {
    
    // Adder Tree
    float sum = 0.0;
    float random_buffer[M], origin_buffer[M];
    // Divide buffers in small registers:
    // BUT -> Completely partitioning array 'random_buffer' (src/vadd.cpp:18) accessed 
    //through non-constant indices on dimension 1 (src/vadd.cpp:26:2), which may result 
    //in long runtime and suboptimal QoR due to large multiplexers. Please consider wrapping 
    //the array access into a function or using a register file core instead.
    #pragma HLS array_partition variable=random_buffer dim=1 complete
    #pragma HLS array_partition variable=origin_buffer dim=1 complete

    // Fill the buffers
    // loop1:
    // for (int i=0; i<M; i++) {
    // #pragma HLS pipeline II=1
    //     random_buffer[i] = random_vector.read();
    //     origin_buffer[i] = origin_vector.read();
    // }

    // // Do computation in parallel
    // // May need to manual enrroll
    // loop2:
    // for (int i=0; i<M / 10; i++) {
    //     #pragma HLS pipeline II=1 enable_flush rewind

    //     sum += random_buffer[i] * origin_buffer[i] 
    //         + random_buffer[i+1*M/10] * origin_buffer[i+1*M/10]
    //         + random_buffer[i+2*M/10] * origin_buffer[i+2*M/10]
    //         + random_buffer[i+3*M/10] * origin_buffer[i+3*M/10]
    //         + random_buffer[i+4*M/10] * origin_buffer[i+4*M/10]
    //         + random_buffer[i+5*M/10] * origin_buffer[i+5*M/10]
    //         + random_buffer[i+6*M/10] * origin_buffer[i+6*M/10]
    //         + random_buffer[i+7*M/10] * origin_buffer[i+7*M/10]
    //         + random_buffer[i+8*M/10] * origin_buffer[i+7*M/10]
    //         + random_buffer[i+9*M/10] * origin_buffer[i+7*M/10];
    // }

    // This way we are not reading in parallel, because FIFOs doesn't support
    // In fact, we have an II of 10!
    // What we can do: load from memory as 512-bit blocks, then divide them
    // in blocks of 16-bit (of a float) in different 16 FIFOs, so we can
    // read them in parallel!
    // See example:
    // https://github.com/WenqiJiang/Auto-ANNS-Accelerator/blob/MICRO/PE_optimization/helper_HBM_interconnection/HBM_parser_21_channels/src/HBM_interconnections.hpp#L140-L165
    // ap_ap_ap_uint512_to_three_PQ_codes

    loop2:
    for (int i=0; i<M / 10; i++) {
        #pragma HLS pipeline II=1 enable_flush rewind

        sum += random_vector.read() * origin_vector.read()
            + random_vector.read() * origin_vector.read()
            + random_vector.read() * origin_vector.read()
            + random_vector.read() * origin_vector.read()
            + random_vector.read() * origin_vector.read()
            + random_vector.read() * origin_vector.read()
            + random_vector.read() * origin_vector.read()
            + random_vector.read() * origin_vector.read()
            + random_vector.read() * origin_vector.read()
            + random_vector.read() * origin_vector.read();
    }

    // write result to out stream
    out_result.write(sum);
}

// Make 2 readMemory to do loops in parallel!
void ReadMemory ( float const *rands,
                        stream<float> &random_1,
                        stream<float> &random_2,
                        stream<float> &random_3,
                        stream<float> &random_4,
                        stream<float> &random_5,
                        stream<float> &random_6,
                        float const *orig,
                        stream<float> &origin_pipe_1,
                        stream<float> &origin_pipe_2,
                        stream<float> &origin_pipe_3,
                        stream<float> &origin_pipe_4,
                        stream<float> &origin_pipe_5,
                        stream<float> &origin_pipe_6) {

    // Divide the array in 6 parts may help
    // BUT -> Array_Partition/Array_Reshape pragma is ignored, because variable is scalar type
    // array_partiton only makes sense with DRAM memory (but rands is in HBM) #pragma HLS array_partition variable=rands dim=1 factor=6

    // All in 1 loop
    for (int i=0; i<M; i++) {
    #pragma HLS PIPELINE II=1
        random_1.write(rands[i]);
        random_2.write(rands[i+1*M]);
        random_3.write(rands[i+2*M]);
        random_4.write(rands[i+3*M]);
        random_5.write(rands[i+4*M]);
        random_6.write(rands[i+5*M]);
    }

     // Distribute the origin pipe into different streams
    for (int i=0; i<M; i++){
    #pragma HLS PIPELINE II=1
    /*
        Alternative to do broadcast: use systolic arrays; but with only 6 PEs it's fine
    */
        origin_pipe_1.write(orig[i]);
        origin_pipe_2.write(orig[i]);
        origin_pipe_3.write(orig[i]);
        origin_pipe_4.write(orig[i]);
        origin_pipe_5.write(orig[i]);
        origin_pipe_6.write(orig[i]);
    }

}

// Make 2 readMemory to do loops in parallel!
/*
    Very slow...how to speed-up??
*/
void ReadMemory_rands ( float const *rands,
                        stream<float> &random_1,
                        stream<float> &random_2,
                        stream<float> &random_3,
                        stream<float> &random_4,
                        stream<float> &random_5,
                        stream<float> &random_6) {

    // All in 1 loop
    for (int i=0; i<M; i++) {
    // #pragma HLS PIPELINE II=1
        random_1.write(rands[i]);
        random_2.write(rands[i+1*M]);
        random_3.write(rands[i+2*M]);
        random_4.write(rands[i+3*M]);
        random_5.write(rands[i+4*M]);
        random_6.write(rands[i+5*M]);
    }

}


void ReadMemory_orig (  float const *orig,
                        stream<float> &origin_pipe_1,
                        stream<float> &origin_pipe_2,
                        stream<float> &origin_pipe_3,
                        stream<float> &origin_pipe_4,
                        stream<float> &origin_pipe_5,
                        stream<float> &origin_pipe_6) {
    
    // Distribute the origin pipe into different streams
    for (int i=0; i<M; i++){
    #pragma HLS PIPELINE II=1
    /*
        Alternative to do broadcast: use systolic arrays; but with only 6 PEs it's fine
    */
        origin_pipe_1.write(orig[i]);
        origin_pipe_2.write(orig[i]);
        origin_pipe_3.write(orig[i]);
        origin_pipe_4.write(orig[i]);
        origin_pipe_5.write(orig[i]);
        origin_pipe_6.write(orig[i]);
    }

    
}


void WriteMemory    (   stream<float> &out_1,
                        stream<float> &out_2,
                        stream<float> &out_3,
                        stream<float> &out_4,
                        stream<float> &out_5,
                        stream<float> &out_6,
                        float *proj) {

    // Divide the array in all parts may help
    #pragma HLS array_partition variable=proj dim=1 complete
    proj[0] = out_1.read();
    proj[1] = out_2.read();
    proj[2] = out_3.read();
    proj[3] = out_4.read();
    proj[4] = out_5.read();
    proj[5] = out_6.read();
}

/*
    In general: replace double with float
*/
extern "C" {

    /*
        Vector Addition Kernel

        Arguments:
            in1  (input)  --> Input vector 1
            in2  (input)  --> Input vector 2
            out  (output) --> Output vector
            size (input)  --> Number of elements in vector
    */

    void vadd   (   float const *rands,
                    float const *orig,
                    float *proj,
                    int size_d,
                    int size_m) {

        #pragma HLS INTERFACE m_axi port = rands bundle=gmem0
        #pragma HLS INTERFACE m_axi port = orig bundle=gmem1

        #pragma HLS INTERFACE s_axilite port = rands 
        #pragma HLS INTERFACE s_axilite port = orig 

        #pragma HLS INTERFACE s_axilite port = size_d 
        #pragma HLS INTERFACE s_axilite port = size_m
        #pragma HLS INTERFACE s_axilite port = proj 

        stream<float> random_1;
        stream<float> random_2;
        stream<float> random_3;
        stream<float> random_4;
        stream<float> random_5;
        stream<float> random_6;
        stream<float> origin_pipe_1;
        stream<float> origin_pipe_2;
        stream<float> origin_pipe_3;
        stream<float> origin_pipe_4;
        stream<float> origin_pipe_5;
        stream<float> origin_pipe_6;
        stream<float> out_1;
        stream<float> out_2;
        stream<float> out_3;
        stream<float> out_4;
        stream<float> out_5;
        stream<float> out_6;
        /*
            -> May want to use stream of arrays!
        */
    
       // As in Example4, we make use of Dataflow
       #pragma HLS DATAFLOW

        ReadMemory_orig(orig, origin_pipe_1, origin_pipe_2, origin_pipe_3, origin_pipe_4, origin_pipe_5, origin_pipe_6 );
        ReadMemory_rands(rands, random_1, random_2, random_3, random_4, random_5, random_6);

       //ReadMemory_rands(rands, random_1, random_2, random_3, random_4, random_5, random_6, orig, origin_pipe_1, origin_pipe_2, origin_pipe_3, origin_pipe_4, origin_pipe_5, origin_pipe_6 );

       PE_dotproduct(random_1, origin_pipe_1, out_1);
       PE_dotproduct(random_2, origin_pipe_2, out_2);
       PE_dotproduct(random_3, origin_pipe_3, out_3);
       PE_dotproduct(random_4, origin_pipe_4, out_4);
       PE_dotproduct(random_5, origin_pipe_5, out_5);
       PE_dotproduct(random_6, origin_pipe_6, out_6);

       WriteMemory(out_1, out_2, out_3, out_4, out_5, out_6, proj);

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
    +---------+---------+----------+----------+-----+-----+----------+
    |  Latency (cycles) |  Latency (absolute) |  Interval | Pipeline |
    |   min   |   max   |    min   |    max   | min | max |   Type   |
    +---------+---------+----------+----------+-----+-----+----------+
    |      768|      768|  5.486 us|  5.486 us|  769|  769|  dataflow|
    +---------+---------+----------+----------+-----+-----+----------+
*/

/*
    Questions:
        - read memory rands is very slow! How better?

*/
