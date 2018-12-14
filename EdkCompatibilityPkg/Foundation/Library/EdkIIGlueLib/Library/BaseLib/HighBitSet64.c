/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  HighBitSet64.c
  
Abstract: 

  Math worker functions.

--*/

#include "BaseLibInternal.h"

/**
  Returns the bit position of the highest bit set in a 64-bit value. Equivalent
  to log2(x).

  This function computes the bit position of the highest bit set in the 64-bit
  value specified by Operand. If Operand is zero, then -1 is returned.
  Otherwise, a value between 0 and 63 is returned.

  @param  Operand The 64-bit operand to evaluate.

  @return Position of the highest bit set in Operand if found.
  @retval -1  Operand is zero.

**/
INTN
EFIAPI
HighBitSet64 (
  IN      UINT64                    Operand
  )
{
  INTN                              BitIndex;

  for (BitIndex = -1;
       Operand != 0;
       BitIndex++, Operand = RShiftU64 (Operand, 1));
  return BitIndex;
}
