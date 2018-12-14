/*++
Copyright (c) 2010 Intel Corporation. All rights reserved.
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  VfrExtension.h

Abstract:

  OEM Specific Setup Variables and Structures
--*/

#ifndef __SETUP_PASSWORD___H__
#define __SETUP_PASSWORD___H__


#define TSESETUP_GUID \
  { \
    0xc811fa38, 0x42c8, 0x4579, { 0xa9, 0xbb, 0x60, 0xe9, 0x4e, 0xdd, 0xfb, 0x34 } \
  }

#define SETUP_PASSWORD_VARIABLE_NAME   L"SysPs"

#define SYS_PASSWORD_VAR_ATTRIBUTE (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE)

#define PASSWORD_MAX_SIZE            20
#define USER_LOGIN_TYPE_ADMIN        0
#define USER_LOGIN_TYPE_POP          1

typedef struct {
  UINT8  HasPowerOnPasswd;
  UINT8  HasAdminPasswd;
  UINT8  PowerOnPasswdHash[32];
  UINT8  AdminPasswdHash[32];
} TSESETUP;

extern EFI_GUID gSetupPasswordVariableTseGuid;

#endif