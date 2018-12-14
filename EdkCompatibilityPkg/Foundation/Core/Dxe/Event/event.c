/*++

Copyright (c) 2004 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  event.c

Abstract:

  EFI Event support.
 
--*/

#include "exec.h"
#include "hand.h"

//
// Enumerate the valid types
//
UINT32 mEventTable[] = {
  //
  // 0x80000200       Timer event with a notification function that is
  // queue when the event is signaled with SignalEvent()
  //
  EFI_EVENT_TIMER | EFI_EVENT_NOTIFY_SIGNAL,
  //
  // 0x80000000       Timer event without a notification function. It can be
  // signaled with SignalEvent() and checked with CheckEvent() or WaitForEvent().
  //
  EFI_EVENT_TIMER,
  //
  // 0x00000100       Generic event with a notification function that 
  // can be waited on with CheckEvent() or WaitForEvent()
  //
  EFI_EVENT_NOTIFY_WAIT,
  //
  // 0x00000200       Generic event with a notification function that 
  // is queue when the event is signaled with SignalEvent()
  //
  EFI_EVENT_NOTIFY_SIGNAL,
  //
  // 0x00000201       ExitBootServicesEvent.  
  //
  EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES,
  //
  // 0x60000202       SetVirtualAddressMapEvent.
  //
  EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
#if (EFI_SPECIFICATION_VERSION < 0x00020000)
  //
  // 0x00000203       ReadyToBootEvent.
  //
  EFI_EVENT_SIGNAL_READY_TO_BOOT,
  //
  // 0x00000204       LegacyBootEvent.
  //
  EFI_EVENT_SIGNAL_LEGACY_BOOT,
  //
  // 0x00000603       Signal all ReadyToBootEvents.
  //
  EFI_EVENT_NOTIFY_SIGNAL_ALL | EFI_EVENT_SIGNAL_READY_TO_BOOT,
  //
  // 0x00000604       Signal all LegacyBootEvents.
  //
  EFI_EVENT_NOTIFY_SIGNAL_ALL | EFI_EVENT_SIGNAL_LEGACY_BOOT,
#endif
  //
  // 0x00000000       Generic event without a notification function. 
  // It can be signaled with SignalEvent() and checked with CheckEvent() 
  // or WaitForEvent().
  //
  0x00000000,
  //
  // 0x80000100       Timer event with a notification function that can be 
  // waited on with CheckEvent() or WaitForEvent()
  //
  EFI_EVENT_TIMER | EFI_EVENT_NOTIFY_WAIT,
};


VOID
CoreAcquireEventLock (
  VOID
  )
/*++

Routine Description:

  Enter critical section by acquiring the lock on gEventQueueLock.

Arguments:

  None

Returns:

  None

--*/
{
  CoreAcquireLock (&gEventQueueLock);
}


VOID
CoreReleaseEventLock (
  VOID
  )
/*++

Routine Description:

  Exit critical section by releasing the lock on gEventQueueLock.

Arguments:

  None

Returns:

  None

--*/
{
  CoreReleaseLock (&gEventQueueLock);
}


EFI_STATUS
CoreInitializeEventServices (
  VOID
  )
/*++

Routine Description:

  Initializes "event" support and populates parts of the System and Runtime Table.

Arguments:

  None
    
Returns:

  EFI_SUCCESS - Always return success

--*/
{
  UINTN        Index;

  for (Index=0; Index <= EFI_TPL_HIGH_LEVEL; Index ++) {
    InitializeListHead (&gEventQueue[Index]);
  }

  CoreInitializeTimer ();
  
  return EFI_SUCCESS;
}


VOID
CoreDispatchEventNotifies (
  IN EFI_TPL      Priority
  )
