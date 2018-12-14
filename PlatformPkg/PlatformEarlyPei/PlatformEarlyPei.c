

#include <PiPei.h>
#include <Pi/PiBootMode.h>      // HobLib.h +
#include <Pi/PiHob.h>           // HobLib.h +
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Ppi/Smbus.h>
#include <Ppi/SecPerformance.h>
#include <PlatformDefinition.h>
#include <AsiaCpuPpi.h>



EFI_STATUS
InitSmbus (
  IN CONST EFI_PEI_SERVICES  **PeiServices
);

VOID SioInit();

VOID InitSVADDefTable
(  IN CONST EFI_PEI_SERVICES      **PeiServices
);

VOID InitDVADDefTable
( IN CONST EFI_PEI_SERVICES      **PeiServices
);

VOID UpdateSVADTable
( IN EFI_PEI_SERVICES      **PeiServices
);

VOID UpdateDVADTable
( IN EFI_PEI_SERVICES      **PeiServices
);

extern EFI_GUID gEfiTscFrequencyGuid;





//--------------------------------------------------------------
// has synched with SOC FPGA 0612 Code
VOID SetAcpiWakeUpSrcPcd()
{
  UINT8      WakeUpSrc;
  UINT16     En;
  UINT16     Sts;
//UINT32     PciBase;	

  WakeUpSrc = WAK_TYPE_NONE;

  Sts = IoRead16(PMIO_REG(PMIO_PM_STA));///PMIO_Rx00[15:0] Power Management Status
  En  = IoRead16(PMIO_REG(PMIO_PM_ENABLE));    ///PMIO_Rx02[15:0] Power Management Enable
  if(!(Sts & PMIO_WAKA_STS)){
    goto ProcExit;		
  }		
  if(Sts & PMIO_PWF_STS){
    WakeUpSrc = WAK_TYPE_PBOR;
    goto ProcExit;
  }
  if(Sts & En & PMIO_PBTN_STS){
    WakeUpSrc = WAK_TYPE_POWERBUTTON;
    goto ProcExit;
  }
  if(Sts & En & PMIO_RTC_STS){
    WakeUpSrc = WAK_TYPE_RTC;
    goto ProcExit;
  }
  if((Sts & PMIO_PCIEWK_STS) && !(En & PMIO_PCIEWK_DIS)){
    WakeUpSrc = WAK_TYPE_PCIE;
    goto ProcExit;    
  }	

  Sts = IoRead16(PMIO_REG(PMIO_GENERAL_PURPOSE_STA));       ///PMIO_Rx20[15:0] General Purpose Status
  En  = IoRead16(PMIO_REG(PMIO_GENERAL_PURPOSE_SCI_RESUME_ENABLE)); ///PMIO_Rx22[15:0] General Purpose SCI / RESUME Enable
  if(Sts & En & PMIO_KBPME_STS){
    WakeUpSrc = WAK_TYPE_PS2_KB;
    goto ProcExit;		
  }	
  if(Sts & En & PMIO_MSPME_STS){
    WakeUpSrc = WAK_TYPE_PS2_MS;
    goto ProcExit;		
  }
  if(Sts & En & PMIO_WUSB_STS){
    WakeUpSrc = WAK_TYPE_USB;
    goto ProcExit;		
  }
  if(Sts & En & PMIO_RI_STS){
    WakeUpSrc = WAK_TYPE_RING;
    goto ProcExit;		
  }		

  WakeUpSrc = WAK_TYPE_UNKNOWN;

ProcExit:
  DEBUG((EFI_D_INFO, "WakeUpSrc:%d\n", WakeUpSrc));	
  PcdSet8(PcdAcpiWakeupSrc, WakeUpSrc);	
  return;
}


STATIC
UINT64
CalculateTscFrequency (
  VOID
  )
{
  UINT64      StartTSC;
  UINT64      EndTSC;
  UINT32      TimerAddr;
  UINT32      Ticks;
  UINT64      TscFrequency;


  TimerAddr = PMIO_REG(PMIO_ACPI_TIMER);  ///PMIO_Rx08[31:0] ACPI Timer
  Ticks     = IoRead32 (TimerAddr) + (3579);   // Set Ticks to 1ms in the future
  StartTSC  = AsmReadTsc();                    // Get base value for the TSC
  //
  // Wait until the ACPI timer has counted 1ms.
  // Timer wrap-arounds are handled correctly by this function.
  // When the current ACPI timer value is greater than 'Ticks', the while loop will exit.
  //
  while (((Ticks - IoRead32 (TimerAddr)) & BIT23) == 0) {
    CpuPause();
  }
  EndTSC = AsmReadTsc();    // TSC value 1ms later

  TscFrequency = MultU64x32 (
                   (EndTSC - StartTSC),    // Number of TSC counts in 1ms
                   1000                    // Number of ms in a second
                   );

  return TscFrequency;
}


