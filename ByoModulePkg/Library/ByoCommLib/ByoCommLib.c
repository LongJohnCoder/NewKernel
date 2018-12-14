
#include <Uefi.h>
#include <Pi/PiBootMode.h>      // HobLib.h +
#include <Pi/PiHob.h>           // HobLib.h +
#include <Uefi/UefiSpec.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Atapi.h>
#include <IndustryStandard/Smbios.h>
#include <Uefi/UefiAcpiDataTable.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/PciLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/PciIo.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/ScsiIo.h>
#include <Library/ByoCommLib.h>
#include <VerbTableHdr.h>


extern EFI_GUID gEfiConsoleOutDeviceGuid;

#define PCI_DEV_MMBASE(Bus, Device, Function) \
    ( \
      (UINTN)PcdGet64(PcdPciExpressBaseAddress) + (UINTN) (Bus << 20) + (UINTN) (Device << 15) + (UINTN) \
        (Function << 12) \
    )

UINT8 PcieRead8(UINTN PcieAddr)
{
  UINT8   Shift;
  UINT32  Data32;
  
  Data32 = MmioRead32(PcieAddr & ~(BIT0 + BIT1));
  Shift  = (UINT8)((PcieAddr & (BIT0 + BIT1))*8);
  
  return (UINT8)((Data32 >> Shift) & 0xFF);
}


// W = (d+2*m+3*(m+1)/5+y+y/4-y/100+y/400) mod 7
// 0 - Monday
UINT8 CaculateWeekDay(UINT16 y, UINT8 m, UINT8 d)
{
  UINT8 Weekday;
  
  if(m == 1 || m == 2){
    m += 12;
    y--;
  }
  Weekday = (d + 2*m + 3*(m + 1)/5 + y + y/4 - y/100 + y/400) % 7;
  return Weekday;
}  


BOOLEAN IsLeapYear(UINT16 Year)
{
  if (Year % 4 == 0) {
    if (Year % 100 == 0) {
      if (Year % 400 == 0) {
        return TRUE;
      } else {
        return FALSE;
      }
    } else {
      return TRUE;
    }
  } else {
    return FALSE;
  }
}



//------------------------------------ DEBUG -----------------------------------
VOID DumpMem32(VOID *Base, UINTN Size)
{
  UINTN  Index;
	UINTN  Addr;

  Addr = (UINTN)Base;
  Addr &= ~0x3;

  DEBUG((EFI_D_ERROR, "%a(%X,%X)\n", __FUNCTION__, Base, Size));
  for (Index=0; Index < Size; Index+=4) {
    if(Index%16==0){
      DEBUG((EFI_D_ERROR, "%08X: ", Addr+Index));
    }
    DEBUG((EFI_D_ERROR, "%08X ", MmioRead32(Addr+Index)));
    if((Index+4)%16==0){
      DEBUG((EFI_D_ERROR, "\n"));
    }
  }

  DEBUG((EFI_D_ERROR, "\n"));
}


VOID DumpMem8(VOID *Base, UINTN Size)
{
  UINT8  *Data8;
  UINTN  Index;

  DEBUG((EFI_D_ERROR, "%a(%X,%X)\n", __FUNCTION__, Base, Size));
  Data8 = (UINT8*)Base;
  for(Index=0; Index<Size; Index++){
    DEBUG((EFI_D_ERROR, "%02X ", Data8[Index]));
    if(((Index+1)%16)==0){
      DEBUG((EFI_D_ERROR, "\n"));
    }
  }
  DEBUG((EFI_D_ERROR, "\n"));
}


VOID DumpCmos()
{
  UINTN  Index;

  DEBUG((EFI_D_ERROR, "%a()\n", __FUNCTION__));
  for(Index=0;Index<128;Index++){
    IoWrite8(0x70, (UINT8)Index);
    DEBUG((EFI_D_ERROR, "%02X ", IoRead8(0x71)));
    if((Index+1)%16==0){
      DEBUG((EFI_D_ERROR, "\n"));
    }
  }  
}


VOID DumpPci(UINT8 Bus, UINT8 Dev, UINT8 Func)
{
  UINTN   Index;
  UINTN   Base;

#if 0
  Base = PCI_DEV_MMBASE(Bus, Dev, Func);
  DEBUG((EFI_D_ERROR, "(%02X,%02X,%02X)\n", Bus, Dev, Func));
  for(Index=0;Index<256;Index+=4){
    DEBUG((EFI_D_ERROR, "%08X ", MmioRead32(Base+Index)));
    if((Index+4)%16==0){
      DEBUG((EFI_D_ERROR, "\n"));
    }
  }  
#else
	 Base = PCI_DEV_MMBASE(Bus, Dev, Func);
	 DEBUG((EFI_D_ERROR,"Device(%d, %d, %d):\n	 ",Bus, Dev, Func));
	 for(Index = 0; Index < 16; Index++) DEBUG((EFI_D_ERROR," %02X", Index));
	 DEBUG((EFI_D_ERROR,"\n  +"));
	 for(Index = 0; Index <= 3*16; Index++) DEBUG((EFI_D_ERROR,"-"));
	// DEBUG((EFI_D_ERROR, "\n"));
	// DEBUG((EFI_D_ERROR, "PciDev(%02X,%02X,%02X)\n", Bus, Dev, Func));
	 for(Index=0;Index<256;Index++){
	   if(Index%16==0) DEBUG((EFI_D_ERROR,"\n%02X|", Index/16));
	   DEBUG((EFI_D_ERROR, " %02X", MmioRead8(Base+Index)));
	   //if((Index+4)%16==0){
	   //  DEBUG((EFI_D_ERROR, "\n"));
	   //}
	 }	
	 DEBUG((EFI_D_ERROR,"\n"));
#endif
}

VOID DumpIo4(UINTN Base, UINT16 Size)
{
  UINT16  Index;

  DEBUG((EFI_D_ERROR, "%a(%X,%X)\n", __FUNCTION__, Base, Size));
  for (Index=0; Index < Size; Index+=4) {
    DEBUG((EFI_D_ERROR, "%08X ", IoRead32(Base+Index)));
    if((Index+4)%16==0){
      DEBUG((EFI_D_ERROR, "\n"));
    }
  }
  DEBUG((EFI_D_ERROR, "\n"));
}


VOID DumpAllPciIntLinePin()
{
  UINTN   Base;
  UINT8   Bus, Dev, Func;
  UINT8   IntPin;

  for(Bus=0;Bus<64;Bus++){
    for(Dev=0;Dev<32;Dev++){
      for(Func=0;Func<8;Func++){
        Base = PCI_DEV_MMBASE(Bus, Dev, Func);
        if(MmioRead16(Base+0) == 0xFFFF){    // not present
          if(Func == 0){
            Func = 7;
          }
          continue;
        }
        IntPin = MmioRead8(Base+0x3D);
        if(IntPin != 0){
          DEBUG((EFI_D_ERROR, "(%02X,%02X,%02X) %02X(%c) -> %02X\n",
                               Bus, Dev, Func,
                               IntPin,
                               IntPin + 'A' - 1,
                               MmioRead8(Base+0x3C)
                               ));
        }
        if(Func == 0 && !(MmioRead8(Base + 0xE) & BIT7)){
          Func = 7;
        }
      }
    }
  }
}


