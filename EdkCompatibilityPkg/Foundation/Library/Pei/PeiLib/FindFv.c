/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name:
  
    FindFv.c
   
Abstract:

  Library function to find fv by hob.

--*/

#include "Tiano.h"
#include "Pei.h"
#include "PeiLib.h"
#include "PeiHobLib.h"

static
VOID *
GetHob (
  IN UINT16  Type,
  IN VOID    *HobStart
  )
/*++

Routine Description:

  This function returns the first instance of a HOB type in a HOB list.
  
Arguments:

  Type          The HOB type to return.
  HobStart      The first HOB in the HOB list.
    
Returns:

  HobStart      There were no HOBs found with the requested type.
  else          Returns the first HOB with the matching type.

--*/
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = HobStart;
  //
  // Return input if not found
  //
  if (HobStart == NULL) {
    return HobStart;
  }

  //
  // Parse the HOB list, stop if end of list or matching type found.
  //
  while (!END_OF_HOB_LIST (Hob)) {

    if (Hob.Header->HobType == Type) {
      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  
  //
  // Return input if not found
  //
  if (END_OF_HOB_LIST (Hob)) {
    return HobStart;
  }

  return (VOID *) (Hob.Raw);
}

EFI_STATUS
FindFv (
  IN     EFI_FIND_FV_PPI             *This,
  IN     EFI_PEI_SERVICES            **PeiServices,
  IN OUT UINT8                       *FvNumber,
  IN OUT EFI_FIRMWARE_VOLUME_HEADER  **FvAddress
  )
/*++

Routine Description:

  Search Fv in Hob.

Arguments:
  
  This        - Interface pointer that implement the Find Fv PPI
  
  PeiServices - Pointer to the PEI Service Table
  
  FvNumber    - The index of the fireware volume to locate
  
  FVAddress   - The address of the volume to discover

Returns:

  EFI_SUCCESS           - An addtional fv found
  EFI_OUT_OF_RESOURCES  - There are no fireware volume for given fvnumber
  EFI_INVALID_PARAMETER - *FvAddress is NULL

--*/
{
  EFI_STATUS              Status;
  EFI_PEI_HOB_POINTERS    HobStart;
  EFI_PEI_HOB_POINTERS    Hob;
  EFI_HOB_FIRMWARE_VOLUME *FvHob;
  UINT8                   FvIndex;

  if (*FvAddress == NULL){
    return EFI_INVALID_PARAMETER;
  }

  Hob.Raw = NULL;

  //
  // Get the Hob table pointer
  //
  Status = (*PeiServices)->GetHobList (
                            PeiServices,
                            &HobStart.Raw
                            );

  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Loop to search the wanted FirmwareVolume Hob
  //
  for (FvIndex = 0; FvIndex <= *FvNumber; FvIndex++) {
    Hob.Raw = GetHob (EFI_HOB_TYPE_FV, HobStart.Raw);
    if (Hob.Header->HobType != EFI_HOB_TYPE_FV) {
      *FvNumber = 0;
      return EFI_OUT_OF_RESOURCES;
    }

    HobStart.Raw = Hob.Raw + Hob.Header->HobLength;
  }

  FvHob       = Hob.FirmwareVolume;

  *FvAddress  = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) FvHob->BaseAddress);
  (*FvNumber)++;

  return EFI_SUCCESS;
}
