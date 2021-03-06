#include <Uefi.h>
#include <Library/UefiLib.h>
#include <x86emu/x86emu.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/TianoDecompress.h>
#include <Protocol/GuidedSectionExtraction.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LegacyBios.h>
#include <Guid/FileInfo.h>
#include <FirmwareVolume.h>

typedef struct {
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
} EFI_VIDEO_CARD;

typedef struct {
	UINT16                                     Signature;                     //Should be 0x55AA
	UINT8                                      x86Init[22];                   //Safe to ignore, x86 boot code of 16h
	UINT16                                     PciDataStructure;              //Relative Pointer to PCIDataStructure
} PCI_OPTION_ROM;

typedef struct {
	UINT32                                     Signature;                      //Should be PCIR (0x52494350)
	UINT16                                     VendorId;                       //PCI Vendor ID for the Device
	UINT16                                     DeviceId;                       //PCI Device ID for the Device
	UINT16                                     Reserved0;
	UINT16                                     Length;                         //Length of PCI Data Structure
	UINT8                                      Revision;                       //PCIDataStructure Revision should be 0
	UINT8                                      ClassCode[3];                   //PCI Class Code for the Device
	UINT16                                     ImageLength;                    //512byte Units
	UINT16                                     ImageRevision;                  //Revision of the actual software build
	UINT8                                      CodeType;                       //0 Intel x86, PC-AT compatible, 1 Open Firmware standard for PCI48, 2 Hewlett-Packard PA RISC, 3 Extensible Firmware Interface (EFI)
	UINT8                                      LastImageIndicator;             //1 if at the last image in the ROM, otherwise, other images are chained
	UINT16                                     Reserved1;
} PCI_DATA_STRUCTURE;

typedef struct {
	VOID                                      *OptionRom;
	UINTN                                      OptionRomSize;
	UINT16                                     VendorId;
	UINT16                                     DeviceId;
	BOOLEAN                                    VGA;
	BOOLEAN                                    BIOSCode;
	BOOLEAN                                    OpenFirmwareCode;
	BOOLEAN                                    HPPACode;
	BOOLEAN                                    EFICode;
} EFI_OPTION_ROM;

typedef struct 	{
	UINT16                                     StructureSize;
	UINT8                                      TableFormatRevision;            //Change it when the Parser is not backward compatible
	UINT8                                      TableContentRevision;           //Change it only when the table needs to change but the firmware
	                                                                           //Image can't be updated, while Driver needs to carry the new table!
} ATOM_COMMON_TABLE_HEADER;

typedef struct {
	ATOM_COMMON_TABLE_HEADER                   Header;
	UINT32                                     FirmWareSignature;              //Signature to distinguish between Atombios and non-atombios,
	UINT16                                     BiosRuntimeSegmentAddress;      //atombios should init it as "ATOM", don't change the position
	UINT16                                     ProtectedModeInfoOffset;
	UINT16                                     ConfigFilenameOffset;
	UINT16                                     CRC_BlockOffset;
	UINT16                                     BIOS_BootupMessageOffset;
	UINT16                                     Int10Offset;
	UINT16                                     PciBusDevInitCode;
	UINT16                                     IoBaseAddress;
	UINT16                                     SubsystemVendorID;
	UINT16                                     SubsystemID;
	UINT16                                     PCI_InfoOffset;
	UINT16                                     MasterCommandTableOffset;        //Offset for SW to get all command table offsets, Don't change the position
	UINT16                                     MasterDataTableOffset;           //Offset for SW to get all data table offsets, Don't change the position
	UINT8                                      ExtendedFunctionCode;
	UINT8                                      Reserved;
} ATOM_ROM_HEADER;

