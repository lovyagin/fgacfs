/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.

  This code is based on bbfs fuse-tutorial code
  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>
*/

#include <config.h>
#include <stdio.h>
#include "fgacops.h"

const char * const fgacprg_errlist[] =
{
#include "msg-err.res"
};

const char * const fgacprg_msglist[] =
{
#include "msg-msg.res"
};

#define FGACFS_MSG_USAGE   (FGAC_PRG |  0)
#define FGACFS_MSG_VERSION (FGAC_PRG |  1)
#define FGACFS_MSG_HELP    (FGAC_PRG |  2)
#define FGACFS_MSG_NOCMD   (FGAC_PRG |  3)

#define FGACFS_ERR_NOMOUNT   (FGAC_PRG | FGAC_ERR | 0)

char fgac_prg_name[FGAC_LIMIT_PATH];

int str_to_int (const char *str, unsigned long *value)
{
    char *end;
    if (!str || !*str) return 0;
    *value = strtoul (str, &end, 10);
    return *end ? 0 : 1;
}


#define USAGE { fgac_put_msg(FGACFS_MSG_USAGE, fgac_prg_name); return 1; }

#define OPARSE                                      \
        if (!strcmp(p, "nocmd"))         cmd   = 0; \
        else if (!strcmp(p, "direct_io")) dio  = 1; \
        else if (!strcmp(p, "ro"))      oro    = 1; \
        else if (!strcmp(p, "rw"))      oro    = 0; \
        else if (!strcmp(p, "suid"))    osuid  = 1; \
        else if (!strcmp(p, "nosuid"))  osuid  = 0; \
        else if (!strcmp(p, "dev"))     odev   = 1; \
        else if (!strcmp(p, "nodev"))   odev   = 0; \
        else if (!strcmp(p, "exec"))    oexec  = 1; \
        else if (!strcmp(p, "noexec"))  oexec  = 0; \
        else if (!strcmp(p, "atime"))   oatime = 1; \
        else if (!strcmp(p, "noatime")) oatime = 0; \
        else if (!strcmp(p, "nosc"))    osc    = 0; \
        else                                        \
        {                                           \
            char *e = strchr(p, '=');               \
            if (!e) USAGE                           \
            *e = '\0';                              \
            if (strcmp(p, "cache")) USAGE           \
            char *v = e+1;                          \
            unsigned long i;                        \
            if (!str_to_int (v, &i)) USAGE          \
            csize = i;                              \
        }                                           

#define OPT_PARSE(str)                     \
    char *p = str, *o;                     \
    while ((o = strchr(p, ',')))           \
    {                                      \
        *o = '\0';                         \
        OPARSE                             \
        p = o+1;                           \
    }                                      \
    OPARSE


