#include "project.h"
#undef DEBUG

#define SHMERRORPTR (pointer)(-1)

#define _INT10_PRIVATE
#include "include/xf86int10.h"
#include "include/x86emu.h"
#include "include/xf86x86emu.h"
#include "include/lrmi.h"
#include "x86-common.h"

#define DEBUG
#define ALLOC_ENTRIES(x) (V_RAM - 1)
#undef TRUE
#undef FALSE
#define TRUE 1
#define FALSE 0


UINT8 *real_memory;


static struct LRMI_regs *regs;

static uint8_t read_b(int addr) {
	return *((uint8_t *)real_memory + addr);
}

static CARD8
vbe_inb(CARD16 port)
{
	CARD8 val;
	val = inb_local(port);
	return val;
}

static CARD16
vbe_inw(CARD16 port)
{
	CARD16 val;
	val = inw_local(port);
	return val;
}

static CARD32
vbe_inl(CARD16 port)
{
	CARD32 val;
	val = inl_local(port);
	return val;
}

static void
vbe_outb(CARD16 port, CARD8 val)
{
        outb_local(port, val);

}

static void
vbe_outw(CARD16 port, CARD16 val)
{
        outw_local(port, val);
}

static void vbe_outl(CARD16 port, CARD32 val)
{

	outl_local(port, val);
}


uint8_t X86API vbe_rdb(uint32_t addr)
{
	if (addr > M.mem_size - 1) {
		print("mem_read: address %#lx out of range!\n", addr);
		HALT_SYS();
		}
	if ((addr>=0xa0000) && (addr<=0xbffff))
		return *(uint8_t*)(addr);
	else
		return *(uint8_t*)(M.mem_base + addr);
}

uint16_t X86API vbe_rdw( uint32_t addr)
{
	if (addr > M.mem_size - 2) {
		print("mem_read: address %#lx out of range!\n", addr);
		HALT_SYS();
		}

	if ((addr>=0xa0000) && (addr<=0xbfffe))
		return *(uint16_t*)(addr);
	else
		return *(uint16_t*)(M.mem_base + addr);
}

uint32_t X86API vbe_rdl( uint32_t addr)
 {
	if (addr > M.mem_size - 4) {
		print("mem_read: address %#lx out of range!\n", addr);
		HALT_SYS();
		}

	if ((addr>=0xa0000) && (addr<=0xbfffc))
		return *(uint32_t*)(addr);
	else
		return *(uint32_t*)(M.mem_base + addr);
}

void X86API vbe_wrb( uint32_t addr, uint8_t val)
{
    if (addr > M.mem_size - 1) {
		print("mem_write: address %#lx out of range!\n", addr);
		HALT_SYS();
    }
    if ((addr>=0xa0000) && (addr<=0xbffff)) {
	*(uint8_t*)(addr) = val;
    }
    else
	*(uint8_t*)(M.mem_base + addr) = val;
}

void X86API vbe_wrw( uint32_t addr, uint16_t val)
{
	if (addr > M.mem_size - 2) {
		print("mem_write: address %#lx out of range!\n", addr);
		HALT_SYS();
		}
    if ((addr>=0xa0000) && (addr<=0xbffff)) {
	*(uint16_t*)(addr) = val;
    }
    else
	*(uint16_t*)(M.mem_base + addr) = val;
}

void X86API vbe_wrl( uint32_t addr, uint32_t val)
{
	if (addr > M.mem_size - 4) {
		print("mem_write: address %#lx out of range!\n", addr);
		HALT_SYS();
		}
    if ((addr>=0xa0000) && (addr<=0xbffff))
	 *(uint32_t*)(addr) = val;
    else
	 *(uint32_t*)(M.mem_base + addr) = val;
}


void pushw(uint16_t val)
{
	X86_ESP -= 2;
	MEM_WW(((u32) X86_SS << 4) + X86_SP, val);
}

