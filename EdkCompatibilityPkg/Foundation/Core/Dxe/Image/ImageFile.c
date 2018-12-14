/*++

Copyright (c) 2004 - 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  ImageFile.c


Abstract:

  


Revision History

--*/

#include "Image.h"
#include "DxeCore.h"


VOID
CoreDevicePathToFileName (
  IN  FILEPATH_DEVICE_PATH      *FilePath,
  OUT CHAR16                    **String
  );



EFI_STATUS
CoreOpenImageFile (
  IN BOOLEAN                        BootPolicy,
  IN VOID                           *SourceBuffer   OPTIONAL,
  IN UINTN                          SourceSize,
  IN OUT EFI_DEVICE_PATH_PROTOCOL   **FilePath,
  OUT EFI_HANDLE                    *DeviceHandle,
  IN IMAGE_FILE_HANDLE              *ImageFileHandle,
  OUT UINT32                        *AuthenticationStatus
  )
/*++

Routine Description:

    Opens a file for (simple) reading.  The simple read abstraction
    will access the file either from a memory copy, from a file
    system interface, or from the load file interface.

Arguments:

  BootPolicy    - Policy for Open Image File.
  SourceBuffer  - Pointer to the memory location containing copy
                  of the image to be loaded.
  SourceSize    - The size in bytes of SourceBuffer.
  FilePath      - The specific file path from which the image is loaded
  DeviceHandle  - Pointer to the return device handle.
  ImageFileHandle      - Pointer to the image file handle.
  AuthenticationStatus - Pointer to a caller-allocated UINT32 in which the authentication status is returned. 
    
Returns:

    EFI_SUCCESS     - Image file successfully opened.
    
    EFI_LOAD_ERROR  - If the caller passed a copy of the file, and SourceSize is 0.
    
    EFI_INVALID_PARAMETER   - File path is not valid.
    
    EFI_NOT_FOUND   - File not found.

--*/
{
  EFI_STATUS                        Status;
  EFI_DEVICE_PATH_PROTOCOL          *TempFilePath;
  FILEPATH_DEVICE_PATH              *FilePathNode;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *FwVolFilePathNode;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *Volume;
  EFI_FILE_HANDLE                   FileHandle;
  EFI_FILE_HANDLE                   LastHandle;
  EFI_LOAD_FILE_PROTOCOL            *LoadFile;
#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  EFI_LOAD_FILE2_PROTOCOL           *LoadFile2;
#endif
  EFI_SECTION_TYPE                  SectionType;
  UINT8                             *Pe32Buffer;
  UINTN                             Pe32BufferSize;
  EFI_FV_FILETYPE                   Type;
  EFI_FV_FILE_ATTRIBUTES            Attrib;
  EFI_FILE_INFO                     *FileInfo;
  UINTN                             FileInfoSize;
  EFI_GUID                          *NameGuid;
  FILEPATH_DEVICE_PATH              *OriginalFilePathNode;
#if (PI_SPECIFICATION_VERSION < 0x00010000)
  EFI_FIRMWARE_VOLUME_PROTOCOL      *FwVol;
#else
  EFI_FIRMWARE_VOLUME2_PROTOCOL     *FwVol;
#endif

  *AuthenticationStatus = 0;
  EfiCommonLibZeroMem (ImageFileHandle, sizeof (IMAGE_FILE_HANDLE));
  ImageFileHandle->Signature = IMAGE_FILE_HANDLE_SIGNATURE;

  //
  // If the caller passed a copy of the file, then just use it
  //
  if (SourceBuffer != NULL) {
    ImageFileHandle->Source     = SourceBuffer;
    ImageFileHandle->SourceSize = SourceSize;
    *DeviceHandle     = NULL;
    CoreLocateDevicePath (&gEfiDevicePathProtocolGuid, FilePath, DeviceHandle);
    if (SourceSize > 0) {
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_LOAD_ERROR;
    }
    goto Done;
  }

  //
  // Make sure FilePath is valid
  //
  if (*FilePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check to see if it's in a Firmware Volume
  //
  FwVolFilePathNode = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) *FilePath;
  Status = CoreDevicePathToInterface (
         #if (PI_SPECIFICATION_VERSION < 0x00010000)
            &gEfiFirmwareVolumeProtocolGuid, 
         #else
            &gEfiFirmwareVolume2ProtocolGuid, 
         #endif
            (EFI_DEVICE_PATH_PROTOCOL **)&FwVolFilePathNode, 
            (VOID*)&FwVol, 
            DeviceHandle
            );
  if (!EFI_ERROR (Status) && (FwVol != NULL)) {
    //
    // For FwVol File system there is only a single file name that is a GUID.
    //
    NameGuid = CoreGetNameGuidFromFwVolDevicePathNode (FwVolFilePathNode);
    if (NameGuid != NULL) {

      SectionType = EFI_SECTION_PE32;
      Pe32Buffer  = NULL;
      Status = FwVol->ReadSection (
                        FwVol, 
                        &FwVolFilePathNode->NameGuid,  
                        SectionType,   
                        0,
                        &Pe32Buffer,
                        &Pe32BufferSize,
                        AuthenticationStatus
                        );
      if (EFI_ERROR (Status)) {
        //
        // Try a raw file, since a PE32 SECTION does not exist
        //
        if (Pe32Buffer != NULL) {
          CoreFreePool (Pe32Buffer);
          *AuthenticationStatus = 0;
        }
        Pe32Buffer = NULL;
        Status = FwVol->ReadFile (
                          FwVol, 
                          &FwVolFilePathNode->NameGuid, 
                          &Pe32Buffer,
                          &Pe32BufferSize,
                          &Type,
                          &Attrib,
                          AuthenticationStatus
                          );
      }
            
      if (!EFI_ERROR (Status)) {
        //
        // One of the reads passed so we are done
        //
        ImageFileHandle->Source = Pe32Buffer;
        ImageFileHandle->SourceSize = Pe32BufferSize;
        ImageFileHandle->FreeBuffer = TRUE;
        goto Done;
      }
    }
  }

  //
  // Attempt to access the file via a file system interface
  //
  FilePathNode = (FILEPATH_DEVICE_PATH *) *FilePath;
  Status = CoreDevicePathToInterface (
            &gEfiSimpleFileSystemProtocolGuid, 
            (EFI_DEVICE_PATH_PROTOCOL **)&FilePathNode, 
            (VOID*)&Volume, 
            DeviceHandle
            );
  if (!EFI_ERROR (Status) && (Volume != NULL)) {
    //
    // Open the Volume to get the File System handle
    //
    Status = Volume->OpenVolume (Volume, &FileHandle);
    if (!EFI_ERROR (Status)) {
      //
      // Duplicate the device path to avoid the access to unaligned device path node.
      // Because the device path consists of one or more FILE PATH MEDIA DEVICE PATH
      // nodes, It assures the fields in device path nodes are 2 byte aligned. 
      //
      FilePathNode = (FILEPATH_DEVICE_PATH *) CoreDuplicateDevicePath(
                                                (EFI_DEVICE_PATH_PROTOCOL *)(UINTN)FilePathNode
                                                );
      if (FilePathNode == NULL) {
        FileHandle->Close (FileHandle);
        FileHandle = NULL;
        Status = EFI_OUT_OF_RESOURCES;
      } else {
        OriginalFilePathNode = FilePathNode;
        //
        // Parse each MEDIA_FILEPATH_DP node. There may be more than one, since the
        //  directory information and filename can be seperate. The goal is to inch
        //  our way down each device path node and close the previous node
        //
        while (!IsDevicePathEnd (&FilePathNode->Header)) {
          if (DevicePathType (&FilePathNode->Header) != MEDIA_DEVICE_PATH ||
              DevicePathSubType (&FilePathNode->Header) != MEDIA_FILEPATH_DP) {
            Status = EFI_UNSUPPORTED;
          }

          if (EFI_ERROR (Status)) {
            //
            // Exit loop on Error
            //
            break;
          }

          LastHandle = FileHandle;
          FileHandle = NULL;
          Status = LastHandle->Open (
                                LastHandle,
                                &FileHandle,
                                FilePathNode->PathName,
                                EFI_FILE_MODE_READ,
                                0
                                );

          //
          // Close the previous node
          //
          LastHandle->Close (LastHandle);

          FilePathNode = (FILEPATH_DEVICE_PATH *) NextDevicePathNode (&FilePathNode->Header);
        }
        //
        // Free the allocated memory pool 
        //
        CoreFreePool (OriginalFilePathNode);
      }

      if (!EFI_ERROR (Status)) {
        //
        // We have found the file. Now we need to read it. Before we can read the file we need to
        // figure out how big the file is.
        //
        FileInfo = NULL;
        FileInfoSize = sizeof (EFI_FILE_INFO);
        while (CoreGrowBuffer (&Status, &FileInfo, FileInfoSize)) {
          //
          // Automatically allocate buffer of the correct size and make the call
          //
          Status = FileHandle->GetInfo (
                                FileHandle,
                                &gEfiFileInfoGuid,
                                &FileInfoSize,
                                FileInfo                               
                                );
        }
        if (!EFI_ERROR (Status) && (FileInfo != NULL)) {
          //
          // Allocate space for the file
          //
          ImageFileHandle->Source = CoreAllocateBootServicesPool ((UINTN)FileInfo->FileSize);
          if (ImageFileHandle->Source != NULL) {
            //
            // Read the file into the buffer we allocated
            //
            ImageFileHandle->SourceSize = (UINTN)FileInfo->FileSize;
            ImageFileHandle->FreeBuffer = TRUE;
            Status = FileHandle->Read (FileHandle, &ImageFileHandle->SourceSize, ImageFileHandle->Source);
          } else {
            Status = EFI_OUT_OF_RESOURCES;
          }
          //
          // Close the file since we are done
          //
          FileHandle->Close (FileHandle);
          CoreFreePool (FileInfo);
          goto Done;
        }

        if (FileInfo != NULL) {
          CoreFreePool (FileInfo);
        }
      }
      if (FileHandle != NULL) {
        FileHandle->Close (FileHandle);
      }
    }
  }


#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  //
  // Try LoadFile2 style
  //
  if (!BootPolicy) {
    TempFilePath = *FilePath;
    Status = CoreDevicePathToInterface (
               &gEfiLoadFile2ProtocolGuid,
               &TempFilePath,
               (VOID*)&LoadFile2,
               DeviceHandle
               );
    if (!EFI_ERROR (Status) && (LoadFile2 != NULL)) {
      //
      // Call LoadFile2 with the correct buffer size
      //
      while (CoreGrowBuffer (&Status, &ImageFileHandle->Source, ImageFileHandle->SourceSize)) {
        Status = LoadFile2->LoadFile (
                              LoadFile2,
                              TempFilePath,
                              BootPolicy,
                              &ImageFileHandle->SourceSize,
                              ImageFileHandle->Source
                              );
        //
        // If success or other error happens, stop loop
        //
        if (Status != EFI_BUFFER_TOO_SMALL) {
          break;
        }
      }

      if (!EFI_ERROR (Status)) {
        ImageFileHandle->FreeBuffer = TRUE;
        goto Done;
      }
    }
  }
#endif

  //
  // Try LoadFile style
  //
  TempFilePath = *FilePath;
  Status = CoreDevicePathToInterface (
              &gEfiLoadFileProtocolGuid,
              &TempFilePath,
              (VOID*)&LoadFile,
              DeviceHandle
              );
  if (!EFI_ERROR (Status) && (LoadFile != NULL)) {
    //
    // Call LoadFile with the correct buffer size
    //
    while (CoreGrowBuffer (&Status, &ImageFileHandle->Source, ImageFileHandle->SourceSize)) {
      Status = LoadFile->LoadFile (
                           LoadFile,
                           TempFilePath,
                           BootPolicy,
                           &ImageFileHandle->SourceSize,
                           ImageFileHandle->Source
                           );
      //
      // If success or other error happens, stop loop
      //
      if (Status != EFI_BUFFER_TOO_SMALL) {
        break;
      }
    }

    if (!EFI_ERROR (Status) || Status == EFI_ALREADY_STARTED) {
      ImageFileHandle->FreeBuffer = TRUE;
      goto Done;
    }
  }

  //
  // Nothing else to try
  //
  DEBUG ((EFI_D_LOAD|EFI_D_WARN, "CoreOpenImageFile: Device did not support a known load protocol\n"));
  Status = EFI_NOT_FOUND;

Done:

  //
  // If the file was not accessed, clean up
  //
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    if (ImageFileHandle->FreeBuffer) {
      //
      // Free the source buffer if we allocated it
      //
      CoreFreePool (ImageFileHandle->Source);
    }
  }

  return Status;
}



