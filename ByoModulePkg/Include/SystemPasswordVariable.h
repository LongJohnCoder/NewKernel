/** @file

Copyright (c) 2006 - 2018, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SystemPassword.h

Abstract:
  Implementation of basic setup password function.

Revision History:

**/

#ifndef __SYSTEM_PASSWORD_VARIABLE___H__
#define __SYSTEM_PASSWORD_VARIABLE___H__

#define SYSTEM_PASSWORD_LENGTH    20
#define SYSTEM_PASSWORD_HASH_LENGTH    12

#define SYSTEM_PASSWORD_GUID \
  { \
     0x7d925a5d, 0x90c2, 0x4c85, { 0x90, 0x6d, 0x1, 0x3f, 0x56, 0x5e, 0x33, 0xee }\
  }

#pragma pack(1)
typedef struct _SYSTEM_PASSWORD{
  UINT16    Admin[SYSTEM_PASSWORD_LENGTH];
  UINT8     AdminHash[SYSTEM_PASSWORD_HASH_LENGTH];
  UINT8     bHaveAdmin;
  UINT16    PowerOn[SYSTEM_PASSWORD_LENGTH];
  UINT8     PowerOnHash[SYSTEM_PASSWORD_HASH_LENGTH];
  UINT8     bHavePowerOn;
  UINT8     EnteredType;
  UINT8     VerifyTimes;
  UINT8     VerifyTimesAdmin;
  UINT8     VerifyTimesPop;
  UINT8     RequirePopOnRestart;
  UINT8     ChangePopByUser;  
} SYSTEM_PASSWORD;
#pragma pack()

#define SYSTEM_PASSWORD_NAME   L"SysPd"

#define SYSTEM_PASSWORD_VARSTORE    \
     efivarstore SYSTEM_PASSWORD, attribute = 0x7, name = SysPd, guid = SYSTEM_PASSWORD_GUID;

#define SYS_PD                       SYSTEM_PASSWORD
#define LOGIN_USER_ADMIN      0
#define LOGIN_USER_POP        1
#define LOGIN_USER_MAX        2

#define LOGIN_TYPE                grayoutif ideqval SYSTEM_PASSWORD.EnteredType
#define LOGIN_TYPE_ADMIN     grayoutif NOT ideqval SYSTEM_PASSWORD.EnteredType == LOGIN_USER_ADMIN
#define END_LOGIN_TYPE        endif

#endif
