EFI_STATUS           Status;
EFI_HANDLE           ImageHandle;
EFI_SYSTEM_TABLE     *ST;
EFI_BOOT_SERVICES    *BS;
EFI_RUNTIME_SERVICES *RS;

EFI_STATUS EfiParseSectionStream(IN VOID *Stream, IN UINTN StreamSize, OUT VOID *OptionRom, OUT UINTN *OptionRomSize);

EFI_STATUS EfiParseSection(IN VOID *Stream, IN UINTN StreamSize, OUT VOID **OptionRom, OUT UINTN *OptionRomSize);