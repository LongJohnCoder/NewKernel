/** @file
Setup mouse driver.
Copyright (c) 2017, Byosoft Software Corporation. All Rights Reserved.

You may not reproduce, distribute, publish, display, perform, modify, adapt,
transmit, broadcast, present, recite, release, license or otherwise exploit
any part of this publication in any form, by any means, without the prior
written permission of Byosoft Software Corporation.

File Name:
  SetupMouse.c

Abstract:
  Setup Mouse Protocol implementation

Revision History:

**/

#include "SetupMouse.h"

PRIVATE_MOUSE_DATA          *mPrivate = NULL;
MOUSE_ACTION_INFO    MouseAction = {0, 0, MOUSE_MAX};


VOID
BrowserMouseEvent (
  IN  EFI_EVENT    Event,
  IN  VOID    *Context
  )
{
  PRIVATE_MOUSE_DATA    *Private;
  Private = mPrivate;
  if (1 == Private->LDoubleButton) {
    MouseAction.Action = MOUSE_LEFT_DOUBLE_CLICK;
    MouseAction.Column =  (Private->SaveCursorX - Private->MouseRange.StartX)/EFI_GLYPH_WIDTH;
    MouseAction.Row = (Private->SaveCursorY - Private->MouseRange.StartY) / EFI_GLYPH_HEIGHT;
    gBS->SignalEvent (Event);
    Private->LDoubleButton = 0;
    Private->LButton = 0;
  } else if (1 == Private->LButton) {
    MouseAction.Action = MOUSE_LEFT_CLICK;
    MouseAction.Column = (Private->SaveCursorX - Private->MouseRange.StartX)/EFI_GLYPH_WIDTH;
    MouseAction.Row = (Private->SaveCursorY - Private->MouseRange.StartY) / EFI_GLYPH_HEIGHT;
    gBS->SignalEvent (Event);
    Private->LButton = 0;
  } else if (1 == Private->RButton) {
    MouseAction.Action = MOUSE_RIGHT_CLICK;
    MouseAction.Column = (Private->SaveCursorX - Private->MouseRange.StartX)/EFI_GLYPH_WIDTH;
    MouseAction.Row = (Private->SaveCursorY - Private->MouseRange.StartY) / EFI_GLYPH_HEIGHT;
    gBS->SignalEvent (Event);
    Private->RButton = 0;
  }
  
  return; 
}


EFI_STATUS
GetMouseData (
  IN OUT MOUSE_ACTION_INFO  *Action
  )
{
  PRIVATE_MOUSE_DATA    *Private;
  Private = mPrivate;

  if (NULL == Action) {
    return EFI_INVALID_PARAMETER;
  }
  Action->Action =MOUSE_MAX;

  if (!Private->IsStart) {
    return EFI_NOT_READY;
  }
  if (!Private->HaveData) {
    return EFI_NOT_READY;
  }
  Private->HaveData = FALSE;
  Private->BeSetSate = FALSE;

  Action->Column =MouseAction.Column;
  MouseAction.Column = 0;
  
  Action->Row= MouseAction.Row;
  MouseAction.Row = 0;

  Action->Action = MouseAction.Action;
  MouseAction.Action = MOUSE_MAX;

  DMSG ((EFI_D_ERROR, "GetMouseData, Column :%d, Row :%d, Action :%d. \n", Action->Column, Action->Row, Action->Action));  

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
InitializeSetupMouse (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS    Status;
  PRIVATE_MOUSE_DATA    *Private;
  EFI_HANDLE    Handle;
  
  Private = AllocateZeroPool (sizeof (PRIVATE_MOUSE_DATA));
  if (NULL == Private) {
    return EFI_OUT_OF_RESOURCES;
  }  
  Private->SetupMouse.Start = SetupMouseStart;
  Private->SetupMouse.Close = SetupMouseClose;
  Private->SetupMouse.QueryState = QueryState;
  Private->SetupMouse.SetState = SetMouseState; 
  Private->SetupMouse.GetData = GetMouseData;
  Private->IsStart = FALSE;
  mPrivate = Private;

  Status = gBS->CreateEvent (
                     EVT_NOTIFY_WAIT,
                     TPL_NOTIFY,
                     BrowserMouseEvent,
                     (VOID*)(&Private->SetupMouse),
                     &Private->SetupMouse.WaitForMouse
                     );
  if (EFI_ERROR(Status)) {
    FreePool (Private);
    return EFI_NOT_READY;
  }

  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                 &Handle,
                 &gEfiSetupMouseProtocolGuid,
                 EFI_NATIVE_INTERFACE,
                 &(Private->SetupMouse)
                 );
  
  return Status;
}

