AUTOMAKE_OPTIONS = nostdinc

CPPFLAGS_ALL_COMMON = \
	-I$(abs_builddir) \
	-I$(abs_builddir)/../lib  \
	-I$(abs_srcdir) \
	-I$(abs_srcdir)/../lib \
	-I$(includedir)	

LDFLAGS_ALL_COMMON = -L$(abs_builddir) -L$(abs_builddir)/../lib -L$(libdir)

CPPFLAGS_BIN_COMMON = $(CPPFLAGS_ALL_COMMON)
LDFLAGS_BIN_COMMON = $(LDFLAGS_ALL_COMMON) -version-info 1:0:0 -no-undefined
LIBADD_BIN_COMMON = $(LIBM) 

bin_PROGRAMS = moo

##################################################
# moo
##################################################
moo_SOURCES = main.c
moo_CPPFLAGS = $(CPPFLAGS_ALL_COMMON)
moo_LDFLAGS = $(LDFLAGS_ALL_COMMON)
moo_LDADD = $(LIBADD_BIN_COMMON) -lmoox 
if ENABLE_STATIC_MODULE
moo_DEPENDENCIES = ../lib/libmoox.la
endif

