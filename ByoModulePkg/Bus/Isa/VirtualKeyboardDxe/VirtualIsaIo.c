
#include <Protocol/IsaIo.h>
#include <Protocol/VirtualKeyboardGuid.h>
#include <Protocol/PciIo.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>

EFI_STATUS
EFIAPI
IsaIoIoRead (
  IN  EFI_ISA_IO_PROTOCOL        *This,
  IN  EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN  UINT32                     Offset,
  IN  UINTN                      Count,
  OUT VOID                       *Buffer
  )
{
  UINT8  *Data;

  Data  = (UINT8 *)Buffer;
  *Data = IoRead8 ((UINT8)Offset);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IsaIoIoWrite (
  IN EFI_ISA_IO_PROTOCOL        *This,
  IN EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN UINT32                     Offset,
  IN UINTN                      Count,
  IN VOID                       *Buffer
  )
{
  UINT8  *Data;

  Data = Buffer;
  IoWrite8 ((UINT8)Offset, *Data);
  return EFI_SUCCESS;
}

//
// Module Variables
//
EFI_ISA_IO_PROTOCOL mIsaIoInterface = {
  {
    NULL,
    NULL
  },
  {
    IsaIoIoRead,
    IsaIoIoWrite
  },
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  0,
  NULL
};

extern EFI_GUID gEfiUsb2HcProtocolGuid;

void
InstallVirtualIsaIo (
  IN EFI_EVENT    Event,
  IN VOID         *Context
)
{
  EFI_STATUS    Status;
  void          *HcProtocol;
  EFI_HANDLE    VirtualController = NULL;
  
  Status = gBS->LocateProtocol(&gEfiUsb2HcProtocolGuid, NULL, (VOID**)&HcProtocol);
  if (EFI_ERROR(Status)) {
    return;
  }

  gBS->CloseEvent(Event);

  //
  //Install the NULL ISA I/O protocol on the Virtual Controller handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &VirtualController,
                  &gEfiVirtualIsaIoProtocolGuid,
                  &mIsaIoInterface,
                  NULL
                  );
  DEBUG ((EFI_D_ERROR,"%a(),VirtualController:%x,Status:%r\n",__FUNCTION__,VirtualController,Status));
  Status = gBS->ConnectController(VirtualController, NULL, NULL, FALSE);

  return;
}

EFI_STATUS
EFIAPI
VirtualIsaIoDxeEntry(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  void  *Registration;
  
  if (!PcdGetBool (PcdKbcPresent)) {
    DEBUG ((EFI_D_ERROR,"%a() \n",__FUNCTION__));
    InstallVirtualIsaIo (NULL, NULL);
    EfiCreateProtocolNotifyEvent (
      &gEfiUsb2HcProtocolGuid,
      TPL_CALLBACK,
      InstallVirtualIsaIo,
      NULL,
      &Registration
      );
  }
  return EFI_SUCCESS;
}

