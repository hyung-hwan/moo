AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = $(PTHREAD_CFLAGS)

AM_CPPFLAGS = \
	-I$(abs_builddir) \
	-I$(abs_builddir)/../lib \
	-I$(abs_srcdir) \
	-I$(abs_srcdir)/../lib \
	-I$(includedir)

AM_LDFLAGS = -L$(abs_builddir)/../lib  -L$(libdir)
LDADD = $(PTHREAD_LIBS)

bin_PROGRAMS = t-001 t-002 t-003 t-004

t_001_SOURCES = t-001.c

t_002_SOURCES = t-002.c
t_002_LDADD = -lmoo $(LDADD)
t_002_DEPENDENCIES = ../lib/libmoo.la

t_003_SOURCES = t-003.c
t_003_LDADD = -lmoo $(LDADD)
t_003_DEPENDENCIES = ../lib/libmoo.la

t_004_SOURCES = t-004.c
t_004_LDADD = -lmoox $(LDADD)
t_004_DEPENDENCIES = ../lib/libmoox.la
