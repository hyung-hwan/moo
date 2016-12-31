AUTOMAKE_OPTION = foreign
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST =

if ENABLE_STATIC_MODULE
SUBDIRS = libltdl mod lib
else
SUBDIRS = libltdl lib mod
endif

DIST_SUBDIRS = $(SUBDIRS) 

distclean-local:
	@rm -rf $(top_srcdir)/autom4te.cache
