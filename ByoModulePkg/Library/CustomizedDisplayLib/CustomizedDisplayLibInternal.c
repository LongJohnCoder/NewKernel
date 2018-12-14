/** @file

  This library class defines a set of interfaces to customize Display module

Copyright (c) 2013-2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "CustomizedDisplayLibInternal.h"

EFI_SCREEN_DESCRIPTOR         gScreenDimensions;
CHAR16                        *mLibUnknownString;
extern EFI_HII_HANDLE         mCDLStringPackHandle;
CHAR16                        *mSpaceBuffer;
#define SPACE_BUFFER_SIZE      1000

//
// Browser Global Strings
//
CHAR16            *gEnterString;
CHAR16            *gEnterCommitString;
CHAR16            *gEnterEscapeString;
CHAR16            *gEscapeString;
CHAR16            *gMoveHighlight;
CHAR16            *gDecNumericInput;
CHAR16            *gHexNumericInput;
CHAR16            *gToggleCheckBox;
CHAR16            *gLibEmptyString;
CHAR16            *gAreYouSure;
CHAR16            *gYesResponse;
CHAR16            *gNoResponse;
CHAR16            *gPlusString;
CHAR16            *gMinusString;
CHAR16            *gAdjustNumber;
CHAR16            *gSaveChanges;
CHAR16            *gNvUpdateMessage;
CHAR16            *gInputErrorMessage;

CHAR16            *gByoCopyRight;
CHAR16            *gByoFunctionKeyOne;
CHAR16            *gByoKeyPlusAndMinus;
CHAR16            *gByoKeyPlusAndMinusHelp;
CHAR16            *gByoFunctionKeyNine;
CHAR16            *gByoFunctionKeyNineHelp;
CHAR16            *gByoFunctionKeyEsc;
CHAR16            *gByoFunctionKeyEscHelp;
CHAR16            *gByoFunctionKeyEnter;
CHAR16            *gByoFunctionKeyEnterHelp;
CHAR16            *gByoFunctionKeyEnterTen;
CHAR16            *gByoFunctionKeyEnterTenHelp;
CHAR16            *gByoFunctionKeyOneHelp;
CHAR16            *gKeyUpDownHelp;
CHAR16            *gByoKeyLeftRightHelp;
CHAR16            *gByoHelpMessage;
CHAR16            *gByoGeneralHelp;
CHAR16            *gByoGeneralHelp1;
CHAR16            *gByoGeneralHelp2;
CHAR16            *gByoGeneralHelp3;
CHAR16            *gByoContinue;
CHAR16            *gByoGeneralHelp2_F3;
CHAR16            *gByoGeneralHelp3_F4;
CHAR16            *gByoFunctionKeyThree;
CHAR16            *gByoFunctionKeyFour;
CHAR16            *gByoSetupConfig;
	
UINT16 gLastFormId = 0xFFFF;

MOUSE_CURSOR_RANGE    mKeyRange_F1, mKeyRange_Up, mKeyRange_Down, mKeyRange_Minus, mKeyRange_Plus, mKeyRange_F9;
MOUSE_CURSOR_RANGE    mKeyRange_Esc, mKeyRange_Left, mKeyRange_Right, mKeyRange_Enter, mKeyRange_F10;

/**

  Print banner info for front page.

  @param[in]  FormData             Form Data to be shown in Page
  
**/
VOID
PrintBannerInfo ( 
  IN FORM_DISPLAY_ENGINE_FORM       *FormData
  )
{
  UINT8                  Line;
  UINT8                  Alignment;
  CHAR16                 *StrFrontPageBanner;
  UINT8                  RowIdx;
  UINT8                  ColumnIdx;

  ClearLines (
    gScreenDimensions.LeftColumn,
    gScreenDimensions.RightColumn,
    gScreenDimensions.TopRow,
    FRONT_PAGE_HEADER_HEIGHT - 1 + gScreenDimensions.TopRow,
    BANNER_TEXT | BANNER_BACKGROUND
    );

  //
  //    for (Line = 0; Line < BANNER_HEIGHT; Line++) {
  //
  for (Line = (UINT8) gScreenDimensions.TopRow; Line < BANNER_HEIGHT + (UINT8) gScreenDimensions.TopRow; Line++) {
    //
    //      for (Alignment = 0; Alignment < BANNER_COLUMNS; Alignment++) {
    //
    for (Alignment = (UINT8) gScreenDimensions.LeftColumn;
         Alignment < BANNER_COLUMNS + (UINT8) gScreenDimensions.LeftColumn;
         Alignment++
        ) {
      RowIdx    = (UINT8) (Line - (UINT8) gScreenDimensions.TopRow);
      ColumnIdx = (UINT8) (Alignment - (UINT8) gScreenDimensions.LeftColumn);
  
      ASSERT (RowIdx < BANNER_HEIGHT && ColumnIdx < BANNER_COLUMNS);
  
      if (gBannerData!= NULL && gBannerData->Banner[RowIdx][ColumnIdx] != 0x0000) {
        StrFrontPageBanner = LibGetToken (gBannerData->Banner[RowIdx][ColumnIdx], FormData->HiiHandle);
      } else {
        continue;
      }
  
      switch (Alignment - gScreenDimensions.LeftColumn) {
      case 0:
        //
        // Handle left column
        //
        PrintStringAt (gScreenDimensions.LeftColumn + BANNER_LEFT_COLUMN_INDENT, Line, StrFrontPageBanner);
        break;
  
      case 1:
        //
        // Handle center column
        //
        PrintStringAt (
          gScreenDimensions.LeftColumn + (gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn) / 3,
          Line,
          StrFrontPageBanner
          );
        break;
  
      case 2:
        //
        // Handle right column
        //
        PrintStringAt (
          gScreenDimensions.LeftColumn + (gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn) * 2 / 3,
          Line,
          StrFrontPageBanner
          );
        break;
      }
  
      FreePool (StrFrontPageBanner);
    }
  }
}

/**
  If Sting length greater Max, Copy Max string to Buf.
  
**/
BOOLEAN
CutMaxString ( 
  CHAR16    **Buf,
  CHAR16    *String,
  UINTN    Max
  )
{
  if (StrLen(String) > Max) {
    ZeroMem(*Buf, (Max + 1) * sizeof(CHAR16));
    CopyMem(*Buf, String, Max * sizeof(CHAR16));
    return TRUE;    
  }
  return FALSE;  
}

/**
  Draw Help Info.
  
**/
VOID
DrawBottomHelpInfo (
  VOID
  )
{
  EFI_SCREEN_DESCRIPTOR  LocalScreen;
  UINTN    Width;
  UINTN    StartColum;
  UINTN    StartRow;
  UINTN    MaxLength;
  CHAR16    *Buf;
  UINTN    PrintWidth;

  ZeroMem (&LocalScreen, sizeof (EFI_SCREEN_DESCRIPTOR));
  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  Width = (LocalScreen.RightColumn - LocalScreen.LeftColumn - 2)/4;
  MaxLength = Width - 4;
  PrintWidth = 4;

  Buf = NULL; 
  Buf = AllocateZeroPool(Width * sizeof(CHAR16));
  ASSERT(NULL != Buf);
 	
  StartColum = LocalScreen.LeftColumn + 2;
  StartRow = LocalScreen.BottomRow - 2;
  //
  // Print HotKey, Line at LocalScreen.BottomRow - 2.
  //
  gST->ConOut->SetAttribute (gST->ConOut, EFI_GREEN|EFI_BACKGROUND_BLUE);
  PrintStringAt(StartColum, StartRow, gByoFunctionKeyOne);
  SetRange(&mKeyRange_F1, StartColum, 2, StartRow, 0);
  PrintAt (PrintWidth, StartColum + Width - 6, StartRow, L"%c%c", ARROW_UP, ARROW_DOWN);  
  SetRange(&mKeyRange_Up, StartColum + Width - 6, 1, StartRow, 0);
  SetRange(&mKeyRange_Down, StartColum + Width - 6 + 1, 1, StartRow, 0);

  PrintStringAt (StartColum + Width * 2 - 6, StartRow, gByoKeyPlusAndMinus);
  SetRange(&mKeyRange_Minus, StartColum + Width * 2 - 6, 1, StartRow, 0);
  SetRange(&mKeyRange_Plus, StartColum + Width * 2 - 6 + 2, 1, StartRow, 0);

  if (PcdGetBool (PcdEnableSetupHotkeyF3F4)) {
    PrintStringAt (StartColum + Width * 3, StartRow, gByoFunctionKeyThree);
  } else {
    PrintStringAt (StartColum + Width * 3, StartRow, gByoFunctionKeyNine);
  }
  SetRange(&mKeyRange_F9, StartColum + Width * 3, 2, StartRow, 0);

  // Print HotKey, Line at LocalScreen.BottomRow - 1.
  //
  StartRow +=1;
  PrintStringAt(StartColum , StartRow, gByoFunctionKeyEsc);
  SetRange(&mKeyRange_Esc, StartColum, 3, StartRow, 0);
  PrintAt (PrintWidth, StartColum + Width - 6, StartRow, L"%c%c", ARROW_LEFT, ARROW_RIGHT);  
  SetRange(&mKeyRange_Left, StartColum + Width - 6, 1, StartRow, 0);
  SetRange(&mKeyRange_Right, StartColum + Width - 6 + 1, 1, StartRow, 0);  
  PrintStringAt (StartColum + Width * 2 - 6, StartRow, gByoFunctionKeyEnter);
  SetRange(&mKeyRange_Enter, StartColum + Width * 2 - 6, 5, StartRow, 0);

  if (PcdGetBool (PcdEnableSetupHotkeyF3F4)) {
    PrintStringAt (StartColum + Width * 3, StartRow, gByoFunctionKeyFour);
  } else {
    PrintStringAt (StartColum + Width * 3, StartRow, gByoFunctionKeyEnterTen);
  }
  SetRange(&mKeyRange_F10, StartColum + Width * 3, 3, StartRow, 0);
  //
  // Print Help String, Line at LocalScreen.BottomRow - 2.
  //
  StartColum += 4;
  StartRow -=1;
  gST->ConOut->SetAttribute (gST->ConOut, EFI_LIGHTGRAY |EFI_BACKGROUND_BLUE);
  if (CutMaxString(&Buf, gByoFunctionKeyOneHelp, MaxLength)) {
    PrintAt (PrintWidth, StartColum, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum, StartRow, gByoFunctionKeyOneHelp);
  }

  if (CutMaxString(&Buf, gKeyUpDownHelp, MaxLength)) {
    PrintAt (PrintWidth, StartColum + Width - 6, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum + Width - 6, StartRow, gKeyUpDownHelp);
  }

  if (CutMaxString(&Buf, gByoKeyPlusAndMinusHelp, MaxLength)) {
    PrintAt (PrintWidth, StartColum + Width * 2 - 4, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum + Width * 2 - 4, StartRow, gByoKeyPlusAndMinusHelp);
  }
  
  if (CutMaxString(&Buf, gByoFunctionKeyNineHelp, MaxLength)) {
    PrintAt (PrintWidth, StartColum + Width * 3, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum + Width * 3, StartRow, gByoFunctionKeyNineHelp);
  }

  //
  // Print Help String, Line at LocalScreen.BottomRow - 1.
  //
  StartRow +=1;
  if (CutMaxString(&Buf, gByoFunctionKeyEscHelp, MaxLength)) {
    PrintAt (PrintWidth, StartColum, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum, StartRow, gByoFunctionKeyEscHelp);
  }

  if (CutMaxString(&Buf, gByoKeyLeftRightHelp, MaxLength)) {
    PrintAt (PrintWidth, StartColum + Width - 6, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum + Width - 6, StartRow, gByoKeyLeftRightHelp);
  }

  if (CutMaxString(&Buf,  gByoFunctionKeyEnterHelp, MaxLength)) {
    PrintAt (PrintWidth, StartColum + Width * 2 - 4, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum + Width * 2 - 4, StartRow, gByoFunctionKeyEnterHelp);  
  }

  if (CutMaxString(&Buf, gByoFunctionKeyEnterTenHelp, MaxLength)) {
    PrintAt (PrintWidth, StartColum + Width * 3, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum + Width * 3, StartRow, gByoFunctionKeyEnterTenHelp);
  }
  
  return;
}

