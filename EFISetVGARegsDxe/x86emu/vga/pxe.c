#include "project.h"

/*
 * attempt to initalize pxe (exactly) once 
 * and return a pointer to the EFI_PXE... structure
 * if pxe fails, return NULL
 *
 */

EFI_PXE_BASE_CODE *
get_efi_pxe (void)
{
  static EFI_PXE_BASE_CODE *pxe = NULL;
  static int failed = 0;
  EFI_LOADED_IMAGE *info;
  EFI_STATUS st;

  if (pxe)
    return pxe;
  if (failed)
    return NULL;

  info = get_efi_info ();
  if (!info)
    return NULL;

  st = BS->HandleProtocol (info->DeviceHandle, &PxeBaseCodeProtocol,
                           (VOID **) & pxe);

  if (EFI_SUCCESS != st)
    {
      EFI_HANDLE net = get_network_handle ();

      st = BS->HandleProtocol (net, &PxeBaseCodeProtocol, (VOID **) & pxe);
    }

  if (EFI_SUCCESS != st)
    {
      Print (L"    HandleProtocol(PxeBaseCodeProtocol): %s\n",
             efi_strerror (st));
      failed++;
      return NULL;
    }

  if (!pxe->Mode->Started && EFI_SUCCESS != (st = pxe->Start (pxe, 0)))
    {
      Print (L"    pxe->Start(): %s\n", efi_strerror (st));
      failed++;
      return NULL;
    }

  util_sleep (5);

  if (!pxe->Mode->DhcpAckReceived && EFI_SUCCESS != (st = pxe->Dhcp (pxe, 0)))
    {
      Print (L"    pxe->Dhcp(): %s\n", efi_strerror (st));
      failed++;
      return NULL;
    }

  if (!pxe->Mode->DhcpAckReceived)
    {
      Print (L"    DHCP ACK not yet received; not netbooted?\n");
      return NULL;
    }

  return pxe;
}
