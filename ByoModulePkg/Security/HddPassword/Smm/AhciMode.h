/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  AhciMode.h

Abstract: 
  part of Hdd password SMM driver.

Revision History:

Bug 3178 - add Hdd security Frozen support.
TIME: 2011-12-06
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Send ATA security frozen command to HDDs at ReadyToBoot and 
     _WAK().
$END--------------------------------------------------------------------

Bug 2749 - Fix S3 resume failed if HDD password enabled.
TIME: 2011-08-18
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Give more delay according to SATA spec.
$END--------------------------------------------------------------------

Bug2274: improve hdd password feature.
TIME: 2011-06-09
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. give user max 3 times to input password at post.
  2. check device is hdd or not before unlock it when S3 resume.
  3. remove personal label such as wztest.
$END--------------------------------------------------------------------

Bug2164: system cannot resume from S3 when hdd password has been set before.
TIME: 2011-05-26
$AUTHOR: Zhang Lin
$REVIEWERS: Chen Daolin
$SCOPE: SugarBay
$TECHNICAL: 
$END----------------------------------------------------------------------------

**/



#ifndef __HD_PASSWORD_AHCI_MODE_H__
#define __HD_PASSWORD_AHCI_MODE_H__

#define EFI_AHCI_BAR_INDEX                     0x05

#define EFI_AHCI_CAPABILITY_OFFSET             0x0000
#define   EFI_AHCI_CAP_SSS                     BIT27
#define   EFI_AHCI_CAP_S64A                    BIT31
#define EFI_AHCI_GHC_OFFSET                    0x0004
#define   EFI_AHCI_GHC_RESET                   BIT0
#define   EFI_AHCI_GHC_IE                      BIT1
#define   EFI_AHCI_GHC_ENABLE                  BIT31
#define EFI_AHCI_IS_OFFSET                     0x0008
#define EFI_AHCI_PI_OFFSET                     0x000C

typedef struct {
  UINT32  Lower32;
  UINT32  Upper32;
} DATA_32;

typedef union {
  DATA_32   Uint32;
  UINT64    Uint64;
} DATA_64;



// Refer SATA1.0a spec section 5.2, the Phy detection time should be less than 10ms.
#define  EFI_AHCI_BUS_PHY_DETECT_TIMEOUT       10

// Refer SATA1.0a spec, the FIS enable time should be less than 500ms.
#define  EFI_AHCI_PORT_CMD_FR_CLEAR_TIMEOUT    EFI_TIMER_PERIOD_MILLISECONDS(500)

// Refer SATA1.0a spec, the bus reset time should be less than 1s.
#define  EFI_AHCI_BUS_RESET_TIMEOUT            EFI_TIMER_PERIOD_SECONDS(1)




#define  EFI_AHCI_ATAPI_DEVICE_SIG             0xEB140000
#define  EFI_AHCI_ATA_DEVICE_SIG               0x00000000
#define  EFI_AHCI_PORT_MULTIPLIER_SIG          0x96690000
#define  EFI_AHCI_ATAPI_SIG_MASK               0xFFFF0000


//
// Each PRDT entry can point to a memory block up to 4M byte
//
#define EFI_AHCI_MAX_DATA_PER_PRDT             0x400000

#define EFI_AHCI_FIS_REGISTER_H2D              0x27      //Register FIS - Host to Device
#define   EFI_AHCI_FIS_REGISTER_H2D_LENGTH     20 
#define EFI_AHCI_FIS_REGISTER_D2H              0x34      //Register FIS - Device to Host
#define   EFI_AHCI_FIS_REGISTER_D2H_LENGTH     20
#define EFI_AHCI_FIS_DMA_ACTIVATE              0x39      //DMA Activate FIS - Device to Host
#define   EFI_AHCI_FIS_DMA_ACTIVATE_LENGTH     4
#define EFI_AHCI_FIS_DMA_SETUP                 0x41      //DMA Setup FIS - Bi-directional
#define   EFI_AHCI_FIS_DMA_SETUP_LENGTH        28
#define EFI_AHCI_FIS_DATA                      0x46      //Data FIS - Bi-directional
#define EFI_AHCI_FIS_BIST                      0x58      //BIST Activate FIS - Bi-directional
#define   EFI_AHCI_FIS_BIST_LENGTH             12
#define EFI_AHCI_FIS_PIO_SETUP                 0x5F      //PIO Setup FIS - Device to Host
#define   EFI_AHCI_FIS_PIO_SETUP_LENGTH        20
#define EFI_AHCI_FIS_SET_DEVICE                0xA1      //Set Device Bits FIS - Device to Host
#define   EFI_AHCI_FIS_SET_DEVICE_LENGTH       8

