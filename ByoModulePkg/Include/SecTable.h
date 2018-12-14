#ifndef __BIOS_SEC_TABLE_H__
#define __BIOS_SEC_TABLE_H__


#define BIOS_SEC_TABLE_SIGN    0x244C425443455324ULL

#pragma pack(1)

typedef struct {
  UINT64  Sign;
  UINT32  Version;
  UINT32  BiosStartAddress;
  UINT32  FvMainStart;
  UINT32  FvMainSize;
  UINT32  EventLogStart;
  UINT32  EventLogSize;
} BIOS_SEC_TABLE_V1;

typedef struct {
  UINT64  Sign;
  UINT32  Version;
  UINT32  BiosStartAddress;
  UINT32  FvMainStart;
  UINT32  FvMainSize;
  UINT32  EventLogStart;
  UINT32  EventLogSize;
  
  UINT32  BiosSize;
  UINT16  AcpiIoBase;
  UINT16  SmiPort;
  UINT32  BiosInfoOffset;
  UINT32  BiosIdAddress;
  UINT32  FvBBBUAddress;
} BIOS_SEC_TABLE_V2;

#pragma pack()

#endif

