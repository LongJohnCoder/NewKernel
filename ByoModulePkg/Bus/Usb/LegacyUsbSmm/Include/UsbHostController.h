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
  UsbHostController.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _USB_HOSTCONTROLLER_H_
#define _USB_HOSTCONTROLLER_H_

#include "usb.h"
#include "UsbIo.h"



#define EFI_USB_HC_PROTOCOL_GUID \
  { \
    0xf5089266, 0x1aa0, 0x4953, 0x97, 0xd8, 0x56, 0x2f, 0x8a, 0x73, 0xb5, 0x19 \
  }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_USB_HC_PROTOCOL     EFI_USB_HC_PROTOCOL;

typedef enum {
    EfiUsbHcStateHalt,
    EfiUsbHcStateOperational,
    EfiUsbHcStateSuspend,
    EfiUsbHcStateMaximum
} EFI_USB_HC_STATE;

#define EFI_USB_HC_RESET_GLOBAL           0x0001
#define EFI_USB_HC_RESET_HOST_CONTROLLER  0x0002
#define EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG  0x0004
#define EFI_USB_HC_RESET_HOST_WITH_DEBUG  0x0008

typedef enum {
    EfiVirtualOn,
    EfiVirtualOff,
    EfiVirtualStatus,
    EfiVirtualClearStatus,
    EfiVirtualControlReg,
    EfiVirtualStatusReg,
    EfiVirtualInputData,
    EfiVirtualOuputData
} EFI_USB_VIRTUAL_FEATURE;

//
// Protocol definitions
//
typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_RESET) (
    IN EFI_USB_HC_PROTOCOL    * This,
    IN UINT16                 Attributes
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_GET_STATE) (
    IN  EFI_USB_HC_PROTOCOL    * This,
    OUT EFI_USB_HC_STATE       * State
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_SET_STATE) (
    IN EFI_USB_HC_PROTOCOL    * This,
    IN EFI_USB_HC_STATE       State
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_CONTROL_TRANSFER) (
    IN     EFI_USB_HC_PROTOCOL       * This,
    IN     UINT8                     DeviceAddress,
    IN     BOOLEAN                   IsSlowDevice,
    IN     UINT8                     MaximumPacketLength,
    IN     EFI_USB_DEVICE_REQUEST    * Request,
    IN     EFI_USB_DATA_DIRECTION    TransferDirection,
    IN OUT VOID                      *Data OPTIONAL,
    IN OUT UINTN                     *DataLength OPTIONAL,
    IN     UINTN                     TimeOut,
    OUT    UINT32                    *TransferResult
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_BULK_TRANSFER) (
    IN     EFI_USB_HC_PROTOCOL    * This,
    IN     UINT8                  DeviceAddress,
    IN     UINT8                  EndPointAddress,
    IN     UINT8                  MaximumPacketLength,
    IN OUT VOID                   *Data,
    IN OUT UINTN                  *DataLength,
    IN OUT UINT8                  *DataToggle,
    IN     UINTN                  TimeOut,
    OUT    UINT32                 *TransferResult
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_ASYNC_INTERRUPT_TRANSFER) (
    IN     EFI_USB_HC_PROTOCOL                                 * This,
    IN     UINT8                                               DeviceAddress,
    IN     UINT8                                               EndPointAddress,
    IN     BOOLEAN                                             IsSlowDevice,
    IN     UINT8                                               MaxiumPacketLength,
    IN     BOOLEAN                                             IsNewTransfer,
    IN OUT UINT8                                               *DataToggle,
    IN     UINTN                                               PollingInterval  OPTIONAL,
    IN     UINTN                                               DataLength       OPTIONAL,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK                     CallBackFunction OPTIONAL,
    IN     VOID                                                *Context OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_ASYNC_INTERRUPT_ONLY_TRANSFER) (
    IN     EFI_USB_HC_PROTOCOL                                 * This,
    IN     BOOLEAN                                             IsNewTransfer,
    IN     UINTN                                               PollingInterva,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK                     CallBackFunction,
    IN     VOID                                                *Context
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_SYNC_INTERRUPT_TRANSFER) (
    IN     EFI_USB_HC_PROTOCOL    * This,
    IN     UINT8                  DeviceAddress,
    IN     UINT8                  EndPointAddress,
    IN     BOOLEAN                IsSlowDevice,
    IN     UINT8                  MaximumPacketLength,
    IN OUT VOID                   *Data,
    IN OUT UINTN                  *DataLength,
    IN OUT UINT8                  *DataToggle,
    IN     UINTN                  TimeOut,
    OUT    UINT32                 *TransferResult
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_ISOCHRONOUS_TRANSFER) (
    IN     EFI_USB_HC_PROTOCOL    * This,
    IN     UINT8                  DeviceAddress,
    IN     UINT8                  EndPointAddress,
    IN     UINT8                  MaximumPacketLength,
    IN OUT VOID                   *Data,
    IN     UINTN                  DataLength,
    OUT    UINT32                 *TransferResult
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_ASYNC_ISOCHRONOUS_TRANSFER) (
    IN     EFI_USB_HC_PROTOCOL                * This,
    IN     UINT8                              DeviceAddress,
    IN     UINT8                              EndPointAddress,
    IN     UINT8                              MaximumPacketLength,
    IN OUT VOID                               *Data,
    IN     UINTN                              DataLength,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    IsochronousCallBack,
    IN     VOID                               *Context OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_GET_ROOTHUB_PORT_NUMBER) (
    IN EFI_USB_HC_PROTOCOL    * This,
    OUT UINT8                 *PortNumber
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_GET_ROOTHUB_PORT_STATUS) (
    IN EFI_USB_HC_PROTOCOL     * This,
    IN  UINT8                  PortNumber,
    OUT EFI_USB_PORT_STATUS    * PortStatus
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_SET_ROOTHUB_PORT_FEATURE) (
    IN EFI_USB_HC_PROTOCOL     * This,
    IN UINT8                   PortNumber,
    IN EFI_USB_PORT_FEATURE    PortFeature
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_CLEAR_ROOTHUB_PORT_FEATURE) (
    IN EFI_USB_HC_PROTOCOL     * This,
    IN UINT8                   PortNumber,
    IN EFI_USB_PORT_FEATURE    PortFeature
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_DISABLE_LEGACY_SUPPORTE) (
    IN EFI_USB_HC_PROTOCOL     * This
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_ENABLE_LEGACY_SUPPORT) (
    IN EFI_USB_HC_PROTOCOL     * This
);

