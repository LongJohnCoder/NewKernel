
#ifndef __PLAT_COMM_LIB_H__
#define __PLAT_COMM_LIB_H__

#include <Uefi.h>

extern EFI_GUID gEfiPlatformMemInfoGuid;

VOID ZX_DumpPciDevSetting();


#ifdef IOE_EXIST
EFI_STATUS
LoadIoeXhciFw(
  UINT8 BusOfEptrfc,
  VOID *FwAddr_alloc
);

EFI_STATUS LoadIoeMcuFw(UINT8 BusOfEptrfc, VOID *FwAddr_alloc,UINT16 AutoFillAddr,UINT16 AutoFillLen);

#endif
EFI_STATUS LoadXhciFw(UINT8 BusNum, UINT8 DevNum, UINT8 FunNum, UINT32  FwAddr_Lo, UINT32  FwAddr_Hi);
EFI_STATUS LoadPeMcuFw(VOID *PeMcuFw,VOID *PeMcuData, UINT8 IsDoEQ, UINT8 TP_Value,UINT8 TargetBus);

VOID *GetPlatformMemInfo(VOID);
VOID *GetPlatformDimmInfo(VOID);

UINT8 WaitIdeDeviceReady(UINTN Bus, UINTN Dev, UINTN Func, UINTN DetectTimeOutInms);

extern VOID  *gOemVerbTableData;
extern UINTN  gOemVerbTableSize;

UINT8 CmosRead(UINT8 Address);
VOID  CmosWrite(UINT8 Address, UINT8 Data);  
UINT8 CheckAndConvertBcd8ToDecimal8(UINT8 Value);
EFI_STATUS RtcWaitToUpdate();

VOID *GetAcpiRam(VOID);
VOID *GetAcpiTableScat(VOID);
VOID GetS3Cr3Stack(UINT32 *S3Cr3, UINT32 *S3StackBase, UINT32 *S3StackSize);
VOID *GetSetupDataHobData(VOID);
#ifdef ZX_TXT_SUPPORT
VOID *GetAsiaVariableHobData(VOID);
#endif
VOID PlatRecordS3DebugData(CHAR8 *Name, UINT32 Data32);
VOID *GetS3RecordTable();
VOID *GetS3MtrrTable();
VOID *GetCarTopData();
BOOLEAN IsSmrrTypeSetWB();

VOID SetCmosVRT(BOOLEAN OnOff);

VOID SystemSoftOff();




#endif


