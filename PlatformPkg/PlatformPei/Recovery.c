/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

Module Name:

  Recovery.c

Abstract:

  PEIM to provide the platform recovery functionality.

--*/


#include "PlatformPei.h"
#include "CHX002Reg.h"
#include <Ppi/AtaController.h>
#include <Ppi/BootInRecoveryMode.h>
#include <IndustryStandard/Pci.h>



EFI_STATUS
EFIAPI
BuildDefaultDataHobForRecoveryVariable (
  IN EFI_PEI_SERVICES  **PeiServices
  );


#define SB_SATA_PRIMARY_CMD_BAR      0x1F0
#define SB_SATA_PRIMARY_CTL_BAR      0x3F6
#define SB_SATA_SECONDARY_CMD_BAR    0x170
#define SB_SATA_SECONDARY_CTL_BAR    0x376

//
// Required Service
//
EFI_STATUS
EnableAtaChannel (
  IN EFI_PEI_SERVICES       **PeiServices,
  IN PEI_ATA_CONTROLLER_PPI *This,
  IN UINT8                  ChannelIndex
  );

UINT32
GetIdeRegsBaseAddr (
  IN  EFI_PEI_SERVICES       **PeiServices,
  IN  PEI_ATA_CONTROLLER_PPI *This,
  OUT IDE_REGS_BASE_ADDR     *IdeRegsBaseAddr
  );

//
// Module globals
//
static PEI_ATA_CONTROLLER_PPI mAtaControllerPpi = {
  EnableAtaChannel,
  GetIdeRegsBaseAddr
};

static EFI_PEI_PPI_DESCRIPTOR mRecoveryPpiList[] = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiBootInRecoveryModePpiGuid,
  NULL
};
  
static EFI_PEI_PPI_DESCRIPTOR mAtaControllerPpiList[] = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiAtaControllerPpiGuid,
  &mAtaControllerPpi
};


static IDE_REGS_BASE_ADDR   mAtaChannelIoSpace[2] = {
  {SB_SATA_PRIMARY_CMD_BAR,   SB_SATA_PRIMARY_CTL_BAR},
  {SB_SATA_SECONDARY_CMD_BAR, SB_SATA_SECONDARY_CTL_BAR},    
};

EFI_STATUS
EnableAtaChannel (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_ATA_CONTROLLER_PPI         *This,
  IN UINT8                          ChannelMask
  )
{
// ChannelMask:
//   PEI_ICH_IDE_PRIMARY
//   PEI_ICH_IDE_SECONDARY
  DEBUG((EFI_D_INFO, "%a(M:%X)\n", __FUNCTION__, ChannelMask));

  return EFI_SUCCESS;
}

UINT32
GetIdeRegsBaseAddr (
  IN  EFI_PEI_SERVICES       **PeiServices,
  IN  PEI_ATA_CONTROLLER_PPI *This,
  OUT IDE_REGS_BASE_ADDR     *IdeRegsBaseAddr
  )
{
  UINT8  Channels;
  UINT8  DefChs;

  Channels = WaitIdeDeviceReady(0, 15, 0, 3000);

  DumpMem32((VOID*)SATA_PCI_REG(0), 256);

  CopyMem(IdeRegsBaseAddr, mAtaChannelIoSpace, sizeof(mAtaChannelIoSpace));
  DefChs = sizeof(mAtaChannelIoSpace) / sizeof(IDE_REGS_BASE_ADDR);
  Channels = MIN(Channels, DefChs);
  DEBUG((EFI_D_INFO, "IDEChannels:%d\n", Channels));
  
  return Channels;
}

EFI_STATUS
EFIAPI
PeimInitializeRecovery (
  IN EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = (*PeiServices)->InstallPpi(PeiServices, &mRecoveryPpiList[0]);
  ASSERT_EFI_ERROR (Status);

  Status = (*PeiServices)->InstallPpi(PeiServices, &mAtaControllerPpiList[0]);
  ASSERT_EFI_ERROR (Status);
  
  BuildDefaultDataHobForRecoveryVariable(PeiServices);

  return Status;
}


