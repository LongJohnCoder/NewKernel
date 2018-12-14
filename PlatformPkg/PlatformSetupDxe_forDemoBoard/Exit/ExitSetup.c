/** @file

Copyright (c) 2006 - 2018, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  ExitSetup.c

Abstract:
  Exit Setup Rountines

Revision History:

$END--------------------------------------------------------------------

**/


#include <PlatformSetupDxe.h>
#include <Protocol/SetupSaveNotify.h>






void
SaveUserDefault (
  EDKII_FORM_BROWSER_EXTENSION2_PROTOCOL  *BrowserExtension
  )
{
  EFI_STATUS                      Status;
  BOOLEAN                         IsSucceed; 
  SETUP_DATA                      SetupData;	
  UINTN                           VariableSize;  


  DEBUG((EFI_D_INFO, "ExitSetup, SaveUserDefault(),\n"));
  
  Status = BrowserExtension->ExecuteAction (BROWSER_ACTION_SAVE_USER_DEFAULT, 0);
  if (!EFI_ERROR(Status)) {
  	VariableSize = sizeof(SETUP_DATA);
    IsSucceed = HiiGetBrowserData (
                  &gPlatformSetupVariableGuid, 
                  L"Setup",
                  VariableSize, 
                  (UINT8*)&SetupData
                  );
    
    if(IsSucceed) {                      
      Status = gRT->SetVariable (
                  L"SetupUserDefault",
                  &gPlatformSetupVariableGuid,
                  EFI_VARIABLE_NON_VOLATILE | 
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | 
                  EFI_VARIABLE_RUNTIME_ACCESS,
                  VariableSize,
                  &SetupData
                  );
      ASSERT_EFI_ERROR (Status);
    }

//  BrowserExtension->SaveNotify (SetupSaveNotifyTypeSaveUserDefault);
  }
  
}

void
LoadUserDefault (
  EDKII_FORM_BROWSER_EXTENSION2_PROTOCOL  *BrowserExtension
  )
{
  SETUP_DATA            SetupData;		
  UINTN                 VariableSize;
  EFI_STATUS            Status;  


  DEBUG((EFI_D_INFO, "ExitSetup, LoadUserDefault(),\n"));

  Status = BrowserExtension->ExecuteAction (BROWSER_ACTION_LOAD_USER_DEFAULT, 0);
  if (!EFI_ERROR(Status)) {
    VariableSize = sizeof (SETUP_DATA);
    Status = gRT->GetVariable (
                    L"SetupUserDefault",
                    &gPlatformSetupVariableGuid,
                    NULL,
                    &VariableSize,
                    &SetupData
                    );
  
    if (!EFI_ERROR(Status)) {
      HiiSetBrowserData (
        &gPlatformSetupVariableGuid,
        L"Setup",
        VariableSize,
        (UINT8 *) &SetupData,
        NULL
      );
    }  
    
//    BrowserExtension->SaveNotify (SetupSaveNotifyTypeLoadUserDefault);
  }
}









// ----------------------------------------------------------------------------
// Procedure: ExitFormCallBackFunction
//
// Description: This function validates the Flex Ratio setup value
//
// Input:
//       EFI_FORM_CALLBACK_PROTOCOL   *This
//       UINT16                       KeyValue
//       EFI_IFR_DATA_ARRAY           *Data,
//       EFI_HII_CALLBACK_PACKET      **Packet
//
// Output:
//        EFI_SUCCESS
//
// ----------------------------------------------------------------------------
//
EFI_STATUS
EFIAPI
ExitFormCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION                   Action,
  IN EFI_QUESTION_ID                      KeyValue,
  IN UINT8                                Type,
  IN EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
  )
{
  EFI_STATUS                              Status;
  EDKII_FORM_BROWSER_EXTENSION2_PROTOCOL  *BrowserExtension;


  if (Action != EFI_BROWSER_ACTION_CHANGING) {
    return EFI_SUCCESS;
  }

  DEBUG((EFI_D_INFO, "%a K:%X\n", __FUNCTION__, KeyValue));
  
  Status = EFI_SUCCESS;

  Status = gBS->LocateProtocol (
                  &gEdkiiFormBrowserEx2ProtocolGuid,
                  NULL,
                  (void**)&BrowserExtension
                  );
  ASSERT(!EFI_ERROR(Status));

  switch (KeyValue) {

  case KEY_SAVE_AND_EXIT_VALUE:
    Status = BrowserExtension->ExecuteAction (BROWSER_ACTION_SUBMIT | BROWSER_ACTION_EXIT, 0);
    break;

  case KEY_DISCARD_AND_EXIT_VALUE:
    Status = BrowserExtension->ExecuteAction (BROWSER_ACTION_DISCARD | BROWSER_ACTION_EXIT, 0);
    break;
        
  case KEY_RESTORE_DEFAULTS_VALUE:
    Status = BrowserExtension->ExecuteAction (BROWSER_ACTION_DEFAULT, 0); 
    break;    

  case KEY_SAVE_USER_DEFAULTS_VALUE:
    SaveUserDefault (BrowserExtension);
    break;
    
  case KEY_RESTORE_USER_DEFAULTS_VALUE:
    LoadUserDefault (BrowserExtension);
    break;

  default:
    break;
    
  }

  return EFI_SUCCESS;
}



