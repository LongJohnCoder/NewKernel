/** @file

Copyright (c) 2006 - 2016, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SystemPassword.c

Abstract:
  Implementation of basic setup password function.

Revision History:

**/
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>

#include <SystemPasswordVariable.h>
#include <Library/SystemPasswordLib.h>
#include <Protocol/SystemPasswordProtocol.h>
#include <Protocol/ByoFormSetManager.h>


#define ERROE_MESSAGE_WIDTH    (45)

VERIFY_MODE    gVerifyMode = VERIFY_MODE_MAX;
BOOLEAN    bPasswordChecked = FALSE;

VOID
SetUnicodeMem (
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



EFI_STATUS  
GetPasswordKey(  
  EFI_INPUT_KEY    *Key
  )
{
  EFI_STATUS    Status;
  EFI_KEY_DATA    KeyData;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL    *ConInEx = NULL;
  UINT32    KeyShiftState;
  UINTN    Index;
  
  EFI_EVENT   TimerEvent;
  EFI_EVENT   WaitEventList[2];
  BOOLEAN    CursorFlag;

  Status = gBS->HandleProtocol(
                  gST->ConsoleInHandle, 
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID**)&ConInEx
                  );
  if(EFI_ERROR(Status)){
    Status = gBS->LocateProtocol(&gEfiSimpleTextInputExProtocolGuid, NULL, (VOID**)&ConInEx);
    if(EFI_ERROR(Status)){
      return Status;
    }
  }
  //
  //Create time event to flash Cursor.
  //
  WaitEventList[0] = ConInEx->WaitForKeyEx;
  Index = 1;
  CursorFlag = TRUE;
  while(Index == 1) {
    Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);
    if (!EFI_ERROR (Status)) {
      gBS->SetTimer (
             TimerEvent,
             TimerRelative,
             10000*300
             );
      //
      // Wait for the original event or the timer
      //
      WaitEventList[1] = TimerEvent;
      Status = gBS->WaitForEvent (2, WaitEventList, &Index);
      DMSG ((EFI_D_ERROR, "\n GetPasswordKey, Evney Index :%d-%r.\n", Index, Status));

      gST->ConOut->EnableCursor (gST->ConOut, CursorFlag);
      CursorFlag =  !CursorFlag;
    }
  }
  //
  // Read input Key.
  //
  Status = ConInEx->ReadKeyStrokeEx(ConInEx, &KeyData);
  if(!EFI_ERROR(Status)){
    Key->ScanCode = KeyData.Key.ScanCode;
    Key->UnicodeChar = KeyData.Key.UnicodeChar;		

    KeyShiftState = KeyData.KeyState.KeyShiftState;
    if(KeyShiftState & EFI_SHIFT_STATE_VALID){
      if(KeyShiftState & (EFI_RIGHT_ALT_PRESSED | 
                        EFI_LEFT_ALT_PRESSED | 
                        EFI_RIGHT_CONTROL_PRESSED | 
                        EFI_LEFT_CONTROL_PRESSED)) {
        Key->ScanCode = SCAN_NULL;
        Key->UnicodeChar = CHAR_NULL;		
      }                      
    }
  }
  
  return Status;
}

