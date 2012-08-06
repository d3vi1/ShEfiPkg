#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>
//#include <Protocol/LegacyBios.h>
//#include <Protocol/Legacy8259.h>
#include <Guid/GlobalVariable.h>
#include <Pi/PiDxeCis.h>
#include <DevicePathToText.h>
#include <ShadowOptionRom.h>

typedef struct {
	UINT16                                    VendorId;
	UINT16                                    DeviceId;
 	UINT16                                    Command;
	UINT16                                    Status;
 	UINT8                                     RevisionId;
	UINT8                                     ClassCode[3];
 	UINT8                                     CacheLineSize;
	UINT8                                     PrimaryLatencyTimer;
	UINT8                                     HeaderType;
	UINT8                                     BIST;
} EFI_PCI_DEVICE_HEADER;

/*typedef struct {
	UINT16                                     VendorId;
	UINT16                                     DeviceId;
	UINT8                                      RevisionId;
	UINT8                                      ClassCode[3];
	UINTN                                      Bus;
	UINTN                                      Device;
	UINTN                                      Function;
	BOOLEAN                                    AttributeVgaIo;
	BOOLEAN                                    AttributeVgaMemory;
	BOOLEAN                                    AttributeIo;
	BOOLEAN                                    AttributeBusMaster;
	BOOLEAN                                    IsMainVideoCard;
	BOOLEAN                                    IsActiveVideoCard;
	VOID                                      *RomLocation;
	UINT64                                     RomSize;
	EFI_HANDLE                                *Handle;
} EFI_VIDEO_CARD;*/

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

#define EFI_CPU_EFLAGS_IF 0x200
