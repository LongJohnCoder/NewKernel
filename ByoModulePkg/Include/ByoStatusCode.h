/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  ByoStatusCode.h

Abstract: 
  Status code definition.

Revision History:

Bug 2517:   Create the Module StatusCodeHandler to report status code to 
            all supported devide in ByoModule
TIME:       2011-7-22
$AUTHOR:    Liu Chunling
$REVIEWERS:  
$SCOPE:     All Platforms
$TECHNICAL:  
  1. Create the module StatusCodeHandler to support Serial Port, Memory, Port80,
     Beep and OEM devices to report status code.
  2. Create the Port80 map table and the Beep map table to convert status code 
     to code byte and beep times.
  3. Create new libraries to support status code when StatusCodePpi,
     StatusCodeRuntimeProtocol, SmmStatusCodeProtocol has not been installed yet.
$END--------------------------------------------------------------------

**/


#ifndef __BYO_STATUS_CODE_HANDLER_H__
#define __BYO_STATUS_CODE_HANDLER_H__

#include <Pi\PiStatusCode.h>
#include <Framework\StatusCode.h>

typedef struct{
  EFI_STATUS_CODE_VALUE Value;
  UINT32                Data;
} STATUS_CODE_TO_DATA_MAP;

//                       
// Enable PEI/DXE status code
//                         
#define PEI_STATUS_CODE 1  
#define DXE_STATUS_CODE 1  
  
#define STATUS_CODE_TYPE(Type)                ((Type)&EFI_STATUS_CODE_TYPE_MASK)
#define STATUS_CODE_CLASS(Value)              ((Value)&EFI_STATUS_CODE_CLASS_MASK)

//Progress/Error codes
#define PEI_CORE_STARTED                      (EFI_SOFTWARE_PEI_CORE | EFI_SW_PEI_CORE_PC_ENTRY_POINT)         
#define PEI_RESET_NOT_AVAILABLE               (EFI_SOFTWARE_PEI_CORE | EFI_SW_PS_EC_RESET_NOT_AVAILABLE)       
#define PEI_DXEIPL_NOT_FOUND                  (EFI_SOFTWARE_PEI_CORE | EFI_SW_PEI_CORE_EC_DXEIPL_NOT_FOUND)    
#define PEI_DXE_CORE_NOT_FOUND                (EFI_SOFTWARE_PEI_CORE | EFI_SW_PEI_CORE_EC_DXE_CORRUPT)         
#define PEI_S3_RESUME_ERROR	                  (EFI_SOFTWARE_PEI_CORE | EFI_SW_PEI_EC_S3_RESUME_FAILED)         
#define PEI_RECOVERY_FAILED                   (EFI_SOFTWARE_PEI_CORE | EFI_SW_PEI_EC_RECOVERY_FAILED)          
#define DXE_CORE_STARTED                      (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_ENTRY_POINT)
      
//#define DXE_EXIT_BOOT_SERVICES_BEGIN 0xF8
#define DXE_EXIT_BOOT_SERVICES_END            (EFI_SOFTWARE_EFI_BOOT_SERVICE | EFI_SW_BS_PC_EXIT_BOOT_SERVICES)
                                              
// Reported by CPU PEIM                       
#define PEI_CAR_CPU_INIT                      (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_POWER_ON_INIT)
                                              
// Reported by NB PEIM                        
#define PEI_CAR_NB_INIT                       (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_PC_PEI_CAR_NB_INIT)
                                              
// Reported by SB PEIM                        
#define PEI_CAR_SB_INIT                       (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_PC_PEI_CAR_SB_INIT)

