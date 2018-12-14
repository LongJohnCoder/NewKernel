/*++
  This file contains an 'Intel Pre-EFI Module' and is licensed  
  for Intel CPUs and Chipsets under the terms of your license   
  agreement with Intel or your vendor.  This file may be        
  modified by the user, subject to additional terms of the      
  license agreement                                             
--*/
/** @file
  C funtions in SEC

Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


**/


#include "SecMain.h"


EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI gSecTemporaryRamSupportPpi = {
  SecTemporaryRamSupport
};

EFI_PEI_PPI_DESCRIPTOR            mPeiSecPlatformInformationPpi[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiTemporaryRamSupportPpiGuid,
    &gSecTemporaryRamSupportPpi
  },
};


//
// These are IDT entries pointing to 10:FFFFFFE4h.
//
UINT64  mIdtEntryTemplate = 0xffff8e000010ffe4ULL;

/**
  Caller provided function to be invoked at the end of InitializeDebugAgent().

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param[in] Context    The first input parameter of InitializeDebugAgent().

**/
VOID
EFIAPI
SecStartupPhase2(
  IN VOID                     *Context
  );

EFI_STATUS
EFIAPI
PlatformSecLibConstructor (
  );

/**

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.


  @param SizeOfRam           Size of the temporary memory available for use.
  @param TempRamBase         Base address of tempory ram
  @param BootFirmwareVolume  Base address of the Boot Firmware Volume.
**/
VOID
EFIAPI
SecStartup (
  IN UINT32                   SizeOfRam,
  IN UINT32                   TempRamBase,
  IN VOID                     *BootFirmwareVolume
  )
{
  EFI_SEC_PEI_HAND_OFF        SecCoreData;
  IA32_DESCRIPTOR             IdtDescriptor;
  SEC_IDT_TABLE               IdtTableInStack;
  UINT32                      Index;
  UINT32                      PeiStackSize;

  PeiStackSize = PcdGet32 (PcdPeiTemporaryRamStackSize);
  if (PeiStackSize == 0) {
    PeiStackSize = (SizeOfRam >> 1);
  }

  ASSERT (PeiStackSize < SizeOfRam);

  //
  // Process all libraries constructor function linked to SecCore.
  //
  ProcessLibraryConstructorList ();
  PlatformSecLibConstructor();

  //
  // Initialize floating point operating environment
  // to be compliant with UEFI spec.
  //
  InitializeFloatingPointUnits ();


  // |-------------------|---->
  // |Idt Table          |
  // |-------------------|
  // |PeiService Pointer |    PeiStackSize
  // |-------------------|
  // |                   |
  // |      Stack        |
  // |-------------------|---->
  // |                   |
  // |                   |
  // |      Heap         |    PeiTemporayRamSize
  // |                   |
  // |                   |
  // |-------------------|---->  TempRamBase

  IdtTableInStack.PeiService = 0;
  for (Index = 0; Index < SEC_IDT_ENTRY_COUNT; Index ++) {
    CopyMem ((VOID*)&IdtTableInStack.IdtTable[Index], (VOID*)&mIdtEntryTemplate, sizeof (UINT64));
  }

  IdtDescriptor.Base  = (UINTN) &IdtTableInStack.IdtTable;
  IdtDescriptor.Limit = (UINT16)(sizeof (IdtTableInStack.IdtTable) - 1);

  AsmWriteIdtr (&IdtDescriptor);

  //
  // Update the base address and length of Pei temporary memory
  //
  SecCoreData.DataSize               = (UINT16) sizeof (EFI_SEC_PEI_HAND_OFF);
  SecCoreData.BootFirmwareVolumeBase = BootFirmwareVolume;
  SecCoreData.BootFirmwareVolumeSize = (UINTN)(0x100000000ULL - (UINTN) BootFirmwareVolume);
  SecCoreData.TemporaryRamBase       = (VOID*)(UINTN) TempRamBase;
  SecCoreData.TemporaryRamSize       = SizeOfRam;
  SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize    = SizeOfRam - PeiStackSize;
  SecCoreData.StackBase              = (VOID*)(UINTN)(TempRamBase + SecCoreData.PeiTemporaryRamSize);
  SecCoreData.StackSize              = PeiStackSize;

  //
  // Initialize Debug Agent to support source level debug in SEC/PEI phases before memory ready.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_PREMEM_SEC, &SecCoreData, SecStartupPhase2);

}

