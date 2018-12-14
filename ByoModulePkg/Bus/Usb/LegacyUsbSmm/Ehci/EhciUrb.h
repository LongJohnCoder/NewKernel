/*++
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2009 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================
Module Name:

    EhciUrb.h

Abstract:

    This file contains URB request, each request is warpped in a
    URB (Usb Request Block)

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _EFI_EHCI_URB_H_
#define _EFI_EHCI_URB_H_

typedef struct _EHC_QTD     EHC_QTD;
typedef struct _EHC_QH      EHC_QH;
typedef struct _URB         URB;


enum {
    //
    // Transfer types, used in URB to identify the transfer type
    //
    EHC_CTRL_TRANSFER       = 0x01,
    EHC_BULK_TRANSFER       = 0x02,
    EHC_INT_TRANSFER_SYNC   = 0x04,
    EHC_INT_TRANSFER_ASYNC  = 0x08,
    EHC_INT_ONLY_TRANSFER_ASYNC  = 0x10,

    EHC_QTD_SIG             = SIGNATURE_32 ('U', 'S', 'B', 'T'),
    EHC_QH_SIG              = SIGNATURE_32 ('U', 'S', 'B', 'H'),
    EHC_URB_SIG             = SIGNATURE_32 ('U', 'S', 'B', 'R'),

    //
    // Hardware related bit definitions
    //
    EHC_TYPE_ITD            = 0x00,
    EHC_TYPE_QH             = 0x02,
    EHC_TYPE_SITD           = 0x04,
    EHC_TYPE_FSTN           = 0x06,

    QH_NAK_RELOAD           = 3,
    QH_HSHBW_MULTI          = 1,

    QTD_MAX_ERR             = 3,
    QTD_PID_OUTPUT          = 0x00,
    QTD_PID_INPUT           = 0x01,
    QTD_PID_SETUP           = 0x02,

    QTD_STAT_DO_OUT         = 0,
    QTD_STAT_DO_SS          = 0,
    QTD_STAT_DO_PING        = 0x01,
    QTD_STAT_DO_CS          = 0x02,
    QTD_STAT_TRANS_ERR      = 0x08,
    QTD_STAT_BABBLE_ERR     = 0x10,
    QTD_STAT_BUFF_ERR       = 0x20,
    QTD_STAT_HALTED         = 0x40,
    QTD_STAT_ACTIVE         = 0x80,
    QTD_STAT_ERR_MASK       = QTD_STAT_TRANS_ERR | QTD_STAT_BABBLE_ERR | QTD_STAT_BUFF_ERR,

    QTD_MAX_BUFFER          = 4,
    QTD_BUF_LEN             = 4096,
    QTD_BUF_MASK            = 0x0FFF,

    QH_MICROFRAME_0         = 0x01,
    QH_MICROFRAME_1         = 0x02,
    QH_MICROFRAME_2         = 0x04,
    QH_MICROFRAME_3         = 0x08,
    QH_MICROFRAME_4         = 0x10,
    QH_MICROFRAME_5         = 0x20,
    QH_MICROFRAME_6         = 0x40,
    QH_MICROFRAME_7         = 0x80,

    USB_ERR_SHORT_PACKET    = 0x200,
    EHC_INTERRUPT_ONLY_DEVICE_ADDRESS = 0x7F,
    EHC_INTERRUPT_ONLY_ENDPOINT_ADDRESS = 0x00,
};

//
// Fill in the hardware link point: pass in a EHC_QH/QH_HW
// pointer to QH_LINK; A EHC_QTD/QTD_HW pointer to QTD_LINK
//
#define QH_LINK(Addr, Type, Term) \
          ((UINT32) ((EHC_LOW_32BIT (Addr) & 0xFFFFFFE0) | (Type) | ((Term) ? 1 : 0)))

#define QTD_LINK(Addr, Term)      QH_LINK((Addr), 0, (Term))

//
// The defination of EHCI hardware used data structure for
// little endian architecture. The QTD and QH structures
// are required to be 32 bytes aligned. Don't add members
// to the head of the associated software strucuture.
//
#pragma pack(1)
typedef struct {
    UINT32                  NextQtd;
    UINT32                  AltNext;

    UINT32                  Status       : 8;
    UINT32                  Pid          : 2;
    UINT32                  ErrCnt       : 2;
    UINT32                  CurPage      : 3;
    UINT32                  IOC          : 1;
    UINT32                  TotalBytes   : 15;
    UINT32                  DataToggle   : 1;

    UINT32                  Page[5];
    UINT32                  PageHigh[5];
} QTD_HW;

typedef struct {
    UINT32                  HorizonLink;
    //
    // Endpoint capabilities/Characteristics DWord 1 and DWord 2
    //
    UINT32                  DeviceAddr   : 7;
    UINT32                  Inactive     : 1;
    UINT32                  EpNum        : 4;
    UINT32                  EpSpeed      : 2;
    UINT32                  DtCtrl       : 1;
    UINT32                  ReclaimHead  : 1;
    UINT32                  MaxPacketLen : 11;
    UINT32                  CtrlEp       : 1;
    UINT32                  NakReload    : 4;

    UINT32                  SMask        : 8;
    UINT32                  CMask        : 8;
    UINT32                  HubAddr      : 7;
    UINT32                  PortNum      : 7;
    UINT32                  Multiplier   : 2;

    //
    // Transaction execution overlay area
    //
    UINT32                  CurQtd;
    UINT32                  NextQtd;
    UINT32                  AltQtd;

    UINT32                  Status       : 8;
    UINT32                  Pid          : 2;
    UINT32                  ErrCnt       : 2;
    UINT32                  CurPage      : 3;
    UINT32                  IOC          : 1;
    UINT32                  TotalBytes   : 15;
    UINT32                  DataToggle   : 1;

    UINT32                  Page[5];
    UINT32                  PageHigh[5];
} QH_HW;
#pragma pack()


//
// Endpoint address and its capabilities
//
typedef struct _USB_ENDPOINT {
    UINT8                   DevAddr;
    UINT8                   EpAddr;     // Endpoint address, no direction encoded in
    EFI_USB_DATA_DIRECTION  Direction;
    UINT8                   DevSpeed;
    UINTN                   MaxPacket;
    UINT8                   HubAddr;
    UINT8                   HubPort;
    UINT8                   Toggle;     // Data toggle, not used for control transfer
    UINTN                   Type;
    UINTN                   PollRate;   // Polling interval used by EHCI
} USB_ENDPOINT;

//
// Software QTD strcture, this is used to manage all the
// QTD generated from a URB. Don't add fields before QtdHw.
//
typedef struct _EHC_QTD {
    QTD_HW                  QtdHw;
    UINT32                  Signature;
    LIST_ENTRY              QtdList;   // The list of QTDs to one end point
    UINT8                   *Data;     // Buffer of the original data
    UINTN                   DataLen;   // Original amount of data in this QTD
} EHC_QTD;

//
// Software QH structure. All three different transaction types
// supported by UEFI USB, that is the control/bulk/interrupt
// transfers use the queue head and queue token strcuture.
//
// Interrupt QHs are linked to periodic frame list in the reversed
// 2^N tree. Each interrupt QH is linked to the list starting at
// frame 0. There is a dummy interrupt QH linked to each frame as
// a sentinental whose polling interval is 1. Synchronous interrupt
// transfer is linked after this dummy QH.
//
// For control/bulk transfer, only synchronous (in the sense of UEFI)
// transfer is supported. A dummy QH is linked to EHCI AsyncListAddr
// as the reclamation header. New transfer is inserted after this QH.
//
typedef struct _EHC_QH {
    QH_HW                   QhHw;
    UINT32                  Signature;
    EHC_QH                  *NextQh;    // The queue head pointed to by horizontal link
    LIST_ENTRY              Qtds;       // The list of QTDs to this queue head
    UINTN                   Interval;
} EHC_QH;

//
// URB (Usb Request Block) contains information for all kinds of
// usb requests.
//
typedef struct _URB {
    UINT32                          Signature;
    LIST_ENTRY                      UrbList;

    //
    // Transaction information
    //
    USB_ENDPOINT                    Ep;
    EFI_USB_DEVICE_REQUEST          *Request;     // Control transfer only
    VOID                            *Data;
    UINTN                           DataLen;
    EFI_ASYNC_USB_TRANSFER_CALLBACK Callback;
    VOID                            *Context;

    //
    // Schedule data
    //
    EHC_QH                          *Qh;

    //
    // Transaction result
    //
    UINT32                          Result;
    UINTN                           Completed;    // completed data length
    UINT8                           DataToggle;
} URB;


EHC_QTD *
EhcCreateQtd (
    IN USB2_HC_DEV          *Ehc,
    IN UINT8                *Data,
    IN UINTN                DataLen,
    IN UINT8                PktId,
    IN UINT8                Toggle,
    IN UINTN                MaxPacket
)
/*++

Routine Description:

  Create a single QTD to hold the data

Arguments:

  Ehc       - The EHCI device
  Data      - Current data not associated with a QTD
  DataLen   - The length of the data
  PktId     - Packet ID to use in the QTD
  Toggle    - Data toggle to use in the QTD
  MaxPacket - Maximu packet length of the endpoint

Returns:

  Created QTD or NULL if failed to create one

--*/
;


