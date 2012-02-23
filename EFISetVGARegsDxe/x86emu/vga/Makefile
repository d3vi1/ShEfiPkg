#
#
#

include ../Make.defaults
include ../Make.rules

TOPDIR=$(CDIR)/..

FILES=crc32.o find_roms.o load_roms.o pci.o roms.o util.o main.o  vbetool/vbetool.o vbetool/thunk.o \
	x86emu/debug.o  x86emu/decode.o  x86emu/fpu.o  x86emu/ops.o  x86emu/ops2.o \
	x86emu/prim_ops.o  x86emu/sys.o vbetool/x86-common.o



TARGET=vga.o

all: $(TARGET)

$(TARGET): $(TOPDIR)/Make.defaults $(FILES)
	$(LD) -r -o $@ $(FILES)

clean:
	$(RM) -f $(TARGET) $(FILES)

