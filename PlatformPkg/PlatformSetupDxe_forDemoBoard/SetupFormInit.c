/*++

Copyright (c) 2010 - 2015, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:

Abstract:
  Platform configuration setup.

Revision History:


--*/
#include "PlatformSetupDxe.h"
#include "CpuSetup.h"
#include <Protocol/DiskInfo.h>
#include <IndustryStandard/Atapi.h>
#include "SetupItemId.h"
#include <SetupVariable.h>
#include <Library/PlatformCommLib.h>
#include <Library/PciLib.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <AsiaCpuProtocol.h>
#include <AsiaNbProtocol.h>
#include <AsiaSbProtocol.h>
#include <CHX002Cfg.h>
#include <CHX002/GENERAL.h>
#include <Protocol/BdsBootManagerProtocol.h>
#include <Protocol/PlatHostInfoProtocol.h>
#include <Protocol/Smbios.h>
#include <Library/BiosIdLib.h>
#include <Protocol/SystemPasswordProtocol.h>
#include <library/ByoCommLib.h>



#define _48_BIT_ADDRESS_FEATURE_SET_SUPPORTED   0x0400
#define ATAPI_DEVICE                            0x8000

CHAR8* Iso6392LanguageList[] = {"eng",  "zho", "uqi", NULL}; 
CHAR8* Rfc4646LanguageList[] = {"en-US", "zh-Hans", "uqi", NULL}; 

CHAR16            *mUnknownString = L"!";
EFI_STRING_ID     *gLanguageToken = NULL;
UINTN             gDefaultLanguageOption = 0;
 

VOID 
InitString (
  EFI_HII_HANDLE    HiiHandle, 
  EFI_STRING_ID     StrRef, 
  CHAR16            *sFormat, ...
  )
{
  STATIC CHAR16 s[1024];
  VA_LIST  Marker;

  VA_START (Marker, sFormat);
  UnicodeVSPrint (s, sizeof (s),  sFormat, Marker);
  VA_END (Marker);
    
  HiiSetString (HiiHandle, StrRef, s, NULL);
}

CHAR16 *
GetToken (
  IN  EFI_STRING_ID                Token,
  IN  EFI_HII_HANDLE               HiiHandle
  )
{
  EFI_STRING  String;

  if (HiiHandle == NULL) {
    return NULL;
  }

  String = HiiGetString (HiiHandle, Token, NULL);
  if (String == NULL) {
    String = AllocateCopyPool (sizeof (L"!"), L"!");
    ASSERT (String != NULL);
  }
  return (CHAR16 *) String;
}









