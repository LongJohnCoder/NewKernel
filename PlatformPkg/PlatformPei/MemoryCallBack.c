
#include "PlatformPei.h"
#include <ByoCapsuleExt.h>
#include <Library/PerformanceLib.h>
#include <SetupVariable.h>


#define RES_IO_BASE   0x1000
#define RES_IO_LIMIT  0xFFFF

#define DDR3_SPD_MIN_CYCLE_TIME_OFFSET       12
#define DDR4_SPD_MIN_CYCLE_TIME_OFFSET       18

#ifdef IOE_EXIST
EFI_STATUS
HandleIoeMcuXhciFwPei(
  PLATFORM_S3_RECORD *S3Record,
  SETUP_DATA         *SetupData  
	);
#endif

EFI_STATUS 
HandleXhciFwPei(
    PLATFORM_S3_RECORD  *S3Record
    );
EFI_STATUS 
HandlePemcuFwPei(
  PLATFORM_S3_RECORD *S3Record
  );

STATIC EFI_MEMORY_TYPE_INFORMATION gDefaultMemoryTypeInformation[] = {
  { EfiACPIReclaimMemory,   0x40   },     // ASL
  { EfiACPIMemoryNVS,       0x100  },     // ACPI NVS (including S3 related)
  { EfiReservedMemoryType,  0x100  },     // BIOS Reserved (including S3 related)
  { EfiRuntimeServicesData, 0x50   },     // Runtime Service Data
  { EfiRuntimeServicesCode, 0x50   },     // Runtime Service Code
  { EfiBootServicesData,    0x100  },     // Boot Service Data
  { EfiBootServicesCode,    0x100  },     // Boot Service Code
  { EfiMaxMemoryType,       0      }
};


#define SYS_MEM_ATTRIB \
      EFI_RESOURCE_ATTRIBUTE_PRESENT | \
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE | \
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE | \
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE | \
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE | \
      EFI_RESOURCE_ATTRIBUTE_TESTED

#define SYS_MMIO_ATTRIB \
          EFI_RESOURCE_ATTRIBUTE_PRESENT | \
          EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
          EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE | \
          EFI_RESOURCE_ATTRIBUTE_TESTED

/**
   Validate variable data for the MemoryTypeInformation. 

   @param MemoryData       Variable data.
   @param MemoryDataSize   Variable data length.

   @return TRUE            The variable data is valid.
   @return FALSE           The variable data is invalid.

**/
STATIC
BOOLEAN
ValidateMemoryTypeInfoVariable (
  IN EFI_MEMORY_TYPE_INFORMATION      *MemoryData,
  IN UINTN                            MemoryDataSize
  )
{
  UINTN                       Count;
  UINTN                       Index;

  // Check the input parameter.
  if (MemoryData == NULL) {
    return FALSE;
  }

  // Get Count
  Count = MemoryDataSize / sizeof (*MemoryData);

  // Check Size
  if (Count * sizeof(*MemoryData) != MemoryDataSize) {
    return FALSE;
  }

  // Check last entry type filed.
  if (MemoryData[Count - 1].Type != EfiMaxMemoryType) {
    return FALSE;
  }

  // Check the type filed.
  for (Index = 0; Index < Count - 1; Index++) {
    if (MemoryData[Index].Type >= EfiMaxMemoryType) {
      return FALSE;
    }
  }

  return TRUE;
}



// 20 - 800
// 15 - 1066
// 12 - 1333
// 10 - 1600
STATIC UINT16 GetMemSpdFreq(UINT8 Tck)
{
  UINT16  Freq;

  switch(Tck){
    case 20:
      Freq = 800;
      break;
			
    case 15:
      Freq = 1066;
      break;
			
    case 12:
      Freq = 1333;
      break;
			
    case 10:
      Freq = 1600;
      break;

    case 9:
      Freq = 1866;
      break;
      
  	case 8:
  	  Freq = 2133;
      break;
      
  	case 7:
  	  Freq = 2400;
      break;
      
  	case 6:
  	  Freq = 2666;
      break;
      
  	case 5:
  	  Freq = 3200;
      break;
			
    default:
      Freq = 0;
      break;
  }
  
  return Freq;
}



