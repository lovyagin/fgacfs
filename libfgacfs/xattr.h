/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#ifndef XATTR_H_INCLUDED
#define XATTR_H_INCLUDED

#include "fgacfs.h"

int xattr_open (fgac_state *state);
int xattr_close (fgac_state *state);
int xattr_create (const char *hostdir);

int xattr_get_owner (fgac_state *state, fgac_path *path, uid_t *uid);
int xattr_set_owner (fgac_state *state, fgac_path *path, uid_t uid);

int xattr_get_group (fgac_state *state, fgac_path *path, gid_t *gid);
int xattr_set_group (fgac_state *state, fgac_path *path, gid_t gid);

int xattr_add (fgac_state *state, fgac_path *path, uid_t uid, gid_t gid);
int xattr_delete (fgac_state *state, fgac_path *path);
int xattr_rename (fgac_state *state, fgac_path *path, const char* newpath);

int xattr_set_inh (fgac_state *state, fgac_path *path, uint64_t inh);
int xattr_get_inh (fgac_state *state, fgac_path *path, uint64_t *inh);

int xattr_set_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm);
int xattr_get_prm (fgac_state *state, fgac_path *path, fgac_prm *prm);
int xattr_unset_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm);

fgac_prms xattr_get_prms (fgac_state *state, fgac_path *path);


#endif // XATTR_H_INCLUDED