BOOLEAN
MouseGetHotkey (
  IN MOUSE_ACTION_INFO    *Mouse,
  IN OUT EFI_INPUT_KEY    *Key
  )
{
  if (Mouse == NULL || Key == NULL) {
    return FALSE;
  }
  
  Key->UnicodeChar = CHAR_NULL;
  Key->ScanCode = SCAN_NULL;

  if (IsMouseInRange(&mKeyRange_F1, Mouse)) {
    Key->UnicodeChar = CHAR_NULL;
    Key->ScanCode = SCAN_F1;
    return TRUE;
  }
  if (IsMouseInRange(&mKeyRange_Up, Mouse)) {
    Key->UnicodeChar = CHAR_NULL;
    Key->ScanCode = SCAN_UP;
    return TRUE;
  }

  if (IsMouseInRange(&mKeyRange_Down, Mouse)) {
    Key->UnicodeChar = CHAR_NULL;
    Key->ScanCode = SCAN_DOWN;
    return TRUE;
  }

  if (IsMouseInRange(&mKeyRange_Minus, Mouse)) {
    Key->UnicodeChar = L'-';
    Key->ScanCode = SCAN_NULL;
    return TRUE;
  }

  if (IsMouseInRange(&mKeyRange_Plus, Mouse)) {
    Key->UnicodeChar = L'+';
    Key->ScanCode = SCAN_NULL;
    return TRUE;
  }

  if (IsMouseInRange(&mKeyRange_F9, Mouse)) {
    Key->UnicodeChar = CHAR_NULL;
    Key->ScanCode = SCAN_F9;
    return TRUE;
  }

  if (IsMouseInRange(&mKeyRange_Esc, Mouse)) {
    Key->UnicodeChar = CHAR_NULL;
    Key->ScanCode = SCAN_ESC;
    return TRUE;
  }

  if (IsMouseInRange(&mKeyRange_Left, Mouse)) {
    Key->UnicodeChar = CHAR_NULL;
    Key->ScanCode = SCAN_LEFT;
    return TRUE;
  }

  if (IsMouseInRange(&mKeyRange_Right, Mouse)) {
    Key->UnicodeChar = CHAR_NULL;
    Key->ScanCode = SCAN_RIGHT;
    return TRUE;
  }

  if (IsMouseInRange(&mKeyRange_Enter, Mouse)) {
    Key->UnicodeChar = CHAR_CARRIAGE_RETURN;
    Key->ScanCode = SCAN_NULL;
    return TRUE;
  }

  if (IsMouseInRange(&mKeyRange_F10, Mouse)) {
    Key->UnicodeChar = CHAR_NULL;
    Key->ScanCode = SCAN_F10;
    return TRUE;
  }
 
  return FALSE;
}


/**
  Calculate Starting Menu Index from end to begin.

**/
UINTN
GetStartMenuIndex (
  IN CONST LIST_ENTRY    *FormSetList,
  IN UINTN    CurrentIndex
  )
{
  UINTN    StartItem;
  UINTN    TotalItems;
  BYO_BROWSER_FORMSET    *FormSet;
  LIST_ENTRY    *Link;
  CHAR16    *FormsetTitle;
  UINTN    BarLength;
  UINTN    StringWidth;
  UINTN    MenuCount;

  if (FormSetList == NULL) {
    return (UINTN) -1;
  }

  BarLength = 3;
  TotalItems = 0;
  Link = GetFirstNode (FormSetList);
  while (!IsNull (FormSetList, Link)) {
    FormSet = BYO_FORM_BROWSER_FORMSET_FROM_LINK (Link);

    if (0 != FormSet->FormSetTitle && NULL != FormSet->HiiHandle) {
      FormsetTitle = NULL;	      
      FormsetTitle = LibGetToken(FormSet->FormSetTitle, FormSet->HiiHandle);
      if (FormsetTitle) {
        BarLength += LibGetStringWidth(FormsetTitle) / 2;
        BarLength ++;
        FreePool (FormsetTitle);
      }
    }
    TotalItems++;
    Link = GetNextNode (FormSetList, Link);
  }
  if (CurrentIndex > TotalItems) {
    return (UINTN) -1;
  }
  if (gScreenDimensions.RightColumn > BarLength) {
    return 0;
  }
  
  //
  // Check if have more item to show on menu bar.
  //
  StartItem = 0;
  BarLength = 3;
  MenuCount = 0;
  Link = GetPreviousNode (FormSetList, Link);
  while (!IsNull (FormSetList, Link)) {
    FormSet = BYO_FORM_BROWSER_FORMSET_FROM_LINK (Link);

    if (0 != FormSet->FormSetTitle && NULL != FormSet->HiiHandle) {
      if (CurrentIndex >= (TotalItems - MenuCount - 1)) {
      FormsetTitle = NULL;	      
      FormsetTitle = LibGetToken(FormSet->FormSetTitle, FormSet->HiiHandle);
      if (FormsetTitle) {
        StringWidth =  LibGetStringWidth(FormsetTitle) / 2;
        if (gScreenDimensions.RightColumn < BarLength + StringWidth) {
          StartItem = TotalItems - MenuCount;
          break;
        }
        BarLength += StringWidth;
        BarLength ++;		
        FreePool (FormsetTitle);
      }
      }
    }
    MenuCount ++;
    Link = GetPreviousNode (FormSetList, Link);
  }
  
  return StartItem;
}

