/** @file

Copyright (c) 2006 - 2018, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  PasswordFormCallback.c

Abstract:
  PasswordFormCallback Setup Rountines

Revision History:


**/


#include <PlatformSetupDxe.h>
#include <Library/BaseCryptLib.h>
#include <Protocol/ByoPlatformSetupProtocol.h>
#include <Protocol/SystemPasswordProtocol.h>

EFI_STATUS
EFIAPI
PasswordFormCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION                   Action,
  IN EFI_QUESTION_ID                      KeyValue,
  IN UINT8                                Type,
  IN EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;  
  EFI_SYSTEM_PASSWORD_PROTOCOL    *SystemPassword = NULL;
  SYSTEM_PASSWORD    PasswordVar;
  static UINTN PasswordState = BROWSER_STATE_VALIDATE_PASSWORD;
  CHAR16    *Password = NULL;  
  EFI_HII_HANDLE    HiiHandle = NULL;    
  SETUP_FORMSET_INFO *SetupFormSet;
  

  if (Action != EFI_BROWSER_ACTION_CHANGING) {
    return EFI_SUCCESS;
  }

  DEBUG((EFI_D_INFO, "PasswordFormCallback A:%X K:%X T:%X\n", Action, KeyValue, Type));  
  
  //
  // Get system password protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiSystemPasswordProtocolGuid,
                  NULL,
                  (VOID**)&SystemPassword
                  );
  if (EFI_ERROR(Status)) {    
    return Status;
  }    	 
  SystemPassword->SetMode(VERIFY_MODE_SETUP);

  //
  // Get password status.
  //  
  if (!HiiGetBrowserData(&gEfiSystemPasswordVariableGuid, SYSTEM_PASSWORD_NAME, sizeof (SYSTEM_PASSWORD), (UINT8*) &PasswordVar)) {
    DEBUG((EFI_D_INFO, "PasswordFormCallback, System Password Not Found.\n"));  
    return EFI_NOT_FOUND;
  }
  //
  // Get Security form hii databas handle.
  //
  SetupFormSet = BYO_FORMSET_INFO_FROM_THIS(This);
  HiiHandle = SetupFormSet->HiiHandle;
  ASSERT(HiiHandle != NULL);
  if (Type == EFI_IFR_TYPE_STRING && Value->string != 0) {
    DEBUG((EFI_D_INFO, "Sting Value :%d. \n", Value->string));
    Password = HiiGetString(HiiHandle, Value->string, NULL);	
  }

  if (NULL == Password && PasswordState == BROWSER_STATE_SET_PASSWORD) {
    PasswordState = BROWSER_STATE_VALIDATE_PASSWORD;
    return EFI_SUCCESS;
  }

  switch (KeyValue) {
  case SEC_KEY_ADMIN_PD:
    switch (PasswordState) {
      case BROWSER_STATE_VALIDATE_PASSWORD:
        if (!SystemPassword->BeHave(PD_ADMIN)) {
	   PasswordState = BROWSER_STATE_SET_PASSWORD;		
          return EFI_SUCCESS;
        }
        if (NULL ==Password) {
          PasswordState = BROWSER_STATE_VALIDATE_PASSWORD;
          return EFI_NOT_READY;
        }
        if (SystemPassword->Verify(PD_ADMIN, Password)) {
	   PasswordState = BROWSER_STATE_SET_PASSWORD;		
          Status = EFI_SUCCESS;
        } else {
          Status = EFI_NOT_READY;
        }
        break;
		
      case BROWSER_STATE_SET_PASSWORD:
        if (*Password != CHAR_NULL) {
          SystemPassword->Write(PD_ADMIN, Password);
          PasswordVar.bHaveAdmin = 1;
          *ActionRequest = EFI_BROWSER_ACTION_REQUEST_RESET;
        } else  {
          SystemPassword->Clear(PD_ADMIN);
          PasswordVar.bHaveAdmin = 0;
          PasswordVar.ChangePopByUser = 1;
          *ActionRequest = EFI_BROWSER_ACTION_REQUEST_RESET;
        }
        PasswordState = BROWSER_STATE_VALIDATE_PASSWORD;
        break;

      default:
        PasswordState = BROWSER_STATE_VALIDATE_PASSWORD;
        break;
    }
    break;
    
  case SEC_KEY_POWER_ON_PD:
    switch (PasswordState) {
      case BROWSER_STATE_VALIDATE_PASSWORD:
        if (!SystemPassword->BeHave(PD_POWER_ON)) {
	   PasswordState = BROWSER_STATE_SET_PASSWORD;		
          return EFI_SUCCESS;
        }
        if (NULL ==Password) {
          PasswordState = BROWSER_STATE_VALIDATE_PASSWORD;
          return EFI_NOT_READY;
        }
        if (SystemPassword->Verify(PD_POWER_ON, Password)) {
	   PasswordState = BROWSER_STATE_SET_PASSWORD;		
          Status = EFI_SUCCESS;
        } else {
          Status = EFI_NOT_READY;
        }
        break;
		
      case BROWSER_STATE_SET_PASSWORD:
        if (*Password != CHAR_NULL) {
          SystemPassword->Write(PD_POWER_ON, Password);
          PasswordVar.bHavePowerOn = 1;
          *ActionRequest = EFI_BROWSER_ACTION_REQUEST_RESET;
        } else  {
          SystemPassword->Clear(PD_POWER_ON);
          PasswordVar.bHavePowerOn = 0;
          PasswordVar.RequirePopOnRestart = 0;
          *ActionRequest = EFI_BROWSER_ACTION_REQUEST_RESET;
        }
        PasswordState = BROWSER_STATE_VALIDATE_PASSWORD;
        break;

      default:
        PasswordState = BROWSER_STATE_VALIDATE_PASSWORD;
        break;
    }
    break;

  case SEC_KEY_CLEAR_USER_PD:
    if (SystemPassword->BeHave(PD_POWER_ON)) {
      SystemPassword->Clear (PD_POWER_ON);
      PasswordVar.bHavePowerOn = 0;
	PasswordVar.RequirePopOnRestart = 0;
	*ActionRequest = EFI_BROWSER_ACTION_REQUEST_RESET;
    }
    break;
    
  default:
    break;    
  }

  //
  // Restore password to browser.
  //
  HiiSetBrowserData (
            &gEfiSystemPasswordVariableGuid, 
            SYSTEM_PASSWORD_NAME, 
            sizeof (SYSTEM_PASSWORD), 
            (UINT8 *)&PasswordVar, 
            NULL
            );

  return Status;
}
