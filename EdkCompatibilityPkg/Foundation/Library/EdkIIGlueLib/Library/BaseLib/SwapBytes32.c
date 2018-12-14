/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  SwapBytes32.c
  
Abstract: 

  Math worker functions.

--*/

#include "BaseLibInternal.h"

/**
  Switches the endianess of a 32-bit integer.

  This function swaps the bytes in a 32-bit unsigned value to switch the value
  from little endian to big endian or vice versa. The byte swapped value is
  returned.

  @param  Operand A 32-bit unsigned value.

  @return The byte swaped Operand.

**/
UINT32
EFIAPI
SwapBytes32 (
  IN      UINT32                    Operand
  )
{
  return (UINT32)(
           ((UINT32)SwapBytes16 ((UINT16)Operand) << 16) |
           ((UINT32)SwapBytes16 ((UINT16)(Operand >> 16)))
           );
}
