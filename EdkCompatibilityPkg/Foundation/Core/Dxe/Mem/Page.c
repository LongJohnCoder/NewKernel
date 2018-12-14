/*++

Copyright (c) 2007 - 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  page.c

Abstract:

  EFI Memory page management


Revision History

--*/

#include "imem.h"

#define EFI_DEFAULT_PAGE_ALLOCATION_ALIGNMENT  (EFI_PAGE_SIZE)

//
// Entry for tracking the memory regions for each memory type to help coalesce like memory types
//
typedef struct {
  EFI_PHYSICAL_ADDRESS  BaseAddress;           // Base address of the coalesce bin if NumberOfPages is not 0, or 0 otherwise
  EFI_PHYSICAL_ADDRESS  MaximumAddress;        // Top address of the coalesce bin if NumberOfPages is not 0, or the top address below all bin ranges
  UINT64                CurrentNumberOfPages;  // CurrentNumberOfPages allocated for this memory type
  UINT64                NumberOfPages;         // Number of pages specified in gMemoryTypeInformation
  UINTN                 InformationIndex;      // Index into gMemoryTypeInformation
  BOOLEAN               Special;               // If this type of coalesce bin needs to be filled in memory map
  BOOLEAN               Runtime;               // If this type is runtime available
} EFI_MEMORY_TYPE_STAISTICS;

//
// MemoryMap - The current memory map
//
UINTN     mMemoryMapKey = 0;

//
// mMapStack - space to use as temp storage to build new map descriptors
// mMapDepth - depth of new descriptor stack
//

#define MAX_MAP_DEPTH 6
UINTN         mMapDepth = 0;
MEMORY_MAP    mMapStack[MAX_MAP_DEPTH];
UINTN         mFreeMapStack = 0;
//
// This list maintain the free memory map list
//
EFI_LIST_ENTRY   mFreeMemoryMapEntryList  = INITIALIZE_LIST_HEAD_VARIABLE (mFreeMemoryMapEntryList);
BOOLEAN mMemoryTypeInformationInitialized = FALSE;

EFI_MEMORY_TYPE_STAISTICS mMemoryTypeStatistics[EfiMaxMemoryType + 1] = {
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE },  // EfiReservedMemoryType
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiLoaderCode
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiLoaderData
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiBootServicesCode
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiBootServicesData
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  TRUE  },  // EfiRuntimeServicesCode
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  TRUE  },  // EfiRuntimeServicesData
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiConventionalMemory
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiUnusableMemory
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE },  // EfiACPIReclaimMemory
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE },  // EfiACPIMemoryNVS
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiMemoryMappedIO
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiMemoryMappedIOPortSpace
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  TRUE  },  // EfiPalCode
  { 0, EFI_MAX_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE }   // EfiMaxMemoryType
};

EFI_PHYSICAL_ADDRESS mDefaultMaximumAddress = EFI_MAX_ADDRESS;

EFI_MEMORY_TYPE_INFORMATION gMemoryTypeInformation[EfiMaxMemoryType + 1] = {
  { EfiReservedMemoryType,      0 },
  { EfiLoaderCode,              0 },
  { EfiLoaderData,              0 },
  { EfiBootServicesCode,        0 },
  { EfiBootServicesData,        0 },
  { EfiRuntimeServicesCode,     0 },
  { EfiRuntimeServicesData,     0 },
  { EfiConventionalMemory,      0 },
  { EfiUnusableMemory,          0 },
  { EfiACPIReclaimMemory,       0 },
  { EfiACPIMemoryNVS,           0 },
  { EfiMemoryMappedIO,          0 },
  { EfiMemoryMappedIOPortSpace, 0 },
  { EfiPalCode,                 0 },
  { EfiMaxMemoryType,           0 }
};

//
// Internal prototypes
//
VOID 
PromoteMemoryResource (
  VOID
);

STATIC
VOID
CoreAddRange (
  IN EFI_MEMORY_TYPE          Type,
  IN EFI_PHYSICAL_ADDRESS     Start,
  IN EFI_PHYSICAL_ADDRESS     End,
  IN UINT64                   Attribute
  );

STATIC
VOID
CoreFreeMemoryMapStack (
  VOID
  );

STATIC
EFI_STATUS
CoreConvertPages (
  IN UINT64           Start,
  IN UINT64           NumberOfPages,
  IN EFI_MEMORY_TYPE  NewType
  );

STATIC
VOID
RemoveMemoryMapEntry (
  MEMORY_MAP      *Entry
  );
  

MEMORY_MAP *
AllocateMemoryMapEntry ( 
 );
 
VOID
CoreAcquireMemoryLock (
  VOID
  )
/*++

Routine Description:

  Enter critical section by gaining lock on gMemoryLock

Arguments:

  None

Returns:

  None

--*/
{
  CoreAcquireLock (&gMemoryLock);
}


VOID
CoreReleaseMemoryLock (
  VOID
  )
/*++

Routine Description:

  Exit critical section by releasing lock on gMemoryLock

Arguments:

  None

Returns:

  None

--*/
{
  CoreReleaseLock (&gMemoryLock);
}

VOID
PromoteMemoryResource (
  VOID
  )
