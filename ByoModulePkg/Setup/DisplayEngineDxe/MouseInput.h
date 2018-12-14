/** @file
  FormDiplay protocol to show Form

Copyright (c) 2013 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __MOUSE_INPUT_H__
#define __MOUSE_INPUT_H__

#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Guid/MdeModuleHii.h>
#include <Protocol/SetupMouseProtocol.h>

#include "FormDisplay.h"


typedef enum {
  SETUP_AREA_TITLE,
  SETUP_AREA_MENU,
  SETUP_AREA_HOTKEY,
  SETUP_AREA_SCROLL_BAR,
  SETUP_AREA_HELP,
  SETUP_AREA_MAX
} SETUP_AREA_TYPE;

SETUP_AREA_TYPE
GetSetupEreaType (
  MOUSE_ACTION_INFO    *Mouse,
  UINTN    LayoutStyle
  );
  
LIST_ENTRY *
MouseGetMenuOption ( 
  IN MOUSE_ACTION_INFO *Mouse,
  IN LIST_ENTRY    *TopOfScreen
  );

BOOLEAN
MouseGotoFormset (
  IN MOUSE_ACTION_INFO *Mouse
  );

#endif
