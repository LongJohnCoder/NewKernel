
#include "BdsPlatform.h"
#include <Guid/SetupPassword.h>
#include <Protocol/ExitPmAuth.h>
#include <Protocol/DxeSmmReadyToLock.h>
#include <Protocol/AcpiS3Save.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/LegacyBiosPlatform.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/UpdateFlash.h>
#include <Library/ReportStatusCodeLib.h>
#include <ByoCapsuleExt.h>
#include <Protocol/EsrtManagement.h>
#include <Protocol/PlatHostInfoProtocol.h>
#include <library/ByoCommLib.h>
#include <library/PerformanceLib.h>


VOID
InvokeGetBbsInfo (
  VOID
  );

VOID
InstallAdditionalOpRom (
  VOID
  );


/**
  This procedure is used to process flash update under
  reocver mode or flash update mode

  @param  BootMode              Indicate which boot mode system is in

  @return  EFI_NOT_FOUND        Not find flash update proccess procedure
  @return  EFI_SUCCESS          Flash update is proccessed successfully

**/
EFI_STATUS
ProccessFlashUpdate (
  EFI_BOOT_MODE   BootMode
  )
{
  EFI_STATUS               Status;
  UPDATE_FLASH_PROTOCOL    *UpdateFlash;
  EFI_INPUT_KEY            Key;


  Status = gBS->LocateProtocol(&gUpdateFlashProtocolGuid, NULL, &UpdateFlash);
  if (EFI_ERROR(Status)) {
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_RED);	
    Print (L"BiosUpdate Not Ready!\n");
    while(1){}
  }

  Status = UpdateFlash->ProcessFlash(BootMode);

// recovery fail
  if(BootMode == BOOT_IN_RECOVERY_MODE){
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_RED);	
    Print (L"BIOS Recovery failed\n");
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);    
    Print (L"Press Any Key to reboot...\n");
    while (gST->ConIn != NULL) {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);    	
      if (!EFI_ERROR (Status)) {      
        gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
      }
      gBS->Stall (1000 * 100);
    }
  }
  
  return Status;
}

/**
  This function converts an input device structure to a Unicode string.

  @param DevPath                  A pointer to the device path structure.

  @return A new allocated Unicode string that represents the device path.

**/
CHAR16 *
DevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  )
{
  EFI_STATUS                       Status;
  CHAR16                           *ToText;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *DevPathToText;

  if (DevPath == NULL) {
    return NULL;
  }

  Status = gBS->LocateProtocol (
                  &gEfiDevicePathToTextProtocolGuid,
                  NULL,
                  (VOID **) &DevPathToText
                  );
  ASSERT_EFI_ERROR (Status);
  ToText = DevPathToText->ConvertDevicePathToText (
                            DevPath,
                            FALSE,
                            TRUE
                            );
  ASSERT (ToText != NULL);
  return ToText;
}



VOID
UpdateEfiGlobalVariable (
  CHAR16           *VariableName,
  EFI_GUID         *AgentGuid,
  PROCESS_VARIABLE ProcessVariable
  )
