/*++

Copyright (c) 2009 - 2010, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DebugMask.c
    
Abstract:

  Installs DebugMask PPI.

--*/

#include "Tiano.h"
#include "PeiCore.h"
#include EFI_PPI_CONSUMER (DebugMask)
#include EFI_PPI_DEPENDENCY (Variable)
#include EFI_GUID_DEFINITION (GlobalVariable)

EFI_STATUS
EFIAPI
DebugMaskNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

static EFI_PEI_NOTIFY_DESCRIPTOR mNotifyList = {
   EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
   &gPeiReadOnlyVariablePpiGuid,
   DebugMaskNotifyCallback
};

VOID
InstallCoreDebugMaskPpi (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_CORE_INSTANCE *OldCoreData
  )
/*++

Routine Description:

  Installs DebugMask PPI.

Arguments:

  PeiServices - The PEI core services table.
  OldCoreData - Pointer to the old core data.
                NULL if being run in non-permament memory mode.
Returns:

  None

--*/
{
  EFI_STATUS                      Status;
  EFI_DEBUG_MASK_PPI              *DebugMaskPpi;
  EFI_PEI_PPI_DESCRIPTOR          *DebugMaskPpiDesc;

  if (OldCoreData == NULL) {
    Status = ((*PeiServices)->AllocatePool) (PeiServices, sizeof (EFI_DEBUG_MASK_PPI), &DebugMaskPpi);
    if (EFI_ERROR(Status) || (DebugMaskPpi == NULL)) {
      return;
    }

    Status = ((*PeiServices)->AllocatePool) (PeiServices, sizeof (EFI_PEI_PPI_DESCRIPTOR), &DebugMaskPpiDesc);
    if (EFI_ERROR(Status) || (DebugMaskPpiDesc == NULL)) {
      return;
    }

    DebugMaskPpiDesc->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
    DebugMaskPpiDesc->Guid  = &gEfiDebugMaskPpiGuid;
    DebugMaskPpiDesc->Ppi   = DebugMaskPpi;

    DebugMaskPpi->ImageDebugMask = gErrorLevel;

    Status = (**PeiServices).InstallPpi (PeiServices, DebugMaskPpiDesc);
    ASSERT_PEI_ERROR (PeiServices, Status);
    if (EFI_ERROR(Status)) {
      return;
    }

    PeiNotifyPpi (PeiServices, &mNotifyList);
  }
  return;
}

EFI_STATUS
EFIAPI
DebugMaskNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
/*++

Routine Description:

  Callback function to update DebugMask value

Arguments:

  PeiServices       - The PEI core services table.
  NotifyDescriptor  - The descriptor for the notification event.
  Ppi               - Pointer to the PPI in question.

Returns:

  EFI_SUCCESS - The function is successfully processed.

--*/
{
  EFI_STATUS                      Status;
  PEI_READ_ONLY_VARIABLE_PPI      *VariableServices;
  UINTN                           DebugMaskSize;
  UINT64                          DebugMask;
  EFI_DEBUG_MASK_PPI              *DebugMaskPpi;

  //
  // Locate Variable Ppi
  //
  Status = (*PeiServices)->LocatePpi (PeiServices, &gPeiReadOnlyVariablePpiGuid, 0, NULL, &VariableServices);

  if (VariableServices) {
    //
    // Get L"EFIDebug" Variable
    //
    DebugMaskSize = sizeof (UINT32);
    Status = VariableServices->PeiGetVariable (
                                PeiServices,
                                L"EFIDebug",
                                &gEfiGlobalVariableGuid,
                                NULL,
                                &DebugMaskSize,
                                &DebugMask
                                );

    if (!EFI_ERROR(Status)) {
      Status = (*PeiServices)->LocatePpi (PeiServices, &gEfiDebugMaskPpiGuid, 0, NULL, &DebugMaskPpi);

      if (!EFI_ERROR(Status)) {
        DebugMaskPpi->ImageDebugMask = (UINTN)DebugMask;
      }
    }
  }

  return Status;
}
