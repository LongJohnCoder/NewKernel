/** @file

Copyright (c) 2006 - 2016, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SmiFlash.c

Abstract:
  Provides Access to flash backup Services through SMI

Revision History:

**/

#include "SmiFlash.h"
#include <Framework/SmmCis.h>
#include <Library/IoLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/BiosIdLib.h>
#include <Library/UefiLib.h>
#include <Guid/Acpi.h>
//#include <Foundation/Efi/Guid/Acpi/Acpi.h>

// OA3
#include <Protocol/AcpiTable.h>

//---------------------------------------------------------------------------
NV_MEDIA_ACCESS_PROTOCOL              *mMediaAccess;
EFI_SMM_CPU_PROTOCOL                  *mSmmCpu;
UINT8                                 *BlockBuffer;
BOOLEAN                               EnvPrepared  = FALSE;
UINT16                                AcpiPmEnData = 0;
BIOS_ID_IMAGE                         gBiosIdImage;



// oa3 Function
#pragma pack (1)
typedef struct {
  UINT32                           MsdmVersion;
  UINT32                           MsdmReserved;
  UINT32                           MdsmDataType;
  UINT32                           MsdmDataReserved;
  UINT32                           MsdmDataLength;
  UINT8                            MsdmData[29];      //5*5 Product Key, including "-"
} EFI_ACPI_MSDM_DATA_STRUCTURE;

//
// MSDM Table structure
//
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER        Header;
  EFI_ACPI_MSDM_DATA_STRUCTURE       MsdmData;
} EFI_ACPI_MS_DIGITAL_MARKER_TABLE;
#pragma pack ()


UINTN
GetOa3Base(
  VOID
  )
{
  return FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE  + EFI_PAGE_SIZE;
}

VOID  *mRsdp3;

VOID
GetRsdpTable(
  VOID
  )
{
  EFI_STATUS          Status;

  Status = EfiGetSystemConfigurationTable (
             &gEfiAcpiTableGuid, 
             &mRsdp3
             );
  DEBUG((EFI_D_ERROR, "GetRsdp: %r %lx\n", Status, mRsdp3));
}


UINTN
GetMsdmTableAddress(
  VOID
  )
{
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp3;
  EFI_ACPI_DESCRIPTION_HEADER                   *XsdtHdr;
  EFI_ACPI_DESCRIPTION_HEADER                   *TblHdr;
  UINTN                                         TableCount;
  UINT64                                        *TblAddr64;
  UINTN                                         Index;

  DEBUG((EFI_D_ERROR, "%a start\n", __FUNCTION__));

  DEBUG((EFI_D_ERROR, "Get acpi Table hdr\n"));
  Rsdp3 = (EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)mRsdp3;
  DEBUG((EFI_D_ERROR, "Found ACPI Table: %lx\n", Rsdp3));

  XsdtHdr = (EFI_ACPI_DESCRIPTION_HEADER*)(UINTN)Rsdp3->XsdtAddress;
  TableCount = (XsdtHdr->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
  TblAddr64  = (UINT64*)((UINT8*)XsdtHdr + sizeof(EFI_ACPI_DESCRIPTION_HEADER));
  DEBUG((EFI_D_ERROR, "Acpi Count: %x, TblAddress: %lx\n", TableCount, TblAddr64));
  for(Index = 0; Index < TableCount; ++Index) {
    TblHdr = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(TblAddr64[Index]);
    if ((TblHdr->Signature == SIGNATURE_32('S','S','D','M')) || (TblHdr->Signature == SIGNATURE_32('M','S','D','M'))) {
      DEBUG((EFI_D_ERROR, "Found MsdmTable! %lx\n", (UINTN)TblHdr));
      return (UINTN)TblHdr;
    }
  }

  return 0;
}

VOID
CheckOa3(
  VOID
  )
{
  EFI_STATUS                            Status;
  EFI_ACPI_MSDM_DATA_STRUCTURE          *NvMsdmData;
  EFI_ACPI_TABLE_PROTOCOL               *AcpiTableProtocol;
  EFI_ACPI_MS_DIGITAL_MARKER_TABLE      *MsdmPointer;
  UINT32                                TableLength;
  UINTN                                 TableHandle;

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, &AcpiTableProtocol);
  if(EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Locate ApcitableProtocol %r\n", Status));
  }

  NvMsdmData = (EFI_ACPI_MSDM_DATA_STRUCTURE *)GetOa3Base();
  DEBUG((EFI_D_ERROR, "nvMsdmData: %x\n", NvMsdmData));
  if(NvMsdmData->MsdmVersion == 0xffffffff) {
    return;
  }

  TableLength = sizeof (EFI_ACPI_MS_DIGITAL_MARKER_TABLE);
  Status = gBS->AllocatePool(EfiACPIMemoryNVS, TableLength, &MsdmPointer); 
  ASSERT (MsdmPointer != NULL);
  if (MsdmPointer == NULL) {
    return;
  }

  //
  //Update the OA30 table header.
  //
  MsdmPointer->Header.Signature = SIGNATURE_32('S','S','D','M');
  MsdmPointer->Header.Length = TableLength;
  MsdmPointer->Header.Revision = 0x01;
  MsdmPointer->Header.CreatorRevision = 0x01;

  if(NvMsdmData->MsdmVersion != 0x54534554){
    MsdmPointer->Header.Signature = SIGNATURE_32('M','S','D','M');
  }

  //
  // Copy MSDM data to the memory pool created in OA table.
  //
  CopyMem (&(MsdmPointer->MsdmData), NvMsdmData, sizeof (EFI_ACPI_MSDM_DATA_STRUCTURE));

  //
  // Add the OA 3.0 table to ACPI memory.
  //
  TableHandle = 0;
  Status = AcpiTableProtocol->InstallAcpiTable (AcpiTableProtocol, MsdmPointer, TableLength, &TableHandle);
  DEBUG((EFI_D_INFO,"%a(%d) Status = %r\n",__FUNCTION__,__LINE__, Status));

}


