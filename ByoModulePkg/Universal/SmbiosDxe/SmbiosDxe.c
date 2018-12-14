/** @file
  This code produces the Smbios protocol. It also responsible for constructing 
  SMBIOS table into system table.
  
Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "SmbiosDxe.h"

//
// Module Global:
// Since this driver will only ever produce one instance of the
// protocol you are not required to dynamically allocate the PrivateData.
//
SMBIOS_INSTANCE mPrivateData;
BOOLEAN         mAllDriversConnected = FALSE;
extern EFI_GUID gByoAllDriversConnectedProtocolGuid;
//
// Chassis for SMBIOS entry point structure that is to be installed into EFI system config table.
//
SMBIOS_TABLE_ENTRY_POINT *EntryPointStructure    = NULL;
SMBIOS_TABLE_ENTRY_POINT EntryPointStructureData = {
  //
  // AnchorString
  //
  {
    0x5f,
    0x53,
    0x4d,
    0x5f
  },
  //
  // EntryPointStructureChecksum,TO BE FILLED
  //
  0,
  //
  // EntryPointStructure Length
  //
  0x1f,
  //
  // MajorVersion
  //
  (UINT8) (FixedPcdGet16 (PcdSmbiosVersion) >> 8),
  //
  // MinorVersion
  //
  (UINT8) (FixedPcdGet16 (PcdSmbiosVersion) & 0x00ff),
  //
  // MaxStructureSize, TO BE FILLED
  //
  0,
  //
  // EntryPointRevision
  //
  0,
  //
  // FormattedArea
  //
  {
    0,
    0,
    0,
    0,
    0
  },
  //
  // IntermediateAnchorString
  //
  {
    0x5f,
    0x44,
    0x4d,
    0x49,
    0x5f
  },
  //
  // IntermediateChecksum, TO BE FILLED
  //
  0,
  //
  // StructureTableLength, TO BE FILLED
  //
  0,
  //
  // StructureTableAddress, TO BE FILLED
  //
  0,
  //
  // NumberOfSmbiosStructures, TO BE FILLED
  //
  0,
  //
  // SmbiosBcdRevision
  //
  0  
};

BOOLEAN
SmbiosCheckRecord (
  IN EFI_SMBIOS_TABLE_HEADER *SmbiosRecord
  )
{
  UINT8 *RecordPtr;
  UINT8 *RecordEndPtr;
  PNP_52_DATA_BUFFER *DataBufferInRec;

  RecordPtr = (UINT8 *)(UINTN)mSmbiosRecBase;
  RecordEndPtr = (UINT8 *)(UINTN) (mSmbiosRecBase + SMBIOS_REC_SIZE);

  while (RecordPtr < RecordEndPtr) {
     if (((SMBIOS_REC_HEADER *)RecordPtr)->Signature != SMBIOS_REC_SIGNATURE)
        break;

     DataBufferInRec = (PNP_52_DATA_BUFFER *)(RecordPtr +sizeof(SMBIOS_REC_HEADER));

     if ((DataBufferInRec->StructureHeader.Type == SmbiosRecord->Type)
         && (DataBufferInRec->StructureHeader.Handle== SmbiosRecord->Handle))
         return TRUE;

     RecordPtr = RecordPtr + ((SMBIOS_REC_HEADER *)RecordPtr)->RecordLen;
  }

  return FALSE;
}

EFI_STATUS
SmbiosUpdateSimpleData (
  IN PNP_52_DATA_BUFFER *DataBufferInRec,
  IN EFI_SMBIOS_TABLE_HEADER *SmbiosRecord
  )
{
  UINT8 Value8;
  UINT16 Value16;
  UINT32 Value32;

  if (DataBufferInRec->Command == ByteChanged) {
    Value8 = *(UINT8 *)(((UINT8 *)SmbiosRecord) + DataBufferInRec->FieldOffset);
    Value8 &= DataBufferInRec->ChangeMask;
    Value8 |= DataBufferInRec->ChangeValue;
    *(UINT8 *)(((UINT8 *)SmbiosRecord) + DataBufferInRec->FieldOffset) = Value8;
  } else if (DataBufferInRec->Command == WordChanged) {
    Value16 = *(UINT16 *)(((UINT8 *)SmbiosRecord) + DataBufferInRec->FieldOffset);
    Value16 &= DataBufferInRec->ChangeMask;
    Value16 |= DataBufferInRec->ChangeValue;
    *(UINT16 *)(((UINT8 *)SmbiosRecord) + DataBufferInRec->FieldOffset) = Value16;
  } else if (DataBufferInRec->Command == DoubleWordChanged) {
    Value32 = *(UINT32 *)(((UINT8 *)SmbiosRecord) + DataBufferInRec->FieldOffset);
    Value32 &= DataBufferInRec->ChangeMask;
    Value32 |= DataBufferInRec->ChangeValue;
    *(UINT32 *)(((UINT8 *)SmbiosRecord) + DataBufferInRec->FieldOffset) = Value32;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SmbiosUpdateBlockData (
  IN PNP_52_DATA_BUFFER *DataBufferInRec,
  IN EFI_SMBIOS_TABLE_HEADER *SmbiosRecord
  )
{
  UINT8 *BlockDataPtr = NULL;

  BlockDataPtr = (((UINT8 *)SmbiosRecord) + DataBufferInRec->FieldOffset);

  //
  // Copy the content into the block area
  //
  CopyMem (BlockDataPtr, DataBufferInRec->StructureData, DataBufferInRec->DataLength);
  return EFI_SUCCESS;
}

EFI_STATUS
SmbiosUpdateStructureWithRecord (
  IN  EFI_SMBIOS_TABLE_HEADER  **SmbiosRecord
  )
{
  UINT8              *RecordPtr;
  UINT8              *RecordEndPtr;
  PNP_52_DATA_BUFFER *DataBufferInRec;
  EFI_STATUS         Status;
  UINT8              Command;

  RecordPtr = (UINT8 *)(UINTN)mSmbiosRecBase;
  RecordEndPtr = (UINT8 *)(UINTN)(mSmbiosRecBase + SMBIOS_REC_SIZE);

  while (RecordPtr < RecordEndPtr) {
    if (((SMBIOS_REC_HEADER *)RecordPtr)->Signature != SMBIOS_REC_SIGNATURE)
      break;

    DataBufferInRec = (PNP_52_DATA_BUFFER *)(RecordPtr + sizeof (SMBIOS_REC_HEADER));

    if ((DataBufferInRec->StructureHeader.Type == (*SmbiosRecord)->Type)
        && (DataBufferInRec->StructureHeader.Handle == (*SmbiosRecord)->Handle)) {
      Status  = EFI_SUCCESS;
      Command = DataBufferInRec->Command;
      if ((Command == ByteChanged) ||(Command == WordChanged) ||(Command == DoubleWordChanged)) {
        Status = SmbiosUpdateSimpleData (DataBufferInRec, *SmbiosRecord);
      } else if (Command == BlockChanged) {
        Status = SmbiosUpdateBlockData (DataBufferInRec, *SmbiosRecord);
      }
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    RecordPtr = RecordPtr + ((SMBIOS_REC_HEADER *)RecordPtr)->RecordLen;
  }

   return EFI_SUCCESS;
}

VOID
UpdateRecordToSmbiosTable (
  IN CONST EFI_SMBIOS_PROTOCOL *This
)
{
  UINT8                     *RecordPtr;
  UINT8                     *RecordEndPtr;
  PNP_52_DATA_BUFFER        *DataBufferInRec;
  EFI_SMBIOS_TABLE_HEADER   *Record;
  EFI_SMBIOS_HANDLE         Handle;
  EFI_STATUS                Status;

  RecordPtr = (UINT8 *)(UINTN)mSmbiosRecBase;
  RecordEndPtr = (UINT8 *)(UINTN)(mSmbiosRecBase + SMBIOS_REC_SIZE);

  while (RecordPtr < RecordEndPtr) {
    if (((SMBIOS_REC_HEADER *)RecordPtr)->Signature != SMBIOS_REC_SIGNATURE)
      break;

    DataBufferInRec = (PNP_52_DATA_BUFFER *)(RecordPtr + sizeof (SMBIOS_REC_HEADER));
    Handle = DataBufferInRec->StructureHeader.Handle;
    if (DataBufferInRec->Command == AddChanged) {
      Record = (EFI_SMBIOS_TABLE_HEADER *)&DataBufferInRec->StructureHeader;
      Status = mPrivateData.Smbios.Add (This, NULL, &Handle, Record);
      DEBUG((EFI_D_ERROR,"Smbios Add Status = %r\n", Status));
      if (Status == EFI_ALREADY_STARTED) {
        //
        // Re-add with the Handle = SMBIOS_HANDLE_PI_RESERVED, an available handle will be assigned
        //
        Handle = SMBIOS_HANDLE_PI_RESERVED;
        Status = mPrivateData.Smbios.Add (This, NULL, &Handle, Record);
        DEBUG((EFI_D_ERROR,"Smbios Re-Add Status = %r\n", Status));
      }
    } else if (DataBufferInRec->Command == DeleteChanged) {
      mPrivateData.Smbios.Remove (This, Handle);
    }
    RecordPtr = RecordPtr + ((SMBIOS_REC_HEADER *)RecordPtr)->RecordLen;
  }

}

/**

  Get the full size of SMBIOS structure including optional strings that follow the formatted structure.

  @param This                   The EFI_SMBIOS_PROTOCOL instance.
  @param Head                   Pointer to the beginning of SMBIOS structure.
  @param Size                   The returned size.
  @param NumberOfStrings        The returned number of optional strings that follow the formatted structure.

  @retval EFI_SUCCESS           Size retured in Size.
  @retval EFI_INVALID_PARAMETER Input SMBIOS structure mal-formed or Size is NULL.
  
**/
EFI_STATUS
EFIAPI
GetSmbiosStructureSize (
  IN   CONST EFI_SMBIOS_PROTOCOL        *This,
  IN   EFI_SMBIOS_TABLE_HEADER          *Head,
  OUT  UINTN                            *Size,
  OUT  UINTN                            *NumberOfStrings
  )
{
  UINTN  FullSize;
  UINT8  StrLen;
  INT8*  CharInStr;
  
  if (Size == NULL || NumberOfStrings == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FullSize = Head->Length;
  CharInStr = (INT8*)Head + Head->Length;
  *Size = FullSize;
  *NumberOfStrings = 0;
  StrLen = 0;
  //
  // look for the two consecutive zeros, check the string limit by the way.
  //
  while (*CharInStr != 0 || *(CharInStr+1) != 0) { 
    if (*CharInStr == 0) {
      *Size += 1;
      CharInStr++;
    }

    if (This->MajorVersion < 2 || (This->MajorVersion == 2 && This->MinorVersion < 7)){
      for (StrLen = 0 ; StrLen < SMBIOS_STRING_MAX_LENGTH; StrLen++) {
        if (*(CharInStr+StrLen) == 0) {
          break;
        }
      }

      if (StrLen == SMBIOS_STRING_MAX_LENGTH) {
        return EFI_INVALID_PARAMETER;
      }
    } else {
      //
      // Reference SMBIOS 2.7, chapter 6.1.3, it will have no limit on the length of each individual text string
      //
      for (StrLen = 0 ;; StrLen++) {
        if (*(CharInStr+StrLen) == 0) {
          break;
        }
      }
    }

    //
    // forward the pointer
    //
    CharInStr += StrLen;
    *Size += StrLen;
    *NumberOfStrings += 1;
  }

  //
  // count ending two zeros.
  //
  *Size += 2;
  return EFI_SUCCESS;
}

/**

  Determin whether an SmbiosHandle has already in use.

  @param Head        Pointer to the beginning of SMBIOS structure.
  @param Handle      A unique handle will be assigned to the SMBIOS record.

  @retval TRUE       Smbios handle already in use.
  @retval FALSE      Smbios handle is NOT used.
  
**/
BOOLEAN
EFIAPI
CheckSmbiosHandleExistance (
  IN  LIST_ENTRY           *Head,
  IN  EFI_SMBIOS_HANDLE    Handle
  )
{
  LIST_ENTRY              *Link;
  SMBIOS_HANDLE_ENTRY     *HandleEntry;
  
  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    HandleEntry = SMBIOS_HANDLE_ENTRY_FROM_LINK(Link);
    if (HandleEntry->SmbiosHandle == Handle) {
      return TRUE;
    }
  }

  return FALSE;
}

