AUTOMAKE_OPTIONS = nostdinc

CPPFLAGS_ALL_COMMON = \
	-I$(abs_builddir) \
	-I$(abs_srcdir) \
	-I$(includedir)	

LDFLAGS_ALL_COMMON = -L$(abs_builddir) -L$(libdir)

##################################################
# MAIN LIBRARY 
##################################################

CPPFLAGS_LIB_COMMON = $(CPPFLAGS_ALL_COMMON) $(LTDLINCL)
LDFLAGS_LIB_COMMON = $(LDFLAGS_ALL_COMMON) -version-info 1:0:0 -no-undefined
LIBADD_LIB_COMMON = $(LIBM) $(LIBLTDL) 

if WIN32
# you must adjust the value of DEFAULT_MODPOSTFIX according 
# to the first number in -version-info above
CPPFLAGS_LIB_COMMON += -DSTIX_DEFAULT_MODPREFIX=\"libstix-\" -DSTIX_DEFAULT_MODPOSTFIX=\"-1\"
else
CPPFLAGS_LIB_COMMON += -DSTIX_DEFAULT_MODPREFIX=\"$(libdir)/libstix-\" -DSTIX_DEFAULT_MODPOSTFIX=\"\"
endif

pkgincludedir = $(includedir)
pkglibdir = $(libdir)

pkginclude_HEADERS = \
	stix-cfg.h \
	stix-cmn.h \
	stix-rbt.h \
	stix-utl.h \
	stix.h

pkglib_LTLIBRARIES = libstix.la
libstix_la_SOURCES = \
	stix-prv.h \
	stix-rbt.c \
	stix-utl.c \
	stix.c \
	heap.c \
	gc.c \
	obj.c \
	sym.c \
	dic.c \
	proc.c \
	bigint.c \
	ignite.c \
	utf8.c \
	comp.c \
	exec.c \
	debug.c 
libstix_la_CPPFLAGS = $(CPPFLAGS_LIB_COMMON)
libstix_la_LDFLAGS = $(LDFLAGS_LIB_COMMON)
libstix_la_LIBADD = $(LIBADD_LIB_COMMON)

bin_PROGRAMS = stix
stix_SOURCES = main.c
stix_CPPFLAGS = $(CPPFLAGS_LIB_COMMON)
stix_LDFLAGS = $(LDFLAGS_LIB_COMMON)
stix_LDADD = $(LIBADD_LIB_COMMON) -lstix #-ldyncall_s

if ENABLE_STATIC_MODULE

##################################################
# STATIC MODULES BUILT INTO MAIN LIBRARY
##################################################
#libstix_la_SOURCES += \
#	mod-dir.c mod-dir.h \
#libstix_la_LIBADD +=

else
##################################################
# DYNAMIC MODULES 
##################################################

CPPFLAGS_MOD_COMMON = $(CPPFLAGS_LIB_COMMON)
LDFLAGS_MOD_COMMON = $(LDFLAGS_LIB_COMMON)
LIBADD_MOD_COMMON = -lstix

pkgmodexecdir = $(libdir)

pkgmodexec_LTLIBRARIES = libstix-snd.la
libstix_snd_la_SOURCES = mod-snd.c mod-snd.h
libstix_snd_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON)
libstix_snd_la_LDFLAGS = $(LDFLAGS_MOD_COMMON)
libstix_snd_la_LIBADD = $(LIBADD_MOD_COMMON)

endif


install-data-hook:
	@echo "#ifndef _STIX_CFG_H_" > "$(DESTDIR)$(pkgincludedir)/stix-cfg.h"
	@echo "#define _STIX_CFG_H_" >> "$(DESTDIR)$(pkgincludedir)/stix-cfg.h"
	@$(EGREP) "#define[ ]+STIX_" "$(abs_builddir)/stix-cfg.h" >> "$(DESTDIR)$(pkgincludedir)/stix-cfg.h" 
	@echo "#endif" >> "$(DESTDIR)$(pkgincludedir)/stix-cfg.h"
	@$(RM) "$(DESTDIR)$(pkgincludedir)/stix-cfg.h.in"
	@$(SED) 's|/\*#define STIX_HAVE_CFG_H\*/|#define STIX_HAVE_CFG_H|' "$(srcdir)/stix-cmn.h" > "$(DESTDIR)$(pkgincludedir)/stix-cmn.h"

uninstall-hook:
	@$(RM) "$(DESTDIR)$(pkgincludedir)/stix-cfg.h"

