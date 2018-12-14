
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <AsiaCpuProtocol.h>


/*
STATIC
VOID
AsiaCpuCallBack (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_ASIA_CPU_PROTOCOL *AsiaCpu = NULL;  
  UINT64                *CpuMcCVer;
  EFI_STATUS            Status;
  
  
  Status = gBS->LocateProtocol(&gAsiaCpuProtocolGuid, NULL, (VOID**)&AsiaCpu);
  if (EFI_ERROR(Status)) {
    return;
  }
  gBS->CloseEvent(Event);
  
  CpuMcCVer = (UINT64*)Context;
  
  *CpuMcCVer = AsiaCpu->GetUpdateRevision();
}



VOID UpdateCpuMcVer(UINT64 *CpuMcCVer)
{
  VOID  *Registration;
  
  EfiCreateProtocolNotifyEvent (
    &gAsiaCpuProtocolGuid,
    TPL_CALLBACK,
    AsiaCpuCallBack,
    CpuMcCVer,
    &Registration
    );    
}
*/


VOID UpdateCpuMcVer(UINT64 *CpuMcVer)
{
  AsmWriteMsr64 (0x8B, 0);
  AsmCpuid(1, NULL, NULL, NULL, NULL);
  *CpuMcVer = (UINT32)RShiftU64(AsmReadMsr64(0x8B), 32);
  DEBUG((EFI_D_INFO, "CpuMcVer:%X\n", (UINT32)*CpuMcVer));
}