/**
  Draw form title.

  @param  Selection               Input Selection data structure.
**/
VOID
DrawFormSetMenuBar (
  IN FORM_DISPLAY_ENGINE_FORM       *FormData
  )
{
  EFI_SCREEN_DESCRIPTOR  LocalScreen;
  BYO_BROWSER_FORMSET   *FormSet;
  BYO_BROWSER_FORMSET   *CurrentFormSet;
  LIST_ENTRY          *Link;
  UINTN                  CursorPos;
  UINTN                  Index;
  UINTN                  Count;
  UINTN                  TotalItems;
  UINTN                  CurrentItem;
  UINTN                  StartItem;
  UINTN                  StringWidth;
  CHAR16               *FormsetTitle;
  static UINTN         LastIndex = (UINTN) -1;
  static BOOLEAN    LastMainFormTitle = FALSE;
  BOOLEAN             bMainFormTitle = TRUE;
  EFI_FORM_ID       FirstFormId = 1;
  UINTN    BarLength;
  static UINTN    LastStartItem = 0;
  static UINTN    LastEndItem = 0;

  //
  // Check parameter.
  //
  ZeroMem (&LocalScreen, sizeof (EFI_SCREEN_DESCRIPTOR));
  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));
  BarLength = LocalScreen.RightColumn - LocalScreen.LeftColumn; 

  CurrentFormSet = NULL;
  if (NULL != FormData->ByoCurrentFormSetLink) {
    CurrentFormSet = BYO_FORM_BROWSER_FORMSET_FROM_LINK (FormData->ByoCurrentFormSetLink);
    FirstFormId = CurrentFormSet->FirstFormId;
  } else {
    //
    //Draw Tilte in Middle.
    //
    FormsetTitle = NULL;
    FormsetTitle = LibGetToken(FormData->FormSetTitle, FormData->HiiHandle);
    if (NULL == FormsetTitle) {
      return;
    }
    StringWidth = LibGetStringWidth(FormsetTitle)/2;
    if (BarLength >  StringWidth) { 
      CursorPos = (BarLength - StringWidth)/2;
    } else {
      CursorPos = 3;
      FormsetTitle[BarLength - 6] = CHAR_NULL;
    }
	
    ClearWithBlt (LocalScreen.LeftColumn, LocalScreen.RightColumn, LocalScreen.TopRow + 1, LocalScreen.TopRow + 1, EFI_BLUE| EFI_BACKGROUND_LIGHTGRAY);
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE | EFI_BACKGROUND_LIGHTGRAY); 
    PrintStringAt ( CursorPos,LocalScreen.TopRow + 1, FormsetTitle);
    FreePool (FormsetTitle);
    return;
  }

  if (FirstFormId != FormData->FormId) {
    bMainFormTitle = FALSE;
  }
  if (!IsByoFormset(FormData->ByoFormSetList, FormData->HiiHandle)) {
    bMainFormTitle = FALSE;
  }
  DMSG ((EFI_D_ERROR, "\n DrawFormSetMenuBar, bMainFormTitle :%d, CurrentFormSet :0x%x. \n", bMainFormTitle, CurrentFormSet));

  if (gLibIsFirstForm) {
    LastMainFormTitle = FALSE;
  }
  if (LastMainFormTitle != bMainFormTitle) {
    LastIndex = (UINTN) -1;
  }
  
  //
  // Get info of ByoFormSetList.
  //
  TotalItems = 0;
  CurrentItem = 0;  
  Link = GetFirstNode (FormData->ByoFormSetList);
  while (!IsNull (FormData->ByoFormSetList, Link)) {
    FormSet = BYO_FORM_BROWSER_FORMSET_FROM_LINK (Link);
    if (NULL != CurrentFormSet) {
      if (CompareGuid(&FormSet->Guid, &CurrentFormSet->Guid)) {
        CurrentItem = TotalItems;
      }
    }
    TotalItems++;
    Link = GetNextNode (FormData->ByoFormSetList, Link);
  }
  
  //
  // Calculate start item.
  //
  StartItem = 0;
  if (CurrentItem) {
    if (LastStartItem) {
      if (CurrentItem >= LastStartItem) {
        StartItem = GetStartMenuIndex (FormData->ByoFormSetList, CurrentItem);;
      } else if (CurrentItem == 0 && LastStartItem == TotalItems -1) {
        StartItem = 0;
        LastStartItem = 0;
      }
    } else {
      StartItem = GetStartMenuIndex (FormData->ByoFormSetList, CurrentItem);
      LastStartItem = 0;
    }  
  } else {
    StartItem = 0;
    LastStartItem = 0;
  }
  
  //
  // Draw all menu title.
  //
  if (LastIndex == (UINTN)-1) { 
    ClearWithBlt (LocalScreen.LeftColumn, LocalScreen.RightColumn, LocalScreen.TopRow + 1, LocalScreen.TopRow + 1, EFI_LIGHTGRAY| EFI_BACKGROUND_BLUE);
  }
  
  CursorPos = 3;
  for (Index = StartItem; Index < TotalItems; Index++) {   
    //
    // Get current Formset.
    //
    Link = GetFirstNode (FormData->ByoFormSetList);
    Count = 0;
    FormSet = NULL;
    while (!IsNull (FormData->ByoFormSetList, Link)) {
      FormSet = BYO_FORM_BROWSER_FORMSET_FROM_LINK (Link);
      if (Index == Count) {
        break;
      }
      Count++;
      Link = GetNextNode (FormData->ByoFormSetList, Link);
    }
  
    FormsetTitle = NULL;
    if (0 != FormSet->FormSetTitle && NULL != FormSet->HiiHandle) {
      FormsetTitle = LibGetToken(FormSet->FormSetTitle, FormSet->HiiHandle);
    } else {
      FormsetTitle = L"NULL";
    }
    StringWidth = LibGetStringWidth(FormsetTitle) / 2;
    if (LocalScreen.RightColumn < CursorPos + StringWidth) {	
      if (LastEndItem <= CurrentItem) {
        LastStartItem =  LastStartItem + 1;	
      }
      break;
    }

    if (FALSE == bMainFormTitle && CurrentItem != Index) {
      CursorPos = CursorPos + StringWidth + 1;	
      continue;
    }
    //
    // Draw Title.
    //
    if (CurrentItem == Index) {
      gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE | EFI_BACKGROUND_LIGHTGRAY);
      PrintStringAt(CursorPos,  LocalScreen.TopRow + 1, L" ");
      PrintStringAt(CursorPos + 1,  LocalScreen.TopRow + 1, FormsetTitle);
      PrintStringAt(CursorPos + StringWidth,  LocalScreen.TopRow + 1, L" ");
    } else {
      if (LastIndex <= TotalItems) {
        if (LastIndex == Index) {
          gST->ConOut->SetAttribute (gST->ConOut, EFI_LIGHTGRAY| EFI_BACKGROUND_BLUE);
          PrintStringAt(CursorPos,  LocalScreen.TopRow + 1, L" ");
          PrintStringAt(CursorPos + 1,  LocalScreen.TopRow + 1, FormsetTitle);
          PrintStringAt(CursorPos + StringWidth,  LocalScreen.TopRow + 1, L" ");
        }
      } else {
        gST->ConOut->SetAttribute (gST->ConOut, EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE);
        PrintStringAt(CursorPos,  LocalScreen.TopRow + 1, L" ");
        PrintStringAt(CursorPos + 1,  LocalScreen.TopRow + 1, FormsetTitle);
        PrintStringAt(CursorPos + StringWidth,  LocalScreen.TopRow + 1, L" ");
      }
    } 
  
    if (0 != FormSet->FormSetTitle && NULL != FormSet->HiiHandle) {
      FreePool(FormsetTitle);
    }

    CursorPos = CursorPos + StringWidth + 1;	
  }

  LastIndex = CurrentItem;
  LastMainFormTitle = bMainFormTitle;
  LastEndItem = Index - 1;
  
  //
  //Show red arrow to mark the more main fromset.
  //
  if (StartItem > 0 ) {
    gST->ConOut->SetAttribute (gST->ConOut, EFI_RED| EFI_BACKGROUND_BLUE);
    PrintCharAt (
        LocalScreen.LeftColumn,
        LocalScreen.TopRow + 1,
        GEOMETRICSHAPE_LEFT_TRIANGLE
        );
  }
  if (LastEndItem < TotalItems - 1) {
    gST->ConOut->SetAttribute (gST->ConOut, EFI_RED| EFI_BACKGROUND_BLUE);
    PrintCharAt (
        LocalScreen.RightColumn-1,
        LocalScreen.TopRow + 1,
        GEOMETRICSHAPE_RIGHT_TRIANGLE
        );
  }

  return ;
}

/**
  Check whether formset is in ByoFormSetList.

**/
BOOLEAN
IsByoFormset (
  IN  LIST_ENTRY    *ByoFormSetList,
  IN  EFI_HII_HANDLE    HiiHandle
  )
{
  LIST_ENTRY           *Link;
  BYO_BROWSER_FORMSET   *FormSet;

  if (NULL == ByoFormSetList || NULL == HiiHandle) {
    return FALSE;
  }

  if (IsListEmpty(ByoFormSetList)) {
    return FALSE;
  }

  Link = GetFirstNode (ByoFormSetList);
  while (!IsNull (ByoFormSetList, Link)) {
    FormSet = BYO_FORM_BROWSER_FORMSET_FROM_LINK (Link);
    if (FormSet->HiiHandle == HiiHandle) {
      return TRUE;
    }
    Link = GetNextNode (ByoFormSetList, Link);
  }
  return FALSE;
}


/**
  Repair black line on the top and bottom..

**/
VOID
RepairScreen ()
{
  EFI_STATUS    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL     *GraphicsOutput = NULL;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    BackGroundBlt_Blue =   {0x98, 0x00, 0x00, 0x00};  // LIGHTBLUE

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput
                  );
  if (!EFI_ERROR(Status)) {  	
    DMSG ((EFI_D_ERROR, "\n RepairScreen, \n"));
    DMSG ((EFI_D_ERROR, "HorizontalResolution :%d, VerticalResolution :%d.\n", GraphicsOutput->Mode->Info->HorizontalResolution, GraphicsOutput->Mode->Info->VerticalResolution));
    DMSG ((EFI_D_ERROR, "RightColumn :%d, BottomRow :%d.\n", gScreenDimensions.RightColumn, gScreenDimensions.BottomRow));

    //
    // paint first line to repaire Black line on top.
    //
    Status = GraphicsOutput->Blt (
                        GraphicsOutput,
                        &BackGroundBlt_Blue,
                        EfiBltVideoFill,
                        0, //SourceX
                        0, //SourceY
                        0,//DestinationX
                        0,//DestinationY
                        EFI_GLYPH_WIDTH * gScreenDimensions.RightColumn, //Width
                        EFI_GLYPH_HEIGHT, //Height
                        0
                        );

    //
    // paint last line to repaire Black line on bottom.
    //
    Status = GraphicsOutput->Blt (
                        GraphicsOutput,
                        &BackGroundBlt_Blue,
                        EfiBltVideoFill,
                        0, //SourceX
                        0, //SourceY
                        0,//DestinationX
                        (GraphicsOutput->Mode->Info->VerticalResolution - EFI_GLYPH_HEIGHT), //DestinationY
                        EFI_GLYPH_WIDTH * gScreenDimensions.RightColumn, //Width
                        EFI_GLYPH_HEIGHT, //Height
                        0
                        );

  }

  return ;
}

EFI_GRAPHICS_OUTPUT_BLT_PIXEL        mLibEfiColors[16] = {
  //
  // B    G    R   reserved
  //
  {0x00, 0x00, 0x00, 0x00},  // BLACK
  {0x98, 0x00, 0x00, 0x00},  // LIGHTBLUE
  {0x00, 0x98, 0x00, 0x00},  // LIGHGREEN
  {0x98, 0x98, 0x00, 0x00},  // LIGHCYAN
  {0x00, 0x00, 0x98, 0x00},  // LIGHRED
  {0x98, 0x00, 0x98, 0x00},  // MAGENTA
  {0x00, 0x98, 0x98, 0x00},  // BROWN
  {0x98, 0x98, 0x98, 0x00},  // LIGHTGRAY
  {0x30, 0x30, 0x30, 0x00},  // DARKGRAY - BRIGHT BLACK
  {0xff, 0x00, 0x00, 0x00},  // BLUE
  {0x00, 0xff, 0x00, 0x00},  // LIME
  {0xff, 0xff, 0x00, 0x00},  // CYAN
  {0x00, 0x00, 0xff, 0x00},  // RED
  {0xff, 0x00, 0xff, 0x00},  // FUCHSIA
  {0x00, 0xff, 0xff, 0x00},  // YELLOW
  {0xff, 0xff, 0xff, 0x00}   // WHITE
};

