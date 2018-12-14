
#include "InternalBdsLib.h"
#include <Library/IoLib.h>
#include <Library/LogoLib.h>
#include <Library/PcdLib.h>
#include <Protocol/IsaIo.h>
#include <SysMiscCfg.h>
#include <Protocol/PlatHostInfoProtocol.h>
#include <Library/ByoCommLib.h>


#define   PCI_USB_UHCI_CLASS        0x0C030000
#define   PCI_USB_OHCI_CLASS        0x0C031000
#define   PCI_USB_EHCI_CLASS        0x0C032000
#define   PCI_USB_XHCI_CLASS        0x0C033000


CHAR16       *mConVarName[] = {
  L"ConIn",
  L"ConOut",
  L"ErrOut",
  L"ConInDev",
  L"ConOutDev",
  L"ErrOutDev"
};

EFI_GUID  gEfiVirtualIsaIoProtocolGuid = { 0x893e441f, 0x5a6a, 0x40d1, { 0x83, 0xcb, 0x8a, 0xa1, 0x6, 0xdd, 0xd7, 0xab } };

/**
  Performs an ISA I/O Read Cycle

  @param[in]  This              A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in]  Width             Specifies the width of the I/O operation.
  @param[in]  Offset            The offset in ISA I/O space to start the I/O operation.
  @param[in]  Count             The number of I/O operations to perform.
  @param[out] Buffer            The destination buffer to store the results

  @retval EFI_SUCCESS           The data was read from the device sucessfully.
  @retval EFI_UNSUPPORTED       The Offset is not valid for this device.
  @retval EFI_INVALID_PARAMETER Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
**/
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

