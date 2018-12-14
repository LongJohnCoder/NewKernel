

#include "PlatformDxe.h"
#include <Guid/Acpi.h>
#include <Guid/AcpiS3Context.h>
#include <IndustryStandard/Pci.h>
#include <Library/SerialPortLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/LockBoxLib.h>
#include <Library/MtrrLib.h>
#include <Protocol/ExitPmAuth.h>
#include <Protocol/VariableWrite.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SmmCommunication.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/NvMediaAccess.h>
#include <Protocol/PlatHostInfoProtocol.h>
#include <CHX002Reg.h>
#include <Guid/EventLegacyBios.h>
#include <Library/TcgPhysicalPresenceLib.h>
#include <Library/Tcg2PhysicalPresenceLib.h>
#include <library/ByoCommLib.h>
#ifdef ZX_SECRET_CODE
#include <Protocol/MpService.h>
#endif
#ifdef ZX_SECRET_CODE
#include <Protocol/MpConfig.h>
#endif
#ifdef IOE_EXIST
#include <EepromInfo.h>
#endif

VOID
SmbiosCallback (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  );

#ifdef ZX_SECRET_CODE

	VOID EFIAPI CpuDebug();  // hxz-20171012 add
	extern EFI_FSBC_DUMP_PROTOCOL	  mFsbcDump;
#endif
extern EFI_GUID gBdsAllDriversConnectedProtocolGuid;
extern EFI_GUID gEfiSetupEnterGuid;

EFI_ASIA_CPU_PROTOCOL *gPtAsiaCpu = NULL;
EFI_ASIA_SB_PROTOCOL  *gPtAsiaSb  = NULL;
EFI_ASIA_NB_PROTOCOL  *gPtAsiaNb  = NULL;
ASIA_NB_CONFIGURATION *gAsiaNbCfg = NULL;
ASIA_SB_CONFIGURATION *gAsiaSbCfg = NULL;
CONST SETUP_DATA      *gSetupData;
VOID                  *gResvdPage = NULL;
BOOLEAN               gAfterReadyToBoot = FALSE;


VOID
SaveBridgeRegistersForS3 (
  IN  UINTN  BaseAddr
)  
{
  EFI_STATUS  Status;

  if(MmioRead16(BaseAddr + PCI_VID_REG) == 0xFFFF){
    return;
  }
  
  Status = S3BootScriptSaveMemWrite (
             S3BootScriptWidthUint32,
             BaseAddr + PCI_PBN_REG,
             7,
             (VOID*)(UINTN)(BaseAddr + PCI_PBN_REG)
             );
  ASSERT_EFI_ERROR(Status);

  Status = S3BootScriptSaveMemWrite (
             S3BootScriptWidthUint32,
             BaseAddr + PCI_INT_LINE_REG,
             1,
             (VOID*)(UINTN)(BaseAddr + PCI_INT_LINE_REG)
             );
  ASSERT_EFI_ERROR(Status);   
  
  Status = S3BootScriptSaveMemWrite (
             S3BootScriptWidthUint32,
             BaseAddr + PCI_CMD_REG,
             1,
             (VOID*)(UINTN)(BaseAddr + PCI_CMD_REG)
             );
  ASSERT_EFI_ERROR(Status);  
}

#ifdef IOE_EXIST
VOID
EFIAPI
EepromInitCallBack (
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
	VOID		*Interface;
	EFI_STATUS	Status;
	UINT16 		PciVID, PciDID;
	UINTN					  HandleCount;
	EFI_HANDLE				  *HandleBuffer;
	UINTN					  Index;
    EFI_PCI_IO_PROTOCOL       *PciIo;
	UINT32		temp32;
	UINTN Size = sizeof(EEPROM_INFO_SAVE);
	EEPROM_INFO_SAVE EepromSave;	
	UINT8		temp8;

 	DEBUG((EFI_D_INFO, "EepromInitCallBack!!! \n"));		

	Status = gBS->LocateProtocol(&gBdsAllDriversConnectedProtocolGuid, NULL, &Interface);
	if (EFI_ERROR (Status)) {
	  return;
	}
	gBS->CloseEvent(Event);

    //
    // Find the IOE GNIC controller
    //
    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiPciIoProtocolGuid,
                    NULL,
                    &HandleCount,
                    &HandleBuffer
                    );
	ASSERT_EFI_ERROR(Status);	

    for (Index = 0; Index < HandleCount; Index++) 
	{
		Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, &PciIo);
		if (!EFI_ERROR (Status))
		{
	        // Step1.
	        // Check for IOE GNIC
	        //
	        Status = PciIo->Pci.Read (
	                          PciIo,
	                          EfiPciIoWidthUint16,
	                          PCI_VID_REG,
	                          1,
	                          &PciVID
	                          );
	        Status = PciIo->Pci.Read (
	                          PciIo,
	                          EfiPciIoWidthUint16,
	                          PCI_DID_REG,
	                          1,
	                          &PciDID
	                          );
			
	        if (!EFI_ERROR (Status) && (PciVID==0x1D17) && (PciDID==0x9180)) 
			{
				DEBUG((EFI_D_INFO, "IOE GNIC Found!!! \n", PciVID, PciDID));	

				// Step2.
				// Check for VEEPROM configuration
				//
				PciIo->Mem.Read (
				           PciIo,
				           EfiPciIoWidthUint8,
				           1,
				           MMIO_Rx5D8_REG,
				           1,
				           &temp8
				           );		  

				DEBUG((EFI_D_INFO, "IOE GNIC Mem.Read 0x5D8: %02x \n", temp8));		  
				  
				if((temp8&0x02))
				{
					// Step3.
					// Check for VEEPROM shadow
					//
					Status = PciIo->Pci.Read (
									  PciIo,
									  EfiPciIoWidthUint8,
									  PCI_Rx5C_REG+3,
									  1,
									  &temp8
									  );

					if((temp8&0x03)!=0x03)
					{
						//need shadow process
						Status = gRT->GetVariable(
										  EEPROM_INFO_VARIABLE_NAME, 
										  &gEepromInfoGuid, 
										  NULL, 
										  &Size, 
										  &EepromSave
										  );
						if(Status){
							DEBUG((EFI_D_ERROR, "No EEPROM Variable found!!! Status: %x \n", Status));
						}
						else
						{			
							//case of VEEPROM Enable and EEPROM Variable found!!
							DEBUG((EFI_D_ERROR, "EEPROM Variable found!!! Status: %x \n", Status));
							temp32 = 0;
							
							//enable SEEPR first Bit[24]
							temp32 |= BIT24;
							PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, PCI_Rx5C_REG, 1, &temp32);				

							//shadow VEEPROM
							for(Index=0;Index<EEMAX;Index++)
							{
								DEBUG((EFI_D_ERROR, "EepromInfoSave.Index[0]: %4x\n", EepromSave.Index[Index]));
								temp32 = EepromSave.Index[Index] | (UINT32) Index<<16;
								DEBUG((EFI_D_ERROR, "temp32 write to 0x58: %8x\n", temp32));					
								PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, PCI_Rx58_REG, 1, &temp32);

								/*
								temp32 = (UINT32) (Index<<16)|BIT27;
								PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, PCI_Rx5C_REG, 1, &temp32);
								Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, PCI_Rx5C_REG, 1, &temp32);
								DEBUG((EFI_D_ERROR, "Read from to 0x5C: %8x\n", temp32));										
								*/
							}
						

							PciIo->Mem.Read (
							           PciIo,
							           EfiPciIoWidthUint8,
							           1,
							           MMIO_Rx93_REG,
							           1,
							           &temp8
							           );		  			
							DEBUG((EFI_D_ERROR, "temp8 read from MMIO 0x93: %02x\n", temp8));
							
							temp8 |= BIT5;
							DEBUG((EFI_D_ERROR, "temp8 write to MMIO 0x93: %02x\n", temp8)); 			
							PciIo->Mem.Write(
							           PciIo,
							           EfiPciIoWidthUint8,
							           1,
							           MMIO_Rx93_REG,
							           1,
							           &temp8
							           );		  														
						
							//Read SEELD Bit[25], RELOAD[5]
							do{
								Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, PCI_Rx5C_REG, 1, &temp32);					
								DEBUG((EFI_D_ERROR, "Read from 0x5C: %8x\n", temp32));					

								PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, 1, MMIO_Rx93_REG, 1, &temp8);																
								DEBUG((EFI_D_ERROR, "temp8 read from 0x93: %02x\n", temp8)); 						
							}while(!(temp32&BIT25)||(temp8&BIT5));	
						}
					}				
		  		} 		  
	            break;
			}
		}
    }
	
    FreePool (HandleBuffer);	 

}
#endif