#define EFI_AHCI_D2H_FIS_OFFSET                0x40
#define EFI_AHCI_DMA_FIS_OFFSET                0x00
#define EFI_AHCI_PIO_FIS_OFFSET                0x20
#define EFI_AHCI_SDB_FIS_OFFSET                0x58
#define EFI_AHCI_FIS_TYPE_MASK                 0xFF
#define EFI_AHCI_U_FIS_OFFSET                  0x60

//
// Port register
//
#define EFI_AHCI_PORT_START                    0x0100
#define EFI_AHCI_PORT_REG_WIDTH                0x0080
#define EFI_AHCI_PORT_CLB                      0x0000
#define EFI_AHCI_PORT_CLBU                     0x0004
#define EFI_AHCI_PORT_FB                       0x0008
#define EFI_AHCI_PORT_FBU                      0x000C
#define EFI_AHCI_PORT_IS                       0x0010
#define   EFI_AHCI_PORT_IS_DHRS                BIT0
#define   EFI_AHCI_PORT_IS_PSS                 BIT1
#define   EFI_AHCI_PORT_IS_SSS                 BIT2
#define   EFI_AHCI_PORT_IS_SDBS                BIT3
#define   EFI_AHCI_PORT_IS_UFS                 BIT4
#define   EFI_AHCI_PORT_IS_DPS                 BIT5
#define   EFI_AHCI_PORT_IS_PCS                 BIT6
#define   EFI_AHCI_PORT_IS_DIS                 BIT7
#define   EFI_AHCI_PORT_IS_PRCS                BIT22
#define   EFI_AHCI_PORT_IS_IPMS                BIT23
#define   EFI_AHCI_PORT_IS_OFS                 BIT24
#define   EFI_AHCI_PORT_IS_INFS                BIT26
#define   EFI_AHCI_PORT_IS_IFS                 BIT27
#define   EFI_AHCI_PORT_IS_HBDS                BIT28
#define   EFI_AHCI_PORT_IS_HBFS                BIT29
#define   EFI_AHCI_PORT_IS_TFES                BIT30
#define   EFI_AHCI_PORT_IS_CPDS                BIT31
#define   EFI_AHCI_PORT_IS_CLEAR               0xFFFFFFFF
#define   EFI_AHCI_PORT_IS_FIS_CLEAR           0x0000000F