typedef struct {
  CHAR8    FvName[8];
  UINT32   FvBase;
  BOOLEAN  NeedPeiHandle;
} FV_INFO_LIST;

STATIC FV_INFO_LIST gFvList[] = {
  {{"FvMain"}, _PCD_VALUE_PcdFlashFvMainBase,  TRUE},      // DxeMain
  {{"FvNet"},  _PCD_VALUE_PcdFlashFvMain2Base, FALSE},  
};


VOID
ReportResourceForDxe (
  IN  EFI_PEI_SERVICES      **PeiServices,
  IN  PLATFORM_MEMORY_INFO  *MemInfo,
  IN  EFI_BOOT_MODE         BootMode
  )
{
  UINTN                            DataSize;
  UINT32                           FvAddr;
  UINT32                           FvSize;
  VOID                             *FvData;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi;  
  EFI_MEMORY_TYPE_INFORMATION      MemoryData[EfiMaxMemoryType + 1];
  EFI_STATUS                       Status;
  UINTN                            Index;	
  UINTN                            Count;
  EFI_ASIA_DRAM_PPI                *DramPpi;
  ASIA_DRAM_INFO                   DramInfo;	
  PLAT_DIMM_INFO                   *DimmInfo;
  UINTN                            DimmInfoSize;
  UINT32                           PmmBase;
  UINT32                           PmmSize;
  UINT32                           UsableSize;
  CONST UINT8                      *Spd;
  UINT32                           Address;
  UINT32                           RangeSize;
  EFI_FIRMWARE_VOLUME_HEADER       *FvHdr;


  GetAsiaPpi(NULL, NULL, &DramPpi, NULL, NULL);
  Status = DramPpi->DramGetInfo(PeiServices, DramPpi, &DramInfo);
  ASSERT_EFI_ERROR(Status);
  ASSERT(ASIA_MAX_SOCKETS >= 1);
  
  DimmInfoSize = sizeof(PLAT_DIMM_INFO) + (ASIA_MAX_SOCKETS - 1)*sizeof(DIMM_SPD_INFO);
  DimmInfo = BuildGuidHob(&gEfiPlatDimmInfoGuid, DimmInfoSize);
  ASSERT(DimmInfo != NULL);
  ZeroMem(DimmInfo, DimmInfoSize);  

  DimmInfo->DimmCount = ASIA_MAX_SOCKETS;
  DimmInfo->DimmFreq  = DramInfo.DramFreq;
  for(Index=0;Index<(UINTN)DimmInfo->DimmCount;Index++){	
    DimmInfo->SpdInfo[Index].DimmSize = DramInfo.RankInfo[Index*2].RankSize + 
                                        DramInfo.RankInfo[Index*2+1].RankSize;
    DimmInfo->DimmTotalSizeMB += DimmInfo->SpdInfo[Index].DimmSize;
    if(DramInfo.Spd[Index].SpdPresent){	
      Spd = DramInfo.Spd[Index].Buffer;

      if(Spd[2] == 0x0B){                        // DDR3
        DimmInfo->SpdInfo[Index].DimmSpeed = GetMemSpdFreq(Spd[DDR3_SPD_MIN_CYCLE_TIME_OFFSET]);
        DimmInfo->SpdInfo[Index].Sn = (UINT32)((Spd[122]<<24)|(Spd[123]<<16)|(Spd[124]<<8)|Spd[125]);
        CopyMem(DimmInfo->SpdInfo[Index].PartNo, &Spd[128], 18);
        
      } else if(Spd[2] == 0x0C){                 // DDR4
        DimmInfo->SpdInfo[Index].DimmSpeed = GetMemSpdFreq(Spd[DDR4_SPD_MIN_CYCLE_TIME_OFFSET]);
        DimmInfo->SpdInfo[Index].Sn = (UINT32)((Spd[325]<<24)|(Spd[326]<<16)|(Spd[327]<<8)|Spd[328]);
        CopyMem(DimmInfo->SpdInfo[Index].PartNo, &Spd[329], 18);
      }
      
	  // Fill the module manufacturer ID code.
      DimmInfo->SpdInfo[Index].Spd320 = DramInfo.Spd[Index].Buffer[320];
      DimmInfo->SpdInfo[Index].Spd321 = DramInfo.Spd[Index].Buffer[321];
 		
      DEBUG((DEBUG_ERROR, "Dimm[%d] SPD Speed :%d\n", Index, DimmInfo->SpdInfo[Index].DimmSpeed));	
    }			
  }	

  DimmInfo->DramCL = DramInfo.DramCL;
  DimmInfo->DramTras = DramInfo.DramTras;
  DimmInfo->DramTrcd = DramInfo.DramTrcd;
  DimmInfo->DramTrp = DramInfo.DramTrp;
  CopyMem(DimmInfo->EfuseData, DramInfo.EfuseData, sizeof(DimmInfo->EfuseData));
  
  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID **)&Var2Ppi
             );
  ASSERT_EFI_ERROR(Status);  


   
