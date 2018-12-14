/*++

Module Name:
  
  MultiUsbController.c

Abstract:

  Usb Controller PPI Init

--*/
#include "UsbController.h"
#include <PlatformDefinition.h>

//
// PPI interface function
//
STATIC
EFI_STATUS
EFIAPI
GetUsbController (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN  PEI_USB_CONTROLLER_PPI        *This,
  IN UINT8                          UsbControllerId,
  OUT UINTN                         *ControllerType,
  OUT UINTN                         *MemBaseAddress
  );

//
// Globals
//
STATIC PEI_USB_CONTROLLER_PPI  mUsbControllerPpi = { GetUsbController };

STATIC EFI_PEI_PPI_DESCRIPTOR   mPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiUsbControllerPpiGuid,
  NULL
};

//
// PPI interface implementation
//
STATIC
EFI_STATUS
EFIAPI
GetUsbController (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  PEI_USB_CONTROLLER_PPI          *This,
  IN  UINT8                           UsbControllerId,
  OUT UINTN                           *ControllerType,
  OUT UINTN                           *MemBaseAddress
  )
{
  USB_CONTROLLER_INFO *PeiUsbControllerInfo;
  UINTN XhciIdStart,UhciIdStart,OhciIdStart;

  PeiUsbControllerInfo = USB_CONTROLLER_INFO_FROM_THIS(This);

  if (UsbControllerId >= PeiUsbControllerInfo->TotalUsbControllers) {
    return EFI_INVALID_PARAMETER;
  }

  XhciIdStart = PeiUsbControllerInfo->EhciControllersNum;
  UhciIdStart = XhciIdStart + PeiUsbControllerInfo->XhciControllersNum;
  OhciIdStart = UhciIdStart + PeiUsbControllerInfo->UhciControllersNum;
  
  PeiUsbControllerInfo = USB_CONTROLLER_INFO_FROM_THIS (This);

  if (UsbControllerId < XhciIdStart){
    *ControllerType = PEI_EHCI_CONTROLLER;
  } else if (UsbControllerId < UhciIdStart){
    *ControllerType = PEI_XHCI_CONTROLLER;
  } else if (UsbControllerId < OhciIdStart){
    *ControllerType = PEI_UHCI_CONTROLLER;
  } else {
    *ControllerType = PEI_OHCI_CONTROLLER;
  }

  *MemBaseAddress = PeiUsbControllerInfo->MemBase[UsbControllerId];

  DEBUG((EFI_D_INFO, "GetUsbHost %d T:%d A:%X\n", UsbControllerId, *ControllerType, *MemBaseAddress));

  return EFI_SUCCESS;
}






EFI_STATUS
InitUsbController (
  IN       EFI_PEI_FILE_HANDLE      FileHandle,
  IN CONST EFI_PEI_SERVICES         **PeiServices
  )
{
  EFI_STATUS                Status;
  USB_CONTROLLER_INFO       *PeiUsbControllerInfo;
  UINTN                     EhciNum = 1;
  UINTN                     XhciNum = 1;
	

  DEBUG ((EFI_D_INFO, "InitUsbController() Start\n"));

  PeiUsbControllerInfo = (USB_CONTROLLER_INFO*) AllocateZeroPool(sizeof(USB_CONTROLLER_INFO));
  if (PeiUsbControllerInfo == NULL) {
    DEBUG ((EFI_D_ERROR, "Failed to allocate memory for PeiEhciDev!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  if (XhciNum) {
    HandleXhciFwForRecovery();
  }

  PeiUsbControllerInfo->Signature           = PEI_USB_CONTROLLER_SIGNATURE;
  PeiUsbControllerInfo->UsbControllerPpi    = mUsbControllerPpi;
  PeiUsbControllerInfo->PpiList             = mPpiList;
  PeiUsbControllerInfo->PpiList.Ppi         = &PeiUsbControllerInfo->UsbControllerPpi;
  PeiUsbControllerInfo->EhciControllersNum  = EhciNum;
  PeiUsbControllerInfo->XhciControllersNum  = XhciNum;
  PeiUsbControllerInfo->UhciControllersNum  = 0;
  PeiUsbControllerInfo->TotalUsbControllers = EhciNum + XhciNum;
  DEBUG ((EFI_D_INFO, "USB HostCount:%d\n", PeiUsbControllerInfo->TotalUsbControllers));

  InitUsbControl (PeiServices, PeiUsbControllerInfo);  

  Status = PeiServicesInstallPpi (&PeiUsbControllerInfo->PpiList);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((EFI_D_INFO, "InitUsbController() End\n"));

  return Status;

}


