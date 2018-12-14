/*++

Copyright (c) 2010 - 2015, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:

Abstract:
  Platform configuration setup.

Revision History:


--*/

#ifndef _PLATFORM_SETUP_H_
#define _PLATFORM_SETUP_H_

#define EFI_FORWARD_DECLARATION(x)

#include <Protocol/Variable.h>
#include <Protocol/VariableWrite.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiString.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/SmbusHc.h>
#include <Protocol/AcpiSupport.h>
#include <Protocol/SerialIo.h>
#include <Protocol/Cpu.h>
#include <Protocol/MpService.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/ByoFormBrowserEx.h>
#include <Protocol/ByoFormSetManager.h>
#include <Protocol/SetupSaveNotify.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <library/DxeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PerformanceLib.h>
#include <Library/IoLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/DevicePathLib.h>
#include <Library/Pcdlib.h>
#include <Guid/GlobalVariable.h>
#include <Guid/EventGroup.h>
#include <Guid/HobList.h>
#include <IndustryStandard/Acpi.h>
#include <Library/BaseLib.h>
#include <Uefi/UefiInternalFormRepresentation.h>
#include <SetupVariable.h>
#include "PlatformDefinition.h"
#include "SetupItemId.h"
#include "SetupCallback.h"
#include "SetupFormInit.h"
#include <Guid/MdeModuleHii.h>
#include <Protocol/ByoPlatformSetupProtocol.h>
#include <Protocol/FormBrowserEx2.h>
#include <ByoPlatformSetupConfig.h>
#include <Guid/ByoSetupFormsetGuid.h>


extern UINT8  PlatformSetupDxeStrings[];
extern UINT8  FormsetMainBin[];
extern UINT8  FormsetAdvancedBin[];
extern UINT8  FormsetSecurityBin[];
extern UINT8  FormsetExitBin[];
extern UINT8  FormsetDevicesBin[];
extern UINT8  FormsetPowerBin[];


/**
  Install Byo Setup Protocol interface.

**/
EFI_STATUS
InstallByoSetupProtocol (
  VOID
  );

extern EFI_BYO_PLATFORM_SETUP_PROTOCOL    *mByoSetup;

EFI_STATUS
EFIAPI
HiiExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL    *This,
  IN  CONST EFI_STRING    Request,
  OUT EFI_STRING    *Progress,
  OUT EFI_STRING    *Results
  );


EFI_STATUS
EFIAPI
HiiRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL    *This,
  IN  CONST EFI_STRING    Configuration,
  OUT EFI_STRING    *Progress
  );

EFI_STATUS
FormCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL    *This,
  IN EFI_BROWSER_ACTION    Action,
  IN EFI_QUESTION_ID    KeyValue,
  IN UINT8    Type,
  IN EFI_IFR_TYPE_VALUE    *Value,
  OUT EFI_BROWSER_ACTION_REQUEST    *ActionRequest
);

EFI_STATUS
InitializeForm (
  VOID 
  );
  
//IVS-20180417 For CPU Temperature Show
EFI_STATUS
GetTmpHandle(
 // IN EFI_EVENT    Event,
 // IN VOID         *Context
 OUT UINT8   *mTemp
 );
 
 VOID 
InitString (
  EFI_HII_HANDLE    HiiHandle, 
  EFI_STRING_ID     StrRef, 
  CHAR16            *sFormat, ...
  );

#endif
