/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

"malloc error, out of memory?",                                     /* FGAC_ERR_MALLOC    */
"could not open fs.db, file access error or database corrupt",      /* FGAC_ERR_DBOPEN    */
"could not find xattr or hostdir/data, either filesystem moved to "
    "xattr-unsupported hostdir or its fs.db lost",                  /* FGAC_ERR_XATTROPEN */
"could not open log file",                                          /* FGAC_ERR_LOG       */
"could not open hostdir/data directory",                            /* FGAC_ERR_NODATA    */
"could not create xattr attribute, possible xattr unsupported",     /* FGAC_ERR_XATTRCRT  */
"bad xattr, fgacfs bug or host filesystem altered wrong way",       /* FGAC_ERR_XATTRFMT  */
"fail creating/accessing host directory not a directory?",          /* FGAC_ERR_HOSTDIR   */
"host directory not empty",                                         /* FGAC_ERR_HOSTEMPTY */
"could not access host directory, access restricted or I/O error",  /* FGAC_ERR_HOSTERROR */
"bad FGACFS type, bug in FGACFS application",                       /* FGAC_ERR_TYPE      */
"could not create FGACFS database, access error?",                  /* FGAC_ERR_DBCREATE  */
"could not process DB query, something wrong in fsdb database "
    "or fgacfs bug",                                                /* FGAC_ERR_DBQUERY   */
"strange permission category, FGACFS bug?",                         /* FGAC_ERR_PRMCAT    */
"XATTR hash limit reached, too many exec permissions",              /* FGAC_ERR_XHASH     */
"file path length limit reached, too long file path",               /* FGAC_ERR_PATH      */
"could not find parent directory, probably FGACFS bug?",            /* FGAC_ERR_NOPARENT  */
"could not set mount lock, access error or already mounted, "
    "if not so, please remove mount.lock file manually",            /* FGAC_ERR_LOCK      */
"could not open FGACFS owned by another without being root ",       /* FGAC_ERR_BADUID    */
