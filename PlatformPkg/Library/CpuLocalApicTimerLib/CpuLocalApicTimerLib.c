/** @file
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2017 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/ZhaoXinCpuLib.h>
#include <Library/LocalApicLib.h>

//
// The following array is FSB frequencies defined in Pentinum 4 family
// and Xeon family CPUs, its value unit is HZ.
//
GLOBAL_REMOVE_IF_UNREFERENCED
CONST UINT32                          ZhaoXinFSBFrequencies[] = {
  266666667,
  133333333,
  200000000,
  166666667,
  333333333,
  100000000,
  400000000,
};


/**
  The function to get CPU intended FSB frequency.
  @retval CPU intended FSB frequency.

**/
UINT32
GetIntendFsbFrequency (
  VOID
  )
{
  UINT32              FrequencyIndex;
  FrequencyIndex = (UINT32) BitFieldRead64 (AsmReadMsr64 (EFI_MSR_PSB_CLOCK_STATUS), 0, 2);
  return ZhaoXinFSBFrequencies[FrequencyIndex];
}


/**
  Internal function to return the frequency of the local APIC timer.
  @return The frequency of the timer in Hz.
**/
UINT32
InternalX86GetTimerFrequency (
  VOID
  )
{
  UINT32 Freq;
  UINTN  DivideValue;

  Freq = GetIntendFsbFrequency ();
  GetApicTimerState (&DivideValue, NULL, NULL);
  return Freq / ((UINT32) DivideValue);
}

/**
  Internal function to read the current tick counter of local APIC.

  Internal function to read the current tick counter of local APIC.

  @return The tick counter read.

**/
INT32
InternalX86GetTimerTick (
  VOID
  )
{
  return GetApicTimerCurrentCount ();
}

/**
  Stalls the CPU for at least the given number of ticks.

  Stalls the CPU for at least the given number of ticks. It's invoked by
  MicroSecondDelay() and NanoSecondDelay().

  @param  Delay     A period of time to delay in ticks.

**/
VOID
InternalX86Delay (
  IN      UINT32                    Delay
  )
{
  INT32                             Ticks;

  //
  // The target timer count is calculated here
  //
  Ticks = InternalX86GetTimerTick () - Delay;

  //
  // Wait until time out
  // Delay > 2^31 could not be handled by this function
  // Timer wrap-arounds are handled correctly by this function
  //
  while (InternalX86GetTimerTick () - Ticks >= 0);
}

/**
  Stalls the CPU for at least the given number of microseconds.

  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return MicroSeconds

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN                     MicroSeconds
  )
{
  InternalX86Delay (
    (UINT32)DivU64x32 (
              MultU64x64 (
                InternalX86GetTimerFrequency (),
                MicroSeconds
                ),
              1000000u
              )
    );
  return MicroSeconds;
}

/**
  Stalls the CPU for at least the given number of nanoseconds.

  Stalls the CPU for the number of nanoseconds specified by NanoSeconds.

  @param  NanoSeconds The minimum number of nanoseconds to delay.

  @return NanoSeconds

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN      UINTN                     NanoSeconds
  )
{
  InternalX86Delay (
    (UINT32)DivU64x32 (
              MultU64x64 (
                InternalX86GetTimerFrequency (),
                NanoSeconds
                ),
              1000000000u
              )
    );
  return NanoSeconds;
}

/**
  Retrieves the current value of a 64-bit free running performance counter.

  Retrieves the current value of a 64-bit free running performance counter. The
  counter can either count up by 1 or count down by 1. If the physical
  performance counter counts by a larger increment, then the counter values
  must be translated. The properties of the counter can be retrieved from
  GetPerformanceCounterProperties().

  @return The current value of the free running performance counter.

**/
UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  return (UINT64)(UINT32)InternalX86GetTimerTick ();
}

/**
  Retrieves the 64-bit frequency in Hz and the range of performance counter
  values.

  If StartValue is not NULL, then the value that the performance counter starts
  with immediately after is it rolls over is returned in StartValue. If
  EndValue is not NULL, then the value that the performance counter end with
  immediately before it rolls over is returned in EndValue. The 64-bit
  frequency of the performance counter in Hz is always returned. If StartValue
  is less than EndValue, then the performance counter counts up. If StartValue
  is greater than EndValue, then the performance counter counts down. For
  example, a 64-bit free running counter that counts up would have a StartValue
  of 0 and an EndValue of 0xFFFFFFFFFFFFFFFF. A 24-bit free running counter
  that counts down would have a StartValue of 0xFFFFFF and an EndValue of 0.

  @param  StartValue  The value the performance counter starts with when it
                      rolls over.
  @param  EndValue    The value that the performance counter ends with before
                      it rolls over.

  @return The frequency in Hz.

**/
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT      UINT64                    *StartValue,  OPTIONAL
  OUT      UINT64                    *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = GetApicTimerInitCount ();
  }

  if (EndValue != NULL) {
    *EndValue = 0;
  }

  return (UINT64) InternalX86GetTimerFrequency ();
}

/**
  Converts elapsed ticks of performance counter to time in nanoseconds.

  This function converts the elapsed ticks of running performance counter to
  time value in unit of nanoseconds.

  @param  Ticks     The number of elapsed ticks of running performance counter.

  @return The elapsed time in nanoseconds.

**/
UINT64
EFIAPI
GetTimeInNanoSecond (
  IN      UINT64                     Ticks
  )
{
  UINT64  Frequency;
  UINT64  NanoSeconds;
  UINT64  Remainder;
  INTN    Shift;

  Frequency = GetPerformanceCounterProperties (NULL, NULL);

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x64Remainder (Ticks, Frequency, &Remainder), 1000000000u);

  //
  // Ensure (Remainder * 1,000,000,000) will not overflow 64-bit.
  // Since 2^29 < 1,000,000,000 = 0x3B9ACA00 < 2^30, Remainder should < 2^(64-30) = 2^34,
  // i.e. highest bit set in Remainder should <= 33.
  //
  Shift = MAX (0, HighBitSet64 (Remainder) - 33);
  Remainder = RShiftU64 (Remainder, (UINTN) Shift);
  Frequency = RShiftU64 (Frequency, (UINTN) Shift);
  NanoSeconds += DivU64x64Remainder (MultU64x32 (Remainder, 1000000000u), Frequency, NULL);

  return NanoSeconds;
}