static
UINT8
EFIAPI
CalculateCheckSum (
  IN      CONST UINT8              *Buffer,
  IN      UINTN                     Length
  )
{
  UINT8     CheckSum;
  UINTN     Count;

  ASSERT (Buffer != NULL);
  ASSERT (Length <= (MAX_ADDRESS - ((UINTN) Buffer) + 1));

  for (CheckSum = 0, Count = 0; Count < Length; Count++) {
    CheckSum = (UINT8) (CheckSum + *(Buffer + Count));
  }

  //
  // Return the checksum based on 2's complement.
  //
  return (UINT8) (0x100 - CheckSum);
}


EFI_STATUS
EFIAPI
UpdateOA30TableToMem (
  EFI_PHYSICAL_ADDRESS         Data
  )
{
  EFI_STATUS                                    Status = EFI_SUCCESS;
  EFI_ACPI_MS_DIGITAL_MARKER_TABLE              *MsdmPointer;
  EFI_PHYSICAL_ADDRESS                          MsdmAddress;
  UINT32                                        BufferSize;
  EFI_ACPI_MSDM_DATA_STRUCTURE                  *Address;

  Address = (EFI_ACPI_MSDM_DATA_STRUCTURE *)(UINTN)Data;

  MsdmPointer = NULL;
  BufferSize = sizeof (EFI_ACPI_MSDM_DATA_STRUCTURE);
  DEBUG((EFI_D_INFO,"OA30SMI:UpdateOA30Table\n"));
  DEBUG((EFI_D_INFO,"%a(%d) Data = %x\n",__FUNCTION__,__LINE__, Data));

  //if ((Address->MsdmVersion != SIGNATURE_32('M','S','D','M')) || (Address->MsdmDataLength != 0x1d)) {
    //return EFI_NOT_AVAILABLE_YET;
  //}

  MsdmAddress = (EFI_PHYSICAL_ADDRESS)GetMsdmTableAddress();

  DEBUG((EFI_D_INFO,"MsdmAddress:%x\n",MsdmAddress));
  if (MsdmAddress == 0) {
    return EFI_NOT_FOUND;
  }

  MsdmPointer = (EFI_ACPI_MS_DIGITAL_MARKER_TABLE*)(UINTN)MsdmAddress;
  DEBUG((EFI_D_INFO,"MsdmPointer:%x\n",MsdmPointer));
  //
  //Compare the org MSDM Table with updated MSDM table
  //
  if (!CompareMem((UINT8*)(UINTN)&MsdmPointer->MsdmData, Address, sizeof (EFI_ACPI_MSDM_DATA_STRUCTURE))) {
    return EFI_NOT_AVAILABLE_YET;
  }
  CopyMem (&(MsdmPointer->MsdmData), Address, sizeof (EFI_ACPI_MSDM_DATA_STRUCTURE));
  DEBUG((EFI_D_INFO,"OA30SMI: Memory copied\n"));
  MsdmPointer->Header.Signature = SIGNATURE_32('M','S','D','M');
  MsdmPointer->Header.Checksum = 0;
  MsdmPointer->Header.Checksum = CalculateCheckSum((UINT8 *) MsdmPointer, MsdmPointer->Header.Length);
  DEBUG((EFI_D_INFO,"OA30SMI: new check sum = %x\n",MsdmPointer->Header.Checksum));

  return Status;
}