/*++

Routine Description:

  Find untested but initialized memory regions in GCD map and convert them to be DXE allocatable.

Arguments:

  None

Returns:

  None

--*/
{
  EFI_LIST_ENTRY                   *Link;
  EFI_GCD_MAP_ENTRY                *Entry;

  DEBUG ((EFI_D_ERROR | EFI_D_PAGE, "Promote the memory resource\n"));
  
  CoreAcquireGcdMemoryLock ();
  
  Link = mGcdMemorySpaceMap.ForwardLink;
  while (Link != &mGcdMemorySpaceMap) {

    Entry = CR (Link, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);

    if (Entry->GcdMemoryType == EfiGcdMemoryTypeReserved &&
        Entry->EndAddress < EFI_MAX_ADDRESS &&
        (Entry->Capabilities & (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED)) ==
          (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED)) {
      //
      // Update the GCD map
      //
      Entry->GcdMemoryType = EfiGcdMemoryTypeSystemMemory;
      Entry->Capabilities |= EFI_MEMORY_TESTED;
      Entry->ImageHandle  = gDxeCoreImageHandle;
      Entry->DeviceHandle = NULL;

      //
      // Add to allocable system memory resource
      //      

      CoreAddRange (
        EfiConventionalMemory, 
        Entry->BaseAddress, 
        Entry->EndAddress, 
        Entry->Capabilities & ~(EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED | EFI_MEMORY_RUNTIME)
        );
      mMemoryTypeStatistics[EfiConventionalMemory].CurrentNumberOfPages += RShiftU64 ((Entry->EndAddress - Entry->BaseAddress + 1), EFI_PAGE_SHIFT);
      CoreFreeMemoryMapStack ();
      
    }

    Link = Link->ForwardLink;
  }
  
  CoreReleaseGcdMemoryLock ();
  
  return;
}

VOID
CoreAddMemoryDescriptor (
  IN EFI_MEMORY_TYPE       Type,
  IN EFI_PHYSICAL_ADDRESS  Start,
  IN UINT64                NumberOfPages,
  IN UINT64                Attribute
  )
/*++

Routine Description:

  Called to initialize the memory map and add descriptors to
  the current descriptor list.

  N.B. The first descriptor that is added must be general usable
  memory as the addition allocates heap.

Arguments:

  Type          - The type of memory to add

  Start         - The starting address in the memory range
                  Must be page aligned

  NumberOfPages - The number of pages in the range

  Attribute     - Attributes of the memory to add

Returns:

  None.  The range is added to the memory map

--*/
{
  EFI_PHYSICAL_ADDRESS        End;
  EFI_STATUS                  Status;
  UINTN                       Index;
  UINTN                       FreeIndex;

  if ((Start & EFI_PAGE_MASK) != 0) {
    return;
  }

  if (Type >= EfiMaxMemoryType && Type <= 0x7fffffff) {
    return;
  }
  
  CoreAcquireMemoryLock ();
  End = Start + LShiftU64 (NumberOfPages, EFI_PAGE_SHIFT) - 1;
  CoreAddRange (Type, Start, End, Attribute);
  mMemoryTypeStatistics[Type].CurrentNumberOfPages += NumberOfPages;
  CoreFreeMemoryMapStack ();
  CoreReleaseMemoryLock ();

  //
  // Check to see if the statistics for the different memory types have already been established
  //
  if (mMemoryTypeInformationInitialized) {
    return;
  }

  //
  // Loop through each memory type in the order specified by the gMemoryTypeInformation[] array
  //
  for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Make sure the memory type in the gMemoryTypeInformation[] array is valid
    //
    Type = gMemoryTypeInformation[Index].Type;
    if (Type < 0 || Type > EfiMaxMemoryType) {
      continue;
    }

    if (gMemoryTypeInformation[Index].NumberOfPages != 0) {
      //
      // Allocate pages for the current memory type from the top of available memory
      //
      Status = CoreAllocatePages (
                 AllocateAnyPages,
                 Type,
                 gMemoryTypeInformation[Index].NumberOfPages,
                 &mMemoryTypeStatistics[Type].BaseAddress
                 );
      if (EFI_ERROR (Status)) {
        //
        // If an error occurs allocating the pages for the current memory type, then 
        // free all the pages allocates for the previous memory types and return.  This
        // operation with be retied when/if more memory is added to the system
        //
        for (FreeIndex = 0; FreeIndex < Index; FreeIndex++) {
          //
          // Make sure the memory type in the gMemoryTypeInformation[] array is valid
          //
          Type = gMemoryTypeInformation[FreeIndex].Type;
          if (Type < 0 || Type > EfiMaxMemoryType) {
            continue;
          }

          if (gMemoryTypeInformation[FreeIndex].NumberOfPages != 0) {
            CoreFreePages (
              mMemoryTypeStatistics[Type].BaseAddress, 
              gMemoryTypeInformation[FreeIndex].NumberOfPages
              );
            mMemoryTypeStatistics[Type].BaseAddress    = 0;
            mMemoryTypeStatistics[Type].MaximumAddress = EFI_MAX_ADDRESS;
          }
        }
        return;
      }

      //
      // Compute the address at the top of the current statistics
      //
      mMemoryTypeStatistics[Type].MaximumAddress = 
        mMemoryTypeStatistics[Type].BaseAddress + 
        LShiftU64 (gMemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT) - 1;

      //
      // If the current base address is the lowest address so far, then update the default 
      // maximum address
      //
      if (mMemoryTypeStatistics[Type].BaseAddress < mDefaultMaximumAddress) {
        mDefaultMaximumAddress = mMemoryTypeStatistics[Type].BaseAddress - 1;
      }
    }
  }

  //
  // There was enough system memory for all the the memory types were allocated.  So,
  // those memory areas can be freed for future allocations, and all future memory
  // allocations can occur within their respective bins
  //
  for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Make sure the memory type in the gMemoryTypeInformation[] array is valid
    //
    Type = gMemoryTypeInformation[Index].Type;
    if (Type < 0 || Type > EfiMaxMemoryType) {
      continue;
    }

    if (gMemoryTypeInformation[Index].NumberOfPages != 0) {
      CoreFreePages (
        mMemoryTypeStatistics[Type].BaseAddress, 
        gMemoryTypeInformation[Index].NumberOfPages
        );
      mMemoryTypeStatistics[Type].NumberOfPages   = gMemoryTypeInformation[Index].NumberOfPages;
      gMemoryTypeInformation[Index].NumberOfPages = 0;
    }
  }

  //
  // If the number of pages reserved for a memory type is 0, then all allocations for that type
  // should be in the default range.
  //
  for (Type = 0; Type < EfiMaxMemoryType; Type++) {
    for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
      if (Type == (EFI_MEMORY_TYPE)gMemoryTypeInformation[Index].Type) {
        mMemoryTypeStatistics[Type].InformationIndex = Index;
      }
    }
    if (mMemoryTypeStatistics[Type].MaximumAddress == EFI_MAX_ADDRESS) {
      mMemoryTypeStatistics[Type].MaximumAddress = mDefaultMaximumAddress;
    }
  }

  mMemoryTypeInformationInitialized = TRUE;
}


