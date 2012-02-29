#include <Protocol/FrameworkFirmwareVolumeBlock.h>
#include <Protocol/FirmwareVolume.h>
///Since we're in PI1.0 mode (EFI 1.10),
///we have special capabilities that are
///no longer documented in FirmwareVolume.h
#include <EfiFirmwareVolumeHeader1.h>
#include <EfiVariable.h>


///Testing only
EFI_GET_VARIABLE                 OrigGetVariable;
EFI_SET_VARIABLE                 OrigSetVariable;
EFI_GET_NEXT_VARIABLE_NAME       OrigGetNextVariableName;

///Usual stuff
EFI_HANDLE                       ImageHandle;
EFI_BOOT_SERVICES               *BS;
EFI_RUNTIME_SERVICES            *RS;
EFI_SYSTEM_TABLE                *ST;
EFI_STATUS                       Status;

EFI_RUNTIME_SERVICES            *OldRS;
EFI_EVENT                        mVirtualAddressChangeEvent;
EFI_EVENT                        mRuntimeServicesEvent;
VARIABLE_STORE_HEADER           *VSHeader;
UINT64                           MaxVarSize;
UINT64                           VarStoreSize;
UINT64                           VarStoreFree;
BOOLEAN                          AtRuntime;
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;

///
///Don't want to use the Print
///function from the UEFILib
///Just have a clean implementation
///TODO: Add our own SPrintF implementation
///
void Print(IN CHAR16 *String){
	if (AtRuntime == FALSE){
		ConOut->OutputString(ConOut, String);
	}
}


