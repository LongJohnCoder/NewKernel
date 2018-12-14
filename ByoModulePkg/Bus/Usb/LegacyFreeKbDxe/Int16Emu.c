

#include <Uefi.h>



#pragma pack(1)

/*
  40:00-01 Com1
  40:02-03 Com2
  40:04-05 Com3
  40:06-07 Com4
  40:08-09 Lpt1
  40:0A-0B Lpt2
  40:0C-0D Lpt3
  40:0E-0F Ebda segment
  40:10-11 MachineConfig
  40:12    Bda12 - skip
  40:13-14 MemSize below 1MB
  40:15-16 Bda15_16 - skip
  40:17    Keyboard Shift status
  40:18-19 Bda18_19 - skip
  40:1A-1B Key buffer head
  40:1C-1D Key buffer tail
  40:1E-3D Bda1E_3D- key buffer -skip
  40:3E-3F FloppyData 3E = Calibration status 3F = Motor status
  40:40    FloppyTimeout
  40:41-74 Bda41_74 - skip
  40:75    Number of HDD drives
  40:76-77 Bda76_77 - skip
  40:78-79 78 = Lpt1 timeout, 79 = Lpt2 timeout
  40:7A-7B 7A = Lpt3 timeout, 7B = Lpt4 timeout
  40:7C-7D 7C = Com1 timeout, 7D = Com2 timeout
  40:7E-7F 7E = Com3 timeout, 7F = Com4 timeout
  40:80-81 Pointer to start of key buffer
  40:82-83 Pointer to end of key buffer
  40:84-87 Bda84_87 - skip
  40:88    HDD Data Xmit rate
  40:89-8f skip
  40:90    Floppy data rate
  40:91-95 skip
  40:96    Keyboard Status
  40:97    LED Status
  40:98-101 skip
*/
typedef struct {
  UINT16  Com1;
  UINT16  Com2;
  UINT16  Com3;
  UINT16  Com4;
  UINT16  Lpt1;
  UINT16  Lpt2;
  UINT16  Lpt3;
  UINT16  Ebda;
  UINT16  MachineConfig;
  UINT8   Bda12;
  UINT16  MemSize;
  UINT8   Bda15_16[0x02];
  UINT8   ShiftStatus;
  UINT8   Bda18_19[0x02];
  UINT16  KeyHead;
  UINT16  KeyTail;
  UINT16  Bda1E_3D[0x10];
  UINT16  FloppyData;
  UINT8   FloppyTimeout;
  UINT8   Bda41_74[0x34];
  UINT8   NumberOfDrives;
  UINT8   Bda76_77[0x02];
  UINT16  Lpt1_2Timeout;
  UINT16  Lpt3_4Timeout;
  UINT16  Com1_2Timeout;
  UINT16  Com3_4Timeout;
  UINT16  KeyStart;
  UINT16  KeyEnd;
  UINT8   Bda84_87[0x4];
  UINT8   DataXmit;
  UINT8   Bda89_8F[0x07];
  UINT8   FloppyXRate;
  UINT8   Bda91_95[0x05];
  UINT8   KeyboardStatus;
  UINT8   LedStatus;
} BDA_STRUC;
#pragma pack()



STATIC
VOID
RemoveKey (BDA_STRUC *Bda)
{
  UINT16    NextPtr;
  //
  // Remove any key
  //
  if (Bda->KeyHead >= Bda->KeyEnd - 2) {
      NextPtr = Bda->KeyStart;
      Bda->KeyTail = Bda->KeyStart;
  } else {
      NextPtr = Bda->KeyHead + 2;
  }

  Bda->KeyHead = NextPtr;
}


typedef struct{
    UINT8    Asc_code;
    UINT8    Scan_code;
} KEY_SCAN;

typedef union  {
    UINT16    WORD;
    KEY_SCAN  CHAR;
} KEY_CODE;


BOOLEAN
ConvertKey (
  UINT16   *Data16
  )
{
  KEY_CODE    KeyCode;
  
  KeyCode.WORD = *Data16;

  //
  // Translate extended key
  //
  if (KeyCode.WORD == 0xE00D) {
      //
      // Enter-num
      //
      KeyCode.WORD = 0x1C0D;
  } else if (KeyCode.WORD == 0xE00A) {
      //
      // Enter-num with Control
      //
      KeyCode.WORD = 0x1C0A;
  } else if (KeyCode.WORD == 0xE02F) {
      //
      // Gray
      //
      KeyCode.WORD = 0x352F;
  } else if (KeyCode.CHAR.Scan_code > 0x84) {
      return FALSE;
  } else if (KeyCode.CHAR.Scan_code == 0) {
      return FALSE;
  }
  
  *Data16 = KeyCode.WORD;
  return TRUE;
}



BOOLEAN
GetKeyFromBda(
  UINT16 *ScanCode, 
  UINT16 *KeyChar
  )
{
  UINT16      KeyNumber;
  BDA_STRUC   *Bda;
  UINT16      Data16;
  
  Bda = (BDA_STRUC*)(UINTN)0x400;
  KeyNumber = (Bda->KeyTail - Bda->KeyHead)/2;
  if(KeyNumber == 0){
    return FALSE;
  }

  Data16 = Bda->Bda1E_3D[(Bda->KeyHead - Bda->KeyStart)/2];
  RemoveKey(Bda);
  ConvertKey(&Data16);

  *ScanCode = (Data16 >> 8) & 0xFF;
  *KeyChar  = Data16 & 0xFF;  

  return TRUE;
}



