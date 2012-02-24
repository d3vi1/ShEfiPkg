/*
 * video.c:
 * load the BIOS and VIDEO roms and data structures.
 *
 */

static const char rcsid[] =
  "$Id: load_roms.c,v 1.1 2006/05/27 18:41:14 root Exp $";

#include "project.h"

/* set_always_power_on
 * Configure the machine to always power up when power is available. */


#define bios_tbl_len 258


#if 0
void
load_rom (char *name, int base, int len)
{
  void *buf;
  int img_size;

  buf = alloc_buf (len);
  if (!buf)
    {
      print ("    Buffer allocate failed");
      return;
    }

  if (-1 == (img_size = fetch_file (name, buf, ELILO_BUFFER_LEN)))
    {
      print ("    failed to load rom %a", name);
      return;
    }

  print ("    installing rom\n");

  memcpy ((void *) base, buf, len);

  free_buf (buf);
}
#endif




void
load_roms_to_address (uint8_t * addr)
{
  print ("        loading roms at address %x\n", (int) addr);

  /*Video Rom C000:0000 - D000:FFFF */
  memcpy ((void *) (addr + 0xc0000) + video_rom.load_offset,
          video_rom.base, video_rom.rom_len);

  /*System Rom E000:0000 - F000:FFFF */
  memset ((void *) (addr + 0xe0000), 0, 0x20000);

  /*Interrupt table */
  memset ((void *) (addr + 0x0), 0, 0x400);

  /*Bios parameters table */
  //memcpy((void *) (addr+0x400),bios_tbl,bios_tbl_len);
  memset ((void *) (addr + 0x400), 0, bios_tbl_len);

}


void
copy_roms_to_system (void)
{
  /*Map RW ram under the system roms */

  print ("Copying ROM images into \"ROMS\"\n");
  print ("    Mapping RAM under ROMs\n");

  conf1_write_8 (0, 0, 0x90, 0x30);
  conf1_write_8 (0, 0, 0x91, 0x33);
  conf1_write_8 (0, 0, 0x92, 0x33);
  conf1_write_8 (0, 0, 0x93, 0x33);
  conf1_write_8 (0, 0, 0x94, 0x33);
  conf1_write_8 (0, 0, 0x95, 0x33);
  conf1_write_8 (0, 0, 0x96, 0x33);
  conf1_write_8 (0, 0, 0x97, 0x00);

  print ("    Copying rom code\n");

  /*Interrupt table */
  memcpy ((void *) (0x0), real_memory, 0x400);
  /*BIOS table */
  memcpy ((void *) (0x400), real_memory + 0x400, bios_tbl_len);
  /*Video rom */
  memcpy ((void *) (0xc0000), real_memory + 0xc0000, 0x20000);
  /*System rom */
  memcpy ((void *) (0xe0000), real_memory + 0xe0000, 0x20000);

  print ("    Setting RAM under ROMs readonly\n");
  conf1_write_8 (0, 0, 0x90, 0x10);
  conf1_write_8 (0, 0, 0x91, 0x11);
  conf1_write_8 (0, 0, 0x92, 0x11);
  conf1_write_8 (0, 0, 0x93, 0x11);
  conf1_write_8 (0, 0, 0x94, 0x11);

  print ("    Done\n");

}
