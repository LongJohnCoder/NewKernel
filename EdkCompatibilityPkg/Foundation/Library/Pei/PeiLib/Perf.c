/*++

Copyright (c) 2004 - 2012, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Perf.c

Abstract:

  Support for FPDT performance structures. 

--*/

#include "Tiano.h"
#include "Pei.h"
#include "PeiLib.h"
#include "PeiHob.h"
#include "CpuIA32.h"
#include "EfiCommonLib.h"

#include EFI_GUID_DEFINITION (PeiPerformanceHob)

#define MSR_PLATFORM_INFO             0xce
#define MAX_NON_TURBO_RATIO_OFFSET    8
#define MAX_NON_TURBO_RATIO_MASK      0xff
#define LOCAL_APIC_BASE               0xfee00000
#define APIC_ID_REGISTER              0x20
#define MSR_EXT_XAPIC_LOGICAL_APIC_ID 0x802
#define MSR_XAPIC_BASE                0x1b
#define MSR_XAPIC_BASE_MASK           0x0c00

//
// Prototype functions
//  
UINT64
IA32API
EfiReadMsr (
  IN UINT32     Index
  );

UINT64 GetTimeInNanoSec (
  UINT64 Ticker
  )
/*++

Routine Description:

  Internal routine to convert TSC value into nano second value

Arguments:

  Ticker - OPTIONAL. TSC value supplied by caller function

Returns:

  UINT64 - returns calculated timer value

--*/
{
  UINT64 Tick, pi;
  UINT8  Ratio;

  if(Ticker != 0){
    Tick = Ticker;
  } else {
    Tick = EfiReadTsc();
  }

  pi = EfiReadMsr(MSR_PLATFORM_INFO);
  Ratio = (UINT8)( ((UINT32)(UINTN)RShiftU64(pi, MAX_NON_TURBO_RATIO_OFFSET)) & MAX_NON_TURBO_RATIO_MASK);

  return (UINT64)DivU64x32(MultU64x32(Tick, 10), (UINTN)(Ratio), NULL);
}


UINT32 GetApicId (
  VOID
  )
/*++

Routine Description:

  Internal routine to retrieve current APIC Id

Arguments:

  None

Returns:

  UINT32 - returns Apic Id value

--*/
{
  BOOLEAN x2ApicEnabled;
  UINT32  ApicId;

  x2ApicEnabled = (BOOLEAN)(((EfiReadMsr (MSR_XAPIC_BASE)) & (MSR_XAPIC_BASE_MASK)) == MSR_XAPIC_BASE_MASK);
  if (x2ApicEnabled) {
    ApicId = (UINT32) EfiReadMsr (MSR_EXT_XAPIC_LOGICAL_APIC_ID);
  } else {
    ApicId = (UINT8) (*(volatile UINT32 *) (UINTN) (LOCAL_APIC_BASE + APIC_ID_REGISTER) >> 24);
  }

  return ApicId;
}

VOID
PeiPerfMeasureEx (
  EFI_PEI_SERVICES       **PeiServices,
  IN UINT16              *Token,
  IN EFI_FFS_FILE_HEADER *FileHeader,
  IN UINT16              Identifier,
  IN BOOLEAN             EntryExit,
  IN UINT64              Value
  )
