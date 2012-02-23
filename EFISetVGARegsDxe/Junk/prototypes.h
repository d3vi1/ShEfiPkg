/* main.c */
extern EFI_SYSTEM_TABLE *systab;
/* util.c */
extern CHAR16 log_print_buf[4096];
extern CHAR16 *efi_strerror(EFI_STATUS st);
extern int safe_to_use_bios;
extern int safe_to_use_efi;
extern void do_log_print(CHAR16 *msg);
extern void *alloc_buf(UINTN size);
extern void free_buf(void *buf);
extern int util_strlen(CHAR8 *in);
extern void utf16_to_utf8(CHAR8 *out, CHAR16 *in);
extern INTN util_memcmp(void *A, void *B, UINTN len);
extern void util_wstrcpy(CHAR16 *dst, CHAR16 *src);
extern INTN memcpy(void *A, void *B, UINTN len);
extern void util_sleep(int s);
extern void usleep(int s);
extern EFI_LOADED_IMAGE *get_efi_info(void);
extern CHAR16 *efi_device_to_str(EFI_HANDLE h);
extern char i_to_xdigit(int i);
extern void *memset(void *_a, int c, size_t len);
extern void *memmove(void *dst, void *src, size_t len);
extern void util_memcpy(void *dst, void *src, size_t len);
/* netlog.c */
extern void netlog(const CHAR16 *str);
/* pci.c */
extern uint8_t conf1_read_8(int bus, int devfn, int off);
extern uint16_t conf1_read_16(int bus, int devfn, int off);
extern uint32_t conf1_read_32(int bus, int devfn, int off);
extern void conf1_write_8(int bus, int devfn, int off, uint8_t val);
/* pxe.c */
extern EFI_PXE_BASE_CODE *get_efi_pxe(void);
/* net.c */
extern EFI_HANDLE get_network_handle(void);
extern EFI_DEVICE_PATH *find_network_dp(void);
extern int get_mac_from_dp(EFI_DEVICE_PATH *dp, uint8_t *mac);
extern EFI_DEVICE_PATH *get_pxe_file_dp(void);
/* vbetool/vbetool.c */
extern int do_vbe_service(unsigned int AX, unsigned int BX, reg_frame *regs);
extern int do_real_post(unsigned pci_device);
extern int do_blank(int state);
extern int do_set_mode(int mode, int vga);
extern int do_get_mode(void);
extern int enable_vga(void);
extern int disable_vga(void);
extern void bios_move_cursor(int col, int row);
extern void bios_write_char(int c);
extern void bios_set_active_page(int c);
extern void linux_basic_detect(int *have_vga, int *video_ega_bx);
extern void linux_mode_params(int *cursor_pos, int *video_page, int *video_mode, int *font_points, int *video_cols, int *video_lines);
/* vbetool/thunk.c */
extern uint8_t *real_memory;
extern uint8_t vbe_rdb(uint32_t addr);
extern uint16_t vbe_rdw(uint32_t addr);
extern uint32_t vbe_rdl(uint32_t addr);
extern void vbe_wrb(uint32_t addr, uint8_t val);
extern void vbe_wrw(uint32_t addr, uint16_t val);
extern void vbe_wrl(uint32_t addr, uint32_t val);
extern void pushw(uint16_t val);
extern int LRMI_init(void);
extern int real_call(struct LRMI_regs *registers);
extern int LRMI_int(int num, struct LRMI_regs *registers);
extern int LRMI_call(struct LRMI_regs *registers);
extern size_t LRMI_base_addr(void);
extern void wacky_trace_code(void);
/* load_roms.c */
extern void load_roms_to_address(uint8_t *addr);
extern void copy_roms_to_system(void);
/* find_roms.c */
extern int find_rom(struct rom *rom);
extern int find_roms(void);
/* crc32.c */
extern unsigned int crc32(const unsigned char *buf, unsigned int size);
/* efistate.c */
extern int save_efi_video_state(void);
extern int restore_efi_video_state(void);
