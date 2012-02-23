#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/EdidActive.h>
#include <Protocol/DeviceIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LegacyRegion.h>
#include <Protocol/Runtime.h>
#include <Guid/FileInfo.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/TianoDecompress.h>
#include <Protocol/GuidedSectionExtraction.h>
#include <Guid/DxeServices.h>
#include <Guid/GlobalVariable.h>
#include <Pi/PiDxeCis.h>
#include <FirmwareVolume.h>
#include <DevicePathToText.h>
//#include <TianoDecompress.h>


typedef struct {
    EFI_TABLE_HEADER                Hdr;
    
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
    
    EFI_DISPATCH                    Dispatch;
    EFI_SCHEDULE                    Schedule;
    EFI_TRUST                       Trust;
    
    EFI_PROCESS_FIRMWARE_VOLUME     ProcessFirmwareVolume;
    
} EFI_DXE_SERVICES_TABLE; //Didn't want to go throught the messed up build system for this. Just included what I needed.

typedef struct {
	UINT16                          VendorId;
	UINT16                          DeviceId;
 	
	UINT16                          Command;
	UINT16                          Status;
 	
	UINT8                           RevisionId;
	UINT8                           ClassCode[3];
 	
	UINT8                           CacheLineSize;
	UINT8                           PrimaryLatencyTimer;
	UINT8                           HeaderType;
	UINT8                           BIST; 	
} EFI_PCI_DEVICE_HEADER;

typedef struct {
	UINT16               VendorId;
	UINT16               DeviceId;
	UINT8                RevisionId;
	
	UINT8                ClassCode[3];
	
	UINTN                Bus;
	UINTN                Device;
	UINTN                Function;
	
	BOOLEAN              AttributeVgaIo;
	BOOLEAN              AttributeVgaMemory;
	BOOLEAN              AttributeIo;
	BOOLEAN              AttributeBusMaster;
	
	BOOLEAN              IsMainVideoCard;
	BOOLEAN              IsActiveVideoCard;
	
	VOID                *RomLocation;
	UINTN                RomSize;
	
	EFI_HANDLE          *Handle;
	
} EFI_VIDEO_CARD;

typedef struct {
	UINT16	Signature;        //Should be 0x55AA
	UINT8   x86Init[22];      //Safe to ignore, x86 boot code of 16h
	UINT16	PciDataStructure; //Relative Pointer to PCIDataStructure
} PCI_OPTION_ROM;

typedef struct {
	UINT32  Signature;        //Should be PCIR (0x52494350)
	UINT16	VendorId;         //PCI Vendor ID for the Device
	UINT16	DeviceId;         //PCI Device ID for the Device
	UINT16	Reserved0;
	UINT16  Length;           //Length of PCI Data Structure
	UINT8   Revision;         //PCIDataStructure Revision should be 0
	UINT8   ClassCode[3];     //PCI Class Code for the Device
	UINT16  ImageLength;      //512byte Units
	UINT16  ImageRevision;    //Revision of the actual software build
	UINT8   CodeType;         //0 Intel x86, PC-AT compatible, 1 Open Firmware standard for PCI48, 2 Hewlett-Packard PA RISC, 3 Extensible Firmware Interface (EFI)
	UINT8   LastImageIndicator; //1 if at the last image in the ROM, otherwise, other images are chained
	UINT16  Reserved1;
} PCI_DATA_STRUCTURE;

typedef struct {
	VOID    *OptionRom;
	UINTN    OptionRomSize;
	UINT16   VendorId;
	UINT16   DeviceId;
	BOOLEAN  VGA;
	BOOLEAN  BIOSCode;
	BOOLEAN  OpenFirmwareCode;
	BOOLEAN  HPPACode;
	BOOLEAN  EFICode;
} EFI_OPTION_ROM;

typedef struct {
	VOID                                      *Protocol;
	EFI_HANDLE                                *ParentHandle;
	BOOLEAN                                    SupportsStdResolutions;
	UINT32                                     MaxX;
	UINT32                                     MaxY;
	UINT32                                     CurrentVirtualX;
	UINT32                                     CurrentVirtualY;
	UINT32                                     CurrentXOffset;
	UINT32                                     CurrentYOffset;
	VOID                                      *VirtualFrameBuffer;
	UINTN                                      CurrentVFBSize;
	EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE    QueryMode;
	EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE      SetMode;
	EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT           Blt;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION      *Mode;
} EFI_GOP_IMPLEMENTATION;

typedef struct {
	UINT32  horizontal;
	UINT32  vertical;
} EFI_RESOLUTIONS_LIST;

typedef struct {
  LIST_ENTRY      Link;
  UINT64          TriggerTime;
  UINT64          Period;
} TIMER_EVENT_INFO;

#define EVENT_SIGNATURE         SIGNATURE_32('e','v','n','t')
typedef struct {
  UINTN                   Signature;
  UINT32                  Type;
  UINT32                  SignalCount;
  ///
  /// Entry if the event is registered to be signalled
  ///
  LIST_ENTRY              SignalLink;
  ///
  /// Notification information for this event
  ///
  EFI_TPL                 NotifyTpl;
  EFI_EVENT_NOTIFY        NotifyFunction;
  VOID                    *NotifyContext;
  EFI_GUID                EventGroup;
  LIST_ENTRY              NotifyLink;
  BOOLEAN                 ExFlag;
  ///
  /// A list of all runtime events
  ///
  EFI_RUNTIME_EVENT_ENTRY RuntimeData;
  TIMER_EVENT_INFO        Timer;
} IEVENT;

///
/// Enumerate the valid types
///
UINT32 mEventTable[] = {
  ///
  /// 0x80000200       Timer event with a notification function that is
  /// queue when the event is signaled with SignalEvent()
  ///
  EVT_TIMER | EVT_NOTIFY_SIGNAL,
  ///
  /// 0x80000000       Timer event without a notification function. It can be
  /// signaled with SignalEvent() and checked with CheckEvent() or WaitForEvent().
  ///
  EVT_TIMER,
  ///
  /// 0x00000100       Generic event with a notification function that
  /// can be waited on with CheckEvent() or WaitForEvent()
  ///
  EVT_NOTIFY_WAIT,
  ///
  /// 0x00000200       Generic event with a notification function that
  /// is queue when the event is signaled with SignalEvent()
  ///
  EVT_NOTIFY_SIGNAL,
  ///
  /// 0x00000201       ExitBootServicesEvent.
  ///
  EVT_SIGNAL_EXIT_BOOT_SERVICES,
  ///
  /// 0x60000202       SetVirtualAddressMapEvent.
  ///
  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,

  ///
  /// 0x00000000       Generic event without a notification function.
  /// It can be signaled with SignalEvent() and checked with CheckEvent()
  /// or WaitForEvent().
  ///
  0x00000000,
  ///
  /// 0x80000100       Timer event with a notification function that can be
  /// waited on with CheckEvent() or WaitForEvent()
  ///
  EVT_TIMER | EVT_NOTIFY_WAIT,
};


#define EFI_2_31_REVISION  ((2 << 16) | (31))
#define EFI_2_30_REVISION  ((2 << 16) | (30))
#define EFI_2_20_REVISION  ((2 << 16) | (20))
#define EFI_2_10_REVISION  ((2 << 16) | (10))
#define EFI_2_00_REVISION  ((2 << 16) | (00))
#define EFI_1_10_REVISION  ((1 << 16) | (10))
#define EFI_1_02_REVISION  ((1 << 16) | (02))

EFI_VIDEO_CARD          *MyVideoCards;
UINTN                    NumberVideoCards;

EFI_OPTION_ROM          *MyOptionRoms;
UINTN                    NumberOptionRoms;

EFI_GOP_IMPLEMENTATION  *MyGOPImplementations;
UINTN                    NumberGOPImplementations;

EFI_RESOLUTIONS_LIST    *MyResolutions;
UINTN                    NumberResolutions;

EFI_BOOT_SERVICES       *BS;
EFI_RUNTIME_SERVICES    *RS;
EFI_SYSTEM_TABLE        *ST;
EFI_DXE_SERVICES_TABLE  *DS;
EFI_STATUS               Status;

//
//Dummy implementation for the missing RuntimeServices
//
EFI_STATUS SHEFI_UPDATE_CAPSULE (IN EFI_CAPSULE_HEADER   **CapsuleHeaderArray,  IN UINTN CapsuleCount,  IN EFI_PHYSICAL_ADDRESS ScatterGatherList OPTIONAL ){
	return EFI_UNSUPPORTED;
}

EFI_STATUS SHEFI_QUERY_CAPSULE_CAPABILITIES ( IN  EFI_CAPSULE_HEADER **CapsuleHeaderArray, IN  UINTN CapsuleCount, OUT UINT64 *MaximumCapsuleSize, OUT EFI_RESET_TYPE *ResetType){
	return EFI_UNSUPPORTED;
}

