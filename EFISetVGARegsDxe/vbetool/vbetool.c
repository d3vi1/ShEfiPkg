/*
Run video BIOS code for various purposes

Copyright Matthew Garrett <mjg59@srcf.ucam.org>, heavily based on
vbetest.c from the lrmi package and read-edid.c by John Fremlin

This program is released under the terms of the GNU General Public License,
version 2
*/

#include "project.h"
#include "io.h"

#include "include/lrmi.h"
#include "vbetool.h"

#define access_ptr_register(reg_frame,reg) (reg_frame -> reg)
#define access_seg_register(reg_frame,es) reg_frame.es
#define real_mode_int(interrupt,reg_frame_ptr) !LRMI_int(interrupt,reg_frame_ptr)

#define DPMS_STATE_ON 0x0000
#define DPMS_STATE_STANDBY 0x0100
#define DPMS_STATE_SUSPEND 0x0200
#define DPMS_STATE_OFF 0x0400
#define DPMS_STATE_LOW 0x0800


extern int LRMI_int(int num, struct LRMI_regs *registers);
extern int LRMI_call(struct LRMI_regs *registers);

#if 0
static struct pci_access *pacc;

int main(int argc, char *argv[])
{
	if (!LRMI_init()) {
		fprintf(stderr, "Failed to initialise LRMI.\n");
		return 1;
	}

	ioperm(0, 1024, 1);
	iopl(3);

	pacc = pci_alloc();
	pacc->numeric_ids = 1;
	pci_init(pacc);

	if (argc < 2) {
		goto usage;
	} else if (!strcmp(argv[1], "vbestate")) {
		/* VBE save/restore tends to break when done underneath X */
		int err = check_console();

		if (err) {
			return err;
		}

		if (!strcmp(argv[2], "save")) {
			save_state();
		} else if (!strcmp(argv[2], "restore")) {
			restore_state();
		} else {
			goto usage;
		}
	} else if (!strcmp(argv[1], "dpms")) {
		if (!strcmp(argv[2], "on")) {
			return do_blank(DPMS_STATE_ON);
		} else if (!strcmp(argv[2], "suspend")) {
			return do_blank(DPMS_STATE_SUSPEND);
		} else if (!strcmp(argv[2], "standby")) {
			return do_blank(DPMS_STATE_STANDBY);
		} else if (!strcmp(argv[2], "off")) {
			return do_blank(DPMS_STATE_OFF);
		} else if (!strcmp(argv[2], "reduced")) {
			return do_blank(DPMS_STATE_LOW);
		} else {
			goto usage;
		}
	} else if (!strcmp(argv[1], "vbemode")) {
		if (!strcmp(argv[2], "set")) {
			return do_set_mode(atoi(argv[3]),0);
		} else if (!strcmp(argv[2], "get")) {
			return do_get_mode();
		} else {
			goto usage;
		}
	} else if (!strcmp(argv[1], "vgamode")) {
		if (!strcmp(argv[2], "set")) {
			return do_set_mode(atoi(argv[3]),1);
		} else {
			return do_set_mode(atoi(argv[2]),1);
		}
	} else if (!strcmp(argv[1], "post")) {
		/* Again, we don't really want to do this while X is in 
		   control */
		int err = check_console();

		if (err) {
			return err;
		}

		return do_post();
	} else if (!strcmp(argv[1], "vgastate")) {
		if (!strcmp(argv[2], "on")) {
			return enable_vga();
		} else {
			return disable_vga();
		}
	} else {
	      usage:
		fprintf(stderr,
			"%s: Usage %s [[vbestate save|restore]|[vbemode set|get]|[vgamode]|[dpms on|off|standby|suspend|reduced]|[post]|[vgastate on|off]]\n",
			argv[0], argv[0]);
		return 1;
	}

	return 0;
}
#endif

int do_vbe_service(unsigned int AX, unsigned int BX, reg_frame * regs)
{
	const unsigned interrupt = 0x10;
	unsigned function_sup;
	unsigned success;
	regs->ds = 0x0040;

	access_ptr_register(regs, eax) = AX;
	access_ptr_register(regs, ebx) = BX;

	if (real_mode_int(interrupt, regs)) {
		print("Error: something went wrong performing real mode interrupt\n");
		return -1;
	}

	AX = access_ptr_register(regs, eax);

	function_sup = ((AX & 0xff) == 0x4f);
	success = ((AX & 0xff00) == 0);

	if (!success)
		return -2;
	if (!function_sup)
		return -3;

	return access_ptr_register(regs, ebx);
}

