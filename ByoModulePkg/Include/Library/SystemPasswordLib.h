/** @file

Copyright (c) 2006 - 2016, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SystemPasswordLib.h

Abstract:
  System password library.

Revision History:

**/
#ifndef _SYSTEM_PASSWORD_LIB_H_
#define _SYSTEM_PASSWORD_LIB_H_

#include <SystemPasswordVariable.h>

#define UNIQUE_SYSTEM_NUMBER_LENGTH    8


EFI_STATUS
EFIAPI
Sha256Hash (
  IN  CONST VOID        *Data,
  IN        UINTN       DataLen,
  OUT       UINT8       *Digest,
  IN        UINTN       DigestSize
  );

UINTN
Unicode2Ascii (
  IN  CHAR16    *String16,
  OUT  UINT8    **String8
  );


UINTN
GetUniqueNumber (
  UINT8    **Number
  );


UINTN
EncodePassword (
  IN  UINT8    *Password,
  IN  UINTN    Length,
  OUT  UINT8    **Hash,
  IN  UINTN    Type    //Type :Ascii or Scancode
  );

BOOLEAN
CompareInputPassword (
  IN  UINT8    *PasswordHash,
  IN  CHAR16    *InputPassword
  );

/**
  Check Power-On Passwrod.

**/
EFI_STATUS
CheckPopPassword (VOID );

/**
  Check Setup Passwrod.

**/
EFI_STATUS
CheckSetupPassword (VOID );

/**
  Check System Passwrod.

**/
EFI_STATUS
CheckSysPd (
  BOOLEAN   BootSetup
  );

BOOLEAN 
IsValidPdKey (
  EFI_INPUT_KEY *Key
  ); 

#endif