VOID
ExitPmAuthCallBack (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  VOID              *Interface;
  EFI_STATUS        Status;
 // UINT32            Data32And, Data32Or;
  UINT32            PciId;
  UINT32            Data32;
  

  Status = gBS->LocateProtocol(&gExitPmAuthProtocolGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }
  gBS->CloseEvent(Event);
  DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));  

  
  Status = gPtAsiaSb->PreBootInit(gPtAsiaSb);
  ASSERT_EFI_ERROR(Status);
 
  Status = gPtAsiaNb->PreBootInit(gPtAsiaNb);
  ASSERT_EFI_ERROR(Status);

  Status = S3BootScriptSaveMemWrite (
             S3BootScriptWidthUint32,
             HIF_PCI_REG(PAGE_C_SHADOW_CTRL_REG),
             1,
             (VOID*)(UINTN)HIF_PCI_REG(PAGE_C_SHADOW_CTRL_REG)
             ); 
  ASSERT_EFI_ERROR(Status);    

  if(MmioRead16(IGDAC_PCI_REG(PCI_VID_REG))!=0xFFFF){
    Status = S3BootScriptSaveMemWrite (
               S3BootScriptWidthUint32,
               IGDAC_PCI_REG(PCI_BAR0_REG),
               1,
               (VOID*)(UINTN)IGDAC_PCI_REG(PCI_BAR0_REG)
               );
    ASSERT_EFI_ERROR(Status);
  }

// update IGD SSID.
  PciId = MmioRead32(IGD_PCI_REG(PCI_VID_REG));
  if((UINT16)PciId!=0xFFFF){
    Data32 = MmioRead32(IGD_PCI_REG(PCI_BAR0_REG)) & 0xFFFFFFF0;
    MmioWrite32(Data32 + IGD_SVID_MMIO_OFFSET, PciId);
    MmioWrite32(Data32 + IGDAC_SVID_MMIO_OFFSET, MmioRead32(IGDAC_PCI_REG(PCI_VID_REG)));

    Status = S3BootScriptSaveMemWrite (
               S3BootScriptWidthUint32,
               IGD_PCI_REG(PCI_BAR0_REG),
               3,
               (VOID*)(UINTN)IGD_PCI_REG(PCI_BAR0_REG)
               );
    ASSERT_EFI_ERROR(Status);

    Status = S3BootScriptSaveMemWrite (
               S3BootScriptWidthUint8,
               IGD_PCI_REG(PCI_CMD_REG),
               1,
               (VOID*)(UINTN)IGD_PCI_REG(PCI_CMD_REG)
               );
    ASSERT_EFI_ERROR(Status);

    Status = S3BootScriptSaveMemWrite (
               S3BootScriptWidthUint32,
               Data32 + IGD_SVID_MMIO_OFFSET,
               1,
               (VOID*)(UINTN)(Data32 + IGD_SVID_MMIO_OFFSET)
               );
    ASSERT_EFI_ERROR(Status);

    Status = S3BootScriptSaveMemWrite (
               S3BootScriptWidthUint32,
               Data32 + IGDAC_SVID_MMIO_OFFSET,
               1,
               (VOID*)(UINTN)(Data32 + IGDAC_SVID_MMIO_OFFSET)
               );
    ASSERT_EFI_ERROR(Status);     
  }  
  
