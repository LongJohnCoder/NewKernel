
#include "PlatHost.h"


BOOLEAN 
NvmeOpRomCheck (
  EFI_HANDLE               Handle,
  EFI_PCI_IO_PROTOCOL      *PciIo,
  PLAT_HOST_INFO_PROTOCOL  *HostInfo
  )
{
  UINT8                       ClassCode[3];
  EFI_STATUS                  Status;
  BOOLEAN                     HasImage = FALSE;
  
  
  Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, PCI_CC_PI_REG, 3, ClassCode);
   
  //
  // Examine Nvm Express controller PCI Configuration table fields
  //
  if ((ClassCode[0] != PCI_IF_NVMHCI) || (ClassCode[1] != PCI_CLASS_MASS_STORAGE_NVM) || (ClassCode[2] != PCI_CLASS_MASS_STORAGE)) {
    return FALSE;
  }

  if (PciIo->RomImage != NULL && PciIo->RomSize !=0){
    HasImage = TRUE;
	DEBUG ((EFI_D_INFO, "NvmeOpRomCheck:RomImage=%x RonSize=%x\n",PciIo->RomImage,PciIo->RomSize));
  }
  
  if (gSetupHob->NvmeOpRomPriority == 1 && HasImage) {
  	return FALSE; // Addon OPROM exists and first
  }	
  
  return TRUE;  
}

BOOLEAN 
OnBoardGnicOpRomCheck(
  EFI_HANDLE               Handle,
  EFI_PCI_IO_PROTOCOL      *PciIo,
  PLAT_HOST_INFO_PROTOCOL  *HostInfo
  )
{
  UINT16 					  VendorID, DeviceID;
  EFI_STATUS				  Status;
  EFI_DEVICE_PATH_PROTOCOL	  *Dp;
  UINTN                       Index;
  BOOLEAN					  Rc = FALSE;
  BOOLEAN                     DpMatch = FALSE;


  Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, PCI_VID_REG, 1, &VendorID);
	Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, PCI_DID_REG, 1, &DeviceID);
  
  if((VendorID!=0x1D17) && (DeviceID!=0x9180)){
	  goto ProcExit;
  }

  Status = gBS->HandleProtocol (
				  Handle,
				  &gEfiDevicePathProtocolGuid,
				  &Dp
				  );
  if(EFI_ERROR(Status)){
	goto ProcExit;
  } 
  
  for(Index=0;Index<HostInfo->HostCount;Index++){
		if(HostInfo->HostList[Index].HostType != PLATFORM_HOST_LAN){
		  continue;
		}
		if(CompareMem(HostInfo->HostList[Index].Dp, Dp, GetDevicePathSize(Dp))==0){
		  DpMatch = TRUE;
		  break;
		} 
  }
  if(!DpMatch){
	  goto ProcExit;
  }

  if(!gSetupHob->ObLanBoot){
	  goto ProcExit;	
  }

  Rc = TRUE;

ProcExit:
  return Rc;  

}


BOOLEAN 
OnBoardLanOpRomCheck (
  EFI_HANDLE               Handle,
  EFI_PCI_IO_PROTOCOL      *PciIo,
  PLAT_HOST_INFO_PROTOCOL  *HostInfo
  )
{
  UINT8                       ClassCode[3];
  EFI_STATUS                  Status;
  EFI_DEVICE_PATH_PROTOCOL    *Dp;
  BOOLEAN                     Rc = FALSE;
  UINTN                       Index;
  BOOLEAN                     DpMatch = FALSE;


  Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, PCI_CC_PI_REG, 3, ClassCode);
  if(ClassCode[2] != PCI_CLASS_NETWORK){
    goto ProcExit;
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  &Dp
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  

  if(HostInfo == NULL){
    goto ProcExit;
  } 

  for(Index=0;Index<HostInfo->HostCount;Index++){
    if(HostInfo->HostList[Index].HostType != PLATFORM_HOST_LAN){
      continue;
    }
    if(CompareMem(HostInfo->HostList[Index].Dp, Dp, GetDevicePathSize(Dp))==0){
      DpMatch = TRUE;
      break;
    } 
  }
  if(!DpMatch){
    goto ProcExit;
  }

// legacy : 0
// uefi   : 1
// no     : 2
  if(!gSetupHob->ObLanBoot || PcdGet8(PcdPxeOpRomLaunchPolicy) != 0){
    goto ProcExit;  
  }

  Rc = TRUE;

ProcExit:
  return Rc;  
}


