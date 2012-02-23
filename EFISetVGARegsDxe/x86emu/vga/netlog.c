/*
 * netlog.c:
 * Log messages to the network.
 *
 * Copyright (c) 2006 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] =
  "$Id: netlog.c,v 1.1 2006/05/27 18:41:14 root Exp $";

#include "project.h"

const static EFI_IP_ADDRESS broadcast = {.v4 = {{255, 255, 255, 255}} };
const static EFI_PXE_BASE_CODE_UDP_PORT syslogport = 514;
const static CHAR16 *months[] =
  { L"x", L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug",
L"Sep", L"Oct", L"Nov", L"Dec" };


/* netlog MESSAGE
 * Send MESSAGE to the local broadcast address as a BSD syslog packet. Any
 * listening syslogd should pick this up and log it. Messages are sent with a
 * fixed facility LOCAL0 and priority INFO. */
void
netlog (const CHAR16 * str)
{
  static int saw_error;

  static CHAR16 hostname[128];

  EFI_PXE_BASE_CODE *pxe;
  CHAR16 ubuf[NETLOG_MAXLEN], *p;
  UINT8 buf[NETLOG_MAXLEN], *q;
  UINTN len;
  EFI_TIME when;
  EFI_STATUS st;

  pxe = get_efi_pxe ();

  if (!pxe)
    return;

  /* In principle we could get the hostname from the DHCP packet, but it
   * looks like EFI doesn't offer an easy way to do that (we'd have to
   * parse DHCP options manually, at least) and in any case we'd need a
   * fallback for the case where no hostname is assigned. So just use the
   * IP address. XXX consider forming hostname according to our standard
   * naming scheme, i.e., the hw address with dashes between octets. */
  if (!hostname[0])
    SPrint (hostname, sizeof hostname,
            L"%d.%d.%d.%d",
            pxe->Mode->DhcpAck.Dhcpv4.BootpYiAddr[0],
            pxe->Mode->DhcpAck.Dhcpv4.BootpYiAddr[1],
            pxe->Mode->DhcpAck.Dhcpv4.BootpYiAddr[2],
            pxe->Mode->DhcpAck.Dhcpv4.BootpYiAddr[3]);

  /* form a syslog packet in buf; see RFC3164 */
  RT->GetTime (&when, NULL);

  SPrint (ubuf, sizeof ubuf, L"<133>" /* local0, info */
          "%s %d %02d:%02d:%02d " /* should be local time, but so what? */
          "%s "
          "vgaefi %s",
          months[when.Month], when.Day,
          when.Hour, when.Minute, when.Second, hostname, str);

  for (p = ubuf, q = buf; p < ubuf + NETLOG_MAXLEN && *p; ++p, ++q)
    {
      if (*p == L'\n' || *p == L'\r')
        *q = ' ';
      else
        *q = (UINT8) * p;       /* XXX */
    }

  len = q - buf;

  /* strip off trailing whitespace. */
  while (len > 0 && buf[len - 1] == ' ')
    --len;

  st = pxe->UdpWrite (pxe, 0, (EFI_IP_ADDRESS *) & broadcast,
                      (EFI_PXE_BASE_CODE_UDP_PORT *) & syslogport, NULL, NULL,
                      (EFI_PXE_BASE_CODE_UDP_PORT *) & syslogport, NULL, NULL,
                      &len, buf);

  if ((EFI_SUCCESS != st) && !saw_error)
    {
      Print
        (L"netlog: UdpWrite: %s; further such errors will not be reported\n",
         efi_strerror (st));
      saw_error = 1;
    }
}
