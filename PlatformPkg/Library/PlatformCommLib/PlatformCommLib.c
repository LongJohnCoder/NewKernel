
#include <Uefi.h>
#include <Pi/PiBootMode.h>      // HobLib.h +
#include <Pi/PiHob.h>           // HobLib.h +
#include <Uefi/UefiSpec.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Atapi.h>
#include <IndustryStandard/Smbios.h>
#include <Uefi/UefiAcpiDataTable.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/PciLib.h>
#include <Library/PlatformCommLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/PciIo.h>
#include <Protocol/DevicePathToText.h>
#include <RtcDef.h>
#include <PlatformDefinition.h>
#include <Protocol/ScsiIo.h>
#include <PlatS3Record.h>
#include <SetupVariable.h>
#ifdef ZX_TXT_SUPPORT
#include <AsiaVariable.h>
#endif

extern EFI_GUID gPlatformSetupVariableGuid;
extern EFI_GUID gEfiConsoleOutDeviceGuid;


UINT8 CmosRead(UINT8 Address)
{
  IoWrite8(PCAT_RTC_ADDRESS_REGISTER, (UINT8)(Address|(UINT8)(IoRead8(PCAT_RTC_ADDRESS_REGISTER) & 0x80)));
  return IoRead8(PCAT_RTC_DATA_REGISTER);
}

VOID CmosWrite_Original(UINT8 Address, UINT8 Data)
{
  IoWrite8(PCAT_RTC_ADDRESS_REGISTER, (UINT8)(Address|(UINT8)(IoRead8(PCAT_RTC_ADDRESS_REGISTER) & 0x80)));
  IoWrite8(PCAT_RTC_DATA_REGISTER, Data);
}
VOID CmosWrite(UINT8 Address, UINT8 Data)
{
	RTC_REGISTER_A	RegisterA;
	RTC_REGISTER_B	RegisterB;


		if(PciRead8(PCI_LIB_ADDRESS(0, 0, 4, 0xF6))==0){	
		//RTC patch for CHX002 A0
		//CMOS Rx0xA-OxD  
		if((Address>=0xA)&&(Address<=0xD)){
		//	DEBUG((EFI_D_ERROR, "DLA:[CmosWrite] [Address:0x%x]  = 0x%x\n",Address,Data));
		  RegisterB.Data	  = CmosRead (RTC_ADDRESS_REGISTER_B);

		  if(RegisterB.Bits.Set==0){

		  RegisterA.Data	= CmosRead (RTC_ADDRESS_REGISTER_A);
		
			//wait for uip=0
		  	while (RegisterA.Bits.Uip == 1 ) {
				MicroSecondDelay (10);
				RegisterA.Data = CmosRead (RTC_ADDRESS_REGISTER_A);
		  	}   
			}
			}
			}
		CmosWrite_Original(Address,Data);		

}


UINT8 CheckAndConvertBcd8ToDecimal8(UINT8 Value)
{
  if ((Value < 0xa0) && ((Value & 0xf) < 0xa)) {
    return BcdToDecimal8(Value);
  }
  return 0xff;
}


