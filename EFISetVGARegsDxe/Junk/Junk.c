	Status = EfiCorrectEdidBlock();
	
	///
	///Get the gMux information.
	///Eventually we need to add proper gMux setting support for switching between the cards.
	///
	Status = EfiGetGMuxInformation();
	if(EFI_ERROR(Status)) {Print(L"EfiGetGMuxInformation Error: %r\n", Status); }
	



///
///It reads the EDID detected by the GraphicsCard
///
EFI_STATUS EfiCorrectEdidBlock(){

	EFI_HANDLE                    *HandleBuffer;
	EFI_EDID_ACTIVE_PROTOCOL      *EdidActive;
	EFI_EDID_DISCOVERED_PROTOCOL  *EdidDiscovered;
	UINT8                         *EstablishedTimings1;
	UINT8                         *EstablishedTimings2;
	UINTN                          Index;
	UINTN                          BufferSize;
	
	BufferSize = 0;
	
	Status = BS->LocateHandleBuffer(ByProtocol, &gEfiEdidActiveProtocolGuid, NULL, &BufferSize, &HandleBuffer);
	if (EFI_ERROR (Status)) { Print(L"EfiCorrectEdidBlock: Error getting any Edid Active Protocol instances Handles: %r\n", Status); };
	
	///for each filesystem
	for (Index=0; Index<BufferSize; Index++) {
		Status = BS->OpenProtocol (HandleBuffer[Index], &gEfiEdidActiveProtocolGuid, (VOID *) &EdidActive, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
		if (EFI_ERROR (Status)) { Print(L"EfiPopulateVgaVectors: Error getting the EDID Active Protocol instance in the count section: %r\n", Status); return Status; };
		Print(L"Found EDID Active Protocol. Block Size: %d. Established Timings 1: %d\n", EdidActive->SizeOfEdid, *(UINT8 *)(EdidActive->Edid+35));
		EstablishedTimings1=EdidActive->Edid+35;
		EstablishedTimings2=EdidActive->Edid+36;
		*EstablishedTimings1=0x21;
		*EstablishedTimings2=0x08;
		Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiEdidActiveProtocolGuid, ImageHandle, NULL);
	}
	
	BufferSize = 0;
	
	Status = BS->LocateHandleBuffer(ByProtocol, &gEfiEdidDiscoveredProtocolGuid, NULL, &BufferSize, &HandleBuffer);
	if (EFI_ERROR (Status)) { Print(L"EfiCorrectEdidBlock: Error getting any Edid Active Protocol instances Handles: %r\n", Status); };
	
	///for each filesystem
	for (Index=0; Index<BufferSize; Index++) {
		Status = BS->OpenProtocol (HandleBuffer[Index], &gEfiEdidDiscoveredProtocolGuid, (VOID *) &EdidDiscovered, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
		if (EFI_ERROR (Status)) { Print(L"EfiPopulateVgaVectors: Error getting the EDID Active Protocol instance in the count section: %r\n", Status); return Status; };
		Print(L"Found EDID Active Protocol. Block Size: %d. Established Timings 1: %d\n", EdidDiscovered->SizeOfEdid, *(UINT8 *)(EdidDiscovered->Edid+35));
		EstablishedTimings1=EdidDiscovered->Edid+35;
		EstablishedTimings2=EdidDiscovered->Edid+36;
		*EstablishedTimings1=0x21;
		*EstablishedTimings2=0x08;
		Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiEdidDiscoveredProtocolGuid, ImageHandle, NULL);
	}

	return EFI_SUCCESS;
}


