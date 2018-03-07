AUTOMAKE_OPTION = foreign
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST =
SUBDIRS =

if ENABLE_STATIC_MODULE
SUBDIRS += mod lib
else
SUBDIRS += lib mod
endif

DIST_SUBDIRS = $(SUBDIRS) 

distclean-local:
	@rm -rf $(top_srcdir)/autom4te.cache