///
///Locate the freaking variable store
///It only locates the first variable store
///and it doesn't check if it's valid.
///
EFI_STATUS GetVariableStore(){
	EFI_HANDLE                                    *HandleBuffer;
	UINTN                                          BufferSize;
	FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FVBlock;
	EFI_FIRMWARE_VOLUME_HEADER                    *FVHeader;
	UINTN                                          FVHeaderSize;
	UINTN                                          FVAlignment;
	EFI_PHYSICAL_ADDRESS                           FVBAddress;
	EFI_FVB_ATTRIBUTES                             FVAttributes;
	EFI_TPL                                        PreviousPriority;
	UINTN                                          Index;
	UINTN                                          i;
	
	///
	///Find all the firmware volume block protocols
	///
	Status=BS->LocateHandleBuffer(ByProtocol, &gFrameworkEfiFirmwareVolumeBlockProtocolGuid, NULL, &BufferSize, &HandleBuffer);
	if (EFI_ERROR (Status)) { return EFI_NOT_FOUND; }
	
	///
	///You don't work with the firmware unless you raise the task
	///priority level to NOTIFY or above. We'll restore
	///the TPL at the end of the execution.
	///
	PreviousPriority=BS->RaiseTPL(TPL_NOTIFY);
	
	///
	///Let's take the firmware volumes one by one.
	///
	for (Index = 0; Index < BufferSize; Index++) {
	
		///
		///And open them
		///
		Status = BS->OpenProtocol ( HandleBuffer[Index], &gFrameworkEfiFirmwareVolumeBlockProtocolGuid, (VOID *) &FVBlock, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
		if (EFI_ERROR (Status)) {  Print(L"Could not open firmware Volume\n"); continue; }
		
		///
		///Let's see the Header
		///
		FVHeader = NULL;
		Status   = FVBlock->GetPhysicalAddress(FVBlock, &FVBAddress);
		FVHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FVBAddress;
		FVAlignment = 1;
		
		///
		///Let's see how long the header is.
		///It's mandatory to have at least one Block Map
		///and one blank Block Map that states we've
		///finished going through the Block Maps
		///
		FVHeaderSize = sizeof (EFI_FIRMWARE_VOLUME_HEADER);
		for (i = 0; FVHeader->BlockMap[i].Length !=0; i++) {
			FVHeaderSize += sizeof (EFI_FV_BLOCK_MAP_ENTRY);
		}
		
		///
		///Let's see if the firmware has any alignment rules.
		///
		Status = FVBlock->GetAttributes(FVBlock, &FVAttributes);
		if (FVAttributes & EFI_FVB_ALIGNMENT_CAP) {
			///Bit shifting seems more elegant than a lot of if's.
			FVAlignment = 2 * (FVAttributes >> 16);
			FVAlignment = 8;
			
		} else {
			///1 Byte aligment (aka no alignment)
			FVAlignment = 1;
			
		};
		
		///FileSystemGuid: gEfiSystemNvDataFvGuid
		///{ 0xFFF12B8D, 0x7696, 0x4C8B, { 0xA9, 0x85, 0x27, 0x47, 0x07, 0x5B, 0x4F, 0x50 }}
		///TODO: Write a EfiCompareGuid function and compare to gEfiSysteNvDataFvGuid
		///TODO: Also check the Signature in the Header
		if ((FVHeader->FileSystemGuid.Data1==0xFFF12B8D)&(FVHeader->FileSystemGuid.Data2==0x7696)&(FVHeader->FileSystemGuid.Data3==0x4C8B)) {
			VSHeader=(VOID *)((UINTN)FVHeader+FVHeaderSize);
			if(((UINTN)VSHeader-(UINTN)FVHeader)%8!=0){
				VSHeader=(VOID *)(UINTN)((((UINTN)VSHeader-(UINTN)FVHeader)/8 + 1) * 8 + (UINTN)FVHeader);
			}
		}
		
		///
		///And finally, close the Firmware Volume Block
		///
		Status = BS->CloseProtocol (HandleBuffer[Index], &gFrameworkEfiFirmwareVolumeBlockProtocolGuid, ImageHandle, NULL);
		
		///
		///Since we're here, it means that we were in the
		///Root Firmware Volume Block so we don't need to continue.
		///
	}
	
	///
	///CleanUp:
	///Restore Task Priority Level since we won't touch the Firmware anymore.
	///Clear the buffers.
	///
	BS->RestoreTPL(PreviousPriority);
	BS->FreePool(HandleBuffer);
	
	return EFI_SUCCESS;
}


///
///Binary Search to find the biggest possible Variable
///Since this takes a long time(2-3 seconds), we're
///caching the value in a variable in the NVRAM.
///
UINTN GetMaxVariableSize(){
	UINTN            IndexMax;
	VOID            *Data;
	UINTN            MaxVariableSizeSize;
	UINTN            Max;
	UINTN            End;
	UINTN            Mid;
	UINTN            MaxVariableSize;
	EFI_GUID         MyGuid;
	
	MyGuid.Data1=0x6a98937E;
	MyGuid.Data2=    0xED34;
	MyGuid.Data3=    0x44eb;
	MyGuid.Data4[0]=   0xAE;
	MyGuid.Data4[1]=   0x97;
	MyGuid.Data4[2]=   0x1F;
	MyGuid.Data4[3]=   0xA5;
	MyGuid.Data4[4]=   0xE8;
	MyGuid.Data4[5]=   0xBD;
	MyGuid.Data4[6]=   0x21;
	MyGuid.Data4[7]=   0x19;
	
	MaxVariableSize=0;
	MaxVariableSizeSize=sizeof(UINTN);
	
	///Delete the test variable if it exists
	Status=RS->SetVariable(L"TestVar", &MyGuid, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, 0, NULL);
	
	///
	///Check if we have a cached value from a previous run.
	///
	Status = RS->GetVariable(L"MaxVariableSize", &MyGuid, NULL, &MaxVariableSizeSize, &MaxVariableSize);
	if (!EFI_ERROR (Status)) { return MaxVariableSize; };
	
	///
	///Let's see which power of 2 variable
	///size is too big. Gotta love binary searches.
	///
	for(IndexMax=0;; IndexMax++) {
		Max = 0x1ll<<IndexMax;
		BS->AllocatePool(EfiBootServicesData, Max, &Data);
		///Set the variable
		Status=RS->SetVariable(L"TestVar", &MyGuid, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, Max, Data);
		if (EFI_ERROR(Status)) { BS->FreePool(Data); break; }
		///Delete the variable
		Status=RS->SetVariable(L"TestVar", &MyGuid, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, 0, NULL);
		BS->FreePool(Data);
	}
	
	Mid = 0x1ll<<(IndexMax-1);
	End = Max;
	while (Mid<End) {
		Max = Mid + (End - Mid) / 2;
		BS->AllocatePool(EfiBootServicesData, Max, &Data);
		///Set the variable
		Status=RS->SetVariable(L"TestVar", &MyGuid, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, Max, Data);
		if (EFI_ERROR(Status)) { End = Max;}
		///Delete the variable
		Status=RS->SetVariable(L"TestVar", &MyGuid, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, 0, NULL);
		Mid = Max;
		BS->FreePool(Data);
	}
	
	
	///
	///Cache it for future
	///
	Status=RS->SetVariable(L"MaxVariableSize", &MyGuid, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, MaxVariableSizeSize, &Mid);
	
	return Mid;
}


///
///The next functions are needed by VariableServiceQueryVariableInfo
///
BOOLEAN IsValidVariableHeader (IN VARIABLE_HEADER *Variable) {
	if (Variable == NULL || Variable->StartId != VARIABLE_DATA) {
		return FALSE;
	}
	
	return TRUE;
}

UINTN NameSizeOfVariable (IN VARIABLE_HEADER *Variable){
	if (Variable->State    == (UINT8) (-1) ||
		Variable->DataSize == (UINT32) (-1) ||
		Variable->NameSize == (UINT32) (-1) ||
		Variable->Attributes == (UINT32) (-1)) {
		return 0;
	}
	return (UINTN) Variable->NameSize;
}

UINTN DataSizeOfVariable (IN VARIABLE_HEADER *Variable){
	if (Variable->State    == (UINT8)  (-1) ||
		Variable->DataSize == (UINT32) (-1) ||
		Variable->NameSize == (UINT32) (-1) ||
		Variable->Attributes == (UINT32) (-1)) {
		return 0;
	}
	return (UINTN) Variable->DataSize;
}

CHAR16 * GetVariableNamePtr (IN VARIABLE_HEADER *Variable){
	return (CHAR16 *) (Variable + 1);
}

UINT8 * GetVariableDataPtr (IN VARIABLE_HEADER *Variable){
	UINTN Value;
	Value =  (UINTN) GetVariableNamePtr (Variable);
	Value += NameSizeOfVariable (Variable);
	Value += GET_PAD_SIZE (NameSizeOfVariable (Variable));
	
	return (UINT8 *) Value;
}

VARIABLE_HEADER * GetNextVariablePtr (IN VARIABLE_HEADER *Variable){
	UINTN Value;
	
	if (!IsValidVariableHeader (Variable)) {
		return NULL;
	}
	
	Value =  (UINTN) GetVariableDataPtr (Variable);
	Value += DataSizeOfVariable (Variable);
	Value += GET_PAD_SIZE (DataSizeOfVariable (Variable));
	
	return (VARIABLE_HEADER *) HEADER_ALIGN (Value);
}

VARIABLE_HEADER * GetStartPointer (IN VARIABLE_STORE_HEADER *VarStoreHeader){
	return (VARIABLE_HEADER *) HEADER_ALIGN (VarStoreHeader + 1);
}

VARIABLE_HEADER * GetEndPointer (IN VARIABLE_STORE_HEADER *VarStoreHeader){
	return (VARIABLE_HEADER *) HEADER_ALIGN ((UINTN) VarStoreHeader + VarStoreHeader->Size);
}


///
///What Windows is actually looking for
///This function is the only thing missing for a
///UEFI compliant Runtime Services implementation
///
EFI_STATUS EFIAPI VariableServiceQueryVariableInfo (IN UINT32 Attributes, OUT UINT64 *MaximumVariableStorageSize, OUT UINT64 *RemainingVariableStorageSize, OUT UINT64 *MaximumVariableSize){
	VARIABLE_HEADER        *Variable;
	VARIABLE_HEADER        *NextVariable;
	UINT64                  VariableSize;
	UINT64                  CommonVariableTotalSize;
	
	CommonVariableTotalSize = 0;
	
	if(MaximumVariableStorageSize == NULL || RemainingVariableStorageSize == NULL || MaximumVariableSize == NULL || Attributes == 0) {
		return EFI_INVALID_PARAMETER;
	}
	
	if((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == 0) {
		///
		/// Make sure the Attributes combination is supported by the platform.
		///
		return EFI_UNSUPPORTED;  
	} else if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
		///
		/// Make sure if runtime bit is set, boot service bit is set also.
		///
		return EFI_INVALID_PARAMETER;
	} else if ( AtRuntime && ((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0)) {
		///
		/// Make sure RT Attribute is set if we are in Runtime phase.
		///
		return EFI_INVALID_PARAMETER;
	} else if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
		///
		/// Make sure Hw Attribute is set with NV.
		///
		return EFI_INVALID_PARAMETER;
	} else if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
		///
		/// Not support authentiated variable write yet.
		///
		return EFI_UNSUPPORTED;
	}
	
	*MaximumVariableStorageSize   = VSHeader->Size - sizeof (VARIABLE_STORE_HEADER);
	
	*MaximumVariableSize = MaxVarSize;
	
	Variable = GetStartPointer (VSHeader);
	
	while ((Variable < GetEndPointer (VSHeader)) && IsValidVariableHeader (Variable)) {
		NextVariable = GetNextVariablePtr (Variable);
		VariableSize = (UINT64) (UINTN) NextVariable - (UINT64) (UINTN) Variable;
		
		CommonVariableTotalSize += VariableSize;
		
		Variable = NextVariable;
	}
	
	*RemainingVariableStorageSize = *MaximumVariableStorageSize - CommonVariableTotalSize;
	
	if (*RemainingVariableStorageSize < sizeof (VARIABLE_HEADER)) {
		*MaximumVariableSize = 0;
	} else if ((*RemainingVariableStorageSize - sizeof (VARIABLE_HEADER)) < *MaximumVariableSize) {
		*MaximumVariableSize = *RemainingVariableStorageSize - sizeof (VARIABLE_HEADER);
	}

	return EFI_SUCCESS;
}


