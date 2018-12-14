/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Dispatcher.c

Abstract:

  Tiano DXE Dispatcher.

  Step #1 - When a FV protocol is added to the system every driver in the FV
            is added to the mDiscoveredList. The SOR, Before, and After Depex are 
            pre-processed as drivers are added to the mDiscoveredList. If an Apriori 
            file exists in the FV those drivers are addeded to the 
            mScheduledQueue. The mFvHandleList is used to make sure a 
            FV is only processed once.

  Step #2 - Dispatch. Remove driver from the mScheduledQueue and load and
            start it. After mScheduledQueue is drained check the 
            mDiscoveredList to see if any item has a Depex that is ready to 
            be placed on the mScheduledQueue.

  Step #3 - Adding to the mScheduledQueue requires that you process Before 
            and After dependencies. This is done recursively as the call to add
            to the mScheduledQueue checks for Before and recursively adds 
            all Befores. It then addes the item that was passed in and then 
            processess the After dependecies by recursively calling the routine.

  Dispatcher Rules:
  The rules for the dispatcher are in chapter 10 of the DXE CIS. Figure 10-3 
  is the state diagram for the DXE dispatcher

  Depex - Dependency Expresion.
  SOR   - Schedule On Request - Don't schedule if this bit is set.

--*/

#include "Tiano.h"
#include "DxeCore.h"
#include "FwVolBlock.h"
#include EFI_GUID_DEFINITION (Apriori)

//
// The Driver List contains one copy of every driver that has been discovered.
// Items are never removed from the driver list. List of EFI_CORE_DRIVER_ENTRY
//
EFI_LIST_ENTRY  mDiscoveredList = INITIALIZE_LIST_HEAD_VARIABLE (mDiscoveredList);  

//
// Queue of drivers that are ready to dispatch. This queue is a subset of the
// mDiscoveredList.list of EFI_CORE_DRIVER_ENTRY.
//
EFI_LIST_ENTRY  mScheduledQueue = INITIALIZE_LIST_HEAD_VARIABLE (mScheduledQueue);

//
// List of handles who's Fv's have been parsed and added to the mFwDriverList.
//
EFI_LIST_ENTRY  mFvHandleList = INITIALIZE_LIST_HEAD_VARIABLE (mFvHandleList);           // list of KNOWN_HANDLE

//
// Lock for mDiscoveredList, mScheduledQueue, mDispatcherRunning.
//
EFI_LOCK  mDispatcherLock = EFI_INITIALIZE_LOCK_VARIABLE (EFI_TPL_HIGH_LEVEL);


//
// Flag for the DXE Dispacher.  TRUE if dispatcher is execuing.
//
BOOLEAN  mDispatcherRunning = FALSE;

//
// Module globals to manage the FwVol registration notification event
//
EFI_EVENT       mFwVolEvent;
VOID            *mFwVolEventRegistration;

//
// List of file types supported by dispatcher
//
static EFI_FV_FILETYPE mDxeFileTypes[] = { 
  EFI_FV_FILETYPE_DRIVER, 
  EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER, 
  EFI_FV_FILETYPE_DXE_CORE,
  EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE
};

typedef struct {
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH   File;
  EFI_DEVICE_PATH_PROTOCOL            End;
} FV_FILEPATH_DEVICE_PATH;

FV_FILEPATH_DEVICE_PATH mFvDevicePath;
//
// Function Prototypes
//
VOID
CoreInsertOnScheduledQueueWhileProcessingBeforeAndAfter (
  IN  EFI_CORE_DRIVER_ENTRY   *InsertedDriverEntry
  );
 
VOID
EFIAPI
CoreFwVolEventProtocolNotify (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  );

EFI_DEVICE_PATH_PROTOCOL *
CoreFvToDevicePath (
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL    *Fv,
  IN  EFI_HANDLE                      FvHandle,
  IN  EFI_GUID                        *DriverName
  );

STATIC 
EFI_STATUS
CoreAddToDriverList (
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL  *Fv,
  IN  EFI_HANDLE                    FvHandle,
  IN  EFI_GUID                      *DriverName
  );

STATIC
EFI_STATUS 
CoreProcessFvImageFile (
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL    *Fv,
  IN  EFI_HANDLE                      FvHandle,
  IN  EFI_GUID                        *DriverName
  );


VOID
CoreAcquireDispatcherLock (
  VOID
  )
