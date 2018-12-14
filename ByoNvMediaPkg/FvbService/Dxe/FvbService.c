/** @file
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2017 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "FvbService.h"

NV_MEDIA_ACCESS_PROTOCOL *pMediaAccessProtocol;

//
// Global variable for this FVB driver  which contains
// the private data of all firmware volume block instances
//
FWB_GLOBAL   mFvbModuleGlobal;

//
// This platform driver knows there are 3 FVs on
// FD, which are FvRecovery, FvMain and FvNvStorage.
//
UINT32 mPlatformFvBaseAddress[] = {
    FixedPcdGet32(PcdFlashNvStorageBase),
};

FV_MEMMAP_DEVICE_PATH mFvMemmapDevicePathTemplate = {
    {
        {
            HARDWARE_DEVICE_PATH,
            HW_MEMMAP_DP,
            {
                (UINT8)(sizeof (MEMMAP_DEVICE_PATH)),
                (UINT8)(sizeof (MEMMAP_DEVICE_PATH) >> 8)
            }
        },
        EfiMemoryMappedIO,
        (EFI_PHYSICAL_ADDRESS) 0,
        (EFI_PHYSICAL_ADDRESS) 0,
    },
    {
        END_DEVICE_PATH_TYPE,
        END_ENTIRE_DEVICE_PATH_SUBTYPE,
        {
            END_DEVICE_PATH_LENGTH,
            0
        }
    }
};

FV_PIWG_DEVICE_PATH mFvPIWGDevicePathTemplate = {
    {
        {
            MEDIA_DEVICE_PATH,
            MEDIA_PIWG_FW_VOL_DP,
            {
                (UINT8)(sizeof (MEDIA_FW_VOL_DEVICE_PATH)),
                (UINT8)(sizeof (MEDIA_FW_VOL_DEVICE_PATH) >> 8)
            }
        },
        { 0 }
    },
    {
        END_DEVICE_PATH_TYPE,
        END_ENTIRE_DEVICE_PATH_SUBTYPE,
        {
            END_DEVICE_PATH_LENGTH,
            0
        }
    }
};

//
// Template structure used when installing FVB protocol
//
EFI_FW_VOL_BLOCK_DEVICE mFvbDeviceTemplate = {
    FVB_DEVICE_SIGNATURE,
    NULL,
    0, // Instance
    {
        FvbProtocolGetAttributes,
        FvbProtocolSetAttributes,
        FvbProtocolGetPhysicalAddress,
        FvbProtocolGetBlockSize,
        FvbProtocolRead,
        FvbProtocolWrite,
        FvbProtocolEraseBlocks,
        NULL
    } // FwVolBlockInstance
};

/**
  Get the pointer to EFI_FW_VOL_INSTANCE from the buffer pointed
  by mFvbModuleGlobal.FvInstance based on a index.
  Each EFI_FW_VOL_INSTANCE is  with variable length as
  we have a block map at the end of the EFI_FIRMWARE_VOLUME_HEADER.

  @param[in] Instance The index of the EFI_FW_VOL_INSTANCE.

  @return A pointer to EFI_FW_VOL_INSTANCE.

**/
STATIC
EFI_FW_VOL_INSTANCE *
GetFvbInstance (
    IN  UINTN             Instance
)
{
    EFI_FW_VOL_INSTANCE   *FwhRecord;

    if ( Instance >= mFvbModuleGlobal.NumFv ) {
        ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
        return NULL;
    }

    //
    // Find the right instance of the FVB private data
    //
    FwhRecord = mFvbModuleGlobal.FvInstance;
    while ( Instance > 0 ) {
        FwhRecord = (EFI_FW_VOL_INSTANCE *) ((UINTN)((UINT8 *)FwhRecord) +
                                             FwhRecord->VolumeHeader.HeaderLength +
                                             (sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER)));
        Instance --;
    }

    return FwhRecord;

}



/**
  Call back function on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.

  Fixup internal data so that the driver is callable in EFI  runtime
  in virtual mode. Convert the mFvbModuleGlobal date items to there
  virtual address.

  @param  Event     Event whose notification function is being invoked.
  @param  Context   The context of the Notification context. Not used in
                    this call back function.

**/
VOID
EFIAPI
FvbVirtualddressChangeEvent (
    IN EFI_EVENT        Event,
    IN VOID             *Context
)
{
    EFI_FW_VOL_INSTANCE       *FwhInstance;
    UINTN                     Index;

    //
    // Convert the base address of all the instances
    //
    for (Index = 0; Index < mFvbModuleGlobal.NumFv; Index++) {
        FwhInstance = GetFvbInstance (Index);
        EfiConvertPointer (0, (VOID **) &FwhInstance->FvBase);
    }

    EfiConvertPointer (0, (VOID **) &mFvbModuleGlobal.FvInstance);
}


/**
  Get the EFI_FVB_ATTRIBUTES_2 of a FV.

  @param[in]  The index of the EFI_FW_VOL_INSTANCE.

  @return     EFI_FVB_ATTRIBUTES_2 of the FV identified by Instance.

**/
STATIC
EFI_FVB_ATTRIBUTES_2
FvbGetVolumeAttributes (
    IN UINTN                                Instance
)
{
    return GetFvbInstance(Instance)->VolumeHeader.Attributes;
}



