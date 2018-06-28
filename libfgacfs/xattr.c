/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#include <config.h>
#include "fgacfs.h"
#include "xattr.h"
#include <sys/xattr.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <attr/xattr.h>

int xattr_close (fgac_state *state)
{
    (void) state;
    return FGAC_OK;
}

int xattr_delete (fgac_state *state, fgac_path *path)
{
    (void) state;
    (void) path;
    return FGAC_OK;
}

int xattr_rename (fgac_state *state, fgac_path *path, const char* newpath)
{
    (void) state;
    (void) path;
    (void) newpath;
    return FGAC_OK;
}


int int_to_hex (uint64_t i, char* hex)
{
    return snprintf (hex, 17, "%016" PRIX64, i) != 16;
}

int hex_to_int (const char* hex, uint64_t *i)
{
    return sscanf (hex, "%" SCNx64, i) != -1;
/*
    printf("hex=%s ", hex);
    int rc=sscanf (hex, "%" SCNx64, i);
    printf("rc=%i i=%llu rv=%i\n", rc, (long long unsigned) *i, rc != -1);
    return rc != -1;
*/    
}

uint64_t CityHash64(const char *s);

int xattr_open (fgac_state *state)
{
     char value[5];
     if (lgetxattr(state->datadir, "user.fgacfs", value, 6) != 6 ||
         memcmp(value, "xattr", 6)
        )
         return FGAC_ERR_XATTROPEN;

     state->type = FGAC_XATTR;
     return FGAC_OK;
}