EFI_STATUS SHEFI_QUERY_VARIABLE_INFO ( IN  UINT32 Attributes, OUT UINT64 *MaximumVariableStorageSize, OUT UINT64 *RemainingVariableStorageSize, OUT UINT64 *MaximumVariableSize){
	*MaximumVariableStorageSize=32768;
	*RemainingVariableStorageSize=16384;
	*MaximumVariableSize=16384;
	return EFI_SUCCESS;
}
/*
EFI_LOCK gEventQueueLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_HIGH_LEVEL);

LIST_ENTRY      gEventSignalQueue = INITIALIZE_LIST_HEAD_VARIABLE (gEventSignalQueue);

EFI_RUNTIME_ARCH_PROTOCOL gRuntimeTemplate = {
  INITIALIZE_LIST_HEAD_VARIABLE (gRuntimeTemplate.ImageHead),
  INITIALIZE_LIST_HEAD_VARIABLE (gRuntimeTemplate.EventHead),

  //
  // Make sure Size != sizeof (EFI_MEMORY_DESCRIPTOR). This will
  // prevent people from having pointer math bugs in their code.
  // now you have to use *DescriptorSize to make things work.
  //
  sizeof (EFI_MEMORY_DESCRIPTOR) + sizeof (UINT64) - (sizeof (EFI_MEMORY_DESCRIPTOR) % sizeof (UINT64)),
  EFI_MEMORY_DESCRIPTOR_VERSION,
  0,
  NULL,
  NULL,
  FALSE,
  FALSE
};

EFI_RUNTIME_ARCH_PROTOCOL *gRuntime = &gRuntimeTemplate;

VOID CoreAcquireLock (IN EFI_LOCK  *Lock){
	if (Lock != NULL) {
		if(Lock->Lock == EfiLockReleased){
			Lock->OwnerTpl = BS->RaiseTPL(Lock->Tpl);
			Lock->Lock     = EfiLockAcquired;
		}
	}
}

VOID CoreReleaseLock (IN EFI_LOCK  *Lock){
	EFI_TPL Tpl;

	if (Lock != NULL) {
		if(Lock->Lock == EfiLockAcquired){
			Tpl = Lock->OwnerTpl;
			Lock->Lock = EfiLockReleased;
			BS->RestoreTPL (Tpl);
		}
	}
}

VOID CoreAcquireEventLock (VOID){
  CoreAcquireLock (&gEventQueueLock);
}

VOID CoreReleaseEventLock (VOID) {
  CoreReleaseLock (&gEventQueueLock);
}

EFI_STATUS SHEFI_CREATE_EVENT_EX ( IN UINT32 Type, IN EFI_TPL NotifyTpl, IN EFI_EVENT_NOTIFY NotifyFunction, OPTIONAL IN CONST VOID *NotifyContext, OPTIONAL IN CONST EFI_GUID *EventGroup, OPTIONAL OUT EFI_EVENT *Event) {

	EFI_GUID gEfiEventExitBootServicesGuid;
	EFI_GUID gEfiEventVirtualAddressChangeGuid;
	EFI_GUID gEfiEventMemoryMapChangeGuid;
	EFI_GUID gEfiEventReadyToBootGuid;

	EFI_STATUS      Status;
	IEVENT          *IEvent;
	INTN            Index;
	
	gEfiEventExitBootServicesGuid.Data1=0x27abf055;
	gEfiEventExitBootServicesGuid.Data2=0xb1b8;
	gEfiEventExitBootServicesGuid.Data3=0x4c26;
	gEfiEventExitBootServicesGuid.Data4[0]=0x80;
	gEfiEventExitBootServicesGuid.Data4[1]=0x48;
	gEfiEventExitBootServicesGuid.Data4[2]=0x74;
	gEfiEventExitBootServicesGuid.Data4[3]=0x8f;
	gEfiEventExitBootServicesGuid.Data4[4]=0x37;
	gEfiEventExitBootServicesGuid.Data4[5]=0xba;
	gEfiEventExitBootServicesGuid.Data4[6]=0xa2;
	gEfiEventExitBootServicesGuid.Data4[7]=0xdf;

	gEfiEventVirtualAddressChangeGuid.Data1=0x13fa7698;
	gEfiEventVirtualAddressChangeGuid.Data2=0xc831;
	gEfiEventVirtualAddressChangeGuid.Data3=0x49c7;
	gEfiEventVirtualAddressChangeGuid.Data4[0]=0x87;
	gEfiEventVirtualAddressChangeGuid.Data4[1]=0xea;
	gEfiEventVirtualAddressChangeGuid.Data4[2]=0x8f;
	gEfiEventVirtualAddressChangeGuid.Data4[3]=0x43;
	gEfiEventVirtualAddressChangeGuid.Data4[4]=0xfc;
	gEfiEventVirtualAddressChangeGuid.Data4[5]=0xc2;
	gEfiEventVirtualAddressChangeGuid.Data4[6]=0x51;
	gEfiEventVirtualAddressChangeGuid.Data4[7]=0x96;

	gEfiEventMemoryMapChangeGuid.Data1=0x78bee926;
	gEfiEventMemoryMapChangeGuid.Data2=0x692f;
	gEfiEventMemoryMapChangeGuid.Data3=0x48fd;
	gEfiEventMemoryMapChangeGuid.Data4[0]=0x9e;
	gEfiEventMemoryMapChangeGuid.Data4[1]=0xdb;
	gEfiEventMemoryMapChangeGuid.Data4[2]=0x01;
	gEfiEventMemoryMapChangeGuid.Data4[3]=0x42;
	gEfiEventMemoryMapChangeGuid.Data4[4]=0x2e;
	gEfiEventMemoryMapChangeGuid.Data4[5]=0xf0;
	gEfiEventMemoryMapChangeGuid.Data4[6]=0xd7;
	gEfiEventMemoryMapChangeGuid.Data4[7]=0xab;

	gEfiEventReadyToBootGuid.Data1=0x7ce88fb3;
	gEfiEventReadyToBootGuid.Data2=0x4bd7;
	gEfiEventReadyToBootGuid.Data3=0x4679;
	gEfiEventReadyToBootGuid.Data4[0]=0x87;
	gEfiEventReadyToBootGuid.Data4[1]=0xa8;
	gEfiEventReadyToBootGuid.Data4[2]=0xa8;
	gEfiEventReadyToBootGuid.Data4[3]=0xd8;
	gEfiEventReadyToBootGuid.Data4[4]=0xde;
	gEfiEventReadyToBootGuid.Data4[5]=0xe5;
	gEfiEventReadyToBootGuid.Data4[6]=0x0d;
	gEfiEventReadyToBootGuid.Data4[7]=0x2b;
	
	if (Event == NULL) {
		return EFI_INVALID_PARAMETER;
	}

	//
	// Check to make sure no reserved flags are set
	//
	Status = EFI_INVALID_PARAMETER;
	for (Index = 0; Index < (sizeof (mEventTable) / sizeof (UINT32)); Index++) {
		if (Type == mEventTable[Index]) {
			Status = EFI_SUCCESS;
			break;
		}
	}
	if(EFI_ERROR (Status)) {
		return EFI_INVALID_PARAMETER;
	}

	//
	// Convert Event type for pre-defined Event groups
	//
	if (EventGroup != NULL) {
		//
		// For event group, type EVT_SIGNAL_EXIT_BOOT_SERVICES and EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE
		// are not valid
		//
		if ((Type == EVT_SIGNAL_EXIT_BOOT_SERVICES) || (Type == EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE)) {
			return EFI_INVALID_PARAMETER;
		}
		if (CompareGuid (EventGroup, &gEfiEventExitBootServicesGuid)) {
			Type = EVT_SIGNAL_EXIT_BOOT_SERVICES;
		} else if (CompareGuid (EventGroup, &gEfiEventVirtualAddressChangeGuid)) {
			Type = EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE;
		}
	} else {
		//
		// Convert EFI 1.10 Events to their UEFI 2.0 CreateEventEx mapping
		//
		if (Type == EVT_SIGNAL_EXIT_BOOT_SERVICES) {
			EventGroup = &gEfiEventExitBootServicesGuid;
		} else if (Type == EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE) {
			EventGroup = &gEfiEventVirtualAddressChangeGuid;
		}
	}

	//
	// If it's a notify type of event, check its parameters
	//
	if ((Type & (EVT_NOTIFY_WAIT | EVT_NOTIFY_SIGNAL)) != 0) {
		//
		// Check for an invalid NotifyFunction or NotifyTpl
		//
		if ((NotifyFunction == NULL) ||
		(NotifyTpl <= TPL_APPLICATION) ||
		(NotifyTpl >= TPL_HIGH_LEVEL)) {
			return EFI_INVALID_PARAMETER;
		}

	} else {
		//
		// No notification needed, zero ignored values
		//
		NotifyTpl = 0;
		NotifyFunction = NULL;
		NotifyContext = NULL;
	}

	//
	// Allocate and initialize a new event structure.
	//
	if ((Type & EVT_RUNTIME) != 0) {
		IEvent = AllocateRuntimeZeroPool (sizeof (IEVENT));
	} else {
		IEvent = AllocateZeroPool (sizeof (IEVENT));
	}
	if (IEvent == NULL) {
		return EFI_OUT_OF_RESOURCES;
	}

	IEvent->Signature = EVENT_SIGNATURE;
	IEvent->Type = Type;

	IEvent->NotifyTpl      = NotifyTpl;
	IEvent->NotifyFunction = NotifyFunction;
	IEvent->NotifyContext  = (VOID *)NotifyContext;
	if (EventGroup != NULL) {
		CopyGuid (&IEvent->EventGroup, EventGroup);
		IEvent->ExFlag = TRUE;
	}

	*Event = IEvent;

	if ((Type & EVT_RUNTIME) != 0) {
		//
		// Keep a list of all RT events so we can tell the RT AP.
		//
		IEvent->RuntimeData.Type           = Type;
		IEvent->RuntimeData.NotifyTpl      = NotifyTpl;
		IEvent->RuntimeData.NotifyFunction = NotifyFunction;
		IEvent->RuntimeData.NotifyContext  = (VOID *) NotifyContext;
		IEvent->RuntimeData.Event          = (EFI_EVENT *) IEvent;
		InsertTailList (&gRuntime->EventHead, &IEvent->RuntimeData.Link);
	}
	//gRuntime
	CoreAcquireEventLock ();

	if ((Type & EVT_NOTIFY_SIGNAL) != 0x00000000) {
		//
		// The Event's NotifyFunction must be queued whenever the event is signaled
		//
		InsertHeadList (&gEventSignalQueue, &IEvent->SignalLink);
	}

	CoreReleaseEventLock ();

	//
	// Done
	//
	return EFI_SUCCESS;
}
*/
//
//Print out the current table informations.
//Also change the table versions to $revision.$version.
//TODO: Protect the actual UpdateCapsule, QueryCapsuleCapabilities
//      and QueryVariableInfo executable code as EfiRuntimeServicesCode
//