STATIC
VOID
CoreAddRange (
  IN EFI_MEMORY_TYPE        Type,
  IN EFI_PHYSICAL_ADDRESS     Start,
  IN EFI_PHYSICAL_ADDRESS     End,
  IN UINT64           Attribute
  )
/*++

Routine Description:

  Internal function.  Adds a ranges to the memory map.
  The range must not already exist in the map.

Arguments:

  Type    - The type of memory range to add

  Start   - The starting address in the memory range
            Must be paged aligned

  End     - The last address in the range
          Must be the last byte of a page

  Attribute - The attributes of the memory range to add

Returns:

  None.  The range is added to the memory map

--*/
{
  EFI_LIST_ENTRY    *Link;
  MEMORY_MAP        *Entry;

  ASSERT ((Start & EFI_PAGE_MASK) == 0);
  ASSERT (End > Start) ;

  ASSERT_LOCKED (&gMemoryLock);
  
  DEBUG ((EFI_D_PAGE, "AddRange: %lx-%lx to %d\n", Start, End, (UINTN)Type));

  //
  // Memory map being altered
  //

  mMemoryMapKey += 1;
  
  //
  // Look for adjoining memory descriptor
  //
  
  // Two memory descriptors can only be merged if they have the same Type
  // and the same Attribute
  //

  Link = gMemoryMap.ForwardLink;
  while (Link != &gMemoryMap) {
    Entry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);
    Link  = Link->ForwardLink;

    if (Entry->Type != Type) {
      continue;
    }

    if (Entry->Attribute != Attribute) {
      continue;
    }

    if (Entry->End + 1 == Start) {
      
      Start = Entry->Start;
      RemoveMemoryMapEntry (Entry);

    } else if (Entry->Start == End + 1) {
      
      End = Entry->End;
      RemoveMemoryMapEntry (Entry);
    }
  }

  //
  // Add descriptor 
  //

  mMapStack[mMapDepth].Signature     = MEMORY_MAP_SIGNATURE;
  mMapStack[mMapDepth].FromPages      = FALSE;
  mMapStack[mMapDepth].Type          = Type;
  mMapStack[mMapDepth].Start         = Start;
  mMapStack[mMapDepth].End           = End;
  mMapStack[mMapDepth].VirtualStart  = 0;
  mMapStack[mMapDepth].Attribute     = Attribute;
  InsertTailList (&gMemoryMap, &mMapStack[mMapDepth].Link);

  mMapDepth += 1;
  ASSERT (mMapDepth < MAX_MAP_DEPTH);
  return ;
}

STATIC
VOID
CoreFreeMemoryMapStack (
  VOID
  )
/*++

Routine Description:

  Internal function.  Moves any memory descriptors that are on the
  temporary descriptor stack to heap.

Arguments:

  None

Returns:

  None

--*/
{
  MEMORY_MAP      *Entry;
  MEMORY_MAP      *Entry2;
  EFI_LIST_ENTRY  *Link2;

  ASSERT_LOCKED (&gMemoryLock);

  //
  // If already freeing the map stack, then return
  //
  if (mFreeMapStack) {
    return ;
  }

  //
  // Move the temporary memory descriptor stack into pool
  //
  mFreeMapStack += 1;

  while (mMapDepth) {
    //
    // Deque an memory map entry from mFreeMemoryMapEntryList 
    //
    Entry = AllocateMemoryMapEntry ();
    
    ASSERT (Entry);

    //
    // Update to proper entry
    //
    mMapDepth -= 1;

    if (mMapStack[mMapDepth].Link.ForwardLink != NULL) {

      //
      // Move this entry to general memory
      //
      RemoveEntryList (&mMapStack[mMapDepth].Link);
      mMapStack[mMapDepth].Link.ForwardLink = NULL;

      *Entry = mMapStack[mMapDepth];
      Entry->FromPages = TRUE;

      //
      // Find insertion location
      //
      for (Link2 = gMemoryMap.ForwardLink; Link2 != &gMemoryMap; Link2 = Link2->ForwardLink) {
        Entry2 = CR (Link2, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);
        if (Entry2->FromPages && Entry2->Start > Entry->Start) {
          break;
        }
      }

      InsertTailList (Link2, &Entry->Link);

    } else {
      // 
      // This item of mMapStack[mMapDepth] has already been dequeued from gMemoryMap list,
      // so here no need to move it to memory.
      //
      InsertTailList (&mFreeMemoryMapEntryList, &Entry->Link);
    }
  }

  mFreeMapStack -= 1;
}

STATIC
VOID
RemoveMemoryMapEntry (
  MEMORY_MAP      *Entry
  )
/*++

Routine Description:

  Internal function.  Removes a descriptor entry.

Arguments:

  Entry   - The entry to remove

Returns:

  None

--*/
{
  RemoveEntryList (&Entry->Link);
  Entry->Link.ForwardLink = NULL;

  if (Entry->FromPages) {
  	//
  	// Insert the free memory map descriptor to the end of mFreeMemoryMapEntryList
  	//
    InsertTailList (&mFreeMemoryMapEntryList, &Entry->Link);
  }
}

MEMORY_MAP *
AllocateMemoryMapEntry ( 
 )