/**

  Get the max SmbiosHandle that could be use.

  @param  This           The EFI_SMBIOS_PROTOCOL instance.
  @param  MaxHandle      The max handle that could be assigned to the SMBIOS record.

**/
VOID
EFIAPI
GetMaxSmbiosHandle (
  IN CONST  EFI_SMBIOS_PROTOCOL   *This,
  IN OUT    EFI_SMBIOS_HANDLE     *MaxHandle
  ) 
{
  if (This->MajorVersion == 2 && This->MinorVersion == 0) {
    *MaxHandle = 0xFFFE;
  } else {
    *MaxHandle = 0xFEFF;
  }
}

/**

  Get an SmbiosHandle that could use.

  @param  This                   The EFI_SMBIOS_PROTOCOL instance.
  @param  SmbiosHandle           A unique handle will be assigned to the SMBIOS record.

  @retval EFI_SUCCESS            Smbios handle got.
  @retval EFI_OUT_OF_RESOURCES   Smbios handle is NOT available.
  
**/
EFI_STATUS
EFIAPI
GetAvailableSmbiosHandle (
  IN CONST EFI_SMBIOS_PROTOCOL   *This,
  IN OUT   EFI_SMBIOS_HANDLE     *Handle
  )
{
  LIST_ENTRY              *Head;
  SMBIOS_INSTANCE         *Private;
  EFI_SMBIOS_HANDLE       MaxSmbiosHandle;
  EFI_SMBIOS_HANDLE       AvailableHandle;

  GetMaxSmbiosHandle(This, &MaxSmbiosHandle);

  Private = SMBIOS_INSTANCE_FROM_THIS (This);
  Head = &Private->AllocatedHandleListHead;
  for (AvailableHandle = 0; AvailableHandle < MaxSmbiosHandle; AvailableHandle++) {
    if (!CheckSmbiosHandleExistance(Head, AvailableHandle)) {
      *Handle = AvailableHandle;
      return EFI_SUCCESS;
    }
  }

  return EFI_OUT_OF_RESOURCES;
}