//Reported by Memory Detection PEIM
#define PEI_MEMORY_SPD_READ                   (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_SPD_READ)
#define PEI_MEMORY_PRESENCE_DETECT            (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_PRESENCE_DETECT)
#define PEI_MEMORY_TIMING                     (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_TIMING)
#define PEI_MEMORY_CONFIGURING                (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_CONFIGURING)
#define PEI_MEMORY_OPTIMIZING                 (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_OPTIMIZING)
#define PEI_MEMORY_INIT                       (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_INIT)
#define PEI_MEMORY_TEST                       (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_TEST)
#define PEI_MEMORY_INVALID_TYPE               (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_INVALID_TYPE)
#define PEI_MEMORY_INVALID_SPEED              (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_INVALID_SPEED)
#define PEI_MEMORY_SPD_FAIL                   (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_SPD_FAIL)
#define PEI_MEMORY_INVALID_SIZE               (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_INVALID_SIZE)
#define PEI_MEMORY_MISMATCH                   (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_MISMATCH)
#define PEI_MEMORY_S3_RESUME_FAILED           (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_S3_RESUME_FAIL)
#define PEI_MEMORY_NOT_DETECTED               (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_NONE_DETECTED)
#define PEI_MEMORY_NONE_USEFUL                (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_NONE_USEFUL)
#define PEI_MEMORY_ERROR                      (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_EC_NON_SPECIFIC)
#define PEI_MEMORY_INSTALLED                  (EFI_SOFTWARE_PEI_SERVICE  | EFI_SW_PS_PC_INSTALL_PEI_MEMORY)
#define PEI_MEMORY_NOT_INSTALLED              (EFI_SOFTWARE_PEI_SERVICE  | EFI_SW_PEI_CORE_EC_MEMORY_NOT_INSTALLED)
#define PEI_MEMORY_INSTALLED_TWICE            (EFI_SOFTWARE_PEI_SERVICE  | EFI_SW_PS_EC_MEMORY_INSTALLED_TWICE)

//Reported by CPU DXE
#define DXE_CPU_INIT                          (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_PC_INIT_BEGIN)
#define DXE_CPU_CACHE_INIT                    (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_CACHE_INIT)
#define DXE_CPU_BSP_SELECT                    (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_BSP_SELECT)
#define DXE_CPU_AP_INIT                       (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_AP_INIT)
#define DXE_CPU_SMM_INIT                      (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_SMM_INIT)
#define DXE_CPU_INVALID_TYPE                  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_INVALID_TYPE)
#define DXE_CPU_INVALID_SPEED                 (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_INVALID_SPEED)
#define DXE_CPU_MISMATCH                      (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_MISMATCH)
#define DXE_CPU_SELF_TEST_FAILED              (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_SELF_TEST)
#define DXE_CPU_CACHE_ERROR                   (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_CACHE)
#define DXE_CPU_MICROCODE_UPDATE_FAILED       (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_MICROCODE_UPDATE)
#define DXE_CPU_NO_MICROCODE                  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_NO_MICROCODE_UPDATE)
//If non of the errors above apply use this one
#define DXE_CPU_INTERNAL_ERROR                (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_INTERNAL)
//Generic CPU error. It should only be used if non of the errors above apply
#define DXE_CPU_ERROR                         (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_EC_NON_SPECIFIC)
                                              
// Reported by NB PEIM                        
#define PEI_MEM_NB_INIT                       (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_PC_PEI_MEM_NB_INIT)
// Reported by SB PEIM                        
#define PEI_MEM_SB_INIT                       (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_PC_PEI_MEM_SB_INIT)

//Reported by PEIM which detected forced or auto recovery condition
#define PEI_RECOVERY_AUTO                     (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_RECOVERY_AUTO)
#define PEI_RECOVERY_USER                     (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_RECOVERY_USER)
                                              
//Reported by DXE IPL                         
#define PEI_RECOVERY_PPI_NOT_FOUND            (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_RECOVERY_PPI_NOT_FOUND)
#define PEI_S3_RESUME_PPI_NOT_FOUND           (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_S3_RESUME_PPI_NOT_FOUND)
#define PEI_S3_RESUME_FAILED                  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_S3_RESUME_FAILED)

//Reported by Recovery PEIM
#define PEI_RECOVERY_STARTED                  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_RECOVERY_BEGIN)
#define PEI_RECOVERY_CAPSULE_FOUND            (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_CAPSULE_LOAD)
#define PEI_RECOVERY_NO_CAPSULE               (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_NO_RECOVERY_CAPSULE)
#define PEI_RECOVERY_CAPSULE_LOADED           (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_CAPSULE_START)
#define PEI_RECOVERY_INVALID_CAPSULE          (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_INVALID_CAPSULE_DESCRIPTOR)

