/*
 * vga.c:
 * EFI VGA patch.
 *
 * Copyright (c) 2006 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#include "project.h"
#include "elilo.h"

static const char rcsid[] = "$Id:";


/* efi_main
 * Entry point. */
void
vga_init (boot_params_t * bp)
{

  Print (L"EFI legacy vga patch (c) 2006 James McKenzie\n");
  LogPrint (L"EFI legacy vga patch (c) 2006 James McKenzie\n");
  LogPrint (L"    %a\n", rcsid);

  LogPrint (L"    params at 0x%08x\n", (uint32_t) bp);

  /*Switch off the second function of the VGA card */
  /*the 2nd interface is not used or needed and */
  /*confuses other operating systems, OS-X */
  /*doesn't seem to mind */

  conf1_write_8 (0, 0, 0x54, 0x89);

  real_memory = alloc_buf (1024 * 1024);
  memset (real_memory, 0, 1024 * 1024);

  if (find_roms ())
    {
      LogPrint (L"Failed to find a VGA ROM so can't initialize it.\n");
      LogPrint (L"sleeping for 30 seconds\n");

      util_sleep (30);
      return;
    }

  load_roms_to_address (real_memory);
  LRMI_init ();

  /* Stop using EFI's Print from now on */
  safe_to_use_efi = 0;

  LogPrint (L"Calling POST\n");
  do_real_post (((1 << 8) | (0 << 3) | (0 & 0x7)) & 0x0000FFFF);
  LogPrint (L"done\n");

  LogPrint (L"Calling set_mode(3,1)\n");
  do_set_mode (3, 1);

  /*Copy the roms from our emulator into the system */
  copy_roms_to_system ();

  bios_set_active_page (0x0);
  bios_move_cursor (0, 0);
  LogPrint (L"done\n");

  /* Call into the real mode bios to write to the screen now */
  safe_to_use_bios = 1;
  LogPrint (L"VGAEFI - VGA now initialized\n");

  {
    int have_vga, video_ega_bx = 0;
    int cursor_pos = 0;
    int video_page = 0;
    int video_mode = 0;
    int font_points = 0;
    int video_cols = 0;
    int video_lines = 0;

    linux_basic_detect (&have_vga, &video_ega_bx);
    LogPrint (L"linux basic detect: have_vga=0x%x video_ega_bx=0x%x\n",
              have_vga, video_ega_bx);

    linux_mode_params (&cursor_pos, &video_page, &video_mode, &font_points,
                       &video_cols, &video_lines);

    LogPrint
      (L"linux mode params: cursor_pos=0x%x video_page=0x%x video_mode=0x%x font_points=0x%x video_cols=0x%x video_lines=0x%x\n",
       cursor_pos, video_page, video_mode, font_points, video_cols,
       video_lines);


    have_vga = 1;

    bp->s.orig_cursor_col = cursor_pos & 0xff;
    bp->s.orig_cursor_row = cursor_pos >> 8;
    bp->s.orig_video_page = video_page;
    bp->s.orig_video_mode = video_mode;
    bp->s.orig_video_cols = video_cols;
    bp->s.orig_video_rows = video_lines + 1;

    bp->s.orig_ega_bx = video_ega_bx;
    bp->s.is_vga = have_vga;
    bp->s.orig_video_points = font_points;

  }
  safe_to_use_bios = 0;

  free_buf (real_memory);
}
