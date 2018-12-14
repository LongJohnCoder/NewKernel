/*++

Copyright (c) 2011 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.              

Module Name:

  EdkIIGluePeiFirmwarePerformanceLib.c
  
Abstract:

  PEI Library for FPDT performance logging. 

--*/

#include "EdkIIGluePeim.h"
#include EFI_GUID_DEFINITION (PeiPerformanceHob)

//
// MAX perfomance HOB Entries
//

EFI_STATUS
PeiPerfMeasureEx (
  IN VOID    *FileHeader,
  IN UINT16  *Token,
  IN BOOLEAN EntryExit,
  IN UINT64  TimeStamp,
  IN UINT16  Identifier
  )
/*++

Routine Description:

  Log an extended timestamp value into pre-allocated hob.

Arguments:

  FileHeader  - Pointer to the file header

  Token       - Pointer to Token Name
  
  EntryExit   - Indicates start or stop measurement

  Timestamp   - The TSC value

  Identifier  - Identifier of the record  

Returns:

  EFI_BUFFER_TOO_SMALL - Allocate buffer is not enough to hold new records
  
  EFI_SUCCESS          - Successfully updated the record in hob

--*/
{
  PEI_FIRMWARE_PERFORMANCE_HOB  *FirmwarePerformanceHob;
  PEI_GUID_EVENT_REC            *PeiGuidRec;
  EFI_PEI_SERVICES              **PeiServices;
  UINTN                         BufferSize;
  EFI_HOB_GUID_TYPE             *GuidHob;

  PeiServices = GetPeiServicesTablePointer ();
  //
  // Locate the Pei Performance Log Hob.
  //
  GuidHob = GetFirstGuidHob (&gPeiFirmwarePerformanceGuid);
  ASSERT (GuidHob != NULL);

  if (GuidHob != NULL) {
    //
    // PEI Performance HOB was found, then return the existing one.
    //
    FirmwarePerformanceHob = GET_GUID_HOB_DATA (GuidHob);
  } else {
    //
    // PEI Performance HOB was not found, then build one.
    //
    BufferSize = (UINT16) (sizeof (PEI_FIRMWARE_PERFORMANCE_HOB) + 
                            ((MAX_FIRMWARE_PERFORMANCE_ENTRIES-1) * sizeof (PEI_GUID_EVENT_REC))
                           );

    FirmwarePerformanceHob = BuildGuidHob (
                              &gPeiFirmwarePerformanceGuid,
                              BufferSize
                              );

    if (FirmwarePerformanceHob == NULL) {
      return EFI_BUFFER_TOO_SMALL;
    }

    FirmwarePerformanceHob->NumberOfEntries = 0;
    FirmwarePerformanceHob->Reserved = 0;
  }

  //
  // return if pre-allocated performance hob has filled up
  //
  if (FirmwarePerformanceHob->NumberOfEntries >= MAX_FIRMWARE_PERFORMANCE_ENTRIES) {
    return EFI_BUFFER_TOO_SMALL;
  }

  PeiGuidRec = &(FirmwarePerformanceHob->GuidEventRecord[FirmwarePerformanceHob->NumberOfEntries]);
  ((*PeiServices)->SetMem) (PeiGuidRec, sizeof (PEI_GUID_EVENT_REC), 0);

  //
  // If not NULL pointer, copy the file name
  //
  if (FileHeader != NULL) {
    PeiGuidRec->Guid = ((EFI_FFS_FILE_HEADER *)FileHeader)->Name;
  }
  //
  // Record the time stamp nanosec value.
  //
  if (TimeStamp == 0) {
    TimeStamp = AsmReadTsc();
  }
  PeiGuidRec->Timestamp = GetTimeInNanoSec (TimeStamp);

  //
  // Copy the progress ID
  //
  PeiGuidRec->ProgressID = Identifier;

  //
  // Record the APIC Id
  //
  PeiGuidRec->ApicID = GetApicId ();

  //
  // Increment the number of valid log entries.
  //
  FirmwarePerformanceHob->NumberOfEntries++;

  return EFI_SUCCESS;
}

