// Includes
#include <hls_vector.h>
#include <hls_stream.h>
#include "assert.h"
#include "math.h"
#include "ap_int.h"

#define M 96
#define D 6
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
                        stream<ap_uint512_t> &origin_pipe_1,
                        stream<ap_uint512_t> &origin_pipe_2,
                        stream<ap_uint512_t> &origin_pipe_3,
                        stream<ap_uint512_t> &origin_pipe_4,
                        stream<ap_uint512_t> &origin_pipe_5,
                        stream<ap_uint512_t> &origin_pipe_6) {

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
        origin_pipe_2.write(temp);
        origin_pipe_3.write(temp);
        origin_pipe_4.write(temp);
        origin_pipe_5.write(temp);
        origin_pipe_6.write(temp);
        size -= 512;
    }
}


void WriteMemory    (   int const amount,
                        stream<float> &out_1,
                        stream<float> &out_2,
                        stream<float> &out_3,
                        stream<float> &out_4,
                        stream<float> &out_5,
                        stream<float> &out_6,
                        float *proj) {

    for(int i=0; i<var_amount; i++) {
        proj[0+i*6] = out_1.read();
        proj[1+i*6] = out_2.read();
        proj[2+i*6] = out_3.read();
        proj[3+i*6] = out_4.read();
        proj[4+i*6] = out_5.read();
        proj[5+i*6] = out_6.read();
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
        /*
            -> May want to use stream of arrays!
        */
    
       // As in Example4, we make use of Dataflow
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

       WriteMemory(amount, out_1, out_2, out_3, out_4, out_5, out_6, proj);

    }
}