EFI_STATUS RtcWaitToUpdate()
{
  RTC_REGISTER_A  RegisterA;
  RTC_REGISTER_D  RegisterD;
	UINTN           Timeout;

  Timeout = PcdGet32(PcdRealTimeClockUpdateTimeout);

  RegisterD.Data = CmosRead (RTC_ADDRESS_REGISTER_D);
  if (RegisterD.Bits.Vrt == 0) {
    return EFI_DEVICE_ERROR;
  }

  Timeout         = (Timeout / 10) + 1;
  RegisterA.Data  = CmosRead (RTC_ADDRESS_REGISTER_A);
  while (RegisterA.Bits.Uip == 1 && Timeout > 0) {
    MicroSecondDelay (10);
    RegisterA.Data = CmosRead (RTC_ADDRESS_REGISTER_A);
    Timeout--;
  }

  RegisterD.Data = CmosRead (RTC_ADDRESS_REGISTER_D);
  if (Timeout == 0 || RegisterD.Bits.Vrt == 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}



// 0:POWER_OFF
// 1:POWER_PREVIOUS
// 2:POWER_ON 
//
// RTC Register Rx0D[7]
//   (0) Power Off  : always 1
//   (2) Power On   : always 0
//   (1) Last State : 0 in POST, 1 in S3/S4/S5.

VOID SetCmosVRT(BOOLEAN OnOff)
{
  UINT32          Data32;
  RTC_REGISTER_D  RegD;
  
  Data32 = MmioRead32(LPC_PCI_REG(D17F0_SOUTH_MODULE_MISC_CTL_1));
  if(Data32 & D17F0_ENRX0DWP){
    MmioAnd32(LPC_PCI_REG(D17F0_SOUTH_MODULE_MISC_CTL_1), (UINT32)~D17F0_ENRX0DWP);
  }
  
  RegD.Data = CmosRead(RTC_ADDRESS_REGISTER_D);
  RegD.Bits.Vrt = OnOff?1:0;
  CmosWrite(RTC_ADDRESS_REGISTER_D, RegD.Data);
  
  if(Data32 & D17F0_ENRX0DWP){
    MmioOr32(LPC_PCI_REG(D17F0_SOUTH_MODULE_MISC_CTL_1), D17F0_ENRX0DWP);
  }  
}


VOID SystemSoftOff()
{
  UINT16   Data16;
  
  Data16  = IoRead16(PMIO_REG(PMIO_PM_CTL));///PMIO_Rx04[15:0] Power Management Control
  Data16 &= ~(PMIO_SLP_EN | PMIO_SLP_TYP);
  Data16 |= PMIO_PM1_CNT_S5;
  IoWrite16(PMIO_REG(PMIO_PM_CTL), Data16);///PMIO_Rx04[15:0] Power Management Control
  Data16 |= PMIO_SLP_EN;
  IoWrite16(PMIO_REG(PMIO_PM_CTL), Data16); ///PMIO_Rx04[15:0] Power Management Control
  
  CpuDeadLoop(); 
}








VOID *GetPlatformMemInfo(VOID)
{  
  VOID                  *MemInfo;
  EFI_PEI_HOB_POINTERS  GuidHob;    
  
  GuidHob.Raw = GetFirstGuidHob(&gEfiPlatformMemInfoGuid);
  ASSERT(GuidHob.Raw != NULL);  
  MemInfo = (VOID*)(GuidHob.Guid+1);
  
  return MemInfo;
}


VOID *GetPlatformDimmInfo(VOID)
{  
  VOID                  *DimmInfo;
  EFI_PEI_HOB_POINTERS  GuidHob;    
  
  GuidHob.Raw = GetFirstGuidHob(&gEfiPlatDimmInfoGuid);
  ASSERT(GuidHob.Raw != NULL);  
  DimmInfo = (VOID*)(GuidHob.Guid+1);
  
  return DimmInfo;
}


VOID *GetSetupDataHobData(VOID)
{
  EFI_PEI_HOB_POINTERS  GuidHob;
  VOID                  *SetupData;

  GuidHob.Raw = GetFirstGuidHob(&gPlatformSetupVariableGuid);
  ASSERT(GuidHob.Raw != NULL);
  SetupData = (VOID*)(GuidHob.Guid+1);

  return SetupData;
}

#ifdef ZX_TXT_SUPPORT
VOID *GetAsiaVariableHobData(VOID)
{
  EFI_PEI_HOB_POINTERS  GuidHob;
  VOID                  *AsiaVariable;
  EFI_GUID              gAsiaVariableGuid = ASIA_VARIABLE_GUID;

  GuidHob.Raw = GetFirstGuidHob(&gAsiaVariableGuid);
  ASSERT(GuidHob.Raw != NULL);
  AsiaVariable = (VOID *)(GuidHob.Guid + 1);

  return AsiaVariable;
}
#endif

VOID *GetAcpiRam(VOID)
{
  PLATFORM_S3_RECORD  *S3Record;
  EFI_ACPI_RAM_DATA   *AcpiRam;
  
  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  ASSERT(S3Record->Signature == PLAT_S3_RECORD_SIGNATURE);  
  AcpiRam  = &S3Record->AcpiRam;
  ASSERT(AcpiRam->Signature == ACPI_RAM_DATA_SIGNATURE);
  return (VOID*)AcpiRam;
}

VOID PlatRecordS3DebugData(CHAR8 *Name, UINT32 Data32)
{
  PLATFORM_S3_RECORD  *S3Record;
  UINT32              NameData;
  UINTN 	            Length;

  NameData = 0x20202020;
  Length = AsciiStrLen(Name);
  if(Length > 4){Length = 4;}
  CopyMem(&NameData, Name, Length);
  
  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  ASSERT(S3Record->Signature == PLAT_S3_RECORD_SIGNATURE);
  
  if(S3Record->DebugDataIndex >= PLAT_DBG_DATA_DD_COUNT){
    return;
  }  

  S3Record->DebugData[S3Record->DebugDataIndex].Id   = NameData;
  S3Record->DebugData[S3Record->DebugDataIndex].Data = Data32;
  S3Record->DebugDataIndex++;
}

VOID *GetAcpiTableScat(VOID)
{
  PLATFORM_S3_RECORD               *S3Record;
  EFI_SMM_COMMUNICATION_ACPI_TABLE *Scat;
  
  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  ASSERT(S3Record->Signature == PLAT_S3_RECORD_SIGNATURE);  
  Scat = (EFI_SMM_COMMUNICATION_ACPI_TABLE*)(UINTN)S3Record->ScatAddr;
  ASSERT(Scat->UefiAcpiDataTable.Header.Signature == EFI_ACPI_4_0_UEFI_ACPI_DATA_TABLE_SIGNATURE);
  return (VOID*)Scat;
}


VOID *GetS3MtrrTable()
{
  PLATFORM_S3_RECORD  *S3Record;
  
  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  ASSERT(S3Record->Signature == PLAT_S3_RECORD_SIGNATURE); 

  return (VOID*)&S3Record->MtrrTable;
}


VOID GetS3Cr3Stack(UINT32 *S3Cr3, UINT32 *S3StackBase, UINT32 *S3StackSize)
{
  PLATFORM_S3_RECORD               *S3Record;
  
  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  ASSERT(S3Record->Signature == PLAT_S3_RECORD_SIGNATURE);  
  if(S3Cr3!=NULL){*S3Cr3 = S3Record->S3Cr3;}
  if(S3StackBase!=NULL){*S3StackBase = S3Record->S3StackBase;}  
  if(S3StackSize!=NULL){*S3StackSize = S3Record->S3StackSize;}
}

VOID *GetS3RecordTable()
{
  PLATFORM_S3_RECORD               *S3Record;
  
  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  ASSERT(S3Record->Signature == PLAT_S3_RECORD_SIGNATURE);

  return (VOID*)S3Record;
}

BOOLEAN IsSmrrTypeSetWB()
{
  PLATFORM_S3_RECORD  *S3Record;

  S3Record = (PLATFORM_S3_RECORD*)GetS3RecordTable();
  if(S3Record->SmrrType == CacheWriteBack){
    return TRUE;
  } else {
    return FALSE;
  }
}


VOID *GetCarTopData()
{
  EFI_PEI_HOB_POINTERS   GuidHob;
  CAR_TOP_DATA           *CarTopData;

  GuidHob.Raw = GetFirstGuidHob(&gCarTopDataHobGuid);
  ASSERT(GuidHob.Raw != NULL);  
  CarTopData  = (CAR_TOP_DATA*)(GuidHob.Guid+1);

  return (VOID*)CarTopData;
}
#ifdef IOE_EXIST
EFI_STATUS LoadIoeMcuFw(UINT8 BusOfEptrfc, VOID *FwAddr_alloc,UINT16 AutoFillAddr,UINT16 AutoFillLen)
{
	
	UINT8 Dev = 0;
	UINT8 Func = 0;
	UINT32	FwAddr_Hi,FwAddr_Lo;
	UINT8 TmpReg;
	UINT64 *FwAddr;
	
	//[3]Set the parameters into registers
	//Setting the FW instruction execution Start_address/Length in 8051
  	MmioWrite32((PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_AUTOFILL_START_ADDR), (UINT32)AutoFillAddr);	//set firmware auto fill address
  	MmioWrite32((PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_AUTOFILL_LENGTH), AutoFillLen<<1); 	 //Max 32KB - AutoLoad length

	//Setting the FW
	FwAddr = (UINT64 *)FwAddr_alloc;
	//FwAddr= (UINT64)((UINT32*)FwAddr_alloc);

	DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU] [Line:%d] FW address in DRAM = %x\n",__LINE__,FwAddr));
  	FwAddr_Lo= (UINT32)((UINT64)((UINT32*)FwAddr));
	FwAddr_Hi= (UINT32)(((UINT64)((UINT32*)FwAddr))>>32);
	if((FwAddr_Lo&0xFFFFFFC0) != FwAddr_Lo){
		ASSERT_EFI_ERROR(EFI_ACCESS_DENIED);
	}
	DEBUG((EFI_D_ERROR, "              [Line:%d] FwAddr_Lo=%x  FwAddr_Hi=%x\n",__LINE__,FwAddr_Lo, FwAddr_Hi));
	FwAddr_Hi = 0;
	MmioWrite32((PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_FW_INSTRUCTION_BASEADDR_Lo), FwAddr_Lo);
	MmioWrite32((PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_FW_INSTRUCTION_BASEADDR_Hi), FwAddr_Hi);
	
  	//Flush cache[xxx]
  	AsmWbinvd();
  	DEBUG((EFI_D_ERROR, "              [Line:%d] Flush Cache\n",__LINE__));


#if 1
	//Reset FW
	TmpReg = MmioRead8((PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_SOFTWARE_RESET));
	MmioWrite8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_SOFTWARE_RESET, TmpReg&0xFE);		//reset state
	//MmioWrite8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_SOFTWARE_RESET, TmpReg|0x01);		//exit reset state	
