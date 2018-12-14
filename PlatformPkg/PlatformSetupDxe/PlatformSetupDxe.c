/*++

Copyright (c) 2006 - 2018, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:

Abstract:
  Implement of platform setup protocol.

Revision History:


--*/


#include "PlatformSetupDxe.h"

SETUP_DATA                         gSetupData;
STATIC SETUP_SAVE_NOTIFY_PROTOCOL  gSetupSaveNotify;


FORMSET_INFO    gSetupFormSets[] = {
  {FORMSET_MAIN,       FORMSET_GUID_MAIN,       FormsetMainBin,     PlatformSetupDxeStrings, {HiiExtractConfig, HiiRouteConfig, FormCallback}},
  {FORMSET_ADVANCE,    FORMSET_GUID_ADVANCE,    FormsetAdvancedBin, PlatformSetupDxeStrings, {HiiExtractConfig, HiiRouteConfig, FormCallback}},
  {FORMSET_DEVICE,     FORMSET_GUID_DEVICE,     FormsetDevicesBin,  PlatformSetupDxeStrings, {HiiExtractConfig, HiiRouteConfig, FormCallback}},
  {FORMSET_POWER,      FORMSET_GUID_POWER,      FormsetPowerBin,    PlatformSetupDxeStrings, {HiiExtractConfig, HiiRouteConfig, FormCallback}},
  {FORMSET_SECURITY,   FORMSET_GUID_SECURITY,   FormsetSecurityBin, PlatformSetupDxeStrings, {HiiExtractConfig, HiiRouteConfig, FormCallback}},
  {FORMSET_EXIT,       FORMSET_GUID_EXIT,       FormsetExitBin,     PlatformSetupDxeStrings, {HiiExtractConfig, HiiRouteConfig, FormCallback}},
};

FORM_CALLBACK_ITEM	gFormCallback[] = {

  {FORMSET_SECURITY,    SEC_KEY_ADMIN_PD,                PasswordFormCallback},  
  {FORMSET_SECURITY,    SEC_KEY_POWER_ON_PD,             PasswordFormCallback},  
  {FORMSET_SECURITY,    SEC_KEY_CLEAR_USER_PD,           PasswordFormCallback},  

  {FORMSET_EXIT,        KEY_SAVE_AND_EXIT_VALUE  ,       ExitFormCallback},
  {FORMSET_EXIT,        KEY_DISCARD_AND_EXIT_VALUE,      ExitFormCallback},
  {FORMSET_EXIT,        KEY_RESTORE_DEFAULTS_VALUE,      ExitFormCallback},
  {FORMSET_EXIT,        KEY_SAVE_USER_DEFAULTS_VALUE,    ExitFormCallback},
  {FORMSET_EXIT,        KEY_RESTORE_USER_DEFAULTS_VALUE, ExitFormCallback},
  
  {FORMSET_DEVICE,      KEY_VALUE_PCIERST,             DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_PCIERP ,	           DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_PCIE_PE0 ,	         DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_PCIE_PE1 ,			     DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_PCIE_PE2 ,	         DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_PCIE_PE3 ,	         DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_PCIE_PE4 ,	         DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_PCIE_PE5 ,	         DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_PCIE_PE6 ,	         DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_PCIE_PE7 ,	         DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_IOE_PECTL ,	         DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_IOE_PECTL_PEA0 ,	         DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_IOE_PECTL_PEA1 ,	         DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_IOE_PECTL_PEA2 ,	         DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_IOE_PECTL_PEA3 ,	         DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_IOE_PECTL_PEA4 ,	         DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_IOE_PECTL_PEB0 ,	         DevicesFormCallback},
  {FORMSET_DEVICE,	    KEY_VALUE_IOE_PECTL_PEB1 ,	         DevicesFormCallback},
  
  
  {FORMSET_POWER,	      KEY_C4P_CONTROL ,	             PowerFormCallback},
  
  {FORMSET_ADVANCE,	    KEY_VALUE_NBSPE ,		           AdvancedFormCallback},
  {FORMSET_ADVANCE,     KEY_VALUE_SBSPE , 		         AdvancedFormCallback},
  {FORMSET_ADVANCE,	    KEY_VALUE_IOVEN ,		           AdvancedFormCallback},
  {FORMSET_ADVANCE,     KEY_VALUE_IOVQIEN , 		       AdvancedFormCallback},    
};



