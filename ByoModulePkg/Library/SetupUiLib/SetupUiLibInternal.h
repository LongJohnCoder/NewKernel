/** @file
Copyright (c) 2017, Byosoft Software Corporation. All Rights Reserved.

You may not reproduce, distribute, publish, display, perform, modify, adapt,
transmit, broadcast, present, recite, release, license or otherwise exploit
any part of this publication in any form, by any means, without the prior
written permission of Byosoft Software Corporation.

File Name:
  SetupUiLibInternal.h

Abstract:
  Setup Ui Library implementation.

Revision History:

**/

#ifndef __SETUP_UI_LIB_INTERNAL_H__
#define __SETUP_UI_LIB_INTERNAL_H__

#include <PiDxe.h>

#include <Protocol/SimpleTextOut.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/DevicePath.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>
#include <Protocol/UserManager.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/ByoFormSetManager.h>

#include <Guid/MdeModuleHii.h>
#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/HiiFormMapMethodGuid.h>
#include <Guid/ConsoleOutDevice.h>

#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>
#include <Library/SetupUiLib.h>
#include <Library/PlatformLanguageLib.h>

extern EFI_HII_HANDLE    mSetupUiStringHandle;
extern EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *gBltBuffer;
extern DIALOG_RANGE    gConfirmRange;


//
// Character definitions
//
#define UPPER_LOWER_CASE_OFFSET 0x20

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
  );

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
  );

/**
  Initialize the HII String Token to the correct values.

**/
VOID
UiInitializeLibStrings (
  VOID
  );

/**
  Free the HII String.

**/
VOID
UiFreeLibStrings (
  VOID
  );

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
  );

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
  );


/**
  Prints a chracter to the default console,
  using L"%c" format.

  @param  Character  Character to print.

  @return Length of string printed to the console.

**/
UINTN
UiPrintChar (
  CHAR16       Character
  );

/**
  Prints a unicode string to the default console,
  using L"%s" format.

  @param  String     String pointer.

  @return Length of string printed to the console

**/
UINTN
UiPrintString (
  IN CHAR16       *String
  );

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
  );

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
  );

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
  );

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
  );



EFI_STATUS 
UiSaveStoreScreenRange (
  IN VOID        *BootServices, 
  IN BOOLEAN     IsSave,
  IN UINTN       Start,
  IN UINTN       End,
  IN UINTN       Top,
  IN UINTN       Bottom
  );
  
#endif
