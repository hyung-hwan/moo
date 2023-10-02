AUTOMAKE_OPTIONS = nostdinc

CPPFLAGS_COMMON = \
	-I$(abs_builddir) \
	-I$(abs_srcdir) \
	-I$(includedir)

CFLAGS_COMMON =

if ENABLE_STATIC_MODULE
LDFLAGS_COMMON = -L$(abs_builddir)/../mod -L$(abs_builddir) -L$(libdir)
else
LDFLAGS_COMMON = -L$(abs_builddir) -L$(libdir)
endif

LDFLAGS_COMMON += -version-info 1:0:0 -no-undefined
LIBADD_COMMON =

if WIN32
# you must adjust the value of DEFAULT_MODPOSTFIX according 
# to the first number in -version-info above
CPPFLAGS_PFMOD = -DMOO_DEFAULT_PFMODPREFIX=\"libmoo-\" -DMOO_DEFAULT_PFMODPOSTFIX=\"-1.dll\"
else
CPPFLAGS_PFMOD = -DMOO_DEFAULT_PFMODPREFIX=\"$(libdir)/libmoo-\" 
if MACOSX
CPPFLAGS_PFMOD += -DMOO_DEFAULT_PFMODPOSTFIX=\".dylib\"
else
CPPFLAGS_PFMOD += -DMOO_DEFAULT_PFMODPOSTFIX=\".so\"
endif
endif

##################################################
# include files to deploy
##################################################
pkgincludedir = $(includedir)

pkginclude_HEADERS = \
	moo.h \
	moo-chr.h \
	moo-cmn.h \
	moo-fmt.h \
	moo-opt.h \
	moo-rbt.h \
	moo-utl.h \
	moo-xma.h \
	moo-std.h

##################################################
# libraries to deploy
##################################################
pkglibdir = $(libdir)
pkglib_LTLIBRARIES = libmoo.la

##################################################
# libmoo
##################################################
## core source code
libmoo_la_SOURCES = \
	bct-lab.h \
	moo.h \
	moo-chr.h \
	moo-cmn.h \
	moo-fmt.h \
	moo-rbt.h \
	moo-utl.h \
	moo-prv.h \
	pack1.h \
	unpack.h \
	bigint.c \
	chr.c \
	comp.c \
	debug.c  \
	decode.c \
	dic.c \
	err.c \
	exec.c \
	fmt-imp.h \
	fmt.c \
	gc.c \
	heap.c \
	mb8.c \
	moo.c \
	number.c \
	obj.c \
	pf-basic.c \
	pf-sys.c \
	pf-utf8.c \
	rbt.c \
	sym.c \
	uch-case.h \
	uch-prop.h \
	utf8.c \
	utf16.c \
	utl.c \
	xma.c 

# non-core/helper source code
libmoo_la_SOURCES += \
	moo-opt.h \
	moo-std.h \
	opt-imp.h \
	opt.c \
	std.c
if WIN32
libmoo_la_SOURCES += poll-msw.c poll-msw.h
endif

libmoo_la_CPPFLAGS = $(CPPFLAGS_COMMON) 
libmoo_la_CFLAGS = $(CFLAGS_COMMON)
libmoo_la_LDFLAGS = $(LDFLAGS_COMMON)
libmoo_la_LIBADD = $(LIBADD_COMMON) $(LIBM)

if ENABLE_LIBLTDL
libmoo_la_LIBADD += $(LTDL_LIBS)
else
libmoo_la_LIBADD += $(DL_LIBS)
endif
if ENABLE_LIBUNWIND
libmoo_la_LIBADD += $(UNWIND_LIBS)
endif
if WIN32
libmoo_la_LIBADD += -lpsapi $(SOCKET_LIBS)
endif

if ENABLE_STATIC_MODULE

if ENABLE_MOD_CON
libmoo_la_LIBADD += ../mod/libmoo-con.la
endif

if ENABLE_MOD_FFI
libmoo_la_LIBADD += ../mod/libmoo-ffi.la
endif

libmoo_la_LIBADD += ../mod/libmoo-io.la

if ENABLE_MOD_SCK
libmoo_la_LIBADD += ../mod/libmoo-sck.la
endif

libmoo_la_LIBADD += ../mod/libmoo-stdio.la

if ENABLE_MOD_X11
libmoo_la_LIBADD += ../mod/libmoo-x11.la
endif
endif


##############################################################
# installation
##############################################################
install-data-hook:
	@echo "#ifndef _MOO_CFG_H_" > "$(DESTDIR)$(pkgincludedir)/moo-cfg.h"
	@echo "#define _MOO_CFG_H_" >> "$(DESTDIR)$(pkgincludedir)/moo-cfg.h"
	@$(EGREP) "#define[ ]+MOO_" "$(abs_builddir)/moo-cfg.h" >> "$(DESTDIR)$(pkgincludedir)/moo-cfg.h" 
	@echo "#endif" >> "$(DESTDIR)$(pkgincludedir)/moo-cfg.h"
	@rm -f "$(DESTDIR)$(pkgincludedir)/moo-cfg.h.in"
	@$(SED) 's|/\*#define MOO_HAVE_CFG_H\*/|#define MOO_HAVE_CFG_H|' "$(srcdir)/moo-cmn.h" > "$(DESTDIR)$(pkgincludedir)/moo-cmn.h"

uninstall-hook:
	@rm -f "$(DESTDIR)$(pkgincludedir)/moo-cfg.h"
