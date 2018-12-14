/*++

Copyright (c) 2004 - 2005, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Security.c

Abstract:

  EFI PEI Core Security services

--*/

#include "Tiano.h"
#include "PeiCore.h"
#include EFI_PPI_DEFINITION (Security)

EFI_STATUS
EFIAPI
SecurityPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

static EFI_PEI_NOTIFY_DESCRIPTOR mNotifyList = {
   EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
   &gPeiSecurityPpiGuid,
   SecurityPpiNotifyCallback
};

VOID
InitializeSecurityServices (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_CORE_INSTANCE *OldCoreData
  )
/*++

Routine Description:

  Initialize the security services.

Arguments:

  PeiServices - The PEI core services table.
  OldCoreData - Pointer to the old core data.
                NULL if being run in non-permament memory mode.
Returns:

  None

--*/
{
  if (OldCoreData == NULL) {
    PeiNotifyPpi (PeiServices, &mNotifyList);
  }
  return;
}

EFI_STATUS
EFIAPI
SecurityPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
/*++

Routine Description:

  Provide a callback for when the security PPI is installed.

Arguments:

  PeiServices       - The PEI core services table.
  NotifyDescriptor  - The descriptor for the notification event.
  Ppi               - Pointer to the PPI in question.

Returns:

  EFI_SUCCESS - The function is successfully processed.

--*/
{
  PEI_CORE_INSTANCE                       *PrivateData;

  //
  // Get PEI Core private data
  //
  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);
  
  //
  // If there isn't a security PPI installed, use the one from notification
  //
  if (PrivateData->PrivateSecurityPpi == NULL) {
    PrivateData->PrivateSecurityPpi = (PEI_SECURITY_PPI *)Ppi;
  }
  return EFI_SUCCESS;
}

EFI_STATUS
VerifyPeim (
  IN EFI_PEI_SERVICES     **PeiServices,
  IN EFI_FFS_FILE_HEADER  *CurrentPeimAddress
  )
/*++

Routine Description:

  Provide a callout to the security verification service.

Arguments:

  PeiServices          - The PEI core services table.
  CurrentPeimAddress   - Pointer to the Firmware File under investigation.

Returns:

  EFI_SUCCESS             - Image is OK
  EFI_SECURITY_VIOLATION  - Image is illegal

--*/
{
  PEI_CORE_INSTANCE               *PrivateData;
  EFI_STATUS                      Status;
  UINT32                          AuthenticationStatus;
  BOOLEAN                         StartCrisisRecovery;

  //
  // Set a default authentication state
  //
  AuthenticationStatus = 0;

  //
  // get security PPI instance from PEI private data
  //
  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);

  if (PrivateData->PrivateSecurityPpi == NULL) {
    Status = EFI_NOT_FOUND;
  } else {
    //
    // Check to see if the image is OK
    //
    Status = PrivateData->PrivateSecurityPpi->AuthenticationState (
                                                PeiServices,
                                                PrivateData->PrivateSecurityPpi,
                                                AuthenticationStatus,
                                                CurrentPeimAddress,
                                                &StartCrisisRecovery
                                                );
    if (StartCrisisRecovery) {
      Status = EFI_SECURITY_VIOLATION;
    }
  }
  return Status;
}


EFI_STATUS
VerifyFv (
  IN EFI_FIRMWARE_VOLUME_HEADER  *CurrentFvAddress
  )
/*++

Routine Description:

  Verify a Firmware volume

Arguments:

  CurrentFvAddress - Pointer to the current Firmware Volume under consideration

Returns:

  EFI_SUCCESS             - Firmware Volume is legal
  EFI_SECURITY_VIOLATION  - Firmware Volume fails integrity test

--*/
{
  //
  // Right now just pass the test.  Future can authenticate and/or check the
  // FV-header or other metric for goodness of binary.
  //
  return EFI_SUCCESS;
}