/**
  Clear retangle with Blt of GOP, It's replace of ClearLines to improve performance.

  @param  LeftColumn     Left column of retangle.
  @param  RightColumn    Right column of retangle.
  @param  TopRow         Start row of retangle.
  @param  BottomRow      End row of retangle.
  @param  TextAttribute  The character foreground and background.

**/
VOID
ClearWithBlt (
  IN UINTN               LeftColumn,
  IN UINTN               RightColumn,
  IN UINTN               TopRow,
  IN UINTN               BottomRow,
  IN UINTN               TextAttribute
  )
{
  EFI_STATUS    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL     *GraphicsOutput = NULL;

  //DMSG ((EFI_D_ERROR, "ClearWithBlt, H :%d-%d, V :%d-%d. \n", LeftColumn, RightColumn, TopRow, BottomRow));  
  if (LeftColumn > RightColumn || TopRow > BottomRow) {
    //DMSG ((EFI_D_ERROR, "ClearWithBlt, Parameter Error.\n"));  
    return;
  }

  if (1 == PcdGet8 (PcdUCREnable)) {
    ClearLines (LeftColumn, RightColumn, TopRow, BottomRow, TextAttribute);
    return;
  }

  //
  // GOP or UGA.
  //
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput
                  );
  if (!EFI_ERROR(Status)) {
    Status = GraphicsOutput->Blt (
                        GraphicsOutput,
                        &mLibEfiColors[TextAttribute>>4],
                        EfiBltVideoFill,
                        0,
                        0,
                        (GraphicsOutput->Mode->Info->HorizontalResolution - gScreenDimensions.RightColumn * EFI_GLYPH_WIDTH)/2 + EFI_GLYPH_WIDTH * LeftColumn,
                        (GraphicsOutput->Mode->Info->VerticalResolution - gScreenDimensions.BottomRow * EFI_GLYPH_HEIGHT)/2 + EFI_GLYPH_HEIGHT * TopRow,
                        EFI_GLYPH_WIDTH * (RightColumn - LeftColumn),
                        EFI_GLYPH_HEIGHT* (BottomRow - TopRow + 1),
                        0
                        );
    gST->ConOut->SetAttribute (gST->ConOut, TextAttribute);
  }

  if (NULL == GraphicsOutput) {
    ClearLines (LeftColumn, RightColumn, TopRow, BottomRow, TextAttribute);
  }

  return ;
}


/**
  Clear retangle with Blt of GOP, It's replace of ClearLines to improve performance.
  No mather if PcdUCREnable is TRUE.

  @param  LeftColumn     Left column of retangle.
  @param  RightColumn    Right column of retangle.
  @param  TopRow         Start row of retangle.
  @param  BottomRow      End row of retangle.
  @param  TextAttribute  The character foreground and background.

**/
VOID
ClearWithBltAlways (
  IN UINTN               LeftColumn,
  IN UINTN               RightColumn,
  IN UINTN               TopRow,
  IN UINTN               BottomRow,
  IN UINTN               TextAttribute
  )
{
  EFI_STATUS    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL     *GraphicsOutput = NULL;

  DMSG ((EFI_D_ERROR, "ClearWithBlt, H :%d-%d, V :%d-%d. \n", LeftColumn, RightColumn, TopRow, BottomRow));  
  if (LeftColumn > RightColumn || TopRow > BottomRow) {
    DMSG ((EFI_D_ERROR, "ClearWithBlt, Parameter Error.\n"));  
    return;
  }

  //
  // GOP or UGA.
  //
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput
                  );
  if (!EFI_ERROR(Status)) {
    Status = GraphicsOutput->Blt (
                        GraphicsOutput,
                        &mLibEfiColors[TextAttribute>>4],
                        EfiBltVideoFill,
                        0,
                        0,
                        (GraphicsOutput->Mode->Info->HorizontalResolution - gScreenDimensions.RightColumn * EFI_GLYPH_WIDTH)/2 + EFI_GLYPH_WIDTH * LeftColumn,
                        (GraphicsOutput->Mode->Info->VerticalResolution - gScreenDimensions.BottomRow * EFI_GLYPH_HEIGHT)/2 + EFI_GLYPH_HEIGHT * TopRow,
                        EFI_GLYPH_WIDTH * (RightColumn - LeftColumn),
                        EFI_GLYPH_HEIGHT* (BottomRow - TopRow + 1),
                        0
                        );
    gST->ConOut->SetAttribute (gST->ConOut, TextAttribute);
  }

  if (NULL == GraphicsOutput) {
    ClearLines (LeftColumn, RightColumn, TopRow, BottomRow, TextAttribute);
  }

  return ;
}

/**
  Return TRUE If setup console mode match Text mode.

**/
BOOLEAN
CheckSetupConsoleMode (
  VOID
  )
{
  EFI_STATUS    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *SimpleTextOut;
  UINTN    BootTextColumn;
  UINTN    BootTextRow;

  //
  // Get current video resolution and text mode 
  //
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID**)&GraphicsOutput
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID**)&SimpleTextOut
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Check current video resolution and text mode.
  //
  Status = SimpleTextOut->QueryMode (
                                SimpleTextOut,
                                SimpleTextOut->Mode->Mode,
                                &BootTextColumn,
                                &BootTextRow
                                );
  if (!EFI_ERROR (Status)) {
    if (GraphicsOutput->Mode->Info->HorizontalResolution / EFI_GLYPH_WIDTH == PcdGet32 (PcdSetupConOutColumn) &&
      GraphicsOutput->Mode->Info->VerticalResolution / EFI_GLYPH_HEIGHT == PcdGet32 (PcdSetupConOutRow)) {
      
      DMSG ((EFI_D_ERROR, "CheckSetupConsoleMode, Setup console mode is matched. \n"));
      return TRUE;
    }
  }

  DMSG ((EFI_D_ERROR, "CheckSetupConsoleMode, Setup console mode is mismatched. \n"));  
  return FALSE;
}


/**
  Print framework and form title for a page.

  @param[in]  FormData             Form Data to be shown in Page
**/
VOID
PrintFramework (
  IN FORM_DISPLAY_ENGINE_FORM       *FormData
  )
{
  UINTN                  Index;
  CHAR16                 Character;
  CHAR16                 *Buffer;
  UINTN                  Row;
  static BOOLEAN    bFirstIn = TRUE;
  static EFI_HII_HANDLE    LastHiiHandle = NULL;

  DMSG ((EFI_D_ERROR, "\n PrintFramework, \n"));
  if (gClassOfVfr != FORMSET_CLASS_PLATFORM_SETUP) {
    //
    // Only Setup page needs Framework
    //
    ClearLines (
      gScreenDimensions.LeftColumn,
      gScreenDimensions.RightColumn,
      gScreenDimensions.BottomRow - STATUS_BAR_HEIGHT - gFooterHeight,
      gScreenDimensions.BottomRow - STATUS_BAR_HEIGHT - 1,
      KEYHELP_TEXT | KEYHELP_BACKGROUND
      );
    return;
  }

  if (((LastHiiHandle != FormData->HiiHandle) && (!IsByoFormset(FormData->ByoFormSetList, FormData->HiiHandle))) || gLibIsFirstForm) {
    LastHiiHandle = FormData->HiiHandle;
    bFirstIn = TRUE;
    gLastFormId = 0xFFFF;
  }

  if (BeLanguageChanged()) {
    bFirstIn = TRUE;
    gLastFormId = 0xFFFF;    
    gLibIsFirstForm = TRUE;

    FreeLibStrings ();
    InitializeLibStrings ();
  }

  if (bFirstIn ) {
    //
    // Fill background of help info to avoid scroll screen.
    //
    ClearWithBlt (
        gScreenDimensions.LeftColumn,
        gScreenDimensions.RightColumn-1,
        gScreenDimensions.BottomRow - 2,
        gScreenDimensions.BottomRow - 1,
        EFI_WHITE |EFI_BACKGROUND_BLUE
        );
    ClearWithBltAlways (
        gScreenDimensions.RightColumn-1,
        gScreenDimensions.RightColumn,
        gScreenDimensions.BottomRow - 2,
        gScreenDimensions.BottomRow - 1,
        EFI_WHITE |EFI_BACKGROUND_BLUE
        );
    gST->ConOut->SetAttribute (gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLUE);

    //
    //Repair black block on bottom and top of screen.
    //
    DMSG ((EFI_D_ERROR, "PrintFramework, PcdUCREnable :%d. \n", PcdGet8 (PcdUCREnable)));
    if (!PcdGet8 (PcdUCREnable) && CheckSetupConsoleMode()) {
      RepairScreen();
    }
	
    //
    // Copy right title.
    //
    ClearWithBlt(
      gScreenDimensions.LeftColumn,
      gScreenDimensions.RightColumn,
      gScreenDimensions.TopRow,
      gScreenDimensions.TopRow,
      EFI_WHITE |EFI_BACKGROUND_BLUE
    );
    gST->ConOut->SetAttribute (gST->ConOut, EFI_GREEN | EFI_BACKGROUND_BLUE);
    PrintStringAt (
      (gScreenDimensions.RightColumn + gScreenDimensions.LeftColumn - LibGetStringWidth (gByoCopyRight) / 2) / 2,
      gScreenDimensions.TopRow,
      gByoCopyRight
    );
  }
  
  //
  // Formsets Menu title.
  //
  DMSG ((EFI_D_ERROR, "PrintFramework, DrawFormSetMenuBar, \n"));
  DrawFormSetMenuBar (FormData);

  Buffer = AllocateZeroPool (0x10000);
  ASSERT (Buffer != NULL);
  Character = BOXDRAW_HORIZONTAL;
  for (Index = 0; Index + 2 < (gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn); Index++) {
    Buffer[Index] = Character;
  }

  if (bFirstIn) {
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE | EFI_BACKGROUND_LIGHTGRAY);

    Character = BOXDRAW_DOWN_RIGHT;
    PrintCharAt (gScreenDimensions.LeftColumn, gScreenDimensions.TopRow + 2, Character);
    PrintStringAt ((UINTN) -1, (UINTN) -1, Buffer);

    Character = BOXDRAW_DOWN_LEFT;
    PrintCharAt ((UINTN) -1, (UINTN) -1, Character);

    Character = BOXDRAW_VERTICAL;
    for (Row = gScreenDimensions.TopRow + 3; Row <= gScreenDimensions.BottomRow - 4; Row++) {
      PrintCharAt (gScreenDimensions.LeftColumn, Row, Character);
      PrintCharAt (gScreenDimensions.RightColumn - 1, Row, Character);
    }

    Character = BOXDRAW_UP_RIGHT;
    PrintCharAt (gScreenDimensions.LeftColumn, gScreenDimensions.BottomRow - 3, Character);
    PrintStringAt ((UINTN) -1, (UINTN) -1, Buffer);

    Character = BOXDRAW_UP_LEFT;
    PrintCharAt ((UINTN) -1, (UINTN) -1, Character);
  }
  
  if (bFirstIn) {
    DrawBottomHelpInfo ();
    bFirstIn = FALSE;
  }
  
  FreePool (Buffer);
}