/*++

Routine Description:

  Internal function.  Deque a descriptor entry from the mFreeMemoryMapEntryList.
  If the list is emtry, then allocate a new page to refuel the list. 
  Please Note this algorithm to allocate the memory map descriptor has a property
  that the memory allocated for memory entries always grows, and will never really be freed 
  For example, if the current boot uses 2000 memory map entries at the maximum point, but
  ends up with only 50 at the time the OS is booted, then the memory associated with the 1950 
  memory map entries is still allocated from EfiBootServicesMemory.  

Arguments:

  NONE

Returns:

  The Memory map descriptor dequed from the mFreeMemoryMapEntryList

--*/ 
{
  MEMORY_MAP*            FreeDescriptorEntries;
  MEMORY_MAP*            Entry;
  UINTN                  Index;
  
  if (IsListEmpty (&mFreeMemoryMapEntryList)) {
    // 
    // The list is empty, to allocate one page to refuel the list
    //
    FreeDescriptorEntries = CoreAllocatePoolPages (EfiBootServicesData, EFI_SIZE_TO_PAGES(DEFAULT_PAGE_ALLOCATION), DEFAULT_PAGE_ALLOCATION);
    if(FreeDescriptorEntries != NULL) {
      //
      // Enque the free memmory map entries into the list
      //
      for (Index = 0; Index< DEFAULT_PAGE_ALLOCATION / sizeof(MEMORY_MAP); Index++) {
        FreeDescriptorEntries[Index].Signature = MEMORY_MAP_SIGNATURE;
        InsertTailList (&mFreeMemoryMapEntryList, &FreeDescriptorEntries[Index].Link);
      }     
    } else {
      return NULL;
    }
  }
  //
  // dequeue the first descriptor from the list
  //
  Entry = CR (mFreeMemoryMapEntryList.ForwardLink, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);
  RemoveEntryList (&Entry->Link);
  
  return Entry;
}    

STATIC
EFI_STATUS
CoreConvertPages (
  IN UINT64           Start,
  IN UINT64           NumberOfPages,
  IN EFI_MEMORY_TYPE  NewType
  )
/*++

Routine Description:

  Internal function.  Converts a memory range to the specified type.
  The range must exist in the memory map.

Arguments:

  Start         - The first address of the range
                  Must be page aligned

  NumberOfPages - The number of pages to convert

  NewType       - The new type for the memory range

Returns:

  EFI_INVALID_PARAMETER   - Invalid parameter
  
  EFI_NOT_FOUND           - Could not find a descriptor cover the specified range 
                            or convertion not allowed.
  
  EFI_SUCCESS             - Successfully converts the memory range to the specified type.

--*/
{

  UINT64          NumberOfBytes;
  UINT64          End;
  UINT64          RangeEnd;
  UINT64          Attribute;
  EFI_LIST_ENTRY  *Link;
  MEMORY_MAP      *Entry;
  UINT64          NumberOfRangePages;

  Entry = NULL;
  NumberOfBytes = LShiftU64 (NumberOfPages, EFI_PAGE_SHIFT);
  End = Start + NumberOfBytes - 1;

  ASSERT (NumberOfPages);
  ASSERT ((Start & EFI_PAGE_MASK) == 0);
  ASSERT (End > Start) ;
  ASSERT_LOCKED (&gMemoryLock);

  if (NumberOfPages == 0 || (Start & EFI_PAGE_MASK ) || (Start > (Start + NumberOfBytes))) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Convert the entire range
  //

  while (Start < End) {

    //
    // Find the entry that the covers the range
    //
    for (Link = gMemoryMap.ForwardLink; Link != &gMemoryMap; Link = Link->ForwardLink) {
      Entry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);

      if (Entry->Start <= Start && Entry->End > Start) {
        break;
      }
    }

    if ((Link == &gMemoryMap) || (Entry == NULL)) {
      DEBUG ((EFI_D_ERROR | EFI_D_PAGE, "ConvertPages: failed to find range %lx - %lx\n", Start, End));
      return EFI_NOT_FOUND;
    }

    //
    // Convert range to the end, or to the end of the descriptor
    // if that's all we've got
    //
    RangeEnd = End;
    if (Entry->End < End) {
      RangeEnd = Entry->End;
    }

    DEBUG ((EFI_D_ERROR, "ConvertRange: %lx-%lx to %d\n", Start, RangeEnd, (UINTN)NewType));

    //
    // Debug code - verify conversion is allowed
    //
    if (!(NewType == EfiConventionalMemory ? 1 : 0) ^ (Entry->Type == EfiConventionalMemory ? 1 : 0)) {
      return EFI_NOT_FOUND;
    }  

    //
    // Update counters for the number of pages allocated to each memory type
    //
    NumberOfRangePages = RShiftU64 (RangeEnd - Start + 1, EFI_PAGE_SHIFT);
    if (Entry->Type >= 0 && Entry->Type < EfiMaxMemoryType) {
      ASSERT (NumberOfRangePages <= mMemoryTypeStatistics[Entry->Type].CurrentNumberOfPages);
      mMemoryTypeStatistics[Entry->Type].CurrentNumberOfPages -= NumberOfRangePages;
      gMemoryTypeInformation[mMemoryTypeStatistics[Entry->Type].InformationIndex].NumberOfPages = (UINT32)mMemoryTypeStatistics[Entry->Type].CurrentNumberOfPages;
    }

    if (NewType >= 0 && NewType < EfiMaxMemoryType) {
      mMemoryTypeStatistics[NewType].CurrentNumberOfPages += NumberOfRangePages;
      gMemoryTypeInformation[mMemoryTypeStatistics[NewType].InformationIndex].NumberOfPages = (UINT32)mMemoryTypeStatistics[NewType].CurrentNumberOfPages;
    }
    
    //
    // Pull range out of descriptor
    //
    if (Entry->Start == Start) {
      
      //
      // Clip start
      //
      Entry->Start = RangeEnd + 1;

    } else if (Entry->End == RangeEnd) {
      
      //
      // Clip end
      //
      Entry->End = Start - 1;

    } else {

      //
      // Pull it out of the center, clip current
      //
      
      //
      // Add a new one
      //
      mMapStack[mMapDepth].Signature = MEMORY_MAP_SIGNATURE;
      mMapStack[mMapDepth].FromPages  = FALSE;
      mMapStack[mMapDepth].Type      = Entry->Type;
      mMapStack[mMapDepth].Start     = RangeEnd+1;
      mMapStack[mMapDepth].End       = Entry->End;

      //
      // Inherit Attribute from the Memory Descriptor that is being clipped
      //
      mMapStack[mMapDepth].Attribute = Entry->Attribute;

      Entry->End = Start - 1;
      ASSERT (Entry->Start < Entry->End);

      Entry = &mMapStack[mMapDepth];
      InsertTailList (&gMemoryMap, &Entry->Link);

      mMapDepth += 1;
      ASSERT (mMapDepth < MAX_MAP_DEPTH);
    }

    //
    // The new range inherits the same Attribute as the Entry 
    //it is being cut out of
    //
    Attribute = Entry->Attribute;

    //
    // If the descriptor is empty, then remove it from the map
    //
    if (Entry->Start == Entry->End + 1) {
      RemoveMemoryMapEntry (Entry);
      Entry = NULL;
    }
    
    //
    // Add our new range in
    //
    CoreAddRange (NewType, Start, RangeEnd, Attribute);

    //
    // Move any map descriptor stack to general pool
    //
    CoreFreeMemoryMapStack ();

    //
    // Bump the starting address, and convert the next range
    //
    Start = RangeEnd + 1;
  }

  //
  // Converted the whole range, done
  //

  return EFI_SUCCESS;
}