#define EFI_AHCI_PORT_IE                       0x0014
#define EFI_AHCI_PORT_CMD                      0x0018
#define   EFI_AHCI_PORT_CMD_ST_MASK            0xFFFFFFFE
#define   EFI_AHCI_PORT_CMD_ST                 BIT0
#define   EFI_AHCI_PORT_CMD_SUD                BIT1
#define   EFI_AHCI_PORT_CMD_POD                BIT2
#define   EFI_AHCI_PORT_CMD_CLO                BIT3
#define   EFI_AHCI_PORT_CMD_CR                 BIT15
#define   EFI_AHCI_PORT_CMD_FRE                BIT4
#define   EFI_AHCI_PORT_CMD_FR                 BIT14
#define   EFI_AHCI_PORT_CMD_MASK               ~(EFI_AHCI_PORT_CMD_ST | EFI_AHCI_PORT_CMD_FRE | EFI_AHCI_PORT_CMD_COL)
#define   EFI_AHCI_PORT_CMD_PMA                BIT17
#define   EFI_AHCI_PORT_CMD_HPCP               BIT18
#define   EFI_AHCI_PORT_CMD_MPSP               BIT19
#define   EFI_AHCI_PORT_CMD_CPD                BIT20
#define   EFI_AHCI_PORT_CMD_ESP                BIT21
#define   EFI_AHCI_PORT_CMD_ATAPI              BIT24
#define   EFI_AHCI_PORT_CMD_DLAE               BIT25
#define   EFI_AHCI_PORT_CMD_ALPE               BIT26
#define   EFI_AHCI_PORT_CMD_ASP                BIT27
#define   EFI_AHCI_PORT_CMD_ICC_MASK           (BIT28 | BIT29 | BIT30 | BIT31)
#define   EFI_AHCI_PORT_CMD_ACTIVE             (1 << 28 )
#define EFI_AHCI_PORT_TFD                      0x0020
#define   EFI_AHCI_PORT_TFD_MASK               (BIT7 | BIT3 | BIT0)
#define   EFI_AHCI_PORT_TFD_BSY                BIT7
#define   EFI_AHCI_PORT_TFD_DRQ                BIT3
#define   EFI_AHCI_PORT_TFD_ERR                BIT0
#define   EFI_AHCI_PORT_TFD_ERR_MASK           0x00FF00
#define EFI_AHCI_PORT_SIG                      0x0024
#define EFI_AHCI_PORT_SSTS                     0x0028
#define   EFI_AHCI_PORT_SSTS_DET_MASK          0x000F
#define   EFI_AHCI_PORT_SSTS_DET               0x0001
#define   EFI_AHCI_PORT_SSTS_DET_PCE           0x0003
#define   EFI_AHCI_PORT_SSTS_SPD_MASK          0x00F0
#define EFI_AHCI_PORT_SCTL                     0x002C
#define   EFI_AHCI_PORT_SCTL_DET_MASK          0x000F
#define   EFI_AHCI_PORT_SCTL_MASK              (~EFI_AHCI_PORT_SCTL_DET_MASK)
#define   EFI_AHCI_PORT_SCTL_DET_INIT          0x0001
#define   EFI_AHCI_PORT_SCTL_DET_PHYCOMM       0x0003
#define   EFI_AHCI_PORT_SCTL_SPD_MASK          0x00F0
#define   EFI_AHCI_PORT_SCTL_IPM_MASK          0x0F00
#define   EFI_AHCI_PORT_SCTL_IPM_INIT          0x0300
#define   EFI_AHCI_PORT_SCTL_IPM_PSD           0x0100
#define   EFI_AHCI_PORT_SCTL_IPM_SSD           0x0200
#define EFI_AHCI_PORT_SERR                     0x0030
#define   EFI_AHCI_PORT_SERR_RDIE              BIT0
#define   EFI_AHCI_PORT_SERR_RCE               BIT1
#define   EFI_AHCI_PORT_SERR_TDIE              BIT8
#define   EFI_AHCI_PORT_SERR_PCDIE             BIT9
#define   EFI_AHCI_PORT_SERR_PE                BIT10
#define   EFI_AHCI_PORT_SERR_IE                BIT11
#define   EFI_AHCI_PORT_SERR_PRC               BIT16
#define   EFI_AHCI_PORT_SERR_PIE               BIT17
#define   EFI_AHCI_PORT_SERR_CW                BIT18
#define   EFI_AHCI_PORT_SERR_BDE               BIT19
#define   EFI_AHCI_PORT_SERR_DE                BIT20
#define   EFI_AHCI_PORT_SERR_CRCE              BIT21
#define   EFI_AHCI_PORT_SERR_HE                BIT22          
#define   EFI_AHCI_PORT_SERR_LSE               BIT23
#define   EFI_AHCI_PORT_SERR_TSTE              BIT24
#define   EFI_AHCI_PORT_SERR_UFT               BIT25
#define   EFI_AHCI_PORT_SERR_EX                BIT26
#define   EFI_AHCI_PORT_ERR_CLEAR              0xFFFFFFFF
#define EFI_AHCI_PORT_SACT                     0x0034
#define EFI_AHCI_PORT_CI                       0x0038
#define EFI_AHCI_PORT_SNTF                     0x003C


