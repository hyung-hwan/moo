AUTOMAKE_OPTIONS = nostdinc

CPPFLAGS_COMMON = \
	-I$(abs_builddir) \
	-I$(abs_builddir)/../lib \
	-I$(abs_srcdir) \
	-I$(abs_srcdir)/../lib \
	-I$(includedir)

if WIN32
# you must adjust the value of DEFAULT_MODPOSTFIX according 
# to the first number in -version-info above
CPPFLAGS_COMMON += -DSTIX_DEFAULT_MODPREFIX=\"libstix-\" -DSTIX_DEFAULT_MODPOSTFIX=\"-1\"
else
CPPFLAGS_COMMON += -DSTIX_DEFAULT_MODPREFIX=\"$(libdir)/libstix-\" -DSTIX_DEFAULT_MODPOSTFIX=\"\"
endif

if ENABLE_STATIC_MODULE

##################################################
# STATIC MODULES BUILT INTO MAIN LIBRARY
##################################################

noinst_LIBRARIES = libstix-stdio.a

libstix_stdio_a_SOURCES = stdio.c 
libstix_stdio_a_CPPFLAGS = $(CPPFLAGS_COMMON)

else

##################################################
# DYNAMIC MODULES 
##################################################
LDFLAGS_COMMON = -L$(abs_builddir)/../lib -L$(libdir) -version-info 1:0:0 -no-undefined
LIBADD_COMMON = -lstix

pkgmodexecdir = $(libdir)

pkgmodexec_LTLIBRARIES = libstix-stdio.la libstix-console.la

libstix_stdio_la_SOURCES = stdio.c 
libstix_stdio_la_CPPFLAGS = $(CPPFLAGS_COMMON)
libstix_stdio_la_LDFLAGS = $(LDFLAGS_COMMON)
libstix_stdio_la_LIBADD = $(LIBADD_COMMON)

libstix_console_la_SOURCES = console.c console.h
libstix_console_la_CPPFLAGS = $(CPPFLAGS_COMMON)
libstix_console_la_LDFLAGS = $(LDFLAGS_COMMON)
libstix_console_la_LIBADD = $(LIBADD_COMMON) -ltermcap 

endif