///
///Testing: A version of QueryVariable info that gives
///the cached results from the first run of the real
///implementation. It's used only for testing. After
///ConvertPointers is solved, this shouldn't be necessary.
///
EFI_STATUS EFIAPI RTQueryVariableInfo (IN UINT32 Attributes, OUT UINT64 *MaximumVariableStorageSize, OUT UINT64 *RemainingVariableStorageSize, OUT UINT64 *MaximumVariableSize){

	if(MaximumVariableStorageSize == NULL || RemainingVariableStorageSize == NULL || MaximumVariableSize == NULL || Attributes == 0) {
		return EFI_INVALID_PARAMETER;
	}
	
	if((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == 0) {
		///
		/// Make sure the Attributes combination is supported by the platform.
		///
		return EFI_UNSUPPORTED;  
	} else if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
		///
		/// Make sure if runtime bit is set, boot service bit is set also.
		///
		return EFI_INVALID_PARAMETER;
	} else if ( AtRuntime && ((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0)) {
		///
		/// Make sure RT Attribute is set if we are in Runtime phase.
		///
		return EFI_INVALID_PARAMETER;
	} else if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
		///
		/// Make sure Hw Attribute is set with NV.
		///
		return EFI_INVALID_PARAMETER;
	} else if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
		///
		/// Not support authentiated variable write yet.
		///
		return EFI_UNSUPPORTED;
	}
	
	MaximumVariableStorageSize = &VarStoreSize;
	RemainingVariableStorageSize = &VarStoreFree;
	MaximumVariableSize = &MaxVarSize;
