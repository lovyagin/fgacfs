/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#include <config.h>
#include <stdarg.h>
#include <stdio.h>

#include "fgacfs.h"

const char * const fgac_errlist[] =
{
#include "msg-err.res"
};
const char * const fgac_msglist[] =
{
#include "msg-msg.res"
};

const char *fgac_msg (size_t idx)
{
    const char * const * list = idx & FGAC_ERR ? (idx & FGAC_PRG ? fgacprg_errlist : fgac_errlist)
                                               : (idx & FGAC_PRG ? fgacprg_msglist : fgac_msglist);
    idx &= ~(FGAC_ERR | FGAC_PRG);

    return list[idx];
}

void fgac_put_msg (size_t idx, ...)
{
    FILE *stream = (idx & FGAC_ERR) ? stderr : stdout;

    if (idx & FGAC_ERR) fprintf(stream, "%s error: ", fgac_prg_name);

    va_list ap;
    va_start(ap, idx);
    vfprintf(stream, fgac_msg(idx), ap);
    fprintf(stream, "\n");
}

void fgac_msg_version ()
{
    fgac_put_msg(FGAC_MSG_VERSION, fgac_prg_name, fgac_prg_version, fgac_name, fgac_version);
}

