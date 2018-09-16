/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#ifndef CACHE_H_INCLUDED
#define CACHE_H_INCLUDED

#include "fgacfs.h"

typedef enum {RED, BLACK} rb_color;

typedef struct rb_node
{
    fgac_prm prm;
    rb_color color;
    struct rb_node *left, *right, *parent;
} rb_node;

typedef struct st_node
{
    uid_t    uid;
    gid_t    gid;
    gid_t   *groups;
    size_t   ngrp;
    mode_t   mode;
    int      dex;
    rb_color color;
    struct st_node *left, *right, *parent;
} st_node;


struct hash_node;

struct cache_entry
{
    struct cache_entry *next, *prev;
    struct hash_node   *n;
    uid_t               uid;
    gid_t               gid;
    uint64_t            inh;
    struct rb_node     *rb;
    struct st_node     *st;
};

typedef struct
{
    struct cache_entry *front, *back;
} deque;

typedef struct hash_node
{
    char               *path;
    struct cache_entry *entry;
    struct hash_node   *next,  *prev, **cell;
} hash_node;

typedef struct
{
    hash_node **data;
    size_t      size;
    size_t      capacity;
    size_t      length;
} hash_table;

struct cache
{
    hash_table *hash;
    deque      *list;
    int         stat_cache;
};

struct cache * cache_init (size_t capacity, int stat_cache);

void cache_free (struct cache *c);


int cache_get_owner (fgac_state *state, fgac_path *path, uid_t *uid);
int cache_set_owner (fgac_state *state, fgac_path *path, uid_t uid);

int cache_get_group (fgac_state *state, fgac_path *path, gid_t *gid);
int cache_set_group (fgac_state *state, fgac_path *path, gid_t gid);

int cache_get_inh (fgac_state *state, fgac_path *path, uint64_t *inh);
int cache_set_inh (fgac_state *state, fgac_path *path, uint64_t inh);

int cache_add (fgac_state *state, fgac_path *path, uid_t uid, gid_t gid);
int cache_delete (fgac_state *state, fgac_path *path);
int cache_rename (fgac_state *state, fgac_path *path, const char* newpath);


int cache_get_prm (fgac_state *state, fgac_path *path, fgac_prm *prm);
int cache_set_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm);
int cache_unset_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm);

int cache_get_dex (fgac_state *state, fgac_path *path, const fgac_prc *prc, int *dex);
int cache_get_mode (fgac_state *state, fgac_path *path, const fgac_prc *prc, mode_t *mode);
void cache_set_dex (fgac_state *state, fgac_path *path, const fgac_prc *prc, int dex);
void cache_set_mode (fgac_state *state, fgac_path *path, const fgac_prc *prc, mode_t mode);

void cache_stat_cleanup (fgac_state *state, const char *path);

#endif // CACHE_H_INCLUDED