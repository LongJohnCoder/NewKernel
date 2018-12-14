
#ifndef __PLAT_HOST_H__
#define __PLAT_HOST_H__

#include <Protocol/PlatHostInfoProtocol.h>
#include <Protocol/DiskInfo.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Library/ByoCommLib.h>
#include <IndustryStandard/Pci22.h>
#include <PlatformDefinition.h>
#include <SetupVariable.h>


extern CONST SETUP_DATA        *gSetupHob;


typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           IsaBridge;
  ACPI_HID_DEVICE_PATH      IsaSerial;
  UART_DEVICE_PATH          Uart;
  VENDOR_DEVICE_PATH        TerminalType;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ISA_SERIAL_DEVICE_PATH;


#endif