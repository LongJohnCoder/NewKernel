
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <PlatformDefinition.h>


EFI_STATUS UpdateTolumVar(UINTN BitsOfAlignment, UINT64 AddrLen)
{
  UINT32                 Tolum;
  UINTN                  VarSize;
  PLAT_NV_INFO           NvInfo;
  UINT16                 TolumMB;
  UINT16                 HobTolumMB;
  UINT32                 MemSizeMB;
  EFI_STATUS             Status;
  EFI_BOOT_MODE          BootMode; 
  PLATFORM_MEMORY_INFO   *MemInfo;
  EFI_PEI_HOB_POINTERS   GuidHob; 
  UINTN                  BitsSize;
  

  BootMode = GetBootModeHob();
  if (BootMode == BOOT_ON_FLASH_UPDATE || BootMode == BOOT_IN_RECOVERY_MODE) {
    goto ProcExit;
  }  
  
  ASSERT(AddrLen < SIZE_4GB);
  BitsSize = LShiftU64(1, BitsOfAlignment);
  
  GuidHob.Raw = GetFirstGuidHob(&gEfiPlatformMemInfoGuid);
  ASSERT(GuidHob.Raw != NULL);  
  MemInfo  = (PLATFORM_MEMORY_INFO*)(GuidHob.Guid+1);   
  MemSizeMB = (UINT32)RShiftU64(MemInfo->PhyMemSize, 20);  
  
  if(AddrLen > (PCI_MMIO_TOP_ADDRESS - 0xF0000000) || BitsOfAlignment > 28){
    Tolum = 0xE0000000 - (UINT32)AddrLen;
    if(Tolum & ((UINT32)BitsSize - 1)){
      Tolum  = (UINT32)ALIGN_VALUE(Tolum, BitsSize);
      Tolum -= (UINT32)BitsSize;
    }

  } else {
    Tolum = 0xE0000000;
  }

  if(Tolum & (SIZE_256MB  - 1)){
    Tolum  = ALIGN_VALUE(Tolum, SIZE_256MB);
    Tolum -= SIZE_256MB;
  } 
  
  TolumMB = (UINT16)(Tolum >> 20);
  if((UINT32)TolumMB > MemSizeMB){
    TolumMB = (UINT16)MemSizeMB;
  }
  
  DEBUG((EFI_D_ERROR, "WantTolumMB:%X\n", TolumMB));

  VarSize = sizeof(PLAT_NV_INFO);
  ZeroMem(&NvInfo, VarSize);  
  Status = gRT->GetVariable (
                  NVINFO_TOLUM_VAR_NAME,
                  &gEfiPlatformNvInfoGuid,
                  NULL,
                  &VarSize,
                  &NvInfo
                  );   
  if(EFI_ERROR(Status) || TolumMB != NvInfo.Tolum){
    NvInfo.Tolum = TolumMB;
    Status = gRT->SetVariable (
                    NVINFO_TOLUM_VAR_NAME,
                    &gEfiPlatformNvInfoGuid,
                    NVINFO_TOLUM_VAR_ATTRIBUTE,
                    sizeof(PLAT_NV_INFO),
                    &NvInfo
                    );
    ASSERT(!EFI_ERROR(Status));
    
    HobTolumMB = (UINT16)(MemInfo->Tolum >> 20); 
    if(HobTolumMB != TolumMB){
      DEBUG((EFI_D_INFO, "Tolum Changed(Hob:%X, Var:%X, Want:%X), Reset ...\n", HobTolumMB, NvInfo.Tolum, TolumMB));
      gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
    }  
  }

ProcExit:  
  return EFI_SUCCESS;
}