int main(int argc, char *argv[])
{

#ifndef NDEBUG
    int i;
    for (i=0; i<argc; i++) printf("%i: %s\n", i,argv[i]);
#endif

    struct fuse_operations *fgacfs_ops = calloc (sizeof (struct fuse_operations), 1);

    char *fuse_args[50], *hostdir, *mountdir;

    fgac_state *state;
    
    int oro=0, osuid=0, odev=0, oexec=1, oatime=1, dio=0, osc=1;
    char oplist[FGAC_LIMIT_PATH];
    
    
    int rc, cmd = 1;
    size_t csize = 4096;

    fgac_str_cpy(fgac_prg_name, argv[0], FGAC_LIMIT_PATH);

    if       (argc == 2)
    {
        if      (!strcmp (argv[1], "-v")) { fgac_put_msg(FGACFS_MSG_VERSION, fgac_prg_name, VERSION, fgac_version); return 0; }
        else if (!strcmp (argv[1], "-h")) { fgac_put_msg(FGACFS_MSG_HELP, fgac_prg_name); return 0; }
        else USAGE
    }
    else if (argc == 3)
    {
        hostdir      = realpath(argv[1], NULL);
        mountdir     = realpath(argv[2], NULL);
        fuse_args[4] = argv[2];
    }
    else if (argc == 4)
    {
        if (argv[1][0] == '-' &&
            argv[1][1] == 'o'
           ) 
        {
            OPT_PARSE(argv[1]+2);
            hostdir      = realpath(argv[2], NULL);
            mountdir     = realpath(argv[3], NULL);
            fuse_args[4] = argv[3];
        }
        else if (argv[3][0] == '-' &&
                 argv[3][1] == 'o'
                ) 
        {
            OPT_PARSE(argv[3]+2);
            hostdir      = realpath(argv[1], NULL);
            mountdir     = realpath(argv[2], NULL);
            fuse_args[4] = argv[2];
        }
        else USAGE
    }
    else if (argc == 5)
    {
        if (!strcmp (argv[1], "-o"))
        {
            OPT_PARSE(argv[2]);
            hostdir      = realpath(argv[3], NULL);
            mountdir     = realpath(argv[4], NULL);
            fuse_args[4] = argv[4];
        }
        else if (!strcmp (argv[3], "-o"))
        {
            OPT_PARSE(argv[4]);
            hostdir      = realpath(argv[1], NULL);
            mountdir     = realpath(argv[2], NULL);
            fuse_args[4] = argv[2];
        }
        else USAGE
    }
    else USAGE

    fuse_args[0] = argv[0];
    fuse_args[1] = "-s";
    fuse_args[2] = "-o";
    fuse_args[3] = oplist;
#ifndef NDEBUG
    fuse_args[5] = "-d";
    fuse_args[6] = "-f";
#endif // NDEBUG

    if(snprintf(oplist, FGAC_LIMIT_PATH, "allow_other,%shard_remove,%s,%s,%s,%s,%s,fsname=%s",
                !odev && !osuid ? "auto_unmount," : "",
                oro     ? "ro"     : "rw",
                osuid   ? "suid"   : "nosuid", 
                odev    ? "dev"    : "nodev",
                oexec   ? "exec"   : "noexec",
                oatime  ? "atime"  : "noatime",
                hostdir
               )  >= FGAC_LIMIT_PATH) {fgac_put_msg(FGAC_ERR_PATH); return 5;}
#ifndef NDEBUG    
    printf("%s\n", oplist);
#endif    
    
    if (!mountdir) 
    {
        fgac_put_msg(FGACFS_ERR_NOMOUNT, argv[0]);
        return 1;
    }

    if (cmd && geteuid() != 0)
    {
        cmd = 0;
        fgac_put_msg(FGACFS_MSG_NOCMD, argv[0]);
    }

    if ((rc = fgac_open(hostdir, cmd, dio, &state, csize, osc))) {fgac_put_msg(rc); return 2;}
    if ((rc = fgac_mount(state, mountdir)))                      {fgac_put_msg(rc); return 2;}

    if (!fgacfs_ops)
    {
        fgac_put_msg(FGAC_ERR_MALLOC);
        return 3;
    }

    fgacfs_ops->flag_nullpath_ok   = 0;
    fgacfs_ops->flag_nopath        = 0;
    fgacfs_ops->flag_utime_omit_ok = 0;

    fgacfs_ops->getattr     = fgacfs_getattr;
    fgacfs_ops->readlink    = fgacfs_readlink;
    fgacfs_ops->mknod       = fgacfs_mknod;
    fgacfs_ops->mkdir       = fgacfs_mkdir;
    fgacfs_ops->unlink      = fgacfs_unlink;
    fgacfs_ops->rmdir       = fgacfs_rmdir;
    fgacfs_ops->symlink     = fgacfs_symlink;
    fgacfs_ops->rename      = fgacfs_rename;
    fgacfs_ops->chmod       = fgacfs_chmod;
    fgacfs_ops->chown       = fgacfs_chown;
    fgacfs_ops->truncate    = fgacfs_truncate;
    fgacfs_ops->utime       = fgacfs_utime;
    fgacfs_ops->open        = fgacfs_open;
    fgacfs_ops->read        = fgacfs_read;
    fgacfs_ops->write       = fgacfs_write;
    fgacfs_ops->statfs      = fgacfs_statfs;
    fgacfs_ops->flush       = fgacfs_flush;
    fgacfs_ops->release     = fgacfs_release;
    fgacfs_ops->fsync       = fgacfs_fsync;
    fgacfs_ops->setxattr    = fgacfs_setxattr;
    fgacfs_ops->getxattr    = fgacfs_getxattr;
    fgacfs_ops->listxattr   = fgacfs_listxattr;
    fgacfs_ops->removexattr = fgacfs_removexattr;
    fgacfs_ops->opendir     = fgacfs_opendir;
    fgacfs_ops->readdir     = fgacfs_readdir;
    fgacfs_ops->releasedir  = fgacfs_releasedir;
/*    fgacfs_ops->fsyncdir    = fgacfs_fsyncdir; */
/*    fgacfs_ops->access      = fgacfs_access;   */
/*    fgacfs_ops->create      = fgacfs_create;   */
    fgacfs_ops->ftruncate   = fgacfs_ftruncate;
    fgacfs_ops->fgetattr    = fgacfs_fgetattr;

#ifndef NDEBUG    
    printf("%s\n", fuse_args[3]);
#endif    


    rc = fuse_main(
#ifndef NDEBUG
7
#else
5
#endif
, fuse_args, fgacfs_ops, state);

    free (fgacfs_ops);

    fgac_close(&state);
    free(hostdir);


    return rc;
}
