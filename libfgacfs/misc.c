/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#include <config.h>
#include <string.h>
#include <stdlib.h>
#include "fgacfs.h"

fgac_prms fgac_prms_init ()
{
    fgac_prms r;
    r.array  = NULL;
    r.length = 0;
    return r;
}

void fgac_prms_push (fgac_prms *prms, const fgac_prm *prm)
{
    fgac_prm *i, *e;
    if (fgac_prms_is_error(prms)) return;

    for (i = prms->array, e = prms->array + prms->length; i < e; ++i)
        if (prm->cat == i->cat)
        {
            if (prm->cat == FGAC_CAT_ALL ||
                prm->cat == FGAC_CAT_OGR ||
                prm->cat == FGAC_CAT_OTH ||
                (prm->cat == FGAC_CAT_UID && prm->prc.uid == i->prc.uid) ||
                (prm->cat == FGAC_CAT_GID && prm->prc.gid == i->prc.gid) ||
                (prm->cat == FGAC_CAT_PEX && !strcmp(prm->prc.cmd, i->prc.cmd))
               )
            {
                i->allow |= prm->allow;
                i->deny  |= prm->deny;
                return;
            }
        }
    fgac_prms_add (prms, prm);
}

void fgac_prms_add (fgac_prms *prms, const fgac_prm *prm)
{
    fgac_prm *a, *e;
    if (fgac_prms_is_error(prms)) return;

    a = realloc (prms->array, sizeof (fgac_prm) * (prms->length + 1));
    if (!a)
    {
        fgac_prms_free(prms);
        prms->length = ~(size_t)0;
        return;
    }
    prms->array = a;

    e = prms->array + prms->length;
    *e = *prm;
    if (prm->cat == FGAC_CAT_PEX)
    {
        size_t l = strlen(prm->prc.cmd);
        e->prc.cmd = malloc (l + 1);
        if (!e->prc.cmd)
        {
            fgac_prms_free(prms);
            prms->length = ~(size_t)0;
            return;
        }
        memcpy(e->prc.cmd, prm->prc.cmd, l+1);
    }
    ++prms->length;
}

void fgac_prms_remove (fgac_prms *prms, size_t i)
{
    fgac_prm *a;
    if (fgac_prms_is_error(prms)) return;

    if (i >= prms->length)
    {
        fgac_prms_free(prms);
        prms->length = ~(size_t)0;
        return;
    }

    if (prms->length == 1)
    {
        fgac_prms_free(prms);
        return;
    }

    a = realloc (prms->array, sizeof (fgac_prm) * (prms->length - 1));
    if (!a)
    {
        fgac_prms_free(prms);
        prms->length = ~(size_t)0;
        return;
    }
    prms->array = a;

    prms->array[i] = prms->array[prms->length - 1];
    --prms->length;
}

void fgac_prms_join (fgac_prms *prms, const fgac_prms *source)
{
    fgac_prm *p, *q;

    if (fgac_prms_is_error(prms)) return;
    if (fgac_prms_is_error(source))
    {
        fgac_prms_free(prms);
        *prms = *source;
        return;
    }

    for (p = source->array, q = source->array + source->length; p<q; ++p)
    {
        fgac_prms_push (prms, p);
        if (fgac_prms_is_error (prms)) return;
    }
}

void fgac_prms_free (fgac_prms *prms)
{
    fgac_prm *p, *q;
    if (!prms->array) {prms->length = 0; return; }

    for (p = prms->array, q = prms->array + prms->length; p<q; ++p)
    {
        if (p->cat == FGAC_CAT_PEX) free(p->prc.cmd);
    }

    free (prms->array);
    prms->array  = NULL;
    prms->length = 0;
}

fgac_prms fgac_prms_error ()
{
    fgac_prms r;
    r.array  = NULL;
    r.length = ~(size_t)0;
    return r;
}

fgac_prm * fgac_prms_element (fgac_prms *prms, size_t i)
{
    return i < prms->length && !fgac_prms_is_error (prms) ? prms->array + i : NULL;
}

const fgac_prm * fgac_prms_get (const fgac_prms *prms, size_t i)
{
    return i < prms->length && !fgac_prms_is_error (prms) ? prms->array + i : NULL;
}

size_t fgac_prms_size (const fgac_prms *prms)
{
    return prms->length == ~(size_t)0 ? 0 : prms->length;
}

int fgac_str_cpy (char *dest, const char *source, size_t size)
{
    size_t len;
    if (!dest) return 0;
    if (!source || (len = strlen(source)) >= size)
    {
        if (size) *dest = '\0';
        return !source ? 1 : 0;
    }
    memcpy (dest, source, len + 1);
    return 1;
}

int fgac_str_cat2 (char *dest, const char *s1, const char *s2, size_t size)
{
    size_t l1 = s1 ? strlen(s1) : 0, l2 = s2 ? strlen(s2) : 0;
    if (!dest) return 0;
    if (l1 + l2 >= size)
    {
        if (size) *dest = '\0';
        return 0;
    }
    if (l1) memcpy (dest, s1, l1);
    if (l2) memcpy (dest + l1, s2, l2 + 1); else dest[l1] = '\0';
    return 1;

}

int fgac_str_add (char *dest, size_t shift, const char *source, size_t size)
{
    size_t len = source ? strlen(source) : 0;
    if (!dest) return 0;
    if (len + shift >= size)
    {
        if (size) *dest = '\0';
        return 0;
    }
    if (len) memcpy (dest + shift, source, len + 1); else dest[shift] = '\0';
    return 1;
}


int fgac_str_cat (char *dest, const char *source, size_t size)
{
    return fgac_str_add (dest, strlen(dest), source, size);
}

