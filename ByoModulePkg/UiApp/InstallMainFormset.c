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


//
// module global data.
//
HII_VENDOR_DEVICE_PATH  gHiiByoDevicePath = {
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8) (sizeof (HII_VENDOR_DEVICE_PATH_NODE)),
          (UINT8) ((sizeof (HII_VENDOR_DEVICE_PATH_NODE)) >> 8)
        }
      },
      EFI_BYO_IFR_GUID,
    },
    0,
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH
    }
  }
};

EFI_STATUS
InstallFormset (
  SETUP_FORMSET_INFO    * SetupFormSet
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  LIST_ENTRY    *Link;
  SETUP_DYNAMIC_ITEM    *SetupItem;


  DEBUG((EFI_D_INFO, "%a\n", __FUNCTION__));
  
  if (SetupFormSet == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SetupFormSet->VendorDevicePath = AllocateCopyPool(sizeof (HII_VENDOR_DEVICE_PATH), &gHiiByoDevicePath);
  if (SetupFormSet->VendorDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }  
  //
  // Use memory address as unique ID to distinguish from different device paths
  //
  SetupFormSet->VendorDevicePath->UniqueId = (UINTN)SetupFormSet->VendorDevicePath;
  
  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  CopyMem(&SetupFormSet->SetupConfig, &gSetupFormSetConfig, sizeof (EFI_HII_CONFIG_ACCESS_PROTOCOL));
  Status = gBS->InstallMultipleProtocolInterfaces (
                    &SetupFormSet->DriverHandle,
                    &gEfiDevicePathProtocolGuid,
                    SetupFormSet->VendorDevicePath,
                    &gEfiHiiConfigAccessProtocolGuid,
                    &SetupFormSet->SetupConfig,
                    NULL
                    );
  DMSG((EFI_D_ERROR, "InstallFormset, Install ConfigAccess :%r.\n", Status));
  if (EFI_ERROR (Status)) {
    return Status;  	
  }

  //
  // Publish All HII data.
  //
  SetupFormSet->HiiHandle = HiiAddPackages (
                                       &SetupFormSet->FormSetInfo.FormSetGuid,
                                       SetupFormSet->DriverHandle,
                                       SetupFormSet->FormSetInfo.IfrPack,
                                       SetupFormSet->FormSetInfo.StrPack,
                                       NULL
                                       );
  DMSG((EFI_D_ERROR, "InstallFormset, New Hii Handle :0x%x.\n", SetupFormSet->HiiHandle));
  if (SetupFormSet->HiiHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }
	
  //
  //Add Dynamic Item  to Main formset.
  //  
  SetupItem = NULL;
  Link = GetFirstNode (&gByoSetup->DynamicItemList);
  while (!IsNull (&gByoSetup->DynamicItemList, Link)) {
    SetupItem = BYO_SETUP_ITEM_FROM_LINK (Link);
    if (SetupFormSet->FormSetInfo.Class == SetupItem->DynamicItem.Class) {
#if 0      
      //
      //Add Common Function.
      //
      if (SetupItem->DynamicItem.RefreshLabel == LABEL_CHANGE_LANGUAGE) {    
        Status = UpdateLanguageSettingItem (
            SetupFormSet->HiiHandle, 
            &SetupFormSet->FormSetInfo.FormSetGuid, 
            SetupItem->DynamicItem.FormId, 
            SetupItem->DynamicItem.Prompt,
            SetupItem->DynamicItem.Help
            );
        SetupItem->Key = KEY_CHANGE_LANGUAGE;
        DMSG((EFI_D_ERROR, "InstallFormset, Update Change Language Item :0x%x-%r.\n", SetupItem->DynamicItem.RefreshLabel, Status));
      }
      //
      //Add Third-part device Dynamic Formset.
      //
      if (SetupItem->DynamicItem.RefreshLabel == LABEL_DYNAMIC_FORMSET) {    
        Status = AddDynamicFormset (
            SetupFormSet->HiiHandle, 
            &SetupFormSet->FormSetInfo.FormSetGuid, 
            SetupItem->DynamicItem.FormId, 
            SetupItem->DynamicItem.Prompt,
            SetupItem->DynamicItem.Help
            );
        DMSG((EFI_D_ERROR, "InstallFormset, Add Third-part Dynamic FormSet :0x%x-%r.\n", SetupItem->DynamicItem.RefreshLabel, Status));
      }
      //
      //Add Boot Order.
      //
      if (SetupItem->DynamicItem.RefreshLabel == LABEL_CHANGE_BOOT_ORDER) {    
        Status = AddBootOrderFormset (
            SetupFormSet->HiiHandle, 
            &SetupFormSet->FormSetInfo.FormSetGuid, 
            SetupItem->DynamicItem.FormId, 
            SetupItem->DynamicItem.Prompt,
            SetupItem->DynamicItem.Help
            );
        DMSG((EFI_D_ERROR, "InstallFormset, Add Boot Order :0x%x-%r.\n", SetupItem->DynamicItem.RefreshLabel, Status));
      }
#endif
	
    }
    Link = GetNextNode (&gByoSetup->DynamicItemList, Link);
  }
	
  return Status;
}



EFI_STATUS
AddBaseFormset (
  VOID 
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;
  LIST_ENTRY            *Link;
  SETUP_FORMSET_INFO    *SetupFormSet = NULL;
  
  Link = GetFirstNode (&gByoSetup->MainFormsetList);
  while (!IsNull (&gByoSetup->MainFormsetList, Link)) {
    SetupFormSet = BYO_FORMSET_INFO_FROM_LINK (Link);
    Status = InstallFormset(SetupFormSet);	
    Link = GetNextNode (&gByoSetup->MainFormsetList, Link);
  }
  return Status;
}


VOID
RemoveHiiResource ( 
  VOID 
  )
{
  LIST_ENTRY    *Link;
  SETUP_FORMSET_INFO    * SetupFormSet;
  SETUP_DYNAMIC_ITEM    *SetupItem;
  
  DMSG((EFI_D_ERROR, "\n RemoveHiiResource, \n"));
  SetupFormSet = NULL;
  Link = GetFirstNode (&gByoSetup->MainFormsetList);
  while (!IsNull (&gByoSetup->MainFormsetList, Link)) {
    SetupFormSet = BYO_FORMSET_INFO_FROM_LINK (Link);
    Link = GetNextNode (&gByoSetup->MainFormsetList, Link);

    DMSG((EFI_D_ERROR, "RemoveHiiResource, HiiHandle :0x%x, Class :%d. \n", SetupFormSet->HiiHandle, SetupFormSet->FormSetInfo.Class));

    if (SetupFormSet->DriverHandle != NULL) {
      gBS->UninstallMultipleProtocolInterfaces (
               SetupFormSet->DriverHandle,
               &gEfiDevicePathProtocolGuid,
               SetupFormSet->VendorDevicePath,
               &gEfiHiiConfigAccessProtocolGuid,
               &SetupFormSet->SetupConfig,
               NULL
               );
      SetupFormSet->DriverHandle = NULL;
    }

    if (SetupFormSet->VendorDevicePath != NULL) {
      FreePool (SetupFormSet->VendorDevicePath);
      SetupFormSet->VendorDevicePath = NULL;
    }

    if (SetupFormSet->HiiHandle != NULL) {
      HiiRemovePackages (SetupFormSet->HiiHandle);
      SetupFormSet->HiiHandle = NULL;
    }

    RemoveEntryList (&SetupFormSet->Link);
    FreePool (SetupFormSet);
  }


  DMSG((EFI_D_ERROR, "RemoveHiiResource, Remove Dynamic Item, \n"));
  SetupItem = NULL;
  Link = GetFirstNode (&gByoSetup->DynamicItemList);
  while (!IsNull (&gByoSetup->DynamicItemList, Link)) {
    SetupItem = BYO_SETUP_ITEM_FROM_LINK (Link);
    Link = GetNextNode (&gByoSetup->DynamicItemList, Link);

    RemoveEntryList (&SetupItem->Link);
    FreePool (SetupItem);
  }

  DMSG((EFI_D_ERROR, "RemoveHiiResource, End. \n"));
  return;
}