///
///Question: How the heck do we identify what the values are for each video card?
///It currently displays the information about the current settings of the video card.
///
EFI_STATUS EfiGetGMuxInformation(){
	EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev;
	UINT8                             MajorVersion;
	UINT8                             MinorVersion;
	UINT8                             ReleaseVersion;
	UINT8                             DisplayStatus;
	UINT8                             ExternalStatus;
	UINT8                             DDCStatus;
	UINT8                             PowerStatus;
	UINT8                             InterruptStatus;
	
	MajorVersion    = 0;
	MinorVersion    = 0;
	ReleaseVersion  = 0;
	DisplayStatus   = 0;
	ExternalStatus  = 0;
	DDCStatus       = 0;
	PowerStatus     = 0;
	InterruptStatus = 0;
	
	#define PORT_VER_MAJOR 0x704
	#define PORT_VER_MINOR 0x705
	#define PORT_VER_RELEASE 0x706

	#define PORT_SWITCH_DISPLAY 0x710
	#define PORT_SWITCH_DDC 0x728
	#define PORT_SWITCH_EXTERNAL 0x740
	#define PORT_DISCRETE_POWER 0x750
	#define PORT_INTERRUPT_ENABLE 0x714
	#define PORT_INTERRUPT_STATUS 0x716

	#define INTERRUPT_ENABLE 0xFF
	#define INTERRUPT_DISABLE 0x0
	#define INTERRUPT_STATUS_ACTIVE 0
	#define INTERRUPT_STATUS_DISPLAY 1
	#define INTERRUPT_STATUS_POWER 4
	#define INTERRUPT_STATUS_UNK 5
	
	Status = BS->LocateProtocol(&gEfiPciRootBridgeIoProtocolGuid, NULL, &IoDev);
	if(!EFI_ERROR(Status)){

		Status=IoDev->Io.Read(IoDev, EfiPciWidthUint8, PORT_VER_MAJOR,        1, &MajorVersion   );
		Status=IoDev->Io.Read(IoDev, EfiPciWidthUint8, PORT_VER_MINOR,        1, &MinorVersion   );
		Status=IoDev->Io.Read(IoDev, EfiPciWidthUint8, PORT_VER_RELEASE,      1, &ReleaseVersion );		
		Status=IoDev->Io.Read(IoDev, EfiPciWidthUint8, PORT_SWITCH_DISPLAY,   1, &DisplayStatus  );		
		Status=IoDev->Io.Read(IoDev, EfiPciWidthUint8, PORT_SWITCH_DDC,       1, &DDCStatus      );		
		Status=IoDev->Io.Read(IoDev, EfiPciWidthUint8, PORT_SWITCH_EXTERNAL,  1, &ExternalStatus );		
		Status=IoDev->Io.Read(IoDev, EfiPciWidthUint8, PORT_DISCRETE_POWER,   1, &PowerStatus    );		

		Print(L"Found gMux\ngMux Version:          %d.%d.%d\ngMux Display:          %x\ngMux ExternalDisplay:  %x\ngMux DDC:              %x\ngMux Power:            %x\ngMux Interrupt Status: ", MajorVersion, MinorVersion, ReleaseVersion, DisplayStatus, ExternalStatus, DDCStatus, PowerStatus);
		
		switch(InterruptStatus){
			case INTERRUPT_STATUS_ACTIVE:
				Print(L"INTERRUPT_STATUS_ACTIVE ");
				break;
			case INTERRUPT_STATUS_DISPLAY:
				Print(L"INTERRUPT_STATUS_DISPLAY");
				break;
			case INTERRUPT_STATUS_POWER:
				Print(L"INTERRUPT_STATUS_POWER  ");
				break;
			case INTERRUPT_STATUS_UNK:
				Print(L"INTERRUPT_STATUS_UNK    ");
				break;
			default:
				Print(L"Unknown Interrupt Status");
				break;
		}

		Print(L"\n\n");
		
		//Are we on discrete?
		//Attempt to move to discrete
		//Shutdown integrated
		//
	}
	return EFI_SUCCESS;
	
}