/*++

Routine Description:

  Generic function to update the console variable.
  Please refer to FastBootSupport.c for how to use it.

Arguments:

  VariableName    - The name of the variable to be updated
  AgentGuid       - The Agent GUID
  ProcessVariable - The function pointer to update the variable
                    NULL means to restore to the original value

--*/
{
  EFI_STATUS  Status;
  CHAR16      BackupVariableName[20];
  CHAR16      FlagVariableName[20];
  VOID        *Variable;
  VOID        *BackupVariable;
  VOID        *NewVariable;
  UINTN       VariableSize;
  UINTN       BackupVariableSize;
  UINTN       NewVariableSize;
  BOOLEAN     Flag;
  BOOLEAN     *FlagVariable;
  UINTN       FlagSize;
  CHAR16      *Str;

  ASSERT (StrLen (VariableName) <= 13);
  UnicodeSPrint (BackupVariableName, sizeof (BackupVariableName), L"%sBackup", VariableName);
  UnicodeSPrint (FlagVariableName, sizeof (FlagVariableName), L"%sModify", VariableName);

  Variable       = EfiBootManagerGetVariableAndSize (VariableName, &gEfiGlobalVariableGuid, &VariableSize);
  BackupVariable = EfiBootManagerGetVariableAndSize (BackupVariableName, AgentGuid, &BackupVariableSize);
  FlagVariable   = EfiBootManagerGetVariableAndSize (FlagVariableName, AgentGuid, &FlagSize);
  if (ProcessVariable != NULL) {
    if (FlagVariable == NULL) {
      //
      // Last boot is normal boot
      // Set flag
      // BackupVariable <- Variable
      // Variable       <- ProcessVariable (Variable)
      //
      Flag   = TRUE;
      Status = gRT->SetVariable (
                      FlagVariableName,
                      AgentGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      sizeof (Flag),
                      &Flag
                      );
      ASSERT_EFI_ERROR (Status);

      Status = gRT->SetVariable (
                      BackupVariableName,
                      AgentGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      VariableSize,
                      Variable
                      );
      ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));


      NewVariable     = Variable;
      NewVariableSize = VariableSize;
      ProcessVariable (&NewVariable, &NewVariableSize);
      DEBUG ((EFI_D_ERROR, "============================%s============================\n", VariableName));
      Str = DevicePathToStr ((EFI_DEVICE_PATH_PROTOCOL *) Variable);    DEBUG ((EFI_D_ERROR, "O:%s\n", Str)); gBS->FreePool (Str);
      Str = DevicePathToStr ((EFI_DEVICE_PATH_PROTOCOL *) NewVariable); DEBUG ((EFI_D_ERROR, "N:%s\n", Str)); gBS->FreePool (Str);
      
      Status = gRT->SetVariable (
                      VariableName,
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      NewVariableSize,
                      NewVariable
                      );
      ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));

      if (NewVariable != NULL) {
        FreePool (NewVariable);
      }
    } else { // LastBootIsModifiedPtr != NULL
      //
      // Last Boot is modified boot
      //
    }
  } else {
    if (FlagVariable != NULL) {
      //
      // Last boot is modified boot
      // Clear LastBootIsModified flag
      // Variable       <- BackupVariable
      // BackupVariable <- NULL
      //
      Status = gRT->SetVariable (
                      FlagVariableName,
                      AgentGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      0,
                      NULL
                      );
      ASSERT_EFI_ERROR (Status);

      Status = gRT->SetVariable (
                      VariableName,
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      BackupVariableSize,
                      BackupVariable
                      );
      ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));

      Status = gRT->SetVariable (
                      BackupVariableName,
                      AgentGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      0,
                      NULL
                      );
      ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));
    } else { // LastBootIsModifiedPtr == NULL
      //
      // Last boot is normal boot
      //
    }
  }

  if (Variable != NULL) {
    FreePool (Variable);
  }

  if (BackupVariable != NULL) {
    FreePool (BackupVariable);
  }

  if (FlagVariable != NULL) {
    FreePool (FlagVariable);
  }

}



VOID
SignalAllDriversConnected (
  VOID
  )
{
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  SignalProtocolEvent(gBS, &gBdsAllDriversConnectedProtocolGuid, FALSE);
}



VOID
ExitPmAuth (
  VOID
  )
{
  EFI_STATUS                 Status;
  EFI_ACPI_S3_SAVE_PROTOCOL  *AcpiS3Save;

  
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));  
  //
  // Prepare S3 information, this MUST be done before ExitPmAuth
  //
  Status = gBS->LocateProtocol (&gEfiAcpiS3SaveProtocolGuid, NULL, (VOID **)&AcpiS3Save);
  if (!EFI_ERROR (Status)) {
    AcpiS3Save->S3Save (AcpiS3Save, NULL);
  }

  //
  // Inform the SMM infrastructure that we're entering BDS and may run 3rd party code hereafter 
  // NOTE: We can NOT put it to PlatformBdsInit, because many boot script touch PCI BAR. :-(
  //       We have to connect PCI root bridge, allocate resource, then ExitPmAuth().
  //
  SignalProtocolEvent(gBS, &gExitPmAuthProtocolGuid, FALSE);

  EfiEventGroupSignal(&gEfiEndOfDxeEventGroupGuid);
  
  //
  // NOTE: We need install DxeSmmReadyToLock directly here because many boot script is added via ExitPmAuth callback.
  // If we install them at same callback, these boot script will be rejected because BootScript Driver runs first to lock them done.
  // So we seperate them to be 2 different events, ExitPmAuth is last chance to let platform add boot script. DxeSmmReadyToLock will
  // make boot script save driver lock down the interface.
  //
  SignalProtocolEvent(gBS, &gEfiDxeSmmReadyToLockProtocolGuid, FALSE);

}








