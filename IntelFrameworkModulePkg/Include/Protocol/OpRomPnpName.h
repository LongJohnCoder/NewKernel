
#ifndef __MY_OPROM_PNP_NAME_H__
#define __MY_OPROM_PNP_NAME_H__

// {5BD912E9-E82E-451f-BCE9-92E93E748BC9}
#define EFI_OPROM_PNP_NAME_PROTOCOL_GUID \
{ 0x5bd912e9, 0xe82e, 0x451f, { 0xbc, 0xe9, 0x92, 0xe9, 0x3e, 0x74, 0x8b, 0xc9 } }

extern EFI_GUID gOpromPnpNameProtocolGuid;


typedef struct {
  CHAR16  Name[64];
  UINT16  BBSIndex;
} OPROM_PNP_DEV_NAME;

#endif