VOID
UpdateAdvCpuInfo (
  EFI_HII_HANDLE HiiHandle
  )
{
  EFI_ASIA_CPU_PROTOCOL  *PtAsiaCpu;
  ACPU_SPEED_INFO        CpuSpeedInfo;
  CHAR8                  BrandString[48+1];
  UINT32                 IntendedFreq;
  UINT8                  NumCores;
  UINT8                  NumClusters;
  EFI_STATUS             Status;
  UINT16                 Value1;
  UINT16                 Value2;
  EFI_PEI_HOB_POINTERS	 GuidHob;
  ASIA_CPU_CONFIGURATION *CpuFeature;  
  UINT16                 CacheSize[3];     // unit in KB
  ACPU_CACHE_INFO        *CacheInfo; 
  UINT32                 CPUBusSpeed;
  ACPU_MICROCODE_UPDATE_HEADER  *CpuMc; 
  UINT32                        Cpuid;
  UINT32                        McAddress;	
  PLAT_HOST_INFO_PROTOCOL       *ptHostInfo;  


  Status = gBS->LocateProtocol(&gPlatHostInfoProtocolGuid, NULL, &ptHostInfo);
  ASSERT(!EFI_ERROR(Status));
  
  GuidHob.Raw = GetFirstGuidHob(&gAsiaCpuCfgHobGuid);
  ASSERT(GuidHob.Raw!=NULL);
  CpuFeature = (ASIA_CPU_CONFIGURATION*)(GuidHob.Guid+1); 

  if(CpuFeature->VrmSupport&BIT4){  // VRM-3
    InitString(
      HiiHandle,
      STRING_TOKEN(STR_VRM_VALUE), 
      L"SVID %d-Phase", 
      CpuFeature->VrmSupport & 0xF
      );
  } else {
    InitString(
      HiiHandle,
      STRING_TOKEN(STR_VRM_VALUE), 
      L"PVID %d-Phase", 
      CpuFeature->VrmSupport & 0xF
      );
  }

  Status = gBS->LocateProtocol(&gAsiaCpuProtocolGuid, NULL, (VOID**)&PtAsiaCpu);
  ASSERT_EFI_ERROR(Status);

  PtAsiaCpu->GetCpuBrandString(BrandString);
  PtAsiaCpu->GetCpuSpeedInfo( &CpuSpeedInfo);
  NumCores = PtAsiaCpu->GetCpuCores();
  NumClusters=PtAsiaCpu->GetCpuClusters();

  DEBUG((EFI_D_INFO, "CPUFreq:%d * %d\n", CpuSpeedInfo.CPUBusSpeed, CpuSpeedInfo.CurrentBusRatio));

  IntendedFreq = (CpuSpeedInfo.CPUBusSpeed * CpuSpeedInfo.CurrentBusRatio) / 2;
  IntendedFreq = ((IntendedFreq + 5) / 10) * 10;
  Value1 = (UINT16)(IntendedFreq / 1000);
  Value2 = (UINT16)(IntendedFreq % 1000);
  Value2 = Value2/10;

  InitString(
    HiiHandle,
    STRING_TOKEN(STR_PROCESSOR_VERSION_VALUE), 
    L"%a", 
    BrandString
    );

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_PROCESSOR_SPEED_VALUE),
    L"%d.%02d GHz",
    Value1,
    Value2
    );

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_PROCESSOR_CORE_COUNT_VALUE),
    L"%d",
    NumCores
    );

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_PROCESSOR_CLUSTER_COUNT_VALUE),
    L"%d",
    NumClusters
    );

  CacheInfo = NULL;
  CacheInfo = AllocatePool(3*sizeof(ACPU_CACHE_INFO));
  ASSERT(CacheInfo != NULL);
  PtAsiaCpu->GetCpuCacheInfo(CacheInfo);

  CacheSize[0] = CacheInfo[0].CacheSize;
  CacheSize[1] = CacheInfo[1].CacheSize;
  CacheSize[2] = CacheInfo[2].CacheSize;

  InitString(
    HiiHandle,
    STRING_TOKEN(STR_CACHE_L1I_SIZE_VALUE), 
    L"%d KB", 
    CacheSize[0]
    );

  InitString(
    HiiHandle,
    STRING_TOKEN(STR_CACHE_L1D_SIZE_VALUE), 
    L"%d KB", 
    CacheSize[1]
    );

  InitString(
    HiiHandle,
    STRING_TOKEN(STR_CACHE_L2_SIZE_VALUE), 
    L"%d KB", 
    CacheSize[2]
    );

  CPUBusSpeed=CpuSpeedInfo.CPUBusSpeed;
  InitString(
    HiiHandle,
    STRING_TOKEN(STR_CURRENT_CPUBUS_FREQ_VALUE), 
    L"%d MHz", 
    CPUBusSpeed
    );


  AsmCpuid(1, &Cpuid, NULL, NULL, NULL);
  McAddress = (UINT32)PcdGet64(PcdCpuMicrocodePatchAddress);
  CpuMc = (ACPU_MICROCODE_UPDATE_HEADER*)(UINTN)McAddress;
    
  InitString (
    HiiHandle,
    STRING_TOKEN(STR_CPUID_VALUE), 
    L"%X", 
    Cpuid
    );
  
  InitString (
    HiiHandle,
    STRING_TOKEN(STR_CPU_MICROCODE_REV_VALUE), 
    L"%08lX(%02d/%02d/%04d)", 
    ptHostInfo->CpuMcVer,
    CpuMc->Month,
    CpuMc->Day,
    CpuMc->Year
    );    
  
}


STATIC EFI_STRING_ID gDimmSlotStrList[] = {
  STRING_TOKEN(STR_MEMORY_SLOT0_VALUE),
  STRING_TOKEN(STR_MEMORY_SLOT1_VALUE), 
  STRING_TOKEN(STR_MEMORY_SLOT2_VALUE), 
  STRING_TOKEN(STR_MEMORY_SLOT3_VALUE),  
};

#define DIMM_SLOT_STRLIST_COUNT   (sizeof(gDimmSlotStrList)/sizeof(gDimmSlotStrList[0]))