//	*MaximumVariableStorageSize = (UINT64) VarStoreSize;
//	*RemainingVariableStorageSize = (UINT64) VarStoreFree;
//	*MaximumVariableSize = (UINT64) MaxVarSize;
	
	return EFI_SUCCESS;
}


///
///Overriding the functions to see if
///the convertPointer stuff works.
///
EFI_STATUS EFIAPI RTGetNextVariableName(IN OUT UINTN *VariableNameSize, IN OUT CHAR16 *VariableName, IN OUT EFI_GUID *VendorGuid){
return EFI_SUCCESS;//	return(OrigGetNextVariableName(VariableNameSize, VariableName, VendorGuid));
}

EFI_STATUS EFIAPI RTGetVariable(IN CHAR16 *VariableName, IN EFI_GUID *VendorGuid, OUT UINT32 *Attributes OPTIONAL, IN OUT UINTN *DataSize, OUT VOID *Data){
return EFI_SUCCESS;//	return(OrigGetVariable(VariableName, VendorGuid, Attributes, DataSize, Data));
}

EFI_STATUS EFIAPI RTSetVariable(IN CHAR16 *VariableName, IN EFI_GUID *VendorGuid, IN UINT32 Attributes, IN UINTN DataSize, IN VOID *Data){
return EFI_SUCCESS;//	return(OrigSetVariable(VariableName, VendorGuid, Attributes, DataSize, Data));
}


