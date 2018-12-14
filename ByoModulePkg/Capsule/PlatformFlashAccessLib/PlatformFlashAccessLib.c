/*++
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2018 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================
Module Name:
  PlatformFlashAccessLib.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include <PiDxe.h>
#include <Protocol/DevicePath.H>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformFlashAccessLib.h>
#include <Library/SpiFlash.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Uefi.h>
#include <PiDxe.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <ScAccess.h>
#include <SeCAccess.h>
#include <Library/FileHandleLib.h>
#include <Library/DevicePathLib.h>
typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           PciDevice;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_PCI_DEVICE_PATH;

#define PCI_DEVICE_PATH_NODE(Func, Dev) \
  { \
    { \
      HARDWARE_DEVICE_PATH, \
      HW_PCI_DP, \
      { \
        (UINT8) (sizeof (PCI_DEVICE_PATH)), \
        (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8) \
      } \
    }, \
    (Func), \
    (Dev) \
  }


#define PNPID_DEVICE_PATH_NODE(PnpId) \
  { \
    { \
      ACPI_DEVICE_PATH, \
      ACPI_DP, \
      { \
        (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), \
        (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8) \
      }, \
    }, \
    EISA_PNP_ID((PnpId)), \
    0 \
  }


#ifndef gPciRootBridge
#define gPciRootBridge \
  PNPID_DEVICE_PATH_NODE(0x0A03)
#endif
#ifndef gEndEntire
#define gEndEntire \
  { \
    END_DEVICE_PATH_TYPE, \
    END_ENTIRE_DEVICE_PATH_SUBTYPE, \
    { \
      END_DEVICE_PATH_LENGTH, \
      0 \
    } \
  }
#endif

PLATFORM_PCI_DEVICE_PATH gEmmcDevPath0 = {
  gPciRootBridge,
  PCI_DEVICE_PATH_NODE (0x00, 0x1c),
  gEndEntire
};

PLATFORM_PCI_DEVICE_PATH gUSBDevPath0 = {
  gPciRootBridge,
  PCI_DEVICE_PATH_NODE (0x00, 21),
  gEndEntire
};

PLATFORM_PCI_DEVICE_PATH gSataDevPath0 = {
  gPciRootBridge,
  PCI_DEVICE_PATH_NODE (0x00, 0x12),
  gEndEntire
};

#define SECTOR_SIZE_64KB  0x10000      // Common 64kBytes sector size
#define ALINGED_SIZE  SECTOR_SIZE_64KB
SC_SPI_PROTOCOL                *mSpiProtocol;
EFI_GRAPHICS_OUTPUT_PROTOCOL   *GraphicsOutput;
extern EFI_STATUS EFIAPI EnableQuietBoot (IN  EFI_GUID  *LogoFile);

EFI_STATUS
FlashRead (
  IN  UINTN             BaseAddress,
  IN  UINT8             *Byte,
  IN  UINTN             Length,
  IN  SPI_REGION_TYPE   SpiRegionType
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT32                SectorSize;
  UINT32                SpiAddress;
  UINT8                 Buffer[SECTOR_SIZE_4KB];
  UINTN                 NumBytes = 0;

  SpiAddress = (UINT32)(UINTN)(BaseAddress);
  SectorSize = SECTOR_SIZE_4KB;

  while (NumBytes < Length) {
    Status = mSpiProtocol ->FlashRead (
                              mSpiProtocol,
                              SpiRegionType,
                              SpiAddress,
                              SECTOR_SIZE_4KB,
                              Buffer
                              );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Read SPI ROM Failed [%08x]\n", SpiAddress));
      return Status;
    }

    CopyMem (Byte+NumBytes, (VOID *)Buffer, SectorSize);
    SpiAddress += SectorSize;
    NumBytes += SectorSize;
  }
  return Status;
}

BOOLEAN
FlashReadCompare (
  IN  UINTN             BaseAddress,
  IN  UINT8             *Byte,
  IN  UINTN             Length,
  IN  SPI_REGION_TYPE   SpiRegionType
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT32                SpiAddress;
  UINT8                 Buffer[SECTOR_SIZE_4KB];

  SpiAddress = (UINT32)(BaseAddress);

  while (Length > 0) {
    Status = mSpiProtocol ->FlashRead (
                              mSpiProtocol,
                              SpiRegionType,
                              SpiAddress,
                              SECTOR_SIZE_4KB,
                              Buffer
                              );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Read SPI ROM Failed [0x%08x], Status: %r\n", SpiAddress, Status));
      return FALSE;
    }

    SpiAddress += SECTOR_SIZE_4KB;
    Length     -= SECTOR_SIZE_4KB;
    if (CompareMem(Buffer, Byte, SECTOR_SIZE_4KB)) {
      return FALSE;
    }
  }
  return TRUE;
}

EFI_STATUS
FlashErase (
  IN  UINTN             BaseAddress,
  IN  UINTN             NumBytes,
  IN  SPI_REGION_TYPE   SpiRegionType
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT32                SpiAddress;

  SpiAddress = (UINT32)(BaseAddress);
  while (NumBytes > 0) {
    Status = mSpiProtocol->FlashErase (
                              mSpiProtocol,
                              SpiRegionType,
                              SpiAddress,
                              SECTOR_SIZE_4KB
                              );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Erase failed. Status: %r NumBytes = 0x%x\n", Status, NumBytes));
      break;
    }
    SpiAddress += SECTOR_SIZE_4KB;
    NumBytes   -= SECTOR_SIZE_4KB;
  }

  return Status;
}

EFI_STATUS
FlashWrite (
  IN  UINTN             DstBufferPtr,
  IN  UINT8             *Byte,
  IN  UINTN             Length,
  IN  SPI_REGION_TYPE   SpiRegionType
  )
{
  EFI_STATUS                Status;
  UINT32                    NumBytes = (UINT32)Length;
  UINT8*                    pBuf8 = Byte;
  UINT32                    SpiAddress;

  SpiAddress = (UINT32)(DstBufferPtr);
  Status = mSpiProtocol->FlashWrite(
                           mSpiProtocol,
                           SpiRegionType,
                           SpiAddress,
                           NumBytes,
                           pBuf8
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SPI Wirte failed. Status: %r.\n", Status));
    return Status;
  }

  return Status;
}

EFI_STATUS
BIOSVerify (
  IN  UINTN             FileSize,
  IN  UINT8             *FileBuffer,
  IN  SPI_REGION_TYPE   SpiRegionType
  )
{
  EFI_STATUS       Status = EFI_SUCCESS;
  UINT32           DataIndex;
  BOOLEAN          Flag = TRUE;

  for (DataIndex = 0; DataIndex < FileSize; DataIndex += SECTOR_SIZE_4KB) {
    if (FlashReadCompare(DataIndex, FileBuffer + DataIndex, SECTOR_SIZE_4KB, SpiRegionType)) {
      Print(L"Verifying... %d%% Completed.  \r", (DataIndex * 100 / FileSize));
      continue;
    }
    Print(L"Verifying... %d%% Completed   Error at [%08x].  \n", (DataIndex * 100 / FileSize), DataIndex);
    Flag = FALSE;
  }
  Print(L"Flash Verify Complete. ");
  if (Flag) {
    Print(L"It's same.  \n");
  } else {
    Print(L"It's different as show!\n");
  }
  Status = Flag?EFI_SUCCESS:EFI_ABORTED;

  return Status;
}

EFI_STATUS
BIOSVerifyEx (
  IN  UINTN             FileSize,
  IN  UINT8             *FileBuffer,
  IN  SPI_REGION_TYPE   SpiRegionType,
  IN  UINTN             RegionOffset
  )
{
  EFI_STATUS         Status = EFI_SUCCESS;
  UINTN              DataIndex;
  BOOLEAN            Flag = TRUE;

  for (DataIndex = 0; DataIndex < FileSize; DataIndex += SECTOR_SIZE_4KB) {
    if (FlashReadCompare(DataIndex + RegionOffset, FileBuffer + DataIndex, SECTOR_SIZE_4KB, SpiRegionType)) {
      continue;
    }
    Print(L"Verifying... %d%% Completed   Error at [%08x].  \n", (DataIndex * 100 / FileSize), DataIndex);
    Flag = FALSE;
    break;
  }

  Status = Flag?EFI_SUCCESS:EFI_ABORTED;

  return Status;
}


EFI_STATUS
BIOSFlash (
  IN  UINTN             FileSize,
  IN  UINT8             *FileBuffer,
  IN  SPI_REGION_TYPE   SpiRegionType
  )
{
  EFI_STATUS         Status;
  UINT32             DataIndex;

  for (DataIndex = 0; DataIndex < FileSize; DataIndex += SECTOR_SIZE_4KB) {
    if (FlashReadCompare(DataIndex, FileBuffer + DataIndex, SECTOR_SIZE_4KB, SpiRegionType)) {
      Print(L"Updating firmware... %3d%% Completed.  \r", (DataIndex * 100 / FileSize));
      continue;
    }

    Status = FlashErase(DataIndex, SECTOR_SIZE_4KB, SpiRegionType);
    if (EFI_ERROR (Status)) {
      Print(L"SPI Erase failed. Status: %r, Error at [%08x]\n", Status, DataIndex);
      return Status;
    }

    Print(L"Updating firmware... %3d%% Completed.  \r", (DataIndex * 100 / FileSize));
    Status = FlashWrite(DataIndex, FileBuffer + DataIndex, SECTOR_SIZE_4KB, SpiRegionType);
    if (EFI_ERROR (Status)) {
      Print(L"SPI Write failed. Status: %r, Error at [%08x]\n", Status, DataIndex);
      return Status;
    }
  }

  return EFI_SUCCESS;
}
    
#define ERASE_STRING L"[                                                                                             ]"
UINTN   gPreviousValue = 0;
  
/**
  Show progress bar for FOTA update.

  @param[in] TitleForeground        The foreground of the title
  @param[in] TitleBackground        The background of the title
  @param[in] Title                  The string of title
  @param[in] ProgressColor          The color of progress bar
  @param[in] Progress               The percentage of progress
  @param[in] PreviousValue          The previous percentage number

  @retval  EFI_INVALID_PARAMETER    The pamrameter is invalid
  @retval  EFI_UNSUPPORTED          The Graphic output is unsupported
  @retval  EFI_SUCCESS              The progress status is showed successfully
**/
EFI_STATUS
CapShowProgress (
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleForeground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleBackground,
  IN CHAR16                        *Title,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL ProgressColor,
  IN UINTN                         Progress,
  IN UINTN                         PreviousValue
  )
{

  EFI_STATUS                     Status;
  UINT32                         SizeOfX;
  UINT32                         SizeOfY;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Color;
  UINTN                          BlockHeight;
  UINTN                          BlockWidth;
  UINTN                          BlockNum;
  UINTN                          PosX;
  UINTN                          PosY;
  UINTN                          Index;

  if (Progress > 100) {
    return EFI_INVALID_PARAMETER;
  }

  if (GraphicsOutput == NULL) {
    return EFI_NOT_READY;
  }


  SizeOfX = GraphicsOutput->Mode->Info->HorizontalResolution;
  SizeOfY = GraphicsOutput->Mode->Info->VerticalResolution;


  BlockWidth  = SizeOfX / 100;
  BlockHeight = SizeOfY / 50;

  BlockNum    = Progress;

  PosX        = 0;
  PosY        = SizeOfY * 48 / 50;

  if (BlockNum == 0) {
    //
    // Clear progress area
    //
    SetMem (&Color, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);


    Status = GraphicsOutput->Blt (
                              GraphicsOutput,
                              &Color,
                              EfiBltVideoFill,
                              0,
                              0,
                              0,
                              PosY - EFI_GLYPH_HEIGHT - 1,
                              SizeOfX,
                              SizeOfY - (PosY - EFI_GLYPH_HEIGHT - 1),
                              SizeOfX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                              );
    }
    //
    // Show progress by drawing blocks
    //
    for (Index = gPreviousValue; Index < BlockNum; Index++) {
      PosX = Index * BlockWidth;
      Status = GraphicsOutput->Blt (
                            GraphicsOutput,
                            &ProgressColor,
                            EfiBltVideoFill,
                            0,
                            0,
                            PosX,
                            PosY,
                            BlockWidth - 1,
                            BlockHeight,
                            (BlockWidth) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                            );
   
    }
    
    //
    //Clear
    //
    PrintXY (
      (SizeOfX - StrLen (ERASE_STRING) * EFI_GLYPH_WIDTH) / 2,
      PosY - EFI_GLYPH_HEIGHT - 20,
      &TitleForeground,
      &TitleBackground,
      ERASE_STRING
      );
    PrintXY (
      (SizeOfX - StrLen (Title) * EFI_GLYPH_WIDTH) / 2,
      PosY - EFI_GLYPH_HEIGHT - 20,
      &TitleForeground,
      &TitleBackground,
      Title
      );
    
    gPreviousValue = Progress;
    return EFI_SUCCESS;
}