EFI_STATUS
EFIAPI
CoreReadImageFile (
  IN     VOID    *UserHandle,
  IN     UINTN   Offset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  )
/*++

Routine Description:

  Read image file (specified by UserHandle) into user specified buffer with specified offset
  and length.

Arguments:

  UserHandle      - Image file handle
  
  Offset          - Offset to the source file
  
  ReadSize        - For input, pointer of size to read;
                    For output, pointer of size actually read.
  
  Buffer          - Buffer to write into

Returns:

  EFI_SUCCESS     - Successfully read the specified part of file into buffer.

--*/
{
  UINTN               EndPosition;
  IMAGE_FILE_HANDLE  *FHand;

  FHand = (IMAGE_FILE_HANDLE  *)UserHandle;
  ASSERT (FHand->Signature == IMAGE_FILE_HANDLE_SIGNATURE);

  //
  // Move data from our local copy of the file
  //
  EndPosition = Offset + *ReadSize;
  if (EndPosition > FHand->SourceSize) {
    *ReadSize = (UINT32)(FHand->SourceSize - Offset);
  }  
  if (Offset >= FHand->SourceSize) {
      *ReadSize = 0;
  }

  EfiCommonLibCopyMem (Buffer, (CHAR8 *)FHand->Source + Offset, *ReadSize);
  return EFI_SUCCESS;
}

