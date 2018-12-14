/*++
Copyright (c) 2011 Byosoft Corporation. All rights reserved.

Module Name:

  SmiFlash.h

Abstract:

 This file contains the Includes, Definitions, typedefs,
 Variable and External Declarations, Structure and
 function prototypes needed for the SmiFlash driver
--*/

#include <PiSmm.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/SetMemAttributeSmmLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/NvMediaAccess.h>
#include <Protocol/Smbios.h>
#include <Library/BiosIdLib.h>
#include <Guid/SmBios.h>
#include <IndustryStandard/SmBios.h>
#include <Library/UefiLib.h>

#define SW_SMI_FLASH_SERVICES	                0xec
#define GET_FD_AREA_SUBFUNCTION                 0x02
#define   FUNC_CLEAR_RECOVERY                   0x03
#define   FUNC_UPDATE_SMBIOS_DATA               0x05
#define   FUNC_UPDATE_BOOT_LOGO                 0x06
#define   FUNC_KEEP_VARIABLE_STORAGE            0x07
#define   FUNC_CLEAR_SMBOIS_OA3                 0x08
#define   FUNC_PROGRAM_FLASH                    0x09
#define   FUNC_PREPARE_BIOS_FLASH_ENV           0x0B
#define   FUNC_CLEAR_BIOS_FLASH_ENV             0x0C
#define   FUNC_CHECK_BIOS_ID                    0x0D
#define   UPDATE_OA30_DATA                      0x0E



#define FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE        PcdGet32 (PcdFlashNvStorageSmbiosBase)
#define FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_SIZE        PcdGet32 (PcdFlashNvStorageSmbiosSize)
#define FLASH_REGION_NVLOGO_STORE_BASE                               PcdGet32 (PcdFlashNvLogoBase)
#define FLASH_REGION_NVLOGO_STORE_SIZE                               PcdGet32 (PcdFlashNvLogoSize)
#define FLASH_REGION_NVVARIABLE_STORE_BASE                           PcdGet32 (PcdFlashNvStorageVariableBase)
#define FLASH_REGION_NVVARIABLE_STORE_SIZE                           PcdGet32 (PcdFlashNvStorageVariableSize)
//#define FLASH_REGION_Flash_AreaBase_Address                          PcdGet32(PcdFlashAreaBaseAddress)

#define SMBIOS_BUFFER_SIZE        4096
#define END_HANDLE                0xFFFF
#define SMBIOS_REC_SIGNATURE      0x55AA
#define MAX_STRING_LENGTH         30


#pragma pack(1)
typedef struct {
    UINT16 Signature;
    UINT16 RecordLen;
} SMBIOS_REC_HEADER;

typedef struct {
    UINT8   Type;
    UINT8   Length;
    UINT16  Handle;
} SMBIOS_HEADER;

typedef struct {
  UINT8 Command;
  UINT8 FieldOffset;
  UINT32 ChangeMask;
  UINT32 ChangeValue;
  UINT16 DataLength;
  SMBIOS_HEADER StructureHeader;
  UINT8 StructureData[1];
} PNP_52_DATA_BUFFER;

typedef struct {
  UINT8               SubFun;
  PNP_52_DATA_BUFFER  Parameter;
} UPDATE_SMBIOS_PARAMETER;

#define EFI_PNP_52_SIGNATURE  SIGNATURE_32('_','p','n','p')
typedef struct {
  UINTN               Signature;
  LIST_ENTRY          Link;
  SMBIOS_REC_HEADER   header;
  PNP_52_DATA_BUFFER  *pRecord;
} PNP_52_RECORD;

///
/// Identifies the structure-setting operation to be performed.
///
typedef enum {
  ByteChanged,
  WordChanged,
  DoubleWordChanged,
  AddChanged,
  DeleteChanged,
  StringChanged,
  BlockChanged,
  Reseved
} FUNC52_CMD;

