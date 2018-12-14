/*++

Copyright (c) 2010 - 2015, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:

Abstract:
  Platform configuration setup.

Revision History:


--*/

#ifndef __SETUP_SAVE_NOTIFY_H__
#define __SETUP_SAVE_NOTIFY_H__

typedef struct _SETUP_SAVE_NOTIFY_PROTOCOL  SETUP_SAVE_NOTIFY_PROTOCOL;

typedef enum {
  SetupSaveNotifyTypeSaveValue,
  SetupSaveNotifyTypeDiscardVaule,
  SetupSaveNotifyTypeLoadDefault,
  SetupSaveNotifyTypeSaveUserDefault,
  SetupSaveNotifyTypeLoadUserDefault,
} SETUP_SAVE_NOTIFY_TYPE;


typedef
EFI_STATUS
(EFIAPI *EFI_SETUP_SAVE_SAVE_VALUE)(
 SETUP_SAVE_NOTIFY_PROTOCOL *This
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SETUP_SAVE_DISCARD_VALUE)(
 SETUP_SAVE_NOTIFY_PROTOCOL *This
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SETUP_SAVE_LOAD_DEFAULT)(
 SETUP_SAVE_NOTIFY_PROTOCOL *This
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SETUP_SAVE_USER_DEFAULT)(
 SETUP_SAVE_NOTIFY_PROTOCOL *This
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SETUP_LOAD_USER_DEFAULT)(
 SETUP_SAVE_NOTIFY_PROTOCOL *This
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SETUP_IS_DATA_CHANGED)(
 IN  SETUP_SAVE_NOTIFY_PROTOCOL *This,
 OUT BOOLEAN                    *IsDataChanged
  );


struct _SETUP_SAVE_NOTIFY_PROTOCOL{
  EFI_HANDLE                    DriverHandle;
  EFI_SETUP_SAVE_SAVE_VALUE     SaveValue;
  EFI_SETUP_SAVE_DISCARD_VALUE  DiscardValue;
  EFI_SETUP_SAVE_LOAD_DEFAULT   LoadDefault;
  EFI_SETUP_SAVE_USER_DEFAULT   SaveUserDefault;
  EFI_SETUP_LOAD_USER_DEFAULT   LoadUserDefault;
  EFI_SETUP_IS_DATA_CHANGED     IsSetupDataChanged;
};

extern EFI_GUID gSetupSaveNotifyProtocolGuid;

#endif    // end, __SETUP_SAVE_NOTIFY_H__









