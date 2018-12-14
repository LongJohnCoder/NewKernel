/*++
Copyright (c) 2010 Intel Corporation. All rights reserved.
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  CpuSetup.c

Abstract:

  CPU Setup Routines.
  
Revision History:

--*/

#include <PlatformSetup.h>
#include <AsiaCpuProtocol.h>


STATIC ACPU_MICROCODE_UPDATE_HEADER  gDummyCpuMicroCodeHdr;

UINT32 HexToChar(UINT32 hex_byte){
       
      UINT16 result = 0;
      UINT8   pdata,a[4];
      UINTN   i;

      for(i=0;i<4;i++){
      a[i] = (hex_byte>>(8*i)) & 0xFF;
      if((a[i]>=0x30)&&(a[i]<=0x39))
	  	pdata = a[i]-0x30;
	  else if((a[i]>=0x41)&&(a[i]<=0x46))
	  	pdata = a[i]-0x37;
	  	  else if((a[i]>=0x61)&&(a[i]<=0x66))
	  	pdata = a[i]-0x57;
	  else
	  	pdata=0;
	     result = (result<<4)|pdata;
	}
      return    result;

}

VOID 
InitCpuStrings(
  EFI_HII_HANDLE    HiiHandle, 
  UINT16            Class
)
/*++

Routine Description:

  This function initializes the Acpi related setup option values

Arguments:

  HiiHandle - HiiHandle Handle to HII database
  Class     - Indicates the setup class

Returns:

  VOID

--*/
{
  UINT32                        Cpuid;
  UINT32                        McRev;
  ACPU_MICROCODE_UPDATE_HEADER  *CpuMc; 
  UINT32                        McAddress;	
  
  DEBUG((EFI_D_ERROR, "<InitCpuStrings>"));

  if (Class == ADVANCED_FORM_SET_CLASS) {

    AsmWriteMsr64(0x8B, 0);
    AsmCpuid(1, &Cpuid, NULL, NULL, NULL);
    McRev = (UINT32)RShiftU64(AsmReadMsr64(0x8B), 32);

    McAddress = PcdGet32(PcdCpuMicroCodeAddress);
    if(McAddress){
//YKN-20161107 -S
//      CpuMc = ((ACPU_MICROCODE_UPDATE_HEADER*)(UINTN)McAddress)-1;
      CpuMc = (ACPU_MICROCODE_UPDATE_HEADER*)(UINTN)McAddress;
//YKN-20161107 -E

    } else {
      ZeroMem(&gDummyCpuMicroCodeHdr, sizeof(gDummyCpuMicroCodeHdr));
      CpuMc = &gDummyCpuMicroCodeHdr;
    }    
    
    InitString (
      HiiHandle,
      STRING_TOKEN(STR_CPUID_VALUE), 
      L"%X", 
      Cpuid
      );
    InitString (
      HiiHandle,
      STRING_TOKEN(STR_CPU_MICROCODE_REV_VALUE), 
      L"%08X(%02d/%02d/%04d)", 
      McRev,
      CpuMc->Month,
      CpuMc->Day,
      CpuMc->Year
      );  
#ifdef ZX_SECRET_CODE
    InitString (
      HiiHandle,
      STRING_TOKEN(STR_CPU_MICROCODE_REV_FULL_VALUE), 
      L"%4X%2X", 
       HexToChar(CpuMc->Reserved2),
       ((HexToChar(CpuMc->Reserved3))>>8)&0xFF
      ); 
#endif
//HexCharToUintn(IN CHAR16 Char)
//hex_to_string(unsigned char * buffer, long len)
  }
}