EFI_STATUS EfiSetVersion(IN UINT32 revision, IN UINT32 version) {
	
	EFI_RUNTIME_SERVICES     *NewRS;
//	EFI_BOOT_SERVICES        *NewBS;
	
	
	switch(ST->Hdr.Revision) {
		case EFI_1_02_REVISION:
			Print(L"SystemTable Version: 1.02\n");
			break;
		case EFI_1_10_REVISION:
			Print(L"SystemTable Version: 1.10\n");
			break;
		case EFI_2_00_REVISION:
			Print(L"SystemTable Version: 2.00\n");
			break;
		case EFI_2_10_REVISION:
			Print(L"SystemTable Version: 2.10\n");
			break;
		case EFI_2_20_REVISION:
			Print(L"SystemTable Version: 2.20\n");
			break;
		case EFI_2_30_REVISION:
			Print(L"SystemTable Version: 2.30\n");
			break;
		case EFI_2_31_REVISION:
			Print(L"SystemTable Version: 2.31\n");
			break;
		default:
			Print(L"SystemTable Version: Unknown: %x\n", ST->Hdr.Revision);
			break;
	}
	
	switch(BS->Hdr.Revision) {
		case EFI_1_02_REVISION:
			Print(L"BootServices Version: 1.02\n");
			break;
		case EFI_1_10_REVISION:
			Print(L"BootServices Version: 1.10\n");
			break;
		case EFI_2_00_REVISION:
			Print(L"BootServices Version: 2.00\n");
			break;
		case EFI_2_10_REVISION:
			Print(L"BootServices Version: 2.10\n");
			break;
		case EFI_2_20_REVISION:
			Print(L"BootServices Version: 2.20\n");
			break;
		case EFI_2_30_REVISION:
			Print(L"BootServices Version: 2.30\n");
			break;
		case EFI_2_31_REVISION:
			Print(L"BootServices Version: 2.31\n");
			break;
		default:
			Print(L"BootServices Version: Unknown: %x\n", BS->Hdr.Revision);
			break;
	}
	
	switch(RS->Hdr.Revision) {
		case EFI_1_02_REVISION:
			Print(L"RuntimeServices Version: 1.02\n");
			break;
		case EFI_1_10_REVISION:
			Print(L"RuntimeServices Version: 1.10\n");
			break;
		case EFI_2_00_REVISION:
			Print(L"RuntimeServices Version: 2.00\n");
			break;
		case EFI_2_10_REVISION:
			Print(L"RuntimeServices Version: 2.10\n");
			break;
		case EFI_2_20_REVISION:
			Print(L"RuntimeServices Version: 2.20\n");
			break;
		case EFI_2_30_REVISION:
			Print(L"RuntimeServices Version: 2.30\n");
			break;
		case EFI_2_31_REVISION:
			Print(L"RuntimeServices Version: 2.31\n");
			break;
		default:
			Print(L"RuntimeServices Version: Unknown: %x\n", RS->Hdr.Revision);
			break;
	}
	
	switch(DS->Hdr.Revision) {
		case EFI_1_02_REVISION:
			Print(L"DXEServices Version: 1.02\n");
			break;
		case EFI_1_10_REVISION:
			Print(L"DXEServices Version: 1.10\n");
			break;
		case EFI_2_00_REVISION:
			Print(L"DXEServices Version: 2.00\n");
			break;
		case EFI_2_10_REVISION:
			Print(L"DXEServices Version: 2.10\n");
			break;
		case EFI_2_20_REVISION:
			Print(L"DXEServices Version: 2.20\n");
			break;
		case EFI_2_30_REVISION:
			Print(L"DXEServices Version: 2.30\n");
			break;
		case EFI_2_31_REVISION:
			Print(L"DXEServices Version: 2.31\n");
			break;
		default:
			Print(L"DXEServices Version: Unknown: %x\n", DS->Hdr.Revision);
			break;
	}
	
	Print(L"DEBUG Revision: ST: %x, BS: %x, RS: %x, DS: %x\n", ST->Hdr.Revision, BS->Hdr.Revision, RS->Hdr.Revision, DS->Hdr.Revision);
	
	//Change EFI Version to 2.10 or whatever you want it to say
	ST->Hdr.Revision=((revision << 16) | version);
	//BS->Hdr.Revision=((revision << 16) | version);
	RS->Hdr.Revision=((revision << 16) | version);
	DS->Hdr.Revision=((revision << 16) | version);

	/*
	Status=BS->AllocatePool(EfiBootServicesCode, sizeof(EFI_BOOT_SERVICES), &NewBS);
	NewBS->Hdr.Signature                         = BS->Hdr.Signature;
	NewBS->Hdr.Revision                          = BS->Hdr.Revision;
	NewBS->Hdr.HeaderSize                        = sizeof(EFI_BOOT_SERVICES);
	NewBS->Hdr.CRC32                             = BS->Hdr.CRC32;
	NewBS->Hdr.Reserved                          = BS->Hdr.Reserved;
	NewBS->RaiseTPL                              = BS->RaiseTPL;
	NewBS->RestoreTPL                            = BS->RestoreTPL; 
	NewBS->AllocatePages                         = BS->AllocatePages; 
	NewBS->FreePages                             = BS->FreePages; 
	NewBS->GetMemoryMap                          = BS->GetMemoryMap; 
	NewBS->AllocatePool                          = BS->AllocatePool; 
	NewBS->FreePool                              = BS->FreePool; 
	NewBS->CreateEvent                           = BS->CreateEvent; 
	NewBS->SetTimer                              = BS->SetTimer; 
	NewBS->WaitForEvent                          = BS->WaitForEvent; 
	NewBS->SignalEvent                           = BS->SignalEvent; 
	NewBS->CloseEvent                            = BS->CloseEvent; 
	NewBS->CheckEvent                            = BS->CheckEvent; 
	NewBS->InstallProtocolInterface              = BS->InstallProtocolInterface; 
	NewBS->ReinstallProtocolInterface            = BS->ReinstallProtocolInterface; 
	NewBS->UninstallProtocolInterface            = BS->UninstallProtocolInterface; 
	NewBS->HandleProtocol                        = BS->HandleProtocol; 
	NewBS->Reserved                              = BS->Reserved; 
	NewBS->RegisterProtocolNotify                = BS->RegisterProtocolNotify; 
	NewBS->LocateHandle                          = BS->LocateHandle; 
	NewBS->LocateDevicePath                      = BS->LocateDevicePath; 
	NewBS->InstallConfigurationTable             = BS->InstallConfigurationTable; 
	NewBS->LoadImage                             = BS->LoadImage; 
	NewBS->StartImage                            = BS->StartImage; 
	NewBS->Exit                                  = BS->Exit; 
	NewBS->UnloadImage                           = BS->UnloadImage; 
	NewBS->ExitBootServices                      = BS->ExitBootServices; 
	NewBS->GetNextMonotonicCount                 = BS->GetNextMonotonicCount; 
	NewBS->Stall                                 = BS->Stall; 
	NewBS->SetWatchdogTimer                      = BS->SetWatchdogTimer; 
	NewBS->ConnectController                     = BS->ConnectController; 
	NewBS->DisconnectController                  = BS->DisconnectController;
	NewBS->OpenProtocol                          = BS->OpenProtocol; 
	NewBS->CloseProtocol                         = BS->CloseProtocol; 
	NewBS->OpenProtocolInformation               = BS->OpenProtocolInformation; 
	NewBS->ProtocolsPerHandle                    = BS->ProtocolsPerHandle; 
	NewBS->LocateHandleBuffer                    = BS->LocateHandleBuffer; 
	NewBS->LocateProtocol                        = BS->LocateProtocol; 
	NewBS->InstallMultipleProtocolInterfaces     = BS->InstallMultipleProtocolInterfaces; 
	NewBS->UninstallMultipleProtocolInterfaces   = BS->UninstallMultipleProtocolInterfaces; 
	NewBS->CalculateCrc32                        = BS->CalculateCrc32; 
	NewBS->CopyMem                               = BS->CopyMem; 
	NewBS->SetMem                                = BS->SetMem;
	NewBS->CreateEventEx                         = SHEFI_CREATE_EVENT_EX;
	
	ST->BootServices=NewBS;
	
	Status=BS->FreePool(BS);
	BS=NewBS;
	*/
	
	Status=BS->AllocatePool(EfiRuntimeServicesCode, sizeof(EFI_RUNTIME_SERVICES), &NewRS);
	NewRS->Hdr.Signature             = RS->Hdr.Signature;
	NewRS->Hdr.Revision              = RS->Hdr.Revision;
	NewRS->Hdr.HeaderSize            = sizeof(EFI_RUNTIME_SERVICES);
	NewRS->Hdr.CRC32                 = RS->Hdr.CRC32;
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
	NewRS->UpdateCapsule             = SHEFI_UPDATE_CAPSULE;
	NewRS->QueryCapsuleCapabilities  = SHEFI_QUERY_CAPSULE_CAPABILITIES;
	NewRS->QueryVariableInfo         = SHEFI_QUERY_VARIABLE_INFO;
	
	ST->RuntimeServices=NewRS;
	
	Status=BS->FreePool(RS);
	RS=NewRS;
	
	Print(L"DEBUG Revision: ST: %x, BS: %x, RS: %x, DS: %x\n\n", ST->Hdr.Revision, BS->Hdr.Revision, RS->Hdr.Revision, DS->Hdr.Revision);
	
	Print(L"DEBUG CRC32: ST: %08x, BS: %08x, RS: %08x, DS: %08x\n", ST->Hdr.CRC32, BS->Hdr.CRC32, RS->Hdr.CRC32, DS->Hdr.CRC32);
	
	//Change the CRC32 Value to 0 in order to recalculate it
	ST->Hdr.CRC32=0;
	BS->Hdr.CRC32=0;
	RS->Hdr.CRC32=0;
	DS->Hdr.CRC32=0;
	Print(L"DEBUG CRC32: ST: %08x, BS: %08x, RS: %08x, DS: %08x\n", ST->Hdr.CRC32, BS->Hdr.CRC32, RS->Hdr.CRC32, DS->Hdr.CRC32);
		
	//Calculate the new CRC32 Value
	Status=BS->CalculateCrc32 (ST, ST->Hdr.HeaderSize, (VOID *) &ST->Hdr.CRC32);
	Status=BS->CalculateCrc32 (BS, BS->Hdr.HeaderSize, (VOID *) &BS->Hdr.CRC32);
	Status=BS->CalculateCrc32 (DS, DS->Hdr.HeaderSize, (VOID *) &DS->Hdr.CRC32);
	Status=BS->CalculateCrc32 (RS, RS->Hdr.HeaderSize, (VOID *) &RS->Hdr.CRC32);
	
	Print(L"DEBUG CRC32: ST: %08x, BS: %08x, RS: %08x, DS: %08x\n\n", ST->Hdr.CRC32, BS->Hdr.CRC32, RS->Hdr.CRC32, DS->Hdr.CRC32);
	Print(L"DEBUG Size Actual  : ST: %x, BS: %x, RS: %x, DS: %x\n", ST->Hdr.HeaderSize, BS->Hdr.HeaderSize, RS->Hdr.HeaderSize, DS->Hdr.HeaderSize);
	Print(L"DEBUG Size Expected: ST: %x, BS: %x, RS: %x, DS: %x\n\n", sizeof(EFI_SYSTEM_TABLE), sizeof(EFI_BOOT_SERVICES), sizeof(EFI_RUNTIME_SERVICES), sizeof(EFI_DXE_SERVICES));
	
	return EFI_SUCCESS;
	
}

