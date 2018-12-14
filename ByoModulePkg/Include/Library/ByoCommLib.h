#ifndef __BYO_COMMON_LIB_H__
#define __BYO_COMMON_LIB_H__

#include <Uefi.h>

typedef union {
  struct {
    UINT32  Lower32;
    UINT32  Upper32;
  } Uint32;
  UINT64    Uint64;
} DATA_64;

typedef union {
  struct {
    UINT8  Data[4];
  } Uint8;
  UINT32  Uint32;
} DATA_32_4;

#define PCI_CLASS_MASS_STORAGE_NVM                0x08
#define PCI_IF_NVMHCI                             0x02


UINT8 PcieRead8(UINTN PcieAddr);

UINT8 CaculateWeekDay(UINT16 y, UINT8 m, UINT8 d);

BOOLEAN IsLeapYear(UINT16 Year);

VOID DumpMem32(VOID *Base, UINTN Size);

VOID DumpMem8(VOID *Base, UINTN Size);

VOID DumpCmos();

VOID DumpPci(UINT8 Bus, UINT8 Dev, UINT8 Func);

VOID DumpIo4(UINTN Base, UINT16 Size);

VOID DumpAllPciIntLinePin();

VOID DumpAllPci();

VOID DumpHob();



EFI_STATUS KbcWaitInputBufferFree();
EFI_STATUS KbcWaitOutputBufferFull();
EFI_STATUS KbcWaitOutputBufferFree();
EFI_STATUS KbcSendCmd(UINT8 Cmd);
BOOLEAN CheckKbcPresent(VOID);
VOID IsPs2DevicePresent(EFI_BOOT_SERVICES *BS, BOOLEAN *Ps2Kb, BOOLEAN *Ps2Ms);


EFI_STATUS
KbcCmdReadData (
  IN  UINT8  Cmd,
  OUT UINT8  *Data  OPTIONAL
  );


EFI_STATUS 
EFIAPI
LibCalcCrc32 (
  IN  VOID    *Data,
  IN  UINTN   DataSize,
  OUT UINT32  *CrcOut
  );

BOOLEAN LibVerifyDataCrc32(VOID *Data, UINTN DataSize, UINTN CrcOffset, UINT32 *CalcCrc32 OPTIONAL);
  
EFI_STATUS 
AzaliaLoadVerbTable (
  IN UINTN           HostPcieAddr,
  IN VOID            *VerbTable, 
  IN UINTN           VerbTableSize
  );
  
VOID *LibGetGOP(EFI_BOOT_SERVICES *pBS);

EFI_STATUS BltSaveAndRetore(VOID *BootServices, BOOLEAN Save);

VOID 
AcpiTableUpdateChksum (
  IN VOID *AcpiTable
  );

UINTN
MyGetDevicePathSize (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );
  
VOID
ShowDevicePathDxe (
  IN EFI_BOOT_SERVICES         *BS,
  IN EFI_DEVICE_PATH_PROTOCOL  *ptDevPath
  );

VOID
SwapWordArray (
  IN  UINT8   *Data,
  IN  UINTN   DataSize
  );

CHAR8 *TrimStr8(CHAR8 *Str);

CHAR16 *TrimStr16(CHAR16 *Str);

EFI_STATUS
GetOnboardLanMacAddress (
  IN  EFI_BOOT_SERVICES         *pBS,
  IN  VOID                      *Dp,
  OUT UINT8                     MacAddr[6]
  );  
  
BOOLEAN IsSataDp(VOID  *DevicePath);

BOOLEAN 
IsAhciHddDp(
  IN  EFI_BOOT_SERVICES       *pBS,
  IN  VOID                    *DevicePath,
  OUT VOID                    **pDiskInfo  OPTIONAL
  );

VOID*
AllocateAcpiNvsZeroMemoryBelow4G (
  IN EFI_BOOT_SERVICES  *BS,
  IN UINTN              Size
  );

VOID*
AllocateReservedZeroMemoryBelow4G (
  IN EFI_BOOT_SERVICES  *BS,
  IN UINTN              Size
  );

VOID*
AllocateReservedZeroMemoryBelow1M (
  IN EFI_BOOT_SERVICES  *BS,
  IN UINTN              Size
  );

VOID*
AllocateBsZeroMemoryBelow4G (
  IN EFI_BOOT_SERVICES  *BS,
  IN UINTN              Size
  );

EFI_STATUS InvokeHookProtocol(EFI_BOOT_SERVICES *BS, EFI_GUID *Protocol);  

typedef
VOID
(EFIAPI *EFI_MY_HOOK_PROTOCOL) (
  VOID
);

VOID
SignalProtocolEvent (
  EFI_BOOT_SERVICES *BS,
  EFI_GUID          *ProtocolGuid,
  BOOLEAN           NeedUnInstall
  );  

#endif
