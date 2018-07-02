/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#include <config.h>
#include "fgacfs.h"
#include "cache.h"

#include <stdlib.h>
#include <string.h>

void rb_clear (rb_node *node)
{
    if (!node) return;
    rb_clear (node->left);
    rb_clear (node->right);
    if (node->prm.cat == FGAC_CAT_PEX) free (node->prm.prc.cmd);
    free(node);
}

int prm_cmp (const fgac_prm *key1, const fgac_prm *key2)
{
    if (key1->cat < key2->cat) return -1;
    if (key1->cat > key2->cat) return  1;
    switch (key1->cat)
    {
        case FGAC_CAT_PEX: return strcmp(key1->prc.cmd, key2->prc.cmd);
        case FGAC_CAT_UID: return key1->prc.uid < key2->prc.uid ? -1 : (key1->prc.uid > key2->prc.uid ? 1 : 0);
        case FGAC_CAT_GID: return key1->prc.gid < key2->prc.gid ? -1 : (key1->prc.gid > key2->prc.gid ? 1 : 0);
        default:         return 0;
    }
}

rb_node * rb_search (rb_node *node, const fgac_prm *prm)
{
    while (node)
    {
        switch (prm_cmp(prm, &(node->prm)))
        {
            case -1: node = node->left;  break;
            case +1: node = node->right; break;
            default: return node;
        }
    }
    return NULL;
}

rb_node * rb_rotate_left (rb_node *root, rb_node *x)
{
    rb_node *y = x->right;

    x->right = y->left;
    if (y->left)
        y->left->parent = x;

    if (!x->parent)
        root = y;
    else
    {
        if (x == x->parent->left)
            x->parent->left = y;
        else
            x->parent->right = y;
    }
    y->parent = x->parent;

    y->left = x;
    x->parent = y;
    return root;
}

rb_node * rb_rotate_right (rb_node *root, rb_node *x)
{
    rb_node *y = x->left;

    x->left = y->right;
    if (y->right)
        y->right->parent = x;

    if (!x->parent)
        root = y;
    else
    {
        if (x == x->parent->left)
            x->parent->left = y;
        else
            x->parent->right = y;
    }
    y->parent = x->parent;

    y->right = x;
    x->parent = y;
    return root;
}

rb_node * rb_bst_insert (rb_node *x, const fgac_prm *prm)
{
    rb_node *y = NULL, *root = x, *z;

    while (x)
    {
        y = x;

        switch (prm_cmp(prm, &x->prm))
        {
            case -1: x = x->left;  break;
            case +1: x = x->right; break;
            default: 
                {
                    x->prm.allow = prm->allow;
                    x->prm.deny  = prm->deny;
                    return root;
                }
        }
    }


    z = malloc (sizeof(rb_node));
    if (!z) return NULL;

    z->color = RED;
    z->left  = z->right   = NULL;
    z->prm   = *prm;
    if (prm->cat == FGAC_CAT_PEX)
    {
        size_t len = strlen(prm->prc.cmd);
        z->prm.prc.cmd = malloc(len+1);
        if (!z->prm.prc.cmd)
        {
            free(z);
            return NULL;
        }
        memcpy(z->prm.prc.cmd, prm->prc.cmd, len+1);
    }

    z->parent = y;
    if (y)
    {
        if (prm_cmp(&(z->prm), &(y->prm)) < 0)
            y->left = z;
        else
            y->right = z;
    }

    return z;
}

rb_node * rb_insert (rb_node *t, const fgac_prm *prm)
{
    rb_node *x = rb_bst_insert(t, prm);
    
    if (!x) return NULL;
    if (!t) t = x;

    while (x != t && x->parent->color == RED)
    {
        rb_node *y = x->parent == x->parent->parent->left ? x->parent->parent->right
                                                          : x->parent->parent->left;

        if (y && y->color == RED)
        {
            x->parent->color         = BLACK;
            y->color                 = BLACK;
            x->parent->parent->color = RED;
            x = x->parent->parent;
        }
        else
        {            
            if (x->parent == x->parent->parent->left && x == x->parent->right)
            {
                x = x->parent;
                t = rb_rotate_left(t, x);
            }
            else if (x->parent == x->parent->parent->right && x == x->parent->left)
            {
                x = x->parent;
                t = rb_rotate_right(t, x);
            }

            x->parent->color         = BLACK;
            x->parent->parent->color = RED;
            t = x == x->parent->left ? rb_rotate_right(t, x->parent->parent)
                                     : rb_rotate_left (t, x->parent->parent);
        }
    }

    t->color = BLACK;

    return t;
}



uint32_t hash_func(const void *key, size_t size) 
{
    size_t i = strlen(key);
    uint32_t hv = 0; 
    const unsigned char *s = key;
    while (i--) 
    {
        hv += *s++;
        hv += (hv << 10);
        hv ^= (hv >> 6);
    }
    hv += (hv << 3);
    hv ^= (hv >> 11);
    hv += (hv << 15);

    return hv;
}

