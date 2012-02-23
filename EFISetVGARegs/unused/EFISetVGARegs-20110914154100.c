#include <Uefi.h>
//#include <EfiDriverLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/EdidActive.h>
#include <Protocol/DeviceIo.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadFile.h>

EFI_BOOT_SERVICES  *BS;
//EFI_DXE_SERVICES *DS;
EFI_SYSTEM_TABLE  *ST;
EFI_STATUS Status;
#define CALC_EFI_PCI_ADDRESS(Bus, Dev, Func, Reg) \
    ((UINT64) ((((UINTN) Bus) << 24) + (((UINTN) Dev) << 16) + (((UINTN) Func) << 8) + ((UINTN) Reg)))

EFI_DEVICE_PATH_PROTOCOL * EfiFileDevicePath (IN EFI_HANDLE Device OPTIONAL, IN CHAR16 *FileName )
{
    UINTN                      Size;
    FILEPATH_DEVICE_PATH      *FilePath;
    EFI_DEVICE_PATH_PROTOCOL  *Eop;
    EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

    Size        = StrSize (FileName);
    FilePath    = AllocateZeroPool (Size + SIZE_OF_FILEPATH_DEVICE_PATH + sizeof (EFI_DEVICE_PATH_PROTOCOL));
    DevicePath  = NULL;

    if (FilePath != NULL) {
      //
      // Build a file path
      //
      FilePath->Header.Type     = MEDIA_DEVICE_PATH;
      FilePath->Header.SubType  = MEDIA_FILEPATH_DP;
      SetDevicePathNodeLength (&FilePath->Header, Size + SIZE_OF_FILEPATH_DEVICE_PATH);
      CopyMem (FilePath->PathName, FileName, Size);
      Eop = NextDevicePathNode (&FilePath->Header);
      SetDevicePathEndNode (Eop);

      //
      // Append file path to device's device path
      //
      DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) FilePath;
      if (Device != NULL) 
      {
        //Print(L"Device is not NULL\n");
        DevicePath = AppendDevicePath (DevicePathFromHandle (Device),DevicePath);
        FreePool (FilePath);
        //assert (DevicePath);// Doesn't work on my MS C++ Compiler
      }
    }
    //Print(L"Returning DevicePath\n");
    return DevicePath;
}

EFI_STATUS EfiSetVersion(IN UINT32 revision, IN UINT32 version)
{
    //Change EFI Version to 2.10 or whatever you want it to say
    ST->Hdr.Revision=((revision << 16) | version);

	return EFI_SUCCESS;
}

