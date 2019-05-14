AUTOMAKE_OPTIONS = nostdinc

CPPFLAGS_ALL_COMMON = \
	-I$(abs_builddir) \
	-I$(abs_builddir)/../lib  \
	-I$(abs_srcdir) \
	-I$(abs_srcdir)/../lib \
	-I$(includedir)	

LDFLAGS_ALL_COMMON = -L$(abs_builddir) -L$(abs_builddir)/../lib -L$(libdir)

##################################################
# MAIN LIBRARY 
##################################################

CPPFLAGS_LIB_COMMON = $(CPPFLAGS_ALL_COMMON) $(LTDLINCL)
LDFLAGS_LIB_COMMON = $(LDFLAGS_ALL_COMMON) -version-info 1:0:0 -no-undefined
LIBADD_LIB_COMMON = $(LIBM) 

if ENABLE_LIBLTDL
LIBADD_LIB_COMMON += $(LTDL_LIBS)
else
LIBADD_LIB_COMMON += $(DL_LIBS)
endif

bin_PROGRAMS = moo
moo_SOURCES = main.c
if WIN32
moo_SOURCES += poll-msw.c poll-msw.h
endif
moo_CPPFLAGS = $(CPPFLAGS_ALL_COMMON)
moo_LDFLAGS = $(LDFLAGS_ALL_COMMON)
moo_LDADD = $(LIBADD_LIB_COMMON) -lmoo $(PTHREAD_LIBS)
if WIN32
moo_LDADD += $(SOCKET_LIBS)
endif
if ENABLE_STATIC_MODULE
moo_DEPENDENCIES = ../lib/libmoo.la
endif