// Reported by SmmPlatform and SB SMM driver                  
#define PEI_S3_SUSPEND_STARTED                 (EFI_SOFTWARE_SMM_DRIVER | (EFI_OEM_SPECIFIC | 0x00000000))
#define PEI_S3_SUSPEND_ENDED                   (EFI_SOFTWARE_SMM_DRIVER | (EFI_OEM_SPECIFIC | 0x00000001))
                                              
// Reported by S3 Resume PEIM                  
#define PEI_S3_RESUME_STARTED                 (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_S3_STARTED)
#define PEI_S3_BOOT_SCRIPT                    (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_S3_BOOT_SCRIPT)
#define PEI_S3_OS_WAKE                        (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_OS_WAKE)
#define PEI_S3_BOOT_SCRIPT_ERROR              (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_S3_BOOT_SCRIPT_ERROR)
#define PEI_S3_OS_WAKE_ERROR                  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_S3_OS_WAKE_ERROR)
                                              
#define PEI_PEIM_STARTED                      (EFI_SOFTWARE_PEI_CORE | EFI_SW_PC_INIT_BEGIN)
#define PEI_PEIM_ENDED                        (EFI_SOFTWARE_PEI_CORE | EFI_SW_PC_INIT_END)
                                              
//Reported by DXE IPL                         
#define PEI_DXE_IPL_STARTED                   (EFI_SOFTWARE_PEI_CORE | EFI_SW_PEI_CORE_PC_HANDOFF_TO_NEXT)

//Reported by PEIM which installs Reset PPI
#define PEI_RESET_SYSTEM                      (EFI_SOFTWARE_PEI_SERVICE | EFI_SW_PS_PC_RESET_SYSTEM)

//Reported by the PEIM or DXE driver which detected the error
#define GENERIC_MEMORY_CORRECTABLE_ERROR      (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_CORRECTABLE)
#define GENERIC_MEMORY_UNCORRECTABLE_ERROR    (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_UNCORRECTABLE)

//Reported by Flash Update DXE driver
#define DXE_FLASH_UPDATE_FAILED               (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_UPDATE_FAIL)

//Reported by the PEIM or DXE driver which detected the error
#define GENERIC_CPU_THERMAL_ERROR             (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_THERMAL)
#define GENERIC_CPU_LOW_VOLTAGE               (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_LOW_VOLTAGE)
#define GENERIC_CPU_HIGH_VOLTAGE              (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_HIGH_VOLTAGE)
#define GENERIC_CPU_CORRECTABLE_ERROR         (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_CORRECTABLE)
#define GENERIC_CPU_UNCORRECTABLE_ERROR       (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_UNCORRECTABLE)
#define GENERIC_BAD_DATE_TIME_ERROR           (EFI_SOFTWARE_UNSPECIFIED | EFI_SW_EC_BAD_DATE_TIME)
#define GENERIC_MEMORY_SIZE_DECREASE          (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_MISMATCH)

//Reported by DXE Core
#define DXE_DRIVER_STARTED                    (EFI_SOFTWARE_EFI_DXE_SERVICE | EFI_SW_PC_INIT_BEGIN)
#define DXE_DRIVER_ENED                       (EFI_SOFTWARE_DXE_CORE | EFI_SW_PC_INIT_END)
#define DXE_ARCH_PROTOCOLS_AVAILABLE          (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_ARCH_READY)
#define DXE_DRIVER_CONNECTED                  (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_START_DRIVER)
#define DXE_ARCH_PROTOCOL_NOT_AVAILABLE       (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_EC_NO_ARCH)

//Reported by DXE CPU driver
#define DXE_CPU_SELF_TEST_FAILED              (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_SELF_TEST)

//Reported by PCI Host Bridge driver
#define DXE_NB_HB_INIT                        (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_PC_DXE_HB_INIT)

