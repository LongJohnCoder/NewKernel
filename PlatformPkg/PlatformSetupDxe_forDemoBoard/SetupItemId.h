#ifndef __SETUP__ITEM_ID_H__
#define __SETUP__ITEM_ID_H__

#define PRINTABLE_LANGUAGE_NAME_STRING_ID     0x0001
#define MAIN_MAIN                      4096 
#define ADVANCED_MAIN                  4097 
#define BOOT_MAIN                      4098 
#define CPU_FORM_ID                    4109
#define TEST_FORM_ID                   4110
#define ConsoleRedirection_FORM_ID     4325

  #define  DEBUG_MAIN_FORM_ID  4634
#define POWER_CX_C4P_FORM_ID    4329
#define DRAM_FAST_BOOT_FORM_ID    4330
 #define CHIPSET_SDIO_FORM_ID 4636
 #define  DRAM_FORM_ID	4637
 #define NBDEBUG_FORM_ID 4638
 #define CHIPSET_FORM_ID 4339
 #define DEBUGSIGNAL_FORM_ID 4340
 #define POWER_CX_LATENCY_FORM_ID 4342
 #define POWER_DYNAMIC_CLK_CTRL_FORM_ID 4343
 #define POWER_OTHER_CTRL_FORM_ID 4344
 #define POWER_GATING_CTRL_FORM_ID 4345
 #define OTHERS_CTRL_FORM_ID 4346
 #define RAIDA_CTRL_FORM_ID 4347
 #define POWER_LEGACY_SEL_CTRL_FORM_ID 4348
 #define ERROR_REPORTING_FORM_ID 4349
 #define CHIPSET_DEBUG_FEATURE_FORM_ID 4350
 #define CHIPSET_DEBUG_FORM_ID 4351
 #define INTERRUPT_CTRL_FORM_ID 4352
#define  KEY_USER_ACCESS_LEVEL         4106
  #define  KEY_SAVE_AND_EXIT_VALUE  4353
  #define  KEY_DISCARD_AND_EXIT_VALUE  4354
  #define  KEY_RESTORE_DEFAULTS_VALUE  4355

  #define KEY_SAVE_USER_DEFAULTS_VALUE      8000
  #define KEY_RESTORE_USER_DEFAULTS_VALUE   8001
  
  #define  SEC_KEY_ADMIN_PD         4356
  #define  SEC_KEY_POWER_ON_PD      4357
  #define  SEC_KEY_CLEAR_USER_PD    4358
  
  #define  KEY_RESET_TO_SETUP_MODE  4360
  #define  KEY_RESTORE_FACTORY_KEYS 4361
  #define  CX_STATE_CTRL_FORM_ID 4362
  #define  KEY_VALUE_PCIERP 4363  
#define KEY_VALUE_PCIERST                    4364
#define KEY_VALUE_PCIE_PE0                    4365
#define KEY_VALUE_PCIE_PE1                    4366
#define KEY_VALUE_PCIE_PE2                    4367
#define KEY_VALUE_PCIE_PE3                    4368
#define KEY_VALUE_PCIE_PE4                    4369
#define KEY_VALUE_PCIE_PE5                    4370
//#define KEY_VALUE_PCIE_PEG                    4371
#define KEY_VALUE_PCIE_PE6                    4372
#define KEY_VALUE_PCIE_PE7                    4373
#define   KEY_ACTIMING_OPTION                  4374
#define   KEY_VALUE_NBSPE                  4375
#define   KEY_VALUE_SBSPE                  4376
#define   KEY_VALUE_IOVEN                  4377
#define   KEY_VALUE_IOVQIEN                4378
#define   KEY_VALUE_CRB_MODE_SEL                4711