/* + Performance & Resource Estimates: with fixed data: amount = 1 && size = 100 
    
    PS: '+' for module; 'o' for loop; '*' for dataflow
    +-------------------------------------------------+--------+-------+---------+-----------+----------+---------+------+----------+------+----------+-------------+------------+-----+
    |                     Modules                     |  Issue |       | Latency |  Latency  | Iteration|         | Trip |          |      |          |             |            |     |
    |                     & Loops                     |  Type  | Slack | (cycles)|    (ns)   |  Latency | Interval| Count| Pipelined| BRAM |    DSP   |      FF     |     LUT    | URAM|
    +-------------------------------------------------+--------+-------+---------+-----------+----------+---------+------+----------+------+----------+-------------+------------+-----+
    |+ vadd*                                          |  Timing|  -0.00|      145|  1.036e+03|         -|      112|     -|  dataflow|     -|  30 (~0%)|  17809 (~0%)|  20388 (1%)|    -|
    | + entry_proc                                    |       -|   3.80|        0|      0.000|         -|        0|     -|        no|     -|         -|      3 (~0%)|    29 (~0%)|    -|
    | + ReadMemory_orig                               |  Timing|  -0.00|      111|    792.873|         -|      111|     -|        no|     -|         -|    119 (~0%)|   400 (~0%)|    -|
    |  + ReadMemory_orig_Pipeline_VITIS_LOOP_84_1     |  Timing|  -0.00|      103|    735.729|         -|      103|     -|        no|     -|         -|     45 (~0%)|   131 (~0%)|    -|
    |   o VITIS_LOOP_84_1                             |       -|   5.21|      101|    721.443|         3|        1|   100|       yes|     -|         -|            -|           -|    -|
    | + ReadMemory_rands                              |  Timing|  -0.00|      111|    792.873|         -|      111|     -|        no|     -|         -|    118 (~0%)|   299 (~0%)|    -|
    |  + ReadMemory_rands_Pipeline_VITIS_LOOP_65_1    |  Timing|  -0.00|      103|    735.729|         -|      103|     -|        no|     -|         -|     45 (~0%)|    84 (~0%)|    -|
    |   o VITIS_LOOP_65_1                             |       -|   5.21|      101|    721.443|         3|        1|   100|       yes|     -|         -|            -|           -|    -|
    | + ReadMemory_rands_1                            |  Timing|  -0.00|      111|    792.873|         -|      111|     -|        no|     -|         -|    118 (~0%)|   299 (~0%)|    -|
    |  + ReadMemory_rands_1_Pipeline_VITIS_LOOP_65_1  |  Timing|  -0.00|      103|    735.729|         -|      103|     -|        no|     -|         -|     45 (~0%)|    84 (~0%)|    -|
    |   o VITIS_LOOP_65_1                             |       -|   5.21|      101|    721.443|         3|        1|   100|       yes|     -|         -|            -|           -|    -|
    | + ReadMemory_rands_2                            |  Timing|  -0.00|      111|    792.873|         -|      111|     -|        no|     -|         -|    118 (~0%)|   299 (~0%)|    -|
    |  + ReadMemory_rands_2_Pipeline_VITIS_LOOP_65_1  |  Timing|  -0.00|      103|    735.729|         -|      103|     -|        no|     -|         -|     45 (~0%)|    84 (~0%)|    -|
    |   o VITIS_LOOP_65_1                             |       -|   5.21|      101|    721.443|         3|        1|   100|       yes|     -|         -|            -|           -|    -|
    | + ReadMemory_rands_3                            |  Timing|  -0.00|      111|    792.873|         -|      111|     -|        no|     -|         -|    118 (~0%)|   299 (~0%)|    -|
    |  + ReadMemory_rands_3_Pipeline_VITIS_LOOP_65_1  |  Timing|  -0.00|      103|    735.729|         -|      103|     -|        no|     -|         -|     45 (~0%)|    84 (~0%)|    -|
    |   o VITIS_LOOP_65_1                             |       -|   5.21|      101|    721.443|         3|        1|   100|       yes|     -|         -|            -|           -|    -|
    | + ReadMemory_rands_4                            |  Timing|  -0.00|      111|    792.873|         -|      111|     -|        no|     -|         -|    118 (~0%)|   299 (~0%)|    -|
    |  + ReadMemory_rands_4_Pipeline_VITIS_LOOP_65_1  |  Timing|  -0.00|      103|    735.729|         -|      103|     -|        no|     -|         -|     45 (~0%)|    84 (~0%)|    -|
    |   o VITIS_LOOP_65_1                             |       -|   5.21|      101|    721.443|         3|        1|   100|       yes|     -|         -|            -|           -|    -|
    | + ReadMemory_rands_5                            |  Timing|  -0.00|      111|    792.873|         -|      111|     -|        no|     -|         -|    118 (~0%)|   299 (~0%)|    -|
    |  + ReadMemory_rands_5_Pipeline_VITIS_LOOP_65_1  |  Timing|  -0.00|      103|    735.729|         -|      103|     -|        no|     -|         -|     45 (~0%)|    84 (~0%)|    -|
    |   o VITIS_LOOP_65_1                             |       -|   5.21|      101|    721.443|         3|        1|   100|       yes|     -|         -|            -|           -|    -|
    | + PE_dotproduct                                 |       -|   0.08|      135|    964.305|         -|      135|     -|        no|     -|   5 (~0%)|   1449 (~0%)|  1159 (~0%)|    -|
    |  o loop2                                        |      II|   5.21|      134|    957.162|        45|       10|    10|       yes|     -|         -|            -|           -|    -|
    | + PE_dotproduct_6                               |       -|   0.08|      135|    964.305|         -|      135|     -|        no|     -|   5 (~0%)|   1449 (~0%)|  1159 (~0%)|    -|
    |  o loop2                                        |      II|   5.21|      134|    957.162|        45|       10|    10|       yes|     -|         -|            -|           -|    -|
    | + PE_dotproduct_7                               |       -|   0.08|      135|    964.305|         -|      135|     -|        no|     -|   5 (~0%)|   1449 (~0%)|  1159 (~0%)|    -|
    |  o loop2                                        |      II|   5.21|      134|    957.162|        45|       10|    10|       yes|     -|         -|            -|           -|    -|
    | + PE_dotproduct_8                               |       -|   0.08|      135|    964.305|         -|      135|     -|        no|     -|   5 (~0%)|   1449 (~0%)|  1159 (~0%)|    -|
    |  o loop2                                        |      II|   5.21|      134|    957.162|        45|       10|    10|       yes|     -|         -|            -|           -|    -|
    | + PE_dotproduct_9                               |       -|   0.08|      135|    964.305|         -|      135|     -|        no|     -|   5 (~0%)|   1449 (~0%)|  1159 (~0%)|    -|
    |  o loop2                                        |      II|   5.21|      134|    957.162|        45|       10|    10|       yes|     -|         -|            -|           -|    -|
    | + PE_dotproduct_10                              |       -|   0.08|      135|    964.305|         -|      135|     -|        no|     -|   5 (~0%)|   1449 (~0%)|  1159 (~0%)|    -|
    |  o loop2                                        |      II|   5.21|      134|    957.162|        45|       10|    10|       yes|     -|         -|            -|           -|    -|
    | + WriteMemory                                   |  Timing|  -0.00|       12|     85.716|         -|       12|     -|        no|     -|         -|    270 (~0%)|   203 (~0%)|    -|
    +-------------------------------------------------+--------+-------+---------+-----------+----------+---------+------+----------+------+----------+-------------+------------+-----+


    + Performance & Resource Estimates: with fixed data: amount = 100 && size = 100
    
    PS: '+' for module; 'o' for loop; '*' for dataflow
    +-------------------------------------------------+--------+-------+---------+-----------+----------+---------+-------+----------+------+----------+-------------+------------+-----+
    |                     Modules                     |  Issue |       | Latency |  Latency  | Iteration|         |  Trip |          |      |          |             |            |     |
    |                     & Loops                     |  Type  | Slack | (cycles)|    (ns)   |  Latency | Interval| Count | Pipelined| BRAM |    DSP   |      FF     |     LUT    | URAM|
    +-------------------------------------------------+--------+-------+---------+-----------+----------+---------+-------+----------+------+----------+-------------+------------+-----+
    |+ vadd*                                          |  Timing|  -0.00|    10045|  7.175e+04|         -|    10012|      -|  dataflow|     -|  30 (~0%)|  18218 (~0%)|  21363 (1%)|    -|
    | + entry_proc                                    |       -|   3.80|        0|      0.000|         -|        0|      -|        no|     -|         -|      3 (~0%)|    29 (~0%)|    -|
    | + ReadMemory_orig                               |  Timing|  -0.00|    10011|  7.151e+04|         -|    10011|      -|        no|     -|         -|    126 (~0%)|   409 (~0%)|    -|
    |  + ReadMemory_orig_Pipeline_VITIS_LOOP_84_1     |  Timing|  -0.00|    10003|  7.145e+04|         -|    10003|      -|        no|     -|         -|     52 (~0%)|   140 (~0%)|    -|
    |   o VITIS_LOOP_84_1                             |       -|   5.21|    10001|  7.144e+04|         3|        1|  10000|       yes|     -|         -|            -|           -|    -|
    | + ReadMemory_rands                              |  Timing|  -0.00|    10011|  7.151e+04|         -|    10011|      -|        no|     -|         -|    125 (~0%)|   308 (~0%)|    -|
    |  + ReadMemory_rands_Pipeline_VITIS_LOOP_65_1    |  Timing|  -0.00|    10003|  7.145e+04|         -|    10003|      -|        no|     -|         -|     52 (~0%)|    93 (~0%)|    -|
    |   o VITIS_LOOP_65_1                             |       -|   5.21|    10001|  7.144e+04|         3|        1|  10000|       yes|     -|         -|            -|           -|    -|
    | + ReadMemory_rands_1                            |  Timing|  -0.00|    10011|  7.151e+04|         -|    10011|      -|        no|     -|         -|    125 (~0%)|   308 (~0%)|    -|
    |  + ReadMemory_rands_1_Pipeline_VITIS_LOOP_65_1  |  Timing|  -0.00|    10003|  7.145e+04|         -|    10003|      -|        no|     -|         -|     52 (~0%)|    93 (~0%)|    -|
    |   o VITIS_LOOP_65_1                             |       -|   5.21|    10001|  7.144e+04|         3|        1|  10000|       yes|     -|         -|            -|           -|    -|
    | + ReadMemory_rands_2                            |  Timing|  -0.00|    10011|  7.151e+04|         -|    10011|      -|        no|     -|         -|    125 (~0%)|   308 (~0%)|    -|
    |  + ReadMemory_rands_2_Pipeline_VITIS_LOOP_65_1  |  Timing|  -0.00|    10003|  7.145e+04|         -|    10003|      -|        no|     -|         -|     52 (~0%)|    93 (~0%)|    -|
    |   o VITIS_LOOP_65_1                             |       -|   5.21|    10001|  7.144e+04|         3|        1|  10000|       yes|     -|         -|            -|           -|    -|
    | + ReadMemory_rands_3                            |  Timing|  -0.00|    10011|  7.151e+04|         -|    10011|      -|        no|     -|         -|    125 (~0%)|   308 (~0%)|    -|
    |  + ReadMemory_rands_3_Pipeline_VITIS_LOOP_65_1  |  Timing|  -0.00|    10003|  7.145e+04|         -|    10003|      -|        no|     -|         -|     52 (~0%)|    93 (~0%)|    -|
    |   o VITIS_LOOP_65_1                             |       -|   5.21|    10001|  7.144e+04|         3|        1|  10000|       yes|     -|         -|            -|           -|    -|
    | + ReadMemory_rands_4                            |  Timing|  -0.00|    10011|  7.151e+04|         -|    10011|      -|        no|     -|         -|    125 (~0%)|   308 (~0%)|    -|
    |  + ReadMemory_rands_4_Pipeline_VITIS_LOOP_65_1  |  Timing|  -0.00|    10003|  7.145e+04|         -|    10003|      -|        no|     -|         -|     52 (~0%)|    93 (~0%)|    -|
    |   o VITIS_LOOP_65_1                             |       -|   5.21|    10001|  7.144e+04|         3|        1|  10000|       yes|     -|         -|            -|           -|    -|
    | + ReadMemory_rands_5                            |  Timing|  -0.00|    10011|  7.151e+04|         -|    10011|      -|        no|     -|         -|    125 (~0%)|   308 (~0%)|    -|
    |  + ReadMemory_rands_5_Pipeline_VITIS_LOOP_65_1  |  Timing|  -0.00|    10003|  7.145e+04|         -|    10003|      -|        no|     -|         -|     52 (~0%)|    93 (~0%)|    -|
    |   o VITIS_LOOP_65_1                             |       -|   5.21|    10001|  7.144e+04|         3|        1|  10000|       yes|     -|         -|            -|           -|    -|
    | + PE_dotproduct                                 |       -|   0.08|    10035|  7.168e+04|         -|    10035|      -|        no|     -|   5 (~0%)|   1507 (~0%)|  1257 (~0%)|    -|
    |  o VITIS_LOOP_22_1_loop2                        |      II|   5.21|    10034|  7.167e+04|        45|       10|   1000|       yes|     -|         -|            -|           -|    -|
    | + PE_dotproduct_6                               |       -|   0.08|    10035|  7.168e+04|         -|    10035|      -|        no|     -|   5 (~0%)|   1507 (~0%)|  1257 (~0%)|    -|
    |  o VITIS_LOOP_22_1_loop2                        |      II|   5.21|    10034|  7.167e+04|        45|       10|   1000|       yes|     -|         -|            -|           -|    -|
    | + PE_dotproduct_7                               |       -|   0.08|    10035|  7.168e+04|         -|    10035|      -|        no|     -|   5 (~0%)|   1507 (~0%)|  1257 (~0%)|    -|
    |  o VITIS_LOOP_22_1_loop2                        |      II|   5.21|    10034|  7.167e+04|        45|       10|   1000|       yes|     -|         -|            -|           -|    -|
    | + PE_dotproduct_8                               |       -|   0.08|    10035|  7.168e+04|         -|    10035|      -|        no|     -|   5 (~0%)|   1507 (~0%)|  1257 (~0%)|    -|
    |  o VITIS_LOOP_22_1_loop2                        |      II|   5.21|    10034|  7.167e+04|        45|       10|   1000|       yes|     -|         -|            -|           -|    -|
    | + PE_dotproduct_9                               |       -|   0.08|    10035|  7.168e+04|         -|    10035|      -|        no|     -|   5 (~0%)|   1507 (~0%)|  1257 (~0%)|    -|
    |  o VITIS_LOOP_22_1_loop2                        |      II|   5.21|    10034|  7.167e+04|        45|       10|   1000|       yes|     -|         -|            -|           -|    -|
    | + PE_dotproduct_10                              |       -|   0.08|    10035|  7.168e+04|         -|    10035|      -|        no|     -|   5 (~0%)|   1507 (~0%)|  1257 (~0%)|    -|
    |  o VITIS_LOOP_22_1_loop2                        |      II|   5.21|    10034|  7.167e+04|        45|       10|   1000|       yes|     -|         -|            -|           -|    -|
    | + WriteMemory                                   |  Timing|  -0.00|      611|  4.364e+03|         -|      611|      -|        no|     -|         -|    282 (~0%)|   527 (~0%)|    -|
    |  + WriteMemory_Pipeline_VITIS_LOOP_110_1        |  Timing|  -0.00|      603|  4.307e+03|         -|      603|      -|        no|     -|         -|    209 (~0%)|   235 (~0%)|    -|
    |   o VITIS_LOOP_110_1                            |      II|   5.21|      601|  4.293e+03|         8|        6|    100|       yes|     -|         -|            -|           -|    -|
    +-------------------------------------------------+--------+-------+---------+-----------+----------+---------+-------+----------+------+----------+-------------+------------+-----+
*/