/**
  Performs an ISA I/O Write Cycle

  @param[in] This                A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Width               Specifies the width of the I/O operation.
  @param[in] Offset              The offset in ISA I/O space to start the I/O operation.
  @param[in] Count               The number of I/O operations to perform.
  @param[in] Buffer              The source buffer to write data from

  @retval EFI_SUCCESS            The data was writen to the device sucessfully.
  @retval EFI_UNSUPPORTED        The Offset is not valid for this device.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
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



EFI_STATUS
EFIAPI
InstallVirtualIsaIo (
  IN OUT EFI_HANDLE *VirtualController
)
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT32                    PciClassCode;

  //
  //Install the NULL ISA I/O protocol on the Virtual Controller handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  VirtualController,
                  &gEfiVirtualIsaIoProtocolGuid,
                  &mIsaIoInterface,
                  NULL
                  );

  //
  // Here we need to connect USB host controller. Then the USB SMM driver
  // can start off bus enumeration and prepare for legacy service including port 60/64
  // trap. This is necessary to make virtual keyboard work.
  //
  HandleCount = 0;
  HandleBuffer = NULL;
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiPciIoProtocolGuid,
         NULL,
         &HandleCount,
         &HandleBuffer
         );
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    &PciIo
                    );
    if (!EFI_ERROR(Status)) {
      PciIo->Pci.Read (
                   PciIo,
                   EfiPciIoWidthUint32,
                   8,
                   1,
                   &PciClassCode
                   );
      PciClassCode &= 0x0FFFFFF00;
      if ((PciClassCode == PCI_USB_UHCI_CLASS) || (PciClassCode == PCI_USB_OHCI_CLASS) ||
          (PciClassCode == PCI_USB_EHCI_CLASS) || (PciClassCode == PCI_USB_XHCI_CLASS)) {
        gBS->ConnectController (HandleBuffer[Index], NULL, NULL, FALSE);
        break;
      }
    }
  }
  return Status;
}



VOID ShowGopDp()
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer = NULL;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *Dp;  


  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status) || HandleCount == 0) {
    goto ProcExit;
  }

  DEBUG((EFI_D_INFO, "GOPs:%d\n", HandleCount));
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol(HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID**)&Dp);
    ShowDevicePathDxe(gBS, Dp);
  }

ProcExit:
  if(HandleBuffer != NULL){
    FreePool(HandleBuffer);
  }
  return;
}




EFI_STATUS 
GetVideoController (
    EFI_HANDLE  **DisplayHost,
    UINTN       *HostCount
  )
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer = NULL;
  UINTN                     Index;
  EFI_HANDLE                VideoController = NULL;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT8                     ClassCode[3];
  PLAT_HOST_INFO_PROTOCOL   *ptHostInfo;
  EFI_DEVICE_PATH_PROTOCOL  *IgdPciDp = NULL;
  UINTN                     IgdPciDpSize = 0;
  EFI_HANDLE                IgdHandle = NULL;
  EFI_HANDLE                PegHandle = NULL;
  EFI_DEVICE_PATH_PROTOCOL  *PciDp;
  UINT32                    SystemMiscCfg;
  EFI_HANDLE                *VideoHandles = NULL;
  EFI_HANDLE                Temp;
  

  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  *HostCount = 0;
  SystemMiscCfg = PcdGet32(PcdSystemMiscConfig);

  Status = gBS->LocateProtocol(&gPlatHostInfoProtocolGuid, NULL, &ptHostInfo);
  if(!EFI_ERROR(Status)){
    IgdPciDp = ptHostInfo->IgdDp;
    IgdPciDpSize = ptHostInfo->IgdDpSize;
  }
  DEBUG((EFI_D_INFO, "IgdPciDp:%X, IgdPciDpSize:%X\n", IgdPciDp, IgdPciDpSize)); 

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status) || HandleCount == 0) {
    goto ProcExit;
  }

  for (Index = 0; Index < HandleCount; Index++) {

    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID**)&PciIo);
    ASSERT(!EFI_ERROR (Status));

    Status = PciIo->Pci.Read (
                      PciIo,
                      EfiPciIoWidthUint8,
                      0x9,
                      3,
                      ClassCode
                      );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "ClassCode Read error:%r\n", Status));
      continue;
    }

    if(ClassCode[2] != 3 || ClassCode[1] != 0 || ClassCode[0] != 0){
      continue;
    }

    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID**)&PciDp);
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "Dp not found\n"));
      continue;
    }
    if(IgdPciDp != NULL && CompareMem(PciDp, IgdPciDp, IgdPciDpSize) == 0){
      IgdHandle = HandleBuffer[Index];
    } else {
      if(PegHandle == NULL){
        PegHandle = HandleBuffer[Index];
      }
    }
  
    VideoHandles = ReallocatePool (
                     sizeof(EFI_HANDLE) * (*HostCount),
                     sizeof(EFI_HANDLE) * (*HostCount + 1),
                     VideoHandles
                     );
    VideoHandles[(*HostCount)++] = HandleBuffer[Index];

  }

  if(SystemMiscCfg & SYS_MISC_CFG_PRI_VGA_IGD){
    if(IgdHandle != NULL){
      VideoController = IgdHandle;
    } else {
      VideoController = PegHandle;
    }
  } else {
    if(PegHandle != NULL){
      VideoController = PegHandle;
    } else {
      VideoController = IgdHandle;
    }
  }

  if(VideoController != NULL){
    for(Index=0;Index<*HostCount;Index++){
      if(VideoHandles[Index] == VideoController){
        break;
      }
    }
    if(Index != 0){
      Temp = VideoHandles[Index];
      VideoHandles[Index] = VideoHandles[0];
      VideoHandles[0] = Temp;
    }
  }

ProcExit:
  if(HandleBuffer != NULL){
    FreePool(HandleBuffer);
  }
  if(*HostCount != 0){
    *DisplayHost = VideoHandles;
    Status = EFI_SUCCESS;
    DEBUG((EFI_D_INFO, "%a HostCount:%X\n", __FUNCTION__, *HostCount));
  } else {
    Status = EFI_NOT_FOUND;
  }
  return Status;
}



EFI_STATUS 
GetHostGopHandle (
  EFI_HANDLE                 PciHandle, 
  EFI_HANDLE                 *GopHandle,
  EFI_DEVICE_PATH_PROTOCOL   **ppGopDp
  )
{
  EFI_STATUS                 Status;
  EFI_DEVICE_PATH_PROTOCOL   *PciDp;
  EFI_DEVICE_PATH_PROTOCOL   *GopDp;  
  UINTN                      HandleCount;
  EFI_HANDLE                 *HandleBuffer = NULL;
  UINTN                      Index;
  UINTN                      PciDpSize;
  UINTN                      GopDpSize;


  Status = gBS->HandleProtocol(PciHandle, &gEfiDevicePathProtocolGuid, (VOID**)&PciDp);
  if(EFI_ERROR(Status)){
    DEBUG((EFI_D_ERROR, "%a() (L%d) %r\n", __FUNCTION__, __LINE__, Status));
    goto ProcExit;
  }    

  ShowDevicePathDxe(gBS, PciDp);

  PciDpSize = GetDevicePathSize(PciDp);

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status) || HandleCount == 0) {
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }

  DEBUG((EFI_D_INFO, "GOPs:%d, PciDpSize:%d\n", HandleCount, PciDpSize));

  Status = EFI_NOT_FOUND;
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol(HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID**)&GopDp);
    if(EFI_ERROR(Status)){
      continue;
    }
    GopDpSize = GetDevicePathSize(GopDp);
    DEBUG((EFI_D_INFO, "GopDpSize:%d\n", GopDpSize));    

    if(PciDpSize > 4 && GopDpSize >= PciDpSize && CompareMem(GopDp, PciDp, PciDpSize-4) == 0){
      *GopHandle = HandleBuffer[Index];
      *ppGopDp   = GopDp;
      Status = EFI_SUCCESS;
      break;
    }
  }

ProcExit:
  if(HandleBuffer != NULL){
    FreePool(HandleBuffer);
  }  
  DEBUG((EFI_D_INFO, "%a() Exit:%r\n", __FUNCTION__, Status));
  return Status;
}


/**
  Find the platform active active video controller and connect it.

  @retval EFI_NOT_FOUND There is no active video controller.
  @retval EFI_SUCCESS   The video controller is connected.
**/
EFI_STATUS
ConnectVideoController (
  VOID
  )
{
  EFI_HANDLE                 *DisplayHost = NULL;
  UINTN                      HostCount;
  EFI_DEVICE_PATH_PROTOCOL   *GopDp;
  EFI_STATUS                 Status;
  EFI_HANDLE                 GopHandle;
  UINTN                      Index;
  UINT32                     SystemMiscCfg;
  

  Status = GetVideoController(&DisplayHost, &HostCount);
  if (EFI_ERROR(Status)) {
    return EFI_NOT_FOUND;
  }

  SystemMiscCfg = PcdGet32(PcdSystemMiscConfig);

  if((SystemMiscCfg & SYS_MISC_CFG_DUAL_VGA) && HostCount > 1){
    Status = PcdSet32S (PcdVideoHorizontalResolution, 1024);
    Status = PcdSet32S (PcdVideoVerticalResolution, 768);
  }

  //
  // Try to connect the PCI device path, so that GOP dirver could start on this 
  // device and create child handles with GraphicsOutput Protocol installed
  // on them, then we get device paths of these child handles and select 
  // them as possible console device.
  //

  for(Index=0;Index<HostCount;Index++){

    DEBUG((EFI_D_INFO, "DisplayHost[%d]\n", Index));
    
    gBS->ConnectController (DisplayHost[Index], NULL, NULL, FALSE);

    Status = GetHostGopHandle(DisplayHost[Index], &GopHandle, &GopDp);
    if(EFI_ERROR(Status)){
      DEBUG((EFI_D_ERROR, "%a() (L%d) %r\n", __FUNCTION__, __LINE__, Status));
      continue;
    }

    ShowDevicePathDxe(gBS, GopDp);
    EfiBootManagerUpdateConsoleVariable(ConOut, GopDp, NULL);

// Necessary for ConPlatform and ConSplitter driver to start up again after ConOut is updated.
    Status = gBS->ConnectController (GopHandle, NULL, NULL, TRUE);
    DEBUG((EFI_D_INFO, "GOP[%d] : %r\n", Index, Status));

  }

  if(DisplayHost != NULL){
    FreePool(DisplayHost);
  }
  return Status;
}