#pragma pack(1)
//
// Command List structure includes total 32 entries.
// The entry data structure is listed at the following.
//
typedef struct {
  UINT32   AhciCmdCfl:5;      //Command FIS Length
  UINT32   AhciCmdA:1;        //ATAPI
  UINT32   AhciCmdW:1;        //Write
  UINT32   AhciCmdP:1;        //Prefetchable
  UINT32   AhciCmdR:1;        //Reset
  UINT32   AhciCmdB:1;        //BIST
  UINT32   AhciCmdC:1;        //Clear Busy upon R_OK
  UINT32   AhciCmdRsvd:1;
  UINT32   AhciCmdPmp:4;      //Port Multiplier Port
  UINT32   AhciCmdPrdtl:16;   //Physical Region Descriptor Table Length
  UINT32   AhciCmdPrdbc;      //Physical Region Descriptor Byte Count
  UINT32   AhciCmdCtba;       //Command Table Descriptor Base Address
  UINT32   AhciCmdCtbau;      //Command Table Descriptor Base Address Upper 32-BITs
  UINT32   AhciCmdRsvd1[4]; 
} EFI_AHCI_COMMAND_LIST;

//
// This is a software constructed FIS.
// For data transfer operations, this is the H2D Register FIS format as 
// specified in the Serial ATA Revision 2.6 specification.
//
typedef struct {
  UINT8    AhciCFisType;
  UINT8    AhciCFisPmNum:4;
  UINT8    AhciCFisRsvd:1;
  UINT8    AhciCFisRsvd1:1;
  UINT8    AhciCFisRsvd2:1;
  UINT8    AhciCFisCmdInd:1;
  UINT8    AhciCFisCmd;
  UINT8    AhciCFisFeature;
  UINT8    AhciCFisSecNum;
  UINT8    AhciCFisClyLow;
  UINT8    AhciCFisClyHigh;
  UINT8    AhciCFisDevHead;
  UINT8    AhciCFisSecNumExp;
  UINT8    AhciCFisClyLowExp;
  UINT8    AhciCFisClyHighExp;
  UINT8    AhciCFisFeatureExp;
  UINT8    AhciCFisSecCount;
  UINT8    AhciCFisSecCountExp;
  UINT8    AhciCFisRsvd3;
  UINT8    AhciCFisControl;
  UINT8    AhciCFisRsvd4[4];
  UINT8    AhciCFisRsvd5[44];
} EFI_AHCI_COMMAND_FIS;

//
// ACMD: ATAPI command (12 or 16 bytes)
//
typedef struct {
  UINT8    AtapiCmd[0x10];
} EFI_AHCI_ATAPI_COMMAND;

//
// Physical Region Descriptor Table includes up to 65535 entries
// The entry data structure is listed at the following.
// the actual entry number comes from the PRDTL field in the command
// list entry for this command slot. 
//
typedef struct {
  UINT32   AhciPrdtDba;       //Data Base Address
  UINT32   AhciPrdtDbau;      //Data Base Address Upper 32-BITs
  UINT32   AhciPrdtRsvd;
  UINT32   AhciPrdtDbc:22;    //Data Byte Count
  UINT32   AhciPrdtRsvd1:9;
  UINT32   AhciPrdtIoc:1;     //Interrupt on Completion
} EFI_AHCI_COMMAND_PRDT;

//
// Command table data strucute which is pointed to by the entry in the command list
//
typedef struct {
  EFI_AHCI_COMMAND_FIS      CommandFis;       // A software constructed FIS.
  EFI_AHCI_ATAPI_COMMAND    AtapiCmd;         // 12 or 16 bytes ATAPI cmd.
  UINT8                     Reserved[0x30];
  EFI_AHCI_COMMAND_PRDT     PrdtTable;    // The scatter/gather list for data transfer
} EFI_AHCI_COMMAND_TABLE;

//
// Received FIS structure
//
typedef struct {
  UINT8    AhciDmaSetupFis[0x1C];         // Dma Setup Fis: offset 0x00
  UINT8    AhciDmaSetupFisRsvd[0x04];
  UINT8    AhciPioSetupFis[0x14];         // Pio Setup Fis: offset 0x20
  UINT8    AhciPioSetupFisRsvd[0x0C];     
  UINT8    AhciD2HRegisterFis[0x14];      // D2H Register Fis: offset 0x40
  UINT8    AhciD2HRegisterFisRsvd[0x04];
  UINT64   AhciSetDeviceBitsFis;          // Set Device Bits Fix: offset 0x58
  UINT8    AhciUnknownFis[0x40];          // Unkonwn Fis: offset 0x60
  UINT8    AhciUnknownFisRsvd[0x60];      
} EFI_AHCI_RECEIVED_FIS; 