#endif


	//Enable AutoFill fw
	TmpReg = MmioRead8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_AUTOFILL_EN); 
	TmpReg |= 0x01;
  	MmioWrite8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_AUTOFILL_EN,TmpReg); 				//auto fill enable



	DEBUG((EFI_D_ERROR, "              [Line:%d] Wait...\n",__LINE__));

	{
		UINT32 ddd,i;
		for(i=0;i<0xFF;i+=4){
			if((i&0xF)==0x0){
				DEBUG((EFI_D_ERROR, "\n              Rx0x%02x: ",i));
			}
			ddd = MmioRead32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + i);
			DEBUG((EFI_D_ERROR, " 0x%08x ",ddd));
		}
		DEBUG((EFI_D_ERROR, "\n"));
	}

#if 1//ALJ20161030: for 4f hang issue ,remove for now.
	//Wait for autofill done
	do{
		TmpReg =  MmioRead8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_AUTOFILL_DONE);

		//**
		//we need to add a timer to wait this bit to be cleard
		//and if timeout 
		//we should assert() an error break 
	}while((TmpReg&BIT0) == 0x00);
	DEBUG((EFI_D_ERROR, "              [Line:%d] FW AutoFill Done\n",__LINE__));
#else 
	{
		UINTN pp,xx;
		for(pp = 0;pp<0xFFFF ;pp++ )
			for(xx =0; xx<0xFFF; xx++);
	}