// VISA has saved azalia info in InitAzaliaAudio() for S3. 
//LNA-20161031-S
// Save Bridge Registers
  SaveBridgeRegistersForS3(PE0_PCI_REG(PCI_VID_REG));  
  SaveBridgeRegistersForS3(PE1_PCI_REG(PCI_VID_REG)); 
  SaveBridgeRegistersForS3(PE2_PCI_REG(PCI_VID_REG));  
  SaveBridgeRegistersForS3(PE3_PCI_REG(PCI_VID_REG)); 
  SaveBridgeRegistersForS3(PEG0_PCI_REG(PCI_VID_REG)); 	
  SaveBridgeRegistersForS3(PEG1_PCI_REG(PCI_VID_REG)); 	
  SaveBridgeRegistersForS3(PEG2_PCI_REG(PCI_VID_REG)); 	
  SaveBridgeRegistersForS3(PEG3_PCI_REG(PCI_VID_REG)); 	
//LNA-20161031-E


}



STATIC PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformIgdDevice = {
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


UINT32 GetAcpiTableSmmCommAddr(VOID)
{
  EFI_ACPI_4_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Xsdt;
  UINTN                                         Index;
  UINT64                                        *XTableAddress;
  UINTN                                         TableCount;
  EFI_STATUS                                    Status;	
  EFI_SMM_COMMUNICATION_ACPI_TABLE              *SmmAcpi;
	

  Status = EfiGetSystemConfigurationTable(&gEfiAcpiTableGuid, &Rsdp);
  ASSERT(!EFI_ERROR(Status));	
  if (EFI_ERROR(Status)) {
    return 0;
  }

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER*)(UINTN)Rsdp->XsdtAddress;
  TableCount = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT64);
  XTableAddress = (UINT64 *)(Xsdt + 1);
  for (Index = 0; Index < TableCount; Index++) {
    SmmAcpi = (EFI_SMM_COMMUNICATION_ACPI_TABLE *)(UINTN)XTableAddress[Index];
    if ((SmmAcpi->UefiAcpiDataTable.Header.Signature == EFI_ACPI_4_0_UEFI_ACPI_DATA_TABLE_SIGNATURE) &&
        (SmmAcpi->UefiAcpiDataTable.Header.Length == sizeof (EFI_SMM_COMMUNICATION_ACPI_TABLE)) &&
        CompareGuid (&(SmmAcpi->UefiAcpiDataTable.Identifier), &gEfiSmmCommunicationProtocolGuid) ) {
      return (UINT32)(UINTN)SmmAcpi;
    }
  }

  ASSERT(FALSE);
  return 0;
}


#ifndef MDEPKG_NDEBUG	
VOID DumpSysSetting()
{
  PLATFORM_S3_RECORD    *S3Record;
  UINT32                Index;
  

  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
 
  DEBUG((EFI_D_INFO, "XhciMcuFw_Lo :%X\n", S3Record->XhciMcuFw_Lo));
  DEBUG((EFI_D_INFO, "XhciMcuFw_Hi :%X\n", S3Record->XhciMcuFw_Hi));
  DEBUG((EFI_D_INFO, "XhciMcuFwSize:%X\n", S3Record->XhciMcuFwSize));
  DEBUG((EFI_D_INFO, "S3StackBase  :%X\n", S3Record->S3StackBase)); 
  DEBUG((EFI_D_INFO, "S3StackSize  :%X\n", S3Record->S3StackSize));   
  DEBUG((EFI_D_INFO, "S3Cr3        :%X\n", S3Record->S3Cr3));
  DEBUG((EFI_D_INFO, "ScatAddr     :%X\n", S3Record->ScatAddr));
  DEBUG((EFI_D_INFO, "MtrrTable    :%p\n", &S3Record->MtrrTable));   
  DEBUG((EFI_D_INFO, "CpuCount     :%X\n", S3Record->CpuCount)); 
  DEBUG((EFI_D_INFO, "SmmUcData    :%X\n", S3Record->SmmUcData));
  DEBUG((EFI_D_INFO, "SmmUcDataSize:%X\n", S3Record->SmmUcDataSize));
  DEBUG((EFI_D_INFO, "SmrrBase     :%X\n", S3Record->SmrrBase));
  DEBUG((EFI_D_INFO, "SmrrSize     :%X\n", S3Record->SmrrSize));
  DEBUG((EFI_D_INFO, "CpuApVector  :%X\n", S3Record->CpuApVector));

  for(Index=0;Index<S3Record->CpuCount;Index++){
    DEBUG((EFI_D_INFO, "CPU[%d] ID:%X BASE:%X\n", Index, \
      S3Record->CpuApicId[Index], S3Record->CpuSmBase[Index]));
  }  
}
#endif