EFI_STATUS
PeiPerfMeasure (
  IN VOID     *FileHeader,
  IN UINT16   *Token,
  IN BOOLEAN  EntryExit,
  IN UINT64   TimeStamp
  )
/*++

Routine Description:

  Log a timestamp value into pre-allocated buffer.
  Creates performance hob if not already installed

Arguments:

  FileHeader  - Pointer to the file header

  Token       - Pointer to Token Name

  EntryExit   - Indicates start or stop measurement

  TimeStamp   - The start time or the stop time

Returns:

  EFI_BUFFER_TOO_SMALL - Allocate buffer is not enough to hold new records

  EFI_UNSUPPORTED      - Unable to recognize used token
  
  EFI_SUCCESS          - Successfully updated the record in hob

--*/
{
  PEI_FIRMWARE_PERFORMANCE_HOB  *FirmwarePerformanceHob;
  PEI_GUID_EVENT_REC            *PeiGuidRec;
  EFI_PEI_SERVICES              **PeiServices;
  UINTN                         BufferSize;
  EFI_HOB_GUID_TYPE             *GuidHob;

  PeiServices = GetPeiServicesTablePointer ();
  //
  // Locate the Pei Performance Log Hob.
  //
  GuidHob = GetFirstGuidHob (&gPeiFirmwarePerformanceGuid);
  ASSERT (GuidHob != NULL);

  if (GuidHob != NULL) {
    //
    // PEI Performance HOB was found, then return the existing one.
    //
    FirmwarePerformanceHob = GET_GUID_HOB_DATA (GuidHob);
  } else {
    //
    // PEI Performance HOB was not found, then build one.
    //
    BufferSize = (UINT16) (sizeof (PEI_FIRMWARE_PERFORMANCE_HOB) + 
                            ((MAX_FIRMWARE_PERFORMANCE_ENTRIES-1) * sizeof (PEI_GUID_EVENT_REC))
                           );

    FirmwarePerformanceHob = BuildGuidHob (
                              &gPeiFirmwarePerformanceGuid,
                              BufferSize
                              );

    if (FirmwarePerformanceHob == NULL) {
      return EFI_BUFFER_TOO_SMALL;
    }

    FirmwarePerformanceHob->NumberOfEntries = 0;
    FirmwarePerformanceHob->Reserved = 0;
  }

  if (FirmwarePerformanceHob->NumberOfEntries >= MAX_FIRMWARE_PERFORMANCE_ENTRIES) {
    return EFI_BUFFER_TOO_SMALL;
  }

  PeiGuidRec = &(FirmwarePerformanceHob->GuidEventRecord[FirmwarePerformanceHob->NumberOfEntries]);
  ((*PeiServices)->SetMem) (PeiGuidRec, sizeof (PEI_GUID_EVENT_REC), 0);

  //
  // If not NULL pointer, copy the file name
  //
  if (FileHeader != NULL) {
    PeiGuidRec->Guid = ((EFI_FFS_FILE_HEADER *)FileHeader)->Name;
  }
  //
  // Record the time stamp nanosec value.
  //
  if (TimeStamp == 0) {
    TimeStamp = AsmReadTsc();
  }
  PeiGuidRec->Timestamp = GetTimeInNanoSec (TimeStamp);

  //
  // Record the Progress Id
  // Tokens are used by PEI core to log various phases of PEI
  //
  if (!StrCmp (Token, L"PEIM")) {
    if (!EntryExit) {
      PeiGuidRec->ProgressID = PEIM_START_ID;
    } else {
      PeiGuidRec->ProgressID = PEIM_END_ID;
    }
  } else if (!StrCmp (Token, L"PreMem")) {
    if (!EntryExit) {
      PeiGuidRec->ProgressID = PREMEM_START_ID;
    } else {
      PeiGuidRec->ProgressID = PREMEM_END_ID;
    }
  } else if (!StrCmp (Token, L"DisMem")) {
    if (!EntryExit) {
      PeiGuidRec->ProgressID = DISMEM_START_ID;
    } else {
      PeiGuidRec->ProgressID = DISMEM_END_ID;
    }
  } else if (!StrCmp (Token, L"PostMem")) {
    if (!EntryExit) {
      PeiGuidRec->ProgressID = POSTMEM_START_ID;
    } else {
      PeiGuidRec->ProgressID = POSTMEM_END_ID;
    }
  } else {
    return EFI_UNSUPPORTED;
  }
  //
  // Record the APIC Id
  //
  PeiGuidRec->ApicID = GetApicId ();

  //
  // Increment the number of valid log entries.
  //
  FirmwarePerformanceHob->NumberOfEntries++;

  return EFI_SUCCESS;
}

