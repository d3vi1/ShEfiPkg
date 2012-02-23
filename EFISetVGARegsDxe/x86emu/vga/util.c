/*
 * util.c:
 * Various utility functions.
 *
 * Copyright (c) 2006 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] =
  "$Id: util.c,v 1.1 2006/05/27 18:41:14 root Exp $";

#include "project.h"

CHAR16 log_print_buf[LOG_PRINT_LEN];

/* efi_strerror STATUS
 * Return a string describing STATUS. */
CHAR16 *
efi_strerror (EFI_STATUS st)
{
  static CHAR16 errbuf[256];
  StatusToString (errbuf, st);
  return errbuf;
}


int safe_to_use_bios = 0;

static void
bios_write_string (CHAR16 * s)
{
  if (!safe_to_use_bios)
    return;

  safe_to_use_bios = 0;
  while (*s)
    {
      if (*s == '\n')
        bios_write_char ('\r');
      bios_write_char (*s);
      s++;
    }
  safe_to_use_bios = 1;
}


int safe_to_use_efi = 1;

/* do_log_print MESSAGE
 * Print an error to the console and, if possible, to the network. */
void
do_log_print (CHAR16 * msg)
{
  if (safe_to_use_efi)
    Print (L"%s", msg);
  bios_write_string (msg);
}

/* alloc_buf SIZE
 * Allocate a buffer of SIZE bytes from the EfiLoaderData pool. */
void *
alloc_buf (UINTN size)
{
  EFI_STATUS st;
  void *buf;

  if (EFI_SUCCESS != (st = BS->AllocatePool (EfiLoaderData, size, &buf)))
    {
      LogPrint (L"    AllocatePool: %s\n", efi_strerror (st));
      return NULL;
    }

//    LogPrint(L"    Allocated %d bytes at %lx\n", size, buf);

  return buf;
}

void
free_buf (void *buf)
{
  FreePool (buf);
}

/* 
 * String utilities (analogues for <string.h>)
 */

int
util_strlen (CHAR8 * in)
{
  int ret = 0;
  while (*(in++))
    ret++;
  return ret;
}

void
utf16_to_utf8 (OUT CHAR8 * out, IN CHAR16 * in)
{
  while (*in)
    {
      *(out++) = *(in++);
    }
  *out = 0;
}

/* util_memcmp A B N
 * Wrapper for CompareMem. */
INTN
util_memcmp (VOID * A, VOID * B, UINTN len)
{
  return CompareMem (A, B, len);
}

/* util_wstrcpy A B
 * Wrapper for StrCpy. */
void
util_wstrcpy (CHAR16 * dst, CHAR16 * src)
{
  StrCpy (dst, src);
}

/* util_memcpy A B N
 * Wrapper for CopyMem. */
INTN
memcpy (VOID * A, VOID * B, UINTN len)
{
  CopyMem (A, B, len);
  return 0;
}

/* util_sleep N
 * Stall for N seconds. */
void
util_sleep (int s)
{
  while (s--)
    systab->BootServices->Stall (1000000);
}

void
usleep (int s)
{
  systab->BootServices->Stall (s);
}

#if 0
/* get_efi_info
 * Return the EFI_LOADED_IMAGE information for the current image. */
EFI_LOADED_IMAGE *
get_efi_info (void)
{
  static EFI_LOADED_IMAGE *info = NULL;
  static int failed = 0;
  EFI_STATUS st;

  if (info)
    return info;
  if (failed)
    return NULL;

  st = BS->HandleProtocol (image, &LoadedImageProtocol, (VOID **) & info);

  if (EFI_SUCCESS != st)
    {
      Print (L"    HandleProtocol(LoadedImageProtocol): %s\n",
             efi_strerror (st));
      failed++;
      return NULL;
    }

  return info;
}
#endif

/*
 * efi_Device_to_str, return a string which describes the 
 * EFI_HANDLE of a device h
 */

CHAR16 *
efi_device_to_str (EFI_HANDLE h)
{
  EFI_DEVICE_PATH *dp;

  if (!h)
    return L"(null)";
  BS->HandleProtocol (h, &DevicePathProtocol, (VOID **) & dp);
  return DevicePathToStr (dp);
}

char
i_to_xdigit (int i)
{
  if (i < 10)
    return '0' + i;
  return 'A' + (i - 10);
}

void *
memset (void *_a, int c, size_t len)
{
  uint8_t *a = (uint8_t *) _a;

  while (len--)
    *(a++) = c;

  return _a;
}

void *
memmove (void *dst, void *src, size_t len)
{
  void *tmp = alloc_buf (len);

  memcpy (tmp, src, len);
  memcpy (dst, tmp, len);

  free_buf (tmp);
  return dst;
}

void
util_memcpy (void *dst, void *src, size_t len)
{
  memcpy (dst, src, len);
}