#endif
	

	//disable AutoFill fw
	TmpReg = MmioRead8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_AUTOFILL_EN); 
	TmpReg &= 0xFE;
  	MmioWrite8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_AUTOFILL_EN,TmpReg);


#if 1
	//Reset FW
	TmpReg = MmioRead8((PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_SOFTWARE_RESET));
	//MmioWrite8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_SOFTWARE_RESET, TmpReg&0xFE);		//reset state
	MmioWrite8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_SOFTWARE_RESET, TmpReg|0x01);		//exit reset state	
#endif



  return EFI_SUCCESS;	
}

EFI_STATUS
LoadIoeXhciFw(
  UINT8 BusOfEptrfc,
  VOID *FwAddr_alloc
)
{
  UINT8   Dev = 18;
  UINT8   Func = 0;
  UINT8   TmpReg;
  UINT16  TmpReg16;
  UINT32  TmpReg32;
  UINT32  FwAddr_Hi, FwAddr_Lo;

  UINT64 *FwAddr;
  
  UINT16  AutoFillAddr    = 0x500;
  UINT16  AutoFillLen     = 0x5000;   // 20KB

  AsmWbinvd();
  MicroSecondDelay (5);

  //Setting the FW
  FwAddr    = (UINT64 *)FwAddr_alloc;
  FwAddr_Lo = (UINT32)((UINT64)((UINT32*)FwAddr));
  FwAddr_Hi = (UINT32)(((UINT64)((UINT32*)FwAddr))>>32);
  if((FwAddr_Lo & 0xFFFF0000) != FwAddr_Lo) {
    return EFI_ACCESS_DENIED;
  }

  FwAddr_Hi = 0;
  DEBUG((EFI_D_INFO, "                  FwAddr_Lo=%x  FwAddr_Hi=%x\n", FwAddr_Lo, FwAddr_Hi));

  //enable bus master
  MmioOr8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+PCI_COMMAND_OFFSET, EFI_PCI_COMMAND_BUS_MASTER); 
  TmpReg=MmioRead8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+PCI_COMMAND_OFFSET);
  DEBUG((EFI_D_INFO, "                  Rx04 is %x\n", TmpReg));

  //operation enable
  MmioOr8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+XHCI_OPT_RX43, XHCI_OPT_CFG_EN); 
  TmpReg=MmioRead8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+XHCI_OPT_RX43);
  DEBUG((EFI_D_INFO, "                  Rx43 is %x\n", TmpReg));

  //whether the value of Xhci Openrisc Software Reset Controlled by BIOS is 1
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE+0x20);
  TmpReg = MmioRead8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+XHCI_OPT_CFG_DAT);  
  DEBUG((EFI_D_INFO, "                  the default value of MCU software reset is %x\n", TmpReg));

  //MCU_ON_BOARD_APP=1 Rx04[0] = 1'b
  //SPIROM_ON_BOARD=0, MCU_INS_BUF_EN=1(Rx05[1:0]=10'b)
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+ XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE+0x04);
  MmioWrite16(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+ XHCI_OPT_CFG_DAT, 0x0201);
  TmpReg16 = MmioRead16(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+ XHCI_OPT_CFG_DAT);
  DEBUG((EFI_D_INFO, "                  Rx30004  %x [0201]\n", TmpReg16));

  //Set Xhci Fw Lower Base Address in system memory
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE+0x28);
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT, (UINT32)(UINTN)FwAddr_Lo);
  TmpReg32 =  MmioRead32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT);
  DEBUG((EFI_D_INFO, "                  Rx30028 FW Low Address  %x \n", TmpReg32));

  //Set Xhci Fw Upper Base Address in system memory
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE+0x2C); 
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT, FwAddr_Hi);   
  TmpReg32 =  MmioRead32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT);
  DEBUG((EFI_D_INFO, "                  Rx3002C FW High Address  %x \n", TmpReg32));

  //Xhci Openrisc auto fill start address
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE+0x0C);
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT, AutoFillAddr); 
  TmpReg32 =  MmioRead32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT);
  DEBUG((EFI_D_INFO, "                  Rx3000C Autofill Start Address  %x \n", TmpReg32));

  //Xhci Openrisc auto fill length
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE+0x08);
  MmioWrite16(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT+2, AutoFillLen); 

  TmpReg16 =  MmioRead16(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT+2);
  DEBUG((EFI_D_INFO, "                  Rx30008 Autofill Length  %x \n", TmpReg16));

  //Xhci Openrisc Instruction Auto-fill Enable Rx08=1 
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE+0x08);
  MmioWrite8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT, 1);    

  //operation disable
  MmioAnd8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+XHCI_OPT_RX43, (UINT8)~XHCI_OPT_CFG_EN);

  DEBUG((EFI_D_INFO, "                  Auto-fill Enable and Wait...\n"));

  do {
    TmpReg32 =  MmioRead32(PCI_DEV_MMBASE(BusOfEptrfc, Dev, Func) + 0xB0);
  } while ( (TmpReg32 & BIT0) != BIT0 );
  DEBUG((EFI_D_INFO, "                  Xhci FW Init Done:%x\n", TmpReg32));

  //disable  bus master 
  MmioAnd8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+PCI_COMMAND_OFFSET, (UINT8)~EFI_PCI_COMMAND_BUS_MASTER); 

  return EFI_SUCCESS;
}
#endif


