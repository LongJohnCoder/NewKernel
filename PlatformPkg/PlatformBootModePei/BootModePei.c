

#include <PiPei.h>
#include <Pi/PiBootMode.h>
#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Ppi/MasterBootMode.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/Capsule.h>
#include <Library/PlatformCommLib.h>
#include <Guid/CapsuleVendor.h>
#include <Guid/MemoryTypeInformation.h>
#include <Ppi/Capsule.h>
#include <Framework/StatusCode.h>
#include <PlatformDefinition.h>




STATIC EFI_PEI_PPI_DESCRIPTOR  gBootModePpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMasterBootModePpiGuid,
  NULL
};


//  TRUE  - A wake event occured without power failure.
//  FALSE - Power failure occured or not a wakeup.
BOOLEAN
GetSleepTypeAfterWakeup (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  OUT UINT16                 *SleepType
  )
{
  UINT16  PmSts;
  UINT16  PmCtrl;

  PmSts  = IoRead16(PMIO_REG(PMIO_PM_STA));///PMIO_Rx00[15:0] Power Management Status
  PmCtrl = IoRead16(PMIO_REG(PMIO_PM_CTL));///PMIO_Rx04[15:0] Power Management Control

  if(PmSts & PMIO_WAKA_STS){
    *SleepType = (PmCtrl & PMIO_SLP_TYP);
    if ((*SleepType == PMIO_PM1_CNT_S3) && ((PmSts & PMIO_PWF_STS) !=0 )){
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}



EFI_STATUS
BootModeInit (
  IN CONST EFI_PEI_SERVICES                 **PeiServices,
  IN       EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi,
  OUT      EFI_BOOT_MODE                    *pBootMode OPTIONAL
  )
{
  EFI_STATUS                            Status;
  EFI_BOOT_MODE                         BootMode;
  UINT16                                SleepType;
  UINTN                                 Size;
  BOOLEAN                               BootState;
  volatile EFI_FIRMWARE_VOLUME_HEADER   *FvHdr;

  
  BootMode = BOOT_WITH_FULL_CONFIGURATION;
  
// 1. Check Recovery
  FvHdr = (volatile EFI_FIRMWARE_VOLUME_HEADER*)(UINTN)PcdGet32(PcdFlashFvMainBase);
  if(FvHdr->Signature != EFI_FVH_SIGNATURE               ||
     (FvHdr->HeaderLength & BIT0)                        ||
     CalculateSum16((UINT16*)FvHdr, FvHdr->HeaderLength) ||
     FvHdr->FvLength  != PcdGet32(PcdFlashFvMainSize)){
    BootMode = BOOT_IN_RECOVERY_MODE;
    (*PeiServices)->ReportStatusCode (
                      PeiServices,
                      EFI_PROGRESS_CODE,
                      (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEIM_PC_RECOVERY_AUTO),
                      0,
                      NULL,
                      NULL
                      );    
    goto UpdateBootMode; 
  }
  
  FvHdr = (volatile EFI_FIRMWARE_VOLUME_HEADER*)(UINTN)PcdGet32(PcdFlashFvRecoveryBackUpBase);
  if(FvHdr->Signature != EFI_FVH_SIGNATURE               ||
     (FvHdr->HeaderLength & BIT0)                        ||
     CalculateSum16((UINT16*)FvHdr, FvHdr->HeaderLength) ||
     FvHdr->FvLength  != PcdGet32(PcdFlashFvRecoveryBackUpSize)){
    BootMode = BOOT_IN_RECOVERY_MODE;
    (*PeiServices)->ReportStatusCode (
                      PeiServices,
                      EFI_PROGRESS_CODE,
                      (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEIM_PC_RECOVERY_AUTO),
                      0,
                      NULL,
                      NULL
                      );    
    goto UpdateBootMode; 
  }

  

// 2. default setting.
  Size = 0;
  Status = Var2Ppi->GetVariable (
                      Var2Ppi,
                      EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
                      &gEfiMemoryTypeInformationGuid,
                      NULL,
                      &Size,
                      NULL
                      );

  if (Status == EFI_NOT_FOUND) {
    DEBUG((EFI_D_INFO, "NoMTI\n"));
    BootMode = BOOT_WITH_DEFAULT_SETTINGS;
    goto UpdateBootMode;
  }

  if(GetSleepTypeAfterWakeup(PeiServices, &SleepType)){
    switch (SleepType) {
      case PMIO_PM1_CNT_S3:
        BootMode = BOOT_ON_S3_RESUME;
        goto CheckNext;
        break;

      case PMIO_PM1_CNT_S4:
        BootMode = BOOT_ON_S4_RESUME;
        break;

      case PMIO_PM1_CNT_S5:
        BootMode = BOOT_ON_S5_RESUME;
        goto CheckNext;
        break;

      default:
        DEBUG((EFI_D_ERROR, "SleepType:0x%X\n", SleepType));
        goto CheckNext;
        break;
    }
    goto UpdateBootMode;
  }

CheckNext:
  if(BootMode == BOOT_ON_S3_RESUME){
    Size = 0;
    Status = Var2Ppi->GetVariable (
                        Var2Ppi,
                        EFI_CAPSULE_VARIABLE_NAME,
                        &gEfiCapsuleVendorGuid,
                        NULL,
                        &Size,
                        NULL
                        );
    if(Status == EFI_BUFFER_TOO_SMALL){
      BootMode = BOOT_ON_FLASH_UPDATE;
    }
    goto UpdateBootMode;    
  }
	
  BootState = PcdGetBool(PcdBootState);
  DEBUG((EFI_D_INFO, "BootState:%d\n", BootState));
  if(!BootState){
    BootMode = BOOT_ASSUMING_NO_CONFIGURATION_CHANGES;
    goto UpdateBootMode;
  }

UpdateBootMode:
  DEBUG((DEBUG_ERROR, "BootMode:%d\n", BootMode));
  Status = (**PeiServices).SetBootMode(PeiServices, BootMode);
  ASSERT_EFI_ERROR(Status);
  Status = (**PeiServices).InstallPpi(PeiServices, &gBootModePpi);
  ASSERT_EFI_ERROR (Status);
  if(pBootMode!=NULL){
    *pBootMode = BootMode;
  }

  return Status;
}



EFI_STATUS
EFIAPI
BootModePeiEntry (
  IN       EFI_PEI_FILE_HANDLE      FileHandle,
  IN CONST EFI_PEI_SERVICES         **PeiServices
  )
{
  EFI_STATUS                       Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi;  
  
  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID**)&Var2Ppi
             );
  ASSERT_EFI_ERROR(Status);  
  
  Status = BootModeInit(PeiServices, Var2Ppi, NULL);
  ASSERT_EFI_ERROR(Status);
  
  return Status;
}