struct cache_entry * deque_push (deque *l)
{
    struct cache_entry *n = malloc (sizeof(struct cache_entry));
    if (!n) return NULL;
    n->next = l->back;
    n->prev = NULL;
    n->rb = NULL;
    n->gid=-1;
    n->uid=-1;
    n->inh=~(uint64_t)0;
    
    if (!l->back)
        l->front = l->back = n;
    else
    {
        l->back->prev = n;
        l->back = n;
    }
    
    return n;
}

hash_node * deque_pop (deque *l)
{
    struct cache_entry *t = l->front;
    if (!t) return NULL;
    
    l->front = l->front->prev;
    if (!l->front) 
        l->back = NULL;
    else
        l->front->next = NULL;
        
    rb_clear(t->rb);
    
    hash_node *n = t->n;
        
    free (t);
    
    return n;
}

void deque_down (deque *l, struct cache_entry *e)
{
    if (!e->prev) return;
    e->prev->next = e->next;
    if (e->next) 
        e->next->prev = e->prev;
    else
	l->front = e->prev;
    
    e->prev = NULL;
    e->next = l->back;
    l->back->prev = e;
    l->back = e;
    
    return;
}

hash_node * deque_remove (deque *l, struct cache_entry *e)
{
    hash_node *n = e->n;
    if (!e->prev) 
        l->back  = e->next;
    else
        e->prev->next = e->next;
        
    if (!e->next) 
        l->front = e->prev;
    else
        e->next->prev = e->prev;
        
    rb_clear (e->rb);
    free(e);
    
    return n;
}

void deque_free(deque *l)
{
    while (l->front) deque_pop(l);
    free(l);
}

hash_table * hash_init (size_t capacity)
{
    hash_table *hash = malloc (sizeof(hash_table));
    if (!hash) return NULL;
    
    hash->size     = 0;
    hash->capacity = capacity;
    hash->length   = capacity;

    --hash->length;
    hash->length |= hash->length >> 1;
    hash->length |= hash->length >> 2;
    hash->length |= hash->length >> 4;
    hash->length |= hash->length >> 8;
    hash->length |= hash->length >> 16;
    ++hash->length;
    hash->length <<= 1;
    
    hash->data = calloc(sizeof(hash_node *), hash->length);
    if (!hash->data)
    {
        free (hash);
        return NULL;
    }
    return hash;
}

void hash_free (hash_table *hash)
{
    hash_node **n;
    for (n = hash->data; n < hash->data+hash->length; ++n)
    {
        hash_node *t = *n;
        while (t)
        {
            hash_node *q = t->next;
            free(t->path);
            free(t);
            t = q;
        }
    }
    free (hash->data);
    free (hash);
}

struct cache * cache_init (size_t capacity)
{
    struct cache *c = malloc(sizeof(struct cache));
    if (!c) return NULL;
    
    c->hash = hash_init (capacity);
    if (!c->hash)
    {
        free(c);
        return NULL;
    }
    
    c->list = malloc (sizeof(deque));
    if (!c->list)
    {
        hash_free(c->hash);
        free(c);
        return NULL;
    }
    
    c->list->front = c->list->back = NULL;
    return c;
}

void cache_free (struct cache *c)
{
    hash_free(c->hash);
    deque_free(c->list);
    free(c);
}

void cache_pop (struct cache *c, hash_node *n)
{
    if (!n->prev) 
        *(n->cell) = n->next;
    else
	n->prev->next = n->next;

    if (n->next)
        n->next->prev = n->prev;
        
    free (n->path);
    free (n);
}

void cache_remove (struct cache *c, struct cache_entry *e)
{
    hash_node *n = deque_remove(c->list, e);
    if (n) cache_pop(c, n);
}

struct cache_entry * cache_find (struct cache *c, const char *path)
{
    uint32_t h = hash_func(path, strlen(path)) % c->hash->length;
    hash_node **cell = c->hash->data + h;
    hash_node *n = *cell;
    while (n) 
    {
        if (!strcmp(path, n->path)) 
        {
           deque_down(c->list, n->entry);
           return n->entry;
        }
        n = n->next;
    }
    
    if (c->hash->size == c->hash->capacity)
    {
        hash_node *t = deque_pop (c->list);
        if (!t) return NULL;
        cache_pop(c, t);
        --c->hash->size;
    }
    
    n = calloc (sizeof(hash_node), 1);
    if (!n) return NULL;
    
    size_t len = strlen(path)+1;
    
    n->path = malloc (len);
    if (!n->path) 
    {
        free (n);
        return NULL;
    }
    
    
    struct cache_entry *e = deque_push(c->list);
    if (!e)
    {
        free(n->path);
        free(n);
        n = NULL;
        return NULL;
    }
    
    memcpy(n->path, path, len);
    ++c->hash->size;
    e->n = n;
    