VOID
UpdateDramInfo(
  EFI_HII_HANDLE  HiiHandle
  )
{
  PLAT_DIMM_INFO       *DimmInfo;
  UINTN                Index;


  DimmInfo = GetPlatformDimmInfo();

  InitString(
    HiiHandle,
    STRING_TOKEN(STR_DRAM_CL_VALUE), 
    L"%d", 
    DimmInfo->DramCL
    );

  InitString(
    HiiHandle,
    STRING_TOKEN(STR_DRAMTRP_VALUE), 
    L"%d", 
    DimmInfo->DramTrp
    );

  InitString(
    HiiHandle,
    STRING_TOKEN(STR_DRAMTRCD_VALUE), 
    L"%d", 
    DimmInfo->DramTrcd
    );

  InitString(
    HiiHandle,
    STRING_TOKEN(STR_DRAMTRAS_VALUE), 
    L"%d", 
    DimmInfo->DramTras
    );

	for(Index=0;Index<DimmInfo->DimmCount;Index++){

    if(Index >= DIMM_SLOT_STRLIST_COUNT){
      DEBUG((EFI_D_ERROR, "[ERROR] gDimmSlotStrList is too small!"));
      break;
    }
    
	  if(DimmInfo->SpdInfo[Index].DimmSize){
      InitString(
        HiiHandle,
        gDimmSlotStrList[Index], 
        L"%d MB (DDR4)", 
        DimmInfo->SpdInfo[Index].DimmSize
        );
	  }
    
  }
  
  InitString(
    HiiHandle,
    STRING_TOKEN(STR_TOTAL_MEMORY_VALUE), 
    L"%d MB", 
    DimmInfo->DimmTotalSizeMB
    );

  InitString(
    HiiHandle,
    STRING_TOKEN(STR_DRAM_CURRENT_FREQ_VALUE), 
    L"%d MHz", 
    DimmInfo->DimmFreq
    );
  
}




EFI_STATUS GetSataHostDp(UINTN HostIndex, EFI_DEVICE_PATH_PROTOCOL **HostDp)
{
  EFI_STATUS                Status;
  PLAT_HOST_INFO_PROTOCOL   *ptHostInfo;  
  UINTN                     Index;
  UINTN                     SataHostIndex = 0;


  Status = gBS->LocateProtocol(&gPlatHostInfoProtocolGuid, NULL, &ptHostInfo);
  ASSERT(!EFI_ERROR(Status));

  for(Index=0;Index<ptHostInfo->HostCount;Index++){
    if(ptHostInfo->HostList[Index].HostType == PLATFORM_HOST_SATA){
      if(SataHostIndex == HostIndex){
        *HostDp = ptHostInfo->HostList[Index].Dp;
        return EFI_SUCCESS;
      }
      SataHostIndex++;
    }
  }

  return EFI_NOT_FOUND;  
}



STATIC EFI_STRING_ID gObSataStrList[] = {
  STRING_TOKEN(STR_SATA_DRIVE0_VALUE),
  STRING_TOKEN(STR_SATA_DRIVE1_VALUE), 
};

STATIC EFI_STRING_ID gIoeSataStrList[] = {
  STRING_TOKEN(STR_IOE_SATA_DRIVE0_VALUE),
  STRING_TOKEN(STR_IOE_SATA_DRIVE1_VALUE), 
  STRING_TOKEN(STR_IOE_SATA_DRIVE2_VALUE), 
  STRING_TOKEN(STR_IOE_SATA_DRIVE3_VALUE),  
};

#define OB_SATA_DEV_STRLIST_COUNT   (sizeof(gObSataStrList)/sizeof(gObSataStrList[0]))
#define IOE_SATA_DEV_STRLIST_COUNT   (sizeof(gIoeSataStrList)/sizeof(gIoeSataStrList[0]))


