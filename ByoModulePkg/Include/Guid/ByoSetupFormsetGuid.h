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

#ifndef __BYO_SETUP_FORMSET_GUID_H__
#define __BYO_SETUP_FORMSET_GUID_H__

  #define FORMSET_GUID_MAIN \
    { \
      0x985eee91, 0xbcac, 0x4238, { 0x87, 0x78, 0x57, 0xef, 0xdc, 0x93, 0xf2, 0x4e } \
    }

  #define FORMSET_GUID_ADVANCE \
    { \
      0xe14f04fa, 0x8706, 0x4353, { 0x92, 0xf2, 0x9c, 0x24, 0x24, 0x74, 0x6f, 0x9f } \
    }

  #define FORMSET_GUID_DEVICE \
    { \
      0xadfe34c8, 0x9ae1, 0x4f8f, { 0xbe, 0x13, 0xcf, 0x96, 0xa2, 0xcb, 0x2c, 0x5b } \
    }

  #define FORMSET_GUID_POWER \
  { \
    0x5b5eb989, 0x4702, 0x47c5, { 0xbb, 0xe0, 0x4, 0xb9, 0x99, 0xf6, 0x2, 0x1e } \
  }

  #define FORMSET_GUID_BOOT \
    { \
      0xff5f25d, 0x2804, 0x462a, { 0xa1, 0xd3, 0x32, 0x70, 0x5e, 0x6f, 0xb9, 0x25 }\
    }

  #define FORMSET_GUID_SECURITY \
    { \
      0x981ceaee, 0x931c, 0x4a17, { 0xb9, 0xc8, 0x66, 0xc7, 0xbc, 0xfd, 0x77, 0xe1 } \
    }

  #define FORMSET_GUID_EXIT \
    { \
      0xa43b03dc, 0xc18a, 0x41b1, { 0x91, 0xc8, 0x3f, 0xf9, 0xaa, 0xa2, 0x57, 0x13 } \
    }

#ifndef VFRCOMPILE
  extern EFI_GUID gEfiFormsetGuidMain;
  extern EFI_GUID gEfiFormsetGuidAdvance;
  extern EFI_GUID gEfiFormsetGuidDevices;
  extern EFI_GUID gEfiFormsetGuidBoot;
  extern EFI_GUID gEfiFormsetGuidSecurity;
  extern EFI_GUID gEfiFormsetGuidExit;
  extern EFI_GUID gEfiFormsetGuidPower;
#endif

#endif
