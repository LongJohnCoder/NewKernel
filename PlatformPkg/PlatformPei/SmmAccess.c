
#include "PlatformPei.h"
#include <Guid/AcpiS3Context.h>
#include <Ppi/SmmAccess.h>


#define SMM_ACCESS_PRIVATE_DATA_SIGNATURE   SIGNATURE_32('4','5','s','a')

typedef struct {
  UINT32                          Signature;
  BOOLEAN                         CloseLock;
  PEI_SMM_ACCESS_PPI              SmmAccess;
  EFI_PEI_PPI_DESCRIPTOR          PpiList;
  UINT32                          SmramDescSize;
  UINT32                          SmramDescCount;
  EFI_SMRAM_DESCRIPTOR            SmramDesc[1];
} SMM_ACCESS_PRIVATE_DATA;

#define SMM_ACCESS_PRIVATE_DATA_FROM_THIS(a) \
  CR(a, SMM_ACCESS_PRIVATE_DATA, SmmAccess, SMM_ACCESS_PRIVATE_DATA_SIGNATURE)

STATIC
EFI_STATUS
EFIAPI
PeiSmmOpen (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN UINTN                           DescriptorIndex
  )
{
  SMM_ACCESS_PRIVATE_DATA  *Private;

  Private = SMM_ACCESS_PRIVATE_DATA_FROM_THIS(This);

  if(DescriptorIndex >= Private->SmramDescCount){
    return EFI_INVALID_PARAMETER;
  }
  
  if(Private->CloseLock){
    return EFI_ACCESS_DENIED;
  } else {
    return EFI_SUCCESS;
  }  
}


STATIC
EFI_STATUS
EFIAPI
PeiSmmClose (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN UINTN                           DescriptorIndex
  )
{
  SMM_ACCESS_PRIVATE_DATA  *Private;

  Private = SMM_ACCESS_PRIVATE_DATA_FROM_THIS(This); 

  if(DescriptorIndex >= Private->SmramDescCount){
    return EFI_INVALID_PARAMETER;
  }
 
  if(!Private->CloseLock){
    MmioOr8(HIF_PCI_REG(SMM_APIC_DECODE_REG), TSMM_EN); 
    if(IsSmrrTypeSetWB()) {
      MmioAnd8(HIF_PCI_REG(SVAD_VGA_DECODE_REG), (UINT8)~SVAD_ABSEG_SEL);
      MmioOr8(HIF_PCI_REG(SVAD_VGA_DECODE_REG), TSEG_C2M_PROTECT_DISABLE);    
    }
    Private->CloseLock = TRUE;  
  }  

  Private->SmmAccess.OpenState = FALSE;
  
  return EFI_SUCCESS;
}


STATIC  
EFI_STATUS
EFIAPI
PeiSmmLock (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN UINTN                           DescriptorIndex
  )
{
  SMM_ACCESS_PRIVATE_DATA  *Private;

  Private = SMM_ACCESS_PRIVATE_DATA_FROM_THIS(This);
  
  if(DescriptorIndex >= Private->SmramDescCount){
    return EFI_INVALID_PARAMETER;
  }  
  
  if(!Private->CloseLock){
    MmioOr8(HIF_PCI_REG(SMM_APIC_DECODE_REG), TSMM_EN); 
    Private->CloseLock = TRUE;    
  }  

  Private->SmmAccess.LockState = TRUE;
  
  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
EFIAPI
PeiSmmGetCapabilities (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN OUT UINTN                       *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR        *SmramMap
  )
{
  UINTN                    DescSize;
  EFI_STATUS               Status;
  SMM_ACCESS_PRIVATE_DATA  *Private;
  
  Private = SMM_ACCESS_PRIVATE_DATA_FROM_THIS(This);
  DescSize = sizeof(Private->SmramDescSize);
  if (*SmramMapSize < DescSize) {
    Status = EFI_BUFFER_TOO_SMALL;
  } else {
    CopyMem (SmramMap, Private->SmramDesc, DescSize);
    Status = EFI_SUCCESS;
  }

  *SmramMapSize = DescSize;  
  return Status;
}


EFI_STATUS
SmmAccessPpiInstall (
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS               Status;
  SMM_ACCESS_PRIVATE_DATA  *Private;  
  BOOLEAN                  CloseLock;
  EFI_PEI_HOB_POINTERS     GuidHob;
  EFI_SMRAM_DESCRIPTOR     *SmramDesc;
  UINTN                    SmramDescSize;


  GuidHob.Raw = GetFirstGuidHob(&gSmramDescTableGuid);
  ASSERT(GuidHob.Raw != NULL);
  SmramDesc = (VOID*)(GuidHob.Guid+1);
  SmramDescSize = GET_GUID_HOB_DATA_SIZE(GuidHob.Guid);
  DEBUG((EFI_D_INFO, "SMRAM Desc Size:%d\n", SmramDescSize));
  
  Private = AllocateZeroPool(sizeof(*Private) + SmramDescSize - sizeof(EFI_SMRAM_DESCRIPTOR));
  if (Private == NULL) {
    DEBUG((EFI_D_ERROR, "Alloc SmmAccessPrivate fail.\n"));
    return EFI_OUT_OF_RESOURCES;
  }  
 
  CloseLock = (SmramDesc[0].RegionState & EFI_SMRAM_CLOSED)?TRUE:FALSE;
 
  Private->Signature = SMM_ACCESS_PRIVATE_DATA_SIGNATURE;  
  Private->SmmAccess.Open            = PeiSmmOpen;
  Private->SmmAccess.Close           = PeiSmmClose;
  Private->SmmAccess.Lock            = PeiSmmLock;
  Private->SmmAccess.GetCapabilities = PeiSmmGetCapabilities;
  Private->SmmAccess.LockState       = CloseLock;
  Private->SmmAccess.OpenState       = !CloseLock;
  Private->CloseLock                 = CloseLock;

  Private->SmramDescSize = SmramDescSize;
  Private->SmramDescCount = SmramDescSize/sizeof(EFI_SMRAM_DESCRIPTOR);
  CopyMem(Private->SmramDesc, SmramDesc, SmramDescSize);
  
  Private->PpiList.Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
  Private->PpiList.Guid  = &gPeiSmmAccessPpiGuid;
  Private->PpiList.Ppi   = &Private->SmmAccess;

  Status = PeiServicesInstallPpi(&Private->PpiList);
  ASSERT_EFI_ERROR (Status);

  return Status;
}




