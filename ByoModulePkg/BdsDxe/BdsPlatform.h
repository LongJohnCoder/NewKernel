
#ifndef _BDS_PLATFORM_H
#define _BDS_PLATFORM_H

#include <PiDxe.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/LoadFile.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/PciIo.h>
#include <Protocol/CpuIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/OEMBadging.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/GenericMemoryTest.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/FirmwareVolume2.h>
#include <Guid/CapsuleVendor.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/GlobalVariable.h>
#include <Guid/EventGroup.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/ByoUefiBootManagerLib.h>
#include "PlatformBootManagerLib.h"
#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <IndustryStandard/Pci30.h>
#include <Protocol/BdsBootManagerProtocol.h>



///
/// ConnectType
///
#define CONSOLE_OUT 0x00000001
#define STD_ERROR   0x00000002
#define CONSOLE_IN  0x00000004
#define CONSOLE_ALL (CONSOLE_OUT | CONSOLE_IN | STD_ERROR)


//
// Below is the boot option device path
//
typedef struct {
  BBS_BBS_DEVICE_PATH       LegacyHD;
  EFI_DEVICE_PATH_PROTOCOL  End;
} LEGACY_HD_DEVICE_PATH;

#define CLASS_HID           3
#define SUBCLASS_BOOT       1
#define PROTOCOL_KEYBOARD   1



typedef
VOID
(*PROCESS_VARIABLE) (
  VOID  **Variable,
  UINTN *VariableSize
  );
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
;

/**
  Perform the memory test base on the memory test intensive level,
  and update the memory resource.

  @param  Level         The memory test intensive level.

  @retval EFI_STATUS    Success test all the system memory and update
                        the memory resource

**/
EFI_STATUS
MemoryTest (
  IN EXTENDMEM_COVERAGE_LEVEL Level
  );

EFI_STATUS
PlatformBdsShowProgress (
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleForeground,
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleBackground,
  CHAR16                        *Title,
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL ProgressColor,
  UINTN                         Progress,
  UINTN                         PreviousValue
  )
/*++

Routine Description:
  
  Show progress bar with title above it. It only works in UGA mode.

Arguments:
  
  TitleForeground  -  Foreground color for Title.
  TitleBackground  -  Background color for Title.
  Title            -  Title above progress bar.
  ProgressColor    -  Progress bar color.
  Progress         -  Progress (0-100)
  PreviousValue    -  Previous value of progress.
  
Returns: 
  
  EFI_STATUS  -  Success update the progress bar.

--*/
;

VOID
ConnectSequence (
  VOID
  )
/*++

Routine Description:

  Connect with predeined platform connect sequence, 
  the OEM/IBV can customize with their own connect sequence.
  
Arguments:

  None.
 
Returns:

  None.
  
--*/
;

/**
  This function locks platform flash that is not allowed to be updated during normal boot path.
  The flash layout is platform specific.

  **/
BOOLEAN
EFIAPI
CompareBootOption (
  CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Left,
  CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Right
  );

STATIC
VOID 
PrintBootPrompt (
  VOID
  );

EFI_STATUS
EFIAPI
ProcessCapsules (
  EFI_BOOT_MODE BootMode
  );

UINT8
GetDisplayBootMode (
  VOID
  );
  
#endif