EFI_STATUS
CoreDevicePathToInterface (
  IN EFI_GUID                     *Protocol,
  IN EFI_DEVICE_PATH_PROTOCOL     **FilePath,
  OUT VOID                        **Interface,
  OUT EFI_HANDLE                  *Handle
  )
/*++

Routine Description:

  Search a handle to a device on a specified device path that supports a specified protocol,
  interface of that protocol on that handle is another output.

Arguments:

  Protocol      - The protocol to search for
  
  FilePath      - The specified device path
  
  Interface     - Interface of the protocol on the handle
  
  Handle        - The handle to the device on the specified device path that supports the protocol.
  
Returns:

  Status code.

--*/
{
  EFI_STATUS                      Status;

  Status = CoreLocateDevicePath (Protocol, FilePath, Handle);
  if (!EFI_ERROR (Status)) {
    Status = CoreHandleProtocol (*Handle, Protocol, Interface);
  }
  return Status;
}


VOID
CoreDevicePathToFileName (
  IN  FILEPATH_DEVICE_PATH      *FilePath,
  OUT CHAR16                    **String
  )
/*++

Routine Description:

  Transfer a device's full path a string.

Arguments:

  FilePath      - Device path
  
  String        - The string represent the device's full path

Returns:

  None

--*/
{
  UINTN                     StringSize;
  FILEPATH_DEVICE_PATH      *FilePathNode;
  CHAR16                    *Str;

  *String = NULL;
  StringSize = 0;
  FilePathNode = FilePath;
  while (!IsDevicePathEnd (&FilePathNode->Header)) {

    //
    // For filesystem access each node should be a filepath component
    //
    if (DevicePathType (&FilePathNode->Header) != MEDIA_DEVICE_PATH ||
        DevicePathSubType (&FilePathNode->Header) != MEDIA_FILEPATH_DP) {

      return;
    }

    StringSize += EfiStrLen (FilePathNode->PathName);

    FilePathNode = (FILEPATH_DEVICE_PATH *) NextDevicePathNode (&FilePathNode->Header);
  }

  *String = CoreAllocateBootServicesPool (StringSize);
  if (*String == NULL) {
    return;
  }

  FilePathNode = FilePath;
  Str = *String;
  while (!IsDevicePathEnd (&FilePathNode->Header)) {
    EfiStrCat (Str, FilePathNode->PathName);
    FilePathNode = (FILEPATH_DEVICE_PATH *) NextDevicePathNode (&FilePathNode->Header);
  }
}


