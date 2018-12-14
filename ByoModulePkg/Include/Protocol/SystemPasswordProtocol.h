/** @file

Copyright (c) 2006 - 2016, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SystemPasswordProtocol.h

Abstract:
  Implementation of basic setup password function.

Revision History:

**/

#ifndef __SYSTEM_PASSWORD_PROTOCOL___H__
#define __SYSTEM_PASSWORD_PROTOCOL___H__

#include <SystemPasswordVariable.h>

extern EFI_GUID gEfiSystemPasswordVariableGuid;
extern EFI_GUID gEfiSystemPasswordProtocolGuid;

#define MAX_VERIFY_TIMES    3
#define MAX_PASSWORD_LENGTH    SYSTEM_PASSWORD_LENGTH

typedef enum {
  VERIFY_ACTION_ADD = 0,
  VERIFY_ACTION_SUB,
  VERIFY_ACTION_RESET,
  VERIFY_ACTION_MAX
} VERIFY_TIMES_ACTION;

typedef enum {
  VERIFY_MODE_INTO_POP = 0,
  VERIFY_MODE_INTO_SETUP,
  VERIFY_MODE_SETUP,
  VERIFY_MODE_HDP,
  VERIFY_MODE_MAX
} VERIFY_MODE;

typedef enum {
  PD_ADMIN     = 0,
  PD_POWER_ON,
  PD_MAX
} PASSWORD_TYPE;

typedef struct _EFI_SYSTEM_PASSWORD_PROTOCOL EFI_SYSTEM_PASSWORD_PROTOCOL;

typedef EFI_STATUS
(*READ_SYSTEM_PASSWORD) (
  IN PASSWORD_TYPE    Type,
  OUT UINT8    **Password
);

typedef EFI_STATUS
(*WRITE_SYSTEM_PASSWORD) (
  IN PASSWORD_TYPE    Type,
  OUT UINT16    *Password
);

typedef BOOLEAN
(*BE_HAVE_SYSTEM_PASSWORD) (
  IN PASSWORD_TYPE    Type
);

typedef BOOLEAN
(*VERIFY_SYSTEM_PASSWORD) (
  IN PASSWORD_TYPE    Type,
  OUT UINT16    *Password
);

typedef EFI_STATUS
(*CLEAR_SYSTEM_PASSWORD) (
  IN PASSWORD_TYPE    Type
);

typedef EFI_STATUS
(*INPUT_PASSWORD) (
  IN UINTN    Column,
  IN UINTN    Row,
  IN OUT CHAR16    *String
  );

typedef EFI_STATUS
(*SET_VERIFY_MODE) (
  IN VERIFY_MODE    Mode
);

typedef BOOLEAN
(*GET_CHECKED_STATUS) (
  VOID
);

typedef EFI_STATUS
(*SET_VERIFY_TIMES) (
  IN PASSWORD_TYPE    Type,
  IN VERIFY_TIMES_ACTION    Action
);

typedef UINT8
(*GET_VERIFY_TIMES) (
  IN PASSWORD_TYPE    Type
);

typedef UINT8
(*GET_ENTERED_TYPE) (
  VOID
);

typedef struct _EFI_SYSTEM_PASSWORD_PROTOCOL{
  READ_SYSTEM_PASSWORD    Read;
  WRITE_SYSTEM_PASSWORD    Write;
  BE_HAVE_SYSTEM_PASSWORD    BeHave;
  VERIFY_SYSTEM_PASSWORD    Verify;
  CLEAR_SYSTEM_PASSWORD    Clear;
  INPUT_PASSWORD    Input;
  SET_VERIFY_MODE    SetMode;
  SET_VERIFY_TIMES    SetTimes;
  GET_VERIFY_TIMES    GetTimes;
  GET_CHECKED_STATUS    GetPopChecked;
  GET_ENTERED_TYPE    GetEnteredType;
} EFI_SYSTEM_PASSWORD_PROTOCOL;


#endif  // End, __SYSTEM_PASSWORD_PROTOCOL___H__
