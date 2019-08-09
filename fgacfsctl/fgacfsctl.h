/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#ifndef FGACFSCTL_H_INCLUDED
#define FGACFSCTL_H_INCLUDED

#include <fgacfs.h>

#define FGACFSCTL_MSG_USAGE   (FGAC_PRG |  0)
#define FGACFSCTL_MSG_HELP    (FGAC_PRG |  1)

#define FGACFSCTL_ERR_SETUID  (FGAC_PRG | FGAC_ERR |  0)
#define FGACFSCTL_ERR_PRM     (FGAC_PRG | FGAC_ERR |  1)
#define FGACFSCTL_ERR_NUMBER  (FGAC_PRG | FGAC_ERR |  2)
#define FGACFSCTL_ERR_GROUPS  (FGAC_PRG | FGAC_ERR |  3)
#define FGACFSCTL_ERR_INH     (FGAC_PRG | FGAC_ERR |  4)
#define FGACFSCTL_ERR_SET     (FGAC_PRG | FGAC_ERR |  5)
#define FGACFSCTL_ERR_CAT     (FGAC_PRG | FGAC_ERR |  6)
#define FGACFSCTL_ERR_PFL     (FGAC_PRG | FGAC_ERR |  7)
#define FGACFSCTL_ERR_PID     (FGAC_PRG | FGAC_ERR |  8)
#define FGACFSCTL_ERR_AD      (FGAC_PRG | FGAC_ERR |  9)
#define FGACFSCTL_ERR_DFP     (FGAC_PRG | FGAC_ERR | 10)
#define FGACFSCTL_ERR_DFI     (FGAC_PRG | FGAC_ERR | 11)
#define FGACFSCTL_ERR_PRMGET  (FGAC_PRG | FGAC_ERR | 12)

int fgacfsctl_type (fgac_state *state, fgac_prc *prc, int argc, char *argv[]);
int fgacfsctl_show (fgac_state *state, fgac_prc *prc, int argc, char *argv[]);
int fgacfsctl_idump (fgac_state *state, fgac_prc *prc, int argc, char *argv[]);
int fgacfsctl_pdump (fgac_state *state, fgac_prc *prc, int argc, char *argv[]);
int fgacfsctl_test (fgac_state *state, fgac_prc *prc, int argc, char *argv[]);

int fgacfsctl_get_uid (fgac_state *state, fgac_prc *prc, int argc, char *argv[]);
int fgacfsctl_get_gid (fgac_state *state, fgac_prc *prc, int argc, char *argv[]);
int fgacfsctl_get_inh (fgac_state *state, fgac_prc *prc, int argc, char *argv[]);
int fgacfsctl_get_prm (fgac_state *state, fgac_prc *prc, int argc, char *argv[]);

int fgacfsctl_set_uid (fgac_state *state, fgac_prc *prc, int argc, char *argv[]);
int fgacfsctl_set_gid (fgac_state *state, fgac_prc *prc, int argc, char *argv[]);
int fgacfsctl_set_inh (fgac_state *state, fgac_prc *prc, int argc, char *argv[]);
int fgacfsctl_set_prm (fgac_state *state, fgac_prc *prc, int argc, char *argv[]);

#endif // FGACFSCTL_H_INCLUDED
