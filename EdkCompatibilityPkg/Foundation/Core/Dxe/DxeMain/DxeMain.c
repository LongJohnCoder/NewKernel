/*++

Copyright (c) 2004 - 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DxeMain.c

Abstract:

  DXE Core Main Entry Point

--*/

#include "Tiano.h"
#include "DxeCore.h"
#include "EfiHobLib.h"
#include "EfiPerf.h"
#include "FwVolBlock.h"


VOID
EFIAPI
DxeMain (
  IN  VOID *HobStart
  );

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg0 (
  VOID
  );

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg1 (
  UINTN Arg1
  );

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg2 (
  UINTN Arg1,
  UINTN Arg2
  );

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg3 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3
  );

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg4 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3,
  UINTN Arg4
  );

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg5 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3,
  UINTN Arg4,
  UINTN Arg5
  );

EFI_STATUS
CoreGetPeiProtocol (
  IN EFI_GUID  *ProtocolGuid,
  IN VOID      **Interface
  );
  
PERF_CODE (                    
  EFI_STATUS
  GetTimerValue (
   OUT UINT64    *TimerValue
    );
)

//
// DXE Core Global Variables for Protocols from PEI
//
EFI_DECOMPRESS_PROTOCOL                   *gEfiDecompress               = NULL;
EFI_TIANO_DECOMPRESS_PROTOCOL             *gEfiTianoDecompress          = NULL;
EFI_CUSTOMIZED_DECOMPRESS_PROTOCOL        *gEfiCustomizedDecompress     = NULL;
EFI_PEI_FLUSH_INSTRUCTION_CACHE_PROTOCOL  *gEfiPeiFlushInstructionCache = NULL;
EFI_PEI_PE_COFF_LOADER_PROTOCOL           *gEfiPeiPeCoffLoader          = NULL;
EFI_PEI_TRANSFER_CONTROL_PROTOCOL         *gEfiPeiTransferControl       = NULL;

//
// DXE Core globals for Architecture Protocols
//
EFI_SECURITY_ARCH_PROTOCOL        *gSecurity      = NULL;
EFI_CPU_ARCH_PROTOCOL             *gCpu           = NULL;
EFI_METRONOME_ARCH_PROTOCOL       *gMetronome     = NULL;
EFI_TIMER_ARCH_PROTOCOL           *gTimer         = NULL;
EFI_BDS_ARCH_PROTOCOL             *gBds           = NULL;
EFI_WATCHDOG_TIMER_ARCH_PROTOCOL  *gWatchdogTimer = NULL;

//
// DXE Core Global used to update core loaded image protocol handle
//
EFI_GUID                           *gDxeCoreFileName;
EFI_LOADED_IMAGE_PROTOCOL          *gDxeCoreLoadedImage;

//
// BugBug: I'n not runtime, but is the PPI?
//
EFI_STATUS_CODE_PROTOCOL     gStatusCodeInstance = {
  NULL
};

EFI_STATUS_CODE_PROTOCOL     *gStatusCode    = &gStatusCodeInstance;

//
// DXE Core Module Variables
//

