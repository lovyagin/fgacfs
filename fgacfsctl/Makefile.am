if ENABLE_FGACFSPROGS

AM_LFLAGS	= $(all_libraries)
LIBTOOL_LINK	= $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

bin_PROGRAMS	= fgacfsctl

CLEANFILES	= $(PROGRAMS)

MAINTAINERCLEANFILES	= Makefile.in

fgacfsctl_LDADD	= $(AM_LIBS) $(SQLITE3_MODULE_LIBS) $(FUSE_MODULE_LIBS) $(top_builddir)/libfgacfs/libfgacfs.la
fgacfsctl_CFLAGS	= $(AM_CFLAGS) $(FUSE_CFLAGS) $(SQLITE3_CFLAGS) -I$(top_srcdir)/include/ $(all_includes)
fgacfsctl_SOURCES	= fgacfsctl.c main.c

install-exec-hook:
	$(CHMOD) 6755 $(DESTDIR)/$(bindir)/fgacfsctl

endif
