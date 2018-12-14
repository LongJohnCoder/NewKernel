

#include "FrontPage.h"
#include "../../PlatformPkg/Include/SetupVariable.h"
#include <Protocol/BootLogo.h>
#include <Guid/TcmSetupCfgGuid.h>
#include <Protocol/BdsBootManagerProtocol.h>
#include <Guid/IScsiConfigHii.h>
#include <ByoPlatformSetupConfig.h>



extern EFI_GUID gSecureBootConfigFormSetGuid;
extern EFI_GUID gTcgConfigFormSetGuid;
extern EFI_GUID gTcg2ConfigFormSetGuid;
extern VOID     *gMyFontBin;
extern EFI_GUID gEfiNetworkInterfaceIdentifierProtocolGuid_31;

EFI_GUID  gHddPasswordVendorGuid = 
  {0xd5fd1546, 0x22c5, 0x4c2e, { 0x96, 0x9f, 0x27, 0x3c, 0x0, 0x77, 0x10, 0x80}};

EFI_FORM_BROWSER2_PROTOCOL              *gFormBrowser2;
EFI_BYO_FORM_BROWSER_EXTENSION_PROTOCOL *gFormBrowserEx;


#define _FREE_NON_NULL(POINTER) \
  do{ \
    if((POINTER) != NULL) { \
      FreePool((POINTER)); \
      (POINTER) = NULL; \
    } \
  } while(FALSE)


#define FORMSET_ADD_IGNORE      0
#define FORMSET_ADD_DEVICE      1
#define FORMSET_ADD_SECURITY    2

typedef struct {
  EFI_GUID  *FormsetGuid;
  UINTN     Action;
} FORMSET_ADD_ACTION;

STATIC FORMSET_ADD_ACTION gFormsetAddActionList[] = {
  {&gIScsiConfigGuid,        FORMSET_ADD_DEVICE},          // do not move its position
  
  {&gEfiFormsetGuidMain,     FORMSET_ADD_IGNORE},
  {&gEfiFormsetGuidAdvance,  FORMSET_ADD_IGNORE},
  {&gEfiFormsetGuidDevices,  FORMSET_ADD_IGNORE},
  {&gEfiFormsetGuidPower,    FORMSET_ADD_IGNORE},
  {&gEfiFormsetGuidBoot,     FORMSET_ADD_IGNORE},
  {&gEfiFormsetGuidSecurity, FORMSET_ADD_IGNORE},
  {&gEfiFormsetGuidExit,     FORMSET_ADD_IGNORE},
  
  {&gTcgConfigFormSetGuid,        FORMSET_ADD_SECURITY},
  {&gTcg2ConfigFormSetGuid,       FORMSET_ADD_SECURITY},
  {&gTcmSetupConfigGuid,          FORMSET_ADD_SECURITY},
  {&gSecureBootConfigFormSetGuid, FORMSET_ADD_SECURITY},
  {&gHddPasswordVendorGuid,       FORMSET_ADD_SECURITY},

};  

UINTN GetFormsetAddAction(EFI_GUID *FormsetGuid)
{
  UINTN  Index;
  UINTN  Count;

  Count = sizeof(gFormsetAddActionList)/sizeof(gFormsetAddActionList[0]);
  for(Index=0;Index<Count;Index++){
    if(CompareGuid(FormsetGuid, gFormsetAddActionList[Index].FormsetGuid)){
      return gFormsetAddActionList[Index].Action;
    }  
  }

  return FORMSET_ADD_DEVICE;
}




typedef struct {
  EFI_GUID       FormsetGuid;
  EFI_STRING     FormSetTitle;
  EFI_STRING     FormSetHelp;
  EFI_STRING_ID    FormSetTitleId;
  EFI_HII_HANDLE    HiiHandle;
} HII_FORMSET_GOTO_INFO;


