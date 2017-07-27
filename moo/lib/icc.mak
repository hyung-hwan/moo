#
# Intel 386(tm)/486(tm) C Code Builder(tm) Kit, Version 1.0 
#

# /zmod486 - 80486 instructions
# /zfloatsync - FPU is operand-synchronized with the CPU
# /m create map file
# /g produce debug info
CFLAGS :=  /D__DOS__ /DMOO_ENABLE_STATIC_MODULE /DNDEBUG 
LDFLAGS := /xnovm /xregion=12m

SRCS := \
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
	rbt.c \
	sym.c \
	utf8.c \
	utl.c \
	main.c \

OBJS := $(SRCS:.c=.obj)

EXEFILE=moo.exe
MODFILE=..\mod\moomod.lib
RSPFILE := $(EXEFILE,B,S/.*/&.RSP/)


all: $(OBJS) hlt.obj
	echo $(OBJS) > $(RSPFILE)
	echo hlt.obj >> $(RSPFILE)
	echo $(MODFILE) >> $(RSPFILE)
	echo $(LDFLAGS) >> $(RSPFILE)
	$(CC) @$(RSPFILE) /e $(EXEFILE)

hlt.obj: hlt.asm
	386asm -twocase hlt.asm
