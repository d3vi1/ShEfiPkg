#include <ShadowOptionRom.h>

///Stuff imported from EFISetVGARegsDxe.c
extern EFI_VIDEO_CARD          *MyVideoCards;
extern UINTN                    NumberVideoCards;
extern EFI_HANDLE               ImageHandle;
extern EFI_BOOT_SERVICES       *BS;
extern EFI_RUNTIME_SERVICES    *RS;
extern EFI_SYSTEM_TABLE        *ST;
extern EFI_STATUS               Status;


///
///The Read, IORead, Write and IOWrite functions
///are used by the x86emulator for physical device
///access and for reading and writing to the
///1MB emulator memory buffer.
///
UINT8  Read8    (UINT32 Address){
	UINT8    *Result;
	
	Result = (UINT8 *)((UINTN)Address + (UINTN)RealModeMemory);
	return *Result;
}

UINT16 Read16   (UINT32 Address){
	UINT16   *Result;
	
	Result = (UINT16*)((UINTN)Address + (UINTN)RealModeMemory);
	return *Result;
}

UINT32 Read32   (UINT32 Address){
	UINT32    *Result;
	
	Result = (UINT32*)((UINTN)Address + (UINTN)RealModeMemory);
	return *Result;
}

void   Write8   (UINT32 Address, UINT8  Value){
	UINT8    *ValuePointer;
	
	ValuePointer = (UINT8 *)((UINTN)Address + (UINTN)RealModeMemory);
	*ValuePointer=Value;
}

void   Write16  (UINT32 Address, UINT16 Value){
	UINT16   *ValuePointer;
	
	ValuePointer = (UINT16*)((UINTN)Address + (UINTN)RealModeMemory);
	*ValuePointer=Value;
}

void   Write32  (UINT32 Address, UINT32 Value){
	UINT32   *ValuePointer;
	
	ValuePointer = (UINT32*)((UINTN)Address + (UINTN)RealModeMemory);
	*ValuePointer=Value;
}

UINT8  IORead8  (X86EMU_pioAddr Address){
	EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev;
	UINT8                             ResponseValue;
	
	ResponseValue=0;
	
	Status=BS->LocateProtocol(&gEfiPciRootBridgeIoProtocolGuid, NULL, (VOID **)&IoDev);
	
	IoDev->Io.Read(IoDev, EfiPciWidthUint8, (UINT64)Address, 1, &ResponseValue);
	
	IoDev=NULL;
	
	return ResponseValue;
}

UINT16 IORead16 (X86EMU_pioAddr Address){
	EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev;
	UINT16                            ResponseValue;
	
	ResponseValue=0;
	
	Status=BS->LocateProtocol(&gEfiPciRootBridgeIoProtocolGuid, NULL, (VOID **)&IoDev);
	
	IoDev->Io.Read(IoDev, EfiPciWidthUint16, (UINT64)Address, 1, &ResponseValue);
	
	IoDev=NULL;
	
	return ResponseValue;
}

UINT32 IORead32 (X86EMU_pioAddr Address){
	EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev;
	UINT32                            ResponseValue;
	
	ResponseValue=0;
	
	Status=BS->LocateProtocol(&gEfiPciRootBridgeIoProtocolGuid, NULL, (VOID **)&IoDev);
	
	IoDev->Io.Read(IoDev, EfiPciWidthUint32, (UINT64)Address, 1, &ResponseValue);
	
	IoDev=NULL;
	
	return ResponseValue;
}

void   IOWrite8 (X86EMU_pioAddr Address, UINT8  Value){
	EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev;
	
	Status=BS->LocateProtocol(&gEfiPciRootBridgeIoProtocolGuid, NULL, (VOID **)&IoDev);
	
	IoDev->Io.Write(IoDev, EfiPciWidthUint8, (UINT64)Address, 1, &Value);
	
	IoDev=NULL;
}

void   IOWrite16(X86EMU_pioAddr Address, UINT16 Value){
	EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev;
	
	Status=BS->LocateProtocol(&gEfiPciRootBridgeIoProtocolGuid, NULL, (VOID **)&IoDev);
	
	IoDev->Io.Write(IoDev, EfiPciWidthUint16, (UINT64)Address, 1, &Value);
	
	IoDev=NULL;
}

void   IOWrite32(X86EMU_pioAddr Address, UINT32 Value){
	EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev;
	
	Status=BS->LocateProtocol(&gEfiPciRootBridgeIoProtocolGuid, NULL, (VOID **)&IoDev);
	
	IoDev->Io.Write(IoDev, EfiPciWidthUint32, (UINT64)Address, 1, &Value);
	
	IoDev=NULL;
}

void   pushw(UINT16 val){
	X86_ESP -= 2;
	Write16(((u32) X86_SS << 4) + X86_SP, val);
}


static void x86emu_do_int(int num) {
	UINT32 eflags;
    
	return;

	eflags = X86_EFLAGS;
	eflags = eflags | X86_IF_MASK;
	pushw((UINT16)eflags);
	pushw(X86_CS);
	pushw(X86_IP);
	X86_EFLAGS = X86_EFLAGS & ~(X86_VIF_MASK | X86_TF_MASK);
	X86_CS = (Read8((num << 2) + 3) << 8) + Read8((num << 2) + 2);
	X86_IP = (Read8((num << 2) + 1) << 8) + Read8((num << 2));
}


