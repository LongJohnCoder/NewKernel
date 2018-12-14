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

    EhciUrb.c

Abstract:

    This file contains URB request, each request is warpped in a
    URB (Usb Request Block)

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include "Ehci.h"
#include "../UsbLib/UsbMem.h"

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
{
    EHC_QTD                 *Qtd;
    QTD_HW                  *QtdHw;
    UINTN                   Index;
    UINTN                   Len;
    UINTN                   ThisBufLen;

    ASSERT (Ehc != NULL);

    Qtd = UsbAllocatePool(sizeof (EHC_QTD));

    if (Qtd == NULL) {
        return NULL;
    }

    Qtd->Signature    = EHC_QTD_SIG;
    Qtd->Data         = Data;
    Qtd->DataLen      = 0;

    InitializeListHead (&Qtd->QtdList);

    QtdHw             = &Qtd->QtdHw;
    QtdHw->NextQtd    = QTD_LINK (NULL, TRUE);
    QtdHw->AltNext    = QTD_LINK (NULL, TRUE);
    QtdHw->Status     = QTD_STAT_ACTIVE;
    QtdHw->Pid        = PktId;
    QtdHw->ErrCnt     = QTD_MAX_ERR;
    QtdHw->IOC        = 0;
    QtdHw->TotalBytes = 0;
    QtdHw->DataToggle = Toggle;

    //
    // Fill in the buffer points
    //
    if (Data != NULL) {
        Len = 0;

        for (Index = 0; Index <= QTD_MAX_BUFFER; Index++) {
            //
            // Set the buffer point (Check page 41 EHCI Spec 1.0). No need to
            // compute the offset and clear Reserved fields. This is already
            // done in the data point.
            //
            QtdHw->Page[Index]      = EHC_LOW_32BIT (Data);
            QtdHw->PageHigh[Index]  = EHC_HIGH_32BIT (Data);

            ThisBufLen              = QTD_BUF_LEN - (EHC_LOW_32BIT (Data) & QTD_BUF_MASK);

            if (Len + ThisBufLen >= DataLen) {
                Len = DataLen;
                break;
            }

            Len += ThisBufLen;
            Data += ThisBufLen;
        }

        //
        // Need to fix the last pointer if the Qtd can't hold all the
        // user's data to make sure that the length is in the unit of
        // max packets. If it can hold all the data, there is no such
        // need.
        //
        if (Len < DataLen) {
            Len = Len - Len % MaxPacket;
        }

        QtdHw->TotalBytes = (UINT32) Len;
        Qtd->DataLen      = Len;
    }

    return Qtd;
}

