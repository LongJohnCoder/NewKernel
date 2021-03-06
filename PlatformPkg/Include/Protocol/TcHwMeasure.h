
#ifndef __TC_HW_MEASURE_H__
#define __TC_HW_MEASURE_H__

#define EFI_TC_HW_MEASURE_PROTOCOL_GUID \
  { 0x811f95db, 0x9f60, 0x4df6, { 0x86, 0xe9, 0x2, 0x8b, 0xb1, 0x26, 0x11, 0x1c } }

typedef struct _EFI_TC_HW_MEASURE_PROTOCOL  EFI_TC_HW_MEASURE_PROTOCOL;

typedef enum {
  EFI_TC_HW_MEASURE_ITEM_FVMAIN,
  EFI_TC_HW_MEASURE_ITEM_VBIOS,
  EFI_TC_HW_MEASURE_ITEM_HDD_SN,
  EFI_TC_HW_MEASURE_ITEM_PCI,
  EFI_TC_HW_MEASURE_ITEM_OS_FILE
} EFI_TC_HW_MEASURE_ITEM;

typedef
EFI_STATUS
(EFIAPI *EFI_TC_HW_MEASURE_PROTOCOL_MEASURE)(
  IN EFI_TC_HW_MEASURE_PROTOCOL    *This,
  IN EFI_TC_HW_MEASURE_ITEM        Item
  );

struct _EFI_TC_HW_MEASURE_PROTOCOL {
  EFI_TC_HW_MEASURE_PROTOCOL_MEASURE  Check;
};

extern EFI_GUID gEfiTcHwMeasureProtocolGuid;

#endif