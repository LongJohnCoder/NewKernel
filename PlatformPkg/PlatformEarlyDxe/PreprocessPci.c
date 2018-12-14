
#include <Uefi.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Protocol/PlatHostInfoProtocol.h>
#include <AsiaSbProtocol.h>
#include <AsiaNbProtocol.h>


EFI_STATUS
PlatPreprocessPciCallBack (
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL                 *Io,
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS     PciAddress,
  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE    Phase    
  )
{
  EFI_STATUS            Status;
  EFI_ASIA_NB_PROTOCOL  *AsiaNb;
  EFI_ASIA_SB_PROTOCOL  *AsiaSb;

  DEBUG((EFI_D_INFO, "PrePci (%X,%X,%X)\n", PciAddress.Bus, PciAddress.Device, PciAddress.Function));
  
  Status = gBS->LocateProtocol(&gAsiaNbProtocolGuid, NULL, &AsiaNb);
  ASSERT_EFI_ERROR(Status);
  Status = gBS->LocateProtocol(&gAsiaSbProtocolGuid, NULL, &AsiaSb);
  ASSERT_EFI_ERROR(Status);

  Status = AsiaSb->PreprocessPciController(AsiaSb, Io, PciAddress, Phase);
  ASSERT_EFI_ERROR(Status);
  Status = AsiaNb->PreprocessPciController(AsiaNb, Io, PciAddress, Phase);
  ASSERT_EFI_ERROR(Status); 

  return Status;  
}