VOID ProperInsertList (
  IN OUT  LIST_ENTRY                *ListHead,
  IN OUT  LIST_ENTRY                *Entry,
  IN      EFI_SMBIOS_TYPE           Type
)
{
  LIST_ENTRY               *Link;
  LIST_ENTRY               *Previous;
  EFI_SMBIOS_ENTRY         *SmbiosEntry;
  EFI_SMBIOS_TABLE_HEADER  *SmbiosTableHeader;  
  UINTN                    Index = 0;

 
  if(ListHead->ForwardLink == ListHead || Type == EFI_SMBIOS_TYPE_END_OF_TABLE){  // empty or end
    InsertTailList (ListHead, Entry);
    goto ProcExit;
  }

  for (Link = ListHead->ForwardLink; Link != ListHead; Link = Link->ForwardLink) {
    Index++;
    SmbiosEntry = SMBIOS_ENTRY_FROM_LINK(Link);
    SmbiosTableHeader = (EFI_SMBIOS_TABLE_HEADER*)(SmbiosEntry->RecordHeader + 1);
    if(SmbiosTableHeader->Type != EFI_SMBIOS_TYPE_END_OF_TABLE &&
       SmbiosTableHeader->Type <= Type){
      continue;
    }

    Previous = Link->BackLink;
    Entry->ForwardLink = Previous->ForwardLink;
    Entry->BackLink = Previous;
    Previous->ForwardLink = Entry;
    Link->BackLink = Entry;

    goto ProcExit;
  }

  InsertTailList (ListHead, Entry);

ProcExit:
  DEBUG((EFI_D_INFO, "%a() T:%d Index:%d\n", __FUNCTION__, Type, Index));  
  return;
}





