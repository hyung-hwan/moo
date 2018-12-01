#
# Intel 386(tm)/486(tm) C Code Builder(tm) Kit, Version 1.0 
#

# /zmod486 - 80486 instructions
# /zfloatsync - FPU is operand-synchronized with the CPU
# /m create map file
# /g produce debug info
###CFLAGS :=  /D__DOS__ /DMOO_ENABLE_STATIC_MODULE /DNDEBUG 
CFLAGS :=  /D__DOS__ /DMOO_ENABLE_STATIC_MODULE  /DMOO_BUILD_DEBUG
LDFLAGS := /xnovm /xregion=12m

LIBSRCS := \
	bigint.c \
	comp.c \
	debug.c  \
	decode.c \
	dic.c \
	err.c \
	exec.c \
	gc.c \
	heap.c \
	logfmt.c \
	moo.c \
	obj.c \
	pf.c \
	rbt.c \
	std.c \
	sym.c \
	utf16.c \
	utf8.c \
	utl.c

EXESRCS := \
	opt.c \
	main.c

LIBOBJS := $(LIBSRCS:.c=.obj)
EXEOBJS := $(EXESRCS:.c=.obj)

LIBFILE=moo.lib
EXEFILE=moo.exe
MODFILE=..\mod\moomod.lib
LIBRSPFILE := $(LIBFILE,B,S/.*/&.RSP/)
EXERSPFILE := $(EXEFILE,B,S/.*/&.RSP/)

all: lib exe

# hlt.obj should be included into the library 
# but for an unknown reason, the linker was not able to
# find the _halt_cpu symbol when halt.obj was added to
# the library. i suspect 386asm not producing a fully 
# compatiable object file.

exe: hlt.obj $(EXEOBJS) 
	echo $(EXEOBJS) > $(EXERSPFILE)
	echo hlt.obj >> $(EXERSPFILE)
	echo $(LIBFILE) >> $(EXERSPFILE)
	echo $(MODFILE) >> $(EXERSPFILE)
	echo $(LDFLAGS) >> $(EXERSPFILE)
	$(CC) @$(EXERSPFILE) /e $(EXEFILE)

lib: $(LIBOBJS)
	echo > $(LIBRSPFILE)
	!foreach x $(.NEWER) 
			modname /r $x >> $(LIBRSPFILE)
	!end
	echo compress >> $(LIBRSPFILE)
	echo update >> $(LIBRSPFILE)
	echo quit exit >> $(LIBRSPFILE)
	del $(LIBFILE)
	lib32 $(LIBFILE) batch < $(LIBRSPFILE)

hlt.obj: hlt.asm
	386asm -twocase hlt.asm
