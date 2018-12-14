
#include <PiPei.h>
#include <Ppi/SecPlatformInformation.h>
#include <Ppi/TemporaryRamSupport.h>
#include <Ppi/SecPerformance.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PerformanceLib.h>
#include <Library/LocalApicLib.h>
#include <Library/TimerLib.h>

extern UINT32                        *TopOfCar;

/**
  This interface conveys performance information out of the Security (SEC) phase into PEI.

  This service is published by the SEC phase. The SEC phase handoff has an optional
  EFI_PEI_PPI_DESCRIPTOR list as its final argument when control is passed from SEC into the
  PEI Foundation. As such, if the platform supports collecting performance data in SEC,
  this information is encapsulated into the data structure abstracted by this service.
  This information is collected for the boot-strap processor (BSP) on IA-32.

  @param[in]  PeiServices  The pointer to the PEI Services Table.
  @param[in]  This         The pointer to this instance of the PEI_SEC_PERFORMANCE_PPI.
  @param[out] Performance  The pointer to performance data collected in SEC phase.

  @retval EFI_SUCCESS  The data was successfully returned.

**/
EFI_STATUS
EFIAPI
SecGetPerformance (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN       PEI_SEC_PERFORMANCE_PPI   *This,
  OUT      FIRMWARE_SEC_PERFORMANCE  *Performance
  );


/**
  Perform those platform specific operations that are requried 
  to be executed as early as possibile.
  @return TRUE always return true.
**/
EFI_STATUS
EFIAPI
PlatformSecLibConstructor (
  )
{
  //
  // Init Apic Timer for Performance collection.
  // Use EXCEPT_IA32_BOUND as interrupte type.
  //
  PERF_CODE (
    InitializeApicTimer (0, (UINT32) -1, TRUE, 5);
    DisableApicTimerInterrupt ();
  );

  return EFI_SUCCESS;
}




/**
  A developer supplied function to perform platform specific operations.

  It's a developer supplied function to perform any operations appropriate to a
  given platform. It's invoked just before passing control to PEI core by SEC
  core. Platform developer may modify the SecCoreData and PPI list that is
  passed to PEI Core. 

  @param  SecCoreData           The same parameter as passing to PEI core. It
                                could be overridden by this function.

  @return The platform specific PPI list to be passed to PEI core or
          NULL if there is no need of such platform specific PPI list.

**/
EFI_PEI_PPI_DESCRIPTOR *
EFIAPI
SecPlatformMain (
  IN OUT   EFI_SEC_PEI_HAND_OFF        *SecCoreData
  )
{
  return NULL;
}