VOID
UpdateSataPortInfo (
  EFI_HII_HANDLE         HiiHandle
  )
{
  UINTN                           Index;
  EFI_STATUS                      Status;
  EFI_DISK_INFO_PROTOCOL          *DiskInfo;
  UINT32                          BufferSize;
  ATA_IDENTIFY_DATA               *IdentifyData = NULL;
  UINTN                           HandleCount;
  EFI_HANDLE                      *HandleBuffer=NULL;
  CHAR8                           *String = NULL;
  CHAR8                           *ModelNumber = NULL;
  UINT32                          SataPortIndex, IdeChannel;
  UINT32                          PortIndex;
  UINT64                          NumSectors = 0; 
  UINT64                          DriveSizeInBytes = 0;
  UINT64                          RemainderInBytes = 0;
  UINT32                          DriveSizeInGB = 0;
  UINT32                          NumTenthsOfGB = 0; 
  EFI_DEVICE_PATH_PROTOCOL        *Dp = NULL;
  EFI_DEVICE_PATH_PROTOCOL        *DevPath;
  UINTN                           SataHostIndex = 0;
  UINT32                          PortIndex_Len=0;
  

  DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));
  //Gary 2018-11-22 Modify the IOE-Sata and Onborad Sata display issue.
  for(SataHostIndex=0;SataHostIndex<2;SataHostIndex++){

  Status = GetSataHostDp(SataHostIndex, &Dp);
  if(EFI_ERROR(Status)){
    continue;
  }  
  
  String = AllocatePool(64);
  ASSERT(String != NULL);
  ModelNumber = AllocatePool(40+1);
  ModelNumber[40] = 0;
  ASSERT (ModelNumber != NULL);  
  IdentifyData = AllocatePool(sizeof(ATA_IDENTIFY_DATA));
  ASSERT (IdentifyData != NULL);  

  Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiDiskInfoProtocolGuid,
                    NULL,
                    &HandleCount,
                    &HandleBuffer
                    );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  

  DEBUG((EFI_D_INFO, "DiskInfoCount:%d\n", HandleCount));
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID**)&DevPath
                    );
    if(EFI_ERROR(Status)){
      continue;
    }

    if(CompareMem(Dp, DevPath, GetDevicePathSize(Dp)-4)!=0){
      DEBUG((EFI_D_INFO, "DP not match\n"));
      continue;
    }  

    Status = gBS->HandleProtocol(
                    HandleBuffer[Index],
                    &gEfiDiskInfoProtocolGuid,
                    &DiskInfo
                    );
    ASSERT_EFI_ERROR (Status);
  
    Status = DiskInfo->WhichIde (
                         DiskInfo,
                         &IdeChannel,
                         &SataPortIndex
                         );
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR,"No Disk!\n"));
      continue;
    }

    if(CompareGuid(&DiskInfo->Interface, &gEfiDiskInfoIdeInterfaceGuid)){
      PortIndex = IdeChannel*2 + SataPortIndex;
    } else if(CompareGuid(&DiskInfo->Interface, &gEfiDiskInfoAhciInterfaceGuid)){
      PortIndex = IdeChannel;
    } else {
      continue;
    }
    DEBUG((EFI_D_INFO, "PortIndex:%d\n", PortIndex));
	if(SataHostIndex==0){
		PortIndex_Len=OB_SATA_DEV_STRLIST_COUNT;
    }
	else{
	   PortIndex_Len=IOE_SATA_DEV_STRLIST_COUNT;
	}
	if(PortIndex >= PortIndex_Len){
		DEBUG((EFI_D_INFO, "[ERROR] PortIndex out of range\n"));
		continue;
	}	
    
    BufferSize = sizeof(ATA_IDENTIFY_DATA);
    Status = DiskInfo->Identify (
                         DiskInfo,
                         IdentifyData,
                         &BufferSize
                         );
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR,"Identify failed!\n"));
      continue;
    }

    CopyMem (ModelNumber, IdentifyData->ModelName, 40);
    SwapWordArray(ModelNumber, 40);
    TrimStr8(ModelNumber);
     
    if ((!(IdentifyData->config & ATAPI_DEVICE)) || (IdentifyData->config == 0x848A)) {
      if (IdentifyData->command_set_supported_83 & _48_BIT_ADDRESS_FEATURE_SET_SUPPORTED) { 
        NumSectors = *(UINT64 *)&IdentifyData->maximum_lba_for_48bit_addressing; 
      } else {
        NumSectors = IdentifyData->user_addressable_sectors_lo + (IdentifyData->user_addressable_sectors_hi << 16) ; 
      }
      DriveSizeInBytes = MultU64x32 (NumSectors, 512); 

      //
      // DriveSizeInGB is DriveSizeInBytes / 1 GB (1 Binary GB = 2^30 bytes)
      // DriveSizeInGB = (UINT32) Div64(DriveSizeInBytes, (1 << 30), &RemainderInBytes); 
      // Convert the Remainder, which is in bytes, to number of tenths of a Binary GB.
      // NumTenthsOfGB = GetNumTenthsOfGB(RemainderInBytes); 
      // DriveSizeInGB is DriveSizeInBytes / 1 GB (1 Decimal GB = 10^9 bytes)
      //
      DriveSizeInGB = (UINT32) DivU64x64Remainder (DriveSizeInBytes, 1000000000, &RemainderInBytes);  

      //
      // Convert the Remainder, which is in bytes, to number of tenths of a Decimal GB.
      //
      NumTenthsOfGB = (UINT32) DivU64x64Remainder (RemainderInBytes, 100000000, NULL);       
      AsciiSPrint (String, 64, "HDD %a %d.%dGB", ModelNumber, DriveSizeInGB, NumTenthsOfGB);
    } else {
      AsciiSPrint(String, 64, "ODD %a", ModelNumber);
    } 
    if(SataHostIndex==0){
      InitString(
      HiiHandle,
      gObSataStrList[PortIndex], 
      L"%a", 
      String
      );
    }
	else{
	  InitString(
      HiiHandle,
      gIoeSataStrList[PortIndex], 
      L"%a", 
      String
      );
	}
		
  }
  }