/**
  Retrieves the starting address of an LBA in an FV. It also
  return a few other attribut of the FV.

  @param[in]  Instance        The index of the EFI_FW_VOL_INSTANCE.
  @param[in]  Lba             The logical block address
  @param[out] LbaAddress      On output, contains the physical starting address
                              of the Lba
  @param[out] LbaLength       On output, contains the length of the block
  @param[out] NumOfBlocks     A pointer to a caller allocated UINTN in which the
                              number of consecutive blocks starting with Lba is
                              returned. All blocks in this range have a size of
                              BlockSize

  @retval   EFI_SUCCESS Successfully returns
  @retval   EFI_INVALID_PARAMETER Instance not found

**/
STATIC
EFI_STATUS
FvbGetLbaAddress (
    IN  UINTN                               Instance,
    IN  EFI_LBA                             Lba,
    OUT UINTN                               *LbaAddress,
    OUT UINTN                               *LbaLength,
    OUT UINTN                               *NumOfBlocks
)
{
    UINT32                                  NumBlocks;
    UINT32                                  BlockLength;
    UINTN                                   Offset;
    EFI_LBA                                 StartLba;
    EFI_LBA                                 NextLba;
    EFI_FW_VOL_INSTANCE                     *FwhInstance;
    EFI_FV_BLOCK_MAP_ENTRY                  *BlockMap;

    //
    // Find the right instance of the FVB private data
    //
    FwhInstance = GetFvbInstance (Instance);

    StartLba  = 0;
    Offset    = 0;
    BlockMap  = &(FwhInstance->VolumeHeader.BlockMap[0]);

    //
    // Parse the blockmap of the FV to find which map entry the Lba belongs to
    //
    while (TRUE) {
        NumBlocks   = BlockMap->NumBlocks;
        BlockLength = BlockMap->Length;

        if ( NumBlocks == 0 || BlockLength == 0) {
            return EFI_INVALID_PARAMETER;
        }

        NextLba = StartLba + NumBlocks;

        //
        // The map entry found
        //
        if (Lba >= StartLba && Lba < NextLba) {
            Offset = Offset + (UINTN)MultU64x32((Lba - StartLba), BlockLength);
            if (LbaAddress ) {
                *LbaAddress = FwhInstance->FvBase + Offset;
            }

            if (LbaLength ) {
                *LbaLength = BlockLength;
            }

            if (NumOfBlocks ) {
                *NumOfBlocks = (UINTN)(NextLba - Lba);
            }
            return EFI_SUCCESS;
        }

        StartLba  = NextLba;
        Offset    = Offset + NumBlocks * BlockLength;
        BlockMap++;
    }
}



/**
  Reads specified number of bytes into a buffer from the specified block

  @param[in]      Instance        The FV instance to be read from
  @param[in]      Lba             The logical block address to be read from
  @param[in]      BlockOffset     Offset into the block at which to begin reading
  @param[in]      NumBytes        Pointer that on input contains the total size of
                                  the buffer. On output, it contains the total number
                                  of bytes read
  @param[in]      Buffer          Pointer to a caller allocated buffer that will be
                                  used to hold the data read


  @retval         EFI_SUCCESS         The firmware volume was read successfully and
                                      contents are in Buffer
  @retval         EFI_BAD_BUFFER_SIZE Read attempted across a LBA boundary. On output,
                                      NumBytes contains the total number of bytes returned
                                      in Buffer
  @retval         EFI_ACCESS_DENIED   The firmware volume is in the ReadDisabled state
  @retval         EFI_DEVICE_ERROR    The block device is not functioning correctly and
                                      could not be read
  @retval         EFI_INVALID_PARAMETER Instance not found, or NumBytes, Buffer are NULL

**/
STATIC
EFI_STATUS
FvbReadBlock (
    IN UINTN                                Instance,
    IN EFI_LBA                              Lba,
    IN UINTN                                BlockOffset,
    IN OUT UINTN                            *NumBytes,
    IN UINT8                                *Buffer
)
{
    EFI_FVB_ATTRIBUTES_2                    Attributes;
    UINTN                                   LbaAddress;
    UINTN                                   LbaLength;
    EFI_STATUS                              Status;

    if ( (NumBytes == NULL) || (Buffer == NULL)) {
        return (EFI_INVALID_PARAMETER);
    }
    if (*NumBytes == 0) {
        return (EFI_INVALID_PARAMETER);
    }

    Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaLength, NULL);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Attributes = FvbGetVolumeAttributes (Instance);

    if ( (Attributes & EFI_FVB2_READ_STATUS) == 0) {
        return (EFI_ACCESS_DENIED);
    }

    if (BlockOffset > LbaLength) {
        return (EFI_INVALID_PARAMETER);
    }

    if (LbaLength < ( *NumBytes + BlockOffset ) ) {
        *NumBytes = (UINT32) (LbaLength - BlockOffset);
        Status = EFI_BAD_BUFFER_SIZE;
    }

    Status = pMediaAccessProtocol->Read(pMediaAccessProtocol, (LbaAddress + BlockOffset), (void *)Buffer, NumBytes, SPI_MEDIA_TYPE);

    return Status;
}



