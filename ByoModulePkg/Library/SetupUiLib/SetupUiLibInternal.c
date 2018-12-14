/** @file
Copyright (c) 2017, Byosoft Software Corporation. All Rights Reserved.

You may not reproduce, distribute, publish, display, perform, modify, adapt,
transmit, broadcast, present, recite, release, license or otherwise exploit
any part of this publication in any form, by any means, without the prior
written permission of Byosoft Software Corporation.

File Name:
  SetupUiLibInternal.c

Abstract:
  Setup Ui Library implementation.

Revision History:

**/

#include "SetupUiLibInternal.h"

CHAR16    *mLibUnknownStr; 
CHAR16    *mSpaceBuf;
#define SPACE_BUFFER_SIZE      1000
EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *gBltBuffer = NULL;

CHAR16    *gStrUiSetupConfirmation;
CHAR16    *gStrUiDiscard;
CHAR16    *gStrUiDiscardExit;
CHAR16    *gStrUiLoadDefault;
CHAR16    *gStrUiSaving;
CHAR16    *gStrUiSavingExit;
CHAR16    *gStrUiSaveUserDefault;
CHAR16    *gStrUiLoadUserDefault;
CHAR16    *gStrUiLoadFormset;

/**
  Get the string based on the StringId and HII Package List Handle.

  @param  Token                  The String's ID.
  @param  HiiHandle              The package list in the HII database to search for
                                 the specified string.

  @return The output string.

**/
CHAR16 *
UiGetToken (
  IN  EFI_STRING_ID                Token,
  IN  EFI_HII_HANDLE               HiiHandle
  )
{
  EFI_STRING  String;

  String = HiiGetString (HiiHandle, Token, NULL);
  if (String == NULL) {
    String = AllocateCopyPool (StrSize (mLibUnknownStr), mLibUnknownStr);
    ASSERT (String != NULL);
  }

  return (CHAR16 *) String;
}