EFI_STATUS
StartMeasure (
  IN VOID   *Handle, 
  IN UINT16 *Token,  
  IN UINT16 *Module, 
  IN UINT64 TimeStamp
  )
/*++

Routine Description:

  Start measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle     - Handle to measure
  Token      - Token to measure
  Module     - Module to measure
  Timestamp  - Ticker as start tick

Returns:

  EFI_SUCCESS - Located hob successfully, and buffer is updated with new record
  EFI_UNSUPPORTED - Failure in update
  EFI_BUFFER_TOO_SMALL - Allocate buffer is not enough to hold new records

--*/
{
  EFI_STATUS Status;

  Status = PeiPerfMeasure (Handle, Token, FALSE, TimeStamp);
  return Status;
}

EFI_STATUS
EndMeasure (
  IN VOID   *Handle, 
  IN UINT16 *Token,  
  IN UINT16 *Module, 
  IN UINT64 TimeStamp
  )
/*++

Routine Description:

  End measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle     - Handle to stop
  Token      - Token to stop
  Module     - Module to stop
  Timestamp  - Ticker as end tick

Returns:

  EFI_SUCCESS - Located hob successfully, and buffer is updated with new record
  EFI_UNSUPPORTED - Failure in update
  EFI_BUFFER_TOO_SMALL - Allocate buffer is not enough to hold new records

--*/
{
  EFI_STATUS Status;

  Status = PeiPerfMeasure (Handle, Token, TRUE, TimeStamp);
  return Status;
}

EFI_STATUS
StartMeasureEx (
  IN VOID   *Handle, 
  IN UINT16 *Token,  
  IN UINT16 *Module, 
  IN UINT64 TimeStamp,
  IN UINT16 Identifier
  )
/*++

Routine Description:

  Start extended measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle     - Handle to stop
  Token      - Token to stop
  Module     - Module to stop
  Timestamp  - Ticker as end tick
  Identifier - Identifier for a given record

Returns:

  EFI_SUCCESS - Located hob successfully, and buffer is updated with new record
  EFI_UNSUPPORTED - Failure in update
  EFI_BUFFER_TOO_SMALL - Allocate buffer is not enough to hold new records

--*/
{
  EFI_STATUS Status;

  Status = PeiPerfMeasureEx (Handle, Token, FALSE, TimeStamp, Identifier);
  return Status;
}

EFI_STATUS
EndMeasureEx (
  IN VOID   *Handle, 
  IN UINT16 *Token,  
  IN UINT16 *Module, 
  IN UINT64 TimeStamp,
  IN UINT16 Identifier
  )
/*++

Routine Description:

  End extended measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle     - Handle to stop
  Token      - Token to stop
  Module     - Module to stop
  Timestamp  - Ticker as end tick
  Identifier - Identifier for a given record

Returns:

  EFI_SUCCESS - Located hob successfully, and buffer is updated with new record
  EFI_UNSUPPORTED - Failure in update
  EFI_BUFFER_TOO_SMALL - Allocate buffer is not enough to hold new records

--*/
{
  EFI_STATUS Status;

  Status = PeiPerfMeasureEx (Handle, Token, TRUE, TimeStamp, Identifier);
  return Status;
}