/** @file
Setup mouse driver.
Copyright (c) 2017, Byosoft Software Corporation. All Rights Reserved.

You may not reproduce, distribute, publish, display, perform, modify, adapt,
transmit, broadcast, present, recite, release, license or otherwise exploit
any part of this publication in any form, by any means, without the prior
written permission of Byosoft Software Corporation.

File Name:
  SetupMouseProtocol.h

Abstract:
  Setup Mouse Protocol implementation

Revision History:

**/
 
#ifndef _SETUP_MOUSE_PROTOCOL_H_
#define _SETUP_MOUSE_PROTOCOL_H_

#define SETUP_MOUSE_PROTOCOL_GUID \
  { \
  	0xa53d7284, 0xc9be, 0x4f8a, { 0xb6, 0x9e, 0x9a, 0xb7, 0x2d, 0x5f, 0x64, 0x36 }\
  }
extern EFI_GUID gEfiSetupMouseProtocolGuid;


typedef struct {
  UINTN    StartX;
  UINTN    StartY;
  UINTN    EndX;
  UINTN    EndY;
} MOUSE_CURSOR_RANGE;

typedef struct {
  INTN    X;
  INTN    Y;
  BOOLEAN    LeftClicK;
  BOOLEAN    LeftDoubleClicK;
  BOOLEAN    RightClick;
} MOUSE_STATE;

typedef enum {
  MOUSE_LEFT_CLICK = 1,
  MOUSE_LEFT_DOUBLE_CLICK,
  MOUSE_RIGHT_CLICK,
  MOUSE_MAX
} MOUSE_ACTION;

typedef struct {
  UINTN    Column;
  UINTN    Row;
  MOUSE_ACTION    Action;
} MOUSE_ACTION_INFO;

typedef struct _EFI_SETUP_MOUSE_PROTOCOL  EFI_SETUP_MOUSE_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_START_MOUSE) (
  VOID
);

typedef
EFI_STATUS
(EFIAPI *EFI_CLOSE_MOUSE) (
  VOID
  );

typedef
EFI_STATUS
(EFIAPI *EFI_QUERY_MOUSE_STATE) (
  IN  OUT MOUSE_STATE    *State
);

typedef
EFI_STATUS
(EFIAPI *EFI_SET_MOUSE_STATE) (
  IN MOUSE_STATE    *State
);

typedef
EFI_STATUS
(EFIAPI *EFI_GET_MOUSE_DATA) (
  IN OUT MOUSE_ACTION_INFO  *Action
  );
  
typedef struct _EFI_SETUP_MOUSE_PROTOCOL {
  EFI_EVENT    WaitForMouse;
  EFI_START_MOUSE    Start;
  EFI_CLOSE_MOUSE    Close;
  EFI_QUERY_MOUSE_STATE    QueryState;
  EFI_SET_MOUSE_STATE    SetState;
  EFI_GET_MOUSE_DATA    GetData;
} EFI_SETUP_MOUSE_PROTOCOL;

#endif
