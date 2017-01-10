#
# Intel 386(tm)/486(tm) C Code Builder(tm) Kit, Version 1.0 
#

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
	proc.c \
	rbt.c \
	sym.c \
	utf8.c \
	utl.c \
	main.c \

OBJS := $(SRCS:.c=.obj)

EXEFILE=moo.exe
MODFILE=..\mod\moomod.lib
RSPFILE := $(EXEFILE,B,S/.*/&.RSP/)

all: $(OBJS)
	echo $(OBJS) > $(RSPFILE)
	echo $(MODFILE) >> $(RSPFILE)
	echo $(LDFLAGS) >> $(RSPFILE)
	$(CC) @$(RSPFILE) /e $(EXEFILE)
