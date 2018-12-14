
#ifndef __PLATFORM_DEFINITION_H__
#define __PLATFORM_DEFINITION_H__

#include <uefi.h>
#include <Library/IoLib.h>
#include "PlatformDefinition2.h"
#include "RtcDef.h"
//#include "SysMiscCfg.h"


//------------------------- Platform define -------------------------
#define DMI_MANUFACTURER     Shanghai Zhaoxin Semiconductor Co., Ltd.
#define DMI_NOT_SET_STR      TBD

/// Maybe should revise it with CHX001 EVB layout.
#define ONBOARD_LAN_BRIDGE_DEV_NUM     5

//ives_20170628
#if defined(HX002EH0_01)||defined(HX002EL0_05)
	#define THERMAL_IC_SUPPORT             0
#else
#define THERMAL_IC_SUPPORT             1
#endif
#define    DRAM_Vol_IC_SUPPORT          0
#define   ADM1032_SMB_SLAVE_ADDR       0x4C
#define   TPS53819A_SMB_SLAVE_ADDR     0x10


#define PCI_CACHE_LINE_SIZE            0x10

#define PEI_MEMORY_BASE                SIZE_1MB
#define PEI_MEMORY_SIZE	               SIZE_64MB
#define PEI_BU_MEMORY_SIZE             SIZE_128MB   // bios update mode

#define PCI_MMIO_TOP_ADDRESS           0xFE000000
#define PCI64_MMIO_SIZE                0x100000000  // should 512MB align to save MTRR

#define S3_PEI_MEMORY_SIZE             0xC0000

#define S3_DATA_RECORD_SIZE            0x1000

#define SMRAM_DESC_COUNT               1

#define PLATFORM_MEMORY_MAX_CAP_GB     64

typedef struct {
  UINT16  DimmSize;      // in MB, for SMBIOS
  UINT16  DimmSpeed;
  UINT32  Sn;
  UINT8   Spd320;        // Module manufacturer ID code, LSB
  UINT8   Spd321;        // Module manufacturer ID code, MSB
  CHAR8   PartNo[18+1];  // Bytes 128 ~ 145: Module Part Number
} DIMM_SPD_INFO;

typedef struct {
  UINT16  DimmCount;
  UINT16  DimmFreq;
  UINT32  DimmTotalSizeMB;

  UINT8   DramCL;
  UINT8   DramTrcd;
  UINT8   DramTrp;
  UINT8   DramTras;
  UINT32  EfuseData[2];
  
  DIMM_SPD_INFO SpdInfo[1];   // dynamic size ...
} PLAT_DIMM_INFO;  

extern EFI_GUID gEfiPlatDimmInfoGuid;

#define PLATFORM_MAX_DIMM_COUNT    4


typedef struct {
  UINT64  PhyMemSize;
  UINT64  Pci64Base;
  UINT64  Pci64Size;   
  UINT32  Tolum;           // top of low usable memory.
  UINT32  BiosLowMem;      // low physical memory controlled by BIOS(Ex:TOLUM - IGD_FB).
  UINT32  LowMemSize;
  UINT32  VgaBufSize;
  UINT32  TSegAddr;
  UINT32  TSegSize;
#ifdef ZX_TXT_SUPPORT
   UINT32  DprAddr;
   UINT32  DprSize;
#endif
  UINT32  S3DataRecord;	 
  UINT32  S3MemoryAddr;
  UINT32  CapsuleBase;
  UINT32  CapsuleSize;
  UINT8   PhyAddrBits;
  UINT32  PmmBase;
  UINT32  PmmSize;

} PLATFORM_MEMORY_INFO;  


#define EFI_PLATFORM_MEMORY_INFO_GUID \
  { 0xa47b0ce7, 0xe7f4, 0x4a26, { 0x96, 0x6a, 0x2, 0x9e, 0x5a, 0x5f, 0x95, 0xae }}

extern EFI_GUID gEfiPlatformMemInfoGuid;



// Platform NvInfo Variable
#define EFI_PLATFORM_NVINFO_GUID \
 { 0xe1cec2e6, 0x2cd0, 0x4834, { 0xa5, 0x4e, 0x61, 0xae, 0x6c, 0x2e, 0x9f, 0xa7 }}

extern EFI_GUID gEfiPlatformNvInfoGuid;

#define NVINFO_TOLUM_VAR_NAME           L"tolum"
#define NVINFO_TOLUM_VAR_ATTRIBUTE      (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE)

typedef struct {
  UINT16  Tolum;		// unit in MB
} PLAT_NV_INFO;  


#define PCI_DEV_MMBASE(Bus, Device, Function) \
    ( \
      (UINTN)PcdGet64(PcdPciExpressBaseAddress) + (UINTN) (Bus << 20) + (UINTN) (Device << 15) + (UINTN) \
        (Function << 12) \
    )