///
///Load and execute an image from the firmware.
///   
EFI_STATUS EfiExecuteFirmwareFile(IN EFI_GUID *gEfiGuid) {
	
	EFI_HANDLE                     *HandleBuffer;
	EFI_HANDLE                      ImageFileHandle;
	EFI_FIRMWARE_VOLUME_PROTOCOL   *FirmwareVolume;
	EFI_GUID                        gEfiFirmwareVolumeProtocolGuid;
	EFI_TPL                         PreviousPriority;
	VOID                           *ImageFile;
	UINTN                           ImageFileSize;
	UINTN                           BufferSize;
	UINTN                           Index;
	UINT32                          AuthenticationStatus;
	
	ImageFileHandle = NULL;
	//Got a better idea?
	// 0x389F751F, 0x1838, 0x4388, {0x83, 0x90, 0xCD, 0x81, 0x54, 0xBD, 0x27, 0xF8} just doesn't work
	gEfiFirmwareVolumeProtocolGuid.Data1=   0x389F751F;
	gEfiFirmwareVolumeProtocolGuid.Data2=   0x1838;
	gEfiFirmwareVolumeProtocolGuid.Data3=   0x4388;
	gEfiFirmwareVolumeProtocolGuid.Data4[0]=0x83;
	gEfiFirmwareVolumeProtocolGuid.Data4[1]=0x90;
	gEfiFirmwareVolumeProtocolGuid.Data4[2]=0xCD;
	gEfiFirmwareVolumeProtocolGuid.Data4[3]=0x81;
	gEfiFirmwareVolumeProtocolGuid.Data4[4]=0x54;
	gEfiFirmwareVolumeProtocolGuid.Data4[5]=0xBD;
	gEfiFirmwareVolumeProtocolGuid.Data4[6]=0x27;
	gEfiFirmwareVolumeProtocolGuid.Data4[7]=0xF8;
	
	//Get all the FirmwarVolumeProtocol instances
	Status = BS->LocateHandleBuffer (ByProtocol, &gEfiFirmwareVolumeProtocolGuid, NULL, &BufferSize, &HandleBuffer);
	if (EFI_ERROR (Status)) { return EFI_NOT_FOUND; }
	
	//You don't work with the firmware unless you raise the task
	//priority level to NOTIFY or above. We'll restore
	//the TPL at the end of the execution.
	PreviousPriority=BS->RaiseTPL(TPL_NOTIFY);
	
	//Let's take the firmware volumes one by one.
	for (Index = 0; Index < BufferSize; Index++) {
		
		//And open them
		Status = BS->OpenProtocol ( HandleBuffer[Index], &gEfiFirmwareVolumeProtocolGuid, (VOID *) &FirmwareVolume, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
		if (EFI_ERROR (Status)) {  Print(L"Could not open firmware Volume number %d: %r\n", Index, Status); continue; }

		ImageFile = NULL;
		ImageFileSize  = 0;
				
		//Use EfiParseSectionStream for horizontal parsing. It's known to be broken.
		Status=FirmwareVolume->ReadSection(FirmwareVolume, gEfiGuid, EFI_SECTION_PE32, 0, &ImageFile, &ImageFileSize, &AuthenticationStatus);
		BS->AllocatePool(EfiBootServicesData, ImageFileSize, &ImageFile);
		Status=FirmwareVolume->ReadSection(FirmwareVolume, gEfiGuid, EFI_SECTION_PE32, 0, &ImageFile, &ImageFileSize, &AuthenticationStatus);
		
		if (EFI_ERROR(Status)){
			BS->FreePool(ImageFile);
			ImageFile=NULL;
		} else {
			Status=BS->LoadImage(FALSE, ImageHandle, NULL, ImageFile, ImageFileSize, (VOID *) &ImageFileHandle);
			if (EFI_ERROR(Status)) { Print(L"FirmwareVolume%d: Error while loading the file: %r\n", Index, Status); };
			BS->FreePool(ImageFile);
		}
		
		Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiFirmwareVolumeProtocolGuid, ImageHandle, NULL);

		//Restore Task Priority Level since we won't touch the Firmware anymore.
		BS->RestoreTPL(PreviousPriority);
		
	};//For each firmware volume
	
	BS->FreePool(HandleBuffer);
	
	if (ImageFileHandle!=NULL) {
		Status = BS->StartImage(ImageFileHandle, NULL, NULL);
		return Status;
	} else {
		return EFI_NOT_FOUND;
	}
	
}