// Reported by NB Driver
#define DXE_NB_INIT                           (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_PC_DXE_NB_INIT )
#define DXE_NB_SMM_INIT                       (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_PC_DXE_NB_SMM_INIT )
#define DXE_NB_ERROR                          (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_EC_DXE_NB_ERROR )

// Reported by SB Driver(s)
#define DXE_SBRUN_INIT                        (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_PC_DXE_SB_RT_INIT )
#define DXE_SB_INIT                           (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_PC_DXE_SB_INIT )
#define DXE_SB_SMM_INIT                       (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_PC_DXE_SB_SMM_INIT )
#define DXE_SB_DEVICES_INIT                   (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_PC_DXE_SB_DEVICES_INIT )
#define DXE_SB_BAD_BATTERY                    (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_EC_BAD_BATTERY)
#define DXE_SB_ERROR                          (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_EC_DXE_SB_ERROR )

//Reported by DXE Core
#define DXE_BDS_STARTED                       (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_HANDOFF_TO_NEXT)

//Reported by BDS
#define DXE_BDS_CONNECT_DRIVERS               (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_BEGIN_CONNECTING_DRIVERS)
#define DXE_BDS_LENOVO_COMMON_ERROR           (EFI_SOFTWARE_DXE_BS_DRIVER | BYO_EC_LENOVO_COMMON_ERROR)

//Reported by Boot Manager
#define DXE_READY_TO_BOOT                     (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_READY_TO_BOOT_EVENT)

//Reported by DXE Core
#define DXE_EXIT_BOOT_SERVICES                (EFI_SOFTWARE_EFI_BOOT_SERVICE | EFI_SW_BS_PC_EXIT_BOOT_SERVICES)
#define DXE_EXIT_BOOT_SERVICES_EVENT          (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_EXIT_BOOT_SERVICES_EVENT)

//Reported by driver that installs Runtime AP
#define RT_SET_VIRTUAL_ADDRESS_MAP_BEGIN      (EFI_SOFTWARE_EFI_RUNTIME_SERVICE | EFI_SW_RS_PC_SET_VIRTUAL_ADDRESS_MAP)
#define RT_SET_VIRTUAL_ADDRESS_MAP_END        (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_VIRTUAL_ADDRESS_CHANGE_EVENT)

// Reported by CSM
#define DXE_LEGACY_OPROM_INIT                 (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_LEGACY_OPROM_INIT)
#define DXE_LEGACY_BOOT                       (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_LEGACY_BOOT_EVENT)
#define DXE_LEGACY_OPROM_NO_SPACE             (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_EC_LEGACY_OPROM_NO_SPACE)

//Reported by SETUP
//#define DXE_SETUP_VERIFYING_PASSWORD        (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_VERIFYING_PASSWORD)
#define DXE_SETUP_START                       (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_USER_SETUP)
#define DXE_SETUP_INPUT_WAIT                  (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_INPUT_WAIT)
#define DXE_INVALID_PASSWORD                  (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_EC_INVALID_PASSWORD)
#define DXE_INVALID_IDE_PASSWORD              (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_EC_INVALID_IDE_PASSWORD)
#define DXE_BOOT_OPTION_LOAD_ERROR            (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_EC_BOOT_OPTION_LOAD_ERROR)
#define DXE_BOOT_OPTION_FAILED                (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_EC_BOOT_OPTION_FAILED)

// Reported by a Driver that installs Reset AP
#define DXE_RESET_SYSTEM                      (EFI_SOFTWARE_EFI_RUNTIME_SERVICE | EFI_SW_RS_PC_RESET_SYSTEM)
#define DXE_RESET_NOT_AVAILABLE               (EFI_SOFTWARE_EFI_RUNTIME_SERVICE | EFI_SW_PS_EC_RESET_NOT_AVAILABLE)

// Reported by variable service driver
#define DXE_VARIABLE_INIT                     (EFI_SOFTWARE_EFI_RUNTIME_SERVICE | BYO_DXE_RT_PC_VARIABLE_INIT)  
#define DXE_VARIABLE_CLEAN_UP                 (EFI_SOFTWARE_EFI_RUNTIME_SERVICE | BYO_DXE_RT_PC_VARIABLE_CLEAN_UP)