VOID DumpAllPci()
{
  UINTN   Base;
  UINT8   Bus, Dev, Func;

  for(Bus=0;Bus<64;Bus++){                 // assume max bus number is 64
    for(Dev=0;Dev<32;Dev++){
      for(Func=0;Func<8;Func++){
        Base = PCI_DEV_MMBASE(Bus, Dev, Func);
        if(MmioRead16(Base+0) == 0xFFFF){    // not present
          if(Func == 0){
            Func = 7;
          }
          continue;
        }

        DumpPci(Bus, Dev, Func);

        if(Func == 0 && !(MmioRead8(Base + 0xE) & BIT7)){
          Func = 7;
        }
      }
    }
  }
  DEBUG((EFI_D_ERROR, "\n"));  
}


VOID DumpHob()
{
  EFI_PEI_HOB_POINTERS  Hob;

  for (Hob.Raw = GetHobList(); !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
	  DEBUG((EFI_D_ERROR, "T:%d A:%X\n", GET_HOB_TYPE(Hob), Hob));
  }
}



//---------------------------------- PS2 ---------------------------------------
#define  CMD_READ_OUTPUT_PORT     0xD0
#define  CMD_WRITE_OUTPUT_PORT    0xD1
#define    OUTPUT_PORT_GATEA20      BIT1
#define  KBC_DISMS_INF           0xA7
#define  KBC_ENMS_INF            0xA8
#define  KBC_KB_INF_ST           0xAB
#define  KBC_DISKEYBD_INF        0xAD
#define  KBC_ENAKEYBD_INF        0xAE
#define  KBC_WRITMSD_BYTE        0xD4
#define  MS_READ_DEV_TYPE        0xF2
#define  MS_DISABLE_REPORT       0xF5
#define  KB_ECHO                 0xEE
#define  KB_ENABLE_SCANING       0xF4
#define  KBC_ACK_STATUS          0xFA

#define KBC_STS_ERR0   0
#define KBC_STS_ERR1   0xFF
#define KBC_CMD_PORT   0x64
#define KBC_STS_PORT   0x64
#define KBC_OUT_BUFFUL BIT0
#define KBC_IN_BUFFUL  BIT1  
#define KBC_DAT_PORT   0x60
#define KBC_TIMEOUT    65536   // 0.07s

EFI_STATUS KbcWaitInputBufferFree()
{
  UINT32  TimeOut;
  
  for (TimeOut = 0; TimeOut < KBC_TIMEOUT; TimeOut += 30) {
    if (!(IoRead8(KBC_STS_PORT) & KBC_IN_BUFFUL)) {
      break;
    }
    MicroSecondDelay(30);
  }
  if(TimeOut == KBC_TIMEOUT){
    return EFI_TIMEOUT;
  }else{
    return EFI_SUCCESS;
  }
}

EFI_STATUS KbcWaitOutputBufferFull()
{
  UINT32  TimeOut;
  
  for (TimeOut = 0; TimeOut < KBC_TIMEOUT; TimeOut += 30) {
    if (IoRead8(KBC_STS_PORT) & KBC_OUT_BUFFUL) {
      break;
    }
    MicroSecondDelay(30);
  }
  if(TimeOut == KBC_TIMEOUT){
    return EFI_TIMEOUT;
  }else{
    return EFI_SUCCESS;
  }
}

EFI_STATUS KbcWaitOutputBufferFree()
{
  UINT32  TimeOut;
  UINT8   Data8;
  
  for (TimeOut = 0; TimeOut < KBC_TIMEOUT; TimeOut += 30) {
    if (!(IoRead8(KBC_STS_PORT) & KBC_OUT_BUFFUL)) {
      break;
    }
    Data8 = IoRead8(KBC_DAT_PORT);
  }
  if(TimeOut == KBC_TIMEOUT){
    return EFI_DEVICE_ERROR;
  }else{
    return EFI_SUCCESS;
  }
}

EFI_STATUS KbcSendCmd(UINT8 Cmd)
{
  EFI_STATUS  Status;

  Status = KbcWaitInputBufferFree();
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  IoWrite8(KBC_CMD_PORT, Cmd);
  Status = KbcWaitInputBufferFree();
  
ProcExit:
  return Status;  
}


BOOLEAN CheckKbcPresent(VOID)
{
  UINT8   KbcSts;

  KbcSts = IoRead8 (KBC_CMD_PORT);

  if (KbcSts == KBC_STS_ERR0) {
    return FALSE;
  }

  if (KbcSts == KBC_STS_ERR1) {
    return FALSE;
  }

  return TRUE;
}


EFI_STATUS
KbcCmdReadData (
  IN  UINT8  Cmd,
  OUT UINT8  *Data  OPTIONAL
  )
{
  EFI_STATUS  Status;
  BOOLEAN     IntState;
  UINT8       Data8;

  IntState = SaveAndDisableInterrupts();  
  
  if(!CheckKbcPresent()){
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }
  
  Status = KbcWaitOutputBufferFree();
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  
  
  Status = KbcSendCmd(Cmd);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  
  Status = KbcWaitOutputBufferFull();
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  Data8 = IoRead8(KBC_DAT_PORT);
  if(Data != NULL){
    *Data = Data8;
  }

ProcExit: 
  SetInterruptState(IntState);
  return Status;
}


