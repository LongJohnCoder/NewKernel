
#include "PlatHost.h"

extern PLATFORM_HOST_INFO                gPlatHostInfo[];
extern PLAT_HOST_INFO_PROTOCOL           gPlatHostInfoProtocol;
extern PLATFORM_ISA_SERIAL_DEVICE_PATH   gSerialDevicePath;



EFI_DEVICE_PATH_PROTOCOL*
GetPlatUcrDp (
    UINTN   *pDpSize,
    UINT16  *pIoBase
  )
{
  PLATFORM_ISA_SERIAL_DEVICE_PATH  *UartDp;
  UINT16                           IoBase;


  if(!gSetupHob->UCREnable){
    return NULL;
  }
  
  UartDp = &gSerialDevicePath;
  
  switch(gSetupHob->SerialPortSelect){
    case 0:
      IoBase = 0x3F8;
      UartDp->IsaSerial.UID = 0;
      break;

    case 1:
      IoBase = 0x2F8;
      UartDp->IsaSerial.UID = 1;
      break;

    case 2:
      IoBase = 0x3E8;
      UartDp->IsaSerial.UID = 2;
      break;

    default:
    case 3:
      IoBase = 0x2E8;
      UartDp->IsaSerial.UID = 3;
      break;
  }  

  switch (gSetupHob->SerialBaudrate) {
    case 0:
      UartDp->Uart.BaudRate = 9600;
      break;
      
    case 1:
      UartDp->Uart.BaudRate = 19200;
      break;
      
    case 2:
      UartDp->Uart.BaudRate = 38400;
      break;
      
    case 3:
      UartDp->Uart.BaudRate = 57600;
      break;
      
    case 4:
    default:
      UartDp->Uart.BaudRate = 115200;
      break;
  }

  switch(gSetupHob->TerminalType) {
    case 0:
      CopyMem(&UartDp->TerminalType.Guid, &gEfiPcAnsiGuid, sizeof(EFI_GUID));
      break;

    default:  
    case 1:
      CopyMem(&UartDp->TerminalType.Guid, &gEfiVT100Guid, sizeof(EFI_GUID));
      break;
    case 2:
      CopyMem(&UartDp->TerminalType.Guid, &gEfiVT100PlusGuid, sizeof(EFI_GUID));
      break;
    case 3:
      CopyMem(&UartDp->TerminalType.Guid, &gEfiVTUTF8Guid, sizeof(EFI_GUID));
      break;
  }  

  if(pIoBase != NULL){
    *pIoBase = IoBase;
  }

  if(pDpSize != NULL){
    *pDpSize = sizeof(gSerialDevicePath);
  }

  return (EFI_DEVICE_PATH_PROTOCOL*)UartDp;
}  







UINT16
GetPlatSataPortIndex (
  IN  EFI_HANDLE          Handle
  )
{
  EFI_STATUS                    Status;
  UINT16                        DevIndex = 0xFFFF;
  EFI_DISK_INFO_PROTOCOL        *DiskInfo;
  UINT32                        IdeChannel;
  UINT32                        IdeDevice;
  UINT8                         SataMode;


  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDiskInfoProtocolGuid,
                  (VOID**)&DiskInfo
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  if(CompareGuid(&DiskInfo->Interface, &gEfiDiskInfoIdeInterfaceGuid)){
    SataMode = 1;
  } else if(CompareGuid(&DiskInfo->Interface, &gEfiDiskInfoAhciInterfaceGuid)){
    SataMode = 6;
  } else {
    goto ProcExit;
  }

  Status = DiskInfo->WhichIde (
                       DiskInfo,
                       &IdeChannel,
                       &IdeDevice
                       );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  if(SataMode == 1){
    DevIndex = (UINT16)(IdeChannel * 2 + IdeDevice);
  } else if(SataMode == 6){                                      // must AHCI
    DevIndex = (UINT16)IdeChannel;
  }

ProcExit:
  return DevIndex;
}



UINT16 
GetPlatSataHostIndex(
  EFI_HANDLE          Handle
  )
{
  UINTN                              Index;
  EFI_STATUS                         Status = EFI_SUCCESS;
  PLATFORM_HOST_INFO                 *HostInfo;
  PLATFORM_HOST_INFO_SATA_CTX        *Ctx;
  EFI_DEVICE_PATH_PROTOCOL           *SataHostDp;


  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID**)&SataHostDp
                  );
  if(EFI_ERROR(Status)){
    DEBUG((EFI_D_ERROR, "%a (L:%d) %r\n", __FUNCTION__, __LINE__, Status));
    goto ProcExit;
  }

  HostInfo = gPlatHostInfo;
  for(Index=0;Index<gPlatHostInfoProtocol.HostCount;Index++){
    if(HostInfo[Index].HostType != PLATFORM_HOST_SATA){
      continue;
    }
    Ctx = (PLATFORM_HOST_INFO_SATA_CTX*)(HostInfo[Index].HostCtx);
    if(CompareMem(HostInfo[Index].Dp, SataHostDp, Ctx->DpSize-4) == 0){
      if(gPlatHostInfoProtocol.SataHostCount == 1){
        return 0x8000;
      } else {
        return Ctx->HostIndex;
      }
    }
  }

ProcExit:
  return 0xFFFF;
}



VOID
PlatUpdateBootOption (
  EFI_BOOT_MANAGER_LOAD_OPTION  **BootOptions,
  UINTN                         *BootOptionCount
  )
{
  DEBUG((EFI_D_INFO, "%a(%d)\n", __FUNCTION__, *BootOptionCount));
}