// Reported by PCI bus driver
#define DXE_PCI_BUS_BEGIN                     (EFI_IO_BUS_PCI | EFI_IOB_PC_INIT)
#define DXE_PCI_BUS_ENUM                      (EFI_IO_BUS_PCI | EFI_IOB_PCI_BUS_ENUM)
#define DXE_PCI_BUS_HPC_INIT                  (EFI_IO_BUS_PCI | EFI_IOB_PCI_HPC_INIT)
#define DXE_PCI_BUS_REQUEST_RESOURCES         (EFI_IO_BUS_PCI | EFI_IOB_PCI_RES_ALLOC)
#define DXE_PCI_BUS_ASSIGN_RESOURCES          (EFI_IO_BUS_PCI | EFI_IOB_PCI_RES_ASSIGN)
#define DXE_PCI_BUS_HOTPLUG                   (EFI_IO_BUS_PCI | EFI_IOB_PC_HOTPLUG)
#define DXE_PCI_BUS_OUT_OF_RESOURCES          (EFI_IO_BUS_PCI | EFI_IOB_EC_RESOURCE_CONFLICT)

// Reported by USB bus driver
#define DXE_USB_BEGIN                         (EFI_IO_BUS_USB | EFI_IOB_PC_INIT)
#define DXE_USB_RESET                         (EFI_IO_BUS_USB | EFI_IOB_PC_RESET)
#define DXE_USB_DETECT                        (EFI_IO_BUS_USB | EFI_IOB_PC_DETECT)
#define DXE_USB_ENABLE                        (EFI_IO_BUS_USB | EFI_IOB_PC_ENABLE)
#define DXE_USB_HOTPLUG                       (EFI_IO_BUS_USB | EFI_IOB_PC_HOTPLUG)
#define DXE_USB_HAND_OFF                      (EFI_IO_BUS_USB | BYO_USB_HAND_OFF)
#define DXE_USB_HOTPLUG_OUT                   (EFI_IO_BUS_USB | EFI_IOB_PC_HOTPLUG_OUT)


// Reported by ATA Pass-Through driver
#define DXE_ATA_PASS_THRU_BEGIN               (EFI_IO_BUS_ATA_ATAPI | BYO_ATA_PASS_THRU_BEGIN)
#define DXE_ATA_PASS_THRU_RESET               (EFI_IO_BUS_ATA_ATAPI | BYO_ATA_PASS_THRU_RESET)
#define DXE_ATA_PASS_THRU_ENUMERATION         (EFI_IO_BUS_ATA_ATAPI | BYO_ATA_PASS_THRU_ENUMERATION)

//Reported by ATA bus driver
#define DXE_ATA_BEGIN                         (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_PC_INIT)
#define DXE_ATA_RESET                         (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_PC_RESET)
#define DXE_ATA_DETECT                        (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_PC_DETECT)
#define DXE_ATA_ENABLE                        (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_PC_ENABLE)
#define DXE_ATA_SMART_ERROR                   (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_ATA_BUS_SMART_OVERTHRESHOLD)
#define DXE_ATA_CONTROLLER_ERROR              (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_EC_CONTROLLER_ERROR)
#define DXE_ATA_DEVICE_FAILURE                (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_EC_INTERFACE_ERROR)

// Reported by SCSI bus driver
#define DXE_SCSI_BEGIN                        (EFI_IO_BUS_SCSI | EFI_IOB_PC_INIT)
#define DXE_SCSI_RESET                        (EFI_IO_BUS_SCSI | EFI_IOB_PC_RESET)
#define DXE_SCSI_DETECT                       (EFI_IO_BUS_SCSI | EFI_IOB_PC_DETECT)
#define DXE_SCSI_ENABLE                       (EFI_IO_BUS_SCSI | EFI_IOB_PC_ENABLE)

// Reported by Super I/O driver
#define DXE_SIO_INIT                          (EFI_IO_BUS_LPC | EFI_IOB_PC_INIT)

