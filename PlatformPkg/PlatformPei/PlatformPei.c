
//------------------------------------------------------------------------------
#include "PlatformPei.h"
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/MpServices.h>
#include <SetupVariable.h>
#include <ByoStatusCode.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PerformanceLib.h>
#include <Library/MtrrLib.h>
#include <PlatS3Record.h>
#ifdef ZX_SECRET_CODE
#include <Ppi/CpuMpConfig.h>
#endif
#ifdef ZX_TXT_SUPPORT
#include <AsiaVariable.h>
#endif



//------------------------------------------------------------------------------
EFI_STATUS
EFIAPI
EndOfPeiCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

EFI_STATUS
EFIAPI
AcpiWakeVectorCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

EFI_STATUS
EFIAPI
CpuMpPeiCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );
 
EFI_STATUS
EFIAPI
CpuDebugPei(	
	IN EFI_PEI_SERVICES 		  **PeiServices,
	IN EFI_PEI_MP_SERVICES_PPI    *MpSvr,
	IN SETUP_DATA				   *SetupHob
  );

EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );  




#ifdef EDKC_DEBUG_MASK
  STATIC EFI_DEBUG_MASK_PPI gDebugMaskPpi  = {EDKC_DEBUG_MASK};
#else
  STATIC EFI_DEBUG_MASK_PPI gDebugMaskPpi  = {EFI_D_ERROR|EFI_D_INFO};
#endif
  
STATIC EFI_GUID gEfiDebugMaskPpiGuid     = EFI_DEBUG_MASK_PPI_GUID;

STATIC EFI_PEI_NOTIFY_DESCRIPTOR  gPpiNotifyList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiEndOfPeiSignalPpiGuid,
    EndOfPeiCallback  
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiPeiMpServicesPpiGuid,
    CpuMpPeiCallback  
  },  
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMemoryDiscoveredPpiGuid,
    MemoryDiscoveredPpiNotifyCallback 
  }
};

STATIC EFI_PEI_PPI_DESCRIPTOR  gPpiInstallList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiDebugMaskPpiGuid,
    &gDebugMaskPpi
  }
};


EFI_STATUS  
GetAsiaPpi (
  EFI_ASIA_SB_PPI                  **SbPpi,
  EFI_ASIA_NB_PPI                  **NbPpi,
  EFI_ASIA_DRAM_PPI                **DramPpi,
  EFI_ASIA_CPU_PPI_PROTOCOL        **CpuPpi,
  ASIA_SB_LIB_PPI                  **SbLibPpi
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  if(SbPpi!=NULL){
    Status = PeiServicesLocatePpi (
               &gAsiaSbPpiGuid,
               0,
               NULL,
               (VOID**)SbPpi
               );
    ASSERT_EFI_ERROR(Status);
  }  

  if(NbPpi!=NULL){
    Status = PeiServicesLocatePpi (
               &gAsiaNbPpiGuid,
               0,
               NULL,
               (VOID**)NbPpi
               );
    ASSERT_EFI_ERROR(Status);
  }  

  if(CpuPpi!=NULL){
    Status = PeiServicesLocatePpi (
               &gAsiaCpuPpiGuid,
               0,
               NULL,
               (VOID**)CpuPpi
               );
    ASSERT_EFI_ERROR(Status);
  }  

  if(DramPpi!=NULL){
    Status = PeiServicesLocatePpi (
               &gAsiaDramPpiGuid,
               0,
               NULL,
               (VOID**)DramPpi
               );
    ASSERT_EFI_ERROR(Status);
  }  
  
  if(SbLibPpi!=NULL){
    Status = PeiServicesLocatePpi (
               &gAsiaSbLibPpiGuid,
               0,
               NULL,
               (VOID**)SbLibPpi
               );
    ASSERT_EFI_ERROR(Status);  
  }
  
  return Status;
}

// JNY_IOE_PORTING [20180329] - START
#ifdef IOE_EXIST
EFI_STATUS
HandleIoeMcuXhciFwPei(
  PLATFORM_S3_RECORD *S3Record,
  SETUP_DATA         *SetupData
	)
{

	#define VIDDID_IOE 		0x071F1106
	#define VIDDID_IOE_ZX	0x071F1D17

	EFI_STATUS Status = EFI_SUCCESS;
	VOID *IoeMcuMemAddr;
	UINT16 IoeMcuAddrStart,IoeMcuAddrLen;
	UINT8 IoeEptrfcBusNum;
  VOID *IoeXhciMemAddr;

	
	//
	//Here we only think the IOE connected to RP(Bus number = 0)
	//
	UINT8 Tbus,Tdev,Tfunc;
	UINT32 Tex;
	UINT8 Tmpx,Tmpx1;

	//	UINT8 Orig19;//, Orig1A;

	//UINT32 OrigMemBaseLimit;
	UINT32 ReadBar;
	UINT8 ReadEpBus,ReadDnportBus,ReadPcieifBus;
	UINT8 HideFlag = 0;

	//
	// Read Scratch register to determine whether to load fw
	//
	if (! (BIT0 & MmioRead8(PCI_DEV_MMBASE(0, 0, 6) + 0x47))) {
		DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU/XHCI] SKIP CND003 Init [HandleIoeMcuXhciFwPei()]\n"));
		return EFI_SUCCESS;
	}

	DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU/XHCI] Start to load fw... [8051 + XHCI]\n"));

	//Get the S3Record
	//IoeEptrfcBusNum = (UINT8)S3Record->PcieIoeEptrfcBusNum;
	IoeMcuMemAddr = (VOID*)(UINTN)S3Record->PcieIoeMcu;
	IoeMcuAddrStart = (UINT16)S3Record->PcieIoeMcuAddr;
	IoeMcuAddrLen = (UINT16)S3Record->PcieIoeMcuLen;
  	//ttpandbg
  IoeXhciMemAddr = (VOID*)(UINTN)S3Record->PcieIoeXhci;




#if 0
	//Search IOE on RPs
	Tbus = 0;
	for(Tdev = 0; Tdev < 0x32; Tdev++){
		for(Tfunc = 0; Tfunc<8; Tfunc++){
			Tex = MmioRead32(PCI_DEV_MMBASE(Tbus, Tdev, 0) + 0x00);
			if(Tex == 0xFFFFFFFF){
				break;
			}
			//Bridge?
			if(!(MmioRead8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0xE) & BIT0)){
				continue;
			}
				
			Orig19 = MmioRead8(PCI_DEV_MMBASE(Tbus, Tdev, 0) + 0x19);
		
			Tex = MmioRead32(PCI_DEV_MMBASE(Orig19, 0, 0) + 0x00);	
			
			if((Tex == VIDDID_IOE)||(Tex ==VIDDID_IOE_ZX)){
				DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU/XHCI] Found Ioe [%d|%d|%d]\n",Tbus,Tdev,Tfunc));

				ReadEpBus= MmioRead8(PCI_DEV_MMBASE(Tbus,Tdev,Tfunc)+0x19);
				ReadDnportBus= MmioRead8(PCI_DEV_MMBASE(ReadEpBus, 0, 0)+0x19);
				ReadPcieifBus= MmioRead8(PCI_DEV_MMBASE(ReadDnportBus, 8, 0)+0x19);
				IoeEptrfcBusNum = MmioRead8(PCI_DEV_MMBASE(ReadPcieifBus, 0, 0)+0x19);
				DEBUG((EFI_D_ERROR, "                   Get the bus number of ISB/EPTRFC = %02x\n", IoeEptrfcBusNum));
				DEBUG((EFI_D_ERROR, "                   PEEP on Bus:%d\n",ReadEpBus));
				DEBUG((EFI_D_ERROR, "                   PEDN ports on Bus:%d\n",ReadDnportBus));
				DEBUG((EFI_D_ERROR, "                   PCIEIF on Bus:%d\n",ReadPcieifBus));
				DEBUG((EFI_D_ERROR, "                   ISB on Bus:%d\n",IoeEptrfcBusNum));

				goto _FoundIoe;
			}
				
		}
	}
	DEBUG((EFI_D_ERROR, "                   Can't find Ioe\n"));
	Status = EFI_NOT_FOUND;
	return Status;

