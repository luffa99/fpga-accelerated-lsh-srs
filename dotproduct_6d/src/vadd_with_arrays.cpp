// Includes
#include <hls_vector.h>
#include <hls_stream.h>
#include "assert.h"
#include "math.h"

#define M 100
#define D 6

using namespace hls;

void PE_dotproduct  (   float const random_vector[M], 
                        float const origin_vector[M],
                        stream<float> &out_result) {
    
    // Adder Tree
    float sum = 0.0;

    loop2:
    for (int i=0; i<M; i++) {
        // #pragma HLS pipeline II=1 enable_flush rewind
        #pragma HLS unroll
        sum += random_vector[i] * origin_vector[i];
    }

    // write result to out stream
    out_result.write(sum);
}


// Making 2 readMemory to do loops in parallel!
/*
    Very slow...how to speed-up??
*/
void ReadMemory_rands ( float const rands[M*D],
                        float *random_1,
                        float *random_2,
                        float *random_3,
                        float *random_4,
                        float *random_5,
                        float *random_6) {

    // All in 1 loop
    for (int i=0; i<M; i++) {
    // #pragma HLS PIPELINE II=1
        random_1[i] = rands[i];
        random_2[i] = rands[i+1*M];
        random_3[i] = rands[i+2*M];
        random_4[i] = rands[i+3*M];
        random_5[i] = rands[i+4*M];
        random_6[i] = rands[i+5*M];
    }

}


void ReadMemory_orig (float const *orig,
                        float *origin_pipe_1,
                        float *origin_pipe_2,
                        float *origin_pipe_3,
                        float *origin_pipe_4,
                        float *origin_pipe_5,
                        float *origin_pipe_6 ) {

    // Distribute the origin pipe into different streams
    for (int i=0; i<M; i++){
    // #pragma HLS PIPELINE II=1
        origin_pipe_1[i] = orig[i];
        origin_pipe_2[i] = orig[i];
        origin_pipe_3[i] = orig[i];
        origin_pipe_4[i] = orig[i];
        origin_pipe_5[i] = orig[i];
        origin_pipe_6[i] = orig[i];
    }
}


void WriteMemory    (   stream<float> &out_1,
                        stream<float> &out_2,
                        stream<float> &out_3,
                        stream<float> &out_4,
                        stream<float> &out_5,
                        stream<float> &out_6,
                        float proj[D]) {

    // Divide the array in all parts may help
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

    void vadd   (   float const rands[M*D],
                    float const *orig,
                    float proj[D],
                    int size_d,
                    int size_m) {

        #pragma HLS INTERFACE m_axi port = rands bundle=gmem0
        #pragma HLS INTERFACE m_axi port = orig bundle=gmem1
        #pragma HLS INTERFACE m_axi port = proj bundle=gmem2

        #pragma HLS INTERFACE s_axilite port = rands 
        #pragma HLS INTERFACE s_axilite port = orig 

        #pragma HLS INTERFACE s_axilite port = size_d 
        #pragma HLS INTERFACE s_axilite port = size_m
        #pragma HLS INTERFACE s_axilite port = proj 

        float random_1[M];
        float random_2[M];
        float random_3[M];
        float random_4[M];
        float random_5[M];
        float random_6[M];
        float origin_pipe_1[M];
        float origin_pipe_2[M];
        float origin_pipe_3[M];
        float origin_pipe_4[M];
        float origin_pipe_5[M];
        float origin_pipe_6[M];
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


    #pragma HLS array_partition variable=random_1 dim=0 complete
    #pragma HLS array_partition variable=random_2 dim=0 complete
    #pragma HLS array_partition variable=random_3 dim=0 complete
    #pragma HLS array_partition variable=random_4 dim=0 complete
    #pragma HLS array_partition variable=random_5 dim=0 complete
    #pragma HLS array_partition variable=random_6 dim=0 complete

    #pragma HLS array_partition variable=origin_pipe_1 dim=0 complete
    #pragma HLS array_partition variable=origin_pipe_2 dim=0 complete
    #pragma HLS array_partition variable=origin_pipe_3 dim=0 complete
    #pragma HLS array_partition variable=origin_pipe_4 dim=0 complete
    #pragma HLS array_partition variable=origin_pipe_5 dim=0 complete
    #pragma HLS array_partition variable=origin_pipe_6 dim=0 complete


    #pragma HLS array_partition variable=proj dim=1 complete

       PE_dotproduct(random_1, origin_pipe_1, out_1);
       PE_dotproduct(random_2, origin_pipe_2, out_2);
       PE_dotproduct(random_3, origin_pipe_3, out_3);
       PE_dotproduct(random_4, origin_pipe_4, out_4);
       PE_dotproduct(random_5, origin_pipe_5, out_5);
       PE_dotproduct(random_6, origin_pipe_6, out_6);

       WriteMemory(out_1, out_2, out_3, out_4, out_5, out_6, proj);

    }
}