EHC_QH *
EhcCreateQh (
    IN USB2_HC_DEV          *Ehci,
    IN USB_ENDPOINT         *Ep
)
/*++

Routine Description:

  Allocate and initialize a EHCI queue head

Arguments:

  Ehci  - The EHCI device
  Ep    - The endpoint to create queue head for

Returns:

  Created queue head or NULL if failed to create one

--*/
;

VOID
EhcFreeUrb (
    IN USB2_HC_DEV          *Ehc,
    IN URB                  *Urb
)
/*++

Routine Description:

  Free an allocated URB. It is possible for it to be partially inited.

Arguments:

  Ehc - The EHCI device
  Urb - The URB to free

Returns:

  None

--*/
;

URB *
EhcCreateUrb (
    IN USB2_HC_DEV                        *Ehc,
    IN UINT8                              DevAddr,
    IN UINT8                              EpAddr,
    IN UINT8                              DevSpeed,
    IN UINT8                              Toggle,
    IN UINTN                              MaxPacket,
    IN EFI_USB2_HC_TRANSACTION_TRANSLATOR *Hub,
    IN UINTN                              Type,
    IN EFI_USB_DEVICE_REQUEST             *Request,
    IN VOID                               *Data,
    IN UINTN                              DataLen,
    IN EFI_ASYNC_USB_TRANSFER_CALLBACK    Callback,
    IN VOID                               *Context,
    IN UINTN                              Interval
)
/*++

Routine Description:

  Create a new URB and its associated QTD

Arguments:

  Ehc       - The EHCI device
  DevAddr   - The device address
  EpAddr    - Endpoint addrress & its direction
  DevSpeed  - The device speed
  Toggle    - Initial data toggle to use
  MaxPacket - The max packet length of the endpoint
  Hub       - The transaction translator to use
  Type      - The transaction type
  Request   - The standard USB request for control transfer
  Data      - The user data to transfer
  DataLen   - The length of data buffer
  Callback  - The function to call when data is transferred
  Context   - The context to the callback
  Interval  - The interval for interrupt transfer

Returns:

  Created URB or NULL

--*/
;
#endif
