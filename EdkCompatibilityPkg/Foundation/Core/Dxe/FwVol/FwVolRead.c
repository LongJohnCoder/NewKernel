/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FwVolRead.c

Abstract:

  Implements read firmware file

--*/

#include "FwVolDriver.h"
#include "DxeCore.h"

/*++

Required Alignment    Alignment Value in FFS       Alignment Value in
(bytes)                        Attributes Field               Firmware Volume Interfaces
1                                    0                                     0
2                                    0                                     1
4                                    0                                     2
8                                    0                                     3
16                                   1                                     4
128                                  2                                     7
512                                  3                                     9
1 KB                                 4                                     10
4 KB                                 5                                     12
32 KB                                6                                     15
64 KB                                7                                     16

--*/

UINT8 mFvAttributes[] = {0, 4, 7, 9, 10, 12, 15, 16}; 


STATIC
EFI_FV_FILE_ATTRIBUTES
FfsAttributes2FvFileAttributes (
  IN EFI_FFS_FILE_ATTRIBUTES FfsAttributes
  )
/*++

  Routine Description:
    Convert the FFS File Attributes to FV File Attributes
    
  Arguments:
    FfsAttributes   -   The attributes of UINT8 type.
    
  Returns:
    The attributes of EFI_FV_FILE_ATTRIBUTES
    
--*/
{
  FfsAttributes = (EFI_FFS_FILE_ATTRIBUTES)((FfsAttributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3);
  ASSERT (FfsAttributes < 8);

  return (EFI_FV_FILE_ATTRIBUTES) mFvAttributes[FfsAttributes];
}


EFI_STATUS
EFIAPI
FvGetNextFile (
  IN         EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN OUT     VOID                            *Key,
  IN OUT     EFI_FV_FILETYPE                *FileType,
  OUT        EFI_GUID                      *NameGuid,
  OUT        EFI_FV_FILE_ATTRIBUTES        *Attributes,
  OUT        UINTN                           *Size
  )
/*++

Routine Description:
    Given the input key, search for the next matching file in the volume.

Arguments:
    This          -   Indicates the calling context.
    FileType      -   FileType is a pointer to a caller allocated
                      EFI_FV_FILETYPE. The GetNextFile() API can filter it's
                      search for files based on the value of *FileType input.
                      A *FileType input of 0 causes GetNextFile() to search for
                      files of all types.  If a file is found, the file's type
                      is returned in *FileType.  *FileType is not modified if
                      no file is found.
    Key           -   Key is a pointer to a caller allocated buffer that
                      contains implementation specific data that is used to
                      track where to begin the search for the next file.
                      The size of the buffer must be at least This->KeySize
                      bytes long. To reinitialize the search and begin from
                      the beginning of the firmware volume, the entire buffer
                      must be cleared to zero. Other than clearing the buffer
                      to initiate a new search, the caller must not modify the
                      data in the buffer between calls to GetNextFile().
    NameGuid      -   NameGuid is a pointer to a caller allocated EFI_GUID.
                      If a file is found, the file's name is returned in
                      *NameGuid.  *NameGuid is not modified if no file is
                      found.
    Attributes    -   Attributes is a pointer to a caller allocated
                      EFI_FV_FILE_ATTRIBUTES.  If a file is found, the file's
                      attributes are returned in *Attributes. *Attributes is
                      not modified if no file is found.
    Size          -   Size is a pointer to a caller allocated UINTN.
                      If a file is found, the file's size is returned in *Size.
                      *Size is not modified if no file is found.

Returns:
    EFI_SUCCESS                 - Successfully find the file.
    EFI_DEVICE_ERROR            - Device error.
    EFI_ACCESS_DENIED           - Fv could not read.
    EFI_NOT_FOUND               - No matching file found.
    EFI_INVALID_PARAMETER       - Invalid parameter

--*/
{
  EFI_STATUS                                  Status;
  FV_DEVICE                                   *FvDevice;
  EFI_FV_ATTRIBUTES                           FvAttributes;
  EFI_FFS_FILE_HEADER                         *FfsFileHeader;
  UINTN                                       *KeyValue;
  EFI_LIST_ENTRY                              *Link;
  FFS_FILE_LIST_ENTRY                         *FfsFileEntry;
  UINTN                                       FileLength;

  FvDevice = FV_DEVICE_FROM_THIS (This);

  Status = FvGetVolumeAttributes (This, &FvAttributes);
  if (EFI_ERROR (Status)){
    return Status;
  }

  //
  // Check if read operation is enabled
  //
  if ((FvAttributes & EFI_FV_READ_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }

  if (*FileType > EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE) {
    //
    // File type needs to be in 0 - 0x0B
    //
    return EFI_INVALID_PARAMETER;
  }

  KeyValue = (UINTN *)Key;
  for (;;) {
    if (*KeyValue == 0) {
      //
      // Search for 1st matching file
      //
      Link = &FvDevice->FfsFileListHeader;
    } else {
      //
      // Key is pointer to FFsFileEntry, so get next one
      //
      Link = (EFI_LIST_ENTRY *)(*KeyValue);
    }

    if (Link->ForwardLink == &FvDevice->FfsFileListHeader) {
      //
      // Next is end of list so we did not find data
      //
      return EFI_NOT_FOUND;
    }

    FfsFileEntry = (FFS_FILE_LIST_ENTRY *)Link->ForwardLink;
    FfsFileHeader = (EFI_FFS_FILE_HEADER *)FfsFileEntry->FfsHeader;

    //
    // remember the key
    //
    *KeyValue = (UINTN)FfsFileEntry;

    if (FfsFileHeader->Type == EFI_FV_FILETYPE_FFS_PAD) {
      //
      // we ignore pad files
      //
      continue;
    }

    if (*FileType == 0) {
      //
      // Process all file types so we have a match
      //
      break;
    }

    if (*FileType == FfsFileHeader->Type) {
      //
      // Found a matching file type
      //
      break;
    }

  } 

  //
  // Return FileType, NameGuid, and Attributes
  //
  *FileType = FfsFileHeader->Type;
  EfiCommonLibCopyMem (NameGuid, &FfsFileHeader->Name, sizeof (EFI_GUID));
  *Attributes = FfsAttributes2FvFileAttributes (FfsFileHeader->Attributes);

  //
  // Read four bytes out of the 3 byte array and throw out extra data
  //
  FileLength = *(UINT32 *)&FfsFileHeader->Size[0] & 0x00FFFFFF;

  //
  // we need to substract the header size
  //
  *Size = FileLength - sizeof(EFI_FFS_FILE_HEADER);

  if (FfsFileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
    //
    // If tail is present substract it's size;
    //
    *Size -= sizeof(EFI_FFS_FILE_TAIL);
  }

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
FvReadFile (
  IN     EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN     EFI_GUID                       *NameGuid,
  IN OUT VOID                           **Buffer,
  IN OUT UINTN                          *BufferSize,
  OUT    EFI_FV_FILETYPE               *FoundType,
  OUT    EFI_FV_FILE_ATTRIBUTES        *FileAttributes,
  OUT    UINT32                        *AuthenticationStatus
  )
/*++

Routine Description:
    Locates a file in the firmware volume and
    copies it to the supplied buffer.

Arguments:
    This              -   Indicates the calling context.
    NameGuid          -   Pointer to an EFI_GUID, which is the filename.
    Buffer            -   Buffer is a pointer to pointer to a buffer in
                          which the file or section contents or are returned.
    BufferSize        -   BufferSize is a pointer to caller allocated
                          UINTN. On input *BufferSize indicates the size
                          in bytes of the memory region pointed to by
                          Buffer. On output, *BufferSize contains the number
                          of bytes required to read the file.
    FoundType         -   FoundType is a pointer to a caller allocated
                          EFI_FV_FILETYPE that on successful return from Read()
                          contains the type of file read.  This output reflects
                          the file type irrespective of the value of the
                          SectionType input.
    FileAttributes    -   FileAttributes is a pointer to a caller allocated
                          EFI_FV_FILE_ATTRIBUTES.  On successful return from
                          Read(), *FileAttributes contains the attributes of
                          the file read.
    AuthenticationStatus -  AuthenticationStatus is a pointer to a caller
                          allocated UINTN in which the authentication status
                          is returned.
Returns:
    EFI_SUCCESS                   - Successfully read to memory buffer.
    EFI_WARN_BUFFER_TOO_SMALL     - Buffer too small.
    EFI_NOT_FOUND                 - Not found.
    EFI_DEVICE_ERROR              - Device error.
    EFI_ACCESS_DENIED             - Could not read.
    EFI_INVALID_PARAMETER         - Invalid parameter.
    EFI_OUT_OF_RESOURCES          - Not enough buffer to be allocated.

--*/
{
  EFI_STATUS                        Status;
  FV_DEVICE                         *FvDevice;
  EFI_GUID                          SearchNameGuid;
  EFI_FV_FILETYPE                   LocalFoundType;
  EFI_FV_FILE_ATTRIBUTES            LocalAttributes;
  UINTN                             FileSize;
  UINT8                             *SrcPtr;
  EFI_FFS_FILE_HEADER               *FfsHeader;
  UINTN                             InputBufferSize;
  
  if (NULL == NameGuid) {
    return EFI_INVALID_PARAMETER;
  }

  FvDevice = FV_DEVICE_FROM_THIS (This);
  

  //
  // Keep looking until we find the matching NameGuid.
  // The Key is really an FfsFileEntry
  //
  FvDevice->LastKey = 0;
  do {
    LocalFoundType = 0;
    Status = FvGetNextFile (
              This,
              &FvDevice->LastKey,
              &LocalFoundType,
              &SearchNameGuid,
              &LocalAttributes,
              &FileSize
              );
    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }
  } while (!EfiCompareGuid (&SearchNameGuid, NameGuid));

  //
  // Get a pointer to the header
  //
  FfsHeader = FvDevice->LastKey->FfsHeader;

  //
  // Remember callers buffer size
  //
  InputBufferSize = *BufferSize;

  //
  // Calculate return values
  //
  *FoundType = FfsHeader->Type;
  *FileAttributes = FfsAttributes2FvFileAttributes (FfsHeader->Attributes);
  *AuthenticationStatus = 0;
  *BufferSize = FileSize;

  if (Buffer == NULL) {
    //
    // If Buffer is NULL, we only want to get the information colected so far
    //
    return EFI_SUCCESS;
  }

  //
  // Skip over file header
  //
  SrcPtr = ((UINT8 *)FfsHeader) + sizeof (EFI_FFS_FILE_HEADER);

  Status = EFI_SUCCESS;
  if (*Buffer == NULL) {
    //
    // Caller passed in a pointer so allocate buffer for them
    //
    *Buffer = CoreAllocateBootServicesPool (FileSize);
    if (*Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else if (FileSize > InputBufferSize) {
    //
    // Callers buffer was not big enough
    // 
    Status = EFI_WARN_BUFFER_TOO_SMALL;
    FileSize = InputBufferSize;
  }
  
  //
  // Copy data into callers buffer 
  //
  EfiCommonLibCopyMem (*Buffer, SrcPtr, FileSize);

  return Status;
}


EFI_STATUS
EFIAPI
FvReadFileSection (
  IN     EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN     EFI_GUID                       *NameGuid,
  IN     EFI_SECTION_TYPE               SectionType,
  IN     UINTN                          SectionInstance,
  IN OUT VOID                           **Buffer,
  IN OUT UINTN                          *BufferSize,
  OUT    UINT32                         *AuthenticationStatus
  )
/*++

  Routine Description:
    Locates a section in a given FFS File and
    copies it to the supplied buffer (not including section header).

  Arguments:
    This              -   Indicates the calling context.
    NameGuid          -   Pointer to an EFI_GUID, which is the filename.
    SectionType       -   Indicates the section type to return.
    SectionInstance   -   Indicates which instance of sections with a type of
                          SectionType to return.
    Buffer            -   Buffer is a pointer to pointer to a buffer in which
                          the file or section contents or are returned.
    BufferSize        -   BufferSize is a pointer to caller allocated UINTN.
    AuthenticationStatus -AuthenticationStatus is a pointer to a caller
                          allocated UINT32 in which the authentication status
                          is returned.

  Returns:
    EFI_SUCCESS                     - Successfully read the file section into buffer.
    EFI_WARN_BUFFER_TOO_SMALL       - Buffer too small.
    EFI_NOT_FOUND                   - Section not found.
    EFI_DEVICE_ERROR                - Device error.
    EFI_ACCESS_DENIED               - Could not read.
    EFI_INVALID_PARAMETER           - Invalid parameter.

--*/
{
  EFI_STATUS                        Status;
  FV_DEVICE                         *FvDevice;
  EFI_FV_FILETYPE                   FileType;
  EFI_FV_FILE_ATTRIBUTES            FileAttributes;
  UINTN                             FileSize;
  UINT8                             *FileBuffer;
  EFI_SECTION_EXTRACTION_PROTOCOL   *Sep;
  FFS_FILE_LIST_ENTRY               *FfsEntry;
 
  if (NULL == NameGuid || Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FvDevice = FV_DEVICE_FROM_THIS (This);

  //
  // Read the whole file into buffer
  //
  FileBuffer = NULL;
  Status = FvReadFile (
            This,
            NameGuid,
            &FileBuffer,
            &FileSize,
            &FileType,
            &FileAttributes,
            AuthenticationStatus
            );             
  //
  // Get the last key used by our call to FvReadFile as it is the FfsEntry for this file.
  //  
  FfsEntry = (FFS_FILE_LIST_ENTRY *)FvDevice->LastKey;

  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Check to see that the file actually HAS sections before we go any further.
  //
  if (FileType == EFI_FV_FILETYPE_RAW) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  //
  // Use FfsEntry to cache Section Extraction Protocol Inforomation
  //
  if (FfsEntry->StreamHandle == 0) {
    //
    // Located the protocol
    //
    Status = CoreLocateProtocol (&gEfiSectionExtractionProtocolGuid, NULL, &Sep);
    //
    // Section Extraction Protocol is part of Dxe Core so this should never fail
    //
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status) || (Sep == NULL)) {
      goto Done;
    }

    Status = Sep->OpenSectionStream (
                    Sep,
                    FileSize,
                    FileBuffer,
                    &FfsEntry->StreamHandle
                    );
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    FfsEntry->Sep = Sep;
  } else {
    //
    // Get cached copy of Sep
    //
    Sep = FfsEntry->Sep;
  }

  //
  // If SectionType == 0 We need the whole section stream
  //
  Status = Sep->GetSection (
                  Sep,
                            FfsEntry->StreamHandle,
                            (SectionType == 0) ? NULL : &SectionType,
                            NULL,
                            (SectionType == 0) ? 0 : SectionInstance,
                            Buffer,
                            BufferSize,
                            AuthenticationStatus
                            );

  //
  // Close of stream defered to close of FfsHeader list to allow SEP to cache data
  //

Done:
  CoreFreePool (FileBuffer);

  return Status;
}