///
///Powers on the Discrete Video Card
///Does not call the EFI to connect all drivers
///
EFI_STATUS EfiStartDiscrete(){
	EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev;
	UINT8                             InterruptStatus;
	UINT8                             InterruptEnableValue;

	#define PORT_INTERRUPT_ENABLE 0x714
	#define PORT_INTERRUPT_STATUS 0x716

	#define INTERRUPT_ENABLE 0xFF
	#define INTERRUPT_DISABLE 0x0
	#define INTERRUPT_STATUS_ACTIVE 0
	#define INTERRUPT_STATUS_DISPLAY 1
	#define INTERRUPT_STATUS_POWER 4
	#define INTERRUPT_STATUS_UNK 5

	Status = BS->LocateProtocol(&gEfiPciRootBridgeIoProtocolGuid, NULL, &IoDev);
	if(!EFI_ERROR(Status)){
		Status=IoDev->Io.Read(IoDev, EfiPciWidthUint8, PORT_INTERRUPT_STATUS, 1, &InterruptStatus);
		if(InterruptStatus==INTERRUPT_STATUS_POWER) {
			
			InterruptEnableValue=INTERRUPT_DISABLE;
			Status=IoDev->Io.Write(IoDev, EfiPciWidthUint8, PORT_INTERRUPT_ENABLE, 1, &InterruptEnableValue);
			
			Status=IoDev->Io.Read(IoDev, EfiPciWidthUint8, PORT_INTERRUPT_STATUS, 1, &InterruptStatus);
			if(InterruptStatus==INTERRUPT_STATUS_POWER) {

				InterruptEnableValue=INTERRUPT_STATUS_POWER;
				Status=IoDev->Io.Write(IoDev, EfiPciWidthUint8, PORT_INTERRUPT_STATUS, 1, &InterruptEnableValue);
			}
			
			Status=IoDev->Io.Read(IoDev, EfiPciWidthUint8, PORT_INTERRUPT_STATUS, 1, &InterruptStatus);
			if(InterruptStatus==INTERRUPT_STATUS_ACTIVE) {

				InterruptEnableValue=INTERRUPT_ENABLE;
				Status=IoDev->Io.Write(IoDev, EfiPciWidthUint8, PORT_INTERRUPT_ENABLE, 1, &InterruptEnableValue);
			};
		};
	};
	return EFI_SUCCESS;
};


///
///Shuts down the Discrete Video Card
///Does not call the EFI to disconnect all drivers
///
EFI_STATUS EfiShutdownDiscrete(){

	EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev;
	UINT8                             InterruptStatus;
	UINT8                             InterruptEnableValue;

	#define PORT_INTERRUPT_ENABLE 0x714
	#define PORT_INTERRUPT_STATUS 0x716

	#define INTERRUPT_ENABLE 0xFF
	#define INTERRUPT_DISABLE 0x0
	#define INTERRUPT_STATUS_ACTIVE 0
	#define INTERRUPT_STATUS_DISPLAY 1
	#define INTERRUPT_STATUS_POWER 4
	#define INTERRUPT_STATUS_UNK 5

	Status = BS->LocateProtocol(&gEfiPciRootBridgeIoProtocolGuid, NULL, &IoDev);
	
	if(!EFI_ERROR(Status)){

		Status=IoDev->Io.Read(IoDev, EfiPciWidthUint8, PORT_INTERRUPT_STATUS, 1, &InterruptStatus);
		if(InterruptStatus==INTERRUPT_STATUS_POWER) {
			
			InterruptEnableValue=INTERRUPT_DISABLE;
			Status=IoDev->Io.Write(IoDev, EfiPciWidthUint8, PORT_INTERRUPT_ENABLE, 1, &InterruptEnableValue);
			
			Status=IoDev->Io.Read(IoDev, EfiPciWidthUint8, PORT_INTERRUPT_STATUS, 1, &InterruptStatus);
			if(InterruptStatus==INTERRUPT_STATUS_POWER) {

				InterruptEnableValue=INTERRUPT_STATUS_POWER;
				Status=IoDev->Io.Write(IoDev, EfiPciWidthUint8, PORT_INTERRUPT_STATUS, 1, &InterruptEnableValue);
			}
			
			Status=IoDev->Io.Read(IoDev, EfiPciWidthUint8, PORT_INTERRUPT_STATUS, 1, &InterruptStatus);
			if(InterruptStatus==INTERRUPT_STATUS_ACTIVE) {

				InterruptEnableValue=INTERRUPT_ENABLE;
				Status=IoDev->Io.Write(IoDev, EfiPciWidthUint8, PORT_INTERRUPT_ENABLE, 1, &InterruptEnableValue);
			};
		};
	};
	
	return EFI_SUCCESS;
};