/**
  Input Paasword from KB.

**/
EFI_STATUS
InputPassword  (
  IN UINTN               Column,
  IN UINTN               Row,
  IN OUT CHAR16     *StringPtr
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  UINTN    CurrentCursor;
  BOOLEAN    CursorVisible;
  EFI_INPUT_KEY    Key;
  CHAR16    *BufferedString;
  CHAR16    *TempString;
  UINTN    Index;
  UINTN    Index2;
  UINTN    Count;
  UINTN    StringLen;
  UINTN    PasswordColor;
  CHAR16    KeyPad[2];
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *ConOut;

  DMSG ((EFI_D_ERROR, "\n InputPassword, StringPtr :%s.\n", StringPtr));
  
  BufferedString = AllocateZeroPool ((MAX_PASSWORD_LENGTH + 1) * sizeof (CHAR16));
  ASSERT (BufferedString);

  TempString = AllocateZeroPool ((MAX_PASSWORD_LENGTH + 1)* sizeof (CHAR16));
  ASSERT (TempString);

  PasswordColor = EFI_WHITE | EFI_BACKGROUND_BLACK;
  ConOut = gST->ConOut;

  CursorVisible = gST->ConOut->Mode->CursorVisible;
  gST->ConOut->SetCursorPosition (gST->ConOut, Column, Row);
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);

  //
  // Clean more input before entering this.
  //
  while (gST->ConIn != NULL) {
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);    
    if (EFI_ERROR (Status)) {
       break;
    }
  }  
  
  //
  //Main Loop.
  //
  CurrentCursor = 0;
  do {
    Status = GetPasswordKey(&Key);
    if (EFI_ERROR (Status)) {
      continue;
    }

    DMSG ((EFI_D_ERROR, "\n InputPassword, Key :%c-%d.\n", Key.UnicodeChar, Key.ScanCode));	
    gST->ConOut->SetAttribute (gST->ConOut, PasswordColor);
    switch (Key.UnicodeChar) {
    case CHAR_NULL:
      switch (Key.ScanCode) {
      case SCAN_LEFT:
        if (CurrentCursor > 0) {
          CurrentCursor--;
        }
        break;

      case SCAN_RIGHT:
        if (CurrentCursor < (StrLen (StringPtr))) {
          CurrentCursor++;
        }
        break;

      default:
        break;
      }
      break;

    case CHAR_CARRIAGE_RETURN:
      DMSG ((EFI_D_ERROR, "InputPassword, CHAR_CARRIAGE_RETURN, StringPtr :%s.\n", StringPtr));
      if (StrLen (StringPtr) >= sizeof (CHAR16)) {

        FreePool (TempString);
        FreePool (BufferedString);
        gST->ConOut->SetAttribute (gST->ConOut, PasswordColor);
        gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
        return EFI_SUCCESS;
      }
      if (*StringPtr == CHAR_NULL) {
        continue;
      }
	  
      FreePool (TempString);
      FreePool (BufferedString);
      gST->ConOut->SetAttribute (gST->ConOut, PasswordColor);
      gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
      return EFI_DEVICE_ERROR;
      break;

    case CHAR_BACKSPACE:
      DMSG ((EFI_D_ERROR, "InputPassword, CHAR_BACKSPACE, CurrentCursor :%d.\n", CurrentCursor));
      if (StringPtr[0] != CHAR_NULL && CurrentCursor != 0) {
        for (Index = 0; Index < CurrentCursor - 1; Index++) {
          TempString[Index] = StringPtr[Index];
        }
        Count = StrLen (StringPtr);
        if (Count >= CurrentCursor) {
          for (Index = CurrentCursor - 1, Index2 = CurrentCursor; Index2 < Count; Index++, Index2++) {
            TempString[Index] = StringPtr[Index2];
          }
          TempString[Index] = CHAR_NULL;
        }
        //
        // Effectively truncate string by 1 character
        //
        StrCpy (StringPtr, TempString);
        CurrentCursor --;
      }

    default:
      //
      // Check invallid char.
      //
      if (!IsValidPdKey(&Key) && Key.UnicodeChar != CHAR_BACKSPACE) {
        break;
      }

      //
      // Add new char.
      //
      StringLen = StrLen (StringPtr);
      if (CHAR_BACKSPACE == Key.UnicodeChar) {
        if (StringPtr[0] == CHAR_NULL) {
          SetUnicodeMem (BufferedString, MAX_PASSWORD_LENGTH, L' ');
          BufferedString[MAX_PASSWORD_LENGTH] = CHAR_NULL;

          ConOut->SetCursorPosition(ConOut, Column, Row);
          ConOut->OutputString(ConOut, BufferedString);

          gST->ConOut->SetCursorPosition (gST->ConOut, Column, Row);
          gST->ConOut->EnableCursor (gST->ConOut, TRUE);          
          break;
        }
      } else if (StringPtr[0] == CHAR_NULL) {
        StrnCpy (StringPtr, &Key.UnicodeChar, 1);
        StringLen = 1;
        CurrentCursor++;
      } else if (StringLen < MAX_PASSWORD_LENGTH) {
        KeyPad[0] = Key.UnicodeChar;
        KeyPad[1] = CHAR_NULL;
        Count = StringLen;
        if (CurrentCursor < Count) {
          for (Index = 0; Index < CurrentCursor; Index++) {
            TempString[Index] = StringPtr[Index];
          }
          TempString[Index] = CHAR_NULL;
          StrCat (TempString, KeyPad);
          StrCat (TempString, StringPtr + CurrentCursor);
          StrCpy (StringPtr, TempString);
        } else {
          StrCat (StringPtr, KeyPad);
        }
        CurrentCursor++;
        StringLen++;
      } else {
        DMSG ((EFI_D_ERROR, "InputPassword, Exceed MAX_PASSWORD_LENGTH.\n"));
        continue;
      }

      //
      // Print "*".
      //
      gST->ConOut->EnableCursor (gST->ConOut, FALSE);
      SetUnicodeMem (BufferedString, MAX_PASSWORD_LENGTH, L' ');
      BufferedString[MAX_PASSWORD_LENGTH] = CHAR_NULL;
      ConOut->SetCursorPosition(ConOut, Column, Row);
      ConOut->OutputString(ConOut, BufferedString);	  
      DMSG ((EFI_D_ERROR, "InputPassword, StringLen :%d.\n", StringLen));	  
      if (StringLen > 0) {
        SetUnicodeMem (BufferedString, StringLen, L'*');
        BufferedString[StringLen + 1] = CHAR_NULL;
        ConOut->SetCursorPosition(ConOut, Column, Row);
        ConOut->OutputString(ConOut, BufferedString);
      }
      break;
    }

    gST->ConOut->SetAttribute (gST->ConOut, PasswordColor);
    gST->ConOut->SetCursorPosition (gST->ConOut, Column + CurrentCursor, Row);
    gST->ConOut->EnableCursor (gST->ConOut, TRUE);
  } while (TRUE);

  return Status;
}