typedef struct {
	UINT8                                      Header[8];                        //EDID header "00 FF FF FF FF FF FF 00"
	UINT16                                     ManufactureName;                  //EISA 3-character ID
	UINT16                                     ProductCode;                      //Vendor assigned code
	UINT32                                     SerialNumber;                     //32-bit serial number
	UINT8                                      WeekOfManufacture;                //Week number
	UINT8                                      YearOfManufacture;                //Year
	UINT8                                      EdidVersion;                      //EDID Structure Version
	UINT8                                      EdidRevision;                     //EDID Structure Revision
	UINT8                                      VideoInputDefinition;
	UINT8                                      MaxHorizontalImageSize;           //cm
	UINT8                                      MaxVerticalImageSize;             //cm
	UINT8                                      DisplayTransferCharacteristic;
	UINT8                                      FeatureSupport;
	UINT8                                      RedGreenLowBits;                  //Rx1 Rx0 Ry1 Ry0 Gx1 Gx0 Gy1Gy0
	UINT8                                      BlueWhiteLowBits;                 //Bx1 Bx0 By1 By0 Wx1 Wx0 Wy1 Wy0
	UINT8                                      RedX;                             //Red-x Bits 9 - 2
	UINT8                                      RedY;                             //Red-y Bits 9 - 2
	UINT8                                      GreenX;                           //Green-x Bits 9 - 2
	UINT8                                      GreenY;                           //Green-y Bits 9 - 2
	UINT8                                      BlueX;                            //Blue-x Bits 9 - 2
	UINT8                                      BlueY;                            //Blue-y Bits 9 - 2
	UINT8                                      WhiteX;                           //White-x Bits 9 - 2
	UINT8                                      WhiteY;                           //White-x Bits 9 - 2
	UINT8                                      EstablishedTimings[3];
	UINT8                                      StandardTimingIdentification[16];
	UINT8                                      DetailedTimingDescriptions[72];
	UINT8                                      ExtensionFlag;                    //Number of (optional) 128-byte EDID extension blocks to follow
	UINT8                                      Checksum;
} EDID_BLOCK;

EFI_OPTION_ROM                 *MyOptionRoms;
UINTN                           NumberOptionRoms;

VOID                           *RealModeMemory;  //The pointer to the REAL mode emulated system memory

EFI_STATUS EfiShadowVGAOptionROM (IN EFI_VIDEO_CARD *MyVideoCard);
EFI_STATUS EfiGetOptionRoms();
#define M _X86EMU_env

#define X86_EAX M.x86.R_EAX
#define X86_EBX M.x86.R_EBX
#define X86_ECX M.x86.R_ECX
#define X86_EDX M.x86.R_EDX
#define X86_ESI M.x86.R_ESI
#define X86_EDI M.x86.R_EDI
#define X86_EBP M.x86.R_EBP
#define X86_EIP M.x86.R_EIP
#define X86_ESP M.x86.R_ESP
#define X86_EFLAGS M.x86.R_EFLG

#define X86_FLAGS M.x86.R_FLG
#define X86_AX M.x86.R_AX
#define X86_BX M.x86.R_BX
#define X86_CX M.x86.R_CX
#define X86_DX M.x86.R_DX
#define X86_SI M.x86.R_SI
#define X86_DI M.x86.R_DI
#define X86_BP M.x86.R_BP
#define X86_IP M.x86.R_IP
#define X86_SP M.x86.R_SP
#define X86_CS M.x86.R_CS
#define X86_DS M.x86.R_DS
#define X86_ES M.x86.R_ES
#define X86_SS M.x86.R_SS
#define X86_FS M.x86.R_FS
#define X86_GS M.x86.R_GS

#define X86_AL M.x86.R_AL
#define X86_BL M.x86.R_BL
#define X86_CL M.x86.R_CL
#define X86_DL M.x86.R_DL

#define X86_AH M.x86.R_AH
#define X86_BH M.x86.R_BH
#define X86_CH M.x86.R_CH
#define X86_DH M.x86.R_DH
#define X86_TF_MASK             0x00000100
#define X86_IF_MASK             0x00000200
#define X86_IOPL_MASK           0x00003000
#define X86_NT_MASK             0x00004000
#define X86_VM_MASK             0x00020000
#define X86_AC_MASK             0x00040000
#define X86_VIF_MASK            0x00080000      /* virtual interrupt flag */
#define X86_VIP_MASK            0x00100000      /* virtual interrupt pending */
#define X86_ID_MASK             0x00200000