UINTN   ProBar = 0;
EFI_STATUS
BIOSFlashEx(
  IN UINTN              BufSize,
  IN UINT8              *BufferPtr,
  IN SPI_REGION_TYPE    SpiRegionType,
  IN UINTN              RegionOffset  //Region offset is the offset from beginning of the region.
  )
{
  EFI_STATUS                            Status = EFI_SUCCESS;
  UINTN                                 DataIndex;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         Color;
  SetMem (&Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xFF);
  SetMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);
  SetMem (&Color, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xFF);
  if ((RegionOffset & 0xFFF) != 0) {
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  for (DataIndex = 0; DataIndex < BufSize; DataIndex += SECTOR_SIZE_4KB) {
    if (FlashReadCompare(DataIndex + RegionOffset, BufferPtr + DataIndex, SECTOR_SIZE_4KB, SpiRegionType)) {
      //
      // Skips over unchanged bytes
      //
      continue;
    }
    if (ProBar >= 100) {
      ProBar = 0;
    }
    
    CapShowProgress (Foreground, Background, L"WARN:Update BIOS, Please Wait!!", Color, ProBar, ProBar?(ProBar - 1):ProBar);
    ProBar += 1;
    Status = FlashErase(DataIndex+RegionOffset, SECTOR_SIZE_4KB, SpiRegionType);
    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_ERROR, "In BIOSFlashEx: Failed to erase 4K blocks.\n"));
      return Status;
    }

    Status = FlashWrite(DataIndex+RegionOffset, BufferPtr + DataIndex, SECTOR_SIZE_4KB, SpiRegionType);
    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_ERROR, "In BIOSFlashEx: Failed to write 4K blocks.\n"));
      return Status;
    }
  }

  return Status;
}




 
/**
  Perform flash write opreation.

  @param[in] FirmwareType      The type of firmware.
  @param[in] FlashAddress      The address of flash device to be accessed.
  @param[in] FlashAddressType  The type of flash device address.
  @param[in] Buffer            The pointer to the data buffer.
  @param[in] Length            The length of data buffer in bytes.

  @retval EFI_SUCCESS           The operation returns successfully.
  @retval EFI_WRITE_PROTECTED   The flash device is read only.
  @retval EFI_UNSUPPORTED       The flash device access is unsupported.
  @retval EFI_INVALID_PARAMETER The input parameter is not valid.
**/
EFI_STATUS
EFIAPI
PerformFlashWrite (
  IN PLATFORM_FIRMWARE_TYPE       FirmwareType,
  IN EFI_PHYSICAL_ADDRESS         FlashAddress,
  IN FLASH_ADDRESS_TYPE           FlashAddressType,
  IN VOID                         *Buffer,
  IN UINTN                        Length
  )
{
  EFI_STATUS                            Status;
  UINT32                                *Data32;
//  UINTN                                 Index;
  UINT8                                 *fBuffer;
  Data32 = Buffer;
  if (FirmwareType == BACKUP_COPY) { //Backup RomHole to image
    fBuffer = AllocateZeroPool (Length);
    FlashRead (FlashAddress, fBuffer, Length, FlashRegionAll);
    CopyMem (Buffer, fBuffer, Length);
    FreePool (fBuffer);
    return EFI_SUCCESS;
  }
  if (FirmwareType == BACKUP_STROAGE) {
    BackupBinaryonESP (L"OBB.fd", Buffer, Length);
    fBuffer = AllocateZeroPool (Length);
    FlashRead (FlashAddress, fBuffer, Length, FlashRegionAll);
    BackupBinaryonESP (L"OBB1.fd", fBuffer, Length);
    FreePool (fBuffer);
    return EFI_SUCCESS;
  }
  if (FirmwareType == BPDT_GREEN) { //green
    *Data32 = 0x000055AA;
  } 
  if (FirmwareType == BPDT_YELLOW) { //yellow
    *Data32 = 0x00AA55AA;
  } 
  if (FirmwareType == BPDT_RED) { //red
    *Data32 = 0xFFFFFFFF;
  }
  DEBUG((DEBUG_INFO, "PerformFlashWrite - 0x%x(%x) - 0x%x\n", (UINTN)FlashAddress, (UINTN)FlashAddressType, Length));
  Status = BIOSFlashEx (Length, (UINT8 *)Buffer, FlashRegionAll, (UINTN)FlashAddress);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  return EFI_SUCCESS;
}



EFI_STATUS
EFIAPI
PerformFlashAccessLibConstructor (
  VOID
  )
{
  EFI_STATUS    Status;
  Status = gBS->LocateProtocol (
                  &gEfiSpiProtocolGuid,
                  NULL,
                  (VOID **)&mSpiProtocol
                  );
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "locate SPI protocol : %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}