STATIC
UINT64
CoreFindFreePagesI (
  IN UINT64           MaxAddress,
  IN UINT64           NumberOfPages,
  IN EFI_MEMORY_TYPE  NewType,
  IN UINTN            Alignment
  )
/*++

Routine Description:

  Internal function. Finds a consecutive free page range below
  the requested address.

Arguments:

  MaxAddress    - The address that the range must be below

  NumberOfPages - Number of pages needed

  NewType       - The type of memory the range is going to be turned into

  Alignment     - Bits to align with

Returns:

  The base address of the range, or 0 if the range was not found

--*/
{
  UINT64          NumberOfBytes;
  UINT64          Target;
  UINT64          DescStart;
  UINT64          DescEnd;
  UINT64          DescNumberOfBytes;
  EFI_LIST_ENTRY  *Link;
  MEMORY_MAP      *Entry;

  if ((MaxAddress < EFI_PAGE_MASK) ||(NumberOfPages == 0)) {
    return 0;
  }

  if ((MaxAddress & EFI_PAGE_MASK) != EFI_PAGE_MASK) {
    
    //
    // If MaxAddress is not aligned to the end of a page
    //
    
    //
    // Change MaxAddress to be 1 page lower
    //
    MaxAddress -= (EFI_PAGE_MASK + 1);
    
    //
    // Set MaxAddress to a page boundary
    //
    MaxAddress &= ~EFI_PAGE_MASK;
    
    //
    // Set MaxAddress to end of the page
    //
    MaxAddress |= EFI_PAGE_MASK;
  }

  NumberOfBytes = LShiftU64 (NumberOfPages, EFI_PAGE_SHIFT);
  Target = 0;

  for (Link = gMemoryMap.ForwardLink; Link != &gMemoryMap; Link = Link->ForwardLink) {
    Entry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);
  
    //
    // If it's not a free entry, don't bother with it
    //
    if (Entry->Type != EfiConventionalMemory) {
      continue;
    }

    DescStart = Entry->Start;
    DescEnd = Entry->End;

    //
    // If desc is past max allowed address, skip it
    //
    if (DescStart >= MaxAddress) {
      continue;
    }

    //
    // If desc ends past max allowed address, clip the end
    //
    if (DescEnd >= MaxAddress) {
      DescEnd = MaxAddress;
    }

    DescEnd = ((DescEnd + 1) & (~(Alignment - 1))) - 1;

    //
    // Compute the number of bytes we can used from this 
    // descriptor, and see it's enough to satisfy the request
    //
    DescNumberOfBytes = DescEnd - DescStart + 1;

    if (DescNumberOfBytes >= NumberOfBytes) {

      //
      // If this is the best match so far remember it
      //
      if (DescEnd > Target) {
        Target = DescEnd;
      }
    }
  }          

  //
  // If this is a grow down, adjust target to be the allocation base
  //
  Target -= NumberOfBytes - 1;

  //
  // If we didn't find a match, return 0
  //
  if ((Target & EFI_PAGE_MASK) != 0) {
    return 0;
  }

  return Target;
}

STATIC
UINT64
FindFreePages (
    IN UINT64           MaxAddress,
    IN UINT64           NoPages,
    IN EFI_MEMORY_TYPE  NewType,
    IN UINTN            Alignment
    )
/*++

Routine Description:

    Internal function.  Finds a consecutive free page range below
    the requested address

Arguments:

    MaxAddress          - The address that the range must be below

    NoPages             - Number of pages needed

    NewType             - The type of memory the range is going to be turned into

    Alignment           - Bits to align with

Returns:

    The base address of the range, or 0 if the range was not found.

--*/
{
  UINT64  NewMaxAddress;
  UINT64  Start;

  NewMaxAddress = MaxAddress;

  if (NewType >= 0 && NewType < EfiMaxMemoryType && NewMaxAddress >= mMemoryTypeStatistics[NewType].MaximumAddress) {
    NewMaxAddress  = mMemoryTypeStatistics[NewType].MaximumAddress;
  } else {
    if (NewMaxAddress > mDefaultMaximumAddress) {
      NewMaxAddress  = mDefaultMaximumAddress;
    }
  }

  Start = CoreFindFreePagesI (NewMaxAddress, NoPages, NewType, Alignment);
  if (!Start) {
    Start = CoreFindFreePagesI (MaxAddress, NoPages, NewType, Alignment);
    if (!Start) {
      //
      // Here means there may be no enough memory to use, so try to go through
      // all the memory descript to promote the untested memory directly
      //
      PromoteMemoryResource ();

      //
      // Allocate memory again after the memory resource re-arranged
      //
      Start = CoreFindFreePagesI (MaxAddress, NoPages, NewType, Alignment);
    }
  }

  return Start;
}

