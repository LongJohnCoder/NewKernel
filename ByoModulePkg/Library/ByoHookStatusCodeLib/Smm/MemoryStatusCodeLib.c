/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  MemoryStatusCodeLib.c

Abstract: 
  Memory Status Code report library.

Revision History:

Bug 2517:   Create the Module StatusCodeHandler to report status code to 
            all supported devide in ByoModule
TIME:       2011-7-22
$AUTHOR:    Liu Chunling
$REVIEWERS:  
$SCOPE:     All Platforms
$TECHNICAL:  
  1. Create the module StatusCodeHandler to support Serial Port, Memory, Port80,
     Beep and OEM devices to report status code.
  2. Create the Port80 map table and the Beep map table to convert status code 
     to code byte and beep times.
  3. Create new libraries to support status code when StatusCodePpi,
     StatusCodeRuntimeProtocol, SmmStatusCodeProtocol has not been installed yet.
$END--------------------------------------------------------------------

**/
/** @file
  Runtime memory status code worker.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            
                                                                                            
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "ByoHookStatusCodeSmmLib.h"

RUNTIME_MEMORY_STATUSCODE_HEADER  *mSmmMemoryStatusCodeTable;

/**
  Initialize SMM memory status code table as initialization for memory status code worker
 
  @retval EFI_SUCCESS  SMM memory status code table successfully initialized.

**/
EFI_STATUS
MemoryStatusCodeInitializeWorker (
  VOID
  )
{
  //
  // Allocate SMM memory status code pool.
  //
  mSmmMemoryStatusCodeTable = (RUNTIME_MEMORY_STATUSCODE_HEADER *)AllocateZeroPool (sizeof (RUNTIME_MEMORY_STATUSCODE_HEADER) + PcdGet16 (PcdStatusCodeMemorySize) * 1024);
  ASSERT (mSmmMemoryStatusCodeTable != NULL);

  mSmmMemoryStatusCodeTable->MaxRecordsNumber = (PcdGet16 (PcdStatusCodeMemorySize) * 1024) / sizeof (MEMORY_STATUSCODE_RECORD);
  return EFI_SUCCESS;
}


/**
  Report status code into runtime memory. If the runtime pool is full, roll back to the 
  first record and overwrite it.
 
  @param  CodeType                Indicates the type of status code being reported.
  @param  Value                   Describes the current status of a hardware or software entity.
                                  This included information about the class and subclass that is used to
                                  classify the entity as well as an operation.
  @param  Instance                The enumeration of a hardware or software entity within
                                  the system. Valid instance numbers start with 1.
  @param  CallerId                This optional parameter may be used to identify the caller.
                                  This parameter allows the status code driver to apply different rules to
                                  different callers.
  @param  Data                    This optional parameter may be used to pass additional data.
 
  @retval EFI_SUCCESS             Status code successfully recorded in runtime memory status code table.

**/
EFI_STATUS
EFIAPI
MemoryStatusCodeReportWorker (
  IN EFI_STATUS_CODE_TYPE               CodeType,
  IN EFI_STATUS_CODE_VALUE              Value,
  IN UINT32                             Instance,
  IN EFI_GUID                           *CallerId,
  IN EFI_STATUS_CODE_DATA               *Data OPTIONAL
  )
{
  MEMORY_STATUSCODE_RECORD              *Record;

  //
  // Locate current record buffer.
  //
  Record = (MEMORY_STATUSCODE_RECORD *) (mSmmMemoryStatusCodeTable + 1);
  Record = &Record[mSmmMemoryStatusCodeTable->RecordIndex++];

  //
  // Save status code.
  //
  Record->CodeType = CodeType;
  Record->Value    = Value;
  Record->Instance = Instance;

  //
  // If record index equals to max record number, then wrap around record index to zero.
  //
  // The reader of status code should compare the number of records with max records number,
  // If it is equal to or larger than the max number, then the wrap-around had happened,
  // so the first record is pointed by record index.
  // If it is less then max number, index of the first record is zero.
  //
  mSmmMemoryStatusCodeTable->NumberOfRecords++;
  if (mSmmMemoryStatusCodeTable->RecordIndex == mSmmMemoryStatusCodeTable->MaxRecordsNumber) {
    //
    // Wrap around record index.
    //
    mSmmMemoryStatusCodeTable->RecordIndex = 0;
  }

  return EFI_SUCCESS;
}