VOID
FatalErrorMessage (
  VOID
  )
{
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *ConOut;
  EFI_SIMPLE_TEXT_OUTPUT_MODE        SavedConsoleMode;
  UINTN                              Column;
  UINTN                              Row;
  UINTN                              AttribComm;
  CHAR16                             *Line;
  EFI_INPUT_KEY                      Key;

  CHAR16    *Title = L"Fatal Error ... System Halted.";

  AttribComm = EFI_WHITE |EFI_BACKGROUND_RED | EFI_BRIGHT;
  ConOut = gST->ConOut;

  CopyMem(&SavedConsoleMode, ConOut->Mode, sizeof(SavedConsoleMode));
  ConOut->QueryMode(ConOut, SavedConsoleMode.Mode, &Column, &Row);
  ConOut->EnableCursor(ConOut, FALSE);
  ConOut->SetAttribute(ConOut, AttribComm);
  Row    = (Row - 3) / 2;
  Column = (Column - ERROE_MESSAGE_WIDTH) / 2;

  Line = AllocateZeroPool((ERROE_MESSAGE_WIDTH + 3) * sizeof(CHAR16));
  // line 1.
  SetMem16(Line, (ERROE_MESSAGE_WIDTH + 2) * 2, BOXDRAW_HORIZONTAL);
  Line[0]             = BOXDRAW_DOWN_RIGHT;
  Line[ERROE_MESSAGE_WIDTH + 1] = BOXDRAW_DOWN_LEFT;
  Line[ERROE_MESSAGE_WIDTH + 2] = L'\0';

  ConOut->SetCursorPosition(ConOut, Column, Row++);
  ConOut->OutputString(ConOut, Line);

  // line 2.
  SetMem16(Line, (ERROE_MESSAGE_WIDTH + 2) * 2, L' ');
  Line[0]             = BOXDRAW_VERTICAL;
  Line[ERROE_MESSAGE_WIDTH + 1] = BOXDRAW_VERTICAL;
  Line[ERROE_MESSAGE_WIDTH + 2] = L'\0';
  ConOut->SetCursorPosition(ConOut, Column, Row);
  ConOut->OutputString(ConOut, Line);
  ConOut->SetCursorPosition(ConOut, Column + 1 + (ERROE_MESSAGE_WIDTH - StrLen(Title)) / 2, Row++);
  ConOut->OutputString(ConOut, Title);

  //line 3.
  SetMem16(Line, (ERROE_MESSAGE_WIDTH + 2) * 2, BOXDRAW_HORIZONTAL);
  Line[0]             = BOXDRAW_UP_RIGHT;
  Line[ERROE_MESSAGE_WIDTH + 1] = BOXDRAW_UP_LEFT;
  Line[ERROE_MESSAGE_WIDTH + 2] = L'\0';
  ConOut->SetCursorPosition(ConOut, Column, Row++);
  ConOut->OutputString(ConOut, Line);
  FreePool(Line);
  
  ConOut->EnableCursor(ConOut, SavedConsoleMode.CursorVisible);
  ConOut->SetCursorPosition(ConOut, SavedConsoleMode.CursorColumn, SavedConsoleMode.CursorRow);
  ConOut->SetAttribute(ConOut, SavedConsoleMode.Attribute);

  while (gST->ConIn) {
    gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);

    gBS->RaiseTPL(TPL_HIGH_LEVEL);
    CpuDeadLoop();
	
  }

  return;
}

