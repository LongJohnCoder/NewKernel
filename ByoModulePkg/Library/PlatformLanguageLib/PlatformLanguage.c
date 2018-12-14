/** @file

Copyright (c) 2006 - 2016, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

**/

#include "PlatformLanguage.h"

CHAR8    *gLastLanguage = NULL;

/**
  Get next language from language code list (with separator ';').

  If LangCode is NULL, then ASSERT.
  If Lang is NULL, then ASSERT.

  @param  LangCode    On input: point to first language in the list. On
                                 output: point to next language in the list, or
                                 NULL if no more language in the list.
  @param  Lang           The first language in the list.

**/
VOID
EFIAPI
GetNextLanguage (
  IN OUT CHAR8      **LangCode,
  OUT CHAR8         *Lang
  )
{
  UINTN  Index;
  CHAR8  *StringPtr;

  ASSERT (LangCode != NULL);
  ASSERT (*LangCode != NULL);
  ASSERT (Lang != NULL);

  Index = 0;
  StringPtr = *LangCode;
  while (StringPtr[Index] != 0 && StringPtr[Index] != ';') {
    Index++;
  }

  CopyMem (Lang, StringPtr, Index);
  Lang[Index] = 0;

  if (StringPtr[Index] == ';') {
    Index++;
  }
  *LangCode = StringPtr + Index;
}

/**
  Check if lang is in supported language codes according to language string.

  This code is used to check if lang is in in supported language codes. It can handle
  RFC4646 and ISO639 language tags.
  In ISO639 language tags, take 3-characters as a delimitation to find matched string.
  In RFC4646 language tags, take semicolon as a delimitation to find matched string.

  For example:
    SupportedLang  = "engfraengfra"
    Iso639Language = TRUE
    Lang           = "eng", the return value is "TRUE", or
    Lang           = "chs", the return value is "FALSE".
  Another example:
    SupportedLang  = "en;fr;en-US;fr-FR"
    Iso639Language = FALSE
    Lang           = "en", the return value is "TRUE", or
    Lang           = "zh", the return value is "FALSE".

  @param  SupportedLang               Platform supported language codes.
  @param  Lang                        Configured language.
  @param  Iso639Language              A bool value to signify if the handler is operated on ISO639 or RFC4646.

  @retval TRUE  lang is in supported language codes.
  @retval FALSE lang is not in supported language codes.

**/
BOOLEAN
IsLangInSupportedLangCodes(
  IN  CHAR8            *SupportedLang,
  IN  CHAR8            *Lang,
  IN  BOOLEAN          Iso639Language
  ) 
{
  UINTN    Index;
  UINTN    CompareLength;
  UINTN    LanguageLength;

  if (Iso639Language) {
    CompareLength = ISO_639_2_ENTRY_SIZE;
    for (Index = 0; Index < AsciiStrLen (SupportedLang); Index += CompareLength) {
      if (AsciiStrnCmp (Lang, SupportedLang + Index, CompareLength) == 0) {
        //
        // Successfully find the Lang string in SupportedLang string.
        //
        return TRUE;
      }
    }
    return FALSE;
  } else {
    //
    // Compare RFC4646 language code
    //
    for (LanguageLength = 0; Lang[LanguageLength] != '\0'; LanguageLength++);

    for (; *SupportedLang != '\0'; SupportedLang += CompareLength) {
      //
      // Skip ';' characters in SupportedLang
      //
      for (; *SupportedLang != '\0' && *SupportedLang == ';'; SupportedLang++);
      //
      // Determine the length of the next language code in SupportedLang
      //
      for (CompareLength = 0; SupportedLang[CompareLength] != '\0' && SupportedLang[CompareLength] != ';'; CompareLength++);
      
      if ((CompareLength == LanguageLength) && 
          (AsciiStrnCmp (Lang, SupportedLang, CompareLength) == 0)) {
        //
        // Successfully find the Lang string in SupportedLang string.
        //
        return TRUE;
      }
    }
    return FALSE;
  }
}

