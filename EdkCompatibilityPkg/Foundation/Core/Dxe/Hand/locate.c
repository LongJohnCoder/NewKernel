/*++

Copyright (c) 2004 - 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  locate.c

Abstract:

  Locate handle functions    

Revision History

--*/

#include "hand.h"

//
// ProtocolRequest - Last LocateHandle request ID
//
UINTN mEfiLocateHandleRequest = 0;

//
// Internal prototypes
//

typedef struct {
  EFI_GUID        *Protocol;
  VOID            *SearchKey;
  EFI_LIST_ENTRY  *Position;
  PROTOCOL_ENTRY  *ProtEntry;
} LOCATE_POSITION;

typedef 
IHANDLE *
(* CORE_GET_NEXT) (
  IN OUT LOCATE_POSITION    *Position,
  OUT VOID                  **Interface
  );

STATIC
IHANDLE *
CoreGetNextLocateAllHandles (
  IN OUT LOCATE_POSITION    *Position,
  OUT VOID                  **Interface
  );

STATIC
IHANDLE *
CoreGetNextLocateByRegisterNotify (
  IN OUT LOCATE_POSITION    *Position,
  OUT VOID                  **Interface
  );

STATIC
IHANDLE *
CoreGetNextLocateByProtocol (
  IN OUT LOCATE_POSITION    *Position,
  OUT VOID                  **Interface
  );

//
//
//



EFI_BOOTSERVICE 
EFI_STATUS
EFIAPI
CoreLocateHandle (
  IN EFI_LOCATE_SEARCH_TYPE   SearchType,
  IN EFI_GUID                 *Protocol   OPTIONAL,
  IN VOID                     *SearchKey  OPTIONAL,
  IN OUT UINTN                *BufferSize,
  OUT EFI_HANDLE              *Buffer
  )