VOID IsPs2DevicePresent(EFI_BOOT_SERVICES *BS, BOOLEAN *Ps2Kb, BOOLEAN *Ps2Ms)
{
  EFI_STATUS  Status;
  BOOLEAN     KbInfDis = FALSE;
  BOOLEAN     MsInfDis = FALSE;
  UINTN       Loops;
  UINTN       ErrCount;
  EFI_TPL     OldTpl;
  UINT8       Data8;
  
 
  *Ps2Kb = FALSE;
  *Ps2Ms = FALSE;

  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  OldTpl = BS->RaiseTPL(TPL_HIGH_LEVEL);

  if(!CheckKbcPresent()){
    goto ProcExit;
  }

  Status = KbcSendCmd(KBC_DISKEYBD_INF);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  } 
  KbInfDis = TRUE;
  
  Status = KbcSendCmd(KBC_DISMS_INF);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  } 
  MsInfDis = TRUE;

  Loops = 20;
  ErrCount = 0;
  while((--Loops) && ErrCount < 3){
    Status = KbcWaitOutputBufferFree();
    if(EFI_ERROR(Status)){
      DEBUG((EFI_D_INFO, "(L%d) %r\n", __LINE__, Status));
      continue;
    }
    Status = KbcWaitInputBufferFree();
    if(EFI_ERROR(Status)){
      DEBUG((EFI_D_INFO, "(L%d) %r\n", __LINE__, Status));
      continue;
    }
    
    Status = KbcSendCmd(KBC_WRITMSD_BYTE);
    if(EFI_ERROR(Status)){
      DEBUG((EFI_D_INFO, "(L%d) %r\n", __LINE__, Status));
      continue;
    }
    IoWrite8(KBC_DAT_PORT, MS_READ_DEV_TYPE);
    Status = KbcWaitOutputBufferFull();
    if(EFI_ERROR(Status)){
      DEBUG((EFI_D_INFO, "(L%d) %r\n", __LINE__, Status));
      continue;
    }
    Data8 = IoRead8(KBC_DAT_PORT);
    if(Data8 != 0xFA){
      ErrCount++;
      DEBUG((EFI_D_INFO, "(L%d) %X\n", __LINE__, Data8));
      continue;
    }
    
    Status = KbcWaitOutputBufferFull();
    if(EFI_ERROR(Status)){
      DEBUG((EFI_D_INFO, "(L%d) %r\n", __LINE__, Status));
      continue;
    }
    Data8 = IoRead8(KBC_DAT_PORT);
    if (Data8 == 0x00) {
      *Ps2Ms = TRUE;
      break;
    } else {
      ErrCount++;
      DEBUG((EFI_D_INFO, "(L%d) %X\n", __LINE__, Data8));
    }
  }

  DEBUG((EFI_D_INFO, "Loops:%d, ErrCount:%d, Ps2Ms:%d\n", Loops, ErrCount, *Ps2Ms));


  Loops = 20;
  ErrCount = 0;  
  while((--Loops) && ErrCount < 3){
    Status = KbcWaitOutputBufferFree();
    if(EFI_ERROR(Status)){
      DEBUG((EFI_D_INFO, "(L%d) %r\n", __LINE__, Status));
      continue;
    }
    Status = KbcWaitInputBufferFree();
    if(EFI_ERROR(Status)){
      DEBUG((EFI_D_INFO, "(L%d) %r\n", __LINE__, Status));
      continue;
    }
    
    IoWrite8(KBC_DAT_PORT, KB_ECHO);
    Status = KbcWaitOutputBufferFull();
    if(EFI_ERROR(Status)){
      DEBUG((EFI_D_INFO, "(L%d) %r\n", __LINE__, Status));
      continue;
    }
    Data8 = IoRead8(KBC_DAT_PORT);
    if(Data8 == KB_ECHO) {
      *Ps2Kb = TRUE;
      break;
    } else {
      ErrCount++;
      DEBUG((EFI_D_INFO, "(L%d) %X\n", __LINE__, Data8));
    }
  }

  DEBUG((EFI_D_INFO, "Loops:%d, ErrCount:%d, Ps2Kb:%d\n", Loops, ErrCount, *Ps2Kb));  


ProcExit:
  if(KbInfDis){
    Status = KbcSendCmd(KBC_ENAKEYBD_INF);
    DEBUG((EFI_D_INFO, "(L%d) %r\n", __LINE__, Status));
  }
  if(MsInfDis){
    Status = KbcSendCmd(KBC_ENMS_INF);
    DEBUG((EFI_D_INFO, "(L%d) %r\n", __LINE__, Status));
  }
  BS->RestoreTPL(OldTpl);
  return;
}