int do_real_post(unsigned pci_device)
{
	int error = 0;
	struct LRMI_regs r;
	memset(&r, 0, sizeof(r));

	/* Several machines seem to want the device that they're POSTing in
	   here */
	r.eax =  pci_device;

	/* 0xc000 is the video option ROM.  The init code for each
	   option ROM is at 0x0003 - so jump to c000:0003 and start running */
	r.cs = 0xc000;
	r.ip = 0x0003;

	/* This is all heavily cargo culted but seems to work */
	r.edx = 0x80;
	r.ds = 0x0040;

	print("Calling into the bios\n");

	if (!LRMI_call(&r)) {
		print("Error: something went wrong performing real mode call\n");
		error = 1;
	}
	print("Back from the bios with code %x\n",error);

	return error;
}

#if 0
int do_post()
{
	unsigned int c;
	unsigned int pci_id;
	int error;

	pci_scan_bus(pacc);

	for (p = pacc->devices; p; p = p->next) {
		c = pci_read_word(p, PCI_CLASS_DEVICE);
		if (c == 0x300) {
			pci_id =
			    (p->bus << 8) + (p->dev << 3) +
			    (p->func & 0x7);
			error = do_real_post(pci_id);
			if (error != 0) {
				return error;
			}
		}
	}
	return 0;
	return do_real_post(0);
}
#endif

#if 0
void restore_state()
{
	struct LRMI_regs r;

	char *data = NULL;
	char tmpbuffer[524288];
	int i, length = 0;

	/* We really, really don't want to fail to read the entire set */
	while ((i = read(0, tmpbuffer + length, sizeof(tmpbuffer)-length))) {
		if (i == -1) {
			if (errno != EAGAIN && errno != EINTR) {
				perror("Failed to read state - ");
				return;
			}
		} else {
			length += i;
		}
	}

	data = LRMI_alloc_real(length);
	memcpy(data, tmpbuffer, length);

	/* VGA BIOS mode 3 is text mode */
	do_set_mode(3,1);

	memset(&r, 0, sizeof(r));

	r.eax = 0x4f04;
	r.ecx = 0xf;		/* all states */
	r.edx = 2;		/* restore state */
	r.es = (unsigned int) (data - LRMI_base_addr()) >> 4;
	r.ebx = (unsigned int) (data - LRMI_base_addr()) & 0xf;
	r.ds = 0x0040;

	if (!LRMI_int(0x10, &r)) {
		fprintf(stderr,
			"Can't restore video state (vm86 failure)\n");
	} else if ((r.eax & 0xffff) != 0x4f) {
		fprintf(stderr, "Restore video state failed\n");
	}

	LRMI_free_real(data);

	ioctl(0, KDSETMODE, KD_TEXT);
}
#endif

#if 0
uint8_t *save_state(int *len)
{
	struct LRMI_regs r;
	unsigned int size;
	uint8_t *ret=NULL;
	extern uint8_t *real_memory;

	memset(&r, 0, sizeof(r));

	r.eax = 0x4f04;
	r.ecx = 0xf;		/* all states */
	r.edx = 0;		/* get buffer size */
	r.ds = 0x0040;

	if (!LRMI_int(0x10, &r)) {
		print( "Can't get video state buffer size (vm86 failure)\n");
		return NULL;
	}
	memset(real_memory+0x502,0,0xa0000-0x502);

	if ((r.eax & 0xffff) != 0x4f) {
		print( "Get video state buffer size failed\n");
		return NULL;
	}

	size = (r.ebx & 0xffff) * 64;

	print("state buffer %d\n",size);

	memset(&r, 0, sizeof(r));

	r.eax = 0x4f04;
	r.ecx = 0xf;		/* all states */
	r.edx = 1;		/* save state */
	
	r.es =  0x2000;
	r.ebx = 0x0000;
	r.ds = 0x0040;

	print("ES: 0x%04X EBX: 0x%04X\n", r.es, r.ebx);


//	x86emu_dump_regs();
	print("Doing call\n");

#if 0
	if (!LRMI_int(0x10, &r)) {
		print("Can't save video state (vm86 failure)\n");
		x86emu_dump_regs();
		memset(real_memory+0x502,0,0xa0000-0x502);
		return NULL;
	}
#endif

//	x86emu_dump_regs();

	if ((r.eax & 0xffff) != 0x4f) {
		print("Save video state failed\n");
		memset(real_memory+0x502,0,0xa0000-0x502);
		return NULL;
	}

#if 0
	ret=alloc_buf(size);

	memcpy(ret,
		real_memory + (uint32_t) buffer,
		size);

 	if (len) *len=size;
#endif
	memset(real_memory+0x502,0,0xa0000-0x502);

	return ret;
}
#endif