/**
  Writes specified number of bytes from the input buffer to the block

  @param[in]  Instance          The FV instance to be written to
  @param[in]  Lba               The starting logical block index to write to
  @param[in]  BlockOffset       Offset into the block at which to begin writing
  @param[in]  NumBytes          Pointer that on input contains the total size of
                                 the buffer. On output, it contains the total number
                                 of bytes actually written
  @param[in]  Buffer            Pointer to a caller allocated buffer that contains
                                 the source for the write
  @retval     EFI_SUCCESS         The firmware volume was written successfully
  @retval     EFI_BAD_BUFFER_SIZE Write attempted across a LBA boundary. On output,
                                  NumBytes contains the total number of bytes
                                  actually written
  @retval     EFI_ACCESS_DENIED   The firmware volume is in the WriteDisabled state
  @retval     EFI_DEVICE_ERROR    The block device is not functioning correctly and
                                  could not be written
  @retval     EFI_INVALID_PARAMETER Instance not found, or NumBytes, Buffer are NULL

**/

EFI_STATUS
FvbWriteBlock (
    IN UINTN                                Instance,
    IN EFI_LBA                              Lba,
    IN UINTN                                BlockOffset,
    IN OUT UINTN                            *NumBytes,
    IN UINT8                                *Buffer
)
{
    EFI_FVB_ATTRIBUTES_2                    Attributes;
    UINTN                                   LbaAddress;
    UINTN                                   LbaLength;
    EFI_FW_VOL_INSTANCE                     *FwhInstance;
    EFI_STATUS                              Status;
    EFI_STATUS                              Status1;

    FwhInstance = GetFvbInstance (Instance);

    if ( (NumBytes == NULL) || (Buffer == NULL)) {
        return (EFI_INVALID_PARAMETER);
    }
    if (*NumBytes == 0) {
        return (EFI_INVALID_PARAMETER);
    }

    Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaLength, NULL);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    //
    // Check if the FV is write enabled
    //
    Attributes = FvbGetVolumeAttributes (Instance);
    if ( (Attributes & EFI_FVB2_WRITE_STATUS) == 0)  {
        return (EFI_ACCESS_DENIED);
    }

    //
    // Perform boundary checks and adjust NumBytes
    //
    if (BlockOffset > LbaLength) {
        return (EFI_INVALID_PARAMETER);
    }

    if ( LbaLength < ( *NumBytes + BlockOffset ) ) {
        DEBUG ((EFI_D_ERROR,
                "FvWriteBlock: Reducing Numbytes from 0x%x to 0x%x\n",
                *NumBytes,
                (UINT32)(LbaLength-BlockOffset))
              );
        *NumBytes = (UINT32) (LbaLength - BlockOffset);
        Status = EFI_BAD_BUFFER_SIZE;
    }

    Status1 = pMediaAccessProtocol->Write(pMediaAccessProtocol, (LbaAddress + BlockOffset), 	(void *)Buffer, *NumBytes, SPI_MEDIA_TYPE);

    WriteBackInvalidateDataCacheRange ((VOID *) (LbaAddress + BlockOffset), *NumBytes);

    if ( EFI_ERROR (Status1) ) {
        return Status1;
    }

    return Status;
}



/**
  Erases and initializes a firmware volume block

  @param[in]    Instance    The FV instance to be erased
  @param[in]    Lba         The logical block index to be erased

  @retval   EFI_SUCCESS       The erase request was successfully completed
  @retval   EFI_ACCESS_DENIED The firmware volume is in the WriteDisabled state
  @retval   EFI_DEVICE_ERROR  The block device is not functioning correctly and
                              could not be written. Firmware device may have been
                              partially erased
  @retval   EFI_INVALID_PARAMETER Instance not found

**/
EFI_STATUS
FvbEraseBlock (
    IN UINTN                                Instance,
    IN EFI_LBA                              Lba
)
{

    EFI_FVB_ATTRIBUTES_2                    Attributes;
    UINTN                                   LbaAddress;
    EFI_FW_VOL_INSTANCE                     *FwhInstance;
    UINTN                                   LbaLength;
    EFI_STATUS                              Status;
    UINTN             BlockCount;
    //
    // Find the right instance of the FVB private data
    //
    FwhInstance = GetFvbInstance (Instance);

    //
    // Check if the FV is write enabled
    //
    Attributes = FvbGetVolumeAttributes (Instance);

    if ( (Attributes & EFI_FVB2_WRITE_STATUS) == 0)  {
        return (EFI_ACCESS_DENIED);
    }

    //
    // Get the starting address of the block for erase.
    //
    Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaLength, NULL);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    BlockCount = LbaLength / SF_SECTOR_SIZE;
    while (BlockCount > 0) {
        Status = pMediaAccessProtocol->Erase(pMediaAccessProtocol, LbaAddress, SF_SECTOR_SIZE, SPI_MEDIA_TYPE);
        if (EFI_ERROR(Status)) {
            break;
        }
        LbaAddress += SF_SECTOR_SIZE;
        BlockCount--;
    }

    WriteBackInvalidateDataCacheRange ((VOID *) LbaAddress, LbaLength);

    return Status;
}

