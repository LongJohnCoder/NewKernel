/** @file

Copyright (c) 2006 - 2016, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

**/


#ifndef _UNICODE_FONT_H_
#define _UNICODE_FONT_H_

#include <Uefi.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/DevicePath.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>

#include <Guid/MdeModuleHii.h>

#include <Protocol/HiiFont.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/ByoFormSetManager.h>


extern EFI_WIDE_GLYPH                   gUsStdWideGlyphData[];
extern UINT32 mWideFontSize;


#endif
