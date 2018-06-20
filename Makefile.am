AUTOMAKE_OPTIONS = gnu

MAINTAINERCLEANFILES=           \
	$(srcdir)/configure     \
	$(srcdir)/Makefile.in 

SUBDIRS = include libfgacfs fgacfs mkfgacfs fgacfsctl

doc_DATA = README

dist-hook:
	$(MKDIR_P) "$(distdir)/m4"

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

libs:
	(cd libfgacfs && $(MAKE) libs) || exit 1;

libfgacfs:
	(cd libfgacfs && $(MAKE) libs) || exit 1;

drivers: lbfgacfs
	(cd fgacfs && $(MAKE) drivers) || exit 1;

fgacfsprogs: libfgacfs
	(cd fgacfsctl && $(MAKE)) || exit 1;
	(cd mkfgacfs && $(MAKE)) || exit 1;