/**
  Print Form layout, include tltle and help.

  @param[in]  FormData             Form Data to be shown in Page
**/
VOID
PrintFormLayout (
  IN FORM_DISPLAY_ENGINE_FORM       *FormData,
  IN UINTN    MiddleVerticalLineColumn
  )
{
  UINTN    Index;
  CHAR16    Character;
  CHAR16    *Buffer;
  CHAR16    *TitleStr;
  UINTN    TitleColumn;
  UINTN    Row;
  
  if (gClassOfVfr != FORMSET_CLASS_PLATFORM_SETUP) {
    return;
  }
  DMSG ((EFI_D_ERROR, "PrintFormLayout, ByoLayoutStyle :%d. \n", FormData->ByoLayoutStyle));
  
  Buffer = AllocateZeroPool (0x10000);
  ASSERT (Buffer != NULL);

  if (0 == FormData->ByoLayoutStyle) {
    //
    // Main Form style.
    //  	
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE | EFI_BACKGROUND_LIGHTGRAY);
    Character = BOXDRAW_VERTICAL;
    PrintCharAt (gScreenDimensions.LeftColumn, gScreenDimensions.TopRow + 4, Character);

    Character = BOXDRAW_VERTICAL_RIGHT;
    PrintCharAt (MiddleVerticalLineColumn, gScreenDimensions.TopRow + 4, Character);

    Character = BOXDRAW_HORIZONTAL;
    for (Index = 0; Index < (gScreenDimensions.RightColumn - MiddleVerticalLineColumn - 2); Index++) {
      Buffer[Index] = Character;
    }
    Buffer[Index] = CHAR_NULL;
    PrintStringAt ((UINTN) -1, (UINTN) -1, Buffer);

    Character = BOXDRAW_VERTICAL_LEFT;
    PrintCharAt ((UINTN) -1, (UINTN) -1, Character);

    Character = BOXDRAW_DOWN_HORIZONTAL;
    PrintCharAt (MiddleVerticalLineColumn, gScreenDimensions.TopRow + 2, Character);

    Character = BOXDRAW_VERTICAL;
    PrintCharAt (MiddleVerticalLineColumn, gScreenDimensions.TopRow + 3, Character);

    Character = BOXDRAW_VERTICAL;
    for (Row = gScreenDimensions.TopRow + 5; Row <= gScreenDimensions.BottomRow - 4; Row++) {
      PrintCharAt (MiddleVerticalLineColumn, Row, Character);
    }

    Character = BOXDRAW_UP_HORIZONTAL;
    PrintCharAt (MiddleVerticalLineColumn, gScreenDimensions.BottomRow - 3, Character);

   TitleColumn = MiddleVerticalLineColumn+ (gScreenDimensions.RightColumn -MiddleVerticalLineColumn - LibGetStringWidth (gByoHelpMessage) / 2) / 2;
    PrintStringAtWithWidth (MiddleVerticalLineColumn + 1, gScreenDimensions.TopRow + 3, gLibEmptyString, gScreenDimensions.RightColumn - MiddleVerticalLineColumn - 2);
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BLACK | EFI_BACKGROUND_LIGHTGRAY);
    PrintStringAtWithWidth (TitleColumn, gScreenDimensions.TopRow + 3, gByoHelpMessage, (gScreenDimensions.RightColumn - TitleColumn - 4));
  }else if (1 == FormData->ByoLayoutStyle) {
    //
    // Sub Form style.
    //
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE | EFI_BACKGROUND_LIGHTGRAY);
    TitleStr = LibGetToken (FormData->FormTitle, FormData->HiiHandle);
    ASSERT (TitleStr != NULL);

    if (gLastFormId != FormData->FormId) {
      PrintStringAtWithWidth (gScreenDimensions.LeftColumn + 1, gScreenDimensions.TopRow + 3, gLibEmptyString, MiddleVerticalLineColumn - 1);
    }
    if ((MiddleVerticalLineColumn - 4) > LibGetStringWidth (TitleStr) / 2) {
      TitleColumn = gScreenDimensions.LeftColumn +(MiddleVerticalLineColumn - LibGetStringWidth (TitleStr) / 2) / 2;
      PrintStringAtWithWidth (
        TitleColumn,
        gScreenDimensions.TopRow + 3,
        TitleStr,
        MiddleVerticalLineColumn - TitleColumn - 1
        );
    } else {
      TitleColumn = 4;
      CopyMem (Buffer, TitleStr, (MiddleVerticalLineColumn - 8) * sizeof (CHAR16));
      PrintStringAtWithWidth (
        TitleColumn,
        gScreenDimensions.TopRow + 3,
        Buffer,
        MiddleVerticalLineColumn - 8
        );      
    }
    if (NULL != TitleStr) {	
      FreePool (TitleStr);
    }

    Character = BOXDRAW_HORIZONTAL;
    for (Index = 0; Index + 1 < (MiddleVerticalLineColumn - gScreenDimensions.LeftColumn); Index++) {
      Buffer[Index] = Character;
    }

    Character = BOXDRAW_VERTICAL_RIGHT;
    PrintCharAt (gScreenDimensions.LeftColumn, gScreenDimensions.TopRow + 4, Character);
    PrintStringAt ((UINTN) -1, (UINTN) -1, Buffer);

    Character = BOXDRAW_VERTICAL_HORIZONTAL;
    PrintCharAt ((UINTN) -1, (UINTN) -1, Character);

    Character = BOXDRAW_HORIZONTAL;
    for (Index = 0; Index < (gScreenDimensions.RightColumn - MiddleVerticalLineColumn - 2); Index++) {
      Buffer[Index] = Character;
    }
    Buffer[Index] = CHAR_NULL;
    PrintStringAt ((UINTN) -1, (UINTN) -1, Buffer);

    Character = BOXDRAW_VERTICAL_LEFT;
    PrintCharAt ((UINTN) -1, (UINTN) -1, Character);

    Character = BOXDRAW_DOWN_HORIZONTAL;
    PrintCharAt (MiddleVerticalLineColumn, gScreenDimensions.TopRow + 2, Character);

    Character = BOXDRAW_VERTICAL;
    PrintCharAt (MiddleVerticalLineColumn, gScreenDimensions.TopRow + 3, Character);

    Character = BOXDRAW_VERTICAL;
    for (Row = gScreenDimensions.TopRow + 5; Row <= gScreenDimensions.BottomRow - 4; Row++) {
      PrintCharAt (MiddleVerticalLineColumn, Row, Character);
    }

    Character = BOXDRAW_UP_HORIZONTAL;
    PrintCharAt (MiddleVerticalLineColumn, gScreenDimensions.BottomRow - 3, Character);

   TitleColumn = MiddleVerticalLineColumn+ (gScreenDimensions.RightColumn -MiddleVerticalLineColumn - LibGetStringWidth (gByoHelpMessage) / 2) / 2;
   if (gLastFormId != FormData->FormId) {
     PrintStringAtWithWidth (MiddleVerticalLineColumn + 1, gScreenDimensions.TopRow + 3, gLibEmptyString, gScreenDimensions.RightColumn - MiddleVerticalLineColumn - 2);
     gLastFormId = FormData->FormId;
   }

    gST->ConOut->SetAttribute (gST->ConOut, EFI_BLACK | EFI_BACKGROUND_LIGHTGRAY);
    PrintStringAtWithWidth (TitleColumn, gScreenDimensions.TopRow + 3, gByoHelpMessage, (gScreenDimensions.RightColumn - TitleColumn - 2));	  
  }

  FreePool (Buffer);
}

/**
  Middle vertical line maybe covered by Pop-up Dialog.

  @param[in]  FormData             Form Data to be shown in Page
**/
VOID
RepaintMiddleLine (
  IN FORM_DISPLAY_ENGINE_FORM       *FormData,
  IN UINTN    MiddleVerticalLineColumn
  )
{
  UINTN    Row;
  
  if (gClassOfVfr != FORMSET_CLASS_PLATFORM_SETUP) {
    return;
  }
  
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE | EFI_BACKGROUND_LIGHTGRAY);  
  for (Row = gScreenDimensions.TopRow + 5; Row <= gScreenDimensions.BottomRow - 4; Row++) {
    PrintCharAt (gScreenDimensions.LeftColumn, Row, BOXDRAW_VERTICAL);
    PrintCharAt (MiddleVerticalLineColumn, Row, BOXDRAW_VERTICAL);
    PrintCharAt (gScreenDimensions.RightColumn - 1, Row, BOXDRAW_VERTICAL);
  }

  return;
}

