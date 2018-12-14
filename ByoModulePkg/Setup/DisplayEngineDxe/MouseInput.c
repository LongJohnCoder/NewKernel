/** @file
Entry and initialization module for the browser.

Copyright (c) 2007 - 2017, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2014, Hewlett-Packard Development Company, L.P.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "MouseInput.h"

extern  EFI_SCREEN_DESCRIPTOR         gScreenDimensions;
extern  CHAR16            gHelpBlockWidth;


BOOLEAN
IsSelectable (
  UI_MENU_OPTION   *MenuOption
  );

UINTN
AdjustDateAndTimePosition (
  IN     BOOLEAN                     DirectionUp,
  IN OUT LIST_ENTRY                  **CurrentPosition
  );

UINTN
LibGetStringWidth (
  IN CHAR16               *String
  );
  
LIST_ENTRY *
MouseGetMenuOption ( 
  IN MOUSE_ACTION_INFO *Mouse,
  IN LIST_ENTRY    *TopOfScreen
  )
{
  LIST_ENTRY    *Link;
  UINTN    Row;
  UI_MENU_OPTION    *Menu;

  if (NULL == TopOfScreen) {
    return NULL;
  }
  //
  // Get Top Row.
  //
  if (0 == gFormData->ByoLayoutStyle) {
    Row = gStatementDimensions.TopRow + SCROLL_ARROW_HEIGHT - 2;
  } else {
    Row = gStatementDimensions.TopRow + SCROLL_ARROW_HEIGHT;
  }  
  
  Menu = NULL;
  for (Link = TopOfScreen; Link != &gMenuOption; Link = Link->ForwardLink) {
    Menu = MENU_OPTION_FROM_LINK (Link);

    if (Mouse->Row >= Row && Mouse->Row < (Row + Menu->Skip)) {      
      if (IsSelectable(Menu)) {	 
	 if (Menu->ThisTag->OpCode->OpCode == EFI_IFR_DATE_OP ||
	 	Menu->ThisTag->OpCode->OpCode == EFI_IFR_TIME_OP) {
	 	
	   AdjustDateAndTimePosition (TRUE, &Link);
	   if (Mouse->Column >= Menu->OptCol && Mouse->Column < (Menu->OptCol + 5)) {
	     Link = Link->ForwardLink;
	     Link = Link->ForwardLink;
	   }
	   if (Mouse->Column < (Menu->OptCol - 1) && Mouse->Column > (Menu->OptCol - 4) ) {
	     Link = Link->ForwardLink;
	   }
	 }
	 return Link;  
      } else {
        return NULL;
      }
    }	
    Row = Row + Menu->Skip;
  }

  return NULL;
}

SETUP_AREA_TYPE
GetSetupEreaType (
  MOUSE_ACTION_INFO    *Mouse,
  UINTN    LayoutStyle
  )
{
  if (NULL == Mouse) {
    return SETUP_AREA_MAX;
  }

  DMSG ((EFI_D_ERROR, "\n GetSetupEreaType, BottomRow :%d, RightColumn :%d.  \n", gScreenDimensions.BottomRow, gStatementDimensions.RightColumn));  
  if (Mouse->Row == (gScreenDimensions.TopRow + 1) || 
    Mouse->Row == (gScreenDimensions.TopRow + 2)) {
    if (0 == LayoutStyle) {
      DMSG ((EFI_D_ERROR, "GetSetupEreaType, SETUP_AREA_TITLE, \n"));
      return SETUP_AREA_TITLE;
    }
  }

  if (0 == LayoutStyle) { 
    //
    // Main menu style.
    //
    if (Mouse->Row >= (gScreenDimensions.TopRow + 3) && 
      Mouse->Row <  (gScreenDimensions.BottomRow - 3) &&
      Mouse->Column > (gScreenDimensions.LeftColumn + 1) &&
      Mouse->Column < (gStatementDimensions.RightColumn - gHelpBlockWidth - 1) ) {
      DMSG ((EFI_D_ERROR, "GetSetupEreaType, SETUP_AREA_MENU, \n"));	  	
      return SETUP_AREA_MENU;
    }		
  } else if (1 == LayoutStyle) { 
    //
    //Sub Menu style.
    //
    if (Mouse->Row >= (gScreenDimensions.TopRow + 5) && 
      Mouse->Row <  (gScreenDimensions.BottomRow - 3) &&
      Mouse->Column > (gScreenDimensions.LeftColumn + 1) &&
      Mouse->Column < (gStatementDimensions.RightColumn - gHelpBlockWidth - 1) ) {
      DMSG ((EFI_D_ERROR, "GetSetupEreaType, SETUP_AREA_MENU, \n"));	  	
      return SETUP_AREA_MENU;
    }		
  }

  if (Mouse->Row == (gScreenDimensions.BottomRow - 1) || 
    Mouse->Row == (gScreenDimensions.BottomRow - 2)) {
    DMSG ((EFI_D_ERROR, "GetSetupEreaType, SETUP_AREA_HOTKEY, \n"));
    return SETUP_AREA_HOTKEY;
  }
	
  return SETUP_AREA_MAX;
}

BOOLEAN
MouseGotoFormset (
  IN MOUSE_ACTION_INFO *Mouse
  )
{
  LIST_ENTRY    *Link;
  BYO_BROWSER_FORMSET    *FormSet;
  CHAR16    *Title;
  UINTN    Width;
  UINTN    LastX;

	  
  LastX = 3;
  Link = GetFirstNode (gFormData->ByoFormSetList);
  while (!IsNull (gFormData->ByoFormSetList, Link)) {
    FormSet = BYO_FORM_BROWSER_FORMSET_FROM_LINK (Link);
    Title = GetToken(FormSet->FormSetTitle, FormSet->HiiHandle);
    Width = LibGetStringWidth (Title) / 2 - 1;

    if (Mouse->Column >= LastX && Mouse->Column < (LastX + Width + 2)) {	
      if (gFormData->ByoCurrentFormSetLink != Link) { 
        gFormData->ByoCurrentFormSetLink = Link;
        gUserInput->Action = BROWSER_ACTION_GOTO_BYO;
	 gUserInput->SelectedStatement = NULL;
        return TRUE;
      }    
    }
    if (Mouse->Column < LastX) {
      break;
    }
	
    LastX = LastX + Width + 2;
    FreePool(Title);
    Link = GetNextNode (gFormData->ByoFormSetList, Link);
  }
      
  return FALSE;
}