typedef
VOID
(EFIAPI *EFI_USB_HC_LEGACY_CALLBACK) (
    IN EFI_USB_HC_PROTOCOL     * This
);

typedef
VOID
(EFIAPI *EFI_USB_BUS_ENUMERATION_CALLBACK) (
    IN VOID                   *  Context
);

typedef struct _EFI_USB_HC_PROTOCOL {
    EFI_USB_HC_PROTOCOL_RESET                       Reset;
    EFI_USB_HC_PROTOCOL_GET_STATE                   GetState;
    EFI_USB_HC_PROTOCOL_SET_STATE                   SetState;
    EFI_USB_HC_PROTOCOL_CONTROL_TRANSFER            ControlTransfer;
    EFI_USB_HC_PROTOCOL_BULK_TRANSFER               BulkTransfer;
    EFI_USB_HC_PROTOCOL_ASYNC_INTERRUPT_TRANSFER    AsyncInterruptTransfer;
    EFI_USB_HC_PROTOCOL_SYNC_INTERRUPT_TRANSFER     SyncInterruptTransfer;
    EFI_USB_HC_PROTOCOL_ISOCHRONOUS_TRANSFER        IsochronousTransfer;
    EFI_USB_HC_PROTOCOL_ASYNC_ISOCHRONOUS_TRANSFER  AsyncIsochronousTransfer;
    EFI_USB_HC_PROTOCOL_GET_ROOTHUB_PORT_NUMBER     GetRootHubPortNumber;
    EFI_USB_HC_PROTOCOL_GET_ROOTHUB_PORT_STATUS     GetRootHubPortStatus;
    EFI_USB_HC_PROTOCOL_SET_ROOTHUB_PORT_FEATURE    SetRootHubPortFeature;
    EFI_USB_HC_PROTOCOL_CLEAR_ROOTHUB_PORT_FEATURE  ClearRootHubPortFeature;
    UINT16                                          MajorRevision;
    UINT16                                          MinorRevision;

    EFI_USB_HC_PROTOCOL_ASYNC_INTERRUPT_ONLY_TRANSFER AsyncInterruptOnlyTransfer;
    //
    // Here we added one extension for legacy support
    //
    EFI_USB_HC_PROTOCOL_DISABLE_LEGACY_SUPPORTE     DisableLegacySupport;
    EFI_USB_HC_PROTOCOL_ENABLE_LEGACY_SUPPORT       EnableLegacySupport;
    EFI_USB_HC_LEGACY_CALLBACK                      LegacyCallback;
    EFI_USB_BUS_ENUMERATION_CALLBACK                BusEnumerationCallback;
    VOID                                            *EnumerationContext;
} EFI_USB_HC_PROTOCOL;

extern EFI_GUID gEfiUsbHcProtocolGuid;


