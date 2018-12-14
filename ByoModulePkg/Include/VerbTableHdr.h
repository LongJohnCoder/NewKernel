
#ifndef __VERB_TABLE_HDR_H__
#define __VERB_TABLE_HDR_H__

#include <Base.h>

typedef struct {
  UINT32  Vdid;
  UINT32  Ssid;
  UINT8   RevId;
  UINT8   FpSupport;
  UINT16  RearJacks;
  UINT16  FrontJacks;
} OEM_VERB_TABLE_HEADER;

typedef struct {
  OEM_VERB_TABLE_HEADER  Hdr;
  UINTN                  VerbDataSize;
  UINT32                 *VerbData;
} OEM_VERB_TABLE;

#endif