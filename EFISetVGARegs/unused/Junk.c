

///
///Required for horizontally parsing the Firmware files
///
EFI_STATUS EfiParseSectionStream(IN VOID *Stream, IN UINTN StreamSize, OUT VOID *SectionImage, OUT UINTN *SectionImageSize){
	
	EFI_COMMON_SECTION_HEADER                *SectionType;
	VOID                                     *NewStream;
	UINTN                                     SectionSize;
	UINTN                                     TotalSize;
	
	Status=EFI_SUCCESS;
	TotalSize=0;
	while(TotalSize<StreamSize){
		
		SectionType=Stream;
		SectionSize=SECTION_SIZE(SectionType);
		
		TotalSize+=SectionSize;
		
/*		Print(L"SectionType: ");
		
		switch(SectionType->Type){
			case EFI_SECTION_COMPRESSION:
				Print(L"EFI_SECTION_COMPRESSION ");
				break;
			case EFI_SECTION_GUID_DEFINED:
				Print(L"EFI_SECTION_GUID_DEFINED ");
				break;
			case EFI_SECTION_PE32:
				Print(L"EFI_SECTION_PE32 ");
				break;
			case EFI_SECTION_PIC:
				Print(L"EFI_SECTION_PIC ");
				break;
			case EFI_SECTION_TE:
				Print(L"EFI_SECTION_TE ");
				break;
			case EFI_SECTION_DXE_DEPEX:
				Print(L"EFI_SECTION_DXE_DEPEX ");
				break;
			case EFI_SECTION_VERSION:
				Print(L"EFI_SECTION_VERSION ");
				break;
			case EFI_SECTION_USER_INTERFACE:
				Print(L"EFI_SECTION_USER_INTERFACE ");
				break;
			case EFI_SECTION_COMPATIBILITY16:
				Print(L"EFI_SECTION_COMPATIBILITY16 ");
				break;
			case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:
				Print(L"EFI_SECTION_FIRMWARE_VOLUME_IMAGE ");
				break;
			case EFI_SECTION_FREEFORM_SUBTYPE_GUID:
				Print(L"EFI_SECTION_FREEFORM_SUBTYPE_GUID ");
				break;
			case EFI_SECTION_RAW:
				Print(L"EFI_SECTION_RAW ");
				break;
			case EFI_SECTION_PEI_DEPEX:
				Print(L"EFI_SECTION_PEI_DEPEX ");
				break;
			default:
				Print(L"EFI_UNKNOWN_SECTION_TYPE %d", SectionType->Type);
				return EFI_SUCCESS;
		}

		Print(L"StreamSize: %d SectionSize: %d\n", StreamSize, SectionSize);*/
		
		Status=EfiParseSection(Stream, SectionSize, SectionImage, SectionImageSize);
		
		if((SectionSize<StreamSize)&(SectionSize>0)){ //If there's another section
			Stream=(VOID *)(UINTN)((UINTN)Stream + SectionSize);
			NewStream=ALIGN_POINTER(Stream, 4);
			
			StreamSize+=(UINTN)NewStream-(UINTN)Stream;
			Stream=NewStream;
			continue;
		} else { break; }
	}
	
	return(Status);
};


