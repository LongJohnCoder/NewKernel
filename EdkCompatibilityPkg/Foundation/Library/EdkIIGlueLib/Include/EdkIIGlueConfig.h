/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGlueConfig.h
  
Abstract: 

  Configurable items provided by GlueLib

--*/

#ifndef __EDKII_GLUE_CONFIG_H__
#define __EDKII_GLUE_CONFIG_H__

#include "EdkIIGlueProcessorBind.h"

//
//  Glue Library version
//
//  0x3000  - the 3rd release
//  0x5000  - support IPF. Jan, 2007
//  0x6000  - support EBC. Feb, 2007
//  0x7000  - size reduction, Jun, 2007
//  0x7100  - backward compatibility supported, Jun, 2007
//
//  For reference only, don't change the value
//
#define EDKII_GLUE_LIBRARY_VERSION 0x7100


//
// Check to make sure EFI_SPECIFICATION_VERSION and TIANO_RELEASE_VERSION are defined.
//
#if !defined(EFI_SPECIFICATION_VERSION)
  #error EFI_SPECIFICATION_VERSION not defined
#elif !defined(TIANO_RELEASE_VERSION)
  #error TIANO_RELEASE_VERSION not defined
#elif (TIANO_RELEASE_VERSION == 0)
  #error TIANO_RELEASE_VERSION can not be zero
#endif


//
//  Glue Library debug flag
//
//  Controls debug ON/OFF of GlueLib itself, no
//  effect on any other libraries or modules
//
//  Values:
//    FALSE           : debug off
//    any TRUE value  : debug on
//
#define EDKII_GLUE_LIBRARY_DEBUG_ENABLE  0


//
// max unicode string length
//
#define EDKII_GLUE_MaximumUnicodeStringLength   1000000

//
// max ascii string length
//
#define EDKII_GLUE_MaximumAsciiStringLength     1000000

//
// spin lock timeout
//
#define EDKII_GLUE_SpinLockTimeout              10000000

//
// max linked list length
//
#define EDKII_GLUE_MaximumLinkedListLength      1000000

//
// debug print level
//
#ifndef EDKII_GLUE_DebugPrintErrorLevel
#define EDKII_GLUE_DebugPrintErrorLevel         EFI_D_ERROR
#endif

//
// debug propery mask
//
#ifndef EDKII_GLUE_DebugPropertyMask
#define EDKII_GLUE_DebugPropertyMask           (  DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED                \
                                                  | DEBUG_PROPERTY_DEBUG_PRINT_ENABLED               \
                                                  | DEBUG_PROPERTY_DEBUG_CODE_ENABLED                \
                                                  | DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED              \
                                                  | DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED           \
                                               )
#endif

//
// clear memory value
//
#define EDKII_GLUE_DebugClearMemoryValue        0xAF

//
// pci express base address
//
#ifndef EDKII_GLUE_PciExpressBaseAddress
#define EDKII_GLUE_PciExpressBaseAddress        0xE0000000
#endif

//
//
// This value is FSB Clock frequency. Its unit is Hz and its 
// default value is 200000000, that means FSB frequency is 200Mhz.
//
#ifndef EDKII_GLUE_FSBClock
#define EDKII_GLUE_FSBClock                     200000000
#endif

//
// This Value is MSR offset for platform information in hex
//
#ifndef EDKII_GLUE_PlatformInfoMsr
#define EDKII_GLUE_PlatformInfoMsr              0xCE
#endif

//
// This Value is MSR offset for APIC Base in hex
//
#ifndef EDKII_GLUE_XAPICBaseMsr
#define EDKII_GLUE_XAPICBaseMsr                 0x1B
#endif

//
// This Value is MSR offset for EXT XAPIC Logical APIC ID in hex
//
#ifndef EDKII_GLUE_ExtXAPICLogicalAPICIdMsr
#define EDKII_GLUE_ExtXAPICLogicalAPICIdMsr     0x802
#endif

//
// post code property mask
//
#define EDKII_GLUE_PostCodePropertyMask         (  POST_CODE_PROPERTY_POST_CODE_ENABLED               \
                                                     | POST_CODE_PROPERTY_POST_CODE_DESCRIPTION_ENABLED \
                                                )

//
// status code property mask
//
#define EDKII_GLUE_ReportStatusCodePropertyMask (  REPORT_STATUS_CODE_PROPERTY_PROGRESS_CODE_ENABLED  \
                                                     | REPORT_STATUS_CODE_PROPERTY_ERROR_CODE_ENABLED   \
                                                     | REPORT_STATUS_CODE_PROPERTY_DEBUG_CODE_ENABLED   \
                                                  )

//
// for IPF only
// The base address of IPF IO Block
//
#ifdef MDE_CPU_IPF
#ifndef EDKII_GLUE_IoBlockBaseAddressForIpf
#define EDKII_GLUE_IoBlockBaseAddressForIpf     0x0ffffc000000
#endif
#endif

#endif
