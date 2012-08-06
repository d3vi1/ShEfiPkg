#include <EFISetVGARegsDxe.h>

#define DEBUG_OUTPUT

EFI_VIDEO_CARD          *MyVideoCards;
UINTN                    NumberVideoCards;

EFI_GOP_IMPLEMENTATION  *MyGOPImplementations;
UINTN                    NumberGOPImplementations;

EFI_HANDLE               ImageHandle;
EFI_BOOT_SERVICES       *BS;
EFI_RUNTIME_SERVICES    *RS;
EFI_SYSTEM_TABLE        *ST;
EFI_STATUS               Status;


///
///Currently a hack that works on Query the NVRAM for
///predefined value that points to the Bootcamp Video Card.
///TODO: Get the correct video card from NVRAM.
///TODO: Set the gMux to the video card in the NVRAM.
///
EFI_STATUS EfiWriteActiveGraphicsAdaptorPCIRegs() {
	
	EFI_PCI_IO_PROTOCOL                *IoDev;
	UINTN                               SegmentNumber;
	UINTN                               BusNumber;
	UINTN                               DevNumber;
	UINTN                               FuncNumber;
	UINTN                               i;
	
	SegmentNumber = 0;
	BusNumber     = 0;
	DevNumber     = 0;
	FuncNumber    = 0;
	
	//First disable the VGA Regs on the other card
	for (i=0; i<NumberVideoCards; i++) {
	
		Status = BS->OpenProtocol (MyVideoCards[i].Handle, &gEfiPciIoProtocolGuid, (VOID *) &IoDev, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
		if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs: Error getting the PciIo Protocol instance: %r\n", Status); return Status; };
		
		Status = IoDev->GetLocation(IoDev, &SegmentNumber, &BusNumber, &DevNumber, &FuncNumber);
		if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs: Could not locate the PCI Device: %r\n", Status); return Status; };
		
		if (MyVideoCards[i].IsMainVideoCard!=TRUE) {
			
			Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs:   PCI VGA Adapter on %02x:%02x.%02x: Disabling the VGA Regs.\n",  BusNumber, DevNumber, FuncNumber, Status); 
			
			Status=IoDev->Attributes (IoDev, EfiPciIoAttributeOperationDisable, EFI_PCI_DEVICE_ENABLE | EFI_VGA_DEVICE_ENABLE, NULL);
			if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs:   PCI VGA Adapter on %02x:%02x.%02x: Couldn't disable Attributes: %r\n", BusNumber, DevNumber, FuncNumber, Status); };
			
		};
		
		Status = BS->CloseProtocol (MyVideoCards[i].Handle, &gEfiPciIoProtocolGuid, ImageHandle, NULL);
		if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs:   PCI VGA Adapter on %02x:%02x.%02x: Error closing the PciIo Protocol instance: %r\n",  BusNumber, DevNumber, FuncNumber, Status); return Status; };
		
	}
	
	//Then enable the VGA Regs for the correct card
	for (i=0; i<NumberVideoCards; i++) {
		
		Status = BS->OpenProtocol (MyVideoCards[i].Handle, &gEfiPciIoProtocolGuid, (VOID *) &IoDev, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
		if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs: Error getting the PciIo Protocol instance: %r\n", Status); return Status; };
		
		Status = IoDev->GetLocation(IoDev, &SegmentNumber, &BusNumber, &DevNumber, &FuncNumber);
		if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs: Could not locate the PCI Device: %r\n", Status); return Status; };
		
		if (MyVideoCards[i].IsMainVideoCard==TRUE) {
			
			Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs:   PCI VGA Adapter on %02x:%02x.%02x: Enabling the VGA Regs.\n",  BusNumber, DevNumber, FuncNumber, Status); 
			
			Status=IoDev->Attributes (IoDev, EfiPciIoAttributeOperationEnable,  EFI_PCI_DEVICE_ENABLE | EFI_VGA_DEVICE_ENABLE, NULL);
			if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs:   PCI VGA Adapter on %02x:%02x.%02x: Couldn't set Attributes: %r\n", BusNumber, DevNumber, FuncNumber, Status); };
			
			Status = BS->CloseProtocol (MyVideoCards[i].Handle, &gEfiPciIoProtocolGuid, ImageHandle, NULL);
			if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs:   PCI VGA Adapter on %02x:%02x.%02x: Error closing the PciIo Protocol instance: %r\n",  BusNumber, DevNumber, FuncNumber, Status); return Status; };
			
			Status=EfiShadowVGAOptionROM(&MyVideoCards[i]);
			
			return EFI_SUCCESS;
		} else {
			
			Status = BS->CloseProtocol (MyVideoCards[i].Handle, &gEfiPciIoProtocolGuid, ImageHandle, NULL);
			if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs:   PCI VGA Adapter on %02x:%02x.%02x: Error closing the PciIo Protocol instance: %r\n",  BusNumber, DevNumber, FuncNumber, Status); return Status; };
			
		};
		
	}
	
	return EFI_NOT_FOUND;
	
}


///
///Make a list of all the video cards.
///and all their information. Mostly done!
///
EFI_STATUS EfiPopulateVgaVectors() {
	
	EFI_HANDLE                         *HandleBuffer;
	EFI_HANDLE                         *DevicePathHandle;
	EFI_PCI_IO_PROTOCOL                *IoDev;
	EFI_DEVICE_PATH_PROTOCOL           *DevicePath;
	EFI_HANDLE                         *LegacyVgaHandle;
	EFI_PCI_DEVICE_HEADER               PciHeader;
	
	UINTN                               LegacyVgaHandleValue;
	UINTN                               BufferSize;
	UINTN                               Index;
	UINTN                               PointerSize;
	UINT64                             *IoDevAttributes;
	
	UINTN                               Segment;
	UINTN                               Bus;
	UINTN                               Device;
	UINTN                               Function;
	
	PointerSize=sizeof(UINTN);
	LegacyVgaHandleValue= 0;
	Segment             = 0;
	Bus                 = 0;
	Device              = 0;
	Function            = 0;
	NumberVideoCards    = 0;
	
	//Get all the PCI Devices
	Status = BS->LocateHandleBuffer(ByProtocol, &gEfiPciIoProtocolGuid, NULL, &BufferSize, &HandleBuffer);
	if (EFI_ERROR (Status)) { Print(L"EfiPopulateVgaVectors: Error getting any PciIo Protocol instances Handles: %r\n", Status); return Status; };
	
	//Count the number of VGA Cards
	for(Index = 0; Index < BufferSize; Index++) {
		
		//This should always work.
		Status = BS->OpenProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID *) &IoDev, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
		if (EFI_ERROR (Status)) { Print(L"EfiPopulateVgaVectors: Error getting the PciIo Protocol instance in the count section: %r\n", Status); return Status; };
		
		//Let's get the PCI Header
		IoDev->Pci.Read (IoDev, EfiPciWidthUint32, 0, sizeof (PciHeader) / sizeof (UINT32), &PciHeader);
		
		//Is it a video card?
		if (PciHeader.ClassCode[2]==3) { NumberVideoCards++; }
		
		//Clean up after ourselves
		Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, ImageHandle, NULL);
		
	}
	
	//Allocate the buffer of VGA Cards
	BS->AllocatePool(EfiBootServicesData, sizeof (EFI_VIDEO_CARD) * NumberVideoCards, (void**) &MyVideoCards);
	
	//We start from 0 again since we're going through all the devices one more time
	NumberVideoCards = 0;
	
	//Go through all the PCI Devices and attempt to find video cards.
	//Populate the Video Cards Vector.
	for(Index = 0; Index < BufferSize; Index++) {
		
		//This should always work.
		Status = BS->OpenProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID *) &IoDev, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
		if (EFI_ERROR (Status)) { Print(L"EfiPopulateVgaVectors: Error getting the PciIo Protocol instance in the populate section: %r\n", Status); return Status; };
		
		//Let's get the PCI Header
		IoDev->Pci.Read (IoDev, EfiPciWidthUint32, 0, sizeof (PciHeader) / sizeof (UINT32), &PciHeader);
		IoDev->GetLocation (IoDev, &Segment, &Bus, &Device, &Function);
		
		//Is it a video card?
		if (PciHeader.ClassCode[2]==3) {
			
			//Print(L"WritePciReg:   PCI VGA Adapter on %02x:%02x.%02x: VendorID: %04x, DeviceID: %04x, Class: %02x.%02x.%02x\n", Bus, Device, Function, PciHeader.VendorId, PciHeader.DeviceId, PciHeader.ClassCode[2], PciHeader.ClassCode[1], PciHeader.ClassCode[0]);
			
			//Populate most of the MyVideoCards
			MyVideoCards[NumberVideoCards].VendorId          = PciHeader.VendorId;
			MyVideoCards[NumberVideoCards].DeviceId          = PciHeader.DeviceId;
			MyVideoCards[NumberVideoCards].RevisionId        = PciHeader.RevisionId;
			MyVideoCards[NumberVideoCards].ClassCode[0]      = PciHeader.ClassCode[0];
			MyVideoCards[NumberVideoCards].ClassCode[1]      = PciHeader.ClassCode[1];
			MyVideoCards[NumberVideoCards].ClassCode[2]      = PciHeader.ClassCode[2];
			MyVideoCards[NumberVideoCards].Bus               = Bus;
			MyVideoCards[NumberVideoCards].Device            = Device;
			MyVideoCards[NumberVideoCards].Function          = Function;
			MyVideoCards[NumberVideoCards].IsMainVideoCard   = FALSE;
			MyVideoCards[NumberVideoCards].IsActiveVideoCard = FALSE;
			MyVideoCards[NumberVideoCards].AttributeVgaIo    = FALSE;
			MyVideoCards[NumberVideoCards].AttributeVgaMemory= FALSE;
			MyVideoCards[NumberVideoCards].AttributeIo       = FALSE;
			MyVideoCards[NumberVideoCards].AttributeBusMaster= FALSE;
			MyVideoCards[NumberVideoCards].RomSize           = IoDev->RomSize;
			MyVideoCards[NumberVideoCards].RomLocation       = IoDev->RomImage;
			MyVideoCards[NumberVideoCards].Handle            = HandleBuffer[Index];
			
			Status=IoDev->Attributes (IoDev, EfiPciIoAttributeOperationGet, 0, (VOID *) &IoDevAttributes);
			if (EFI_ERROR (Status)) { Print(L"EfiPopulateVgaVectors:   PCI VGA Adapter on %02x:%02x.%02x: Couldn't get PCI Attributes: %r\n", Bus, Device, Function, Status); };
			
			if ((*IoDevAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_IO)     > 0) { MyVideoCards[NumberVideoCards].AttributeVgaIo     = TRUE; }
			if ((*IoDevAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY) > 0) { MyVideoCards[NumberVideoCards].AttributeVgaMemory = TRUE; }
			if ((*IoDevAttributes & EFI_PCI_IO_ATTRIBUTE_IO)         > 0) { MyVideoCards[NumberVideoCards].AttributeIo        = TRUE; }
			if ((*IoDevAttributes & EFI_PCI_IO_ATTRIBUTE_BUS_MASTER) > 0) { MyVideoCards[NumberVideoCards].AttributeBusMaster = TRUE; }
			
			//Get the LEGACYVGAHANDLE variable
			Status = RS->GetVariable (L"LEGACYVGAHANDLE", &gEfiGlobalVariableGuid, NULL, &PointerSize, (VOID *) &LegacyVgaHandleValue);
			if (EFI_ERROR (Status)) { Print(L"Could not read the LEGACYVGAHANDLE NVRAM variable: %r\n", Status); return Status; };
			LegacyVgaHandle=(VOID *)LegacyVgaHandleValue;
			
			//See if it's the one referenced in LegacyVgaHandle
			if (HandleBuffer[Index]==LegacyVgaHandle) {	MyVideoCards[NumberVideoCards].IsMainVideoCard = TRUE;}
			
			//Let's see if it has a GraphicsOutputProtocol on it
			Status = BS->OpenProtocol (HandleBuffer[Index], &gEfiGraphicsOutputProtocolGuid, NULL, ImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
			
			if (Status == EFI_UNSUPPORTED) {
				
				//Get it's device path
				Status = BS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID *) &DevicePath);
				if (EFI_ERROR (Status)) { Print(L"EfiPopulateVgaVectors: Couldn't open DevicePath: %r\n", Status); return Status;};
				
				DevicePathHandle=NULL;
				//See if any of the children handles (based on the device path) have the GraphicsOutput Protocol
				Status = BS->LocateDevicePath(&gEfiGraphicsOutputProtocolGuid, &DevicePath, DevicePathHandle);
				
				if (EFI_ERROR (Status)) {
					//Print(L"EfiPopulateVgaVectors: PCI VGA Adapter on %02x:%02x.%02x: LocateDevicePath Error: %r\n", Bus, Device, Function, Status);
					MyVideoCards[NumberVideoCards].IsActiveVideoCard = FALSE; //Pointless, but helps with the reading of the code
				}; 
				
				Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, ImageHandle, NULL);
				
			} else {
				
				MyVideoCards[NumberVideoCards].IsActiveVideoCard = TRUE;
				
			}; //if Status(BS->OpenProtocol)
			
			NumberVideoCards++;
			
		}; //If (PciHeader.ClassCode)
		
		Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, ImageHandle, NULL);
	}
	
	BS->FreePool(HandleBuffer);
	
	return EFI_SUCCESS;
}