///
///Loads the Legacy BIOS Protocol
///
void EfiCheckAndLoadLegacyProtocols(){
	EFI_GUID    gEfiLegacyBiosProtocolGuid;
	EFI_GUID    gEfiLegacyBiosDxeGuid;
	EFI_GUID    gEfiLegacyRegionProtocolGuid;
	EFI_GUID    gEfiLegacyRegionDxeGuid;
	EFI_GUID    gEfiLegacyRegionDxe2Guid;
	EFI_GUID    gEfiLegacyBiosPlatformProtocolGuid;
	EFI_GUID    gEfiLegacyBiosPlatformDxeGuid;
	VOID       *Ignore;

{	gEfiLegacyBiosDxeGuid.Data1=      0xF122A15C;
	gEfiLegacyBiosDxeGuid.Data2=          0xC10B;
	gEfiLegacyBiosDxeGuid.Data3=          0x4D54;
	gEfiLegacyBiosDxeGuid.Data4[0]=         0x8F;
	gEfiLegacyBiosDxeGuid.Data4[1]=         0x48;
	gEfiLegacyBiosDxeGuid.Data4[2]=         0x60;
	gEfiLegacyBiosDxeGuid.Data4[3]=         0xF4;
	gEfiLegacyBiosDxeGuid.Data4[4]=         0xF0;
	gEfiLegacyBiosDxeGuid.Data4[5]=         0x6D;
	gEfiLegacyBiosDxeGuid.Data4[6]=         0xD1;
	gEfiLegacyBiosDxeGuid.Data4[7]=         0xAD;
	gEfiLegacyBiosProtocolGuid.Data1= 0xdb9a1e3d;
	gEfiLegacyBiosProtocolGuid.Data2=     0x45cb;
	gEfiLegacyBiosProtocolGuid.Data3=     0x4abb;
	gEfiLegacyBiosProtocolGuid.Data4[0]=    0x85;
	gEfiLegacyBiosProtocolGuid.Data4[1]=    0x3b;
	gEfiLegacyBiosProtocolGuid.Data4[2]=    0xe5;
	gEfiLegacyBiosProtocolGuid.Data4[3]=    0x38;
	gEfiLegacyBiosProtocolGuid.Data4[4]=    0x7f;
	gEfiLegacyBiosProtocolGuid.Data4[5]=    0xdb;
	gEfiLegacyBiosProtocolGuid.Data4[6]=    0x2e;
	gEfiLegacyBiosProtocolGuid.Data4[7]=    0x2d;
	gEfiLegacyBiosPlatformDxeGuid.Data1= 0xBC6D08DC;
	gEfiLegacyBiosPlatformDxeGuid.Data2=     0x865D;
	gEfiLegacyBiosPlatformDxeGuid.Data3=     0x4FFE;
	gEfiLegacyBiosPlatformDxeGuid.Data4[0]=    0x8B;
	gEfiLegacyBiosPlatformDxeGuid.Data4[1]=    0x7A;
	gEfiLegacyBiosPlatformDxeGuid.Data4[2]=    0xFB;
	gEfiLegacyBiosPlatformDxeGuid.Data4[3]=    0x5F;
	gEfiLegacyBiosPlatformDxeGuid.Data4[4]=    0xB0;
	gEfiLegacyBiosPlatformDxeGuid.Data4[5]=    0x4F;
	gEfiLegacyBiosPlatformDxeGuid.Data4[6]=    0x12;
	gEfiLegacyBiosPlatformDxeGuid.Data4[7]=    0xF1;
	gEfiLegacyBiosPlatformProtocolGuid.Data1= 0x783658a3;
	gEfiLegacyBiosPlatformProtocolGuid.Data2=     0x4172;
	gEfiLegacyBiosPlatformProtocolGuid.Data3=     0x4421;
	gEfiLegacyBiosPlatformProtocolGuid.Data4[0]=    0xa2;
	gEfiLegacyBiosPlatformProtocolGuid.Data4[1]=    0x99;
	gEfiLegacyBiosPlatformProtocolGuid.Data4[2]=    0xe0;
	gEfiLegacyBiosPlatformProtocolGuid.Data4[3]=    0x9;
	gEfiLegacyBiosPlatformProtocolGuid.Data4[4]=    0x7;
	gEfiLegacyBiosPlatformProtocolGuid.Data4[5]=    0x9c;
	gEfiLegacyBiosPlatformProtocolGuid.Data4[6]=    0xc;
	gEfiLegacyBiosPlatformProtocolGuid.Data4[7]=    0xb4;
	gEfiLegacyRegionDxeGuid.Data1= 0x208117F2;
	gEfiLegacyRegionDxeGuid.Data2=     0x25F8;
	gEfiLegacyRegionDxeGuid.Data3=     0x479D;
	gEfiLegacyRegionDxeGuid.Data4[0]=    0xB7;
	gEfiLegacyRegionDxeGuid.Data4[1]=    0x26;
	gEfiLegacyRegionDxeGuid.Data4[2]=    0x10;
	gEfiLegacyRegionDxeGuid.Data4[3]=    0xC1;
	gEfiLegacyRegionDxeGuid.Data4[4]=    0x0B;
	gEfiLegacyRegionDxeGuid.Data4[5]=    0xED;
	gEfiLegacyRegionDxeGuid.Data4[6]=    0x6D;
	gEfiLegacyRegionDxeGuid.Data4[7]=    0xC1;
	gEfiLegacyRegionDxe2Guid.Data1= 0x8C439043;
	gEfiLegacyRegionDxe2Guid.Data2=     0x85CA;
	gEfiLegacyRegionDxe2Guid.Data3=     0x467A;
	gEfiLegacyRegionDxe2Guid.Data4[0]=    0x96;
	gEfiLegacyRegionDxe2Guid.Data4[1]=    0xF1;
	gEfiLegacyRegionDxe2Guid.Data4[2]=    0xCB;
	gEfiLegacyRegionDxe2Guid.Data4[3]=    0x14;
	gEfiLegacyRegionDxe2Guid.Data4[4]=    0xF4;
	gEfiLegacyRegionDxe2Guid.Data4[5]=    0xD0;
	gEfiLegacyRegionDxe2Guid.Data4[6]=    0xDC;
	gEfiLegacyRegionDxe2Guid.Data4[7]=    0xDA;
	gEfiLegacyRegionProtocolGuid.Data1= 0x0fc9013a;
	gEfiLegacyRegionProtocolGuid.Data2=     0x0568;
	gEfiLegacyRegionProtocolGuid.Data3=     0x4ba9;
	gEfiLegacyRegionProtocolGuid.Data4[0]=    0x9b;
	gEfiLegacyRegionProtocolGuid.Data4[1]=    0x7e;
	gEfiLegacyRegionProtocolGuid.Data4[2]=    0xc9;
	gEfiLegacyRegionProtocolGuid.Data4[3]=    0xc3;
	gEfiLegacyRegionProtocolGuid.Data4[4]=    0x90;
	gEfiLegacyRegionProtocolGuid.Data4[5]=    0xa6;
	gEfiLegacyRegionProtocolGuid.Data4[6]=    0x60;
	gEfiLegacyRegionProtocolGuid.Data4[7]=    0x9b;

}
	
	Status = BS->LocateProtocol(&gEfiLegacyBiosPlatformProtocolGuid, NULL, &Ignore);
	
	if (EFI_ERROR(Status)){
		Print(L"Attempting to load Legacy Bios Platform DXE\n");
		Status = EfiExecuteFirmwareFile(&gEfiLegacyBiosPlatformDxeGuid);
		if(EFI_ERROR(Status)) { Print(L"Could not load the Legacy Bios Platform DXE: %r\n", Status); }//return Status;
	}
	
	Status = BS->LocateProtocol(&gEfiLegacyRegionProtocolGuid, NULL, &Ignore);
	if (EFI_ERROR(Status)){
		Print(L"Attempting to load Legacy Region DXE\n");
		Status = EfiExecuteFirmwareFile(&gEfiLegacyRegionDxeGuid);
		if(EFI_ERROR(Status)) { Print(L"Could not load the Legacy Region DXE: %r\n", Status); }//return Status;
	}

	Status = BS->LocateProtocol(&gEfiLegacyRegionProtocolGuid, NULL, &Ignore);
	if (EFI_ERROR(Status)){
		Print(L"Attempting to load Legacy Region Alt DXE\n");
		Status = EfiExecuteFirmwareFile(&gEfiLegacyRegionDxe2Guid);
		if(EFI_ERROR(Status)) { Print(L"Could not load the Legacy Region DXE: %r\n", Status); }//return Status;
	}
	
	Status = BS->LocateProtocol(&gEfiLegacyBiosProtocolGuid, NULL, &Ignore);
	
	if (EFI_ERROR(Status)){
		Print(L"Attempting to load Legacy Bios DXE\n");
		Status = EfiExecuteFirmwareFile(&gEfiLegacyBiosDxeGuid);
		if(EFI_ERROR(Status)) { Print(L"Could not load the Legacy Bios DXE: %r\n", Status); }//return Status;
	}
	
}


#define REAL_MEM_BASE 	((void *)(char *)0x10000)
#define REAL_MEM_SIZE   0x40000
#define REAL_MEM_BLOCKS 	0x100

struct mem_block {
	unsigned int size : 20;
	unsigned int free : 1;
};

static struct {
	int ready;
	int count;
	struct mem_block blocks[REAL_MEM_BLOCKS];
} mem_info = { 0 };

