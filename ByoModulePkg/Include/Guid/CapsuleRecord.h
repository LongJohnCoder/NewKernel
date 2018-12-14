/** @file
  Defines Hob extend GUID to record capusle info loaded from media for
  crisis recovery

  These are contracts between the recovery module and device recovery module
  that convey the name of a given recovery module type.

**/

#ifndef __CAPSULE_RECORD__H
#define __CAPSULE_RECORD__H

#include <Library\BiosIdLib.h>

#define MAX_CAPUSLE_NUMBER 8

typedef struct {
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  UINT64                  Length;
  BIOS_ID_IMAGE           BiosIdImage;
  EFI_GUID                DeviceId;
  BOOLEAN                 Actived;
} CAPSULE_INFO;

typedef struct {
  CAPSULE_INFO        CapsuleInfo[MAX_CAPUSLE_NUMBER];
  UINT8               CapsuleCount;
} CAPSULE_RECORD;

extern EFI_GUID gRecoveryCapsuleRecordGuid;

#endif

