
#include "PlatHost.h"
#include <IndustryStandard/Smbios.h>


EFI_STATUS UpdateTolumVar(UINTN BitsOfAlignment, UINT64 AddrLen);

VOID UpdateCpuMcVer(UINT64 *CpuMcCVer);

EFI_STATUS
PlatPreprocessPciCallBack (
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL                 *Io,
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS     PciAddress,
  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE    Phase    
  );


UINT16 
GetPlatSataHostIndex(
  EFI_HANDLE          Handle
  );

UINT16
GetPlatSataPortIndex (
  IN  EFI_HANDLE          Handle
  );

VOID
PlatUpdateBootOption (
  EFI_BOOT_MANAGER_LOAD_OPTION  **BootOptions,
  UINTN                         *BootOptionCount
  );

EFI_DEVICE_PATH_PROTOCOL*
GetPlatUcrDp (
    UINTN   *DpSize,
    UINT16  *IoBase
  );

extern PLAT_HOST_INFO_PROTOCOL gPlatHostInfoProtocol;
extern ADDITIONAL_ROM_TABLE    gLegacyOpRomTable[];
extern UINTN                   gLegacyOpRomTableSize;



#define gPciRootBridge \
  { \
    ACPI_DEVICE_PATH, ACPI_DP, (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), (UINT8) \
      ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8), EISA_PNP_ID (0x0A03), 0 \
  }

#define gEndEntire \
  { \
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, END_DEVICE_PATH_LENGTH, 0 \
  } 

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           Pci0Device;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH;

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           Pci0Device;
  PCI_DEVICE_PATH           Pci1Device;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2;

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           Pci0Device;
  PCI_DEVICE_PATH           Pci1Device;
  PCI_DEVICE_PATH           Pci2Device;
  PCI_DEVICE_PATH           Pci3Device;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH4;

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           Pci0Device;
  PCI_DEVICE_PATH           Pci1Device;
  PCI_DEVICE_PATH           Pci2Device;
  PCI_DEVICE_PATH           Pci3Device;
  PCI_DEVICE_PATH           Pci4Device;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH5;



#define CLASS_HID           3
#define SUBCLASS_BOOT       1
#define PROTOCOL_KEYBOARD   1

typedef struct {
  USB_CLASS_DEVICE_PATH           UsbClass;
  EFI_DEVICE_PATH_PROTOCOL        End;
} USB_CLASS_FORMAT_DEVICE_PATH;

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           IsaBridge;
  ACPI_HID_DEVICE_PATH      Keyboard;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_KEYBOARD_DEVICE_PATH;




PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gIgdDpData = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x1
  },
  gEndEntire
};



PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformOnboardNicDevice = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    5
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};


PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformPcie0Device = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    3
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};

PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformPcie1Device = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x1,
    3
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};

PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformPcie2Device = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x2,
    3
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};

PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformPcie3Device = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x3,
    3
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};

PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformPcie4Device = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    4
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};

PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformPcie5Device = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x1,
    4
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};


PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformPcie7Device = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x1,
    5
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};


//
// Onboard SATA controller device path
//
PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformObSataDevice = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0xF
  },
  gEndEntire
};


//Add for IOE

PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH5  gPlatformIoeSataAhciDevice = {
    gPciRootBridge,
    {
      HARDWARE_DEVICE_PATH,
      HW_PCI_DP,
      (UINT8) (sizeof (PCI_DEVICE_PATH)),
      (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
      0x0,
      0x3
    },
    {
      HARDWARE_DEVICE_PATH,
      HW_PCI_DP,
      (UINT8) (sizeof (PCI_DEVICE_PATH)),
      (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
      0x0,
      0x0
    },
    {
      HARDWARE_DEVICE_PATH,
      HW_PCI_DP,
      (UINT8) (sizeof (PCI_DEVICE_PATH)),
      (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
      0x0,
      0x8
    },
    {
      HARDWARE_DEVICE_PATH,
      HW_PCI_DP,
      (UINT8) (sizeof (PCI_DEVICE_PATH)),
      (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
      0x0,
      0x0
    },
    {
      HARDWARE_DEVICE_PATH,
      HW_PCI_DP, 
      (UINT8) (sizeof (PCI_DEVICE_PATH)),
      (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
      0x0,
      0xF
    },
    gEndEntire
};

//
// IOE Temp GNIC controller device path
//
PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH5 gPlatformIoeGnicDevice = {
    gPciRootBridge,
    {
      HARDWARE_DEVICE_PATH,
      HW_PCI_DP,
      (UINT8) (sizeof (PCI_DEVICE_PATH)),
      (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
      0x0,
      0x3
    },
    {
      HARDWARE_DEVICE_PATH,
      HW_PCI_DP,
      (UINT8) (sizeof (PCI_DEVICE_PATH)),
      (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
      0x0,
      0x0
    },
    {
      HARDWARE_DEVICE_PATH,
      HW_PCI_DP,
      (UINT8) (sizeof (PCI_DEVICE_PATH)),
      (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
      0x0,
      0x8
    },
    {
      HARDWARE_DEVICE_PATH,
      HW_PCI_DP,
      (UINT8) (sizeof (PCI_DEVICE_PATH)),
      (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
      0x0,
      0x0
    },
    {
      HARDWARE_DEVICE_PATH,
      HW_PCI_DP, 
      (UINT8) (sizeof (PCI_DEVICE_PATH)),
      (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
      0x0,
      0xD
    },    
    gEndEntire
};


PLATFORM_ISA_SERIAL_DEVICE_PATH   gSerialDevicePath = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0,
    0x11
  },
  {
    ACPI_DEVICE_PATH,
    ACPI_DP,
    (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)),
    (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8),
    EISA_PNP_ID(0x0501),
    0
  },
  {
    MESSAGING_DEVICE_PATH,
    MSG_UART_DP,
    (UINT8) (sizeof (UART_DEVICE_PATH)),
    (UINT8) ((sizeof (UART_DEVICE_PATH)) >> 8),
    0,
    115200,
    8,
    1,
    1
  },
  {
    MESSAGING_DEVICE_PATH,
    MSG_VENDOR_DP,
    (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
    (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8),
    DEVICE_PATH_MESSAGING_PC_ANSI
  },
  gEndEntire
};



PLATFORM_KEYBOARD_DEVICE_PATH  gPs2KbDevicePath = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0,
    0x11
  },
  {
    ACPI_DEVICE_PATH,
    ACPI_DP,
    (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)),
    (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8),
    EISA_PNP_ID(0x0303),
    0
  },
  gEndEntire
};


USB_CLASS_FORMAT_DEVICE_PATH gUsbClassKeyboardDevicePath = {
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_USB_CLASS_DP,
      (UINT8) (sizeof (USB_CLASS_DEVICE_PATH)),
      (UINT8) ((sizeof (USB_CLASS_DEVICE_PATH)) >> 8)
    },
    0xffff,           // VendorId 
    0xffff,           // ProductId 
    CLASS_HID,        // DeviceClass 
    SUBCLASS_BOOT,    // DeviceSubClass
    PROTOCOL_KEYBOARD // DeviceProtocol
  },

  { 
    END_DEVICE_PATH_TYPE, 
    END_ENTIRE_DEVICE_PATH_SUBTYPE, 
    END_DEVICE_PATH_LENGTH, 
    0
  }
};


STATIC EFI_DEVICE_PATH_PROTOCOL *gConInDpList[] = {
  (EFI_DEVICE_PATH_PROTOCOL*)&gPs2KbDevicePath,
  (EFI_DEVICE_PATH_PROTOCOL*)&gUsbClassKeyboardDevicePath
};


STATIC PLATFORM_HOST_INFO_SATA_CTX gPlatHostSata0Ctx = {sizeof(gPlatformObSataDevice),      0, 2};
STATIC PLATFORM_HOST_INFO_SATA_CTX gPlatHostSata1Ctx = {sizeof(gPlatformIoeSataAhciDevice), 1, 4};


/*
(0,3,0) M.2       J53                   (0)
(0,3,2) PCIE      J55      1x           (2)
(0,3,3) PCI       J61                   (3)
(0,4,0) PCIE      J52      16x          (4)
(0,5,0) oblan
(0,5,1) PCIE      J56      1x           (7)
*/
STATIC PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformPcie0SlotDp = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0,
    3
  },
  gEndEntire
};

STATIC PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformPcie2SlotDp = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    2,
    3
  },
  gEndEntire
};

STATIC PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformPcie3SlotDp = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    3,
    3
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },    
  gEndEntire
};

STATIC PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformPcie4SlotDp = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0,
    4
  },
  gEndEntire
};

STATIC PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformPcie7SlotDp = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    1,
    5
  },
  gEndEntire
};