void * memmove(void *Destination, void *Source, UINTN Length)
{
	void *temp;

	Status=BS->AllocatePool(EfiLoaderData, Length, &temp);

	BS->CopyMem(temp, Source, Length);
	BS->CopyMem(Destination, temp, Length);

	Status=BS->FreePool(&temp);
	return Destination;
}

static void
insert_block(int i)
{
	memmove(
	 mem_info.blocks + i + 1,
	 mem_info.blocks + i,
	 (mem_info.count - i) * sizeof(struct mem_block));

	mem_info.count++;
}

void *
LRMI_alloc_real(int size)
{
	int i;
	char *r = (char *)REAL_MEM_BASE;

	if (!mem_info.ready)
		return NULL;

	if (mem_info.count == REAL_MEM_BLOCKS)
		return NULL;

	size = (size + 15) & ~15;

	for (i = 0; i < mem_info.count; i++) {
		if (mem_info.blocks[i].free && size < mem_info.blocks[i].size) {
			insert_block(i);///FindMe

			mem_info.blocks[i].size = size;
			mem_info.blocks[i].free = 0;
			mem_info.blocks[i + 1].size -= size;

			return (void *)r;
		}

		r += mem_info.blocks[i].size;
	}

	return NULL;
}

void x86Emu_init(VOID *real_memory){
	X86EMU_intrFuncs intFuncs[256];
	void *stack;
 	int i;

	X86EMU_pioFuncs pioFuncs = {
		(&IORead8),
		(&IORead16),
		(&IORead32),
		(&IOWrite8),
		(&IOWrite16),
		(&IOWrite32)};
	
	X86EMU_memFuncs memFuncs = {
		(&Read8),
		(&Read16),
		(&Read32),
		(&Write8),
		(&Write16),
		(&Write32)};
	
	X86EMU_setupMemFuncs(&memFuncs);
	X86EMU_setupPioFuncs(&pioFuncs);
	
	for (i=0; i<256; i++) intFuncs[i] = x86emu_do_int;
	
	X86EMU_setupIntrFuncs(intFuncs);
	
	X86_EFLAGS = X86_IF_MASK | X86_IOPL_MASK;
	
	//Status = BS->AllocatePool(EfiLoaderCode, 64 * 1024, &stack);
	stack = LRMI_alloc_real(64 * 1024);
	X86_SS = (UINT32)(UINTN) stack >> 4;
	X86_ESP = 0xFFFE;

	M.mem_base = (UINT32)(UINTN) real_memory;
	M.mem_size = 1024*1024;

}

void PostOptionRom(unsigned pci_device) {
	///Several machines seem to want the device that they're POSTing in
	///here
	
	X86_EAX = pci_device;

	///0xc000 is the video option ROM.  The init code for each
	///option ROM is at 0x0003 - so jump to c000:0003 and start running
	X86_CS  = 0xc000;
	X86_EIP = 0x0003;

	///This is all heavily cargo culted but seems to work
	X86_EDX = 0x80;
	X86_DS  = 0x0040;

	X86_EBX = 0;
	X86_ECX = 0;
	X86_ESI = 0;
	X86_EDI = 0;
	X86_EBP = 0;
	X86_ES  = 0;
	X86_FS  = 0;
	X86_GS  = 0;
	X86_SS  = 0;
	X86_ESP = 0;

	X86EMU_exec();
}