ProcExit:
  if (HandleBuffer != NULL){
    FreePool(HandleBuffer);
  }
  if(ModelNumber!=NULL){FreePool(ModelNumber);}
  if(String!=NULL){FreePool(String);}
  if(IdentifyData!=NULL){FreePool(IdentifyData);}
}



STATIC UINT16 SmbiosGetStrOffset(CONST SMBIOS_STRUCTURE *Hdr, UINT8 Index)
{
  CONST UINT8 *pData8;
  UINT8       i;

  if(Index == 0){return 0;}
  
  pData8  = (UINT8*)Hdr;
  pData8 += Hdr->Length;
  
  i = 1;
  while(i != Index){
    while(*pData8!=0){pData8++;}

    if(pData8[1] == 0){     // if next byte of a string end is NULL, type end.
      break;
    }
    
    pData8++;
    i++;
  }
  if(i == Index){
    return (UINT16)(pData8 - (UINT8*)Hdr);
  } else {
    return 0;
  }
}


STATIC
CHAR8 * 
SmbiosGetStringInTypeByIndex(
  SMBIOS_STRUCTURE_POINTER Hdr, 
  SMBIOS_TABLE_STRING      StrIndex
  )
{
  CHAR8  *Str8;
  if(StrIndex == 0){
    return "";
  }
  Str8 = (CHAR8*)(Hdr.Raw + SmbiosGetStrOffset(Hdr.Hdr, StrIndex));
  return Str8;
}



VOID
UpdateCpuInfo (
  EFI_HII_HANDLE            HiiHandle,
  EFI_SMBIOS_PROTOCOL       *Smbios
  )
{
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  EFI_SMBIOS_TYPE           SmbiosType;
  EFI_SMBIOS_TABLE_HEADER   *SmbiosHdr;
  SMBIOS_STRUCTURE_POINTER  p;
  EFI_STATUS                Status;
  CHAR8                     *CpuName;
  CHAR8                     *CpuNameNew = NULL; 
  UINT16                    CurSpeed;
  UINT16                    CoreCount;
  UINT16                    ThreadCount;
  PLAT_HOST_INFO_PROTOCOL   *ptHostInfo;  


  Status = gBS->LocateProtocol(&gPlatHostInfoProtocolGuid, NULL, &ptHostInfo);
  ASSERT(!EFI_ERROR(Status));

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  SmbiosType = EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION;
  Status = Smbios->GetNext(Smbios, &SmbiosHandle, &SmbiosType, &SmbiosHdr, NULL);
  ASSERT(!EFI_ERROR(Status));
  p.Hdr = SmbiosHdr;
  CpuName = SmbiosGetStringInTypeByIndex(p, p.Type4->ProcessorVersion);
  CurSpeed = p.Type4->CurrentSpeed;
  CoreCount = p.Type4->CoreCount;
  ThreadCount = p.Type4->ThreadCount;
  DEBUG((EFI_D_INFO, "CpuName:[%a]\n", CpuName));

  CpuNameNew = AllocatePool(AsciiStrSize(CpuName));
  ASSERT(CpuNameNew != NULL);
  AsciiStrCpy(CpuNameNew, CpuName);
  TrimStr8(CpuNameNew);

  while(1){
    Status = Smbios->GetNext(Smbios, &SmbiosHandle, &SmbiosType, &SmbiosHdr, NULL);
    if(EFI_ERROR(Status)){
      break;
    }
    p.Hdr = SmbiosHdr;
    CoreCount += p.Type4->CoreCount;
    ThreadCount += p.Type4->ThreadCount;
  }

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_PROCESSOR_VERSION_VALUE), 
    L"%a", 
    CpuNameNew
  );

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_PROCESSOR_SPEED_VALUE),
    L"%d MHz",
    CurSpeed
  );

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_PROCESSOR_CORE_COUNT_VALUE),
    L"%d Cores %d Threads",
    CoreCount,
    ThreadCount
  );  

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_PROCESSOR_MICROCODE_VALUE),
    L"0x%lX",
    ptHostInfo->CpuMcVer
  );


  if(CpuNameNew != NULL){
    FreePool(CpuNameNew);
  }
}