/**
  Add an SMBIOS record.

  @param  This                  The EFI_SMBIOS_PROTOCOL instance.
  @param  ProducerHandle        The handle of the controller or driver associated with the SMBIOS information. NULL
                                means no handle.
  @param  SmbiosHandle          On entry, the handle of the SMBIOS record to add. If FFFEh, then a unique handle
                                will be assigned to the SMBIOS record. If the SMBIOS handle is already in use,
                                EFI_ALREADY_STARTED is returned and the SMBIOS record is not updated.
  @param  Record                The data for the fixed portion of the SMBIOS record. The format of the record is
                                determined by EFI_SMBIOS_TABLE_HEADER.Type. The size of the formatted area is defined 
                                by EFI_SMBIOS_TABLE_HEADER.Length and either followed by a double-null (0x0000) or 
                                a set of null terminated strings and a null.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added due to lack of system resources.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed in was already in use.

**/
EFI_STATUS
EFIAPI
SmbiosAdd (
  IN CONST EFI_SMBIOS_PROTOCOL  *This,
  IN EFI_HANDLE                 ProducerHandle, OPTIONAL
  IN OUT EFI_SMBIOS_HANDLE      *SmbiosHandle,
  IN EFI_SMBIOS_TABLE_HEADER    *Record
  )
{
  VOID                        *Raw;
  UINTN                       TotalSize;
  UINTN                       RecordSize;
  UINTN                       StructureSize;
  UINTN                       NumberOfStrings;
  EFI_STATUS                  Status;
  LIST_ENTRY                  *Head;
  SMBIOS_INSTANCE             *Private;
  EFI_SMBIOS_ENTRY            *SmbiosEntry;
  EFI_SMBIOS_HANDLE           MaxSmbiosHandle;
  SMBIOS_HANDLE_ENTRY         *HandleEntry;
  EFI_SMBIOS_RECORD_HEADER    *InternalRecord;
  
  if (SmbiosHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  Private = SMBIOS_INSTANCE_FROM_THIS (This);
  //
  // Check whether SmbiosHandle is already in use
  //
  Head = &Private->AllocatedHandleListHead;
  if (*SmbiosHandle != SMBIOS_HANDLE_PI_RESERVED && CheckSmbiosHandleExistance(Head, *SmbiosHandle)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // when SmbiosHandle is 0xFFFE, an available handle will be assigned
  //
  if (*SmbiosHandle == SMBIOS_HANDLE_PI_RESERVED) {
    Status = GetAvailableSmbiosHandle(This, SmbiosHandle);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  } else {
    //
    // Check this handle validity
    //
    GetMaxSmbiosHandle(This, &MaxSmbiosHandle);
    if (*SmbiosHandle > MaxSmbiosHandle) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Calculate record size and string number
  //
  Status = GetSmbiosStructureSize(This, Record, &StructureSize, &NumberOfStrings);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Enter into critical section
  //  
  Status = EfiAcquireLockOrFail (&Private->DataLock);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  RecordSize  = sizeof (EFI_SMBIOS_RECORD_HEADER) + StructureSize;
  TotalSize   = sizeof (EFI_SMBIOS_ENTRY) + RecordSize;

  //
  // Allocate internal buffer
  //
  SmbiosEntry = AllocateZeroPool (TotalSize);
  if (SmbiosEntry == NULL) {
    EfiReleaseLock (&Private->DataLock);
    return EFI_OUT_OF_RESOURCES;
  }
  HandleEntry = AllocateZeroPool (sizeof(SMBIOS_HANDLE_ENTRY));
  if (HandleEntry == NULL) {
    EfiReleaseLock (&Private->DataLock);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Build Handle Entry and insert into linked list
  //
  HandleEntry->Signature     = SMBIOS_HANDLE_ENTRY_SIGNATURE;
  HandleEntry->SmbiosHandle  = *SmbiosHandle;
  InsertTailList(&Private->AllocatedHandleListHead, &HandleEntry->Link);

  InternalRecord  = (EFI_SMBIOS_RECORD_HEADER *) (SmbiosEntry + 1);
  Raw     = (VOID *) (InternalRecord + 1);

  //
  // Build internal record Header
  //
  InternalRecord->Version     = EFI_SMBIOS_RECORD_HEADER_VERSION;
  InternalRecord->HeaderSize  = (UINT16) sizeof (EFI_SMBIOS_RECORD_HEADER);
  InternalRecord->RecordSize  = RecordSize;
  InternalRecord->ProducerHandle = ProducerHandle;
  InternalRecord->NumberOfStrings = NumberOfStrings;
  //
  // Insert record into the internal linked list
  //
  SmbiosEntry->Signature    = EFI_SMBIOS_ENTRY_SIGNATURE;
  SmbiosEntry->RecordHeader = InternalRecord;
  SmbiosEntry->RecordSize   = TotalSize;
  if(PcdGetBool(PcdSortSmbiosByType)){
    ProperInsertList(&Private->DataListHead, &SmbiosEntry->Link, Record->Type);
  } else {
    InsertTailList (&Private->DataListHead, &SmbiosEntry->Link);
  }

  CopyMem (Raw, Record, StructureSize);
  ((EFI_SMBIOS_TABLE_HEADER*)Raw)->Handle = *SmbiosHandle;

  //
  // Some UEFI drivers (such as network) need some information in SMBIOS table.
  // Here we create SMBIOS table and publish it in
  // configuration table, so other UEFI drivers can get SMBIOS table from
  // configuration table without depending on PI SMBIOS protocol.
  //
  SmbiosTableConstruction ();
  
  //
  // Leave critical section
  //
  EfiReleaseLock (&Private->DataLock);
  return EFI_SUCCESS;
}

/**
  Update the string associated with an existing SMBIOS record.

  @param  This                  The EFI_SMBIOS_PROTOCOL instance.
  @param  SmbiosHandle          SMBIOS Handle of structure that will have its string updated.
  @param  StringNumber          The non-zero string number of the string to update
  @param  String                Update the StringNumber string with String.

  @retval EFI_SUCCESS           SmbiosHandle had its StringNumber String updated.
  @retval EFI_INVALID_PARAMETER SmbiosHandle does not exist.
  @retval EFI_UNSUPPORTED       String was not added because it is longer than the SMBIOS Table supports.
  @retval EFI_NOT_FOUND         The StringNumber.is not valid for this SMBIOS record.

**/
EFI_STATUS
EFIAPI
SmbiosUpdateString (
  IN CONST EFI_SMBIOS_PROTOCOL      *This,
  IN EFI_SMBIOS_HANDLE              *SmbiosHandle,
  IN UINTN                          *StringNumber,
  IN CHAR8                          *String
  )
{
  UINTN                     InputStrLen;
  UINTN                     TargetStrLen;
  UINTN                     StrIndex;
  UINTN                     TargetStrOffset;
  UINTN                     NewEntrySize;
  CHAR8                     *StrStart;
  VOID                      *Raw;
  LIST_ENTRY                *Link;
  LIST_ENTRY                *Head;
  EFI_STATUS                Status;
  SMBIOS_INSTANCE           *Private;
  EFI_SMBIOS_ENTRY          *SmbiosEntry;
  EFI_SMBIOS_ENTRY          *ResizedSmbiosEntry;
  EFI_SMBIOS_HANDLE         MaxSmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER   *Record;
  EFI_SMBIOS_RECORD_HEADER  *InternalRecord;
  
  //
  // Check args validity
  //
  GetMaxSmbiosHandle(This, &MaxSmbiosHandle);

  if (*SmbiosHandle > MaxSmbiosHandle) {
    return EFI_INVALID_PARAMETER;
  }

  if (String == NULL) {
    return EFI_ABORTED;
  }

  if (*StringNumber == 0) {
    return EFI_NOT_FOUND;
  }

  InputStrLen = AsciiStrLen(String);

  //
  // Reference SMBIOS 2.7, chapter 6.1.3, it will have no limit on the length of each individual text string
  //
  if (This->MajorVersion < 2 || (This->MajorVersion == 2 && This->MinorVersion < 7)) {
    if (InputStrLen > SMBIOS_STRING_MAX_LENGTH) {
      return EFI_UNSUPPORTED;
    }
  }

  Private = SMBIOS_INSTANCE_FROM_THIS (This);
  //
  // Enter into critical section
  //  
  Status = EfiAcquireLockOrFail (&Private->DataLock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Head = &Private->DataListHead;
  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    SmbiosEntry = SMBIOS_ENTRY_FROM_LINK(Link);
    Record = (EFI_SMBIOS_TABLE_HEADER*)(SmbiosEntry->RecordHeader + 1);

    if (Record->Handle == *SmbiosHandle) {
      //
      // Find out the specified SMBIOS record
      //
      if (*StringNumber > SmbiosEntry->RecordHeader->NumberOfStrings) {
        EfiReleaseLock (&Private->DataLock);
        return EFI_NOT_FOUND;
      }
      //
      // Point to unformed string section
      //
      StrStart = (CHAR8 *) Record + Record->Length;
     
      for (StrIndex = 1, TargetStrOffset = 0; StrIndex < *StringNumber; StrStart++, TargetStrOffset++) {
        //
        // A string ends in 00h
        //
        if (*StrStart == 0) {
          StrIndex++;
        }
        
        //
        // String section ends in double-null (0000h)
        //
        if (*StrStart == 0 && *(StrStart + 1) == 0) {
          EfiReleaseLock (&Private->DataLock);
          return EFI_NOT_FOUND;
        } 
      }

      if (*StrStart == 0) {
        StrStart++;
        TargetStrOffset++;
      }
      
      //
      // Now we get the string target
      //
      TargetStrLen = AsciiStrLen(StrStart);
      if (InputStrLen == TargetStrLen) {
        AsciiStrCpy(StrStart, String);
        //
        // Some UEFI drivers (such as network) need some information in SMBIOS table.
        // Here we create SMBIOS table and publish it in
        // configuration table, so other UEFI drivers can get SMBIOS table from
        // configuration table without depending on PI SMBIOS protocol.
        //
        SmbiosTableConstruction ();
        EfiReleaseLock (&Private->DataLock);
        return EFI_SUCCESS;
      }

      //
      // Original string buffer size is not exactly match input string length.
      // Re-allocate buffer is needed.
      //
      NewEntrySize = SmbiosEntry->RecordSize + InputStrLen - TargetStrLen;
      ResizedSmbiosEntry = AllocateZeroPool (NewEntrySize);

      if (ResizedSmbiosEntry == NULL) {
        EfiReleaseLock (&Private->DataLock);
        return EFI_OUT_OF_RESOURCES;
      }

      InternalRecord  = (EFI_SMBIOS_RECORD_HEADER *) (ResizedSmbiosEntry + 1);
      Raw     = (VOID *) (InternalRecord + 1);

      //
      // Build internal record Header
      //
      InternalRecord->Version     = EFI_SMBIOS_RECORD_HEADER_VERSION;
      InternalRecord->HeaderSize  = (UINT16) sizeof (EFI_SMBIOS_RECORD_HEADER);
      InternalRecord->RecordSize  = SmbiosEntry->RecordHeader->RecordSize + InputStrLen - TargetStrLen;
      InternalRecord->ProducerHandle = SmbiosEntry->RecordHeader->ProducerHandle;
      InternalRecord->NumberOfStrings = SmbiosEntry->RecordHeader->NumberOfStrings;

      //
      // Copy SMBIOS structure and optional strings.
      //
      CopyMem (Raw, SmbiosEntry->RecordHeader + 1, Record->Length + TargetStrOffset);
      CopyMem ((VOID*)((UINTN)Raw + Record->Length + TargetStrOffset), String, InputStrLen + 1);
      CopyMem ((CHAR8*)((UINTN)Raw + Record->Length + TargetStrOffset + InputStrLen + 1),
               (CHAR8*)Record + Record->Length + TargetStrOffset + TargetStrLen + 1,
               SmbiosEntry->RecordHeader->RecordSize - sizeof (EFI_SMBIOS_RECORD_HEADER) - Record->Length - TargetStrOffset - TargetStrLen - 1);

      //
      // Insert new record
      //
      ResizedSmbiosEntry->Signature    = EFI_SMBIOS_ENTRY_SIGNATURE;
      ResizedSmbiosEntry->RecordHeader = InternalRecord;
      ResizedSmbiosEntry->RecordSize   = NewEntrySize;
      InsertTailList (Link->ForwardLink, &ResizedSmbiosEntry->Link);

      //
      // Remove old record
      //
      RemoveEntryList(Link);
      FreePool(SmbiosEntry);
      //
      // Some UEFI drivers (such as network) need some information in SMBIOS table.
      // Here we create SMBIOS table and publish it in
      // configuration table, so other UEFI drivers can get SMBIOS table from
      // configuration table without depending on PI SMBIOS protocol.
      //
      SmbiosTableConstruction ();
      EfiReleaseLock (&Private->DataLock);
      return EFI_SUCCESS;
    }
  }

  EfiReleaseLock (&Private->DataLock);
  return EFI_INVALID_PARAMETER;
}

/**
  Remove an SMBIOS record.

  @param  This                  The EFI_SMBIOS_PROTOCOL instance.
  @param  SmbiosHandle          The handle of the SMBIOS record to remove.

  @retval EFI_SUCCESS           SMBIOS record was removed.
  @retval EFI_INVALID_PARAMETER SmbiosHandle does not specify a valid SMBIOS record.

**/
EFI_STATUS
EFIAPI
SmbiosRemove (
  IN CONST EFI_SMBIOS_PROTOCOL   *This,
  IN EFI_SMBIOS_HANDLE           SmbiosHandle
  )
{
  LIST_ENTRY                 *Link;
  LIST_ENTRY                 *Head;
  EFI_STATUS                 Status;
  EFI_SMBIOS_HANDLE          MaxSmbiosHandle;
  SMBIOS_INSTANCE            *Private;
  EFI_SMBIOS_ENTRY           *SmbiosEntry;
  SMBIOS_HANDLE_ENTRY        *HandleEntry;
  EFI_SMBIOS_TABLE_HEADER    *Record;

  //
  // Check args validity
  //
  GetMaxSmbiosHandle(This, &MaxSmbiosHandle);

  if (SmbiosHandle > MaxSmbiosHandle) {
    return EFI_INVALID_PARAMETER;
  }

  Private = SMBIOS_INSTANCE_FROM_THIS (This);
  //
  // Enter into critical section
  //  
  Status = EfiAcquireLockOrFail (&Private->DataLock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Head = &Private->DataListHead;
  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    SmbiosEntry = SMBIOS_ENTRY_FROM_LINK(Link);
    Record = (EFI_SMBIOS_TABLE_HEADER*)(SmbiosEntry->RecordHeader + 1);
    if (Record->Handle == SmbiosHandle) {
      //
      // Remove specified smobios record from DataList
      //
      RemoveEntryList(Link);
      FreePool(SmbiosEntry);
      // 
      // Remove this handle from AllocatedHandleList
      //
      Head = &Private->AllocatedHandleListHead;
      for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
        HandleEntry = SMBIOS_HANDLE_ENTRY_FROM_LINK(Link);
        if (HandleEntry->SmbiosHandle == SmbiosHandle) {
          RemoveEntryList(Link);
          FreePool(HandleEntry);
          break;
        }
      }
      //
      // Some UEFI drivers (such as network) need some information in SMBIOS table.
      // Here we create SMBIOS table and publish it in
      // configuration table, so other UEFI drivers can get SMBIOS table from
      // configuration table without depending on PI SMBIOS protocol.
      //
      SmbiosTableConstruction ();
      EfiReleaseLock (&Private->DataLock);
      return EFI_SUCCESS;
    }
  }

  //
  // Leave critical section
  //
  EfiReleaseLock (&Private->DataLock);
  return EFI_INVALID_PARAMETER;
  
}

/**
  Allow the caller to discover all or some of the SMBIOS records.

  @param  This                  The EFI_SMBIOS_PROTOCOL instance.
  @param  SmbiosHandle          On entry, points to the previous handle of the SMBIOS record. On exit, points to the
                                next SMBIOS record handle. If it is FFFEh on entry, then the first SMBIOS record
                                handle will be returned. If it returns FFFEh on exit, then there are no more SMBIOS records.
  @param  Type                  On entry it means return the next SMBIOS record of type Type. If a NULL is passed in 
                                this functionally it ignored. Type is not modified by the GetNext() function.
  @param  Record                On exit, points to the SMBIOS Record consisting of the formatted area followed by
                                the unformatted area. The unformatted area optionally contains text strings.
  @param  ProducerHandle        On exit, points to the ProducerHandle registered by Add(). If no ProducerHandle was passed into Add() NULL is returned. 
                                If a NULL pointer is passed in no data will be returned 
                                
  @retval EFI_SUCCESS           SMBIOS record information was successfully returned in Record.
  @retval EFI_NOT_FOUND         The SMBIOS record with SmbiosHandle was the last available record.

**/
EFI_STATUS
EFIAPI
SmbiosGetNext (
  IN CONST EFI_SMBIOS_PROTOCOL      *This,
  IN OUT EFI_SMBIOS_HANDLE          *SmbiosHandle,
  IN EFI_SMBIOS_TYPE                *Type,          OPTIONAL
  OUT EFI_SMBIOS_TABLE_HEADER       **Record,
  OUT EFI_HANDLE                    *ProducerHandle OPTIONAL
  )
{
  BOOLEAN                  StartPointFound;
  LIST_ENTRY               *Link;
  LIST_ENTRY               *Head;
  SMBIOS_INSTANCE          *Private;
  EFI_SMBIOS_ENTRY         *SmbiosEntry;
  EFI_SMBIOS_TABLE_HEADER  *SmbiosTableHeader;

  if (SmbiosHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StartPointFound = FALSE;
  Private = SMBIOS_INSTANCE_FROM_THIS (This);
  Head = &Private->DataListHead;
  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    SmbiosEntry = SMBIOS_ENTRY_FROM_LINK(Link);
    SmbiosTableHeader = (EFI_SMBIOS_TABLE_HEADER*)(SmbiosEntry->RecordHeader + 1); 

    //
    // If SmbiosHandle is 0xFFFE, the first matched SMBIOS record handle will be returned
    //
    if (*SmbiosHandle == SMBIOS_HANDLE_PI_RESERVED) {
      if ((Type != NULL) && (*Type != SmbiosTableHeader->Type)) {
        continue;  
      }

      *SmbiosHandle = SmbiosTableHeader->Handle;
      *Record =SmbiosTableHeader;
      if (ProducerHandle != NULL) {
        *ProducerHandle = SmbiosEntry->RecordHeader->ProducerHandle;
      }
      return EFI_SUCCESS;
    }

    //
    // Start this round search from the next SMBIOS handle
    //
    if (!StartPointFound && (*SmbiosHandle == SmbiosTableHeader->Handle)) {
      StartPointFound = TRUE;
      continue;
    }

    if (StartPointFound) {
      if ((Type != NULL) && (*Type != SmbiosTableHeader->Type)) {
        continue; 
      }
      
      *SmbiosHandle = SmbiosTableHeader->Handle;
      *Record = SmbiosTableHeader; 
      if (ProducerHandle != NULL) {
        *ProducerHandle = SmbiosEntry->RecordHeader->ProducerHandle;
      }

      return EFI_SUCCESS;   
    }
  }

  *SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  return EFI_NOT_FOUND;
  
}

/**
  Allow the caller to discover all of the SMBIOS records.

  @param  This                  The EFI_SMBIOS_PROTOCOL instance.
  @param  CurrentSmbiosEntry    On exit, points to the SMBIOS entry on the list which includes the returned SMBIOS record information. 
                                If *CurrentSmbiosEntry is NULL on entry, then the first SMBIOS entry on the list will be returned. 
  @param  Record                On exit, points to the SMBIOS Record consisting of the formatted area followed by
                                the unformatted area. The unformatted area optionally contains text strings.
                                
  @retval EFI_SUCCESS           SMBIOS record information was successfully returned in Record.
                                *CurrentSmbiosEntry points to the SMBIOS entry which includes the returned SMBIOS record information.
  @retval EFI_NOT_FOUND         There is no more SMBIOS entry.

**/
EFI_STATUS
EFIAPI
GetNextSmbiosRecord (
  IN CONST EFI_SMBIOS_PROTOCOL         *This,
  IN OUT EFI_SMBIOS_ENTRY              **CurrentSmbiosEntry,
  OUT EFI_SMBIOS_TABLE_HEADER          **Record
  )
{
  LIST_ENTRY               *Link;
  LIST_ENTRY               *Head;
  SMBIOS_INSTANCE          *Private;
  EFI_SMBIOS_ENTRY         *SmbiosEntry;
  EFI_SMBIOS_TABLE_HEADER  *SmbiosTableHeader;

  Private = SMBIOS_INSTANCE_FROM_THIS (This);
  if (*CurrentSmbiosEntry == NULL) {
    //
    // Get the beginning of SMBIOS entry.
    //
    Head = &Private->DataListHead;
  } else {
    //
    // Get previous SMBIOS entry and make it as start point.
    //
    Head = &(*CurrentSmbiosEntry)->Link;
  }
  
  Link  = Head->ForwardLink;
  
  if (Link == &Private->DataListHead) {
    //
    // If no more SMBIOS entry in the list, return not found.
    //
    return EFI_NOT_FOUND;
  }
  
  SmbiosEntry = SMBIOS_ENTRY_FROM_LINK(Link);
  SmbiosTableHeader = (EFI_SMBIOS_TABLE_HEADER*)(SmbiosEntry->RecordHeader + 1);
  *Record = SmbiosTableHeader; 
  *CurrentSmbiosEntry = SmbiosEntry;
  return EFI_SUCCESS;   
}

/**
  Assembles SMBIOS table from the SMBIOS protocol. Produce Table
  Entry Point and return the pointer to it.
  
  @param  TableEntryPointStructure   On exit, points to the SMBIOS entrypoint structure.
                                
  @retval EFI_SUCCESS                Structure created sucessfully.
  @retval EFI_NOT_READY              Some of The SMBIOS records was not available yet.
  @retval EFI_OUT_OF_RESOURCES       No enough memory.
  
**/
EFI_STATUS
EFIAPI
SmbiosCreateTable (
  OUT VOID    **TableEntryPointStructure
  )
{
  UINT8                           *BufferPointer;
  UINTN                           RecordSize;
  UINTN                           NumOfStr;
  EFI_STATUS                      Status;
  EFI_SMBIOS_HANDLE               SmbiosHandle;
  EFI_SMBIOS_PROTOCOL             *SmbiosProtocol;
  EFI_PHYSICAL_ADDRESS            PhysicalAddress;
  EFI_SMBIOS_TABLE_HEADER         *SmbiosRecord;
  EFI_SMBIOS_TABLE_END_STRUCTURE  EndStructure;
  EFI_SMBIOS_ENTRY                *CurrentSmbiosEntry;
  UINTN                           PreAllocatedPages;
  
  Status            = EFI_SUCCESS;
  BufferPointer     = NULL;

  //
  // Get Smbios protocol to traverse SMBIOS records.
  //
  SmbiosProtocol = &mPrivateData.Smbios;

  if (EntryPointStructure->TableAddress == 0) {
    PreAllocatedPages = 0;
  } else {
    PreAllocatedPages = EFI_SIZE_TO_PAGES (EntryPointStructure->TableLength);
  }

  if (mAllDriversConnected) {
    //
    // Add or delete some records from table according saving records
    //
    UpdateRecordToSmbiosTable (SmbiosProtocol);
  }

  //
  // Make some statistics about all the structures
  //
  EntryPointStructure->NumberOfSmbiosStructures = 0;
  EntryPointStructure->TableLength              = 0;
  EntryPointStructure->MaxStructureSize         = 0;

  //
  // Calculate EPS Table Length
  //
  CurrentSmbiosEntry = NULL;
  do {
    Status = GetNextSmbiosRecord (SmbiosProtocol, &CurrentSmbiosEntry, &SmbiosRecord);
                               
    if (Status == EFI_SUCCESS) {
      if (mAllDriversConnected) {
        //
        // update new strings for some records according saving records
        //
        if (SmbiosCheckRecord (SmbiosRecord)) {
          Status = SmbiosUpdateStructureWithRecord (&SmbiosRecord);
          //ASSERT_EFI_ERROR (Status);
        }
      }
      GetSmbiosStructureSize(SmbiosProtocol, SmbiosRecord, &RecordSize, &NumOfStr);
      //
      // Record NumberOfSmbiosStructures, TableLength and MaxStructureSize
      //
      EntryPointStructure->NumberOfSmbiosStructures++;
      EntryPointStructure->TableLength = (UINT16) (EntryPointStructure->TableLength + RecordSize);
      if (RecordSize > EntryPointStructure->MaxStructureSize) {
        EntryPointStructure->MaxStructureSize = (UINT16) RecordSize;
      }
    }
  } while (!EFI_ERROR(Status));
  if (mAllDriversConnected) {
    mAllDriversConnected = FALSE;
  }
  //
  // Create End-Of-Table structure
  //
  GetMaxSmbiosHandle(SmbiosProtocol, &SmbiosHandle);
  EndStructure.Header.Type = EFI_SMBIOS_TYPE_END_OF_TABLE;
  EndStructure.Header.Length = (UINT8) sizeof (EFI_SMBIOS_TABLE_HEADER);
  EndStructure.Header.Handle = SmbiosHandle;
  EndStructure.Tailing[0] = 0;
  EndStructure.Tailing[1] = 0;
  EntryPointStructure->NumberOfSmbiosStructures++;
  EntryPointStructure->TableLength = (UINT16) (EntryPointStructure->TableLength + sizeof (EndStructure));
  if (sizeof (EndStructure) > EntryPointStructure->MaxStructureSize) {
    EntryPointStructure->MaxStructureSize = (UINT16) sizeof (EndStructure);
  }

  if ((UINTN) EFI_SIZE_TO_PAGES (EntryPointStructure->TableLength) > PreAllocatedPages) {
    //
    // If new SMBIOS talbe size exceeds the original pre-allocated page, 
    // it is time to re-allocate memory (below 4GB).
    // 
    if (EntryPointStructure->TableAddress != 0) {
      //
      // Free the original pre-allocated page
      //      
      FreePages (
            (VOID*)(UINTN)EntryPointStructure->TableAddress,
            PreAllocatedPages
            );
      EntryPointStructure->TableAddress = 0;
    }
    
    PhysicalAddress = 0xffffffff;
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiReservedMemoryType,
                    EFI_SIZE_TO_PAGES (EntryPointStructure->TableLength),
                    &PhysicalAddress
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "SmbiosCreateTable() could not allocate SMBIOS table < 4GB\n"));
      EntryPointStructure->TableAddress = 0;
      return EFI_OUT_OF_RESOURCES;
    } else {
      EntryPointStructure->TableAddress = (UINT32) PhysicalAddress;
      DEBUG((EFI_D_ERROR, "SMBIOS_TBL:%lX,%X\n", PhysicalAddress, EFI_SIZE_TO_PAGES(EntryPointStructure->TableLength))); 			
    }
  }
  
  //
  // Assemble the tables
  //
  ASSERT (EntryPointStructure->TableAddress != 0);
  BufferPointer = (UINT8 *) (UINTN) EntryPointStructure->TableAddress;
  CurrentSmbiosEntry = NULL;
  do {
    Status = GetNextSmbiosRecord (SmbiosProtocol, &CurrentSmbiosEntry, &SmbiosRecord);

    if (Status == EFI_SUCCESS) {
      GetSmbiosStructureSize(SmbiosProtocol, SmbiosRecord, &RecordSize, &NumOfStr);
      CopyMem (BufferPointer, SmbiosRecord, RecordSize);
      BufferPointer = BufferPointer + RecordSize;
    }
  } while (!EFI_ERROR(Status));
  
  //
  // Assemble End-Of-Table structure
  //
  CopyMem (BufferPointer, &EndStructure, sizeof (EndStructure));

  //
  // Fixup checksums in the Entry Point Structure
  //
  EntryPointStructure->IntermediateChecksum = 0;
  EntryPointStructure->EntryPointStructureChecksum = 0;

  EntryPointStructure->IntermediateChecksum =
    CalculateCheckSum8 ((UINT8 *) EntryPointStructure + 0x10, EntryPointStructure->EntryPointLength - 0x10);
  EntryPointStructure->EntryPointStructureChecksum =
    CalculateCheckSum8 ((UINT8 *) EntryPointStructure, EntryPointStructure->EntryPointLength);

  //
  // Returns the pointer
  //
  *TableEntryPointStructure = EntryPointStructure;

  return EFI_SUCCESS;
}