//
//Simply shadows an optionRom and sets the correct int10h value.
//Only to be called by EfiWriteActiveGraphicsAdaptorPCIRegs
//TODO: Implement method to unlock the RAM at 0xC0000.
//TODO: Get the int10h values and ROMs from the vector that
//      EfiGetOpRomInformation builds.
//
EFI_STATUS EfiShadowVGAOptionROM (IN EFI_VIDEO_CARD *MyVideoCard, IN EFI_HANDLE ImageHandle) {
	
	VOID                       *ROM_Addr;             //Pointer to the VGA ROM Shadow location
	UINT32                      Int10h_Addr;          //Pointer to the int10h Software Interrupt Handler pointer
	UINT32                      Int10h_Value;         //The Value of the int10h Software Interrupt Handler Vector
	EFI_LEGACY_REGION_PROTOCOL *LegacyRegionProtocol;
	EFI_PCI_IO_PROTOCOL        *IoDev;
	UINT32                      Granularity;
	
	ROM_Addr         = (VOID *)(UINTN)0xC0000;
	Int10h_Addr      = 0x40;
//	Int10h_Value     = 0xC00017C9;//MBP6,2
	Int10h_Value     = 0xC000F21B;//MBP5,3
	
	//
	//Unlock the memory for the shadow space
	//
	Status=BS->LocateProtocol(&gEfiLegacyRegionProtocolGuid, NULL, &LegacyRegionProtocol);
	if(EFI_ERROR(Status)) { LegacyRegionProtocol=NULL;}

	//
	//Open the EFI_PCI_IO_PROTOCOL
	//
	Status = BS->OpenProtocol (MyVideoCard->Handle, &gEfiPciIoProtocolGuid, (VOID *) &IoDev, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
	if (EFI_ERROR (Status)) { Print(L"EfiShadowVGAOptionROM: Error getting the PciIo Protocol instance: %r\n", Status); return Status; };
	
	if (LegacyRegionProtocol != NULL) {
		Status=LegacyRegionProtocol->UnLock(LegacyRegionProtocol, 0xC0000, (UINT32) IoDev->RomSize, &Granularity);
		if (EFI_ERROR (Status)) { Print(L"EfiShadowVGAOptionROM: Could not lock the LegacyRegion", Status); };
	}
	
	//
	//Copy the option ROM to the shadow space
	//
	BS->CopyMem(ROM_Addr, IoDev->RomImage, IoDev->RomSize);
	
	//
	//Close the EFI_PCI_IO_PROTOCOL
	//
	Status = BS->CloseProtocol (MyVideoCard->Handle, &gEfiPciIoProtocolGuid, ImageHandle, NULL);
	if (EFI_ERROR (Status)) { Print(L"EfiShadowVGAOptionROM: Error closing the PciIo Protocol instance: %r\n", Status); return Status; };
	
	if (LegacyRegionProtocol != NULL) {
		Status=LegacyRegionProtocol->Lock(LegacyRegionProtocol, 0xC0000, (UINT32) IoDev->RomSize, &Granularity);
		if (EFI_ERROR (Status)) { Print(L"EfiShadowVGAOptionROM: Could not lock the LegacyRegion", Status); };
	}

	//
	//Set the Int10h Pointer
	//
	*(UINT32 *)(UINTN)Int10h_Addr=*(UINT32 *)&Int10h_Value;
	
	return EFI_SUCCESS;
	
}

//
//Currently a hack that works on Query the NVRAM for predefined value that
//points to the Bootcamp Video Card.
//TODO: Get the correct video card from NVRAM.
//TODO: Start EfiShadowVGAOptionROM.
//TODO: Set the gMux to the video card in the NVRAM.
//
EFI_STATUS EfiWriteActiveGraphicsAdaptorPCIRegs(IN EFI_HANDLE ImageHandle) {
	
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
	
	/*PointerSize=sizeof(UINTN);
	LegacyVgaHandleValue=0;
	
	Status = RS->GetVariable (L"LEGACYVGAHANDLE", &gEfiGlobalVariableGuid, NULL, &PointerSize, (VOID *) &LegacyVgaHandleValue);
	if (EFI_ERROR (Status)) { Print(L"Could not read the LEGACYVGAHANDLE NVRAM variable: %r\n", Status); return Status; };
	
	LegacyVgaHandle=(VOID *)LegacyVgaHandleValue;
	Status=EfiShadowVGAOptionROM (IoDev);
	if (EFI_ERROR (Status)) { Print(L"Couldn't shadow the Option ROM: %r\n", Status);};
	
	Print(L"Shadowed the Option ROM\n");
	Status = BS->CloseProtocol (LegacyVgaHandle, &gEfiPciIoProtocolGuid, ImageHandle, NULL);*/
	
	for (i=0; i<NumberVideoCards; i++) {
	
		Status = BS->OpenProtocol (MyVideoCards[i].Handle, &gEfiPciIoProtocolGuid, (VOID *) &IoDev, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
		if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs: Error getting the PciIo Protocol instance: %r\n", Status); return Status; };
		
		Status = IoDev->GetLocation(IoDev, &SegmentNumber, &BusNumber, &DevNumber, &FuncNumber);
		if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs: Could not locate the PCI Device: %r\n", Status); return Status; };
		
		if (MyVideoCards[i].IsMainVideoCard==TRUE) {
		
			Status=IoDev->Attributes (IoDev, EfiPciIoAttributeOperationEnable,  EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY | EFI_PCI_IO_ATTRIBUTE_IO | EFI_PCI_IO_ATTRIBUTE_BUS_MASTER, NULL);
			if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs:   PCI VGA Adapter on %02x:%02x.%02x: Couldn't set Attributes: %r\n", BusNumber, DevNumber, FuncNumber, Status); };
			
			Status=EfiShadowVGAOptionROM(&MyVideoCards[i], ImageHandle);
			if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs:   PCI VGA Adapter on %02x:%02x.%02x: EfiShadowVGAOptionROM gave an error: %r\n", BusNumber, DevNumber, FuncNumber, Status);};
			
		} else {
			
			Status=IoDev->Attributes (IoDev, EfiPciIoAttributeOperationDisable, EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY | EFI_PCI_IO_ATTRIBUTE_IO | EFI_PCI_IO_ATTRIBUTE_BUS_MASTER, NULL);
			if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs:   PCI VGA Adapter on %02x:%02x.%02x: Couldn't disable Attributes: %r\n", BusNumber, DevNumber, FuncNumber, Status); };
			
		};
		
		Status = BS->CloseProtocol (MyVideoCards[i].Handle, &gEfiPciIoProtocolGuid, ImageHandle, NULL);
		if (EFI_ERROR (Status)) { Print(L"EfiWriteActiveGraphicsAdaptorPCIRegs:   PCI VGA Adapter on %02x:%02x.%02x: Error closing the PciIo Protocol instance: %r\n", Status); return Status; };
		
	}

	return EFI_SUCCESS;
	
}


//
// Make a list of all the video cards.
// and all their information. Mostly done!
//
EFI_STATUS EfiPopulateVgaVectors(IN EFI_HANDLE ImageHandle) {
	
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
	LegacyVgaHandleValue=0;
	Segment          = 0;
	Bus              = 0;
	Device           = 0;
	Function         = 0;
	NumberVideoCards = 0;
	
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
	MyVideoCards = AllocateZeroPool(sizeof (EFI_VIDEO_CARD) * NumberVideoCards);
	
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
			
			if ((*IoDevAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_IO) > 0)     { MyVideoCards[NumberVideoCards].AttributeVgaIo     = TRUE; }
			if ((*IoDevAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY) > 0) { MyVideoCards[NumberVideoCards].AttributeVgaMemory = TRUE; }
			if ((*IoDevAttributes & EFI_PCI_IO_ATTRIBUTE_IO) > 0)         { MyVideoCards[NumberVideoCards].AttributeIo        = TRUE; }
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
				if (EFI_ERROR (Status)) { Print(L"WritePciReg: Couldn't open DevicePath: %r\n", Status); return Status;};
				
				DevicePathHandle=NULL;
				//See if any of the children handles (based on the device path) have the GraphicsOutput Protocol
				Status = BS->LocateDevicePath(&gEfiGraphicsOutputProtocolGuid, &DevicePath, DevicePathHandle);
				
				if (EFI_ERROR (Status)) {
					Print(L"WritePciReg:   PCI VGA Adapter on %02x:%02x.%02x: LocateDevicePath Error: %r\n", Bus, Device, Function, Status);
					MyVideoCards[NumberVideoCards].IsActiveVideoCard = FALSE; //Pointless, but helps with the reading of the code
				}; 
				
				Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, ImageHandle, NULL);
				
			} else {
				
				MyVideoCards[NumberVideoCards].IsActiveVideoCard = TRUE;
				
			}; //if Status(BS->OpenProtocol)
			
			NumberVideoCards++;
			
		} else {
			
			//Print(L"WritePciReg: Other PCI Adapter on %02x:%02x.%02x: VendorID: %04x, DeviceID: %04x, Class: %02x.%02x.%02x\n", Bus, Device, Function, PciHeader.VendorId, PciHeader.DeviceId, PciHeader.ClassCode[2], PciHeader.ClassCode[1], PciHeader.ClassCode[0]);
			
		}; //If (PciHeader.ClassCode)
		
		Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, ImageHandle, NULL);
	}
	
	FreePool(HandleBuffer);
	
	return EFI_SUCCESS;
}