int do_blank(int state)
{
	reg_frame regs;
	int error;

	memset(&regs, 0, sizeof(regs));
	error = do_vbe_service(0x4f10, state |= 0x01, &regs);
	if (error<0) {
		return error;
	}
	return 0;
}

int do_set_mode (int mode, int vga) {
	reg_frame regs;
	int error;

	memset(&regs, 0, sizeof(regs));

	if (vga) {
		error = do_vbe_service(mode, 0, &regs);
	} else {
		error = do_vbe_service(0x4f02, mode, &regs);
	}

	if (error<0) {
		return error;
	}
	
	return 0;
}

int do_get_mode() {
	reg_frame regs;
	int error;

	memset(&regs, 0, sizeof(regs));
	error = do_vbe_service(0x4f03, 0, &regs);

	if (error<0) {
		return error;
	}
	
	print("%d\n",error);
	return 0;
}


int enable_vga() {
	out_8(0x03 | in_8(0x3CC),  0x3C2);
	out_8(0x01 | in_8(0x3C3),  0x3C3);
	out_8(0x08 | in_8(0x46e8), 0x46e8);
	out_8(0x01 | in_8(0x102),  0x102);
	return 0;
}

int disable_vga() {
	out_8(~0x03 & in_8(0x3CC),  0x3C2);
	out_8(~0x01 & in_8(0x3C3),  0x3C3);
	out_8(~0x08 & in_8(0x46e8), 0x46e8);
	out_8(~0x01 & in_8(0x102),  0x102);
	return 0;
}

void bios_move_cursor(int col,int row)
{
	struct LRMI_regs r;

	memset(&r, 0, sizeof(r));

	r.eax = 0x0200;
	r.ebx = 0;		
	r.edx = (row <<8)| col;	
	r.ds = 0x0040;

	if (!LRMI_int(0x10, &r)) {
		print("int 0x10 0x0300 failed");
	}
}

void bios_write_char(int c)
{
	struct LRMI_regs r;

	memset(&r, 0, sizeof(r));

	r.eax = 0x0e00 | c;
	r.ebx = 0x0007;		
	r.ds = 0x0040;

	if (!LRMI_int(0x10, &r)) {
		print("int 0x10 0x0e00 failed");
	}
}

void bios_set_active_page(int c)
{
	struct LRMI_regs r;

	memset(&r, 0, sizeof(r));

	r.eax = 0x0500 | c;
	r.ds = 0x0040;

	if (!LRMI_int(0x10, &r)) {
		print("int 0x10 0x0500 failed");
	}
}


void linux_basic_detect(int *have_vga,int *video_ega_bx)
{
	struct LRMI_regs r;

	memset(&r, 0, sizeof(r));

  	*have_vga=0;

	r.eax = 0x1200;
        r.ebx=  0x0010;
	r.ds = 0x0040;

	LRMI_int(0x10, &r);

	*video_ega_bx=r.ebx &0xffff;

	if ((r.ebx &0xff)==0x10) return;
	

        r.eax=0x1a00;
	LRMI_int(0x10, &r);
	
	if ((r.eax & 0xff)==0x1a) {
		*have_vga++;
	}
}


void linux_mode_params(int *cursor_pos,int *video_page,int *video_mode,int *font_points,int *video_cols,int *video_lines)
{
	struct LRMI_regs r;

	memset(&r, 0, sizeof(r));

	r.eax=0x0300;
	r.ebx=0x0;
	
	LRMI_int(0x10, &r);

	*cursor_pos=r.edx;

	r.eax=0x0f00;
	LRMI_int(0x10, &r);

	*video_page=r.ebx;
	*video_mode=r.eax & 0xff;

	*font_points=vbe_rdw(0x485);
        *video_lines=vbe_rdb(0x484);
	*video_cols=80; //FIXME:

}

