
#include <PlatformSetupDxe.h>


EFI_STATUS
EFIAPI
AdvancedFormCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION                   Action,
  IN EFI_QUESTION_ID                      KeyValue,
  IN UINT8                                Type,
  IN EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
  )
{
  SETUP_DATA        CurrentSetupData;

   
  DEBUG((EFI_D_ERROR,"\nDevicesFormCallback(), Action :%d.\n", Action));
    
  if ((Action == EFI_BROWSER_ACTION_FORM_OPEN) || \
      (Action == EFI_BROWSER_ACTION_FORM_CLOSE)) {
    return EFI_SUCCESS;
  }

  if (Action != EFI_BROWSER_ACTION_CHANGING && Action != EFI_BROWSER_ACTION_CHANGED) {
    return EFI_UNSUPPORTED;
  }

  DEBUG((EFI_D_INFO, "AdvancedFormCallback(), A:%X K:%X\n", Action, KeyValue));
  
  HiiGetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *) &CurrentSetupData);

	switch (KeyValue){

  case KEY_VALUE_NBSPE:
   if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 != 0)) {
	 CurrentSetupData.D0F0SPEValue=CurrentSetupData.D0F1SPEValue=CurrentSetupData.D0F2SPEValue=CurrentSetupData.D0F3SPEValue=CurrentSetupData.D0F4SPEValue\
	 =CurrentSetupData.D0F5SPEValue=CurrentSetupData.D0F6SPEValue=CurrentSetupData.D3F0SPEValue=CurrentSetupData.D3F1SPEValue\
	 =CurrentSetupData.D3F2SPEValue=CurrentSetupData.D3F3SPEValue=CurrentSetupData.D4F0SPEValue=CurrentSetupData.D4F1SPEValue=CurrentSetupData.D5F0SPEValue\
	 =CurrentSetupData.D5F1SPEValue=CurrentSetupData.RCRBHSPEValue=CurrentSetupData.PCIEEPHYSPEValue=CurrentSetupData.D1F0SPEValue\
	 =CurrentSetupData.D8F0SPEValue=CurrentSetupData.D9F0SPEValue=CurrentSetupData.NBSPEValue-1;
   }
   
   break;

 case KEY_VALUE_SBSPE:
   if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 != 0)) {
	   CurrentSetupData.VARTSPEValue=CurrentSetupData.ESPISPEValue=CurrentSetupData.BusCtrlSPEValue\
	   	=CurrentSetupData.PMUSPEValue=CurrentSetupData.PCCASPEValue=CurrentSetupData.HDACSPEValue=CurrentSetupData.SPISPEValue=CurrentSetupData.SBSPEValue-1;
   }
   break;
   
 case KEY_VALUE_IOVEN:
	 if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 != 0)) {
	   //Only DMA cycle on snoop path can be remapped by TACTL, so GFX must be configurated to using snoop path only.
	   //Only DMA cycle on snoop path can be remapped by TACTL, so the HDAC should be configurated to use snoop path only when TACTL is enabled.
	   //CurrentSetupData.GFX_snoop_path_only=0;
	   CurrentSetupData.GoNonSnoopPath=0;
	 }
	 else
	 {
	   //CurrentSetupData.GFX_snoop_path_only=1;
	   CurrentSetupData.GoNonSnoopPath=1;
	 }
	 break;
	 
case KEY_VALUE_IOVQIEN:
	 if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 != 0))
	 {
		CurrentSetupData.IOVINTREnable = 1;
	 }
	 else
	 {
		//if QI capability is disabled, INTR capability should also be disabled.
		CurrentSetupData.IOVINTREnable = 0;
	 }
	 break;
case KEY_VALUE_CRB_MODE_SEL:
	 if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
	 {
//CRB Platform Mode choose notebook mode
		CurrentSetupData.CpuCState = 5;
		CurrentSetupData.CxEEnable = 0;
		CurrentSetupData.XhcU1U2Ctrl = 1;
		CurrentSetupData.AHCIALPMEn = 1;
		CurrentSetupData.IDEHIPMEn = 1;
		CurrentSetupData.PcieASPM = 0;
		CurrentSetupData.PcieASPMBootArch = 0;
		#ifdef IOE_EXIST
		CurrentSetupData.IOESataHIPMEn = 1;
		CurrentSetupData.IOESataALPMEn = 1;
        #endif
	 }
	 else
	 {
	
//CRB Platform Mode choose  desktop mode

		CurrentSetupData.CxEEnable = 0;
		CurrentSetupData.CpuCState = 0;
		CurrentSetupData.XhcU1U2Ctrl = 0;
		CurrentSetupData.AHCIALPMEn = 0;
		CurrentSetupData.IDEHIPMEn = 0;
		CurrentSetupData.PcieASPM = 0;
		CurrentSetupData.PcieASPMBootArch = 0;
		#ifdef IOE_EXIST
		CurrentSetupData.IOESataHIPMEn = 0;
		CurrentSetupData.IOESataALPMEn = 0;
        #endif
	 }

     break;
  default:
		 break;
	 }
	
	HiiSetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *)&CurrentSetupData, NULL);

	return EFI_SUCCESS;
}