EFI_BOOTSERVICE
EFI_STATUS
EFIAPI
CoreAllocatePages (
  IN EFI_ALLOCATE_TYPE      Type,
  IN EFI_MEMORY_TYPE        MemoryType,
  IN UINTN                  NumberOfPages,
  IN OUT EFI_PHYSICAL_ADDRESS  *Memory
  )
/*++

Routine Description:

  Allocates pages from the memory map.

Arguments:

  Type          - The type of allocation to perform

  MemoryType    - The type of memory to turn the allocated pages into

  NumberOfPages - The number of pages to allocate

  Memory        - A pointer to receive the base allocated memory address

Returns:

  Status. On success, Memory is filled in with the base address allocated

  EFI_INVALID_PARAMETER     - Parameters violate checking rules defined in spec.
  
  EFI_NOT_FOUND             - Could not allocate pages match the requirement.
  
  EFI_OUT_OF_RESOURCES      - No enough pages to allocate.
  
  EFI_SUCCESS               - Pages successfully allocated.

--*/
{
  EFI_STATUS      Status;
  UINT64          Start;
  UINT64          MaxAddress;
  UINTN           Alignment;

  if (Type < AllocateAnyPages || Type >= (UINTN) MaxAllocateType) {
    return EFI_INVALID_PARAMETER;
  }

  if ((MemoryType >= EfiMaxMemoryType && MemoryType <= 0x7fffffff) ||
       MemoryType == EfiConventionalMemory) {
    return EFI_INVALID_PARAMETER;
  }

  Alignment = EFI_DEFAULT_PAGE_ALLOCATION_ALIGNMENT;

  if  (MemoryType == EfiACPIReclaimMemory   ||
       MemoryType == EfiACPIMemoryNVS       ||
       MemoryType == EfiRuntimeServicesCode ||
       MemoryType == EfiRuntimeServicesData) {

    Alignment = EFI_ACPI_RUNTIME_PAGE_ALLOCATION_ALIGNMENT;
  }

  if (Type == AllocateAddress) {
    if ((*Memory & (Alignment - 1)) != 0) {
      return EFI_NOT_FOUND;
    }
  }

  NumberOfPages += EFI_SIZE_TO_PAGES (Alignment) - 1;
  NumberOfPages &= ~(EFI_SIZE_TO_PAGES (Alignment) - 1);

  //
  // If this is for below a particular address, then 
  //
  Start = *Memory;
  
  //
  // The max address is the max natively addressable address for the processor
  //
  MaxAddress = EFI_MAX_ADDRESS;
  
  if (Type == AllocateMaxAddress) {
    MaxAddress = Start;
  }

  CoreAcquireMemoryLock ();
  
  //
  // If not a specific address, then find an address to allocate
  //
  if (Type != AllocateAddress) {
    Start = FindFreePages (MaxAddress, NumberOfPages, MemoryType, Alignment);
    if (Start == 0) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }
  }

  //
  // Convert pages from FreeMemory to the requested type
  //
  Status = CoreConvertPages (Start, NumberOfPages, MemoryType);

Done:
  CoreReleaseMemoryLock ();

  if (!EFI_ERROR (Status)) {
    *Memory = Start;
  }

  return Status;
}



EFI_BOOTSERVICE
EFI_STATUS 
EFIAPI
CoreFreePages (
  IN EFI_PHYSICAL_ADDRESS   Memory,
  IN UINTN                  NumberOfPages
  )
/*++

Routine Description:

  Frees previous allocated pages.

Arguments:

  Memory        - Base address of memory being freed

  NumberOfPages - The number of pages to free

Returns:

  EFI_NOT_FOUND       - Could not find the entry that covers the range
  
  EFI_INVALID_PARAMETER   - Address not aligned
  
  EFI_SUCCESS         -Pages successfully freed.

--*/
{
  EFI_STATUS      Status;
  EFI_LIST_ENTRY  *Link;
  MEMORY_MAP      *Entry;
  UINTN           Alignment;

  //
  // Free the range
  //
  CoreAcquireMemoryLock ();

  //
  // Find the entry that the covers the range
  //
  Entry = NULL;
  for (Link = gMemoryMap.ForwardLink; Link != &gMemoryMap; Link = Link->ForwardLink) {
    Entry = CR(Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);
    if (Entry->Start <= Memory && Entry->End > Memory) {
        break;
    }
  }
  if ((Link == &gMemoryMap) || (Entry == NULL)) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  Alignment = EFI_DEFAULT_PAGE_ALLOCATION_ALIGNMENT;

  if  (Entry->Type == EfiACPIReclaimMemory   ||
       Entry->Type == EfiACPIMemoryNVS       ||
       Entry->Type == EfiRuntimeServicesCode ||
       Entry->Type == EfiRuntimeServicesData) {

    Alignment = EFI_ACPI_RUNTIME_PAGE_ALLOCATION_ALIGNMENT;

  }

  if ((Memory & (Alignment - 1)) != 0) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  NumberOfPages += EFI_SIZE_TO_PAGES (Alignment) - 1;
  NumberOfPages &= ~(EFI_SIZE_TO_PAGES (Alignment) - 1);

  Status = CoreConvertPages (Memory, NumberOfPages, EfiConventionalMemory);

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Destroy the contents
  //
  if (Memory < EFI_MAX_ADDRESS) {
    DEBUG_SET_MEMORY ((VOID *)(UINTN)Memory, NumberOfPages << EFI_PAGE_SHIFT);
  }

 Done:
  CoreReleaseMemoryLock ();
  
  return Status;
}


