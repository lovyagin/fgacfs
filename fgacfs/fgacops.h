/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.

  This code is based on bbfs fuse-tutorial code
  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>
*/

#define FUSE_USE_VERSION 26

#include <limits.h>
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <unistd.h>
#include <utime.h>
#include <fuse.h>
#include <fgacfs.h>

int fgacfs_getattr     (const char *, struct stat *);

int fgacfs_readlink    (const char *, char *, size_t);

int fgacfs_mknod       (const char *, mode_t, dev_t);

int fgacfs_mkdir       (const char *, mode_t);

int fgacfs_unlink      (const char *);

int fgacfs_rmdir       (const char *);

int fgacfs_symlink     (const char *, const char *);

int fgacfs_rename      (const char *, const char *);

int fgacfs_chmod       (const char *, mode_t);

int fgacfs_chown       (const char *, uid_t, gid_t);

int fgacfs_truncate    (const char *, off_t);

int fgacfs_utime       (const char *, struct utimbuf *);

int fgacfs_open        (const char *, struct fuse_file_info *);

int fgacfs_read        (const char *, char *, size_t, off_t, struct fuse_file_info *);

int fgacfs_write       (const char *, const char *, size_t, off_t, struct fuse_file_info *);

int fgacfs_statfs      (const char *, struct statvfs *);

int fgacfs_flush       (const char *, struct fuse_file_info *);

int fgacfs_release     (const char *, struct fuse_file_info *);

int fgacfs_fsync       (const char *, int, struct fuse_file_info *);

int fgacfs_setxattr    (const char *, const char *, const char *, size_t, int);

int fgacfs_getxattr    (const char *, const char *, char *, size_t);

int fgacfs_listxattr   (const char *, char *, size_t);

int fgacfs_removexattr (const char *, const char *);

int fgacfs_opendir     (const char *, struct fuse_file_info *);

int fgacfs_readdir     (const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *);

int fgacfs_releasedir  (const char *, struct fuse_file_info *);

/* int fgacfs_fsyncdir    (const char *, int, struct fuse_file_info *); */

/* int fgacfs_access      (const char *, int); */

/* int fgacfs_create      (const char *, mode_t, struct fuse_file_info *); */

int fgacfs_ftruncate   (const char *, off_t, struct fuse_file_info *);

int fgacfs_fgetattr    (const char *, struct stat *, struct fuse_file_info *);