int xattr_create (const char *hostdir)
{
    char datadir[FGAC_LIMIT_PATH];
    if (!fgac_str_cat2 (datadir, hostdir, "/data", FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
    return lsetxattr(datadir, "user.fgacfs", "xattr", 6, XATTR_CREATE) ? FGAC_ERR_XATTRCRT : FGAC_OK;
}

int xattr_set (fgac_state *state, fgac_path *path, const char *name, const void *value, ssize_t s)
{
    return lsetxattr(FGAC_HOSTPATH, name, value, s, 0) ? FGAC_ERR_XATTRCRT : FGAC_OK;
}

ssize_t xattr_size (fgac_state *state, fgac_path *path, const char *name)
{
     char value;
     ssize_t s = lgetxattr(FGAC_HOSTPATH, name, &value, 0);
     return s < 0 && errno == ENOATTR ? 0 : s;
}


/*
 * value - buffer return value
 * bs    - buffer size
 * rs    - real xattr string size, including '\0' if used (rs could be NULL if not needed)
 * return ERR_XATRRFGACM in case of too small buffer (rs is set accordingly to real size)
 */
int xattr_get (fgac_state *state, fgac_path *path, const char *name, void *value, ssize_t bs, ssize_t *rs)
{
     ssize_t s = lgetxattr(FGAC_HOSTPATH, name, value, 0);
     int rc;

     if (rs) *rs = s;

     if (s < 0 && errno == ENOATTR)
     {
         if (rs) *rs = 0;
         return FGAC_OK;
     }

     if (s > bs) return FGAC_ERR_XATTRFMT;

     rc = lgetxattr(FGAC_HOSTPATH, name, value, s);

     if (rc != s) return FGAC_ERR_XATTRFMT;

     return FGAC_OK;
}

int xattr_unset (fgac_state *state, fgac_path *path, const char *name)
{
    int r = lremovexattr(FGAC_HOSTPATH, name);
    return (r && errno != ENOATTR) ? FGAC_ERR_XATTRCRT : FGAC_OK;
}

int xattr_get_int (fgac_state *state, fgac_path *path, const char *name, uint64_t *attr)
{
     int rc;
     ssize_t rs;
     char value[17];

     if ((rc = xattr_get(state, path, name, value, 16, &rs))) return rc;

     if (rs == 0)
     {
         *attr = 0;
         return FGAC_OK;
     }

     if (rs != 16) return FGAC_ERR_XATTROPEN;

     value[16] = '\0';
     if (!hex_to_int(value, attr)) return FGAC_ERR_XATTRFMT;

     return FGAC_OK;

}

int xattr_set_int (fgac_state *state, fgac_path *path, const char *name, uint64_t attr)
{
    char value[17];
    int rc;

    if (int_to_hex(attr, value)) return FGAC_ERR_XATTRFMT;

    if ((rc = xattr_set(state, path, name, value, 16))) return rc;

    return FGAC_OK;

}

int xattr_set_string (fgac_state *state, fgac_path *path, const char *name, const char *str)
{
    size_t s = 2 * strlen(str);
    void *value = malloc (s);
    char *o = value;
    const char *i;
    int rc;

    if (!value) return FGAC_ERR_MALLOC;

    for (i = str; *i; ++i, o += 2)
        snprintf (o, 3, "%02X", (unsigned int) (unsigned char) *i);

    rc = xattr_set(state, path, name, value, s);

    free(value);

    return rc;
}

/*
 * bs - buffer size including '\0'
 */
int xattr_get_string (fgac_state *state, fgac_path *path, const char *name, char *str, size_t bs)
{
    size_t s = 2 * (bs - 1);
    char *value = malloc (s), *i = value, *e;
    char buffer[3], *o = str;
    int rc, v;
    ssize_t rs;

    if (!value) return FGAC_ERR_MALLOC;

    if ((rc = xattr_get(state, path, name, value, s, &rs))) return rc;
    if (rs % 2) return FGAC_ERR_XATTRFMT;

    buffer[2] = '\0';
    for (e = value + rs; i < e; i += 2, ++o)
    {
        buffer[0] = *((char*) i);
        buffer[1] = *(((char*) i) + 1);

        if (sscanf (buffer, "%x", &v) != 1) return FGAC_ERR_XATTRFMT;

        *o = (char) v;
    }
    *o = '\0';
    return FGAC_OK;

}

int xattr_set_hash (fgac_state *state, fgac_path *path, const char *name, const void *attr, ssize_t s)
{
    char hash_name [46] = "user.fgacfs.N", hash_value [46] = "user.fgacfs.V";

    char value [FGAC_LIMIT_PATH];

    uint64_t i;
    int rc;

    char hash  [17];
    char index [17];

    if (strlen (name) >= FGAC_LIMIT_PATH) return FGAC_ERR_XATTRFMT;

    int_to_hex (CityHash64(name), hash);
    strcpy (hash_name  + 13, hash);
    strcpy (hash_value + 13, hash);

    i = 0;
    while (i < FGAC_LIMIT_XHASH)
    {
        int_to_hex(i, index);
        strcpy (hash_name + 29, index);

        if ((rc = xattr_get_string(state, path, hash_name, value, FGAC_LIMIT_PATH)) != FGAC_OK) return rc;

        if (!*value)
        {
            if ((rc = xattr_set_string(state, path, hash_name, name)) != FGAC_OK) return rc;
            strcpy (hash_value + 29, index);
            return xattr_set (state, path, hash_value, attr, s);
        }

        if (!strcmp(value, name))
        {
            strcpy (hash_value + 29, index);
            return xattr_set (state, path, hash_value, attr, s);
        }

        ++i;
    };

    return FGAC_ERR_XATTROPEN;

}

int xattr_get_hash (fgac_state *state, fgac_path *path, const char *name, char *attr, ssize_t bs, ssize_t *rs)
{
    char hash_name [46] = "user.fgacfs.N", hash_value [46] = "user.fgacfs.V";

    char value [FGAC_LIMIT_PATH];

    uint64_t i;
    int rc;

    char hash  [17];
    char index [17];

    if (strlen (name) >= FGAC_LIMIT_PATH) return FGAC_ERR_XATTRFMT;

    int_to_hex (CityHash64(name), hash);
    strcpy (hash_name  + 13, hash);
    strcpy (hash_value + 13, hash);

    i = 0;
    while (i < FGAC_LIMIT_XHASH)
    {
        int_to_hex(i, index);
        strcpy (hash_name + 29, index);

        if ((rc = xattr_get_string(state, path, hash_name, value, FGAC_LIMIT_PATH)) != FGAC_OK) return rc;

        if (!*value)
        {
            *rs = 0;
            *attr = '\0';

            return FGAC_OK;
        }

        if (strcmp(value, name) == 0)
        {
            strcpy (hash_value + 29, index);
            return xattr_get (state, path, hash_value, attr, bs, rs);
        }

        ++i;
    };

    return FGAC_ERR_XATTROPEN;

}

int xattr_unset_hash (fgac_state *state, fgac_path *path, const char *name)
{
    char hash_name [46] = "user.fgacfs.N", hash_value [46] = "user.fgacfs.V";

    char value [FGAC_LIMIT_PATH];

    uint64_t i;
    int rc;

    char hash  [17];
    char index [17];

    uint64_t pos = 0, last = 0;

    if (strlen (name) >= FGAC_LIMIT_PATH) return FGAC_ERR_XATTRFMT;

    int_to_hex (CityHash64(name), hash);
    strcpy (hash_name  + 13, hash);
    strcpy (hash_value + 13, hash);

    i = 0;
    while (i < FGAC_LIMIT_XHASH)
    {
        int_to_hex(i, index);
        strcpy (hash_name + 29, index);

        if ((rc = xattr_get_string(state, path, hash_name, value, FGAC_LIMIT_PATH)) != FGAC_OK) return rc;

        if (!*value) return FGAC_OK;

        if (!strcmp(value, name))
        {
            strcpy (hash_value + 29, index);

            pos = i++;
            last = pos;

            break;
        }

        ++i;
    };

    while (i < FGAC_LIMIT_XHASH)
    {
        int_to_hex(i, index);
        strcpy (hash_name + 29, index);

        if ((rc = xattr_get_string(state, path, hash_name, value, FGAC_LIMIT_PATH)) != FGAC_OK) return rc;

        if (!*value) break;

        last = i;
        ++i;

    }

    if (last != pos)
    {
        void *buffer;
        ssize_t lsize, rlsize;

        int_to_hex(last, index);
        strcpy (hash_name  + 29, index);
        strcpy (hash_value + 29, index);

        lsize = xattr_size(state, path, hash_value);
        if (lsize <= 0) return FGAC_ERR_XATTRFMT;
        buffer = malloc (lsize);
        if (!buffer) return FGAC_ERR_MALLOC;

        if ((rc = xattr_get_string(state, path, hash_name,  value, FGAC_LIMIT_PATH  )) != FGAC_OK) return rc;
        if ((rc = xattr_get       (state, path, hash_value, buffer, lsize, &rlsize)) != FGAC_OK) return rc;
        xattr_unset(state, path, hash_name);
        xattr_unset(state, path, hash_value);

        int_to_hex(pos, index);
        strcpy (hash_name  + 29, index);
        strcpy (hash_value + 29, index);

        if ((rc = xattr_set_string(state, path, hash_name, value           )) != FGAC_OK) return rc;
        if ((rc = xattr_set       (state, path, hash_value, buffer, rlsize )) != FGAC_OK) return rc;
    }

    return FGAC_OK;
}


int xattr_set_inh (fgac_state *state, fgac_path *path, uint64_t inh)
{
    return xattr_set_int (state, path, "user.fgacfs.inh", inh);
}

int xattr_get_inh (fgac_state *state, fgac_path *path, uint64_t *inh)
{
    return xattr_get_int (state, path, "user.fgacfs.inh", inh);
}

int xattr_set_owner (fgac_state *state, fgac_path *path, uid_t uid)
{
    return xattr_set_int (state, path, "user.fgacfs.owner", uid);
}

int xattr_set_group (fgac_state *state, fgac_path *path, gid_t gid)
{
    return xattr_set_int (state, path, "user.fgacfs.group", gid);
}

int xattr_get_owner (fgac_state *state, fgac_path *path, uid_t *uid)
{
    uint64_t attr;
    int rc = xattr_get_int (state, path, "user.fgacfs.owner", &attr);
    if (rc) return rc;

    *uid = attr;

    return rc;
}

int xattr_get_group (fgac_state *state, fgac_path *path, uid_t *uid)
{
    uint64_t attr;
    int rc = xattr_get_int (state, path, "user.fgacfs.group", &attr);
    if (rc) return rc;

    *uid = attr;

    return rc;
}

int xattr_add (fgac_state *state, fgac_path *path, uid_t uid, gid_t gid)
{
    int rc;
    rc = xattr_set_owner(state, path, uid);
    if (rc) return rc;

    rc = xattr_set_group(state, path, gid);
    if (rc) return rc;

    return FGAC_OK;
}



int xattr_unset_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm)
{
    char name[33], index[17];
    switch (prm->cat)
    {
        case FGAC_CAT_ALL: /**< ALL users                                  **/
                           return xattr_unset(state, path, "user.fgacfs.all");
        case FGAC_CAT_OUS: /**< Owner USer                                 **/
                           return xattr_unset(state, path, "user.fgacfs.ous");
        case FGAC_CAT_OGR: /**< Owner Group                                **/
                           return xattr_unset(state, path, "user.fgacfs.ogr");
        case FGAC_CAT_OTH: /**< OTHer                                      **/
                           return xattr_unset(state, path, "user.fgacfs.oth");
        case FGAC_CAT_UID: /**< User IDentifier                            **/
                           int_to_hex(prm->prc.uid, index);
                           fgac_str_cat2(name, "user.fgacfs.uid.", index, 33);
                           return xattr_unset(state, path, name);
        case FGAC_CAT_GID: /**< Group IDentifier                           **/
                           int_to_hex(prm->prc.gid, index);
                           fgac_str_cat2(name, "user.fgacfs.gid.", index, 33);
                           return xattr_unset(state, path, name);
        case FGAC_CAT_PEX: /**< Process EXecutable                         **/
                           return xattr_unset_hash(state, path, prm->prc.cmd);
        default:           return FGAC_ERR_PRMCAT;
    }

    return FGAC_OK;

}


int xattr_set_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm)
{
    if (!(prm->allow | prm->deny)) return xattr_unset_prm(state, path, prm);
    char allow[17], deny [17],
         name [33], index[17],
         value[33];

    int_to_hex(prm->allow, allow);
    int_to_hex(prm->deny,  deny );

    fgac_str_cat2 (value, allow, deny, 33);

    switch (prm->cat)
    {
        case FGAC_CAT_ALL: /**< ALL users                                  **/
                           return xattr_set(state, path, "user.fgacfs.all", value, 32);
        case FGAC_CAT_OUS: /**< Owner USer                                 **/
                           return xattr_set(state, path, "user.fgacfs.ous", value, 32);
        case FGAC_CAT_OGR: /**< Owner Group                                **/
                           return xattr_set(state, path, "user.fgacfs.ogr", value, 32);
        case FGAC_CAT_OTH: /**< OTHer                                      **/
                           return xattr_set(state, path, "user.fgacfs.oth", value, 32);
        case FGAC_CAT_UID: /**< User IDentifier                            **/
                           int_to_hex(prm->prc.uid, index);
                           fgac_str_cat2(name, "user.fgacfs.uid.", index, 33);
                           return xattr_set(state, path, name, value, 32);
        case FGAC_CAT_GID: /**< Group IDentifier                           **/
                           int_to_hex(prm->prc.gid, index);
                           fgac_str_cat2(name, "user.fgacfs.gid.", index, 33);
                           return xattr_set(state, path, name, value, 32);
        case FGAC_CAT_PEX: /**< Process EXecutable                         **/
                           return xattr_set_hash(state, path, prm->prc.cmd, value, 32);
        default:           return FGAC_ERR_PRMCAT;
    }
    return FGAC_OK;
}