//
//For debugging only
//
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
		
		Status = BS->HandleProtocol(MyVideoCards[Index].Handle, &gEfiDevicePathProtocolGuid, &DevicePath);
		Print(L"VGA%d %02x:%02x.%02x: DevicePath:              %s\n", Index, MyVideoCards[Index].Bus, MyVideoCards[Index].Device, MyVideoCards[Index].Function, ConvertDevicePathToText(DevicePath, FALSE, FALSE));

		Print(L"\n");
		
	}
	
	return EFI_SUCCESS;
}


//
//Required for horizontally parsing the Option Rom Firmware files
//
/*
EFI_STATUS EfiParseSectionStream(IN VOID *Stream, IN UINTN StreamSize, OUT VOID *OptionRom, OUT UINTN *OptionRomSize){
	
	EFI_COMMON_SECTION_HEADER                *SectionType;
	UINTN                                     SectionSize;
	
	Status=EFI_SUCCESS;
	
	while(TRUE){
		
		SectionType=Stream;
		SectionSize=SECTION_SIZE(SectionType);
		Status=EfiParseSection(Stream, SectionSize, *OptionRom, *OptionRomSize)
		
		if(SectionSize<StreamSize){ //If there's another section
			Stream=Stream+SectionSize;
			StreamSize=StreamSize-SectionSize;
		} else { break; }
	}
	
	return(Status);
};

*/


//
//Required for vertically parsing the Option Rom Firmware files
//TODO: Might help if we also try using the Decompress protocol
//      instead of the TianoDecompress Library. Done. Might work.
//TODO: Optionally, try to also parse horizontally.
//
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
			//Print(L"> EFI_SECTION_COMPRESSION ");
			
			CompressedStream=(EFI_COMPRESSION_SECTION *) Stream;
			
			Status=BS->LocateProtocol(&gEfiTianoDecompressProtocolGuid, NULL, (VOID **)&Decompress);
			
			Status=Decompress->GetInfo(Decompress, CompressedStream + 1, (UINT32)StreamSize, &UncompressedLength, &ScratchSize);
			
//			Status=TianoGetInfo(CompressedStream + 1, (UINT32)StreamSize, &UncompressedLength, &ScratchSize);
			
			if ((CompressedStream->UncompressedLength==UncompressedLength)&(CompressedStream->CompressionType==01)){
				
				ScratchBuffer=AllocateZeroPool(ScratchSize);
				DecompressBuffer=AllocateZeroPool(UncompressedLength);
				
				Status=Status=Decompress->Decompress(Decompress, CompressedStream + 1, (UINT32)StreamSize, DecompressBuffer, UncompressedLength, ScratchBuffer, ScratchSize);
//				Status=TianoDecompress(CompressedStream + 1, (UINT32)StreamSize, DecompressBuffer, UncompressedLength, ScratchBuffer, ScratchSize);
				if (EFI_ERROR(Status)) { Print (L"Decompress->Decompress Status: %r", Status); };
				
				SubsectionSize=UncompressedLength;
				Status=EfiParseSection(DecompressBuffer, SubsectionSize, OptionRom, OptionRomSize);
				
				FreePool(ScratchBuffer);
				FreePool(DecompressBuffer);
				
			} else {
				Print(L"Decompressor error: could not decompress: CS->UL: %d, UL: %d, CS-CType: %d\n", CompressedStream->UncompressedLength, UncompressedLength, CompressedStream->CompressionType);
				return EFI_INVALID_PARAMETER;
			}
			break;//EFI_SECTION_COMPRESSION
			
		case EFI_SECTION_GUID_DEFINED:
			//Print(L"> EFI_SECTION_GUID_DEFINED ");
			
			GuidDefinedStream=(VOID *)(UINTN)SectionType;
			Status=BS->LocateProtocol(&GuidDefinedStream->SectionDefinitionGuid, NULL, (VOID **)&GuidedExtraction);
			if(!EFI_ERROR(Status)){
				Status=GuidedExtraction->ExtractSection(GuidedExtraction, GuidDefinedStream, &Subsection, &SubsectionSize, &AuthenticationStatus);
				if(EFI_ERROR(Status)){ Print(L"Error during GuidedExtraction: %r ", Status);};
				Status=EfiParseSection(Subsection, SubsectionSize, OptionRom, OptionRomSize);
				//Print(L"< Guid: OptionRom %8x ", OptionRom);
			};
			break;//EFI_SECTION_GUID_DEFINED
			
		case EFI_SECTION_RAW:
			//Print(L"> EFI_SECTION_RAW ");
			
			*OptionRomSize=StreamSize-sizeof(EFI_COMMON_SECTION_HEADER);
			
			*OptionRom=AllocatePool(*OptionRomSize);
			
			BS->CopyMem(*OptionRom, (VOID *)(UINTN)((UINTN)Stream + sizeof(EFI_COMMON_SECTION_HEADER)), *OptionRomSize);
			
			//Print(L"Stream: %8x OptionRom: %8x StreamSize: %d OptionRomSize: %d ", Stream, OptionRom, StreamSize, *OptionRomSize);
			break;//EFI_SECTION_RAW
			
		default:
			Print(L"Could not parse the section");
			return(EFI_INVALID_PARAMETER);
			break;
	}
	
	return(EFI_SUCCESS);
}


//
//Adds a loaded Option ROM to the Option ROM Vector.
//It requires the validation of the OptionROM signatures.
//If it's not a valid Option ROM, it frees the memory and
//returns EFI_UNSUPPORTED.
//
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
				MyOptionRoms=AllocatePool(sizeof(EFI_OPTION_ROM));
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
				
				MyNewOptionRoms=AllocatePool(sizeof(EFI_OPTION_ROM)*(NumberOptionRoms+1));

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
			
			FreePool(MyOptionRoms);
			MyOptionRoms=NULL;
			MyOptionRoms=MyNewOptionRoms;
			
			NumberOptionRoms++;
			return EFI_SUCCESS;
		}//If we've located the data structure.
		
	}//If the inner part of the section is an option Rom.
	
	FreePool(OptionRom);
	return EFI_UNSUPPORTED;	
}


//
//
//Currently explores the contents of all the firmware volumes.
//Reads the files and checks for sections. It tries to identify
//the sections and print them out with descriptive information.
//
//
EFI_STATUS EfiGetFirmwareOpRomInformation(IN EFI_HANDLE ImageHandle) {
	
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
	
	Print(L"Found %d Firmware Volumes: %r\n", BufferSize, Status);
	
	//You don't work with the firmware unless you raise the task
	//priority level to NOTIFY or above. We'll restore
	//the TPL at the end of the execution.
	PreviousPriority=BS->RaiseTPL(TPL_NOTIFY);
	
	//Let's take the firmware volumes one by one.
	for (Index = 0; Index < BufferSize; Index++) {
		
		//And open them
		Status = BS->OpenProtocol ( HandleBuffer[Index], &gEfiFirmwareVolumeProtocolGuid, (VOID *) &FirmwareVolume, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
		if (EFI_ERROR (Status)) {  Print(L"Could not open firmware Volume number %d: %r\n", Index, Status); continue; }
		
		//Allocate the search index buffer to the one
		//specified by the Firmware Volume
		Key=AllocatePool(FirmwareVolume->KeySize);
		if (Key != NULL) { ZeroMem(Key, FirmwareVolume->KeySize); }
		
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
							
				File = AllocateZeroPool(FileSize);
				
				//Read the file to a buffer (*File)
				Status = FirmwareVolume->ReadFile(FirmwareVolume, &FileGuid, &File, &FileSize, &FileType, &FileAttributes, &AuthenticationStatus);
				if (EFI_ERROR(Status)) { Print(L"FirmwareVolume%d: File%03d: Error while reading file: %r\n", Index, FileCount, Status); };
								
				SectionType = File;
				
				if((SectionType->Type==EFI_SECTION_RAW)|(SectionType->Type==EFI_SECTION_COMPRESSION)|(SectionType->Type==EFI_SECTION_GUID_DEFINED)) {
				
					OptionRomSize=0;
					
					//Use EfiParseSectionStream for horizontal parsing. It's known to be broken.
					Status=EfiParseSection(SectionType, FileSize, &OptionRom, &OptionRomSize);
					
					//It's OK, since we're not de-allocating the sections.
					Status=EfiAddOptionRomToVector(OptionRom, OptionRomSize);
					
				} //If it's a known sectionType.
				
				FreePool(File);
				
			}; //if filetype
			FileCount++;
			
		}//While (!FilesDone)
		
		Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiFirmwareVolumeProtocolGuid, ImageHandle, NULL);
		FreePool(Key);
		
	};//For each firmware volume
	
	FreePool(HandleBuffer);
	
	//Restore Task Priority Level since we won't touch the Firmware anymore.
	BS->RestoreTPL(PreviousPriority);
	
	return EFI_SUCCESS;
	
}


//
//Same as above, but with the Option ROMs already loaded by the VGA Cards
//
EFI_STATUS EfiGetPciOpRomInformation() {
	UINTN     i;
	
	for (i=0; i<NumberVideoCards; i++){
		if (MyVideoCards[i].RomLocation != NULL) {
			EfiAddOptionRomToVector(MyVideoCards[i].RomLocation, MyVideoCards[i].RomSize);
		}
	}
	return EFI_SUCCESS;
}