/**
  Create SMBIOS Table and install it to the System Table.
**/
VOID
EFIAPI
SmbiosTableConstruction (
  VOID
  )
{
  UINT8       *Eps;
  EFI_STATUS  Status;

  Status = SmbiosCreateTable ((VOID **) &Eps);
  if (!EFI_ERROR (Status)) {
    gBS->InstallConfigurationTable (&gEfiSmbiosTableGuid, Eps);
  }
}

EFI_STATUS
UpdateSmbiosString ()
{
  UINT8                           *RecordPtr;
  UINT8                           *RecordEndPtr;
  PNP_52_DATA_BUFFER              *DataBufferInRec;
  EFI_STATUS                      Status;
  UINTN                           StringNumber;
  EFI_SMBIOS_HANDLE               SmbiosHandle;
  UINT8                           Type;
  CHAR8                           *String;
  EFI_SMBIOS_PROTOCOL             *Smbios;
  EFI_SMBIOS_TABLE_HEADER         *SmbiosRecord;

  RecordPtr    = (UINT8 *)(UINTN)mSmbiosRecBase;
  RecordEndPtr = (UINT8 *)(UINTN)(mSmbiosRecBase + SMBIOS_REC_SIZE);
  Smbios       = &mPrivateData.Smbios;

  while (RecordPtr < RecordEndPtr) {
    if (((SMBIOS_REC_HEADER *)RecordPtr)->Signature != SMBIOS_REC_SIGNATURE ||
         ((SMBIOS_REC_HEADER *)RecordPtr)->RecordLen == 0){
      break;
    }  

    DataBufferInRec = (PNP_52_DATA_BUFFER *)(RecordPtr + sizeof (SMBIOS_REC_HEADER));
    if (DataBufferInRec->Command == StringChanged) {
      SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
      Type         = DataBufferInRec->StructureHeader.Type;
      Status       = Smbios->GetNext (Smbios, &SmbiosHandle, &Type, &SmbiosRecord, NULL);
      if (EFI_ERROR(Status)) {
        DEBUG ((DEBUG_ERROR, "Smbios->GetNext(%d):%r\n", Type, Status));
        goto TryNext;
      }
      StringNumber = *(((UINT8 *)SmbiosRecord) + DataBufferInRec->FieldOffset);
      String       = (CHAR8 *)DataBufferInRec->StructureData;
      DEBUG ((DEBUG_INFO, "SmbiosHandle:%x,type:%x\n",SmbiosHandle,DataBufferInRec->StructureHeader.Type));
      DEBUG ((DEBUG_INFO, "StringNumber:%x\n",StringNumber));
      DEBUG ((DEBUG_INFO, "String:%a\n",String));
      Status = SmbiosUpdateString (
                 Smbios,
                 &SmbiosHandle,
                 &StringNumber,
                 String
                 );
      DEBUG ((DEBUG_INFO, "SmbiosUpdateString:%r\n",Status));
      if (EFI_ERROR (Status)) {
        goto TryNext;
      }
    }
    
TryNext:    
    RecordPtr = RecordPtr + ((SMBIOS_REC_HEADER *)RecordPtr)->RecordLen;
  }

   return EFI_SUCCESS;
}