int xattr_get_prm (fgac_state *state, fgac_path *path, fgac_prm *prm)
{
    char name [33], index[17],
         value[33];
    int rc;
    ssize_t s;

    switch (prm->cat)
    {
        case FGAC_CAT_ALL: /**< ALL users                                  **/

                           rc = xattr_get(state, path, "user.fgacfs.all", value, 33, &s);
                           if (rc != FGAC_OK || (s != 32 && s != 0)) return FGAC_ERR_XATTRFMT;

                           break;
        case FGAC_CAT_OUS: /**< Owner USer                                 **/

                           rc = xattr_get(state, path, "user.fgacfs.ous", value, 33, &s);
                           if (rc != FGAC_OK || (s != 32 && s != 0)) return FGAC_ERR_XATTRFMT;

                           break;

        case FGAC_CAT_OGR: /**< Owner Group                                **/

                           rc = xattr_get(state, path, "user.fgacfs.ogr", value, 33, &s);
                           if (rc != FGAC_OK || (s != 32 && s != 0)) return FGAC_ERR_XATTRFMT;

                           break;

        case FGAC_CAT_OTH: /**< OTHer                                      **/

                           rc = xattr_get(state, path, "user.fgacfs.oth", value, 33, &s);
                           if (rc != FGAC_OK || (s != 32 && s != 0)) return FGAC_ERR_XATTRFMT;

                           break;

        case FGAC_CAT_UID: /**< User IDentifier                            **/
                           int_to_hex(prm->prc.uid, index);
                           fgac_str_cat2(name, "user.fgacfs.uid.", index, 33);

                           rc = xattr_get(state, path, name, value, 33, &s);
                           if (rc != FGAC_OK || (s != 32 && s != 0)) return FGAC_ERR_XATTRFMT;

                           break;

        case FGAC_CAT_GID: /**< Group IDentifier                           **/
                           int_to_hex(prm->prc.gid, index);
                           fgac_str_cat2(name, "user.fgacfs.gid.", index, 33);

                           rc = xattr_get(state, path, name, value, 33, &s);
                           if (rc != FGAC_OK || (s != 32 && s != 0)) return FGAC_ERR_XATTRFMT;

                           break;

        case FGAC_CAT_PEX: /**< Process EXecutable                         **/
                           rc = xattr_get_hash(state, path, prm->prc.cmd, value, 33, &s);
                           if (rc != FGAC_OK || (s != 32 && s != 0)) return FGAC_ERR_XATTRFMT;
                           break;

        default:           return FGAC_ERR_PRMCAT;
    }

    if (s)
    {
        value[32] = '\0';
        hex_to_int(value + 16, &(prm->deny ));
        value[16] = '\0';
        hex_to_int(value     , &(prm->allow));
    }
    else
    {
        prm->allow = prm->deny = 0;
    }

    return FGAC_OK;
}

