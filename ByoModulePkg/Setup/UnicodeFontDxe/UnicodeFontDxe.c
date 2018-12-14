/** @file

Copyright (c) 2006 - 2016, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

**/

#include "UnicodeFont.h"

EFI_HII_HANDLE    mHiiHandle;
VOID    *mHiiRegistration;
EFI_GUID    mFontPackageListGuid = { 0x2ff9adef, 0xf423, 0x497c, { 0xa8, 0x40, 0x35, 0x68, 0xd1, 0x44, 0x89, 0x5d } };

/**
  HII Database Protocol notification event handler.

  Register font package when HII Database Protocol has been installed.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
RegisterFontPackage (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS                           Status;
  EFI_HII_SIMPLE_FONT_PACKAGE_HDR      *SimplifiedFont;
  UINT32                               PackageLength;
  UINT8                                *Package;
  UINT8                                *Location;
  EFI_HII_DATABASE_PROTOCOL            *HiiDatabase;
  //
  // Locate HII Database Protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **) &HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Add 4 bytes to the header for entire length for HiiAddPackages use only.
  //
  //    +--------------------------------+ <-- Package
  //    |                                |
  //    |    PackageLength(4 bytes)      |
  //    |                                |
  //    |--------------------------------| <-- SimplifiedFont
  //    |                                |
  //    |EFI_HII_SIMPLE_FONT_PACKAGE_HDR |
  //    |                                |
  //    |--------------------------------| <-- Location
  //    |                                |
  //    |     gUsStdWideGlyphData        |
  //    |                                |
  //    +--------------------------------+

  PackageLength   = sizeof (EFI_HII_SIMPLE_FONT_PACKAGE_HDR) + mWideFontSize + 4;
  Package = AllocateZeroPool (PackageLength);
  ASSERT (Package != NULL);

  WriteUnaligned32((UINT32 *) Package,PackageLength);
  SimplifiedFont = (EFI_HII_SIMPLE_FONT_PACKAGE_HDR *) (Package + 4);
  SimplifiedFont->Header.Length        = (UINT32) (PackageLength - 4);
  SimplifiedFont->Header.Type          = EFI_HII_PACKAGE_SIMPLE_FONTS;


  SimplifiedFont->NumberOfNarrowGlyphs = 0;
  SimplifiedFont->NumberOfWideGlyphs = (UINT16) (mWideFontSize / sizeof (EFI_WIDE_GLYPH));
  Location = (UINT8 *) (&SimplifiedFont->NumberOfWideGlyphs + 1);
  CopyMem (Location, gUsStdWideGlyphData, mWideFontSize);

	
  //
  // Add this simplified font package to a package list then install it.
  //
  mHiiHandle = HiiAddPackages (
                 &mFontPackageListGuid,
                 NULL,
                 Package,
                 NULL
                 );
  ASSERT (mHiiHandle != NULL);
  FreePool (Package);
}

/**
  The user Entry Point for module GraphicsConsole. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval  EFI_SUCCESS       The entry point is executed successfully.
  @return  other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeUnicodeFont (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  //
  // Register notify function on HII Database Protocol to add font package.
  //
  EfiCreateProtocolNotifyEvent (
    &gEfiHiiDatabaseProtocolGuid,
    TPL_CALLBACK,
    RegisterFontPackage,
    NULL,
    &mHiiRegistration
    );

  return Status;
}