FORM_INIT_ITEM gFormInit[] = {
  {FORMSET_MAIN,    MainFormInit},
  {FORMSET_ADVANCE, AdvanceFormInit},
  {FORMSET_DEVICE,  DeviceFormInit},  
};



EFI_STATUS
EFIAPI
HiiExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL    *This,
  IN  CONST EFI_STRING    Request,
  OUT EFI_STRING    *Progress,
  OUT EFI_STRING    *Results
  )
{
  EFI_STATUS    Status;
  SETUP_FORMSET_INFO    *SetupFormSet;
  EFI_STRING    ConfigRequestHdr;
  EFI_STRING    ConfigRequest;
  BOOLEAN    AllocatedRequest;
  UINTN    Size;
  UINTN    BufferSize;
  VOID    *SystemConfigPtr;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  SetupFormSet = BYO_FORMSET_INFO_FROM_THIS (This);
  DMSG((EFI_D_ERROR, "\n PlatformSetupDxe, HiiExtractConfig(), HiiHandle :0x%x, Class :%d. \n", SetupFormSet->HiiHandle, SetupFormSet->FormSetInfo.Class));

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &gPlatformSetupVariableGuid, PLATFORM_SETUP_VARIABLE_NAME)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest = NULL;
  Size = 0;
  AllocatedRequest = FALSE;
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&gPlatformSetupVariableGuid, PLATFORM_SETUP_VARIABLE_NAME, SetupFormSet->DriverHandle);
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    BufferSize = sizeof (SETUP_DATA);
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }
  SystemConfigPtr = GetVariable(PLATFORM_SETUP_VARIABLE_NAME, &gPlatformSetupVariableGuid);

  if (SystemConfigPtr == NULL) {
    ZeroMem(&gSetupData, sizeof(SETUP_DATA));
  } else {
    CopyMem(&gSetupData, SystemConfigPtr, sizeof(SETUP_DATA));
    FreePool(SystemConfigPtr);
  }
  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8*)&gSetupData,
                                sizeof (SETUP_DATA),
                                Results,
                                Progress
                                );
  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }
  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}



EFI_STATUS
EFIAPI
HiiRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL    *This,
  IN  CONST EFI_STRING    Configuration,
  OUT EFI_STRING    *Progress
  )
{
  SETUP_FORMSET_INFO    *SetupFormSet;

  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Configuration;

  SetupFormSet =  BYO_FORMSET_INFO_FROM_THIS (This);
  DEBUG((EFI_D_ERROR, "\n PlatformSetupDxe, HiiRouteConfig(), HiiHandle :0x%x, Class :%d. \n", SetupFormSet->HiiHandle, SetupFormSet->FormSetInfo.Class));

  if (!HiiIsConfigHdrMatch (Configuration, &gPlatformSetupVariableGuid, PLATFORM_SETUP_VARIABLE_NAME)) {
    return EFI_NOT_FOUND;
  }

  *Progress = Configuration + StrLen (Configuration);
  if (!HiiGetBrowserData (&gPlatformSetupVariableGuid, PLATFORM_SETUP_VARIABLE_NAME, sizeof(SETUP_DATA), (UINT8*)&gSetupData)) {
    //
    // FakeNvData can't be got from SetupBrowser, which doesn't need to be set.
    //
    return EFI_SUCCESS;
  }

  gRT->SetVariable(
    PLATFORM_SETUP_VARIABLE_NAME,
    &gPlatformSetupVariableGuid,
    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof(SETUP_DATA),
    &gSetupData
 );

  return EFI_SUCCESS;
}



