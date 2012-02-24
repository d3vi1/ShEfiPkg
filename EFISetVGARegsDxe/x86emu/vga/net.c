#include "project.h"

static const char rcsid[] = "$Id: net.c,v 1.1 2006/05/27 18:41:14 root Exp $";

/* get_network_handle
 * Return a handle on the first installed network interface, or NULL if none
 * is installed. */
EFI_HANDLE
get_network_handle (void)
{
#define MAXNETS    256
  EFI_STATUS st;
  static EFI_HANDLE ret = NULL;
  EFI_HANDLE nets[MAXNETS];
  UINTN nnets = MAXNETS;

  /*short circuit if we know the answer */
  if (ret)
    return ret;

  nnets = MAXNETS * sizeof (EFI_HANDLE);
  if (EFI_SUCCESS !=
      (st =
       BS->LocateHandle (ByProtocol, &NetworkInterfaceIdentifierProtocol,
                         NULL, &nnets, nets)))
    {
      LogPrint (L"    LocateHandle(netifid_guid): %s\n", efi_strerror (st));
      return NULL;
    }
  nnets /= sizeof (EFI_HANDLE);

  if (nnets == 0)
    {
      LogPrint (L"    No netifs; not configured?\n");
      return NULL;
    }

  ret = nets[0];

  return ret;

#undef MAXNETS
}

/* find_network_dp
 * Return the device path of the first network interface, or NULL if none is
 * available. */
EFI_DEVICE_PATH *
find_network_dp (void)
{
  EFI_DEVICE_PATH *dp;
  EFI_HANDLE net;

  net = get_network_handle ();
  if (!net)
    return NULL;

  BS->HandleProtocol (net, &DevicePathProtocol, (VOID **) & dp);
  LogPrint (L"        net device path: %s\n", DevicePathToStr (dp));
  return dp;
}

/* get_mac_from_dp PATH MAC
 * Extract the final MAC address from PATH, returning 0 on success or -1 on
 * failure. */
int
get_mac_from_dp (EFI_DEVICE_PATH * dp, uint8_t * mac)
{
  EFI_DEVICE_PATH *DevPathNode;
  int found = 0;

  dp = UnpackDevicePath (dp);
  if (!dp)
    {
      LogPrint (L"    UnpackDevicePath failed\n");
      return -1;
    }

  DevPathNode = dp;
  while (!IsDevicePathEnd (DevPathNode))
    {
      if (DevicePathType (DevPathNode) == MESSAGING_DEVICE_PATH
          && DevicePathSubType (DevPathNode) == MSG_MAC_ADDR_DP)
        {
          MAC_ADDR_DEVICE_PATH *hdp;
          hdp = (MAC_ADDR_DEVICE_PATH *) DevPathNode;
          CopyMem (mac, &hdp->MacAddress, MAC_LEN);
          found++;
        }
      DevPathNode = NextDevicePathNode (DevPathNode);
    }

  if (!found)
    {
      LogPrint (L"    No mac address found for %s\n", DevicePathToStr (dp));
      return -1;
    }

  return 0;
}

/* get_pxe_file_dp
 * Return the device path of the PXE (or DHCP) boot file, assuming that
 * pxe is working */

EFI_DEVICE_PATH *
get_pxe_file_dp (void)
{
  EFI_PXE_BASE_CODE *pxe;
  CHAR16 buf[1024];

  if (!(pxe = get_efi_pxe ()))
    return NULL;

  SPrint (buf, sizeof buf, L"%a", pxe->Mode->DhcpAck.Dhcpv4.BootpBootFile);
  return FileDevicePath (NULL, buf);
}