//--------------------------------- CRC32 --------------------------------------
STATIC UINT32 gCrcTable[256] = {
  0x00000000, 
  0x77073096, 
  0xEE0E612C, 
  0x990951BA, 
  0x076DC419, 
  0x706AF48F, 
  0xE963A535, 
  0x9E6495A3, 
  0x0EDB8832, 
  0x79DCB8A4, 
  0xE0D5E91E, 
  0x97D2D988, 
  0x09B64C2B, 
  0x7EB17CBD, 
  0xE7B82D07, 
  0x90BF1D91, 
  0x1DB71064, 
  0x6AB020F2, 
  0xF3B97148, 
  0x84BE41DE, 
  0x1ADAD47D, 
  0x6DDDE4EB, 
  0xF4D4B551, 
  0x83D385C7, 
  0x136C9856, 
  0x646BA8C0, 
  0xFD62F97A, 
  0x8A65C9EC, 
  0x14015C4F, 
  0x63066CD9, 
  0xFA0F3D63, 
  0x8D080DF5, 
  0x3B6E20C8, 
  0x4C69105E, 
  0xD56041E4, 
  0xA2677172, 
  0x3C03E4D1, 
  0x4B04D447, 
  0xD20D85FD, 
  0xA50AB56B, 
  0x35B5A8FA, 
  0x42B2986C, 
  0xDBBBC9D6, 
  0xACBCF940, 
  0x32D86CE3, 
  0x45DF5C75, 
  0xDCD60DCF, 
  0xABD13D59, 
  0x26D930AC, 
  0x51DE003A, 
  0xC8D75180, 
  0xBFD06116, 
  0x21B4F4B5, 
  0x56B3C423, 
  0xCFBA9599, 
  0xB8BDA50F, 
  0x2802B89E, 
  0x5F058808, 
  0xC60CD9B2, 
  0xB10BE924, 
  0x2F6F7C87, 
  0x58684C11, 
  0xC1611DAB, 
  0xB6662D3D, 
  0x76DC4190, 
  0x01DB7106, 
  0x98D220BC, 
  0xEFD5102A, 
  0x71B18589, 
  0x06B6B51F, 
  0x9FBFE4A5, 
  0xE8B8D433, 
  0x7807C9A2, 
  0x0F00F934, 
  0x9609A88E, 
  0xE10E9818, 
  0x7F6A0DBB, 
  0x086D3D2D, 
  0x91646C97, 
  0xE6635C01, 
  0x6B6B51F4, 
  0x1C6C6162, 
  0x856530D8, 
  0xF262004E, 
  0x6C0695ED, 
  0x1B01A57B, 
  0x8208F4C1, 
  0xF50FC457, 
  0x65B0D9C6, 
  0x12B7E950, 
  0x8BBEB8EA, 
  0xFCB9887C, 
  0x62DD1DDF, 
  0x15DA2D49, 
  0x8CD37CF3, 
  0xFBD44C65, 
  0x4DB26158, 
  0x3AB551CE, 
  0xA3BC0074, 
  0xD4BB30E2, 
  0x4ADFA541, 
  0x3DD895D7, 
  0xA4D1C46D, 
  0xD3D6F4FB, 
  0x4369E96A, 
  0x346ED9FC, 
  0xAD678846, 
  0xDA60B8D0, 
  0x44042D73, 
  0x33031DE5, 
  0xAA0A4C5F, 
  0xDD0D7CC9, 
  0x5005713C, 
  0x270241AA, 
  0xBE0B1010, 
  0xC90C2086, 
  0x5768B525, 
  0x206F85B3, 
  0xB966D409, 
  0xCE61E49F, 
  0x5EDEF90E, 
  0x29D9C998, 
  0xB0D09822, 
  0xC7D7A8B4, 
  0x59B33D17, 
  0x2EB40D81, 
  0xB7BD5C3B, 
  0xC0BA6CAD, 
  0xEDB88320, 
  0x9ABFB3B6, 
  0x03B6E20C, 
  0x74B1D29A, 
  0xEAD54739, 
  0x9DD277AF, 
  0x04DB2615, 
  0x73DC1683, 
  0xE3630B12, 
  0x94643B84, 
  0x0D6D6A3E, 
  0x7A6A5AA8, 
  0xE40ECF0B, 
  0x9309FF9D, 
  0x0A00AE27, 
  0x7D079EB1, 
  0xF00F9344, 
  0x8708A3D2, 
  0x1E01F268, 
  0x6906C2FE, 
  0xF762575D, 
  0x806567CB, 
  0x196C3671, 
  0x6E6B06E7, 
  0xFED41B76, 
  0x89D32BE0, 
  0x10DA7A5A, 
  0x67DD4ACC, 
  0xF9B9DF6F, 
  0x8EBEEFF9, 
  0x17B7BE43, 
  0x60B08ED5, 
  0xD6D6A3E8, 
  0xA1D1937E, 
  0x38D8C2C4, 
  0x4FDFF252, 
  0xD1BB67F1, 
  0xA6BC5767, 
  0x3FB506DD, 
  0x48B2364B, 
  0xD80D2BDA, 
  0xAF0A1B4C, 
  0x36034AF6, 
  0x41047A60, 
  0xDF60EFC3, 
  0xA867DF55, 
  0x316E8EEF, 
  0x4669BE79, 
  0xCB61B38C, 
  0xBC66831A, 
  0x256FD2A0, 
  0x5268E236, 
  0xCC0C7795, 
  0xBB0B4703, 
  0x220216B9, 
  0x5505262F, 
  0xC5BA3BBE, 
  0xB2BD0B28, 
  0x2BB45A92, 
  0x5CB36A04, 
  0xC2D7FFA7, 
  0xB5D0CF31, 
  0x2CD99E8B, 
  0x5BDEAE1D, 
  0x9B64C2B0, 
  0xEC63F226, 
  0x756AA39C, 
  0x026D930A, 
  0x9C0906A9, 
  0xEB0E363F, 
  0x72076785, 
  0x05005713, 
  0x95BF4A82, 
  0xE2B87A14, 
  0x7BB12BAE, 
  0x0CB61B38, 
  0x92D28E9B, 
  0xE5D5BE0D, 
  0x7CDCEFB7, 
  0x0BDBDF21, 
  0x86D3D2D4, 
  0xF1D4E242, 
  0x68DDB3F8, 
  0x1FDA836E, 
  0x81BE16CD, 
  0xF6B9265B, 
  0x6FB077E1, 
  0x18B74777, 
  0x88085AE6, 
  0xFF0F6A70, 
  0x66063BCA, 
  0x11010B5C, 
  0x8F659EFF, 
  0xF862AE69, 
  0x616BFFD3, 
  0x166CCF45, 
  0xA00AE278, 
  0xD70DD2EE, 
  0x4E048354, 
  0x3903B3C2, 
  0xA7672661, 
  0xD06016F7, 
  0x4969474D, 
  0x3E6E77DB, 
  0xAED16A4A, 
  0xD9D65ADC, 
  0x40DF0B66, 
  0x37D83BF0, 
  0xA9BCAE53, 
  0xDEBB9EC5, 
  0x47B2CF7F, 
  0x30B5FFE9, 
  0xBDBDF21C, 
  0xCABAC28A, 
  0x53B39330, 
  0x24B4A3A6, 
  0xBAD03605, 
  0xCDD70693, 
  0x54DE5729, 
  0x23D967BF, 
  0xB3667A2E, 
  0xC4614AB8, 
  0x5D681B02, 
  0x2A6F2B94, 
  0xB40BBE37, 
  0xC30C8EA1, 
  0x5A05DF1B, 
  0x2D02EF8D
};