#define EFI_USB2_HC_PROTOCOL_GUID \
  { \
    0x3e745226, 0x9818, 0x45b6, 0xa2, 0xac, 0xd7, 0xcd, 0xe, 0x8b, 0xa2, 0xbc \
  }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_USB2_HC_PROTOCOL     EFI_USB2_HC_PROTOCOL;

#define EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG 0x0004
#define EFI_USB_HC_RESET_HOST_WITH_DEBUG   0x0008

typedef struct {
    UINT8      TranslatorHubAddress;
    UINT8      TranslatorPortNumber;
} EFI_USB2_HC_TRANSACTION_TRANSLATOR;

//
// Protocol definitions
//
typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_GET_CAPABILITY) (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    OUT UINT8                 *MaxSpeed,
    OUT UINT8                 *PortNumber,
    OUT UINT8                 *Is64BitCapable
);

#define EFI_USB_SPEED_FULL 0x0000
#define EFI_USB_SPEED_LOW  0x0001
#define EFI_USB_SPEED_HIGH 0x0002
#define EFI_USB_SPEED_SUPER 0x0003

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_RESET) (
    IN EFI_USB2_HC_PROTOCOL   *This,
    IN UINT16                 Attributes
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_GET_STATE) (
    IN  EFI_USB2_HC_PROTOCOL    *This,
    OUT EFI_USB_HC_STATE        *State
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_SET_STATE) (
    IN EFI_USB2_HC_PROTOCOL    *This,
    IN EFI_USB_HC_STATE        State
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_CONTROL_TRANSFER) (
    IN     EFI_USB2_HC_PROTOCOL               *This,
    IN     UINT8                              DeviceAddress,
    IN     UINT8                              DeviceSpeed,
    IN     UINTN                              MaximumPacketLength,
    IN     EFI_USB_DEVICE_REQUEST             *Request,
    IN     EFI_USB_DATA_DIRECTION             TransferDirection,
    IN OUT VOID                               *Data       OPTIONAL,
    IN OUT UINTN                              *DataLength OPTIONAL,
    IN     UINTN                              TimeOut,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
    OUT    UINT32                             *TransferResult
);

#define EFI_USB_MAX_BULK_BUFFER_NUM 10

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_BULK_TRANSFER) (
    IN     EFI_USB2_HC_PROTOCOL               *This,
    IN     UINT8                              DeviceAddress,
    IN     UINT8                              EndPointAddress,
    IN     UINT8                              DeviceSpeed,
    IN     UINTN                              MaximumPacketLength,
    IN     UINT8                              DataBuffersNumber,
    IN OUT VOID                               *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
    IN OUT UINTN                              *DataLength,
    IN OUT UINT8                              *DataToggle,
    IN     UINTN                              TimeOut,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
    OUT    UINT32                             *TransferResult
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_ASYNC_INTERRUPT_TRANSFER) (
    IN     EFI_USB2_HC_PROTOCOL                                *This,
    IN     UINT8                                               DeviceAddress,
    IN     UINT8                                               EndPointAddress,
    IN     UINT8                                               DeviceSpeed,
    IN     UINTN                                               MaxiumPacketLength,
    IN     BOOLEAN                                             IsNewTransfer,
    IN OUT UINT8                                               *DataToggle,
    IN     UINTN                                               PollingInterval  OPTIONAL,
    IN     UINTN                                               DataLength       OPTIONAL,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK                     CallBackFunction OPTIONAL,
    IN     VOID                                                *Context         OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_ASYNC_INTERRUPT_ONLY_TRANSFER) (
    IN     EFI_USB2_HC_PROTOCOL               *This,
    IN     BOOLEAN                            IsNewTransfer,
    IN     UINTN                              PollingInterval,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,
    IN     VOID                               *Context
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_SYNC_INTERRUPT_TRANSFER) (
    IN     EFI_USB2_HC_PROTOCOL   *This,
    IN     UINT8                  DeviceAddress,
    IN     UINT8                  EndPointAddress,
    IN     UINT8                  DeviceSpeed,
    IN     UINTN                  MaximumPacketLength,
    IN OUT VOID                   *Data,
    IN OUT UINTN                  *DataLength,
    IN OUT UINT8                  *DataToggle,
    IN     UINTN                  TimeOut,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
    OUT    UINT32                 *TransferResult
);