//
//Same as above, but with the Option ROMs found in EFI\\ShEFI
//on each EFI System Partition of the system.
//
EFI_STATUS EfiGetFileSystemOpRomInformation(IN EFI_HANDLE ImageHandle){
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
	UINT64                          *FileSize;
	
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
		Status=FileSystem->Open(FileSystem, &Directory, (VOID *)"\\EFI\\ShEFI", EFI_FILE_MODE_READ, 0);
		if (EFI_ERROR(Status)) { Directory=NULL; continue;};
		
		if (Directory!=NULL){
		
			DirEntryInfo=NULL;
			DirEntrySize=0;
		
			Status=Directory->GetInfo(Directory, &gEfiFileInfoGuid, &DirEntrySize, DirEntryInfo);
			DirEntryInfo=AllocatePool(DirEntrySize);
			Status=Directory->GetInfo(Directory, &gEfiFileInfoGuid, &DirEntrySize, DirEntryInfo);
			
			if (DirEntryInfo->Attribute & EFI_FILE_DIRECTORY) {
				FreePool(DirEntryInfo);
				DirEntrySize=0;
			
				while(TRUE){//For each entry
					Status=Directory->Read(Directory, &DirEntrySize, DirEntryInfo);
					
					if (DirEntrySize==0) { break; }//We're at the end of the directory;
					
					DirEntryInfo=AllocatePool(DirEntrySize);
					Status=Directory->Read(Directory, &DirEntrySize, DirEntryInfo);
					
					if((!(DirEntryInfo->Attribute & EFI_FILE_DIRECTORY))&(DirEntryInfo->FileSize<=0x100000)){//If the directory entry it isn't a directory and it's smaller than 1MByte
						
						Status=Directory->Open(Directory, &File, DirEntryInfo->FileName, EFI_FILE_MODE_READ, 0);
						
						OptionRom=AllocatePool(DirEntryInfo->FileSize);
						
						FileSize=AllocatePool(sizeof(UINT64));
						*FileSize=DirEntryInfo->FileSize;
						
						Status=File->Read(File, FileSize, OptionRom);
						
						Status=EfiAddOptionRomToVector(OptionRom, DirEntryInfo->FileSize);
						FreePool(FileSize);
						FreePool(File);
					}//If the directory entry it isn't a directory
					
				}//For each entry
			
			} //If it's a directory
			FreePool(Directory);
		} //If the \\EFI\\ShEfi File exists
		
		FreePool(FileSystem);
		Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiSimpleFileSystemProtocolGuid, ImageHandle, NULL);//This also frees the Volume variable;
		
	}//for each filesystem
	
	//attempt to read all the files in EFI\\ShEFI
	
	FreePool (HandleBuffer);
	return EFI_SUCCESS;
}

//
//This OverrideFunction Screws a bit with the
//GraphicsOutputProtocol and is used to hijack
//the GOP implementation. It returns the current
//Resolution as well as 1024x768, 800x600 and
//640x480.
//
EFI_STATUS EfiOverrideGraphicsOutputQueryMode(IN  EFI_GRAPHICS_OUTPUT_PROTOCOL *This, IN  UINT32 ModeNumber, OUT UINTN *SizeOfInfo, OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info){

	UINTN                                 MyRealGopIndex;
	UINTN                                 Index;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *CurrentMode;

	//Find out for which GOP instance we're overriding.
	for(Index=0; Index<NumberGOPImplementations; Index++){
		if(This==MyGOPImplementations[Index].Protocol){
			MyRealGopIndex=Index;
			break;
		}
	}
	
	CurrentMode=NULL;
	if (ModeNumber==0){
		*SizeOfInfo=NumberResolutions;
		*Info=AllocatePool(sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION)*NumberResolutions);
		for(Index=0; Index<NumberResolutions; Index++){
			CurrentMode=*Info;
			CurrentMode->Version=0;
			CurrentMode->HorizontalResolution=MyResolutions[Index].horizontal;
			CurrentMode->VerticalResolution=MyResolutions[Index].vertical;
			CurrentMode->PixelFormat=PixelBltOnly;
			CurrentMode->PixelInformation.RedMask=MyGOPImplementations[Index].Mode->PixelInformation.RedMask;
			CurrentMode->PixelInformation.GreenMask=MyGOPImplementations[Index].Mode->PixelInformation.GreenMask;
			CurrentMode->PixelInformation.BlueMask=MyGOPImplementations[Index].Mode->PixelInformation.BlueMask;
			CurrentMode->PixelInformation.ReservedMask=MyGOPImplementations[Index].Mode->PixelInformation.ReservedMask;
			CurrentMode->PixelsPerScanLine=MyGOPImplementations[Index].Mode->PixelsPerScanLine;
			CurrentMode=NULL;
		}
		
	} else {
		*SizeOfInfo=1;
		*Info=AllocatePool(sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
		CurrentMode=*Info;
		CurrentMode->Version=0;
		CurrentMode->HorizontalResolution=MyGOPImplementations[Index].CurrentVirtualX;
		CurrentMode->VerticalResolution=MyGOPImplementations[Index].CurrentVirtualY;
		CurrentMode->PixelFormat=PixelBltOnly;
		CurrentMode->PixelInformation.RedMask=MyGOPImplementations[Index].Mode->PixelInformation.RedMask;
		CurrentMode->PixelInformation.GreenMask=MyGOPImplementations[Index].Mode->PixelInformation.GreenMask;
		CurrentMode->PixelInformation.BlueMask=MyGOPImplementations[Index].Mode->PixelInformation.BlueMask;
		CurrentMode->PixelInformation.ReservedMask=MyGOPImplementations[Index].Mode->PixelInformation.ReservedMask;
		CurrentMode->PixelsPerScanLine=MyGOPImplementations[Index].Mode->PixelsPerScanLine;
		CurrentMode=NULL;
	}
	
	return EFI_SUCCESS;
}

//
//This OverrideFunction Screws a bit with the
//GraphicsOutputProtocol implementation and is
//used to hijack the GOP implementation.
//It will allow setting of different resolutions.
//
EFI_STATUS EfiOverrideGraphicsOutputSetMode(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This, IN UINT32 ModeNumber) {

	UINTN                                    MyRealGopIndex;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL            BlackPixel;
	UINTN                                    Index;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION    *Info;
	UINTN                                    SizeOfInfo;

	Status=EfiOverrideGraphicsOutputQueryMode(This, ModeNumber, &SizeOfInfo, &Info);

	MyRealGopIndex=0;
	//Find out for which GOP instance we're overriding.
	for(Index=0; Index<NumberGOPImplementations; Index++){
		if(This==MyGOPImplementations[Index].Protocol){
			MyRealGopIndex=Index;
			break;
		}
	}
		
	//If we're really changing the video mode.
	if ((Info->HorizontalResolution < MyGOPImplementations[MyRealGopIndex].MaxX) | (Info->VerticalResolution < MyGOPImplementations[MyRealGopIndex].MaxY)){
		//Blank the screen
		//This should be black.
		BlackPixel.Blue=0;
		BlackPixel.Green=0;
		BlackPixel.Red=0;
		BlackPixel.Reserved=0;
		MyGOPImplementations[MyRealGopIndex].Blt(This, &BlackPixel, EfiBltVideoFill, 0, 0, 0, 0, MyGOPImplementations[MyRealGopIndex].MaxX-1, MyGOPImplementations[MyRealGopIndex].MaxX-1, 0);
		MyGOPImplementations[MyRealGopIndex].CurrentVirtualX=Info->HorizontalResolution;
		MyGOPImplementations[MyRealGopIndex].CurrentVirtualY=Info->VerticalResolution;
		MyGOPImplementations[MyRealGopIndex].CurrentXOffset=(MyGOPImplementations[MyRealGopIndex].MaxX - Info->HorizontalResolution)/2;
		MyGOPImplementations[MyRealGopIndex].CurrentYOffset=(MyGOPImplementations[MyRealGopIndex].MaxY - Info->VerticalResolution)/2;
	};
	
	return EFI_SUCCESS;
	
}

//
//This OverrideFunction Screws a bit with the
//GraphicsOutputProtocol implementation and is
//used to hijack the GOP implementation.
//It will fine-tune the Blt in another buffer.
//And will call the video card Blt with the
//corrected parameters.
//
EFI_STATUS EfiOverrideGraphicsOutputBlt(
	IN EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
	IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *BltBuffer, OPTIONAL
	IN     EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
	IN     UINTN                              SourceX,
	IN     UINTN                              SourceY,
	IN     UINTN                              DestinationX,
	IN     UINTN                              DestinationY,
	IN     UINTN                              Width,
	IN     UINTN                              Height,
	IN     UINTN                              Delta OPTIONAL) {
	
	
	return EFI_SUCCESS;
	
}

