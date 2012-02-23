#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/EdidActive.h>
#include <Protocol/DeviceIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DevicePathToText.h>
#include <Guid/DxeServices.h>
#include <Guid/GlobalVariable.h>
#include <Pi/PiDxeCis.h>
#include <MtrrLib.h>


typedef struct {
  EFI_TABLE_HEADER                Hdr;

  //
  // Global Coherency Domain Services
  //
  EFI_ADD_MEMORY_SPACE            AddMemorySpace;
  EFI_ALLOCATE_MEMORY_SPACE       AllocateMemorySpace;
  EFI_FREE_MEMORY_SPACE           FreeMemorySpace;
  EFI_REMOVE_MEMORY_SPACE         RemoveMemorySpace;
  EFI_GET_MEMORY_SPACE_DESCRIPTOR GetMemorySpaceDescriptor;
  EFI_SET_MEMORY_SPACE_ATTRIBUTES SetMemorySpaceAttributes;
  EFI_GET_MEMORY_SPACE_MAP        GetMemorySpaceMap;
  EFI_ADD_IO_SPACE                AddIoSpace;
  EFI_ALLOCATE_IO_SPACE           AllocateIoSpace;
  EFI_FREE_IO_SPACE               FreeIoSpace;
  EFI_REMOVE_IO_SPACE             RemoveIoSpace;
  EFI_GET_IO_SPACE_DESCRIPTOR     GetIoSpaceDescriptor;
  EFI_GET_IO_SPACE_MAP            GetIoSpaceMap;

  //
  // Dispatcher Services
  //
  EFI_DISPATCH                    Dispatch;
  EFI_SCHEDULE                    Schedule;
  EFI_TRUST                       Trust;
  //
  // Service to process a single firmware volume found in a capsule
  //
  EFI_PROCESS_FIRMWARE_VOLUME     ProcessFirmwareVolume;
} EFI_DXE_SERVICES;

typedef struct {
	UINT16               VendorId;
	UINT16               DeviceId;
	UINT8                RevisionId;
	UINT8                ClassCode[3];
	UINT8                Bus;
	UINT8                Device;
	UINT8                Function;
	BOOLEAN              IsMainVideoCard;
	BOOLEAN              IsActiveVideoCard;
	VOID                *RomLocation;
	UINTN                RomSize;
	EFI_PCI_IO_PROTOCOL *IoDev;
} EFI_VIDEO_CARDS_LIST;

EFI_VIDEO_CARDS_LIST*   MyVideoCards;
UINTN                   NumberVideoCards;


EFI_BOOT_SERVICES      *BS;
EFI_RUNTIME_SERVICES   *RS;
EFI_SYSTEM_TABLE       *ST;
EFI_STATUS              Status;

EFI_STATUS EfiSetVersion(IN UINT32 revision, IN UINT32 version) {

	//Change EFI Version to 2.10 or whatever you want it to say
	ST->Hdr.Revision=((revision << 16) | version);
	return EFI_SUCCESS;
	
}

