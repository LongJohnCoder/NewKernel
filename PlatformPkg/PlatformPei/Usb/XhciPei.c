
#include <PlatformDefinition.h>
#include <UsbController.h>
#include <IndustryStandard/Pci.h>
#include <Library/TimerLib.h>
#include <Library/PlatformCommLib.h>


STATIC
EFI_STATUS
GetXhciFwFromFv (
  OUT VOID   **pMcuFile,
  OUT UINT32  *pMcuSize
  )
{
  EFI_STATUS                 Status;
  UINTN                      Instance;
  EFI_PEI_FV_HANDLE          VolumeHandle;
  EFI_PEI_FILE_HANDLE        FileHandle;
  VOID                       *McuFile;
  BOOLEAN                    HasFound;
  UINT32                     McuSize;
  EFI_PHYSICAL_ADDRESS       Address;


  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  ASSERT(pMcuFile!=NULL);
  
  Instance = 0;
  HasFound = FALSE;
  McuFile  = NULL;

  while (1) {
    Status = PeiServicesFfsFindNextVolume(Instance, &VolumeHandle);
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = PeiServicesFfsFindFileByName((EFI_GUID*)PcdGetPtr(PcdXhciMcuFwFile), VolumeHandle, &FileHandle);
    if(!EFI_ERROR(Status)){
      Status = PeiServicesFfsFindSectionData(EFI_SECTION_RAW, FileHandle, &McuFile);
      ASSERT_EFI_ERROR(Status);
      HasFound = TRUE;
      break;
    }

    Instance++;
  }

  if(!HasFound){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }

  McuSize  = *(UINT32*)(&((EFI_COMMON_SECTION_HEADER*)((UINT8*)McuFile - sizeof(EFI_COMMON_SECTION_HEADER)))->Size);
  McuSize &= 0xFFFFFF;
  McuSize -= sizeof(EFI_COMMON_SECTION_HEADER);
  DEBUG((EFI_D_INFO, "Mcu(%X,%X)\n", McuFile, McuSize));

  Status = PeiServicesAllocatePages(EfiBootServicesData, EFI_SIZE_TO_PAGES(McuSize+0x10000), &Address);
  ASSERT_EFI_ERROR(Status);
  DEBUG((EFI_D_INFO, "(L%d)Address:%lX\n", __LINE__, Address));
  Address = ALIGN_VALUE(Address, 0x10000);
  CopyMem((VOID*)(UINTN)Address, McuFile, McuSize);
  McuFile = (VOID*)(UINTN)Address;
  *pMcuFile = McuFile;
  *pMcuSize = McuSize;

ProcExit:
  DEBUG((EFI_D_INFO, "(L%d)%r\n", __LINE__, Status));
  return Status;
}


EFI_STATUS
HandleXhciFwForRecovery ()
{
  EFI_STATUS          Status;
  VOID                *McuFile;
  UINT32              McuSize;


  if((UINT16)MmioRead32(XHCI_PCI_REG(PCI_VENDOR_ID_OFFSET)) == 0xFFFF){
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }

  Status = GetXhciFwFromFv(&McuFile, &McuSize);
  ASSERT_EFI_ERROR(Status);
//temp  Status = LoadXhciFw((UINT32)(UINTN)McuFile, 0);
  ASSERT_EFI_ERROR(Status);

ProcExit:
  return Status;
}