/**
  Initialize Lang or PlatformLang variable, if Lang or PlatformLang variable is not found,
  or it has been set to an unsupported value(not one of platform supported language codes),
  set the default language code to it.

  @param  LangName                    Language name, L"Lang" or L"PlatformLang".
  @param  SupportedLang               Platform supported language codes.
  @param  DefaultLang                 Default language code.
  @param  Iso639Language              A bool value to signify if the handler is operated on ISO639 or RFC4646,
                                      TRUE for L"Lang" LangName or FALSE for L"PlatformLang" LangName.

**/
VOID
InitializeLangVariable (
  IN CHAR16     *LangName,
  IN CHAR8      *SupportedLang,
  IN CHAR8      *DefaultLang,     
  IN BOOLEAN    Iso639Language
  )
{
  CHAR8       *Lang;

  //
  // Find current Lang or PlatformLang from EFI Variable.
  //
  GetEfiGlobalVariable2 (LangName, (VOID **) &Lang, NULL);
  //
  // If Lang or PlatformLang variable is not found,
  // or it has been set to an unsupported value(not one of the supported language codes),
  // set the default language code to it.
  //
  if ((Lang == NULL) || !IsLangInSupportedLangCodes (SupportedLang, Lang, Iso639Language)) {
    //
    // The default language code should be one of the supported language codes.
    //
    ASSERT (IsLangInSupportedLangCodes (SupportedLang, DefaultLang, Iso639Language));
    gRT->SetVariable  (
                    LangName,
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    AsciiStrSize (DefaultLang),
                    DefaultLang
                    );
  }

  if (Lang != NULL) {
    FreePool (Lang);
  }
}

/**
  Determine the current language that will be used
  based on language related EFI Variables.

  @param LangCodesSettingRequired - If required to set LangCodes variable

**/
VOID
InitializeLanguage (
  BOOLEAN LangCodesSettingRequired
  )
{
  CHAR8       *LangCodes;
  CHAR8       *PlatformLangCodes;  


  if(PcdGetBool(PcdLangInited)){
    return;
  }

  PcdSetBool(PcdLangInited, TRUE);
  DEBUG((EFI_D_INFO, "%a(%d)\n", __FUNCTION__, LangCodesSettingRequired));

  LangCodes = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultLangCodes);
  PlatformLangCodes = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes);
  if (LangCodesSettingRequired) {
    if (!FeaturePcdGet (PcdUefiVariableDefaultLangDeprecate)) {
      //
      // UEFI 2.0 depricated this variable so we support turning it off
      //
      gRT->SetVariable (
                      L"LangCodes",
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      AsciiStrSize (LangCodes),
                      LangCodes
                      );
    }

    gRT->SetVariable (
                    L"PlatformLangCodes",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    AsciiStrSize (PlatformLangCodes),
                    PlatformLangCodes
                    );
  }

  if (!FeaturePcdGet (PcdUefiVariableDefaultLangDeprecate)) {
    //
    // UEFI 2.0 depricated this variable so we support turning it off
    //
    InitializeLangVariable (L"Lang", LangCodes, (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultLang), TRUE);
  }
  InitializeLangVariable (L"PlatformLang", PlatformLangCodes, (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLang), FALSE);
}

/**
  Get current language and store to gLastLanguage.
  Return TRUE if language is changed .

**/
BOOLEAN
BeLanguageChanged (
  VOID
  )
{
  EFI_STATUS    Status;
  UINTN    DataSize = 0;
  CHAR8    *Language = NULL;
  DMSG ((EFI_D_ERROR, "\n BeLanguageChanged, gLastLanguage :%a.\n", gLastLanguage));

  Status = gRT->GetVariable (EFI_PLATFORM_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, NULL, &DataSize, Language);
  if (!EFI_ERROR(Status)) {
    return FALSE;
  }
  
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Language = AllocatePool(DataSize + 1);
    if (Language == NULL) {
      return FALSE;
    }
    Status = gRT->GetVariable (EFI_PLATFORM_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, NULL, &DataSize, Language);
    if (!EFI_ERROR(Status)) {
      if (NULL != gLastLanguage) {	  
        if (AsciiStriCmp(Language, gLastLanguage)) {	  	        
          gBS->FreePool(gLastLanguage);          
          gLastLanguage = Language;
          DMSG ((EFI_D_ERROR, "\n BeLanguageChanged, Changed.\n"));
          return TRUE;
        }
      } else {
        gLastLanguage = Language;
        return FALSE;
      }
    }
    gBS->FreePool(Language); 
  }
    
  return FALSE;
}

