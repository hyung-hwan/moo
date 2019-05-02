AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = $(PTHREAD_CFLAGS)

AM_CPPFLAGS = \
	-I$(abs_builddir) \
	-I$(abs_srcdir) \
	-I$(includedir)

AM_LDFLAGS = -L$(libdir)
LDADD = -lmoo $(PTHREAD_LIBS)

if WIN32
LDADD += $(SOCKET_LIBS)
endif

bin_PROGRAMS = t-001
t_001_SOURCES = t-001.c
