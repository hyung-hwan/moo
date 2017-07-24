AUTOMAKE_OPTIONS = nostdinc

CPPFLAGS_ALL_COMMON = \
	-I$(abs_builddir) \
	-I$(abs_srcdir) \
	-I$(includedir)

if ENABLE_STATIC_MODULE
LDFLAGS_ALL_COMMON = -L$(abs_builddir)/../mod -L$(abs_builddir) -L$(libdir)
else
LDFLAGS_ALL_COMMON = -L$(abs_builddir) -L$(libdir)
endif

##################################################
# MAIN LIBRARY 
##################################################

CPPFLAGS_LIB_COMMON = $(CPPFLAGS_ALL_COMMON) 
LDFLAGS_LIB_COMMON = $(LDFLAGS_ALL_COMMON) -version-info 1:0:0 -no-undefined
LIBADD_LIB_COMMON = $(LIBM) 

if ENABLE_LIBLTDL
CPPFLAGS_LIB_COMMON += $(LTDLINCL)
LIBADD_LIB_COMMON += $(LIBLTDL)
endif

if WIN32
# you must adjust the value of DEFAULT_MODPOSTFIX according 
# to the first number in -version-info above
CPPFLAGS_LIB_COMMON += -DMOO_DEFAULT_MODPREFIX=\"libmoo-\" -DMOO_DEFAULT_MODPOSTFIX=\"-1\"
else
CPPFLAGS_LIB_COMMON += -DMOO_DEFAULT_MODPREFIX=\"$(libdir)/libmoo-\" -DMOO_DEFAULT_MODPOSTFIX=\"\"
endif

pkgincludedir = $(includedir)
pkglibdir = $(libdir)

pkginclude_HEADERS = \
	moo-cmn.h \
	moo-rbt.h \
	moo-utl.h \
	moo.h

noinst_HEADERS = moo-prv.h

pkglib_LTLIBRARIES = libmoo.la
libmoo_la_SOURCES = \
	bigint.c \
	comp.c \
	debug.c  \
	decode.c \
	dic.c \
	err.c \
	exec.c \
	logfmtv.h \
	logfmt.c \
	gc.c \
	heap.c \
	moo.c \
	obj.c \
	rbt.c \
	sym.c \
	utf8.c \
	utl.c
libmoo_la_CPPFLAGS = $(CPPFLAGS_LIB_COMMON)
libmoo_la_LDFLAGS = $(LDFLAGS_LIB_COMMON)
libmoo_la_LIBADD = $(LIBADD_LIB_COMMON)

if ENABLE_STATIC_MODULE
libmoo_la_LIBADD += -lmoo-console -lmoo-stdio
libmoo_la_DEPENDENCIES = \
	$(abs_builddir)/../mod/libmoo-console.la \
	$(abs_builddir)/../mod/libmoo-stdio.la

if ENABLE_MOD_FFI
libmoo_la_LIBADD += -lmoo-ffi
libmoo_la_DEPENDENCIES += $(abs_builddir)/../mod/libmoo-ffi.la
endif
if ENABLE_MOD_X11
libmoo_la_LIBADD += -lmoo-x11
libmoo_la_DEPENDENCIES += $(abs_builddir)/../mod/libmoo-x11.la
endif

endif

bin_PROGRAMS = moo
moo_SOURCES = main.c
moo_CPPFLAGS = $(CPPFLAGS_LIB_COMMON)
moo_LDFLAGS = $(LDFLAGS_LIB_COMMON)
moo_LDADD = $(LIBADD_LIB_COMMON) -lmoo -lpthread


install-data-hook:
	@echo "#ifndef _MOO_CFG_H_" > "$(DESTDIR)$(pkgincludedir)/moo-cfg.h"
	@echo "#define _MOO_CFG_H_" >> "$(DESTDIR)$(pkgincludedir)/moo-cfg.h"
	@$(EGREP) "#define[ ]+MOO_" "$(abs_builddir)/moo-cfg.h" >> "$(DESTDIR)$(pkgincludedir)/moo-cfg.h" 
	@echo "#endif" >> "$(DESTDIR)$(pkgincludedir)/moo-cfg.h"
	@rm -f "$(DESTDIR)$(pkgincludedir)/moo-cfg.h.in"
	@$(SED) 's|/\*#define MOO_HAVE_CFG_H\*/|#define MOO_HAVE_CFG_H|' "$(srcdir)/moo-cmn.h" > "$(DESTDIR)$(pkgincludedir)/moo-cmn.h"

uninstall-hook:
	@rm -f "$(DESTDIR)$(pkgincludedir)/moo-cfg.h"