EFI_STATUS 
EFIAPI
LibCalcCrc32 (
  IN  VOID    *Data,
  IN  UINTN   DataSize,
  OUT UINT32  *CrcOut
  )
{
  UINT32  Crc;
  UINTN   Index;
  UINT8   *Ptr;

  if (Data == NULL || DataSize == 0 || CrcOut == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Crc = 0xffffffff;
  for (Index = 0, Ptr = Data; Index < DataSize; Index++, Ptr++) {
    Crc = (Crc >> 8) ^ gCrcTable[(UINT8) Crc ^ *Ptr];
  }

  *CrcOut = Crc ^ 0xffffffff;
  return EFI_SUCCESS;
}


BOOLEAN LibVerifyDataCrc32(VOID *Data, UINTN DataSize, UINTN CrcOffset, UINT32 *CalcCrc32 OPTIONAL)
{
  UINT32      OldCrc32;
  UINT32      Crc32;
  EFI_STATUS  Status;
  BOOLEAN     rc;
  UINT32      *CrcPos;
  
  CrcPos   = (UINT32*)((UINT8*)Data+CrcOffset);
  OldCrc32 = *CrcPos;
  *CrcPos  = 0;
  rc       = FALSE;
  
  Status  = LibCalcCrc32(Data, DataSize, &Crc32);
  *CrcPos = OldCrc32;
  if(CalcCrc32 != NULL){
    *CalcCrc32 = Crc32;
  }
    
  if((!EFI_ERROR(Status)) && (Crc32==OldCrc32)){
    rc = TRUE;
  }

  return rc;
}






//-------------------------------- Azalia --------------------------------------
#define REG_HDA_GCAP            0x00      // 2
#define REG_HDA_VMIN            0x02      // 1
#define REG_HDA_VMAJ            0x03      // 1
#define REG_HDA_OUTPAY          0x04      // 2
#define REG_HDA_INPAY           0x06      // 2
#define REG_HDA_GCTL            0x08      // 4
#define   REG_HDA_GCTL_RESET      BIT0
#define REG_HDA_WAKEEN          0x0C      // 2
#define REG_HDA_STATESTS        0x0E      // 2
#define REG_HDA_GSTS            0x10      // 2
#define REG_HDA_IC              0x60      // 4
#define REG_HDA_IR              0x64      // 4
#define REG_HDA_IRS             0x68      // 4
#define   REG_HDA_IRS_BUSY        BIT0
#define   REG_HDA_IRS_VALID       BIT1
#define REG_HDA_OSD0CTL         0x100     // 3
#define REG_HDA_OSD0CBL         0x108     // 4
#define REG_HDA_OSD0LVI         0x10C     // 2
#define REG_HDA_OSD0FIFOS       0x110     // 2
#define REG_HDA_OSD0FMT         0x112     // 2
#define REG_HDA_OSD0BDPL        0x118     // 4
#define REG_HDA_OSD0BDPU        0x11C     // 4

#define ROOT_NODE_ID		                0x00

#define VERB_GET_PARAMETER  		        0xF00
#define PARAMETER_ID_VENDOR_ID          0x00

#define CREATE_VERB12_FOEMAT(CAd, NodeID, VerbID, PayLoad) \
  (UINT32)((((CAd)&0x0F)<<28) | (((NodeID)&0xFF)<<20) | (((VerbID)&0xFFF)<<8) | ((PayLoad)&0xFF))

#define AZALIA_MAX_SID_NUMBER     2
#define AZALIA_MAX_SID_MASK       ((1 << AZALIA_MAX_SID_NUMBER) - 1)
#define AZALIA_MAX_LOOP_TIME      10
#define AZALIA_WAIT_PERIOD        100
#define AZALIA_RESET_WAIT_TIME    300

  
STATIC 
EFI_STATUS 
AzaliaSendCommand (
  UINTN   HdaBar, 
  UINT32  Verb, 
  UINT32  *Res  OPTIONAL
  )
{
  UINTN       TimeOut;
  UINTN       Address;
  EFI_STATUS  Status;

//DEBUG((EFI_D_INFO, "Verb:%08X\n", Verb));  
  Status  = EFI_SUCCESS;  
  TimeOut = AZALIA_MAX_LOOP_TIME;
  Address = HdaBar + REG_HDA_IRS;
  while(TimeOut--){
    if(!(MmioRead32(Address)&REG_HDA_IRS_BUSY)){
      break;
    }
    MicroSecondDelay(AZALIA_WAIT_PERIOD);
  }
  if(MmioRead32(Address)&REG_HDA_IRS_BUSY){
    Status = EFI_DEVICE_ERROR;
    goto ProcExit;
  }
        
  MmioWrite32(Address, MmioRead32(Address)|REG_HDA_IRS_VALID);  // clear
  MmioWrite32(HdaBar+REG_HDA_IC, Verb);
  MmioWrite32(Address, MmioRead32(Address)|REG_HDA_IRS_BUSY);   // start
  
  TimeOut = AZALIA_MAX_LOOP_TIME;
  while(TimeOut--){
    if(MmioRead32(Address)&REG_HDA_IRS_VALID){
      break;
    }
    MicroSecondDelay(AZALIA_WAIT_PERIOD);
  }
  if(!(MmioRead32(Address)&REG_HDA_IRS_VALID)){
    Status = EFI_DEVICE_ERROR;
    goto ProcExit;    
  }
  
  if(Res!=NULL){
    *Res = MmioRead32(HdaBar+REG_HDA_IR);
  }

ProcExit:
  return Status;    
}


STATIC EFI_STATUS AzaliaReset(UINTN HdaBar)
{
  UINTN       TimeOut;
  UINTN       Address;
  EFI_STATUS  Status;
  
  Status = EFI_SUCCESS;
  
// 1. clear STATESTS
  Address = HdaBar + REG_HDA_STATESTS;
  MmioOr8(Address, AZALIA_MAX_SID_MASK);

// 2. reset controller
  Address = HdaBar + REG_HDA_GCTL;
  MmioWrite32(Address, MmioRead32(Address) & ~REG_HDA_GCTL_RESET);
	TimeOut = AZALIA_MAX_LOOP_TIME;
	while((MmioRead32(Address)&REG_HDA_GCTL_RESET) && TimeOut--){
		MicroSecondDelay(AZALIA_WAIT_PERIOD);
	}
  if(MmioRead32(Address)&REG_HDA_GCTL_RESET){
    Status = EFI_DEVICE_ERROR;
    goto ProcExit;    
  }

  MicroSecondDelay(AZALIA_RESET_WAIT_TIME);  
  
// 3. Bring controller out of reset
  Address = HdaBar + REG_HDA_GCTL;
  MmioWrite32(Address, MmioRead32(Address) | REG_HDA_GCTL_RESET);
	TimeOut = AZALIA_MAX_LOOP_TIME;
	while((!(MmioRead32(Address)&REG_HDA_GCTL_RESET)) && TimeOut--){
		MicroSecondDelay(AZALIA_WAIT_PERIOD);
	}
  if(!(MmioRead32(Address)&REG_HDA_GCTL_RESET)){
    Status = EFI_DEVICE_ERROR;
    goto ProcExit;    
  }
  
ProcExit:
  return Status;
}



STATIC UINTN GetHdaBar(UINTN HostPcieAddr)
{
  DATA_64  Data64;
  UINTN    HdaBar;

  Data64.Uint64 = 0;
  Data64.Uint32.Lower32 = MmioRead32(HostPcieAddr + 0x10);

  if((Data64.Uint32.Lower32 & (BIT1 + BIT2)) == BIT2){         // 64bit Address.
    Data64.Uint32.Upper32 = MmioRead32(HostPcieAddr + 0x14);
  }
  HdaBar = (UINTN)(Data64.Uint64 & ~(BIT0 + BIT1 + BIT2 + BIT3));
  ASSERT(HdaBar!=0);
  ASSERT(HdaBar < SIZE_4GB);
  
  return HdaBar;
}


// Only handle the first found codec.
EFI_STATUS 
AzaliaLoadVerbTable (
  IN UINTN           HostPcieAddr,
  IN VOID            *VerbTable, 
  IN UINTN           VerbTableSize
  )
{
	EFI_STATUS		      Status;
  UINTN               HdaBar;
  UINT16              CodecMask;  
  UINT8               CAd;
  UINT32              Verb;
  UINT32              RetVal;
  UINTN               TableIndex;
  UINTN               TableCount;  
  UINTN               VerbIndex;
  UINTN               VerbCount;
  UINT32              *VerbData;
  UINTN               LoopTime;
  OEM_VERB_TABLE      *OemVerbTable;
  UINTN               VerbRealCount;
  
  
  ASSERT(VerbTable!=NULL && VerbTableSize!=0);

  if(MmioRead16(HostPcieAddr)==0xFFFF){
    DEBUG((EFI_D_INFO, "Azalia Not Present\n"));
    Status = EFI_SUCCESS;
    goto ProcExit;		
  }		

  HdaBar = GetHdaBar(HostPcieAddr);
  DEBUG((EFI_D_INFO, "HdaBar:%X\n", HdaBar));  
  Status = AzaliaReset(HdaBar);
  ASSERT_EFI_ERROR(Status);
  
  CodecMask = 0;
  for (LoopTime = 0; LoopTime < AZALIA_MAX_LOOP_TIME; LoopTime++) {
    CodecMask = MmioRead16(HdaBar + REG_HDA_STATESTS) & AZALIA_MAX_SID_MASK;
    if(CodecMask!=0){
      break;
    } 
    MicroSecondDelay(AZALIA_WAIT_PERIOD);
  }  
  if(!CodecMask){
    DEBUG((EFI_D_ERROR, "No Codec Detect!\n"));
    goto ProcExit;
  }

  OemVerbTable = (OEM_VERB_TABLE*)VerbTable;
  TableCount = VerbTableSize/sizeof(OEM_VERB_TABLE);  
  for(CAd=0;CAd<AZALIA_MAX_SID_NUMBER;CAd++){
    if(!((UINT16)(1<<CAd) & CodecMask)){
      continue;
    }
    Verb   = CREATE_VERB12_FOEMAT(CAd, ROOT_NODE_ID, VERB_GET_PARAMETER, PARAMETER_ID_VENDOR_ID);
    Status = AzaliaSendCommand(HdaBar, Verb, &RetVal);
    ASSERT_EFI_ERROR(Status);
    DEBUG((EFI_D_INFO, "SDI[%d] %08X\n", CAd, RetVal));
    
    for(TableIndex=0;TableIndex<TableCount;TableIndex++){
      if(RetVal != OemVerbTable[TableIndex].Hdr.Vdid){
        continue;
      }
      VerbCount = OemVerbTable[TableIndex].VerbDataSize/sizeof(UINT32);
      VerbData  = OemVerbTable[TableIndex].VerbData;
      VerbRealCount = 0;
      for(VerbIndex=0;VerbIndex<VerbCount;VerbIndex++){
        if(VerbData[VerbIndex] == 0 || VerbData[VerbIndex] == 0xFFFFFFFF){
          continue;
        }
        Verb   = (VerbData[VerbIndex] & 0x0FFFFFFF) | (CAd<<28);
        Status = AzaliaSendCommand(HdaBar, Verb, &RetVal);
        ASSERT_EFI_ERROR(Status);
        VerbRealCount++;
      }
      DEBUG((EFI_D_INFO, "VerbRealCount:%d\n", VerbRealCount));
      break;
    }
    if(TableIndex>=TableCount){
      DEBUG((EFI_D_ERROR, "VerbTable Not Found!\n"));
    }
    break;
  }
  
ProcExit:
  return Status;  
}




// I am a common library, I can not use Arch dependent library.
// So below gBS should be passed from caller.
VOID *LibGetGOP(EFI_BOOT_SERVICES *pBS)
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *ptGO;
  VOID                                  *Interface;
  EFI_STATUS                            Status;  
  EFI_HANDLE                            *HandleBuffer;
  UINTN                                 HandleCount;
  UINTN                                 Index;  

  HandleBuffer = NULL;
  ptGO         = NULL;
  
  Status = pBS->LocateHandleBuffer (
              ByProtocol,
              &gEfiGraphicsOutputProtocolGuid,
              NULL,
              &HandleCount,
              &HandleBuffer
              );
  if(EFI_ERROR(Status) || HandleCount==0){
    goto ProcExit;
  }  
              
  for(Index=0;Index<HandleCount;Index++){
    Status = pBS->HandleProtocol(
               HandleBuffer[Index], 
               &gEfiConsoleOutDeviceGuid, 
               &Interface
               );
    if(EFI_ERROR(Status)){
      continue;
    }
    Status = pBS->HandleProtocol(
               HandleBuffer[Index], 
               &gEfiGraphicsOutputProtocolGuid, 
               &ptGO
               );
    ASSERT(!EFI_ERROR(Status) && ptGO!=NULL);               
  }

ProcExit:
  pBS->FreePool(HandleBuffer);
  return (VOID*)ptGO;
}



