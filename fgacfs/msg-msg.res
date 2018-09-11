/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

"Usage: %s [-h|-v] [-o mount_options] hostdir mountpoint",               /* FGACFS_MSG_USAGE     */
"%s version %s, libfgacfs version %s",                                   /* FGACFS_MSG_VERSION   */
"FGACFS fuse module\n"
    "%s [-h|-v] [-o mount_options] hostdir mountpoint\n"
    "-h\t\tshow this help\n"
    "-v\t\tprint version\n"
    "-o coma-separated mount options\n"
    "\tcache=NUM\tset cache size (4096 entries by default, 0 for no cahce)\n"
    "\tnocmd\tdon't check program\'s permissions\n\t\t\tthis control is enabled by default on root-mount,\n\t\t\talways disabled on user mount\n"
    "\tdirect_io\tuse direct i/o (fast write, very slow read)\n"
    "\tro, rw, suid, nosuid, dev, nodev, exec, noexec, atime, noatime FUSE options are supported\n"
    "hostdir\t\tFGACFS host directory\n"
    "mountpoint\tmount point\n",                                         /* FGACFS_MSG_HELP      */
"%s warning: user mount, -o nocmd applied",                              /* FGACFS_MSG_NOCMD     */