///
///For debugging only!!
///
EFI_STATUS EfiPrintVgaVector () {
	
	UINTN Index;
	EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
	
	if (NumberVideoCards==0) { Print (L"No Video Cards found!!!\n"); return EFI_UNSUPPORTED; };
	
	Print(L"\nFound %d VGA Cards\n\n", NumberVideoCards);
	
	for (Index = 0; Index < NumberVideoCards; Index++) {
		
		Print(L"VGA%d %02x:%02x.%02x: VendorId:                    %04x\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function, MyVideoCards[Index].VendorId);
		Print(L"VGA%d %02x:%02x.%02x: DeviceId:                    %04x\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function, MyVideoCards[Index].DeviceId);
		Print(L"VGA%d %02x:%02x.%02x: Revision:                    %04x\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function, MyVideoCards[Index].RevisionId);
		Print(L"VGA%d %02x:%02x.%02x: PCI Class:                    %x%x%x\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function, MyVideoCards[Index].ClassCode[0], MyVideoCards[Index].ClassCode[1], MyVideoCards[Index].ClassCode[2]);
		
		if (MyVideoCards[Index].IsMainVideoCard) {
			Print(L"VGA%d %02x:%02x.%02x: Main Video Card:             TRUE\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function);
		} else {
			Print(L"VGA%d %02x:%02x.%02x: Main Video Card:            FALSE\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function);
		}
		
		if (MyVideoCards[Index].AttributeVgaIo) {
			Print(L"VGA%d %02x:%02x.%02x: PCI VGA IO Attribute:        TRUE\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function);
		} else {
			Print(L"VGA%d %02x:%02x.%02x: PCI VGA IO Attribute:       FALSE\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function);
		}
		
		if (MyVideoCards[Index].AttributeVgaMemory) {
			Print(L"VGA%d %02x:%02x.%02x: PCI VGA Memory Attribute:    TRUE\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function);
		} else {
			Print(L"VGA%d %02x:%02x.%02x: PCI VGA Memory Attribute:   FALSE\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function);
		}
		
		if (MyVideoCards[Index].AttributeIo) {
			Print(L"VGA%d %02x:%02x.%02x: PCI IO Attribute:            TRUE\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function);
		} else {
			Print(L"VGA%d %02x:%02x.%02x: PCI IO Attribute:           FALSE\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function);
		}
		
		if (MyVideoCards[Index].AttributeBusMaster) { 
			Print(L"VGA%d %02x:%02x.%02x: PCI Bus Master Attribute:    TRUE\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function);
		} else {
			Print(L"VGA%d %02x:%02x.%02x: PCI Bus Master Attribute:   FALSE\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function);
		}
		
		if (MyVideoCards[Index].IsActiveVideoCard) {
			Print(L"VGA%d %02x:%02x.%02x: Has GraphicsOutputProtocol:  TRUE\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function);
		} else {
			Print(L"VGA%d %02x:%02x.%02x: Has GraphicsOutputProtocol: FALSE\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function);
		}
		
		if (MyVideoCards[Index].RomSize>0) {
			
			Print(L"VGA%d %02x:%02x.%02x: Option ROM:              %08x - %08x (%x)\n",  Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function, MyVideoCards[Index].RomLocation, (((UINTN)MyVideoCards[Index].RomLocation) + MyVideoCards[Index].RomSize), MyVideoCards[Index].RomSize);
			
		} else {
			
			Print(L"VGA%d %02x:%02x.%02x: Option ROM:                  NONE\n",  Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function);
			
		}
		
		Print(L"VGA%d %02x:%02x.%02x: EFI Handle:              %x\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function, MyVideoCards[Index].Handle);
		
		Status = BS->HandleProtocol(MyVideoCards[Index].Handle, &gEfiDevicePathProtocolGuid, (VOID **) &DevicePath);
		Print(L"VGA%d %02x:%02x.%02x: DevicePath:              %s\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function, ConvertDevicePathToText(DevicePath, FALSE, FALSE));
		
		Print(L"\n");
		
	}
	
	return EFI_SUCCESS;
}


///
///Eventually it should identify all the GOP implementations,
///their controlling device (that is, to which PCI device it belongs).
///And if the multiple implementations point to the same GOP implementation.
///We just need to implement climbing up from Controller Handle to
///Controller Handle until we hit a PciIo.
///If the GOP Implementation does not support the standard
///resolutions, it will override the GOP Implementation with our own.
///
EFI_STATUS EfiIdentifyGraphicsOutputProtocol() {
	
	
	EFI_HANDLE                            *HandleBuffer;
	EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *InfoMode;
	EFI_OPEN_PROTOCOL_INFORMATION_ENTRY   *ProtocolInformation;
//	EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
//	CHAR16                                *DevicePathString;
	
	BOOLEAN                                FoundVGA;
	BOOLEAN                                FoundSVGA;
	BOOLEAN                                FoundXGA;
	UINTN                                  BufferSize;
	UINTN                                  ProtocolInformationCount;
	UINTN                                  Index;
	UINTN                                  ProtocolIndex;
	UINT32                                 ModeIndex;
	UINTN                                  SizeOfInfo;
	
	BufferSize = 0;
	
	///Get all the Graphics Output Protocol Devices
	Status = BS->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &BufferSize, &HandleBuffer);
	if (EFI_ERROR (Status)) { Print(L"EfiOverrideGraphicsOutputProtocol: Error getting any Graphics Output Protocol instances Handles: %r\n", Status); return Status; };
	
#ifdef DEBUG_OUTPUT
	///Debug:::
	Print(L"Found %d GraphicsOutput Protocol Instances\n\n", BufferSize);
#endif
	
	BS->AllocatePool(EfiBootServicesData, sizeof(EFI_GOP_IMPLEMENTATION)*BufferSize, (void**) &MyGOPImplementations);
	NumberGOPImplementations=BufferSize;
	
	for (Index=0; Index < BufferSize; Index++) {
		
#ifdef DEBUG_OUTPUT
		///Debug:::
		Print(L"GraphicsOutput %d: EFI Handle                               %08x\n", Index, HandleBuffer[Index]);
#endif
		Status = BS->OpenProtocolInformation(HandleBuffer[Index], &gEfiGraphicsOutputProtocolGuid, &ProtocolInformation, &ProtocolInformationCount);
		if (EFI_ERROR (Status)) { Print(L"EfiOverrideGraphicsOutputProtocol: Error getting the Protocol Information for instance %d: %r\n", Index, Status); return Status; };
			
/*#ifdef DEBUG_OUTPUT
		///Debug:::
		Status = BS->HandleProtocol(HandleBuffer[Index], &gEfiDevicePathProtocolGuid, &DevicePath);
		DevicePathString=ConvertDevicePathToText(DevicePath, FALSE, FALSE);

		Print(L"GraphicsOutput %d: DevicePath:                              %s\n", Index, DevicePathString);
		BS->FreePool(DevicePathString);
		DevicePath=NULL;
#endif*/
		
#ifdef DEBUG_OUTPUT
		///Debug
		for (ProtocolIndex = 0; ProtocolIndex < ProtocolInformationCount; ProtocolIndex++) {
				
			Print(L"GraphicsOutput %d: ProtocolInformation %d: AgentHandle:      %08x\n", Index, ProtocolIndex, ProtocolInformation[ProtocolIndex].AgentHandle);
			Print(L"GraphicsOutput %d: ProtocolInformation %d: ControllerHandle: %08x\n", Index, ProtocolIndex, ProtocolInformation[ProtocolIndex].ControllerHandle);
			Print(L"GraphicsOutput %d: ProtocolInformation %d: Attributes:       %8x\n",  Index, ProtocolIndex, ProtocolInformation[ProtocolIndex].Attributes);
			Print(L"GraphicsOutput %d: ProtocolInformation %d: OpenCount:        %8d\n",  Index, ProtocolIndex, ProtocolInformation[ProtocolIndex].OpenCount);
				
		}
#endif
		
		///If the controller handle is a driver, let's set IsActiveVideoCard on the correct vga card
		if(ProtocolInformation[0].ControllerHandle!=NULL){
			for (ProtocolIndex = 0; ProtocolIndex < NumberVideoCards; ProtocolIndex++) {
				if(MyVideoCards->Handle==ProtocolInformation[0].ControllerHandle) {
					MyVideoCards->IsActiveVideoCard=TRUE;
				}
			}
		}
		
		BS->FreePool(ProtocolInformation);
		
		Status = BS->OpenProtocol(HandleBuffer[Index], &gEfiGraphicsOutputProtocolGuid, (VOID *) &GraphicsOutput, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
		if (EFI_ERROR (Status)) { Print(L"EfiOverrideGraphicsOutputProtocol: Error getting the Graphics Output Protocol instance %d: %r\n", Index, Status); GraphicsOutput = NULL; return Status; };
		
		
		if (GraphicsOutput != NULL) {
			
#ifdef DEBUG_OUTPUT
			///Debug:::Print the current GraphicsOutput mode
			Print(L"GraphicsOutput %d: Current Mode: %4dx%4d     FrameBuffer: %x-%x(%x)\n", Index, GraphicsOutput->Mode->Info->HorizontalResolution, GraphicsOutput->Mode->Info->VerticalResolution, GraphicsOutput->Mode->FrameBufferBase, GraphicsOutput->Mode->FrameBufferBase+GraphicsOutput->Mode->FrameBufferSize, GraphicsOutput->Mode->FrameBufferSize);
#endif
			FoundVGA =FALSE;
			FoundSVGA=FALSE;
			FoundXGA =FALSE;

			///Print all the GraphicsOutput Modes
			for (ModeIndex = 0; ModeIndex < GraphicsOutput->Mode->MaxMode; ModeIndex++){
				Status=GraphicsOutput->QueryMode(GraphicsOutput, ModeIndex, &SizeOfInfo, &InfoMode);
#ifdef DEBUG_OUTPUT
				///Debug:::Print Mode Information
				Print(L"GraphicsOutput %d: Mode ID %d   : %4dx%4d     FrameBuffer: %x-%x(%x)\n", Index, (ModeIndex), InfoMode->HorizontalResolution, InfoMode->VerticalResolution, GraphicsOutput->Mode->FrameBufferBase, GraphicsOutput->Mode->FrameBufferBase+GraphicsOutput->Mode->FrameBufferSize, GraphicsOutput->Mode->FrameBufferSize);
#endif
				if ((InfoMode->HorizontalResolution == 640) & (InfoMode->VerticalResolution == 480)) { FoundVGA=TRUE;};
				if ((InfoMode->HorizontalResolution == 800) & (InfoMode->VerticalResolution == 600)) { FoundSVGA=TRUE;};
				if ((InfoMode->HorizontalResolution ==1024) & (InfoMode->VerticalResolution == 768)) { FoundXGA=TRUE;};
			};
			
			MyGOPImplementations[Index].Protocol               = GraphicsOutput;
			MyGOPImplementations[Index].SupportsStdResolutions = TRUE;
			MyGOPImplementations[Index].MaxX                   = GraphicsOutput->Mode->Info->HorizontalResolution;
			MyGOPImplementations[Index].MaxY                   = GraphicsOutput->Mode->Info->VerticalResolution;
			MyGOPImplementations[Index].CurrentVirtualX        = GraphicsOutput->Mode->Info->HorizontalResolution;
			MyGOPImplementations[Index].CurrentVirtualY        = GraphicsOutput->Mode->Info->VerticalResolution;
			MyGOPImplementations[Index].CurrentXOffset         = 0;
			MyGOPImplementations[Index].CurrentYOffset         = 0;
			MyGOPImplementations[Index].VirtualFrameBuffer     = NULL;
			MyGOPImplementations[Index].CurrentVFBSize         = 0;
			MyGOPImplementations[Index].QueryMode              = GraphicsOutput->QueryMode;
			MyGOPImplementations[Index].SetMode                = GraphicsOutput->SetMode;
			MyGOPImplementations[Index].Blt                    = GraphicsOutput->Blt;
			MyGOPImplementations[Index].Mode                   = GraphicsOutput->Mode->Info;
			
			///Cleanup after ourselves
			Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiGraphicsOutputProtocolGuid, ImageHandle, NULL);
			if (EFI_ERROR (Status)) { Print(L"EfiOverrideGraphicsOutputProtocol: Error closing the Graphics Output Protocol instance %d: %r (Handle %08x, ImageHandle %08x)\n", Index, Status, HandleBuffer[Index], ImageHandle); return Status; };
			
			Print(L"\n");
		}
	}
	
	///Final Cleanup
	BS->FreePool(HandleBuffer);
	return EFI_SUCCESS;
	
}


///
///EFI Version of:
///int main (int argc, char *argv[]){ return 0;}
///
EFI_STATUS EFIAPI DxeMain(IN EFI_HANDLE SelfImageHandle, IN EFI_SYSTEM_TABLE  *SystemTable) {
	
	///
	///I'm Lazy. I don't want to type SystemTable->BootServices 1000 times.
	///
	ST = SystemTable;
	BS = SystemTable->BootServices;
	RS = SystemTable->RuntimeServices;
	ImageHandle = SelfImageHandle;
	
	///
	///Populate the VGA Cards Vector
	///
	Status = EfiPopulateVgaVectors();
	if(EFI_ERROR(Status)) { Print (L"Could not populate the VGA Cards Vector: %r\n", Status); } //return Status;}
	
	
	///
	///Identify the GraphicsOutput Protocol and put it in a Vector
	///
	Status = EfiIdentifyGraphicsOutputProtocol();
	if(EFI_ERROR(Status)) { Print(L"EfiIdentifyGraphicsOutputProtocol Error: %r\n", Status); }
	
	
	///
	///Identify all the available Option Roms
	///
	Status = EfiGetOptionRoms();
	if(EFI_ERROR(Status)) { Print(L"EfiGetOptionRoms Error: %r\n", Status); }
	
	///
	///Find the active VGA Card and set the VGA PCI Registers on it.
	///TODO: This is a dummy implementation that doesn't take into account the VGA Cards Vector information
	///
	Status = EfiWriteActiveGraphicsAdaptorPCIRegs();
	if(EFI_ERROR(Status)) { Print(L"EfiWritePciRegisters Error: %r\n", Status); }//return Status;  }
	
	///
	///Let's CleanUp after ourselves 
	///
	BS->FreePool (MyVideoCards);
	
	return EFI_SUCCESS;	
}