EFI_STATUS BltSaveAndRetore(VOID *BootServices, BOOLEAN Save)
{
         EFI_STATUS                     Status = EFI_SUCCESS;
         EFI_GRAPHICS_OUTPUT_PROTOCOL   *ptGO;
  STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *gBltBuffer = NULL;
         UINTN                          Width;
         UINTN                          Height;
         EFI_BOOT_SERVICES              *pBS;      
  

  pBS = (EFI_BOOT_SERVICES*)BootServices;
  ASSERT(pBS->Hdr.Signature == EFI_BOOT_SERVICES_SIGNATURE);
  
  ptGO = (EFI_GRAPHICS_OUTPUT_PROTOCOL*)LibGetGOP(pBS);
  if(ptGO == NULL){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }    
  
  Width  = ptGO->Mode->Info->HorizontalResolution;
  Height = ptGO->Mode->Info->VerticalResolution;

  if(Save){
    if(gBltBuffer != NULL){
      pBS->FreePool(gBltBuffer);
      gBltBuffer = NULL;
    }  
    Status = pBS->AllocatePool(
                    EfiBootServicesData,
                    sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)*Width*Height,
                    &gBltBuffer
                    );
    if(EFI_ERROR(Status)){
      Status = EFI_OUT_OF_RESOURCES;
      goto ProcExit;		
    }	

    Status = ptGO->Blt(
                    ptGO,
                    gBltBuffer,
                    EfiBltVideoToBltBuffer,
                    0,
                    0,
                    0,
                    0,
                    Width,
                    Height,
                    Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)   // the number of bytes in a row of the BltBuffer.
                    );  
    DEBUG((EFI_D_INFO, "BltSave:%r\n", Status));    
  } else {
    
    if(gBltBuffer == NULL){
      Status = EFI_INVALID_PARAMETER;
      goto ProcExit;	      
    }
    Status = ptGO->Blt(
                    ptGO,
                    gBltBuffer,
                    EfiBltBufferToVideo,
                    0,
                    0,
                    0,
                    0,
                    Width,
                    Height,
                    Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                    );
    DEBUG((EFI_D_INFO, "BltRestore:%r\n", Status));
    pBS->FreePool(gBltBuffer);
    gBltBuffer = NULL;    
  }
	
ProcExit:
  return Status;	
}



VOID 
AcpiTableUpdateChksum (
  IN VOID *AcpiTable
  )
{
  UINTN  ChecksumOffset;
  UINT8  *Buffer;
  EFI_ACPI_COMMON_HEADER  *Hdr;

  Hdr = (EFI_ACPI_COMMON_HEADER*)AcpiTable;
  if(Hdr->Signature == EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE){
    return;
  }  

  Buffer = (UINT8*)Hdr;
  ChecksumOffset = OFFSET_OF(EFI_ACPI_DESCRIPTION_HEADER, Checksum);
  Buffer[ChecksumOffset] = 0;
  Buffer[ChecksumOffset] = CalculateCheckSum8(Buffer, Hdr->Length);
}