EFI_STATUS
ErrorWarning (
  IN  PASSWORD_TYPE    Type
  )
{
  EFI_STATUS    Status;
  UINTN    VariableSize;
  EFI_GUID    PasswordGuid = SYSTEM_PASSWORD_GUID;
  SYSTEM_PASSWORD    SetupPassword;

  VariableSize = sizeof(SYSTEM_PASSWORD);
  Status = gRT->GetVariable (
                  SYSTEM_PASSWORD_NAME,
                  &PasswordGuid,
                  NULL,
                  &VariableSize,
                  &SetupPassword
                  );
  if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
  }

  //
  // Check verify times.
  //
  if (VERIFY_MODE_INTO_POP == gVerifyMode ||
    VERIFY_MODE_INTO_SETUP == gVerifyMode ) {
    if (SetupPassword.VerifyTimes < MAX_VERIFY_TIMES ) {
      return EFI_NOT_READY;
    }
    SetupPassword.VerifyTimes = 0;
  } else if (VERIFY_MODE_SETUP == gVerifyMode) {
    if (PD_ADMIN == Type) {
      if (SetupPassword.VerifyTimesAdmin < MAX_VERIFY_TIMES ) {
        return EFI_NOT_READY;
      }
      SetupPassword.VerifyTimesAdmin = 0;
    } else if (PD_POWER_ON == Type) {
      if (SetupPassword.VerifyTimesPop < MAX_VERIFY_TIMES ) {
        return EFI_NOT_READY;
      }
      SetupPassword.VerifyTimesPop = 0;
    }
  } else if (VERIFY_MODE_HDP== gVerifyMode) {
    return EFI_NOT_READY;  // transfer error handing to HDP.
  } else {
    return EFI_NOT_READY;
  }


  //
  // 1. Clear VerifyTimes.
  //
  gRT->SetVariable (
                SYSTEM_PASSWORD_NAME,
                &gEfiSystemPasswordVariableGuid,
                EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                VariableSize,
                &SetupPassword
                );

  //
  // 2. Warning dialog.
  //
  FatalErrorMessage();

  return EFI_SUCCESS;
}


EFI_STATUS
ReadPassword(
  IN  PASSWORD_TYPE    Type,
  OUT UINT8    **Password
  )
{
  EFI_STATUS    Status;
  UINTN             VariableSize;
  EFI_GUID        PasswordGuid = SYSTEM_PASSWORD_GUID;
  SYSTEM_PASSWORD  SetupPassword;
  UINT8             *PasswordBuf = NULL;

  VariableSize = sizeof(SYSTEM_PASSWORD);
  Status = gRT->GetVariable (
                  SYSTEM_PASSWORD_NAME,
                  &PasswordGuid,
                  NULL,
                  &VariableSize,
                  &SetupPassword
                  );
  if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
  }

  switch (Type) {
  case PD_ADMIN:
    PasswordBuf = AllocateCopyPool (SYSTEM_PASSWORD_HASH_LENGTH, &SetupPassword.AdminHash);

    break;
  case PD_POWER_ON:
    PasswordBuf = AllocateCopyPool (SYSTEM_PASSWORD_HASH_LENGTH, &SetupPassword.PowerOnHash);
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  *Password = PasswordBuf;
  return EFI_SUCCESS;
}

