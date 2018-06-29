if ENABLE_FGACFSPROGS

AM_LFLAGS	= $(all_libraries)
LIBTOOL_LINK	= $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

bin_PROGRAMS	= mkfgacfs

CLEANFILES	= $(PROGRAMS)

MAINTAINERCLEANFILES	= Makefile.in

mkfgacfs_LDADD	= $(AM_LIBS) $(SQLITE3_MODULE_LIBS) $(FUSE_MODULE_LIBS) $(top_builddir)/libfgacfs/libfgacfs.la
mkfgacfs_CFLAGS	= $(AM_CFLAGS) $(FUSE_MODULE_CFLAGS) $(SQLITE3_MODULE_CFLAGS) -I$(top_srcdir)/include/ $(all_includes)
mkfgacfs_SOURCES	= main.c

if ENABLE_MOUNT_HELPER
install-exec-hook:
	$(INSTALL) -d $(DESTDIR)/$(sbindir)
	$(LN_S) -f $(DESTDIR)/$(bindir)/mkfgacfs $(DESTDIR)/$(sbindir)/mkfs.fgacfs
endif

endif
