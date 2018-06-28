# Autoconf
AC_PREREQ(2.59)
AC_INIT([fgacfs],[2018.06.01],[lovyagin@mail.com])
LIBFGACFS_VERSION="1"
AC_CONFIG_SRCDIR([fgacfs/fgacops.c])
AC_CONFIG_AUX_DIR([build-aux])

# Environment
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AC_CONFIG_MACRO_DIR([m4])

# Automake
AM_INIT_AUTOMAKE
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE

# Options
AC_ARG_ENABLE(
	[debug],
	[AS_HELP_STRING([--enable-debug],[enable debugging code and output])],
	,
	[enable_debug="no"]
)

AC_ARG_ENABLE(
	[warnings],
	[AS_HELP_STRING([--enable-warnings],[enable lots of compiler warnings])],
	,
	[enable_warnings="no"]
)

AC_ARG_ENABLE(
	[pedantic],
	[AS_HELP_STRING([--enable-pedantic],[enable compile pedantic mode])],
	,
	[enable_pedantic="no"]
)

AC_ARG_ENABLE(
    [mount-helper],
    [AS_HELP_STRING([--enable-mount-helper],[install mount helper @<:@default=enabled for linux@:>@])],
    ,
    [
	case "${target_os}" in
	    linux*) enable_mount_helper="yes" ;;
	    *) enable_mount_helper="no" ;;
	esac
    ]
)

AC_ARG_ENABLE(
    [ldscript],
    [AS_HELP_STRING([--enable-ldscript],[use ldscript instead of .so symlink])],
    ,
    [enable_ldscript="no"]
)

AC_ARG_ENABLE(
    [ldconfig],
    [AS_HELP_STRING([--disable-ldconfig],[do not update dynamic linker cache using ldconfig])],
    ,
    [enable_ldconfig="yes"]
)

AC_ARG_ENABLE(
    [library],
    [AS_HELP_STRING([--disable-library],[do not install libacfs but link it into fgacfs])],
    ,
    [enable_library="yes"]
)

AC_ARG_ENABLE(
    [fgacfs],
    [AS_HELP_STRING([--disable-fgacfs],[disable the fgacfs FUSE driver])],
    ,
    [enable_fgacfs="yes"]
)

AC_ARG_ENABLE(
    [fgacfsprogs],
    [AS_HELP_STRING([--disable-fgacfsprogs],[disable fgacfsprogs utilities
	       (default=no)])],
    ,
    [enable_fgacfsprogs="yes"]
)


AC_GNU_SOURCE

AC_PROG_CC(gcc cc)
AC_PROG_LN_S
AM_PROG_CC_C_O

ifdef(
	[LT_INIT],
	[LT_INIT],
	[AC_PROG_LIBTOOL]
)

AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG

AC_ARG_VAR([LDCONFIG], [ldconfig utility])
AC_PATH_PROG([LDCONFIG], [ldconfig], [true], [/sbin /usr/sbin $PATH])

AC_PATH_PROG([CHMOD], [chmod])