EFI_STATUS
WritePassword(
  IN  PASSWORD_TYPE    Type,
  IN  CHAR16    *Password
  )
{
  EFI_STATUS    Status;
  UINTN    VariableSize;
  EFI_GUID    PasswordGuid = SYSTEM_PASSWORD_GUID;
  SYSTEM_PASSWORD    SetupPassword;
  UINTN    Len;
  UINT8    *Str;
  UINT8    *PasswordHash;

  VariableSize = sizeof(SYSTEM_PASSWORD);
  Status = gRT->GetVariable (
                  SYSTEM_PASSWORD_NAME,
                  &PasswordGuid,
                  NULL,
                  &VariableSize,
                  &SetupPassword
                  );
  if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
  }

  switch (Type) {
    case PD_ADMIN:
      Len = Unicode2Ascii(Password, &Str);
      if (0 < Len ) {
        EncodePassword(Str, Len, &PasswordHash,0);
        CopyMem(&SetupPassword.AdminHash, PasswordHash, SYSTEM_PASSWORD_HASH_LENGTH);
        SetupPassword.bHaveAdmin = TRUE;
        FreePool (Str);
        FreePool (PasswordHash);
      } else {
        ZeroMem(&SetupPassword.AdminHash, SYSTEM_PASSWORD_HASH_LENGTH);
        SetupPassword.bHaveAdmin = FALSE;
      }
      break;

    case PD_POWER_ON:
      Len = Unicode2Ascii(Password, &Str);
      if (0 < Len ) {
        EncodePassword(Str, Len, &PasswordHash,0);
        CopyMem(&SetupPassword.PowerOnHash, PasswordHash, SYSTEM_PASSWORD_HASH_LENGTH);
        SetupPassword.bHavePowerOn = TRUE;
        FreePool (Str);
        FreePool (PasswordHash);
      } else {
        ZeroMem(&SetupPassword.PowerOnHash, SYSTEM_PASSWORD_HASH_LENGTH);
        SetupPassword.bHavePowerOn = FALSE;
      }
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  SetupPassword.VerifyTimes = 0;

  Status = gRT->SetVariable (
                    SYSTEM_PASSWORD_NAME,
                    &gEfiSystemPasswordVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    VariableSize,
                    &SetupPassword
                    );

  return Status;
}

