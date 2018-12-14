/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Library.h

Abstract:

Revision History

--*/

#ifndef _DXE_LIBRARY_H_
#define _DXE_LIBRARY_H_

typedef struct {
  EFI_TPL     Tpl;
  EFI_TPL     OwnerTpl;
  UINTN       Lock;
} EFI_LOCK;


//
// Macro to initialize the state of a lock when a lock variable is declared
//
#define EFI_INITIALIZE_LOCK_VARIABLE(Tpl) {Tpl,0,0}

VOID
CoreReportProgressCode (
  IN  EFI_STATUS_CODE_VALUE   Value
  )
/*++

Routine Description:

  Report status code of type EFI_PROGRESS_CODE by caller ID gEfiDxeServicesTableGuid.
    
Arguments:

  Value    - Describes the class/subclass/operation of the hardware or software entity 
             that the Status Code relates to. 
   
Returns:

  None

--*/
;

VOID
CoreReportProgressCodeSpecific (
  IN  EFI_STATUS_CODE_VALUE   Value,
  IN  EFI_HANDLE              Handle
  )
/*++

Routine Description:

  Report status code of type EFI_PROGRESS_CODE by caller ID gEfiDxeServicesTableGuid, 
  with a handle as additional information.
    
Arguments:

  Value    - Describes the class/subclass/operation of the hardware or software entity 
             that the Status Code relates to. 
             
  Handle   - Additional information.
   
Returns:

  None

--*/
;

VOID
CoreAcquireLock (
  IN EFI_LOCK *Lock
  )
/*++

Routine Description:

  Raising to the task priority level of the mutual exclusion
  lock, and then acquires ownership of the lock.
    
Arguments:

  Lock - The lock to acquire
    
Returns:

  Lock owned

--*/
;

EFI_STATUS
CoreAcquireLockOrFail (
  IN EFI_LOCK  *Lock
  )
/*++

Routine Description:

  Initialize a basic mutual exclusion lock.   Each lock
  provides mutual exclusion access at it's task priority
  level.  Since there is no-premption (at any TPL) or
  multiprocessor support, acquiring the lock only consists
  of raising to the locks TPL.
    
Arguments:

  Lock        - The EFI_LOCK structure to initialize
   
Returns:

  EFI_SUCCESS       - Lock Owned.
  EFI_ACCESS_DENIED - Reentrant Lock Acquisition, Lock not Owned.

--*/
;

VOID
CoreReleaseLock (
  IN EFI_LOCK *Lock
  )
/*++

Routine Description:

    Releases ownership of the mutual exclusion lock, and
    restores the previous task priority level.
    
Arguments:

    Lock - The lock to release
    
Returns:

    Lock unowned

--*/
;

//
// Device Path functions
//

