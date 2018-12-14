/** @file
  Byosoft Ui App module is driver for BDS phase.

 Copyright (c) 2010 - 2018, Byosoft Corporation.<BR>
 All rights reserved.This software and associated documentation (if any)
 is furnished under a license and may only be used or copied in
 accordance with the terms of the license. Except as permitted by such
 license, no part of this software or documentation may be reproduced,
 stored in a retrieval system, or transmitted in any form or by any
 means without the express written consent of Byosoft Corporation.

 File Name:

 Abstract:
    Byosoft Ui App module.

 Revision History:

**/
#include "FrontPage.h"

EFI_STATUS
EFIAPI
SetupFormSetExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL    *This,
  IN  CONST EFI_STRING    Request,
  OUT EFI_STRING    *Progress,
  OUT EFI_STRING    *Results
  )
{
  EFI_STATUS     Status;
  LIST_ENTRY    *Link;
  SETUP_FORMSET_INFO    *SetupFormSet;
  SETUP_FORMSET_INFO    *FormSet;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DMSG((EFI_D_ERROR, "\n SetupFormSetExtractConfig, \n"));
  //
  //Invoke Formset's Extracting function registered by Platform Setup. 
  //
  Status = EFI_SUCCESS;
  SetupFormSet =  BYO_FORMSET_INFO_FROM_THIS (This);
  Link = GetFirstNode (&gByoSetup->MainFormsetList);
  while (!IsNull (&gByoSetup->MainFormsetList, Link)) {
    FormSet  = BYO_FORMSET_INFO_FROM_LINK (Link);
    if (SetupFormSet->FormSetInfo.Class == FormSet->FormSetInfo.Class && SetupFormSet->HiiHandle == FormSet->HiiHandle) {

      DMSG((EFI_D_ERROR, "SetupFormSetExtractConfig, HiiHandle :0x%x, Class :%d. \n", SetupFormSet->HiiHandle, SetupFormSet->FormSetInfo.Class));
      Status = SetupFormSet->FormSetInfo.FormSetConfig.ExtractConfig(This, Request, Progress, Results);
    }		
    Link = GetNextNode (&gByoSetup->MainFormsetList, Link);
  }


  return Status;
}




EFI_STATUS
EFIAPI
SetupFormSetRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL    *This,
  IN  CONST EFI_STRING    Configuration,
  OUT EFI_STRING    *Progress
  )
{
  EFI_STATUS     Status;
  LIST_ENTRY    *Link;
  SETUP_FORMSET_INFO    *SetupFormSet;
  SETUP_FORMSET_INFO    *FormSet;

  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SetupFormSet =  BYO_FORMSET_INFO_FROM_THIS (This);
  DMSG((EFI_D_ERROR, "\n SetupFormSetRouteConfig, HiiHandle :0x%x. \n", SetupFormSet->HiiHandle));

  //
  //Invoke Formset's Routing function registered by Platform Setup. 
  //
  Status = EFI_SUCCESS;
  Link = GetFirstNode (&gByoSetup->MainFormsetList);
  while (!IsNull (&gByoSetup->MainFormsetList, Link)) {
    FormSet  = BYO_FORMSET_INFO_FROM_LINK (Link);
    if (SetupFormSet->FormSetInfo.Class == FormSet->FormSetInfo.Class && SetupFormSet->HiiHandle == FormSet->HiiHandle) {

      DMSG((EFI_D_ERROR, "SetupFormSetRouteConfig, HiiHandle :0x%x, Class :%d. \n", SetupFormSet->HiiHandle, SetupFormSet->FormSetInfo.Class));
      Status = SetupFormSet->FormSetInfo.FormSetConfig.RouteConfig (This, Configuration, Progress);
    }		
    Link = GetNextNode (&gByoSetup->MainFormsetList, Link);
  }

  return Status;
}




EFI_STATUS
SetupFormSetCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL    *This,
  IN EFI_BROWSER_ACTION    Action,
  IN EFI_QUESTION_ID    KeyValue,
  IN UINT8    Type,
  IN EFI_IFR_TYPE_VALUE    *Value,
  OUT EFI_BROWSER_ACTION_REQUEST    *ActionRequest
)
{
  EFI_STATUS     Status;
  LIST_ENTRY    *Link;
  SETUP_FORMSET_INFO    *SetupFormSet;
  SETUP_FORMSET_INFO    *FormSet;
  SETUP_DYNAMIC_ITEM    *SetupItem;

  SetupFormSet =  BYO_FORMSET_INFO_FROM_THIS (This);
  DMSG((EFI_D_ERROR, "\n SetupFormSetCallback, HiiHandle :0x%x. \n", SetupFormSet->HiiHandle));

  //
  //Invoke Formset's Callbck function registered by Platform Setup. 
  //
  Status = EFI_SUCCESS;
  Link = GetFirstNode (&gByoSetup->MainFormsetList);
  while (!IsNull (&gByoSetup->MainFormsetList, Link)) {
    FormSet  = BYO_FORMSET_INFO_FROM_LINK (Link);
    if (SetupFormSet->FormSetInfo.Class == FormSet->FormSetInfo.Class && SetupFormSet->HiiHandle == FormSet->HiiHandle) {

      DMSG((EFI_D_ERROR, "SetupFormSetCallback, HiiHandle :0x%x, FormSetInfo.Class :%d. \n", SetupFormSet->HiiHandle, SetupFormSet->FormSetInfo.Class));
      Status = SetupFormSet->FormSetInfo.FormSetConfig.Callback (This, Action, KeyValue, Type, Value, ActionRequest);
    }		
    Link = GetNextNode (&gByoSetup->MainFormsetList, Link);
  }

  //
  //Invoke Callbck of common function.
  //
  SetupItem = NULL;
  Link = GetFirstNode (&gByoSetup->DynamicItemList);
  while (!IsNull (&gByoSetup->DynamicItemList, Link)) {
    SetupItem = BYO_SETUP_ITEM_FROM_LINK (Link);
    if (SetupFormSet->FormSetInfo.Class == SetupItem->DynamicItem.Class && KeyValue == SetupItem->Key) {
      
#if 0
      if (SetupItem->DynamicItem.RefreshLabel == LABEL_CHANGE_LANGUAGE) {
        Status = SetLanguageCallback  (This, Action, KeyValue, Type, Value, ActionRequest);
        DMSG((EFI_D_ERROR, "SetupFormSetCallback, Change Language :%r.\n", Status));
      }
#endif
	
    }
    Link = GetNextNode (&gByoSetup->DynamicItemList, Link);
  }

  
  return Status;
}


EFI_HII_CONFIG_ACCESS_PROTOCOL    gSetupFormSetConfig = {SetupFormSetExtractConfig, SetupFormSetRouteConfig, SetupFormSetCallback};

