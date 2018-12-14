/** @file
  Implementation of loading microcode on processors.

  Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "MpLib.h"
#include <Library/IoLib.h>
/**
  Get microcode update signature of currently loaded microcode update.

  @return  Microcode signature.
**/
UINT32
GetCurrentMicrocodeSignature (
  VOID
  )
{
  MSR_IA32_BIOS_SIGN_ID_REGISTER   BiosSignIdMsr;

  AsmWriteMsr64 (MSR_IA32_BIOS_SIGN_ID, 0);
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, NULL);
  BiosSignIdMsr.Uint64 = AsmReadMsr64 (MSR_IA32_BIOS_SIGN_ID);
  return BiosSignIdMsr.Bits.MicrocodeUpdateSignature;
}



VOID PrepareMicroCode(IN CPU_MP_DATA *CpuMpData)
{
  UINT32   McPatchAddr;
  UINT32   McPatchSize;

  if(CpuMpData->MicroCode == NULL){
    McPatchAddr = (UINT32)PcdGet64(PcdCpuMicrocodePatchAddress);
    McPatchSize = (UINT32)PcdGet64(PcdCpuMicrocodePatchRegionSize);
    if(McPatchAddr != 0 && McPatchSize != 0){
      CpuMpData->MicroCode = AllocatePages(EFI_SIZE_TO_PAGES(McPatchSize));
      ASSERT(CpuMpData->MicroCode != NULL);
      CopyMem(CpuMpData->MicroCode, (VOID*)(UINTN)McPatchAddr, McPatchSize);
      DEBUG((EFI_D_INFO, "MicroCode:%X\n", CpuMpData->MicroCode));
    }
  }
}




/**
  Detect whether specified processor can find matching microcode patch and load it.

  @param[in]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
MicrocodeDetect (
  IN CPU_MP_DATA             *CpuMpData
  )
{
  if(CpuMpData->MicroCode != NULL){
    AsmWriteMsr64 (
      MSR_IA32_BIOS_UPDT_TRIG,
      (UINTN)((UINT32)(UINT64)CpuMpData->MicroCode+FixedPcdGet32(PcdMicroCodeHeaderSize))
      );
	ASSERT((AsmReadMsr64(0x1205)&0xFF) == 0x01);
	//YKN-20161104 -s
	  if((AsmReadMsr64(0x1205)&0xFF) != 0x01) {
		IoWrite8(0x80, 0xd3); 
		while(1);
	  }
	//YKN-20161104 -e
    DEBUG((EFI_D_INFO, "Mc:%X\n", GetCurrentMicrocodeSignature()));
  }
}