EFI_STATUS
ZX_UpdateWdrt (
  )
{
  EFI_STATUS                                      Status;
  UINT64              	                          Address;
  UINT32                                          WdrtBaseAddress;
  UINT8                                           TempB;
  UINT16                                          TempW;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL                 *PciRootBridgeIo;
  ////
  UINTN      VariableSize;
  SETUP_DATA BackupSetupData;

  ////
  Status = EFI_SUCCESS;

  ////
  VariableSize = sizeof (SETUP_DATA);
  Status = gRT->GetVariable (
                  PLATFORM_SETUP_VARIABLE_NAME,
                  &gPlatformSetupVariableGuid,
                  NULL,
                  &VariableSize,
                  &BackupSetupData
                  );
  ASSERT_EFI_ERROR (Status);  
  //// gEfiPciRootBridgeIoProtocolGuid
  Status = gBS->LocateProtocol (
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  &PciRootBridgeIo
                  );
  ASSERT_EFI_ERROR (Status);    
  //
  // If WDRT is disabled in setup, don't publish the table.
  //
  if (!BackupSetupData.WatchDogTimer) {
    DEBUG((EFI_D_ERROR,"\n D17F0_PMU WatchDogTimer is disable now \n"));
    return EFI_UNSUPPORTED;
  }

  //program Base register
  WdrtBaseAddress = 0xFEB41000; // Ref ASIAConfig.h
  Address = (CHX002_BUSC|D17F0_PMU_WATCHDOG_TIMER_MEM_BASE);
  Status = PciRootBridgeIo->Pci.Write (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint32,
                                  Address,
                                  1,
                                  &WdrtBaseAddress
                                  );

  //SCRIPT_PCI_CFG_WRITE (
  //  EFI_ACPI_S3_RESUME_SCRIPT_TABLE,
  //  EfiBootScriptWidthUint32,
  //  Address,
  //  1,
  //  &WdrtBaseAddress
  //  );

  //program Enable bits
  Address = (CHX002_BUSC|D17F0_PMU_WATCHDOG_TIMER_CTL_C3_LATENCY_CTL);
  Status = PciRootBridgeIo->Pci.Read (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint8,
                                  Address,
                                  1,
                                  &TempB
                                  );
  TempB |= 0x03;
  Status = PciRootBridgeIo->Pci.Write (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint8,
                                  Address,
                                  1,
                                  &TempB
                                  );

  //SCRIPT_PCI_CFG_WRITE (
  //  EFI_ACPI_S3_RESUME_SCRIPT_TABLE,
  //  EfiBootScriptWidthUint8,
  //  Address,
  //  1,
  //  &TempB
  //  );

  //program Action and Run/Stop
  TempB = 0;
  if (BackupSetupData.WatchDogTimerAction) TempB |=0x04;
  if (BackupSetupData.WatchDogTimerRunStop) TempB |=0x01;
  Address = WdrtBaseAddress;
  Status = PciRootBridgeIo->Mem.Write (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint8,
                                  Address,
                                  1,
                                  &TempB
                                  );

  //program Count
  switch(BackupSetupData.WatchDogTimerCount) {
    case 0:
      TempW = 72;
      break;
    case 1:
      TempW = 389;
      break;
    case 2:
      TempW = 706;
      break;
    case 3:
      TempW = 1023;
      break;
  }

  Address = WdrtBaseAddress + 4;
  Status = PciRootBridgeIo->Mem.Write (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint16,
                                  Address,
                                  1,
                                  &TempW
                                  );

  //program Trigger
  Address = WdrtBaseAddress;
  Status = PciRootBridgeIo->Mem.Read(
                                  PciRootBridgeIo,
                                  EfiPciWidthUint8,
                                  Address,
                                  1,
                                  &TempB
                                  );
  TempB &= 0xFD;
  TempB |= 0x80;
  Status = PciRootBridgeIo->Mem.Write(
                                  PciRootBridgeIo,
                                  EfiPciWidthUint8,
                                  Address,
                                  1,
                                  &TempB
                                  );

  return Status;
}

//// 2016-10-23+E

// DBZ-2017110601, +S
EFI_STATUS
SetShellStartupDelayVar(VOID)
{
	#define EDK_SHELL_ENVIRONMENT_VARIABLE_ID \
	  {0x47c7b224, 0xc42a, 0x11d2, 0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b}

	EFI_STATUS  Status;
	UINTN       ValSize;
	CHAR16      String[2] = {0x32, 0x00}; // String L"2"
	EFI_GUID    MyGuid = EDK_SHELL_ENVIRONMENT_VARIABLE_ID;

	ValSize = sizeof(String);
	Status = gRT->SetVariable(
		L"StartupDelay",
		&MyGuid,
		EFI_VARIABLE_BOOTSERVICE_ACCESS,
		ValSize,
		String
	);
	
	return Status;
}
// DBZ-2017110601, +E


VOID
EFIAPI
PlatOnReadyToBoot (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  PLATFORM_MEMORY_INFO  *MemInfo;
  EFI_STATUS            Status;
  PLATFORM_S3_RECORD    *S3Record;  
  ACPI_S3_CONTEXT       *S3Ctx;	
  UINTN                 S3CtxSize;
  UINT64                HighMemSize = 0;
  EFI_PHYSICAL_ADDRESS  PmmMemory;
    
  
  DEBUG((EFI_D_INFO, __FUNCTION__"()\n")); 
  gBS->CloseEvent(Event);

  MemInfo = (PLATFORM_MEMORY_INFO*)GetPlatformMemInfo(); 
  
  if(MemInfo->PhyMemSize > (UINT64)MemInfo->Tolum){
	  HighMemSize = MemInfo->PhyMemSize - MemInfo->Tolum - MemInfo->VgaBufSize;
    if(MemInfo->Pci64Base &&  SIZE_4GB + HighMemSize > MemInfo->Pci64Base){
      HighMemSize = MemInfo->Pci64Base - SIZE_4GB;
    }
  }
	if(HighMemSize > 0) {
    DEBUG((EFI_D_INFO, "HighMemSize:%lX\n", HighMemSize));
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeSystemMemory,
                    SIZE_4GB,
                    HighMemSize,
                    EFI_MEMORY_UC|EFI_MEMORY_WC|EFI_MEMORY_WT|EFI_MEMORY_WB
                    );
    ASSERT_EFI_ERROR(Status);  
	}	else {
		DEBUG((EFI_D_INFO, "There is no physical memory above 4G\n")); 
	}

  if(MemInfo->PmmBase && MemInfo->PmmSize){
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeSystemMemory,
                    MemInfo->PmmBase,
                    MemInfo->PmmSize,
                    EFI_MEMORY_UC|EFI_MEMORY_WC|EFI_MEMORY_WT|EFI_MEMORY_WB
                    );
    ASSERT_EFI_ERROR(Status);

    PmmMemory = MemInfo->PmmBase;
    Status = gBS->AllocatePages(
                    AllocateAddress,
                    EfiBootServicesData,
                    EFI_SIZE_TO_PAGES(MemInfo->PmmSize),
                    &PmmMemory
                    );
    ASSERT((!EFI_ERROR(Status)) && (PmmMemory == MemInfo->PmmBase));
  }

  
  Status = AzaliaLoadVerbTable(HDAC_PCI_REG(0), gOemVerbTableData, gOemVerbTableSize);
  ASSERT_EFI_ERROR(Status);
  
  IoWrite16(PMIO_REG(PMIO_PM_STA), PMIO_TMR_STS|PMIO_BM_STS);///PMIO_Rx00[4][0] Bus Master Status/ACPI Timer Carry Status
  IoWrite16(PMIO_REG(PMIO_GLOBAL_STA), PMIO_PINT1_STS);  ///PMIO_Rx28[7] Primary IRQ/INIT/NMI/SMI Resume Status

  S3Record = (PLATFORM_S3_RECORD*)GetS3RecordTable();
	S3Record->ScatAddr = GetAcpiTableSmmCommAddr();
  S3Record->CpuApVector = 0x9F000;

  S3CtxSize = sizeof(ACPI_S3_CONTEXT);
  S3Ctx = (ACPI_S3_CONTEXT*)gResvdPage;
  ASSERT(S3Ctx != NULL);
  DEBUG((EFI_D_INFO, "S3Ctx:%X\n", S3Ctx));
  RestoreLockBox(&gEfiAcpiS3ContextGuid, S3Ctx, &S3CtxSize);	
  S3Record->S3StackBase = (UINT32)S3Ctx->BootScriptStackBase;
  S3Record->S3StackSize = (UINT32)S3Ctx->BootScriptStackSize;
  S3Record->S3Cr3       = (UINT32)S3Ctx->S3NvsPageTableAddress;

  MtrrGetAllMtrrs(&S3Record->MtrrTable);

  #if 0
  // Set variable "StartupDelay" for shell startup.
  SetShellStartupDelayVar();
  #endif

  ZX_UpdateWdrt();