///
///Prints out information about the console device
///
EFI_STATUS EfiPrintConsoleInformation(){
	EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
	CHAR16                                *DevicePathString;
	UINTN                                  PointerSize;
	
	DevicePath=NULL;
	PointerSize=0;
	Status = RS->GetVariable (L"ConOut", &gEfiGlobalVariableGuid, NULL, &PointerSize, DevicePath);
	DevicePath=AllocatePool(PointerSize);
	Status = RS->GetVariable (L"ConOut", &gEfiGlobalVariableGuid, NULL, &PointerSize, DevicePath);
	DevicePathString=ConvertDevicePathToText(DevicePath, FALSE, FALSE);
	FreePool(DevicePath);
	Print(L"NVRAM    Console Out : DevicePath: %s\n", DevicePathString);
	FreePool(DevicePathString);

	DevicePath=NULL;
	PointerSize=0;
	Status = RS->GetVariable (L"ConOutDev", &gEfiGlobalVariableGuid, NULL, &PointerSize, DevicePath);
	DevicePath=AllocatePool(PointerSize);
	Status = RS->GetVariable (L"ConOutDev", &gEfiGlobalVariableGuid, NULL, &PointerSize, DevicePath);
	DevicePathString=ConvertDevicePathToText(DevicePath, FALSE, FALSE);
	FreePool(DevicePath);
	Print(L"NVRAM    ConOutDev   : DevicePath: %s\n", DevicePathString);
	FreePool(DevicePathString);
	
	Status = BS->HandleProtocol(ST->ConsoleOutHandle, &gEfiDevicePathProtocolGuid, &DevicePath);
	DevicePathString=ConvertDevicePathToText(DevicePath, FALSE, FALSE);
	Print(L"SysTable Console Out : DevicePath: %s\n", DevicePathString);
	FreePool(DevicePathString);
	
	Status = BS->HandleProtocol(ST->ConsoleInHandle, &gEfiDevicePathProtocolGuid, &DevicePath);
	DevicePathString=ConvertDevicePathToText(DevicePath, FALSE, FALSE);
	Print(L"SysTable Console In  : DevicePath: %s\n", DevicePathString);
	FreePool(DevicePathString);
	
	Status = BS->HandleProtocol(ST->StandardErrorHandle, &gEfiDevicePathProtocolGuid, &DevicePath);
	DevicePathString=ConvertDevicePathToText(DevicePath, FALSE, FALSE);
	Print(L"SysTable Standard Err: DevicePath: %s\n", DevicePathString);
	FreePool(DevicePathString);
	
	Print(L"\n");
	
	DevicePath=NULL;
	return EFI_SUCCESS;
}


