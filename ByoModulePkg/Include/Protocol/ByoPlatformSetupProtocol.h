/** @file
 Copyright (c) 2010 - 2018, Byosoft Corporation.<BR>
 All rights reserved.This software and associated documentation (if any)
 is furnished under a license and may only be used or copied in
 accordance with the terms of the license. Except as permitted by such
 license, no part of this software or documentation may be reproduced,
 stored in a retrieval system, or transmitted in any form or by any
 means without the express written consent of Byosoft Corporation.

 File Name:

 Abstract:
    Interface of Byo Platform Setup Protocol.

 Revision History:

**/

#ifndef __BYO_PLATFORM_SETUP_H__
#define __BYO_PLATFORM_SETUP_H__

#define EFI_BYO_PLATFORM_SETUP_GUID \
  { 0xe37b667e, 0xbd0, 0x45b8, { 0xab, 0xf5, 0x7a, 0x8b, 0xc4, 0x7c, 0x8f, 0x8f } }
extern EFI_GUID gEfiByoPlatformSetupGuid;

//
//Definition of main formset Type.
//
typedef enum {
  FORMSET_MAIN = 0,
  FORMSET_ADVANCE,
  FORMSET_DEVICE,  
  FORMSET_POWER,
  FORMSET_SECURITY, 
  FORMSET_BOOT,
  FORMSET_EXIT,
  FORMSET_MAX
} FORM_CLASS;

//
//Device Path for Formset.
//
#pragma pack(1)
typedef struct {
  VENDOR_DEVICE_PATH    VendorDevicePath;
  UINT32    Reserved;
  UINT64    UniqueId;
} HII_VENDOR_DEVICE_PATH_NODE;

typedef struct {
  HII_VENDOR_DEVICE_PATH_NODE    VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;
#pragma pack()

//
//Formset Infomation.
//
typedef struct _FORMSET_INFO {
  FORM_CLASS    Class;
  EFI_GUID    FormSetGuid;
  UINT8    *IfrPack;
  UINT8    *StrPack;
  EFI_HII_CONFIG_ACCESS_PROTOCOL    FormSetConfig;
} FORMSET_INFO;


typedef struct _SETUP_FORMSET_INFO {
  UINTN     Signature;
  LIST_ENTRY    Link;

  EFI_HANDLE    DriverHandle;  
  HII_VENDOR_DEVICE_PATH_NODE    *VendorDevicePath;
  EFI_HII_HANDLE    HiiHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL    SetupConfig;
  FORMSET_INFO    FormSetInfo;

} SETUP_FORMSET_INFO;

#define BYO_FORMSET_LIST_SIGNATURE  SIGNATURE_32 ('B', 'F', 'L', 'T')
#define BYO_FORMSET_INFO_FROM_LINK(a)  CR (a, SETUP_FORMSET_INFO, Link, BYO_FORMSET_LIST_SIGNATURE)
#define BYO_FORMSET_INFO_FROM_THIS(a)  CR (a, SETUP_FORMSET_INFO, SetupConfig, BYO_FORMSET_LIST_SIGNATURE)

typedef struct _FORM_CALLBACK_ITEM {
  FORM_CLASS    Class;
  UINT16    Key;
  EFI_HII_ACCESS_FORM_CALLBACK    Callback;
} FORM_CALLBACK_ITEM;

typedef
EFI_STATUS
(EFIAPI *EFI_FORM_INIT)(
  IN EFI_HII_HANDLE    Handle
 );

typedef struct _FORM_INIT_ITEM {
  FORM_CLASS    Class;
  EFI_FORM_INIT    FormInit;
} FORM_INIT_ITEM;

typedef struct _DYNAMIC_ITEM {
  FORM_CLASS    Class;
  EFI_FORM_ID     FormId;
  UINT16   RefreshLabel;
  EFI_STRING_ID    Prompt;
  EFI_STRING_ID    Help;
} DYNAMIC_ITEM;

typedef struct _SETUP_DYNAMIC_ITEM {
  UINTN     Signature;
  LIST_ENTRY    Link;
  DYNAMIC_ITEM    DynamicItem;
  EFI_QUESTION_ID    Key;
} SETUP_DYNAMIC_ITEM;

#define BYO_SETUP_ITEM_SIGNATURE  SIGNATURE_32 ('B', 'S', 'I', 'M')
#define BYO_SETUP_ITEM_FROM_LINK(a)  CR (a, SETUP_DYNAMIC_ITEM, Link, BYO_SETUP_ITEM_SIGNATURE)


typedef struct _EFI_BYO_PLATFORM_SETUP_PROTOCOL EFI_BYO_PLATFORM_SETUP_PROTOCOL;

typedef EFI_STATUS
(*ADD_FORMSET) (
  IN EFI_BYO_PLATFORM_SETUP_PROTOCOL    *This,
  FORMSET_INFO    *FormSetInfo
  );

typedef EFI_STATUS
(*ADD_DYNAMIC_ITEM) (
  IN EFI_BYO_PLATFORM_SETUP_PROTOCOL    *This,
  DYNAMIC_ITEM    *Item
  );

typedef EFI_STATUS
(*INITIALIZE_MAIN_FORMSET) (
  IN EFI_BYO_PLATFORM_SETUP_PROTOCOL    *This
  );

typedef EFI_STATUS
(*RUN_BYO_SETUP) (
  IN EFI_BYO_PLATFORM_SETUP_PROTOCOL    *This
  );

typedef struct _EFI_BYO_PLATFORM_SETUP_PROTOCOL {
  EFI_HANDLE    DriverHandle;
  LIST_ENTRY    MainFormsetList;
  LIST_ENTRY    DynamicItemList;
  ADD_FORMSET    AddFormset;
  ADD_DYNAMIC_ITEM    AddDynamicItem;
  INITIALIZE_MAIN_FORMSET    InitializeMainFormset;  
  RUN_BYO_SETUP    Run;
} _EFI_BYO_PLATFORM_SETUP_PROTOCOL;

#endif
