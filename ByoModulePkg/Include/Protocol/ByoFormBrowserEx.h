/** @file
  Extension Form Browser Protocol provides the services that can be used to
  register the different hot keys for the standard Browser actions described in UEFI specification.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __BYO_FORM_BROWSER_EXTENSION_H__
#define __BYO_FORM_BROWSER_EXTENSION_H__

#include <Protocol/FormBrowserEx.h>
#include <Protocol/SetupSaveNotify.h>

#define BYO_FORM_BROWSER_EXTENSION_PROTOCOL_GUID  \
  { 0xcf49d6f3, 0x35bb, 0x488c, { 0x88, 0xab, 0x47, 0x0, 0x92, 0x2f, 0x3a, 0xc4 } }

typedef struct _EFI_BYO_FORM_BROWSER_EXTENSION_PROTOCOL   EFI_BYO_FORM_BROWSER_EXTENSION_PROTOCOL;

//
// Define Byo's Browser actions Which should begin from BIT10 to up,
// and Exclude BIT16 and BIT17.
//#define BROWSER_ACTION_NONE         BIT16
//#define BROWSER_ACTION_FORM_EXIT    BIT17
// 
#define BROWSER_ACTION_GOTO_BYO         BIT10
#define BROWSER_ACTION_MOVE_UP         BIT11
#define BROWSER_ACTION_MOVE_DOWN         BIT12
#define BROWSER_ACTION_SAVE_USER_DEFAULT         BIT13
#define BROWSER_ACTION_LOAD_USER_DEFAULT         BIT14


/**
  Check whether the browser data has been modified.

  @retval TRUE        Browser data is modified.
  @retval FALSE       No browser data is modified.

**/
typedef
BOOLEAN
(EFIAPI *IS_BROWSER_DATA_MODIFIED) (
  VOID
  );


/**
  Execute the action requested by the Action parameter.

  @param[in] Action     Execute the request action.
  @param[in] DefaultId  The default Id info when need to load default value.

  @retval EFI_SUCCESS              Execute the request action succss.

**/
typedef
EFI_STATUS
(EFIAPI *EXECUTE_ACTION) (
  IN UINT32        Action,
  IN UINT16        DefaultId
  );

/**
  Oem Platform reset function.

**/
typedef
VOID
(EFIAPI *OEM_PLATFORM_RESET) (
  VOID
  );

/**
  Trigger notify protocol function.

  @param  SETUP_SAVE_NOTIFY_TYPE Notify type.

  @retval EFI_STATUS return status from the nofify function.

**/
typedef
EFI_STATUS
(EFIAPI *SETUP_SAVE_NOTIFY) (
  IN SETUP_SAVE_NOTIFY_TYPE Type 
  );


struct _EFI_BYO_FORM_BROWSER_EXTENSION_PROTOCOL {
  IS_BROWSER_DATA_MODIFIED  IsBrowserDataModified;
  EXECUTE_ACTION            ExecuteAction;
  OEM_PLATFORM_RESET        PlatformReset;
  SETUP_SAVE_NOTIFY    SaveNotify;
};

extern EFI_GUID gEfiByoFormBrowserExProtocolGuid;



#endif

