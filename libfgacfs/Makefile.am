MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

if INSTALL_LIBRARY
lib_LTLIBRARIES    = libfgacfs.la
pkgconfig_DATA     = libfgacfs.pc
else
noinst_LTLIBRARIES = libfgacfs.la
endif

libfgacfs_la_CFLAGS	= $(AM_CFLAGS) -I$(top_srcdir)/include
libfgacfs_la_LIBADD	= $(LIBFGACFS_LIBS)
libfgacfs_la_LDFLAGS	= -version-info $(LIBFGACFS_VERSION) -no-undefined

libfgacfs_la_SOURCES	= cityhash.c db.c fgacfs.c fxattr.c misc.c msg.c xattr.c cache.c

if ENABLE_FGACFSPROGS
libs:	$(lib_LTLIBRARIES)
endif