// Report FV  
  if(BootMode != BOOT_IN_RECOVERY_MODE){

    PERF_START (NULL, "report", "FV", 0);	

    Count = sizeof(gFvList)/sizeof(gFvList[0]);
    for(Index=0;Index<Count;Index++){
      FvAddr = gFvList[Index].FvBase;
      if(FvAddr == 0){
        continue;
      }
      FvHdr = (EFI_FIRMWARE_VOLUME_HEADER*)(UINTN)FvAddr;
      if(FvHdr->Signature != EFI_FVH_SIGNATURE){
        continue;
      }
      FvSize = (UINT32)FvHdr->FvLength;

// TPM/TCM will Measure FV Data at the callback of FvInfoPpi
// So here we should copy it to memory before, then DxeCore will 
// not read it again to save boot time.
      FvData = AllocatePages(EFI_SIZE_TO_PAGES(FvSize));
      ASSERT(FvData!=NULL);
      
      PERF_START (NULL, gFvList[Index].FvName, "FV", 0);	
      CopyMem(FvData, (VOID*)FvHdr, FvSize);
      PERF_END(NULL, gFvList[Index].FvName, "FV", 0);
      
      DEBUG((EFI_D_INFO, "FvMem(%X,%X)\n", FvData, FvSize));
    
      BuildFvHob((UINTN)FvData, FvSize);

      if(gFvList[Index].NeedPeiHandle){
        PeiServicesInstallFvInfoPpi(
          NULL,
          FvData,
          FvSize,
          NULL,
          NULL
          ); 
      }
    }
    
    PERF_END(NULL, "report", "FV", 0);

    FvSize = PcdGet32(PcdFlashNvLogoSize);
    FvData = AllocatePages(EFI_SIZE_TO_PAGES(FvSize));
    ASSERT(FvData!=NULL);
    FvHdr = (EFI_FIRMWARE_VOLUME_HEADER*)(UINTN)PcdGet32(PcdFlashNvLogoBase);
    CopyMem(FvData, (VOID*)FvHdr, FvSize);
    PcdSet32(PcdLogoDataAddress, (UINT32)(UINTN)FvData);
    PcdSet32(PcdLogoDataSize, FvSize);
    
  }		


  
// Report CPU FV
  BuildCpuHob(MemInfo->PhyAddrBits, 16);

  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    SYS_MEM_ATTRIB,
    0,
    0xA0000
    );


  UsableSize = MemInfo->LowMemSize;

  PmmSize = PcdGet32(PcdHighPmmMemorySize);
  if(PmmSize){
    PmmSize = ALIGN_VALUE(PmmSize, EFI_PAGE_SIZE);
    UsableSize -= PmmSize;
    PmmBase     = UsableSize;
    ASSERT((PmmBase & (EFI_PAGE_SIZE - 1)) == 0);
    MemInfo->PmmBase = PmmBase;
    MemInfo->PmmSize = PmmSize;
    PcdSet32(PcdHighPmmMemoryAddress, PmmBase);
  } 
  
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    SYS_MEM_ATTRIB,
    SIZE_1MB,
    UsableSize - SIZE_1MB    // Memory above 4G will be reported at ReadyToBoot.
    );    
  
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_RESERVED,
    SYS_MEM_ATTRIB,
    MemInfo->LowMemSize,
#ifdef ZX_TXT_SUPPORT
    S3_PEI_MEMORY_SIZE + S3_DATA_RECORD_SIZE + MemInfo->DprSize + MemInfo->TSegSize
#else
    S3_PEI_MEMORY_SIZE + S3_DATA_RECORD_SIZE + MemInfo->TSegSize
