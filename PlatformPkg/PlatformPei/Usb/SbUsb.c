/*++

Module Name:
  
  SbUsb.c

Abstract:

  Ehci PPI Init

--*/
#include "UsbController.h"
#include <PlatformDefinition.h>
#include <IndustryStandard/Pci.h>


#define UHCI_IO_BASE                       0xF000
#define UHCI_IO_LENGTH                     0x20
#define EHCI_MEMORY_BASE                   0xFF000000
#define XHCI_MEMORY_BASE                   0xFF100000


STATIC
EFI_STATUS
EnableEhciController (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN USB_CONTROLLER_INFO      *PeiUsbControllerInfo,
  IN UINT8                    UsbControllerIndex
  )
{
  UINTN  BaseAddress;

  if (UsbControllerIndex >= PeiUsbControllerInfo->EhciControllersNum) {
    return EFI_INVALID_PARAMETER;
  }

  BaseAddress = PeiUsbControllerInfo->MemBase[UsbControllerIndex];

  MmioWrite32 (EHCI_PCI_REG(PCI_BASE_ADDRESSREG_OFFSET), BaseAddress);

  MmioOr16(EHCI_PCI_REG(PCI_COMMAND_OFFSET), BIT2 | BIT1);

  DEBUG((EFI_D_INFO, "EHCIBAR(%08X) = %08X\n", BaseAddress, MmioRead32(BaseAddress)));

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EnableXhciController (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN USB_CONTROLLER_INFO      *PeiUsbControllerInfo,
  IN UINT8                    UsbControllerIndex
  )
{
  UINTN BaseAddress;
  
  if (UsbControllerIndex < PeiUsbControllerInfo->EhciControllersNum ||
      UsbControllerIndex >= PeiUsbControllerInfo->EhciControllersNum + PeiUsbControllerInfo->XhciControllersNum) {
    return EFI_INVALID_PARAMETER;
  }

  BaseAddress = PeiUsbControllerInfo->MemBase[UsbControllerIndex];

  MmioWrite32(XHCI_PCI_REG(PCI_BASE_ADDRESSREG_OFFSET), BaseAddress);
  MmioOr16(XHCI_PCI_REG(PCI_COMMAND_OFFSET), BIT2 | BIT1);

  BaseAddress = MmioRead32(XHCI_PCI_REG (PCI_BASE_ADDRESSREG_OFFSET)) & 0xFFFFFFF0;;
  if(BaseAddress != PeiUsbControllerInfo->MemBase[UsbControllerIndex]) {
    PeiUsbControllerInfo->MemBase[UsbControllerIndex] = 0;
    DEBUG ((EFI_D_ERROR, "specified Xhci devie is not present !\n"));
  }

  DEBUG((EFI_D_INFO, "XHCIBAR(%08X) %08X\n", BaseAddress, MmioRead32(BaseAddress)));

  return EFI_SUCCESS;
}



EFI_STATUS
InitUsbControl (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  USB_CONTROLLER_INFO           *PeiUsbControllerInfo
  )
{
  EFI_STATUS            Status;
  UINTN                 XhciIdStart;


  DEBUG ((EFI_D_INFO, "InitUsbControl()\n"));
  
  Status = EFI_SUCCESS;
  XhciIdStart = PeiUsbControllerInfo->EhciControllersNum;

  if (PeiUsbControllerInfo->EhciControllersNum) {
    PeiUsbControllerInfo->MemBase[0] = EHCI_MEMORY_BASE;
    Status = EnableEhciController (PeiServices, PeiUsbControllerInfo, 0);
    ASSERT_EFI_ERROR (Status);
  }
  if (PeiUsbControllerInfo->XhciControllersNum) {
    PeiUsbControllerInfo->MemBase[XhciIdStart] = XHCI_MEMORY_BASE;
    Status = EnableXhciController (PeiServices, PeiUsbControllerInfo, (UINT8)XhciIdStart);
    ASSERT_EFI_ERROR (Status);
  }

  DEBUG ((EFI_D_INFO, "InitUsbControl() End\n"));

  return Status;

}


