# Directories
OUTDIR := out
APPNAME := XT
SRCDIR := src
OBJDIR := obj
XBASEDIR := xbase

# GCC binaries
AS := human68k-gcc
CC := human68k-gcc
OBJCOPY := human68k-objcopy

# C build flags
CFLAGS := -std=c11 -O2 -Wall -Werror -I$(SRCDIR) -I$(XBASEDIR)
CFLAGS += -mcpu=68000
CFLAGS += -fomit-frame-pointer
CFLAGS += -frename-registers
CFLAGS += -fshort-enums
CFLAGS += -fconserve-stack
CFLAGS += -fwrapv
CFLAGS += -fno-gcse
CFLAGS += -fms-extensions
CFLAGS += -fno-web -fno-unit-at-a-time

CFLAGS += -DXB_DISPLAY_512PX_PCG_HACK

# TODO: Define X68000 memory map as extern uint8_t arrays and have linker
# properly point them to the correct locations.
# TODO: array-bounds and stringop-overflow are bugged on GCC 11.2.0
CFLAGS += -Wno-array-bounds -Wno-stringop-overflow

# Assembler flags
ASFLAGS := $(CFLAGS)
ASFLAGS += -Wa,-I$(SRCDIR) -Wa,-I$(OBJDIR) -Wa,-I$(XBASEDIR)
ASFLAGS += -x assembler-with-cpp
ASFLAGS += -Wa,-m68000
ASFLAGS += -Wa,--bitwise-or
ASFLAGS += -Wa,--register-prefix-optional

# Linker flags
LDFLAGS := -Wl,-q
LDFLAGS += -Wl,-Map=$(APPNAME).map

# TODO: use GCC's automatic deps generation?
SOURCES_H := $(shell find ./$(SRCDIR)/ -type f -name '*.h') $(shell find ./$(XBASEDIR)/ -type f -name '*.h')
SOURCES_ASM := $(shell find ./$(SRCDIR)/ -type f -name '*.a68') $(shell find ./$(XBASEDIR)/ -type f -name '*.a68')
SOURCES_C := $(shell find ./$(SRCDIR)/ -type f -name '*.c') $(shell find ./$(XBASEDIR)/ -type f -name '*.c')
RESOURCES := $(shell find ./$(RESDIR)/ -type f -name '*.rc')
OBJECTS_C := $(addprefix $(OBJDIR)/, $(SOURCES_C:.c=.o))
OBJECTS_ASM := $(addprefix $(OBJDIR)/, $(SOURCES_ASM:.a68=.o))

# Physical target information.
TARGET_DEV := /dev/disk/by-id/usb-x68k_DEVDISK_000000000000-0:1
TARGET_SEEK := 0

# -----------------------------------------------------------------------------

.PHONY: all clean resources $(OUTDIR)/$(APPNAME).X

all: $(OUTDIR)/$(APPNAME).X

clean:
	$(RM) $(OBJECTS_C) $(OBJECTS_ASM)
	rm -rf $(OBJDIR)
	rm -rf $(OUTDIR)

resources:
	@mkdir -p $(OUTDIR)
	@cp -r res/* $(OUTDIR)/

$(OUTDIR)/$(APPNAME).X: $(OBJECTS_C) $(OBJECTS_ASM) resources
	@bash -c 'printf "\t\e[94m[ LNK ]\e[0m $(OBJECTS_ASM) $(OBJECTS_C)\n"'
	$(CC) -o $(APPNAME).bin $(LDFLAGS) $(CFLAGS) $(OBJECTS_C) $(OBJECTS_ASM)
	@mkdir -p $(OUTDIR)
	$(OBJCOPY) -v -O xfile $(APPNAME).bin $(OUTDIR)/$(APPNAME).X > /dev/null
	@rm $(APPNAME).bin
	@bash -c 'printf "\e[92m\n\tBuild Complete. \e[0m\n\n"'

# On the X68000 side, run SUSIE.X <Drive>: -ID<SCSI ID>
# To find the SCSI ID, just run SUSIE.X without arguments.
upload: $(OUTDIR)/$(APPNAME).X
	mkdir -p target_mount
	-sudo umount $(TARGET_DEV)
	sudo mount $(TARGET_DEV) target_mount
	sudo rm -rf target_mount/*
	sudo cp -r $(OUTDIR)/* target_mount/
	sync
	sudo umount $(TARGET_DEV)

$(OUTDIR)/mo_image.mos: $(OUTDIR)/$(APPNAME).X resources
	mkdir -p mo_image
	unzip empty_mo.zip
	mv empty_mo.mos $(OUTDIR)/mo_image.mos
	sudo mount $(OUTDIR)/mo_image.mos mo_image
	sudo cp -r $(OUTDIR)/* mo_image/
	sync
	sudo umount mo_image
	sudo rm -r mo_image

image: $(OUTDIR)/mo_image.mos
	sudo dd if=mo_image.mos of=$(TARGET_DEV) bs=512 status=progress

$(OBJDIR)/%.o: %.c $(SOURCES_H)
	@mkdir -p $(OBJDIR)/$(<D)
	@bash -c 'printf "\t\e[96m[  C  ]\e[0m $<\n"'
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: %.a68 $(SOURCES_H)
	@mkdir -p $(OBJDIR)/$(<D)
	@bash -c 'printf "\t\e[95m[ ASM ]\e[0m $<\n"'
	$(AS) -c $(ASFLAGS) $< -o $@