EFI_STATUS EfiShadowVGAOptionROM (IN EFI_PCI_IO_PROTOCOL *IoDev) {
	
	VOID             *ROM_Addr;     //Pointer to the VGA ROM Shadow location
	UINT32            Int10h_Addr;  //Pointer to the int10h Software Interrupt Handler pointer
	UINT32            Int10h_Value; //The Value of the int10h Software Interrupt Handler Vector
	
	ROM_Addr         = (VOID *)(UINTN)0xC0000;
	Int10h_Addr      = 0x40;
//  Int10h_Value     = 0xC00017C9; MBP6,2
    Int10h_Value     = 0xC000F21B;
	
	//
	//Unlock the memory for the shadow space
	//

	//	Print(L"Unlocking the memory.\n");

    Status=DS->SetMemorySpaceAttributes (0xC0000, 0x40000, EFI_MEMORY_WB);
	if(EFI_ERROR(Status)) { Print(L"Could not set the Memory Space Attributes: %r\n", Status);}
	
	MtrrDebugPrintAllMtrrs();
	//
	//Copy the option ROM to the shadow space
	//
	Print(L"Shadowing the Option ROM: from:%x to:%x amount:%x\n", (UINTN)ROM_Addr, (UINTN)IoDev->RomImage, IoDev->RomSize);
    BS->CopyMem(ROM_Addr, IoDev->RomImage, IoDev->RomSize);
	Print(L"Shadowed the Option ROM\n");
	
	//
	//Lock the memory again
	//
//	Print(L"Setting the memory Attributes back to the original values.\n");
//	Status=DS->SetMemorySpaceAttributes(Descriptor->BaseAddress, Descriptor->Length, Descriptor->Attributes );
//  if(EFI_ERROR(Status)) { Print(L"Could not set the Memory Space Attributes: %r\n", Status); return Status;}

	//
	//Set the Int10h Pointer
	//
	*(UINT32 *)(UINTN)Int10h_Addr=*(UINT32 *)&Int10h_Value;

	return EFI_SUCCESS;
	
}

EFI_STATUS EfiWriteActiveGraphicsAdaptorPCIRegs(IN EFI_HANDLE ImageHandle) {

	EFI_PCI_IO_PROTOCOL                *IoDev;
	EFI_HANDLE                         *LegacyVgaHandle;
	UINTN                               LegacyVgaHandleValue;
	UINTN                               PointerSize;
	
	PointerSize=sizeof(UINTN);
	LegacyVgaHandleValue=0;
	
	Status = RS->GetVariable (L"LEGACYVGAHANDLE", &gEfiGlobalVariableGuid, NULL, &PointerSize, (VOID *) &LegacyVgaHandleValue);
	if (EFI_ERROR (Status)) { Print(L"Could not read the LEGACYVGAHANDLE NVRAM variable: %r\n", Status); return Status; };
	
	LegacyVgaHandle=(VOID *)LegacyVgaHandleValue;
	
	Status = BS->HandleProtocol (LegacyVgaHandle, &gEfiPciIoProtocolGuid, (VOID *) &IoDev);
	if (EFI_ERROR (Status)) { Print(L"WritePciReg: Error getting the PciIo Protocol instance %r\n", Status); return Status; };

	Status=IoDev->Attributes (IoDev, EfiPciIoAttributeOperationEnable, EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY | EFI_PCI_IO_ATTRIBUTE_IO | EFI_PCI_IO_ATTRIBUTE_BUS_MASTER, NULL);
	if (EFI_ERROR (Status)) { Print(L"WritePciReg:   PCI VGA Adapter on %02x:%02x.%02x: Couldn't set Attributes: %r\n", Status); };

	Status=EfiShadowVGAOptionROM (IoDev);
	if (EFI_ERROR (Status)) { Print(L"Couldn't shadow the Option ROM: %r\n", Status);};
	
	Print(L"Shadowed the Option ROM\n");
//	Status = BS->CloseProtocol (LegacyVgaHandle, &gEfiPciIoProtocolGuid, ImageHandle, NULL);

	return EFI_SUCCESS;

}

EFI_DEVICE_PATH_PROTOCOL * EfiFileDevicePath (IN EFI_HANDLE Device OPTIONAL, IN CHAR16 *FileName ) {

	UINTN                      Size;
	FILEPATH_DEVICE_PATH      *FilePath;
	EFI_DEVICE_PATH_PROTOCOL  *Eop;
	EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

	Size        = StrSize (FileName);
	FilePath    = AllocateZeroPool (Size + SIZE_OF_FILEPATH_DEVICE_PATH + sizeof (EFI_DEVICE_PATH_PROTOCOL));
	DevicePath  = NULL;

	if (FilePath != NULL) {

		FilePath->Header.Type     = MEDIA_DEVICE_PATH;
		FilePath->Header.SubType  = MEDIA_FILEPATH_DP;
		SetDevicePathNodeLength (&FilePath->Header, Size + SIZE_OF_FILEPATH_DEVICE_PATH);
		CopyMem (FilePath->PathName, FileName, Size);
		Eop = NextDevicePathNode (&FilePath->Header);
		SetDevicePathEndNode (Eop);

		DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) FilePath;

		if (Device != NULL) {
			DevicePath = AppendDevicePath (DevicePathFromHandle (Device),DevicePath);
			FreePool (FilePath);
		}

	}
	
	//Print(L"Returning DevicePath\n");
	return DevicePath;
	
}