STATIC 
EFI_STATUS 
GetAllHiiFormset (
  HII_FORMSET_GOTO_INFO  **Info,
  UINTN                  *InfoCount
  )
{
  EFI_STATUS                   Status;
  EFI_HII_HANDLE               HiiHandle;
  EFI_HII_HANDLE               *HiiHandleBuffer;
  UINTN                        Index; 
  EFI_HII_DATABASE_PROTOCOL    *HiiDB;
  UINTN                        BufferSize;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  UINT32                       Offset;
  UINT32                       Offset2;
  UINT32                       PackageListLength;
  UINT8                        *Package;
  UINT8                        *OpCodeData;
  EFI_GUID                     *FormsetGuid;  
  UINT8                        OpCode;
  HII_FORMSET_GOTO_INFO        *GotoInfo = NULL;
  UINTN                        GotoIndex = 0;
  EFI_STRING_ID                FormSetTitle;
  EFI_STRING_ID                FormSetHelp; 

  
  HiiHandleBuffer = NULL;
  HiiPackageList  = NULL;
  
  Status = gBS->LocateProtocol(&gEfiHiiDatabaseProtocolGuid, NULL, &HiiDB);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  
  
  HiiHandleBuffer = HiiGetHiiHandles(NULL);
  if(HiiHandleBuffer == NULL){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }

  for(Index=0; HiiHandleBuffer[Index]!=NULL; Index++){
    HiiHandle = HiiHandleBuffer[Index];
    
    _FREE_NON_NULL(HiiPackageList);
    BufferSize = 0;
    Status = HiiDB->ExportPackageLists(HiiDB, HiiHandle, &BufferSize, HiiPackageList);
    if(Status != EFI_BUFFER_TOO_SMALL){
      if(Status == EFI_SUCCESS){Status = EFI_ABORTED;}
      goto ProcExit;
    }
    HiiPackageList = AllocatePool(BufferSize);
    ASSERT(HiiPackageList != NULL);
    Status = HiiDB->ExportPackageLists(HiiDB, HiiHandle, &BufferSize, HiiPackageList);
    if(EFI_ERROR(Status)){
      goto ProcExit;
    } 

//- DEBUG((EFI_D_INFO, "Pkg[%d] %g\n", Index, &HiiPackageList->PackageListGuid));

    PackageListLength = ReadUnaligned32(&HiiPackageList->PackageLength);
    Offset  = sizeof(EFI_HII_PACKAGE_LIST_HEADER);  // total 
    Offset2 = 0;                                    // form

    while(Offset < PackageListLength){
      Package = (UINT8*)HiiPackageList + Offset;
      CopyMem(&PackageHeader, Package, sizeof(EFI_HII_PACKAGE_HEADER));
      
      if(PackageHeader.Type != EFI_HII_PACKAGE_FORMS){
        Offset += PackageHeader.Length;
        continue;
      }
      
      Offset2 = sizeof(EFI_HII_PACKAGE_HEADER);
      while(Offset2 < PackageHeader.Length){
        OpCodeData = Package + Offset2;
        OpCode     = ((EFI_IFR_OP_HEADER*)OpCodeData)->OpCode;
        if(OpCode != EFI_IFR_FORM_SET_OP){
          Offset2 += ((EFI_IFR_OP_HEADER*)OpCodeData)->Length;
          continue;
        }

        GotoInfo = ReallocatePool(
                     GotoIndex * sizeof(HII_FORMSET_GOTO_INFO), 
                     (GotoIndex+1) * sizeof(HII_FORMSET_GOTO_INFO),
                     GotoInfo
                     );
        ASSERT(GotoInfo != NULL);

        FormsetGuid = (EFI_GUID*)(OpCodeData + OFFSET_OF(EFI_IFR_FORM_SET, Guid));
        CopyMem(&FormSetTitle, &((EFI_IFR_FORM_SET*)OpCodeData)->FormSetTitle, sizeof(EFI_STRING_ID));
        CopyMem(&FormSetHelp, &((EFI_IFR_FORM_SET*)OpCodeData)->Help, sizeof(EFI_STRING_ID));
        CopyMem(&GotoInfo[GotoIndex].FormsetGuid, FormsetGuid, sizeof(EFI_GUID));          

        GotoInfo[GotoIndex].FormSetTitle = HiiGetString(HiiHandle, FormSetTitle, NULL);
        GotoInfo[GotoIndex].FormSetHelp  = HiiGetString(HiiHandle, FormSetHelp, NULL);            
        GotoInfo[GotoIndex].FormSetTitleId = FormSetTitle;
        GotoInfo[GotoIndex].HiiHandle = HiiHandle;
        GotoIndex++;
            
        Offset2 += ((EFI_IFR_OP_HEADER*)OpCodeData)->Length;  
      }
 
      Offset += PackageHeader.Length;
    }
    _FREE_NON_NULL(HiiPackageList);
  }

  DEBUG((EFI_D_INFO, "InfoCount:%d\n", GotoIndex));

  if(GotoIndex){
    *Info      = GotoInfo;
    *InfoCount = GotoIndex;
  } else {
    *Info      = NULL;
    *InfoCount = 0;
  }

ProcExit:
  _FREE_NON_NULL(HiiHandleBuffer);
  _FREE_NON_NULL(HiiPackageList);
  return Status;  
}




