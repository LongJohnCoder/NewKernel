/*++

Copyright (c) 2004 - 2007, Intel Corporation                                                  
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  InstallConfigurationTable.c


Abstract:

  Tiano Miscellaneous Services InstallConfigurationTable service

--*/

#include "Tiano.h"
#include "DxeCore.h"

#define CONFIG_TABLE_SIZE_INCREASED 0x10

UINTN mSystemTableAllocateSize = 0;


EFI_STATUS
CoreGetConfigTable (
  IN EFI_GUID *Guid,
  OUT VOID    **Table
  )
/*++

Routine Description:

  Find a config table by name in system table's ConfigurationTable.

Arguments:

  Guid        - The table name to look for
  
  Table       - Pointer of the config table

Returns: 

  EFI_NOT_FOUND       - Could not find the table in system table's ConfigurationTable.
  
  EFI_SUCCESS         - Table successfully found.

--*/
{
  UINTN Index;

  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (EfiCompareGuid (Guid, &(gST->ConfigurationTable[Index].VendorGuid))) {
      *Table = gST->ConfigurationTable[Index].VendorTable;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


EFI_BOOTSERVICE
EFI_STATUS
EFIAPI
CoreInstallConfigurationTable (
  IN EFI_GUID *Guid,
  IN VOID     *Table
  )
/*++

Routine Description:

  Boot Service called to add, modify, or remove a system configuration table from 
  the EFI System Table.

Arguments:

  Guid     -  Pointer to the GUID for the entry to add, update, or remove
  Table    -  Pointer to the configuration table for the entry to add, update, or
              remove, may be NULL.

Returns:
  
  EFI_SUCCESS               Guid, Table pair added, updated, or removed.
  EFI_INVALID_PARAMETER     Input GUID not valid.
  EFI_NOT_FOUND             Attempted to delete non-existant entry
  EFI_OUT_OF_RESOURCES      Not enough memory available

--*/
{
  UINTN                   Index;
  EFI_CONFIGURATION_TABLE *EfiConfigurationTable;

  //
  // If Guid is NULL, then this operation cannot be performed
  //
  if (Guid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EfiConfigurationTable = gST->ConfigurationTable;

  //
  // Search all the table for an entry that matches Guid
  //
  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (EfiCompareGuid (Guid, &(gST->ConfigurationTable[Index].VendorGuid))) {
      break;
    }
  }

  if (Index < gST->NumberOfTableEntries) {
    //
    // A match was found, so this is either a modify or a delete operation
    //
    if (Table != NULL) {
      //
      // If Table is not NULL, then this is a modify operation.
      // Modify the table enty and return.
      //
      gST->ConfigurationTable[Index].VendorTable = Table;

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
      //
      // Signal Configuration Table change
      //
      CoreNotifySignalList (Guid);
#endif

      return EFI_SUCCESS;
    }

    //
    // A match was found and Table is NULL, so this is a delete operation.
    //
    gST->NumberOfTableEntries--;

    //
    // Copy over deleted entry
    //
    EfiCommonLibCopyMem (
      &(EfiConfigurationTable[Index]),
      &(gST->ConfigurationTable[Index + 1]),
      (gST->NumberOfTableEntries - Index) * sizeof (EFI_CONFIGURATION_TABLE)
      );

  } else {

    //
    // No matching GUIDs were found, so this is an add operation.
    //

    if (Table == NULL) {
      //
      // If Table is NULL on an add operation, then return an error.
      //
      return EFI_NOT_FOUND;
    }

    //
    // Assume that Index == gST->NumberOfTableEntries
    //
    if ((Index * sizeof (EFI_CONFIGURATION_TABLE)) >= mSystemTableAllocateSize) {
      //
      // Allocate a table with one additional entry.
      //
      mSystemTableAllocateSize += (CONFIG_TABLE_SIZE_INCREASED * sizeof (EFI_CONFIGURATION_TABLE));
      EfiConfigurationTable = CoreAllocateRuntimePool (mSystemTableAllocateSize);
      if (EfiConfigurationTable == NULL) {
        //
        // If a new table could not be allocated, then return an error.
        //
        return EFI_OUT_OF_RESOURCES;
      }

      if (gST->ConfigurationTable != NULL) {
        //
        // Copy the old table to the new table.
        //
        EfiCommonLibCopyMem (
          EfiConfigurationTable,
          gST->ConfigurationTable,
          Index * sizeof (EFI_CONFIGURATION_TABLE)
          );

        //
        // Free Old Table
        //
        CoreFreePool (gST->ConfigurationTable);
      }

      //
      // Update System Table
      //
      gST->ConfigurationTable = EfiConfigurationTable;
    }

    //
    // Fill in the new entry
    //
    EfiConfigurationTable[Index].VendorGuid   = *Guid;
    EfiConfigurationTable[Index].VendorTable  = Table;

    //
    // This is an add operation, so increment the number of table entries
    //
    gST->NumberOfTableEntries++;
  }

  //
  // Fix up the CRC-32 in the EFI System Table
  //
  CalculateEfiHdrCrc (&gST->Hdr);

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  //
  // Signal Configuration Table change
  //
  CoreNotifySignalList (Guid);
#endif

  return EFI_SUCCESS;
}