/*++

Routine Description:

  Log an extended timestamp value.

Arguments:

  PeiServices - Pointer to the PEI Core Services table
  
  Token       - Pointer to Token Name
  
  FileHeader  - Pointer to the file header

  Identifier  - Identifier of the record

  EntryExit   - Indicates start or stop measurement

  Value       - The TSC value

Returns:

  None

--*/
{
  EFI_STATUS                   Status;
  PEI_FIRMWARE_PERFORMANCE_HOB *FirmwarePerformanceHob;
  PEI_GUID_EVENT_REC           *PeiGuidRec;
  VOID                         *HobList;
  EFI_PEI_HOB_POINTERS         GuidHob;

  //
  // Get Hob Start
  //
  HobList = NULL;
  (*PeiServices)->GetHobList (PeiServices, &HobList);
  GuidHob.Raw = HobList;

  //
  // Find specific GuidHob
  //
  FirmwarePerformanceHob = NULL;
  for (Status = EFI_NOT_FOUND; EFI_ERROR (Status); ) {
    if (END_OF_HOB_LIST (GuidHob)) {
      break;
    }
    
    if (GuidHob.Header->HobType == EFI_HOB_TYPE_GUID_EXTENSION) {            
      if (EfiCompareGuid (&gPeiFirmwarePerformanceGuid, &GuidHob.Guid->Name)) {
        FirmwarePerformanceHob = (PEI_FIRMWARE_PERFORMANCE_HOB *) (GuidHob.Guid + 1);
        Status  = EFI_SUCCESS;
        break;
      }
    }

    GuidHob.Raw = GET_NEXT_HOB (GuidHob);
  }
  
  //
  // If the Performance Hob was not found, build one.
  //
  if (FirmwarePerformanceHob == NULL || EFI_ERROR(Status)) {
    Status = PeiBuildHobGuid (
               PeiServices,
               &gPeiFirmwarePerformanceGuid,
               (sizeof(PEI_FIRMWARE_PERFORMANCE_HOB) +
                 ((MAX_FIRMWARE_PERFORMANCE_ENTRIES-1) * 
                 sizeof(PEI_GUID_EVENT_REC))
               ),
               &GuidHob.Guid
               );
    ASSERT_PEI_ERROR(PeiServices, Status);
    FirmwarePerformanceHob = (PEI_FIRMWARE_PERFORMANCE_HOB *)(GuidHob.Guid+1);
    FirmwarePerformanceHob->NumberOfEntries = 0;
    FirmwarePerformanceHob->Reserved = 0;
  }

  //
  // return if performance buffer has filled up
  //
  if (FirmwarePerformanceHob->NumberOfEntries >= MAX_FIRMWARE_PERFORMANCE_ENTRIES) {
    return;
  }

  PeiGuidRec = &(FirmwarePerformanceHob->GuidEventRecord[FirmwarePerformanceHob->NumberOfEntries]);
  (*PeiServices)->SetMem (PeiGuidRec, sizeof(PEI_GUID_EVENT_REC), 0);

  //
  // Get the GUID name
  //
  if (FileHeader != NULL) {
    PeiGuidRec->Guid = FileHeader->Name;
  }

  //
  // Record the time stamp nanosec value.
  //
  PeiGuidRec->Timestamp = GetTimeInNanoSec(Value);

  //
  // Copy the progress ID
  //
  PeiGuidRec->ProgressID = Identifier;

  //
  // Record the APIC Id
  //
  PeiGuidRec->ApicID = GetApicId();

  //
  // Increment the number of valid log entries.
  //
  FirmwarePerformanceHob->NumberOfEntries++;

  return;
}

VOID
PeiPerfMeasure (
  EFI_PEI_SERVICES       **PeiServices,
  IN UINT16              *Token,
  IN EFI_FFS_FILE_HEADER *FileHeader,
  IN BOOLEAN             EntryExit,
  IN UINT64              Value
  )
