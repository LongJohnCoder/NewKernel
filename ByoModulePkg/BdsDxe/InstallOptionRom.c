
#include "BdsPlatform.h"
#include <Library/PerformanceLib.h>
#include <Protocol/PlatHostInfoProtocol.h>
#include <IndustryStandard/Pci.h>
#include <Library/ByoCommLib.h>


EFI_STATUS
InstallAddOnOpRom (
  EFI_HANDLE                    PciHandle,
  EFI_PCI_IO_PROTOCOL           *PciIo,
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios
  )
{
  EFI_STATUS  Status;
  UINT64      Supports;
  UINTN       Flags;
  

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );
  if (!EFI_ERROR (Status)) {
    Supports &= EFI_PCI_DEVICE_ENABLE;
    Status = PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationEnable,
                      Supports,
                      NULL
                      );
  }
  if (EFI_ERROR (Status)) {
    goto ProcExit;
  }

  Status = LegacyBios->CheckPciRom (
                         LegacyBios,
                         PciHandle,
                         NULL,
                         NULL,
                         &Flags
                         );
  if (EFI_ERROR (Status)) {
    goto ProcExit;
  }

  Status = LegacyBios->InstallPciRom (
                         LegacyBios,
                         PciHandle,
                         NULL,
                         &Flags,
                         NULL,
                         NULL,
                         NULL,
                         NULL
                         );

ProcExit:
  return Status;
}





VOID
InstallAdditionalOpRom (
  VOID
  )
{
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  EFI_STATUS                    Status;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer = NULL;
  UINTN                         Index;
  UINTN                         TableIndex;
  UINTN                         TableCount;	
  UINTN                         Flags;
  VOID                          *LocalRomImage;
  UINTN                         LocalRomSize;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  UINT16                        VendorId;
  UINT16                        DeviceId;
  BOOLEAN                       RunCheck = FALSE;
  UINT8                         ClassCode[3];
  PLAT_HOST_INFO_PROTOCOL       *ptHostInfo;
  ADDITIONAL_ROM_TABLE          *RomTable;


  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  Status = gBS->LocateProtocol (
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  &LegacyBios
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "LegacyBios Not found! Skip\n"));
    goto ProcExit;
  }

  Status = gBS->LocateProtocol(&gPlatHostInfoProtocolGuid, NULL, &ptHostInfo);
  ASSERT(!EFI_ERROR(Status));

  TableCount = ptHostInfo->OptionRomTableSize/sizeof(ADDITIONAL_ROM_TABLE);
  RomTable   = ptHostInfo->OptionRomTable;

  if(PcdGet8(PcdBiosBootModeType) == 1){            // UEFI ONLY
    for (Index = 0; Index < TableCount; Index++) {
      RomTable[Index].Enable = FALSE;
    }
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  ASSERT(!EFI_ERROR(Status));

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    &PciIo
                    );
    ASSERT(!EFI_ERROR(Status));

    PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, PCI_VENDOR_ID_OFFSET, 1, &VendorId);
    PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, PCI_DEVICE_ID_OFFSET, 1, &DeviceId);

    for (TableIndex = 0; TableIndex < TableCount; TableIndex++) {
      if(!RomTable[TableIndex].Enable) {
        continue;
      }
      if(RomTable[TableIndex].RunCheck == NULL){
        RunCheck = (RomTable[TableIndex].VendorId == VendorId && RomTable[TableIndex].DeviceId == DeviceId);
      } else {
        RunCheck = RomTable[TableIndex].RunCheck(HandleBuffer[Index], PciIo, ptHostInfo);
      }
	  
      if(RunCheck){
        Status = GetSectionFromAnyFv (
                   &RomTable[TableIndex].RomImageGuid,
                   EFI_SECTION_RAW,
                   0,
                   &LocalRomImage,
                   &LocalRomSize
                   );
        if (!EFI_ERROR (Status) && LocalRomImage != NULL && LocalRomSize != 0) {
          Status = LegacyBios->InstallPciRom (
                                 LegacyBios,
                                 HandleBuffer[Index],
                                 &LocalRomImage,
                                 &Flags,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL
                                 );
          break;
        }
      }
    }
    if(TableIndex < TableCount){
      continue;
    }			
  

    if (PciIo->RomImage == NULL || PciIo->RomSize == 0) {
      continue;
    }
    PciIo->Pci.Read (
                 PciIo,
                 EfiPciIoWidthUint8,
                 PCI_CLASSCODE_OFFSET,
                 sizeof(ClassCode),
                 &ClassCode[0]
                 );
    if (((ClassCode[2] == PCI_CLASS_DISPLAY) && (ClassCode[1] == PCI_CLASS_DISPLAY_VGA)) ||
        ((ClassCode[2] == PCI_CLASS_OLD) && (ClassCode[1] == PCI_CLASS_OLD_VGA))) {
      continue;
    }

    if(ClassCode[2] == PCI_CLASS_NETWORK || \
	  (ClassCode[0] == PCI_IF_NVMHCI) && (ClassCode[1] == PCI_CLASS_MASS_STORAGE_NVM) && \
	  (ClassCode[2] == PCI_CLASS_MASS_STORAGE) && !RunCheck){
      InstallAddOnOpRom(HandleBuffer[Index], PciIo, LegacyBios);
    } else {
      Status = gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }  
  }


ProcExit:
  if(HandleBuffer != NULL){
    gBS->FreePool(HandleBuffer);
  }
  return;
}


