#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>
#include <Protocol/TianoDecompress.h>
#include <Protocol/GuidedSectionExtraction.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include <Pi/PiDxeCis.h>    //Common Section Header
#include <FirmwareVolume.h> //Firmware Volume Protocol
#include <EFISetVGARegs.h>

///
///Loads an image to a Runtime buffer and executes it.
///
EFI_STATUS EfiLoadExecuteImage(IN CHAR16 *PathName) {
	
	EFI_HANDLE                        ImageFileHandle;
	EFI_LOADED_IMAGE                 *SelfLoadedImage;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Volume;
	EFI_FILE_PROTOCOL                *Root;
	EFI_FILE_PROTOCOL                *FileProtocol;
	EFI_FILE_INFO                    *FileInfo;
	UINTN                             FileInfoSize;
	UINTN                             FileSize;
	VOID                             *File;
	
	
	///Get me a LoadedImageProtocol of MySelf
	Status = BS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID*)&SelfLoadedImage);
	if(EFI_ERROR(Status)) { Print(L"LoadedImageProtocol Error: %r\n", Status); return Status;  }
	
	///Get the root directory
	Status = BS->OpenProtocol(SelfLoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID *) &Volume, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
	if(EFI_ERROR(Status)){ Print(L"Couldn't load the SimpleFileSystem protocol for my own device handle: %r\n", Status); return Status;};
	
	///Open the Root directory
	Status = Volume->OpenVolume(Volume, &Root);
	if(EFI_ERROR(Status)){ Print(L"Couldn't open the SimpleFileSystem protocol for my own device handle: %r\n", Status); return Status;};
	
	///Open the file from the Root directory
	Status = Root->Open(Root, &FileProtocol, PathName, EFI_FILE_MODE_READ, 0);
	if(EFI_ERROR(Status)){ Print(L"Couldn't open the file protocol for the specified pathname \"%S\": %r\n", PathName, Status); return Status;};
	
	FileSize=0;
	
	FileInfoSize = sizeof(EFI_FILE_INFO);

	Status=BS->AllocatePool(EfiBootServicesData, FileInfoSize, (VOID *) &FileInfo);

	///Get the file size
	Status = FileProtocol->GetInfo(FileProtocol, &gEfiFileInfoGuid, &FileInfoSize, FileInfo);	

	Status=BS->FreePool(FileInfo);

	Status=BS->AllocatePool(EfiBootServicesData, FileInfoSize, (VOID *) &FileInfo);
	
	Status = FileProtocol->GetInfo(FileProtocol, &gEfiFileInfoGuid, &FileInfoSize, FileInfo);
	
	FileSize=(UINTN)FileInfo->FileSize;
	
	///Allocate the buffer
	Status=BS->AllocatePool(EfiRuntimeServicesData, FileSize, &File);
	if(EFI_ERROR(Status)){ return Status;};
	
	///Read the file in the buffer
	Status = FileProtocol->Read(FileProtocol, &FileSize, File);
	if(EFI_ERROR(Status)){ Print(L"Couldn't read the file for the specified pathname \"%S\": %r\n", PathName, Status); BS->FreePool(File); return Status;};
	
	Print(L"Loaded \"%S\" into memory. %d bytes.\n", PathName, FileSize);
	
	///Load the resulting File
	Status = BS->LoadImage(FALSE, ImageHandle, NULL, File, FileSize, (VOID *) &ImageFileHandle);
	if(EFI_ERROR(Status)){ Print(L"Couldn't load the Image \"%S\": %r\n", PathName, Status); return Status;};
	
	///Start executing the resulting LoadedImageProtocol
	Status = BS->StartImage(ImageFileHandle, NULL, NULL);
	if(EFI_ERROR(Status)){ Print(L"BS->StartImage Error \"%S\": %r\n", PathName, Status); return Status;};
	
	Status = BS->CloseProtocol(SelfLoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, ImageHandle, NULL);
	if(EFI_ERROR(Status)){ Print(L"Couldn't close the SimpleFilesystem protocol for my own device handle \"%S\": %r\n", PathName, Status); return Status;};
	
	return Status;
	
}


///
///Creates a full device path from the Filename string and the current device path of ImageHandle.
///
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
		BS->CopyMem (FilePath->PathName, FileName, Size);
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


///
///Loads and executes an image whose path was created with EfiFileDevicePath
///
EFI_STATUS EfiExecuteImage(IN CHAR16 *PathName) {
    
	EFI_HANDLE                        BootLoader;
	EFI_LOADED_IMAGE                 *SelfLoadedImage;
	EFI_DEVICE_PATH_PROTOCOL         *FilePath;
	
	//
	//Get me a LoadedImageProtocol of MySelf
	//
	Status = BS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID*)&SelfLoadedImage);
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


///
///EFI Version of:
///int main (int argc, char *argv[]){ return 0;}
///
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE SelfImageHandle, IN EFI_SYSTEM_TABLE  *SystemTable) {
	
	///
	///I'm Lazy. I don't want to type SystemTable->BootServices 1000 times.
	///
	ST = SystemTable;
	BS = SystemTable->BootServices;
	ImageHandle = SelfImageHandle;

	///
	///Load Variable Runtime DXE
	///
	Status = EfiLoadExecuteImage(L"EFIUpdateReleaseDxe.efi");
	if(EFI_ERROR(Status)) { Print(L"LoadExecuteImage Error: %r\n", Status); }//return Status; }

	///
	///Only after adding the query variable info
	///
	RS = SystemTable->RuntimeServices;
	
	///
	///Load the Set VGA Regs DXE
	///
	Status = EfiLoadExecuteImage(L"EFISetVGARegsDxe.efi");
	if(EFI_ERROR(Status)) { Print(L"LoadExecuteImage Error: %r\n", Status); }//return Status; }

	///
	///Load the Microsoft Bootloader
	///
	Status = EfiExecuteImage(L"EFI\\Microsoft\\Boot\\bootmgfw.efi");
	if(EFI_ERROR(Status)) { Print(L"LoadExecuteImage Error: %r\n", Status); }//return Status; }
	
	///
	///We should not actually ever get here unless we fail to load the bootloader.
	///I wouldn't imagine that there is an exit out of the Windows OS
	///This is not Solaris in OpenFirmware.
	///I'd like to think that I'm doing a good job cleaning up after myself anyway.
	///
	Status = BS->Exit(ImageHandle, EFI_SUCCESS, 0, NULL);
	
	return EFI_SUCCESS;
	
}