//ALJ-CHX002- FWLOAD 
EFI_STATUS LoadXhciFw(
    UINT8   BusNum,
    UINT8   DevNum,
    UINT8   FunNum,
    UINT32  FwAddr_Lo,
    UINT32  FwAddr_Hi
)
{
    UINT16      AutoFillAddr    = 0x500;
    UINT16      AutoFillLen     = 0x1800;   // 6KB
    UINT32      Data;

    AsmWbinvd();
    MicroSecondDelay (5);

    //
    // 0. Check Fw Address if 64KB-aligned
    //
    if (FwAddr_Hi != 0) {
        DEBUG((EFI_D_ERROR, "[CHX002_XHCI_FW]: Firmware address beyond 4G!\n"));
        return EFI_INVALID_PARAMETER;
    }

    if ((FwAddr_Lo & 0xFFFF0000) != FwAddr_Lo) {
        DEBUG((EFI_D_ERROR, "[CHX002_XHCI_FW]: Firmware address is not 64KB-aligned!\n"));
        return EFI_ACCESS_DENIED;
    }

    DEBUG((EFI_D_INFO, "                  Firmware Address Low = 0x%08X  Firmware Address High = 0x%08X\n", FwAddr_Lo, FwAddr_Hi));

    //
    // 1. Enable bus master and OPTCFG access
    //
    MmioOr8(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + PCI_COMMAND_OFFSET, EFI_PCI_COMMAND_BUS_MASTER);
    Data    = MmioRead8(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + PCI_COMMAND_OFFSET);
    DEBUG((EFI_D_INFO, "                  PCI Command(Rx04[2]) is %X\n", ((Data & BIT2) >> 2)));
    MmioOr8(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_RX43, XHCI_OPT_CFG_EN);
    Data    = MmioRead8(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_RX43);
    DEBUG((EFI_D_INFO, "                  Rx43[0] is %X\n", (Data & BIT0)));

    //
    // 2. Peek path for MCU DMA cycle
    //
    MmioWrite32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_ADR, 0xC0);
    Data    = MmioRead32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_DAT);
    DEBUG((EFI_D_INFO, "                  MCUDMASEL(RxC0[5]) is %Xb  (0b for non-snoop, 1b for snoop)\n", ((Data & BIT5) >> 5)));

    //
    // 3. Config Base Address of MCU Firmware in System Memory
    //
    MmioWrite32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE + 0x28);
    MmioWrite32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_DAT, FwAddr_Lo);
    Data    = MmioRead32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_DAT);
    DEBUG((EFI_D_INFO, "                  Base Address of MCU Firmware in System Memory Low(Rx30028):  0x%08X \n", Data));
    MmioWrite32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE + 0x2C);
    MmioWrite32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_DAT, FwAddr_Hi);
    Data    = MmioRead32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_DAT);
    DEBUG((EFI_D_INFO, "                  Base Address of MCU Firmware in System Memory High(Rx3002C):  0x%08X \n", Data));

    //
    // 4. Config Start Address of Auto-fill Instruction
    //
    MmioWrite32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE + 0x0C);
    MmioWrite32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_DAT, AutoFillAddr);
    Data    = MmioRead32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_DAT);
    DEBUG((EFI_D_INFO, "                  Start Address of Auto-fill Instruction(Rx3000C)  0x%08X \n", Data));

    //
    // 5. Config MCU Instruction Auto-fill Length
    //
    MmioWrite32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE + 0x08);
    MmioWrite16(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_DAT + 2, AutoFillLen);
    Data    = MmioRead32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_DAT);
    DEBUG((EFI_D_INFO, "                  MCU Instruction Auto-fill Length(Rx3000A)  0x%04X \n", (UINT16)(Data >> 16)));

    //
    // 6. Start MCU Instruction Auto-fill
    //
    MmioWrite32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE + 0x08);
    MmioWrite8(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_DAT, 0x1);
    Data    = MmioRead8(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_DAT);
    DEBUG((EFI_D_INFO, "                  MCU Instruction Auto-fill Enable(Rx30008[0])  %X\n", (Data & BIT0)));

    //
    // 7. Wait for autofill done
    //
    DEBUG((EFI_D_INFO, "                  Wait autofill done...\n"));
    do {
        MmioWrite32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE + 0x08);
        Data    = MmioRead8(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_DAT + 0x1);
    } while ((Data & BIT0) == 0x00);
    DEBUG((EFI_D_INFO, "                  MCU Instruction Auto-fill Done(Rx30009[0])  %X\n", (Data & BIT0)));

    //
    // 8. Config MCU Software Reset
    //
    MmioWrite32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE + 0x20);
    MmioWrite8(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_CFG_DAT, 0x1);
    // Data    = MmioRead8(PCI_DEV_MMBASE(BusNum, DevNum, FunNum + XHCI_OPT_CFG_DAT);
    // DEBUG((EFI_D_INFO, "                  MCU Software Reset(Rx30020[0]) is %X\n", (Data & BIT0)));

    //
    // 9. Disable bus master and OPTCFG access
    //
    MmioAnd8(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_RX43, (UINT8)~XHCI_OPT_CFG_EN);
    Data    = MmioRead8(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_OPT_RX43);
    DEBUG((EFI_D_INFO, "                  Rx43[0] is %X\n", (Data & BIT0)));

    //
    // 10. Wait FW init done
    //
    DEBUG((EFI_D_INFO, "                  Wait FW init done...\n"));
    do  {
        Data    = MmioRead32(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + XHCI_FWSWMSG0_REG);
    } while ( (Data & XHCI_FWSWMSG0_INITDONE) != XHCI_FWSWMSG0_INITDONE );
    DEBUG((EFI_D_INFO, "                  MCU Init Done(RxB0[0]) is %X\n", (Data & BIT0)));

    // MicroSecondDelay (60*1000);

    MmioAnd8(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + PCI_COMMAND_OFFSET, (UINT8)~EFI_PCI_COMMAND_BUS_MASTER);
    Data    = MmioRead8(PCI_DEV_MMBASE(BusNum, DevNum, FunNum) + PCI_COMMAND_OFFSET);
    DEBUG((EFI_D_INFO, "                  PCI Command(Rx04[2]) is %X\n", ((Data & BIT2) >> 2)));

    return EFI_SUCCESS;
}