#ifdef HX002EK0_03
BOOLEAN 
OnBoardLanISCSIOpRomCheck (
  EFI_HANDLE               Handle,
  EFI_PCI_IO_PROTOCOL      *PciIo,
  PLAT_HOST_INFO_PROTOCOL  *HostInfo
  )
{
  UINT8                       ClassCode[3];
  EFI_STATUS                  Status;
  EFI_DEVICE_PATH_PROTOCOL    *Dp;
  BOOLEAN                     Rc = FALSE;
  UINTN                       Index;
  BOOLEAN                     DpMatch = FALSE;


  Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, PCI_CC_PI_REG, 3, ClassCode);
  if(ClassCode[2] != PCI_CLASS_NETWORK){
    goto ProcExit;
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  &Dp
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  

  if(HostInfo == NULL){
    goto ProcExit;
  } 

  for(Index=0;Index<HostInfo->HostCount;Index++){
    if(HostInfo->HostList[Index].HostType != PLATFORM_HOST_LAN){
      continue;
    }
    if(CompareMem(HostInfo->HostList[Index].Dp, Dp, GetDevicePathSize(Dp))==0){
      DpMatch = TRUE;
      break;
    } 
  }
  if(!DpMatch){
    goto ProcExit;
  }

// legacy : 0
// uefi   : 1
// no     : 2
  if(!gSetupHob->LegacyiSCSIBoot || PcdGet8(PcdPxeOpRomLaunchPolicy) != 0){
    goto ProcExit;  
  }

  Rc = TRUE;

ProcExit:
  return Rc;  
}
#endif

BOOLEAN 
OnBoardAhciOpRomCheck(
  EFI_HANDLE               Handle,
  EFI_PCI_IO_PROTOCOL      *PciIo,
  PLAT_HOST_INFO_PROTOCOL  *HostInfo
  )
{
  UINT8                           ClassCode[3];
  EFI_STATUS                      Status;
  UINTN                           Index;
  EFI_DEVICE_PATH_PROTOCOL        *Dp;
  EFI_ATA_PASS_THRU_PROTOCOL      *AtaPassThru;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *ExtScsiPT;  
  UINT16                          SataPort;
  UINT16                          SataPortMp;
  UINT64                          Lun;  
  UINT8                           Target[TARGET_MAX_BYTES];
  UINT8                           *TargetId;  
  BOOLEAN                         Rc = FALSE;


  Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, PCI_CC_PI_REG, 3, ClassCode);
  if(ClassCode[2] != PCI_BCC_STORAGE || ClassCode[1] != PCI_SCC_AHCI){
    goto ProcExit;
  }
  if(HostInfo == NULL){
    goto ProcExit;
  } 

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  &Dp
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  


  for(Index = 0; Index < HostInfo->HostCount; Index++){
    if(HostInfo->HostList[Index].HostType != PLATFORM_HOST_SATA){
      continue;
    }
    if(CompareMem(Dp, HostInfo->HostList[Index].Dp, GetDevicePathSize(Dp))){
      continue;
    }  
    
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiAtaPassThruProtocolGuid,
                    &AtaPassThru
                    );
    if(EFI_ERROR(Status)){
      goto TryScsi;
    }
    SataPort = 0xFFFF;
    while (TRUE) {
      Status = AtaPassThru->GetNextPort(AtaPassThru, &SataPort);
      if (EFI_ERROR (Status)) {
        break;
      } 
      SataPortMp = 0xFFFF;
      while (TRUE) {
        Status = AtaPassThru->GetNextDevice(AtaPassThru, SataPort, &SataPortMp);
        if (EFI_ERROR (Status)) {
          break;
        }
        Rc = TRUE;
        goto ProcExit;
      }
    }
  }

TryScsi:
  Status = gBS->HandleProtocol(
                  Handle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  &ExtScsiPT
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  Lun = 0;
  TargetId = &Target[0];
  SetMem(Target, sizeof(Target), 0xFF);
  Status = ExtScsiPT->GetNextTargetLun(ExtScsiPT, &TargetId, &Lun);
  if(!EFI_ERROR(Status)){
    Rc = TRUE;
  }
  
ProcExit:
  return Rc;
}


ADDITIONAL_ROM_TABLE  gLegacyOpRomTable[] = {
  #ifdef IOE_EXIST
  {FALSE, 0, 0, LAN_IOEOPROM_FILE_GUID,  OnBoardGnicOpRomCheck},
  #endif
  #ifdef HX002EK0_03
  {TRUE,  0, 0, ISCSI_OPROM_FILE_GUID,  OnBoardLanISCSIOpRomCheck},
  #endif
  {TRUE,  0, 0, LAN_OPROM_FILE_GUID,     OnBoardLanOpRomCheck},
  {TRUE,  0, 0, AHCI_OPROM_FILE_GUID,    OnBoardAhciOpRomCheck},  // legacy & uefi not conflict
  {TRUE,  0, 0, Nvme_OPROM_FILE_GUID,    NvmeOpRomCheck},         // legacy & uefi not conflict
};

UINTN gLegacyOpRomTableSize = sizeof(gLegacyOpRomTable);