EFI_STATUS
ProgramFlash (
  IN  UINT32  BiosAddr,
  IN  UINT32  Size,
  IN  UINT32  Buffer
)
{
  EFI_STATUS                   Status = EFI_SUCCESS;
  EFI_FIRMWARE_VOLUME_HEADER   *FvHdr;
  

  if(BiosAddr == PcdGet32(PcdFlashFvMainBase)){
    FvHdr = (EFI_FIRMWARE_VOLUME_HEADER*)(UINTN)Buffer;
    if(FvHdr->Signature == EFI_FVH_SIGNATURE){
      FvHdr->Signature = 0xFFFFFFFF;
    }
  }

  WriteBackInvalidateDataCacheRange((VOID*)(UINTN)BiosAddr, Size);
  if(CompareMem((VOID*)(UINTN)BiosAddr, (VOID*)(UINTN)Buffer, Size) == 0){
    DEBUG((EFI_D_INFO, "%X Equal\n", BiosAddr));
    goto ProcExit;
  }
  Status = mMediaAccess->Erase (
                           mMediaAccess,
                           BiosAddr,
                           Size,
                           SPI_MEDIA_TYPE
                           );
  Status = mMediaAccess->Write (
                           mMediaAccess,
                           BiosAddr,
                           (VOID*)(UINTN)Buffer,
                           Size,
                           SPI_MEDIA_TYPE
                           );
  if (EFI_ERROR(Status)) {
    goto ProcExit;
  }

  WriteBackInvalidateDataCacheRange((VOID*)(UINTN)BiosAddr, Size);
  if (CompareMem((VOID*)(UINTN)BiosAddr, (VOID*)(UINTN)Buffer, Size)){
    DEBUG((EFI_D_ERROR, "After Write Verify Err\n"));
    Status = EFI_DEVICE_ERROR;
  }

ProcExit:
  return Status;
}



EFI_STATUS CheckBiosId(BIOS_ID_IMAGE *Id)
{
  BIOS_ID_IMAGE  *OldBiosId;
  BIOS_ID_STRING *IdStr;
  EFI_STATUS     Status;

  OldBiosId = &gBiosIdImage;
  if(CompareMem(OldBiosId->Signature, Id->Signature, sizeof(OldBiosId->Signature))){
    Status = EFI_INVALID_PARAMETER;
    goto ProcExit;
  }

  IdStr = &gBiosIdImage.BiosIdString;
  if(CompareMem(IdStr->BoardId, Id->BiosIdString.BoardId, sizeof(IdStr->BoardId)) ||
     CompareMem(IdStr->OemId, Id->BiosIdString.OemId, sizeof(IdStr->OemId))){
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }
  
  Status = EFI_SUCCESS;

ProcExit:
  return Status;
}




EFI_STATUS SetFvMainSign(BOOLEAN Set)
{
  EFI_STATUS  Status;
  UINT32      FvMain;
  UINT32      Sign;


  DEBUG((EFI_D_INFO, "%a(%d)\n", __FUNCTION__, Set));

  FvMain = PcdGet32(PcdFlashFvMainBase);
  if(Set){
    Sign = EFI_FVH_SIGNATURE;
  } else {
    Sign = 0;
  }
  
  Status = mMediaAccess->Write(
                           mMediaAccess,
                           FvMain + OFFSET_OF(EFI_FIRMWARE_VOLUME_HEADER, Signature),
                           (UINT8*)&Sign,
                           sizeof(Sign),
                           SPI_MEDIA_TYPE
                           );

  return Status;
}



