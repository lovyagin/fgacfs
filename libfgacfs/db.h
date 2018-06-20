/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#ifndef DB_H_INCLUDED
#define DB_H_INCLUDED

#include "fgacfs.h"

#define QUERY_BUFFER  2048

int db_open (fgac_state *state);
int db_close (fgac_state *state);
int db_create (const char *hostdir);

int db_get_owner (fgac_state *state, fgac_path *path, uid_t *uid);
int db_set_owner (fgac_state *state, fgac_path *path, uid_t uid);

int db_get_group (fgac_state *state, fgac_path *path, gid_t *gid);
int db_set_group (fgac_state *state, fgac_path *path, gid_t gid);

int db_get_inh (fgac_state *state, fgac_path *path, uint64_t *inh);
int db_set_inh (fgac_state *state, fgac_path *path, uint64_t inh);

int db_add (fgac_state *state, fgac_path *path, uid_t uid, gid_t gid);
int db_delete (fgac_state *state, fgac_path *path);
int db_rename (fgac_state *state, fgac_path *path, const char* newpath);


int db_get_prm (fgac_state *state, fgac_path *path, fgac_prm *prm);
int db_set_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm);
int db_unset_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm);

fgac_prms db_get_prms (fgac_state *state, fgac_path *path);


#endif // DB_H_INCLUDED
