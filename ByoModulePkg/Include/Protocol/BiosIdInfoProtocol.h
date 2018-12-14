#ifndef __BIOS_ID_INFO_H__
#define __BIOS_ID_INFO_H__

#include <Base.h>

typedef struct {
  CHAR8  BiosVer[32];
  CHAR8  BiosDate[11];
  CHAR8  BoardId[16];     // setup may be required for a special name, so give a enough buffer.
  UINT8  VerMajor;
  UINT8  VerMinor;
} BIOS_ID_INFO;

extern EFI_GUID gBiosIdInfoProtocolGuid;

#endif