if test "${enable_ldscript}" = "yes"; then
	AC_MSG_CHECKING([Output format])
	OUTPUT_FORMAT="$(${CC} ${CFLAGS} ${LDFLAGS} -Wl,--verbose 2>&1 | ${SED} -n 's/^OUTPUT_FORMAT("\([[^"]]*\)",.*/\1/p')"
	if test -z "${OUTPUT_FORMAT}"; then
		AC_MSG_RESULT([None])
	else
		AC_MSG_RESULT([${OUTPUT_FORMAT}])
		OUTPUT_FORMAT="OUTPUT_FORMAT ( ${OUTPUT_FORMAT} )"
	fi
fi

# Fuse
if test -z "$PKG_CONFIG"; then
	AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
fi
test "x${PKG_CONFIG}" = "xno" && AC_MSG_ERROR([pkg-config was not found])
PKG_CHECK_MODULES(
	[FUSE_MODULE],
	[fuse >= 2.8.0],
	,
	[
		AC_MSG_ERROR([FUSE >= 2.8.0 was not found])
	]
)
FUSE_MODULE_LIBS=`$PKG_CONFIG --libs fuse`

# Sqlite 
PKG_CHECK_MODULES(
	[SQLITE3_MODULE],
	[sqlite3 >= 3.0.0],
	,
	[
		AC_MSG_ERROR([sqlite >= 3.0 was not found])
	]
)
SQLITE3_MODULE_LIBS=`$PKG_CONFIG --libs sqlite3`

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_MAJOR
AC_CHECK_HEADERS([attr/xattr.h byteswap.h db.h dirent.h errno.h fcntl.h fuse.h grp.h \
inttypes.h libgen.h libkern/OSByteOrder.h limits.h machine/bswap.h memory.h pwd.h sqlite3.h \
stdarg.h stdint.h stdio.h stdlib.h string.h strings.h sys/byteorder.h sys/endian.h \
sys/stat.h sys/statvfs.h sys/time.h sys/types.h sys/xattr.h unistd.h utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_CHECK_MEMBERS([struct stat.st_atim])
AC_CHECK_MEMBERS([struct stat.st_atimespec])
AC_CHECK_MEMBERS([struct stat.st_atimensec])

AC_SYS_LARGEFILE

if test "$GCC" = "yes" ; then
	# We add -Wall to enable some compiler warnings.
	CFLAGS="${CFLAGS} -Wall"
fi

if test "${enable_pedantic}" = "yes"; then
	enable_warnings="yes"
	CFLAGS="${CFLAGS} -pedantic"
fi

if test "${enable_warnings}" = "yes"; then
	CFLAGS="${CFLAGS} -W -Wall -Wbad-function-cast -Wcast-align -Wcast-qual -Wdisabled-optimization -Wdiv-by-zero -Wfloat-equal -Winline -Wmissing-format-attribute -Wmissing-noreturn -Wmultichar -Wnested-externs -Wpointer-arith -Wredundant-decls -Wshadow -Wsign-compare -Wstrict-prototypes -Wundef -Wwrite-strings -Wformat -Wformat-security -Wuninitialized"
fi

if test "${enable_debug}" = "yes"; then
	CFLAGS="${CFLAGS} -ggdb3 -DDEBUG"
	AC_DEFINE(
		[ENABLE_DEBUG],
		[1],
		[Define to 1 if debug should be enabled]
	)
else	
	CFLAGS="${CFLAGS} -DNDEBUG"
fi

CFLAGS="${CFLAGS} -D_FILE_OFFSET_BITS=64"

# Settings
pkgconfigdir="\$(libdir)/pkgconfig"
fgacfsincludedir="\$(includedir)"
AC_SUBST([pkgconfigdir])
AC_SUBST([fgacfsincludedir])
AC_SUBST([LIBFGACFS_LIBS])
AC_SUBST([LIBFGACFS_VERSION])
AC_SUBST([OUTPUT_FORMAT])
AM_CONDITIONAL([INSTALL_HEADERS], [test "${enable_headers}" = "yes"])
AM_CONDITIONAL([INSTALL_LIBRARY], [test "${enable_library}" = "yes"])
AM_CONDITIONAL([ENABLE_MOUNT_HELPER], [test "${enable_mount_helper}" = "yes"])
AM_CONDITIONAL([ENABLE_FGACFS], [test "${enable_fgacfs}" = "yes"])
AM_CONDITIONAL([ENABLE_FGACFSPROGS], [test "${enable_fgacfsprogs}" = "yes"])
AM_CONDITIONAL([RUN_LDCONFIG], [test "${enable_ldconfig}" = "yes"])
AM_CONDITIONAL([GENERATE_LDSCRIPT], [test "${generate_ldscript}" = "yes"])

# workaround for <autoconf-2.60
if test -z "${docdir}"; then
	docdir="\$(datarootdir)/doc/\$(PACKAGE_NAME)"
	AC_SUBST([docdir])
fi
# workaround for <automake-1.10
if test -z "${MKDIR_P}"; then
	MKDIR_P="\$(mkdir_p)"
	AC_SUBST([MKDIR_P])
fi

# generate files
AC_CONFIG_FILES([
	Makefile
	include/Makefile
	libfgacfs/Makefile
	libfgacfs/libfgacfs.pc
	libfgacfs/libfgacfs.script.so
	mkfgacfs/Makefile
	fgacfsctl/Makefile
	fgacfs/Makefile
])
AC_OUTPUT

echo "You can type now 'make' to build fgacfs."