VOID
EFIAPI
AllDriversConnectedCallBack (
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
  VOID        *Interface;
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol(&gByoAllDriversConnectedProtocolGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }
  gBS->CloseEvent(Event);
  mAllDriversConnected = TRUE;
  SmbiosTableConstruction ();
  //
  // update smbios string which string was changed
  //
  UpdateSmbiosString ();
}

/**

  Driver to produce Smbios protocol and pre-allocate 1 page for the final SMBIOS table.

  @param ImageHandle     Module's image handle
  @param SystemTable     Pointer of EFI_SYSTEM_TABLE

  @retval EFI_SUCCESS    Smbios protocol installed
  @retval Other          No protocol installed, unload driver.

**/
EFI_STATUS
EFIAPI
SmbiosDriverEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;
  VOID                  *EventReg;

  mPrivateData.Signature                = SMBIOS_INSTANCE_SIGNATURE;
  mPrivateData.Smbios.Add               = SmbiosAdd;
  mPrivateData.Smbios.UpdateString      = SmbiosUpdateString;
  mPrivateData.Smbios.Remove            = SmbiosRemove;
  mPrivateData.Smbios.GetNext           = SmbiosGetNext;
  mPrivateData.Smbios.MajorVersion      = (UINT8) (FixedPcdGet16 (PcdSmbiosVersion) >> 8);
  mPrivateData.Smbios.MinorVersion      = (UINT8) (FixedPcdGet16 (PcdSmbiosVersion) & 0x00ff);

  InitializeListHead (&mPrivateData.DataListHead);
  InitializeListHead (&mPrivateData.AllocatedHandleListHead);
  EfiInitializeLock (&mPrivateData.DataLock, TPL_NOTIFY);

  //
  // Initialize the EntryPointStructure with initial values.
  // Allocate memory (below 4GB).
  //
  PhysicalAddress = 0xffffffff;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiReservedMemoryType,
                  EFI_SIZE_TO_PAGES (sizeof (SMBIOS_TABLE_ENTRY_POINT)),
                  &PhysicalAddress
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SmbiosDriverEntryPoint() could not allocate EntryPointStructure < 4GB\n"));
    Status = gBS->AllocatePages (
                    AllocateAnyPages,
                    EfiReservedMemoryType,
                    EFI_SIZE_TO_PAGES (sizeof (SMBIOS_TABLE_ENTRY_POINT)),
                    &PhysicalAddress
                    );
   if (EFI_ERROR (Status)) {   
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    DEBUG((EFI_D_ERROR, "SMBIOS_EP:%lX,%X\n", PhysicalAddress, EFI_SIZE_TO_PAGES(sizeof(SMBIOS_TABLE_ENTRY_POINT))));    
  }	

  EntryPointStructure = (SMBIOS_TABLE_ENTRY_POINT *) (UINTN) PhysicalAddress;
  
  CopyMem (
    EntryPointStructure,
    &EntryPointStructureData,
    sizeof (SMBIOS_TABLE_ENTRY_POINT)
    );

  //
  // Pre-allocate 1 page for SMBIOS table below 4GB.
  // SMBIOS table will be updated when new SMBIOS type is added or 
  // existing SMBIOS type is updated. If the total size of SMBIOS table exceeds 1 page,
  // we will re-allocate new memory when creating whole SMBIOS table.
  //
  PhysicalAddress = 0xffffffff;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiReservedMemoryType,
                  1,
                  &PhysicalAddress
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SmbiosDriverEntryPoint() could not allocate SMBIOS table < 4GB\n"));
    EntryPointStructure->TableAddress = 0;
    EntryPointStructure->TableLength  = 0;
  } else {
    EntryPointStructure->TableAddress = (UINT32) PhysicalAddress;
    EntryPointStructure->TableLength  = EFI_PAGES_TO_SIZE (1);
    DEBUG((EFI_D_ERROR, "SMBIOS_TBL:%lX,1\n", PhysicalAddress)); 		
  }
  
  //
  // Make a new handle and install the protocol
  //
  mPrivateData.Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &mPrivateData.Handle,
                  &gEfiSmbiosProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPrivateData.Smbios
                  );
  if (mSmbiosRecBase != 0 && SMBIOS_REC_SIZE != 0) {
    //
    // Register the event to install SMBIOS Table into EFI System Table
    //
    EfiCreateProtocolNotifyEvent (
      &gByoAllDriversConnectedProtocolGuid,
      TPL_CALLBACK,
      AllDriversConnectedCallBack,
      NULL,
      &EventReg
      ); 
  }
  return Status;
}