EFI_BOOTSERVICE
EFI_STATUS
EFIAPI
CoreGetMemoryMap (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  OUT UINTN                     *MapKey,
  OUT UINTN                     *DescriptorSize,
  OUT UINT32                    *DescriptorVersion
  )
/*++

Routine Description:

  This function returns a copy of the current memory map. The map is an array of 
  memory descriptors, each of which describes a contiguous block of memory.

Arguments:

  MemoryMapSize     - A pointer to the size, in bytes, of the MemoryMap buffer. On
                      input, this is the size of the buffer allocated by the caller. 
                      On output, it is the size of the buffer returned by the firmware 
                      if the buffer was large enough, or the size of the buffer needed 
                      to contain the map if the buffer was too small.
  MemoryMap         - A pointer to the buffer in which firmware places the current memory map.
  MapKey            - A pointer to the location in which firmware returns the key for the
                      current memory map.
  DescriptorSize    - A pointer to the location in which firmware returns the size, in
                      bytes, of an individual EFI_MEMORY_DESCRIPTOR.
  DescriptorVersion - A pointer to the location in which firmware returns the version
                      number associated with the EFI_MEMORY_DESCRIPTOR.

Returns:

  EFI_SUCCESS           - The memory map was returned in the MemoryMap buffer.       
  EFI_BUFFER_TOO_SMALL  - The MemoryMap buffer was too small. The current buffer size
                          needed to hold the memory map is returned in MemoryMapSize.
  EFI_INVALID_PARAMETER - One of the parameters has an invalid value.                

--*/
{
  EFI_STATUS                        Status;
  UINTN                             Size;  
  UINTN                             BufferSize;  
  UINTN                             NumberOfRuntimeEntries;
  EFI_LIST_ENTRY                    *Link;
  MEMORY_MAP                        *Entry;  
  EFI_GCD_MAP_ENTRY                 *GcdMapEntry;  
  EFI_MEMORY_TYPE                   Type;

  //
  // Make sure the parameters are valid
  //
  if (MemoryMapSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  CoreAcquireGcdMemoryLock ();
  
  //
  // Count the number of Reserved and MMIO entries that are marked for runtime use
  //
  NumberOfRuntimeEntries = 0;
  for (Link = mGcdMemorySpaceMap.ForwardLink; Link != &mGcdMemorySpaceMap; Link = Link->ForwardLink) {
    GcdMapEntry = CR (Link, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
    if ((GcdMapEntry->GcdMemoryType == EfiGcdMemoryTypeReserved) ||
        ((GcdMapEntry->GcdMemoryType == EfiGcdMemoryTypeMemoryMappedIo) && ((GcdMapEntry->Attributes & EFI_MEMORY_RUNTIME) == EFI_MEMORY_RUNTIME))) {
      NumberOfRuntimeEntries++;
    }
  }

  Size = sizeof (EFI_MEMORY_DESCRIPTOR);

  //
  // Make sure Size != sizeof(EFI_MEMORY_DESCRIPTOR). This will
  // prevent people from having pointer math bugs in their code.
  // now you have to use *DescriptorSize to make things work.
  //
  Size += sizeof(UINT64) - (Size % sizeof (UINT64));

  if (DescriptorSize != NULL) {
    *DescriptorSize = Size;
  }
  
  if (DescriptorVersion != NULL) {
    *DescriptorVersion = EFI_MEMORY_DESCRIPTOR_VERSION;
  }

  CoreAcquireMemoryLock ();

  //
  // Compute the buffer size needed to fit the entire map
  //
  BufferSize = Size * NumberOfRuntimeEntries;
  for (Link = gMemoryMap.ForwardLink; Link != &gMemoryMap; Link = Link->ForwardLink) {
    BufferSize += Size;
  }

  if (*MemoryMapSize < BufferSize) {
    Status = EFI_BUFFER_TOO_SMALL;
    goto Done;
  }

  if (MemoryMap == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  //
  // Build the map
  //
  EfiCommonLibZeroMem (MemoryMap, BufferSize);
  for (Link = gMemoryMap.ForwardLink; Link != &gMemoryMap; Link = Link->ForwardLink) {
    Entry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);
    ASSERT (Entry->VirtualStart == 0);

    //
    // Convert internal map into an EFI_MEMORY_DESCRIPTOR
    //
    MemoryMap->Type          = Entry->Type;
    MemoryMap->PhysicalStart = Entry->Start;
    MemoryMap->VirtualStart  = Entry->VirtualStart;
    MemoryMap->NumberOfPages = RShiftU64 (Entry->End - Entry->Start + 1, EFI_PAGE_SHIFT);
    //
    // If the memory type is EfiConventionalMemory, then determine if the range is part of a
    // memory type bin and needs to be converted to the same memory type as the rest of the 
    // memory type bin in order to minimize EFI Memory Map changes across reboots.  This 
    // improves the chances for a successful S4 resume in the presence of minor page allocation
    // differences across reboots.
    //
    if (MemoryMap->Type == EfiConventionalMemory) {
      for (Type = (EFI_MEMORY_TYPE) 0; Type < EfiMaxMemoryType; Type++) {
        if (mMemoryTypeStatistics[Type].Special                        &&
            mMemoryTypeStatistics[Type].NumberOfPages > 0              &&
            Entry->Start >= mMemoryTypeStatistics[Type].BaseAddress    &&
            Entry->End   <= mMemoryTypeStatistics[Type].MaximumAddress    ) {
          MemoryMap->Type = Type;
        }
      }
    }
    MemoryMap->Attribute = Entry->Attribute;
    if (mMemoryTypeStatistics[MemoryMap->Type].Runtime) {
      MemoryMap->Attribute |= EFI_MEMORY_RUNTIME;
    }
    
    MemoryMap = NextMemoryDescriptor (MemoryMap, Size);
  }

  for (Link = mGcdMemorySpaceMap.ForwardLink; Link != &mGcdMemorySpaceMap; Link = Link->ForwardLink) {
    GcdMapEntry = CR (Link, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
    if ((GcdMapEntry->GcdMemoryType == EfiGcdMemoryTypeReserved) ||
        ((GcdMapEntry->GcdMemoryType == EfiGcdMemoryTypeMemoryMappedIo) && ((GcdMapEntry->Attributes & EFI_MEMORY_RUNTIME) == EFI_MEMORY_RUNTIME))) {
        
        MemoryMap->PhysicalStart = GcdMapEntry->BaseAddress;
        MemoryMap->VirtualStart  = 0;
        MemoryMap->NumberOfPages = RShiftU64 ((GcdMapEntry->EndAddress - GcdMapEntry->BaseAddress + 1), EFI_PAGE_SHIFT);
        MemoryMap->Attribute     = GcdMapEntry->Attributes & ~EFI_MEMORY_PORT_IO;

        if (GcdMapEntry->GcdMemoryType == EfiGcdMemoryTypeReserved) {
          MemoryMap->Type = EfiReservedMemoryType;
        } else if (GcdMapEntry->GcdMemoryType == EfiGcdMemoryTypeMemoryMappedIo) {
          if ((GcdMapEntry->Attributes & EFI_MEMORY_PORT_IO) == EFI_MEMORY_PORT_IO) {
            MemoryMap->Type = EfiMemoryMappedIOPortSpace;
          } else {
            MemoryMap->Type = EfiMemoryMappedIO;
          }
        }

        MemoryMap = NextMemoryDescriptor (MemoryMap, Size);
    }
  }
  
  Status = EFI_SUCCESS;

Done:

  CoreReleaseMemoryLock ();
  
  CoreReleaseGcdMemoryLock ();
  
  // 
  // Update the map key finally 
  // 
  if (MapKey != NULL) {
    *MapKey = mMemoryMapKey;
  }
  
  *MemoryMapSize = BufferSize;
  
  return Status;
}

VOID *
CoreAllocatePoolPages (
  IN EFI_MEMORY_TYPE    PoolType,
  IN UINTN              NumberOfPages,
  IN UINTN              Alignment
  )
/*++

Routine Description:

  Internal function.  Used by the pool functions to allocate pages
  to back pool allocation requests.

Arguments:

  PoolType      - The type of memory for the new pool pages

  NumberOfPages - No of pages to allocate

  Alignment     - Bits to align.

Returns:

  The allocated memory, or NULL

--*/
{
  EFI_STATUS        Status;
  UINT64            Start;

  //
  // Find the pages to convert
  //
  Start = FindFreePages (EFI_MAX_ADDRESS, NumberOfPages, PoolType, Alignment);

  //
  // Convert it to boot services data
  //
  if (Start == 0) {
    DEBUG ((EFI_D_ERROR | EFI_D_PAGE, "AllocatePoolPages: failed to allocate %d pages\n", NumberOfPages));
  } else {
    Status = CoreConvertPages (Start, NumberOfPages, PoolType);
  }

  return (VOID *)(UINTN)Start;
}

VOID
CoreFreePoolPages (
  IN EFI_PHYSICAL_ADDRESS   Memory,
  IN UINTN                  NumberOfPages
  )
/*++

Routine Description:

  Internal function.  Frees pool pages allocated via AllocatePoolPages ()

Arguments:

  Memory        - The base address to free

  NumberOfPages - The number of pages to free

Returns:

  None

--*/
{
  CoreConvertPages (Memory, NumberOfPages, EfiConventionalMemory);
}


EFI_STATUS
CoreTerminateMemoryMap (
  IN UINTN          MapKey
  )
/*++

Routine Description:

  Make sure the memory map is following all the construction rules, 
  it is the last time to check memory map error before exit boot services.

Arguments:

  MapKey        - Memory map key

Returns:

  EFI_INVALID_PARAMETER       - Memory map not consistent with construction rules.
  
  EFI_SUCCESS                 - Valid memory map.

--*/
{
  EFI_STATUS        Status;
  EFI_LIST_ENTRY    *Link;
  MEMORY_MAP        *Entry;

  Status = EFI_SUCCESS;

  CoreAcquireMemoryLock ();

  if (MapKey == mMemoryMapKey) {

    //
    // Make sure the memory map is following all the construction rules
    // This is the last chance we will be able to display any messages on
    // the  console devices.
    //

    for (Link = gMemoryMap.ForwardLink; Link != &gMemoryMap; Link = Link->ForwardLink) {
      Entry = CR(Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);
      if (Entry->Attribute & EFI_MEMORY_RUNTIME) { 
        if (Entry->Type == EfiACPIReclaimMemory || Entry->Type == EfiACPIMemoryNVS) {
          DEBUG((EFI_D_ERROR, "ExitBootServices: ACPI memory entry has RUNTIME attribute set.\n"));
          CoreReleaseMemoryLock ();
          return EFI_INVALID_PARAMETER;
        }
        if (Entry->Start & (EFI_ACPI_RUNTIME_PAGE_ALLOCATION_ALIGNMENT - 1)) {
          DEBUG((EFI_D_ERROR, "ExitBootServices: A RUNTIME memory entry is not on a proper alignment.\n"));
          CoreReleaseMemoryLock ();
          return EFI_INVALID_PARAMETER;
        }
        if ((Entry->End + 1) & (EFI_ACPI_RUNTIME_PAGE_ALLOCATION_ALIGNMENT - 1)) {
          DEBUG((EFI_D_ERROR, "ExitBootServices: A RUNTIME memory entry is not on a proper alignment.\n"));
          CoreReleaseMemoryLock ();
          return EFI_INVALID_PARAMETER;
        }
      }
    }

    //
    // The map key they gave us matches what we expect. Fall through and
    // return success. In an ideal world we would clear out all of
    // EfiBootServicesCode and EfiBootServicesData. However this function
    // is not the last one called by ExitBootServices(), so we have to
    // preserve the memory contents.
    //
  } else {
    Status = EFI_INVALID_PARAMETER;
  }

  CoreReleaseMemoryLock ();

  return Status;
}








