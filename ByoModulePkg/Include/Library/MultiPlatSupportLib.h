/** @file
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2017 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _MULTI_PLATFORM_SUPPORT_LIB_H_
#define _MULTI_PLATFORM_SUPPORT_LIB_H_

EFI_STATUS
EFIAPI
CreateDefaultVariableHob (
  IN  UINT16  DefaultId,
  IN  UINT8   BoardId,
  OUT VOID    **FirstVarData     OPTIONAL
  );
/*++
Description:

  This function finds the matched default data and create GUID hob for it. 

Arguments:

  DefaultId - Specifies the type of defaults to retrieve.
  BoardId   - Specifies the platform board of defaults to retrieve.
  
Returns:

  EFI_SUCCESS - The matched default data is found.
  EFI_NOT_FOUND - The matched default data is not found.
  EFI_OUT_OF_RESOURCES - No enough resource to create HOB.

--*/

#endif