#else

	Tbus = 0;
	for(Tdev = 0; Tdev < 0x32; Tdev++){
		for(Tfunc = 0; Tfunc<8; Tfunc++){
			Tex = MmioRead32(PCI_DEV_MMBASE(Tbus, Tdev, 0) + 0x00);
			if(Tex == 0xFFFFFFFF){
				break;
			}
			//Bridge?
			if(!(MmioRead8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0xE) & BIT0)){
				continue;
			}
			
			MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc)+0x18, 0x0);
			MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc)+0x19, 0x1);
			MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc)+0x1A, 0x4);

			Tex = MmioRead32(PCI_DEV_MMBASE(1, 0, 0) + 0x00);	

			if((Tex == VIDDID_IOE)||(Tex ==VIDDID_IOE_ZX)){
				DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU/XHCI] Found Ioe [%d|%d|%d]\n",Tbus,Tdev,Tfunc));

				MmioWrite8(PCI_DEV_MMBASE(1, 0, 0)+0x18, 0x1);
				MmioWrite8(PCI_DEV_MMBASE(1, 0, 0)+0x19, 0x2);
				MmioWrite8(PCI_DEV_MMBASE(1, 0, 0)+0x1A, 0x4);

				MmioWrite8(PCI_DEV_MMBASE(2, 8, 0)+0x18, 0x2);
				MmioWrite8(PCI_DEV_MMBASE(2, 8, 0)+0x19, 0x3);
				MmioWrite8(PCI_DEV_MMBASE(2, 8, 0)+0x1A, 0x4);			

				MmioWrite8(PCI_DEV_MMBASE(3, 0, 0)+0x18, 0x3);
				MmioWrite8(PCI_DEV_MMBASE(3, 0, 0)+0x19, 0x4);
				MmioWrite8(PCI_DEV_MMBASE(3, 0, 0)+0x1A, 0x4);	
				
				ReadEpBus= 1;
				ReadDnportBus= 2;
				ReadPcieifBus= 3;
				IoeEptrfcBusNum = 4;
				DEBUG((EFI_D_ERROR, "                   Get the bus number of ISB/EPTRFC = %02x\n", IoeEptrfcBusNum));
				DEBUG((EFI_D_ERROR, "                   PEEP on Bus:%d\n",ReadEpBus));
				DEBUG((EFI_D_ERROR, "                   PEDN ports on Bus:%d\n",ReadDnportBus));
				DEBUG((EFI_D_ERROR, "                   PCIEIF on Bus:%d\n",ReadPcieifBus));
				DEBUG((EFI_D_ERROR, "                   ISB on Bus:%d\n",IoeEptrfcBusNum));

				goto _FoundIoe;
			}		
			

			
		}

	}
	DEBUG((EFI_D_ERROR, "                   Can't find Ioe\n"));
	Status = EFI_NOT_FOUND;
	return Status;
#endif





	
_FoundIoe:
	
	//
	// distinguish BIOS/SPI mode
	// In SPI mode, don't need AutoFill, return directly
	// In BIOS mode, continue
	//
	DEBUG((EFI_D_ERROR, "                   Rx1EA=0x%04x\n", MmioRead16(PCI_DEV_MMBASE(1, 0, 0) + 0x1EA)));
	if( BIT15 & MmioRead16(PCI_DEV_MMBASE(ReadEpBus, 0, 0) + 0x1EA) ){
		DEBUG((EFI_D_ERROR, "                   In BIOS mode - Execute Autofill\n"));
	}else{
		DEBUG((EFI_D_ERROR, "                   In SPI mode - Skip Auotfill - Exit\n"));		
		return Status;
	}

	
	// Read MMIO bar & Show the EPTRFC
	ReadBar =MmioRead32(PCI_DEV_MMBASE(ReadEpBus, 0, 0) + 0x10);
	DEBUG((EFI_D_ERROR, "                   Ep Bar is x%08x\n",ReadBar));
	if(MmioRead8(ReadBar+0x1100+0x52) & BIT7){
		MmioWrite8(ReadBar+0x1100+0x52, (MmioRead8(ReadBar+0x1100+0x52)&(~(BIT7)))); 
		HideFlag = 1;
	}else{
		HideFlag = 0;
	}
	DEBUG((EFI_D_ERROR, "                   HideFlag = %d\n",HideFlag));

	// Enable BusMaster Enable bit(Rx04[2])  
	Tmpx = MmioRead8(PCI_DEV_MMBASE(ReadPcieifBus, 0, 0) + 0x04);
	DEBUG((EFI_D_ERROR, "                   The value of PCIEIF Rx04 is 0x%02x\n",Tmpx));
	Tmpx1 = ((Tmpx & (~BIT2))|BIT2);
	MmioWrite8(PCI_DEV_MMBASE(ReadPcieifBus, 0, 0) + 0x04, Tmpx1);
	DEBUG((EFI_D_ERROR, "                   PCIEIF Rx04 will be set as 0x%02x\n",Tmpx1));
	MmioWrite8(PCI_DEV_MMBASE(ReadDnportBus, 8, 0) + 0x04, 0x07);
	MmioWrite8(PCI_DEV_MMBASE(ReadEpBus, 0, 0) + 0x04, 0x07);
	MmioWrite8(PCI_DEV_MMBASE(Tbus,Tdev,Tfunc) + 0x04, 0x07);
	DEBUG((EFI_D_ERROR, "                   [0|7|0][%04x] Rx04=0x%02x\n",MmioRead16(PCI_DEV_MMBASE(Tbus,Tdev,Tfunc) + 0x02),MmioRead8(PCI_DEV_MMBASE(0, 7, 0) + 0x04)));
	DEBUG((EFI_D_ERROR, "                   [1|0|0][%04x] Rx04=0x%02x\n",MmioRead16(PCI_DEV_MMBASE(ReadEpBus, 0, 0) + 0x02),MmioRead8(PCI_DEV_MMBASE(1, 0, 0) + 0x04)));
	DEBUG((EFI_D_ERROR, "                   [2|8|0][%04x] Rx04=0x%02x\n",MmioRead16(PCI_DEV_MMBASE(ReadDnportBus, 8, 0) + 0x02),MmioRead8(PCI_DEV_MMBASE(2, 8, 0) + 0x04)));
	DEBUG((EFI_D_ERROR, "                   [3|0|0][%04x] Rx04=0x%02x\n",MmioRead16(PCI_DEV_MMBASE(ReadPcieifBus, 0, 0) + 0x02),MmioRead8(PCI_DEV_MMBASE(3, 0, 0) + 0x04)));
	

	//
	// FW Autofill load
	//
	if(SetupData->Cnd003Autofill){
  		Status = LoadIoeMcuFw(IoeEptrfcBusNum,IoeMcuMemAddr, IoeMcuAddrStart,IoeMcuAddrLen);
  		ASSERT_EFI_ERROR(Status);
	}

	//
  // IOE Xhci Fw Autofill load
  //
  Status = LoadIoeXhciFw(IoeEptrfcBusNum, IoeXhciMemAddr);
  ASSERT_EFI_ERROR(Status);

	//
	// Hide EPTRFC again
	//
	if(HideFlag){
		MmioWrite8(ReadBar+0x1100+0x52, (MmioRead8(ReadBar+0x1100+0x52)|(BIT7))); 
	}
	