// Reported by Keyboard driver
#define DXE_KEYBOARD_INIT                     (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_INIT)
#define DXE_KEYBOARD_RESET                    (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_RESET)
#define DXE_KEYBOARD_DISABLE                  (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_DISABLE)
#define DXE_KEYBOARD_DETECT                   (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_PRESENCE_DETECT)
#define DXE_KEYBOARD_ENABLE                   (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_ENABLE)
#define DXE_KEYBOARD_CLEAR_BUFFER             (EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_CLEAR_BUFFER)
#define DXE_KEYBOARD_SELF_TEST                (EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_SELF_TEST)

// Reported by Mouse driver
#define DXE_MOUSE_INIT                        (EFI_PERIPHERAL_MOUSE | EFI_P_PC_INIT)
#define DXE_MOUSE_RESET                       (EFI_PERIPHERAL_MOUSE | EFI_P_PC_RESET)
#define DXE_MOUSE_DISABLE                     (EFI_PERIPHERAL_MOUSE | EFI_P_PC_DISABLE)
#define DXE_MOUSE_DETECT                      (EFI_PERIPHERAL_MOUSE | EFI_P_PC_PRESENCE_DETECT)
#define DXE_MOUSE_ENABLE                      (EFI_PERIPHERAL_MOUSE | EFI_P_PC_ENABLE)

// Reported by Mass Storage drivers
#define DXE_FIXED_MEDIA_INIT                  (EFI_PERIPHERAL_FIXED_MEDIA | EFI_P_PC_INIT)
#define DXE_FIXED_MEDIA_RESET                 (EFI_PERIPHERAL_FIXED_MEDIA | EFI_P_PC_RESET)
#define DXE_FIXED_MEDIA_DISABLE               (EFI_PERIPHERAL_FIXED_MEDIA | EFI_P_PC_DISABLE)
#define DXE_FIXED_MEDIA_DETECT                (EFI_PERIPHERAL_FIXED_MEDIA | EFI_P_PC_PRESENCE_DETECT)
#define DXE_FIXED_MEDIA_ENABLE                (EFI_PERIPHERAL_FIXED_MEDIA | EFI_P_PC_ENABLE)
#define DXE_REMOVABLE_MEDIA_INIT              (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_INIT)
#define DXE_REMOVABLE_MEDIA_RESET             (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_RESET)
#define DXE_REMOVABLE_MEDIA_DISABLE           (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_DISABLE)
#define DXE_REMOVABLE_MEDIA_DETECT            (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_PRESENCE_DETECT)
#define DXE_REMOVABLE_MEDIA_ENABLE            (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_ENABLE)

// Reported by CSM
#define DXE_CSM_INIT                          (EFI_SOFTWARE_DXE_BS_DRIVER | BYO_DXE_CSM_INIT)

// Reported by BDS
#define DXE_CON_OUT_CONNECT                   (EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_PC_INIT)
#define DXE_CON_IN_CONNECT                    (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_INIT)
#define DXE_NO_CON_OUT                        (EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_NOT_DETECTED)
#define DXE_NO_CON_IN                         (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_NOT_DETECTED)

// Reported by ACPI
#define DXE_ACPI_ENABLE                       (EFI_SOFTWARE_SMM_DRIVER | BYO_ACPI_ENABLE)




#ifndef BYO_STATUS_CODE_CLASS
///
/// Byosoft specific status code definitions
///
#define BYO_STATUS_CODE_CLASS     0x9000

///
/// Runtime Services specific codes
///
#define BYO_DXE_RT_PC_VARIABLE_INIT                  (BYO_STATUS_CODE_CLASS | 0x00000000)
#define BYO_DXE_RT_PC_VARIABLE_CLEAN_UP              (BYO_STATUS_CODE_CLASS | 0x00000001)

///
/// CSM specific codes
///
#define BYO_DXE_CSM_INIT                             (BYO_STATUS_CODE_CLASS | 0x00000000)

