/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#include <fgacfs.h>
#include <string.h>

#define VERSION "1.0.0"

#define FGACMK_MSG_USAGE   (FGAC_PRG |  0)
#define FGACMK_MSG_HELP    (FGAC_PRG |  1)

const char * const fgacprg_errlist[] =
{
#include "msg-err.res"
};

const char * const fgacprg_msglist[] =
{
#include "msg-msg.res"
};

int main(int argc, char *argv[] )
{
    fgac_type t;
    int rc;
    fgac_state *state;
    char rootpath[] = "/";
    fgac_path path = fgac_path_init(rootpath);
    uid_t uid;
    gid_t gid;

    fgac_init(argv[0], VERSION);

    if (argc < 2 || argc > 3) { fgac_put_msg(FGACMK_MSG_USAGE, argv[0]); return 1; }
    else
    {
        if (!strcmp (argv[1], "--help"))         { fgac_put_msg(FGACMK_MSG_HELP, argv[0]);  return 0; }
        else if (!strcmp (argv[1], "--version")) { fgac_msg_version();                      return 0; }
        else if (argv[1][0] == '-')              { fgac_put_msg(FGACMK_MSG_USAGE, argv[0]); return 1; }
        else
        {
            if (argc == 2) t = FGAC_SQLITE;
            else
            {
                if (!strcmp (argv[2], "db"))          t = FGAC_SQLITE;
                else if (!strcmp (argv[2], "xattr"))  t = FGAC_XATTR;
                else if (!strcmp (argv[2], "fxattr")) t = FGAC_FXATTR;
                else { fgac_put_msg(FGACMK_MSG_USAGE, argv[0]); return 1; }
            }

            uid = getuid();
            gid = getgid();

            if ((rc = fgac_create(argv[1], t)))             { fgac_put_msg(rc); return 2; }

            if ((rc = fgac_open (argv[1], 0, &state, 0)))   { fgac_put_msg(rc); return 2; }

            if ((rc = fgac_add(state, &path, uid, gid)))    { fgac_put_msg(rc); return 2; }

            fgac_close (&state);

            return 0;

        }
    }

}