/*++

Routine Description:

  Enter critical section by gaining lock on mDispatcherLock

Arguments:

  None

Returns:

  None

--*/

{
  CoreAcquireLock (&mDispatcherLock);
}

VOID
CoreReleaseDispatcherLock (
  VOID
  )
/*++

Routine Description:

  Exit critical section by releasing lock on mDispatcherLock

Arguments:

  None

Returns:

  None

--*/
{
  CoreReleaseLock (&mDispatcherLock);
}


EFI_STATUS
CoreGetDepexSectionAndPreProccess (
  IN  EFI_CORE_DRIVER_ENTRY   *DriverEntry
  )
/*++

Routine Description:

  Read Depex and pre-process the Depex for Before and After. If Section Extraction
  protocol returns an error via ReadSection defer the reading of the Depex.

Arguments:

  DriverEntry - Driver to work on.
  
Returns:

  EFI_SUCCESS - Depex read and preprossesed 

  EFI_PROTOCOL_ERROR - The section extraction protocol returned an error and 
                        Depex reading needs to be retried.

  Other Error - DEPEX not found.

--*/
{
  EFI_STATUS                    Status;
  EFI_SECTION_TYPE              SectionType;
  UINT32                        AuthenticationStatus;
  EFI_FIRMWARE_VOLUME_PROTOCOL  *Fv;

  
  Fv = DriverEntry->Fv;

  //
  // Grab Depex info, it will never be free'ed.
  //
  SectionType         = EFI_SECTION_DXE_DEPEX;
  Status = Fv->ReadSection (
                DriverEntry->Fv, 
                &DriverEntry->FileName,
                SectionType, 
                0, 
                &DriverEntry->Depex, 
                (UINTN *)&DriverEntry->DepexSize,
                &AuthenticationStatus
                );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_PROTOCOL_ERROR) {
      //
      // The section extraction protocol failed so set protocol error flag
      //
      DriverEntry->DepexProtocolError = TRUE;
    } else {
      //
      // If no Depex assume EFI 1.1 driver model
      //
      DriverEntry->Depex = NULL;
      DriverEntry->Dependent = TRUE;
      DriverEntry->DepexProtocolError = FALSE;
    }
  } else {
    //
    // Set Before, After, and Unrequested state information based on Depex
    // Driver will be put in Dependent or Unrequested state
    //
    CorePreProcessDepex (DriverEntry);
    DriverEntry->DepexProtocolError = FALSE;
  }  

  return Status;
}

EFI_DXESERVICE
EFI_STATUS
EFIAPI
CoreSchedule (
  IN  EFI_HANDLE  FirmwareVolumeHandle,
  IN  EFI_GUID    *DriverName
  )