BOOLEAN
CoreGrowBuffer (
  IN OUT EFI_STATUS   *Status,
  IN OUT VOID         **Buffer,
  IN UINTN            BufferSize
  )
/*++

Routine Description:

    Helper function called as part of the code needed
    to allocate the proper sized buffer for various 
    EFI interfaces.

Arguments:

    Status      - Current status

    Buffer      - Current allocated buffer, or NULL

    BufferSize  - Current buffer size needed
    
Returns:
    
    TRUE - if the buffer was reallocated and the caller 
    should try the API again.

    FALSE - buffer could not be allocated and the caller
    should not try the API again.

--*/
{
  BOOLEAN         TryAgain;

  TryAgain = FALSE;
  //
  // If this is an initial request, buffer will be null with a new buffer size
  //
  if (*Buffer == NULL) {
    *Status = EFI_BUFFER_TOO_SMALL;
  }

  if (BufferSize == 0) {
    return TRUE;
  }

  //
  // If the status code is "buffer too small", resize the buffer
  //
      
  if (*Status == EFI_BUFFER_TOO_SMALL) {
    if (*Buffer != NULL) {
      CoreFreePool (*Buffer);
    }

    *Buffer = CoreAllocateBootServicesPool (BufferSize);
    if (*Buffer != NULL) {
      TryAgain = TRUE;
    } else {    
      *Status = EFI_OUT_OF_RESOURCES;
    } 
  }

  //
  // If there's an error, free the buffer
  //
  if ((!TryAgain) && (EFI_ERROR (*Status)) && (*Buffer)) {
    CoreFreePool (*Buffer);
    *Buffer = NULL;
  }

  return TryAgain;
}