/*++

Routine Description:

  Dispatches all pending events. 

Arguments:

  Priority - The task priority level of event notifications to dispatch
    
Returns:

  None

--*/
{
  IEVENT          *Event;
  EFI_LIST_ENTRY  *Head;
  
  CoreAcquireEventLock ();
  ASSERT (gEventQueueLock.OwnerTpl == Priority);
  Head = &gEventQueue[Priority];

  //
  // Dispatch all the pending notifications
  //
  while (!IsListEmpty (Head)) {
      
    Event = CR (Head->ForwardLink, IEVENT, NotifyLink, EVENT_SIGNATURE);
    RemoveEntryList (&Event->NotifyLink);

    Event->NotifyLink.ForwardLink = NULL;

    //
    // Only clear the SIGNAL status if it is a SIGNAL type event.
    // WAIT type events are only cleared in CheckEvent()
    //
    if (Event->Type & EFI_EVENT_NOTIFY_SIGNAL) {
      Event->SignalCount = 0;
    }

    CoreReleaseEventLock ();
      
    //
    // Notify this event
    //
    Event->NotifyFunction (Event, Event->NotifyContext);

    //
    // Check for next pending event
    //
    CoreAcquireEventLock ();
  }

  gEventPending &= ~(1 << Priority);
  CoreReleaseEventLock ();
}


VOID
STATIC
CoreNotifyEvent (
  IN  IEVENT      *Event
  )
/*++

Routine Description:

  Queues the event's notification function to fire

Arguments:

  Event       - The Event to notify
    
Returns:

  None

--*/
{

  //
  // Event database must be locked
  //
  ASSERT_LOCKED (&gEventQueueLock);

  //
  // If the event is queued somewhere, remove it
  //

  if (Event->NotifyLink.ForwardLink != NULL) {
    RemoveEntryList (&Event->NotifyLink);
    Event->NotifyLink.ForwardLink = NULL;
  }

  // 
  // Queue the event to the pending notification list
  //

  InsertTailList (&gEventQueue[Event->NotifyTpl], &Event->NotifyLink);
  gEventPending |= (UINTN)1 << Event->NotifyTpl;
}



VOID
CoreNotifySignalList (
  IN EFI_GUID     *EventGroup
  )
/*++

Routine Description:

  Signals all events on the requested list

Arguments:

  SignalType      - The list to signal
    
Returns:

  None

--*/
{
  EFI_LIST_ENTRY          *Link;
  EFI_LIST_ENTRY          *Head;
  IEVENT                  *Event;

  CoreAcquireEventLock ();

  Head = &gEventSignalQueue;
  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    Event = CR (Link, IEVENT, SignalLink, EVENT_SIGNATURE);
    if ((Event->ExFlag) && EfiCompareGuid (&Event->EventGroup, EventGroup)) {
      CoreNotifyEvent (Event);
    }
  }

  CoreReleaseEventLock ();
}

#if (EFI_SPECIFICATION_VERSION < 0x00020000)

static
VOID
EFIAPI
EventNotifySignalAllNullEvent (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  //
  // This null event is a size efficent way to enusre that 
  // EFI_EVENT_NOTIFY_SIGNAL_ALL is error checked correctly.
  // EFI_EVENT_NOTIFY_SIGNAL_ALL is now mapped into 
  // CreateEventEx() and this function is used to make the
  // old error checking in CreateEvent() for Tiano extensions
  // function.
  //
  return;
}

#endif


EFI_BOOTSERVICE
EFI_STATUS
EFIAPI
CoreCreateEvent (
  IN UINT32                   Type,
  IN EFI_TPL                  NotifyTpl,
  IN EFI_EVENT_NOTIFY         NotifyFunction,
  IN VOID                     *NotifyContext,
  OUT EFI_EVENT               *Event
  )