EFI_STATUS
SetupMouseStart (
  VOID
  )
{
  EFI_STATUS    Status;
  UINTN    Index;
  UINTN    Count;
  UINTN    Rows;
  UINTN    Columns;
  UINTN    ScreenBufferSize;
  EFI_SIMPLE_POINTER_PROTOCOL    *SimplePointer;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  EFI_SIMPLE_POINTER_STATE    SimplePointerState;
  EFI_HANDLE    *HandleBuffer;
  GOP_ENTRY    *GopEntry;
  EFI_TPL    OriginalTPL;
  PRIVATE_MOUSE_DATA    *Private;
  
  Private = mPrivate;

  DMSG ((EFI_D_ERROR, "\n SetupMouseStart, IsStart :%d. \n", Private->IsStart));
  OriginalTPL = gBS->RaiseTPL(TPL_NOTIFY);
  if (Private->IsStart) {
    gBS->RestoreTPL (OriginalTPL);
    return EFI_SUCCESS;
  }
  gBS->RestoreTPL (OriginalTPL);  

  //
  // Initial Simple pointer. 
  //
  SimplePointer = NULL;
  Status = gBS->HandleProtocol(
                  gST->ConsoleInHandle,
                  &gEfiSimplePointerProtocolGuid,
                  (VOID**)&SimplePointer
                  );
  DMSG ((EFI_D_ERROR, "SetupMouseStart, Locate Simple Pointer Protocol :%r. \n", Status));  
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  SimplePointer->Reset(SimplePointer, TRUE);
  SimplePointer->GetState(SimplePointer, &SimplePointerState);
  Private->PointerProtocol = SimplePointer;
  
  //
  // Connect GraphicsOutput.
  //
  Count = 0;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  &Count,
                  &HandleBuffer
                  );
  DMSG ((EFI_D_ERROR, "SetupMouseStart, Locate GOP Protocol :%r. \n", Status));  
  if (EFI_ERROR (Status)) {
    goto EXIT_START;
  }

  //
  // Locate all GOP protocols.
  //
  Private->GopList  = AllocateZeroPool (sizeof (GOP_ENTRY) * Count);
  Private->GopCount = 0;

  for (Index = 0; Index < Count; Index++) {
    GraphicsOutput = NULL;

    Status = gBS->HandleProtocol(HandleBuffer[Index], &gEfiGraphicsOutputProtocolGuid, (VOID **)&GraphicsOutput);
    if (EFI_ERROR (Status)) {
      continue;
    }
    //
    // Test to see if this is a physical device by checking if
    // it has a Device Path Protocol.
    //
    DevicePath = NULL;
    Status = gBS->HandleProtocol(HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **) &DevicePath);
    if (EFI_ERROR (Status)) {
      continue;
    }

    GopEntry = &(Private->GopList[Private->GopCount]);
    //
    // Hook all GOP function.
    //
    GopEntry->Gop = GraphicsOutput;
    GopEntry->OriginalBlt= GraphicsOutput->Blt;
    GraphicsOutput->Blt = SetupMouseHookScreenBlt;

    SetRect (
      &GopEntry->Screen.ImageRc,
      0,
      0,
      GraphicsOutput->Mode->Info->HorizontalResolution,
      GraphicsOutput->Mode->Info->VerticalResolution
      );
    CopyMem (&GopEntry->InvalidateRc, &GopEntry->Screen.ImageRc, sizeof (RECT));

    ScreenBufferSize = GraphicsOutput->Mode->Info->HorizontalResolution *
    GraphicsOutput->Mode->Info->VerticalResolution * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    GopEntry->Screen.Image = AllocateZeroPool (ScreenBufferSize);
    GopEntry->BlendBuffer  = AllocateZeroPool (ScreenBufferSize);
    GopEntry->OriginalBlt (
                GraphicsOutput,
                GopEntry->Screen.Image,
                EfiBltVideoToBltBuffer,
                0,
                0,
                0,
                0,
                GraphicsOutput->Mode->Info->HorizontalResolution,
                GraphicsOutput->Mode->Info->VerticalResolution,
                0
                );

    GopEntry->FillLine = AllocateZeroPool (sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * GraphicsOutput->Mode->Info->HorizontalResolution);
    Private->GopCount++;
  }

  gBS->FreePool (HandleBuffer);
  if (Private->GopCount == 0) {
    Status = EFI_NOT_FOUND;
    goto EXIT_START;
  }

  ScreenBufferSize = sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) *
                       Private->GopList[0].Gop->Mode->Info->HorizontalResolution *
                       Private->GopList[0].Gop->Mode->Info->VerticalResolution;
  Private->CheckBuffer = AllocateZeroPool (ScreenBufferSize);
 
  //
  // Set Cursor Range
  //
  Status = gST->ConOut->QueryMode (
                          gST->ConOut,
                          gST->ConOut->Mode->Mode,
                          &Columns,
                          &Rows
                          );
  Private->MouseRange.StartX = ((Private->GopList[0].Gop->Mode->Info->HorizontalResolution - Columns * EFI_GLYPH_WIDTH)) / 2;
  Private->MouseRange.StartY = ((Private->GopList[0].Gop->Mode->Info->VerticalResolution   - Rows * EFI_GLYPH_HEIGHT)) / 2;
  Private->MouseRange.EndX = Private->MouseRange.StartX + (Columns * EFI_GLYPH_WIDTH) - 1;
  Private->MouseRange.EndY = Private->MouseRange.StartY + (Rows * EFI_GLYPH_HEIGHT) - 1;


  //
  // Set start flag.
  //
  Private->IsStart = TRUE;
  InitializeCursor ();
  if (Private->SaveCursorX == 0 && Private->SaveCursorY == 0) {
    Private->SaveCursorX = (Private->MouseRange.StartX + Private->MouseRange.EndX) / 2;
    Private->SaveCursorY = (Private->MouseRange.StartY + Private->MouseRange.EndY) / 2;
  }
  CursorGotoXY (Private->SaveCursorX, Private->SaveCursorY);
  ShowImage (&Private->Cursor);
  RenderImageForAllGop ();
  DMSG ((EFI_D_ERROR, "SetupMouseClose, Show mouse. \n"));  

  //
  //Create Setup Mouse event
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  (EFI_EVENT_NOTIFY) ProcessMouse,
                  (VOID*)(Private),
                  &Private->MouseEvent
                  );
  DMSG ((EFI_D_ERROR, "SetupMouseStart, Create Mouse Event :%r. \n", Status));  
  if (EFI_ERROR (Status)) {
    goto EXIT_START;
  }

  Status = gBS->SetTimer(Private->MouseEvent, TimerPeriodic, MOUSE_TIMER);
  DMSG ((EFI_D_ERROR, "SetupMouseStart, Set Mouse Event Timer :%r. \n", Status));  
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent(Private->MouseEvent);
    goto EXIT_START;
  }

  return EFI_SUCCESS;