    n->cell = cell;
    n->prev = NULL;
    n->next = *cell;
    if (n->next) n->next->prev = n;
    
    
    n->entry = e;
    e->rb = NULL;
    
    *cell = n;

    return e;
}

void cache_cleanup (fgac_state *state)
{
    if (state->fd_fifo != -1)
    {        
        while (1)
        {
            ssize_t rc = read(state->fd_fifo, &state->buffer[state->bpos], 1);
            if (rc != 1) return;            
            if (state->buffer[state->bpos] == '\0')
            {
#ifndef NDEBUG
                printf("Trying to remove from cache: %s\n", state->buffer);
#endif                
                struct cache_entry *e = cache_find (state->cache, state->buffer);
                state->bpos = 0;
                if (e) cache_remove (state->cache, e);
            }
            else
            {
                ++state->bpos;
                if (state->bpos >= FGAC_LIMIT_PATH)
                {
                    state->bpos = 0;
                    return;
                }
            }
        }
    }
}

int cache_get_owner (fgac_state *state, fgac_path *path, uid_t *uid)
{
    if (!state->cache) return 0;
    cache_cleanup(state);
    
    if (!path->cache) path->cache = cache_find(state->cache, path->path);
    if (!path->cache) return 0;
    
    if (path->cache->uid == -1) return 0;
    *uid = path->cache->uid;
    return 1;
}
int cache_set_owner (fgac_state *state, fgac_path *path, uid_t uid)
{
    if (!state->cache) return 0;
    if (!path->cache) path->cache = cache_find(state->cache, path->path);
    if (!path->cache) return 0;

    path->cache->uid = uid;
    return 1;
}

int cache_get_group (fgac_state *state, fgac_path *path, gid_t *gid)
{
    if (!state->cache) return 0;
    cache_cleanup(state);

    if (!path->cache) path->cache = cache_find(state->cache, path->path);
    if (!path->cache) return 0;

    if (path->cache->gid == -1) return 0;
    *gid = path->cache->gid;
    return 1;
}
int cache_set_group (fgac_state *state, fgac_path *path, gid_t gid)
{
    if (!state->cache) return 0;
    if (!path->cache) path->cache = cache_find(state->cache, path->path);
    if (!path->cache) return 0;

    path->cache->gid = gid;
    return 1;
}

int cache_get_inh (fgac_state *state, fgac_path *path, uint64_t *inh)
{
    if (!state->cache) return 0;
    cache_cleanup(state);

    if (!path->cache) path->cache = cache_find(state->cache, path->path);
    if (!path->cache) return 0;

    if (path->cache->inh == ~(uint64_t)0) return 0;
    *inh = path->cache->inh;
    return 1;
}
int cache_set_inh (fgac_state *state, fgac_path *path, uint64_t inh)
{
    if (!state->cache) return 0;
    if (!path->cache) path->cache = cache_find(state->cache, path->path);
    if (!path->cache) return 0;

    path->cache->inh = inh;
    return 1;
}

int cache_add (fgac_state *state, fgac_path *path, uid_t uid, gid_t gid)
{ 
    if (!state->cache) return 0;
    if (!path->cache) path->cache = cache_find(state->cache, path->path);
    if (!path->cache) return 0;

    path->cache->uid = uid;
    path->cache->gid = gid;
    return 1;
}
int cache_delete (fgac_state *state, fgac_path *path)
{
    if (!state->cache) return 0;
    if (!path->cache) path->cache = cache_find(state->cache, path->path);
    if (!path->cache) return 0;

    cache_remove(state->cache, path->cache);
    return 1;
}
int cache_rename (fgac_state *state, fgac_path *path, const char* newpath)
{
    (void) newpath;
    if (!state->cache) return 0;
    if (!path->cache) path->cache = cache_find(state->cache, path->path);
    if (!path->cache) return 0;

    cache_remove(state->cache, path->cache);
    return 1;
}

int cache_get_prm (fgac_state *state, fgac_path *path, fgac_prm *prm)
{
    if (!state->cache) return 0;
    cache_cleanup(state);

    if (!path->cache) path->cache = cache_find(state->cache, path->path);
    if (!path->cache) return 0;

    rb_node *n = rb_search (path->cache->rb, prm);
    if (!n) return 0;
    
    *prm = n->prm;
    
    return 1;
}
int cache_set_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm)
{
    if (!state->cache) return 0;
    if (!path->cache) path->cache = cache_find(state->cache, path->path);
    if (!path->cache) return 0;

    rb_node *n = rb_insert(path->cache->rb, prm);
    if (!n) return 0;
    path->cache->rb = n;
    return 1;
}

int cache_unset_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm)
{
    (void) prm;
    if (!state->cache) return 0;
    if (!path->cache) path->cache = cache_find(state->cache, path->path);
    if (!path->cache) return 0;

    cache_remove(state->cache, path->cache);
    return 1;
}