EFI_BOOT_SERVICES mBootServices = {
  {
    EFI_BOOT_SERVICES_SIGNATURE,                                                          // Signature
    EFI_BOOT_SERVICES_REVISION,                                                           // Revision
    sizeof (EFI_BOOT_SERVICES),                                                           // HeaderSize
    0,                                                                                    // CRC32
    0                                                                                     // Reserved
  },                                                                                      
  (EFI_RAISE_TPL)                               CoreRaiseTpl,                             // RaiseTPL
  (EFI_RESTORE_TPL)                             CoreRestoreTpl,                           // RestoreTPL
  (EFI_ALLOCATE_PAGES)                          CoreAllocatePages,                        // AllocatePages
  (EFI_FREE_PAGES)                              CoreFreePages,                            // FreePages
  (EFI_GET_MEMORY_MAP)                          CoreGetMemoryMap,                         // GetMemoryMap
  (EFI_ALLOCATE_POOL)                           CoreAllocatePool,                         // AllocatePool
  (EFI_FREE_POOL)                               CoreFreePool,                             // FreePool
  (EFI_CREATE_EVENT)                            CoreCreateEvent,                          // CreateEvent
  (EFI_SET_TIMER)                               CoreSetTimer,                             // SetTimer
  (EFI_WAIT_FOR_EVENT)                          CoreWaitForEvent,                         // WaitForEvent
  (EFI_SIGNAL_EVENT)                            CoreSignalEvent,                          // SignalEvent
  (EFI_CLOSE_EVENT)                             CoreCloseEvent,                           // CloseEvent
  (EFI_CHECK_EVENT)                             CoreCheckEvent,                           // CheckEvent
  (EFI_INSTALL_PROTOCOL_INTERFACE)              CoreInstallProtocolInterface,             // InstallProtocolInterface
  (EFI_REINSTALL_PROTOCOL_INTERFACE)            CoreReinstallProtocolInterface,           // ReinstallProtocolInterface
  (EFI_UNINSTALL_PROTOCOL_INTERFACE)            CoreUninstallProtocolInterface,           // UninstallProtocolInterface
  (EFI_HANDLE_PROTOCOL)                         CoreHandleProtocol,                       // HandleProtocol
  (VOID *)                                      NULL,                                     // Reserved
  (EFI_REGISTER_PROTOCOL_NOTIFY)                CoreRegisterProtocolNotify,               // RegisterProtocolNotify
  (EFI_LOCATE_HANDLE)                           CoreLocateHandle,                         // LocateHandle
  (EFI_LOCATE_DEVICE_PATH)                      CoreLocateDevicePath,                     // LocateDevicePath
  (EFI_INSTALL_CONFIGURATION_TABLE)             CoreInstallConfigurationTable,            // InstallConfigurationTable
  (EFI_IMAGE_LOAD)                              CoreLoadImage,                            // LoadImage
  (EFI_IMAGE_START)                             CoreStartImage,                           // StartImage
  (EFI_EXIT)                                    CoreExit,                                 // Exit
  (EFI_IMAGE_UNLOAD)                            CoreUnloadImage,                          // UnloadImage
  (EFI_EXIT_BOOT_SERVICES)                      CoreExitBootServices,                     // ExitBootServices
  (EFI_GET_NEXT_MONOTONIC_COUNT)                CoreEfiNotAvailableYetArg1,               // GetNextMonotonicCount
  (EFI_STALL)                                   CoreStall,                                // Stall
  (EFI_SET_WATCHDOG_TIMER)                      CoreSetWatchdogTimer,                     // SetWatchdogTimer
  (EFI_CONNECT_CONTROLLER)                      CoreConnectController,                    // ConnectController
  (EFI_DISCONNECT_CONTROLLER)                   CoreDisconnectController,                 // DisconnectController
  (EFI_OPEN_PROTOCOL)                           CoreOpenProtocol,                         // OpenProtocol
  (EFI_CLOSE_PROTOCOL)                          CoreCloseProtocol,                        // CloseProtocol
  (EFI_OPEN_PROTOCOL_INFORMATION)               CoreOpenProtocolInformation,              // OpenProtocolInformation
  (EFI_PROTOCOLS_PER_HANDLE)                    CoreProtocolsPerHandle,                   // ProtocolsPerHandle
  (EFI_LOCATE_HANDLE_BUFFER)                    CoreLocateHandleBuffer,                   // LocateHandleBuffer
  (EFI_LOCATE_PROTOCOL)                         CoreLocateProtocol,                       // LocateProtocol
  (EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES)    CoreInstallMultipleProtocolInterfaces,    // InstallMultipleProtocolInterfaces
  (EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES)  CoreUninstallMultipleProtocolInterfaces,  // UninstallMultipleProtocolInterfaces
  (EFI_CALCULATE_CRC32)                         CoreEfiNotAvailableYetArg3,               // CalculateCrc32
  (EFI_COPY_MEM)                                EfiCommonLibCopyMem,                      // CopyMem
  (EFI_SET_MEM)                                 EfiCommonLibSetMem                        // SetMem
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  ,
  //
  // UEFI 2.0 Extension to the table
  //
  (EFI_CREATE_EVENT_EX)                         CoreCreateEventEx
#endif
};