EFI_STATUS
SetVerifyMode (
  IN VERIFY_MODE    Mode
  )
{
  if (VERIFY_MODE_MAX > Mode) {
    gVerifyMode = Mode;
  } else {
    gVerifyMode = VERIFY_MODE_MAX;
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

BOOLEAN
GetPopCheckedStatus (
  VOID
  )
{
  return bPasswordChecked;
}

EFI_STATUS
SetVerifyTimes(
  IN  PASSWORD_TYPE    Type,
  IN VERIFY_TIMES_ACTION    Action
  )
{
  EFI_STATUS    Status;
  UINTN    VariableSize;
  EFI_GUID    PasswordGuid = SYSTEM_PASSWORD_GUID;
  SYSTEM_PASSWORD    SetupPassword;

  if (VERIFY_MODE_MAX == gVerifyMode) {
    return EFI_NOT_READY;
  }

  VariableSize = sizeof(SYSTEM_PASSWORD);
  Status = gRT->GetVariable (
                  SYSTEM_PASSWORD_NAME,
                  &PasswordGuid,
                  NULL,
                  &VariableSize,
                  &SetupPassword
                  );
  if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
  }

  switch (gVerifyMode) {
    case VERIFY_MODE_INTO_POP:
    case VERIFY_MODE_INTO_SETUP:
      switch (Action) {
        case VERIFY_ACTION_ADD:
          if (SetupPassword.VerifyTimes < (MAX_VERIFY_TIMES)) {
            SetupPassword.VerifyTimes += 1;
          } else {
            SetupPassword.VerifyTimes = 0;
          }
          break;

        case VERIFY_ACTION_SUB:
          if (SetupPassword.VerifyTimes > 0) {
           SetupPassword.VerifyTimes -= 1;
          } else {
           SetupPassword.VerifyTimes = 0;
          }
          break;

        case VERIFY_ACTION_RESET:
          SetupPassword.VerifyTimes = 0;
          break;

        default:
          return EFI_INVALID_PARAMETER;
      }
      break;
	  
    case VERIFY_MODE_HDP:
      //
      // Not change verify times when it used by HDP.
      //
      break;
	  
    case VERIFY_MODE_SETUP:

      switch (Action) {
        case VERIFY_ACTION_ADD:
          if (PD_ADMIN == Type) {
            if (SetupPassword.VerifyTimesAdmin < (MAX_VERIFY_TIMES)) {
              SetupPassword.VerifyTimesAdmin += 1;
            } else {
              SetupPassword.VerifyTimesAdmin = 0;
            }
          }
          if (PD_POWER_ON == Type) {
            if (SetupPassword.VerifyTimesPop < (MAX_VERIFY_TIMES)) {
              SetupPassword.VerifyTimesPop += 1;
            } else {
              SetupPassword.VerifyTimesPop = 0;
            }
          }
          break;

        case VERIFY_ACTION_SUB:
          if (PD_ADMIN == Type) {
            if (SetupPassword.VerifyTimesAdmin > 0) {
             SetupPassword.VerifyTimesAdmin -= 1;
            } else {
             SetupPassword.VerifyTimesAdmin = 0;
            }
          }
          if (PD_POWER_ON == Type) {
            if (SetupPassword.VerifyTimesPop > 0) {
             SetupPassword.VerifyTimesPop -= 1;
            } else {
             SetupPassword.VerifyTimesPop = 0;
            }
          }
          break;

        case VERIFY_ACTION_RESET:
          if (PD_ADMIN == Type) {
            SetupPassword.VerifyTimesAdmin = 0;
          }
          if (PD_POWER_ON == Type) {
            SetupPassword.VerifyTimesPop = 0;
          }
          break;

        default:
          return EFI_INVALID_PARAMETER;
      }

      break;

    default:
      return EFI_NOT_READY;
  }

  Status = gRT->SetVariable (
                    SYSTEM_PASSWORD_NAME,
                    &gEfiSystemPasswordVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    VariableSize,
                    &SetupPassword
                    );

  return Status;
}


UINT8
GetVerifyTimes(
  IN  PASSWORD_TYPE    Type
  )
{
  EFI_STATUS    Status;
  UINTN    VariableSize;
  EFI_GUID    PasswordGuid = SYSTEM_PASSWORD_GUID;
  SYSTEM_PASSWORD    SetupPassword;
  UINT8    Times = (UINT8) -1;

  VariableSize = sizeof(SYSTEM_PASSWORD);
  Status = gRT->GetVariable (
                  SYSTEM_PASSWORD_NAME,
                  &PasswordGuid,
                  NULL,
                  &VariableSize,
                  &SetupPassword
                  );
  if (!EFI_ERROR (Status)) {
    switch (gVerifyMode) {
      case VERIFY_MODE_INTO_POP:
      case VERIFY_MODE_INTO_SETUP:
        Times = SetupPassword.VerifyTimes;
        break;
  	  
      case VERIFY_MODE_HDP:
        Times = 0;
        break;
  	  
      case VERIFY_MODE_SETUP:
        if (PD_ADMIN == Type) {
          Times = SetupPassword.VerifyTimesAdmin;
        }
        if (PD_POWER_ON == Type) {
          Times = SetupPassword.VerifyTimesPop;
        }
        break;
  
      default:
        break;
    }
  }
  
  return Times;
}

BOOLEAN
BeHavePassword(
  IN  PASSWORD_TYPE    Type
  )
{
  EFI_STATUS    Status;
  UINT8    *PasswordBuf = NULL;
  UINTN    Index;
  BOOLEAN    bHave;

  bHave = FALSE;
  Status = ReadPassword(Type, &PasswordBuf);
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < SYSTEM_PASSWORD_HASH_LENGTH; Index++) {
      if (0 != PasswordBuf[Index]) {
        bHave = TRUE;
        break;
        }
    }
  }

  if (NULL != PasswordBuf) FreePool(PasswordBuf);
  return bHave;
}