#if 0  //Check/Dump MMIO_ISB
{
	
	UINT32 val;
	UINT32 xx;
	DEBUG((EFI_D_ERROR, "\n[CJW_IOE] S3 Resume dump IOE tree:"));
	DEBUG((EFI_D_ERROR, "\n          [MMIO_ISB-2] Offset 0x1100: "));
	for(xx=0; xx<0xFF; xx+=4){
		if((xx&0xF) == 0){
			DEBUG((EFI_D_ERROR, "\n          Rx%04x: ",xx));
		}
		val = MmioRead32(ReadBar + 0x1100 + xx);
		DEBUG((EFI_D_ERROR, "%08x ", val));

	}

	DEBUG((EFI_D_ERROR, "\n          [D:0:0]-2: "));
	for(xx=0; xx<0xFF; xx+=4){
		if((xx&0xF) == 0){
			DEBUG((EFI_D_ERROR, "\n          Rx%04x: ",xx));
		}
		val = MmioRead32(PCI_DEV_MMBASE(IoeEptrfcBusNum, 0, 0)+xx);
		DEBUG((EFI_D_ERROR, "%08x ", val));

	}

	
}	
#endif
	

  	return Status;
}
#endif
//JNY_IOE_PORTING - END


EFI_STATUS
HandleXhciFwPei(
    PLATFORM_S3_RECORD  *S3Record
    )
{
    EFI_STATUS              Status          = EFI_SUCCESS;
    UINT16                  tVID;
    UINT16                  tDID;

    // Due to only use memory blow 4G, so we don't judge XhciMcuFw_Hi.
    BOOLEAN                 XhciMcuFwLoaded = ( S3Record->XhciMcuFw_Lo != 0 &&
                                                S3Record->XhciMcuFwSize != 0);
    UINT32                  McuFw_Lo, McuFw_Hi;

    // Now we decide whether to load fw or not, only depends on whether it's loaded during normal boot.
    // If loaded fw successfully during normal boot, but loading fail during S3 resume, just let it ASSERT!
    if( XhciMcuFwLoaded ) {
        DEBUG((EFI_D_INFO, "[CHX002_XHCI_FW]: Firmware loading for S3 resume...\n"));

        tVID     = MmioRead16(PCI_DEV_MMBASE(0, CHX002_XHCI_DEV, 0) + PCI_VID_REG);
        tDID     = MmioRead16(PCI_DEV_MMBASE(0, CHX002_XHCI_DEV, 0) + PCI_DID_REG);

        DEBUG((EFI_D_INFO, "                  [%02X|%02X|%02X](%08X) VID = 0x%04X, DID = 0x%04X\n", 0, CHX002_XHCI_DEV, 0, PCI_DEV_MMBASE(0, CHX002_XHCI_DEV, 0) + PCI_VID_REG, tVID, tDID));

        if( !( ((tVID == PCI_VID_ZX) || (tVID == PCI_VID_VIA)) && (tDID == XHCI_DEVICE_ID) ) ) {
            Status = EFI_DEVICE_ERROR;
            DEBUG((EFI_D_ERROR, "[CHX002_XHCI_FW]: [%02X|%02X|%02X](%08X) not exist!(%r)\n", 0, CHX002_XHCI_DEV, 0, PCI_DEV_MMBASE(0, CHX002_XHCI_DEV, 0) + PCI_VID_REG, Status));
            return Status;
        }

        McuFw_Lo  = S3Record->XhciMcuFw_Lo;
        McuFw_Hi  = S3Record->XhciMcuFw_Hi;

        if (McuFw_Hi != 0) {
            Status  = EFI_INVALID_PARAMETER;
            DEBUG((EFI_D_ERROR, "[CHX002_XHCI_FW]: Firmware buffer beyond 4G!(%r)\n", Status));
            return Status;
        }

        Status = LoadXhciFw(0, CHX002_XHCI_DEV, 0, McuFw_Lo, McuFw_Hi);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "[CHX002_XHCI_FW]: xHCI firmware loading failed for S3 resume!(%r)\n", Status));
        }
    } else {
        Status = EFI_SUCCESS;
        DEBUG((EFI_D_ERROR, "[CHX002_XHCI_FW]: xHCI has been disabled, skip firmware loading.\n"));
    }

    return Status;
}


EFI_STATUS 
HandlePemcuFwPei(
  PLATFORM_S3_RECORD *S3Record
  )
{
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi;
  EFI_STATUS          Status;
  VOID                *PeMcuFw;
  VOID                *PeMcuData;
  SETUP_DATA             *SetupData;
  UINTN                  Size;

  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID**)&Var2Ppi
             );
  ASSERT_EFI_ERROR(Status);
  
  SetupData = BuildGuidHob(&gPlatformSetupVariableGuid, sizeof(SETUP_DATA));
  ASSERT(SetupData!=NULL);
  
  Size   = sizeof(SETUP_DATA);  
  Status = Var2Ppi->GetVariable (
                      Var2Ppi,
                      PLATFORM_SETUP_VARIABLE_NAME,
                      &gPlatformSetupVariableGuid,
                      NULL,
                      &Size,
                      SetupData
                      );

  DEBUG((EFI_D_INFO, __FUNCTION__"() - LoadFw = %X, EQ = %X, TxPreset = %X\n",SetupData->PEMCU_LoadFW_WhenBoot, SetupData->PcieEQ, SetupData->EQTxPreset));

	
  if(SetupData->PEMCU_LoadFW_WhenBoot == 0x01){

	  PeMcuFw = (VOID*)(UINTN)S3Record->PeMcuFw;
	  PeMcuData = (VOID*)(UINTN)S3Record->PeMcuData;

	  //LibCalcCrc32(PeMcuFw, S3Record->PeMcuFw, &Crc32);
	  //ASSERT(Crc32 == S3Record->PeMcuFwCrc32);

	  Status = LoadPeMcuFw(PeMcuFw,PeMcuData, SetupData->PcieEQ, SetupData->EQTxPreset, 0);
	  
  	}

  return Status;
}



VOID PlatformSettingForS3Resume()
{
// After CpuS3Init, Smi could be triggered(RestoreLockBox uses it). 
// So we formal enable SMI here.

  IoAnd16(PMIO_REG(PMIO_GLOBAL_CTL), (UINT16)~PMIO_SMIEN); ///PMIO_Rx2C[0] SMI Disable

  IoWrite16(PMIO_REG(PMIO_PM_STA), PMIO_PWF_STS|PMIO_GBL_STS|PMIO_BM_STS|PMIO_TMR_STS); ///PMIO_Rx00[11][5][4][0]
  IoWrite16(PMIO_REG(PMIO_PM_ENABLE), PMIO_PBTN_EN);	///PMIO_Rx00[8] Power Button Enable
  IoWrite16(PMIO_REG(PMIO_GLOBAL_STA), 0xFFFF);		///PMIO_Rx28[15:0] Global Status
  IoOr16(PMIO_REG(PMIO_GLOBAL_ENABLE), PMIO_THRMTRIP_EN|PMIO_SWSMIEN|PMIO_ENSXSMI); ///PMIO_Rx2A[10][9][6] SMI SLP_EN Write/THRMTRIP Power Off/SW SMI
  IoOr16(PMIO_REG(PMIO_PM_CTL), PMIO_SCI_EN); ///PMIO_Rx04[0] SCI/SMI Select

  IoOr16(PMIO_REG(PMIO_GLOBAL_CTL), PMIO_INSMI); ///PMIO_Rx2C[8] SMI Active Status
  IoOr16(PMIO_REG(PMIO_GLOBAL_CTL), PMIO_SMIEN);///PMIO_Rx2C[0] SMI Enable
}