STATIC EFI_STRING_ID gObLanStrList[] = {
  STRING_TOKEN(STR_LAN_MAC_ADDR_VALUE),
};

EFI_STATUS
UpdateOnboardLanMac (
  EFI_HII_HANDLE HiiHandle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  UINT8                     MacAddr[6];
  PLAT_HOST_INFO_PROTOCOL   *ptHostInfo;  
  UINTN                     Index;
  UINTN                     LanIndex = 0;
  UINTN                     LanCount;


  Status = gBS->LocateProtocol(&gPlatHostInfoProtocolGuid, NULL, &ptHostInfo);
  ASSERT(!EFI_ERROR(Status));

  LanCount = sizeof(gObLanStrList)/sizeof(gObLanStrList[0]);

  for(Index=0;Index<ptHostInfo->HostCount;Index++){
    if(ptHostInfo->HostList[Index].HostType == PLATFORM_HOST_LAN && LanIndex < LanCount){
      DevPath = ptHostInfo->HostList[Index].Dp;
      Status = GetOnboardLanMacAddress(gBS, DevPath, MacAddr);
      if(!EFI_ERROR(Status)){
        InitString(
          HiiHandle,
          gObLanStrList[LanIndex], 
          L"%02X-%02X-%02X-%02X-%02X-%02X", 
          MacAddr[0], MacAddr[1], MacAddr[2], MacAddr[3], MacAddr[4], MacAddr[5]
          );
      }
      LanIndex++;
    }
  }

  return Status;  
}



VOID
InitMain (
  EFI_HII_HANDLE HiiHandle
  )
{
  PLAT_DIMM_INFO            *DimmInfo = (PLAT_DIMM_INFO*)GetPlatformDimmInfo();
  EFI_STATUS                Status;
  EFI_SMBIOS_PROTOCOL       *Smbios;
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  EFI_SMBIOS_TYPE           SmbiosType;
  EFI_SMBIOS_TABLE_HEADER   *SmbiosHdr;
  SMBIOS_STRUCTURE_POINTER  p;
  CHAR8                     *BiosVer;
  CHAR8                     *BiosDate;
  CHAR8                     *BoardId;
  CHAR8                     *Manufacturer;
  CHAR8                     *SerialNumber;
  EFI_GUID                  *Uuid;
  CHAR8                     *AssetTag;
  UINT8                     HH = 0, MM = 0;
  CHAR8                     *DbgRlsStr;
  EFI_SYSTEM_PASSWORD_PROTOCOL  *SystemPassword;
  UINT8                         UserType;
  CHAR8                         *UserLoginTypeStr;


  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  Status = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID**)&Smbios);
  if(EFI_ERROR(Status)){
    return;
  } 

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  SmbiosType = EFI_SMBIOS_TYPE_BIOS_INFORMATION;
  Status = Smbios->GetNext(Smbios, &SmbiosHandle, &SmbiosType, &SmbiosHdr, NULL);
  ASSERT(!EFI_ERROR(Status));
  p.Hdr = SmbiosHdr;
  BiosVer = SmbiosGetStringInTypeByIndex(p, p.Type0->BiosVersion);
  BiosDate = SmbiosGetStringInTypeByIndex(p, p.Type0->BiosReleaseDate);

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  SmbiosType = EFI_SMBIOS_TYPE_SYSTEM_INFORMATION;
  Status = Smbios->GetNext(Smbios, &SmbiosHandle, &SmbiosType, &SmbiosHdr, NULL);
  ASSERT(!EFI_ERROR(Status));
  p.Hdr = SmbiosHdr;
  BoardId = SmbiosGetStringInTypeByIndex(p, p.Type1->ProductName);
  Manufacturer = SmbiosGetStringInTypeByIndex(p, p.Type1->Manufacturer);
  SerialNumber = SmbiosGetStringInTypeByIndex(p, p.Type1->SerialNumber);
  Uuid = &p.Type1->Uuid;

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  SmbiosType = EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE;
  Status = Smbios->GetNext(Smbios, &SmbiosHandle, &SmbiosType, &SmbiosHdr, NULL);
  ASSERT(!EFI_ERROR(Status));
  p.Hdr = SmbiosHdr;
  AssetTag = SmbiosGetStringInTypeByIndex(p, p.Type3->AssetTag);

  Status = GetBiosBuildTimeHHMM(&HH, &MM);
  ASSERT(!EFI_ERROR(Status));