/*++

Routine Description:

  Check every driver and locate a matching one. If the driver is found, the Unrequested
  state flag is cleared.

Arguments:

  FirmwareVolumeHandle - The handle of the Firmware Volume that contains the firmware 
                         file specified by DriverName.

  DriverName           - The Driver name to put in the Dependent state.

Returns:

  EFI_SUCCESS   - The DriverName was found and it's SOR bit was cleared

  EFI_NOT_FOUND - The DriverName does not exist or it's SOR bit was not set.

--*/
{
  EFI_LIST_ENTRY        *Link;
  EFI_CORE_DRIVER_ENTRY *DriverEntry;

  //
  // Check every driver
  //
  for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
    DriverEntry = CR(Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
    if (DriverEntry->FvHandle == FirmwareVolumeHandle &&
        DriverEntry->Unrequested && 
        EfiCompareGuid (DriverName, &DriverEntry->FileName)) {
      //
      // Move the driver from the Unrequested to the Dependent state
      //
      CoreAcquireDispatcherLock ();
      DriverEntry->Unrequested  = FALSE;
      DriverEntry->Dependent    = TRUE;
      CoreReleaseDispatcherLock ();
    
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;  
}


EFI_DXESERVICE
EFI_STATUS
EFIAPI
CoreTrust (
  IN  EFI_HANDLE  FirmwareVolumeHandle,
  IN  EFI_GUID    *DriverName
  )
/*++

Routine Description:

  Convert a driver from the Untrused back to the Scheduled state

Arguments:

  FirmwareVolumeHandle - The handle of the Firmware Volume that contains the firmware 
                         file specified by DriverName.

  DriverName           - The Driver name to put in the Scheduled state

Returns:

  EFI_SUCCESS   - The file was found in the untrusted state, and it was promoted 
                  to the trusted state.

  EFI_NOT_FOUND - The file was not found in the untrusted state.

--*/
{
  EFI_LIST_ENTRY        *Link;
  EFI_CORE_DRIVER_ENTRY *DriverEntry;

  //
  // Check every driver
  //
  for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
    DriverEntry = CR(Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
    if (DriverEntry->FvHandle == FirmwareVolumeHandle &&
        DriverEntry->Untrusted && 
        EfiCompareGuid (DriverName, &DriverEntry->FileName)) {
      //
      // Transition driver from Untrusted to Scheduled state.
      //
      CoreAcquireDispatcherLock ();
      DriverEntry->Untrusted = FALSE;
      DriverEntry->Scheduled = TRUE;
      InsertTailList (&mScheduledQueue, &DriverEntry->ScheduledLink);
      CoreReleaseDispatcherLock ();
    
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;  
}


EFI_DXESERVICE
EFI_STATUS
EFIAPI
CoreDispatcher (
  VOID
  )
/*++

Routine Description:

  This is the main Dispatcher for DXE and it exits when there are no more 
  drivers to run. Drain the mScheduledQueue and load and start a PE
  image for each driver. Search the mDiscoveredList to see if any driver can 
  be placed on the mScheduledQueue. If no drivers are placed on the
  mScheduledQueue exit the function. On exit it is assumed the Bds()
  will be called, and when the Bds() exits the Dispatcher will be called 
  again.

Arguments:

  NONE

Returns:

  EFI_ALREADY_STARTED - The DXE Dispatcher is already running

  EFI_NOT_FOUND       - No DXE Drivers were dispatched

  EFI_SUCCESS         - One or more DXE Drivers were dispatched

--*/
{
  EFI_STATUS                      Status;
  EFI_STATUS                      ReturnStatus;
  EFI_LIST_ENTRY                  *Link;
  EFI_CORE_DRIVER_ENTRY           *DriverEntry;
  BOOLEAN                         ReadyToRun;
  UINT8                           Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE];
  EFI_STATUS_CODE_DATA            *StatusCodeData;
  EFI_DEVICE_HANDLE_EXTENDED_DATA *DeviceHandleExtData;

  DeviceHandleExtData = (EFI_DEVICE_HANDLE_EXTENDED_DATA *) Buffer;
  StatusCodeData = (EFI_STATUS_CODE_DATA *) Buffer;
  DeviceHandleExtData->DataHeader.HeaderSize = sizeof (EFI_STATUS_CODE_DATA);
  DeviceHandleExtData->DataHeader.Size = 
                      sizeof (EFI_DEVICE_HANDLE_EXTENDED_DATA) -
                      sizeof (EFI_STATUS_CODE_DATA);

  EfiCommonLibCopyMem (
    &DeviceHandleExtData->DataHeader.Type,
    &gEfiStatusCodeSpecificDataGuid, 
    sizeof (EFI_GUID)
    );

  if (mDispatcherRunning) {
    //
    // If the dispatcher is running don't let it be restarted.
    //
    return EFI_ALREADY_STARTED;
  }

  mDispatcherRunning = TRUE;


  ReturnStatus = EFI_NOT_FOUND;
  do {
    //
    // Drain the Scheduled Queue
    //
    while (!IsListEmpty (&mScheduledQueue)) {
      DriverEntry = CR (
                      mScheduledQueue.ForwardLink,
                      EFI_CORE_DRIVER_ENTRY,
                      ScheduledLink,
                      EFI_CORE_DRIVER_ENTRY_SIGNATURE
                      );

      //
      // Load the DXE Driver image into memory. If the Driver was transitioned from
      // Untrused to Scheduled it would have already been loaded so we may need to
      // skip the LoadImage
      //
      if (DriverEntry->ImageHandle == NULL) {
        Status = CoreLoadImage (
                        FALSE,
                        gDxeCoreImageHandle, 
                        DriverEntry->FvFileDevicePath,
                        NULL, 
                        0, 
                        &DriverEntry->ImageHandle
                        );

        //
        // Update the driver state to reflect that it's been loaded
        //
        if (EFI_ERROR (Status)) {
          CoreAcquireDispatcherLock ();

          if (Status == EFI_SECURITY_VIOLATION) {
            //
            // Take driver from Scheduled to Untrused state
            //
            DriverEntry->Untrusted = TRUE;
          } else {
            //
            // The DXE Driver could not be loaded, and do not attempt to load or start it again.
            // Take driver from Scheduled to Initialized. 
            //
            // This case include the Never Trusted state if EFI_ACCESS_DENIED is returned
            //
            DriverEntry->Initialized  = TRUE;
          }

          DriverEntry->Scheduled = FALSE;
          RemoveEntryList (&DriverEntry->ScheduledLink);
          
          CoreReleaseDispatcherLock ();
        
          //
          // If it's an error don't try the StartImage
          //
          continue;
        }
      }

      CoreAcquireDispatcherLock ();

      DriverEntry->Scheduled    = FALSE;
      DriverEntry->Initialized  = TRUE;
      RemoveEntryList (&DriverEntry->ScheduledLink);
      
      CoreReleaseDispatcherLock ();

      //
      // Report Status Code here to notify drivers has 
      // been initialized (INIT_BEGIN)
      //
      EfiCommonLibCopyMem (
        &DeviceHandleExtData->Handle,
        &DriverEntry->ImageHandle,
        sizeof (EFI_HANDLE)
        );

      CoreReportProgressCodeSpecific (EFI_SOFTWARE_DXE_CORE | EFI_SW_PC_INIT_BEGIN, DriverEntry->ImageHandle);

      Status = CoreStartImage (DriverEntry->ImageHandle, NULL, NULL);

      CoreReportProgressCodeSpecific (EFI_SOFTWARE_DXE_CORE | EFI_SW_PC_INIT_END, DriverEntry->ImageHandle);
  
      ReturnStatus = EFI_SUCCESS;
    }

    //
    // Search DriverList for items to place on Scheduled Queue
    //
    ReadyToRun = FALSE;
    for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
      DriverEntry = CR (Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);

      if (DriverEntry->DepexProtocolError){
    //
        // If Section Extraction Protocol did not let the Depex be read before retry the read
    //
        Status = CoreGetDepexSectionAndPreProccess (DriverEntry);
      } 

      if (DriverEntry->Dependent) {
        if (CoreIsSchedulable (DriverEntry)) {
          CoreInsertOnScheduledQueueWhileProcessingBeforeAndAfter (DriverEntry); 
          ReadyToRun = TRUE;
        } 
      }
    }
  } while (ReadyToRun);

  mDispatcherRunning = FALSE;

  return ReturnStatus;
}


VOID
CoreInsertOnScheduledQueueWhileProcessingBeforeAndAfter (
  IN  EFI_CORE_DRIVER_ENTRY   *InsertedDriverEntry
  )
/*++

Routine Description:

  Insert InsertedDriverEntry onto the mScheduledQueue. To do this you 
  must add any driver with a before dependency on InsertedDriverEntry first.
  You do this by recursively calling this routine. After all the Befores are
  processed you can add InsertedDriverEntry to the mScheduledQueue. 
  Then you can add any driver with an After dependency on InsertedDriverEntry 
  by recursively calling this routine.

Arguments:

  InsertedDriverEntry - The driver to insert on the ScheduledLink Queue

Returns:

  NONE 

--*/
{
  EFI_LIST_ENTRY        *Link;
  EFI_CORE_DRIVER_ENTRY *DriverEntry;

  //
  // Process Before Dependency
  //
  for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
    DriverEntry = CR(Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
    if (DriverEntry->Before && DriverEntry->Dependent) {
      if (EfiCompareGuid (&InsertedDriverEntry->FileName, &DriverEntry->BeforeAfterGuid)) {
        //
        // Recursively process BEFORE
        //
        CoreInsertOnScheduledQueueWhileProcessingBeforeAndAfter (DriverEntry);
      }
    }
  }

  //
  // Convert driver from Dependent to Scheduled state
  //
  CoreAcquireDispatcherLock ();

  InsertedDriverEntry->Dependent = FALSE;
  InsertedDriverEntry->Scheduled = TRUE;
  InsertTailList (&mScheduledQueue, &InsertedDriverEntry->ScheduledLink);
  
  CoreReleaseDispatcherLock ();

  //
  // Process After Dependency
  //
  for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
    DriverEntry = CR(Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
    if (DriverEntry->After && DriverEntry->Dependent) {
      if (EfiCompareGuid (&InsertedDriverEntry->FileName, &DriverEntry->BeforeAfterGuid)) {
        //
        // Recursively process AFTER
        //
        CoreInsertOnScheduledQueueWhileProcessingBeforeAndAfter (DriverEntry);
      }
    }
  }
}


BOOLEAN
FvHasBeenProcessed (
  IN  EFI_HANDLE      FvHandle
  )
/*++

Routine Description:

  Return TRUE if the Fv has been processed, FALSE if not.

Arguments:

  FvHandle - The handle of a FV that's being tested

Returns:

  TRUE  - Fv protocol on FvHandle has been processed

  FALSE - Fv protocol on FvHandle has not yet been processed

--*/
{
  EFI_LIST_ENTRY  *Link;
  KNOWN_HANDLE    *KnownHandle;

  for (Link = mFvHandleList.ForwardLink; Link != &mFvHandleList; Link = Link->ForwardLink) {
    KnownHandle = CR(Link, KNOWN_HANDLE, Link, KNOWN_HANDLE_SIGNATURE);
    if (KnownHandle->Handle == FvHandle) {
      return TRUE;
    }
  }
  return FALSE;
}


VOID
FvIsBeingProcesssed (
  IN  EFI_HANDLE    FvHandle
  )
/*++

Routine Description:

  Remember that Fv protocol on FvHandle has had it's drivers placed on the
  mDiscoveredList. This fucntion adds entries on the mFvHandleList. Items are
  never removed/freed from the mFvHandleList.

Arguments:

  FvHandle - The handle of a FV that has been processed

Returns:

  None

--*/
{
  KNOWN_HANDLE  *KnownHandle;

  KnownHandle = CoreAllocateBootServicesPool (sizeof (KNOWN_HANDLE));
  ASSERT (KnownHandle != NULL);
  if (KnownHandle == NULL) {
    //
    // Failed to allocate memory, no recovery path implemented as this is not expected to occur
    //
    EFI_DEADLOOP ();
  }

  KnownHandle->Signature = KNOWN_HANDLE_SIGNATURE;
  KnownHandle->Handle = FvHandle;
  InsertTailList (&mFvHandleList, &KnownHandle->Link);
}




EFI_DEVICE_PATH_PROTOCOL *
CoreFvToDevicePath (
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL    *Fv,
  IN  EFI_HANDLE                      FvHandle,
  IN  EFI_GUID                        *DriverName
  )
/*++

Routine Description:

  Convert FvHandle and DriverName into an EFI device path

Arguments:

  Fv         - Fv protocol, needed to read Depex info out of FLASH.
  
  FvHandle   - Handle for Fv, needed in the EFI_CORE_DRIVER_ENTRY so that the
               PE image can be read out of the FV at a later time.

  DriverName - Name of driver to add to mDiscoveredList.

Returns:

  Pointer to device path constructed from FvHandle and DriverName

--*/
{
  EFI_STATUS                          Status;
  EFI_DEVICE_PATH_PROTOCOL            *FvDevicePath;
  EFI_DEVICE_PATH_PROTOCOL            *FileNameDevicePath;

  //
  // Remember the device path of the FV
  //
  Status = CoreHandleProtocol (FvHandle, &gEfiDevicePathProtocolGuid, &FvDevicePath);
  if (EFI_ERROR (Status)) {
    FileNameDevicePath = NULL;
  } else {
    //
    // Build a device path to the file in the FV to pass into gBS->LoadImage
    //
    CoreInitializeFwVolDevicepathNode (&mFvDevicePath.File, DriverName);
    mFvDevicePath.End.Type = EFI_END_ENTIRE_DEVICE_PATH;
    mFvDevicePath.End.SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
    SetDevicePathNodeLength (&mFvDevicePath.End, sizeof (EFI_DEVICE_PATH_PROTOCOL));
    FileNameDevicePath = CoreAppendDevicePath (
                            FvDevicePath, 
                            (EFI_DEVICE_PATH_PROTOCOL *)&mFvDevicePath
                            );
  }

  return FileNameDevicePath;
}



EFI_STATUS
CoreAddToDriverList (
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL    *Fv,
  IN  EFI_HANDLE                      FvHandle,
  IN  EFI_GUID                        *DriverName
  )
/*++

Routine Description:

  Add an entry to the mDiscoveredList. Allocate memory to store the DriverEntry, 
  and initilize any state variables. Read the Depex from the FV and store it
  in DriverEntry. Pre-process the Depex to set the SOR, Before and After state.
  The Discovered list is never free'ed and contains booleans that represent the
  other possible DXE driver states.

Arguments:

  Fv         - Fv protocol, needed to read Depex info out of FLASH.
  
  FvHandle   - Handle for Fv, needed in the EFI_CORE_DRIVER_ENTRY so that the
               PE image can be read out of the FV at a later time.

  DriverName - Name of driver to add to mDiscoveredList.

Returns:

  EFI_SUCCESS - If driver was added to the mDiscoveredList.

  EFI_ALREADY_STARTED - The driver has already been started. Only one DriverName
                        may be active in the system at any one time.

--*/
{
  EFI_CORE_DRIVER_ENTRY               *DriverEntry;

 
  //
  // Create the Driver Entry for the list. ZeroPool initializes lots of variables to 
  // NULL or FALSE.
  //
  DriverEntry = CoreAllocateZeroBootServicesPool (sizeof (EFI_CORE_DRIVER_ENTRY));
  ASSERT (DriverEntry != NULL);
  if (DriverEntry == NULL) {
    //
    // Failed to allocate memory, no recovery path implemented as this is not expected to occur
    //
    EFI_DEADLOOP ();
  }

  DriverEntry->Signature        = EFI_CORE_DRIVER_ENTRY_SIGNATURE;
  EfiCommonLibCopyMem (&DriverEntry->FileName, DriverName, sizeof (EFI_GUID));
  DriverEntry->FvHandle         = FvHandle;
  DriverEntry->Fv               = Fv;
  DriverEntry->FvFileDevicePath = CoreFvToDevicePath (Fv, FvHandle, DriverName);

  CoreGetDepexSectionAndPreProccess (DriverEntry);
  
  CoreAcquireDispatcherLock ();
  
  InsertTailList (&mDiscoveredList, &DriverEntry->Link);

  CoreReleaseDispatcherLock ();

  return EFI_SUCCESS;
}



EFI_STATUS 
CoreProcessFvImageFile (
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL    *Fv,
  IN  EFI_HANDLE                      FvHandle,
  IN  EFI_GUID                        *DriverName
  )
/*++

Routine Description:

  Get the driver from the FV through driver name, and produce a FVB protocol on FvHandle.

Arguments:

  Fv          - The FIRMWARE_VOLUME protocol installed on the FV.
  FvHandle    - The handle which FVB protocol installed on.
  DriverName  - The driver guid specified.

Returns:

  EFI_OUT_OF_RESOURCES    - No enough memory or other resource.
  
  EFI_VOLUME_CORRUPTED    - Corrupted volume.
  
  EFI_SUCCESS             - Function successfully returned.
  

--*/
{
  EFI_STATUS                          Status;
  EFI_SECTION_TYPE                    SectionType;
  UINT32                              AuthenticationStatus;
  VOID                                *Buffer;
  UINTN                               BufferSize;

  //
  // Read the first (and only the first) firmware volume section
  //
  SectionType = EFI_SECTION_FIRMWARE_VOLUME_IMAGE;
  Buffer      = NULL;
  BufferSize  = 0;
  Status = Fv->ReadSection (
                Fv, 
                DriverName, 
                SectionType, 
                0, 
                &Buffer, 
                &BufferSize,
                &AuthenticationStatus
                );
  if (!EFI_ERROR (Status)) {
    //
    // Produce a FVB protocol for the file
    //
    Status = ProduceFVBProtocolOnBuffer (
              (EFI_PHYSICAL_ADDRESS) (UINTN) Buffer,
              (UINT64)BufferSize,
              FvHandle,
              NULL
              );
  }

  if (EFI_ERROR (Status) && (Buffer != NULL)) {    
  //
  // ReadSection or Produce FVB failed, Free data buffer
  //
  gBS->FreePool (Buffer); 

  }

  return Status;
}


VOID
EFIAPI
CoreFwVolEventProtocolNotify (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
/*++

Routine Description:

  Event notification that is fired every time a FV dispatch protocol is added. 
  More than one protocol may have been added when this event is fired, so you
  must loop on CoreLocateHandle () to see how many protocols were added and
  do the following to each FV:

  If the Fv has already been processed, skip it. If the Fv has not been 
  processed then mark it as being processed, as we are about to process it.

  Read the Fv and add any driver in the Fv to the mDiscoveredList.The 
  mDiscoveredList is never free'ed and contains variables that define
  the other states the DXE driver transitions to.. 
  
  While you are at it read the A Priori file into memory.
  Place drivers in the A Priori list onto the mScheduledQueue.

Arguments:

  Event   - The Event that is being processed, not used.
  
  Context - Event Context, not used.

Returns:

  None

--*/
{
  EFI_STATUS                    Status;
  EFI_STATUS                    GetNextFileStatus;
  EFI_STATUS                    SecurityStatus;
  EFI_FIRMWARE_VOLUME_PROTOCOL  *Fv;
  EFI_DEVICE_PATH_PROTOCOL      *FvDevicePath;
  EFI_HANDLE                    FvHandle;
  UINTN                         BufferSize;
  EFI_GUID                      NameGuid;
  UINTN                         Key;
  EFI_FV_FILETYPE               Type;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINTN                         Size;
  EFI_CORE_DRIVER_ENTRY         *DriverEntry;
  EFI_GUID                      *AprioriFile;
  UINTN                         AprioriEntryCount;
  UINTN                         Index;
  EFI_LIST_ENTRY                *Link;
  UINT32                        AuthenticationStatus;
  UINTN                         SizeOfBuffer;


  while (TRUE) {
    BufferSize = sizeof (EFI_HANDLE);
    Status = CoreLocateHandle (
                    ByRegisterNotify,
                    NULL,
                    mFwVolEventRegistration,
                    &BufferSize,
                    &FvHandle
                    );
    if (EFI_ERROR (Status)) {
      //
      // If no more notification events exit
      //
      return;
    }

    if (FvHasBeenProcessed (FvHandle)) {
      //
      // This Fv has already been processed so lets skip it!
      //
      continue;
    }

    Status = CoreHandleProtocol (FvHandle, &gEfiFirmwareVolumeDispatchProtocolGuid, &Fv);
    if (EFI_ERROR (Status)) {
      //
      // If no dispatch protocol then skip, but do not marked as being processed as it
      // may show up later.
      //
      continue;
    }

    //
    // Since we are about to process this Fv mark it as processed.
    //
    FvIsBeingProcesssed (FvHandle);


    Status = CoreHandleProtocol (FvHandle, &gEfiFirmwareVolumeProtocolGuid, &Fv);
    if (EFI_ERROR (Status) || (Fv == NULL)) {
      //
      // The Handle has a FirmwareVolumeDispatch protocol and should also contiain
      // a FirmwareVolume protocol thus we should never get here.
      //
      ASSERT (FALSE);
      continue;
    }
    
    Status = CoreHandleProtocol (FvHandle, &gEfiDevicePathProtocolGuid, &FvDevicePath);
    if (EFI_ERROR (Status)) {
      //
      // The Firmware volume doesn't have device path, can't be dispatched.
      //
      continue;
    }
    
    //
    // Evaluate the authentication status of the Firmware Volume through 
    // Security Architectural Protocol
    //
    if (gSecurity != NULL) {
      SecurityStatus = gSecurity->FileAuthenticationState (
                                    gSecurity, 
                                    0, 
                                    FvDevicePath
                                    );
      if (SecurityStatus != EFI_SUCCESS) {
        //
        // Security check failed. The firmware volume should not be used for any purpose.
        //
        continue;
      }
    }   
    
    //
    // Discover Drivers in FV and add them to the Discovered Driver List. 
    // Process EFI_FV_FILETYPE_DRIVER type and then EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER 
    //  EFI_FV_FILETYPE_DXE_CORE is processed to produce a Loaded Image protocol for the core
    //  EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE is processed to create a Fvb
    //
    for (Index = 0; Index < sizeof (mDxeFileTypes)/sizeof (EFI_FV_FILETYPE); Index++) {
      //
      // Initialize the search key
      //
      Key = 0;
      do {
        Type = mDxeFileTypes[Index];
        GetNextFileStatus = Fv->GetNextFile (
                                  Fv, 
                                  &Key,
                                  &Type,  
                                  &NameGuid, 
                                  &Attributes, 
                                  &Size
                                  );
        if (!EFI_ERROR (GetNextFileStatus)) {
          if (Type == EFI_FV_FILETYPE_DXE_CORE) {
            //
            // If this is the DXE core fill in it's DevicePath & DeviceHandle
            //
            if (gDxeCoreLoadedImage->FilePath == NULL) {
              if (EfiCompareGuid (&NameGuid, gDxeCoreFileName)) {
			  	CoreInitializeFwVolDevicepathNode (&mFvDevicePath.File, &NameGuid);
                mFvDevicePath.End.Type = EFI_END_ENTIRE_DEVICE_PATH;
                mFvDevicePath.End.SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
                SetDevicePathNodeLength (&mFvDevicePath.End, sizeof (EFI_DEVICE_PATH_PROTOCOL));

                gDxeCoreLoadedImage->FilePath = CoreDuplicateDevicePath (
                                                  (EFI_DEVICE_PATH_PROTOCOL *)&mFvDevicePath
                                                  );
                gDxeCoreLoadedImage->DeviceHandle = FvHandle;
              }
            }
          } else if (Type == EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE) {
            //
            // Found a firmware volume image. Produce a firmware volume block
            // protocol for it so it gets dispatched from. This is usually a 
            // capsule.
            //
            CoreProcessFvImageFile (Fv, FvHandle, &NameGuid);
          } else {
            //
            // Transition driver from Undiscovered to Discovered state
            //
            CoreAddToDriverList (Fv, FvHandle, &NameGuid);
          }
        }
      } while (!EFI_ERROR (GetNextFileStatus));
    }
    
    //
    // Read the array of GUIDs from the Apriori file if it is present in the firmware volume
    //
    AprioriFile = NULL;
    Status = Fv->ReadSection (
                  Fv,
                  &gAprioriGuid,
                  EFI_SECTION_RAW,
                  0,
                  &AprioriFile,
                  &SizeOfBuffer,
                  &AuthenticationStatus
                  );
    if (!EFI_ERROR (Status)) {
      AprioriEntryCount = SizeOfBuffer / sizeof (EFI_GUID);
    } else {
      AprioriEntryCount = 0;
    }

    //
    // Put drivers on Apriori List on the Scheduled queue. The Discovered List includes
    // drivers not in the current FV and these must be skipped since the a priori list
    // is only valid for the FV that it resided in.
    //
    CoreAcquireDispatcherLock ();
    
    for (Index = 0; Index < AprioriEntryCount; Index++) {
      for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
        DriverEntry = CR(Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
        if (EfiCompareGuid (&DriverEntry->FileName, &AprioriFile[Index]) &&
            (FvHandle == DriverEntry->FvHandle)) {
          DriverEntry->Dependent = FALSE;
          DriverEntry->Scheduled = TRUE;
          InsertTailList (&mScheduledQueue, &DriverEntry->ScheduledLink);
          break;
        }
      }
    }

    CoreReleaseDispatcherLock ();

    //
    // Free data allocated by Fv->ReadSection () 
    //
    gBS->FreePool (AprioriFile);  
  }
}


VOID
CoreInitializeDispatcher (
  VOID
  )
/*++

Routine Description:

  Initialize the dispatcher. Initialize the notification function that runs when
  a FV protocol is added to the system.

Arguments:

  NONE

Returns:

  NONE 

--*/
{
  mFwVolEvent = CoreCreateProtocolNotifyEvent (
                  &gEfiFirmwareVolumeProtocolGuid, 
                  EFI_TPL_CALLBACK,
                  CoreFwVolEventProtocolNotify,
                  NULL,
                  &mFwVolEventRegistration,
                  TRUE
                  );
}

//
// Function only used in debug buils
//
DEBUG_CODE (
VOID
CoreDisplayDiscoveredNotDispatched (
  VOID
  )
/*++

Routine Description:

  Traverse the discovered list for any drivers that were discovered but not loaded 
  because the dependency experessions evaluated to false

Arguments:

  NONE

Returns:

  NONE 

--*/
{
  EFI_LIST_ENTRY                *Link;
  EFI_CORE_DRIVER_ENTRY         *DriverEntry;

  for (Link = mDiscoveredList.ForwardLink;Link !=&mDiscoveredList; Link = Link->ForwardLink) {
    DriverEntry = CR(Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
    if (DriverEntry->Dependent) {
      DEBUG ((EFI_D_LOAD, "Driver %g was discovered but not loaded!!\n", &DriverEntry->FileName));
    }
  }
}
)