/**
  Fill console handle in System Table if there are no valid console handle in.

  Firstly, check the validation of console handle in System Table. If it is invalid,
  update it by the first console device handle from EFI console variable. 

  @param  VarName            The name of the EFI console variable.
  @param  ConsoleGuid        Specified Console protocol GUID.
  @param  ConsoleHandle      On IN,  console handle in System Table to be checked. 
                             On OUT, new console hanlde in system table.
  @param  ProtocolInterface  On IN,  console protocol on console handle in System Table to be checked. 
                             On OUT, new console protocol on new console hanlde in system table.

  @retval TRUE               System Table has been updated.
  @retval FALSE              System Table hasn't been updated.

**/
BOOLEAN 
UpdateSystemTableConsole (
  IN     CHAR16                          *VarName,
  IN     EFI_GUID                        *ConsoleGuid,
  IN OUT EFI_HANDLE                      ConsoleHandle,
  IN OUT VOID                            *ProtocolInterface
  )
{
  EFI_STATUS                Status;
  UINTN                     DevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *FullDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *VarConsole;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  VOID                      *Interface;
  EFI_HANDLE                NewHandle;

  ASSERT (VarName != NULL);
  ASSERT (ConsoleHandle != NULL);
  ASSERT (ConsoleGuid != NULL);
  ASSERT (ProtocolInterface != NULL);

  if (ConsoleHandle != NULL) {
    Status = gBS->HandleProtocol (
                    ConsoleHandle,
                    ConsoleGuid,
                    &Interface
                    );
    if (Status == EFI_SUCCESS && Interface == ProtocolInterface) {
      DEBUG((EFI_D_INFO, "%a() L%d\n", __FUNCTION__, __LINE__));
      //
      // If ConsoleHandle is valid and console protocol on this handle also
      // also matched, just return.
      //
      return FALSE;
    }
  }
  
  //
  // Get all possible consoles device path from EFI variable
  //
  VarConsole = EfiBootManagerGetVariableAndSize (
                 VarName,
                 &gEfiGlobalVariableGuid,
                 &DevicePathSize
                 );
  if (VarConsole == NULL) {
    //
    // If there is no any console device, just return.
    //
    return FALSE;
  }

  FullDevicePath = VarConsole;

  do {
    //
    // Check every instance of the console variable
    //
    Instance  = GetNextDevicePathInstance (&VarConsole, &DevicePathSize);
    if (Instance == NULL) {
      DEBUG ((EFI_D_ERROR, "[Bds] No valid console instance is found for %s!\n", VarName));
      // We should not ASSERT when all the console devices are removed.
      // ASSERT_EFI_ERROR (EFI_NOT_FOUND);
      FreePool (FullDevicePath);
      return FALSE;
    }
    
    //
    // Find console device handle by device path instance
    //
    Status = gBS->LocateDevicePath (
                    ConsoleGuid,
                    &Instance,
                    &NewHandle
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Get the console protocol on this console device handle
      //
      Status = gBS->HandleProtocol (
                      NewHandle,
                      ConsoleGuid,
                      &Interface
                      );
      if (!EFI_ERROR (Status)) {
        //
        // Update new console handle in System Table.
        //
        DEBUG ((EFI_D_ERROR, "UpdateSystemTableConsole, VarName %s!\n", VarName));
        DEBUG ((EFI_D_ERROR, "UpdateSystemTableConsole, Return TURE.\n"));
        
        return TRUE;
      }
    }

  } while (Instance != NULL);

  //
  // No any available console devcie found.
  //
  return FALSE;
}

