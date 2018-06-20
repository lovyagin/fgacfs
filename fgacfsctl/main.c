/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#include <fgacfs.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include "fgacfsctl.h"
#include <stdio.h>

#include <sys/stat.h>
#include <fcntl.h>


const char * const fgacprg_errlist[] =
{
#include "msg-err.res"
};

const char * const fgacprg_msglist[] =
{
#include "msg-msg.res"
};

char fgac_prg_name[FGAC_LIMIT_PATH];

#define VERSION "1.0.0"

#define USAGE { fgac_put_msg(FGACFSCTL_MSG_USAGE, fgac_prg_name); fgac_close(&state); return 1; }

char mountpoint[FGAC_LIMIT_PATH];

int exists (const char *name)
{
    struct stat statbuf;
    return lstat (name, &statbuf) == 0;
}

int lock (int fd)
{
    struct flock fl;
    memset(&fl, 0, sizeof(fl));

    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0;
    
    return fcntl(fd, F_SETLK, &fl) != -1;
}



int main(int argc, char *argv[] )
{
    int rc;
    fgac_state *state;
    struct stat statbuf;
    fgac_prc prc;

    fgac_init(argv[0], VERSION);

    if (argc < 2) { fgac_put_msg(FGACFSCTL_MSG_USAGE, argv[0]); return 1; }

    if (!strcmp (argv[1], "--help"))    { fgac_put_msg(FGACFSCTL_MSG_HELP, fgac_prg_name);  return 0; }
    if (!strcmp (argv[1], "--version")) { fgac_msg_version();                        return 0; }
    if (argv[1][0] == '-')              { fgac_put_msg(FGACFSCTL_MSG_USAGE, fgac_prg_name); return 1; }

    if (argc < 3) { fgac_put_msg(FGACFSCTL_MSG_USAGE, fgac_prg_name); return 1; }

    if ((rc = stat (argv[1], &statbuf))) {fgac_put_msg(FGAC_ERR_HOSTERROR); return 2;}

    prc.uid = getuid();
    prc.gid = getgid();
    prc.cmd = NULL;
    prc.ngroups = getgroups (FGAC_LIMIT_GROUPS, prc.groups);

    if (prc.ngroups == -1) { fgac_put_msg(FGACFSCTL_ERR_GROUPS, fgac_prg_name);                        return 3; }

    if (geteuid() != 0 && geteuid() != statbuf.st_uid) {fgac_put_msg(FGACFSCTL_ERR_SETUID); return 3;}

    if ((rc = fgac_open(argv[1], 1, &state, 0))) {fgac_put_msg(rc); return 2;}
    if ((rc = fgac_attach(state, mountpoint)))   {fgac_put_msg(rc); return 2;}

    argc -= 2;
    argv += 2;


    if      (!strcmp (argv[0], "type"))            rc = fgacfsctl_type(state, &prc, argc - 1, argv + 1);
    else if (!strcmp (argv[0], "test"))            rc = fgacfsctl_test(state, &prc, argc - 1, argv + 1);
    else if (!strcmp (argv[0], "show"))            rc = fgacfsctl_show(state, &prc, argc - 1, argv + 1);
    else if (!strcmp (argv[0], "get"))
    {
        argc -= 1;
        argv += 1;
        if (argc < 1) USAGE
        if      (!strcmp (argv[0], "uid"))         rc = fgacfsctl_get_uid(state, &prc, argc - 1, argv + 1);
        else if (!strcmp (argv[0], "gid"))         rc = fgacfsctl_get_gid(state, &prc, argc - 1, argv + 1);
        else if (!strcmp (argv[0], "inh"))         rc = fgacfsctl_get_inh(state, &prc, argc - 1, argv + 1);
        else if (!strcmp (argv[0], "prm"))         rc = fgacfsctl_get_prm(state, &prc, argc - 1, argv + 1);
        else    USAGE
    }
    else if (!strcmp (argv[0], "set"))
    {
        argc -= 1;
        argv += 1;
        if (argc < 1) USAGE
        else if (!strcmp (argv[0], "uid"))         rc = fgacfsctl_set_uid(state, &prc, argc - 1, argv + 1);
        else if (!strcmp (argv[0], "gid"))         rc = fgacfsctl_set_gid(state, &prc, argc - 1, argv + 1);
        else if (!strcmp (argv[0], "inh"))         rc = fgacfsctl_set_inh(state, &prc, argc - 1, argv + 1);
        else if (!strcmp (argv[0], "prm"))         rc = fgacfsctl_set_prm(state, &prc, argc - 1, argv + 1);
        else    USAGE
    }

    else    USAGE

    if (rc == FGACFSCTL_MSG_USAGE) USAGE

    if (rc) {fgac_put_msg(rc); fgac_close(&state); return 4;}

//    printf ("\n");

    fgac_close(&state);

    return 0;
}
