/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#include <config.h>
#include "fgacfs.h"
#include "fxattr.h"
#include <sys/xattr.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "bswap.h"
#ifdef WORDS_BIGENDIAN
#define uint64_convert_order(x) x = bswap_64(x);
#else
#define uint64_convert_order(x)
#endif

#ifndef ENOATTR
#define ENOATTR ENODATA
#endif

int fxattr_open (fgac_state *state)
{
     char value;
     if (lgetxattr(state->datadir, "user.fgacfs", &value, 1) != 1 ||
         value != '!'
        ) return FGAC_ERR_XATTROPEN;

     state->type = FGAC_FXATTR;

     return FGAC_OK;
}


int fxattr_close (fgac_state *state)
{
    (void) state;
    return FGAC_OK;
}


int fxattr_create (const char *hostdir)
{
    const char c = '!';
    char datadir[FGAC_LIMIT_PATH];
    if (!fgac_str_cat2 (datadir, hostdir, "/data", FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;

    return lsetxattr(datadir, "user.fgacfs", &c, 1, XATTR_CREATE) ? FGAC_ERR_XATTRCRT : FGAC_OK;
}

int fxattr_unset (fgac_state *state, fgac_path *path, const char *name)
{
    int r = lremovexattr(FGAC_HOSTPATH, name);
    return (r && errno != ENOATTR) ? FGAC_ERR_XATTRCRT : FGAC_OK;
}

int fxattr_get_int (fgac_state *state, fgac_path *path, const char *name, uint64_t *attr)
{
     int rc = lgetxattr(FGAC_HOSTPATH, name, attr, 8);

     if (rc == 8)
     {
         uint64_convert_order(*attr);
         return FGAC_OK;
     }

     if (rc == -1 && errno == ENOATTR)
     {
         *attr = 0;
         return FGAC_OK;
     }

     return FGAC_ERR_XATTROPEN;

}

int fxattr_set_int (fgac_state *state, fgac_path *path, const char *name, uint64_t attr)
{
    uint64_convert_order(attr);

    if (lsetxattr(FGAC_HOSTPATH, name, &attr, 8, 0))
        return FGAC_ERR_XATTRCRT;

    return FGAC_OK;

}



int fxattr_get_owner (fgac_state *state, fgac_path *path, uid_t *uid)
{
    int rc;
    uint64_t attr;
    rc = fxattr_get_int (state, path, "user.fgacfs.owner", &attr);
    *uid = attr;
    return rc;
}
int fxattr_set_owner (fgac_state *state, fgac_path *path, uid_t uid)
{
    return fxattr_set_int (state, path, "user.fgacfs.owner", (uint64_t) uid);
}

int fxattr_get_group (fgac_state *state, fgac_path *path, gid_t *gid)
{
    int rc;
    uint64_t attr;
    rc = fxattr_get_int (state, path, "user.fgacfs.group", &attr);
    *gid = attr;
    return rc;
}
int fxattr_set_group (fgac_state *state, fgac_path *path, gid_t gid)
{
    return fxattr_set_int (state, path, "user.fgacfs.group", (uint64_t) gid);
}

int fxattr_set_inh (fgac_state *state, fgac_path *path, uint64_t inh)
{
    return fxattr_set_int (state, path, "user.fgacfs.inh", inh);
}
int fxattr_get_inh (fgac_state *state, fgac_path *path, uint64_t *inh)
{
    return fxattr_get_int (state, path, "user.fgacfs.inh", inh);
}

int fxattr_add (fgac_state *state, fgac_path *path, uid_t uid, gid_t gid)
{
    int rc;
    if ((rc = fxattr_set_owner(state, path, uid))) return rc;
    if ((rc = fxattr_set_group(state, path, gid))) return rc;
    return FGAC_OK;
}

int fxattr_delete (fgac_state *state, fgac_path *path)
{
    (void) state;
    (void) path;
    return FGAC_OK;
}

int fxattr_rename (fgac_state *state, fgac_path *path, const char* newpath)
{
    (void) state;
    (void) path;
    (void) newpath;
    return FGAC_OK;
}

int int_to_hex (uint64_t i, char* hex);
int hex_to_int (const char* hex, uint64_t *i);

#define FGAC_UNSET(name) \
    if ((rc = fxattr_unset(state, path, name))) return rc;

#define FGAC_SET(name,type) \
    if ((rc = fxattr_set_int(state, path, name, prm->type))) return rc;

#define FGAC_GET(name,type) \
    if ((rc = fxattr_get_int(state, path, name, &prm->type))) return rc;

int fxattr_set_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm)
{
    if (!(prm->allow | prm->deny)) return fxattr_unset_prm(state, path, prm);
    char name1 [FGAC_LIMIT_PATH], name2[FGAC_LIMIT_PATH], index[17];
    int rc;

    switch (prm->cat)
    {
        case FGAC_CAT_ALL: /**< ALL users                                  **/
                           FGAC_SET("user.fgacfs.Aall", allow)
                           FGAC_SET("user.fgacfs.Dall", deny )
                           return FGAC_OK;
        case FGAC_CAT_OUS: /**< Owner USer                                 **/
                           FGAC_SET("user.fgacfs.Aous", allow)
                           FGAC_SET("user.fgacfs.Dous", deny )
                           return FGAC_OK;
        case FGAC_CAT_OGR: /**< Owner Group                                **/
                           FGAC_SET("user.fgacfs.Aogr", allow)
                           FGAC_SET("user.fgacfs.Dogr", deny )
                           return FGAC_OK;
        case FGAC_CAT_OTH: /**< OTHer                                      **/
                           FGAC_SET("user.fgacfs.Aoth", allow)
                           FGAC_SET("user.fgacfs.Doth", deny )
                           return FGAC_OK;
        case FGAC_CAT_UID: /**< User IDentifier                            **/
                           int_to_hex(prm->prc.uid, index);
                           fgac_str_cat2(name1, "user.fgacfs.Auid.", index, 34);
                           fgac_str_cat2(name2, "user.fgacfs.Duid.", index, 34);
                           FGAC_SET(name1, allow)
                           FGAC_SET(name2, deny )
                           return FGAC_OK;
        case FGAC_CAT_GID: /**< Group IDentifier                           **/
                           int_to_hex(prm->prc.gid, index);
                           fgac_str_cat2(name1, "user.fgacfs.Agid.", index, 34);
                           fgac_str_cat2(name2, "user.fgacfs.Dgid.", index, 34);
                           FGAC_SET(name1, allow)
                           FGAC_SET(name2, deny )
                           return FGAC_OK;
        case FGAC_CAT_PEX: /**< Process EXecutable                         **/
                           if (!fgac_str_cat2(name1, "user.fgacfs.Apex.", prm->prc.cmd, FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
                           if (!fgac_str_cat2(name1, "user.fgacfs.Dpex.", prm->prc.cmd, FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
                           FGAC_SET(name1, allow)
                           FGAC_SET(name2, deny )
                           return FGAC_OK;
        default:           return FGAC_ERR_PRMCAT;
    }
}

int fxattr_get_prm (fgac_state *state, fgac_path *path, fgac_prm *prm)
{
    char name1 [FGAC_LIMIT_PATH], name2[FGAC_LIMIT_PATH], index[17];
    int rc;

    switch (prm->cat)
    {
        case FGAC_CAT_ALL: /**< ALL users                                  **/
                           FGAC_GET("user.fgacfs.Aall", allow)
                           FGAC_GET("user.fgacfs.Dall", deny )
                           return FGAC_OK;
        case FGAC_CAT_OUS: /**< Owner USer                                 **/
                           FGAC_GET("user.fgacfs.Aous", allow)
                           FGAC_GET("user.fgacfs.Dous", deny )
                           return FGAC_OK;
        case FGAC_CAT_OGR: /**< Owner Group                                **/
                           FGAC_GET("user.fgacfs.Aogr", allow)
                           FGAC_GET("user.fgacfs.Dogr", deny )
                           return FGAC_OK;
        case FGAC_CAT_OTH: /**< OTHer                                      **/
                           FGAC_GET("user.fgacfs.Aoth", allow)
                           FGAC_GET("user.fgacfs.Doth", deny )
                           return FGAC_OK;
        case FGAC_CAT_UID: /**< User IDentifier                            **/
                           int_to_hex(prm->prc.uid, index);
                           fgac_str_cat2(name1, "user.fgacfs.Auid.", index, 34);
                           fgac_str_cat2(name2, "user.fgacfs.Duid.", index, 34);
                           FGAC_GET(name1, allow)
                           FGAC_GET(name2, deny )
                           return FGAC_OK;
        case FGAC_CAT_GID: /**< Group IDentifier                           **/
                           int_to_hex(prm->prc.gid, index);
                           fgac_str_cat2(name1, "user.fgacfs.Agid.", index, 34);
                           fgac_str_cat2(name2, "user.fgacfs.Dgid.", index, 34);
                           FGAC_GET(name1, allow)
                           FGAC_GET(name2, deny )
                           return FGAC_OK;
        case FGAC_CAT_PEX: /**< Process EXecutable                         **/
                           if (!fgac_str_cat2(name1, "user.fgacfs.Apex.", prm->prc.cmd, FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
                           if (!fgac_str_cat2(name1, "user.fgacfs.Dpex.", prm->prc.cmd, FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
                           FGAC_GET(name1, allow)
                           FGAC_GET(name2, deny )
                           return FGAC_OK;
        default:           return FGAC_ERR_PRMCAT;
    }

}

int fxattr_unset_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm)
{
    char name1 [FGAC_LIMIT_PATH], name2[FGAC_LIMIT_PATH], index[17];
    int rc;

    switch (prm->cat)
    {
        case FGAC_CAT_ALL: /**< ALL users                                  **/
                           FGAC_UNSET("user.fgacfs.Aall")
                           FGAC_UNSET("user.fgacfs.Dall")
                           return FGAC_OK;
        case FGAC_CAT_OUS: /**< Owner USer                                 **/
                           FGAC_UNSET("user.fgacfs.Aous")
                           FGAC_UNSET("user.fgacfs.Dous")
                           return FGAC_OK;
        case FGAC_CAT_OGR: /**< Owner Group                                **/
                           FGAC_UNSET("user.fgacfs.Aogr")
                           FGAC_UNSET("user.fgacfs.Dogr")
                           return FGAC_OK;
        case FGAC_CAT_OTH: /**< OTHer                                      **/
                           FGAC_UNSET("user.fgacfs.Aoth")
                           FGAC_UNSET("user.fgacfs.Doth")
                           return FGAC_OK;
        case FGAC_CAT_UID: /**< User IDentifier                            **/
                           int_to_hex(prm->prc.uid, index);
                           fgac_str_cat2(name1, "user.fgacfs.Auid.", index, 34);
                           fgac_str_cat2(name2, "user.fgacfs.Duid.", index, 34);
                           FGAC_UNSET(name1)
                           FGAC_UNSET(name2)
                           return FGAC_OK;
        case FGAC_CAT_GID: /**< Group IDentifier                           **/
                           int_to_hex(prm->prc.gid, index);
                           fgac_str_cat2(name1, "user.fgacfs.Agid.", index, 34);
                           fgac_str_cat2(name2, "user.fgacfs.Dgid.", index, 34);
                           FGAC_UNSET(name1)
                           FGAC_UNSET(name2)
                           return FGAC_OK;
        case FGAC_CAT_PEX: /**< Process EXecutable                         **/
                           if (!fgac_str_cat2(name1, "user.fgacfs.Apex.", prm->prc.cmd, FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
                           if (!fgac_str_cat2(name1, "user.fgacfs.Dpex.", prm->prc.cmd, FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
                           FGAC_UNSET(name1)
                           FGAC_UNSET(name2)
                           return FGAC_OK;
        default:           return FGAC_ERR_PRMCAT;
    }

}

#define FGAC_PRM_ADD                                  \
    if (fxattr_get_prm(state, path, &prm) != FGAC_OK) \
    {                                                 \
        fgac_prms_free(&prms);                        \
        return fgac_prms_error();                     \
    }                                                 \
    fgac_prms_push (&prms, &prm);                     \
    if (fgac_prms_is_error (&prms)) return prms;      \

fgac_prms fxattr_get_prms (fgac_state *state, fgac_path *path)
{
    char *xlist = NULL, *e, *id;
    fgac_prm prm;
    fgac_prms prms = fgac_prms_init();
    ssize_t s;

    s = llistxattr(FGAC_HOSTPATH, xlist, 0);
    xlist = malloc (s);
    if (!xlist || llistxattr(FGAC_HOSTPATH, xlist, s) != s) return fgac_prms_error();

    for (e = xlist + s - 16; xlist < e; xlist += strlen(xlist) + 1)
    {
        if (!memcmp(xlist, "user.fgacfs.Aall", 16))
        {
            prm.cat = FGAC_CAT_ALL;
            FGAC_PRM_ADD
        }
        else if (!memcmp(xlist, "user.fgacfs.Aous", 16))
        {
            prm.cat = FGAC_CAT_OUS;
            FGAC_PRM_ADD
        }
        else if (!memcmp(xlist, "user.fgacfs.Aogr", 16))
        {
            prm.cat = FGAC_CAT_OGR;
            FGAC_PRM_ADD
        }
        else if (!memcmp(xlist, "user.fgacfs.Aoth", 16))
        {
            prm.cat = FGAC_CAT_OTH;
            FGAC_PRM_ADD
        }
        else if (xlist + 1 < e && !memcmp(xlist, "user.fgacfs.Auid.", 17))
        {
            uint64_t uid;
            id = xlist + 17;
            if (!hex_to_int(id, &uid))
            {
                fgac_prms_free(&prms);
                return fgac_prms_error();
            }
            prm.cat = FGAC_CAT_UID;
            prm.prc.uid = uid;
            FGAC_PRM_ADD
        }
        else if (xlist + 1 < e && !memcmp(xlist, "user.fgacfs.Agid.", 17))
        {
            uint64_t gid;
            id = xlist + 17;
            if (!hex_to_int(id, &gid))
            {
                fgac_prms_free(&prms);
                return fgac_prms_error();
            }
            prm.prc.gid = gid;
            prm.cat = FGAC_CAT_GID;
            FGAC_PRM_ADD
        }
        else if (xlist + 1 < e && !memcmp(xlist, "user.fgacfs.Apex.", 17))
        {
            prm.cat = FGAC_CAT_PEX;
            prm.prc.cmd = xlist + 17;
            FGAC_PRM_ADD
        }
    }
    return prms;

}
