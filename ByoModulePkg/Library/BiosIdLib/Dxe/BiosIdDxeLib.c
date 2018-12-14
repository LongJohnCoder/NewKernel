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

#include <Library/BiosIdLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>


STATIC CONST UINT8 gBiosidSign[] = BIOS_ID_SIGN_DATA;



EFI_STATUS
GetBiosId (
  OUT BIOS_ID_IMAGE     *BiosIdImage
  )
/*++
Description:

  This function returns BIOS ID by searching HOB or FV.

Arguments:

  BiosIdImage - The BIOS ID got from HOB or FV
  
Returns:

  EFI_SUCCESS - All parameters were valid and BIOS ID has been got.

  EFI_NOT_FOUND - BiosId image is not found, and no parameter will be modified.

  EFI_INVALID_PARAMETER - The parameter is NULL
     
--*/
{
  EFI_STATUS   Status = EFI_NOT_FOUND;
  UINT8        *Data8;
  UINTN        Index;
  UINTN        Count;
  VOID         *p;

  ASSERT(BiosIdImage != NULL);
  
  p = (VOID*)(UINTN)PcdGet64(PcdBiosIdPtr);
  //temp if(p != NULL){
  //temp  CopyMem(BiosIdImage, p, sizeof(BIOS_ID_IMAGE));
  //temp  ASSERT(CompareMem(BiosIdImage->Signature, gBiosidSign, sizeof(gBiosidSign))==0);
  //temp  return EFI_SUCCESS;
  //temp }

  Data8 = (UINT8*)(UINTN)PcdGet32(PcdFlashFvMainBase);
  Count = PcdGet32(PcdFlashFvMainSize);
  
  for(Index=0;Index<Count;Index+=16){
    if(CompareMem(&Data8[Index], gBiosidSign, sizeof(gBiosidSign)) == 0){
      CopyMem(BiosIdImage, &Data8[Index], sizeof(BIOS_ID_IMAGE));
      Status = EFI_SUCCESS;
      p = AllocatePool(sizeof(BIOS_ID_IMAGE));
      ASSERT(p != NULL);
      CopyMem(p, BiosIdImage, sizeof(BIOS_ID_IMAGE));
      PcdSet64(PcdBiosIdPtr, (UINTN)p);
      break;
    }      
  }

  DEBUG((EFI_D_INFO, "%a():%r\n", __FUNCTION__, Status));
  return Status;
}



EFI_STATUS
GetBiosBuildTimeHHMM (
  OUT UINT8     *HH,
  OUT UINT8     *MM
  )
{
  BIOS_ID_IMAGE  BiosIdImage;
  EFI_STATUS     Status;
  UINT16         HhMm;


  Status = GetBiosId(&BiosIdImage);
  if(EFI_ERROR(Status)){
    return Status;
  }

  HhMm = (UINT16)StrDecimalToUintn(BiosIdImage.BiosIdString.TimeStamp + 6);
  *HH = (UINT8)(HhMm / 100);
  *MM = (UINT8)(HhMm % 100);

  return EFI_SUCCESS;  
}


EFI_STATUS
GetImageBiosId (
  IN UINTN                ImageAddress,
  IN OUT BIOS_ID_IMAGE    *BiosIdImage
  )
{
  UINT8                        *Data8;
  UINTN                        Index;
  UINTN                        Count;
  EFI_FIRMWARE_VOLUME_HEADER   *FvHdr;
  EFI_STATUS                   Status = EFI_NOT_FOUND;

  Count = PcdGet32(PcdFlashFvRecoverySize);
  FvHdr = (EFI_FIRMWARE_VOLUME_HEADER*)(ImageAddress + PcdGet32(PcdFlashFvRecoveryBase) - PcdGet32(PcdFlashAreaBaseAddress));
  if(FvHdr->Signature != EFI_FVH_SIGNATURE || FvHdr->FvLength != Count){
    DEBUG((EFI_D_INFO, "Sign:%08X L:%X vs %X\n", FvHdr->Signature, FvHdr->FvLength, Count));
    goto ProcExit;
  }
  
  FvHdr++;
  Data8 = (UINT8*)ALIGN_POINTER(FvHdr, 16);
  for(Index=0;Index<Count;Index+=16){
    if(CompareMem(&Data8[Index], gBiosidSign, sizeof(gBiosidSign)) == 0){
      CopyMem(BiosIdImage, &Data8[Index], sizeof(BIOS_ID_IMAGE));
      Status = EFI_SUCCESS;
      break;
    }      
  }  

ProcExit:
  if(EFI_ERROR(Status)){	
    DEBUG((EFI_D_INFO, __FUNCTION__":%r\n", Status));
  }
  return Status;
}