static void x86emu_do_int(int num)
{
	u32 eflags;
		
        return;


	eflags = X86_EFLAGS;
	eflags = eflags | X86_IF_MASK;
	pushw(eflags);
	pushw(X86_CS);
	pushw(X86_IP);
	X86_EFLAGS = X86_EFLAGS & ~(X86_VIF_MASK | X86_TF_MASK);
	X86_CS = (read_b((num << 2) + 3) << 8) + read_b((num << 2) + 2);
	X86_IP = (read_b((num << 2) + 1) << 8) + read_b((num << 2));

}

int LRMI_init(void *RealModeMemory) {
	real_memory = RealModeMemory;
	X86EMU_intrFuncs intFuncs[256];
	void *stack;
 	int i;

	if (!LRMI_common_init())
		return 0;

	X86EMU_memFuncs memFuncs = {
		(&vbe_rdb),
		(&vbe_rdw),
		(&vbe_rdl),
		(&vbe_wrb),
		(&vbe_wrw),
		(&vbe_wrl)
	};
	
	X86EMU_setupMemFuncs(&memFuncs);

	
	X86EMU_pioFuncs pioFuncs = {
		(&vbe_inb),
		(&vbe_inw),
		(&vbe_inl),
		(&vbe_outb),
		(&vbe_outw),
		(&vbe_outl)
	};
	
	X86EMU_setupPioFuncs(&pioFuncs);

	for (i=0;i<256;i++)
		intFuncs[i] = x86emu_do_int;
	X86EMU_setupIntrFuncs(intFuncs);
	
	X86_EFLAGS = X86_IF_MASK | X86_IOPL_MASK;

	/*
	 * Allocate a 64k stack.
	 */
	stack = LRMI_alloc_real(64 * 1024);
	X86_SS = (unsigned int) stack >> 4;
	X86_ESP = 0xFFFE;

	M.mem_base = (uint32_t ) real_memory;
	M.mem_size = 1024*1024;

	return 1;
}

int real_call(struct LRMI_regs *registers) {
        regs = registers;

        X86_EAX = registers->eax;
        X86_EBX = registers->ebx;
        X86_ECX = registers->ecx;
        X86_EDX = registers->edx;
        X86_ESI = registers->esi;
        X86_EDI = registers->edi;
        X86_EBP = registers->ebp;
        X86_EIP = registers->ip;
        X86_ES = registers->es;
        X86_FS = registers->fs;
        X86_GS = registers->gs;
        X86_CS = registers->cs;

        if (registers->ss != 0) {
                X86_SS = registers->ss;
        }
	if (registers->ds != 0) { 
		X86_DS = registers->ds;
	}
	if (registers->sp != 0) {
		X86_ESP = registers->sp;
	}

	X86EMU_exec();

	registers->eax = X86_EAX;
	registers->ebx = X86_EBX;
	registers->ecx = X86_ECX;
	registers->edx = X86_EDX;
	registers->esi = X86_ESI;
	registers->edi = X86_EDI;
	registers->ebp = X86_EBP;
	registers->es = X86_ES;

	return 1;
}

int LRMI_int(int num, struct LRMI_regs *registers) {
	u32 eflags;
	eflags = X86_EFLAGS;
	eflags = eflags | X86_IF_MASK;
	X86_EFLAGS = X86_EFLAGS  & ~(X86_VIF_MASK | X86_TF_MASK | X86_IF_MASK | X86_NT_MASK);

	registers->cs = (read_b((num << 2) + 3) << 8) + read_b((num << 2) + 2);
	registers->ip = (read_b((num << 2) + 1) << 8) + read_b((num << 2));
	regs = registers;
	return real_call(registers);
}

int LRMI_call(struct LRMI_regs *registers) {
	return real_call(registers);
}

size_t
LRMI_base_addr(void)
{
	return (size_t)M.mem_base;
}



void wacky_trace_code(void)
{
#if 0
static uint16_t old_cs,old_ip;


old_cs=M.x86.R_CS;
old_ip=M.x86.R_IP;
#endif
}