EFI_STATUS
FlashInterface (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
  )
{
    EFI_STATUS                   Status;
    UINTN                        Index;
    UINTN                        CpuIndex;
    UINT8                        SubFunction;
    BIOS_UPDATE_BLOCK_PARAMETER  *BlockParam;
    UINT32                       RegEax;
    UINT32                       RegEbx;
    EFI_SMM_SAVE_STATE_IO_INFO   IoState;
    UINT16                       AcpiIoBase;
    //UINT8                        *Flag;
	UINT32                        *NvVariableAddr;
		    FD_AREA_INFO                    *FdArea;
    AcpiIoBase = PcdGet16(AcpiIoPortBaseAddress);
    
    CpuIndex = 0;
    for (Index = 0; Index < gSmst->NumberOfCpus; Index++) {
        Status = mSmmCpu->ReadSaveState (
                            mSmmCpu,
                            sizeof (EFI_SMM_SAVE_STATE_IO_INFO),
                            EFI_SMM_SAVE_STATE_REGISTER_IO,
                            Index,
                            &IoState
                            );
        if (!EFI_ERROR (Status) && (IoState.IoData == SW_SMI_FLASH_SERVICES)) {
            CpuIndex = Index;
            break;
        }
    }
    if (Index >= gSmst->NumberOfCpus) {
        CpuDeadLoop ();
    }

    //
    // Ready save state for register
    //
    Status = mSmmCpu->ReadSaveState (
                        mSmmCpu,
                        sizeof (UINT32),
                        EFI_SMM_SAVE_STATE_REGISTER_RAX,
                        CpuIndex,
                        &RegEax
                        );
    ASSERT_EFI_ERROR (Status);

    Status = mSmmCpu->ReadSaveState (
                        mSmmCpu,
                        sizeof (UINT32),
                        EFI_SMM_SAVE_STATE_REGISTER_RBX,
                        CpuIndex,
                        &RegEbx
                        );
    ASSERT_EFI_ERROR (Status);

    SubFunction = (UINT8)(RegEax >> 8);
	  DEBUG((EFI_D_ERROR, "SubFunction: %x\n",SubFunction));
    switch (SubFunction) {
      case GET_FD_AREA_SUBFUNCTION:
        FdArea = (FD_AREA_INFO *) (UINT64)RegEbx;
 
        switch(FdArea->Type) {
          case FD_AREA_TYPE_FD:
          default:
          FdArea->Offset = 0;
          FdArea->Size = 0x800000;
          break;
       }
      Status = EFI_SUCCESS;
      break;
      
	  case FUNC_CLEAR_RECOVERY: 
	    BlockParam = (BIOS_UPDATE_BLOCK_PARAMETER*)(UINTN)RegEbx;
	    DEBUG((EFI_D_ERROR, "BlockParam->BiosAddr %lx \n",BlockParam->BiosAddr));
	    DEBUG((EFI_D_ERROR, "BlockParam->Size     %lx \n",BlockParam->Size));
	  	Status = mMediaAccess->Erase(
                                mMediaAccess,
                                BlockParam->BiosAddr, 
                                BlockParam->Size, 
                                SPI_MEDIA_TYPE
                                );
    	break;
      
      case FUNC_CHECK_BIOS_ID:
        Status = CheckBiosId((BIOS_ID_IMAGE*)(UINTN)RegEbx);
        break;

      case FUNC_PREPARE_BIOS_FLASH_ENV:
        AcpiPmEnData = IoRead16(AcpiIoBase+2);
        IoWrite16(AcpiIoBase+2, AcpiPmEnData & (UINT16)~BIT8);  // disable power button.
        SetFvMainSign(FALSE);
        EnvPrepared = TRUE;
        break;
        
      case FUNC_CLEAR_BIOS_FLASH_ENV:
        if(EnvPrepared){
          SetFvMainSign(TRUE);
          IoWrite16(AcpiIoBase, BIT8);           // clear power button status.
          IoWrite16(AcpiIoBase+2, AcpiPmEnData); // restore
          EnvPrepared = FALSE;
        }
        break;
          
      case FUNC_UPDATE_SMBIOS_DATA:
        Status = HandleSmbiosDataRequest((UPDATE_SMBIOS_PARAMETER*)(UINTN)RegEbx);
        break;

      case FUNC_UPDATE_BOOT_LOGO:
      { 
		
        NVLOGO_PARAMETER        *NvLogoBlock;
        NvLogoBlock = (NVLOGO_PARAMETER *)(UINTN)RegEbx;
	    //DEBUG((EFI_D_ERROR, "NvLogoBlock->LogoOffest %x \n",NvLogoBlock->LogoOffest));
		//DEBUG((EFI_D_ERROR, "NvLogoBlock->BufferSize %x \n",NvLogoBlock->BufferSize));
		//DEBUG((EFI_D_ERROR, "NvLogoBlock->Buffer %lx \n",NvLogoBlock->Buffer));
		if(NvLogoBlock->Flag == 1){
        Status = mMediaAccess->Erase(
                                mMediaAccess,
                                (UINT32)(UINTN)(FLASH_REGION_NVLOGO_STORE_BASE),
                                FLASH_REGION_NVLOGO_STORE_SIZE,
                                SPI_MEDIA_TYPE
                                );
			}
        Status = mMediaAccess->Write(
                                mMediaAccess,
                                (UINT32)(UINTN)(FLASH_REGION_NVLOGO_STORE_BASE + NvLogoBlock->LogoOffest),
                                (void *)(UINTN)(NvLogoBlock->Buffer),
                                EFI_PAGE_SIZE,
                                SPI_MEDIA_TYPE
                                );
       }
        break;

        
      case  FUNC_KEEP_VARIABLE_STORAGE:
        NvVariableAddr = (UINT32 *)(UINTN)RegEbx;
        *NvVariableAddr = (UINT32)(FLASH_REGION_NVVARIABLE_STORE_BASE);
        //DEBUG((EFI_D_ERROR, "NvVariableAddr %lx \n",*NvVariableAddr));
		    break;  
        
        
	  case  FUNC_CLEAR_SMBOIS_OA3:
	  	Status = mMediaAccess->Erase (
                                     mMediaAccess,
                                     FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE,
                                     FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_SIZE + EFI_PAGE_SIZE,
                                     SPI_MEDIA_TYPE
                                     );
		  break;

      
      case FUNC_PROGRAM_FLASH:
        BlockParam = (BIOS_UPDATE_BLOCK_PARAMETER*)(UINTN)RegEbx;
        Status = ProgramFlash (
                   BlockParam->BiosAddr, 
                   BlockParam->Size, 
                   BlockParam->Buffer
                   );
        break;
        
       case UPDATE_OA30_DATA:
    {
      ROM_HOLE_PARAMETER        *RomBlock;

      RomBlock = (ROM_HOLE_PARAMETER *)(UINTN)RegEbx;
      Status = UpdateOA30TableToMem (RomBlock->Buffer);
      if(!EFI_ERROR(Status) || Status == EFI_NOT_FOUND) {
        DEBUG((EFI_D_ERROR, "UpdateOa30ToMem: %r\n", Status));

        Status = mMediaAccess->Erase(
                                mMediaAccess,
                                (UINT32)(UINTN)(FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE + EFI_PAGE_SIZE),
                                EFI_PAGE_SIZE,
                                SPI_MEDIA_TYPE
                                );
        DEBUG((EFI_D_ERROR, " Erase Done\n"));
        Status = mMediaAccess->Write(
                                mMediaAccess,
                                (UINT32)(UINTN)(FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE + EFI_PAGE_SIZE),
                                (void *)(UINTN)(RomBlock->Buffer),
                                EFI_PAGE_SIZE,
                                SPI_MEDIA_TYPE
                                );
       DEBUG((EFI_D_ERROR, " Write Done\n"));
      }
    }
    break;


	
      default:
          Status = RETURN_INVALID_PARAMETER;
          break;
    }

    RegEax = (UINT32)Status;
    Status = mSmmCpu->WriteSaveState (
                        mSmmCpu,
                        sizeof (UINT32),
                        EFI_SMM_SAVE_STATE_REGISTER_RAX,
                        CpuIndex,
                        &RegEax
                        );
    ASSERT_EFI_ERROR (Status);

    return Status;
}






EFI_STATUS
SmiFlashEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL  *SwDispatch = NULL;
  EFI_SMM_SW_REGISTER_CONTEXT   SwContext;
  EFI_HANDLE                    Handle;

 //DEBUG((EFI_D_ERROR, " 123456789\n"));
  Status = GetBiosId(&gBiosIdImage);
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmCpuProtocolGuid,
                    NULL,
                    (VOID **)&mSmmCpu
                    );
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    &SwDispatch
                    );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
      return Status;
  }

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmNvMediaAccessProtocolGuid,
                    NULL,
                    &mMediaAccess
                    );
  ASSERT_EFI_ERROR (Status);                    

  SwContext.SwSmiInputValue = SW_SMI_FLASH_SERVICES;
  Status = SwDispatch->Register (
                         SwDispatch,
                         FlashInterface,
                         &SwContext,
                         &Handle
                         );
  ASSERT_EFI_ERROR (Status);

  Status = AllocDataBuffer();
  ASSERT_EFI_ERROR (Status);
  
  GetRsdpTable();
  CheckOa3();
  

  return Status;
}