EFI_STATUS
GetBiosVersionDateTime (
  OUT CHAR16    *BiosVersion, 
  OUT CHAR16    *BiosReleaseDate,
  OUT CHAR16    *BiosReleaseTime OPTIONAL
  )
/*++
Description:

  This function returns the Version & Release date and time by getting and converting
  BIOS ID.

Arguments:

  BiosVersion - The Bios Version out of the conversion

  BiosReleaseDate - The Bios Release Date out of the conversion

  BiosReleaseTime - The Bios Release Time out of the conversion
  
Returns:

  EFI_SUCCESS - All parameters were valid and Version & Release Date have been set.

  EFI_NOT_FOUND - BiosId image is not found, and no parameter will be modified.

  EFI_INVALID_PARAMETER - One of the parameters is NULL
     
--*/
{
  EFI_STATUS    Status;
  BIOS_ID_IMAGE BiosIdImage;
  
  if ((BiosVersion == NULL) || (BiosReleaseDate == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetBiosId (&BiosIdImage);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  
  //
  // Fill the BiosVersion data from the BIOS ID.
  //
  StrCpy (BiosVersion, (CHAR16 *) (&(BiosIdImage.BiosIdString)));
  
  //
  // Fill the build timestamp data from the BIOS ID in the "MM/DD/YY" format.
  //
  
  BiosReleaseDate[0] = BiosIdImage.BiosIdString.TimeStamp[2];
  BiosReleaseDate[1] = BiosIdImage.BiosIdString.TimeStamp[3];
  BiosReleaseDate[2] = (CHAR16) ((UINT8) ('/'));

  BiosReleaseDate[3] = BiosIdImage.BiosIdString.TimeStamp[4];
  BiosReleaseDate[4] = BiosIdImage.BiosIdString.TimeStamp[5];
  BiosReleaseDate[5] = (CHAR16) ((UINT8) ('/'));

  //
  // Add 20 for SMBIOS table
  // Current Linux kernel will misjudge 09 as year 0, so using 2009 for SMBIOS table
  //
  BiosReleaseDate[6] = '2';
  BiosReleaseDate[7] = '0';
  BiosReleaseDate[8] = BiosIdImage.BiosIdString.TimeStamp[0];
  BiosReleaseDate[9] = BiosIdImage.BiosIdString.TimeStamp[1];

  BiosReleaseDate[10] = (CHAR16) ((UINT8) ('\0'));

  if (BiosReleaseTime != NULL) {

    //
    // Fill the build timestamp time from the BIOS ID in the "HH:MM" format.
    //
  
    BiosReleaseTime[0] = BiosIdImage.BiosIdString.TimeStamp[6];
    BiosReleaseTime[1] = BiosIdImage.BiosIdString.TimeStamp[7];
    BiosReleaseTime[2] = (CHAR16) ((UINT8) (':'));

    BiosReleaseTime[3] = BiosIdImage.BiosIdString.TimeStamp[8];
    BiosReleaseTime[4] = BiosIdImage.BiosIdString.TimeStamp[9];

    BiosReleaseTime[5] = (CHAR16) ((UINT8) ('\0'));
  }
  
  return  EFI_SUCCESS;
}
