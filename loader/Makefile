#
# Makefile for Broadcom BCM947XX boards
#
# Copyright 2001-2003, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# Copyright 2004  Manuel Novoa III <mjn3@codepoet.org>
#   Modified to support bzip'd kernels.
#   Of course, it would be better to integrate bunzip capability into CFE.
#
# Copyright 2005  Oleg I. Vdovikin <oleg@cs.msu.su>
#   Cleaned up, modified for lzma support, removed from kernel
#
# 2009-10-05 - Upgrade to lzma SDK 4.57 by Leonid Lisovskiy
#

TEXT_START	:= 0x80001000
BZ_TEXT_START	:= 0x80500000

OBJCOPY		:= $(CROSS_COMPILE)objcopy -O binary -R .reginfo -R .note -R .comment -R .mdebug -S

CFLAGS		= -D__KERNEL__ -Wall -Wstrict-prototypes -Wno-trigraphs -O2 \
		  -fno-strict-aliasing -fno-common -fomit-frame-pointer -G 0 -mno-abicalls -fno-pic \
		  -ffunction-sections -pipe -Wa,--trap \
		  -mabi=32 -march=mips32 -Wa,-32 -Wa,-march=mips32 -Wa,-mips32

CFLAGS		+= -DLOADADDR=$(TEXT_START) -D_LZMA_IN_CB

ASFLAGS		= $(CFLAGS) -D__ASSEMBLY__ -DBZ_TEXT_START=$(BZ_TEXT_START)

SEDFLAGS	:= s/BZ_TEXT_START/$(BZ_TEXT_START)/;s/TEXT_START/$(TEXT_START)/

OBJECTS		:= head.o data.o

COBJECTS	:= decompress.o LzmaDecode.o

CFLAGS		+= -I$(LZMAPATH) -I$(SRCBASE)/include
vpath %.c $(LZMAPATH)

all: loader.gz

# Don't build dependencies, this may die if $(CC) isn't gcc
dep:

install:

vmlinuz-install:
	$(OBJCOPY) $(LINUXDIR)/vmlinux piggy
	$(LZMA) e piggy $(DESTDIR)/vmlinuz -eos -lc1 -lp2 -pb2

loader.gz: loader
	gzip -nc9 $< > $@

loader: loader.o
	$(OBJCOPY) $< $@

loader.o: loader.lds $(OBJECTS)
	$(LD) -static --gc-sections -no-warn-mismatch -T loader.lds -o $@ $(OBJECTS)

loader.lds: loader.lds.in Makefile
	@sed "$(SEDFLAGS)" < $< > $@

data.o: data.lds decompress.image
	$(LD) -no-warn-mismatch -T data.lds -r -o $@ -b binary decompress.image -b elf32-tradlittlemips

decompress.image: decompress
	$(OBJCOPY) $< $@

decompress: decompress.lds $(COBJECTS)
	$(LD) -static --gc-sections -no-warn-mismatch -T decompress.lds -o $@ $(COBJECTS)

decompress.lds: decompress.lds.in Makefile
	@sed "$(SEDFLAGS)" < $< > $@

mrproper: clean

clean:
	rm -f loader.gz loader decompress loader.lds decompress.lds *.o *.image piggy