///
///This function loads the VGA Option ROM and
///runs it in an emulated x86 PC. It then copies
///the resulting information to the system in
///the real 1st megabyte of the system.
///This includes the interrupt handler, the 
///actual option rom and the option rom data.
///
EFI_STATUS EfiShadowVGAOptionROM (IN EFI_VIDEO_CARD *MyVideoCard) {
	
	UINTN                          i;
	EFI_PCI_IO_PROTOCOL           *IoDev;
	VOID                          *ROM_Addr;             //Pointer to the VGA ROM Shadow location
	UINTN                          PciSegment;
	UINTN                          PciBus;
	UINTN                          PciDevice;
	UINTN                          PciFunction;
	EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
	UINTN                          Index;
	UINT8                          TestValue;
	UINTN                          Flags = 0;
	VOID                          *RomShadowAddress = NULL;
	UINT32                         ShadowedRomSize = 0;

	ROM_Addr         = (VOID *)(UINTN)0xC0000;

	
	for (i=0; i< NumberOptionRoms; i++) {
			
		if ((MyOptionRoms[i].VendorId==MyVideoCard->VendorId)&(MyOptionRoms[i].DeviceId==MyVideoCard->DeviceId)) {

			///Debug:::
			Print(L"EfiShadowVGAOptionROM: OptionROM Ven: %04x Dev: %04x PCI Ven: %04x Dev: %04x\n", MyOptionRoms[i].VendorId, MyOptionRoms[i].DeviceId, MyVideoCard->VendorId, MyVideoCard->DeviceId);

			///Initialize local variables
			IoDev            = NULL;

			///Open the PCI IO Abstraction(s) needed
			Status = BS->OpenProtocol ( MyVideoCard->Handle, &gEfiPciIoProtocolGuid, (VOID **) &IoDev, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
			if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
				return Status;
			}

			///Tell the PCI Io Protocol that the Option ROM is available for Shadowing
			IoDev->RomSize=MyOptionRoms[i].OptionRomSize;
			IoDev->RomImage=MyOptionRoms[i].OptionRom;
			
			///We need the PCI device location for post-ing the OptionROM

			IoDev->GetLocation(IoDev, &PciSegment, &PciBus, &PciDevice, &PciFunction);
			
			///Always clean-up
			Status = BS->CloseProtocol (MyVideoCard->Handle, &gEfiPciIoProtocolGuid, ImageHandle, NULL);
			if (EFI_ERROR (Status)) { Print(L"Could not close the PCI IO Protocol\n", Status); return EFI_NOT_FOUND;}

			
			///Only needed for LegacyRegionDxe
			EfiCheckAndLoadLegacyProtocols();
			
			Print(L"Disconnecting the controller\n");
			
			Status = BS->LocateProtocol(&gEfiLegacyBiosProtocolGuid, NULL, (void **)&LegacyBios);

			Status = LegacyBios->InstallPciRom(LegacyBios, MyVideoCard->Handle, NULL, &Flags, NULL, NULL, &RomShadowAddress, &ShadowedRomSize);
			//Status= BS->DisconnectController(MyVideoCard->Handle, NULL, NULL);


			///Test the VGA Buffer			
			for (Index=0xA0000; Index<0xC0000; Index++){
				BS->CopyMem((UINT8 *)Index, &TestValue, 1);

			}
			
			//Status=BS->ConnectController(MyVideoCard->Handle, NULL, NULL, TRUE);

			return EFI_SUCCESS;


		} //if (OptionRom == Dev)
	
	}//for each OptionRom

	return EFI_NOT_FOUND;

	
}
/*EFI_STATUS EfiShadowVGAOptionROM (IN EFI_VIDEO_CARD *MyVideoCard) {
	
	UINTN                          i;
	EFI_PCI_IO_PROTOCOL           *IoDev;
	VOID                          *ROM_Addr;             //Pointer to the VGA ROM Shadow location
	UINTN                          PciSegment;
	UINTN                          PciBus;
	UINTN                          PciDevice;
	UINTN                          PciFunction;
//	UINT16                         Segment;
//	UINT16                         Offset;
//	EFI_IA32_REGISTER_SET          Regs;
//	EFI_IA32_REGISTER_SET          VgaRegs;
	UINTN                          Index;
	UINT8                          TestValue;

	ROM_Addr         = (VOID *)(UINTN)0xC0000;

	
	for (i=0; i< NumberOptionRoms; i++) {
			
		if ((MyOptionRoms[i].VendorId==MyVideoCard->VendorId)&(MyOptionRoms[i].DeviceId==MyVideoCard->DeviceId)) {

			///Debug:::
			Print(L"EfiShadowVGAOptionROM: OptionROM Ven: %04x Dev: %04x PCI Ven: %04x Dev: %04x\n", MyOptionRoms[i].VendorId, MyOptionRoms[i].DeviceId, MyVideoCard->VendorId, MyVideoCard->DeviceId);

			///Initialize local variables
			IoDev            = NULL;

			///Open the PCI IO Abstraction(s) needed
			Status = BS->OpenProtocol ( MyVideoCard->Handle, &gEfiPciIoProtocolGuid, (VOID **) &IoDev, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
			if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
				return Status;
			}

			///Tell the PCI Io Protocol that the Option ROM is available for Shadowing
			IoDev->RomSize=MyOptionRoms[i].OptionRomSize;
			IoDev->RomImage=MyOptionRoms[i].OptionRom;
			
			///We need the PCI device location for post-ing the OptionROM
			IoDev->GetLocation(IoDev, &PciSegment, &PciBus, &PciDevice, &PciFunction);
			
			///Always clean-up
			Status = BS->CloseProtocol (MyVideoCard->Handle, &gEfiPciIoProtocolGuid, ImageHandle, NULL);
			if (EFI_ERROR (Status)) { Print(L"Could not close the PCI IO Protocol\n", Status); return EFI_NOT_FOUND;}
			
			///Only needed for LegacyRegionDxe
			EfiCheckAndLoadLegacyProtocols();
			
			Print(L"Disconnecting the controller\n");
			
			Status= BS->DisconnectController(MyVideoCard->Handle, NULL, NULL);

//			Print(L"POST-ing the Option ROM\n");

			BS->AllocatePool(EfiLoaderCode, 0x100000, &RealModeMemory);
			BS->SetMem(RealModeMemory,(UINTN)0x100000, 0);
			BS->CopyMem((VOID *)((UINTN)RealModeMemory+(UINTN)0xC0000), MyOptionRoms[i].OptionRom, MyOptionRoms[i].OptionRomSize);
			
			x86Emu_init (RealModeMemory);

			PostOptionRom ( ((PciBus << 0x8) | (PciDevice << 0x3) | (PciFunction)) & 0x0000FFFF);

			///Copy the emulator memory to the system
			BS->CopyMem((VOID *)((UINTN)0x0), RealModeMemory, 0x100000);
			
			///We don't need the RealModeMemory buffer anymore
			BS->FreePool(RealModeMemory);

			///Test the VGA Buffer			
			for (Index=0xA0000; Index<0xC0000; Index++){
				BS->CopyMem((UINT8 *)Index, &TestValue, 1);
			}
			
			Status=BS->ConnectController(MyVideoCard->Handle, NULL, NULL, TRUE);

			return EFI_SUCCESS;

		} //if (OptionRom == Dev)
	
	}//for each OptionRom

	return EFI_NOT_FOUND;
	
}*/


///
///Required for vertically parsing the Option Rom Firmware files
///TODO: Optionally, try to also parse horizontally.
///
EFI_STATUS EfiParseSection(IN VOID *Stream, IN UINTN StreamSize, OUT VOID **OptionRom, OUT UINTN *OptionRomSize){
	
	EFI_COMMON_SECTION_HEADER                *SectionType;
	EFI_COMPRESSION_SECTION                  *CompressedStream;
	EFI_GUID_DEFINED_SECTION                 *GuidDefinedStream;
	EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL   *GuidedExtraction;
	EFI_TIANO_DECOMPRESS_PROTOCOL            *Decompress;
	VOID                                     *Subsection;
	VOID                                     *ScratchBuffer;
	VOID                                     *DecompressBuffer;
	UINT32                                    UncompressedLength;
	UINT32                                    ScratchSize;
	UINTN                                     SubsectionSize;
	UINT32                                    AuthenticationStatus;
	
	SectionType = Stream;
	
	switch (SectionType->Type) {
		case EFI_SECTION_COMPRESSION:
			
			CompressedStream=(EFI_COMPRESSION_SECTION *) Stream;
			
			Status=BS->LocateProtocol(&gEfiTianoDecompressProtocolGuid, NULL, (VOID **)&Decompress);
			
			Status=Decompress->GetInfo(Decompress, CompressedStream + 1, (UINT32)StreamSize, &UncompressedLength, &ScratchSize);
			
			if ((CompressedStream->UncompressedLength==UncompressedLength)&(CompressedStream->CompressionType==01)){
				
				BS->AllocatePool(EfiBootServicesData, ScratchSize, &ScratchBuffer);
				BS->AllocatePool(EfiBootServicesData,UncompressedLength, &DecompressBuffer);
				
				Status=Decompress->Decompress(Decompress, CompressedStream + 1, (UINT32)StreamSize, DecompressBuffer, UncompressedLength, ScratchBuffer, ScratchSize);
				if (EFI_ERROR(Status)) { Print (L"Decompress->Decompress Status: %r", Status); };
				
				SubsectionSize=UncompressedLength;
				Status=EfiParseSection(DecompressBuffer, SubsectionSize, OptionRom, OptionRomSize);
				
				BS->FreePool(ScratchBuffer);
				BS->FreePool(DecompressBuffer);
				
			} else {
				Print(L"Decompressor error: could not decompress: CS->UL: %d, UL: %d, CS-CType: %d\n", CompressedStream->UncompressedLength, UncompressedLength, CompressedStream->CompressionType);
				return EFI_INVALID_PARAMETER;
			}
			break;//EFI_SECTION_COMPRESSION
			
		case EFI_SECTION_GUID_DEFINED:
			
			GuidDefinedStream=(VOID *)(UINTN)SectionType;
			Status=BS->LocateProtocol(&GuidDefinedStream->SectionDefinitionGuid, NULL, (VOID **)&GuidedExtraction);
			if(!EFI_ERROR(Status)){

				Status=GuidedExtraction->ExtractSection(GuidedExtraction, GuidDefinedStream, &Subsection, &SubsectionSize, &AuthenticationStatus);
				if(EFI_ERROR(Status)){ Print(L"Error during GuidedExtraction: %r ", Status);};
				Status=EfiParseSection(Subsection, SubsectionSize, OptionRom, OptionRomSize);
				
			};
			break;//EFI_SECTION_GUID_DEFINED
			
		case EFI_SECTION_RAW:
			
			*OptionRomSize=StreamSize-sizeof(EFI_COMMON_SECTION_HEADER);
			
			BS->AllocatePool(EfiBootServicesData, *OptionRomSize, OptionRom);
			
			BS->CopyMem(*OptionRom, (VOID *)(UINTN)((UINTN)Stream + sizeof(EFI_COMMON_SECTION_HEADER)), *OptionRomSize);
			
			break;//EFI_SECTION_RAW
			
		default:
			return(EFI_INVALID_PARAMETER);
			break;
	}
	
	return(EFI_SUCCESS);
}