/**
  Process some op code which is not recognized by browser core.

  @param OpCodeData                  The pointer to the op code buffer.

  @return EFI_SUCCESS            Pass the statement success.

**/
VOID
ProcessUserOpcode(
  IN  EFI_IFR_OP_HEADER         *OpCodeData
  )
{
  EFI_GUID *   ClassGuid;
  UINT8        ClassGuidNum;

  ClassGuid    = NULL;
  ClassGuidNum = 0;

  switch (OpCodeData->OpCode) {
    case EFI_IFR_FORM_SET_OP:
      //
      // process the statement outside of form,if it is formset op, get its formsetguid or classguid and compared with gFrontPageFormSetGuid
      //
      if (CompareMem (PcdGetPtr (PcdFrontPageFormSetGuid), &((EFI_IFR_FORM_SET *) OpCodeData)->Guid, sizeof (EFI_GUID)) == 0){
        gClassOfVfr = FORMSET_CLASS_FRONT_PAGE;
      } else{
        ClassGuidNum = (UINT8)(((EFI_IFR_FORM_SET *)OpCodeData)->Flags & 0x3);
        ClassGuid    = (EFI_GUID *)(VOID *)((UINT8 *)OpCodeData + sizeof (EFI_IFR_FORM_SET));
        while (ClassGuidNum-- > 0){
          if (CompareGuid((EFI_GUID*)PcdGetPtr (PcdFrontPageFormSetGuid),ClassGuid)){
            gClassOfVfr = FORMSET_CLASS_FRONT_PAGE;
            break;
          }
          ClassGuid ++;
        }
      }
      break;

    case EFI_IFR_GUID_OP:     
      if (CompareGuid (&gEfiIfrTianoGuid, (EFI_GUID *)((CHAR8*) OpCodeData + sizeof (EFI_IFR_OP_HEADER)))) {
        //
        // Tiano specific GUIDed opcodes
        //
        switch (((EFI_IFR_GUID_LABEL *) OpCodeData)->ExtendOpCode) {
        case EFI_IFR_EXTEND_OP_LABEL:
          //
          // just ignore label
          //
          break;

        case EFI_IFR_EXTEND_OP_BANNER:
          //
          // Only in front page form set, we care about the banner data.
          //
          if (gClassOfVfr == FORMSET_CLASS_FRONT_PAGE) {
            //
            // Initialize Driver private data
            //
            if (gBannerData == NULL) {
              gBannerData = AllocateZeroPool (sizeof (BANNER_DATA));
              ASSERT (gBannerData != NULL);
            }
            
            CopyMem (
              &gBannerData->Banner[((EFI_IFR_GUID_BANNER *) OpCodeData)->LineNumber][
              ((EFI_IFR_GUID_BANNER *) OpCodeData)->Alignment],
              &((EFI_IFR_GUID_BANNER *) OpCodeData)->Title,
              sizeof (EFI_STRING_ID)
              );
          }
          break;

        case EFI_IFR_EXTEND_OP_SUBCLASS:
          if (((EFI_IFR_GUID_SUBCLASS *) OpCodeData)->SubClass == EFI_FRONT_PAGE_SUBCLASS) {
            gClassOfVfr = FORMSET_CLASS_FRONT_PAGE;
          }
          break;

        default:
          break;
        }
      }
      break;

    default:
      break;
  }
}

/**
  Process some op codes which is out side of current form.
  
  @param FormData                Pointer to the form data.

  @return EFI_SUCCESS            Pass the statement success.

**/
VOID
ProcessExternedOpcode (
  IN FORM_DISPLAY_ENGINE_FORM       *FormData
  )
{
  LIST_ENTRY                    *Link;
  LIST_ENTRY                    *NestLink;
  FORM_DISPLAY_ENGINE_STATEMENT *Statement;
  FORM_DISPLAY_ENGINE_STATEMENT *NestStatement;

  Link = GetFirstNode (&FormData->StatementListOSF);
  while (!IsNull (&FormData->StatementListOSF, Link)) {
    Statement = FORM_DISPLAY_ENGINE_STATEMENT_FROM_LINK (Link);
    Link = GetNextNode (&FormData->StatementListOSF, Link);

    ProcessUserOpcode(Statement->OpCode);
  }

  Link = GetFirstNode (&FormData->StatementListHead);
  while (!IsNull (&FormData->StatementListHead, Link)) {
    Statement = FORM_DISPLAY_ENGINE_STATEMENT_FROM_LINK (Link);
    Link = GetNextNode (&FormData->StatementListHead, Link);

    ProcessUserOpcode(Statement->OpCode);

    NestLink = GetFirstNode (&Statement->NestStatementList);
    while (!IsNull (&Statement->NestStatementList, NestLink)) {
      NestStatement = FORM_DISPLAY_ENGINE_STATEMENT_FROM_LINK (NestLink);
      NestLink = GetNextNode (&Statement->NestStatementList, NestLink);

      ProcessUserOpcode(NestStatement->OpCode);
    }

  }
}

/**
  Validate the input screen diemenstion info.

  @param  FormData               The input form data info.

  @return EFI_SUCCESS            The input screen info is acceptable.
  @return EFI_INVALID_PARAMETER  The input screen info is not acceptable.

**/
EFI_STATUS 
ScreenDiemensionInfoValidate (
  IN FORM_DISPLAY_ENGINE_FORM       *FormData
  )
{
  LIST_ENTRY           *Link;
  UINTN                Index;

  //
  // Calculate total number of Register HotKeys. 
  //
  Index = 0;
  if (!IsListEmpty (&FormData->HotKeyListHead)){
    Link  = GetFirstNode (&FormData->HotKeyListHead);
    while (!IsNull (&FormData->HotKeyListHead, Link)) {
      Link = GetNextNode (&FormData->HotKeyListHead, Link);
      Index ++;
    }
  }

  //
  // Show three HotKeys help information on one row.
  //
  gFooterHeight = FOOTER_HEIGHT + (Index / 3);  
  ZeroMem (&gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));
  gST->ConOut->QueryMode (
                 gST->ConOut,
                 gST->ConOut->Mode->Mode,
                 &gScreenDimensions.RightColumn,
                 &gScreenDimensions.BottomRow
                 );

  //
  // Check local dimension vs. global dimension.
  //
  if (FormData->ScreenDimensions != NULL) {
    if ((gScreenDimensions.RightColumn < FormData->ScreenDimensions->RightColumn) ||
        (gScreenDimensions.BottomRow < FormData->ScreenDimensions->BottomRow)
        ) {
      return EFI_INVALID_PARAMETER;
    } else {
      //
      // Local dimension validation.
      //
      if ((FormData->ScreenDimensions->RightColumn > FormData->ScreenDimensions->LeftColumn) &&
          (FormData->ScreenDimensions->BottomRow > FormData->ScreenDimensions->TopRow) &&
          ((FormData->ScreenDimensions->RightColumn - FormData->ScreenDimensions->LeftColumn) > 2) &&
          ((FormData->ScreenDimensions->BottomRow - FormData->ScreenDimensions->TopRow) > STATUS_BAR_HEIGHT +
            FRONT_PAGE_HEADER_HEIGHT + gFooterHeight + 3)) {
        CopyMem (&gScreenDimensions, (VOID *) FormData->ScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));
      } else {
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Get the string based on the StringId and HII Package List Handle.

  @param  Token                  The String's ID.
  @param  HiiHandle              The package list in the HII database to search for
                                 the specified string.

  @return The output string.

**/
CHAR16 *
LibGetToken (
  IN  EFI_STRING_ID                Token,
  IN  EFI_HII_HANDLE               HiiHandle
  )
{
  EFI_STRING  String;

  String = HiiGetString (HiiHandle, Token, NULL);
  if (String == NULL) {
    String = AllocateCopyPool (StrSize (mLibUnknownString), mLibUnknownString);
    ASSERT (String != NULL);
  }

  return (CHAR16 *) String;
}


/**
  Count the storage space of a Unicode string.

  This function handles the Unicode string with NARROW_CHAR
  and WIDE_CHAR control characters. NARROW_HCAR and WIDE_CHAR
  does not count in the resultant output. If a WIDE_CHAR is
  hit, then 2 Unicode character will consume an output storage
  space with size of CHAR16 till a NARROW_CHAR is hit.

  If String is NULL, then ASSERT ().

  @param String          The input string to be counted.

  @return Storage space for the input string.

**/
UINTN
LibGetStringWidth (
  IN CHAR16               *String
  )
{
  UINTN Index;
  UINTN Count;
  UINTN IncrementValue;

  ASSERT (String != NULL);
  if (String == NULL) {
    return 0;
  }

  Index           = 0;
  Count           = 0;
  IncrementValue  = 1;

  do {
    //
    // Advance to the null-terminator or to the first width directive
    //
    for (;
         (String[Index] != NARROW_CHAR) && (String[Index] != WIDE_CHAR) && (String[Index] != 0);
         Index++, Count = Count + IncrementValue
        )
      ;

    //
    // We hit the null-terminator, we now have a count
    //
    if (String[Index] == 0) {
      break;
    }
    //
    // We encountered a narrow directive - strip it from the size calculation since it doesn't get printed
    // and also set the flag that determines what we increment by.(if narrow, increment by 1, if wide increment by 2)
    //
    if (String[Index] == NARROW_CHAR) {
      //
      // Skip to the next character
      //
      Index++;
      IncrementValue = 1;
    } else {
      //
      // Skip to the next character
      //
      Index++;
      IncrementValue = 2;
    }
  } while (String[Index] != 0);

  //
  // Increment by one to include the null-terminator in the size
  //
  Count++;

  return Count * sizeof (CHAR16);
}

/**
  Show all registered HotKey help strings on bottom Rows.

  @param FormData          The curent input form data info.
  @param SetState          Set HotKey or Clear HotKey

**/
VOID
PrintHotKeyHelpString (
  IN FORM_DISPLAY_ENGINE_FORM      *FormData,
  IN BOOLEAN                       SetState
  )
{
  UINTN                  CurrentCol;
  UINTN                  CurrentRow;
  UINTN                  BottomRowOfHotKeyHelp;
  UINTN                  ColumnIndexWidth;
  UINTN                  ColumnWidth;
  UINTN                  ColumnIndex;
  UINTN                  Index;
  EFI_SCREEN_DESCRIPTOR  LocalScreen;
  LIST_ENTRY             *Link;
  BROWSER_HOT_KEY        *HotKey;
  CHAR16                 BakChar;
  CHAR16                 *ColumnStr;

  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));
  ColumnWidth            = (LocalScreen.RightColumn - LocalScreen.LeftColumn) / 3;
  BottomRowOfHotKeyHelp  = LocalScreen.BottomRow - STATUS_BAR_HEIGHT - 3;
  ColumnStr              = gLibEmptyString;

  //
  // Calculate total number of Register HotKeys. 
  //
  Index = 0;
  Link  = GetFirstNode (&FormData->HotKeyListHead);
  while (!IsNull (&FormData->HotKeyListHead, Link)) {
    HotKey = BROWSER_HOT_KEY_FROM_LINK (Link);
    //
    // Calculate help information Column and Row.
    //
    ColumnIndex = Index % 3;
    if (ColumnIndex == 0) {
      CurrentCol       = LocalScreen.LeftColumn + 2 * ColumnWidth;
      ColumnIndexWidth = ColumnWidth - 1;
    } else if (ColumnIndex == 1) {
      CurrentCol       = LocalScreen.LeftColumn + ColumnWidth;
      ColumnIndexWidth = ColumnWidth;
    } else {
      CurrentCol       = LocalScreen.LeftColumn + 2;
      ColumnIndexWidth = ColumnWidth - 2;
    }
    CurrentRow = BottomRowOfHotKeyHelp - Index / 3;

    //
    // Help string can't exceed ColumnWidth. One Row will show three Help information. 
    //
    BakChar = L'\0';
    if (StrLen (HotKey->HelpString) > ColumnIndexWidth) {
      BakChar = HotKey->HelpString[ColumnIndexWidth];
      HotKey->HelpString[ColumnIndexWidth] = L'\0';
    }

    //
    // Print HotKey help string on bottom Row.
    //
    if (SetState) {
      ColumnStr = HotKey->HelpString;
    }
    PrintStringAtWithWidth (CurrentCol, CurrentRow, ColumnStr, ColumnIndexWidth);

    if (BakChar != L'\0') {
      HotKey->HelpString[ColumnIndexWidth] = BakChar;
    }
    //
    // Get Next Hot Key.
    //
    Link = GetNextNode (&FormData->HotKeyListHead, Link);
    Index ++;
  }
  
  if (SetState) {
    //
    // Clear KeyHelp
    //
    CurrentRow  = BottomRowOfHotKeyHelp - Index / 3;
    ColumnIndex = Index % 3;
    if (ColumnIndex == 0) {
      CurrentCol       = LocalScreen.LeftColumn + 2 * ColumnWidth;
      ColumnIndexWidth = ColumnWidth - 1;
      ColumnIndex ++;
      PrintStringAtWithWidth (CurrentCol, CurrentRow, gLibEmptyString, ColumnIndexWidth);
    }
    if (ColumnIndex == 1) {
      CurrentCol       = LocalScreen.LeftColumn + ColumnWidth;
      ColumnIndexWidth = ColumnWidth;
      PrintStringAtWithWidth (CurrentCol, CurrentRow, gLibEmptyString, ColumnIndexWidth);
    }
  }
  
  return;
}

