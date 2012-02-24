#ifndef _EFI_FIRMWARE_VOLUME_HEADER_1_H_
#define _EFI_FIRMWARE_VOLUME_HEADER_1_H_

#define EFI_FVB_ALIGNMENT_CAP       0x00008000
#define EFI_FVB_ALIGNMENT_2         0x00010000
#define EFI_FVB_ALIGNMENT_4         0x00020000
#define EFI_FVB_ALIGNMENT_8         0x00040000
#define EFI_FVB_ALIGNMENT_16        0x00080000
#define EFI_FVB_ALIGNMENT_32        0x00100000
#define EFI_FVB_ALIGNMENT_64        0x00200000
#define EFI_FVB_ALIGNMENT_128       0x00400000
#define EFI_FVB_ALIGNMENT_256       0x00800000
#define EFI_FVB_ALIGNMENT_512       0x01000000
#define EFI_FVB_ALIGNMENT_1K        0x02000000
#define EFI_FVB_ALIGNMENT_2K        0x04000000
#define EFI_FVB_ALIGNMENT_4K        0x08000000
#define EFI_FVB_ALIGNMENT_8K        0x10000000
#define EFI_FVB_ALIGNMENT_16K       0x20000000
#define EFI_FVB_ALIGNMENT_32K       0x40000000
#define EFI_FVB_ALIGNMENT_64K       0x80000000

#define EFI_FVB_CAPABILITIES  (EFI_FVB_READ_DISABLED_CAP | \
                              EFI_FVB_READ_ENABLED_CAP | \
                              EFI_FVB_WRITE_DISABLED_CAP | \
                              EFI_FVB_WRITE_ENABLED_CAP | \
                              EFI_FVB_LOCK_CAP \
                              )

#define EFI_FVB_STATUS    (EFI_FVB_READ_STATUS | EFI_FVB_WRITE_STATUS | EFI_FVB_LOCK_STATUS)




#endif