//
//Creates a table of all resolutions that are smaller than the current one
//
VOID EfiPopulateResolutions (UINTN MaxX, UINTN MaxY) {

	EFI_RESOLUTIONS_LIST    *MyTempResolutions;
	UINTN                    Index;
	
	NumberResolutions=0;

	MyTempResolutions=AllocatePool(sizeof(EFI_RESOLUTIONS_LIST)*27);
	if ((MaxX > 640)&(MaxY > 480)) {
		MyTempResolutions[NumberResolutions].horizontal=640;
		MyTempResolutions[NumberResolutions].vertical=480;
		NumberResolutions++;
	};
	if ((MaxX > 768)&(MaxY > 576)) {
		MyTempResolutions[NumberResolutions].horizontal=768;
		MyTempResolutions[NumberResolutions].vertical=765;
		NumberResolutions++;
	}
	if ((MaxX > 800)&(MaxY > 480)) {
		MyTempResolutions[NumberResolutions].horizontal=800;
		MyTempResolutions[NumberResolutions].vertical=480;
		NumberResolutions++;
	}
	if ((MaxX > 854)&(MaxY > 480)) {
		MyTempResolutions[NumberResolutions].horizontal=854;
		MyTempResolutions[NumberResolutions].vertical=480;
		NumberResolutions++;
	}
	if ((MaxX > 800)&(MaxY > 600)) {
		MyTempResolutions[NumberResolutions].horizontal=800;
		MyTempResolutions[NumberResolutions].vertical=600;
		NumberResolutions++;
	}
	if ((MaxX > 960)&(MaxY > 540)) {
		MyTempResolutions[NumberResolutions].horizontal=960;
		MyTempResolutions[NumberResolutions].vertical=540;
		NumberResolutions++;
	}
	if ((MaxX > 1024)&(MaxY > 576)) {
		MyTempResolutions[NumberResolutions].horizontal=1024;
		MyTempResolutions[NumberResolutions].vertical=576;
		NumberResolutions++;
	}
	if ((MaxX > 1024)&(MaxY > 600)) {
		MyTempResolutions[NumberResolutions].horizontal=1024;
		MyTempResolutions[NumberResolutions].vertical=600;
		NumberResolutions++;
	}
	if ((MaxX > 1024)&(MaxY > 768)) {
		MyTempResolutions[NumberResolutions].horizontal=1024;
		MyTempResolutions[NumberResolutions].vertical=768;
		NumberResolutions++;
	}
	if ((MaxX > 1152)&(MaxY > 864)) {
		MyTempResolutions[NumberResolutions].horizontal=1152;
		MyTempResolutions[NumberResolutions].vertical=864;
		NumberResolutions++;
	}
	if ((MaxX > 1280)&(MaxY > 720)) {
		MyTempResolutions[NumberResolutions].horizontal=1280;
		MyTempResolutions[NumberResolutions].vertical=720;
		NumberResolutions++;
	}
	if ((MaxX > 1280)&(MaxY > 768)) {
		MyTempResolutions[NumberResolutions].horizontal=1280;
		MyTempResolutions[NumberResolutions].vertical=768;
		NumberResolutions++;
	}
	if ((MaxX > 1280)&(MaxY > 800)) {
		MyTempResolutions[NumberResolutions].horizontal=1280;
		MyTempResolutions[NumberResolutions].vertical=800;
		NumberResolutions++;
	}
	if ((MaxX > 1280)&(MaxY > 960)) {
		MyTempResolutions[NumberResolutions].horizontal=1280;
		MyTempResolutions[NumberResolutions].vertical=960;
		NumberResolutions++;
	}
	if ((MaxX > 1280)&(MaxY > 1024)) {
		MyTempResolutions[NumberResolutions].horizontal=1280;
		MyTempResolutions[NumberResolutions].vertical=1024;
		NumberResolutions++;
	}
	if ((MaxX > 1360)&(MaxY > 768)) {
		MyTempResolutions[NumberResolutions].horizontal=1360;
		MyTempResolutions[NumberResolutions].vertical=768;
		NumberResolutions++;
	}
	if ((MaxX > 1366)&(MaxY > 768)) {
		MyTempResolutions[NumberResolutions].horizontal=1366;
		MyTempResolutions[NumberResolutions].vertical=768;
		NumberResolutions++;
	}
	if ((MaxX > 1440)&(MaxY > 900)) {
		MyTempResolutions[NumberResolutions].horizontal=1440;
		MyTempResolutions[NumberResolutions].vertical=900;
		NumberResolutions++;
	}
	if ((MaxX > 1440)&(MaxY > 1050)) {
		MyTempResolutions[NumberResolutions].horizontal=1440;
		MyTempResolutions[NumberResolutions].vertical=1050;
		NumberResolutions++;
	}
	if ((MaxX > 1600)&(MaxY > 1000)) {
		MyTempResolutions[NumberResolutions].horizontal=1600;
		MyTempResolutions[NumberResolutions].vertical=1000;
		NumberResolutions++;
	}
	if ((MaxX > 1600)&(MaxY > 1200)) {
		MyTempResolutions[NumberResolutions].horizontal=1600;
		MyTempResolutions[NumberResolutions].vertical=1200;
		NumberResolutions++;
	}
	if ((MaxX > 1920)&(MaxY > 1080)) {
		MyTempResolutions[NumberResolutions].horizontal=1920;
		MyTempResolutions[NumberResolutions].vertical=1080;
		NumberResolutions++;
	}
	if ((MaxX > 1920)&(MaxY > 1200)) {
		MyTempResolutions[NumberResolutions].horizontal=1920;
		MyTempResolutions[NumberResolutions].vertical=1200;
		NumberResolutions++;
	}
	if ((MaxX > 2048)&(MaxY > 1152)) {
		MyTempResolutions[NumberResolutions].horizontal=2048;
		MyTempResolutions[NumberResolutions].vertical=1152;
		NumberResolutions++;
	}
	if ((MaxX > 2048)&(MaxY > 1536)) {
		MyTempResolutions[NumberResolutions].horizontal=2048;
		MyTempResolutions[NumberResolutions].vertical=1536;
		NumberResolutions++;
	}
	if ((MaxX > 2560)&(MaxY > 1600)) {
		MyTempResolutions[NumberResolutions].horizontal=2560;
		MyTempResolutions[NumberResolutions].vertical=1600;
		NumberResolutions++;
	}
	if ((MaxX > 2560)&(MaxY > 2560)) {
		MyTempResolutions[NumberResolutions].horizontal=2560;
		MyTempResolutions[NumberResolutions].vertical=2560;
		NumberResolutions++;
	}
	MyResolutions=AllocatePool(sizeof(EFI_RESOLUTIONS_LIST)*NumberResolutions);
	
	for (Index=0; Index < NumberResolutions; Index++){
		MyResolutions[Index].horizontal=MyTempResolutions[Index].horizontal;
		MyResolutions[Index].vertical=MyTempResolutions[Index].vertical;
	}
	FreePool(MyTempResolutions);
}

//
//Eventually it should identify all the GOP implementations,
//their controlling device (that is, to which PCI device it belongs).
//And if the multiple implementations point to the same GOP implementation.
//We just need to implement climbing up from Controller Handle to
//Controller Handle until we hit a PciIo.
//If the GOP Implementation does not support the standard
//resolutions, it will override the GOP Implementation with our own.
//