/**
  Get step info from numeric opcode.
  
  @param[in] OpCode     The input numeric op code.

  @return step info for this opcode.
**/
UINT64
LibGetFieldFromNum (
  IN  EFI_IFR_OP_HEADER     *OpCode
  )
{
  EFI_IFR_NUMERIC       *NumericOp;
  UINT64                Step;

  NumericOp = (EFI_IFR_NUMERIC *) OpCode;
  
  switch (NumericOp->Flags & EFI_IFR_NUMERIC_SIZE) {
  case EFI_IFR_NUMERIC_SIZE_1:
    Step    = NumericOp->data.u8.Step;
    break;
  
  case EFI_IFR_NUMERIC_SIZE_2:
    Step    = NumericOp->data.u16.Step;
    break;
  
  case EFI_IFR_NUMERIC_SIZE_4:
    Step    = NumericOp->data.u32.Step;
    break;
  
  case EFI_IFR_NUMERIC_SIZE_8:
    Step    = NumericOp->data.u64.Step;
    break;
  
  default:
    Step = 0;
    break;
  }

  return Step;
}

/**
  Initialize the HII String Token to the correct values.

**/
VOID
InitializeLibStrings (
  VOID
  )
{
  mLibUnknownString        = L"!";

  gEnterString          = LibGetToken (STRING_TOKEN (ENTER_STRING), mCDLStringPackHandle);
  gEnterCommitString    = LibGetToken (STRING_TOKEN (ENTER_COMMIT_STRING), mCDLStringPackHandle);
  gEnterEscapeString    = LibGetToken (STRING_TOKEN (ENTER_ESCAPE_STRING), mCDLStringPackHandle);
  gEscapeString         = LibGetToken (STRING_TOKEN (ESCAPE_STRING), mCDLStringPackHandle);
  gMoveHighlight        = LibGetToken (STRING_TOKEN (MOVE_HIGHLIGHT), mCDLStringPackHandle);
  gDecNumericInput      = LibGetToken (STRING_TOKEN (DEC_NUMERIC_INPUT), mCDLStringPackHandle);
  gHexNumericInput      = LibGetToken (STRING_TOKEN (HEX_NUMERIC_INPUT), mCDLStringPackHandle);
  gToggleCheckBox       = LibGetToken (STRING_TOKEN (TOGGLE_CHECK_BOX), mCDLStringPackHandle);

  gAreYouSure           = LibGetToken (STRING_TOKEN (ARE_YOU_SURE), mCDLStringPackHandle);
  gYesResponse          = LibGetToken (STRING_TOKEN (ARE_YOU_SURE_YES), mCDLStringPackHandle);
  gNoResponse           = LibGetToken (STRING_TOKEN (ARE_YOU_SURE_NO), mCDLStringPackHandle);
  gPlusString           = LibGetToken (STRING_TOKEN (PLUS_STRING), mCDLStringPackHandle);
  gMinusString          = LibGetToken (STRING_TOKEN (MINUS_STRING), mCDLStringPackHandle);
  gAdjustNumber         = LibGetToken (STRING_TOKEN (ADJUST_NUMBER), mCDLStringPackHandle);
  gSaveChanges          = LibGetToken (STRING_TOKEN (SAVE_CHANGES), mCDLStringPackHandle);

  gLibEmptyString       = LibGetToken (STRING_TOKEN (EMPTY_STRING), mCDLStringPackHandle);

  gNvUpdateMessage      = LibGetToken (STRING_TOKEN (NV_UPDATE_MESSAGE), mCDLStringPackHandle);
  gInputErrorMessage    = LibGetToken (STRING_TOKEN (INPUT_ERROR_MESSAGE), mCDLStringPackHandle);

  gByoCopyRight = LibGetToken (STRING_TOKEN (BYO_COPY_RIGHT), mCDLStringPackHandle);
  gByoFunctionKeyOne = LibGetToken (STRING_TOKEN (BYO_FUNCTION_KEY_ONE), mCDLStringPackHandle);
  gByoKeyPlusAndMinus = LibGetToken (STRING_TOKEN (BYO_KEY_PLUS_AND_MINUS), mCDLStringPackHandle);
  gByoKeyPlusAndMinusHelp = LibGetToken (STRING_TOKEN (BYO_KEY_PLUS_AND_MINUS_HELP), mCDLStringPackHandle);
  gByoFunctionKeyNine = LibGetToken (STRING_TOKEN (BYO_FUNCTION_KEY_NINE), mCDLStringPackHandle);
  gByoFunctionKeyNineHelp = LibGetToken (STRING_TOKEN (BYO_FUNCTION_KEY_NINE_HELP), mCDLStringPackHandle);  	
  gByoFunctionKeyEsc = LibGetToken (STRING_TOKEN (BYO_FUNCTION_KEY_ESC), mCDLStringPackHandle);
  gByoFunctionKeyEscHelp = LibGetToken (STRING_TOKEN (BYO_FUNCTION_KEY_ESC_HELP), mCDLStringPackHandle);
  gByoFunctionKeyEnter = LibGetToken (STRING_TOKEN (BYO_FUNCTION_KEY_ENTER), mCDLStringPackHandle);
  gByoFunctionKeyEnterHelp = LibGetToken (STRING_TOKEN (BYO_FUNCTION_KEY_ENTER_HELP), mCDLStringPackHandle);
  gByoFunctionKeyEnterTen = LibGetToken (STRING_TOKEN (BYO_FUNCTION_KEY_TEN), mCDLStringPackHandle);
  gByoFunctionKeyEnterTenHelp = LibGetToken (STRING_TOKEN (BYO_FUNCTION_KEY_TEN_HELP), mCDLStringPackHandle);
  gByoFunctionKeyOneHelp = LibGetToken (STRING_TOKEN (BYO_FUNCTION_KEY_ONE_HELP), mCDLStringPackHandle);
  gKeyUpDownHelp = LibGetToken (STRING_TOKEN (BYO_KEY_UP_DOWN_HELP), mCDLStringPackHandle);
  gByoKeyLeftRightHelp = LibGetToken (STRING_TOKEN (BYO_SELECT_MENU_HELP), mCDLStringPackHandle);
  gByoHelpMessage = LibGetToken (STRING_TOKEN (BYO_HELP_MESSAGE), mCDLStringPackHandle);
  gByoGeneralHelp = LibGetToken (STRING_TOKEN (BYO_GENERAL_HELP), mCDLStringPackHandle);
  gByoGeneralHelp1 = LibGetToken (STRING_TOKEN (BYO_GENERAL_HELP_1), mCDLStringPackHandle);
  gByoGeneralHelp2 = LibGetToken (STRING_TOKEN (BYO_GENERAL_HELP_2), mCDLStringPackHandle);  
  gByoGeneralHelp3 = LibGetToken (STRING_TOKEN (BYO_GENERAL_HELP_3), mCDLStringPackHandle);
  gByoContinue = LibGetToken (STRING_TOKEN (BYO_CONTINUE), mCDLStringPackHandle);
  gByoGeneralHelp2_F3 = LibGetToken (STRING_TOKEN (BYO_GENERAL_HELP_2_F3), mCDLStringPackHandle);  
  gByoGeneralHelp3_F4 = LibGetToken (STRING_TOKEN (BYO_GENERAL_HELP_3_F4), mCDLStringPackHandle);
  gByoFunctionKeyThree = LibGetToken (STRING_TOKEN (BYO_FUNCTION_KEY_THREE), mCDLStringPackHandle);
  gByoFunctionKeyFour = LibGetToken (STRING_TOKEN (BYO_FUNCTION_KEY_FOUR), mCDLStringPackHandle);
  gByoSetupConfig = LibGetToken (STRING_TOKEN (STR_BYO_SETUP_CONFIRMATION), mCDLStringPackHandle);
  
  //
  // SpaceBuffer;
  //
  mSpaceBuffer = AllocatePool ((SPACE_BUFFER_SIZE + 1) * sizeof (CHAR16));
  ASSERT (mSpaceBuffer != NULL);
  LibSetUnicodeMem (mSpaceBuffer, SPACE_BUFFER_SIZE, L' ');
  mSpaceBuffer[SPACE_BUFFER_SIZE] = L'\0';
}

