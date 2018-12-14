/*++

Copyright (c) 2004 - 2012, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  PeiPerformanceHob.h
    
Abstract:
  The PEI Firmware performance HOB definition.

--*/

#ifndef _PEI_PERFORMANCE_HOB_GUID_H_
#define _PEI_PERFORMANCE_HOB_GUID_H_

#define PEI_FIRMWARE_PERFORMANCE_GUID \
  { \
    0x55765e8f, 0x21a, 0x41f9, 0x93, 0x2d, 0x4c, 0x49, 0xc5, 0xb7, 0xef, 0x5d \
  }

#pragma pack(push, 1)
typedef struct _PEI_GUID_EVENT_REC {
  UINT16    ProgressID;
  EFI_GUID  Guid;
  UINT32    ApicID;
  UINT64    Timestamp;
} PEI_GUID_EVENT_REC;
#pragma pack(pop)

typedef struct {
  UINT32              NumberOfEntries;
  UINT32              Reserved;
  PEI_GUID_EVENT_REC  GuidEventRecord[1];
} PEI_FIRMWARE_PERFORMANCE_HOB;

#define PEIM_START_ID         0x1
#define PEIM_END_ID           0x2
#define PREMEM_START_ID       0x20
#define PREMEM_END_ID         0x21
#define DISMEM_START_ID       0x30
#define DISMEM_END_ID         0x31
#define POSTMEM_START_ID      0x40
#define POSTMEM_END_ID        0x41

extern EFI_GUID gPeiFirmwarePerformanceGuid;

#ifndef MAX_FIRMWARE_PERFORMANCE_ENTRIES
#define MAX_FIRMWARE_PERFORMANCE_ENTRIES 80
#endif

#endif