BOOLEAN
VerifyPassword(
  IN  PASSWORD_TYPE    Type,
  IN  CHAR16    *String
  )
{
  UINTN    Index;
  UINTN    Len;
  UINT8    *Str;
  UINT8    *InputHash;
  UINT8    *PasswordHash;

  Len = Unicode2Ascii(String, &Str);
  if (0 < Len) {
    EncodePassword(Str, Len, &InputHash,0);
    FreePool (Str);

    ReadPassword(Type, &PasswordHash);
    for (Index = 0; Index < SYSTEM_PASSWORD_HASH_LENGTH; Index++) {
      if (InputHash[Index] != PasswordHash[Index] ) {
        FreePool (PasswordHash);
        FreePool (InputHash);
        SetVerifyTimes(Type, VERIFY_ACTION_ADD);
        ErrorWarning (Type);
        return FALSE;
      }
    }

    FreePool (InputHash);
    FreePool (PasswordHash);
  } else {

    ReadPassword(Type, &PasswordHash);
    for (Index = 0; Index < SYSTEM_PASSWORD_HASH_LENGTH; Index++) {
      if (0 != PasswordHash[Index] ) {
        FreePool (PasswordHash);
        return FALSE;
      }
    }
  }

  SetVerifyTimes(Type, VERIFY_ACTION_RESET);
  bPasswordChecked = TRUE;
  return TRUE;
}


EFI_STATUS
ClearPassword(
  IN  PASSWORD_TYPE    Type
  )
{
  EFI_STATUS    Status;
  UINTN             VariableSize;
  SYSTEM_PASSWORD  SetupPassword;

  bPasswordChecked = FALSE;

  VariableSize = sizeof(SYSTEM_PASSWORD);
  Status = gRT->GetVariable (
                  SYSTEM_PASSWORD_NAME,
                  &gEfiSystemPasswordVariableGuid,
                  NULL,
                  &VariableSize,
                  &SetupPassword
                  );
  if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
  }

  switch (Type) {
  case PD_ADMIN:
    SetupPassword.bHaveAdmin = FALSE;
    ZeroMem(&SetupPassword.AdminHash, SYSTEM_PASSWORD_HASH_LENGTH);
    SetupPassword.ChangePopByUser = 1;
    break;
  case PD_POWER_ON:
    SetupPassword.bHavePowerOn = FALSE;
    ZeroMem(&SetupPassword.PowerOnHash, SYSTEM_PASSWORD_HASH_LENGTH);
    SetupPassword.RequirePopOnRestart = 0;
    break;
  case PD_MAX:
    SetupPassword.bHaveAdmin = FALSE;
    ZeroMem(&SetupPassword.AdminHash, SYSTEM_PASSWORD_HASH_LENGTH);
    SetupPassword.bHavePowerOn = FALSE;
    ZeroMem(&SetupPassword.PowerOnHash, SYSTEM_PASSWORD_HASH_LENGTH);
    SetupPassword.RequirePopOnRestart = 0;
    SetupPassword.ChangePopByUser = 1;
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  SetupPassword.VerifyTimes = 0;

  Status = gRT->SetVariable (
                    SYSTEM_PASSWORD_NAME,
                    &gEfiSystemPasswordVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    VariableSize,
                    &SetupPassword
                    );

  return Status;
}

UINT8
GetEnteredType (
  VOID
  )
{
  EFI_STATUS    Status;
  UINTN    VariableSize;
  SYSTEM_PASSWORD    SetupPassword;

  VariableSize = sizeof(SYSTEM_PASSWORD);
  Status = gRT->GetVariable (
                  SYSTEM_PASSWORD_NAME,
                  &gEfiSystemPasswordVariableGuid,
                  NULL,
                  &VariableSize,
                  &SetupPassword
                  );
  if (!EFI_ERROR (Status)) {
    return SetupPassword.EnteredType;    
  } else {
    return LOGIN_USER_MAX;
  }
}

//check integrity of password