///
///Adds a loaded Option ROM to the Option ROM Vector.
///It requires the validation of the OptionROM signatures.
///If it's not a valid Option ROM, it frees the memory and
///returns EFI_UNSUPPORTED.
///
EFI_STATUS EfiAddOptionRomToVector(IN VOID *OptionRom, IN UINTN Size){

	PCI_OPTION_ROM      *PciOptionRom;
	PCI_DATA_STRUCTURE  *PciDataStructure;
	EFI_OPTION_ROM       MyOptionRom;
	EFI_OPTION_ROM      *MyNewOptionRoms;
	
	UINTN                Index;
	
	PciOptionRom=OptionRom;
	
	if(PciOptionRom->Signature==0xAA55) {
	
		PciDataStructure=(VOID *)(UINTN)((UINTN)PciOptionRom + (UINTN)PciOptionRom->PciDataStructure);
		
		if(PciDataStructure->Signature==0x52494350) {
			if(NumberOptionRoms==0){
				BS->AllocatePool(EfiBootServicesData, sizeof(EFI_OPTION_ROM), (void**) &MyOptionRoms);
				MyOptionRom=MyOptionRoms[NumberOptionRoms];
			}
			
			MyOptionRom.OptionRom=OptionRom;
			MyOptionRom.OptionRomSize=Size;
			MyOptionRom.VendorId=PciDataStructure->VendorId;
			MyOptionRom.DeviceId=PciDataStructure->DeviceId;
			MyOptionRom.VGA=FALSE;
			MyOptionRom.BIOSCode=FALSE;
			MyOptionRom.OpenFirmwareCode=FALSE;
			MyOptionRom.HPPACode=FALSE;
			MyOptionRom.EFICode=FALSE;
			
			if(PciDataStructure->ClassCode[2]==3){
				MyOptionRom.VGA=TRUE;
				Print(L"Found  VGA Class OptionROM VendorID: %4x DeviceID: %4x Arch: ", PciDataStructure->VendorId, PciDataStructure->DeviceId);
			} else {
				Print(L"Found Class: %x%x%x OptionRom VendorID: %4x DeviceID: %4x Arch: ", PciDataStructure->ClassCode[2], PciDataStructure->ClassCode[2], PciDataStructure->ClassCode[2], PciDataStructure->VendorId, PciDataStructure->DeviceId);
			}
			
			switch(PciDataStructure->CodeType){
				case 0:
					Print(L"Intel x86, PC-AT Compatible  ");
					MyOptionRom.BIOSCode=TRUE;
					break;
				case 1:
					Print(L"OpenFirmware for PCI48       ");
					MyOptionRom.OpenFirmwareCode=TRUE;
					break;
				case 2:
					Print(L"Hewlett-Packard PA RISC      ");
					MyOptionRom.HPPACode=TRUE;
					break;
				case 3:
					Print(L"Extensible Firmware Interface");
					MyOptionRom.EFICode=TRUE;
					break;
				default:
					Print(L"Unknown OptionRom Type       ");
					break;
			}
			
			Print(L"\n");
			
			MyNewOptionRoms=NULL;
			
			if(NumberOptionRoms>0){
				
				BS->AllocatePool(EfiBootServicesData, sizeof(EFI_OPTION_ROM)*(NumberOptionRoms+1), (void**) &MyNewOptionRoms);
				
				for(Index=0; Index<NumberOptionRoms; Index++){
					MyNewOptionRoms[Index].OptionRom=MyOptionRoms[Index].OptionRom;
					MyNewOptionRoms[Index].OptionRomSize=MyOptionRoms[Index].OptionRomSize;
					MyNewOptionRoms[Index].VendorId=MyOptionRoms[Index].VendorId;
					MyNewOptionRoms[Index].DeviceId=MyOptionRoms[Index].DeviceId;
					MyNewOptionRoms[Index].VGA=MyOptionRoms[Index].VGA;
					MyNewOptionRoms[Index].BIOSCode=MyOptionRoms[Index].BIOSCode;
					MyNewOptionRoms[Index].OpenFirmwareCode=MyOptionRoms[Index].OpenFirmwareCode;
					MyNewOptionRoms[Index].HPPACode=MyOptionRoms[Index].HPPACode;
					MyNewOptionRoms[Index].EFICode=MyOptionRoms[Index].EFICode;
				}
				
				MyNewOptionRoms[NumberOptionRoms].OptionRom=MyOptionRom.OptionRom;
				MyNewOptionRoms[NumberOptionRoms].OptionRomSize=MyOptionRom.OptionRomSize;
				MyNewOptionRoms[NumberOptionRoms].VendorId=MyOptionRom.VendorId;
				MyNewOptionRoms[NumberOptionRoms].DeviceId=MyOptionRom.DeviceId;
				MyNewOptionRoms[NumberOptionRoms].VGA=MyOptionRom.VGA;
				MyNewOptionRoms[NumberOptionRoms].BIOSCode=MyOptionRom.BIOSCode;
				MyNewOptionRoms[NumberOptionRoms].OpenFirmwareCode=MyOptionRom.OpenFirmwareCode;
				MyNewOptionRoms[NumberOptionRoms].HPPACode=MyOptionRom.HPPACode;
				MyNewOptionRoms[NumberOptionRoms].EFICode=MyOptionRom.EFICode;
			}
			
			BS->FreePool(MyOptionRoms);
			MyOptionRoms=NULL;
			MyOptionRoms=MyNewOptionRoms;
			
			NumberOptionRoms++;
			return EFI_SUCCESS;
		}//If we've located the data structure.
		
	}//If the inner part of the section is an option Rom.
	
	BS->FreePool(OptionRom);
	return EFI_UNSUPPORTED;	
}