EFI_DXE_SERVICES mDxeServices = {
  {
    EFI_DXE_SERVICES_SIGNATURE,                                           // Signature
    EFI_DXE_SERVICES_REVISION,                                            // Revision
    sizeof (EFI_DXE_SERVICES),                                            // HeaderSize
    0,                                                                    // CRC32
    0                                                                     // Reserved
  },
  (EFI_ADD_MEMORY_SPACE)             CoreAddMemorySpace,                  // AddMemorySpace
  (EFI_ALLOCATE_MEMORY_SPACE)        CoreAllocateMemorySpace,             // AllocateMemorySpace
  (EFI_FREE_MEMORY_SPACE)            CoreFreeMemorySpace,                 // FreeMemorySpace
  (EFI_REMOVE_MEMORY_SPACE)          CoreRemoveMemorySpace,               // RemoveMemorySpace
  (EFI_GET_MEMORY_SPACE_DESCRIPTOR)  CoreGetMemorySpaceDescriptor,        // GetMemorySpaceDescriptor
  (EFI_SET_MEMORY_SPACE_ATTRIBUTES)  CoreSetMemorySpaceAttributes,        // SetMemorySpaceAttributes
  (EFI_GET_MEMORY_SPACE_MAP)         CoreGetMemorySpaceMap,               // GetMemorySpaceMap
  (EFI_ADD_IO_SPACE)                 CoreAddIoSpace,                      // AddIoSpace
  (EFI_ALLOCATE_IO_SPACE)            CoreAllocateIoSpace,                 // AllocateIoSpace
  (EFI_FREE_IO_SPACE)                CoreFreeIoSpace,                     // FreeIoSpace
  (EFI_REMOVE_IO_SPACE)              CoreRemoveIoSpace,                   // RemoveIoSpace
  (EFI_GET_IO_SPACE_DESCRIPTOR)      CoreGetIoSpaceDescriptor,            // GetIoSpaceDescriptor
  (EFI_GET_IO_SPACE_MAP)             CoreGetIoSpaceMap,                   // GetIoSpaceMap
  (EFI_DISPATCH)                     CoreDispatcher,                      // Dispatch
  (EFI_SCHEDULE)                     CoreSchedule,                        // Schedule
  (EFI_TRUST)                        CoreTrust,                           // Trust
  (EFI_PROCESS_FIRMWARE_VOLUME)      CoreProcessFirmwareVolume,           // ProcessFirmwareVolume
};

EFI_SYSTEM_TABLE mEfiSystemTableTemplate = {
  {
    EFI_SYSTEM_TABLE_SIGNATURE,                                           // Signature
    EFI_SYSTEM_TABLE_REVISION,                                            // Revision
    sizeof (EFI_SYSTEM_TABLE),                                            // HeaderSize
    0,                                                                    // CRC32
    0                                                                     // Reserved
  },
  NULL,                                                                   // FirmwareVendor
  0,                                                                      // FirmwareRevision
  NULL,                                                                   // ConsoleInHandle
  NULL,                                                                   // ConIn
  NULL,                                                                   // ConsoleOutHandle
  NULL,                                                                   // ConOut
  NULL,                                                                   // StandardErrorHandle
  NULL,                                                                   // StdErr
  NULL,                                                                   // RuntimeServices
  &mBootServices,                                                         // BootServices
  0,                                                                      // NumberOfConfigurationTableEntries
  NULL                                                                    // ConfigurationTable
};

