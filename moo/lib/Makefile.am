AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = $(PTHREAD_CFLAGS)

CPPFLAGS_ALL_COMMON = \
	-I$(abs_builddir) \
	-I$(abs_srcdir) \
	-I$(includedir)

if ENABLE_STATIC_MODULE
LDFLAGS_ALL_COMMON = -L$(abs_builddir)/../mod -L$(abs_builddir) -L$(libdir)
else
LDFLAGS_ALL_COMMON = -L$(abs_builddir) -L$(libdir)
endif

CPPFLAGS_LIB_COMMON = $(CPPFLAGS_ALL_COMMON) 
LDFLAGS_LIB_COMMON = $(LDFLAGS_ALL_COMMON) -version-info 1:0:0 -no-undefined
LIBADD_LIB_COMMON =

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
	moo-std.h

##################################################
# libraries to deploy
##################################################
pkglibdir = $(libdir)
pkglib_LTLIBRARIES = libmoo.la libmoox.la

##################################################
# libmoo
##################################################
libmoo_la_SOURCES = \
	bct-lab.h \
	chr.c \
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
	utl.c
libmoo_la_CPPFLAGS = $(CPPFLAGS_LIB_COMMON) 
libmoo_la_LDFLAGS = $(LDFLAGS_LIB_COMMON)
libmoo_la_LIBADD = $(LIBADD_LIB_COMMON)
libmoo_la_DEPENDENCIES =

if ENABLE_STATIC_MODULE

if ENABLE_MOD_CON
libmoo_la_LIBADD += -lmoo-con
libmoo_la_DEPENDENCIES += ../mod/libmoo-con.la
endif

if ENABLE_MOD_FFI
libmoo_la_LIBADD += -lmoo-ffi
libmoo_la_DEPENDENCIES += ../mod/libmoo-ffi.la
endif

libmoo_la_LIBADD += -lmoo-io
libmoo_la_DEPENDENCIES += ../mod/libmoo-io.la

if ENABLE_MOD_SCK
libmoo_la_LIBADD += -lmoo-sck
libmoo_la_DEPENDENCIES += ../mod/libmoo-sck.la
endif

libmoo_la_LIBADD += -lmoo-stdio
libmoo_la_DEPENDENCIES += ../mod/libmoo-stdio.la

if ENABLE_MOD_X11
libmoo_la_LIBADD += -lmoo-x11
libmoo_la_DEPENDENCIES += ../mod/libmoo-x11.la
endif
endif

##############################################################
# libmoox
##############################################################
libmoox_la_SOURCES = \
	moo-opt.h \
	moo-std.h \
	opt-imp.h \
	opt.c \
	std.c
if WIN32
libmoox_la_SOURCES += poll-msw.c poll-msw.h
endif
libmoox_la_CPPFLAGS = $(CPPFLAGS_LIB_COMMON) $(CPPFLAGS_PFMOD)
libmoox_la_LDFLAGS = $(LDFLAGS_LIB_COMMON)
libmoox_la_LIBADD = $(LIBADD_LIB_COMMON) -lmoo $(LIBM) $(PTHREAD_LIBS)

if ENABLE_LIBLTDL
libmoox_la_LIBADD += $(LTDL_LIBS)
else
libmoox_la_LIBADD += $(DL_LIBS)
endif
if ENABLE_LIBUNWIND
libmoox_la_LIBADD += $(UNWIND_LIBS)
endif
if WIN32
libmoox_la_LIBADD += -lpsapi $(SOCKET_LIBS)
endif

libmoox_la_DEPENDENCIES = libmoo.la

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
