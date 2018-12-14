/*++

Copyright (c) 2010 - 2018, Byosoft Corporation.<BR>
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

EFI_BYO_PLATFORM_SETUP_PROTOCOL    *mByoSetup = NULL;

VOID
CheckDefaultSetupVariable (VOID )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Link;
  SETUP_FORMSET_INFO    * SetupFormSet = NULL;
  UINTN    Size;
  BOOLEAN    IsSetDefault;
  BOOLEAN    ActionFlag;
  SETUP_DATA    SetupData;
  EFI_STRING    ConfigRequest;

  DMSG((EFI_D_ERROR, "\n CheckDefaultSetupVariable(),\n"));
  //
  // Initialize the default vaule of setup variable "Setup" if it doesn't exist.
  //
  Size = sizeof (SETUP_DATA);
  Status = gRT->GetVariable (
                  PLATFORM_SETUP_VARIABLE_NAME,
                  &gPlatformSetupVariableGuid,
                  NULL,
                  &Size,
                  &SetupData
                  );
  if (Status == EFI_NOT_FOUND) {
    ZeroMem(&SetupData, sizeof(SetupData));
     gRT->SetVariable (
                  PLATFORM_SETUP_VARIABLE_NAME,
                  &gPlatformSetupVariableGuid,
                  PLATFORM_SETUP_VARIABLE_FLAG,
                  Size,
                  &SetupData
                  );
  }

  Link = GetFirstNode (&mByoSetup->MainFormsetList);
  while (!IsNull (&mByoSetup->MainFormsetList, Link)) {
    SetupFormSet = BYO_FORMSET_INFO_FROM_LINK (Link);
    DMSG((EFI_D_ERROR, "CheckDefaultSetupVariable, HiiHandle :0x%x, Class :%d.\n", SetupFormSet->HiiHandle, SetupFormSet->FormSetInfo.Class));

    ConfigRequest = HiiConstructConfigHdr (
                               &gPlatformSetupVariableGuid,
                               PLATFORM_SETUP_VARIABLE_NAME,
                               SetupFormSet->DriverHandle
                               );
    ASSERT (ConfigRequest != NULL);
    if(EFI_ERROR(Status)){
      //
      // Get Default EFI variable.
      //		
      IsSetDefault = HiiSetToDefaults (ConfigRequest, EFI_HII_DEFAULT_CLASS_STANDARD);
      DEBUG((EFI_D_ERROR, "CheckDefaultSetupVariable, HiiSetToDefaults :%d.\n", IsSetDefault));
    }else{
      //
      // EFI variable does exist and Validate Current Setting.
      //
      ActionFlag = HiiValidateSettings (ConfigRequest);
      DEBUG((EFI_D_ERROR, "CheckDefaultSetupVariable, HiiValidateSettings :%d.\n", ActionFlag));
    }
    FreePool (ConfigRequest);

    Link = GetNextNode (&mByoSetup->MainFormsetList, Link);
  }
}


EFI_STATUS
InitializeByoMainFormset (
  EFI_BYO_PLATFORM_SETUP_PROTOCOL    *ByoSetup
  )
{
  EFI_STATUS Status;
  EFI_BYO_FORMSET_MANAGER_PROTOCOL    *FormsetManager = NULL;
  
  Status = gBS->LocateProtocol (
                  &gEfiByoFormsetManagerProtocolGuid,
                  NULL,
                  (VOID **) &FormsetManager
                  );
  if (EFI_ERROR(Status) ) {  	
    DEBUG((EFI_D_ERROR, "ByoFormsetManager not found:%r\n", Status));    
    return Status;
  }

  Status =FormsetManager->Insert (FormsetManager, &gEfiFormsetGuidMain);  
  DEBUG((EFI_D_INFO, "Insert Main:%r\n", Status));

  Status =FormsetManager->Insert (FormsetManager, &gEfiFormsetGuidDevices);  
  DEBUG((EFI_D_INFO, "Insert devices:%r\n", Status));
  
  Status =FormsetManager->Insert (FormsetManager, &gEfiFormsetGuidAdvance);    
  DEBUG((EFI_D_INFO, "Insert Advanced:%r\n", Status));

  Status =FormsetManager->Insert (FormsetManager, &gEfiFormsetGuidPower);  
  DEBUG((EFI_D_INFO, "Insert Power:%r\n", Status));
	
  Status =FormsetManager->Insert (FormsetManager, &gEfiFormsetGuidSecurity);
  DEBUG((EFI_D_INFO, "Insert Security:%r\n", Status));

  Status =FormsetManager->Insert (FormsetManager, &gEfiFormsetGuidBoot);  
  DEBUG((EFI_D_INFO, "Insert Boot:%r\n", Status));
  
  Status =FormsetManager->Insert (FormsetManager, &gEfiFormsetGuidExit);  
  DEBUG((EFI_D_INFO, "Insert Exit:%r\n", Status));	

  //
  //Check Variable.
  //
  CheckDefaultSetupVariable ();

  //
  // Initialize Forms.
  // 
  InitializeForm ();
  
  return EFI_SUCCESS;  
}


EFI_STATUS
AddMainFormset (
  IN EFI_BYO_PLATFORM_SETUP_PROTOCOL    *This,
  FORMSET_INFO    *FormSetInfo
  )
{
  LIST_ENTRY    *Link;
  SETUP_FORMSET_INFO    * SetupFormSet;
  SETUP_FORMSET_INFO    *FormSetNode;

  if (FormSetInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  DMSG ((EFI_D_ERROR, "AddMainFormset, FormSetGuid :%g.\n", &FormSetInfo->FormSetGuid));
  
  //
  //Check Formset to avoid duplication.
  //
  Link = GetFirstNode (&This->MainFormsetList);
  while (!IsNull (&This->MainFormsetList, Link)) {
    SetupFormSet = BYO_FORMSET_INFO_FROM_LINK (Link);
    if (SetupFormSet->FormSetInfo.Class == FormSetInfo->Class) {
      DMSG ((EFI_D_ERROR, "AddMainFormset, Formset already be Added.\n"));
      return EFI_ALREADY_STARTED;
    }
    Link = GetNextNode (&This->MainFormsetList, Link);
  }
  
  //
  // Insert new node.
  //
  FormSetNode = AllocateZeroPool (sizeof (SETUP_FORMSET_INFO));
  if (FormSetNode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FormSetNode->Signature = BYO_FORMSET_LIST_SIGNATURE;

  FormSetNode->FormSetInfo.Class = FormSetInfo->Class;
  CopyGuid (&FormSetNode->FormSetInfo.FormSetGuid, &FormSetInfo->FormSetGuid);
  
  FormSetNode->FormSetInfo.IfrPack = FormSetInfo->IfrPack;
  FormSetNode->FormSetInfo.StrPack = FormSetInfo->StrPack;
  

  FormSetNode->FormSetInfo.FormSetConfig.ExtractConfig = FormSetInfo->FormSetConfig.ExtractConfig;
  FormSetNode->FormSetInfo.FormSetConfig.RouteConfig = FormSetInfo->FormSetConfig.RouteConfig;
  FormSetNode->FormSetInfo.FormSetConfig.Callback = FormSetInfo->FormSetConfig.Callback;

  
  InsertTailList (&This->MainFormsetList, &FormSetNode->Link);
  
  return EFI_SUCCESS;  
}

EFI_STATUS
AddSetupDynamicItem (
  IN EFI_BYO_PLATFORM_SETUP_PROTOCOL    *This,
  DYNAMIC_ITEM    *Item
  )
{
  SETUP_DYNAMIC_ITEM    *SetupItem ;

  if (Item == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  DMSG ((EFI_D_ERROR, "AddSetupDynamicItem, RefreshLabel :0x%x. \n", Item->RefreshLabel));

  //
  // Insert new node.
  //
  SetupItem = AllocateZeroPool (sizeof (SETUP_DYNAMIC_ITEM));
  if (SetupItem == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetupItem->Signature = BYO_SETUP_ITEM_SIGNATURE;

  SetupItem->DynamicItem.Class = Item->Class;
  SetupItem->DynamicItem.FormId = Item->FormId;
  SetupItem->DynamicItem.RefreshLabel = Item->RefreshLabel;  
  SetupItem->DynamicItem.Prompt = Item->Prompt;
  SetupItem->DynamicItem.Help = Item->Help;

  InsertTailList (&This->DynamicItemList, &SetupItem->Link);
  
  return EFI_SUCCESS;  
}


EFI_STATUS
RunByoSetup (
  EFI_BYO_PLATFORM_SETUP_PROTOCOL    *ByoSetup
  )
{

  EFI_STATUS Status;
  EFI_BYO_FORMSET_MANAGER_PROTOCOL    *FormsetManager = NULL;
  
  Status = gBS->LocateProtocol (
                  &gEfiByoFormsetManagerProtocolGuid,
                  NULL,
                  (VOID **) &FormsetManager
                  );
  DMSG((EFI_D_ERROR, "\n RunByoSetup, Locate Formset Manager Protocol :%r.\n", Status));
  if (! EFI_ERROR(Status) ) {
    FormsetManager->Run(FormsetManager, &gEfiFormsetGuidMain);
  }
  
  return Status;  
}




/**
  Install Byo Setup Protocol interface.

**/
EFI_STATUS
InstallByoSetupProtocol (
  VOID
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  EFI_BYO_PLATFORM_SETUP_PROTOCOL    *ByoSetup = NULL;

  DMSG((EFI_D_ERROR, "InstallByoSetupProtocol(),\n"));
  Status = gBS->LocateProtocol (
                  &gEfiByoPlatformSetupGuid,
                  NULL,
                  (VOID**)&ByoSetup
                  );
  if ( ! EFI_ERROR(Status) ) {
    DMSG((EFI_D_ERROR, "InstallByoSetupProtocol(), Byo Setup Already Installed.\n"));
    mByoSetup = ByoSetup;
    return EFI_ALREADY_STARTED;
  }

  ByoSetup = AllocateZeroPool (sizeof(EFI_BYO_FORMSET_MANAGER_PROTOCOL));
  if (ByoSetup == NULL) {
    DMSG((EFI_D_ERROR, "InstallByoSetupProtocol(), Memory Out of Resources.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&ByoSetup->MainFormsetList);
  InitializeListHead (&ByoSetup->DynamicItemList);

  ByoSetup->AddFormset = AddMainFormset;
  ByoSetup->AddDynamicItem = AddSetupDynamicItem;
  ByoSetup->InitializeMainFormset = InitializeByoMainFormset;
  ByoSetup->Run = RunByoSetup;
  
  Status = gBS->InstallProtocolInterface (
                 &ByoSetup->DriverHandle,
                 &gEfiByoPlatformSetupGuid,
                 EFI_NATIVE_INTERFACE,
                 ByoSetup
                 );
  if (!EFI_ERROR(Status)) {
    mByoSetup = ByoSetup;
  }
  
  return Status;
}




