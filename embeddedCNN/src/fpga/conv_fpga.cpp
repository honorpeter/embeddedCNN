/*
    Desc:
    
        Conv Op in Xilinx FPGA.

    Note:
        
        Platform: ZCU102
        Toolkit: SDx 2018.1

    Date:

        06/04/2018

    Author:

        Yue Niu
*/

#include "sds_lib.h"

#include "../../include/fpga/conv_fpga.h"
#include "../../include/common.h"

#include "../../include/utils/check.h"

/*
  Top function for convolution in FPGA.
  In order to save data transfer bandwidth, ReLU and optional Pooling
  will be executed after convolution. 

  Argument:
    
    In, pointer to input data.
    Params, pointer to weights and bias
    Out, pointer to output data.
    Lyr, current convolution layer
    TilNum, total tile number in current layer
    ChnlTilNum: total channel tile number in current layer

  Note:
  
    in_buf mem arrangement(chnl-pos):
    0  |0-0  |1-0  |2-0  |...|15-0  |
    1  |0-1  |1-1  |2-1  |...|15-0  |
    2  |0-2  |1-2  |2-2  |...|15-0  |
       |...  |...  |...  |...|...   |
    224|0-224|1-224|2-224|...|15-224|

    out_buf mem arrangement(chnl-pos):
           0       1       2     ...    15
    ^   |0-0    |1-0    |2-0    |...|15-0   |
    |   |0-1    |1-1    |2-1    |...|15-0   |
    |   |0-2    |1-2    |2-2    |...|15-0   |
    |   |...    |...    |...    |...|...    |
    |   |0-441  |0-441  |2-441  |...|15-441 |
    |   |16-0   |17-0   |18-0   |...|31-0   |
    |   |16-1   |17-1   |18-1   |...|31-0   |
  32*441|16-2   |17-2   |18-2   |...|31-0   |
    |   |...    |...    |...    |...|...    |
    |   |16-441 |17-441 |18-441 |...|31-441 |
    |   |...    |...    |...    |...|...    |
    |   |496-0  |497-0  |498-0  |...|511-0  |
    |   |496-1  |497-1  |498-1  |...|511-0  |
    |   |496-2  |497-2  |498-2  |...|511-0  |
    |   |...    |...    |...    |...|...    |
    v   |496-441|497-441|498-441|...|511-441|

    w_buf mem arrangement(pos-ichnl-ochnl)
          0      1    ...   15      16     17         31    ...   255
    ^  |0-0-0 |0-1-0 |...|0-15-0 |0-0-1 |0-1-1 |...|0-15-1 |...|0-15-15|
    |  |1-0-0 |1-1-0 |...|1-15-0 |1-0-1 |1-1-1 |...|1-15-1 |...|1-15-15|
    |  |...   |...   |...|...    |...   |...   |...|...    |...|...    |
    |  |9-0-0 |9-1-0 |...|9-15-0 |9-0-1 |9-1-1 |...|9-15-1 |...|9-15-15|
   288 |0-0-16|0-1-16|...|0-15-16|0-0-17|0-1-17|...|0-15-17|...|0-15-31|
    |  |1-0-16|1-1-16|...|0-15-16|1-0-17|1-1-17|...|1-15-17|...|1-15-31|
    |  |...   |...   |...|...    |...   |...   |...|...    |...|...    |
    |  |9-0-16|9-1-16|...|0-15-16|9-0-17|9-1-17|...|9-15-17|...|9-15-31|
    v  |...   |...   |...|...    |...   |...   |...|...    |...|...    |
    
*/

void conv_fpga(Dtype *In, 
               int Lyr, 
               int TilNum, 
               int ChnlRead, 
               int ColNum)
{
  // Set on-chip buffer
  Dtype in_buf[ITILE][FTILE_W * FTILE_H];
  #pragma HLS array_partition variable=in_buf complete dim=1

  // Dtype out_buf[O_BUF_DEPTH][O_BUF_ROW * FTILE_W * FTILE_H];
  // int fac = OTILE;
  // #pragma HLS array_partition variable=out_buf cyclic factor=fac dim=1

  // Dtype w_buf[OTILE * ITILE * W_BUF_DEPTH];
  // fac = ITILE*OTILE;
  // #pragma HLS array_reshape variable=w_buf cyclic factor=fac dim=1

  // Dtype b_buf[OTILE];
  // #pragma HLS array_partition variable=b_buf complete

  /* Start Convolution */
  // int chnl_num = Lyr == 0 ? 3 : CHNEL[Lyr - 1]; 
  for (int til = 0; til < TilNum; til++){
  //#pragma HLS DATAFLOW
    // Read input feature
    buf_read(In + til * ChnlRead * FTILE_W, // incr In
             in_buf, 
             ChnlRead,
             ColNum); 

    std::cout << "[INFO] " << __FUNCTION__ << ", " << __LINE__ << 
                 ": Check InBuf." << std::endl;
    inbuf_check(In, in_buf, Lyr, til);

    // Conditional read weight

    // Conv op 
  }/* Start Convolution */

  return;
}

/*
  Read inputs from external to in_buf.

  Argument:

    In, pointer to in_buf.
    InBuf, on-chip buffer.
    ChnlNum, input channels to read.
    ColNum, col number to read.

  Note:

    In current design, stride was set to be default 1.
    If stride is not 1, the code should be modified.
*/
void buf_read(Dtype * In, 
              Dtype InBuf[ITILE][FTILE_W * FTILE_H],
              int ChnlNum,
              int ColNum)
{
  for (int n = 0; n < ChnlNum; n++){
    for (int r = 0; r < FTILE_H; r++){
      for (int c = 0; c < ColNum; c++){
      #pragma HLS PIPELINE
      #pragma HLS dependence variable=InBuf inter false
        InBuf[n][r * ColNum + c] = *In++;
      }
    }
  }/* for: input channel tile */

  return;
}