/**
  This function update console variable based on ConVarName, it can
  add or remove one specific console device path from the variable

  @param  ConVarName               Console related variable name, ConIn, ConOut,
                                   ErrOut.
  @param  CustomizedConDevicePath  The console device path which will be added to
                                   the console variable ConVarName, this parameter
                                   can not be multi-instance.
  @param  ExclusiveDevicePath      The console device path which will be removed
                                   from the console variable ConVarName, this
                                   parameter can not be multi-instance.

  @retval EFI_UNSUPPORTED          The added device path is same to the removed one.
  @retval EFI_SUCCESS              Success add or remove the device path from  the
                                   console variable.

**/
EFI_STATUS
EFIAPI
EfiBootManagerUpdateConsoleVariable (
  IN  CONSOLE_TYPE              ConsoleType,
  IN  EFI_DEVICE_PATH_PROTOCOL  *CustomizedConDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ExclusiveDevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *VarConsole;
  UINTN                     DevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath;

  if (ConsoleType >= sizeof (mConVarName) / sizeof (mConVarName[0])) {
    return EFI_INVALID_PARAMETER;
  }

  VarConsole      = NULL;
  DevicePathSize  = 0;

  //
  // Notes: check the device path point, here should check
  // with compare memory
  //
  if (CustomizedConDevicePath == ExclusiveDevicePath) {
    return EFI_UNSUPPORTED;
  }
  //
  // Delete the ExclusiveDevicePath from current default console
  //
  VarConsole = EfiBootManagerGetVariableAndSize (
                 mConVarName[ConsoleType],
                 &gEfiGlobalVariableGuid,
                 &DevicePathSize
                 );

  //
  // Initialize NewDevicePath
  //
  NewDevicePath = VarConsole;

  //
  // If ExclusiveDevicePath is even the part of the instance in VarConsole, delete it.
  // In the end, NewDevicePath is the final device path.
  //
  if (ExclusiveDevicePath != NULL && VarConsole != NULL) {
      NewDevicePath = EfiBootManagerDelPartMatchInstance (VarConsole, ExclusiveDevicePath);
  }
  //
  // Try to append customized device path to NewDevicePath.
  //
  if (CustomizedConDevicePath != NULL) {
    if (!EfiBootManagerMatchDevicePaths (NewDevicePath, CustomizedConDevicePath)) {
      //
      // Check if there is part of CustomizedConDevicePath in NewDevicePath, delete it.
      //
      NewDevicePath = EfiBootManagerDelPartMatchInstance (NewDevicePath, CustomizedConDevicePath);
      //
      // In the first check, the default console variable will be _ModuleEntryPoint,
      // just append current customized device path
      //
      TempNewDevicePath = NewDevicePath;
      NewDevicePath = AppendDevicePathInstance (NewDevicePath, CustomizedConDevicePath);
      if (TempNewDevicePath != NULL) {
        FreePool(TempNewDevicePath);
      }
    }
  }

  //
  // Finally, Update the variable of the default console by NewDevicePath
  //
  gRT->SetVariable (
         mConVarName[ConsoleType],
         &gEfiGlobalVariableGuid,
         EFI_VARIABLE_BOOTSERVICE_ACCESS
         | EFI_VARIABLE_RUNTIME_ACCESS
         | ((ConsoleType < ConInDev) ? EFI_VARIABLE_NON_VOLATILE : 0),
         GetDevicePathSize (NewDevicePath),
         NewDevicePath
         );

  if (VarConsole == NewDevicePath) {
    if (VarConsole != NULL) {
      FreePool(VarConsole);
    }
  } else {
    if (VarConsole != NULL) {
      FreePool(VarConsole);
    }
    if (NewDevicePath != NULL) {
      FreePool(NewDevicePath);
    }
  }

  return EFI_SUCCESS;

}


EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
EfiBootManagerUpdateConsoleDP (
  IN  EFI_DEVICE_PATH_PROTOCOL  *VarConsole,
  IN  EFI_DEVICE_PATH_PROTOCOL  *CustomizedConDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ExclusiveDevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath2;	


  ASSERT(CustomizedConDevicePath != ExclusiveDevicePath);

  NewDevicePath = DuplicateDevicePath(VarConsole);
	TempNewDevicePath2 = NewDevicePath;
	
  if (NewDevicePath != NULL && ExclusiveDevicePath != NULL) {
    NewDevicePath = EfiBootManagerDelPartMatchInstance(NewDevicePath, ExclusiveDevicePath);
  } 
	
  if (CustomizedConDevicePath != NULL) {
    if (!EfiBootManagerMatchDevicePaths (NewDevicePath, CustomizedConDevicePath)) {
      NewDevicePath = EfiBootManagerDelPartMatchInstance(NewDevicePath, CustomizedConDevicePath);
      TempNewDevicePath = NewDevicePath;
      NewDevicePath = AppendDevicePathInstance (NewDevicePath, CustomizedConDevicePath);
      if (TempNewDevicePath != NULL) { FreePool(TempNewDevicePath); }
    }
  }

  if(VarConsole!=NULL){FreePool(VarConsole);}
  if(NewDevicePath != TempNewDevicePath2 && TempNewDevicePath2!=NULL){FreePool(TempNewDevicePath2);}
  return NewDevicePath;
}