///
/// USB specific codes
///
#define BYO_USB_HAND_OFF                             (BYO_STATUS_CODE_CLASS | 0x00000000)

///
/// ACPI specific codes
///
#define BYO_ACPI_ENABLE                              (BYO_STATUS_CODE_CLASS | 0x00000001)

///
/// IO Bus Class ATA/ATAPI Subclass Progress Code definitions.
///
#define BYO_ATA_PASS_THRU_BEGIN                      (BYO_STATUS_CODE_CLASS | 0x00000000)
#define BYO_ATA_PASS_THRU_RESET                      (BYO_STATUS_CODE_CLASS | 0x00000001)
#define BYO_ATA_PASS_THRU_ENUMERATION                (BYO_STATUS_CODE_CLASS | 0x00000002)

#define BYO_EC_LENOVO_COMMON_ERROR                   (BYO_STATUS_CODE_CLASS | 0x00000004)
#define BYO_LENOVO_MACHINE_TYPE_INVALID              (BYO_STATUS_CODE_CLASS | 0x00000005)
#define BYO_LENOVO_CPU_FAN_FAILURE                   (BYO_STATUS_CODE_CLASS | 0x00000006)
#define BYO_LENOVO_KEYBOARD_NOT_FOUND                (BYO_STATUS_CODE_CLASS | 0x00000007)
#define BYO_LENOVO_MEMORY_SIZE_DECREASED             (BYO_STATUS_CODE_CLASS | 0x00000008)
#define BYO_LENOVO_SATA_DEVICE_ERROR                 (BYO_STATUS_CODE_CLASS | 0x00000009)
#define BYO_LENOVO_SETUP_DATA_CHANGED                (BYO_STATUS_CODE_CLASS | 0x0000000A)
#define BYO_LENOVO_SETUP_DATA_ERROR                  (BYO_STATUS_CODE_CLASS | 0x0000000B)
#define BYO_LENOVO_SYSTEM_TAMPERED_WITH              (BYO_STATUS_CODE_CLASS | 0x0000000C)
#define BYO_LENOVO_BIOS_PASSWORD_RETRY_ERROR         (BYO_STATUS_CODE_CLASS | 0x0000000E)
#define BYO_LENOVO_BIOS_PASSWORD_CHANGED             (BYO_STATUS_CODE_CLASS | 0x0000000F)
#define BYO_LENOVO_BIOS_UPDATED                      (BYO_STATUS_CODE_CLASS | 0x00000010)
#define BYO_LENOVO_HDD_PASSWORD_CHANGED              (BYO_STATUS_CODE_CLASS | 0x00000011)
#define BYO_LENOVO_SYS_EVT_LOG_CLEARED               (BYO_STATUS_CODE_CLASS | 0x00000012)
#define BYO_LENOVO_RESERVED                          (BYO_STATUS_CODE_CLASS | 0x00000013)
#define BYO_LENOVO_UCODE_NOT_EXIST                   (BYO_STATUS_CODE_CLASS | 0x00000014)
//Special condition, should be refactorying later
#define BYO_LENOVO_SYS_FAN1_FAILURE                  (BYO_STATUS_CODE_CLASS | 0x00000016) 
#define BYO_LENOVO_PWR_FAN_FAILURE                   (BYO_STATUS_CODE_CLASS | 0x00000026)
#define BYO_LENOVO_SYS_FAN2_FAILURE                  (BYO_STATUS_CODE_CLASS | 0x00000036)
#define BYO_LENOVO_SYS_PASSWORD_CLEAR                (BYO_STATUS_CODE_CLASS | 0x00000037)

#define BYO_LENOVO_HDD_PASSWORD_RETRY_ERROR          (BYO_STATUS_CODE_CLASS | 0x00000038)


#define EFI_IOB_PCI_RES_ASSIGN (EFI_SUBCLASS_SPECIFIC | 0x00000003)

#define EFI_IOB_PC_HOTPLUG_OUT  0x000000067

#define EFI_SW_PEI_PC_S3_STARTED      (EFI_SUBCLASS_SPECIFIC | 0x00000007)


#endif



#endif

