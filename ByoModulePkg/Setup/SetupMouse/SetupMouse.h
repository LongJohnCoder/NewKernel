/** @file
Setup mouse driver.
Copyright (c) 2017, Byosoft Software Corporation. All Rights Reserved.

You may not reproduce, distribute, publish, display, perform, modify, adapt,
transmit, broadcast, present, recite, release, license or otherwise exploit
any part of this publication in any form, by any means, without the prior
written permission of Byosoft Software Corporation.

File Name:
  SetupMouse.h

Abstract:
  Setup Mouse Protocol implementation

Revision History:

**/

#ifndef _SETUP_MOUSE_H
#define _SETUP_MOUSE_H

#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>

#include <Protocol/SimpleTextOut.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SetupMouseProtocol.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/ByoFormSetManager.h>

#define MOUSE_RESOLUTION    0x2800
#define MOUSE_TIMER    (10 * 10000)  
#define DEFAULT_CURSOR_WIDTH	16
#define DEFAULT_CURSOR_HEIGHT	16

typedef struct {
  INT32    left;
  INT32    top;
  INT32    right;
  INT32    bottom;
} RECT;

typedef struct {
  INT32    x;
  INT32    y;
} POINT;

typedef struct {
  BOOLEAN    Visible;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Image;
  RECT    ImageRc;
} IMAGE_INFO;

typedef struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *Gop;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT    OrgBlt;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT    OriginalBlt;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *BlendBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *FillLine;
  IMAGE_INFO    Screen;
  RECT    InvalidateRc;
} GOP_ENTRY;

typedef struct {
  EFI_EVENT    MouseEvent;	
  EFI_SETUP_MOUSE_PROTOCOL    SetupMouse;
  EFI_SIMPLE_POINTER_PROTOCOL    *PointerProtocol;
  GOP_ENTRY    *GopList;
  UINTN    GopCount;
  BOOLEAN    IsStart;
  BOOLEAN    HaveData;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *CheckBuffer;
  MOUSE_CURSOR_RANGE    MouseRange;
  IMAGE_INFO    Cursor;
  BOOLEAN    LButton;
  BOOLEAN    LDoubleButton;
  BOOLEAN    RButton;
  UINTN    SaveCursorX;
  UINTN    SaveCursorY;
  BOOLEAN    BeSetSate;
} PRIVATE_MOUSE_DATA;

extern PRIVATE_MOUSE_DATA    *mPrivate;

//
// Setup Mouse services
//
EFI_STATUS
SetupMouseStart (
  VOID
  );

EFI_STATUS
SetupMouseClose (
  VOID
  );

EFI_STATUS
QueryState (
  IN  OUT MOUSE_STATE    *State
  );

EFI_STATUS
SetMouseState (
  IN MOUSE_STATE    *State
  );

EFI_STATUS
ProcessMouse (
  IN  EFI_EVENT    Event,
  IN  VOID    *Context
  );

EFI_STATUS
SetupMouseHookScreenBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL    *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *BltBuffer, OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION    BltOperation,
  IN  UINTN    SourceX,
  IN  UINTN    SourceY,
  IN  UINTN    DestinationX,
  IN  UINTN    DestinationY,
  IN  UINTN    Width,
  IN  UINTN    Height,
  IN  UINTN    Delta    OPTIONAL
  );

BOOLEAN
IsRectEmpty(
  CONST RECT    *lprc
  );

VOID
SetRectEmpty(
  RECT    *lprc
  );

BOOLEAN
IntersectRect(
  RECT    *dest,
  const RECT    *src1,
  const RECT    *src2
  );

VOID
SetRect(
  RECT    *lprc,
  int    xLeft,
  int    yTop,
  int    xRight,
  int    yBottom
  );

BOOLEAN
OffsetRect(
  RECT    *lprc,
  INT32    dx,
  INT32    dy
  );

EFI_STATUS
InitializeCursor (
  VOID
  );

BOOLEAN
PtInRect (
  IN RECT    *lprc,
  IN POINT    pt
  );

BOOLEAN
UnionRect(
  OUT RECT    *dest,
  IN  CONST RECT    *src1,
  IN  CONST RECT    *src2
  );

EFI_STATUS
RenderImage (
  IN GOP_ENTRY    *GopEntry,
  IN BOOLEAN    NeedSyncScreen
  );

VOID
HideImage (
  IN  IMAGE_INFO    *ImageInfo
  );

VOID
MoveImage (
  IN  IMAGE_INFO    *ImageInfo,
  IN  UINTN    X,
  IN  UINTN    Y
  );

VOID
RenderImageForAllGop (
  VOID
  );

VOID
DestroyImage (
  IN IMAGE_INFO    *ImageInfo
  );

VOID
SyncScreenImage (
  IN  GOP_ENTRY    *GopEntry,
  IN  RECT    *Rc,
  OUT BOOLEAN    *ImageIsSame
  );

VOID
CollectChangedImage (
  IN IMAGE_INFO    *ImageInfo
  );

VOID
ShowImage (
  IN  IMAGE_INFO    *ImageInfo
  );


EFI_STATUS
CursorGotoXY (
  IN  UINTN    X,
  IN  UINTN    Y
  );

#endif
