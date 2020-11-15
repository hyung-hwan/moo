AUTOMAKE_OPTIONS = nostdinc

CPPFLAGS_COMMON = \
	-I$(abs_builddir) \
	-I$(abs_builddir)/../lib  \
	-I$(abs_srcdir) \
	-I$(abs_srcdir)/../lib \
	-I$(includedir) \
	$(PTHREAD_CFLAGS)

CFLAGS_COMMON =
LDFLAGS_COMMON = -L$(abs_builddir)/../lib -L$(libdir)
LIBADD_COMMON = -lmoo
DEPENDENCIES_COMMON =

bin_PROGRAMS = moo

##################################################
# moo
##################################################
moo_SOURCES = main.c
moo_CPPFLAGS = $(CPPFLAGS_COMMON)
moo_CFLAGS = $(CFLAGS_COMMON)
moo_LDFLAGS = $(LDFLAGS_COMMON)
moo_LDADD = $(LIBADD_COMMON)
moo_DEPENDENCIES = ../lib/libmoo.la


if ENABLE_EXPERIMENTAL

##################################################
# moo-gtk - experimental. the following configuration is temporary only.
##################################################
bin_PROGRAMS += moo-gtk
moo_gtk_SOURCES = gtk-main.c
moo_gtk_CPPFLAGS = $(CPPFLAGS_COMMON) -I/usr/include/gtk-3.0 -I/usr/include/at-spi2-atk/2.0 -I/usr/include/at-spi-2.0 -I/usr/include/dbus-1.0 -I/usr/lib64/dbus-1.0/include -I/usr/include/gtk-3.0 -I/usr/include/gio-unix-2.0/ -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng16 -I/usr/include/freetype2 -I/usr/include/libdrm -I/usr/include/libpng16 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/libpng16 -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include
moo_gtk_CFLAGS = $(CFLAGS_COMMON)
moo_gtk_LDFLAGS = $(LDFLAGS_COMMON)
moo_gtk_LDADD = $(LIBADD_COMMON) -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0
moo_gtk_DEPENDENCIES = ../lib/libmoo.la

endif
