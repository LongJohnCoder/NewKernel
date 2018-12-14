/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  HddPasswordSmm.c

Abstract: 
  Hdd password SMM driver.

Revision History:

Bug 3178 - add Hdd security Frozen support.
TIME: 2011-12-06
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Send ATA security frozen command to HDDs at ReadyToBoot and 
     _WAK().
$END--------------------------------------------------------------------

Bug 1989:   Changed to use dynamic software SMI value instead of hard coding.
TIME:       2011-6-15
$AUTHOR:    Peng Xianbing
$REVIEWERS:
$SCOPE:     Define SwSmi value range build a PolicyData table for csm16 to 
            get SwSMI value.
$TECHNICAL:
$END--------------------------------------------------------------------

Bug2274: improve hdd password feature.
TIME: 2011-06-09
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. give user max 3 times to input password at post.
  2. check device is hdd or not before unlock it when S3 resume.
  3. remove personal label such as wztest.

$END--------------------------------------------------------------------

**/



#ifndef _HDD_PASSWORD_SMM_H
#define _HDD_PASSWORD_SMM_H

#include <PiSmm.h>
#include <IndustryStandard/Atapi.h>

#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/PciIo.h>
#include <Protocol/SmmReadyToLock.h>
#include <Protocol/LegacyBios.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/PrintLib.h>

#include <Guid/HddPasswordSecurityVariable.h>

#include "IdeMode.h"
#include "AhciMode.h"


//
// Time out value for ATA pass through protocol
//
#define ATA_TIMEOUT        EFI_TIMER_PERIOD_SECONDS (3)

extern VOID *mBuffer;


#endif