VOID ShowAllHiiFormset(
  HII_FORMSET_GOTO_INFO  *Info,
  UINTN                  InfoCount
)
{
  UINTN  Index;

  for(Index=0;Index<InfoCount;Index++){
    DEBUG((EFI_D_INFO, "[%d] %g %s\n", Index, &Info[Index].FormsetGuid, Info[Index].FormSetTitle));
  }
}


typedef struct {
  EFI_HII_HANDLE         HiiHandle;
  VOID                   *StartOpCodeHandle;
  VOID                   *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL     *StartLabel;
  EFI_IFR_GUID_LABEL     *EndLabel; 
  EFI_GUID               FormsetGuid;
} ADD_LABEL_CTX;


VOID FreeAddLabelCtx(ADD_LABEL_CTX *Ctx)
{
  if(Ctx->StartOpCodeHandle!=NULL){
    HiiFreeOpCodeHandle(Ctx->StartOpCodeHandle);
  }
  if(Ctx->EndOpCodeHandle!=NULL){
    HiiFreeOpCodeHandle(Ctx->EndOpCodeHandle);
  }
}

EFI_STATUS InitAddLabelCtx(ADD_LABEL_CTX *Ctx, EFI_GUID *FormsetGuid, UINT16 StartNo, UINT16 EndNo)
{
  EFI_HII_HANDLE    *DevHii = NULL;
  EFI_STATUS        Status;


  DevHii = HiiGetHiiHandles(FormsetGuid);
  if(DevHii == NULL){
    DEBUG((EFI_D_ERROR, "Formset %g Not Found\n", FormsetGuid));
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }  
  Ctx->HiiHandle = DevHii[0];
  FreePool(DevHii);

  Ctx->StartOpCodeHandle = HiiAllocateOpCodeHandle();
  if (Ctx->StartOpCodeHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;   
  }
  Ctx->EndOpCodeHandle = HiiAllocateOpCodeHandle();
  if (Ctx->EndOpCodeHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit; 
  }
  
  Ctx->StartLabel = (EFI_IFR_GUID_LABEL*)HiiCreateGuidOpCode(Ctx->StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof(EFI_IFR_GUID_LABEL));
  Ctx->StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  Ctx->StartLabel->Number       = StartNo;
  
  Ctx->EndLabel = (EFI_IFR_GUID_LABEL*)HiiCreateGuidOpCode(Ctx->EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof(EFI_IFR_GUID_LABEL));
  Ctx->EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  Ctx->EndLabel->Number       = EndNo;

  CopyMem(&Ctx->FormsetGuid, FormsetGuid, sizeof(EFI_GUID));

  Status = EFI_SUCCESS;

ProcExit:
  return Status;
}


EFI_QUESTION_ID
GenerateUniqueQuestionId (
  EFI_HII_HANDLE    HiiHandle,
  EFI_STRING_ID    FormSetTitle
  )
{
  UINT16   QuestionId;
  UINTN   HandleCount;

  DEBUG ((EFI_D_ERROR, "\n GenerateUniqueQuestionId(), HiiHandle :0x%x, FormSetTitle :0x%x.\n", HiiHandle, FormSetTitle));

  HandleCount = (UINT64)HiiHandle;
  	
  QuestionId =  (UINT16) (HandleCount & 0xFFFF);
  QuestionId +=  (UINT16) (HandleCount>>16);
  QuestionId |=  (UINT16) (FormSetTitle & 0xFFFF);

  DEBUG ((EFI_D_ERROR, "GenerateUniqueQuestionId(), QuestionId :0x%x.\n", QuestionId));
  return (EFI_QUESTION_ID) QuestionId;
}