EFI_STATUS
FindFvContainDxeCore (
  EFI_PEI_FV_HANDLE  *FvHandle
  )
{
  EFI_STATUS            Status;
  UINTN                 Instance;
  EFI_PEI_FV_HANDLE     VolumeHandle;
  EFI_PEI_FILE_HANDLE   FileHandle;
  
  Instance = 0;
  while (TRUE) {
    Status = PeiServicesFfsFindNextVolume(Instance, &VolumeHandle);
    if (EFI_ERROR (Status)) {
      break;
    }
    FileHandle = NULL;
    Status = PeiServicesFfsFindNextFile(EFI_FV_FILETYPE_DXE_CORE, VolumeHandle, &FileHandle);
    if (!EFI_ERROR (Status)) {
      *FvHandle = VolumeHandle;
      return EFI_SUCCESS;
    }
    Instance++;
  }
  
  return EFI_NOT_FOUND;
}



VOID UpdateFvHobForRecovery()
{
  EFI_PEI_FV_HANDLE     FvHandle;
  EFI_STATUS            Status;	
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_PHYSICAL_ADDRESS  BaseAddress;
  UINT64                Length;
  BOOLEAN               FoundIt;
  
	Status = FindFvContainDxeCore(&FvHandle);
	ASSERT(!EFI_ERROR(Status));
  BaseAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)FvHandle;
  Length = ((EFI_FIRMWARE_VOLUME_HEADER*)(UINTN)FvHandle)->FvLength;

  FoundIt = FALSE;
  for (Hob.Raw = (UINT8*)GetHobList(); !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
    if (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_FV) {
      continue;
    }
    if(Hob.FirmwareVolume->BaseAddress == BaseAddress && Hob.FirmwareVolume->Length == Length){
      FoundIt = TRUE;
      break;
    }
  }
  
  if(!FoundIt){
    DEBUG((EFI_D_INFO, "BuildFvHob(%lX,%lX)\n", BaseAddress, Length));  
    BuildFvHob(BaseAddress, Length);
  }
}




typedef struct {
  ASIA_CPU_FEATURE_INIT   CpuFeatureInit;
  ASIA_CPU_CONFIGURATION  *CpuFeature;
  MTRR_SETTINGS           *Mtrrs;
  UINT8                   FinishCount;
  BOOLEAN                 IsS3;
} AP_MISSION_ARG;

VOID
ApMission (  
  IN OUT VOID  *Buffer
  )
{
  AP_MISSION_ARG  *Arg;

  Arg = (AP_MISSION_ARG*)Buffer;
  Arg->CpuFeatureInit(Arg->CpuFeature);

  if(Arg->IsS3){
    MtrrSetAllMtrrs(Arg->Mtrrs);  
  }
  
  Arg->FinishCount++;
}

VOID
DumpCpuFeature (
IN ASIA_CPU_CONFIGURATION     *CpuFeature
)
{
  DEBUG((DEBUG_INFO,  "FastString                     :%d\n", CpuFeature->FastString));   
  DEBUG((DEBUG_ERROR, "EpsEnable                      :%d\n", CpuFeature->EpsEnable));    
  DEBUG((DEBUG_INFO,  "FerrInterruptReporting         :%d\n", CpuFeature->FerrInterruptReporting));   
  DEBUG((DEBUG_INFO,  "BootWithMaxFrequence           :%d\n", CpuFeature->BootWithMaxFrequence));  
  DEBUG((DEBUG_INFO,  "NAPSNAPEnalbe                  :%d\n", CpuFeature->NAPSNAPEnalbe));   
  DEBUG((DEBUG_INFO,  "LimitCpuidMaximumValue         :%d\n", CpuFeature->LimitCpuidMaximumValue));  
  DEBUG((DEBUG_ERROR, "ProcessorCxeEnable             :%d\n", CpuFeature->ProcessorCxeEnable)); 	
  DEBUG((DEBUG_ERROR, "VtEnable                       :%d\n", CpuFeature->VtEnable));
  DEBUG((DEBUG_ERROR, "ExecuteDisableBit              :%d\n", CpuFeature->ExecuteDisableBit));
  DEBUG((DEBUG_INFO,  "MachineCheckEnable             :%d\n", CpuFeature->MachineCheckEnable));    
  DEBUG((DEBUG_INFO,  "MonitorMWaitEnable             :%d\n", CpuFeature->MonitorMWaitEnable));
  DEBUG((DEBUG_INFO,  "TPREchoEnable                  :%d\n", CpuFeature->TPREchoEnable));    
  DEBUG((DEBUG_INFO,  "Msr3AEnable                    :%d\n", CpuFeature->Msr3AEnable)); 
  DEBUG((DEBUG_INFO,  "PLVL2IoBase                    :%X\n", CpuFeature->PLVL2IoBase));     
 // DEBUG((DEBUG_INFO,  "PmonEnable                     :%d\n", CpuFeature->PmonEnable));
  DEBUG((DEBUG_INFO,  "VrmSupport                     :%d\n", CpuFeature->VrmSupport));     
  DEBUG((DEBUG_INFO,  "ActiveCpuNum                   :%d\n", CpuFeature->ActiveCpuNum)); 
  DEBUG((DEBUG_INFO,  "C5Control                      :%d\n", CpuFeature->C5Control));

  DEBUG((DEBUG_INFO,  "C5L2sizingControl              :%d\n", CpuFeature->C5L2sizingControl));
  DEBUG((DEBUG_INFO,  "CxIntFilterControl             :%d\n", CpuFeature->CxIntFilterControl));
  DEBUG((DEBUG_INFO,  "ProcessorCState             :%d\n", CpuFeature->ProcessorCState));
  DEBUG((DEBUG_INFO,  "TSCDeadLine                  :%d\n", CpuFeature->TscDeadlineModeEnable));
#ifdef ZX_SECRET_CODE
  DEBUG((DEBUG_INFO,  "KillAp                         :%d\n", CpuFeature->KillAp));
#endif

}

#ifdef ZX_SECRET_CODE
#define P_MAX                   0x00
#define MANUAL_PTx            0x01
#pragma pack (1)
typedef struct _BVID{
  UINT32  Signature;
  UINT32  Reserved4_7;
  UINT8   OpType;
  UINT8   Reserved9;
  UINT16  Reserved10_11;
  UINT16  Reserved12_15;
}BVID;
#pragma pack ()

VOID
ApPState(  
  IN OUT VOID  *Buffer
  )
{
  UINT64 Msr64;
  Msr64 = *(UINT64*)Buffer;
  AsmWriteMsr64(0x199,Msr64);
}