VOID
ConnectRootBridge (
  VOID
  )
{
  UINTN                            RootBridgeHandleCount;
  EFI_HANDLE                       *RootBridgeHandleBuffer = NULL;
  UINTN                            RootBridgeIndex;


  InvokeHookProtocol(gBS, &gEfiBeforeConnectPciRootBridgeGuid);

  RootBridgeHandleCount = 0;
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiPciRootBridgeIoProtocolGuid,
         NULL,
         &RootBridgeHandleCount,
         &RootBridgeHandleBuffer
         );
  for (RootBridgeIndex = 0; RootBridgeIndex < RootBridgeHandleCount; RootBridgeIndex++) {
    gBS->ConnectController(RootBridgeHandleBuffer[RootBridgeIndex], NULL, NULL, FALSE);
  }

  InvokeHookProtocol(gBS, &gEfiAfterAllPciIoGuid); 

  if(RootBridgeHandleBuffer!=NULL){FreePool(RootBridgeHandleBuffer);}	

  InvokeHookProtocol(gBS, &gEfiAfterConnectPciRootBridgeGuid); 

  DEBUG((EFI_D_INFO, "RootBrigdeConnected\n"));
}


BOOLEAN
IsGopDevicePath (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  while (!IsDevicePathEndType (DevicePath)) {
    if (DevicePathType (DevicePath) == ACPI_DEVICE_PATH &&
        DevicePathSubType (DevicePath) == ACPI_ADR_DP) {
      return TRUE;
    }
    DevicePath = NextDevicePathNode (DevicePath);
  }
  return FALSE;
}


//
// BDS Platform Functions
//

/**
  Connect the USB short form device path.

  @param DevicePath   USB short form device path

  @retval EFI_SUCCESS           Successfully connected the USB device
  @retval EFI_NOT_FOUND         Cannot connect the USB device
  @retval EFI_INVALID_PARAMETER The device path is invalid.
**/
EFI_STATUS
ConnectUsbShortFormDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
{
  EFI_STATUS                            Status;
  EFI_HANDLE                            *Handles;
  UINTN                                 HandleCount;
  UINTN                                 Index;
  EFI_PCI_IO_PROTOCOL                   *PciIo;
  UINT8                                 Class[3];
  BOOLEAN                               AtLeastOneConnected;

  //
  // Check the passed in parameters
  //
  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DevicePathType (DevicePath) != MESSAGING_DEVICE_PATH) ||
      ((DevicePathSubType (DevicePath) != MSG_USB_CLASS_DP) && (DevicePathSubType (DevicePath) != MSG_USB_WWID_DP))
     ) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find the usb host controller firstly, then connect with the remaining device path
  //
  AtLeastOneConnected = FALSE;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **) &PciIo
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Check whether the Pci device is the wanted usb host controller
      //
      Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x09, 3, &Class);
      if (!EFI_ERROR (Status) &&
          ((PCI_CLASS_SERIAL == Class[2]) && (PCI_CLASS_SERIAL_USB == Class[1]))
         ) {
        Status = gBS->ConnectController (
                        Handles[Index],
                        NULL,
                        DevicePath,
                        FALSE
                        );
        if (!EFI_ERROR(Status)) {
          AtLeastOneConnected = TRUE;
        }
      }
    }
  }

  return AtLeastOneConnected ? EFI_SUCCESS : EFI_NOT_FOUND;
}



VOID
UpdateCsm16UCR (
  IN UINT16   UartIoBase
  )
{
  UINTN                      Ebda;

  Ebda = (*(UINT16*)(UINTN)0x40E) << 4;

  *((volatile UINT8*) (Ebda + 0x1C2))  = 1;                  //flag for CSM support
  *((volatile UINT16*)(Ebda + 0x1C3)) = UartIoBase;
}