///
///Required for vertically parsing the Firmware files
///
EFI_STATUS EfiParseSection(IN VOID *Stream, IN UINTN StreamSize, OUT VOID **SectionImage, OUT UINTN *SectionImageSize){
	
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
				
				ScratchBuffer=AllocateZeroPool(ScratchSize);
				DecompressBuffer=AllocateZeroPool(UncompressedLength);
				
				Status=Status=Decompress->Decompress(Decompress, CompressedStream + 1, (UINT32)StreamSize, DecompressBuffer, UncompressedLength, ScratchBuffer, ScratchSize);
				if (EFI_ERROR(Status)) { Print (L"Decompress->Decompress Status: %r", Status); };
				
				SubsectionSize=UncompressedLength;
				Status=EfiParseSectionStream(DecompressBuffer, SubsectionSize, SectionImage, SectionImageSize);
				
				FreePool(ScratchBuffer);
				FreePool(DecompressBuffer);
				
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
				Status=EfiParseSectionStream(Subsection, SubsectionSize, SectionImage, SectionImageSize);
				
				FreePool(Subsection);
			};
			break;//EFI_SECTION_GUID_DEFINED
			
		case EFI_SECTION_RAW:
			*SectionImageSize=StreamSize-sizeof(EFI_COMMON_SECTION_HEADER);
			*SectionImage=AllocatePool(*SectionImageSize);
			BS->CopyMem(*SectionImage, (VOID *)(UINTN)((UINTN)Stream + sizeof(EFI_COMMON_SECTION_HEADER)), *SectionImageSize);
			break;//EFI_SECTION_PE32

		case EFI_SECTION_PE32:			
			*SectionImageSize=StreamSize-sizeof(EFI_COMMON_SECTION_HEADER);
			*SectionImage=AllocatePool(*SectionImageSize);
			BS->CopyMem(*SectionImage, (VOID *)(UINTN)((UINTN)Stream + sizeof(EFI_COMMON_SECTION_HEADER)), *SectionImageSize);
			break;//EFI_SECTION_PE32 EFI_SECTION_RAW
			
		default:
			return(EFI_INVALID_PARAMETER);
			break;
	}
	
	return(EFI_SUCCESS);
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
	EFI_COMMON_SECTION_HEADER      *SectionType;
	VOID                           *File;
	VOID                           *ImageFile;
	UINTN                           ImageFileSize;
	UINTN                           BufferSize;
	UINTN                           Index;
	UINTN                           FileSize;
	UINT8                           FileType;
	UINT32                          FileAttributes;
	UINT32                          AuthenticationStatus;
	
	ImageFileHandle = NULL;
	///Got a better idea?
	/// {0x389F751F, 0x1838, 0x4388, {0x83, 0x90, 0xCD, 0x81, 0x54, 0xBD, 0x27, 0xF8}}
	/// just doesn't work for some odd reason.
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
	
	///Get all the FirmwarVolumeProtocol instances
	Status = BS->LocateHandleBuffer (ByProtocol, &gEfiFirmwareVolumeProtocolGuid, NULL, &BufferSize, &HandleBuffer);
	if (EFI_ERROR (Status)) { return EFI_NOT_FOUND; }
		
	///You don't work with the firmware unless you raise the task
	///priority level to NOTIFY or above. We'll restore
	///the TPL at the end of the execution.
	PreviousPriority=BS->RaiseTPL(TPL_NOTIFY);
	
	///Let's take the firmware volumes one by one.
	for (Index = 0; Index < BufferSize; Index++) {
		
		///And open them
		Status = BS->OpenProtocol ( HandleBuffer[Index], &gEfiFirmwareVolumeProtocolGuid, (VOID *) &FirmwareVolume, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
		if (EFI_ERROR (Status)) {  Print(L"Could not open firmware Volume number %d: %r\n", Index, Status); continue; }
		
		
		///Set the search flag to 0x00
		///(EFI ALL FILES)
		FileType=0x00;
			
		///Read the file to a buffer (*File)
		Status = FirmwareVolume->ReadFile(FirmwareVolume, gEfiGuid, NULL, &FileSize, &FileType, &FileAttributes, &AuthenticationStatus);
		if (EFI_ERROR(Status)) { continue; };
			
		File = AllocateZeroPool(FileSize);
			
		///Read the file to a buffer (*File)
		Status = FirmwareVolume->ReadFile(FirmwareVolume, gEfiGuid, &File, &FileSize, &FileType, &FileAttributes, &AuthenticationStatus);
		if (EFI_ERROR(Status)) { Print(L"FirmwareVolume%d: Error while reading file: %r\n", Index, Status); };
		
		SectionType = File;
			
		if((SectionType->Type==EFI_SECTION_RAW)|(SectionType->Type==EFI_SECTION_PE32)|(SectionType->Type==EFI_SECTION_COMPRESSION)|(SectionType->Type==EFI_SECTION_GUID_DEFINED)) {

			ImageFile = NULL;
			ImageFileSize  = 0;
				
			///Use EfiParseSectionStream for horizontal parsing. It's known to be broken.
			Status=EfiParseSectionStream(SectionType, FileSize, &ImageFile, &ImageFileSize);
			
			///Debug only
			if (ImageFile!=NULL){Print(L"Successfully extracted %g\n", *gEfiGuid);}
			
			Status=BS->LoadImage(FALSE, ImageHandle, NULL, ImageFile, ImageFileSize, (VOID *) &ImageFileHandle);
			if (EFI_ERROR(Status)) { Print(L"FirmwareVolume%d: Error while loading the file: %r\n", Index, Status); };

			FreePool(ImageFile);
					
			break;
		} ///If it's a known sectionType.
				
		FreePool(File);
		
		Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiFirmwareVolumeProtocolGuid, ImageHandle, NULL);

		///Restore Task Priority Level since we won't touch the Firmware anymore.
		BS->RestoreTPL(PreviousPriority);
		
	};///For each firmware volume
	
	FreePool(HandleBuffer);
	
	if (ImageFileHandle!=NULL) {
		Status = BS->StartImage(ImageFileHandle, NULL, NULL);
		return Status;
	} else {
		return EFI_NOT_FOUND;
	}
	
}