VOID
EhcInitIntQh (
    IN USB_ENDPOINT         *Ep,
    IN QH_HW                *QhHw
)
/*++

Routine Description:

  Initialize the queue head for interrupt transfer,
  that is, initialize the following three fields:
    1. SplitXState in the Status field
    2. Microframe S-mask
    3. Microframe C-mask

Arguments:

  Ep    - The queue head's related endpoint
  QhHw  - The queue head to initialize

Returns:

  None

--*/
{
    //
    // Because UEFI interface can't utilitize an endpoint with
    // poll rate faster than 1ms, only need to set one bit in
    // the queue head. simple. But it may be changed later. If
    // sub-1ms interrupt is supported, need to update the S-Mask
    // here
    //
    if (Ep->DevSpeed == EFI_USB_SPEED_HIGH) {
        QhHw->SMask = QH_MICROFRAME_0;
        return ;
    }

    //
    // For low/full speed device, the transfer must go through
    // the split transaction. Need to update three fields
    // 1. SplitXState in the status
    // 2. Microframe S-Mask
    // 3. Microframe C-Mask
    // UEFI USB doesn't exercise admission control. It simplely
    // schedule the high speed transactions in microframe 0, and
    // full/low speed transactions at microframe 1. This also
    // avoid the use of FSTN.
    //
    QhHw->SMask = QH_MICROFRAME_1;
    QhHw->CMask = QH_MICROFRAME_3 | QH_MICROFRAME_4 | QH_MICROFRAME_5;
}

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
{
    EHC_QH                  *Qh;
    QH_HW                   *QhHw;

    Qh = UsbAllocatePool (sizeof (EHC_QH));

    if (Qh == NULL) {
        return NULL;
    }

    Qh->Signature       = EHC_QH_SIG;
    Qh->NextQh          = NULL;
    Qh->Interval        = Ep->PollRate;

    InitializeListHead (&Qh->Qtds);

    QhHw                = &Qh->QhHw;
    QhHw->HorizonLink   = QH_LINK (NULL, 0, TRUE);
    QhHw->DeviceAddr    = Ep->DevAddr;
    QhHw->Inactive      = 0;
    QhHw->EpNum         = Ep->EpAddr;
    QhHw->EpSpeed       = Ep->DevSpeed;
    QhHw->DtCtrl        = 0;
    QhHw->ReclaimHead   = 0;
    QhHw->MaxPacketLen  = (UINT32) Ep->MaxPacket;
    QhHw->CtrlEp        = 0;
    QhHw->NakReload     = QH_NAK_RELOAD;
    QhHw->HubAddr       = Ep->HubAddr;
    QhHw->PortNum       = Ep->HubPort;
    QhHw->Multiplier    = 1;
    QhHw->DataToggle    = Ep->Toggle;

    if (Ep->DevSpeed != EFI_USB_SPEED_HIGH) {
        QhHw->Status |= QTD_STAT_DO_SS;
    }

    switch (Ep->Type) {
    case EHC_CTRL_TRANSFER:
        //
        // Special initialization for the control transfer:
        // 1. Control transfer initialize data toggle from each QTD
        // 2. Set the Control Endpoint Flag (C) for low/full speed endpoint.
        //
        QhHw->DtCtrl = 1;

        if (Ep->DevSpeed != EFI_USB_SPEED_HIGH) {
            QhHw->CtrlEp = 1;
        }
        break;

    case EHC_INT_TRANSFER_ASYNC:
    case EHC_INT_TRANSFER_SYNC:
    case EHC_INT_ONLY_TRANSFER_ASYNC:
        //
        // Special initialization for the interrupt transfer
        // to set the S-Mask and C-Mask
        //
        QhHw->NakReload = 0;
        EhcInitIntQh (Ep, QhHw);
        break;

    case EHC_BULK_TRANSFER:
        if ((Ep->DevSpeed == EFI_USB_SPEED_HIGH) && (Ep->Direction == EfiUsbDataOut)) {
            QhHw->Status |= QTD_STAT_DO_PING;
        }

        break;
    }

    return Qh;
}

UINTN
EhcConvertPollRate (
    IN  UINTN               Interval
)
/*++

Routine Description:

  Convert the poll interval from application to that
  be used by EHCI interface data structure. Only need
  to get the max 2^n that is bigger than interval. UEFI
  can't support high speed endpoint with a interval less
  than 8 microframe because interval is specified in
  the unit of ms (millisecond)

Arguments:

  Interval    - The interval to convert
  DeviceSpeed - The speed of device
  Type        - Transfer Type

Returns:

  The converted interval

--*/
{
    UINTN                   BitCount;
    UINTN                   PollingInterval;

    if (Interval == 0) {
        return 1;
    }

    //
    // Find the index (1 based) of the highest non-zero bit
    //
    BitCount = 0;
    PollingInterval = Interval;
    while (PollingInterval != 0) {
        PollingInterval >>= 1;
        BitCount++;
    }
    PollingInterval = (UINTN)1 << (BitCount - 1);
    if (PollingInterval < Interval)
        PollingInterval <<= 1;

    return PollingInterval;

}


VOID
EhcFreeQtds (
    IN USB2_HC_DEV          *Ehc,
    IN LIST_ENTRY       *Qtds
)
/*++

Routine Description:

  Free a list of QTDs

Arguments:

  Ehc   - The EHCI device
  Qtds  - The list head of the QTD

Returns:

  None

--*/
{
    LIST_ENTRY          *Entry;
    LIST_ENTRY          *Next;
    EHC_QTD                 *Qtd;

    EFI_LIST_FOR_EACH_SAFE (Entry, Next, Qtds) {
        Qtd = EFI_LIST_CONTAINER (Entry, EHC_QTD, QtdList);

        RemoveEntryList (&Qtd->QtdList);
        UsbFreePool (Qtd, sizeof (EHC_QTD));
    }
}

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
{
    if (Urb->Qh != NULL) {
        //
        // Ensure that this queue head has been unlinked from the
        // schedule data structures. Free all the associated QTDs
        //
        EhcFreeQtds (Ehc, &Urb->Qh->Qtds);
        UsbFreePool (Urb->Qh, sizeof (EHC_QH));
    }

    FreePool (Urb);
}