/*++

Routine Description:

  Creates a general-purpose event structure

Arguments:

  Type                - The type of event to create and its mode and attributes
  NotifyTpl           - The task priority level of event notifications
  NotifyFunction      - Pointer to the events notification function
  NotifyContext       - Pointer to the notification functions context; corresponds to
                        parameter "Context" in the notification function
  Event               - Pointer to the newly created event if the call succeeds; undefined otherwise

Returns:

  EFI_SUCCESS           - The event structure was created
  EFI_INVALID_PARAMETER - One of the parameters has an invalid value
  EFI_OUT_OF_RESOURCES  - The event could not be allocated

--*/
{ 
  EFI_GUID            *GuidPtr;
  EFI_EVENT_NOTIFY    Function;
  
  GuidPtr = NULL;
  Function = NotifyFunction;

#if (EFI_SPECIFICATION_VERSION < 0x00020000)
  //
  // Clear EFI_EVENT_NOFITY_SIGNAL_ALL (Tiano extension) as all events in the 
  // EventGroup now have this property. So we need to filter it out.
  //
  if (Type & EFI_EVENT_NOTIFY_SIGNAL_ALL) {
    Type &= ~EFI_EVENT_NOTIFY_SIGNAL_ALL;
    Function = EventNotifySignalAllNullEvent;
  }

  //
  // Map the Tiano extensions Events to CreateEventEx form
  //
  if (Type == EFI_EVENT_SIGNAL_READY_TO_BOOT) {
    GuidPtr = &gEfiEventReadyToBootGuid;
  } else if (Type == EFI_EVENT_SIGNAL_LEGACY_BOOT) {
    GuidPtr = &gEfiEventLegacyBootGuid;
  }
#endif

  //
  // Convert EFI 1.10 Events to thier UEFI 2.0 CreateEventEx mapping
  // 
  if (Type == EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES) {
    Type = 0;
    GuidPtr = &gEfiEventExitBootServicesGuid;
  } else if (Type == EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE) {
    Type = 0;
    GuidPtr = &gEfiEventVirtualAddressChangeGuid;
  }
  
  return CoreCreateEventEx (Type, NotifyTpl, Function, NotifyContext, GuidPtr, Event);
}

EFI_BOOTSERVICE
EFI_STATUS
EFIAPI
CoreCreateEventEx (
  IN UINT32                   Type,
  IN EFI_TPL                  NotifyTpl,
  IN EFI_EVENT_NOTIFY         NotifyFunction,
  IN VOID                     *NotifyContext,
  IN CONST EFI_GUID           *EventGroup,    OPTIONAL
  OUT EFI_EVENT               *Event
  )