EXIT_START:
  Private->IsStart = FALSE;
  return Status;
}

EFI_STATUS
SetupMouseClose (
  VOID
  )
{
  UINT32     Index;
  EFI_TPL    OriginalTPL;
  PRIVATE_MOUSE_DATA    *Private;
  Private = mPrivate;

  DMSG ((EFI_D_ERROR, "\n SetupMouseClose, IsStart :%d. \n", Private->IsStart));  
  OriginalTPL = gBS->RaiseTPL(TPL_NOTIFY);
  if (!Private->IsStart) {
    gBS->RestoreTPL(OriginalTPL);
    return EFI_SUCCESS;
  }  
  Private->IsStart = FALSE;
  gBS->CloseEvent (Private->MouseEvent);
  gBS->RestoreTPL(OriginalTPL);

  Private->SaveCursorX = (UINTN) Private->Cursor.ImageRc.left;
  Private->SaveCursorY = (UINTN) Private->Cursor.ImageRc.top;


  HideImage (&Private->Cursor);
  RenderImageForAllGop ();
  DestroyImage (&Private->Cursor);
  DMSG ((EFI_D_ERROR, "SetupMouseClose, Hide mouse. \n"));  


  if (Private->GopList != NULL) {
    for (Index = 0; Index < Private->GopCount; Index++) {
      Private->GopList[Index].Gop->Blt = Private->GopList[Index].OriginalBlt;
      DestroyImage (&Private->GopList[Index].Screen);

      if (Private->GopList[Index].BlendBuffer != NULL) {
        gBS->FreePool (Private->GopList[Index].BlendBuffer);
      }
      if (Private->GopList[Index].FillLine != NULL) {
        gBS->FreePool (Private->GopList[Index].FillLine);
      }
    }

    gBS->FreePool (Private->GopList);
    Private->GopList = NULL;
    Private->GopCount = 0;
  }

  if (Private->CheckBuffer != NULL) {
    gBS->FreePool (Private->CheckBuffer);
    Private->CheckBuffer = NULL;
  }

  Private->LButton = FALSE;
  Private->RButton = FALSE;

  return EFI_SUCCESS;
}

