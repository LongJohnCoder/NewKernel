
#ifndef __PLAT_HOST_INFO_PROTOCOL_H__
#define __PLAT_HOST_INFO_PROTOCOL_H__

#include <Uefi.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>
#include <UefiBootManagerLoadOption.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>


typedef struct _PLAT_HOST_INFO_PROTOCOL  PLAT_HOST_INFO_PROTOCOL;

typedef
BOOLEAN
(EFIAPI *LEGACY_OPROM_RUN_CHECK)(
  IN EFI_HANDLE               Handle,
  IN EFI_PCI_IO_PROTOCOL      *PciIo,
  IN PLAT_HOST_INFO_PROTOCOL  *HostInfo
  );

typedef struct {
  BOOLEAN                 Enable;
  UINT16                  VendorId;
  UINT16                  DeviceId;
  EFI_GUID                RomImageGuid;
  LEGACY_OPROM_RUN_CHECK  RunCheck;
} ADDITIONAL_ROM_TABLE;



typedef enum {
  PLATFORM_HOST_SATA,
  PLATFORM_HOST_LAN,
  PLATFORM_HOST_PCIE,
  PLATFORM_HOST_SDIO,
  PLATFORM_HOST_IGD,
  PLATFORM_HOST_MAX
} PLATFORM_HOST_TYPE;


typedef struct {
  UINTN                     DpSize;
  UINT16                    HostIndex;  
  UINT16                    PortCount;
} PLATFORM_HOST_INFO_SATA_CTX;

typedef struct {
  CHAR8                     *SlotName;
  UINT8                     SlotType;
  UINT8                     SlotbusWidth;
  UINT8                     SlotUsage;
  UINT8                     SlotLen;
  UINT16                    SlotId;
  EFI_DEVICE_PATH_PROTOCOL  *SlotDp;
} PLATFORM_HOST_INFO_PCIE_CTX;


typedef struct {
  PLATFORM_HOST_TYPE        HostType;
  EFI_DEVICE_PATH_PROTOCOL  *Dp;
  VOID                      *HostCtx;
} PLATFORM_HOST_INFO;


// return 0xFFFF - ERROR
// return 0x8000 - only one host
// return others - OK
typedef
UINT16
(EFIAPI *GET_PLAT_SATA_HOST_INDEX)(
  EFI_HANDLE          Handle
  );

typedef
UINT16
(EFIAPI *GET_PLAT_SATA_PORT_INDEX)(
  EFI_HANDLE          Handle
  );


typedef
VOID
(EFIAPI *UPDATE_BOOT_OPTION)(
  EFI_BOOT_MANAGER_LOAD_OPTION  **BootOptions,
  UINTN                         *BootOptionCount
  );

typedef
EFI_STATUS
(EFIAPI *UPDATE_TOLUM_VAR)(
  UINTN     BitsOfAlignment,
  UINT64    AddrLen
  );


typedef
EFI_STATUS
(EFIAPI *PREPROCESS_PCI_CALLBACK)(
    EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL                 *Io,
    EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS     PciAddress,
    EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE    Phase    
  );

typedef
EFI_DEVICE_PATH_PROTOCOL*
(EFIAPI *GET_PLAT_UCR_DP)(
  UINTN   *DpSize,  OPTIONAL
  UINT16  *IoBase   OPTIONAL
  );



struct _PLAT_HOST_INFO_PROTOCOL {
  PLATFORM_HOST_INFO        *HostList;
  UINTN                     HostCount;
  UINTN                     SataHostCount;
  EFI_DEVICE_PATH_PROTOCOL  *IgdDp;
  UINTN                     IgdDpSize;
  GET_PLAT_SATA_HOST_INDEX  GetSataHostIndex;
  GET_PLAT_SATA_PORT_INDEX  GetSataPortIndex;
  ADDITIONAL_ROM_TABLE      *OptionRomTable;
  UINTN                     OptionRomTableSize;
  GET_PLAT_UCR_DP           GetPlatUcrDp;
  EFI_DEVICE_PATH_PROTOCOL  **ConInDp;
  UINTN                     ConInDpCount;
  UPDATE_BOOT_OPTION        UpdateBootOption;
  UPDATE_TOLUM_VAR          UpdateTolumVar;
  PREPROCESS_PCI_CALLBACK   PreprocessPci;
  UINT64                    CpuMcVer;
};


extern EFI_GUID gPlatHostInfoProtocolGuid;

#endif