/**
@CJW-20171114
@Param:
	PeMcuFw: Memory base address for saving FW
	IsDoEQ: Do EQ or not, if DO EQ, then this function 
	TP_Value: If DO EQ, will request TP_Value as the Tx preset for Endpoint
	TargetBus: For single socket, this should be 0h, for dual socket, 'TargetBus' means BusNumber of target socket
@Return:
	
**/
EFI_STATUS LoadPeMcuFw(
	VOID *PeMcuFw, 	
    VOID *PeMcuData,
	UINT8 IsDoEQ, 
	UINT8 TP_Value, 
	UINT8 TargetBus)
{
  EFI_STATUS  	Status;
  UINTN 		PCIEPhyMMIOBase;
  UINT32    	FwAddr;
  UINT32         DataAddr;  
  UINT8 		TmpReg;
  UINT16		TmpReg16;
  UINT8 		CpuWrContFlag = 0;
  UINT8         Temp8=0;
  SETUP_DATA *gSetupHob;
  
  Status  = EFI_SUCCESS;


  gSetupHob = GetSetupDataHobData();

  //gBS = (EFI_BOOT_SERVICES*)BootServices;

  //
  // Get EPHY Base Address 
  //
  PCIEPhyMMIOBase=(UINTN)MmioRead32((PCI_DEV_MMBASE(TargetBus,0,5)+D0F5_PCIE_EPHY_BASE_ADR)); 	 
  PCIEPhyMMIOBase = PCIEPhyMMIOBase<<8;
  DEBUG((DEBUG_ERROR," PCIEPhyMMIOBase: %x\n", PCIEPhyMMIOBase));

  //
  // Autofill start address of FW
  //
  MmioWrite16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_START_ADR),0);						 
  TmpReg16 = MmioRead16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_START_ADR));   
  DEBUG((DEBUG_ERROR," PCIEPHYCFG_PEMCU_AUTO_FILL_START_ADR: %x\n", TmpReg16));

  //
  //Autofill Lenth (Max 16K for CHX002, All zero means 16K)
  //
  MmioWrite16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_LEN),SIZE_16KB);		 
  TmpReg16 = MmioRead16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_LEN));  
  DEBUG((DEBUG_ERROR," PCIEPHYCFG_PEMCU_AUTO_FILL_LEN: %x\n", TmpReg16));

  //
  //Reformat MemoryAddressBase of PEMCU FW
  //
  FwAddr= (UINT32)((UINT64)((UINT32*)PeMcuFw));
  DEBUG((EFI_D_ERROR, "[Line:%d] FwAddr = %x\n",__LINE__,FwAddr));
  
  FwAddr=FwAddr>>16;
  DEBUG((EFI_D_ERROR, "[Line:%d] FwAddr = %x\n",__LINE__,FwAddr));
  MmioWrite32((PCIEPhyMMIOBase|PCIEPHYCFG_BASE_ADR_OF_PEMCU_FW_FOR_INSTRUCTION),FwAddr); //set instruction base address
  MmioRead32((PCIEPhyMMIOBase|PCIEPHYCFG_BASE_ADR_OF_PEMCU_FW_FOR_INSTRUCTION));   

  //
  //Reformat MemoryAddressBase of PEMCU XDATA 
  //
  DataAddr= (UINT32)((UINT64)((UINT32*)PeMcuData));
  MmioWrite8(DataAddr+0x1000,gSetupHob->PcieEMEQScanTime);
  MmioWrite8(DataAddr+0x1001,gSetupHob->PcieDoEqMethod);
  DEBUG((EFI_D_ERROR, "[Line:%d] DataAddr = %x\n",__LINE__,DataAddr));
  DataAddr=DataAddr>>16;
  DEBUG((EFI_D_ERROR, "[Line:%d] DataAddr = %x\n",__LINE__,DataAddr));
  MmioWrite32((PCIEPhyMMIOBase|PCIEPHYCFG_BASE_ADR_OF_PEMCU_FW_FOR_DATA_SPACE),DataAddr); //set instruction base address
  MmioRead32((PCIEPhyMMIOBase|PCIEPHYCFG_BASE_ADR_OF_PEMCU_FW_FOR_DATA_SPACE));   


  //
  //Flush cache[xxx]
  //
  AsmWbinvd();
  DEBUG((EFI_D_ERROR, "[Line:%d] Flush Cache\n",__LINE__));

  //
  //Autofill Enable - start to load FW from system memory
  //
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE),PCIEPHYCFG_INST_AUTOFILL_EN); 			//auto fill enable

  //
  //Loop to wait Autofill Done
  //
  TmpReg=0;
  while((TmpReg&0x02)==0)  
  {
  	TmpReg=MmioRead8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE));
  }
  TmpReg=MmioRead8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE));
  DEBUG((EFI_D_ERROR, "[Line:%d] Fill done status = %X\n",__LINE__, TmpReg));  
  DEBUG((EFI_D_ERROR, "[Line:%d] FW AutoFill Done\n",__LINE__));

  //
  //Reset MCU to run - Write 0 to reset then write 1 to run
  //
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE),0x00); 	
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE),PCIEPHYCFG_MCU_RST); //reset mcu


  ////////////////////////////////////////////////////////
  ///For EQ
  ///////////////////////////////////////////////////////

