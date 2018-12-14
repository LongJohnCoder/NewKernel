/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  BiosIdLib.h

Abstract:
  BIOS ID library definitions.

Revision History:

**/



#ifndef _BIOS_ID_LIB_H_
#define _BIOS_ID_LIB_H_
//
// BIOS ID string format:
//
// $(BOARD_ID)$(BOARD_REV).$(OEM_ID).$(BUILD_TYPE)$(VERSION_MAJOR).YYMMDDHHMM
//
// Example: "TNAPCRB1.86C.D0008.1106141018"
//

#pragma pack(1)

typedef struct {
  CHAR16  BoardId[7];               // "TNAPCRB"
  CHAR16  BoardRev;                 // "1"
  CHAR16  Dot1;                     // "."
  CHAR16  OemId[3];                 // "86C"
  CHAR16  Dot2;                     // "."
  CHAR16  BuildType;                // "D"
  CHAR16  VersionMajor[4];          // "0008"
  CHAR16  Dot4;                     // "."
  CHAR16  TimeStamp[10];            // "YYMMDDHHMM"
  CHAR16  NullTerminator;           // 0x0000
} BIOS_ID_STRING;


#define BIOS_ID_SIGN_DATA  {'$', 'I', 'B', 'I', 'O', 'S', 'I', '$'}

//
// A signature precedes the BIOS ID string in the FV to enable search by external tools.
//
typedef struct {
  UINT8           Signature[8];     // "$IBIOSI$"
  BIOS_ID_STRING  BiosIdString;     // "TNAPCRB1.86C.D0008.1106141018"
} BIOS_ID_IMAGE;

#pragma pack()

EFI_STATUS
GetBiosId (
  OUT BIOS_ID_IMAGE     *BiosIdImage
  );
/*++
Description:

  This function returns BIOS ID by searching HOB or FV.

Arguments:

  BiosIdImage - The BIOS ID got from HOB or FV
  
Returns:

  EFI_SUCCESS - All parameters were valid and BIOS ID has been got.

  EFI_NOT_FOUND - BiosId image is not found, and no parameter will be modified.

  EFI_INVALID_PARAMETER - The parameter is NULL
     
--*/


EFI_STATUS
GetBiosBuildTimeHHMM (
  OUT UINT8     *HH,
  OUT UINT8     *MM
  );


EFI_STATUS
GetBiosVersionDateTime (
  OUT CHAR16    *BiosVersion, 
  OUT CHAR16    *BiosReleaseDate,
  OUT CHAR16    *BiosReleaseTime OPTIONAL
  );
/*++
Description:

  This function returns the Version & Release date and time by getting and converting
  BIOS ID.

Arguments:

  BiosVersion - The Bios Version out of the conversion

  BiosReleaseDate - The Bios Release Date out of the conversion

  BiosReleaseTime - The Bios Release Time out of the conversion
  
Returns:

  EFI_SUCCESS - All parameters were valid and Version & Release Date have been set.

  EFI_NOT_FOUND - BiosId image is not found, and no parameter will be modified.

  EFI_INVALID_PARAMETER - One of the parameters is NULL
     
--*/

#endif