EFI_STATUS EfiLoadExecuteImage(IN EFI_HANDLE ImageHandle, IN CHAR16 *PathName) {

	EFI_HANDLE                        BootLoader;
	EFI_HANDLE                        SelfImageHandle;
	EFI_LOADED_IMAGE                 *SelfLoadedImage;
	EFI_DEVICE_PATH_PROTOCOL         *FilePath;

	SelfImageHandle = ImageHandle;
	
	//
	//Get me a LoadedImageProtocol of MySelf
	//
	Status = BS->HandleProtocol (SelfImageHandle, &gEfiLoadedImageProtocolGuid, (VOID*)&SelfLoadedImage);
	if(EFI_ERROR(Status)) { Print(L"LoadedImageProtocol Error: %r\n", Status); return Status;  }

	//
	//Use the DeviceHandle part of the LoadedImageProtocol of MySelf and append the MS Bootloader Path to it.
	//
	FilePath=EfiFileDevicePath(SelfLoadedImage->DeviceHandle, PathName);

	//
	//Load the resulting LoadedImageProtocol
	//
	Status = BS->LoadImage(	TRUE, ImageHandle, FilePath, NULL, 0, &BootLoader);
	if(EFI_ERROR(Status)){ Print(L"LoadImage Error: %r\n", Status); return Status;    }
	
	//
	//Start executing the resulting LoadedImageProtocol
	//
	Status = BS->StartImage(BootLoader, NULL, NULL);
	if(EFI_ERROR(Status)){ Print(L"BS->Start(BootLoader) Error: %r\n", Status); return Status;    }

	return Status;
	
}

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE  *SystemTable) {
	
	//
	//I'm Lazy. I don't want to type SystemTable->BootServices 1000 times.
	//
    EFI_DXE_SERVICES   *DS;
	ST = SystemTable;
	BS = SystemTable->BootServices;
	RS = SystemTable->RuntimeServices;

	Status = EfiGetSystemConfigurationTable(&gEfiDxeServicesTableGuid, (VOID *) &DS);
	if(EFI_ERROR(Status)) { Print(L"Could not load the EFI_DXE_SERVICES Table\n", Status); return Status;}
	
	//
	//Set the Efi Version specified in the parameter
	//
	Status = EfiSetVersion(2,10);
	if(EFI_ERROR(Status)) { Print(L"EfiSetVersion Error: %r\n", Status); return Status;  }
	
	//
	//Find the active VGA Card and set the VGA PCI Registers on it.
	//
	Status = EfiWriteActiveGraphicsAdaptorPCIRegs(ImageHandle);
	if(EFI_ERROR(Status)) { Print(L"EfiWritePciRegisters Error: %r\n", Status); return Status;  }
	Print(L"Finished with the VGA card.\n");
	
	//
	//Chainload the OS Bootloader.
	//Try also L"EFI\\Ubuntu\\grubx64.efi"
	//
	Status = EfiLoadExecuteImage(ImageHandle, L"EFI\\Microsoft\\Boot\\bootmgfw.efi");
	if(EFI_ERROR(Status)) { Print(L"LoadExecuteImage Error: %r\n", Status); return Status; }
	
	//
	//We should not actually ever get here.
	//I wouldn't imagine that there is an exit out of the Windows OS
	//This is not Solaris in OpenFirmware.
	//
	return EFI_SUCCESS;

}