#define EFI_USB_MAX_ISO_BUFFER_NUM  7
#define EFI_USB_MAX_ISO_BUFFER_NUM1 2

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_ISOCHRONOUS_TRANSFER) (
    IN     EFI_USB2_HC_PROTOCOL               *This,
    IN     UINT8                              DeviceAddress,
    IN     UINT8                              EndPointAddress,
    IN     UINT8                              DeviceSpeed,
    IN     UINTN                              MaximumPacketLength,
    IN     UINT8                              DataBuffersNumber,
    IN OUT VOID                               *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
    IN     UINTN                              DataLength,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
    OUT    UINT32                             *TransferResult
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_ASYNC_ISOCHRONOUS_TRANSFER) (
    IN     EFI_USB2_HC_PROTOCOL               *This,
    IN     UINT8                              DeviceAddress,
    IN     UINT8                              EndPointAddress,
    IN     UINT8                              DeviceSpeed,
    IN     UINTN                              MaximumPacketLength,
    IN     UINT8                              DataBuffersNumber,
    IN OUT VOID                               *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
    IN     UINTN                              DataLength,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    IsochronousCallBack,
    IN     VOID                               *Context OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_GET_ROOTHUB_PORT_STATUS) (
    IN EFI_USB2_HC_PROTOCOL    *This,
    IN  UINT8                  PortNumber,
    OUT EFI_USB_PORT_STATUS    *PortStatus
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_SET_ROOTHUB_PORT_FEATURE) (
    IN EFI_USB2_HC_PROTOCOL    *This,
    IN UINT8                   PortNumber,
    IN EFI_USB_PORT_FEATURE    PortFeature
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_CLEAR_ROOTHUB_PORT_FEATURE) (
    IN EFI_USB2_HC_PROTOCOL    *This,
    IN UINT8                   PortNumber,
    IN EFI_USB_PORT_FEATURE    PortFeature
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_GSET_VIRTUAL_STATE) (
    IN  EFI_USB2_HC_PROTOCOL      * This,
    IN  EFI_USB_VIRTUAL_FEATURE   VirtualFeature,
    IN  UINT8                     *State,
    IN  BOOLEAN                   RWFlag
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_DISABLE_LEGACY_SUPPORTE) (
    IN EFI_USB2_HC_PROTOCOL     * This
);

typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_ENABLE_LEGACY_SUPPORT) (
    IN EFI_USB2_HC_PROTOCOL     * This
);

typedef
VOID
(EFIAPI *EFI_USB2_HC_LEGACY_CALLBACK) (
    IN EFI_USB2_HC_PROTOCOL     * This
);

typedef struct _EFI_USB2_HC_PROTOCOL {
    EFI_USB2_HC_PROTOCOL_GET_CAPABILITY              GetCapability;
    EFI_USB2_HC_PROTOCOL_RESET                       Reset;
    EFI_USB2_HC_PROTOCOL_GET_STATE                   GetState;
    EFI_USB2_HC_PROTOCOL_SET_STATE                   SetState;
    EFI_USB2_HC_PROTOCOL_CONTROL_TRANSFER            ControlTransfer;
    EFI_USB2_HC_PROTOCOL_BULK_TRANSFER               BulkTransfer;
    EFI_USB2_HC_PROTOCOL_ASYNC_INTERRUPT_TRANSFER    AsyncInterruptTransfer;
    EFI_USB2_HC_PROTOCOL_SYNC_INTERRUPT_TRANSFER     SyncInterruptTransfer;
    EFI_USB2_HC_PROTOCOL_ISOCHRONOUS_TRANSFER        IsochronousTransfer;
    EFI_USB2_HC_PROTOCOL_ASYNC_ISOCHRONOUS_TRANSFER  AsyncIsochronousTransfer;
    EFI_USB2_HC_PROTOCOL_GET_ROOTHUB_PORT_STATUS     GetRootHubPortStatus;
    EFI_USB2_HC_PROTOCOL_SET_ROOTHUB_PORT_FEATURE    SetRootHubPortFeature;
    EFI_USB2_HC_PROTOCOL_CLEAR_ROOTHUB_PORT_FEATURE  ClearRootHubPortFeature;
    UINT16                                           MajorRevision;
    UINT16                                           MinorRevision;

    // Add one special kind of interrupt transfer for polling
    EFI_USB2_HC_PROTOCOL_ASYNC_INTERRUPT_ONLY_TRANSFER AsyncInterruptOnlyTransfer;

    //
    // Here we added one extension for legacy support
    //
    EFI_USB2_HC_PROTOCOL_GSET_VIRTUAL_STATE          GSetVirtualState;
    EFI_USB2_HC_PROTOCOL_DISABLE_LEGACY_SUPPORTE     DisableLegacySupport;
    EFI_USB2_HC_PROTOCOL_ENABLE_LEGACY_SUPPORT       EnableLegacySupport;
    EFI_USB2_HC_LEGACY_CALLBACK                      LegacyCallback;
    EFI_USB_BUS_ENUMERATION_CALLBACK                 BusEnumerationCallback;
    VOID                                             *EnumerationContext;
} EFI_USB2_HC_PROTOCOL;

extern EFI_GUID gEfiUsb2HcProtocolGuid;

#endif
