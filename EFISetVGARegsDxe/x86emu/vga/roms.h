#ifndef MAX_ROM_SIG_LEN
#define MAX_ROM_SIG_LEN 0x100
#endif

struct rom
{
	unsigned char sig[MAX_ROM_SIG_LEN];
	int sig_len;

	int sig_offset;
	int rom_len;

	unsigned int checksum;
	int load_offset;

	unsigned char *base;

	unsigned char *compare;
};
/* video.c */
extern struct rom video_rom;