/**
  Modifies the current settings of the firmware volume according to the
  input parameter, and returns the new setting of the volume

  @param[in]  Instance              The FV instance whose attributes is going to be
                                    modified
  @param[in]  Attributes            On input, it is a pointer to EFI_FVB_ATTRIBUTES_2
                                    containing the desired firmware volume settings.
                                    On successful return, it contains the new settings
                                    of the firmware volume

  @retval     EFI_SUCCESS           Successfully returns
  @retval     EFI_ACCESS_DENIED     The volume setting is locked and cannot be modified
  @retval     EFI_INVALID_PARAMETER Instance not found, or The attributes requested are
                                    in conflict with the capabilities as declared in the
                                    firmware volume header

**/
STATIC
EFI_STATUS
FvbSetVolumeAttributes (
    IN UINTN                                Instance,
    IN OUT EFI_FVB_ATTRIBUTES_2             *Attributes
)
{
    EFI_FW_VOL_INSTANCE                       *FwhInstance;
    EFI_FVB_ATTRIBUTES_2                      OldAttributes;
    EFI_FVB_ATTRIBUTES_2                      *AttribPtr;
    EFI_FVB_ATTRIBUTES_2                      UnchangedAttributes;
    UINT32                                    Capabilities;
    UINT32                                    OldStatus, NewStatus;

    //
    // Find the right instance of the FVB private data
    //
    FwhInstance = GetFvbInstance (Instance);

    AttribPtr     = (EFI_FVB_ATTRIBUTES_2 *) &(FwhInstance->VolumeHeader.Attributes);
    OldAttributes = *AttribPtr;
    Capabilities  = OldAttributes & EFI_FVB2_CAPABILITIES;
    OldStatus     = OldAttributes & EFI_FVB2_STATUS;
    NewStatus     = *Attributes & EFI_FVB2_STATUS;

    UnchangedAttributes = EFI_FVB2_READ_DISABLED_CAP  | \
                          EFI_FVB2_READ_ENABLED_CAP   | \
                          EFI_FVB2_WRITE_DISABLED_CAP | \
                          EFI_FVB2_WRITE_ENABLED_CAP  | \
                          EFI_FVB2_LOCK_CAP           | \
                          EFI_FVB2_STICKY_WRITE       | \
                          EFI_FVB2_MEMORY_MAPPED      | \
                          EFI_FVB2_ERASE_POLARITY     | \
                          EFI_FVB2_READ_LOCK_CAP      | \
                          EFI_FVB2_WRITE_LOCK_CAP     | \
                          EFI_FVB2_ALIGNMENT;

    //
    // Some attributes of FV is read only can *not* be set
    //
    if ((OldAttributes & UnchangedAttributes) ^ (*Attributes & UnchangedAttributes)) {
        return EFI_INVALID_PARAMETER;
    }

    //
    // If firmware volume is locked, no status bit can be updated
    //
    if ( OldAttributes & EFI_FVB2_LOCK_STATUS ) {
        if ( OldStatus ^ NewStatus ) {
            return EFI_ACCESS_DENIED;
        }
    }

    //
    // Test read disable
    //
    if ((Capabilities & EFI_FVB2_READ_DISABLED_CAP) == 0) {
        if ((NewStatus & EFI_FVB2_READ_STATUS) == 0) {
            return EFI_INVALID_PARAMETER;
        }
    }

    //
    // Test read enable
    //
    if ((Capabilities & EFI_FVB2_READ_ENABLED_CAP) == 0) {
        if (NewStatus & EFI_FVB2_READ_STATUS) {
            return EFI_INVALID_PARAMETER;
        }
    }

    //
    // Test write disable
    //
    if ((Capabilities & EFI_FVB2_WRITE_DISABLED_CAP) == 0) {
        if ((NewStatus & EFI_FVB2_WRITE_STATUS) == 0) {
            return EFI_INVALID_PARAMETER;
        }
    }

    //
    // Test write enable
    //
    if ((Capabilities & EFI_FVB2_WRITE_ENABLED_CAP) == 0) {
        if (NewStatus & EFI_FVB2_WRITE_STATUS) {
            return EFI_INVALID_PARAMETER;
        }
    }

    //
    // Test lock
    //
    if ((Capabilities & EFI_FVB2_LOCK_CAP) == 0) {
        if (NewStatus & EFI_FVB2_LOCK_STATUS) {
            return EFI_INVALID_PARAMETER;
        }
    }

    *AttribPtr  = (*AttribPtr) & (0xFFFFFFFF & (~EFI_FVB2_STATUS));
    *AttribPtr  = (*AttribPtr) | NewStatus;
    *Attributes = *AttribPtr;

    return EFI_SUCCESS;
}



//
// FVB protocol APIs
//

/**
  Retrieves the physical address of the device.

  @param[in]  This    A pointer to EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL.
  @param[out] Address Output buffer containing the address.

  retval      EFI_SUCCESS The function always return successfully.

**/
EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
    OUT EFI_PHYSICAL_ADDRESS               *Address
)
{
    EFI_FW_VOL_BLOCK_DEVICE               *FvbDevice;

    FvbDevice = FVB_DEVICE_FROM_THIS (This);

    *Address = GetFvbInstance(FvbDevice->Instance)->FvBase;

    return EFI_SUCCESS;
}