/// Standard PCI Config Space registers definitions.
#define PCI_VID_REG                          0x00
#define PCI_DID_REG                          0x02
#define PCI_CMD_REG                          0x04
#define   PCI_CMD_IO_EN                        BIT0
#define   PCI_CMD_MEM_EN                       BIT1
#define   PCI_CMD_BM_EN                        BIT2
#define   PCI_CMD_INT_DIS                      BIT10
#define PCI_PRI_STS_REG                      0x06
#define   PCI_STS_CAP_LIST                     BIT4
#define PCI_REV_ID_REG                       0x08
#define PCI_CC_PI_REG                        0x09
#define PCI_SCC_REG                          0x0A
#define   PCI_SCC_IDE                          0x01
#define   PCI_SCC_AHCI                         0x06
#define   PCI_SCC_RAID                         0x04
#define PCI_BCC_REG                          0x0B
#define   PCI_BCC_STORAGE                      0x01
#define   PCI_BCC_NETWORK                      0x02
#define   PCI_BCC_DISPLAY                      0x03
#define PCI_HDR_REG                          0x0E
#define PCI_BAR0_REG                         0x10
#define PCI_BAR1_REG                         0x14
#define PCI_BAR2_REG                         0x18
#define PCI_BAR3_REG                         0x1C
#define PCI_BAR4_REG                         0x20
#define PCI_BAR5_REG                         0x24
#define PCI_SSID_REG                         0x2C

#define PCI_PBN_REG                          0x18
#define PCI_SCBN_REG                         0x19
#define PCI_SBBN_REG                         0x1A
#define PCI_PBU32_REG                        0x28
#define PCI_PLU32_REG                        0x2C

#define PCI_CAP_POINT_REG                    0x34
#define PCI_EXROM_ADDR_REG                   0x38
#define PCI_INT_LINE_REG                     0x3C

#define PCI_CAP_ID_PM                        0x01


// Azalia


// IGD
#define IGD_SVID_MMIO_OFFSET          0x2A84
#define IGDAC_SVID_MMIO_OFFSET        0x2A98

///
#include "..\..\AsiaPkg\Asia\Porting\Include\zxibv.h"

///
#define SB_XHCI_COMMAND_REGISTER             0x04
#define   SB_XHCI_COMMAND_BME                  BIT2
#define   SB_XHCI_COMMAND_MSE                  BIT1
#define SB_XHCI_MEM_BASE                     0x10
#define XHCI_OPT_RX43                        0x43
#define   XHCI_OPT_CFG_EN                      BIT0
#define XHCI_MCU_FW_VER                      0x50
#define XHCI_OPTRX5B_REG                     0x5B
#define   XHCI_SSID_FW_PATH                    BIT0
#define XHCI_SVID_SW_REG                     0x5C
#define XHCI_OPT_CFG_ADR                     0x78     // 0x78 ~ 0x7B, Base: 0x30000
#define   XHCI_OPTCFG_MCU_BASE                 0x30000
#define XHCI_OPT_CFG_DAT                     0x7C     // 0x7C ~ 0x7F

#define XHCI_FWSWMSG0_REG                    0xB0
    #define XHCI_FWSWMSG0_INITDONE              BIT0

#define XHCI_LOAD_TIMEOUT           2000000     // 2s
#define XHCI_LOAD_INTERNAL_DELAY    10
#define XHCI_BAR_ALIGNMENT          12
#define XHCI_BAR_LENGTH             (1<<XHCI_BAR_ALIGNMENT)
#define XHCI_BAR_PCI_OFFSET         0x10
#define XHCI_USBSTS_REG             0x4
#define   XHCI_USBSTS_CNR             BIT11
#define XHCI_CAPLENGTH              0x20


//CJW_IOE_FWLOAD
//IOE MCU
//#define IOE_MCU_MMIO_BAR_ADDR                       0x108
#define IOE_MCU_AUTOFILL_EN                   		0x54
#define IOE_MCU_AUTOFILL_START_ADDR                 0x50
#define IOE_MCU_AUTOFILL_LENGTH                     0x54
#define IOE_MCU_FW_INSTRUCTION_BASEADDR_Hi          0x4C
#define IOE_MCU_FW_INSTRUCTION_BASEADDR_Lo          0x48
#define IOE_MCU_SOFTWARE_RESET                   	0x59
#define IOE_MCU_AUTOFILL_DONE						0x58
//#define IOE_MCU_FW_DATA_BASEADDR                    0x9BC
// CJW_IOE_PORTING - END

typedef struct {
  UINT8     Valid;
  UINT16    BusNo;
  UINT8     DevNo;
  UINT8     FuncNo;
  UINT16    VendorId;
  UINT16    DeviceId;
  EFI_GUID  FileName;
} PCI_OPTION_ROM_TABLE;

/// ref to : PlatformX64Pkg.fdf , these marco should be changed to use PCD?
#define IGD_VBIOS_FILE_GUID \
  { 0xabd64f12, 0x2790, 0x4774, {0xb6, 0x22, 0x2d, 0x95, 0xa2, 0x97, 0x3e, 0x21}}