#endif
    );  

  BuildResourceDescriptorHob (
    EFI_RESOURCE_IO,
    EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED,
    RES_IO_BASE,
    RES_IO_LIMIT - RES_IO_BASE + 1
    );    

  Address = (UINT32)PcdGet64(PcdPciExpressBaseAddress);
  ASSERT(Address >= MemInfo->Tolum);  
  RangeSize = Address - MemInfo->Tolum;
  if(RangeSize){
    BuildResourceDescriptorHob (
      EFI_RESOURCE_MEMORY_MAPPED_IO,
      SYS_MMIO_ATTRIB,
      MemInfo->Tolum,
      RangeSize
      );
  }
  
  Address += SIZE_256MB;
  RangeSize = PCI_MMIO_TOP_ADDRESS - Address;
  if(RangeSize){
    BuildResourceDescriptorHob (
      EFI_RESOURCE_MEMORY_MAPPED_IO,
      SYS_MMIO_ATTRIB,
      Address,
      RangeSize 
      );  
  }
  
  if(MemInfo->Pci64Size) {
	  BuildResourceDescriptorHob (
	    EFI_RESOURCE_MEMORY_MAPPED_IO,
	    SYS_MMIO_ATTRIB,
	    MemInfo->Pci64Base,
	    MemInfo->Pci64Size
	    );
  }

  DataSize = sizeof (MemoryData);
  Status = Var2Ppi->GetVariable (
                      Var2Ppi,
                      EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
                      &gEfiMemoryTypeInformationGuid,
                      NULL,
                      &DataSize,
                      &MemoryData
                      );
  DEBUG((EFI_D_INFO, "GMTIV:%r\n", Status));	
  if (EFI_ERROR (Status) || !ValidateMemoryTypeInfoVariable(MemoryData, DataSize)) {
    BuildGuidDataHob (
      &gEfiMemoryTypeInformationGuid,
      gDefaultMemoryTypeInformation,
      sizeof(gDefaultMemoryTypeInformation)
      );
  }
}  

EFI_STATUS
EFIAPI
HandleCapsuleBeforeMemInstall (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN       PLATFORM_MEMORY_INFO  *MemInfo,
  OUT      UINT32                *PeiMemAddr
  )  
{
  EFI_STATUS        Status;
  PEI_CAPSULE_PPI   *Capsule;
  VOID              *CapsuleBuffer;
  UINTN             CapsuleBufferLength;
  UINT32            Memory;
  UINT32            MemEnd;
 
  MemInfo->CapsuleBase = 0;
  MemInfo->CapsuleSize = 0;
  *PeiMemAddr = 0;

  Status = (*PeiServices)->LocatePpi(PeiServices, &gPeiCapsulePpiGuid, 0, NULL, &Capsule);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  CapsuleBuffer = (VOID*)(UINTN)0;
  CapsuleBufferLength = MemInfo->LowMemSize;
  DEBUG((EFI_D_INFO, "CapsuleBuffer(%X,%X)\n", CapsuleBuffer, CapsuleBufferLength));  
  Status = Capsule->Coalesce((EFI_PEI_SERVICES**)PeiServices, &CapsuleBuffer, &CapsuleBufferLength);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  DEBUG((EFI_D_INFO, "Capsule(%X,%X)\n", CapsuleBuffer, CapsuleBufferLength));

  MemInfo->CapsuleBase = (UINT32)(UINTN)CapsuleBuffer;
  MemInfo->CapsuleSize = (UINT32)CapsuleBufferLength;

  MemEnd = MemInfo->LowMemSize - PEI_MEMORY_SIZE;
  for(Memory = SIZE_1MB; Memory < MemEnd; Memory += PEI_MEMORY_SIZE){
    if(Memory + PEI_MEMORY_SIZE < MemInfo->CapsuleBase ||
       MemInfo->CapsuleBase + MemInfo->CapsuleSize < Memory){
      *PeiMemAddr = Memory;
      break;
    }   
  }  

  if(*PeiMemAddr == 0){          // not found
    DEBUG((EFI_D_ERROR, "[ERROR] Could not find PeiMemRange at BIOS_UPDATE mode\n"));
    *PeiMemAddr = MemEnd;
  }  
  
ProcExit:  
  return Status;
}