/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
SystemPasswordInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  EFI_SYSTEM_PASSWORD_PROTOCOL    *SystemPassword = NULL;
  EFI_HANDLE    Handle = NULL;
  SYSTEM_PASSWORD    SetupPassword;
  UINTN    VariableSize;
  UINTN    Index;
	
  //
  // Check password variable.
  //
  VariableSize = sizeof(SYSTEM_PASSWORD);  
  Status = gRT->GetVariable (
                  SYSTEM_PASSWORD_NAME,
                  &gEfiSystemPasswordVariableGuid,
                  NULL,
                  &VariableSize,
                  &SetupPassword
                  );
  DEBUG((EFI_D_INFO, "SystemPasswordInit(), GetVariable(%s):%r-%d\n", SYSTEM_PASSWORD_NAME, Status, VariableSize));	
  if (!EFI_ERROR(Status)) {
    //
    //Check password status.
    //
    if (SetupPassword.bHaveAdmin != BeHavePassword(PD_ADMIN) ||
        SetupPassword.VerifyTimes > 0 ||
        SetupPassword.VerifyTimesAdmin > 0 ||
        SetupPassword.VerifyTimesPop > 0 ||
        SetupPassword.bHavePowerOn != BeHavePassword(PD_POWER_ON)
      ) {
      SetupPassword.bHaveAdmin = BeHavePassword(PD_ADMIN);
      SetupPassword.bHavePowerOn = BeHavePassword(PD_POWER_ON);
      SetupPassword.VerifyTimes = 0;
      SetupPassword.VerifyTimesAdmin = 0;
      SetupPassword.VerifyTimesPop = 0;
      SetupPassword.EnteredType = LOGIN_USER_ADMIN;

      VariableSize = sizeof(SYSTEM_PASSWORD);
      Status = gRT->SetVariable (
                    SYSTEM_PASSWORD_NAME,
                    &gEfiSystemPasswordVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    VariableSize,
                    &SetupPassword
                    );
    }
  } else if(Status == EFI_NOT_FOUND) {
    //
    // First in, we should set variable.
    //
    VariableSize = sizeof(SYSTEM_PASSWORD);
    ZeroMem(&SetupPassword, VariableSize);
	
    SetupPassword.RequirePopOnRestart = 0;
    SetupPassword.ChangePopByUser = 1;
    SetupPassword.EnteredType = LOGIN_USER_ADMIN;

    RandomSeed(NULL, 0);
    for (Index = 0; Index < SYSTEM_PASSWORD_LENGTH; Index++) {	
      RandomBytes((UINT8 *)&SetupPassword.Admin[Index], 2);
    }

    RandomSeed(NULL, 0);
    for (Index = 0; Index < SYSTEM_PASSWORD_LENGTH; Index++) {	
      RandomBytes((UINT8 *)&SetupPassword.PowerOn[Index], 2);
    }
		
    Status = gRT->SetVariable (
                    SYSTEM_PASSWORD_NAME,
                    &gEfiSystemPasswordVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    VariableSize,
                    &SetupPassword
                    );
  }


  //
  // Install password protocol.
  //
  SystemPassword = AllocateZeroPool(sizeof(EFI_SYSTEM_PASSWORD_PROTOCOL));
  if (SystemPassword == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SystemPassword->Read = ReadPassword;
  SystemPassword->Write = WritePassword;
  SystemPassword->BeHave = BeHavePassword;
  SystemPassword->Verify = VerifyPassword;
  SystemPassword->Clear = ClearPassword;
  SystemPassword->Input =  InputPassword;
  SystemPassword->SetMode = SetVerifyMode;
  SystemPassword->SetTimes = SetVerifyTimes;
  SystemPassword->GetTimes = GetVerifyTimes;
  SystemPassword->GetPopChecked = GetPopCheckedStatus;
  SystemPassword->GetEnteredType = GetEnteredType;
  
  Status = gBS->InstallProtocolInterface (
                 &Handle,
                 &gEfiSystemPasswordProtocolGuid,
                 EFI_NATIVE_INTERFACE,
                 SystemPassword
                 );
  ASSERT_EFI_ERROR(Status);

  return Status;
}
