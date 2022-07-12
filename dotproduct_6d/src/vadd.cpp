// Includes
#include <hls_vector.h>
#include <hls_stream.h>
#include "assert.h"
#include "math.h"

#define M 100
#define D 6
#define var_size vector_size //vector_size or M
#define var_amount amount //amount or 1

using namespace hls;

// Each PE mulitplies with one random vector
void PE_dotproduct  (   int const vector_size,
                        int const amount,
                        stream<float> &random_vector, 
                        stream<float> &origin_vector,
                        stream<float> &out_result) {
    
    // Repeat the same process for different vectors!
    for(int i=0; i<var_amount; i++) {
        float sum = 0.0;
        // This way we are not reading in parallel, because FIFOs doesn't support
        // In fact, we have an II of 10!
        // What we can do: load from memory as 512-bit blocks, then divide them
        // in blocks of 16-bit (of a float) in different 16 FIFOs, so we can
        // read them in parallel!
        // See example:
        // https://github.com/WenqiJiang/Auto-ANNS-Accelerator/blob/MICRO/PE_optimization/helper_HBM_interconnection/HBM_parser_21_channels/src/HBM_interconnections.hpp#L140-L165
        // ap_ap_ap_uint512_to_three_PQ_codes

        loop2:
        for (int i=0; i< var_size / 10; i++) {
            #pragma HLS pipeline II=1 rewind

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
}


// Made 2 readMemory to do loops in parallel!
void ReadMemory_rands ( int const vector_size,
                        int const amount,
                        float const *rands,
                        stream<float> &random_pipe) {

    // All in 1 loop
    for (int i=0; i<var_size*var_amount; i++) {
    #pragma HLS PIPELINE II=1
        random_pipe.write(rands[i%var_size]);
    }

}


void ReadMemory_orig (  int const vector_size,
                        int const amount,
                        float const *orig,
                        stream<float> &origin_pipe_1,
                        stream<float> &origin_pipe_2,
                        stream<float> &origin_pipe_3,
                        stream<float> &origin_pipe_4,
                        stream<float> &origin_pipe_5,
                        stream<float> &origin_pipe_6) {
    
    // Distribute the origin pipe into different streams
    for (int i=0; i<var_size*var_amount; i++){
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