/**
  Connect the console device base on the variable ConVarName, if
  device path of the ConVarName is multi-instance device path, if
  anyone of the instances is connected success, then this function
  will return success.

  @param  ConsoleType              ConIn, ConOut or ErrOut.

  @retval EFI_NOT_FOUND            There is not any console devices connected
                                   success
  @retval EFI_SUCCESS              Success connect any one instance of the console
                                   device path base on the variable ConVarName.

**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectConsoleVariable (
  IN  CONSOLE_TYPE              ConsoleType
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *StartDevicePath = NULL;
  UINTN                     VariableSize;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *Next;
  EFI_DEVICE_PATH_PROTOCOL  *CopyOfDevicePath;
  UINTN                     Size;
  BOOLEAN                   DeviceExist = FALSE;


  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  
  if ((ConsoleType != ConIn) && (ConsoleType != ConOut) && (ConsoleType != ErrOut)) {
    return EFI_INVALID_PARAMETER;
  }
  
  Status      = EFI_SUCCESS;
  DeviceExist = FALSE;

  if(ConsoleType == ConOut){
    Status = ConnectVideoController();
    if (!EFI_ERROR (Status)) {
      DeviceExist = TRUE;
    }
  }

  StartDevicePath = EfiBootManagerGetVariableAndSize (
                      mConVarName[ConsoleType],
                      &gEfiGlobalVariableGuid,
                      &VariableSize
                      );
  if (StartDevicePath == NULL) {
    DEBUG((EFI_D_ERROR, "L\"%s\" not found\n", mConVarName[ConsoleType]));
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }
//ShowDevicePath(L"DP:", StartDevicePath, UNI_CRLF);

  CopyOfDevicePath = StartDevicePath;
  do {
    Instance = GetNextDevicePathInstance(&CopyOfDevicePath, &Size);
    if (Instance == NULL) {
      FreePool (StartDevicePath);
      return EFI_UNSUPPORTED;
    }
    Next = Instance;
    while (!IsDevicePathEndType (Next)) {
      Next = NextDevicePathNode (Next);
    }

    SetDevicePathEndNode (Next);
 
    if ((DevicePathType (Instance) == MESSAGING_DEVICE_PATH) &&
        ((DevicePathSubType (Instance) == MSG_USB_CLASS_DP) || (DevicePathSubType (Instance) == MSG_USB_WWID_DP))
       ) {
      Status = EfiBootManagerConnectUsbShortFormDevicePath (Instance);
      if (!EFI_ERROR (Status)) {
        DeviceExist = TRUE;
      }
    } else {
      Status = EfiBootManagerConnectDevicePath (Instance, NULL);
      DEBUG((EFI_D_INFO, "EfiBootManagerConnectDevicePath():%r\n", Status));
      if (!EFI_ERROR (Status)) {
        DeviceExist = TRUE;
      }
    } 
    
    FreePool(Instance);
  } while (CopyOfDevicePath != NULL);
  

ProcExit:
  if(StartDevicePath!=NULL){FreePool (StartDevicePath);}
  if (!DeviceExist) {
    return EFI_NOT_FOUND;
  }
  return EFI_SUCCESS;
}


/**
  This function will search every input/output device in current system,
  and make every input/output device as potential console device.
**/
VOID
EFIAPI
EfiBootManagerConnectAllDefaultConsoles (
  VOID
  )
{
  EFI_STATUS                Status;


  DEBUG((EFI_D_INFO, "ConnectDefCon\n"));  
  
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_PC_INIT));
  PERF_START (NULL, "ConnectConOut", "BDS", 0);	
  Status = EfiBootManagerConnectConsoleVariable (ConOut);
  PERF_END   (NULL, "ConnectConOut", "BDS", 0);	
  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (EFI_ERROR_CODE, (EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_NOT_DETECTED));  
  } else {
// ConSplitterConOutDriverBindingStart.ConSplitterTextOutAddDevice() 
// will clear screen and enable EnableCursor.
    if(gST->ConOut!=NULL){gST->ConOut->EnableCursor(gST->ConOut, FALSE);}
    ShowPostLogo();
  }

  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_INIT));
  PERF_START (NULL, "ConnectConIn", "BDS", 0);	
  Status = EfiBootManagerConnectConsoleVariable (ConIn);
  PERF_END   (NULL, "ConnectConIn", "BDS", 0);	
  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (EFI_ERROR_CODE, (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_NOT_DETECTED));  
  }

  //
  // The _ModuleEntryPoint err out var is legal.
  //
//-EfiBootManagerConnectConsoleVariable (ErrOut);
//-PERF_START (NULL, "ErrOutReady", "BDS", 1);
//-PERF_END   (NULL, "ErrOutReady", "BDS", 0);


  UpdateSystemTableConsole (L"ConIn", &gEfiSimpleTextInProtocolGuid, gST->ConsoleInHandle, (VOID*)gST->ConIn);
  UpdateSystemTableConsole (L"ConOut", &gEfiSimpleTextOutProtocolGuid, gST->ConsoleOutHandle, (VOID*)gST->ConOut);

  DEBUG((EFI_D_INFO, "gST->ConsoleOutHandle:%X\n", gST->ConsoleOutHandle));
  DEBUG((EFI_D_INFO, "gST->ConsoleInHandle :%X\n", gST->ConsoleInHandle));  

//-  if (UpdateSystemTableConsole (L"ErrOut", &gEfiSimpleTextOutProtocolGuid, &gST->StandardErrorHandle, (VOID **) &gST->StdErr)) {
//-    SystemTableUpdated = TRUE;
//-  }

  DEBUG((EFI_D_INFO, "ConnectDefCon Exit\n"));   
}