#if 1	 ///Add for Redo-EQ in DXE stage.  	

  if(IsDoEQ == 0){

	Temp8 = MmioRead8(PCIEPhyMMIOBase|PCIEPHYCFG_PCIE_ROMSIP_REG);	
	if((Temp8&PCIEPHYCFG_R_PEMCU_BIOS) != PCIEPHYCFG_R_PEMCU_BIOS){
		CpuWrContFlag = 0 ;
		MmioWrite8(PCIEPhyMMIOBase|PCIEPHYCFG_PCIE_ROMSIP_REG,Temp8|PCIEPHYCFG_R_PEMCU_BIOS);
		DEBUG((EFI_D_ERROR, "[JNY-DEBUG] R_PEMCU_BIOS Control Bit set 0X%02x\n", MmioRead8(PCIEPhyMMIOBase|PCIEPHYCFG_PCIE_ROMSIP_REG)));
	}else{
	    CpuWrContFlag = 1;
		DEBUG((EFI_D_ERROR, "[JNY-DEBUG] R_PEMCU_BIOS Control Bit is 1!\n"));  
	}
		
  
	///For Debug - Add by ChrisJWang 2015.07.31 
	///[Possible Bug:When No UART debug message output, Pemcu AutoFill Failed]
	///Those Code can check whether FW autofill success 
	MicroSecondDelay(100);					//Wait 100us
    TmpReg = MmioRead8(PCIEPhyMMIOBase+PEMCU_RESET_AUTOFILL_EN);
    if(TmpReg == 0x00){
  	  DEBUG((EFI_D_ERROR, "[AutoFill-001] AutoFill Failed\n"));
	  while(1){
		IoWrite8(0x80,0x52); 						//80 Port show 0x52	
	  }
    } 
    MicroSecondDelay(1000);	         // 1 msec delay  
	
	//
  	//CMD stage ,CMD Tx Preset = 3
  	//
  	DEBUG((EFI_D_ERROR,"TxPreset = SetupData->EQTxPreset - Start \n"));
  	MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z6),0x03); 	
  	MicroSecondDelay(1000);	        // 1 msec delay  
  	// Run CMD (DB=1)
  	MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z3),PCIEPHYCFG_BIOS2PEMCU_DB); 	
  	do{
		TmpReg = MmioRead8((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z3)); 	
	}while(TmpReg == 0x01);

	//
  	//ARG stage - Get from Setup
  	//
  	TmpReg16 = TP_Value;
  	MmioWrite16((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z6),TmpReg16); 	
  	MicroSecondDelay(1000);	         // 1 msec delay  
  	// Run CMD (DB=1)
  	MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z3),PCIEPHYCFG_BIOS2PEMCU_DB); 		
  	do{
		TmpReg = MmioRead8((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z3)); 	
   	}while(TmpReg == 0x01);

  	DEBUG((EFI_D_ERROR," TxPreset = SetupData->EQTxPreset - end \n")); 
	if(CpuWrContFlag==0){
		MmioWrite8(PCIEPhyMMIOBase|PCIEPHYCFG_PCIE_ROMSIP_REG,Temp8);
		DEBUG((EFI_D_ERROR, "[JNY-DEBUG] R_PEMCU_BIOS Control clear to 0X%02x\n",MmioRead8(PCIEPhyMMIOBase|PCIEPHYCFG_PCIE_ROMSIP_REG)));
	}else{
		DEBUG((EFI_D_ERROR, "[JNY-DEBUG] R_PEMCU_BIOS Control Keep 1!\n"));
	}
}

#endif

  return Status;  
}





