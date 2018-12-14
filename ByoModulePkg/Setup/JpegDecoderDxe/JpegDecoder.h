
#ifndef _JPEG_DECODER_H_
#define _JPEG_DECODER_H_

#include "JfifDecode.h"

#define JPEG_DECODER_INSTANCE_SIGNATURE   SIGNATURE_32('J','p','g','D') 
typedef struct {
  UINT32                          Signature;
  EFI_HANDLE                      Handle;
  //
  // Produced protocol(s)
  //
  EFI_JPEG_DECODER_PROTOCOL           JpegDecoder;

} JPEG_DECODER_INSTANCE;

#define JPEG_DECODER_INSTANCE_FROM_THIS(this) \
  CR(this, JPEG_DECODER_INSTANCE, JpegDecoder, JPEG_DECODER_INSTANCE_SIGNATURE)


EFI_STATUS
JpegDecoderDecodeImage (
  IN     EFI_JPEG_DECODER_PROTOCOL     *This,
  IN     UINT8                         *ImageData,
  IN     UINTN                         ImageDataSize,
     OUT UINT8                         **DecodedData,
     OUT UINTN                         *DecodedDataSize,
     OUT UINTN                         *Height,
     OUT UINTN                         *Width,
     OUT EFI_JPEG_DECODER_STATUS       *DecoderStatus
  );

EFI_STATUS
JpegDecoderGetMarkerData (
  IN     EFI_JPEG_DECODER_PROTOCOL     *This,
  IN     UINT8                         *Start,
  IN     UINT8                         *End,
  IN OUT EFI_JPEG_MARKER_TYPE          *MarkerType,
     OUT UINT8                         **MarkerData,
     OUT UINT32                        *DataSize,
     OUT UINT8                         **Next  OPTIONAL
  );


#endif