EFI_STATUS
FormCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL    *This,
  IN EFI_BROWSER_ACTION                      Action,
  IN EFI_QUESTION_ID                         KeyValue,
  IN UINT8                                   Type,
  IN EFI_IFR_TYPE_VALUE                      *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  EFI_STATUS Status;
  UINTN    Index;
  SETUP_FORMSET_INFO    *SetupFormSet;

  SetupFormSet =  BYO_FORMSET_INFO_FROM_THIS (This);
//DEBUG((EFI_D_INFO, "Hii:%X, Class:%X\n", SetupFormSet->HiiHandle, SetupFormSet->FormSetInfo.Class));
  
  Status = EFI_SUCCESS;
  for (Index = 0; Index < (sizeof(gFormCallback) / sizeof(FORM_CALLBACK_ITEM)); Index++) {
    if ((gFormCallback[Index].Class == SetupFormSet->FormSetInfo.Class) &&
      ((gFormCallback[Index].Key == KeyValue) || (gFormCallback[Index].Key == KEY_UNASSIGN_GROUP))) {
      Status = gFormCallback[Index].Callback (This, Action, KeyValue, Type, Value, ActionRequest);
    }
  }
  return Status;
}







EFI_STATUS
InitializeForm (
  VOID 
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  SETUP_FORMSET_INFO    * SetupFormSet = NULL;
  LIST_ENTRY    *Link;
  UINTN    Index;

  //
  // Initialize Forms.
  // 
  DMSG((EFI_D_ERROR, "\n InitializeForm, \n"));
  Link = GetFirstNode (&mByoSetup->MainFormsetList);
  while (!IsNull (&mByoSetup->MainFormsetList, Link)) {
    SetupFormSet = BYO_FORMSET_INFO_FROM_LINK (Link);
    Link = GetNextNode (&mByoSetup->MainFormsetList, Link);

    for (Index = 0; Index < (sizeof(gFormInit) / sizeof(FORM_INIT_ITEM)); Index++) {
      if (SetupFormSet->FormSetInfo.Class == gFormInit[Index].Class && SetupFormSet->HiiHandle != NULL) {
        Status = gFormInit[Index].FormInit(SetupFormSet->HiiHandle);
        DMSG((EFI_D_ERROR, "InitializeForm, Form :%d-%r. \n", SetupFormSet->FormSetInfo.Class, Status));
      }
    }
  }
  DMSG((EFI_D_ERROR, "InitializeForm, End.\n"));  
  return Status;  
}


VOID
SetupEnterCallback (
  IN EFI_EVENT Event,
  IN VOID     *Context
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  UINTN         Index;
  VOID          *Interface;


  Status = gBS->LocateProtocol(&gEfiSetupEnterGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }
  gBS->CloseEvent(Event);

  for (Index = 0; Index < (sizeof(gSetupFormSets) / sizeof(FORMSET_INFO)); Index++) {
    Status = mByoSetup->AddFormset(mByoSetup, (FORMSET_INFO *)&gSetupFormSets[Index]);
  }
}

EFI_STATUS
PlatformLoadDefault (
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  return EFI_SUCCESS;
}


STATIC SETUP_SAVE_NOTIFY_PROTOCOL  gSetupSaveNotify;


EFI_STATUS
PlatformSetupEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  VOID             *Registration;
  EFI_STATUS       Status;


  InstallByoSetupProtocol();
  
  ZeroMem(&gSetupSaveNotify, sizeof(SETUP_SAVE_NOTIFY_PROTOCOL));
  gSetupSaveNotify.LoadDefault = PlatformLoadDefault;
  Status = gBS->InstallProtocolInterface (
                  &gSetupSaveNotify.DriverHandle,
                  &gSetupSaveNotifyProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gSetupSaveNotify
                  );

  EfiCreateProtocolNotifyEvent (
    &gEfiSetupEnterGuid,
    TPL_CALLBACK,
    SetupEnterCallback,
    NULL,
    &Registration
    );

  return EFI_SUCCESS;
}


