#ifndef _EFI_VARIABLE_H_
#define _EFI_VARIABLE_H_

#define VARIABLE_STORE_SIGNATURE  EFI_SIGNATURE_32 ('$', 'V', 'S', 'S')

#ifndef MAX_VARIABLE_SIZE
#define MAX_VARIABLE_SIZE         1024
#endif

//
// Enlarges the hardware error record maximum variable size to 32K bytes
//
#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
#ifndef MAX_HARDWARE_ERROR_VARIABLE_SIZE
#define MAX_HARDWARE_ERROR_VARIABLE_SIZE 0x8000
#endif
#endif

#define VARIABLE_DATA             0x55AA

//
// Variable Store Header flags
//
#define VARIABLE_STORE_FORMATTED  0x5a
#define VARIABLE_STORE_HEALTHY    0xfe

//
// Variable Store Status
//
typedef enum {
  EfiRaw,
  EfiValid,
  EfiInvalid,
  EfiUnknown
} VARIABLE_STORE_STATUS;

//
// Variable State flags
//
#define VAR_IN_DELETED_TRANSITION     0xfe  // Variable is in obsolete transistion
#define VAR_DELETED                   0xfd  // Variable is obsolete
#define VAR_ADDED                     0x7f  // Variable has been completely added
#define IS_VARIABLE_STATE(_c, _Mask)  (BOOLEAN) (((~_c) & (~_Mask)) != 0)

#pragma pack(1)

///
/// Alignment of variable name and data, according to the architecture:
/// * For IA-32 and Intel(R) 64 architectures: 1.
/// * For IA-64 architecture: 8.
///
#if defined (MDE_CPU_IPF)
#define ALIGNMENT         8
#else
#define ALIGNMENT         1
#endif

//
// GET_PAD_SIZE calculates the miminal pad bytes needed to make the current pad size satisfy the alignment requirement.
//
#if (ALIGNMENT == 1)
#define GET_PAD_SIZE(a) (0)
#else
#define GET_PAD_SIZE(a) (((~a) + 1) & (ALIGNMENT - 1))
#endif

///
/// Alignment of Variable Data Header in Variable Store region.
///
#define HEADER_ALIGNMENT  4
#define HEADER_ALIGN(Header)  (((UINTN) (Header) + HEADER_ALIGNMENT - 1) & (~(HEADER_ALIGNMENT - 1)))

typedef struct {
  UINT32  Signature;
  UINT32  Size;
  UINT8   Format;
  UINT8   State;
  UINT16  Reserved;
  UINT32  Reserved1;
} VARIABLE_STORE_HEADER;

typedef struct {
  UINT16    StartId;
  UINT8     State;
  UINT8     Reserved;
  UINT32    Attributes;
  UINT32    NameSize;
  UINT32    DataSize;
  EFI_GUID  VendorGuid;
} VARIABLE_HEADER;
EFI_GUID gFrameworkEfiFirmwareVolumeBlockProtocolGuid;
#pragma pack()

#endif // _EFI_VARIABLE_H_
