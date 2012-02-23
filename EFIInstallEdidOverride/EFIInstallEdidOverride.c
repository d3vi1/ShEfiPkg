#include <Uefi.h>
#include <Library/UefiDriverEntryPoint.h>

EFI_BOOT_SERVICES               *BS;
EFI_RUNTIME_SERVICES            *RS;
EFI_SYSTEM_TABLE                *ST;
EFI_STATUS                       Status;
EFI_EDID_OVERRIDE_PROTOCOL       Private;

EFI_STATUS GetEdid (IN EFI_EDID_OVERRIDE_PROTOCOL *This, IN EFI_HANDLE *ChildHandle, OUT UINT32 *Attributes, IN OUT UINTN *EdidSize, IN OUT UINT8 **Edid){

	///We only respond to requests for our own EDID Override Protocol

	if (This==Private){
			Attributes = 0x0;
			EdidSize = sizeof(EDID);
			Edid = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00//Sync
			         0x06, 0xAF             //Manufacturer
			         0x00, 0x00             //ProductId
			         0x00, 0x00, 0x00, 0x00,//SerialNumber
			return EFI_SUCCESS;
	};
	return EFI_UNSUPPORTED;
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
	RS = SystemTable->BootServices;
	ImageHandle=SelfImageHandle;

	Private->GetEdid=GetEdid;

	Status=BS->InstallProtocolInterface(ImageHandle, gEfiEdidOverrideProtocolGuid, EFI_NATIVE_INTERFACE, &Private);
	
	
}