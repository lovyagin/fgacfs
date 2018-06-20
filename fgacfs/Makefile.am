if ENABLE_FGACFS

AM_LFLAGS	= $(all_libraries)
LIBTOOL_LINK	= $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

bin_PROGRAMS	= fgacfs

CLEANFILES	= $(PROGRAMS)
MAINTAINERCLEANFILES	= Makefile.in

fgacfs_LDADD	= $(AM_LIBS) $(SQLITE3_MODULE_LIBS) $(FUSE_MODULE_LIBS) $(top_builddir)/libfgacfs/libfgacfs.la
fgacfs_CFLAGS	= $(AM_CFLAGS) $(FUSE_CFLAGS) $(SQLITE3_CFLAGS) -I$(top_srcdir)/include/ $(all_includes)
fgacfs_SOURCES	= fgacops.c main.c

drivers : $(FUSE_LIBS) $(SQLITE3_LIBS) fgacfs libfgacfs

install-exec-hook:
if RUN_LDCONFIG
	$(LDCONFIG)
endif

if ENABLE_MOUNT_HELPER
install-exec-local:	install-binPROGRAMS
	$(MKDIR_P) "$(DESTDIR)/$(sbindir)"
	$(LN_S) -f "$(DESTDIR)/$(bindir)/fgacfs" "$(DESTDIR)/$(sbindir)/mount.fgacfs"
endif

endif # ENABLE_FGACFS