///
///This function converts the pointers to Virtual Address Pointers
///once the system has switched to virtual addressing mode from the
///EFI Physical addressing mode.
///
VOID EFIAPI VariableClassAddressChangeEvent (IN EFI_EVENT Event, IN VOID *Context){
	RS->ConvertPointer (0x0, (VOID **) &RS->GetTime);
	RS->ConvertPointer (0x0, (VOID **) &RS->SetTime);
	RS->ConvertPointer (0x0, (VOID **) &RS->GetWakeupTime);
	RS->ConvertPointer (0x0, (VOID **) &RS->SetWakeupTime);
	RS->ConvertPointer (0x0, (VOID **) &RS->ResetSystem);
	RS->ConvertPointer (0x0, (VOID **) &RS->GetNextHighMonotonicCount);
	RS->ConvertPointer (0x0, (VOID **) &RS->GetVariable);
	RS->ConvertPointer (0x0, (VOID **) &RS->SetVariable);
	RS->ConvertPointer (0x0, (VOID **) &RS->GetNextVariableName);
	RS->ConvertPointer (0x0, (VOID **) &RS->QueryVariableInfo);
	RS->ConvertPointer (0x0, (VOID **) &RS->UpdateCapsule);
	RS->ConvertPointer (0x0, (VOID **) &RS->QueryCapsuleCapabilities);
	RS->ConvertPointer (0x0, (VOID **) &OldRS);
	RS->ConvertPointer (0x0, (VOID **) &VSHeader);
	RS->ConvertPointer (0x0, (VOID **) &OrigGetVariable);
	RS->ConvertPointer (0x0, (VOID **) &OrigSetVariable);
	RS->ConvertPointer (0x0, (VOID **) &OrigGetNextVariableName);
	RS->ConvertPointer (0x0, (VOID **) &RS);
	///TODO: Add CRC32 recalculation of the RS header
	///Fixme: Figure Out which pointers need converting
}


///
///We're in Runtime mode from now on.
///Only used in Print
///
VOID EFIAPI ExitBootServicesEvent (IN EFI_EVENT Event, IN VOID *Context){
	AtRuntime=TRUE;
};


