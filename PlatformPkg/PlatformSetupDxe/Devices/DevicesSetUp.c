
#include <PlatformSetupDxe.h>

EFI_STATUS
EFIAPI
DevicesFormCallback (
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

  DEBUG((EFI_D_INFO, "DevicesFormCallback(), A:%X\n", Action));
 
  HiiGetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *) &CurrentSetupData);

	switch (KeyValue){
		case KEY_VALUE_PCIERST:
		  if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0)) {
			  //CurrentSetupData.PcieRstPEG	= 0;
			  CurrentSetupData.PcieRstPE0	= 0;
			  CurrentSetupData.PcieRstPE1	= 0;
			  CurrentSetupData.PcieRstPE2	= 0;
			  CurrentSetupData.PcieRstPE3	= 0;
			  CurrentSetupData.PcieRstPE4	= 0;
			  CurrentSetupData.PcieRstPE5	= 0;
			  CurrentSetupData.PcieRstPE6	= 0;
			  CurrentSetupData.PcieRstPE7	= 0;
		  }
		  break;

	case KEY_VALUE_PCIERP:
	  if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0)) {
	      CurrentSetupData.PciePE0  = 0;
		  CurrentSetupData.PciePE1  = 0;
		  CurrentSetupData.PciePE2  = 0;
		  CurrentSetupData.PciePE3  = 0;
		  CurrentSetupData.PciePE4= 0;
		  CurrentSetupData.PciePE5= 0;
		  CurrentSetupData.PciePE6= 0;
		  CurrentSetupData.PciePE7= 0;
		  //CurrentSetupData.PciePEG= 0;
		  CurrentSetupData.PcieRst= 0;
	  }
	  break;