/*++

Routine Description:

  Locates the requested handle(s) and returns them in Buffer.

Arguments:

  SearchType  - The type of search to perform to locate the handles

  Protocol    - The protocol to search for
  
  SearchKey   - Dependant on SearchType

  BufferSize  - On input the size of Buffer.  On output the 
                size of data returned.  

  Buffer      - The buffer to return the results in


Returns:

  EFI_BUFFER_TOO_SMALL      - Buffer too small, required buffer size is returned in BufferSize.

  EFI_INVALID_PARAMETER     - Invalid parameter
  
  EFI_SUCCESS               - Successfully found the requested handle(s) and returns them in Buffer.
  
--*/
{
  EFI_STATUS          Status;
  LOCATE_POSITION     Position;
  PROTOCOL_NOTIFY     *ProtNotify;
  CORE_GET_NEXT       GetNext;
  UINTN               ResultSize;
  IHANDLE             *Handle;
  IHANDLE             **ResultBuffer;
  VOID                *Interface;
 
  if (BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  if ((*BufferSize > 0) && (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  GetNext = NULL;
  //
  // Set initial position
  //

  Position.Protocol  = Protocol;
  Position.SearchKey = SearchKey;
  Position.Position  = &gHandleList;

  ResultSize = 0;
  ResultBuffer = (IHANDLE **) Buffer;
  Status = EFI_SUCCESS;

  //
  // Lock the protocol database
  //
  
  CoreAcquireProtocolLock ();

  //
  // Get the search function based on type
  //
  switch (SearchType) {
  case AllHandles:      
    GetNext = CoreGetNextLocateAllHandles;       
    break;

  case ByRegisterNotify:    
    //
    // Must have SearchKey for locate ByRegisterNotify
    //
    if (SearchKey == NULL) {
      Status = EFI_INVALID_PARAMETER;
      break;
    }
    GetNext = CoreGetNextLocateByRegisterNotify;   
    break;

  case ByProtocol:      
    GetNext = CoreGetNextLocateByProtocol;
    if (Protocol == NULL) {
      Status = EFI_INVALID_PARAMETER;
      break;
    }
    //
    // Look up the protocol entry and set the head pointer
    //
    Position.ProtEntry = CoreFindProtocolEntry (Protocol, FALSE);
    if (Position.ProtEntry == NULL) {
      Status = EFI_NOT_FOUND;
      break;
    }
    Position.Position = &Position.ProtEntry->Protocols;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }

  if (EFI_ERROR(Status) || (GetNext == NULL)) {
    CoreReleaseProtocolLock ();
    return Status;
  }

  //
  // Enumerate out the matching handles
  //
  mEfiLocateHandleRequest += 1;
  for (; ;) {
    //
    // Get the next handle.  If no more handles, stop
    //
    Handle = GetNext (&Position, &Interface);
    if (NULL == Handle) {
      break;
    }

    //
    // Increase the resulting buffer size, and if this handle
    // fits return it
    //
    ResultSize += sizeof(Handle);
    if (ResultSize <= *BufferSize) {
        *ResultBuffer = Handle;
        ResultBuffer += 1;
    }
  }

  //
  // If the result is a zero length buffer, then there were no
  // matching handles
  //
  if (ResultSize == 0) {
    Status = EFI_NOT_FOUND;
  } else {
    //
    // Return the resulting buffer size.  If it's larger than what
    // was passed, then set the error code
    //
    if (ResultSize > *BufferSize) {
      Status = EFI_BUFFER_TOO_SMALL;
    } 
    
    *BufferSize = ResultSize;

    if (SearchType == ByRegisterNotify && !EFI_ERROR(Status)) {
      //
      // If this is a search by register notify and a handle was
      // returned, update the register notification position
      // 
      ProtNotify = SearchKey;
      ProtNotify->Position = ProtNotify->Position->ForwardLink;
    }
  }

  CoreReleaseProtocolLock ();
  return Status;
}


STATIC
IHANDLE *
CoreGetNextLocateAllHandles (
  IN OUT LOCATE_POSITION    *Position,
  OUT VOID                  **Interface
  )
/*++

Routine Description:

  Routine to get the next Handle, when you are searching for all handles.

Arguments:

  Position  - Information about which Handle to seach for.

  Interface - Return the interface structure for the matching protocol.
  
Returns:
  IHANDLE - An IHANDLE is returned if the next Position is not the end of the
            list. A NULL_HANDLE is returned if it's the end of the list.
  
--*/
{
  IHANDLE     *Handle;

  //
  // Next handle
  //
  Position->Position = Position->Position->ForwardLink;

  //
  // If not at the end of the list, get the handle
  //
  Handle      = NULL_HANDLE;
  *Interface  = NULL;
  if (Position->Position != &gHandleList) {
    Handle = CR (Position->Position, IHANDLE, AllHandles, EFI_HANDLE_SIGNATURE);
  }

  return Handle;
}


STATIC
IHANDLE *
CoreGetNextLocateByRegisterNotify (
  IN OUT LOCATE_POSITION    *Position,
  OUT VOID                  **Interface
  )
/*++

Routine Description:

  Routine to get the next Handle, when you are searching for register protocol 
  notifies.

Arguments:

  Position  - Information about which Handle to seach for.

  Interface - Return the interface structure for the matching protocol.
  
Returns:
  IHANDLE - An IHANDLE is returned if the next Position is not the end of the
            list. A NULL_HANDLE is returned if it's the end of the list.
  
--*/
{
  IHANDLE             *Handle;
  PROTOCOL_NOTIFY     *ProtNotify;
  PROTOCOL_INTERFACE  *Prot;
  EFI_LIST_ENTRY      *Link;    

  Handle      = NULL_HANDLE;
  *Interface  = NULL;
  ProtNotify = Position->SearchKey;

  //
  // If this is the first request, get the next handle
  //
  if (ProtNotify != NULL) {
    ASSERT(ProtNotify->Signature == PROTOCOL_NOTIFY_SIGNATURE);
    Position->SearchKey = NULL;

    //
    // If not at the end of the list, get the next handle
    //
    Link = ProtNotify->Position->ForwardLink;
    if (Link != &ProtNotify->Protocol->Protocols) {
      Prot = CR (Link, PROTOCOL_INTERFACE, ByProtocol, PROTOCOL_INTERFACE_SIGNATURE);
      Handle = (IHANDLE *) Prot->Handle;
      *Interface = Prot->Interface;
    }  
  }

  return Handle;
}


STATIC
IHANDLE *
CoreGetNextLocateByProtocol (
  IN OUT LOCATE_POSITION    *Position,
  OUT VOID                  **Interface
  )
/*++

Routine Description:

  Routine to get the next Handle, when you are searching for a given protocol.

Arguments:

  Position  - Information about which Handle to seach for.

  Interface - Return the interface structure for the matching protocol.
  
Returns:
  IHANDLE - An IHANDLE is returned if the next Position is not the end of the
            list. A NULL_HANDLE is returned if it's the end of the list.
  
--*/
{
  IHANDLE             *Handle;
  EFI_LIST_ENTRY      *Link;
  PROTOCOL_INTERFACE  *Prot;
 
  Handle      = NULL_HANDLE;
  *Interface  = NULL;
  for (; ;) {
    //
    // Next entry
    //
    Link = Position->Position->ForwardLink;
    Position->Position = Link;

    //
    // If not at the end, return the handle
    //
    if (Link == &Position->ProtEntry->Protocols) {
      Handle = NULL_HANDLE;
      break;
    }

    //
    // Get the handle
    //
    Prot = CR(Link, PROTOCOL_INTERFACE, ByProtocol, PROTOCOL_INTERFACE_SIGNATURE);
    Handle = (IHANDLE *) Prot->Handle;
    *Interface = Prot->Interface;

    //
    // If this handle has not been returned this request, then 
    // return it now
    //
    if (Handle->LocateRequest != mEfiLocateHandleRequest) {
      Handle->LocateRequest = mEfiLocateHandleRequest;
      break;
    }
  }

  return Handle;
}


EFI_BOOTSERVICE
EFI_STATUS
EFIAPI
CoreLocateDevicePath (
  IN EFI_GUID                       *Protocol,
  IN OUT EFI_DEVICE_PATH_PROTOCOL   **DevicePath,
  OUT EFI_HANDLE                    *Device
  )
/*++

Routine Description:

  Locates the handle to a device on the device path that best matches the specified protocol.

Arguments:

  Protocol    - The protocol to search for.
  DevicePath  - On input, a pointer to a pointer to the device path. On output, the device
                  path pointer is modified to point to the remaining part of the devicepath.
  Device      - A pointer to the returned device handle.              

Returns:

  EFI_SUCCESS           - The resulting handle was returned.
  EFI_NOT_FOUND         - No handles matched the search.
  EFI_INVALID_PARAMETER - One of the parameters has an invalid value.

--*/
{
  INTN                        SourceSize;
  INTN                        Size;
  INTN                        BestMatch;
  UINTN                       HandleCount;
  UINTN                       Index;
  EFI_STATUS                  Status;
  EFI_HANDLE                  *Handles;
  EFI_HANDLE                  Handle;
  EFI_HANDLE                  BestDevice;
  EFI_DEVICE_PATH_PROTOCOL    *SourcePath;
  EFI_DEVICE_PATH_PROTOCOL    *TmpDevicePath;
  
  if (Protocol == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  if ((DevicePath == NULL) || (*DevicePath == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  BestDevice = NULL;
  SourcePath = *DevicePath;
  TmpDevicePath = SourcePath;
  while (!IsDevicePathEnd (TmpDevicePath)) {
    if (EfiIsDevicePathEndInstance (TmpDevicePath)) {
      //
      // If DevicePath is a multi-instance device path,
      // the function will operate on the first instance 
      //
      break;
    }
    TmpDevicePath = NextDevicePathNode (TmpDevicePath);
  }

  SourceSize = (UINTN) TmpDevicePath - (UINTN) SourcePath;

  //
  // Get a list of all handles that support the requested protocol
  //
  Status = CoreLocateHandleBuffer (ByProtocol, Protocol, NULL, &HandleCount, &Handles);
  if (EFI_ERROR (Status) || HandleCount == 0) {
    return EFI_NOT_FOUND;
  }

  BestMatch = -1;
  for(Index = 0; Index < HandleCount; Index += 1) {
    Handle = Handles[Index];
    Status = CoreHandleProtocol (Handle, &gEfiDevicePathProtocolGuid, &TmpDevicePath);
    if (EFI_ERROR (Status)) {
      //
      // If this handle doesn't support device path, then skip it
      //
      continue;
    }

    //
    // Check if DevicePath is first part of SourcePath
    //
    Size = CoreDevicePathSize (TmpDevicePath) - sizeof(EFI_DEVICE_PATH_PROTOCOL);
    if ((Size <= SourceSize) && EfiCompareMem (SourcePath, TmpDevicePath, Size) == 0) {
      //
      // If the size is equal to the best match, then we
      // have a duplice device path for 2 different device
      // handles
      //
      ASSERT (Size != BestMatch);
      
      //
      // We've got a match, see if it's the best match so far
      //
      if (Size > BestMatch) {
        BestMatch = Size;
        BestDevice = Handle;
      }
    }
  }

  CoreFreePool (Handles);
   
  //
  // If there wasn't any match, then no parts of the device path was found.  
  // Which is strange since there is likely a "root level" device path in the system.
  //
  if (BestMatch == -1) {
    return EFI_NOT_FOUND;
  }

  if (Device == NULL) {
    return  EFI_INVALID_PARAMETER;
  }
  *Device = BestDevice;
  
  //
  // Return the remaining part of the device path
  //
  *DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) (((UINT8 *) SourcePath) + BestMatch);
  return EFI_SUCCESS;
}


EFI_BOOTSERVICE11 
EFI_STATUS
EFIAPI
CoreLocateProtocol (
  IN  EFI_GUID  *Protocol,
  IN  VOID      *Registration OPTIONAL,
  OUT VOID      **Interface
  )
/*++

Routine Description:

  Return the first Protocol Interface that matches the Protocol GUID. If
  Registration is pasased in return a Protocol Instance that was just add
  to the system. If Retistration is NULL return the first Protocol Interface
  you find.

Arguments:

  Protocol     - The protocol to search for
  
  Registration - Optional Registration Key returned from RegisterProtocolNotify() 

  Interface    - Return the Protocol interface (instance).

Returns:

  EFI_SUCCESS - If a valid Interface is returned
  
  EFI_INVALID_PARAMETER       - Invalid parameter
  
  EFI_NOT_FOUND               - Protocol interface not found

--*/
{
  EFI_STATUS              Status;
  LOCATE_POSITION         Position;
  PROTOCOL_NOTIFY         *ProtNotify;
  IHANDLE                 *Handle;

  if (Interface == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (Protocol == NULL) {
    return EFI_NOT_FOUND;
  }
  
  *Interface = NULL;
  Status = EFI_SUCCESS;

  //
  // Set initial position
  //
  Position.Protocol  = Protocol;
  Position.SearchKey = Registration;
  Position.Position  = &gHandleList;
  
  //
  // Lock the protocol database
  //
  CoreAcquireProtocolLock ();

  mEfiLocateHandleRequest += 1;

  if (NULL == Registration) {
    //
    // Look up the protocol entry and set the head pointer
    //
    Position.ProtEntry = CoreFindProtocolEntry (Protocol, FALSE);
    if (Position.ProtEntry == NULL) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }
    Position.Position = &Position.ProtEntry->Protocols;

    Handle = CoreGetNextLocateByProtocol (&Position, Interface);
  } else {
    Handle = CoreGetNextLocateByRegisterNotify (&Position, Interface);   
  }

  if (NULL == Handle) {
    Status = EFI_NOT_FOUND;
  } else if (NULL != Registration) {
    //
    // If this is a search by register notify and a handle was
    // returned, update the register notification position
    // 
    ProtNotify = Registration;
    ProtNotify->Position = ProtNotify->Position->ForwardLink;
  }

Done:
  CoreReleaseProtocolLock ();
  return Status;
}


EFI_BOOTSERVICE11 
EFI_STATUS
EFIAPI
CoreLocateHandleBuffer (
  IN EFI_LOCATE_SEARCH_TYPE       SearchType,
  IN EFI_GUID                     *Protocol OPTIONAL,
  IN VOID                         *SearchKey OPTIONAL,
  IN OUT UINTN                    *NumberHandles,
  OUT EFI_HANDLE                  **Buffer
  )
/*++

Routine Description:

  Function returns an array of handles that support the requested protocol 
  in a buffer allocated from pool. This is a version of CoreLocateHandle()
  that allocates a buffer for the caller.

Arguments:

  SearchType           - Specifies which handle(s) are to be returned.
  Protocol             - Provides the protocol to search by.   
                         This parameter is only valid for SearchType ByProtocol.
  SearchKey            - Supplies the search key depending on the SearchType.
  NumberHandles      - The number of handles returned in Buffer.
  Buffer               - A pointer to the buffer to return the requested array of 
                         handles that support Protocol.

Returns:
  
  EFI_SUCCESS          - The result array of handles was returned.
  EFI_NOT_FOUND        - No handles match the search. 
  EFI_OUT_OF_RESOURCES - There is not enough pool memory to store the matching results.
  EFI_INVALID_PARAMETER   - Invalid parameter

--*/
{
  EFI_STATUS          Status;
  UINTN               BufferSize;

  if (NumberHandles == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BufferSize = 0;
  *NumberHandles = 0;
  *Buffer = NULL;
  Status = CoreLocateHandle (
             SearchType,
             Protocol,
             SearchKey,
             &BufferSize,
             *Buffer
             );
  //
  // LocateHandleBuffer() returns incorrect status code if SearchType is
  // invalid.
  //
  // Add code to correctly handle expected errors from CoreLocateHandle().
  //
  if (EFI_ERROR(Status)) {
    switch (Status) {
    case EFI_BUFFER_TOO_SMALL:
      break;
    case EFI_INVALID_PARAMETER:
      return Status;
    default:
      return EFI_NOT_FOUND;
    }
  }

  *Buffer = CoreAllocateBootServicesPool (BufferSize);
  if (*Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = CoreLocateHandle (
             SearchType,
             Protocol,
             SearchKey,
             &BufferSize,
             *Buffer
             );

  *NumberHandles = BufferSize/sizeof(EFI_HANDLE);
  if (EFI_ERROR(Status)) {
    *NumberHandles = 0;
  }

  return Status;
}


