

#ifndef _HUFFMAN_DECODE_H_
#define _HUFFMAN_DECODE_H_

#include "JfifDecode.h"

#include <Protocol/UgaDraw.h>


#define Cos0 1448 /* 4096*sqrt(1/8): 12 bits*/
#define Cos1 2009 /* 4096*(1/2)*cos(1*pi/16): 12 bits*/
#define Cos2 1892 /* 4096*(1/2)*cos(2*pi/16): 12 bits*/
#define Cos3 1703 /* 4096*(1/2)*cos(3*pi/16): 12 bits*/
#define Cos4 1448 /* 4096*(1/2)*cos(4*pi/16): 12 bits*/
#define Cos5 1138 /* 4096*(1/2)*cos(5*pi/16): 12 bits*/
#define Cos6 784  /* 4096*(1/2)*cos(6*pi/16): 12 bits*/
#define Cos7 500  /* 4096*(1/2)*cos(7*pi/16): 12 bits*/

UINT8
GetNextBit (
  OUT    BOOLEAN                        *IsEnd
  );

BOOLEAN
ReadByte (
  );

EFI_STATUS 
ElementDecode (
  IN        UINT8                        HTIndex,
     OUT    UINT8                        *ZeroCount,
     OUT    INT16                        *DecodedVal,  
     OUT    BOOLEAN                      *IsEnd, 
     OUT    EFI_JPEG_DECODER_STATUS      *DecoderStatus
  );

EFI_STATUS 
BlockDecode (
     OUT    INT16                        *BlockBuff,
  IN        UINT8                        DcAcHTIndex,
     OUT    BOOLEAN                      *IsEnd, 
     OUT    EFI_JPEG_DECODER_STATUS      *DecoderStatus
 );

VOID 
BlockDequantDezigzag (
  IN        INT16                        *BlockSrcBuff,
     OUT    INT16                        *BlockDstBuff,
  IN        UINT8                         BlockIndex
  );

VOID 
BlockIDctAddoffset (
  IN  OUT   INT16                        *BlockSrcDstBuff,
  IN        UINT8                         BlockIndex
  );

EFI_STATUS 
McuRetrieve (
     OUT    INT16                        *McuDstBuff,
  IN        BOOLEAN                      IntervalFlag, 
     OUT    BOOLEAN                      *IsEnd, 
     OUT    EFI_JPEG_DECODER_STATUS      *DecoderStatus
  );

VOID 
McuDecode (
  IN        INT16                        *McuSrcBuff,
     OUT    INT16                        *McuDstBuff
  );


VOID 
StoreMcuToUgaBlt (
  IN        INT16                        *McuSrcBuff,
  IN  OUT   UINT8                        **DstBuff,
  IN  OUT   UINT16                       *CurHPixel,
  IN  OUT   UINT16                       *CurVLine
  );

#endif