///
///Currently explores the contents of all the firmware volumes.
///Reads the files and checks for sections. It tries to identify
///the sections and print them out with descriptive information.
///
EFI_STATUS EfiGetFirmwareOpRomInformation() {
	
	EFI_HANDLE                     *HandleBuffer;
	EFI_FIRMWARE_VOLUME_PROTOCOL   *FirmwareVolume;
	EFI_GUID                        gEfiFirmwareVolumeProtocolGuid;
	
	VOID                           *OptionRom;
	
	BOOLEAN                         FilesDone;
	
	EFI_TPL                         PreviousPriority;
	UINTN                           BufferSize;
	UINTN                           Index;
	UINTN                           FileCount;
	UINT8                           FileType;
	UINTN                           FileSize;
	UINT32                          FileAttributes;
	UINT32                          AuthenticationStatus;
	EFI_COMMON_SECTION_HEADER      *SectionType;
	EFI_GUID                        FileGuid;
	VOID                           *File;
	VOID                           *Key;
	UINTN                           OptionRomSize;
	
	
	//Got a better idea?
	// 0x389F751F, 0x1838, 0x4388, {0x83, 0x90, 0xCD, 0x81, 0x54, 0xBD, 0x27, 0xF8} just doesn't work
	gEfiFirmwareVolumeProtocolGuid.Data1=   0x389F751F;
	gEfiFirmwareVolumeProtocolGuid.Data2=   0x1838;
	gEfiFirmwareVolumeProtocolGuid.Data3=   0x4388;
	gEfiFirmwareVolumeProtocolGuid.Data4[0]=0x83;
	gEfiFirmwareVolumeProtocolGuid.Data4[1]=0x90;
	gEfiFirmwareVolumeProtocolGuid.Data4[2]=0xCD;
	gEfiFirmwareVolumeProtocolGuid.Data4[3]=0x81;
	gEfiFirmwareVolumeProtocolGuid.Data4[4]=0x54;
	gEfiFirmwareVolumeProtocolGuid.Data4[5]=0xBD;
	gEfiFirmwareVolumeProtocolGuid.Data4[6]=0x27;
	gEfiFirmwareVolumeProtocolGuid.Data4[7]=0xF8;
	
	//Get all the FirmwarVolumeProtocol instances
	Status = BS->LocateHandleBuffer (ByProtocol, &gEfiFirmwareVolumeProtocolGuid, NULL, &BufferSize, &HandleBuffer);
	if (EFI_ERROR (Status)) { return EFI_NOT_FOUND; }
	
//	Print(L"Found %d Firmware Volumes: %r\n", BufferSize, Status);
	
	//You don't work with the firmware unless you raise the task
	//priority level to NOTIFY or above. We'll restore
	//the TPL at the end of the execution.
	PreviousPriority=BS->RaiseTPL(TPL_NOTIFY);
	
	//Let's take the firmware volumes one by one.
	for (Index = 0; Index < BufferSize; Index++) {
		
		//And open them
		Status = BS->OpenProtocol ( HandleBuffer[Index], &gEfiFirmwareVolumeProtocolGuid, (VOID *) &FirmwareVolume, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
//		if (EFI_ERROR (Status)) {  Print(L"Could not open firmware Volume number %d: %r\n", Index, Status); continue; }
		
		//Allocate the search index buffer to the one
		//specified by the Firmware Volume
		BS->AllocatePool(EfiBootServicesData, FirmwareVolume->KeySize, &Key);
		if (Key != NULL) { BS->SetMem(Key, FirmwareVolume->KeySize, 0x0); }
		
		//Init variables
		FileCount = 0;
		FilesDone=FALSE;
		
		//Let's try to get all the files
		//included in this firmware volume
		while (!FilesDone) {
			
			//Set the search flag to 0x00
			//(EFI ALL FILES)
			FileType=0x02;
			
			//Get the next search result.
			Status = FirmwareVolume->GetNextFile(FirmwareVolume, Key, &FileType, &FileGuid, &FileAttributes, &FileSize);
			if (EFI_ERROR(Status)) { FilesDone=TRUE; continue; };
			
			//These filetypes have sections in them according to the spec. 0x06 should also be included.
			if ((FileType==0x02)|(FileType==0x04)|(FileType==0x05)|(FileType==0x06)|(FileType==0x07)|(FileType==0x08)|(FileType==0x09)) {
							
				BS->AllocatePool(EfiBootServicesData, FileSize, &File);
				
				//Read the file to a buffer (*File)
				Status = FirmwareVolume->ReadFile(FirmwareVolume, &FileGuid, &File, &FileSize, &FileType, &FileAttributes, &AuthenticationStatus);
//				if (EFI_ERROR(Status)) { Print(L"FirmwareVolume%d: File%03d: Error while reading file: %r\n", Index, FileCount, Status); };
								
				SectionType = File;
				
				if((SectionType->Type==EFI_SECTION_RAW)|(SectionType->Type==EFI_SECTION_COMPRESSION)|(SectionType->Type==EFI_SECTION_GUID_DEFINED)) {
				
					OptionRomSize=0;
					
					//Use EfiParseSectionStream for horizontal parsing. It's known to be broken.
					Status=EfiParseSection(SectionType, FileSize, &OptionRom, &OptionRomSize);
					
					//It's OK, since we're not de-allocating the sections.
					Status=EfiAddOptionRomToVector(OptionRom, OptionRomSize);
					
				} //If it's a known sectionType.
				
				BS->FreePool(File);
				
			}; //if filetype
			FileCount++;
			
		}//While (!FilesDone)
		
		Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiFirmwareVolumeProtocolGuid, ImageHandle, NULL);
		BS->FreePool(Key);
		
	};//For each firmware volume
	
	BS->FreePool(HandleBuffer);
	
	//Restore Task Priority Level since we won't touch the Firmware anymore.
	BS->RestoreTPL(PreviousPriority);
	
	return EFI_SUCCESS;
	
}