EFI_STATUS
UpdateSerialConsoleVariable(
  PLAT_HOST_INFO_PROTOCOL *HostInfo
  ) 
{
  EFI_DEVICE_PATH_PROTOCOL  *VarConsole = NULL;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *SerialDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *Instance = NULL;
  EFI_DEVICE_PATH_PROTOCOL  *Next;
  UINTN                     Size = 0;
  BOOLEAN                   Found = FALSE;
  UINT16                    UartIoBase;
  BOOLEAN                   UcrEnable;
  UINTN                     DpSize;
  

  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  
  SerialDevicePath = HostInfo->GetPlatUcrDp(&DpSize, &UartIoBase);
  if(SerialDevicePath == NULL){
    UcrEnable = FALSE;
  } else {
    UcrEnable = TRUE;
  }
  
  VarConsole = EfiBootManagerGetVariableAndSize (
                 L"ConOut",
                 &gEfiGlobalVariableGuid,
                 &Size
                 );
  DevicePath = VarConsole;
  
  do {
    if(Instance != NULL){
      FreePool(Instance);
      Instance = NULL;
    }
    Instance = GetNextDevicePathInstance (&DevicePath, &Size);
    if(Instance == NULL){
      break;
    }

    Next = Instance;
    while (!IsDevicePathEndType(Next)) {
      //
      // Early break when it's a serial device path
      // Note: Check EISA_PNP_ID (0x501) instead of UART node because we only want to disable the local serial but not the SOL
      //       SOL device path doesn't contain the EISA_PNP_ID (0x501)
      //
      if ((Next->Type == ACPI_DEVICE_PATH) && (Next->SubType == ACPI_DP) &&
          (((ACPI_HID_DEVICE_PATH *) Next)->HID == EISA_PNP_ID (0x501))
          ) {
        Found = TRUE;
        break;
      }
      Next = NextDevicePathNode(Next);
    }
  } while (DevicePath != NULL && Found == FALSE);

  if (Found) {
    if (!UcrEnable) {
      EfiBootManagerUpdateConsoleVariable (ConIn, NULL, Instance);
      EfiBootManagerUpdateConsoleVariable (ConOut, NULL, Instance);
    }
  }

  if (UcrEnable) {
    UpdateCsm16UCR(UartIoBase);
    if (Found && CompareMem(SerialDevicePath, Instance, DpSize) != 0) {
      EfiBootManagerUpdateConsoleVariable (ConIn, NULL, Instance);
      EfiBootManagerUpdateConsoleVariable (ConOut, NULL, Instance);
    }
    EfiBootManagerUpdateConsoleVariable (ConIn, SerialDevicePath, NULL);
    EfiBootManagerUpdateConsoleVariable (ConOut, SerialDevicePath, NULL);
  }

  if(VarConsole != NULL) {
    FreePool(VarConsole);
  }
  if(Instance != NULL) {
    FreePool(Instance);
  }

  return EFI_SUCCESS;
}



EFI_HII_HANDLE gStringPackHandle;

VOID
EFIAPI
PlatformBootManagerBeforeConsole (
  VOID
  )
/*++

Routine Description:

  Platform Bds init. Incude the platform firmware vendor, revision
  and so crc check.

Arguments:

Returns:

  None.

--*/
{
  EFI_STATUS                          Status = EFI_SUCCESS;
  UINTN                               Index;
  PLAT_HOST_INFO_PROTOCOL             *HostInfo;


  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_BEGIN_CONNECTING_DRIVERS)
    );

  ConnectRootBridge();
	
  gStringPackHandle = HiiAddPackages (
                        &gEfiCallerIdGuid,
                        gImageHandle,
                        BdsDxeStrings,
                        NULL
                        );
  ASSERT (gStringPackHandle != NULL);

  Status = gBS->LocateProtocol(&gPlatHostInfoProtocolGuid, NULL, &HostInfo);
  ASSERT(!EFI_ERROR(Status));

  DEBUG((EFI_D_INFO, "ConInDpCount:%d\n", HostInfo->ConInDpCount));
  for (Index = 0; Index < HostInfo->ConInDpCount; Index++) {
    Status = EfiBootManagerUpdateConsoleVariable (ConIn, HostInfo->ConInDp[Index], NULL);
    DEBUG((EFI_D_INFO, "UpdateVar_ConIn:%r\n", Status));
  }
  
  UpdateSerialConsoleVariable(HostInfo);

  gRT->SetVariable (
         EFI_CAPSULE_VARIABLE_NAME,
         &gEfiCapsuleVendorGuid,
         EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
         0,
         NULL
         );
  gRT->SetVariable (
         EFI_CAPSULE_EXT_VARIABLE_NAME,
         &gEfiCapsuleVendorGuid,
         EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
         0,
         NULL
         );

}