EFI_STATUS AddDynamicFormset()
{
  EFI_STATUS             Status;
  HII_FORMSET_GOTO_INFO  *Info;
  UINTN                  InfoCount;
  UINTN                  Index;
  EFI_STRING_ID          Title;
  EFI_STRING_ID          Help;
  VOID                   *DummyIf;
  UINTN                  Action;
  ADD_LABEL_CTX          DevCtx;
  ADD_LABEL_CTX          SecuCtx;
  ADD_LABEL_CTX          *CurCtx; 


  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  ZeroMem(&DevCtx, sizeof(DevCtx));
  ZeroMem(&SecuCtx, sizeof(SecuCtx));  

  Status = gBS->LocateProtocol(
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  NULL,
                  (VOID**)&DummyIf
                  );
  if(EFI_ERROR(Status)){
    gFormsetAddActionList[0].Action = FORMSET_ADD_IGNORE;
  }

  Status = GetAllHiiFormset(&Info, &InfoCount);
  if(EFI_ERROR(Status) || InfoCount == 0){
    goto ProcExit;
  } 

  ShowAllHiiFormset(Info, InfoCount);

  Status = InitAddLabelCtx(&SecuCtx, &gEfiFormsetGuidSecurity, 0x1234, 0xFFFF);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  } 
  Status = InitAddLabelCtx(&DevCtx, &gEfiFormsetGuidDevices, 0x1234, 0xFFFF);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  } 

  for(Index=0;Index<InfoCount;Index++){

    Action = GetFormsetAddAction(&Info[Index].FormsetGuid);
    if(Action == FORMSET_ADD_IGNORE){
      continue;        
    } else if(Action == FORMSET_ADD_DEVICE){
      CurCtx = &DevCtx;
    } else if(Action == FORMSET_ADD_SECURITY){
      CurCtx = &SecuCtx;
    } else {
      continue;
    }
    
    Title = HiiSetString(CurCtx->HiiHandle, 0, Info[Index].FormSetTitle, NULL);
    Help  = HiiSetString(CurCtx->HiiHandle, 0, Info[Index].FormSetHelp, NULL);    
    HiiCreateGotoExOpCode (
      CurCtx->StartOpCodeHandle,
      0,
      Title,
      Help,
      0,
      (EFI_QUESTION_ID)(0xFA00 + Index),
      GenerateUniqueQuestionId (Info[Index].HiiHandle, Info[Index].FormSetTitleId),
      &Info[Index].FormsetGuid,
      0
      );
  }

  CurCtx = &DevCtx;
  Status = HiiUpdateForm (
             CurCtx->HiiHandle,
             &CurCtx->FormsetGuid,
             ROOT_FORM_ID,
             CurCtx->StartOpCodeHandle,
             CurCtx->EndOpCodeHandle
             );
  DEBUG((EFI_D_INFO, "HiiUpdateForm:%r\n", Status));

  CurCtx = &SecuCtx;
  Status = HiiUpdateForm (
             CurCtx->HiiHandle,
             &CurCtx->FormsetGuid,
             ROOT_FORM_ID,
             CurCtx->StartOpCodeHandle,
             CurCtx->EndOpCodeHandle
             );
  DEBUG((EFI_D_INFO, "HiiUpdateForm:%r\n", Status));

ProcExit:
  FreeAddLabelCtx(&DevCtx);
  FreeAddLabelCtx(&SecuCtx);
  return Status;
}








EFI_BYO_PLATFORM_SETUP_PROTOCOL    *gByoSetup = NULL;

EFI_STATUS
AddBaseFormset (
  VOID 
  );

EFI_STATUS
EFIAPI
UiAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_BDS_BOOT_MANAGER_PROTOCOL *BdsBootMgr;
  // SETUP_VOLATILE_DATA           SetupVData;
  // UINTN                         VariableSize;

  
  DEBUG((EFI_D_INFO, "[Setup]\n"));

  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);

  Status = gBS->LocateProtocol(&gEfiBootManagerProtocolGuid, NULL, &BdsBootMgr);
  if(EFI_ERROR(Status)){
    return Status;
  }  

  BdsBootMgr->ConnectAll();
  BdsBootMgr->RefreshOptions();

  // VariableSize = sizeof(SetupVData);
  // ZeroMem(&SetupVData, VariableSize);	
  // Status = gRT->SetVariable (
  //                 SETUP_VOLATILE_VARIABLE_NAME,
  //                 &gPlatformSetupVariableGuid,
  //                 EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
  //                 VariableSize,
  //                 &SetupVData
  //                 );
  // DEBUG((EFI_D_INFO, "SetVar(%s):%r\n", SETUP_VOLATILE_VARIABLE_NAME, Status));


  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiSetupEnterGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );  
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_USER_SETUP);

  BdsStartBootMaint();

  Status = gBS->LocateProtocol (
                  &gEfiByoPlatformSetupGuid,
                  NULL,
                  (VOID**)&gByoSetup
                  );
  DEBUG((EFI_D_INFO, "ByoPlatformSetup:%r\n", Status));
  ASSERT(!EFI_ERROR(Status));

  AddBaseFormset();
  AddDynamicFormset();  
  gByoSetup->InitializeMainFormset(gByoSetup);
  gByoSetup->Run(gByoSetup);
 
  ByoFreeBMPackage();

  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  CpuDeadLoop();

  return EFI_SUCCESS;
}