EFI_STATUS EfiShadowVGAOptionROM (IN EFI_VIDEO_CARD *MyVideoCard) {
	
	VOID                       *ROM_Addr;             //Pointer to the VGA ROM Shadow location
	UINT32                      Int10h_Addr;          //Pointer to the int10h Software Interrupt Handler pointer
	UINT32                      Int10h_Value;         //The Value of the int10h Software Interrupt Handler Vector
	EFI_LEGACY_REGION_PROTOCOL *LegacyRegionProtocol;
	UINT32                      Granularity;
	UINT32                      i;
	UINT32                      CRC32Value;
	
	ROM_Addr         = (VOID *)(UINTN)0xC0000;
	Int10h_Addr      = 0x40;
//	Int10h_Value     = 0xC00017C9;//MBP6,2
	Int10h_Value     = 0xC0001BF2;//MBP5,3 c000f21b?
	
	for (i=0; i< NumberOptionRoms; i++) {
	
		Print(L"EfiShadowVGAOptionROM: OptionROM Ven: %04x Dev: %04x PCI Ven: %04x Dev: %04x\n", MyOptionRoms[i].VendorId, MyOptionRoms[i].DeviceId, MyVideoCard->VendorId, MyVideoCard->DeviceId);
		
		if ((MyOptionRoms[i].VendorId==MyVideoCard->VendorId)&(MyOptionRoms[i].DeviceId==MyVideoCard->DeviceId)) {
	
			//
			//Unlock the memory for the shadow space
			//
			Status=BS->LocateProtocol(&gEfiLegacyRegionProtocolGuid, NULL, &LegacyRegionProtocol);
			if(EFI_ERROR(Status)) { LegacyRegionProtocol=NULL;}
			
			if (LegacyRegionProtocol != NULL) {
				Status=LegacyRegionProtocol->UnLock(LegacyRegionProtocol, 0xC0000, (UINT32) MyOptionRoms[i].OptionRomSize, &Granularity);
				if (EFI_ERROR (Status)) { Print(L"EfiShadowVGAOptionROM: Could not lock the LegacyRegion", Status); };
			}
			
			//
			//Copy the option ROM to the shadow space
			//
			BS->CopyMem(ROM_Addr, MyOptionRoms[i].OptionRom, MyOptionRoms[i].OptionRomSize);
			
			//
			//Lock back the shadow space memory
			//
			if (LegacyRegionProtocol != NULL) {
				Status=LegacyRegionProtocol->Lock(LegacyRegionProtocol, 0xC0000, (UINT32) MyOptionRoms[i].OptionRomSize, &Granularity);
				if (EFI_ERROR (Status)) { Print(L"EfiShadowVGAOptionROM: Could not lock the LegacyRegion", Status); };
			}

			Status=BS->CalculateCrc32 (MyOptionRoms[i].OptionRom, MyOptionRoms[i].OptionRomSize, &CRC32Value);
			
			if (MyOptionRoms[i].VendorId==0x1002) {
				while (true) {
					
				}
			}
			switch(CRC32Value){
				0x12345678:
					Int10h_Value=0xC0001234;
					break;
				default:
					Print(L"Unknown OptionRom CRC32. No int10h values.\n", CRC32Value);
					break;
			};
			
			
			//
			//Set the Int10h Pointer
			//
			*(UINT32 *)(UINTN)Int10h_Addr=*(UINT32 *)&Int10h_Value;
			
			return EFI_SUCCESS;
		}
	}
	return EFI_NOT_FOUND;
	
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
		if(EFI_ERROR(Status)){ Print (L"ReadSection Error: %r\n", Status);};
		BS->AllocatePool(EfiBootServicesData, ImageFileSize, &ImageFile);
		Status=FirmwareVolume->ReadSection(FirmwareVolume, gEfiGuid, EFI_SECTION_PE32, 0, &ImageFile, &ImageFileSize, &AuthenticationStatus);
		if(EFI_ERROR(Status)){ Print (L"ReadSection2 Error: %r\n", Status);};
		
		if (EFI_ERROR(Status)){
			BS->FreePool(ImageFile);
			ImageFile=NULL;
			Print(L"Failed extracting %g\n", *gEfiGuid);
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
///Simply shadows an optionRom and sets the correct int10h value.
///Only to be called by EfiWriteActiveGraphicsAdaptorPCIRegs
///TODO: Get the int10h values and ROMs from the vector that
///      EfiGetOpRomInformation builds.
///
EFI_STATUS EfiShadowVGAOptionROM (IN EFI_VIDEO_CARD *MyVideoCard) {
	
	UINTN                          i;
	EFI_PCI_IO_PROTOCOL           *IoDev;
	EFI_GUID                       VideoBiosGuid;

	
	for (i=0; i< NumberOptionRoms; i++) {
			
		if ((MyOptionRoms[i].VendorId==MyVideoCard->VendorId)&(MyOptionRoms[i].DeviceId==MyVideoCard->DeviceId)) {

			Print(L"EfiShadowVGAOptionROM: OptionROM Ven: %04x Dev: %04x PCI Ven: %04x Dev: %04x\n", MyOptionRoms[i].VendorId, MyOptionRoms[i].DeviceId, MyVideoCard->VendorId, MyVideoCard->DeviceId);

			///Initialize local variables
			IoDev            = NULL;

			///Open the PCI IO Abstraction(s) needed
			Status = BS->OpenProtocol ( MyVideoCard->Handle, &gEfiPciIoProtocolGuid, &IoDev, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
			if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
				return Status;
			}

			///Tell the PCI Io Protocol that the Option ROM is available for Shadowing
			IoDev->RomSize=MyOptionRoms[i].OptionRomSize;
			IoDev->RomImage=MyOptionRoms[i].OptionRom;
			
			//b8 7a 3e cf d0 63
			//
			///Load the VideoBios DXE of the CSM
			///It will do everything for us.
			VideoBiosGuid.Data1=0x29CF55F8;
			VideoBiosGuid.Data2=    0xB675;
			VideoBiosGuid.Data3=    0x4F5D;
			VideoBiosGuid.Data4[0]=   0x8F;
			VideoBiosGuid.Data4[1]=   0x2F;
			VideoBiosGuid.Data4[2]=   0xB8;
			VideoBiosGuid.Data4[3]=   0x7A;
			VideoBiosGuid.Data4[4]=   0x3E;
			VideoBiosGuid.Data4[5]=   0xCF;
			VideoBiosGuid.Data4[6]=   0xD0;
			VideoBiosGuid.Data4[7]=   0x63;
			EfiExecuteFirmwareFile(&VideoBiosGuid);
			
			///Always clean-up
			Status = BS->CloseProtocol (MyVideoCard->Handle, &gEfiPciIoProtocolGuid, ImageHandle, NULL);
			if (EFI_ERROR (Status)) { Print(L"Could not close the PCI IO Protocol\n", Status); return EFI_NOT_FOUND;}

			
			return EFI_SUCCESS;

		} //if (OptionRom == Dev)
	
	}//for each OptionRom
	return EFI_NOT_FOUND;
	
}
/*
0000000000: 02 F6 3F 58 03 36 CB 40 │ 49 94 7E B9 B3 9F 4A FA  ☻ö?X♥6E@I"~13YJú
0000000010: F7 02 72 C1 9F EF B2 A1 │ 93 46 B3 27 6D 32 FC 41  ÷☻rAYï²¡"F3'm2üA
0000000020: 60 42 02 74 69 D9 0F AA │ 23 DC 4C B9 CB 98 D1 77  `B☻tiU☼ª#ÜL1E~Ñw
0000000030: 50 32 2A 02 D7 72 7E 58 │ 50 CC 79 4F 82 09 CA 29  P2*☻xr~XPIyO,○E)
0000000040: 1F C1 A1 0F 03 03 03 08 │                          ▼A¡☼♥♥♥◘

PUSH(F63F580336CB4049947eb9b39f4afaf7) - gEfiSmBiosProtocolGuid
PUSH(72c19fefb2a19346b3276d32fc416042) - gEfiHiiDatabaseProtocolGuid
PUSH(7469d90faa23dc4cb9cb98d17750322a) - gEfiHiiStringProtocolGuid
PUSH(d7727e5850cc794f8209ca291fc1a10f) - gEfiHiiConfigRoutingProtocolGuid
AND
AND
AND
END

gEfiSmBiosProtocolGuid AND gEfiHiiDatabaseProtocolGuid
*/