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

#ifndef _FW_BLOCK_SERVICE_H
#define _FW_BLOCK_SERVICE_H


#include <PiSmm.h>

#include <Guid/EventGroup.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/FaultTolerantWrite.h>
#include <Protocol/DevicePath.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/NvMediaAccess.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HobLib.h>
#include <Protocol/SmmFirmwareVolumeBlock.h>


#define SF_SECTOR_SIZE    0x1000

//
// Define two helper macro to extract the Capability field or Status field in FVB
// bit fields
//
#define EFI_FVB2_CAPABILITIES (EFI_FVB2_READ_DISABLED_CAP | \
                              EFI_FVB2_READ_ENABLED_CAP | \
                              EFI_FVB2_WRITE_DISABLED_CAP | \
                              EFI_FVB2_WRITE_ENABLED_CAP | \
                              EFI_FVB2_LOCK_CAP \
                              )

#define EFI_FVB2_STATUS (EFI_FVB2_READ_STATUS | EFI_FVB2_WRITE_STATUS | EFI_FVB2_LOCK_STATUS)


typedef struct {
    UINTN                       FvBase;
    UINTN                       NumOfBlocks;
    //
    // Note!!!: VolumeHeader must be the last element
    // of the structure.
    //
    EFI_FIRMWARE_VOLUME_HEADER  VolumeHeader;
} EFI_FW_VOL_INSTANCE;


typedef struct {
    EFI_FW_VOL_INSTANCE         *FvInstance;
    UINT32                      NumFv;
} FWB_GLOBAL;

//
// Fvb Protocol instance data
//
#define FVB_DEVICE_FROM_THIS(a) CR(a, EFI_FW_VOL_BLOCK_DEVICE, FwVolBlockInstance, FVB_DEVICE_SIGNATURE)
#define FVB_EXTEND_DEVICE_FROM_THIS(a) CR(a, EFI_FW_VOL_BLOCK_DEVICE, FvbExtension, FVB_DEVICE_SIGNATURE)
#define FVB_DEVICE_SIGNATURE       SIGNATURE_32('F','V','B','C')

typedef struct {
    MEDIA_FW_VOL_DEVICE_PATH  FvDevPath;
    EFI_DEVICE_PATH_PROTOCOL  EndDevPath;
} FV_PIWG_DEVICE_PATH;

typedef struct {
    MEMMAP_DEVICE_PATH          MemMapDevPath;
    EFI_DEVICE_PATH_PROTOCOL    EndDevPath;
} FV_MEMMAP_DEVICE_PATH;

typedef struct {
    UINT32                                Signature;
    EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
    UINTN                                 Instance;
    EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    FwVolBlockInstance;
} EFI_FW_VOL_BLOCK_DEVICE;

EFI_STATUS
GetFvbInfo (
    IN  EFI_PHYSICAL_ADDRESS              FvBaseAddress,
    OUT EFI_FIRMWARE_VOLUME_HEADER        **FvbInfo
);

//
// Protocol APIs
//
EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
    OUT EFI_FVB_ATTRIBUTES_2                      *Attributes
);

EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
    IN OUT EFI_FVB_ATTRIBUTES_2                   *Attributes
);

EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
    OUT EFI_PHYSICAL_ADDRESS               *Address
);

EFI_STATUS
FvbProtocolGetBlockSize (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
    IN  EFI_LBA                            Lba,
    OUT UINTN                              *BlockSize,
    OUT UINTN                              *NumOfBlocks
);

EFI_STATUS
EFIAPI
FvbProtocolRead (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
    IN EFI_LBA                              Lba,
    IN UINTN                                Offset,
    IN OUT UINTN                            *NumBytes,
    OUT UINT8                                *Buffer
);

EFI_STATUS
EFIAPI
FvbProtocolWrite (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
    IN EFI_LBA                              Lba,
    IN UINTN                                Offset,
    IN OUT UINTN                            *NumBytes,
    IN UINT8                                *Buffer
);

EFI_STATUS
EFIAPI
FvbProtocolEraseBlocks (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
    ...
);

#endif