#ifndef MDEPKG_NDEBUG	  
  DumpSysSetting();
#endif 

#ifdef ZX_SECRET_CODE
  DEBUG((EFI_D_ERROR,"+++++Start Set MSR in on ready to Boot\n"));
  
  {  
	  EFI_STATUS Status;
	  EFI_MP_CONFIG_PROTOCOL*	  MpConfig;
	  Status = gBS->LocateProtocol (&gEfiCpuMpConfigProtocolGuid, NULL,(VOID**)&MpConfig);
	  if(Status==0){
		 MpConfig->ConfigMsr(RDB);
	  }
   }
  DEBUG((EFI_D_ERROR,"+++++After Set MSR in on ready to Boot\n"));
#endif

  gAfterReadyToBoot = TRUE;
}



VOID
EFIAPI
AllDriversConnectedCallBack (
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
  VOID        *Interface;
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol(&gBdsAllDriversConnectedProtocolGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }
  gBS->CloseEvent(Event);

  UpdatePs2State();
  InstallAcpiTableDmar();
}


VOID
EFIAPI
HiiConfigAccessCallback (
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
  EFI_STATUS                      Status;  
	EFI_HANDLE                      *HandleBuffer = NULL;
  UINTN                           HandleCount;
  UINTN                           Index;
  UINTN                           i;
  EFI_DEVICE_PATH_PROTOCOL        *DevPath; 
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *CfgAcc;
  PLAT_HOST_INFO_PROTOCOL         *ptHostInfo;
  BOOLEAN                         IsMyLan;


//DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  if(!gAfterReadyToBoot){
    return;
  }

  Status = gBS->LocateProtocol(&gPlatHostInfoProtocolGuid, NULL, &ptHostInfo);
  ASSERT(!EFI_ERROR(Status));

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiHiiConfigAccessProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
//DEBUG((EFI_D_INFO, "HCACB HandleCount:%d\n", HandleCount));  
  for(Index=0; Index<HandleCount; Index++){
    Status = gBS->HandleProtocol(
                    HandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    &DevPath
                    );
    if(EFI_ERROR(Status)){
//    DEBUG((EFI_D_INFO, "HCACB L%d %r\n", __LINE__, Status)); 
      continue;
    }

//  ShowDevicePathDxe(gBS, DevPath);

    IsMyLan = FALSE;
    for(i=0;i<ptHostInfo->HostCount;i++){
      if(ptHostInfo->HostList[i].HostType == PLATFORM_HOST_LAN){
        if(CompareMem(ptHostInfo->HostList[i].Dp, DevPath, GetDevicePathSize(DevPath)) == 0){
//        DEBUG((EFI_D_INFO, "HCACB L%d %d\n", __LINE__, i)); 
          IsMyLan = TRUE;
          break;
        }
      }
    }
    if(!IsMyLan){
      continue;
    }


    Status = gBS->HandleProtocol(
                    HandleBuffer[Index],
                    &gEfiHiiConfigAccessProtocolGuid,
                    (VOID**)&CfgAcc
                    );
    
    Status = gBS->UninstallProtocolInterface(
                    HandleBuffer[Index], 
                    &gEfiHiiConfigAccessProtocolGuid,
                    (VOID*)CfgAcc
                    );

    DEBUG((EFI_D_INFO, "HCACB L%d %r\n", __LINE__, Status)); 
    
    break;
    
  }

  if(HandleBuffer != NULL){
    FreePool(HandleBuffer);
  }
  
}





VOID
EFIAPI
PlatEnterSetupCallBack (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  VOID        *Interface;
  EFI_STATUS  Status;
  

  Status = gBS->LocateProtocol(&gEfiSetupEnterGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }
  gBS->CloseEvent(Event);

  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

}









#ifdef PCISIG_PLUGFEST_WORKAROUND

