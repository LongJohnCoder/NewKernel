/** @file

Copyright (c) 2006 - 2016, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

**/

#ifndef _PLATFORM_LANGUAGE_H_
#define _PLATFORM_LANGUAGE_H_

#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PerformanceLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Protocol/ByoFormSetManager.h>

#include <Guid/GlobalVariable.h>
#include <Library/PlatformLanguageLib.h>

///
/// The size of a 3 character ISO639 language code.
///
#define ISO_639_2_ENTRY_SIZE   3


#endif // _PLATFORM_LANGUAGE_H_