#pragma pack()

typedef struct {
  EFI_AHCI_RECEIVED_FIS     *AhciRFis;
  EFI_AHCI_COMMAND_LIST     *AhciCmdList;
  EFI_AHCI_COMMAND_TABLE    *AhciCommandTable;
} EFI_AHCI_REGISTERS;

extern EFI_AHCI_REGISTERS mAhciRegisters;




/**
  Do AHCI port reset.

  @param  Port               The number of port.
  @param  Timeout            The timeout value of reset.
   
  @retval EFI_DEVICE_ERROR   The port reset unsuccessfully
  @retval EFI_TIMEOUT        The reset operation is time out.
  @retval EFI_SUCCESS        The port reset successfully.

**/
/*
EFI_STATUS
EFIAPI
AhciPortReset (
  IN  UINT8                     Port,
  IN  UINT64                    Timeout
  );
*/


/**
  Send Buffer cmd to specific device.
    
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The timeout value of stop.
  @param  Buffer              The data buffer to store IDENTIFY PACKET data.

  @retval EFI_DEVICE_ERROR    The cmd abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for executing.
  @retval EFI_SUCCESS         The cmd executes successfully.

**/
EFI_STATUS
EFIAPI
AhciIdentify (
  IN EFI_AHCI_REGISTERS       *AhciRegisters,
  IN UINT8                    Port,
  IN UINT8                    PortMultiplier,
  IN OUT ATA_IDENTIFY_DATA    *Buffer  
  );

/**
  Send unlock hdd password cmd to specific device.
    
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The timeout value of stop.
  @param  Buffer              The data buffer to store IDENTIFY PACKET data.

  @retval EFI_DEVICE_ERROR    The cmd abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for executing.
  @retval EFI_SUCCESS         The cmd executes successfully.

**/
EFI_STATUS
EFIAPI
AhciUnlockHddPassword (
  IN EFI_AHCI_REGISTERS       *AhciRegisters,
  IN UINT8                    Port,
  IN UINT8                    PortMultiplier,
  IN CHAR8                    Identifier,
  IN CHAR8                    *Password
  );


/**
  Send Frozen hdd security cmd to specific device.
    
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The timeout value of stop.

  @retval EFI_DEVICE_ERROR    The cmd abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for executing.
  @retval EFI_SUCCESS         The cmd executes successfully.

**/
EFI_STATUS
EFIAPI
AhciFrozenHddSecurity (
  IN EFI_AHCI_REGISTERS       *AhciRegisters,
  IN UINT8                    Port,
  IN UINT8                    PortMultiplier
  );


/**
  Get AHCI mode base address registers' value. 

  @param[in] Bus         The bus number of ata host controller.
  @param[in] Device      The device number of ata host controller.
  @param[in] Function    The function number of ata host controller.
           
  @retval EFI_UNSUPPORTED        Return this value when the BARs is not IO type
  @retval EFI_SUCCESS            Get the Base address successfully
  @retval Other                  Read the pci configureation data error

**/
EFI_STATUS
EFIAPI
GetAhciBaseAddress (
  IN     UINTN                       Bus,
  IN     UINTN                       Device,
  IN     UINTN                       Function
  );

/**
  Allocate transfer-related data struct which is used at AHCI mode.
  
  @retval  EFI_OUT_OF_RESOURCE   The allocation is failure.
  @retval  EFI_SUCCESS           Successful to allocate memory.

**/
EFI_STATUS
EFIAPI
AhciAllocateResource (
  );

/**
  Initialize ATA host controller at AHCI mode.

  The function is designed to initialize ATA host controller. 
  
  @param[in]  Port          The port number to do initialization.

**/
EFI_STATUS
EFIAPI
AhciModeInitialize (
  UINT8      Port
  );

#endif