EFI_STATUS
QueryState (
  IN  OUT MOUSE_STATE    *State
  )
{
  PRIVATE_MOUSE_DATA    *Private;
  Private = mPrivate;

  State->X = 0;
  State->Y = 0;
  State->LeftClicK = 0;
  State->RightClick = 0;
  
  if (!Private->IsStart) {
    return EFI_NOT_READY;
  }

  if (!Private->HaveData) {
    return EFI_NOT_READY;
  }
  Private->HaveData = FALSE;

  State->X = Private->SaveCursorX;
  State->Y = Private->SaveCursorY;

  State->LeftClicK = Private->LButton;
  State->LeftDoubleClicK = Private->LDoubleButton;
  State->RightClick = Private->RButton;

  return EFI_SUCCESS;

}


EFI_STATUS
SetMouseState (
  IN MOUSE_STATE    *State
  )
{
  PRIVATE_MOUSE_DATA    *Private;
  Private = mPrivate;

  
  if (!Private->IsStart) {
    return EFI_NOT_READY;
  }

  if (!Private->HaveData) {
    return EFI_NOT_READY;
  }
  Private->HaveData = FALSE;
  Private->BeSetSate = TRUE;

  HideImage (&Private->Cursor);
  RenderImageForAllGop ();

  SetRect (
    &Private->Cursor.ImageRc,
    (INT32)State->X,
    (INT32)State->Y,
    (INT32)State->X + DEFAULT_CURSOR_WIDTH,
    (INT32)State->Y + DEFAULT_CURSOR_HEIGHT
    );

  Private->SaveCursorX = State->X;
  Private->SaveCursorY = State->Y;
  
  Private->LButton = State->LeftClicK;
  Private->LDoubleButton = State->LeftDoubleClicK;
  Private->RButton = State->RightClick;    
  Private->BeSetSate = FALSE;
  return EFI_SUCCESS;

}

