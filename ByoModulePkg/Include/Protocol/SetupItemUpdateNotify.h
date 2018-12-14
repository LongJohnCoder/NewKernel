#ifndef __SETUP_ITEM_UPDATE_NOTIFY_H__
#define __SETUP_ITEM_UPDATE_NOTIFY_H__

#include <Uefi.h>

// {E4154CBF-B17E-44ec-A233-2CD7BFFCF791}
#define SETUP_ITEM_UPDATE_NOTIFY_GUID \
{ 0xe4154cbf, 0xb17e, 0x44ec, { 0xa2, 0x33, 0x2c, 0xd7, 0xbf, 0xfc, 0xf7, 0x91 }}


typedef struct _SETUP_ITEM_UPDATE_NOTIFY_PROTOCOL  SETUP_ITEM_UPDATE_NOTIFY_PROTOCOL;


#define ITEM_SECUREBOOT            1
#define ITEM_CSM                   2

#define STATUS_ENABLE              1
#define STATUS_DISABLE             0

typedef
EFI_STATUS
(EFIAPI *SETUP_ITEM_UPDATE_NOTIFY)(
  IN SETUP_ITEM_UPDATE_NOTIFY_PROTOCOL    *This,
  IN UINTN                                Item,
  IN UINTN                                NewStatus
  );


struct _SETUP_ITEM_UPDATE_NOTIFY_PROTOCOL {
  SETUP_ITEM_UPDATE_NOTIFY  Notify;
};
  
extern EFI_GUID gSetupItemUpdateNotifyProtocolGuid;
  
#endif