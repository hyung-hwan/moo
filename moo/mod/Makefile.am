AUTOMAKE_OPTIONS = nostdinc

CPPFLAGS_COMMON = \
	-I$(abs_builddir) \
	-I$(abs_builddir)/../lib \
	-I$(abs_srcdir) \
	-I$(abs_srcdir)/../lib \
	-I$(includedir) -fPIC

if WIN32
# you must adjust the value of DEFAULT_MODPOSTFIX according 
# to the first number in -version-info above
CPPFLAGS_COMMON += -DMOO_DEFAULT_MODPREFIX=\"libmoo-\" -DMOO_DEFAULT_MODPOSTFIX=\"-1\"
else
CPPFLAGS_COMMON += -DMOO_DEFAULT_MODPREFIX=\"$(libdir)/libmoo-\" -DMOO_DEFAULT_MODPOSTFIX=\"\"
endif

if ENABLE_STATIC_MODULE

##################################################
# STATIC MODULES BUILT INTO MAIN LIBRARY
##################################################

noinst_LTLIBRARIES = libmoo-console.la libmoo-stdio.la

libmoo_console_la_SOURCES = console.c 
libmoo_console_la_CPPFLAGS = $(CPPFLAGS_COMMON)

libmoo_stdio_la_SOURCES = stdio.c 
libmoo_stdio_la_CPPFLAGS = $(CPPFLAGS_COMMON)

else

##################################################
# DYNAMIC MODULES 
##################################################
LDFLAGS_COMMON = -L$(abs_builddir)/../lib -L$(libdir) -version-info 1:0:0 -no-undefined
LIBADD_COMMON = -lmoo

pkgmodexecdir = $(libdir)

pkgmodexec_LTLIBRARIES = libmoo-stdio.la libmoo-console.la

libmoo_stdio_la_SOURCES = stdio.c _stdio.h
libmoo_stdio_la_CPPFLAGS = $(CPPFLAGS_COMMON)
libmoo_stdio_la_LDFLAGS = $(LDFLAGS_COMMON)
libmoo_stdio_la_LIBADD = $(LIBADD_COMMON)

libmoo_console_la_SOURCES = console.c console.h
libmoo_console_la_CPPFLAGS = $(CPPFLAGS_COMMON)
libmoo_console_la_LDFLAGS = $(LDFLAGS_COMMON)
libmoo_console_la_LIBADD = $(LIBADD_COMMON) -ltermcap 

endif


