[Defines]
	PLATFORM_NAME                   = ShEfiPkg
	PLATFORM_GUID                   = 3340633b-87a8-46b3-9bec-fd64655049e7
	PLATFORM_VERSION                = 0.01
	DSC_SPECIFICATION               = 0x00010005
	OUTPUT_DIRECTORY                = Build/ShEfiPkg
	SUPPORTED_ARCHITECTURES         = IA32|X64|EBC
	BUILD_TARGETS                   = DEBUG|RELEASE
	SKUID_IDENTIFIER                = DEFAULT

[SkuIds]
	0|DEFAULT              # The entry: 0|DEFAULT is reserved and always required.

[LibraryClasses]
	UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
	UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
	BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
	BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
	UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
	PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
	PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
	MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
	UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
	UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
	DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
	DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
	PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
	UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf


[Components]
	ShEfiPkg/EFISetVGARegs/EFISetVGARegs.inf
	ShEfiPkg/EFISetVGARegsDxe/EFISetVGARegsDxe.inf
	ShEfiPkg/EFIUpdateReleaseDxe/EFIUpdateReleaseDxe.inf
	ShEfiPkg/GOPSetRes/GOPSetRes.inf

#DEFINE  EMULATE = 1

#!include StdLib/StdLib.inc
