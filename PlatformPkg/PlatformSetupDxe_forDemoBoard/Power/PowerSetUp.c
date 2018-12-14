
#include <PlatformSetupDxe.h>

EFI_STATUS
EFIAPI
PowerFormCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION                   Action,
  IN EFI_QUESTION_ID                      KeyValue,
  IN UINT8                                Type,
  IN EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
  )
{
  SETUP_DATA        CurrentSetupData;

    
  if ((Action == EFI_BROWSER_ACTION_FORM_OPEN) || \
      (Action == EFI_BROWSER_ACTION_FORM_CLOSE)) {
    return EFI_SUCCESS;
  }

  if (Action != EFI_BROWSER_ACTION_CHANGING && Action != EFI_BROWSER_ACTION_CHANGED) {
    return EFI_UNSUPPORTED;
  }

  DEBUG((EFI_D_INFO, "PowerFormCallback A:%X K:%X\n", Action, KeyValue));
 
  HiiGetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *) &CurrentSetupData);

	switch (KeyValue){
		case KEY_C4P_CONTROL:
		   //
		   // If C4P Control is enabled, then set SATA C4P, USB C4P and NM C4P to enable.
		   //
		   if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0)) 
		  { // SDIO Ver3.0 Support is disable.
			//CurrentSetupData.SataC4P = 0;
			//CurrentSetupData.UsbC4P  = 0;
			//CurrentSetupData.XhciC4P = 0;
			//CurrentSetupData.NMC4P   = 0;
		   }
		   else
		   {
			 //CurrentSetupData.SataC4P = 1;
			//CurrentSetupData.UsbC4P  = 1;
			//CurrentSetupData.XhciC4P = 1;
			 //CurrentSetupData.NMC4P   = 1;
		   }
		   
		   break;

	default:
		 break;
	 }
	
	HiiSetBrowserData(&gPlatformSetupVariableGuid, L"Setup", sizeof(SETUP_DATA), (UINT8*)&CurrentSetupData, NULL);

	return EFI_SUCCESS;
}