EFI_RUNTIME_SERVICES mEfiRuntimeServicesTableTemplate = {
  {
    EFI_RUNTIME_SERVICES_SIGNATURE,                                       // Signature
    EFI_RUNTIME_SERVICES_REVISION,                                        // Revision
    sizeof (EFI_RUNTIME_SERVICES),                                        // HeaderSize
    0,                                                                    // CRC32
    0                                                                     // Reserved
  },
  (EFI_GET_TIME)                   CoreEfiNotAvailableYetArg2,                  // GetTime
  (EFI_SET_TIME)                   CoreEfiNotAvailableYetArg1,                  // SetTime
  (EFI_GET_WAKEUP_TIME)            CoreEfiNotAvailableYetArg3,                  // GetWakeupTime
  (EFI_SET_WAKEUP_TIME)            CoreEfiNotAvailableYetArg2,                  // SetWakeupTime
  (EFI_SET_VIRTUAL_ADDRESS_MAP)    CoreEfiNotAvailableYetArg4,                  // SetVirtualAddressMap
  (EFI_CONVERT_POINTER)            CoreEfiNotAvailableYetArg2,                  // ConvertPointer
  (EFI_GET_VARIABLE)               CoreEfiNotAvailableYetArg5,                  // GetVariable
  (EFI_GET_NEXT_VARIABLE_NAME)     CoreEfiNotAvailableYetArg3,                  // GetNextVariableName
  (EFI_SET_VARIABLE)               CoreEfiNotAvailableYetArg5,                  // SetVariable
  (EFI_GET_NEXT_HIGH_MONO_COUNT)   CoreEfiNotAvailableYetArg1,                  // GetNextHighMonotonicCount
  (EFI_RESET_SYSTEM)               CoreEfiNotAvailableYetArg4,                  // ResetSystem
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  (EFI_UPDATE_CAPSULE)             CoreEfiNotAvailableYetArg3,                  // UpdateCapsule
  (EFI_QUERY_CAPSULE_CAPABILITIES) CoreEfiNotAvailableYetArg4,                  // QueryCapsuleCapabilities  
  (EFI_QUERY_VARIABLE_INFO)        CoreEfiNotAvailableYetArg4                   // QueryVariableInfo
#elif (TIANO_RELEASE_VERSION != 0)
  (EFI_REPORT_STATUS_CODE)         CoreEfiNotAvailableYetArg5                   // ReportStatusCode
#endif
};

EFI_RUNTIME_ARCH_PROTOCOL gRuntimeTemplate = {
  INITIALIZE_LIST_HEAD_VARIABLE (gRuntimeTemplate.ImageHead),
  INITIALIZE_LIST_HEAD_VARIABLE (gRuntimeTemplate.EventHead),

  //
  // Make sure Size != sizeof (EFI_MEMORY_DESCRIPTOR). This will
  // prevent people from having pointer math bugs in their code.
  // now you have to use *DescriptorSize to make things work.
  //
  sizeof (EFI_MEMORY_DESCRIPTOR) + sizeof (UINT64) - (sizeof (EFI_MEMORY_DESCRIPTOR) % sizeof (UINT64)),  
  EFI_MEMORY_DESCRIPTOR_VERSION, 
  0,
  NULL,
  NULL,
  FALSE,
  FALSE
};

EFI_RUNTIME_ARCH_PROTOCOL *gRuntime = &gRuntimeTemplate;

//
// DXE Core Global Variables for the EFI System Table, Boot Services Table, 
// DXE Services Table, and Runtime Services Table
//
EFI_BOOT_SERVICES     *gBS = &mBootServices;
EFI_DXE_SERVICES      *gDS = &mDxeServices;
EFI_SYSTEM_TABLE      *gST = NULL;

//
// For debug initialize gRT to template. gRT must be allocated from RT memory
//  but gRT is used for ASSERT () and DEBUG () type macros so lets give it
//  a value that will not cause debug infrastructure to crash early on.
//
EFI_RUNTIME_SERVICES  *gRT = &mEfiRuntimeServicesTableTemplate;
EFI_HANDLE            gDxeCoreImageHandle;

