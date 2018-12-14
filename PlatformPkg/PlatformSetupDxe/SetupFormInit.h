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

#ifndef _SETUP_FORM_INIT_H_
#define _SETUP_FORM_INIT_H_

  
EFI_STRING_ID 
SetHiiString (
  EFI_HII_HANDLE    HiiHandle, 
  EFI_STRING_ID    StrRef, 
  CHAR16    *sFormat, ...
  );

CHAR16 *
GetToken (
  IN  EFI_STRING_ID                Token,
  IN  EFI_HII_HANDLE               HiiHandle
  );
  
/**
  Main Form Init.

**/
EFI_STATUS
MainFormInit (
  IN EFI_HII_HANDLE    Handle
  );

EFI_STATUS
AdvanceFormInit (
  IN EFI_HII_HANDLE    Handle
  );

EFI_STATUS
DeviceFormInit (
  IN EFI_HII_HANDLE    Handle
  );
  
  
#endif