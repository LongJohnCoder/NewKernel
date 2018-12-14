# @file
#
# Copyright (c) 2006 - 2018, Byosoft Corporation.<BR>
# All rights reserved.This software and associated documentation (if any)
# is furnished under a license and may only be used or copied in
# accordance with the terms of the license. Except as permitted by such
# license, no part of this software or documentation may be reproduced,
# stored in a retrieval system, or transmitted in any form or by any
# means without the express written consent of Byosoft Corporation.
#
# File Name:
#   PlatformCapsule.dsc
#
# Abstract:
#   Platform Configuration File
#
# Revision History:
#
##


[Defines]
  PLATFORM_NAME                  = PlatformCapsule
  PLATFORM_GUID                  = 7896F6C9-936E-4a52-B017-6F4E4E113AD7
  PLATFORM_VERSION               = 0.1
  FLASH_DEFINITION               = ByoModulePkg/Capsule/PlatformCapsule.fdf
  OUTPUT_DIRECTORY               = Build/PlatformPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

