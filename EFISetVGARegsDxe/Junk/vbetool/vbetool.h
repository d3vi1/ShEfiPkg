#ifndef __VBETOOL_H__
#define __VBETOOL_H__

typedef struct LRMI_regs reg_frame;

int do_vbe_service(unsigned int AX, unsigned int BX, reg_frame *regs);
int do_real_post(unsigned pci_device);
int do_blank(int state);
int do_set_mode (int mode, int vga);
int do_get_mode(void);
int do_post(void);
void restore_state(void);
UINT8 * save_state(int *);
void text_mode(void);
int check_console(void);
int enable_vga(void);
int disable_vga(void);

#endif /* __VBETOOL_H__ */
