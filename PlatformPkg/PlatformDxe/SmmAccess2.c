
#include "PlatformDxe.h"
#include <Protocol/SmmAccess2.h>


STATIC
EFI_STATUS
EFIAPI
SmmAccess2Open (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  );

STATIC
EFI_STATUS
EFIAPI
SmmAccess2Close (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  );

STATIC  
EFI_STATUS
EFIAPI
SmmAccess2Lock (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  );

STATIC
EFI_STATUS
EFIAPI
SmmAccess2GetCapabilities (
  IN CONST EFI_SMM_ACCESS2_PROTOCOL  *This,
  IN OUT UINTN                       *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR        *SmramMap
  );

STATIC EFI_SMRAM_DESCRIPTOR  *gSmramDesc;
STATIC UINTN                 gSmramDescSize;
STATIC BOOLEAN               gSmramCloseLock;

STATIC EFI_SMM_ACCESS2_PROTOCOL gSmmAccess2 = {
  SmmAccess2Open,
  SmmAccess2Close,
  SmmAccess2Lock,
  SmmAccess2GetCapabilities,
  FALSE,
  FALSE
};


STATIC
EFI_STATUS
EFIAPI
SmmAccess2Open (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  )
{
  UINTN                  Index;
  UINTN                  Count;


  if(gSmramCloseLock){
    return EFI_ACCESS_DENIED;
  }  

  Count = gSmramDescSize/sizeof(EFI_SMRAM_DESCRIPTOR);

  for (Index = 0; Index < Count; Index++){
    if (gSmramDesc[Index].RegionState & EFI_SMRAM_LOCKED){	    
      DEBUG ((EFI_D_INFO, "Cannot open a locked SMRAM region\n"));
      continue;
    }
    gSmramDesc[Index].RegionState &= (~(UINT64)(EFI_SMRAM_CLOSED |EFI_ALLOCATED));
    gSmramDesc[Index].RegionState |= EFI_SMRAM_OPEN;  	
  }	  	
  
  gSmmAccess2.OpenState = TRUE;
  
  return EFI_SUCCESS;
}  


STATIC
EFI_STATUS
EFIAPI
SmmAccess2Close (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  )
{
  UINTN                  Index;
  UINTN                  Count;

  Count = gSmramDescSize/sizeof(EFI_SMRAM_DESCRIPTOR);
  
  for (Index = 0; Index < Count; Index++){

    if (gSmramDesc[Index].RegionState & EFI_SMRAM_LOCKED){	    
      DEBUG ((EFI_D_INFO, "Cannot close a locked SMRAM region\n"));
      continue;
    }
    if (gSmramDesc[Index].RegionState & EFI_SMRAM_CLOSED){
      continue;
    }

    gSmramDesc[Index].RegionState &= (~(UINT64)EFI_SMRAM_OPEN);
    gSmramDesc[Index].RegionState |= (UINT64)(EFI_SMRAM_CLOSED | EFI_ALLOCATED);
  }	

  if(IsSmrrTypeSetWB()) {
      MmioAnd8(HIF_PCI_REG(SVAD_VGA_DECODE_REG), (UINT8)~SVAD_ABSEG_SEL);
      MmioOr8(HIF_PCI_REG(SVAD_VGA_DECODE_REG), TSEG_C2M_PROTECT_DISABLE);    
  }

  MmioOr8(HIF_PCI_REG(SMM_APIC_DECODE_REG), TSMM_EN);    

  gSmmAccess2.OpenState = FALSE;
  gSmramCloseLock = TRUE;
  
  return EFI_SUCCESS;
}


STATIC  
EFI_STATUS
EFIAPI
SmmAccess2Lock (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  )
{
  UINTN                  Index;
  UINTN                  Count;

  Count = gSmramDescSize/sizeof(EFI_SMRAM_DESCRIPTOR);

  for (Index = 0; Index < Count; Index++){
    if (gSmmAccess2.OpenState){
      DEBUG ((EFI_D_ERROR, "Cannot lock SMRAM when SMRAM regions are still open\n"));
      return EFI_DEVICE_ERROR;
    }

    gSmramDesc[Index].RegionState |= EFI_SMRAM_LOCKED; 	
  }	
   
  gSmmAccess2.LockState = TRUE; 
  gSmramCloseLock = TRUE;  
  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
EFIAPI
SmmAccess2GetCapabilities (
  IN CONST EFI_SMM_ACCESS2_PROTOCOL  *This,
  IN OUT UINTN                       *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR        *SmramMap
  )
{
  UINTN       DescSize;
  EFI_STATUS  Status;
  
  DescSize = gSmramDescSize;
  if (*SmramMapSize < DescSize) {
    Status = EFI_BUFFER_TOO_SMALL;
  } else {
    CopyMem (SmramMap, gSmramDesc, DescSize);
    Status = EFI_SUCCESS;
  }

  *SmramMapSize = DescSize;
  return Status;
}

  






EFI_STATUS
SmmAccess2Install (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_PEI_HOB_POINTERS  GuidHob;
  EFI_SMRAM_DESCRIPTOR  *SmramDesc;
  UINTN                 SmramDescSize;
  

  GuidHob.Raw = GetFirstGuidHob(&gSmramDescTableGuid);
  ASSERT(GuidHob.Raw != NULL);
  SmramDesc = (VOID*)(GuidHob.Guid+1);
  SmramDescSize = GET_GUID_HOB_DATA_SIZE(GuidHob.Guid);
  DEBUG((EFI_D_INFO, "SMRAM Desc Size:%d\n", SmramDescSize));

  gSmramDescSize = SmramDescSize;
  gSmramDesc = AllocatePool(SmramDescSize);
  ASSERT(gSmramDesc != NULL);
  CopyMem(gSmramDesc, SmramDesc, SmramDescSize);

  gSmramCloseLock = (SmramDesc[0].RegionState & EFI_SMRAM_CLOSED)?TRUE:FALSE;
  DEBUG((EFI_D_INFO, "gSmramCloseLock:%d\n", gSmramCloseLock));

  if(!gSmramCloseLock){
    gSmmAccess2.LockState = FALSE;
    gSmmAccess2.OpenState = TRUE;
  } else {
    gSmmAccess2.LockState = TRUE;
    gSmmAccess2.OpenState = FALSE; 
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiSmmAccess2ProtocolGuid,
                  &gSmmAccess2,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);

  return Status;
}