EFI_STATUS
EFIAPI
SetBVID(
   IN EFI_PEI_SERVICES           **PeiServices,
   EFI_PEI_MP_SERVICES_PPI       *MpSvr
   )
{
  BVID* pBVid;
  UINT64 Msr64;
  EFI_STATUS Status = EFI_SUCCESS;
  pBVid = (BVID*)PcdGet32(PcdBootConfigBase);
  //"BVID"
  if(pBVid->Signature ==0x44495642){
  	DEBUG((EFI_D_ERROR,"========Aps Set BVID==============\n"));
    if(pBVid->OpType==P_MAX){
	    Msr64 = AsmReadMsr64(0x198);
	    Msr64 = (Msr64>>32)&0xFFFF;
    }
	else if(pBVid->OpType>=MANUAL_PTx){
		if(pBVid->OpType>5){
		  return EFI_UNSUPPORTED;
	 	}
	    Msr64 = AsmReadMsr64(0x1440+pBVid->OpType-MANUAL_PTx);
		Msr64 &=0xFFFF;
	}
	else{
	   return EFI_UNSUPPORTED;
	}
   Status = MpSvr->StartupAllAPs(PeiServices, MpSvr, ApPState, TRUE, 1000000, &Msr64);
   DEBUG((EFI_D_ERROR,"========     End    ==============\n"));
   return Status;
  }
  return Status;
}
#endif
EFI_STATUS
EFIAPI
CpuMpPeiCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                 Status;
  EFI_PEI_MP_SERVICES_PPI    *MpSvr;
  //ASIA_CPU_CONFIGURATION     CpuFeatureData;
  ASIA_CPU_CONFIGURATION     *CpuFeature;  
  EFI_ASIA_CPU_PPI_PROTOCOL  *CpuPpi;
  EFI_ASIA_SB_PPI            *SbPpi;
  EFI_ASIA_NB_PPI            *NbPpi;
  ASIA_SB_CONFIGURATION      *SbCfg;
  ASIA_NB_CONFIGURATION      *NbCfg;  
  SETUP_DATA                 *SetupHob;
  AP_MISSION_ARG             MissionArg;
  EFI_BOOT_MODE              BootMode;
  PLATFORM_S3_RECORD         *S3Record; 
  //YKN-20160627 -S
  UINTN  NumberOfProcessors, NumberOfEnabledProcessors;
  EFI_PEI_HOB_POINTERS  GuidHob;
 //YKN-20160627 -E
 #ifdef ZX_TXT_SUPPORT
  ASIA_VARIABLE              *AsiaVariable;
 #endif

  Status = PeiServicesGetBootMode(&BootMode);
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesLocatePpi (
             &gEfiPeiMpServicesPpiGuid,
             0,
             NULL,
             (VOID**)&MpSvr
             );
  ASSERT_EFI_ERROR(Status);

  Status = GetAsiaPpi(&SbPpi, &NbPpi, NULL, &CpuPpi, NULL);
  ASSERT_EFI_ERROR(Status);
  SbCfg = (ASIA_SB_CONFIGURATION*)(SbPpi->SbCfg);
  NbCfg = (ASIA_NB_CONFIGURATION*)(NbPpi->NbCfg);
  SetupHob = (SETUP_DATA*)GetSetupDataHobData();
#ifdef ZX_TXT_SUPPORT
  AsiaVariable = (ASIA_VARIABLE*)GetAsiaVariableHobData();
#endif
  S3Record = (PLATFORM_S3_RECORD*)GetS3RecordTable();
#ifdef ZX_SECRET_CODE
		 {
			 EFI_STATUS 				Status;
			 EFI_CPU_CONFIG_PPI_PROTOCOL*	gPeiCpuConfigPpi;
			 Status = PeiServicesLocatePpi (
					 &gCpuConfigPpiGuid,
					 0,
					 NULL,
					 (VOID**)&gPeiCpuConfigPpi
					 );
			 if(Status==0){
				gPeiCpuConfigPpi->ConfigMsr(PeiServices,PEI0);	
			 }
		 }
#endif

 //YKN-20160627  +S
 //Update ASIA CPU activeCpuNum
  GuidHob.Raw = GetFirstGuidHob(&gAsiaCpuCfgHobGuid);
  ASSERT(GuidHob.Raw!=NULL);
  CpuFeature = (ASIA_CPU_CONFIGURATION*)(GuidHob.Guid+1); 
  MpSvr->GetNumberOfProcessors(PeiServices, 
          MpSvr, 
          &NumberOfProcessors, 
          &NumberOfEnabledProcessors);

   CpuPpi->GetCpuSupportFeature(CpuFeature);

  CpuFeature->ActiveCpuNum = (UINT8)NumberOfEnabledProcessors;
  CpuFeature->MCASupport   = SetupHob->MCASupport;
#ifdef ZX_SECRET_CODE
  //Actually CpuFeature->KillAp has no effect, just sync code.
  CpuFeature->KillAp = SetupHob->KillAp;
  //CpuFeature->PartialResetEn = SetupHob->PartialResetEn;
  
#endif
  //YKN-20160627 +E

 //YKN-20160803 +s
 //NOTE: VRM type and phases are MB dependent. 
 //Save the VrmSupport into ASIA_CPU_CFG_HOB for setup CPU sheet display.
#if defined(HX002EB0_00)|| defined(HX002EB0_11)
   CpuFeature->VrmSupport         = 0x3;     // PVID 3-Phase 
#else
  CpuFeature->VrmSupport         = 0x13;     // SVID 3-Phase
#endif
 //YKN-20160803 +e

  CpuFeature->PLVL2IoBase        = PMIO_REG(PMIO_PROCESSOR_LEVEL_2);
 
  //YKN-20160627 +s
  //Tmp disable PMON to avoid mistakenly programming MSRs. CHX001 may need no PMON.
  //CpuFeature->PmonEnable         = FALSE;
  CpuFeature->EpsEnable          = SetupHob->EpsEnable;
  CpuFeature->ProcessorCxeEnable = SetupHob->CxEEnable;
  CpuFeature->TPREchoEnable      = SetupHob->TPREcho;
  CpuFeature->ProcessorCState    = SetupHob->CpuCState;
  CpuFeature->C5Control          = SetupHob->C5Control;
  CpuFeature->C5L2sizingControl  = SetupHob->C5L2sizingControl;
  CpuFeature->CxIntFilterControl = SetupHob->CxIntFilterControl;
  CpuFeature->Msr3AEnable        = SetupHob->Msr3A;
  //YKN-20160627 +e
  CpuFeature->TscDeadlineModeEnable = SetupHob->TSC_Deadline_mode;
  CpuFeature->ExecuteDisableBit &= SetupHob->ExecuteDisable;
#ifdef ZX_TXT_SUPPORT
  CpuFeature->TxtEnable          = AsiaVariable->TXT;
#endif
  
  if(SetupHob->CRBPlatformSelection == CRB_PLATFORM_MODE_SELECTION_DESKTOP) {
    CpuFeature->ProcessorCState = 0;
    CpuFeature->C5Control          = 0;
    CpuFeature->ProcessorCxeEnable =0;
  }

DumpCpuFeature(CpuFeature);

  CpuPpi->CpuFeatureInit(CpuFeature);

  MissionArg.CpuFeatureInit = CpuPpi->CpuFeatureInit;
  MissionArg.CpuFeature     = CpuFeature;
  MissionArg.FinishCount    = 0;
  MissionArg.IsS3           = (BOOLEAN)(BootMode == BOOT_ON_S3_RESUME);
  if(MissionArg.IsS3){
    MissionArg.Mtrrs = &S3Record->MtrrTable;
  }  
  if(NumberOfEnabledProcessors >1){

  Status = MpSvr->StartupAllAPs(PeiServices, MpSvr, ApMission, TRUE, 1000000, &MissionArg);
  DEBUG((EFI_D_INFO, "StartupAllAPs(%r) %d\n", Status, MissionArg.FinishCount));
  ASSERT(!EFI_ERROR(Status));
  }
  // For S3ResumeExecuteBootScript will earlay then EndOfPeiCallback in EDKII requirement.
  // PCIe Train Gen3 with do EQ will early then load PEMCU FW.
  // "Load PEMCU FW in S3 Resume" routine put in CpuMpPeiCallback() is Temporary Workaround.
  // Moreover, the "PPI Patch Strategy" should be the formal solution to this Issue.
  // JNY Modified for PEMCU FW: This code for S3 Resume has been put FirmwareLoadCallBack();
  
  /*if(BootMode == BOOT_ON_S3_RESUME){

    DEBUG((EFI_D_INFO, __FUNCTION__"() - Load PEMCU FW in S3 Resume \n"));
    HandlePemcuFwPei(S3Record);

  	}*/
