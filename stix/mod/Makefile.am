AUTOMAKE_OPTIONS = nostdinc

CPPFLAGS_ALL_COMMON = \
	-I$(abs_builddir) \
	-I$(abs_srcdir) \
	-I$(abs_srcdir)/../lib \
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

if ENABLE_STATIC_MODULE

##################################################
# STATIC MODULES BUILT INTO MAIN LIBRARY
##################################################
#libstix_la_SOURCES += mod-dir.c mod-dir.h \
#libstix_la_LIBADD +=

else

##################################################
# DYNAMIC MODULES 
##################################################

CPPFLAGS_MOD_COMMON = $(CPPFLAGS_LIB_COMMON)
LDFLAGS_MOD_COMMON = $(LDFLAGS_LIB_COMMON)
LIBADD_MOD_COMMON = -lstix

pkgmodexecdir = $(libdir)

pkgmodexec_LTLIBRARIES = libstix-stdio.la libstix-sound.la libstix-console.la

libstix_stdio_la_SOURCES = stdio.c 
libstix_stdio_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON)
libstix_stdio_la_LDFLAGS = $(LDFLAGS_MOD_COMMON)
libstix_stdio_la_LIBADD = $(LIBADD_MOD_COMMON)

libstix_sound_la_SOURCES = sound.c sound.h
libstix_sound_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON)
libstix_sound_la_LDFLAGS = $(LDFLAGS_MOD_COMMON)
libstix_sound_la_LIBADD = $(LIBADD_MOD_COMMON)

libstix_console_la_SOURCES = console.c console.h
libstix_console_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON)
libstix_console_la_LDFLAGS = $(LDFLAGS_MOD_COMMON)
libstix_console_la_LIBADD = $(LIBADD_MOD_COMMON) -ltermcap 

endif