#ifdef MDEPKG_NDEBUG    
    DbgRlsStr = "Release";
#else
    DbgRlsStr = "Debug";
#endif


  Status = gBS->LocateProtocol (
                  &gEfiSystemPasswordProtocolGuid,
                  NULL,
                  (VOID**)&SystemPassword
                  );
  if(!EFI_ERROR(Status)){
    UserType = SystemPassword->GetEnteredType();
    DEBUG((EFI_D_INFO, "UserType:%d\n", UserType));
    switch(UserType){
      case LOGIN_USER_ADMIN:
        UserLoginTypeStr = "Admin";
        break;
        
      case LOGIN_USER_POP:
        UserLoginTypeStr = "POP";
        break; 

      default:
        UserLoginTypeStr = "Unknown";
        break;       
    }
    InitString(
      HiiHandle,
      STRING_TOKEN(STR_LOGIN_TYPE_VALUE), 
      L"%a", 
      UserLoginTypeStr
      );    
  }


  InitString (
    HiiHandle,
    STRING_TOKEN(STR_BOARD_ID_VALUE),
    L"%a",
    BoardId
    );  
  
  InitString (
    HiiHandle,
    STRING_TOKEN(STR_BIOS_DATE_VALUE),
    L"%a %02d:%02d",
    BiosDate,
    HH, MM
    );
  
  InitString (
    HiiHandle,
    STRING_TOKEN(STR_BIOS_VERSION_VALUE),
    L"%a(%a) EFI %d.%d",
    BiosVer,
    DbgRlsStr,
    (EFI_SPECIFICATION_VERSION >> 16) & 0xFFFF,
    EFI_SPECIFICATION_VERSION & 0xFFFF
    );   

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_BIOS_VENDOR_VALUE), 
    L"%s", 
    gST->FirmwareVendor != NULL ? gST->FirmwareVendor : L"Byosoft"
    );

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_SYSTEM_MANUFACTURER_VALUE),
    L"%a",
    Manufacturer
    );

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_SYSTEM_SERIAL_NUMBER_VALUE),
    L"%a",
    SerialNumber
    );

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_ASSET_TAG_VALUE),
    L"%a",
    AssetTag
    );

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_SYSTEM_UUID_VALUE),
    L"%g",
    Uuid
    );

  InitString(
    HiiHandle,
    STRING_TOKEN(STR_MEMORY_SIZE_VALUE), 
    L"%d MB", 
    DimmInfo->DimmTotalSizeMB
    );

  InitString(
    HiiHandle,
    STRING_TOKEN(STR_MEMORY_FREQ_VALUE), 
    L"%d MHz", 
    DimmInfo->DimmFreq
    );
  
  UpdateCpuInfo(HiiHandle, Smbios);
  UpdateOnboardLanMac(HiiHandle); 
}





EFI_STATUS
MainFormInit (
  IN EFI_HII_HANDLE    HiiHandle
  )
{
  InitMain(HiiHandle);
  return EFI_SUCCESS;
}
  
EFI_STATUS
AdvanceFormInit (
  IN EFI_HII_HANDLE    HiiHandle
  )
{
  UpdateAdvCpuInfo(HiiHandle);
  UpdateDramInfo(HiiHandle); 
  return EFI_SUCCESS;  
}
  
EFI_STATUS
DeviceFormInit (
  IN EFI_HII_HANDLE    HiiHandle
  )
{
  UpdateSataPortInfo(HiiHandle); 
  return EFI_SUCCESS;  
}