#ifdef ZX_SECRET_CODE
	 {
		 EFI_STATUS 				Status;
		 EFI_CPU_CONFIG_PPI_PROTOCOL*	gPeiCpuConfigPpi;
		 Status = PeiServicesLocatePpi (
				 &gCpuConfigPpiGuid,
				 0,
				 NULL,
				 (VOID**)&gPeiCpuConfigPpi
				 );
		 if(Status==EFI_SUCCESS){
		   gPeiCpuConfigPpi->ConfigMsr(PeiServices,PEI1);  
		 }
	 }
	 {
		Status = SetBVID(PeiServices,MpSvr);
		if(EFI_ERROR(Status)){
		  DEBUG((EFI_D_ERROR,"SetBVID_Error:(%x)\n",Status));
		  return Status;
		}
	 }
#endif
#ifdef ZX_SECRET_CODE
	// hxz-20180718 -s enable tracer and fsbc in s3
	  if(BootMode==BOOT_ON_S3_RESUME){
		 if(SetupHob->CPU_TRACER_EN||SetupHob->CPU_MASTER_FSBC_EN){
				CpuDebugPei(PeiServices,MpSvr,SetupHob);
		 }
	  }
    // hxz-20180718 -e enable tracer and fsbc in s3
#endif

  return Status;
}


EFI_STATUS
EFIAPI
AcpiWakeVectorCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  SetCacheMtrrAtS3PeiEnd(PeiServices);

  return EFI_SUCCESS;  
}


#if 0
VOID StopXhci(VOID)
{
  UINT8   CapLen;
  UINT32  UsbSts;

  if(MmioRead32(XHCI_PCI_REG(PCI_VID_REG)) == 0xFFFFFFFF){
    return;
  }

  CapLen = MmioRead8(XHCI_BAR_TMP_BASE);
  UsbSts = MmioRead32(XHCI_BAR_TMP_BASE + CapLen + 4);
  if(UsbSts & BIT0){                    // already halt
    return;
  }

  MmioAnd32(XHCI_BAR_TMP_BASE + CapLen, (UINT32)~BIT0);
  
}


VOID StopEhci(VOID)
{
  UINT8   CapLen;
  UINT32  UsbSts;

  if(MmioRead32(EHCI_PCI_REG(PCI_VID_REG)) == 0xFFFFFFFF){
    return;
  }

  CapLen = MmioRead8(EHCI_BAR_TMP_BASE);
  UsbSts = MmioRead32(EHCI_BAR_TMP_BASE + CapLen + 4);
  if(UsbSts & BIT12){                    // already halt
    return;
  }

  MmioAnd32(EHCI_BAR_TMP_BASE + CapLen, (UINT32)~BIT0);
  
}
#endif

VOID UartLegacyRegSetting(UINT16 Base)
{
	IoWrite8(Base + 0x03,0x83);
	IoWrite8(Base + 0x00,0x01);
	IoWrite8(Base + 0x03,0x03);
	IoWrite8(Base + 0x02,0x07);
}

VOID  InitUartReg()
{
	UartLegacyRegSetting(0x3F8);
	UartLegacyRegSetting(0x2F8);
}

EFI_STATUS
EFIAPI
EndOfPeiCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_BOOT_MODE               BootMode;
  EFI_STATUS                  Status;
  PLATFORM_MEMORY_INFO        *MemInfo; 
  PLATFORM_S3_RECORD          *S3Record;
  EFI_ACPI_RAM_DATA           *AcpiRam;
  

  DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));

  Status = PeiServicesGetBootMode(&BootMode);
  ASSERT_EFI_ERROR(Status); 

  MemInfo = (PLATFORM_MEMORY_INFO*)GetPlatformMemInfo(); 

  if(BootMode == BOOT_ON_S3_RESUME){

    S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
    AcpiRam  = (EFI_ACPI_RAM_DATA*)(EFI_ACPI_RAM_DATA*)GetAcpiRam();

    if(PcdGet8(PcdAcpiWakeupSrc) == WAK_TYPE_RTC){
      AcpiRam->IsRtcWake = TRUE;
    } else {
      AcpiRam->IsRtcWake = FALSE;
    }
    
    Status = AzaliaLoadVerbTable(HDAC_PCI_REG(0), gOemVerbTableData, gOemVerbTableSize);
    ASSERT_EFI_ERROR(Status);    
    
    Status = KbcCmdReadData(0xAA, NULL);
    
    PlatformSettingForS3Resume();

    IssueS3PeiEndSwSmi(PeiServices);
    
  } else {           // not S3
  
    if(BootMode == BOOT_IN_RECOVERY_MODE) {
      UpdateFvHobForRecovery();
//      StopXhci();
//      StopEhci();
    }

    PERF_START(NULL, "CACHE2", NULL, 0);    
    SetCacheMtrrAtNormalPeiEnd(PeiServices, MemInfo);
    PERF_END  (NULL, "CACHE2", NULL, 0);    
  }


#ifndef MDEPKG_NDEBUG	
  EndOfPeiDebug(BootMode);
#endif

#ifdef MDEPKG_NDEBUG
  //IVS-20180820 For Relesase BIOS,OS S3 Resume Maybe Use Uart when Grub Set "no_console_suspend"
  InitUartReg();
#endif
  return EFI_SUCCESS;
}


UINT16 
FindTolum(
  IN  ASIA_NB_CONFIGURATION    *NbCfg,   
  IN  ASIA_DRAM_CONFIGURATION  *DramCfg
  );

UINT16
GetTolum (
  IN  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi,
  IN  ASIA_NB_CONFIGURATION            *NbCfg,
  IN  ASIA_DRAM_CONFIGURATION          *DramCfg,
  IN  EFI_BOOT_MODE                    BootMode
  )
{
  PLAT_NV_INFO  NvInfo;
  UINTN         VarSize;
  EFI_STATUS    Status;	


  if(BootMode == BOOT_IN_RECOVERY_MODE){
    return 0x400;                          // 1G
  }
  
  VarSize = sizeof(PLAT_NV_INFO);
  Status  = Var2Ppi->GetVariable (
                      Var2Ppi,
                      NVINFO_TOLUM_VAR_NAME,
                      &gEfiPlatformNvInfoGuid,
                      NULL,
                      &VarSize,
                      &NvInfo
                      );
  if(EFI_ERROR(Status)){
    NvInfo.Tolum = FindTolum(NbCfg, DramCfg);
  }
  if(NvInfo.Tolum > 0xE00){NvInfo.Tolum = 0xE00;}
	
  return NvInfo.Tolum;
}



STATIC VOID ClearAcpiStatus()
{
  /// register synch with CHX001 old code base
	IoWrite16(PMIO_REG(PMIO_PM_ENABLE), 0);///PMIO Rx02[15:0] Power Management Enable
	IoWrite16(PMIO_REG(PMIO_GENERAL_PURPOSE_SCI_RESUME_ENABLE), 0);///PMIO Rx22[15:0] General Purpose SCI / RESUME Enable	  
	IoWrite16(PMIO_REG(PMIO_GENERAL_PURPOSE_SMI_RESUME_ENABLE), 0);///PMIO Rx24[15:0] General Purpose SMI / Resume Enable
	///Try to Disable GPE2 Block SCI Enable Bit
	IoWrite16(PMIO_REG(PMIO_GPI_SCI_RESUME_ENABLE), 0);///PMIO Rx56[15:0] GPI SCI/RESUME Enable
	IoWrite16(PMIO_REG(PMIO_GENERAL_PURPOSE_IO_SCI_RESUME_ENABLE_1), 0);///PMIO Rx58[15:0] General Purpose IO SCI/Resume Enable 1
	IoWrite16(PMIO_REG(PMIO_GENERAL_PURPOSE_IO_SCI_RESUME_ENABLE_3), 0);///PMIO Rx5A[15:0] General Purpose IO SCI/Resume Enable 3
	IoWrite32(PMIO_REG(PMIO_PRIMARY_ACTIVITY_DETECT_ENABLE), 0);///PMIO Rx34[31:0] Primary Activity Detect Enable
	IoWrite32(PMIO_REG(PMIO_PRIMARY_ACTIVITY_DETECT_STA), IoRead32(PMIO_REG(PMIO_PRIMARY_ACTIVITY_DETECT_STA)));///PMIO Rx30[31:0] Primary Activity Detect Status
}