/*	  case KEY_VALUE_PCIE_PEG:
		// If PCIE PEG control is disabled, then disable Reset PEG When Link Fail.
		if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
		{
		   CurrentSetupData.PcieRstPEG = 0;
		}
		else
		{
		  CurrentSetupData.PcieRstPEG = 1;
		}
		
		break;
*/
  case KEY_VALUE_PCIE_PE4:
    // If PCIE PEG control is disabled, then disable Reset PEG When Link Fail.
    if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
    {
       CurrentSetupData.PcieRstPE4 = 0;
	   
	   //CJW_20181112: if function 0 not exist, other functions in this Deivce also disabled
		CurrentSetupData.PciePE5 = 0;	   
    }
    else
    {
      CurrentSetupData.PcieRstPE4 = 1;
    }
    
    break;

	case KEY_VALUE_PCIE_PE5:
    // If PCIE PEG control is disabled, then disable Reset PEG When Link Fail.
     if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
    {
       CurrentSetupData.PcieRstPE5 = 0;
    }
    else
    {
      CurrentSetupData.PcieRstPE5 = 1;
    }
    
    break;
	
 
	case KEY_VALUE_PCIE_PE6:
    // If PCIE PEG control is disabled, then disable Reset PEG When Link Fail.
     if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
    {
       CurrentSetupData.PcieRstPE6 = 0;
	   
	   //CJW_20181112: if function 0 not exist, other functions in this Deivce also disabled
		CurrentSetupData.PciePE7 = 0;	   
    }
    else
    {
      CurrentSetupData.PcieRstPE6 = 1;
    }
    
    break;

  
  case KEY_VALUE_PCIE_PE7:
  // If PCIE PEG control is disabled, then disable Reset PEG When Link Fail.
   if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
  {
	 CurrentSetupData.PcieRstPE7 = 0;
  }
  else
  {
	CurrentSetupData.PcieRstPE7 = 1;
  }
  
  break;
  

  case KEY_VALUE_PCIE_PE0:
    // If PCIE PE0 control is disabled, then disable Reset PE0 When Link Fail.
     if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
    {
       CurrentSetupData.PcieRstPE0 = 0;


	  	//CJW_20181112: if function 0 not exist, other functions in this Deivce also disabled
		CurrentSetupData.PciePE1 = 0;
		CurrentSetupData.PciePE2 = 0;
		CurrentSetupData.PciePE3 = 0;
	   	   
    }
    else
    {
      CurrentSetupData.PcieRstPE0 = 1;
    }
    break;
  
  case KEY_VALUE_PCIE_PE1:
    // If PCIE PE1 control is disabled, then disable Reset PE1 When Link Fail.
    if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
    {
       CurrentSetupData.PcieRstPE1 = 0;
    }
    else
    {
      CurrentSetupData.PcieRstPE1 = 1;
    }
    
    break;
  
  case KEY_VALUE_PCIE_PE2:
    // If PCIE PE2 control is disabled, then disable Reset PE2 When Link Fail.
    if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
    {
       CurrentSetupData.PcieRstPE2 = 0;
    }
    else
    {
      CurrentSetupData.PcieRstPE2 = 1;
    }
    
    break;
  
  case KEY_VALUE_PCIE_PE3:
    // If PCIE PE3 control is disabled, then disable Reset PE3 When Link Fail.
    if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
    {
       CurrentSetupData.PcieRstPE3 = 0;
    }
    else
    {
      CurrentSetupData.PcieRstPE3 = 1;
    }
    
    break;




	case KEY_VALUE_IOE_PECTL:
		 if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0)){
			CurrentSetupData.IoePEA0Ctl = 0;
			CurrentSetupData.IoePEA1Ctl = 0;
			CurrentSetupData.IoePEA2Ctl = 0;
			CurrentSetupData.IoePEA3Ctl = 0;
			CurrentSetupData.IoePEA4Ctl = 0;
			CurrentSetupData.IoePEB0Ctl = 0;
			CurrentSetupData.IoePEB1Ctl = 0;
			
		 	CurrentSetupData.IoeDnPortPEXRST = 1;
			CurrentSetupData.IoePEA0PEXRST = 1;
			CurrentSetupData.IoePEA1PEXRST = 1;
			CurrentSetupData.IoePEA2PEXRST = 1;
			CurrentSetupData.IoePEA3PEXRST = 1;
			CurrentSetupData.IoePEA4PEXRST = 1;
			CurrentSetupData.IoePEB0PEXRST = 1;
			CurrentSetupData.IoePEB1PEXRST = 1;

			
		 }else{
			 CurrentSetupData.IoePEA0Ctl = 1;
			 CurrentSetupData.IoePEA1Ctl = 1;
			 CurrentSetupData.IoePEA2Ctl = 1;
			 CurrentSetupData.IoePEA3Ctl = 1;
			 CurrentSetupData.IoePEA4Ctl = 1;
			 CurrentSetupData.IoePEB0Ctl = 1;
			 CurrentSetupData.IoePEB1Ctl = 1;

		 	CurrentSetupData.IoeDnPortPEXRST = 0;
			CurrentSetupData.IoePEA0PEXRST = 0;
			CurrentSetupData.IoePEA1PEXRST = 0;
			CurrentSetupData.IoePEA2PEXRST = 0;
			CurrentSetupData.IoePEA3PEXRST = 0;
			CurrentSetupData.IoePEA4PEXRST = 0;
			CurrentSetupData.IoePEB0PEXRST = 0;
			CurrentSetupData.IoePEB1PEXRST = 0;			 
		 }
		break;

	case KEY_VALUE_IOE_PECTL_PEA0:
		 if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0)){
		 	CurrentSetupData.IoeDnPortPEXRST = 1;
			CurrentSetupData.IoePEA0PEXRST = 1;
		 }else{
			CurrentSetupData.IoePEA0PEXRST = 0;
		 }
		 break;

	case KEY_VALUE_IOE_PECTL_PEA1:
		 if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0)){
		 	CurrentSetupData.IoeDnPortPEXRST = 1;
			CurrentSetupData.IoePEA1PEXRST = 1;
		 }else{
			CurrentSetupData.IoePEA1PEXRST = 0;
		 }
		 break;

	case KEY_VALUE_IOE_PECTL_PEA2:
		 if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0)){
		 	CurrentSetupData.IoeDnPortPEXRST = 1;
			CurrentSetupData.IoePEA2PEXRST = 1;
		 }else{
			CurrentSetupData.IoePEA2PEXRST = 0;
		 }
		 break;

	case KEY_VALUE_IOE_PECTL_PEA3:
		 if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0)){
		 	CurrentSetupData.IoeDnPortPEXRST = 1;
			CurrentSetupData.IoePEA3PEXRST = 1;
		 }else{
			CurrentSetupData.IoePEA3PEXRST = 0;
		 }
		 break;


	case KEY_VALUE_IOE_PECTL_PEA4:
		 if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0)){
		 	CurrentSetupData.IoeDnPortPEXRST = 1;
			CurrentSetupData.IoePEA4PEXRST = 1;
		 }else{
			CurrentSetupData.IoePEA4PEXRST = 0;
		 }
		 break;

	case KEY_VALUE_IOE_PECTL_PEB0:
		 if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0)){
		 	CurrentSetupData.IoeDnPortPEXRST = 1;
			CurrentSetupData.IoePEB0PEXRST = 1;
		 }else{
			CurrentSetupData.IoePEB0PEXRST = 0;
		 }
		 break;

	case KEY_VALUE_IOE_PECTL_PEB1:
		 if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0)){
		 	CurrentSetupData.IoeDnPortPEXRST = 1;
			CurrentSetupData.IoePEB1PEXRST = 1;
		 }else{
			CurrentSetupData.IoePEB1PEXRST = 0;
		 }
		 break;

		 


 
	default:
		 break;
	 }
	
	HiiSetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *)&CurrentSetupData, NULL);

	return EFI_SUCCESS;
}