EFI_STATUS
EFIAPI
HandleCapsuleAfterMemInstall (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN       PLATFORM_MEMORY_INFO  *MemInfo
  )  
{
  UINT16                           PmEnable;
  EFI_STATUS                       Status;
  PEI_CAPSULE_PPI                  *Capsule;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi;
  BYO_CAPSULE_EXTEND               *CapExt;
  UINTN                            Size;


// disable power button SMI with flash update mode
  PmEnable  = IoRead16 (PMIO_REG (PMIO_PM_ENABLE));///PMIO_Rx02[15:0] Power Management Enable
  PmEnable &= (~PMIO_PBTN_EN);
  IoWrite16 (PMIO_REG (PMIO_PM_ENABLE), PmEnable);///PMIO_Rx02[15:0] Power Management Enable

  Status = (*PeiServices)->LocatePpi(PeiServices, &gPeiCapsulePpiGuid, 0, NULL, &Capsule);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  if(MemInfo->CapsuleSize == 0){
    goto ProcExit;
  }  

  Status = Capsule->CreateState((EFI_PEI_SERVICES**)PeiServices, (VOID*)(UINTN)MemInfo->CapsuleBase, MemInfo->CapsuleSize);
  DEBUG((EFI_D_INFO, "CreateState:%r\n", Status));
  
  CapExt = BuildGuidHob(&gEfiCapsuleVendorGuid, sizeof(BYO_CAPSULE_EXTEND));
  ASSERT(CapExt != NULL);
  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID**)&Var2Ppi
             );
  ASSERT_EFI_ERROR(Status);
  Size   = sizeof(BYO_CAPSULE_EXTEND);  
  Status = Var2Ppi->GetVariable (
                      Var2Ppi,
                      EFI_CAPSULE_EXT_VARIABLE_NAME,
                      &gEfiCapsuleVendorGuid,
                      NULL,
                      &Size,
                      CapExt
                      );

ProcExit:  
  return Status;  
}