/*++

Routine Description:
  Creates a general-purpose event structure

Arguments:
  Type                - The type of event to create and its mode and attributes
  NotifyTpl           - The task priority level of event notifications
  NotifyFunction      - Pointer to the events notification function
  NotifyContext       - Pointer to the notification functions context; corresponds to
                        parameter "Context" in the notification function
  EventGroup          - GUID for EventGroup if NULL act the same as gBS->CreateEvent().
  Event               - Pointer to the newly created event if the call succeeds; undefined otherwise

Returns:
  EFI_SUCCESS           - The event structure was created
  EFI_INVALID_PARAMETER - One of the parameters has an invalid value
  EFI_OUT_OF_RESOURCES  - The event could not be allocated

--*/
{
  EFI_STATUS      Status;
  IEVENT          *IEvent;
  INTN            Index;


  if ((Event == NULL) || (NotifyTpl == EFI_TPL_APPLICATION)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // For event group, type EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES and EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE
  // are not valid
  //
  if (EventGroup != NULL) {
    if ((Type == EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES) || (Type == EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Check to make sure no reserved flags are set
  //
  Status = EFI_INVALID_PARAMETER;
  for (Index = 0; Index < (sizeof (mEventTable) / sizeof (UINT32)); Index++) {
    if (Type == mEventTable[Index]) {
      Status = EFI_SUCCESS;
      break;
    }
  }
  if(EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Convert Event type for pre-defined Event groups
  //
  if (EventGroup != NULL) {
    if (EfiCompareGuid ((VOID *) EventGroup, &gEfiEventExitBootServicesGuid)) {
      Type = EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES;
    } else if (EfiCompareGuid ((VOID *) EventGroup, &gEfiEventVirtualAddressChangeGuid)) {
      Type = EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE;
    }
  }

  //
  // If it's a notify type of event, check its parameters
  //
  if ((Type & (EFI_EVENT_NOTIFY_WAIT | EFI_EVENT_NOTIFY_SIGNAL))) {
    //
    // Check for an invalid NotifyFunction or NotifyTpl
    //
    if ((NotifyFunction == NULL) || 
        (NotifyTpl < EFI_TPL_APPLICATION) || 
       (NotifyTpl >= EFI_TPL_HIGH_LEVEL)) {
      return EFI_INVALID_PARAMETER;
    }

  } else {
    //
    // No notification needed, zero ignored values
    //
    NotifyTpl = 0;
    NotifyFunction = NULL;
    NotifyContext = NULL;
  }

  //
  // Allcoate and initialize a new event structure.
  //
  Status = CoreAllocatePool (
             (Type & EFI_EVENT_RUNTIME) ? EfiRuntimeServicesData: EfiBootServicesData, 
             sizeof (IEVENT),
             (VOID **)&IEvent
             );
  if (EFI_ERROR (Status) || (IEvent == NULL)) {
    return EFI_OUT_OF_RESOURCES;
  }

  EfiCommonLibSetMem (IEvent, sizeof (IEVENT), 0);

  IEvent->Signature      = EVENT_SIGNATURE;
  IEvent->Type           = Type;
  IEvent->NotifyTpl      = NotifyTpl;
  IEvent->NotifyFunction = NotifyFunction;
  IEvent->NotifyContext  = (VOID *)NotifyContext;
  if (EventGroup != NULL) {
    EfiCommonLibCopyMem (&IEvent->EventGroup, (VOID*)EventGroup, sizeof (EFI_GUID));
    IEvent->ExFlag = TRUE;
  }

  *Event = IEvent;

  if (Type & EFI_EVENT_RUNTIME) {
    //
    // Keep a list of all RT events so we can tell the RT AP.
    //
    IEvent->RuntimeData.Type           = Type;
    IEvent->RuntimeData.NotifyTpl      = NotifyTpl;
    IEvent->RuntimeData.NotifyFunction = NotifyFunction;
    IEvent->RuntimeData.NotifyContext  = NotifyContext;
    IEvent->RuntimeData.Event          = (EFI_EVENT *) IEvent;
    InsertTailList (&gRuntime->EventHead, &IEvent->RuntimeData.Link);
  }

  CoreAcquireEventLock ();
  
  if ((Type & EFI_EVENT_NOTIFY_SIGNAL) != 0x00000000) {
    //
    // The Event's NotifyFunction must be queued whenever the event is signaled
    //
    InsertHeadList (&gEventSignalQueue, &IEvent->SignalLink);
  }
  
  CoreReleaseEventLock ();
  
  //
  // Done
  //
  return EFI_SUCCESS;
}

EFI_BOOTSERVICE
EFI_STATUS
EFIAPI
CoreSignalEvent (
  IN EFI_EVENT    UserEvent
  )
/*++

Routine Description:

  Signals the event.  Queues the event to be notified if needed
    
Arguments:

  UserEvent - The event to signal
    
Returns:

  EFI_INVALID_PARAMETER - Parameters are not valid.
  
  EFI_SUCCESS - The event was signaled.

--*/
{
  IEVENT          *Event;

  Event = UserEvent;

  if (Event == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Event->Signature != EVENT_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  CoreAcquireEventLock ();

  //
  // If the event is not already signalled, do so
  //

  if (Event->SignalCount == 0x00000000) {
    Event->SignalCount ++;

    //
    // If signalling type is a notify function, queue it
    //
    if (Event->Type & EFI_EVENT_NOTIFY_SIGNAL) {
      if (Event->ExFlag) {
        //
        // The CreateEventEx() style requires all members of the Event Group 
        //  to be signaled. 
        //
        CoreReleaseEventLock ();
        CoreNotifySignalList (&Event->EventGroup);
        CoreAcquireEventLock ();
       } else {
        CoreNotifyEvent (Event);
      }
    }
  }

  CoreReleaseEventLock ();
  return EFI_SUCCESS;
}

EFI_BOOTSERVICE
EFI_STATUS
EFIAPI
CoreCheckEvent (
  IN EFI_EVENT        UserEvent
  )
/*++

Routine Description:

  Check the status of an event
    
Arguments:

  UserEvent - The event to check
    
Returns:

  EFI_SUCCESS           - The event is in the signaled state
  EFI_NOT_READY         - The event is not in the signaled state
  EFI_INVALID_PARAMETER - Event is of type EVT_NOTIFY_SIGNAL

--*/

{
  IEVENT      *Event;
  EFI_STATUS  Status;

  Event = UserEvent;

  if (Event == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Event->Signature != EVENT_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  if (Event->Type & EFI_EVENT_NOTIFY_SIGNAL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_NOT_READY;

  if (!Event->SignalCount && (Event->Type & EFI_EVENT_NOTIFY_WAIT)) {

    //
    // Queue the wait notify function
    //

    CoreAcquireEventLock ();
    if (!Event->SignalCount) {
      CoreNotifyEvent (Event);
    }
    CoreReleaseEventLock ();
  }

  //
  // If the even looks signalled, get the lock and clear it
  //

  if (Event->SignalCount) {
    CoreAcquireEventLock ();

    if (Event->SignalCount) {
      Event->SignalCount = 0;
      Status = EFI_SUCCESS;
    }

    CoreReleaseEventLock ();
  }

  return Status;
}


EFI_BOOTSERVICE
EFI_STATUS
EFIAPI
CoreWaitForEvent (
  IN UINTN        NumberOfEvents,
  IN EFI_EVENT    *UserEvents,
  OUT UINTN       *UserIndex
  )
/*++

Routine Description:

  Stops execution until an event is signaled.
    
Arguments:

  NumberOfEvents  - The number of events in the UserEvents array
  UserEvents      - An array of EFI_EVENT
  UserIndex       - Pointer to the index of the event which satisfied the wait condition
    
Returns:

  EFI_SUCCESS           - The event indicated by Index was signaled.
  EFI_INVALID_PARAMETER - The event indicated by Index has a notification function or 
                          Event was not a valid type
  EFI_UNSUPPORTED       - The current TPL is not TPL_APPLICATION

--*/

{
  EFI_STATUS      Status;
  UINTN           Index;

  //
  // Can only WaitForEvent at TPL_APPLICATION
  //
  if (gEfiCurrentTpl != EFI_TPL_APPLICATION) {
    return EFI_UNSUPPORTED;
  }

  for(;;) {
      
    for(Index = 0; Index < NumberOfEvents; Index++) {

      Status = CoreCheckEvent (UserEvents[Index]);

      //
      // provide index of event that caused problem
      //
      if (Status != EFI_NOT_READY) {
        *UserIndex = Index;
        return Status;
      }
    }

    //
    // This was the location of the Idle loop callback in EFI 1.x reference
    // code. We don't have that concept in this base at this point.
    // 
  }
}

EFI_BOOTSERVICE
EFI_STATUS
EFIAPI
CoreCloseEvent (
  IN EFI_EVENT    UserEvent
  )
/*++

Routine Description:

  Closes an event and frees the event structure.
    
Arguments:

  UserEvent - Event to close
    
Returns:

  EFI_INVALID_PARAMETER - Parameters are not valid.
  
  EFI_SUCCESS - The event has been closed

--*/

{
  EFI_STATUS  Status;
  IEVENT      *Event;

  Event = UserEvent;

  if (Event == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Event->Signature != EVENT_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If it's a timer event, make sure it's not pending
  //
  if (Event->Type & EFI_EVENT_TIMER) {
    CoreSetTimer (Event, TimerCancel, 0);
  }

  CoreAcquireEventLock ();

  //
  // If the event is queued somewhere, remove it
  //
  
  if (Event->RuntimeData.Link.ForwardLink != NULL) {
    RemoveEntryList (&Event->RuntimeData.Link);
  }
  
  if (Event->NotifyLink.ForwardLink != NULL) {
    RemoveEntryList (&Event->NotifyLink);
  }

  if (Event->SignalLink.ForwardLink != NULL) {
    RemoveEntryList (&Event->SignalLink);
  }

  CoreReleaseEventLock ();

  //
  // If the event is registered on a protocol notify, 
  // then remove it from the protocol database
  //
  CoreUnregisterProtocolNotify (Event);

  Status = CoreFreePool (Event);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