#ifdef HX002EK0_03
#define ISCSI_OPROM_FILE_GUID \
  { 0xfe41daf7, 0xfc59, 0x43ce, {0xad, 0x3a, 0x13, 0xc1, 0x98, 0x29, 0xb1, 0xe8}}
#endif
#define LAN_OPROM_FILE_GUID \
  { 0x8ed0e2d0, 0xb585, 0x4d19, {0xb1, 0xb0, 0x4c, 0x95, 0xeb, 0x20, 0xcd, 0xc6}}  

#define LAN_IOEOPROM_FILE_GUID \
  { 0x82781AE4, 0x292A, 0x4BF1, {0xB0, 0x4C, 0xA2, 0xD8, 0xF9, 0x4B, 0xD2, 0x8D}} 

#define AHCI_OPROM_FILE_GUID \
  { 0x8558eeb2, 0xf5e6, 0x4f9a, {0xa4, 0x54, 0x9e, 0xd4, 0x85, 0xd9, 0xee, 0x7b}}

// {BC16E8BF-3CFD-4b0e-9B13-20D51C566AFD}
#define SD_OPROM_FILE_GUID \
  { 0xbc16e8bf, 0x3cfd, 0x4b0e, {0x9b, 0x13, 0x20, 0xd5, 0x1c, 0x56, 0x6a, 0xfd}}

// {5441A2A9-B4F1-49f1-884A-54F54CBB3D36}
#define Nvme_OPROM_FILE_GUID \
 { 0x5441a2a9, 0xb4f1, 0x49f1, { 0x88, 0x4a, 0x54, 0xf5, 0x4c, 0xbb, 0x3d, 0x36 }}

// {2EEA1265-499B-4462-A364-77E7C2C9FE9A}
#define ASPEED_2400_VBIOS_FILE_GUID \
{ 0x2eea1265, 0x499b, 0x4462, { 0xa3, 0x64, 0x77, 0xe7, 0xc2, 0xc9, 0xfe, 0x9a } }


// firmware.inf
//PcdSystemFmpCapsuleImageTypeIdGuid
// {0FCEAB65-1FAB-4deb-8C00-12088F1F89A3}
//static const GUID <<name>> = 
//{ 0xfceab65, 0x1fab, 0x4deb, { 0x8c, 0x0, 0x12, 0x8, 0x8f, 0x1f, 0x89, 0xa3 }};
//
//PcdEdkiiSystemFirmwareFileGuid
// {85A605FA-1E4A-4d09-ABC9-3CC699BF29CB}
//static const GUID <<name>> = 
//{ 0x85a605fa, 0x1e4a, 0x4d09, { 0xab, 0xc9, 0x3c, 0xc6, 0x99, 0xbf, 0x29, 0xcb}};





#define WAK_TYPE_NONE                0
#define WAK_TYPE_POWERBUTTON         1
#define WAK_TYPE_RTC                 2
#define WAK_TYPE_PS2_KB              3
#define WAK_TYPE_PS2_MS              4
#define WAK_TYPE_PCIE                5
#define WAK_TYPE_OBLAN               6
#define WAK_TYPE_USB                 7
#define WAK_TYPE_PBOR                8						// power button override
#define WAK_TYPE_RING                9
#define WAK_TYPE_POWER_LOSS          10
#define WAK_TYPE_UNKNOWN             0xFF


extern EFI_GUID gCarTopDataHobGuid;

typedef struct {
  UINT32  Microcode;       // include header
  UINT32  ResetTsc;
  UINT32  JmpSecCoreTsc;
} CAR_TOP_DATA;



#define __STR(a)        #a
#define TKN2STR(a)      __STR(a)

#define __TKN2VERSTR(a,b)  #a#b
#define TKN2VERSTR(a,b)    __TKN2VERSTR(a,b)



#define UHCI_DEVICE_ID            0x3038
#define EHCI_DEVICE_ID            0x3104
#define XHCI_DEVICE_ID            0x9204


#define REALTEK_VENDOR_ID             0x10EC  
#define VAS_VENDOR_ID             	  0x1D17 
#define VAS_AHCI_DEVICE_ID            0x9083
#define VAS_IDE_DEVICE_ID             0x9002

#define VAS_AHCI_PCIID               ((VAS_AHCI_DEVICE_ID << 16) | VAS_VENDOR_ID)
#define VAS_IDE_PCIID                ((VAS_IDE_DEVICE_ID << 16)  | VAS_VENDOR_ID)




/*

EA0:

  (0,3,0) PCIE
  (0,3,2) PCIE
  (0,3,3) PCIE
  (0,4,0) PCIE
  (0,5,0) PCIE    (LAN)
  (0,5,1) PCIE

  (0,F,0) SATA
  
  (0,10,0) UHCI
  (0,10,1) UHCI
  (0,10,7) EHCI

  (0,11,0) ISA

  (0,12,0) XHCI

*/




#endif