UINTN
MyGetDevicePathSize (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CONST EFI_DEVICE_PATH_PROTOCOL  *Start;
        UINTN                     Count = 0;
        UINTN                     NodeSize;

  if (DevicePath == NULL) {
    return 0;
  }

  //
  // Search for the end of the device path structure
  //
  Start = DevicePath;
  while (!(DevicePath->Type == END_DEVICE_PATH_TYPE && 
           DevicePath->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE)) {
    NodeSize = (DevicePath->Length[1]<<8) + DevicePath->Length[0];
    if(NodeSize < 4){
      return 0;
    }
    Count++;
    if(Count > 1000){
      return 0;
    }
    DevicePath = (EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)DevicePath + NodeSize);
  }

  return ((UINTN) DevicePath - (UINTN)Start) + (DevicePath->Length[1]<<8) + DevicePath->Length[0];
}






VOID
ShowDevicePathDxe (
  IN EFI_BOOT_SERVICES         *BS,
  IN EFI_DEVICE_PATH_PROTOCOL  *ptDevPath
  )
{
  EFI_STATUS                               Status;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL         *ptDP2Txt;
  CHAR16                                   *Str;
  CHAR16                                   *Alloc = NULL;


  if(ptDevPath==NULL || BS == NULL){
    goto ProcExit;  
  }

  Status = BS->LocateProtocol(
                  &gEfiDevicePathToTextProtocolGuid,
                  NULL,
                  &ptDP2Txt
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  
  if(ptDevPath->Type == END_DEVICE_PATH_TYPE &&
     ptDevPath->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE){
    Str = L"(End)";
  }else{
    Str = ptDP2Txt->ConvertDevicePathToText (
                      ptDevPath,
                      FALSE,
                      TRUE
                      );
    Alloc = Str;
  }
  
  if(Str!=NULL){
    DEBUG((EFI_D_INFO, "DP:%s\n", Str));
  }

ProcExit:
  if(Alloc!=NULL){BS->FreePool(Alloc);}
  return;
}





VOID
SwapWordArray (
  IN  UINT8   *Data,
  IN  UINTN   DataSize
  )
{
  UINTN  Index;
  UINT8  Data8;

  ASSERT(DataSize!=0 && Data!=NULL);

// here has a hidden bug:
// Data size may be odd number, so it will destroy data out of range.
  DataSize &= ~BIT0;          // Fix: make it be even.
  for(Index = 0; Index < DataSize; Index += 2){
    Data8         = Data[Index];
    Data[Index]   = Data[Index+1];
    Data[Index+1] = Data8;
  }

}



CHAR8 *TrimStr8(CHAR8 *Str)
{
  UINTN    StringLength;
  CHAR8    *NewString;
  BOOLEAN  HasLeading;
  UINTN    SrcIndex;
  UINTN    TarIndex;
  
  NewString = NULL;

  StringLength = AsciiStrLen(Str);
  if(StringLength==0){
    return Str; 
  }

  NewString = AllocatePool(StringLength + 1);
  if(NewString==NULL){
    return Str; 
  }

  HasLeading = TRUE;
  TarIndex   = 0;
  SrcIndex   = 0;
  for(;SrcIndex<StringLength;SrcIndex++){
    if(HasLeading){
      if(Str[SrcIndex] == ' '){
        continue;
      }else{
        HasLeading = FALSE;
      }
    }
    if(TarIndex>0){
      if((Str[SrcIndex-1]==Str[SrcIndex]) && (Str[SrcIndex]==' ')){
        continue;
      }
    }
    NewString[TarIndex++] = Str[SrcIndex];
  }

  if(TarIndex && NewString[TarIndex-1] == L' '){
    TarIndex--;
  }
  NewString[TarIndex] = 0;
  ASSERT(TarIndex<=StringLength);
  CopyMem(Str, NewString, (TarIndex+1)*sizeof(Str[0]));
  FreePool(NewString);
  return Str;
}



CHAR16 *TrimStr16(CHAR16 *Str)
{
  UINTN    StringSize;
  UINTN    StringLength;
  CHAR16   *NewString;
  BOOLEAN  HasLeading;
  UINTN    SrcIndex;
  UINTN    TarIndex;
  
  NewString = NULL;
  
  StringSize   = StrSize(Str);
  StringLength = StrLen(Str);
  if(StringLength==0){
    return Str; 
  }

  NewString = AllocatePool(StringSize);
  if(NewString==NULL){
    return Str; 
  }

	HasLeading = TRUE;
	TarIndex   = 0;
	SrcIndex   = 0;
	for(;SrcIndex<StringLength;SrcIndex++){
		if(HasLeading){
			if(Str[SrcIndex] == L' '){
				continue;
			}else{
				HasLeading = FALSE;
			}
		}
		if(TarIndex>0){
			if((Str[SrcIndex-1]==Str[SrcIndex]) && (Str[SrcIndex]==L' ')){
				continue;
			}
		}
		NewString[TarIndex++] = Str[SrcIndex];
	}

  if(TarIndex && NewString[TarIndex-1] == L' '){
    TarIndex--;
  }
  NewString[TarIndex] = 0;
  ASSERT(TarIndex<=StringLength);
  CopyMem(Str, NewString, (TarIndex+1)*sizeof(CHAR16));
  FreePool(NewString);
	
  return Str;
}




STATIC UINT8 gInvalidMacAddress1[6] = {0, 0, 0, 0, 0, 0};
STATIC UINT8 gInvalidMacAddress2[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


// intel i350  (ethernet-controller-i350-datasheet P450)
// 0x5400 + 8*n   0x0040 + 8*n    RAL[0-15]     Receive Address Low (15:0) RW
// 0x5404 + 8*n   0x0044 + 8*n    RAH[0-15]     Receive Address High(15:0) RW
// 
// RAL [31:0]
// RAH [15:0]


EFI_STATUS
GetOnboardLanMacAddress (
  IN  EFI_BOOT_SERVICES         *pBS,
  IN  VOID                      *Dp,
  OUT UINT8                     MacAddr[6]
  )  
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
	EFI_PCI_IO_PROTOCOL       *PciIo; 
  UINT32                    PciId;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;


  DevPath = (EFI_DEVICE_PATH_PROTOCOL*)Dp;

  Status  = pBS->LocateDevicePath(&gEfiPciIoProtocolGuid, &DevPath, &Handle);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  
  
  Status = pBS->HandleProtocol(
                  Handle,
                  &gEfiPciIoProtocolGuid, 
                  &PciIo
                  );
  ASSERT(!EFI_ERROR(Status));

  Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint32, 0, 1, &PciId);
  if(PciId == 0x816810EC){
    Status = PciIo->Attributes(PciIo, EfiPciIoAttributeOperationEnable, EFI_PCI_IO_ATTRIBUTE_IO, NULL);
    Status = PciIo->Io.Read(PciIo, EfiPciIoWidthUint8, 0, 0, 6, MacAddr);
  } else if(PciId == 0x15218086){
    Status = PciIo->Attributes(PciIo, EfiPciIoAttributeOperationEnable, EFI_PCI_IO_ATTRIBUTE_MEMORY, NULL);
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, 0, 0x40, 6, MacAddr);
  } else {
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }

  if(CompareMem(MacAddr, gInvalidMacAddress1, 6) == 0 ||
     CompareMem(MacAddr, gInvalidMacAddress2, 6) == 0){
    Status = EFI_UNSUPPORTED;
    goto ProcExit;    
  }  

