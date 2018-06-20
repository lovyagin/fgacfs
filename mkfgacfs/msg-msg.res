/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

"Usage: %s [--help|--version] hostdir [db|xattr|fxattr]",                      /* FGACMK_MSG_USAGE     */
"Create FGACFS filesystem\n"
    "usage: %s type hostdir\n"
    "\thostdir\t\treal filesystem directory to store files in\n"
    "\ttype:\n"
    "\t\tdb (default)\tstore permissions in database\n"
    "\t\txattr\t\tstore permissions in safe (ASCII) extended attributes\n"
    "\t\tfxattr\t\tstore permissions in fast (binary) extended attributes",    /* FGACMK_MSG_HELP      */
