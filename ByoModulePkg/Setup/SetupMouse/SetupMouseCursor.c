/** @file
Setup mouse driver.
Copyright (c) 2017, Byosoft Software Corporation. All Rights Reserved.

You may not reproduce, distribute, publish, display, perform, modify, adapt,
transmit, broadcast, present, recite, release, license or otherwise exploit
any part of this publication in any form, by any means, without the prior
written permission of Byosoft Software Corporation.

File Name:
  SetupMouseCursor.c

Abstract:
  Setup Mouse Protocol implementation

Revision History:

**/

#include "SetupMouse.h"

EFI_GRAPHICS_OUTPUT_BLT_PIXEL DefaultCursorIcon[DEFAULT_CURSOR_HEIGHT][DEFAULT_CURSOR_WIDTH] = {
  {{  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0, 255}, {255, 255, 255, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {  0,   0,   0, 255}, {  0,   0,   0, 255}, {  0,   0,   0, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0, 255}, {255, 255, 255, 255}, {  0,   0,   0, 255}, {  0,   0,   0, 255}, {255, 255, 255, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0, 255}, {255, 255, 255, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0, 255}, {255, 255, 255, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0, 255}, {255, 255, 255, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0, 255}, {255, 255, 255, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0, 255}, {255, 255, 255, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
  {{  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0, 255}, {  0,   0,   0, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}},
};

EFI_STATUS
InitializeCursor (
  VOID
  )
{
  UINTN    Width;
  UINTN    Height;
  PRIVATE_MOUSE_DATA    *Private;
  Private = mPrivate;

  Width  = DEFAULT_CURSOR_WIDTH;
  Height = DEFAULT_CURSOR_HEIGHT;
  SetRect (
    &Private->Cursor.ImageRc,
    (INT32)Private->MouseRange.StartX,
    (INT32)Private->MouseRange.StartY,
    (INT32)(Private->MouseRange.StartX + Width),
    (INT32)(Private->MouseRange.StartY + Height)
    );

  Private->Cursor.Image = AllocateZeroPool (sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * Width * Height);
  CopyMem (Private->Cursor.Image, DefaultCursorIcon, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * Width * Height);

  return EFI_SUCCESS;
}

EFI_STATUS
CursorGotoXY (
  IN  UINTN    X,
  IN  UINTN    Y
  )
{
  PRIVATE_MOUSE_DATA    *Private;
  Private = mPrivate;

  if (X < Private->MouseRange.StartX || Y < Private->MouseRange.StartY ||
      X > Private->MouseRange.EndX || Y > Private->MouseRange.EndY) {
    return EFI_SUCCESS;
  }

  if ((INT32)X != Private->Cursor.ImageRc.left || (INT32)Y != Private->Cursor.ImageRc.top) {
    MoveImage (&Private->Cursor, X, Y);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
ProcessMouse (
  IN  EFI_EVENT    Event,
  IN  VOID    *Context
  )
{
  EFI_STATUS    Status;
  PRIVATE_MOUSE_DATA    *Private;
  INTN    CurX, CurY;
  EFI_SIMPLE_POINTER_STATE    SimplePointerState;
  EFI_SIMPLE_POINTER_PROTOCOL    *SimplePointerPtr;
  static UINTN    TimeCount = 0;
  static BOOLEAN    LastLeftClick = FALSE;
  static INTN   LastX = 0;
  static INTN   LastY = 0;
  
  Private = mPrivate;
  if (!Private->IsStart || Private->BeSetSate) {
    return EFI_NOT_READY;
  }  

  TimeCount++;
  if (TimeCount > 13) {
    if (LastLeftClick) {
      Private->LButton = 1;
    }	
    LastLeftClick = FALSE;
    TimeCount = 0;
  }

  SimplePointerPtr = (EFI_SIMPLE_POINTER_PROTOCOL*)Private ->PointerProtocol;
  Status = SimplePointerPtr->GetState(SimplePointerPtr, &SimplePointerState);
  if (EFI_ERROR(Status)) {
    return EFI_NOT_READY;
  }

  //
  // update Coordinate.
  //
  CurX = (INTN)Private->Cursor.ImageRc.left;
  CurX += (SimplePointerState.RelativeMovementX) / MOUSE_RESOLUTION;
  if (CurX <= (INTN) Private->MouseRange.StartX) {
    CurX = (INTN) Private->MouseRange.StartX;
  } else if (CurX >= (INTN) Private->MouseRange.EndX) {
    CurX = (INTN)  Private->MouseRange.EndX;
  }

  CurY = (INTN)Private->Cursor.ImageRc.top;  
  CurY += (SimplePointerState.RelativeMovementY) / MOUSE_RESOLUTION;
  if (CurY <= (INTN) Private->MouseRange.StartY) {
    CurY = (INTN) Private->MouseRange.StartY;
  } else if (CurY >= (INTN) Private->MouseRange.EndY) {
    CurY = (INTN) Private->MouseRange.EndY;
  }


  // 
  // update Right button.
  //  
  Private->RButton = SimplePointerState.RightButton;    	

  // 
  // update Left button.
  //  
  if (SimplePointerState.LeftButton && !LastLeftClick) {
    LastLeftClick = TRUE;
    Private->SaveCursorX = CurX;
    Private->SaveCursorY = CurY;

    TimeCount = 0;
  } else if (SimplePointerState.LeftButton && LastLeftClick) {
   if (TimeCount > 6) {
      if (Private->SaveCursorX == (UINTN)CurX && Private->SaveCursorY == (UINTN)CurY) {
        Private->LDoubleButton = TRUE;
        Private->LButton = FALSE;  
        LastLeftClick = FALSE;
      }
    }
  } else {
    if (!LastLeftClick) {
      Private->SaveCursorX = CurX;
      Private->SaveCursorY = CurY;
    }
  }

  Private->HaveData = TRUE;
  if (LastX != CurX ||
     LastY != CurY) {
    //
    // Show Cursor.
    //
    ShowImage (&Private->Cursor);
    CursorGotoXY (CurX, CurY);
    RenderImageForAllGop ();
    DMSG ((EFI_D_ERROR, "ProcessMouse, CurX :%d, CurY :%d. \n", CurX, CurY));  

    LastX = CurX;
    LastY = CurY;
  }

  return EFI_SUCCESS; 
}

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
  )
{
  EFI_STATUS    Status;
  GOP_ENTRY    *GopEntry;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *ScreenFrameBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *VScreen;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *VScreenSrc;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Blt;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *FillPixel;
  UINTN    DstY;
  UINTN    SrcY;
  UINTN    Index;
  EFI_TPL    OriginalTPL;
  UINT32    VerticalResolution;
  UINT32    HorizontalResolution;
  RECT    InvalidateRc;

  if ((BltOperation < 0) || (BltOperation >= EfiGraphicsOutputBltOperationMax)) {
    return EFI_INVALID_PARAMETER;
  }
  if (Width == 0 || Height == 0) {
    return EFI_INVALID_PARAMETER;
  }
  if (Delta == 0) {
    Delta = Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  }

  //
  // Look for Gop.
  //
  for (Index = 0; Index < mPrivate->GopCount; Index++) {
    if (mPrivate->GopList[Index].Gop == This) {
      break;
    }
  }
  if (Index == mPrivate->GopCount) {
    return EFI_UNSUPPORTED;
  }
  GopEntry = (GOP_ENTRY*) (&mPrivate->GopList[Index]);
  ScreenFrameBuffer = GopEntry->Screen.Image;
  VerticalResolution = GopEntry->Gop->Mode->Info->VerticalResolution;
  HorizontalResolution = GopEntry->Gop->Mode->Info->HorizontalResolution;

  //
  // Video to BltBuffer.
  //
  if (BltOperation == EfiBltVideoToBltBuffer) {
    if (SourceY + Height > VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (SourceX + Width > HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
	
    OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);
    for (SrcY = SourceY, DstY = DestinationY; DstY < (Height + DestinationY); SrcY++, DstY++) {
      Blt = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) ((UINT8 *) BltBuffer + (DstY * Delta) + DestinationX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      VScreen = &ScreenFrameBuffer[SrcY * HorizontalResolution + SourceX];
      CopyMem (Blt, VScreen, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * Width);
    }
    gBS->RestoreTPL (OriginalTPL);
	
    return EFI_SUCCESS;
  }

  //
  // BltBuffer to Video.
  //
  if (DestinationY + Height > VerticalResolution) {
    return EFI_INVALID_PARAMETER;
  }

  if (DestinationX + Width > HorizontalResolution) {
    return EFI_INVALID_PARAMETER;
  }

  OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);
  if (BltOperation == EfiBltVideoFill) {
    FillPixel = BltBuffer;
    for (Index = 0; Index < Width; Index++) {
      GopEntry->FillLine[Index] = *FillPixel;
    }
  }

  for (Index = 0; Index < Height; Index++) {
    if (DestinationY <= SourceY) {
      SrcY  = SourceY + Index;
      DstY  = DestinationY + Index;
    } else {
      SrcY  = SourceY + Height - Index - 1;
      DstY  = DestinationY + Height - Index - 1;
    }

    VScreen = &ScreenFrameBuffer[DstY * HorizontalResolution + DestinationX];
    switch (BltOperation) {
    case EfiBltBufferToVideo:
      Blt = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) ((UINT8 *) BltBuffer + (SrcY * Delta) + SourceX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      CopyMem (VScreen, Blt, Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      break;

    case EfiBltVideoToVideo:
      VScreenSrc = &ScreenFrameBuffer[SrcY * HorizontalResolution + SourceX];
      CopyMem (VScreen, VScreenSrc, Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      break;

    case EfiBltVideoFill:
      CopyMem (VScreen, GopEntry->FillLine, Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      break;

    default:
      break;
    }
  }

  SetRect (
    &InvalidateRc,
    (INT32)DestinationX,
    (INT32)DestinationY,
    (INT32)(DestinationX + Width),
    (INT32)(DestinationY + Height)
    );
  UnionRect (&GopEntry->InvalidateRc, &GopEntry->InvalidateRc, &InvalidateRc);
  Status = RenderImage (GopEntry, FALSE);
  gBS->RestoreTPL (OriginalTPL);

  return Status;
}


VOID
SyncScreenImage (
  IN  GOP_ENTRY    *GopEntry,
  IN  RECT    *Rc,
  OUT BOOLEAN    *ImageIsSame
  )
{
  INT32    X;
  INT32    Y;
  INT32    Width;
  INT32    Height;
  UINT32    *BlendBlt;
  UINT32    *CheckBlt;
  UINT32    *ScreenBlt;
  UINTN     HorizontalResolution;
  PRIVATE_MOUSE_DATA    *Private;

  ASSERT (ImageIsSame != NULL);
  Private = mPrivate;
  
  HorizontalResolution = GopEntry->Gop->Mode->Info->HorizontalResolution;
  Width  = Rc->right - Rc->left;
  Height = Rc->bottom - Rc->top;
  
  //
  // Read image from Gop.
  //
  GopEntry->OriginalBlt (
              GopEntry->Gop,
              Private->CheckBuffer,
              EfiBltVideoToBltBuffer,
              Rc->left,
              Rc->top,
              Rc->left,
              Rc->top,
              Width,
              Height,
              sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * HorizontalResolution
              );

  *ImageIsSame = TRUE;
  for (Y = Rc->top; Y < Rc->bottom; Y++) {
    BlendBlt  = (UINT32 *)&GopEntry->BlendBuffer[Y * HorizontalResolution + Rc->left];
    CheckBlt  = (UINT32 *)&Private->CheckBuffer[Y * HorizontalResolution + Rc->left];
    ScreenBlt = (UINT32 *)&GopEntry->Screen.Image[Y * HorizontalResolution + Rc->left];
    for (X = 0; X < Width; X++) {
      if ((*BlendBlt & 0x00FFFFFF) != (*CheckBlt & 0x00FFFFFF)) {
        *ScreenBlt = *CheckBlt;
        *ImageIsSame = FALSE;
      }
      BlendBlt++;
      ScreenBlt++;
      CheckBlt++;
    }
  }
}

VOID
UpdateImage (
  IN GOP_ENTRY    *GopEntry,
  IN IMAGE_INFO    *ImageInfo,
  IN RECT    *UpdateRc,
  IN BOOLEAN    Transparent
  )
{
  INT32    X;
  INT32    Y;
  INT32    Width;
  INT32    Height;
  INT32    ImageWidth;
  INT32    ImageHeight;
  UINT32    *ImageBlt;
  UINT32    *BlendBlt;
  RECT    *ImageRc;
  UINTN    HorizontalResolution;

  Width = UpdateRc->right - UpdateRc->left;
  Height= UpdateRc->bottom - UpdateRc->top;
  ImageWidth  = ImageInfo->ImageRc.right - ImageInfo->ImageRc.left;
  ImageHeight = ImageInfo->ImageRc.bottom - ImageInfo->ImageRc.top;
  if ((Width  > ImageWidth) || (Height > ImageHeight)) {
    return;
  }

  ImageRc = &ImageInfo->ImageRc;
  HorizontalResolution = GopEntry->Gop->Mode->Info->HorizontalResolution;
  for (Y = UpdateRc->top; Y < UpdateRc->bottom; Y++) {
    ImageBlt = (UINT32*)&ImageInfo->Image[(Y - ImageRc->top) * ImageWidth + UpdateRc->left - ImageRc->left];
    BlendBlt = (UINT32*)&GopEntry->BlendBuffer[Y * HorizontalResolution + UpdateRc->left];
    if (!Transparent) {
      CopyMem (BlendBlt, ImageBlt, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * Width);
    } else {
      for (X = 0; X < Width; X++) {
        if (*ImageBlt != 0) {
          *BlendBlt = *ImageBlt;
        }
        BlendBlt++;
        ImageBlt++;
      }
    }
  }
  return;
}

EFI_STATUS
RenderImage (
  IN GOP_ENTRY    *GopEntry,
  IN BOOLEAN    NeedSyncScreen
  )
{
  EFI_STATUS    Status;
  RECT    UpdateRc;
  RECT    InvalidateRc;
  BOOLEAN    ImageIsSame;
  PRIVATE_MOUSE_DATA    *Private;
  Private = mPrivate;

  CopyMem (&InvalidateRc, &GopEntry->InvalidateRc, sizeof (RECT));
  IntersectRect (&InvalidateRc, &InvalidateRc, &GopEntry->Screen.ImageRc);

  if (IsRectEmpty (&InvalidateRc)) {
    return EFI_SUCCESS;
  }

  if (NeedSyncScreen) {
    SyncScreenImage (GopEntry, &InvalidateRc, &ImageIsSame);
  }

  if (IntersectRect (&UpdateRc, &GopEntry->Screen.ImageRc, &InvalidateRc)) {
    UpdateImage (GopEntry, &GopEntry->Screen, &UpdateRc, FALSE);
  }
  
  if (Private->Cursor.Visible && IntersectRect (&UpdateRc, &Private->Cursor.ImageRc, &InvalidateRc)) {
    UpdateImage (GopEntry, &Private->Cursor, &UpdateRc, TRUE);
  }

  Status = GopEntry->OriginalBlt (
                       GopEntry->Gop,
                       GopEntry->BlendBuffer,
                       EfiBltBufferToVideo,
                       InvalidateRc.left,
                       InvalidateRc.top,
                       InvalidateRc.left,
                       InvalidateRc.top,
                       InvalidateRc.right - InvalidateRc.left,
                       InvalidateRc.bottom - InvalidateRc.top,
                       sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * GopEntry->Gop->Mode->Info->HorizontalResolution
                       );

  SetRectEmpty (&GopEntry->InvalidateRc);

  return Status;
}

VOID
RenderImageForAllGop (
  VOID
  )
{
  UINTN Index;
  PRIVATE_MOUSE_DATA    *Private;
  Private = mPrivate;

  for (Index = 0; Index < Private->GopCount; Index++) {
    RenderImage (&Private->GopList[Index], TRUE);
  }
}