/**
  Not FreePool NULL;
  Set string point to NUll afte FreePool.

**/
VOID
UiSafeFree (
  CHAR16 **String
  )
{
  if (NULL == String) {
    return;
  }
  
  if (NULL != *String) {
    FreePool(*String);
    *String = NULL;
  }
  return;
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
UiGetStringWidth (
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
  Initialize the HII String Token to the correct values.

**/
VOID
UiInitializeLibStrings (
  VOID
  )
{
  mLibUnknownStr  = L"!";
  
  //
  // SpaceBuffer;
  //
  mSpaceBuf = AllocatePool ((SPACE_BUFFER_SIZE + 1) * sizeof (CHAR16));
  ASSERT (mSpaceBuf != NULL);
  UiSetUnicodeMem (mSpaceBuf, SPACE_BUFFER_SIZE, L' ');
  mSpaceBuf[SPACE_BUFFER_SIZE] = L'\0';

  DMSG ((EFI_D_ERROR, "\n UiInitializeLibStrings, \n"));
  gStrUiSetupConfirmation = UiGetToken (STRING_TOKEN (STR_UI_SETUP_CONFIRMATION), mSetupUiStringHandle);
  gStrUiDiscard = UiGetToken (STRING_TOKEN (STR_UI_DISCARD), mSetupUiStringHandle);
  gStrUiDiscardExit = UiGetToken (STRING_TOKEN (STR_UI_DISCARD_EXIT), mSetupUiStringHandle);
  gStrUiLoadDefault = UiGetToken (STRING_TOKEN (STR_UI_LOAD_DEFAULT), mSetupUiStringHandle);
  gStrUiLoadFormset  = UiGetToken (STRING_TOKEN (STR_UI_LOAD_FORMSET), mSetupUiStringHandle);
  gStrUiSaving = UiGetToken (STRING_TOKEN (STR_UI_SAVING), mSetupUiStringHandle);
  gStrUiSavingExit = UiGetToken (STRING_TOKEN (STR_UI_SAVING_EXIT), mSetupUiStringHandle);
  gStrUiSaveUserDefault = UiGetToken (STRING_TOKEN (STR_UI_SAVE_USER_DEFAULT), mSetupUiStringHandle);
  gStrUiLoadUserDefault = UiGetToken (STRING_TOKEN (STR_UI_LOAD_USER_DEFAULT), mSetupUiStringHandle);
	
}

/**
  Free the HII String.

**/
VOID
UiFreeLibStrings (
  VOID
  )
{
  FreePool (mSpaceBuf);

  DMSG ((EFI_D_ERROR, "\n UiFreeLibStrings, \n"));
  UiSafeFree(&gStrUiSetupConfirmation);
  UiSafeFree(&gStrUiDiscard);  
  UiSafeFree(&gStrUiDiscardExit);
  UiSafeFree(&gStrUiLoadDefault);
  UiSafeFree(&gStrUiSaving);
  UiSafeFree(&gStrUiSavingExit);
  UiSafeFree(&gStrUiSaveUserDefault);    
  UiSafeFree(&gStrUiLoadUserDefault);    
  UiSafeFree(&gStrUiLoadFormset);    
}

/**
  Refresh UiInitializeLibStrings HII String.

**/
VOID
RefreshSetupUiLibString (
  VOID
  )
{
  UiFreeLibStrings ();
  UiInitializeLibStrings ();

  return;
}

/**
  Set Buffer to Value for Size bytes.

  @param  Buffer                 Memory to set.
  @param  Size                   Number of bytes to set
  @param  Value                  Value of the set operation.

**/
VOID
UiSetUnicodeMem (
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
UiPrintInternal (
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
    Out->OutputString (Out, &mSpaceBuf[SPACE_BUFFER_SIZE - Width + PrintWidth]);
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
UiPrintAt (
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
  LengthOfPrinted = UiPrintInternal (Width, Column, Row, gST->ConOut, Fmt, Args);
  VA_END (Args);
  return LengthOfPrinted;
}



/**
  Prints a formatted unicode string to the default console.

  @param  Fmt        Format string
  @param  ...        Variable argument list for format string.

  @return Length of string printed to the console.

**/
UINTN
EFIAPI
UiConsolePrint (
  IN CHAR16   *Fmt,
  ...
  )
{
  VA_LIST Args;
  UINTN   LengthOfPrinted;

  VA_START (Args, Fmt);
  LengthOfPrinted = UiPrintInternal (0, (UINTN) -1, (UINTN) -1, gST->ConOut, Fmt, Args);
  VA_END (Args);
  return LengthOfPrinted;
}

/**
  Prints a chracter to the default console,
  using L"%c" format.

  @param  Character  Character to print.

  @return Length of string printed to the console.

**/
UINTN
UiPrintChar (
  CHAR16       Character
  )
{
  return UiConsolePrint (L"%c", Character);
}

/**
  Prints a unicode string to the default console,
  using L"%s" format.

  @param  String     String pointer.

  @return Length of string printed to the console

**/
UINTN
UiPrintString (
  IN CHAR16       *String
  )
{
  return UiConsolePrint (L"%s", String);
}

/**
  Prints a unicode string to the default console, at
  the supplied cursor position, using L"%s" format.

  @param  Column     The cursor position to print the string at.
  @param  Row        The cursor position to print the string at
  @param  String     String pointer.

  @return Length of string printed to the console

**/
UINTN
UiPrintStringAt (
  IN UINTN     Column,
  IN UINTN     Row,
  IN CHAR16    *String
  )
{
  return UiPrintAt (0, Column, Row, L"%s", String);
}

/**
  Prints a chracter to the default console, at
  the supplied cursor position, using L"%c" format.

  @param  Column     The cursor position to print the string at.
  @param  Row        The cursor position to print the string at.
  @param  Character  Character to print.

  @return Length of string printed to the console.

**/
UINTN
UiPrintCharAt (
  IN UINTN     Column,
  IN UINTN     Row,
  CHAR16       Character
  )
{
  return UiPrintAt (0, Column, Row, L"%c", Character);
}

/**
  Prints a unicode string to the default console, at
  the supplied cursor position, using L"%s" format.

  @param  Column     The cursor position to print the string at. When it is -1, use current Position.
  @param  Row        The cursor position to print the string at. When it is -1, use current Position.
  @param  String     String pointer.
  @param  Width      Width for String.

  @return Length of string printed to the console

**/
UINTN
EFIAPI
UiPrintStringAtWithWidth (
  IN UINTN     Column,
  IN UINTN     Row,
  IN CHAR16    *String,
  IN UINTN     Width
  )
{
  CHAR16    *StringTmp;
  UINTN     StringWidth;

  StringWidth = UiGetStringWidth(String)/2 -1; 
  if (StringWidth > Width) {
    StringTmp = AllocateCopyPool ((Width + 1)*sizeof(CHAR16), String);
    StringTmp[Width] = CHAR_NULL;
    UiPrintAt (Width, Column, Row, L"%s", StringTmp);
    FreePool (StringTmp);
    return Width;
  } else {
    return UiPrintAt (0, Column, Row, L"%s", String);
  }
}

/**
  Clear retangle with specified text attribute.

  @param  LeftColumn     Left column of retangle.
  @param  RightColumn    Right column of retangle.
  @param  TopRow         Start row of retangle.
  @param  BottomRow      End row of retangle.
  @param  TextAttribute  The character foreground and background.

**/
VOID
EFIAPI
UiClearLines (
  IN UINTN               LeftColumn,
  IN UINTN               RightColumn,
  IN UINTN               TopRow,
  IN UINTN               BottomRow,
  IN UINTN               TextAttribute
  )
{
  CHAR16  *Buffer;
  UINTN   Row;

  //
  // For now, allocate an arbitrarily long buffer
  //
  Buffer = AllocateZeroPool (0x10000);
  ASSERT (Buffer != NULL);

  //
  // Set foreground and background as defined
  //
  gST->ConOut->SetAttribute (gST->ConOut, TextAttribute);

  //
  // Much faster to buffer the long string instead of print it a character at a time
  //
  UiSetUnicodeMem (Buffer, RightColumn - LeftColumn, L' ');

  //
  // Clear the desired area with the appropriate foreground/background
  //
  for (Row = TopRow; Row <= BottomRow; Row++) {
    UiPrintStringAt (LeftColumn, Row, Buffer);
  }

  gST->ConOut->SetCursorPosition (gST->ConOut, LeftColumn, TopRow);

  FreePool (Buffer);
}


VOID *
UiGetGOP (
  EFI_BOOT_SERVICES *pBS
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *ptGO;
  VOID    *Interface;
  EFI_STATUS    Status;  
  EFI_HANDLE    *HandleBuffer;
  UINTN    HandleCount;
  UINTN    Index;  

  HandleBuffer = NULL;
  ptGO = NULL;
  
  Status = pBS->LocateHandleBuffer (
              ByProtocol,
              &gEfiGraphicsOutputProtocolGuid,
              NULL,
              &HandleCount,
              &HandleBuffer
              );
  if(EFI_ERROR(Status) || HandleCount==0){
    goto ProcExit;
  }  
              
  for(Index=0;Index<HandleCount;Index++){
    Status = pBS->HandleProtocol(
               HandleBuffer[Index], 
               &gEfiConsoleOutDeviceGuid, 
               &Interface
               );
    if(EFI_ERROR(Status)){
      continue;
    }
    Status = pBS->HandleProtocol(
               HandleBuffer[Index], 
               &gEfiGraphicsOutputProtocolGuid, 
               (VOID **)&ptGO
               );
    ASSERT(!EFI_ERROR(Status) && ptGO!=NULL);               
  }

ProcExit:
  pBS->FreePool(HandleBuffer);
  return (VOID*)ptGO;
}



EFI_STATUS 
UiSaveStoreScreenRange (
  IN VOID        *BootServices, 
  IN BOOLEAN     IsSave,
  IN UINTN       Start,
  IN UINTN       End,
  IN UINTN       Top,
  IN UINTN       Bottom
  )
{
  EFI_STATUS                      Status = EFI_SUCCESS;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *ptGO;
  UINTN                           Width;
  UINTN                           Height;
  EFI_BOOT_SERVICES               *pBS;      
  UINTN                           Size;
  STATIC DIALOG_RANGE             MyRange;
  UINTN                           Columns;
  UINTN                           Rows;


//  DEBUG((EFI_D_INFO, "%a() %d (%d,%d,%d,%d)\n", __FUNCTION__, \
//                       IsSave, Start, End, Top, Bottom));

  pBS = (EFI_BOOT_SERVICES*)BootServices;
  ASSERT(pBS->Hdr.Signature == EFI_BOOT_SERVICES_SIGNATURE);
  
  ptGO = (EFI_GRAPHICS_OUTPUT_PROTOCOL*)UiGetGOP(pBS);
  if(ptGO == NULL){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }    

  if(IsSave){

    if(gBltBuffer != NULL){
      pBS->FreePool(gBltBuffer);
    }  

    gST->ConOut->QueryMode (
                     gST->ConOut,
                     gST->ConOut->Mode->Mode,
                     &Columns,
                     &Rows
                     );
    if(Top > 0){
      Top--;
    }
    if(Bottom+1 < Rows){
      Bottom++;
    }

    Width = (End - Start + 1) * EFI_GLYPH_WIDTH;
    Height = (Bottom - Top + 1) * EFI_GLYPH_HEIGHT;
    Size = Width * Height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    
    gBltBuffer = AllocatePool(Size);
    if(EFI_ERROR(Status)){
      Status = EFI_OUT_OF_RESOURCES;
      goto ProcExit;		
    }	

    Status = ptGO->Blt(
                    ptGO,
                    gBltBuffer,
                    EfiBltVideoToBltBuffer,
                    Start * EFI_GLYPH_WIDTH,
                    Top * EFI_GLYPH_HEIGHT,
                    0,
                    0,
                    Width,
                    Height,
                    Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)  
                    );  
    MyRange.Left = Start;
    MyRange.Right = End;
    MyRange.Top = Top;
    MyRange.Bottom = Bottom;
    
  } else {                                // restore
    
    if(gBltBuffer == NULL){
      Status = EFI_INVALID_PARAMETER;
      goto ProcExit;	      
    }

    Start  = MyRange.Left;
    End    = MyRange.Right;
    Top    = MyRange.Top;
    Bottom = MyRange.Bottom;

    Width = (End - Start + 1) * EFI_GLYPH_WIDTH;
    Height = (Bottom - Top + 1) * EFI_GLYPH_HEIGHT;
    
    Status = ptGO->Blt(
                    ptGO,
                    gBltBuffer,
                    EfiBltBufferToVideo,
                    0,
                    0,
                    Start * EFI_GLYPH_WIDTH,
                    Top * EFI_GLYPH_HEIGHT,
                    Width,
                    Height,
                    Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                    );
	
    FreePool(gBltBuffer);
    gBltBuffer = NULL;    
  }
	
ProcExit:
  return Status;	
}