/**
  Not FreePool NULL;
  Set string point to NUll afte FreePool.

**/
VOID
SafeFree (
  CHAR16 **String
  )
{
  if (NULL != *String) {
    FreePool(*String);
    *String = NULL;
  }
  return;
}

/**
  Free the HII String.

**/
VOID
FreeLibStrings (
  VOID
  )
{
  FreePool (gEnterString);
  FreePool (gEnterCommitString);
  FreePool (gEnterEscapeString);
  FreePool (gEscapeString);
  FreePool (gMoveHighlight);
  FreePool (gDecNumericInput);
  FreePool (gHexNumericInput);
  FreePool (gToggleCheckBox);

  FreePool (gAreYouSure);
  FreePool (gYesResponse);
  FreePool (gNoResponse);
  FreePool (gPlusString);
  FreePool (gMinusString);
  FreePool (gAdjustNumber);
  FreePool (gSaveChanges);

  FreePool (gLibEmptyString);

  FreePool (gNvUpdateMessage);
  FreePool (gInputErrorMessage);

  SafeFree (&gByoCopyRight);
  SafeFree (&gByoFunctionKeyOne);
  SafeFree (&gByoKeyPlusAndMinus);
  SafeFree (&gByoKeyPlusAndMinusHelp);  
  SafeFree (&gByoFunctionKeyNine);
  SafeFree (&gByoFunctionKeyNineHelp);  
  SafeFree (&gByoFunctionKeyEsc);
  SafeFree (&gByoFunctionKeyEscHelp);  
  SafeFree (&gByoFunctionKeyEnter);
  SafeFree (&gByoFunctionKeyEnterHelp);  
  SafeFree (&gByoFunctionKeyEnterTen);
  SafeFree (&gByoFunctionKeyEnterTenHelp);  
  SafeFree (&gByoFunctionKeyOneHelp);
  SafeFree (&gKeyUpDownHelp);
  SafeFree (&gByoKeyLeftRightHelp);  
  SafeFree (&gByoHelpMessage);  
  SafeFree (&gByoGeneralHelp);
  SafeFree (&gByoGeneralHelp1);
  SafeFree (&gByoGeneralHelp2);  
  SafeFree (&gByoGeneralHelp3);   
  SafeFree (&gByoContinue);
  SafeFree (&gByoGeneralHelp2_F3);  
  SafeFree (&gByoGeneralHelp3_F4);   
  SafeFree (&gByoFunctionKeyThree);
  SafeFree (&gByoFunctionKeyFour);
  SafeFree (&gByoSetupConfig);
  
  FreePool (mSpaceBuffer);

}

/**
  Wait for a key to be pressed by user.

  @param Key         The key which is pressed by user.

  @retval EFI_SUCCESS The function always completed successfully.

**/
EFI_STATUS
WaitForKeyStroke (
  OUT  EFI_INPUT_KEY           *Key
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  while (TRUE) {
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, Key);
    if (!EFI_ERROR (Status)) {
      break;
    }

    if (Status != EFI_NOT_READY) {
      continue;
    }
    
    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &Index);
  }
  return Status;
}

/**
  Set Buffer to Value for Size bytes.

  @param  Buffer                 Memory to set.
  @param  Size                   Number of bytes to set
  @param  Value                  Value of the set operation.

**/
VOID
LibSetUnicodeMem (
  IN VOID   *Buffer,
  IN UINTN  Size,
  IN CHAR16 Value
  )
{
  CHAR16  *Ptr;

  Ptr = Buffer;
  while ((Size--)  != 0) {
    *(Ptr++) = Value;
  }
}

/**
  The internal function prints to the EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
  protocol instance.

  @param Width           Width of string to be print.
  @param Column          The position of the output string.
  @param Row             The position of the output string.
  @param Out             The EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL instance.
  @param Fmt             The format string.
  @param Args            The additional argument for the variables in the format string.

  @return Number of Unicode character printed.

**/
UINTN
PrintInternal (
  IN UINTN                            Width, 
  IN UINTN                            Column,
  IN UINTN                            Row,
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *Out,
  IN CHAR16                           *Fmt,
  IN VA_LIST                          Args
  )
{
  CHAR16  *Buffer;
  CHAR16  *BackupBuffer;
  UINTN   Index;
  UINTN   PreviousIndex;
  UINTN   Count;
  UINTN   TotalCount;
  UINTN   PrintWidth;
  UINTN   CharWidth;

  //
  // For now, allocate an arbitrarily long buffer
  //
  Buffer        = AllocateZeroPool (0x10000);
  BackupBuffer  = AllocateZeroPool (0x10000);
  ASSERT (Buffer);
  ASSERT (BackupBuffer);

  if (Column != (UINTN) -1) {
    Out->SetCursorPosition (Out, Column, Row);
  }

  UnicodeVSPrint (Buffer, 0x10000, Fmt, Args);

  Out->Mode->Attribute = Out->Mode->Attribute & 0x7f;

  Out->SetAttribute (Out, Out->Mode->Attribute);

  Index         = 0;
  PreviousIndex = 0;
  Count         = 0;
  TotalCount    = 0;
  PrintWidth    = 0;
  CharWidth     = 1;

  do {
    for (; (Buffer[Index] != NARROW_CHAR) && (Buffer[Index] != WIDE_CHAR) && (Buffer[Index] != 0); Index++) {
      BackupBuffer[Index] = Buffer[Index];
    }

    if (Buffer[Index] == 0) {
      break;
    }

    //
    // Print this out, we are about to switch widths
    //
    Out->OutputString (Out, &BackupBuffer[PreviousIndex]);
    Count = StrLen (&BackupBuffer[PreviousIndex]);
    PrintWidth += Count * CharWidth;
    TotalCount += Count * CharWidth;

    //
    // Preserve the current index + 1, since this is where we will start printing from next
    //
    PreviousIndex = Index + 1;

    //
    // We are at a narrow or wide character directive.  Set attributes and strip it and print it
    //
    if (Buffer[Index] == NARROW_CHAR) {
      //
      // Preserve bits 0 - 6 and zero out the rest
      //
      Out->Mode->Attribute = Out->Mode->Attribute & 0x7f;
      Out->SetAttribute (Out, Out->Mode->Attribute);
      CharWidth = 1;
    } else {
      //
      // Must be wide, set bit 7 ON
      //
      Out->Mode->Attribute = Out->Mode->Attribute | EFI_WIDE_ATTRIBUTE;
      Out->SetAttribute (Out, Out->Mode->Attribute);
      CharWidth = 2;
    }

    Index++;

  } while (Buffer[Index] != 0);

  //
  // We hit the end of the string - print it
  //
  Out->OutputString (Out, &BackupBuffer[PreviousIndex]);
  Count = StrLen (&BackupBuffer[PreviousIndex]);
  PrintWidth += Count * CharWidth;
  TotalCount += Count * CharWidth;
  if (PrintWidth < Width) {
    Out->Mode->Attribute = Out->Mode->Attribute & 0x7f;
    Out->SetAttribute (Out, Out->Mode->Attribute);
    Out->OutputString (Out, &mSpaceBuffer[SPACE_BUFFER_SIZE - Width + PrintWidth]);
  }

  FreePool (Buffer);
  FreePool (BackupBuffer);
  return TotalCount;
}

/**
  Prints a formatted unicode string to the default console, at
  the supplied cursor position.

  @param  Width      Width of String to be printed.
  @param  Column     The cursor position to print the string at.
  @param  Row        The cursor position to print the string at.
  @param  Fmt        Format string.
  @param  ...        Variable argument list for format string.

  @return Length of string printed to the console

**/
UINTN
EFIAPI
PrintAt (
  IN UINTN     Width,
  IN UINTN     Column,
  IN UINTN     Row,
  IN CHAR16    *Fmt,
  ...
  )
{
  VA_LIST Args;
  UINTN   LengthOfPrinted;

  VA_START (Args, Fmt);
  LengthOfPrinted = PrintInternal (Width, Column, Row, gST->ConOut, Fmt, Args);
  VA_END (Args);
  return LengthOfPrinted;
}