EFI_STATUS WriteActiveGraphicsAdaptorPCIRegs(IN EFI_HANDLE ImageHandle)
{
    EFI_HANDLE                      *HandleBuffer;
    UINTN                            BufferSize;
    UINTN                            Index;
    EFI_PCI_IO_PROTOCOL             *IoDev;
    EFI_HANDLE                      *MainVideoCardHandle;
    EFI_IO_WIDTH                     Width;
    UINTN                            VideoSegment;
    UINTN                            VideoBus;
    UINTN                            VideoDevice;
    UINTN                            VideoFunction;

    IoDev               = NULL;
    HandleBuffer        = NULL;
    MainVideoCardHandle = NULL;
    Width               = EfiPciWidthUint8;
    BufferSize          =    0;
    VideoSegment        =    0;
    VideoBus            =    0;
    VideoDevice         =    0;
    VideoFunction       =    0;
	
    //Get all the PciIo Protocol devices
    Status = BS->LocateHandleBuffer(ByProtocol, &gEfiPciIoProtocolGuid, NULL, &BufferSize, &HandleBuffer);
    if (EFI_ERROR (Status)) { Print(L"WritePCIReg LocateHandleBuffer: %r\n", Status); return Status; };
	
    //Out of all the PciIo Protocol instances, select the one with EdidActive
    for(Index = 0; Index < BufferSize; Index++) {
        Status = BS->OpenProtocol (HandleBuffer[Index], &gEfiEdidActiveProtocolGuid, NULL, ImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL); //The device also has EDIDActive, thus is the main video card and assumably a PCI device.
        if (EFI_ERROR (Status)) { Print(L"WritePCIReg HandleProtocol EdidActive for handle %d: %r\n", Index, Status); continue; };
        MainVideoCardHandle=HandleBuffer[Index];
        Print(L"We have an EdidActiveProtocol at handle %d\n", Index);
    }

    //We identified the Active Video Card. Let's use it now.
    //Let's first get a PCI IO Protocol on it.
    Status = BS->HandleProtocol(MainVideoCardHandle, &gEfiPciIoProtocolGuid, (VOID *) &IoDev);
    if (EFI_ERROR (Status)) { Print(L"WritePCIReg HandleProtocol PciIo: %r\n", Status); return Status; }

    //Save the Bus number for the video card AND
    //Set the PCI regs for the video card
    if (IoDev == NULL) { 
        Print(L"No such Dev! \n");//This shouldn't happen 
        Status = EFI_INVALID_PARAMETER; 
    } else {
        Print(L"We have a PCI Io Protocol for the Handle\n");
        Status=IoDev->Attributes (IoDev, EfiPciIoAttributeOperationEnable, EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY | EFI_PCI_IO_ATTRIBUTE_IO | EFI_PCI_IO_ATTRIBUTE_BUS_MASTER, NULL);
        if (EFI_ERROR (Status)) { Print(L"WritePCIReg Iodev->Attributes: %r\n", Status); }
        Status=IoDev->GetLocation (IoDev, &VideoSegment, &VideoBus, &VideoDevice, &VideoFunction); //For debugging information
        if (EFI_ERROR (Status)) { Print(L"WritePCIReg Iodev->GetLocation: %r\n", Status); }
    }

    //Print out the debugging information, to check if we have the correct device selected.
    Print(L"Main PCI Video Card: Segment=%d Bus=%d, Device=%d Function=%d\n", VideoSegment, VideoBus, VideoDevice, VideoFunction);
	
    return EFI_SUCCESS;
}w

EFI_STATUS LoadExecuteImage(IN EFI_HANDLE ImageHandle, IN CHAR16 *PathName)
{
    EFI_HANDLE                        BootLoader;
    EFI_HANDLE                        SelfImageHandle;
    EFI_LOADED_IMAGE                 *SelfLoadedImage;
    EFI_DEVICE_PATH_PROTOCOL         *FilePath;

    SelfImageHandle = ImageHandle;
	
    //Get me a LoadedImageProtocol of MySelf
    Status = BS->HandleProtocol (SelfImageHandle, &gEfiLoadedImageProtocolGuid, (VOID*)&SelfLoadedImage);
    if(EFI_ERROR(Status)) { Print(L"LoadedImageProtocol Error: %r\n", Status); return Status;  }

    //Use the DeviceHandle part of the LoadedImageProtocol of MySelf and append the MS Bootloader Path to it.
    FilePath=EfiFileDevicePath(SelfLoadedImage->DeviceHandle, PathName);

    //Load the resulting LoadedImageProtocol
    Status = BS->LoadImage(	TRUE, ImageHandle, FilePath, NULL, 0, &BootLoader);
    if(EFI_ERROR(Status)){ Print(L"LoadImage Error: %r\n", Status); return Status;    }
	
    //Start executing the resulting LoadedImageProtocol
    Status = BS->StartImage(BootLoader, NULL, NULL);
    if(EFI_ERROR(Status)){ Print(L"BS->Start(BootLoader) Error: %r\n", Status); return Status;    }

    return Status;
}