STATIC PLATFORM_HOST_INFO_PCIE_CTX gPlatHostPcie0Ctx = {"J53", SlotTypePciExpress,    SlotDataBusWidthUnknown, SlotUsageAvailable, SlotLengthOther, 0, (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformPcie0SlotDp};
STATIC PLATFORM_HOST_INFO_PCIE_CTX gPlatHostPcie2Ctx = {"J55", SlotTypePciExpressX1,  SlotDataBusWidth1X,      SlotUsageAvailable, SlotLengthShort, 2, (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformPcie2SlotDp};
STATIC PLATFORM_HOST_INFO_PCIE_CTX gPlatHostPcie3Ctx = {"J61", SlotTypePci,           SlotDataBusWidth32Bit,   SlotUsageAvailable, SlotLengthLong,  3, (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformPcie3SlotDp};
STATIC PLATFORM_HOST_INFO_PCIE_CTX gPlatHostPcie4Ctx = {"J52", SlotTypePciExpressX16, SlotDataBusWidth16X,     SlotUsageAvailable, SlotLengthLong,  4, (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformPcie4SlotDp};
STATIC PLATFORM_HOST_INFO_PCIE_CTX gPlatHostPcie7Ctx = {"J56", SlotTypePciExpressX1,  SlotDataBusWidth1X,      SlotUsageAvailable, SlotLengthShort, 7, (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformPcie7SlotDp};

PLATFORM_HOST_INFO gPlatHostInfo[] = {
  {PLATFORM_HOST_SATA, (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformObSataDevice,      &gPlatHostSata0Ctx},

#ifdef IOE_EXIST  
  {PLATFORM_HOST_SATA, (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformIoeSataAhciDevice, &gPlatHostSata1Ctx},  
#endif
  
  {PLATFORM_HOST_PCIE, (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformPcie0Device, &gPlatHostPcie0Ctx},
  {PLATFORM_HOST_PCIE, (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformPcie1Device, NULL},
  {PLATFORM_HOST_PCIE, (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformPcie2Device, &gPlatHostPcie2Ctx},
  {PLATFORM_HOST_PCIE, (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformPcie3Device, &gPlatHostPcie3Ctx},
  {PLATFORM_HOST_PCIE, (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformPcie4Device, &gPlatHostPcie4Ctx},
  {PLATFORM_HOST_PCIE, (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformPcie5Device, NULL}, 
  {PLATFORM_HOST_PCIE, (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformPcie7Device, &gPlatHostPcie7Ctx},   

  {PLATFORM_HOST_IGD,  (EFI_DEVICE_PATH_PROTOCOL*)&gIgdDpData, NULL},
  
  {PLATFORM_HOST_LAN,  (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformOnboardNicDevice, NULL},
#ifdef IOE_EXIST 
  {PLATFORM_HOST_LAN,  (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformIoeGnicDevice,    NULL}, 
#endif
};  

#define PLAT_HOST_INFO_COUNT   (sizeof(gPlatHostInfo)/sizeof(gPlatHostInfo[0]))




PLAT_HOST_INFO_PROTOCOL gPlatHostInfoProtocol = {
  gPlatHostInfo,
  PLAT_HOST_INFO_COUNT,
  0,
  (EFI_DEVICE_PATH_PROTOCOL*)&gIgdDpData,
  sizeof(gIgdDpData),
  GetPlatSataHostIndex,
  GetPlatSataPortIndex,
  NULL,
  0,
  GetPlatUcrDp,
  gConInDpList,
  sizeof(gConInDpList)/sizeof(gConInDpList[0]),
  PlatUpdateBootOption,
  UpdateTolumVar,
  PlatPreprocessPciCallBack,
  0,
};

EFI_STATUS
PlatHostInfoInstall (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS               Status;
  UINTN                    Index;
  PLATFORM_HOST_INFO       *HostInfo;
  PLAT_HOST_INFO_PROTOCOL  *PlatHostInfo;


  PlatHostInfo = &gPlatHostInfoProtocol;
  HostInfo     = gPlatHostInfo;

  PlatHostInfo->OptionRomTable = gLegacyOpRomTable;
  PlatHostInfo->OptionRomTableSize = gLegacyOpRomTableSize;

  PlatHostInfo->SataHostCount = 0;
  for(Index=0;Index<PLAT_HOST_INFO_COUNT;Index++){
    if(HostInfo[Index].HostType == PLATFORM_HOST_SATA){
      PlatHostInfo->SataHostCount++;
    }
  }

  UpdateCpuMcVer(&PlatHostInfo->CpuMcVer);
  
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gPlatHostInfoProtocolGuid,
                  PlatHostInfo,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);

  return Status;
}