#if THERMAL_IC_SUPPORT
STATIC UINT8 gTemp[] = {10,30,45,60,80}; 
EFI_STATUS
CpuFanSetting (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN ASIA_SB_CONFIGURATION   *mSbCfg
  )
{
	EFI_STATUS    Status;
	EFI_PEI_SMBUS_PPI   *pSMBusPPI;
	EFI_SMBUS_DEVICE_ADDRESS    Address;
	UINTN      Length;
	UINT8      Data8;
	UINT8      Temp;

	Temp = gTemp[mSbCfg->CpuFanStartTemperature];
	//DEBUG((EFI_D_ERROR,"[IVES] SbCfg->CpuFanStartTemperature = %d\n",mSbCfg->CpuFanStartTemperature));
	//DEBUG((EFI_D_ERROR,"[IVES] temperature = %d\n",Temp));
	Status = (*PeiServices)->LocatePpi(PeiServices,&gEfiPeiSmbusPpiGuid,0,NULL,&pSMBusPPI);
	if(EFI_ERROR(Status))
	{
		DEBUG((EFI_D_ERROR,"Locate SMBus ppi Fail.\n"));
		return Status;
	}

	Address.SmbusDeviceAddress = ADM1032_SMB_SLAVE_ADDR;
	Length = 1;
	Data8 = Temp;
	pSMBusPPI->Execute(
		(EFI_PEI_SERVICES**)PeiServices,
		pSMBusPPI,
		Address,
		0x19,
		EfiSmbusWriteByte,
		0,
		&Length,
		&Data8
	  );
	return EFI_SUCCESS;
}
#endif

////
#include "vdump_asiacfg.c"
////





STATIC 
VOID 
GpioInit (
  IN EFI_PEI_SERVICES  **PeiSrv,
  IN ASIA_SB_LIB_PPI   *SbLibPpi
  )
{
//EFI_STATUS  Status;
//
//DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));
//
//Status = SbLibPpi->SetGpo(PeiSrv, GPIO_44, TRUE);
//DEBUG((EFI_D_INFO, "GPIO_44:%r\n", Status));  
}



EFI_STATUS
EFIAPI
PlatformPeiEntry (
  IN       EFI_PEI_FILE_HANDLE      FileHandle,
  IN CONST EFI_PEI_SERVICES         **PeiServices
  )
{
  EFI_STATUS                       Status;
  EFI_ASIA_SB_PPI                  *SbPpi;
  ASIA_SB_LIB_PPI                  *SbLibPpi;  
  EFI_ASIA_NB_PPI                  *NbPpi;
  EFI_ASIA_DRAM_PPI                *DramPpi;
  EFI_ASIA_CPU_PPI_PROTOCOL        *CpuPpi;
  EFI_PEI_SERVICES                 **PeiSrv;          // EDK does not use "CONST"
  ASIA_SB_CONFIGURATION            *SbCfg;
  ASIA_NB_CONFIGURATION            *NbCfg;
  ASIA_DRAM_CONFIGURATION          *DramCfg;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi;
  EFI_BOOT_MODE                    BootMode;
  ASIA_DRAM_INFO                   AsiaDramInfo;
  ASIA_DRAM_INFO                   *DramInfo;	
  UINT32                           MemSize;	
  PLATFORM_MEMORY_INFO             *MemInfo; 
  UINTN                            Index;
  UINT32                           RegEax;
  UINT32                           BiosUpPeiMem;
  PLATFORM_S3_RECORD               *S3Record;



  DEBUG((DEBUG_INFO, "%a()\n", __FUNCTION__));

  AsmCpuid(0x1, &RegEax, NULL, NULL, NULL);
  DEBUG((DEBUG_ERROR,"CPU_FMS=%x\n",RegEax));
  if((AsmReadMsr64(0x1205)&0xFF)==0x01){
  	DEBUG((EFI_D_ERROR,"\n\n\n BSP Microcode update ok\n\n"));
	  DEBUG((EFI_D_ERROR,"\nVersion:%llx\n",AsmReadMsr64(0x8B)));
  }

  PeiSrv = (EFI_PEI_SERVICES**)PeiServices;
  GetAsiaPpi(&SbPpi, &NbPpi, &DramPpi, &CpuPpi, &SbLibPpi);
  SbCfg   = (ASIA_SB_CONFIGURATION*)(SbPpi->SbCfg);
  NbCfg   = (ASIA_NB_CONFIGURATION*)(NbPpi->NbCfg);
  DramCfg = (ASIA_DRAM_CONFIGURATION*)(DramPpi->DramCfg);


#ifndef MDEPKG_NDEBUG
  PlatformPeiEntryDebug(CpuPpi);
#endif

  //--------------------------------- Before RC ----------------------------------
  IoAnd8(PMIO_REG(PMIO_EXTEND_SMI_IO_TRAP_ENABLE), (UINT8)~PMIO_GP3TO2EN);

  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID**)&Var2Ppi
             );
  ASSERT_EFI_ERROR(Status);

  Status = PeiServicesGetBootMode(&BootMode);
  ASSERT_EFI_ERROR(Status);  

  PERF_START (NULL,"ASIACFG", NULL, 0);	
  Status = UpdateAsiaConfig(BootMode, SbCfg, NbCfg, DramCfg, Var2Ppi);
  PERF_END   (NULL,"ASIACFG", NULL, 0);	
  ASSERT_EFI_ERROR(Status);    

  #if THERMAL_IC_SUPPORT
  CpuFanSetting(PeiServices,SbCfg);
  #endif
  
  #if DRAM_Vol_IC_SUPPORT
	InitDramVoltageChip(PeiServices,DramCfg->DramVoltageControl);
  #endif

  REPORT_STATUS_CODE(EFI_PROGRESS_CODE, PEI_CAR_CPU_INIT);
  
  Status = PeiServicesInstallPpi(&gPpiInstallList[0]);
  ASSERT_EFI_ERROR (Status);  
  Status = PeiServicesNotifyPpi(&gPpiNotifyList[0]);
  ASSERT_EFI_ERROR(Status);

  ClearAcpiStatus();





//------------------------------------ RC --------------------------------------
// Patch BootMode for NbPei.CheckWarmReboot()
  if(BootMode == BOOT_ON_FLASH_UPDATE){
    Status = (*PeiServices)->SetBootMode(PeiServices, BOOT_ON_S3_RESUME);
    ASSERT_EFI_ERROR(Status);    
  }  

  REPORT_STATUS_CODE(EFI_PROGRESS_CODE, PEI_CAR_NB_INIT);
  PERF_START(NULL,"NIBM", NULL, 0);	
  Status  = NbPpi->PreMemoryInit(PeiSrv, NbPpi);
  PERF_END  (NULL,"NIBM", NULL, 0);	
  ASSERT_EFI_ERROR(Status);

  
  REPORT_STATUS_CODE(EFI_PROGRESS_CODE, PEI_CAR_SB_INIT);
  PERF_START(NULL,"SIBM", NULL, 0);
  Status  = SbPpi->PreMemoryInit(PeiSrv, SbPpi);
   PERF_END (NULL,"SIBM", NULL, 0);	
  ASSERT_EFI_ERROR(Status);

  GpioInit(PeiSrv, SbLibPpi);
 
  REPORT_STATUS_CODE(EFI_PROGRESS_CODE, PEI_MEMORY_INIT);  
  if(BootMode == BOOT_ON_S3_RESUME || BootMode == BOOT_ON_FLASH_UPDATE){
    Status  = DramPpi->DramS3Init(PeiSrv, DramPpi);
  } else {
    DramCfg->PCIMemoryStart = GetTolum(Var2Ppi, NbCfg, DramCfg, BootMode);
    DEBUG((EFI_D_INFO, "PCIMemoryStart:%X\n", DramCfg->PCIMemoryStart<<20));
    PERF_START (NULL,"MRC", NULL, 0);			
    Status = DramPpi->DramNormalInit(PeiSrv, DramPpi);
    PERF_END   (NULL,"MRC", NULL, 0);		
    if(Status == EFI_NOT_FOUND){
      while(1){
        REPORT_STATUS_CODE(EFI_ERROR_CODE, PEI_MEMORY_NOT_DETECTED);
        MicroSecondDelay(2000000);
      }		
    }
  }
  ASSERT_EFI_ERROR(Status);

  if(BootMode == BOOT_ON_FLASH_UPDATE){
    Status = (*PeiServices)->SetBootMode(PeiServices, BOOT_ON_FLASH_UPDATE);
    ASSERT_EFI_ERROR(Status);   
  }  