/**
  Retrieve the size of a logical block

  @param[in]  This        Calling context
  @param[in]  Lba         Indicates which block to return the size for.
  @param[out] BlockSize   A pointer to a caller allocated UINTN in which
                          the size of the block is returned
  @param[out] NumOfBlocks A pointer to a caller allocated UINTN in which the
                          number of consecutive blocks starting with Lba is
                          returned. All blocks in this range have a size of
                          BlockSize

  @retval     EFI_SUCCESS The function always return successfully.

**/
EFI_STATUS
EFIAPI
FvbProtocolGetBlockSize (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
    IN  EFI_LBA                            Lba,
    OUT UINTN                              *BlockSize,
    OUT UINTN                              *NumOfBlocks
)
{
    EFI_FW_VOL_BLOCK_DEVICE                 *FvbDevice;

    DEBUG((EFI_D_INFO,
           "FvbProtocolGetBlockSize: Lba: 0x%lx BlockSize: 0x%x NumOfBlocks: 0x%x\n",
           Lba,
           BlockSize,
           NumOfBlocks)
         );

    FvbDevice = FVB_DEVICE_FROM_THIS (This);

    return FvbGetLbaAddress (
               FvbDevice->Instance,
               Lba,
               NULL,
               BlockSize,
               NumOfBlocks
           );
}



/**
  Retrieves Volume attributes.  No polarity translations are done.

  @param[in]    This        Calling context
  @param[out]   Attributes  Output buffer which contains attributes

  @retval       EFI_SUCCESS The function always return successfully.

**/
EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
    OUT EFI_FVB_ATTRIBUTES_2                *Attributes
)
{
    EFI_FW_VOL_BLOCK_DEVICE               *FvbDevice;

    FvbDevice = FVB_DEVICE_FROM_THIS (This);

    *Attributes = FvbGetVolumeAttributes (FvbDevice->Instance);

    DEBUG ((EFI_D_INFO,
            "FvbProtocolGetAttributes: This: 0x%x Attributes: 0x%x\n",
            This,
            *Attributes)
          );

    return EFI_SUCCESS;
}



/**
  Sets Volume attributes. No polarity translations are done.

  @param[in]  This        Calling context
  @param[out] Attributes  Output buffer which contains attributes

  @retval     EFI_SUCCESS The function always return successfully.

**/
EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
    IN OUT EFI_FVB_ATTRIBUTES_2             *Attributes
)
{
    EFI_STATUS                            Status;
    EFI_FW_VOL_BLOCK_DEVICE               *FvbDevice;

    DEBUG((EFI_D_INFO,
           "FvbProtocolSetAttributes: Before SET -  This: 0x%x Attributes: 0x%x\n",
           This,
           *Attributes)
         );

    FvbDevice = FVB_DEVICE_FROM_THIS (This);

    Status = FvbSetVolumeAttributes (FvbDevice->Instance, Attributes);

    DEBUG((EFI_D_INFO,
           "FvbProtocolSetAttributes: After SET -  This: 0x%x Attributes: 0x%x\n",
           This,
           *Attributes)
         );

    return Status;
}



/**
  The EraseBlock() function erases one or more blocks as denoted by the
  variable argument list. The entire parameter list of blocks must be verified
  prior to erasing any blocks.  If a block is requested that does not exist
  within the associated firmware volume (it has a larger index than the last
  block of the firmware volume), the EraseBlock() function must return
  EFI_INVALID_PARAMETER without modifying the contents of the firmware volume.

  @param[in] This         Calling context
  @param[in] ...          Starting LBA followed by Number of Lba to erase.
                          a -1 to terminate the list.

  @retval EFI_SUCCESS       The erase request was successfully completed
  @retval EFI_ACCESS_DENIED The firmware volume is in the WriteDisabled state
  @retval EFI_DEVICE_ERROR  The block device is not functioning correctly and
                            could not be written. Firmware device may have been
                            partially erased

**/EFI_STATUS
EFIAPI
FvbProtocolEraseBlocks (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
    ...
)
{
    EFI_FW_VOL_BLOCK_DEVICE               *FvbDevice;
    EFI_FW_VOL_INSTANCE                   *FwhInstance;
    UINTN                                 NumOfBlocks;
    VA_LIST                               args;
    EFI_LBA                               StartingLba;
    UINTN                                 NumOfLba;
    EFI_STATUS                            Status;

    DEBUG((EFI_D_INFO, "FvbProtocolEraseBlocks: \n"));
    FvbDevice = FVB_DEVICE_FROM_THIS (This);

    FwhInstance  = GetFvbInstance (FvbDevice->Instance);

    NumOfBlocks = FwhInstance->NumOfBlocks;

    VA_START (args, This);

    do {
        StartingLba = VA_ARG (args, EFI_LBA);
        if ( StartingLba == EFI_LBA_LIST_TERMINATOR ) {
            break;
        }

        NumOfLba = VA_ARG (args, UINT32);

        //
        // Check input parameters
        //
        if (NumOfLba == 0) {
            VA_END (args);
            return EFI_INVALID_PARAMETER;
        }

        if ( ( StartingLba + NumOfLba ) > NumOfBlocks ) {
            return EFI_INVALID_PARAMETER;
        }
    } while ( 1 );

    VA_END (args);

    VA_START (args, This);
    do {
        StartingLba = VA_ARG (args, EFI_LBA);
        if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
            break;
        }

        NumOfLba = VA_ARG (args, UINT32);

        while ( NumOfLba > 0 ) {
            Status = FvbEraseBlock (FvbDevice->Instance, StartingLba);
            if ( EFI_ERROR(Status)) {
                VA_END (args);
                return Status;
            }
            StartingLba ++;
            NumOfLba --;
        }

    } while ( 1 );

    VA_END (args);

    return EFI_SUCCESS;
}