#define FGAC_PRM_ADD                                \
    if (xattr_get_prm(state,path, &prm) != FGAC_OK) \
    {                                               \
        fgac_prms_free(&prms);                      \
        return fgac_prms_error();                   \
    }                                               \
    fgac_prms_push (&prms, &prm);                   \
    if (fgac_prms_is_error (&prms)) return prms;    \

fgac_prms xattr_get_prms (fgac_state *state, fgac_path *path)
{
    char *xlist = NULL;
    fgac_prms prms = fgac_prms_init();
    ssize_t i = 0, s = llistxattr(FGAC_HOSTPATH, xlist, 0);

    xlist = malloc (s);
    if (!xlist || llistxattr(FGAC_HOSTPATH, xlist, s) != s) return fgac_prms_error();

    while (i < s)
    {
        char *attr = xlist + i, *id;
        fgac_prm prm;
        i += strlen (attr)+1;
        if (!memcmp(attr, "user.fgacfs.all", 15))
        {
            prm.cat = FGAC_CAT_ALL;
            FGAC_PRM_ADD
        }
        else if (!memcmp(attr, "user.fgacfs.ous", 15))
        {
            prm.cat = FGAC_CAT_OUS;
            FGAC_PRM_ADD
        }
        else if (!memcmp(attr, "user.fgacfs.ogr", 15))
        {
            prm.cat = FGAC_CAT_OGR;
            FGAC_PRM_ADD
        }
        else if (!memcmp(attr, "user.fgacfs.oth", 15))
        {
            prm.cat = FGAC_CAT_OTH;
            FGAC_PRM_ADD
        }
        else if (!memcmp(attr, "user.fgacfs.uid.", 16))
        {
            uint64_t uid;
            id = attr + 16;
            
            if (!hex_to_int(id, &uid))
            {
                fgac_prms_free(&prms);
                return fgac_prms_error();
            }
            prm.cat = FGAC_CAT_UID;
            prm.prc.uid = uid;
            FGAC_PRM_ADD
        }
        else if (!memcmp(attr, "user.fgacfs.gid.", 16))
        {
            uint64_t gid;
            id = attr + 16;
            if (!hex_to_int(id, &gid))
            {
                fgac_prms_free(&prms);
                return fgac_prms_error();
            }
            prm.prc.gid = gid;
            prm.cat = FGAC_CAT_GID;
            FGAC_PRM_ADD
        }
        else if (!memcmp(attr, "user.fgacfs.N", 13))
        {
            char exec [FGAC_LIMIT_PATH];
            if (xattr_get_string (state, path, attr, exec, FGAC_LIMIT_PATH) != FGAC_OK)
            {
                fgac_prms_free(&prms);
                return fgac_prms_error();
            }

            prm.cat = FGAC_CAT_PEX;
            prm.prc.cmd = exec;
            FGAC_PRM_ADD
        }
    }
    return prms;
}