///
///Creates a new RuntimeServices table
///that has the same implementations for the existing ones
///and my own implementation for QueryVariableInfo
///For Testing, I'm also hooking the other variable services
///
EFI_STATUS EfiSetVersion(IN UINT32 revision, IN UINT32 version) {
	
	EFI_RUNTIME_SERVICES     *NewRS;
	
	ST->Hdr.Revision=((revision << 16) | version);
	
	Status = BS->AllocatePool(EfiRuntimeServicesCode, sizeof(EFI_RUNTIME_SERVICES), (void**) &NewRS);
	NewRS->Hdr.Signature             = RS->Hdr.Signature;
	NewRS->Hdr.Revision              = ((revision << 16) | version);
	NewRS->Hdr.HeaderSize            = sizeof(EFI_RUNTIME_SERVICES);
	NewRS->Hdr.CRC32                 = 0;
	NewRS->Hdr.Reserved              = RS->Hdr.Reserved;
	NewRS->GetTime                   = RS->GetTime;
	NewRS->SetTime                   = RS->SetTime;
	NewRS->GetWakeupTime             = RS->GetWakeupTime;
	NewRS->SetWakeupTime             = RS->SetWakeupTime;
	NewRS->SetVirtualAddressMap      = RS->SetVirtualAddressMap;
	NewRS->ConvertPointer            = RS->ConvertPointer;
	NewRS->GetVariable               = RS->GetVariable;
	NewRS->GetNextVariableName       = RS->GetNextVariableName;
	NewRS->SetVariable               = RS->SetVariable;
	NewRS->GetNextHighMonotonicCount = RS->GetNextHighMonotonicCount;
	NewRS->ResetSystem               = RS->ResetSystem;
	NewRS->UpdateCapsule             = RS->UpdateCapsule;
	NewRS->QueryCapsuleCapabilities  = RS->QueryCapsuleCapabilities;
	NewRS->QueryVariableInfo         = RTQueryVariableInfo;
	
	//ST->RuntimeServices=NewRS;
	RS=NewRS;
	OldRS->GetVariable=RTGetVariable;
	OldRS->GetNextVariableName=RTGetNextVariableName;
	OldRS->SetVariable=RTSetVariable;
	//Change EFI Version to 2.10 or whatever you want it to say
	//ST->Hdr.Revision=((revision << 16) | version);
	
	//Change the CRC32 Value to 0 in order to recalculate it
	//ST->Hdr.CRC32=0;
	RS->Hdr.CRC32=0;
	
	//Calculate the new CRC32 Value
	//Status=BS->CalculateCrc32 (ST, ST->Hdr.HeaderSize, (VOID *) &ST->Hdr.CRC32);
	Status=BS->CalculateCrc32 (RS, RS->Hdr.HeaderSize, (VOID *) &RS->Hdr.CRC32);
	
	return EFI_SUCCESS;
}


///
///EFI Version of:
///int main (int argc, char *argv[]){ return 0;}
///
EFI_STATUS EFIAPI DxeMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE  *SystemTable) {
	
	///We're going to need these...
	ST = SystemTable;
	BS = SystemTable->BootServices;
	RS = SystemTable->RuntimeServices;
	OldRS = SystemTable->RuntimeServices;
	
	///Temporary: i'm attempting to test the convertPointers
	OrigGetVariable=OldRS->GetVariable;
	OrigSetVariable=OldRS->SetVariable;
	OrigGetNextVariableName=OldRS->GetNextVariableName;
	
	///Required for "Print"
	ConOut=ST->ConOut;
	AtRuntime = FALSE;
	
	///Update the ST->RS
	Status = EfiSetVersion(2, 10);
	
	///Identify the variable store
	Status = GetVariableStore();
	
	///Get the Maximum Variable Size
	MaxVarSize = GetMaxVariableSize();

	///For Testing Only: Cache the answers in BootServices Mode so we can return them statically in Runtime Mode
	Status = VariableServiceQueryVariableInfo (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, &VarStoreSize, &VarStoreFree, &MaxVarSize);

	///Hook the Events
	Status = BS->CreateEvent ( EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE, TPL_NOTIFY, VariableClassAddressChangeEvent, NULL, &mVirtualAddressChangeEvent);
	Status = BS->CreateEvent ( EVT_SIGNAL_EXIT_BOOT_SERVICES, TPL_NOTIFY, ExitBootServicesEvent, NULL, &mRuntimeServicesEvent);
	
	return EFI_SUCCESS;
}