/**
  Writes data beginning at Lba:Offset from FV. The write terminates either
  when *NumBytes of data have been written, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written. The write opertion does not include erase. This routine will
  attempt to write only the specified bytes. If the writes do not stick,
  it will return an error.

  @param[in]      This      Calling context
  @param[in]      Lba       Block in which to begin write
  @param[in]      Offset    Offset in the block at which to begin write
  @param[in,out]  NumBytes  On input, indicates the requested write size. On
                            output, indicates the actual number of bytes written
  @param[in]      Buffer    Buffer containing source data for the write.

  @retval EFI_SUCCESS           The firmware volume was written successfully
  @retval EFI_BAD_BUFFER_SIZE   Write attempted across a LBA boundary. On output,
                                NumBytes contains the total number of bytes
                                actually written
  @retval EFI_ACCESS_DENIED     The firmware volume is in the WriteDisabled state
  @retval EFI_DEVICE_ERROR      The block device is not functioning correctly and
                                could not be written
  @retval EFI_INVALID_PARAMETER NumBytes or Buffer are NULL

**/
EFI_STATUS
EFIAPI
FvbProtocolWrite (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
    IN EFI_LBA                              Lba,
    IN UINTN                                Offset,
    IN OUT UINTN                            *NumBytes,
    IN UINT8                                *Buffer
)
{

    EFI_FW_VOL_BLOCK_DEVICE               *FvbDevice;

    FvbDevice = FVB_DEVICE_FROM_THIS (This);

    DEBUG((EFI_D_INFO,
           "FvbProtocolWrite: Lba: 0x%lx Offset: 0x%x NumBytes: 0x%x, Buffer: 0x%x\n",
           Lba,
           Offset,
           *NumBytes,
           Buffer)
         );

    return FvbWriteBlock (FvbDevice->Instance, Lba, Offset, NumBytes, Buffer);
}



/**
  Reads data beginning at Lba:Offset from FV. The Read terminates either
  when *NumBytes of data have been read, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written. The write opertion does not include erase. This routine will
  attempt to write only the specified bytes. If the writes do not stick,
  it will return an error.

  @param[in]      This      Calling context
  @param[in]      Lba       Block in which to begin write
  @param[in]      Offset    Offset in the block at which to begin write
  @param[in,out]  NumBytes  On input, indicates the requested write size. On
                            output, indicates the actual number of bytes written
  @param[in]      Buffer    Buffer containing source data for the write.


Returns:
  @retval EFI_SUCCESS           The firmware volume was read successfully and
                                contents are in Buffer
  @retval EFI_BAD_BUFFER_SIZE   Read attempted across a LBA boundary. On output,
                                NumBytes contains the total number of bytes returned
                                in Buffer
  @retval EFI_ACCESS_DENIED     The firmware volume is in the ReadDisabled state
  @retval EFI_DEVICE_ERROR      The block device is not functioning correctly and
                                could not be read
  @retval EFI_INVALID_PARAMETER NumBytes or Buffer are NULL

**/
EFI_STATUS
EFIAPI
FvbProtocolRead (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
    IN EFI_LBA                              Lba,
    IN UINTN                                Offset,
    IN OUT UINTN                            *NumBytes,
    OUT UINT8                                *Buffer
)
{

    EFI_FW_VOL_BLOCK_DEVICE   *FvbDevice;
    EFI_STATUS                Status;

    FvbDevice = FVB_DEVICE_FROM_THIS (This);
    Status = FvbReadBlock (FvbDevice->Instance, Lba, Offset, NumBytes, Buffer);
    DEBUG((EFI_D_INFO,
           "FvbProtocolRead: Lba: 0x%lx Offset: 0x%x NumBytes: 0x%x, Buffer: 0x%x\n",
           Lba,
           Offset,
           *NumBytes,
           Buffer)
         );

    return Status;
}

/**
  Check the integrity of firmware volume header

  @param[in]  FwVolHeader   A pointer to a firmware volume header

  @retval     TRUE          The firmware volume is consistent
  @retval     FALSE         The firmware volume has corrupted.

**/
STATIC
BOOLEAN
IsFvHeaderValid (
    IN       EFI_PHYSICAL_ADDRESS          FvBase,
    IN CONST EFI_FIRMWARE_VOLUME_HEADER    *FwVolHeader
)
{
    if (FvBase == PcdGet32(PcdFlashNvStorageBase)) {
        if (CompareMem (&FwVolHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid, sizeof(EFI_GUID)) != 0 ) {
            return FALSE;
        }
    } else {
        if (CompareMem (&FwVolHeader->FileSystemGuid, &gEfiFirmwareFileSystem2Guid, sizeof(EFI_GUID)) != 0 ) {
            return FALSE;
        }
    }
    if ( (FwVolHeader->Revision != EFI_FVH_REVISION)   ||
            (FwVolHeader->Signature != EFI_FVH_SIGNATURE) ||
            (FwVolHeader->FvLength == ((UINTN) -1))       ||
            ((FwVolHeader->HeaderLength & 0x01 ) !=0) )  {
        return FALSE;
    }

    if (CalculateCheckSum16 ((UINT16 *) FwVolHeader, FwVolHeader->HeaderLength) != 0) {
        return FALSE;
    }

    return TRUE;
}