VOID  *mHobStart;

//
// Main entry point to the DXE Core
//
EFI_DXE_ENTRY_POINT (DxeMain)

VOID
EFIAPI
DxeMain (
  IN  VOID *HobStart
  )
/*++

Routine Description:

  Main entry point to DXE Core.

Arguments:

  HobStart - Pointer to the beginning of the HOB List from PEI

Returns:

  This function should never return

--*/
{
  EFI_STATUS                         Status;
  EFI_HANDLE                         DecompressHandle;
  EFI_PHYSICAL_ADDRESS               MemoryBaseAddress;
  UINT64                             MemoryLength;

#ifdef FIRMWARE_PERFORMANCE
  UINT64                             Tick;

  GetTimerValue (&Tick);
#endif


  mHobStart = HobStart;
  
  //
  // Initialize Memory Services
  //
  CoreInitializeMemoryServices (&HobStart, &MemoryBaseAddress, &MemoryLength);

  //
  // Allocate the EFI System Table and EFI Runtime Service Table from EfiRuntimeServicesData
  // Use the templates to initialize the contents of the EFI System Table and EFI Runtime Services Table
  //
  gST = CoreAllocateRuntimeCopyPool (sizeof (EFI_SYSTEM_TABLE), &mEfiSystemTableTemplate);
  ASSERT (gST != NULL);
  if (gST == NULL) {
    //
    // Failed to allocate memory for EFI System Table.  No recovery path implemented as this is not expected to occur.
    //
    EFI_DEADLOOP ();
  }

  gRT = CoreAllocateRuntimeCopyPool (sizeof (EFI_RUNTIME_SERVICES), &mEfiRuntimeServicesTableTemplate);
  ASSERT (gRT != NULL);

  gST->RuntimeServices = gRT;
  
  //
  // Start the Image Services.
  //
  Status = CoreInitializeImageServices (HobStart);
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize the Global Coherency Domain Services
  //
  Status = CoreInitializeGcdServices (&HobStart, MemoryBaseAddress, MemoryLength);
  ASSERT_EFI_ERROR (Status);

  //
  // The HobStart is relocated in gcd service init. Sync mHobStart variable.
  //
  mHobStart = HobStart;

  //
  // Install the DXE Services Table into the EFI System Tables's Configuration Table
  //
  Status = CoreInstallConfigurationTable (&gEfiDxeServicesTableGuid, gDS);
  ASSERT_EFI_ERROR (Status);

  //
  // Install the HOB List into the EFI System Tables's Configuration Table
  //
  Status = CoreInstallConfigurationTable (&gEfiHobListGuid, HobStart);
  ASSERT_EFI_ERROR (Status);

  //
  // Install Memory Type Information Table into the EFI System Tables's Configuration Table
  //
  Status = CoreInstallConfigurationTable (&gEfiMemoryTypeInformationGuid, &gMemoryTypeInformation);
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize the ReportStatusCode with PEI version, if availible
  //
  CoreGetPeiProtocol (&gEfiStatusCodeRuntimeProtocolGuid, (VOID **)&gStatusCode->ReportStatusCode);
#if ((TIANO_RELEASE_VERSION != 0) && (EFI_SPECIFICATION_VERSION < 0x00020000))
  if (gStatusCode->ReportStatusCode != NULL) {
    gRT->ReportStatusCode = gStatusCode->ReportStatusCode;
  }
#endif

  //
  // Report Status Code here for DXE_ENTRY_POINT once it is available
  //
  CoreReportProgressCode ((EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_ENTRY_POINT));

  //
  // Create the aligned system table pointer structure that is used by external
  // debuggers to locate the system table...  Also, install debug image info
  // configuration table.
  //
  CoreInitializeDebugImageInfoTable ();
  CoreNewDebugImageInfoEntry (
    EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL,
    gDxeCoreLoadedImage,
    gDxeCoreImageHandle
    );

  DEBUG_CODE (
    DEBUG ((EFI_D_INFO | EFI_D_LOAD, "HOBLIST address in DXE = 0x%x\n", HobStart));
  )
  //
  // Initialize the Event Services
  //
  Status = CoreInitializeEventServices ();
  ASSERT_EFI_ERROR (Status);

   
  //
  // Get the Protocols that were passed in from PEI to DXE through GUIDed HOBs
  //
  // These Protocols are not architectural. This implementation is sharing code between 
  // PEI and DXE in order to save FLASH space. These Protocols could also be implemented 
  // as part of the DXE Core. However, that would also require the DXE Core to be ported 
  // each time a different CPU is used, a different Decompression algorithm is used, or a 
  // different Image type is used. By placing these Protocols in PEI, the DXE Core remains 
  // generic, and only PEI and the Arch Protocols need to be ported from Platform to Platform, 
  // and from CPU to CPU.
  //

  Status = CoreGetPeiProtocol (&gEfiDecompressProtocolGuid, &gEfiDecompress);
  if (!EFI_ERROR (Status)) {
    
    //
    // Publish the EFI Decompress protocol for use by other DXE components
    //
    
    DecompressHandle = NULL;
    Status = CoreInstallProtocolInterface (
               &DecompressHandle,
               &gEfiDecompressProtocolGuid,
               EFI_NATIVE_INTERFACE,
               gEfiDecompress
               );
    ASSERT_EFI_ERROR (Status);
  }

  Status = CoreGetPeiProtocol (&gEfiTianoDecompressProtocolGuid, &gEfiTianoDecompress);
  if (!EFI_ERROR (Status)) {
    
    //
    // Publish the Tiano Decompress protocol for use by other DXE components
    //
    
    DecompressHandle = NULL;
    Status = CoreInstallProtocolInterface (
               &DecompressHandle,
               &gEfiTianoDecompressProtocolGuid,
               EFI_NATIVE_INTERFACE,
               gEfiTianoDecompress
               );
    ASSERT_EFI_ERROR (Status);
  }
                                  
  Status = CoreGetPeiProtocol (&gEfiCustomizedDecompressProtocolGuid, &gEfiCustomizedDecompress);
  if (!EFI_ERROR (Status)) {
    
    //
    // Publish the Tiano Decompress protocol for use by other DXE components
    //    
    DecompressHandle = NULL;
    Status = CoreInstallProtocolInterface (
               &DecompressHandle,
               &gEfiCustomizedDecompressProtocolGuid,
               EFI_NATIVE_INTERFACE,
               gEfiCustomizedDecompress
               );
    ASSERT_EFI_ERROR (Status);
  }

  CoreGetPeiProtocol (&gEfiPeiFlushInstructionCacheGuid, &gEfiPeiFlushInstructionCache);

  CoreGetPeiProtocol (&gEfiPeiPeCoffLoaderGuid, &gEfiPeiPeCoffLoader);

  CoreGetPeiProtocol (&gEfiPeiTransferControlGuid, &gEfiPeiTransferControl);


  //
  // Register for the GUIDs of the Architectural Protocols, so the rest of the
  // EFI Boot Services and EFI Runtime Services tables can be filled in.
  //
  CoreNotifyOnArchProtocolInstallation ();

  //
  // Produce Firmware Volume Protocols, one for each FV in the HOB list.
  //
  Status = FwVolBlockDriverInit (gDxeCoreImageHandle, gST);
  ASSERT_EFI_ERROR (Status);

  Status = FwVolDriverInit (gDxeCoreImageHandle, gST);
  ASSERT_EFI_ERROR (Status);

  //
  // Produce the Section Extraction Protocol 
  //
  Status = InitializeSectionExtraction (gDxeCoreImageHandle, gST);
  ASSERT_EFI_ERROR (Status);


  //
  // Initialize Performance monitoring if enabled in build
  //
  PERF_ENABLE (0, gST, Tick);

  PERF_START (0,DXE_TOK, NULL, Tick) ;

  //
  // Initialize the DXE Dispatcher
  //
  PERF_START (0,L"CoreInitializeDispatcher", L"DxeMain", 0) ;
  CoreInitializeDispatcher ();
  PERF_END (0,L"CoreInitializeDispatcher", L"DxeMain", 0) ;

  //
  // Invoke the DXE Dispatcher
  //
  PERF_START (0, L"CoreDispatcher", L"DxeMain", 0);
  CoreDispatcher ();
  PERF_END (0, L"CoreDispatcher", L"DxeMain", 0);

  //
  // Display Architectural protocols that were not loaded if this is DEBUG build
  //
  DEBUG_CODE (
    CoreDisplayMissingArchProtocols ();
  )
  
  //
  // Assert if the Architectural Protocols are not present.
  //
  ASSERT_EFI_ERROR (CoreAllEfiServicesAvailable ());

  //
  // Report Status code before transfer control to BDS
  //
  CoreReportProgressCode ((EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_HANDOFF_TO_NEXT));

  //
  // Display any drivers that were not dispatched because dependency expression
  // evaluated to false if this is a debug build
  //
  DEBUG_CODE (
    CoreDisplayDiscoveredNotDispatched ();
  )

  //
  // Transfer control to the BDS Architectural Protocol
  //
  gBds->Entry (gBds);
  
  //
  // BDS should never return
  //
  ASSERT (FALSE);
  EFI_DEADLOOP ();
}


EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg0 (
  VOID
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  None

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The EFI_BREAKPOINT() is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg1 (
  UINTN Arg1
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The EFI_BREAKPOINT() is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg2 (
  UINTN Arg1,
  UINTN Arg2
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined
  
  Arg2        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The EFI_BREAKPOINT() is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg3 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined
  
  Arg2        - Undefined
  
  Arg3        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The EFI_BREAKPOINT() is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg4 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3,
  UINTN Arg4
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined
  
  Arg2        - Undefined
  
  Arg3        - Undefined
  
  Arg4        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The EFI_BREAKPOINT() is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg5 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3,
  UINTN Arg4,
  UINTN Arg5
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined
  
  Arg2        - Undefined
  
  Arg3        - Undefined
  
  Arg4        - Undefined
  
  Arg5        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The EFI_BREAKPOINT() is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}


EFI_STATUS
CoreGetPeiProtocol (
  IN EFI_GUID  *ProtocolGuid,
  IN VOID      **Interface
  )
/*++

Routine Description:

  Searches for a Protocol Interface passed from PEI through a HOB

Arguments:

  ProtocolGuid - The Protocol GUID to search for in the HOB List

  Interface    - A pointer to the interface for the Protocol GUID

Returns:

  EFI_SUCCESS   - The Protocol GUID was found and its interface is returned in Interface

  EFI_NOT_FOUND - The Protocol GUID was not found in the HOB List

--*/
{
  EFI_STATUS  Status;
  VOID        *HobList;
  VOID        *Buffer;          

  //
  // Initialize 'Buffer' to NULL before usage.
  //
  Buffer = NULL;

  HobList = mHobStart;
  Status = GetNextGuidHob (&HobList, ProtocolGuid, &Buffer, NULL);
  if (EFI_ERROR (Status) || (Buffer == NULL)) {
    //
    // Failed to find a HOB containing the desired protocol
    //
    return Status;
  }

  ASSERT (Buffer != NULL);

  *Interface = (VOID *)(*(UINTN *)(Buffer));

  return Status;
}


VOID
CalculateEfiHdrCrc (
  IN  OUT EFI_TABLE_HEADER    *Hdr
  )
/*++

Routine Description:

  Calcualte the 32-bit CRC in a EFI table using the service provided by the
  gRuntime service.

Arguments:

  Hdr  - Pointer to an EFI standard header

Returns:

  None

--*/
{
  UINT32 Crc;

  Hdr->CRC32 = 0;
  
  //
  // If gBS->CalculateCrce32 () == CoreEfiNotAvailableYet () then
  //  Crc will come back as zero if we set it to zero here
  //
  Crc = 0;
  gBS->CalculateCrc32 ((UINT8 *)Hdr, Hdr->HeaderSize, &Crc);
  Hdr->CRC32 = Crc; 
}


EFI_BOOTSERVICE
EFI_STATUS
EFIAPI
CoreExitBootServices (
  IN EFI_HANDLE   ImageHandle,
  IN UINTN        MapKey
  )
/*++

Routine Description:

  Terminates all boot services.

Arguments:

  ImageHandle   - Handle that identifies the exiting image.
  MapKey        - Key to the latest memory map.

Returns:

  EFI_SUCCESS           - Boot services have been terminated.
  EFI_INVALID_PARAMETER - MapKey is incorrect.

--*/
{
  EFI_STATUS                Status, Status2;
  EFI_TCG_PLATFORM_PROTOCOL *TcgPlatformProtocol;

  //
  // Measure invocation of ExitBootServices, 
  // which is defined by TCG_EFI_Platform_1_20_Final Specification
  //
  TcgPlatformProtocol = NULL;
  Status = CoreLocateProtocol (
             &gEfiTcgPlatformProtocolGuid,
             NULL,
             &TcgPlatformProtocol
             );
  if (!EFI_ERROR (Status) && (TcgPlatformProtocol != NULL)) {
    Status = TcgPlatformProtocol->MeasureAction (EFI_EXIT_BOOT_SERVICES_INVOCATION);
    ASSERT_EFI_ERROR (Status);
  }  

  //
  // Terminate memory services if the MapKey matches
  //
  Status = CoreTerminateMemoryMap (MapKey);
  if (EFI_ERROR (Status)) {
    //
    // Measure failure of ExitBootServices
    //
    if (TcgPlatformProtocol != NULL) {
      Status2 = TcgPlatformProtocol->MeasureAction (EFI_EXIT_BOOT_SERVICES_FAILED);
      ASSERT_EFI_ERROR (Status2);
    }

    return Status;
  }

  //
  // Disable Timer
  //
  gTimer->SetTimerPeriod (gTimer, 0);


  //
  // Notify other drivers that we are exiting boot services.
  //
  CoreNotifySignalList (&gEfiEventExitBootServicesGuid);



  //
  // Disable CPU Interrupts
  //
  gCpu->DisableInterrupt (gCpu);

  //
  // Report that ExitBootServices() has been called
  //
  // We are using gEfiDxeServicesTableGuid as the caller ID for Dxe Core
  //
  CoreReportProgressCode ((EFI_SOFTWARE_EFI_BOOT_SERVICE | EFI_SW_BS_PC_EXIT_BOOT_SERVICES));

  //
  // Clear the non-runtime values of the EFI System Table
  //
  gST->BootServices        = NULL;
  gST->ConIn               = NULL;
  gST->ConsoleInHandle     = NULL;
  gST->ConOut              = NULL;
  gST->ConsoleOutHandle    = NULL;
  gST->StdErr              = NULL;
  gST->StandardErrorHandle = NULL;

  //
  // Recompute the 32-bit CRC of the EFI System Table
  //
  CalculateEfiHdrCrc (&gST->Hdr);

  //
  // Update the AtRuntime field in Runtiem AP.
  //
  gRuntime->AtRuntime = TRUE;
  
  //
  // Measure success of ExitBootServices
  //
  if (TcgPlatformProtocol != NULL) {
    Status2 = TcgPlatformProtocol->MeasureAction (EFI_EXIT_BOOT_SERVICES_SUCCEEDED);
    ASSERT_EFI_ERROR (Status2);
  }  
  
  //
  // Zero out the Boot Service Table
  //
  EfiCommonLibSetMem (gBS, sizeof (EFI_BOOT_SERVICES), 0);
  gBS = NULL;

  return Status;
}