/*
(0, F, 0, A0)[0] - PM device detected
(0, F, 0, A1)[0] - PS device detected
(0, F, 0, A2)[0] - SM device detected
(0, F, 0, A3)[0] - SS device detected
*/ 
// assume this host is under IDE mode.
UINT8 WaitIdeDeviceReady (UINTN Bus, UINTN Dev, UINTN Func, UINTN DetectTimeOutInms)
{
        UINT8       PortStatus[4];
        UINT16      CmdPort[2];
        UINTN       Count;
  CONST UINTN       TimeOut = 3000;
        UINT8       Interface;
        UINTN       SataPciBase;
        UINT8       ChannelCount;
        UINT8       Ch0, Ch1;	
        BOOLEAN     DeviceDetected;


  SataPciBase = PCI_DEV_MMBASE(Bus, Dev, Func);

  Interface = MmioRead8(SataPciBase + PCI_CLASSCODE_OFFSET);
  if(Interface & BIT0){
    CmdPort[0] = MmioRead16(SataPciBase + PCI_BASE_ADDRESSREG_OFFSET) & ((UINT16)~BIT0);
  } else {
    CmdPort[0] = 0x1F0;
  }
  if(Interface & BIT2){
    CmdPort[1] = MmioRead16(SataPciBase + PCI_BASE_ADDRESSREG_OFFSET + 8) & ((UINT16)~BIT0);
  } else {
    CmdPort[1] = 0x170;
  }	
  DEBUG((EFI_D_INFO, "CmdPort: %04X, %04X\n", CmdPort[0], CmdPort[1]));

  MmioOr8 (SataPciBase + PCI_COMMAND_OFFSET, EFI_PCI_COMMAND_IO_SPACE);

  DeviceDetected = FALSE;
  Count = DetectTimeOutInms;
  DEBUG((EFI_D_INFO, "DeviceDetect\n"));
  while(Count--){
    *(UINT32*)&PortStatus[0] = MmioRead32(SataPciBase + 0xA0);
    if((PortStatus[0] & BIT0) || (PortStatus[1] & BIT0) || 
       (PortStatus[2] & BIT0) || (PortStatus[3] & BIT0)){   // Device Detected
      DeviceDetected = TRUE; 
      MicroSecondDelay(100000);
      break;
    }
    MicroSecondDelay(1000);
  }
  *(UINT32*)&PortStatus[0] = MmioRead32(SataPciBase + 0xA0);  

  if(!DeviceDetected) {
    DEBUG((EFI_D_INFO, "(%X,%X,%X) No IDE Device Found!\n", Bus, Dev, Func));
		ChannelCount = 0;
    goto ProcExit;
  }
	
  if(PortStatus[0] & BIT0){                     // port 0 present
    IoWrite8(CmdPort[0]+6, 0xE0);
    MicroSecondDelay(1000);
    Count = TimeOut;
    while(Count--){
      if(!(IoRead8(CmdPort[0]+7) & BIT7)){         // not busy
        break;
      }
      MicroSecondDelay (1000);
    }
    DEBUG((EFI_D_ERROR, "PM_Sts:%X <%dms\n", IoRead8(CmdPort[0]+7), TimeOut-Count));
  }
  if(PortStatus[1] & BIT0){                     // port 1 present
    IoWrite8(CmdPort[0]+6, 0xF0);
    MicroSecondDelay (1000);
    Count = TimeOut;
    while(Count--){
      if(!(IoRead8(CmdPort[0]+7) & BIT7)){         // not busy
        break;
      }
      MicroSecondDelay (1000);
    }
    DEBUG((EFI_D_ERROR, "PS_Sts:%X <%dms\n", IoRead8(CmdPort[0]+7), TimeOut-Count));
  }

  if(PortStatus[2] & BIT0){                     // port 2 present
    IoWrite8(CmdPort[1]+6, 0xE0);
    MicroSecondDelay(1000);
    Count = TimeOut;
    while(Count--){
      if(!(IoRead8(CmdPort[1]+7) & BIT7)){         // not busy
        break;
      }
      MicroSecondDelay (1000);
    }
    DEBUG((EFI_D_ERROR, "SM_Sts:%X <%dms\n", IoRead8(CmdPort[1]+7), TimeOut-Count));
  }
  if(PortStatus[3] & BIT0){                     // port 3 present
    IoWrite8(CmdPort[1]+6, 0xF0);
    MicroSecondDelay (1000);
    Count = TimeOut;
    while(Count--){
      if(!(IoRead8(CmdPort[1]+7) & BIT7)){         // not busy
        break;
      }
      MicroSecondDelay (1000);
    }
    DEBUG((EFI_D_ERROR, "SS_Sts:%X <%dms\n", IoRead8(CmdPort[1]+7), TimeOut-Count));
  }

  Ch0 = (PortStatus[0] & BIT0) || (PortStatus[1] & BIT0);
  Ch1 = (PortStatus[2] & BIT0) || (PortStatus[3] & BIT0);
// CH0  CH1  Count
//   0    0      0
//   0    1      2
//   1    0      1
//   1    1      2
  if(Ch1){
    ChannelCount = 2;
	} else {
    ChannelCount = 1;
	}

ProcExit:
  DEBUG((EFI_D_INFO, "%a(%X,%X,%X) ChCount:%d\n", __FUNCTION__, Bus, Dev, Func, ChannelCount));	
  return ChannelCount;
}