/**
  Enable ide controller.  This gets disabled when LegacyBoot.c is about
  to run the Option ROMs.

  @param  Private        Legacy BIOS Instance data


**/
VOID
EnableIdeController (
  EFI_LEGACY_BIOS_PLATFORM_PROTOCOL *LegacyBiosPlatform
  )
{
  EFI_PCI_IO_PROTOCOL *PciIo;
  EFI_STATUS          Status;
  EFI_HANDLE          IdeController;
  UINT8               ByteBuffer;
  UINTN               HandleCount;
  EFI_HANDLE          *HandleBuffer;

  Status = LegacyBiosPlatform->GetPlatformHandle (
                                          LegacyBiosPlatform,
                                          EfiGetPlatformIdeHandle,
                                          0,
                                          &HandleBuffer,
                                          &HandleCount,
                                          NULL
                                          );
  if (!EFI_ERROR (Status)) {
    IdeController = HandleBuffer[0];
    Status = gBS->HandleProtocol (
                    IdeController,
                    &gEfiPciIoProtocolGuid,
                    (VOID **) &PciIo
                    );
    ByteBuffer = 0x1f;
    if (!EFI_ERROR (Status)) {
      PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, 0x04, 1, &ByteBuffer);
    }
  }
}


/**
  Enable ide controller.  This gets disabled when LegacyBoot.c is about
  to run the Option ROMs.

  @param  Private                 Legacy BIOS Instance data


**/
VOID
EnableAllControllers (
  EFI_LEGACY_BIOS_PLATFORM_PROTOCOL *LegacyBiosPlatform
  )
{
  UINTN               HandleCount;
  EFI_HANDLE          *HandleBuffer;
  UINTN               Index;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE01          PciConfigHeader;
  EFI_STATUS          Status;

  //
  //
  //
  EnableIdeController (LegacyBiosPlatform);

  //
  // Assumption is table is built from low bus to high bus numbers.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  ASSERT_EFI_ERROR (Status);

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **) &PciIo
                    );
    ASSERT_EFI_ERROR (Status);

    PciIo->Pci.Read (
                PciIo,
                EfiPciIoWidthUint32,
                0,
                sizeof (PciConfigHeader) / sizeof (UINT32),
                &PciConfigHeader
                );

    //
    // We do not enable PPB here. This is for HotPlug Consideration.
    // The Platform HotPlug Driver is responsible for Padding enough hot plug
    // resources. It is also responsible for enable this bridge. If it
    // does not pad it. It will cause some early Windows fail to installation.
    // If the platform driver does not pad resource for PPB, PPB should be in
    // un-enabled state to let Windows know that this PPB is not configured by
    // BIOS. So Windows will allocate default resource for PPB.
    //
    // The reason for why we enable the command register is:
    // The CSM will use the IO bar to detect some IRQ status, if the command
    // is disabled, the IO resource will be out of scope.
    // For example:
    // We installed a legacy IRQ handle for a PCI IDE controller. When IRQ
    // comes up, the handle will check the IO space to identify is the
    // controller generated the IRQ source.
    // If the IO command is not enabled, the IRQ handler will has wrong
    // information. It will cause IRQ storm when the correctly IRQ handler fails
    // to run.
    //
    if (!(IS_PCI_VGA (&PciConfigHeader)     ||
          IS_PCI_OLD_VGA (&PciConfigHeader) ||
          IS_PCI_IDE (&PciConfigHeader)     ||
          IS_PCI_P2P (&PciConfigHeader)     ||
          IS_PCI_P2P_SUB (&PciConfigHeader) ||
          IS_PCI_LPC (&PciConfigHeader)     )) {

      PciConfigHeader.Hdr.Command |= 0x1f;

      PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 4, 1, &PciConfigHeader.Hdr.Command);
    }
  }
}

/**

  This is the Workaround code for PCISIG Platform BIOS Test
  (To Workaround "1GB MEM32 and Varioud MEM32 Request" Test Item BIOS Kernel hang Issue)

**/
VOID
PCISIG_Workaround (
	VOID
  )
{
  UINTN               HandleCount;
  UINT8               tmp8;
  EFI_HANDLE          *HandleBuffer;
  UINTN               Index;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE00          PciConfigHeader;
  EFI_STATUS          Status;
  EFI_LEGACY_BIOS_PLATFORM_PROTOCOL  *LegacyBiosPlatform = NULL;
  EFI_LEGACY_BIOS_PROTOCOL           *LegacyBios = NULL;


  Status = gBS->LocateProtocol(&gEfiLegacyBiosProtocolGuid, NULL, (VOID**)&LegacyBios);	
  if(!EFI_ERROR(Status)){	
    Status = gBS->LocateProtocol(&gEfiLegacyBiosPlatformProtocolGuid, NULL, (VOID **) &LegacyBiosPlatform);
  }

  EnableAllControllers(LegacyBiosPlatform);

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  ASSERT_EFI_ERROR (Status);

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **) &PciIo
                    );
    ASSERT_EFI_ERROR (Status);

    PciIo->Pci.Read (
                PciIo,
                EfiPciIoWidthUint32,
                0,
                sizeof (PciConfigHeader) / sizeof (UINT32),
                &PciConfigHeader
                );

    if (!(IS_PCI_BRIDGE (&PciConfigHeader)     ||
          IS_CARDBUS_BRIDGE (&PciConfigHeader)
        )) {

		DEBUG ((EFI_D_ERROR, "ysw_debug_test: VID = %x, DID = %x\n ", PciConfigHeader.Hdr.VendorId, PciConfigHeader.Hdr.DeviceId));

		DEBUG ((EFI_D_ERROR, "Original Command Register: %x\n ", PciConfigHeader.Hdr.Command));

		PciConfigHeader.Hdr.Command &= ~EFI_PCI_COMMAND_MEMORY_SPACE;

		for(tmp8 = 0; tmp8 < 6; tmp8++){

			DEBUG ((EFI_D_ERROR, "BAR[%d] = %x; ", tmp8, PciConfigHeader.Device.Bar[tmp8]));	

			//Not enable "Memory Decode" Bit of PCI Command Register for Non-Bridge Component with 32-Bit MEM Bar with no Resource assigned
			if(((PciConfigHeader.Device.Bar[tmp8] & 0x01) == 0x00) && ((PciConfigHeader.Device.Bar[tmp8] & 0x06) == 0x00) && ((PciConfigHeader.Device.Bar[tmp8] & 0xFFFFFFF0) != 0x00)){

				PciConfigHeader.Hdr.Command |= EFI_PCI_COMMAND_MEMORY_SPACE;

			}else if(((PciConfigHeader.Device.Bar[tmp8] & 0x01) == 0x00) && ((PciConfigHeader.Device.Bar[tmp8] & 0x06) == 0x04)){

				PciConfigHeader.Hdr.Command |= EFI_PCI_COMMAND_MEMORY_SPACE;
			}
		}

		DEBUG ((EFI_D_ERROR, "Setting Command Register: %x\n ", PciConfigHeader.Hdr.Command));
		DEBUG ((EFI_D_ERROR, "\n"));		

    }

    PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 4, 1, &PciConfigHeader.Hdr.Command);
    
  }
}
#endif	//;PCISIG_PLUGFEST_WORKAROUND