STATIC
UINT64
GetTscFrequency (
  VOID
  )
{
  EFI_HOB_GUID_TYPE       *GuidHob;
  VOID                    *DataInHob;
  UINT64                  TscFrequency;


  GuidHob = GetFirstGuidHob(&gEfiTscFrequencyGuid);
  if (GuidHob != NULL) {
    DataInHob = GET_GUID_HOB_DATA(GuidHob);
    TscFrequency = *(UINT64*)DataInHob;
    return TscFrequency;
  }

  TscFrequency = CalculateTscFrequency();

  BuildGuidDataHob (
    &gEfiTscFrequencyGuid,
    &TscFrequency,
    sizeof(UINT64)
    );

  return TscFrequency;
}




STATIC
UINT64
EFIAPI
TscGetTimeInNanoSecond (
  IN      UINT64                     Ticks
  )
{
  UINT64  Frequency;
  UINT64  NanoSeconds;
  UINT64  Remainder;
  INTN    Shift;

  Frequency = GetTscFrequency();

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x64Remainder (Ticks, Frequency, &Remainder), 1000000000u);

  //
  // Ensure (Remainder * 1,000,000,000) will not overflow 64-bit.
  // Since 2^29 < 1,000,000,000 = 0x3B9ACA00 < 2^30, Remainder should < 2^(64-30) = 2^34,
  // i.e. highest bit set in Remainder should <= 33.
  //
  Shift = MAX (0, HighBitSet64 (Remainder) - 33);
  Remainder = RShiftU64 (Remainder, (UINTN) Shift);
  Frequency = RShiftU64 (Frequency, (UINTN) Shift);
  NanoSeconds += DivU64x64Remainder (MultU64x32 (Remainder, 1000000000u), Frequency, NULL);

  return NanoSeconds;
}




STATIC
EFI_STATUS
EFIAPI
SecGetPerformance (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN       PEI_SEC_PERFORMANCE_PPI   *This,
  OUT      FIRMWARE_SEC_PERFORMANCE  *Performance
  )
{
  EFI_PEI_HOB_POINTERS   GuidHob;
  CAR_TOP_DATA           *CarTopData;

  GuidHob.Raw = GetFirstGuidHob(&gCarTopDataHobGuid);
  ASSERT(GuidHob.Raw != NULL);  
  CarTopData  = (CAR_TOP_DATA*)(GuidHob.Guid+1); 
  Performance->ResetEnd = TscGetTimeInNanoSecond(CarTopData->ResetTsc);

  return EFI_SUCCESS;
}


PEI_SEC_PERFORMANCE_PPI  gSecPerformancePpi = {
  SecGetPerformance
};


STATIC EFI_PEI_PPI_DESCRIPTOR  gPpiList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gPeiSecPerformancePpiGuid,
    &gSecPerformancePpi
  }
};




EFI_STATUS
EFIAPI
EarlyPeiEntry (
  IN       EFI_PEI_FILE_HANDLE      FileHandle,
  IN CONST EFI_PEI_SERVICES         **PeiServices
  )
{
  EFI_STATUS             Status;
  CAR_TOP_DATA           *CarTopData;
  UINT8                  *TopofCar;
  ACPU_MICROCODE_UPDATE_HEADER   *CpuMc;   


  CarTopData = BuildGuidHob(&gCarTopDataHobGuid, sizeof(CAR_TOP_DATA));
  ASSERT(CarTopData != NULL);
  TopofCar = (UINT8*)(UINTN)(PcdGet32(PcdTemporaryRamBase) + PcdGet32(PcdTemporaryRamSize));
  CopyMem(CarTopData, TopofCar - sizeof(CAR_TOP_DATA), sizeof(CAR_TOP_DATA));
  CpuMc = (ACPU_MICROCODE_UPDATE_HEADER*)(UINTN)CarTopData->Microcode;
  if(CpuMc!=NULL){
   PcdSet64(PcdCpuMicrocodePatchAddress, CarTopData->Microcode);
   PcdSet64(PcdCpuMicrocodePatchRegionSize, CpuMc->TotalSize);
   DEBUG((EFI_D_INFO, "[MC] (%X,%X)\n", CarTopData->Microcode, CpuMc->TotalSize));
  }else{
   DEBUG((EFI_D_INFO, "Warning: Not Find Appropriate MicroCode!!\n"));
  }


// For Port 0x61, program timer 1 as refresh timer
  IoWrite8(0x43, 0x54);
  IoWrite8(0x41, 0x12);  

// Set TSEG to 4MB
  MmioAndThenOr8(HIF_PCI_REG(SMM_APIC_DECODE_REG), (UINT8)~SMM_TSEG_SIZE_MASK, SMM_TSEG_SIZE_4M);
	
  SetAcpiWakeUpSrcPcd();
  
  Status = InitSmbus(PeiServices);
  ASSERT_EFI_ERROR(Status); 

  Status = PeiServicesInstallPpi(gPpiList);
  ASSERT_EFI_ERROR(Status);
  
  SioInit();

  return Status;  
}  


