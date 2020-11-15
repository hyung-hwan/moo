AUTOMAKE_OPTION = foreign
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST =
SUBDIRS =

if ENABLE_STATIC_MODULE
SUBDIRS += tools mod lib bin t
else
SUBDIRS += tools lib mod bin t
endif

DIST_SUBDIRS = $(SUBDIRS) wasm

distclean-local:
	@rm -rf $(top_srcdir)/autom4te.cache