/*++

Routine Description:

  Log a timestamp count.

Arguments:

  PeiServices - Pointer to the PEI Core Services table
  
  Token       - Pointer to Token Name
  
  FileHeader  - Pointer to the file header

  EntryExit   - Indicates start or stop measurement

  Value       - The start time or the stop time

Returns:

--*/
{
  EFI_STATUS                   Status;
  PEI_FIRMWARE_PERFORMANCE_HOB *FirmwarePerformanceHob;
  PEI_GUID_EVENT_REC           *PeiGuidRec;
  VOID                         *HobList;
  EFI_PEI_HOB_POINTERS         GuidHob;

  //
  // Get Hob Start
  //
  HobList = NULL;
  (*PeiServices)->GetHobList (PeiServices, &HobList);
  GuidHob.Raw = HobList;

  //
  // Find specific GuidHob
  //
  FirmwarePerformanceHob = NULL;
  for (Status = EFI_NOT_FOUND; EFI_ERROR (Status); ) {
    if (END_OF_HOB_LIST (GuidHob)) {
      break;
    }
    
    if (GuidHob.Header->HobType == EFI_HOB_TYPE_GUID_EXTENSION) {            
      if (EfiCompareGuid (&gPeiFirmwarePerformanceGuid, &GuidHob.Guid->Name)) {
        FirmwarePerformanceHob = (PEI_FIRMWARE_PERFORMANCE_HOB *) (GuidHob.Guid + 1);
        Status  = EFI_SUCCESS;
        break;
      }
    }

    GuidHob.Raw = GET_NEXT_HOB (GuidHob);
  }
  
  //
  // If the Performance Hob was not found, build one.
  //
  if (FirmwarePerformanceHob == NULL || EFI_ERROR(Status)) {
    Status = PeiBuildHobGuid (
               PeiServices,
               &gPeiFirmwarePerformanceGuid,
               (sizeof(PEI_FIRMWARE_PERFORMANCE_HOB) +
                 ((MAX_FIRMWARE_PERFORMANCE_ENTRIES-1) * 
                 sizeof(PEI_GUID_EVENT_REC))
               ),
               &GuidHob.Guid
               );
    ASSERT_PEI_ERROR(PeiServices, Status);
    FirmwarePerformanceHob = (PEI_FIRMWARE_PERFORMANCE_HOB *)(GuidHob.Guid+1);
    FirmwarePerformanceHob->NumberOfEntries = 0;
    FirmwarePerformanceHob->Reserved = 0;
  }

  if (FirmwarePerformanceHob->NumberOfEntries >= MAX_FIRMWARE_PERFORMANCE_ENTRIES) {
    return;
  }

  PeiGuidRec = &(FirmwarePerformanceHob->GuidEventRecord[FirmwarePerformanceHob->NumberOfEntries]);
  (*PeiServices)->SetMem (PeiGuidRec, sizeof(PEI_GUID_EVENT_REC), 0);

  //
  // If not NULL pointer, copy the file name
  //
  if (FileHeader != NULL) {
    PeiGuidRec->Guid = FileHeader->Name;
  }

  //
  // Record the time stamp nanosec value.
  //
  PeiGuidRec->Timestamp = GetTimeInNanoSec(Value);

  //
  // Record the Progress Id based upon token field
  //
  if (!EfiStrCmp (Token, L"PEIM")) {
    if(!EntryExit) {
      PeiGuidRec->ProgressID = PEIM_START_ID;
    } else {
      PeiGuidRec->ProgressID = PEIM_END_ID;
    }   
  } else if (!EfiStrCmp (Token, L"PreMem")) {
    if(!EntryExit) {
      PeiGuidRec->ProgressID = PREMEM_START_ID;
    } else {
      PeiGuidRec->ProgressID = PREMEM_END_ID;
    }   
  } else if (!EfiStrCmp (Token, L"DisMem")) {
    if(!EntryExit) {
      PeiGuidRec->ProgressID = DISMEM_START_ID;
    } else {
      PeiGuidRec->ProgressID = DISMEM_END_ID;
    }   
  } else if (!EfiStrCmp (Token, L"PostMem")) {
    if(!EntryExit) {
      PeiGuidRec->ProgressID = POSTMEM_START_ID;
    } else {
      PeiGuidRec->ProgressID = POSTMEM_END_ID;
    }   
  } else {
    return ;
  }
  //
  // Record the APIC Id
  //
  PeiGuidRec->ApicID = GetApicId();

  //
  // Increment the number of valid log entries.
  //
  FirmwarePerformanceHob->NumberOfEntries++;

  return;
}
