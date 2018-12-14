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

#include <PiDxe.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/SystemNvDataGuid.h>

#define FIRMWARE_BLOCK_SIZE         0x8000
#define FVB_MEDIA_BLOCK_SIZE        FIRMWARE_BLOCK_SIZE * 2

#define FV_RECOVERY_BASE_ADDRESS    FixedPcdGet32(PcdFlashFvRecoveryBase)
#define RECOVERY_BIOS_BLOCK_NUM     (FixedPcdGet32(PcdFlashFvRecoverySize) / FVB_MEDIA_BLOCK_SIZE)

#define FV_MAIN_BASE_ADDRESS        FixedPcdGet32(PcdFlashFvMainBase)
#define MAIN_BIOS_BLOCK_NUM         (FixedPcdGet32(PcdFlashFvMainSize) / FVB_MEDIA_BLOCK_SIZE)

#define NV_STORAGE_BASE_ADDRESS     FixedPcdGet32(PcdFlashNvStorageBase)
#define SYSTEM_NV_BLOCK_NUM         (FixedPcdGet32(PcdFlashNvStorageSize) / FVB_MEDIA_BLOCK_SIZE)




typedef struct {
    EFI_PHYSICAL_ADDRESS        BaseAddress;
    EFI_FIRMWARE_VOLUME_HEADER  FvbInfo;
    //
    //EFI_FV_BLOCK_MAP_ENTRY    ExtraBlockMap[n];//n=0
    //
    EFI_FV_BLOCK_MAP_ENTRY      End[1];
} EFI_FVB2_MEDIA_INFO;


EFI_FVB2_MEDIA_INFO mPlatformFvbMediaInfo[] = {
    //
    // Systen NvStorage FVB
    //
    {
        NV_STORAGE_BASE_ADDRESS,
        {
            {0,}, //ZeroVector[16]
            EFI_SYSTEM_NV_DATA_FV_GUID,
            FVB_MEDIA_BLOCK_SIZE * SYSTEM_NV_BLOCK_NUM,
            EFI_FVH_SIGNATURE,
            0x0004feff, // check MdePkg/Include/Pi/PiFirmwareVolume.h for details on EFI_FVB_ATTRIBUTES_2
            sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
            0x0936,    //CheckSum
            0,    //ExtHeaderOffset
            {0,}, //Reserved[1]
            2,    //Revision
            {
                {
                    SYSTEM_NV_BLOCK_NUM,
                    FVB_MEDIA_BLOCK_SIZE,
                }
            }
        },
        {
            {
                0,
                0
            }
        }
    }
};

EFI_STATUS
GetFvbInfo (
    IN  EFI_PHYSICAL_ADDRESS         FvBaseAddress,
    OUT EFI_FIRMWARE_VOLUME_HEADER   **FvbInfo
)
{
    UINTN   Index;

    for (Index=0; Index < sizeof (mPlatformFvbMediaInfo)/sizeof (mPlatformFvbMediaInfo[0]); Index += 1) {
        if (mPlatformFvbMediaInfo[Index].BaseAddress == FvBaseAddress) {
            *FvbInfo =  &mPlatformFvbMediaInfo[Index].FvbInfo;

            DEBUG ((EFI_D_INFO, "\nBaseAddr: 0x%lx \n", FvBaseAddress));
            DEBUG ((EFI_D_INFO, "FvLength: 0x%lx \n", (*FvbInfo)->FvLength));
            DEBUG ((EFI_D_INFO, "HeaderLength: 0x%x \n", (*FvbInfo)->HeaderLength));
            DEBUG ((EFI_D_INFO, "FvBlockMap[0].NumBlocks: 0x%x \n", (*FvbInfo)->BlockMap[0].NumBlocks));
            DEBUG ((EFI_D_INFO, "FvBlockMap[0].BlockLength: 0x%x \n", (*FvbInfo)->BlockMap[0].Length));
            DEBUG ((EFI_D_INFO, "FvBlockMap[1].NumBlocks: 0x%x \n",   (*FvbInfo)->BlockMap[1].NumBlocks));
            DEBUG ((EFI_D_INFO, "FvBlockMap[1].BlockLength: 0x%x \n\n", (*FvbInfo)->BlockMap[1].Length));

            return EFI_SUCCESS;
        }
    }
    return EFI_NOT_FOUND;
}