// RP(0,5,0) by (0,0,5,F4)[10] = 1
VOID 
DisableObLan (
  EFI_ASIA_NB_PPI        *NbPpi,
  EFI_ASIA_SB_PPI        *SbPpi,
  UINT8                  ObLanEn
  )
{
  UINT32                 Base;
  UINT32                 SubBase;
  ASIA_NB_CONFIGURATION  *NbCfg;
  ASIA_SB_CONFIGURATION  *SbCfg;
  UINT16                 PmioBase;
  UINT32                  Data32;
  

  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  
  PmioBase = MmioRead16(PCI_DEV_MMBASE(0,17,0)|D17F0_PMU_PM_IO_BASE) & D17F0_PMU_PMIOBA;  
  NbCfg = (ASIA_NB_CONFIGURATION*)(NbPpi->NbCfg);
  SbCfg = (ASIA_SB_CONFIGURATION*)(SbPpi->SbCfg); 

  #ifdef HX002EH0_01
  Data32 = IoRead32(PmioBase + PMIO_GPIO_PAD_CTL);//PMIO GPIO 14 RxB4[2:0] = 3'b000
  Data32 &= (~PMIO_PAD_GPIO14_2_1_0);
  IoWrite32(PmioBase + PMIO_GPIO_PAD_CTL, Data32);
  
  if(ObLanEn || !NbCfg->PciePE6 ){
  	Data32 = IoRead32(PmioBase + PMIO_GENERAL_PURPOSE_OUTPUT );//PMIO Rx4c 
    Data32 |= BIT27;
    IoWrite32(PmioBase + PMIO_GENERAL_PURPOSE_OUTPUT, Data32);//PMIO Rx4c[27] = 1
  	return;
  }
  #else
  
  if(ObLanEn || !NbCfg->PciePE6 || SbCfg->XhcUartCtrl){
    Data32 = IoRead32(PmioBase + PMIO_GENERAL_PURPOSE_OUTPUT );//PMIO Rx4c[23] 
    Data32 |= BIT23;
    IoWrite32(PmioBase + PMIO_GENERAL_PURPOSE_OUTPUT, Data32);//PMIO Rx4c[23] = 1
    return;
  }
  
  Data32 = IoRead32(PmioBase + PMIO_CR_GPIO_PAD_CTL);//PMIO GPIO 12 RxB4[26:24] = 3'b000
  Data32 &= (~PMIO_PAD_GPIO12_2_1_0);
  IoWrite32(PmioBase + PMIO_CR_GPIO_PAD_CTL, Data32);
  #endif

  Base = PEG2_PCI_REG(0);
  if(MmioRead16(Base+PCI_VID_REG) == 0xFFFF){
    DEBUG((EFI_D_INFO, "PE6 not found!\n"));
    return;
  }
  
  MmioWrite32(Base+PCI_PBN_REG, 0x000E0E00);
  SubBase = PCI_DEV_MMBASE(0xE, 0, 0);
  
  if(MmioRead32(SubBase+PCI_VID_REG) != 0x816810EC){
    //DEBUG((EFI_D_INFO, "ObLan not found!\n"));
    goto ProcExit;
  }

  MmioWrite32(SubBase+PCI_BAR0_REG, 0xD000);
  MmioWrite8(SubBase+PCI_CMD_REG, PCI_CMD_IO_EN);
  MmioWrite16(Base+0x1C, 0xD0D0);
  MmioWrite8(Base+PCI_CMD_REG, PCI_CMD_IO_EN);

  DEBUG((EFI_D_INFO, "[44]:%X, [DA]:%X\n", IoRead8(0xD000 + 0x44), IoRead16(0xD000 + 0xDA)));
  IoAnd8(0xD000 + 0x44, (UINT8)~BIT0);
  IoWrite16(0xD000 + 0xDA, 0);
  DEBUG((EFI_D_INFO, "[44]:%X, [DA]:%X\n", IoRead8(0xD000 + 0x44), IoRead16(0xD000 + 0xDA)));


  MmioWrite8(SubBase+PCI_CMD_REG, 0);
  MmioWrite32(SubBase+PCI_BAR0_REG, 0);
  MmioWrite8(Base+PCI_CMD_REG, 0);  
  MmioWrite16(Base+0x1C, 0);
  MmioWrite32(Base+PCI_PBN_REG, 0);
ProcExit:
  #ifdef HX002EH0_01
  Data32 = IoRead32(PmioBase + PMIO_GENERAL_PURPOSE_OUTPUT );//PMIO Rx4c[27] 
  Data32 &= (~BIT27);
  IoWrite32(PmioBase + PMIO_GENERAL_PURPOSE_OUTPUT, Data32);//PMIO Rx44c[27]= 0
  
  #else
  Data32 = IoRead32(PmioBase + PMIO_GENERAL_PURPOSE_OUTPUT );//PMIO Rx4c[23] 
  Data32 &= (~BIT23);
  IoWrite32(PmioBase + PMIO_GENERAL_PURPOSE_OUTPUT, Data32);//PMIO Rx4c[23] = 0
  #endif
  
  return;
}



EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )  
{
  EFI_STATUS                       Status;
  ASIA_SB_CONFIGURATION            *SbCfg;
  ASIA_NB_CONFIGURATION            *NbCfg;                            // wz180312 +
  EFI_ASIA_SB_PPI                  *SbPpi;
  EFI_ASIA_NB_PPI                  *NbPpi;
  EFI_ASIA_DRAM_PPI                *DramPpi;
  EFI_ASIA_CPU_PPI_PROTOCOL        *CpuPpi;
  EFI_BOOT_MODE                    BootMode;  
  PLATFORM_MEMORY_INFO             *MemInfo; 
  EFI_HOB_GUID_TYPE                *GuidHob;
  SETUP_DATA                       *SetupData;
  PLATFORM_S3_RECORD               *S3Record;
  VOID                             *AcpiVar;
  EFI_SMRAM_DESCRIPTOR             *SmramDesc;
  BOOLEAN                          SmramCloseLock;
  UINT64                           RegionState;
  

  DEBUG((EFI_D_INFO, "MemCallBack, &Status:%X\n", (UINTN)&Status));

  Status = PeiServicesGetBootMode(&BootMode);
  ASSERT_EFI_ERROR(Status);  
  
  GetAsiaPpi(&SbPpi, &NbPpi, &DramPpi, &CpuPpi, NULL); 
  SbCfg = (ASIA_SB_CONFIGURATION*)(SbPpi->SbCfg);
  NbCfg = (ASIA_NB_CONFIGURATION*)(NbPpi->NbCfg);                     // wz180312 +

  GuidHob = GetFirstGuidHob(&gEfiPlatformMemInfoGuid);
  ASSERT(GuidHob!=NULL);	
  MemInfo = (PLATFORM_MEMORY_INFO*)GET_GUID_HOB_DATA(GuidHob); 

  SetupData = (SETUP_DATA*)GetSetupDataHobData();

  Status = CpuCachePpiInit();
  ASSERT_EFI_ERROR(Status); 

  PERF_START(NULL, "CACHE", NULL, 0);		
  SetCacheMtrr(PeiServices, MemInfo);
  PERF_END  (NULL, "CACHE", NULL, 0);		


  SmramCloseLock = MmioRead8(HIF_PCI_REG(SMM_APIC_DECODE_REG)) & TSMM_EN;
  if(SmramCloseLock){
    RegionState = EFI_SMRAM_CLOSED | EFI_SMRAM_LOCKED;
  } else {
    RegionState = EFI_SMRAM_OPEN;
  }

  SmramDesc = (EFI_SMRAM_DESCRIPTOR*)BuildGuidHob (
                                       &gSmramDescTableGuid,
                                       sizeof(EFI_SMRAM_DESCRIPTOR) * 2
                                       );
  ASSERT(SmramDesc != NULL);  
  SmramDesc[0].CpuStart = MemInfo->TSegAddr;
  SmramDesc[0].PhysicalStart = SmramDesc[0].CpuStart;
  SmramDesc[0].PhysicalSize = EFI_PAGE_SIZE;
  SmramDesc[0].RegionState = RegionState | EFI_ALLOCATED;
  
  SmramDesc[1].CpuStart = MemInfo->TSegAddr + EFI_PAGE_SIZE;
  SmramDesc[1].PhysicalStart = SmramDesc[1].CpuStart;
  SmramDesc[1].PhysicalSize = MemInfo->TSegSize - EFI_PAGE_SIZE;
  SmramDesc[1].RegionState = RegionState;

  AcpiVar = BuildGuidHob (
              &gEfiAcpiVariableGuid,
              sizeof(EFI_SMRAM_DESCRIPTOR)
              );
  CopyMem(AcpiVar, SmramDesc, sizeof(EFI_SMRAM_DESCRIPTOR));

  
  if(BootMode == BOOT_ON_S3_RESUME){
    Status = SbPpi->PostMemoryInitS3(PeiServices, SbPpi);
    ASSERT_EFI_ERROR(Status); 
    Status = NbPpi->PostMemoryInitS3(PeiServices, NbPpi);
    ASSERT_EFI_ERROR(Status);
    Status = SmmAccessPpiInstall(PeiServices);
    ASSERT_EFI_ERROR(Status);			
    Status = SmmControlPpiInstall(PeiServices);
    ASSERT_EFI_ERROR(Status);

	  S3Record = (PLATFORM_S3_RECORD*)GetS3RecordTable();
  
	  HandleXhciFwPei(S3Record);
#ifdef IOE_EXIST    
	  Status = HandleIoeMcuXhciFwPei(S3Record, SetupData);
	  ASSERT_EFI_ERROR(Status);   
#endif
	  HandlePemcuFwPei(S3Record);

  } else {
    Status = SbPpi->PostMemoryInit(PeiServices, SbPpi);
    ASSERT_EFI_ERROR(Status); 
    Status = NbPpi->PostMemoryInit(PeiServices, NbPpi);
    ASSERT_EFI_ERROR(Status); 
    ReportResourceForDxe(PeiServices, MemInfo, BootMode);
    if(BootMode == BOOT_IN_RECOVERY_MODE){
      PeimInitializeRecovery(PeiServices);
    } else if(BootMode == BOOT_ON_FLASH_UPDATE){	
      HandleCapsuleAfterMemInstall(PeiServices, MemInfo);
    }
  }

//UpdateSsid();
  #if !defined(HX002EB0_00) && !defined(HX002EB0_11)
  DisableObLan(NbPpi,SbPpi,SetupData->ObLanEn);
  #endif
  
  return Status;
}