VOID
EFIAPI
IoTrapLegacyBootEventNotify (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{		
	EFI_STATUS Status;
	UINT32 mmiobase;//=0xFED12000;
	UINT8 data;
		
	UINT64	Address;
    EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL                 *PciRootBridgeIo;	
	//mmiobase = (AsiaPciRead32(CHX002_BUSC|0xBC))<<8;
	//DEBUG(( EFI_D_ERROR, "	mmio base=0x%x \n",mmiobase));
	//AsiaMemoryModify8((UINT64) (mmiobase+0x54),0x01,0x01);
	
	Status = gBS->LocateProtocol (
					&gEfiPciRootBridgeIoProtocolGuid,
					NULL,
					&PciRootBridgeIo
					);
	ASSERT_EFI_ERROR (Status);	 

	
	Address = (CHX002_BUSC|0xBC);
	Status = PciRootBridgeIo->Pci.Read (
									PciRootBridgeIo,
									EfiPciWidthUint32,
									Address,
									1,
									&mmiobase
									);
	mmiobase = mmiobase<<8;
	DEBUG(( EFI_D_ERROR, "	mmio base=0x%x \n",mmiobase));
	data = *(volatile UINT8 *)(UINTN)(mmiobase+0x54);
	data |= 0x01;
	*(volatile UINT8 *)(UINTN)(mmiobase+0x54)=data;
	DEBUG ((EFI_D_INFO, "Enable IO trap 0x%x\n",*(volatile UINT32 *)(UINTN)(mmiobase+0x54)));

	return;
}


VOID
LockNonUpdatableFlash (
  BOOLEAN  ToLock
  )
{
  UINT32      Base;
  UINT32      Size;
  UINT32      Limit;
  UINT32      LpcMmioBar;
  UINT32      SpiBar;
  UINT32      Value;
  
  LpcMmioBar = (MmioRead32(LPC_PCI_REG(D17F0_MMIO_SPACE_BASE_ADR)) >> 4) << 12;
  SpiBar     = MmioRead32(LpcMmioBar) & ~0xFF;
  if ((MmioRead16(SpiBar + SPI0_SPIS_REG) & SPI0_SPIS_LOCKDOWN) != 0) {
    return;
  }

  if(ToLock){
  Base  = (PcdGet32 (PcdFlashFvMainBase) - PcdGet32 (PcdFlashAreaBaseAddress))>> 12;
  Size  = PcdGet32 (PcdFlashFvMainSize) >> 12;
  Limit = Base + Size - 1;
  Value = BIT31 | Base;
  if (Base >= 0 && Size > 0) {
		
	  Value=Value|(MmioRead32(SpiBar + SPI0_PROTECTED_BIOS_RANGE0_BASE));
      MmioWrite32(SpiBar + SPI0_PROTECTED_BIOS_RANGE0_BASE, Value);

	  MmioWrite32(SpiBar + SPI0_PROTECTED_BIOS_RANGE0_LIMIT,Limit);
      DEBUG((EFI_D_INFO, "R0 base:%X limit: %X\n", Value,Limit));    
    }
  } else {
    MmioAnd32(SpiBar + SPI0_PROTECTED_BIOS_RANGE0_BASE, (UINT32)~BIT31);
  }

  if(ToLock){
  Base  = (PcdGet32 (PcdFlashFvRecoveryBase) - PcdGet32 (PcdFlashAreaBaseAddress)) >> 12;
  Size  = PcdGet32 (PcdFlashFvRecoverySize) >> 12;
  Limit = Base + Size - 1;
  Value = BIT31 | Base ;
  if (Base >= 0 && Size > 0) {

	Value=Value|(MmioRead32(SpiBar + SPI0_PROTECTED_BIOS_RANGE1_BASE));
    MmioWrite32(SpiBar + SPI0_PROTECTED_BIOS_RANGE1_BASE, Value);
	MmioWrite32(SpiBar + SPI0_PROTECTED_BIOS_RANGE1_LIMIT,Limit);
	
	DEBUG((EFI_D_INFO, "R1 base:%X limit: %X\n", Value,Limit));	
    }
  } else {
    MmioAnd32(SpiBar + SPI0_PROTECTED_BIOS_RANGE1_BASE, (UINT32)~BIT31);
  }
}


VOID ClearCmosBad()
{
  UINT8  Cmos0E;

  Cmos0E = CmosRead(0xE);
  if(Cmos0E & BIT7){
    CmosWrite(0xE, Cmos0E & (UINT8)~BIT7);
  }
}


VOID PlatformAfterConsoleStartHook()
{
  BOOLEAN    BootState;

  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  BootState = PcdGetBool(PcdBootState);
  if (BootState) {
    PcdSetBool(PcdBootState, FALSE);
  }
  
  TcgPhysicalPresenceLibProcessRequest();
  Tcg2PhysicalPresenceLibProcessRequest(NULL);  
}

VOID PlatformAfterConsoleEndHook()
{
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

#ifdef PCISIG_PLUGFEST_WORKAROUND
  PCISIG_Workaround();
#endif  

  ClearCmosBad();
}


VOID PlatformBeforeBiosUpdateHook()
{
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  LockNonUpdatableFlash(FALSE);
}


VOID PlatformBeforeConnectPciRootBridgeHook()
{
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  LockNonUpdatableFlash(TRUE);
}


EFI_STATUS
EFIAPI
PlatformDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS              Status;
  VOID                    *Registration;
  EFI_EVENT               Event;  
   
  
  DEBUG((DEBUG_INFO, __FUNCTION__"()\n"));
  ASSERT(sizeof(DATA_64)==8);  

  gResvdPage = AllocateReservedPages(1);
  ASSERT(gResvdPage!=NULL);
  DEBUG((EFI_D_INFO, "gResvdPage:%X\n", gResvdPage));
  
  gSetupData = GetSetupDataHobData();
#ifdef ZX_SECRET_CODE   

  Status = gBS->InstallProtocolInterface (
					&gImageHandle,
					&gEfiFsbcDumpProtocolGuid,
					EFI_NATIVE_INTERFACE,
					&mFsbcDump
					);
  ASSERT_EFI_ERROR (Status);
  
  // hxz-20171012 add for spc test, enable fsbc or tracer
  if(gSetupData->CPU_TRACER_EN||gSetupData->CPU_MASTER_FSBC_EN){
	  CpuDebug();
  }
#endif  
  Status = gBS->LocateProtocol(&gAsiaNbProtocolGuid,  NULL, (VOID**)&gPtAsiaNb);
  ASSERT_EFI_ERROR(Status);
  Status = gBS->LocateProtocol(&gAsiaSbProtocolGuid,  NULL, (VOID**)&gPtAsiaSb);
  ASSERT_EFI_ERROR(Status);
  Status = gBS->LocateProtocol(&gAsiaCpuProtocolGuid, NULL, (VOID**)&gPtAsiaCpu);
  ASSERT_EFI_ERROR(Status);  
  gAsiaNbCfg = (ASIA_NB_CONFIGURATION*)gPtAsiaNb->NbCfg;  
  gAsiaSbCfg = (ASIA_SB_CONFIGURATION*)gPtAsiaSb->SbCfg;  	

  PlatformDebugAtEntryPoint(ImageHandle, SystemTable);
#ifdef ZX_SECRET_CODE
	  DEBUG((EFI_D_ERROR,"+++++Start Set MSR in on DXE\n"));
	  
	  {  
		  EFI_STATUS Status;
		  EFI_MP_CONFIG_PROTOCOL*	  MpConfig;
		  Status = gBS->LocateProtocol (&gEfiCpuMpConfigProtocolGuid, NULL,(VOID**)&
	MpConfig);
		  if(Status==0){
			 MpConfig->ConfigMsr(DXE);
		  }
	   }
	  DEBUG((EFI_D_ERROR,"+++++After Set MSR in on DXE\n"));
#endif

  Status = gPtAsiaSb->PrePciInit(gPtAsiaSb);
  ASSERT_EFI_ERROR(Status);
  Status = gPtAsiaNb->PrePciInit(gPtAsiaNb);
  ASSERT_EFI_ERROR(Status);

  Status = LegacyInterruptInstall(ImageHandle, SystemTable);  
  ASSERT_EFI_ERROR(Status);
  
  Status = LegacyBiosPlatformInstall(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR(Status);  

  Status = SmmAccess2Install(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR(Status);
  
  Status = LegacyRegion2Install(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR(Status);  

  Status = PciPlatformInstall(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR(Status);    

  Status = SataControllerInstall(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR(Status);

  Status = IncompatiblePciDeviceSupportEntryPoint(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR(Status);  
  
  Status = MiscConfigDxe();
  ASSERT_EFI_ERROR(Status);

  Status = IsaAcpiDevListDxe();
  ASSERT_EFI_ERROR(Status);  
  
#if defined(PCAL6416A_PCIE_HOTPLUG_SUPPORT_CHX002) || defined(PCAL6416A_PCIE_HOTPLUG_SUPPORT_IOE)
  Status = PciHotPlugEntryPoint(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR(Status);  
#endif

  EfiCreateProtocolNotifyEvent (
    &gExitPmAuthProtocolGuid,
    TPL_CALLBACK,
    ExitPmAuthCallBack,
    NULL,
    &Registration
    );   

  EfiCreateProtocolNotifyEvent (
    &gBdsAllDriversConnectedProtocolGuid,
    TPL_CALLBACK,
    AllDriversConnectedCallBack,
    NULL,
    &Registration
    );      
#ifdef IOE_EXIST
	EfiCreateProtocolNotifyEvent (
	  &gBdsAllDriversConnectedProtocolGuid,
	  TPL_CALLBACK,
	  EepromInitCallBack,
	  NULL,
	  &Registration
	  );	  
#endif

  EfiCreateProtocolNotifyEvent (
    &gEfiSmbiosProtocolGuid,
    TPL_CALLBACK,
    SmbiosCallback,
    NULL,
    &Registration
    );       

  EfiCreateProtocolNotifyEvent (
    &gEfiHiiConfigAccessProtocolGuid,
    TPL_CALLBACK,
    HiiConfigAccessCallback,
    NULL,
    &Registration
    );  

  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             PlatOnReadyToBoot,
             NULL,
             &Event
             ); 
  ASSERT_EFI_ERROR(Status);             

  EfiCreateProtocolNotifyEvent (
    &gEfiSetupEnterGuid,
    TPL_CALLBACK,
    PlatEnterSetupCallBack,
    NULL,
    &Registration
    ); 

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IoTrapLegacyBootEventNotify,
                  NULL,
                  &gEfiEventLegacyBootGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallMultipleProtocolInterfaces(
                  &ImageHandle,
                  &gPlatAfterConsoleStartProtocolGuid, PlatformAfterConsoleStartHook,
                  &gPlatAfterConsoleEndProtocolGuid, PlatformAfterConsoleEndHook,
                  &gPlatBeforeBiosUpdateProtocolGuid, PlatformBeforeBiosUpdateHook,
                  &gEfiBeforeConnectPciRootBridgeGuid, PlatformBeforeConnectPciRootBridgeHook,
                  NULL
                  );
  
  return Status;
}