EFI_STATUS
EhcCreateQtds (
    IN USB2_HC_DEV          *Ehc,
    IN URB                  *Urb
)
/*++

Routine Description:

  Create a list of QTDs for the URB

Arguments:

  Ehc - The EHCI device
  Urb - The URB to create QTDs for

Returns:

  EFI_OUT_OF_RESOURCES  - Failed to allocate resource for QTD
  EFI_SUCCESS           - The QTDs are allocated for the URB

--*/
{
    USB_ENDPOINT            *Ep;
    EHC_QH                  *Qh;
    EHC_QTD                 *Qtd;
    EHC_QTD                 *StatusQtd;
    EHC_QTD                 *NextQtd;
    LIST_ENTRY              *Entry;
    UINT32                  AlterNext;
    UINT8                   Toggle;
    UINTN                   Len;
    UINT8                   Pid;

    ASSERT ((Urb != NULL) && (Urb->Qh != NULL));

    //
    // EHCI follows the alternet next QTD pointer if it meets
    // a short read and the AlterNext pointer is valid. UEFI
    // EHCI driver should terminate the transfer except the
    // control transfer.
    //
    Toggle    = 0;
    Qh        = Urb->Qh;
    Ep        = &Urb->Ep;
    StatusQtd = NULL;
    AlterNext = QTD_LINK (NULL, TRUE);

    if (Ep->Direction == EfiUsbDataIn) {
        AlterNext = QTD_LINK (Ehc->ShortReadStop, FALSE);
    }

    //
    // Build the Setup and status packets for control transfer
    //
    if (Urb->Ep.Type == EHC_CTRL_TRANSFER) {
        Len = sizeof (EFI_USB_DEVICE_REQUEST);
        Qtd = EhcCreateQtd (Ehc, (UINT8 *)(Urb->Request), Len, QTD_PID_SETUP, 0, Ep->MaxPacket);

        if (Qtd == NULL) {
            return EFI_OUT_OF_RESOURCES;
        }

        InsertTailList (&Qh->Qtds, &Qtd->QtdList);

        //
        // Create the status packet now. Set the AlterNext to it. So, when
        // EHCI meets a short control read, it can resume at the status stage.
        // Use the opposite direction of the data stage, or IN if there is
        // no data stage.
        //
        if (Ep->Direction == EfiUsbDataIn) {
            Pid = QTD_PID_OUTPUT;
        } else {
            Pid = QTD_PID_INPUT;
        }

        StatusQtd = EhcCreateQtd (Ehc, NULL, 0, Pid, 1, Ep->MaxPacket);

        if (StatusQtd == NULL) {
            goto ON_ERROR;
        }

        if (Ep->Direction == EfiUsbDataIn) {
            AlterNext = QTD_LINK (StatusQtd, FALSE);
        }

        Toggle = 1;
    }

    //
    // Build the data packets for all the transfers
    //
    if (Ep->Direction == EfiUsbDataIn) {
        Pid = QTD_PID_INPUT;
    } else {
        Pid = QTD_PID_OUTPUT;
    }

    Qtd = NULL;
    Len = 0;

    while (Len < Urb->DataLen) {
        Qtd = EhcCreateQtd (
                  Ehc,
                  (UINT8 *) Urb->Data + Len,
                  Urb->DataLen - Len,
                  Pid,
                  Toggle,
                  Ep->MaxPacket
              );

        if (Qtd == NULL) {
            goto ON_ERROR;
        }

        if (Urb->Ep.Type == EHC_INT_TRANSFER_ASYNC) {
            Qtd->QtdHw.IOC = 1;
        }

        Qtd->QtdHw.AltNext = AlterNext;
        InsertTailList (&Qh->Qtds, &Qtd->QtdList);

        //
        // Switch the Toggle bit if odd number of packets are included in the QTD.
        //
        if (((Qtd->DataLen + Ep->MaxPacket - 1) / Ep->MaxPacket) % 2) {
            Toggle = 1 - Toggle;
        }

        Len += Qtd->DataLen;
    }

    if (Ep->Type == EHC_INT_ONLY_TRANSFER_ASYNC) {
        Qtd = EhcCreateQtd (
                  Ehc,
                  NULL,
                  0,
                  Pid,
                  Toggle,
                  Ep->MaxPacket
              );
        if (Qtd == NULL) {
            goto ON_ERROR;
        }
        Qtd->QtdHw.ErrCnt = 1;
        Qtd->QtdHw.IOC = 1;
        Qtd->QtdHw.AltNext = AlterNext;
        InsertTailList (&Qh->Qtds, &Qtd->QtdList);
    }

    //
    // Insert the status packet for control transfer
    //
    if (Ep->Type == EHC_CTRL_TRANSFER) {
        InsertTailList (&Qh->Qtds, &StatusQtd->QtdList);
    }

    //
    // OK, all the QTDs needed are created. Now, fix the NextQtd point
    //
    EFI_LIST_FOR_EACH (Entry, &Qh->Qtds) {
        Qtd = EFI_LIST_CONTAINER (Entry, EHC_QTD, QtdList);

        //
        // break if it is the last entry on the list
        //
        if (Entry->ForwardLink == &Qh->Qtds) {
            break;
        }

        NextQtd             = EFI_LIST_CONTAINER (Entry->ForwardLink, EHC_QTD, QtdList);
        Qtd->QtdHw.NextQtd  = QTD_LINK (NextQtd, FALSE);
    }

    //
    // Link the QTDs to the queue head
    //
    NextQtd           = EFI_LIST_CONTAINER (Qh->Qtds.ForwardLink, EHC_QTD, QtdList);
    Qh->QhHw.NextQtd  = QTD_LINK (NextQtd, FALSE);
    return EFI_SUCCESS;

ON_ERROR:
    EhcFreeQtds (Ehc, &Qh->Qtds);
    return EFI_OUT_OF_RESOURCES;
}

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
{
    USB_ENDPOINT                  *Ep;
    EFI_STATUS                    Status;
    URB                           *Urb;

    Urb = AllocateZeroPool (sizeof (URB));
    if (Urb == NULL)
        return NULL;

    Urb->Signature  = EHC_URB_SIG;
    InitializeListHead (&Urb->UrbList);

    Ep              = &Urb->Ep;
    Ep->DevAddr     = DevAddr;
    Ep->EpAddr      = EpAddr & 0x0F;
    Ep->Direction   = ((EpAddr & 0x80) ? EfiUsbDataIn : EfiUsbDataOut);
    Ep->DevSpeed    = DevSpeed;
    Ep->MaxPacket   = MaxPacket;

    Ep->HubAddr     = 0;
    Ep->HubPort     = 0;

    if (DevSpeed != EFI_USB_SPEED_HIGH) {
        ASSERT (Hub != NULL);

        Ep->HubAddr   = Hub->TranslatorHubAddress;
        Ep->HubPort   = Hub->TranslatorPortNumber;
    }

    Ep->Toggle      = Toggle;
    Ep->Type        = Type;
    Ep->PollRate    = EhcConvertPollRate (Interval);

    Urb->Request    = Request;
    Urb->Data       = Data;
    Urb->DataLen    = DataLen;
    Urb->Callback   = Callback;
    Urb->Context    = Context;

    Urb->Qh         = EhcCreateQh (Ehc, &Urb->Ep);
    if (Urb->Qh == NULL) {
        goto ON_ERROR;
    }

    Status = EhcCreateQtds (Ehc, Urb);

    if (EFI_ERROR (Status)) {
        goto ON_ERROR;
    }

    return Urb;

ON_ERROR:
    EhcFreeUrb (Ehc, Urb);
    return NULL;
}

/******************************************************************************
    Schedule function
/*****************************************************************************/