VOID
ConnectSequence (
  VOID
  )
{
  UINTN                      Index;
  EFI_HANDLE                 DeviceHandle;
  EFI_STATUS                 Status;
  PLATFORM_HOST_INFO         *HostInfo;
  EFI_DEVICE_PATH_PROTOCOL   *Dp;
  PLAT_HOST_INFO_PROTOCOL    *ptHostInfo;
  EFI_PCI_IO_PROTOCOL        *PciIo;
  UINT32                     PciId;
  
  
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  Status = gBS->LocateProtocol(&gPlatHostInfoProtocolGuid, NULL, &ptHostInfo);
  if(EFI_ERROR(Status)){
    DEBUG((EFI_D_INFO, "[ERROR] PlatHostInfo not found\n"));
    return;
  }

  HostInfo = ptHostInfo->HostList;
  for (Index = 0; Index < ptHostInfo->HostCount; Index++) {
    Dp = HostInfo[Index].Dp;
    Status = gBS->LocateDevicePath(&gEfiPciIoProtocolGuid, &Dp, &DeviceHandle);
    if (!EFI_ERROR (Status) && IsDevicePathEnd(Dp)) {
      Status = gBS->HandleProtocol(DeviceHandle, &gEfiPciIoProtocolGuid, &PciIo);
      PciIo->Pci.Read(PciIo, EfiPciIoWidthUint32, 0, 1, &PciId);
      Status = gBS->ConnectController(DeviceHandle, NULL, NULL, TRUE);
      DEBUG((EFI_D_INFO, "CC[%d]:%r(%08X)\n", Index, Status, PciId));      
    } 
  }
}





VOID
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
  )
/*++

Routine Description:

  The function will excute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.
  
Arguments:
  DriverOptionList - The header of the driver option link list
  BootOptionList   - The header of the boot option link list
  ProcessCapsules  - A pointer to ProcessCapsules()
  BaseMemoryTest   - A pointer to BaseMemoryTest()
 
Returns:
  None.
  
--*/
{
  EFI_BOOT_MODE                 BootMode;
  ESRT_MANAGEMENT_PROTOCOL      *EsrtManagement;
  EFI_STATUS                    Status;


  InvokeHookProtocol(gBS, &gPlatAfterConsoleStartProtocolGuid);

  Status = gBS->LocateProtocol(&gEsrtManagementProtocolGuid, NULL, (VOID **)&EsrtManagement);
  if (EFI_ERROR(Status)) {
    EsrtManagement = NULL;
  }

  BootMode = GetBootModeHob();
  
  switch (BootMode) {

    case BOOT_ASSUMING_NO_CONFIGURATION_CHANGES:
    case BOOT_WITH_MINIMAL_CONFIGURATION:
    case BOOT_ON_S4_RESUME:
      PERF_START(NULL, "ConnectSequence", "BDS", 0);      
      ConnectSequence();
      PERF_END(NULL, "ConnectSequence", "BDS", 0);

      PERF_START(NULL, "LegacyOpRom", "BDS", 0);      
      InstallAdditionalOpRom();
      PERF_END(NULL, "LegacyOpRom", "BDS", 0);

      PERF_START(NULL, "AllConnectEvent", "BDS", 0); 
      SignalAllDriversConnected();
      PERF_END(NULL, "AllConnectEvent", "BDS", 0);
      
      InvokeGetBbsInfo();	

      if (EsrtManagement != NULL) {
        EsrtManagement->LockEsrtRepository();
      }
      
      break;


    case BOOT_ON_FLASH_UPDATE:
      if (EsrtManagement != NULL) {
        EsrtManagement->SyncEsrtFmp();
      }
      InvokeHookProtocol(gBS, &gPlatBeforeBiosUpdateProtocolGuid);
      ProccessFlashUpdate(BOOT_ON_FLASH_UPDATE);
      break;

    case BOOT_IN_RECOVERY_MODE:
      InvokeHookProtocol(gBS, &gPlatBeforeBiosUpdateProtocolGuid);
      ProccessFlashUpdate(BOOT_IN_RECOVERY_MODE);
      break;

    case BOOT_WITH_FULL_CONFIGURATION:
    case BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS:
    case BOOT_WITH_DEFAULT_SETTINGS:
    default:
      ConnectSequence();
      EfiBootManagerConnectAll();
      InstallAdditionalOpRom();
      SignalAllDriversConnected();
      EfiBootManagerRefreshAllBootOption();
      if (EsrtManagement != NULL) {
        EsrtManagement->SyncEsrtFmp();
      }
      break;
  }

  PERF_START(NULL, "ExitPmAuth", "BDS", 0); 
  ExitPmAuth();
  PERF_END(NULL, "ExitPmAuth", "BDS", 0);  

  InvokeHookProtocol(gBS, &gPlatAfterConsoleEndProtocolGuid);

}



