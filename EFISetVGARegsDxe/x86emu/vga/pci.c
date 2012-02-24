#include "project.h"
#include "io.h"

/* PCI config read/write */
uint8_t
conf1_read_8 (int bus, int devfn, int off)
{
  int addr = 0xcfc + (off & 3);
  out_32 (0x80000000 | (bus << 16) | (devfn << 8) | (off & ~3), 0x0cf8);
  return in_8 (addr);
}

uint16_t
conf1_read_16 (int bus, int devfn, int off)
{
  int addr = 0xcfc + (off & 3);
  out_32 (0x80000000 | (bus << 16) | (devfn << 8) | (off & ~3), 0x0cf8);
  return in_16 (addr);
}

uint32_t
conf1_read_32 (int bus, int devfn, int off)
{
  int addr = 0xcfc + (off & 3);
  out_32 (0x80000000 | (bus << 16) | (devfn << 8) | (off & ~3), 0x0cf8);
  return in_32 (addr);
}

void
conf1_write_8 (int bus, int devfn, int off, uint8_t val)
{
  int addr = 0xcfc + (off & 3);
  out_32 (0x80000000 | (bus << 16) | (devfn << 8) | (off & ~3), 0x0cf8);
  out_8 (val, addr);
}