ProcExit:
  return Status;
}


BOOLEAN IsSataDp(VOID  *DevicePath)
{
  EFI_DEVICE_PATH_PROTOCOL  *Dp;
  UINTN                     NodeSize;
  

  ASSERT(DevicePath != NULL);

  Dp = (EFI_DEVICE_PATH_PROTOCOL*)DevicePath;
  
  while(!(Dp->Type == END_DEVICE_PATH_TYPE &&
          Dp->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE)){
    if(Dp->Type == MESSAGING_DEVICE_PATH &&
       Dp->SubType == MSG_SATA_DP){
       return TRUE;
    }

    NodeSize = (Dp->Length[1]<<8) + Dp->Length[0];
    Dp = (EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)Dp + NodeSize);
  }

  return FALSE;
}



BOOLEAN 
IsAhciHddDp(
  IN  EFI_BOOT_SERVICES       *pBS,
  IN  VOID                    *DevicePath,
  OUT VOID                    **pDiskInfo  OPTIONAL
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *Dp;
  EFI_STATUS                    Status;
  EFI_HANDLE                    DevHandle;  
  EFI_SCSI_IO_PROTOCOL          *ScsiIo;  
  EFI_DISK_INFO_PROTOCOL        *DiskInfo;
  BOOLEAN                       Rc = FALSE;


  ASSERT(DevicePath != NULL);

  Dp = DevicePath;
  Status = pBS->LocateDevicePath(&gEfiDiskInfoProtocolGuid, &Dp, &DevHandle);
  if(EFI_ERROR(Status)){
    DEBUG((EFI_D_INFO, "(L%d) %r\n", __LINE__, Status));
    goto ProcExit;
  }

  Status = pBS->HandleProtocol (
                  DevHandle,
                  &gEfiScsiIoProtocolGuid,
                  (VOID**)&ScsiIo
                  );
  if(!EFI_ERROR(Status)){
    DEBUG((EFI_D_INFO, "Ignore ScsiIo\n"));
    goto ProcExit;
  }
  
  Status = pBS->HandleProtocol (
                  DevHandle,
                  &gEfiDiskInfoProtocolGuid,
                  (VOID**)&DiskInfo
                  );
  ASSERT(!EFI_ERROR(Status));

  if(!CompareGuid(&DiskInfo->Interface, &gEfiDiskInfoAhciInterfaceGuid)){
    DEBUG((EFI_D_INFO, "Not Ahci\n"));
    goto ProcExit;
  }

  Rc = TRUE;
  if(pDiskInfo != NULL){
    *pDiskInfo = (VOID*)DiskInfo;
  }

ProcExit:
  return Rc;
}





VOID*
AllocateAcpiNvsZeroMemoryBelow4G (
  IN EFI_BOOT_SERVICES  *BS,
  IN UINTN              Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer;

  Pages = EFI_SIZE_TO_PAGES(Size);
  Address = 0xffffffff;

  Status  = BS->AllocatePages (
                   AllocateMaxAddress,
                   EfiACPIMemoryNVS,
                   Pages,
                   &Address
                   );
  ASSERT_EFI_ERROR(Status);

  Buffer = (VOID*)(UINTN)Address;
  ZeroMem(Buffer, Size);

  return Buffer;
}



VOID*
AllocateReservedZeroMemoryBelow4G (
  IN EFI_BOOT_SERVICES  *BS,
  IN UINTN              Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer = NULL;

  Pages = EFI_SIZE_TO_PAGES(Size);
  Address = 0xffffffff;

  Status  = BS->AllocatePages (
                   AllocateMaxAddress,
                   EfiReservedMemoryType,
                   Pages,
                   &Address
                   );
  if(EFI_ERROR(Status)){
    return NULL;
  }

  Buffer = (VOID*)(UINTN)Address;
  ZeroMem(Buffer, Size);

  return Buffer;
}


VOID*
AllocateReservedZeroMemoryBelow1M (
  IN EFI_BOOT_SERVICES  *BS,
  IN UINTN              Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer = NULL;

  Pages = EFI_SIZE_TO_PAGES(Size);
  Address = 0xfffff;

  Status  = BS->AllocatePages (
                   AllocateMaxAddress,
                   EfiReservedMemoryType,
                   Pages,
                   &Address
                   );
  if(EFI_ERROR(Status)){
    return NULL;
  }

  Buffer = (VOID*)(UINTN)Address;
  ZeroMem(Buffer, Size);

  return Buffer;
}


VOID*
AllocateBsZeroMemoryBelow4G (
  IN EFI_BOOT_SERVICES  *BS,
  IN UINTN              Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer = NULL;

  Pages = EFI_SIZE_TO_PAGES(Size);
  Address = 0xffffffff;

  Status  = BS->AllocatePages (
                   AllocateMaxAddress,
                   EfiBootServicesData,
                   Pages,
                   &Address
                   );
  if(EFI_ERROR(Status)){
    return NULL;
  }

  Buffer = (VOID*)(UINTN)Address;
  ZeroMem(Buffer, Size);

  return Buffer;
}



EFI_STATUS InvokeHookProtocol(EFI_BOOT_SERVICES *BS, EFI_GUID *Protocol)
{
  UINTN                 HandleCount;
  EFI_HANDLE            *Handles = NULL;
  EFI_STATUS            Status;
  UINTN                 Index;
  EFI_MY_HOOK_PROTOCOL  MyHook;
  

  Status = BS->LocateHandleBuffer (
                 ByProtocol,
                 Protocol,
                 NULL,
                 &HandleCount,
                 &Handles
                 );
  if(EFI_ERROR(Status) || HandleCount == 0){
    goto ProcExit;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = BS->HandleProtocol (
                    Handles[Index],
                    Protocol,
                    (VOID**)&MyHook
                    );
    ASSERT(!EFI_ERROR(Status));
    MyHook();
  }

ProcExit:
  if(Handles != NULL){
    FreePool(Handles);
  }
  return Status;
}



VOID
SignalProtocolEvent (
  EFI_BOOT_SERVICES *BS,
  EFI_GUID          *ProtocolGuid,
  BOOLEAN           NeedUnInstall
  )
{
  EFI_HANDLE     Handle;
  EFI_STATUS     Status;


  Handle = NULL;
  Status = BS->InstallProtocolInterface (
                  &Handle,
                  ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  if(NeedUnInstall){
    Status = BS->UninstallProtocolInterface (
                    Handle,
                    ProtocolGuid,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status); 
  }
}