UINTN
CoreDevicePathSize (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
/*++

Routine Description:

  Calculate the size of a whole device path.    
    
Arguments:

  DevicePath - The pointer to the device path data.
    
Returns:

  Size of device path data structure..

--*/
;

BOOLEAN
CoreIsDevicePathMultiInstance (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
/*++

Routine Description:
  Return TRUE is this is a multi instance device path.

Arguments:
  DevicePath  - A pointer to a device path data structure.


Returns:
  TRUE - If DevicePath is multi instance. FALSE - If DevicePath is not multi
  instance.

--*/
;


EFI_DEVICE_PATH_PROTOCOL *
CoreDuplicateDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
/*++

Routine Description:
  Duplicate a new device path data structure from the old one.

Arguments:
  DevicePath  - A pointer to a device path data structure.

Returns:
  A pointer to the new allocated device path data.
  Caller must free the memory used by DevicePath if it is no longer needed.

--*/
;

EFI_DEVICE_PATH_PROTOCOL *
CoreAppendDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *Src1,
  IN EFI_DEVICE_PATH_PROTOCOL  *Node
  )
/*++

Routine Description:
  Function is used to append a Src1 and Src2 together.

Arguments:
  Src1  - A pointer to a device path data structure.

  Node  - A pointer to a device path data structure.

Returns:

  A pointer to the new device path is returned.
  NULL is returned if space for the new device path could not be allocated from pool.
  It is up to the caller to free the memory used by Src1 and Src2 if they are no longer needed.

--*/
;

VOID *
CoreAllocateBootServicesPool (
  IN  UINTN   AllocationSize
  )
/*++

Routine Description:

  Allocate pool of type EfiBootServicesData, the size is specified with AllocationSize.
    
Arguments:

  AllocationSize    - Size to allocate.
   
Returns:

  Pointer of the allocated pool.

--*/
;

VOID *
CoreAllocateZeroBootServicesPool (
  IN  UINTN   AllocationSize
  )
/*++

Routine Description:

  Allocate pool of type EfiBootServicesData and zero it, the size is specified with AllocationSize.
    
Arguments:

  AllocationSize    - Size to allocate.
   
Returns:

  Pointer of the allocated pool.

--*/
;

EFI_STATUS
CoreGetConfigTable (
  IN EFI_GUID *Guid,
  IN OUT VOID **Table
  )
/*++

Routine Description:

  Find a config table by name in system table's ConfigurationTable.

Arguments:

  Guid        - The table name to look for
  
  Table       - Pointer of the config table

Returns: 

  EFI_NOT_FOUND       - Could not find the table in system table's ConfigurationTable.
  
  EFI_SUCCESS         - Table successfully found.

--*/
;

VOID *
CoreAllocateRuntimeCopyPool (
  IN  UINTN   AllocationSize,
  IN  VOID    *Buffer
  )
/*++

Routine Description:

  Allocate pool of specified size with EfiRuntimeServicesData type, and copy specified buffer to this pool.
    
Arguments:

  AllocationSize    - Size to allocate.
  
  Buffer            - Specified buffer that will be copy to the allocated pool
   
Returns:

  Pointer of the allocated pool.

--*/
;

VOID *
CoreAllocateRuntimePool (
  IN  UINTN   AllocationSize
  )
/*++

Routine Description:

  Allocate pool of type EfiRuntimeServicesData, the size is specified with AllocationSize.
    
Arguments:

  AllocationSize    - Size to allocate.
   
Returns:

  Pointer of the allocated pool.

--*/
;

VOID *
CoreAllocateCopyPool (
  IN  UINTN   AllocationSize,
  IN  VOID    *Buffer
  )
/*++

Routine Description:

  Allocate pool of specified size with EfiBootServicesData type, and copy specified buffer to this pool.
    
Arguments:

  AllocationSize    - Size to allocate.
  
  Buffer            - Specified buffer that will be copy to the allocated pool
   
Returns:

  Pointer of the allocated pool.

--*/
;

VOID
EfiDebugAssert (
  IN CHAR8    *FileName,
  IN INTN     LineNumber,
  IN CHAR8    *Description
  )
/*++

Routine Description:

  Worker function for ASSERT(). If Error Logging hub is loaded, log DEBUG
  information; If not, do BREAKPOINT().

Arguments:

  FileName    - File name of failing routine.

  LineNumber  - Line number of failing ASSERT().

  Description - Descritption, usally the assertion string.
  
Returns:
  
  None

--*/
;

VOID
EfiDebugPrint (
  IN  UINTN ErrorLevel,
  IN  CHAR8 *Format,
  ...
  )
/*++

Routine Description:

  Wrapper for EfiDebugVPrint ()
  
Arguments:

  ErrorLevel - If error level is set do the debug print.

  Format     - String to use for the print, followed by Print arguments.

  ...        - Print arguments.
  
Returns:
  
  None

--*/
;

EFI_EVENT
CoreCreateProtocolNotifyEvent (
  IN EFI_GUID             *ProtocolGuid,
  IN EFI_TPL              NotifyTpl,
  IN EFI_EVENT_NOTIFY     NotifyFunction,
  IN VOID                 *NotifyContext,
  OUT VOID                **Registration,
  IN  BOOLEAN             SignalFlag
  )
/*++

Routine Description:

  Create a protocol notification event and return it.

Arguments:

  ProtocolGuid    - Protocol to register notification event on.

  NotifyTpl       - Maximum TPL to signal the NotifyFunction.

  NotifyFuncition - EFI notification routine.

  NotifyContext   - Context passed into Event when it is created.

  Registration    - Registration key returned from RegisterProtocolNotify().

  SignalFlag      -  Boolean value to decide whether kick the event after register or not. 

Returns:

  The EFI_EVENT that has been registered to be signaled when a ProtocolGuid
  is added to the system.

--*/
;

VOID
EFIAPI
CoreInitializeFwVolDevicepathNode (
  IN  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH     *FvDevicePathNode,
  IN EFI_GUID                               *NameGuid
  )
/*++

Routine Description:

  Initialize a Firmware Volume (FV) Media Device Path node.
  
  Tiano extended the EFI 1.10 device path nodes. Tiano does not own this enum
  so as we move to UEFI 2.0 support we must use a mechanism that conforms with
  the UEFI 2.0 specification to define the FV device path. An UEFI GUIDed 
  device path is defined for PIWG extensions of device path. If the code 
  is compiled to conform with the UEFI 2.0 specification use the new device path
  else use the old form for backwards compatability.

Arguments:

  FvDevicePathNode   - Pointer to a FV device path node to initialize
  NameGuid           - FV file name to use in FvDevicePathNode

--*/
;

EFI_GUID *
EFIAPI
CoreGetNameGuidFromFwVolDevicePathNode (
  IN  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH   *FvDevicePathNode
  )
/*++

Routine Description:

  Check to see if the Firmware Volume (FV) Media Device Path is valid.
  
  Tiano extended the EFI 1.10 device path nodes. Tiano does not own this enum
  so as we move to UEFI 2.0 support we must use a mechanism that conforms with
  the UEFI 2.0 specification to define the FV device path. An UEFI GUIDed 
  device path is defined for PIWG extensions of device path. If the code 
  is compiled to conform with the UEFI 2.0 specification use the new device path
  else use the old form for backwards compatability. The return value to this
  function points to a location in FvDevicePathNode and it does not allocate
  new memory for the GUID pointer that is returned.

Arguments:

  FvDevicePathNode   Pointer to FV device path to check

Returns:

  NULL    - FvDevicePathNode is not valid.
  Other   - FvDevicePathNode is valid and pointer to NameGuid was returned.

--*/
;
#endif