/**
  Caller provided function to be invoked at the end of InitializeDebugAgent().

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param[in] Context    The first input parameter of InitializeDebugAgent().

**/
VOID
EFIAPI
SecStartupPhase2(
  IN VOID                     *Context
  )
{
  EFI_SEC_PEI_HAND_OFF        *SecCoreData;
  EFI_PEI_PPI_DESCRIPTOR      *PpiList;
  UINT32                      Index;
  EFI_PEI_PPI_DESCRIPTOR      AllSecPpiList[FixedPcdGet32(PcdSecCoreMaxPpiSupported)];
  EFI_PEI_CORE_ENTRY_POINT    PeiCoreEntryPoint;

  SecCoreData = (EFI_SEC_PEI_HAND_OFF *) Context;
  //
  // Find Pei Core entry point. It will report SEC and Pei Core debug information if remote debug
  // is enabled.
  //
  FindAndReportEntryPoints ((EFI_FIRMWARE_VOLUME_HEADER *) SecCoreData->BootFirmwareVolumeBase, &PeiCoreEntryPoint);
  if (PeiCoreEntryPoint == NULL){
    CpuDeadLoop ();
  }

  //
  // Perform platform specific initialization before entering PeiCore.
  //
  PpiList = SecPlatformMain (SecCoreData);
  if (PpiList != NULL) {
    //
    // Remove the terminal flag from the terminal Ppi
    //
    CopyMem (AllSecPpiList, mPeiSecPlatformInformationPpi, sizeof (mPeiSecPlatformInformationPpi));
    for (Index = 0; Index < PcdGet32 (PcdSecCoreMaxPpiSupported); Index ++) {
      if ((AllSecPpiList[Index].Flags & EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) == EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) {
        break;
      }
    }
    AllSecPpiList[Index].Flags = AllSecPpiList[Index].Flags & (~EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);

    //
    // Append the platform additional Ppi list
    //
    Index += 1;
    while (Index < PcdGet32 (PcdSecCoreMaxPpiSupported) &&
           ((PpiList->Flags & EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) != EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST)) {
      CopyMem (&AllSecPpiList[Index], PpiList, sizeof (EFI_PEI_PPI_DESCRIPTOR));
      Index++;
      PpiList++;
    }

    //
    // Check whether the total Ppis exceeds the max supported Ppi.
    //
    if (Index >= PcdGet32 (PcdSecCoreMaxPpiSupported)) {
      //
      // the total Ppi is larger than the supported Max
      // PcdSecCoreMaxPpiSupported can be enlarged to solve it.
      //
      CpuDeadLoop ();
    } else {
      //
      // Add the terminal Ppi
      //
      CopyMem (&AllSecPpiList[Index], PpiList, sizeof (EFI_PEI_PPI_DESCRIPTOR));
    }

    //
    // Set PpiList to the total Ppi
    //
    PpiList = &AllSecPpiList[0];
  } else {
    //
    // No addition Ppi, PpiList directly point to the common Ppi list.
    //
    PpiList = &mPeiSecPlatformInformationPpi[0];
  }

  //
  // Transfer the control to the PEI core
  //
  ASSERT (PeiCoreEntryPoint != NULL);
  (*PeiCoreEntryPoint) (SecCoreData, PpiList);

  //
  // Should not come here.
  //
  return ;
}

/**
  This service of the TEMPORARY_RAM_SUPPORT_PPI that migrates temporary RAM into
  permanent memory.

  @param PeiServices            Pointer to the PEI Services Table.
  @param TemporaryMemoryBase    Source Address in temporary memory from which the SEC or PEIM will copy the
                                Temporary RAM contents.
  @param PermanentMemoryBase    Destination Address in permanent memory into which the SEC or PEIM will copy the
                                Temporary RAM contents.
  @param CopySize               Amount of memory to migrate from temporary to permanent memory.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_INVALID_PARAMETER PermanentMemoryBase + CopySize > TemporaryMemoryBase when
                                TemporaryMemoryBase > PermanentMemoryBase.

**/
EFI_STATUS
EFIAPI
SecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  )
{
  IA32_DESCRIPTOR   IdtDescriptor;
  VOID*             OldHeap;
  VOID*             NewHeap;
  VOID*             OldStack;
  VOID*             NewStack;
  DEBUG_AGENT_CONTEXT_POSTMEM_SEC  DebugAgentContext;
  BOOLEAN           OldStatus;
  UINTN             PeiStackSize;

  PeiStackSize = (UINTN)PcdGet32 (PcdPeiTemporaryRamStackSize);
  if (PeiStackSize == 0) {
    PeiStackSize = (CopySize >> 1);
  }

  ASSERT (PeiStackSize < CopySize);

  //
  // |-------------------|---->
  // |      Stack        |    PeiStackSize
  // |-------------------|---->
  // |      Heap         |    PeiTemporayRamSize
  // |-------------------|---->  TempRamBase
  //
  // |-------------------|---->
  // |      Heap         |    PeiTemporayRamSize
  // |-------------------|---->
  // |      Stack        |    PeiStackSize
  // |-------------------|---->  PermanentMemoryBase
  //

  OldHeap = (VOID*)(UINTN)TemporaryMemoryBase;
  NewHeap = (VOID*)((UINTN)PermanentMemoryBase + PeiStackSize);
  
  OldStack = (VOID*)((UINTN)TemporaryMemoryBase + CopySize - PeiStackSize);
  NewStack = (VOID*)(UINTN)PermanentMemoryBase;

  DebugAgentContext.HeapMigrateOffset = (UINTN)NewHeap - (UINTN)OldHeap;
  DebugAgentContext.StackMigrateOffset = (UINTN)NewStack - (UINTN)OldStack;
  
  OldStatus = SaveAndSetDebugTimerInterrupt (FALSE);
  //
  // Initialize Debug Agent to support source level debug in PEI phase after memory ready.
  // It will build HOB and fix up the pointer in IDT table.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, (VOID *) &DebugAgentContext, NULL);

  //
  // Migrate Heap
  //
  CopyMem (NewHeap, OldHeap, CopySize - PeiStackSize);

  //
  // Migrate Stack
  //
  CopyMem (NewStack, OldStack, PeiStackSize);


  //
  // We need *not* fix the return address because currently,
  // The PeiCore is executed in flash.
  //

  //
  // Rebase IDT table in permanent memory
  //
  AsmReadIdtr (&IdtDescriptor);
  IdtDescriptor.Base = IdtDescriptor.Base - (UINTN)OldStack + (UINTN)NewStack;

  AsmWriteIdtr (&IdtDescriptor);


  //
  // Program MTRR
  //

  //
  // SecSwitchStack function must be invoked after the memory migration
  // immediatly, also we need fixup the stack change caused by new call into
  // permenent memory.
  //
  SecSwitchStack (
    (UINT32) (UINTN) OldStack,
    (UINT32) (UINTN) NewStack
    );

  SaveAndSetDebugTimerInterrupt (OldStatus);
  
  return EFI_SUCCESS;
}