//-------------------------------- AFTER RC ------------------------------------
  ClearAcpiStatus();   // some status cannot be cleared before, clear it again.

  MmioAnd8(LPC_PCI_REG(D17F0_LPC_ROM_MEM_ADR_RANGE), (UINT8)~0x0F);   // LPC FF000000h ~ FF7FFFFFh not selected, same SOC IRS.

  DramInfo = &AsiaDramInfo;
  Status = DramPpi->DramGetInfo(PeiSrv, DramPpi, DramInfo);
  ASSERT_EFI_ERROR(Status);

  DEBUG((EFI_D_INFO, "sizeof(PLATFORM_MEMORY_INFO):%d\n", sizeof(PLATFORM_MEMORY_INFO)));
  DEBUG((EFI_D_INFO, "sizeof(PLATFORM_S3_RECORD)  :%d\n", sizeof(PLATFORM_S3_RECORD)));
  
  MemInfo = BuildGuidHob(&gEfiPlatformMemInfoGuid, sizeof(PLATFORM_MEMORY_INFO));
  ASSERT(MemInfo != NULL);
  ZeroMem(MemInfo, sizeof(PLATFORM_MEMORY_INFO));

// +----+ 64G
// |----| PCI64
// |----|
// | FB |
// |----| 
// +----+ 4G
// |____| PCI
// |    |       <-- TOLUM
// |____| TSEG
// |____| S3 Data Record (4K)
// |    | S3 Memory
// |----| <-- valid memory top
// |PMM |                    
// |----| 
// |    |
// |    | Normal
// |____|
// |    | 1M
// +----+

  MemInfo->Tolum      = (UINT32)DramInfo->PciStartAddress<<20;      // TOLUM
  MemInfo->VgaBufSize = (UINT32)DramInfo->UMASize<<20;
  MemInfo->TSegSize   = (UINT32)DramInfo->SmmTSegSize<<20;

  
  MemInfo->BiosLowMem   = MemInfo->Tolum;                            // for WB cache.
  MemInfo->TSegAddr     = MemInfo->BiosLowMem - MemInfo->TSegSize;
#ifdef ZX_TXT_SUPPORT
  MemInfo->DprSize      = (UINT32)DramInfo->DPRSize<<20;
  MemInfo->DprAddr      = MemInfo->TSegAddr - MemInfo->DprSize;
  MemInfo->S3DataRecord = MemInfo->DprAddr - S3_DATA_RECORD_SIZE;
#else
  MemInfo->S3DataRecord = MemInfo->TSegAddr - S3_DATA_RECORD_SIZE;
#endif
  MemInfo->S3MemoryAddr = MemInfo->S3DataRecord - S3_PEI_MEMORY_SIZE;
  MemInfo->LowMemSize   = MemInfo->S3MemoryAddr;	

  if(DramCfg->ScanIOTiming){
    DEBUG((DEBUG_ERROR,"ScanIOTiming Enable Tolum = 0x40000000 LGE DEBUG\n"));
    MemInfo->BiosLowMem = 0x40000000;
  }  
  MemInfo->PhyAddrBits = 36;
  AsmCpuid(0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
    MemInfo->PhyAddrBits = (UINT8)RegEax;
  }
  DEBUG((DEBUG_INFO, "PhyAddrBits: %d\n", MemInfo->PhyAddrBits));

  if(DramCfg->Above4GEnable) {
    MemInfo->Pci64Size = PCI64_MMIO_SIZE;
    MemInfo->Pci64Base = LShiftU64(1, (40-DramCfg->Above4GLocation)) - PCI64_MMIO_SIZE;
  } else {
    MemInfo->Pci64Base = MemInfo->Pci64Size = 0;
  }

  ASSERT(sizeof(PLATFORM_S3_RECORD) <= S3_DATA_RECORD_SIZE);
  PcdSet32(PcdS3RecordAddr, MemInfo->S3DataRecord);


  DEBUG((DEBUG_INFO,  "Tolum:%X\n", MemInfo->Tolum));
  DEBUG((DEBUG_INFO,  "Fake(%X,%X)\n", DramInfo->FakeBegin, DramInfo->FakeLength));	
  DEBUG((DEBUG_ERROR, "LowMemSize:%X, TSegAddr:%X\n", MemInfo->LowMemSize, MemInfo->TSegAddr));
  DEBUG((DEBUG_ERROR, "VgaBufSize:%X\n", MemInfo->VgaBufSize));
  DEBUG((DEBUG_ERROR, "S3PeiMem(%X,%X)\n", MemInfo->S3MemoryAddr, S3_PEI_MEMORY_SIZE));


  S3Record = (PLATFORM_S3_RECORD*)(UINTN)(MemInfo->S3DataRecord);
  if(BootMode != BOOT_ON_S3_RESUME && BootMode != BOOT_ON_FLASH_UPDATE ){          // normal boot
    ZeroMem(S3Record,sizeof(PLATFORM_S3_RECORD));
    MemSize = 0;
    for(Index=0;Index<ASIA_MAX_RANKS;Index++){
      MemSize += DramInfo->RankInfo[Index].RankSize;
    }
    MemInfo->PhyMemSize = LShiftU64(MemSize, 20);  // MB --> B

    S3Record->Signature = PLAT_S3_RECORD_SIGNATURE;
    ZeroMem(&S3Record->AcpiRam, sizeof(S3Record->AcpiRam));
    S3Record->AcpiRam.Signature = ACPI_RAM_DATA_SIGNATURE;
    S3Record->AcpiRam.FlashSize = PcdGet32(PcdFlashAreaSize);
    S3Record->AcpiRam.PciBase   = MemInfo->Tolum;
    S3Record->AcpiRam.PciLength = PCI_MMIO_TOP_ADDRESS - S3Record->AcpiRam.PciBase;	
    S3Record->AcpiRam.AcpiWakeState = 4;   
    S3Record->S3Sleep = 0;
  }
  S3Record->DebugDataIndex = 0;

 
  if(BootMode == BOOT_ON_S3_RESUME){
    PeiServicesInstallPeiMemory(MemInfo->S3MemoryAddr, S3_PEI_MEMORY_SIZE);
    
  } else if (BootMode == BOOT_ON_FLASH_UPDATE){
    HandleCapsuleBeforeMemInstall(PeiServices, MemInfo, &BiosUpPeiMem);
    PeiServicesInstallPeiMemory(BiosUpPeiMem, PEI_BU_MEMORY_SIZE);
    
  } else if(BootMode == BOOT_IN_RECOVERY_MODE){
    PeiServicesInstallPeiMemory(PEI_MEMORY_BASE, MemInfo->LowMemSize/2);
    
  } else {
    PeiServicesInstallPeiMemory(PEI_MEMORY_BASE, PEI_MEMORY_SIZE);
  }

  return Status;
}


