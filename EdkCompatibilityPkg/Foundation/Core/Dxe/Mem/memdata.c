/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  memdata.c

Abstract:

  Global data used in memory service


Revision History

--*/

#include "imem.h"


//
// MemoryLock - synchronizes access to the memory map and pool lists
//
EFI_LOCK          gMemoryLock = EFI_INITIALIZE_LOCK_VARIABLE (EFI_TPL_NOTIFY);

//
// MemoryMap - the current memory map
//
EFI_LIST_ENTRY    gMemoryMap  = INITIALIZE_LIST_HEAD_VARIABLE (gMemoryMap);

//
// MemoryLastConvert - the last memory descriptor used for a conversion request
//
MEMORY_MAP        *gMemoryLastConvert;