#define KEY_VALUE_IOE_PECTL_PEA0		4379
#define KEY_VALUE_IOE_PECTL_PEA1		4380
#define KEY_VALUE_IOE_PECTL_PEA2		4381
#define KEY_VALUE_IOE_PECTL_PEA3		4382
#define KEY_VALUE_IOE_PECTL_PEA4		4383
#define KEY_VALUE_IOE_PECTL_PEB0		4384
#define KEY_VALUE_IOE_PECTL_PEB1		4385
#define KEY_VALUE_IOE_PECTL		4386









#define  DEVICES_MAIN_FORM_ID          4608
#define  CHIPSET_NB_FORM_ID            4609
#define  CHIPSET_SB_FORM_ID            4610
#define  CHIPSET_NB_DRAM_FORM_ID       4611
#define  CHIPSET_NB_VIDEO_FORM_ID      4612
#define  CHIPSET_NB_PCIE_FORM_ID       4613
#define  CHIPSET_NB_CPU_FORM_ID        4614
#define  CHIPSET_SB_UART_FORM_ID       4615
#define  CHIPSET_SB_USB_FORM_ID        4616
  #define  CHIPSET_SB_PMU_ACPI_FORM_ID  4617
  #define  CHIPSET_P2P_FORM_ID  4618
  #define  CHIPSET_HDAC_FORM_ID  4619
  #define  CHIPSET_SNMI_FORM_ID  4620
  #define  CHIPSET_OTHERS_FORM_ID  4621
#define  CHIPSET_SATA_FORM_ID          4622
  #define  CHIPSET_ACPI_C_STATE_FORM_ID  4623
  #define  CHIPSET_ACPI_C_LATENCY_FORM_ID  4624
  #define  CHIPSET_ACPI_DYNAMIC_CLOCK_FORM_ID  4625
  #define  CHIPSET_ACPI_INTERRUPT_FORM_ID  4626
  #define  CHIPSET_ACPI_OTHERS_FORM_ID  4627
#define  POWER_AUTO_POWER_ON_FORM_ID   4628
#define  POWER_USER_DEF_ALARM_FORM_ID  4629	
#define  POWER_MAIN_FORM_ID            4630
#define  NETWORK_SETUP_FORM_ID         4631
#define  NVME_SETUP_FORM_ID            4632
  #define  FAN_FORM_ID  4652
  #define  CHIPSET_SB_AUDIO_FORM_ID  4653 
#define  CHIPSET_PCIE_FORM_ID  4654 
#define KEY_C4P_CONTROL              4651
#define PCIE_KERNAL_CTRL_FORM_ID              4655
#define PCIE_MSGC_PIC_INTROUT_CTRL_FORM_ID              4656
#define PCIE_PE67_APIC_INT_CTRL_FORM_ID              4657
#define PCIE_RP_LOCK_BIT_CTRL_FORM_ID              4658
#define VIRTUALIZATION_CTRL_FORM_ID   4660
#define PCIE_RP_INT_ROUTING_CTRL_FORM_ID   4662
#define PCIE_GEN3_EQ_CTRL_FORM_ID    4663


#define MAIN_PAGE_KEY_LANGUAGE         0x2341

	// IOE_PORTING -start
	#define IOE_PCIE_PTN_FORM_ID    4701
	#define IOE_SATA_FORM_ID        4702
	#define IOE_TOP_FORM_ID         4703
	#define IOE_USB_FORM_ID         4704
	#define IOE_GNIC_FORM_ID        4705
	#define IOE_DEBUGSIGNAL_FORM_ID 4706
	#define IOE_FORM_ID             4707
	#define IOE_SPE_FORM_ID         4708
	#define IOE_DBGCAP_FORM_ID      4709
	#define IOE_CLK_SRCL_FORM_ID    4710
	// IOE_PORTING -end

  
  #define LABEL_SELECT_LANGUAGE    0x2000
  #define LABEL_END                0xffff
  
  #define MAIN_PAGE_FORM_ID              0x0001
  #define MAIN_SYSTEM_SUMMARY_FORM_ID    0x0002


#define AUTO_ID(x) x


#endif
