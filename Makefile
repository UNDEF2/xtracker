APPNAME := XT
OUTDIR := out
SRCDIR := src
RESDIR := res
OBJDIR := obj
XSP2LIBDIR := xsp2lib
XBASEDIR := xbase

# TODO: use GCC's automatic deps generation?
SOURCES_H := $(shell find ./$(SRCDIR)/ -type f -name '*.h') $(shell find ./$(XBASEDIR)/xbase/ -type f -name '*.h')

SOURCES_C := $(shell find ./$(SRCDIR)/ -type f -name '*.c') $(shell find ./$(XBASEDIR)/xbase/ -type f -name '*.c')
SOURCES_ASM := $(shell find ./$(SRCDIR)/ -type f -name '*.a68') $(shell find ./$(XBASEDIR)/xbase/ -type f -name '*.a68')

OBJECTS_C := $(addprefix $(OBJDIR)/, $(SOURCES_C:.c=.o))
OBJECTS_ASM := $(addprefix $(OBJDIR)/, $(SOURCES_ASM:.a68=.o))

# Physical target information.
TARGET_DEV := /dev/disk/by-id/usb-x68k_DEVDISK_000000000000-0:1

include $(XBASEDIR)/xb-rules.mk
# Can add anything to CFLAGS and ASFLAGS here with +=.

CFLAGS += -DXB_DISPLAY_512PX_PCG_HACK

mo_image.mos: $(OUTDIR)/$(APPNAME).X resources
	mkdir -p mo_mnt
	unzip empty_mo.zip
	mv empty_mo.mos $@
	sudo mount $@ mo_mnt
	sudo cp -r $(OUTDIR)/* mo_mnt/
	sync
	sudo umount mo_mnt
	sync
	sudo rmdir mo_mnt

image: mo_image.mos
	sudo dd if=$< of=$(TARGET_DEV) bs=512 status=progress
