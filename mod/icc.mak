#
# Intel 386(tm)/486(tm) C Code Builder(tm) Kit, Version 1.0 
#

CFLAGS := /D__DOS__ /DMOO_ENABLE_STATIC_MODULE /DNDEBUG /I..\lib

SRCS := \
	_con.c \
	ffi.c \
	stdio.c

OBJS := $(SRCS:.c=.obj)

MODFILE=moomod.lib
RSPFILE := $(MODFILE,B,S/.*/&.RSP/)

all: $(OBJS)
	echo > $(RSPFILE)
	!foreach x $(.NEWER) 
		modname /r $x >> $(RSPFILE)
	!end
	echo compress >> $(RSPFILE)
	echo update >> $(RSPFILE)
	echo quit exit >> $(RSPFILE)
	del $(MODFILE)
	lib32 $(MODFILE) batch < $(RSPFILE)
