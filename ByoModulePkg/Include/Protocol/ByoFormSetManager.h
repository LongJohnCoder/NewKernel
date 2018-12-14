/** @file
  ByoFormSetManager.h
  
  Interface of Byo Formset Manager Protocol.
**/

#ifndef __BYO_FORMSET_MANAGER_H__
#define __BYO_FORMSET_MANAGER_H__

//
// Device Manager Setup Protocol GUID
//
#define EFI_BYO_FORMSET_MANAGER_PROTOCOL_GUID \
  { 0x65e4992f, 0xd77c, 0x494d, { 0x9a, 0xd1, 0x68, 0x77, 0x5b, 0xb9, 0x1a, 0xa1 } }
extern EFI_GUID gEfiByoFormsetManagerProtocolGuid;

#define EFI_BYO_SETUP_ENTER_GUID \
  { 0x71202EEE, 0x5F53, 0x40d9, { 0xAB, 0x3D, 0x9E, 0x0C, 0x26, 0xD9, 0x66, 0x57 } }
extern EFI_GUID gEfiByoSetupEnterGuid;

#define EFI_BYO_IFR_GUID \
  { 0xec6d3091, 0xe0a2, 0x4419, { 0xad, 0xb3, 0x41, 0x5b, 0x8, 0xc7, 0xfc, 0x46 } }
extern EFI_GUID gEfiByoIfrGuid;	


typedef struct _EFI_BYO_FORMSET_MANAGER_PROTOCOL EFI_BYO_FORMSET_MANAGER_PROTOCOL;

typedef struct {
  UINTN                           Signature;
  LIST_ENTRY                   Link;
  EFI_GUID                       Guid;
  EFI_HII_HANDLE             HiiHandle;
  EFI_STRING_ID              FormSetTitle;
  EFI_FORM_ID                 FirstFormId;
} BYO_BROWSER_FORMSET;

#define BYO_FORM_BROWSER_FORMSET_SIGNATURE  SIGNATURE_32 ('B', 'F', 'B', 'L')
#define BYO_FORM_BROWSER_FORMSET_FROM_LINK(a)  CR (a, BYO_BROWSER_FORMSET, Link, BYO_FORM_BROWSER_FORMSET_SIGNATURE)

typedef EFI_STATUS
(*INSERT_BYO_FORMSET) (
  IN EFI_BYO_FORMSET_MANAGER_PROTOCOL    *This,
  IN EFI_GUID    *FormsetGuid
);

typedef EFI_STATUS
(*REMOVE_BYO_FORMSET) (
  IN EFI_BYO_FORMSET_MANAGER_PROTOCOL    *This,
  IN EFI_GUID    *FormsetGuid
);

typedef EFI_STATUS
( *RUN_BYO_FORMSET) (
  IN EFI_BYO_FORMSET_MANAGER_PROTOCOL    *This,
  IN EFI_GUID    *FormsetGuid
);

typedef BOOLEAN
( *SETUP_CHECK_FORMSET) (
  IN EFI_GUID    *FormsetGuid
);

typedef EFI_STATUS
( *SETUP_CHECK_PASSWORD) (
  IN CHAR16 *Title,
  OUT CHAR16 *Password
);

typedef struct _EFI_BYO_FORMSET_MANAGER_PROTOCOL {
  LIST_ENTRY    ByoFormSetList;
  INSERT_BYO_FORMSET    Insert;
  REMOVE_BYO_FORMSET    Remove;
  RUN_BYO_FORMSET    Run;
  SETUP_CHECK_FORMSET    CheckFormset;  
} EFI_BYO_FORMSET_MANAGER_PROTOCOL;

//
// Setup Debug Macro.
//
/*
#ifdef SETUP_DEBUG
  #undef SETUP_DEBUG
#endif 
#define SETUP_DEBUG     1
*/

#if  defined(SETUP_DEBUG) && SETUP_DEBUG == 1
  #define DMSG(arg)    DEBUG(arg)  
  #define DMSG_HII(String,Handle,Tag)    if (String) DEBUG((EFI_D_ERROR, "%a, %s :%s. \n", __FUNCTION__, Tag, GetToken(String, Handle)))

#elif defined(SETUP_DEBUG) && SETUP_DEBUG == 2 
  #define DMSG(arg)    DEBUG(arg);\
                                   DEBUG((EFI_D_ERROR, " __Setup, %a, Line: %d\n", __FUNCTION__, __LINE__))                                     
  #define DMSG_HII(String,Handle,Tag)    if (String) DEBUG((EFI_D_ERROR, "%a, %s :%s. \n", __FUNCTION__, Tag, GetToken(String, Handle)));\
                                                                  DEBUG((EFI_D_ERROR, " __Setup, %a, Line: %d\n", __FUNCTION__, __LINE__))  
#else
  #define DMSG(arg)
  #define DMSG_HII(String,Handle,Tag) 
#endif

#endif