EFI_STATUS LoadVGABios(IN EFI_HANDLE ImageHandle, IN CHAR16 *PathName, UINT32 Int10hValue)
{

    VOID                             *ROM_Addr;
	UINTN                             Int10hAddr;
    EFI_LOADED_IMAGE                 *SelfLoadedImage;
	EFI_FILE_PROTOCOL				 *vBiosFile;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Volume;
	EFI_FILE_HANDLE                   VolumeRootHandle;
	UINTN                            *BufferSize; 

	VolumeRootHandle = NULL;
    ROM_Addr         = NULL;
	ROM_Addr         = (VOID *)(UINTN)0xC0000; // Use 0xC0000l for IA32
    Int10hAddr       = 0x40;      //Use 0x40l for IA32
	
	BufferSize       = NULL;
	*BufferSize      = 0x10000;
	
	//Get me a LoadedImageProtocol of MySelf
    Status = BS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID*)&SelfLoadedImage);
    if(EFI_ERROR(Status)) { Print(L"LoadedImageProtocol Error: %r\n", Status); return Status;  }
	
	//Now let's get a File Handle to the Root Directory from SelfImageHandle
	Status = BS->HandleProtocol (SelfLoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID *) &Volume);
	if(EFI_ERROR(Status)) { Print(L"HandleProtocol Volume: %r\n", Status); return Status;  }

	//Open the bloody file (Root Directory)
    Status = Volume->OpenVolume( Volume, &VolumeRootHandle);
    if(EFI_ERROR(Status)) { Print(L"OpenRootVolume Error: %r\n", Status); return Status;  }


	//Open the VideoBios File
	VolumeRootHandle->Open(VolumeRootHandle, &vBiosFile, PathName, 0x0000000000000001, 0);
	if(EFI_ERROR(Status)) { Print(L"OpenFile Error: %r\n", Status); return Status;  }

	//No need for the Volume Root Handle
	VolumeRootHandle->Close(VolumeRootHandle);

	/*//Unlock the RAM at 0xC0000
	//First for the Memory Controller Hub architectures
	Status=writePciRegister(0x00000090, 0x30);
	if(EFI_ERROR(Status)) { Print(L"writePciRegister Error: %r\n", Status); return Status;  }
	Status=writePciRegister(0x00000091, 0x33);
	if(EFI_ERROR(Status)) { Print(L"writePciRegister Error: %r\n", Status); return Status;  }
	Status=writePciRegister(0x00000092, 0x33);
	if(EFI_ERROR(Status)) { Print(L"writePciRegister Error: %r\n", Status); return Status;  }
	Status=writePciRegister(0x00000093, 0x33);
	if(EFI_ERROR(Status)) { Print(L"writePciRegister Error: %r\n", Status); return Status;  }
	Status=writePciRegister(0x00000094, 0x33);
	if(EFI_ERROR(Status)) { Print(L"writePciRegister Error: %r\n", Status); return Status;  }
	Status=writePciRegister(0x00000095, 0x33);
	if(EFI_ERROR(Status)) { Print(L"writePciRegister Error: %r\n", Status); return Status;  }
	Status=writePciRegister(0x00000096, 0x33);
	if(EFI_ERROR(Status)) { Print(L"writePciRegister Error: %r\n", Status); return Status;  }
	Status=writePciRegister(0x00000097, 0x00);
	if(EFI_ERROR(Status)) { Print(L"writePciRegister Error: %r\n", Status); return Status;  }
	//Then for the Platform Controller Hub architectures
    Status = EfiInitializeDriverLib (ImageHandle, SystemTable);
    if (!EFI_ERROR (Status)) {
     Status = EfiGetSystemConfigurationTable (&gEfiDxeServicesTableGuid, (VOID **) &DS);}
    if(EFI_ERROR(Status)) { Print(L"Cannot get DXE Services Table Error: %r\n", Status); return Status;  }
	Status=DS->SetMemorySpaceAttributes(0xC0000ll, 0x10000ll, EFI_MEMORY_WC);
	if(EFI_ERROR(Status)) { Print(L"Cannot set Memory Space Attributes for 0xC0000: %r\n", Status); return Status;  }

	//Allocate the Memory Pages
    //Status = BS->AllocatePages (EfiBootServicesData, *BufferSize/4096, (UINTN)&ROM_Addr);
    //if (EFI_ERROR (Status)) { Print(L"AllocatePool Error: %r\n", Status);return Status;}*/

	//Start reading from the Video Bios File into the Video Bios address
	Status=vBiosFile->Read(vBiosFile, BufferSize, ROM_Addr);
    if(EFI_ERROR(Status)) { Print(L"ReadFile Error: %r\n", Status); return Status;  }
	
	//Lock the RAM at 0xC0000
	/*Status=writePciRegister(0x00000090, 0x10);
	if(EFI_ERROR(Status)) { Print(L"writePciRegister Error: %r\n", Status); return Status;  }
	Status=writePciRegister(0x00000091, 0x11);
	if(EFI_ERROR(Status)) { Print(L"writePciRegister Error: %r\n", Status); return Status;  }
	Status=writePciRegister(0x00000092, 0x11);
	if(EFI_ERROR(Status)) { Print(L"writePciRegister Error: %r\n", Status); return Status;  }
	Status=writePciRegister(0x00000093, 0x11);
	if(EFI_ERROR(Status)) { Print(L"writePciRegister Error: %r\n", Status); return Status;  }
	Status=writePciRegister(0x00000094, 0x11);
	if(EFI_ERROR(Status)) { Print(L"writePciRegister Error: %r\n", Status); return Status;  }*/
	
	Print(L"BufferSize = 0x%x\nMyBuffer Address: 0x%x", *BufferSize, ROM_Addr);
	
	//*BufferSize=262144;
	//BS->CopyMem(MyBuffer, (VOID *)0xc0000ll, *BufferSize);

	vBiosFile->Close(vBiosFile);

	//This is some wierd shit coppied from EFI-Shell! Fsck-it, it works!
	*(UINT32 *)(UINTN)Int10hAddr= *(UINT32 *)&Int10hValue;

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE  *SystemTable)
{	
    //I'm Lazy. I don't want to type SystemTable->BootServices 1000 times.
    ST = SystemTable;
    BS = ST->BootServices;
	
    Status = EfiSetVersion(2,10); //No need for read status. This works on all EFI releases present and future.
    if(EFI_ERROR(Status)) { Print(L"EfiSetVersion Error: %r\n", Status); return Status;  }

	Status = WriteActiveGraphicsAdaptorPCIRegs(ImageHandle);
    if(EFI_ERROR(Status)) { Print(L"WritePCIReg Error: %r\n", Status); return Status;  }

    //Load VGA Bios & Int10h pointer temporary until we have the autodiscovery in the ROM up and running
    //Status = LoadVGABios(ImageHandle, L"\\EFI\\Boot\\vBios.bin", 0x00C0C917); //Self, VBios Path, int10h value for MBP6,1_NVidia
    //Status = LoadVGABios(ImageHandle, L"\\EFI\\Boot\\vBios.bin", 0xC00017C9); //Due to Endianness issues for MBP6,1_NVidia
	//Status = LoadVGABios(ImageHandle, L"\\EFI\\Boot\\vBios.bin", 0xC000F21B); //Due to Endianness issues for MBP5,3_NVidia
    //if(EFI_ERROR(Status)) { Print(L"LoadVGABios Error: %r\n", Status); return Status;  }

    //Status = LoadExecuteImage(ImageHandle, L"EFI\\Ubuntu\\grubx64.efi");         //Valid for Ubuntu x64 EFI
    Print(L"Load and Execute \\EFI\\Microsoft\\Boot\\bootmgfw.efi");
    Status = LoadExecuteImage(ImageHandle, L"EFI\\Microsoft\\Boot\\bootmgfw.efi"); //Valid for Windows x64 UEFI
    if(EFI_ERROR(Status)) { Print(L"LoadExecuteImage Error: %r\n", Status); return Status; }
	
    return EFI_SUCCESS;
}