typedef enum {
    UPDATE_UUID = 1,
    UPDATE_SERIAL_NUMBER,
    UPDATE_CHASSIS_ASSET_TAG,
    UPDATE_MODEL_NUMBER,
    UPDATE_BRAND_ID,
 // UPDATE_LOCK_STATUS,
    UPDATE_BASE_BOARD_SERIAL_NUMBER,
    UPDATE_BASE_BOARD_ASSET_TAG,
    UPDATE_CHASSIS_SERIAL_NUMBER
} UPDATE_SMBIOS_TYPE;


typedef struct {
    UINT32    BiosAddr;
    UINT32    Size;
    UINT32    Buffer;
} BIOS_UPDATE_BLOCK_PARAMETER;
typedef struct {
  UINT32        Type;                         // IN, Type of flash area
  UINT32        Offset;                       // OUT, Offset from FD base address or base address for FD_AREA_TYPE_FD
  UINT32        Size;                         // OUT, Byte size of flash area
} FD_AREA_INFO;

#if 0
typedef enum {
  FD_AREA_TYPE_FD,                          // FD_AREA_INFO.Offset return 0
  FD_AREA_TYPE_FVMAIN_COMPACT,
  FD_AREA_TYPE_NVSTORAGE_VARIABLE,
  FD_AREA_TYPE_NVSTORAGE_MICROCODE,
  FD_AREA_TYPE_FVRECOVERY,
  FD_AREA_TYPE_OEM_RESERVED,
  FD_AREA_TYPE_OEM_LOGO,
  FD_AREA_TYPE_ROMHOLE,
  FD_AREA_TYPE_SMBIOS,
  FD_AREA_TYPE_FVRECOVERY2,
  FD_AREA_TYPE_SEC,
  FD_AREA_TYPE_BIOS_MRC
} FD_AREA_TYPE;
#else
typedef enum {
  FD_AREA_TYPE_FD,
  FD_AREA_TYPE_TEST_MENU,
  FD_AREA_TYPE_VDP,
  FD_AREA_TYPE_LOGO,
  FD_AREA_TYPE_NV_STORAGE,
  FD_AREA_TYPE_NV_STORAGE_SPARE,
  FD_AREA_TYPE_MICROCODE,
  FD_AREA_TYPE_FVMAIN,
  FD_AREA_TYPE_SMBIOS,
  FD_AREA_TYPE_RECOVERY2_BACKUP,
  FD_AREA_TYPE_RECOVERY2,
  FD_AREA_TYPE_RAW_DATA,
  FD_AREA_TYPE_RECOVERY,
  FD_AREA_TYPE_LOW_4M,
} FD_AREA_TYPE;

#endif
/*
EFI_CAPSULE_HEADER
  CapsuleGuid         // 16
  HeaderSize          // 4
  Flags               // 4
  CapsuleImageSize    // 4
-------------------------------------------------------------
  (+) PubkeySize      // 4            +28
  (+) SignSize        // 4            +32
  (+) RangeArraySize  // 4            +36

  (+) Pubkey          //              +40
  (+) Sign            //              +40+PubkeySize            align 4
  (+) Range[]         //              +40+PubkeySize+SignSize   align 4
-------------------------------------------------------------
FD                    // 16 align
*/

//extern EFI_GUID gEfiAcpiTableGuid;

typedef struct {
    UINT32               BufferSize;
    EFI_PHYSICAL_ADDRESS Buffer;
} ROM_HOLE_PARAMETER;

typedef struct {
    UINT32               BufferSize;
	UINT32               LogoOffest;
    EFI_PHYSICAL_ADDRESS              Buffer;
    UINT8                 Flag;
} NVLOGO_PARAMETER;

#pragma pack()


EFI_STATUS HandleSmbiosDataRequest(UPDATE_SMBIOS_PARAMETER *SmbiosPtr);
EFI_STATUS AllocDataBuffer();

extern NV_MEDIA_ACCESS_PROTOCOL  *mMediaAccess;