///
///Same as above, but with the Option ROMs already loaded by the VGA Cards
///
EFI_STATUS EfiGetPciOpRomInformation() {
	UINTN     i;
	
	for (i=0; i<NumberVideoCards; i++){
		if ((MyVideoCards[i].RomLocation != NULL)&(MyVideoCards[i].RomSize!=0)) {
			EfiAddOptionRomToVector(MyVideoCards[i].RomLocation, (UINTN) MyVideoCards[i].RomSize);
		}
	}
	return EFI_SUCCESS;
}


///
///Same as above, but with the Option ROMs found in EFI\\ShEFI
///on each EFI System Partition of the system.
///
EFI_STATUS EfiGetFileSystemOpRomInformation(){
	EFI_HANDLE                      *HandleBuffer;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;
	EFI_FILE_PROTOCOL               *FileSystem;
	EFI_FILE_PROTOCOL               *Directory;
	EFI_FILE_PROTOCOL               *File;
	EFI_FILE_INFO                   *DirEntryInfo;
	VOID                            *OptionRom;
	UINTN                            BufferSize;
	UINTN                            DirEntrySize;
	UINTN                            Index;
	UINTN                            FileSize;
	
	//Get all the FileSystems
	Status = BS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &BufferSize, &HandleBuffer);
	if (EFI_ERROR (Status)) { Print(L"EfiGetFileSystemOpRomInformation: Error getting any SimpleFileSystem Protocol instances Handles: %r\n", Status); return Status; };
	
	if(BufferSize==0) { return EFI_NOT_FOUND;};
	
	//for each filesystem
	for (Index=0; Index<BufferSize; Index++) {
		Status = BS->OpenProtocol (HandleBuffer[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID *) &Volume, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
		if (EFI_ERROR (Status)) { Print(L"EfiGetFileSystemOpRomInformation: Error getting the SimpleFileSystem Protocol instance in the count section: %r\n", Status); return Status; };
		
		Status=Volume->OpenVolume(Volume, &FileSystem);
		//Need to check if it's a directory. This is not currently done.
		Status=FileSystem->Open(FileSystem, &Directory, (VOID *)L"\\EFI\\ShEFI", EFI_FILE_MODE_READ, 0);
		if (EFI_ERROR(Status)) { Directory=NULL; continue;};
		
		if (Directory!=NULL){
		
			DirEntryInfo=NULL;
			DirEntrySize=0;
		
			Status=Directory->GetInfo(Directory, &gEfiFileInfoGuid, &DirEntrySize, DirEntryInfo);
			BS->AllocatePool(EfiBootServicesData,DirEntrySize, (void**) &DirEntryInfo);
			Status=Directory->GetInfo(Directory, &gEfiFileInfoGuid, &DirEntrySize, DirEntryInfo);
			
			if (DirEntryInfo->Attribute & EFI_FILE_DIRECTORY) {
				BS->FreePool(DirEntryInfo);
				DirEntrySize=0;
			
				while(TRUE){//For each entry
					Status=Directory->Read(Directory, &DirEntrySize, DirEntryInfo);
					
					if (DirEntrySize==0) { break; }//We're at the end of the directory;
					
					BS->AllocatePool(EfiBootServicesData, DirEntrySize, (void**) &DirEntryInfo);
					Status=Directory->Read(Directory, &DirEntrySize, DirEntryInfo);
					
					if((!(DirEntryInfo->Attribute & EFI_FILE_DIRECTORY))&(DirEntryInfo->FileSize<=0x100000)){//If the directory entry it isn't a directory and it's smaller than 1MByte
						
						Status=Directory->Open(Directory, &File, DirEntryInfo->FileName, EFI_FILE_MODE_READ, 0);
						
						BS->AllocatePool(EfiBootServicesData, (UINTN) DirEntryInfo->FileSize, &OptionRom);
						
						FileSize = (UINTN) DirEntryInfo->FileSize;
						
						Status=File->Read(File, &FileSize, OptionRom);
						
						Status=EfiAddOptionRomToVector(OptionRom, (UINTN) DirEntryInfo->FileSize);
						BS->FreePool(File);
					}//If the directory entry it isn't a directory
					
				}//For each entry
			
			} //If it's a directory
			BS->FreePool(Directory);
		} //If the \\EFI\\ShEfi File exists
		
		BS->FreePool(FileSystem);
		Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiSimpleFileSystemProtocolGuid, ImageHandle, NULL);//This also frees the Volume variable;
		
	}//for each filesystem
	
	//attempt to read all the files in EFI\\ShEFI
	
	BS->FreePool (HandleBuffer);
	return EFI_SUCCESS;
}


///
///Get all the option Roms
///
EFI_STATUS EfiGetOptionRoms(){
	///Initialize variables
	NumberOptionRoms=0;
	
	///
	///Get all the ROMs in the FirmwareVolume
	///TODO: Implement fallback to FirmwareVolume2Protocol
	///
	Status = EfiGetFirmwareOpRomInformation();
	if(EFI_ERROR(Status)) {Print(L"EfiGetFirmwareOpRomInformation Error: %r\n", Status); }
	
	///
	///Get all the ROMs in the PCI space
	///
	Status = EfiGetPciOpRomInformation();
	if(EFI_ERROR(Status)) {Print(L"EfiGetPciOpRomInformation Error: %r\n", Status); }
	
	///
	///Get all the ROMs in all the FileSystems in EFI\\ShMessenger
	///
	Status = EfiGetFileSystemOpRomInformation();
	if(EFI_ERROR(Status)) {Print(L"EfiGetFileSystemOpRomInformation Error: %r\n", Status); }
	Print(L"\n");
	
	return Status;
}