EFI_STATUS EfiOverrideGraphicsOutputProtocol(IN EFI_HANDLE ImageHandle) {
	
	
	EFI_HANDLE                            *HandleBuffer;
	EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *InfoMode;
	EFI_OPEN_PROTOCOL_INFORMATION_ENTRY   *ProtocolInformation;
	EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
	CHAR16                                *DevicePathString;
	
	BOOLEAN                                FoundVGA;
	BOOLEAN                                FoundSVGA;
	BOOLEAN                                FoundXGA;
	UINTN                                  BufferSize;
	UINTN                                  ProtocolInformationCount;
	UINTN                                  Index;
	UINTN                                  ProtocolIndex;
	UINT32                                 ModeIndex;
	UINTN                                  SizeOfInfo;
//	VOID                                  *VirtualFrameBuffer;
	
	BufferSize = 0;
	
	//Get all the Graphics Output Protocol Devices
	Status = BS->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &BufferSize, &HandleBuffer);
	if (EFI_ERROR (Status)) { Print(L"EfiOverrideGraphicsOutputProtocol: Error getting any Graphics Output Protocol instances Handles: %r\n", Status); return Status; };
	
	Print(L"Found %d GraphicsOutput Protocol Instances\n\n", BufferSize);
	
	MyGOPImplementations=AllocatePool(sizeof(EFI_GOP_IMPLEMENTATION)*BufferSize);
	NumberGOPImplementations=BufferSize;
	
	for (Index=0; Index < BufferSize; Index++) {
		
		Print(L"GraphicsOutput %d: EFI Handle                               %08x\n", Index, HandleBuffer[Index]);
		
		
		//
		//TODO: Identify the parent handle
		//      up and up until we get to the
		//      handle with a PCI IO protocol
		//
//		while (TRUE) {
			Status = BS->OpenProtocolInformation(HandleBuffer[Index], &gEfiGraphicsOutputProtocolGuid, &ProtocolInformation, &ProtocolInformationCount);
			if (EFI_ERROR (Status)) { Print(L"EfiOverrideGraphicsOutputProtocol: Error getting the Protocol Information for instance %d: %r\n", Index, Status); return Status; };
			
			Status = BS->HandleProtocol(HandleBuffer[Index], &gEfiDevicePathProtocolGuid, &DevicePath);
			DevicePathString=ConvertDevicePathToText(DevicePath, FALSE, FALSE);
			Print(L"GraphicsOutput %d: DevicePath:                              %s\n", Index, DevicePathString);
			FreePool(DevicePathString);
			DevicePath=NULL;
			
			for (ProtocolIndex = 0; ProtocolIndex < ProtocolInformationCount; ProtocolIndex++) {
				
				Print(L"GraphicsOutput %d: ProtocolInformation %d: AgentHandle:      %08x\n", Index, ProtocolIndex, ProtocolInformation[ProtocolIndex].AgentHandle);
				Print(L"GraphicsOutput %d: ProtocolInformation %d: ControllerHandle: %08x\n", Index, ProtocolIndex, ProtocolInformation[ProtocolIndex].ControllerHandle);
				Print(L"GraphicsOutput %d: ProtocolInformation %d: Attributes:       %8x\n",  Index, ProtocolIndex, ProtocolInformation[ProtocolIndex].Attributes);
				Print(L"GraphicsOutput %d: ProtocolInformation %d: OpenCount:        %8d\n",  Index, ProtocolIndex, ProtocolInformation[ProtocolIndex].OpenCount);
				
			}
//		}
		
		FreePool(ProtocolInformation);
		
		Status = BS->OpenProtocol(HandleBuffer[Index], &gEfiGraphicsOutputProtocolGuid, (VOID *) &GraphicsOutput, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
		if (EFI_ERROR (Status)) { Print(L"EfiOverrideGraphicsOutputProtocol: Error getting the Graphics Output Protocol instance %d: %r\n", Index, Status); GraphicsOutput = NULL; return Status; };
		
		
		if (GraphicsOutput != NULL) {
			
			//Print the current GraphicsOutput mode
			Print(L"GraphicsOutput %d: Current Mode: %4dx%4d     FrameBuffer: %x-%x(%x)\n", Index, GraphicsOutput->Mode->Info->HorizontalResolution, GraphicsOutput->Mode->Info->VerticalResolution, GraphicsOutput->Mode->FrameBufferBase, GraphicsOutput->Mode->FrameBufferBase+GraphicsOutput->Mode->FrameBufferSize, GraphicsOutput->Mode->FrameBufferSize);
			FoundVGA =FALSE;
			FoundSVGA=FALSE;
			FoundXGA =FALSE;
			//Print all the GraphicsOutput Modes
			for (ModeIndex = 0; ModeIndex < GraphicsOutput->Mode->MaxMode; ModeIndex++){
				Status=GraphicsOutput->QueryMode(GraphicsOutput, ModeIndex, &SizeOfInfo, &InfoMode);
				Print(L"GraphicsOutput %d: Mode ID %d   : %4dx%4d     FrameBuffer: %x-%x(%x)\n", Index, (ModeIndex), InfoMode->HorizontalResolution, InfoMode->VerticalResolution, GraphicsOutput->Mode->FrameBufferBase, GraphicsOutput->Mode->FrameBufferBase+GraphicsOutput->Mode->FrameBufferSize, GraphicsOutput->Mode->FrameBufferSize);
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

			//If we don't have the standard resolutions,
			//let's override the Query, Set and Blt functions
			//and create a new VirtualFrameBuffer.
			//Need to check if we've already done this.
			/*if (!(FoundVGA & FoundSVGA & FoundXGA)) {
				Status=BS->AllocatePool(EfiRuntimeServicesData, GraphicsOutput->Mode->FrameBufferSize, &VirtualFrameBuffer);
				if (EFI_ERROR(Status)) { Print(L"Could not allocate VirtualPool\n"); break;};
				MyGOPImplementations[Index].VirtualFrameBuffer = VirtualFrameBuffer;
				MyGOPImplementations[Index].CurrentVFBSize     = GraphicsOutput->Mode->FrameBufferSize;
				GraphicsOutput->QueryMode = EfiOverrideGraphicsOutputQueryMode;
				GraphicsOutput->SetMode   = EfiOverrideGraphicsOutputSetMode;
				GraphicsOutput->Blt       = EfiOverrideGraphicsOutputBlt;
				EfiPopulateResolutions(MyGOPImplementations[Index].CurrentVirtualX, MyGOPImplementations[Index].CurrentVirtualY);
			}*/
			
			//Cleanup after ourselves
			Status = BS->CloseProtocol (HandleBuffer[Index], &gEfiGraphicsOutputProtocolGuid, ImageHandle, NULL);
			if (EFI_ERROR (Status)) { Print(L"EfiOverrideGraphicsOutputProtocol: Error closing the Graphics Output Protocol instance %d: %r\n", Index, Status); return Status; };
			
			Print(L"\n");
		}
	}
	
	//Final Cleanup
	FreePool(HandleBuffer);
	return EFI_SUCCESS;
	
}

//
//Question: How the heck do we identify what the values are for each video card?
//It currently displays the information about the current settings of the video card.
//
EFI_STATUS EfiGetGMuxInformation(IN EFI_HANDLE ImageHandle){
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

//
//Powers on the Discrete Video Card
//Does not call the EFI to connect all drivers
//
EFI_STATUS EfiStartDiscrete(IN EFI_HANDLE ImageHandle){
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

//
//Shuts down the Discrete Video Card
//Does not call the EFI to disconnect all drivers
//
EFI_STATUS EfiShutdownDiscrete(IN EFI_HANDLE ImageHandle){

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

//
//Prints out information about the console device
//
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

//
//Creates a full device path from the Filename string and the current device path of ImageHandle.
//
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

//
//Loads and executes an image whose path was created with EfiFileDevicePath
//
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

//
//Attempts to modify the memory descriptor for the application
//to EFI_RUNTIME_CODE
//
EFI_STATUS EfiSetRuntimeCodeFlag(IN EFI_HANDLE ImageHandle){

	EFI_HANDLE                        SelfImageHandle;
	EFI_LOADED_IMAGE                 *SelfLoadedImage;
	UINT64                            Base;
	UINT64                            Length;
	EFI_GCD_MEMORY_SPACE_DESCRIPTOR   GcdDescriptor;

	SelfImageHandle = ImageHandle;
	
	//
	//Get me a LoadedImageProtocol of MySelf
	//
	Status = BS->HandleProtocol (SelfImageHandle, &gEfiLoadedImageProtocolGuid, (VOID*)&SelfLoadedImage);
	if(EFI_ERROR(Status)) { Print(L"LoadedImageProtocol Error: %r\n", Status); return Status;  }
	
	Base   = (UINT64)SelfLoadedImage->ImageBase & (~EFI_PAGE_MASK);
	Length = SelfLoadedImage->ImageSize + ((UINT64)SelfLoadedImage->ImageBase - Base);
	Length = (Length + EFI_PAGE_SIZE - 1) & (~EFI_PAGE_MASK);

	Status      = DS->GetMemorySpaceDescriptor (Base, &GcdDescriptor);
    if (EFI_ERROR (Status)) {
		Status = EFI_UNSUPPORTED;
		return Status;
	}

	Status = DS->SetMemorySpaceAttributes (Base, Length, GcdDescriptor.Attributes | EFI_MEMORY_RUNTIME );
	if (EFI_ERROR (Status)) {
		Status = EFI_UNSUPPORTED;
		return Status;
	}
	
	return EFI_SUCCESS;
}

//
//EFI Version of:
//int main (int argc, char *argv[]){ return 0;}
//
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE  *SystemTable) {
	
	//
	//I'm Lazy. I don't want to type SystemTable->BootServices 1000 times.
	//
	//SystemTable, BootServices, RuntimeServices, DXEServices
	ST = SystemTable;
	BS = SystemTable->BootServices;
	RS = SystemTable->RuntimeServices;
	Status = EfiGetSystemConfigurationTable(&gEfiDxeServicesTableGuid, (VOID *) &DS);
	if(EFI_ERROR(Status)) { Print(L"Could not load the EFI_DXE_SERVICES Table\n", Status); return Status;};
	NumberOptionRoms=0;
	
	//
	//Reload as RunTime MemoryType See loaded image protocol
	//
	Status = EfiSetRuntimeCodeFlag(ImageHandle);
	
	
	//
	//Set the Efi Version specified in the parameter
	//
	Status = EfiSetVersion(2,10);
	if(EFI_ERROR(Status)) { Print(L"EfiSetVersion Error: %r\n", Status); } //return Status;  }
	
	//
	//Populate the VGA Cards Vector
	//
	Status = EfiPopulateVgaVectors(ImageHandle);
	if(EFI_ERROR(Status)) { Print (L"Could not populate the VGA Cards Vector: %r\n", Status); } //return Status;}
	
	//
	//Print the content of the VGA Cards Vector. For debugging purposes.
	//TODO: Use the EfiOverrideGraphicsOutputProtocol information to figure out if one trully has GOP enabled.
	//
	Status = EfiPrintVgaVector();
	if(EFI_ERROR(Status)) { Print(L"This shouldn't happen: %r\n", Status); } //return Status; }
	
	//
	//Find the active VGA Card and set the VGA PCI Registers on it.
	//TODO: This is a dummy implementation that doesn't take into account the VGA Cards Vector information
	//
	Status = EfiWriteActiveGraphicsAdaptorPCIRegs(ImageHandle);
	if(EFI_ERROR(Status)) { Print(L"EfiWritePciRegisters Error: %r\n", Status); }//return Status;  }
	
	//
	//Override the GraphicsOutputProtocol.
	//Right now we're just identifying it.
	//TODO: Actually override it to one that supports on BLT ops and converts the numerics.
	//
	Status = EfiOverrideGraphicsOutputProtocol(ImageHandle);
	if(EFI_ERROR(Status)) { Print(L"EfiOverrideGraphicsOutputProtocol Error: %r\n", Status); }
	
	//
	//Get all the ROMs in the FirmwareVolume
	//TODO: Implement fallback to FirmwareVolume2Protocol
	//
	Status = EfiGetFirmwareOpRomInformation(ImageHandle);
	if(EFI_ERROR(Status)) {Print(L"EfiGetFirmwareOpRomInformation Error: %r\n", Status); }
	
	//
	//Get all the ROMs in the PCI space
	//
	Status = EfiGetPciOpRomInformation();
	if(EFI_ERROR(Status)) {Print(L"EfiGetPciOpRomInformation Error: %r\n", Status); }
	
	//
	//Get all the ROMs in all the FileSystems in EFI\\ShMessenger
	//
	Status = EfiGetFileSystemOpRomInformation(ImageHandle);
	if(EFI_ERROR(Status)) {Print(L"EfiGetPciOpRomInformation Error: %r\n", Status); }
	
	Print(L"\n");
	//
	//Get the gMux information.
	//Eventually we need to add proper gMux setting support for switching between the cards.
	//
	Status = EfiGetGMuxInformation(ImageHandle);
	if(EFI_ERROR(Status)) {Print(L"EfiGetGMuxInformation Error: %r\n", Status); }
	
	Status = EfiPrintConsoleInformation();
	
	//
	//Let's CleanUp after ourselves 
	//
	FreePool (MyVideoCards);
	
	//
	//Chainload the OS Bootloader.
	//Try also L"EFI\\Ubuntu\\grubx64.efi"
	//
	Status = EfiLoadExecuteImage(ImageHandle, L"EFI\\Microsoft\\Boot\\bootmgfw.efi");
	if(EFI_ERROR(Status)) { Print(L"LoadExecuteImage Error: %r\n", Status); }//return Status; }
	
	//
	//We should not actually ever get here unless we fail to load the bootloader.
	//I wouldn't imagine that there is an exit out of the Windows OS
	//This is not Solaris in OpenFirmware.
	//I'd like to think that I'm doing a good job cleaning up after myself anyway.
	//
	Status = BS->Exit(ImageHandle, EFI_SUCCESS, 0, NULL);
	
	return EFI_SUCCESS;
	
}