/**
  The function installs EFI_FIRMWARE_VOLUME_BLOCK protocol
  for each FV in the system.

  @param[in]  FwhInstance   The pointer to a FW volume instance structure,
                            which contains the information about one FV.
  @param[in]  InstanceNum   The instance number which can be used as a ID
                            to locate this FwhInstance in other functions.

  @retval     VOID

**/
STATIC
VOID
InstallFvbProtocol (
    IN  EFI_FW_VOL_INSTANCE *FwhInstance,
    IN  UINTN               InstanceNum
)
{
    EFI_FW_VOL_BLOCK_DEVICE               *FvbDevice;
    EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
    EFI_STATUS                            Status;
    EFI_HANDLE                            FwbHandle;
    EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *OldFwbInterface;

    FvbDevice = (EFI_FW_VOL_BLOCK_DEVICE *) AllocateRuntimeCopyPool (
                    sizeof (EFI_FW_VOL_BLOCK_DEVICE),
                    &mFvbDeviceTemplate
                );
    ASSERT (FvbDevice != NULL);

    FvbDevice->Instance = InstanceNum;
    FwVolHeader = &FwhInstance->VolumeHeader;

    //
    // Set up the devicepath
    //
    DEBUG ((EFI_D_INFO, "FwBlockService.c: Setting up DevicePath for 0x%lx:\n", FwhInstance->FvBase));
    if (FwVolHeader->ExtHeaderOffset == 0) {
        //
        // FV does not contains extension header, then produce MEMMAP_DEVICE_PATH
        //
        FvbDevice->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) AllocateRuntimeCopyPool (sizeof (FV_MEMMAP_DEVICE_PATH), &mFvMemmapDevicePathTemplate);
        ((FV_MEMMAP_DEVICE_PATH *) FvbDevice->DevicePath)->MemMapDevPath.StartingAddress = FwhInstance->FvBase;
        ((FV_MEMMAP_DEVICE_PATH *) FvbDevice->DevicePath)->MemMapDevPath.EndingAddress   = FwhInstance->FvBase + FwVolHeader->FvLength - 1;
    } else {
        FvbDevice->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) AllocateRuntimeCopyPool (sizeof (FV_PIWG_DEVICE_PATH), &mFvPIWGDevicePathTemplate);
        CopyGuid (
            &((FV_PIWG_DEVICE_PATH *)FvbDevice->DevicePath)->FvDevPath.FvName,
            (GUID *)(UINTN)(FwhInstance->FvBase + FwVolHeader->ExtHeaderOffset)
        );
    }

    //
    // Find a handle with a matching device path that has supports FW Block protocol
    //
    Status = gBS->LocateDevicePath (&gEfiFirmwareVolumeBlockProtocolGuid, &FvbDevice->DevicePath, &FwbHandle);
    if (EFI_ERROR (Status) ) {
        //
        // LocateDevicePath fails so install a new interface and device path
        //
        DEBUG ((EFI_D_INFO, "FwBlockService.c: LocateDevicePath failed, install new interface 0x%lx:\n", FwhInstance->FvBase));
        FwbHandle = NULL;
        Status =  gBS->InstallMultipleProtocolInterfaces (
                      &FwbHandle,
                      &gEfiFirmwareVolumeBlockProtocolGuid,
                      &FvbDevice->FwVolBlockInstance,
                      &gEfiDevicePathProtocolGuid,
                      FvbDevice->DevicePath,
                      NULL
                  );
        ASSERT_EFI_ERROR (Status);
        DEBUG ((EFI_D_INFO, "FwBlockService.c: IMPI FirmwareVolBlockProt, DevPath 0x%lx: %r\n", FwhInstance->FvBase, Status));

    } else if (IsDevicePathEnd (FvbDevice->DevicePath)) {
        //
        // Device allready exists, so reinstall the FVB protocol
        //
        DEBUG ((EFI_D_ERROR, "FwBlockService.c: LocateDevicePath succeeded, reinstall interface 0x%lx:\n", FwhInstance->FvBase));
        Status = gBS->HandleProtocol (
                     FwbHandle,
                     &gEfiFirmwareVolumeBlockProtocolGuid,
                     (VOID **) &OldFwbInterface
                 );
        ASSERT_EFI_ERROR (Status);

        Status =  gBS->ReinstallProtocolInterface (
                      FwbHandle,
                      &gEfiFirmwareVolumeBlockProtocolGuid,
                      OldFwbInterface,
                      &FvbDevice->FwVolBlockInstance
                  );
        ASSERT_EFI_ERROR (Status);

    } else {
        //
        // There was a FVB protocol on an End Device Path node
        //
        ASSERT (FALSE);
    }

}



