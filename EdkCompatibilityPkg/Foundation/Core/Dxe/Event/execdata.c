/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  execdata.c

Abstract:




Revision History

--*/

#include "exec.h"


//
// gTpl - Task priority level
//
EFI_TPL  gEfiCurrentTpl = EFI_TPL_DRIVER;


//
// gEventQueueLock - Protects the event queus
//
EFI_LOCK gEventQueueLock = EFI_INITIALIZE_LOCK_VARIABLE (EFI_TPL_HIGH_LEVEL);

//
// gEventQueue - A list of event's to notify for each priority level
// gEventPending - A bitmask of the EventQueues that are pending
//
EFI_LIST_ENTRY  gEventQueue[EFI_TPL_HIGH_LEVEL + 1];
UINTN           gEventPending = 0;

//
// gEventSignalQueue - A list of events to signal when the EFI operation occurs
//
EFI_LIST_ENTRY    gEventSignalQueue = INITIALIZE_LIST_HEAD_VARIABLE (gEventSignalQueue);

