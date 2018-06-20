/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#ifndef FXATTR_H_INCLUDED
#define FXATTR_H_INCLUDED

#include "fgacfs.h"

int fxattr_open (fgac_state *state);
int fxattr_close (fgac_state *state);
int fxattr_create (const char *hostdir);

int fxattr_get_owner (fgac_state *state, fgac_path *path, uid_t *uid);
int fxattr_set_owner (fgac_state *state, fgac_path *path, uid_t uid);

int fxattr_get_group (fgac_state *state, fgac_path *path, gid_t *gid);
int fxattr_set_group (fgac_state *state, fgac_path *path, gid_t gid);

int fxattr_add (fgac_state *state, fgac_path *path, uid_t uid, gid_t gid);
int fxattr_delete (fgac_state *state, fgac_path *path);
int fxattr_rename (fgac_state *state, fgac_path *path, const char* newpath);

int fxattr_set_inh (fgac_state *state, fgac_path *path, uint64_t inh);
int fxattr_get_inh (fgac_state *state, fgac_path *path, uint64_t *inh);

int fxattr_set_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm);
int fxattr_get_prm (fgac_state *state, fgac_path *path, fgac_prm *prm);
int fxattr_unset_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm);

fgac_prms fxattr_get_prms (fgac_state *state, fgac_path *path);


#endif // FXATTR_H_INCLUDED