/**
  The driver entry point for Firmware Volume Block Driver.

  The function does the necessary initialization work
  Firmware Volume Block Driver.

  @param[in]  ImageHandle       The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable       A pointer to the EFI system table.

  @retval     EFI_SUCCESS       This funtion always return EFI_SUCCESS.
                                It will ASSERT on errors.

**/
EFI_STATUS
EFIAPI
FvbInitialize (
    IN EFI_HANDLE         ImageHandle,
    IN EFI_SYSTEM_TABLE   *SystemTable
)
{
    EFI_FW_VOL_INSTANCE                   *FwhInstance;
    EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
    EFI_FIRMWARE_VOLUME_HEADER            *FvHeader;
    EFI_FV_BLOCK_MAP_ENTRY                *PtrBlockMapEntry;
    EFI_PHYSICAL_ADDRESS                  BaseAddress;
    EFI_STATUS                            Status;
    UINTN                                 BufferSize;
    UINTN                                 TmpHeaderLength;
    UINTN                                 Idx;
    UINT32                                MaxLbaSize;
    BOOLEAN                               FvHeaderValid;
    EFI_EVENT                             Event;
    EFI_BOOT_MODE                 BootMode;

    UINTN LbaAddress;
    UINTN BlockCount;

    Status = gBS->LocateProtocol (&gEfiNvMediaAccessProtocolGuid, NULL, &pMediaAccessProtocol);
    ASSERT_EFI_ERROR (Status);

    Status = gBS->CreateEventEx (
                 EVT_NOTIFY_SIGNAL,
                 TPL_NOTIFY,
                 FvbVirtualddressChangeEvent,
                 NULL,
                 &gEfiEventVirtualAddressChangeGuid,
                 &Event
             );
    ASSERT_EFI_ERROR (Status);

    //
    // Calculate the total size for all firmware volume block instances
    //
    BufferSize = 0;
    for (Idx = 0; Idx < sizeof (mPlatformFvBaseAddress)/sizeof (mPlatformFvBaseAddress[0]); Idx++) {
        FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) mPlatformFvBaseAddress[Idx];
        BufferSize +=  (FvHeader->HeaderLength +
                        sizeof (EFI_FW_VOL_INSTANCE) -
                        sizeof (EFI_FIRMWARE_VOLUME_HEADER)
                       );
    }

    mFvbModuleGlobal.FvInstance =  (EFI_FW_VOL_INSTANCE *) AllocateRuntimeZeroPool (BufferSize);
    ASSERT (NULL != mFvbModuleGlobal.FvInstance);


    MaxLbaSize      = 0;
    FwhInstance     = mFvbModuleGlobal.FvInstance;
    mFvbModuleGlobal.NumFv   = 0;

    for (Idx = 0; Idx < sizeof (mPlatformFvBaseAddress)/sizeof (mPlatformFvBaseAddress[0]); Idx++) {
        BaseAddress = mPlatformFvBaseAddress[Idx];
    // 
    // for quick boot path, only handle NvStorage FV for best performance. 
    // 
      BootMode = GetBootModeHob();
    if  ((BootMode == BOOT_ASSUMING_NO_CONFIGURATION_CHANGES) 
       && (BaseAddress != FixedPcdGet32(PcdFlashNvStorageBase))){ 
          continue;
	 }	  
        FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) BaseAddress;

        if (!IsFvHeaderValid (BaseAddress, FwVolHeader)) {
            FvHeaderValid = FALSE;

            //
            // If not valid, get FvbInfo from the information carried in
            // FVB driver.
            //
            DEBUG ((EFI_D_ERROR, "Fvb: FV header @ 0x%lx invalid\n", BaseAddress));
            Status          = GetFvbInfo (BaseAddress, &FwVolHeader);
            ASSERT_EFI_ERROR(Status);
            //
            //  Write back a healthy FV header
            //
            DEBUG ((EFI_D_ERROR, "FwBlockService.c: Writing back healthy FV header\n"));
            LbaAddress = (UINTN)(BaseAddress + FwhInstance->FvBase);
            BlockCount = FwVolHeader->BlockMap->Length / SF_SECTOR_SIZE;
            while (BlockCount > 0) {
                Status = pMediaAccessProtocol->Erase(pMediaAccessProtocol, LbaAddress, SF_SECTOR_SIZE, SPI_MEDIA_TYPE);
                if (EFI_ERROR(Status)) {
                    break;
                }
                LbaAddress += SF_SECTOR_SIZE;
                BlockCount--;
            }

            TmpHeaderLength = (UINTN) FwVolHeader->HeaderLength;

            Status = pMediaAccessProtocol->Write(pMediaAccessProtocol, (UINTN)(BaseAddress + FwhInstance->FvBase), (UINT8 *) FwVolHeader, TmpHeaderLength, SPI_MEDIA_TYPE);

            WriteBackInvalidateDataCacheRange (
                (VOID *) (UINTN) BaseAddress,
                FwVolHeader->BlockMap->Length
            );

        }

        CopyMem (&(FwhInstance->VolumeHeader), FwVolHeader, FwVolHeader->HeaderLength);

        FwVolHeader = &(FwhInstance->VolumeHeader);
        FwhInstance->FvBase = (UINTN)BaseAddress;

        //
        // Process the block map for each FV
        //
        FwhInstance->NumOfBlocks   = 0;
        for (PtrBlockMapEntry = FwVolHeader->BlockMap;
                PtrBlockMapEntry->NumBlocks != 0;
                PtrBlockMapEntry++) {
            //
            // Get the maximum size of a block.
            //
            if (MaxLbaSize < PtrBlockMapEntry->Length) {
                MaxLbaSize  = PtrBlockMapEntry->Length;
            }
            FwhInstance->NumOfBlocks += PtrBlockMapEntry->NumBlocks;
        }

        //
        // Add a FVB Protocol Instance
        //
        //
        //
        mFvbModuleGlobal.NumFv++;
        InstallFvbProtocol (FwhInstance, mFvbModuleGlobal.NumFv - 1);

        //
        // Move on to the next FwhInstance
        //
        FwhInstance = (EFI_FW_VOL_INSTANCE *) ((UINTN)((UINT8 *)FwhInstance) +
                                               FwVolHeader->HeaderLength +
                                               (sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER)));

    }

    return EFI_SUCCESS;
}

