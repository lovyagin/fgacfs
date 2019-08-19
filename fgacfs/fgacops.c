/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.

  This code is based on bbfs fuse-tutorial code
  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>
*/
#include <libgen.h>
#include <config.h>
#include "fgacops.h"
#include <fgacfs.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <sys/time.h>
#include <stdio.h>

#define FGACFS_INIT                                                                      \
    int rc;                                                                              \
    fgac_prc fgacprc, *prc = &fgacprc;                                                   \
    fgac_state *state = ((fgac_state *) fuse_get_context()->private_data);               \
    fgac_path fgacpath = fgac_path_init((char *)rpath), *path = &fgacpath;               \
    char pex [FGAC_LIMIT_PATH];                                                          \
    char *hostpath;                                                                      \
    (void) hostpath;                                                                     \
    (void) rc;                                                                           \
                                                                                         \
    fgacprc.uid = fuse_get_context()->uid;                                               \
    fgacprc.gid = fuse_get_context()->gid;                                               \
    fgacprc.cmd = NULL;                                                                  \
                                                                                         \
    if (state->check_prexec)                                                             \
    {                                                                                    \
        char exe [FGAC_LIMIT_PATH];                                                      \
        ssize_t rs;                                                                      \
        snprintf (exe, FGAC_LIMIT_PATH, "/proc/%u/exe", fuse_get_context()->pid);        \
        if ((rs = readlink(exe, pex, FGAC_LIMIT_PATH)) > 0 && rs < FGAC_LIMIT_PATH -1)   \
        {                                                                                \
            pex[rs] = '\0';                                                              \
            fgacprc.cmd = pex;                                                           \
        }                                                                                \
        FGAC_DEBUG_REQUEST                                                               \
    }                                                                                    \
    fgacprc.ngroups = fuse_getgroups(FGAC_LIMIT_GROUPS, fgacprc.groups);                 \
    if (fgacprc.ngroups < 0 || fgacprc.ngroups > FGAC_LIMIT_GROUPS) fgacprc.ngroups = 0;

#ifndef NDEBUG                                                                     
#define FGAC_DEBUG_REQUEST                                                               \
    printf("REQUEST FROM: uid=%u, gid=%u cmd='%s' exe='%s' rs=%i\n",                     \
                (unsigned) fgacprc.uid,                                                  \
                (unsigned) fgacprc.gid,                                                  \
                fgacprc.cmd,                                                             \
                exe,                                                                     \
                (int) rs                                                                 \
          );                                                                       
#else
#define FGAC_DEBUG_REQUEST
#endif                                                                             


#define FGACFS_INIT_PARENT                                                               \
    char pbuffer[FGAC_LIMIT_PATH];                                                       \
    fgac_path fgacparent = fgac_path_init(pbuffer), *parent = &fgacparent;               \
    FGACFS_INIT                                                                          \
    if (!fgac_parent(path, parent)) parent = NULL;                                       \
    if (!fgac_is_dir(state, parent)) return -ENOTDIR;


#define FGACFS_EXISTS if (!fgac_exists(state, path)) return -ENOENT;
#define FGACFS_PARENT_PRM(prm) if (!fgac_check_prm(state, parent, prc, FGAC_PRM_##prm)) return -EACCES;
#define FGACFS_PRM(prm) if (!fgac_check_prm(state, path, prc, FGAC_PRM_##prm)) return -EACCES;
#define FGACFS_PRM2(prm) if (!fgac_check_prm2(state, path, prc, FGAC_PRM_F##prm, FGAC_PRM_D##prm)) return -EACCES;

#define FGACFS_CALL(call) if ((rc = call) < 0) rc = -errno;

#define FGACFS_RETCALL(call) return (call) < 0 ? -errno : 0;

#define FGACFS_FAILCALL(call) if ((rc = call) < 0) return -errno;

#define FGACFS_HOSTPATH hostpath = FGAC_HOSTPATH; if (!hostpath) return -EFAULT;

int fgacfs_getattr (const char *rpath, struct stat *statbuf)
{
    struct stat *stat;

    FGACFS_INIT
    FGACFS_EXISTS
    FGACFS_PRM2(RA)

    stat = fgac_stat(state, path, prc);

    if (!stat) return -errno;
    *statbuf = *stat;

    if (prc->uid == 0 && !path->is_dir)
    {
#ifndef NDEBUG    
        printf ("1\n");        
#endif        
        fgac_prms prms = fgac_get_allprms (state, path);
        
        if (!fgac_prms_is_error(&prms))
        {
            size_t i;
            for (i = 0; i < fgac_prms_size(&prms); ++i)
            {
#ifndef NDEBUG    
        printf ("2 %lX %llX\n",fgac_prms_get(&prms, i)->allow, FGAC_PRM_FEX);        
#endif        
                 if (fgac_prms_get(&prms, i)->allow & FGAC_PRM_FEX) 
                 {
#ifndef NDEBUG    
        printf ("3\n");        
#endif        
                     statbuf->st_mode |= S_IXOTH;
                     break;
                 }
            }
            fgac_prms_free(&prms);
        }
    }

    return 0;
}

int fgacfs_readlink (const char *rpath, char *buffer, size_t s)
{
    FGACFS_INIT
    FGACFS_EXISTS
    FGACFS_PRM(FSL)
    FGACFS_HOSTPATH

    FGACFS_RETCALL(readlink(hostpath, buffer, s - 1));
}


#define CHECK_OWN(t)                         \
    if      (pprm & FGAC_PRM_DO##t) own = 1; \
    else if (pprm & FGAC_PRM_DA##t) own = 0; \
    else return -EACCES;

#define ADD(addf,rmf)                            \
    if (own)                                     \
    {                                            \
        uid = prc->uid;                          \
        gid = prc->gid;                          \
    }                                            \
    else                                         \
    {                                            \
        if (fgac_get_owner(state, parent, &uid)) \
        {                                        \
            if (!source) rmf (hostpath);         \
            return -EACCES;                      \
        }                                        \
        if (fgac_get_group(state, parent, &gid)) \
        {                                        \
            if (!source) rmf (hostpath);         \
            return -EACCES;                      \
        }                                        \
    }                                            \
                                                 \
    if (fgac_add (state, path, uid, gid))        \
    {                                            \
printf("1\n");\
        if (!source) rmf (hostpath);             \
        return -EACCES;                          \
    }                                            \
                                                 \
    if (fgac_set_##addf##_prm (state, path))     \
    {                                            \
printf("2\n");\
        if (!source) rmf (hostpath);             \
        fgac_delete (state, path);               \
        return -EACCES;                          \
    }                                            \
                                                                                              

#define ADD_FILE ADD(mkfile, unlink)
#define ADD_DIR  ADD(mkdir,  rmdir )

#define ADD_ORD_PRM_SINGLE(type,df)                                   \
    prm->allow = prm->deny = 0;                                       \
    if (S_IR##type & stat->st_mode) prm->allow |= FGAC_PRM_##df##REA; \
    if (S_IW##type & stat->st_mode) prm->allow |= FGAC_PRM_##df##WRI; \
    if (S_IX##type & stat->st_mode) prm->allow |= FGAC_PRM_##df##EXE; \
    fgac_set_prm(state, path, prm);                                   \


#define ADD_ORD_PRM_FILE                                 \
    if (                                                 \
        (!fgac_check_inh(state, parent, FGAC_INH_SFI) && \
         !fgac_check_inh(state, parent, FGAC_INH_FPC)    \
        ) ||                                             \
        fgac_check_prm(state, path, prc, FGAC_PRM_FCP)   \
       )                                                 \
    {                                                    \
        fgac_prm fgacprm, *prm = &fgacprm;               \
                                                         \
        prm->cat   = FGAC_CAT_UID;                       \
        prm->prc.uid = prc->uid;                         \
        ADD_ORD_PRM_SINGLE(USR,F)                        \
                                                         \
        prm->cat   = FGAC_CAT_GID;                       \
        prm->prc.gid = prc->gid;                         \
        ADD_ORD_PRM_SINGLE(GRP,F)                        \
                                                         \
        prm->cat   = FGAC_CAT_OUS;                       \
        prm->allow = prm->deny = 0;                      \
        ADD_ORD_PRM_SINGLE(USR,F)                        \
                                                         \
        prm->cat   = FGAC_CAT_OGR;                       \
        ADD_ORD_PRM_SINGLE(GRP,F)                        \
                                                         \
        prm->cat   = FGAC_CAT_OTH;                       \
        ADD_ORD_PRM_SINGLE(OTH,F)                        \
                                                         \
        prm->cat   = FGAC_CAT_ALL;                       \
        prm->allow = prm->deny = 0;                      \
        if (S_ISUID & stat->st_mode)                     \
            prm->allow |= FGAC_PRM_FSU;                  \
        if (S_ISGID & stat->st_mode)                     \
            prm->allow |= FGAC_PRM_FSG;                  \
        fgac_set_prm(state, path, prm);                  \
    }

#define ADD_ORD_PRM_DIRS                                 \
    if (                                                 \
        (!fgac_check_inh(state, parent, FGAC_INH_SDI) && \
         !fgac_check_inh(state, parent, FGAC_INH_DPC)    \
        ) ||                                             \
         fgac_check_prm(state, path, prc, FGAC_PRM_DCP)  \
       )                                                 \
    {                                                    \
        fgac_prm fgacprm, *prm = &fgacprm;               \
                                                         \
        prm->cat   = FGAC_CAT_UID;                       \
        prm->prc.uid = prc->uid;                         \
        ADD_ORD_PRM_SINGLE(USR,D)                        \
                                                         \
        prm->cat   = FGAC_CAT_GID;                       \
        prm->prc.gid = prc->gid;                         \
        ADD_ORD_PRM_SINGLE(GRP,D)                        \
                                                         \
        prm->cat   = FGAC_CAT_OUS;                       \
        prm->allow = prm->deny = 0;                      \
        ADD_ORD_PRM_SINGLE(USR,D)                        \
                                                         \
        prm->cat   = FGAC_CAT_OGR;                       \
        ADD_ORD_PRM_SINGLE(GRP,D)                        \
                                                         \
        prm->cat   = FGAC_CAT_OTH;                       \
        ADD_ORD_PRM_SINGLE(OTH,D)                        \
    }


#define ADD_ORD_PRM_SL                                   \
    if (                                                 \
        (!fgac_check_inh(state, parent, FGAC_INH_SFI) && \
         !fgac_check_inh(state, parent, FGAC_INH_FPC)    \
        ) ||                                             \
        fgac_check_prm(state, path, prc, FGAC_PRM_FCP)   \
       )                                                 \
    {                                                    \
        fgac_prm fgacprm, *prm = &fgacprm;               \
                                                         \
        prm->cat   = FGAC_CAT_UID;                       \
        prm->prc.uid = prc->uid;                         \
        prm->deny = 0;                                   \
        prm->allow = FGAC_PRM_FILE;                      \
                                                         \
        prm->cat   = FGAC_CAT_GID;                       \
        prm->prc.gid = prc->gid;                         \
        prm->deny = 0;                                   \
        prm->allow = FGAC_PRM_FILE;                      \
                                                         \
        prm->cat   = FGAC_CAT_OUS;                       \
        prm->allow = prm->deny = 0;                      \
        prm->deny = 0;                                   \
        prm->allow = FGAC_PRM_FILE;                      \
                                                         \
        prm->cat   = FGAC_CAT_OGR;                       \
        prm->deny = 0;                                   \
        prm->allow = FGAC_PRM_FILE;                      \
                                                         \
        prm->cat   = FGAC_CAT_OTH;                       \
        prm->deny = 0;                                   \
        prm->allow = FGAC_PRM_FILE;                      \
    }
/*
#define CHOWNMOD                                                           \
    if (source)                                                            \
    {                                                                      \
        uint64_t inh, newinh;                                              \
        fgac_prms prms = fgac_get_prms (state, source);                    \
        fgacfs_chown (rpath, uid, gid);                                    \
        if (fgac_prms_is_error(&prms)) return -EFAULT;                     \
        if (fgac_get_inh (state, path,   &newinh)) return -EFAULT;         \
        if (fgac_get_inh (state, source, &inh)) return -EFAULT;            \
                                                                           \
        if (inh != newinh)                                                 \
        {                                                                  \
            int ci = fgac_check_prm2(state, path, prc,                     \
                                   FGAC_PRM_FCI, FGAC_PRM_DCI              \
                                  ),                                       \
                ct = fgac_is_dir(state, path) &&                           \
                     fgac_check_prm(state, path, prc, FGAC_PRM_DCT);       \
            if (!ci)                                                       \
                inh = (inh & ~FGAC_INH_INHS) | (newinh & FGAC_INH_INHS);   \
            if (!ct)                                                       \
                inh = (inh & ~FGAC_INH_TRMS) | (newinh & FGAC_INH_TRMS);   \
                                                                           \
            if (inh != newinh)                                             \
            {                                                              \
                                                                           \
                if ((inh & FGAC_INH_DPI) && !(newinh & FGAC_INH_DPI))      \
                    fgac_unset_dpi (state, path);                          \
                if ((inh & FGAC_INH_FPI) && !(newinh & FGAC_INH_FPI))      \
                    fgac_unset_fpi (state, path);                          \
                else                                                       \
                    fgac_set_inh(state, path, inh);                        \
            }                                                              \
        }                                                                  \
        if (fgac_check_prm2(state, path, prc, FGAC_PRM_FCP, FGAC_PRM_DCP)) \
        {                                                                  \
            size_t i, s = fgac_prms_size(&prms);                           \
            for (i = 0; i < s; ++i)                                        \
            {                                                              \
                fgac_prm prm = *fgac_prms_get(&prms, i);                   \
                fgac_get_prm(state, path, &prm);                           \
                prm.allow |= fgac_prms_get(&prms, i)->allow;               \
                prm.deny  |= fgac_prms_get(&prms, i)->deny;                \
                fgac_set_prm(state, path, &prm);                           \
            }                                                              \
        }                                                                  \
    }
*/

int fgacfs_add(const char  *rpath,
               struct stat *stat,
               int         source,
               const char  *target
              )
{
    int own;
    uint64_t pprm;
    uid_t uid;
    gid_t gid;
    mode_t hostmode = stat->st_mode;
    FGACFS_INIT_PARENT
    FGACFS_HOSTPATH
    if (!parent) return -EACCES;
    pprm = fgac_check_prms(state, parent, prc, 1);

    hostmode &= ~((mode_t) 07777);
    hostmode |= S_IRWXU;

    if (S_ISREG(stat->st_mode))
    {
        int fd;
        

#ifndef NDEBUG
        printf("pprm %lX DOF %llX DAF %llX DOF %llX\n", pprm, pprm & FGAC_PRM_DOF, pprm & FGAC_PRM_DAF, FGAC_PRM_DOF);
#endif        

        
        CHECK_OWN(F)

        if (!source)
        {
            fd = open(hostpath, O_CREAT | O_EXCL | O_WRONLY, hostmode);
            if (fd < 0) return -errno;
            close(fd);
        }

        ADD_FILE
        ADD_ORD_PRM_FILE
//        CHOWNMOD
    }
    else if (S_ISCHR(stat->st_mode))
    {
        CHECK_OWN(C)
        if (!source) FGACFS_FAILCALL(mknod(hostpath, hostmode, stat->st_dev))
        ADD_FILE
        ADD_ORD_PRM_FILE
//        CHOWNMOD
    }
    else if (S_ISBLK(stat->st_mode))
    {
        CHECK_OWN(B)
        if (!source) FGACFS_FAILCALL(mknod(hostpath, hostmode, stat->st_dev))
        ADD_FILE
        ADD_ORD_PRM_FILE
//        CHOWNMOD
    }
    else if (S_ISFIFO(stat->st_mode))
    {
        CHECK_OWN(P)
        if (!source) FGACFS_FAILCALL(mkfifo(hostpath, hostmode))
        ADD_FILE
        ADD_ORD_PRM_FILE
//        CHOWNMOD
    }
    else if (S_ISSOCK(stat->st_mode))
    {
        CHECK_OWN(S)
        if (!source) FGACFS_FAILCALL(mknod(hostpath, hostmode, stat->st_dev))
        ADD_FILE
        ADD_ORD_PRM_FILE
//        CHOWNMOD
    }
    else if (S_ISDIR(stat->st_mode))
    {
        CHECK_OWN(D)
        if (!source) FGACFS_FAILCALL(mkdir(hostpath, hostmode));
        ADD_DIR
        ADD_ORD_PRM_DIRS
//        CHOWNMOD
    }
    else if (S_ISLNK(stat->st_mode))
    {
        CHECK_OWN(L)
        if (!source) FGACFS_FAILCALL(symlink(target, hostpath))
        ADD_FILE
        ADD_ORD_PRM_SL
//        CHOWNMOD
    }
    else return -EACCES;

    return 0;
}

int fgacfs_mknod (const char *rpath, mode_t mode, dev_t dev)
{
    struct stat stat;

    stat.st_mode = mode;
    stat.st_dev  = dev;

    return fgacfs_add(rpath, &stat, 0, NULL);
}

int fgacfs_mkdir (const char *rpath, mode_t mode)
{
    struct stat stat;

    stat.st_mode = mode | S_IFDIR;

    return fgacfs_add(rpath, &stat, 0, NULL);
}

int fgacfs_unlink (const char *rpath)
{
    FGACFS_INIT
    FGACFS_EXISTS
    FGACFS_PRM(FRM)
    FGACFS_HOSTPATH

    FGACFS_FAILCALL(unlink(hostpath));
    fgac_delete(state, path);

    return 0;
}

int fgacfs_rmdir (const char *rpath)
{
    FGACFS_INIT
    FGACFS_EXISTS
    FGACFS_PRM(DRM)
    FGACFS_HOSTPATH

    FGACFS_FAILCALL(rmdir(hostpath));
    fgac_delete(state, path);

    return 0;
}

int fgacfs_symlink (const char *target, const char *rpath)
{
    struct stat stat;

    stat.st_mode = S_IFLNK;
    return fgacfs_add(rpath, &stat, 0, target);
}

struct stat * fgac_prestat (fgac_state *state, fgac_path *path);

int check_rename (fgac_state *state, fgac_path *path, fgac_prc *prc)
{
#ifndef NDEBUG        
    printf ("'%s'\n", path->path);
#endif        
    if (fgac_is_dir(state, path))
    {
        
        DIR *d;
        struct dirent *de;
        
        if (!fgac_check_prm (state, path, prc, FGAC_PRM_DRD) ||
            !fgac_check_prm (state, path, prc, FGAC_PRM_DRA) ||
            !fgac_check_prm (state, path, prc, FGAC_PRM_DEX) ||
            !fgac_check_prm (state, path, prc, FGAC_PRM_DXA) || 
            !fgac_check_prm (state, path, prc, FGAC_PRM_DRM) 
           ) return 0;
           
        d = opendir(path->hostpath);
        if (!d) return 0;
        
        while ((de = readdir(d)))
        {
           if (strcmp (de->d_name, ".") && strcmp (de->d_name, ".."))
           {
               char buffer[FGAC_LIMIT_PATH];
               if (!fgac_str_cat3 (buffer, fgac_get_path(path), "/", de->d_name, FGAC_LIMIT_PATH)) return 0;
               fgac_path child = fgac_path_init(buffer);
               if (!check_rename (state, &child, prc)) return 0;
           }       
        }
        return 1;        
    }
    else
    {
        struct stat *s = fgac_prestat (state, path);
        return (S_ISLNK (s->st_mode) ? fgac_check_prm (state, path, prc, FGAC_PRM_FSL) : fgac_check_prm (state, path, prc, FGAC_PRM_FRD)) &&
               fgac_check_prm (state, path, prc, FGAC_PRM_FRA) &&
               fgac_check_prm (state, path, prc, FGAC_PRM_FXA) &&
               fgac_check_prm (state, path, prc, FGAC_PRM_FRM);
               
    }
}

typedef struct mv_record
{
    char rpath[FGAC_LIMIT_PATH];
    int rd;
    int fex;
    struct stat st;
    uint64_t inh;
    uid_t uid;
    gid_t gid;
    int ex;
    fgac_prms prms;
} mv_record;

typedef struct mv_array
{
    mv_record *data;
    size_t size;
} mv_array;

int load_mv_tree (fgac_state *state, const char *rpath, fgac_prc *prc, mv_array *array)
{
    mv_record *tmp;
    fgac_path path;
    
#ifndef NDEBUG
    printf ("load mv tree: '%s'\n", rpath);
#endif        
    
    ++array->size;
    tmp = realloc(array->data, array->size * sizeof (mv_record));
    if (!tmp) return -ENOMEM;
#ifndef NDEBUG
    printf ("realloc %p -> %p\n", array->data, tmp);    
#endif    
    array->data = tmp;


    
    tmp = &array->data[array->size - 1];
    path = fgac_path_init (tmp->rpath);
    
    if (!fgac_str_cpy (tmp->rpath, rpath, FGAC_LIMIT_PATH)) return -ENOMEM;
    tmp->fex = !fgac_is_dir (state, &path) && fgac_check_prm (state, &path, prc, FGAC_PRM_FEX);
    tmp->rd = fgac_is_dir (state, &path) ? fgac_check_prm (state, &path, prc, FGAC_PRM_DRP) : fgac_check_prm (state, &path, prc, FGAC_PRM_FRP);
    tmp->st = *fgac_prestat (state, &path);
    if (fgac_get_inh (state, &path, &tmp->inh)) return -EACCES;
    if (fgac_get_owner (state, &path, &tmp->uid)) return -EACCES;
    if (fgac_get_group (state, &path, &tmp->gid)) return -EACCES;
    tmp->ex = fgac_is_dir (state, &path) ? 0 : fgac_check_prm (state, &path, prc, FGAC_PRM_FEX);
    if (tmp->rd) tmp->prms = array->size == 1 ? fgac_get_allprms (state, &path) : fgac_get_prms (state, &path);
    if (fgac_prms_is_error (&tmp->prms)) tmp->rd = 0;

#ifndef NDEBUG
    printf ("1\n");
#endif        

    
    if (fgac_is_dir (state, &path))
    {
        DIR *d = opendir (path.hostpath);
        struct dirent *de;
        
        if (!d) return -EACCES;
        
#ifndef NDEBUG
    printf ("2\n");
#endif        
        
//        size_t pos = array->size - 1;
        while ((de = readdir(d)))
        {
           if (strcmp (de->d_name, ".") && strcmp (de->d_name, ".."))
           {
               int rc;
               char child[FGAC_LIMIT_PATH];
#ifndef NDEBUG
    printf ("using: '%p'\n", array->data);
#endif        
               if (!fgac_str_cat3 (child, rpath, "/", de->d_name, FGAC_LIMIT_PATH)) {closedir (d); return 0;}
               
#ifndef NDEBUG
    printf ("child: '%s'\n", child);
#endif        
               
               
               rc = load_mv_tree (state, child, prc, array);
               if (rc) {closedir(d); return rc;}
           }       
        }
        
        closedir (d);
    }
    
    
    return 0;   
}

void free_mv_tree (mv_array *array)
{
    size_t i;
    for (i = 0; i < array->size; ++i)
        fgac_prms_free (&array->data[i].prms);
    free (array->data);
}

int fgacfs_rename_tree (fgac_state *state, fgac_path *path, fgac_prc *prc, mv_array *a, const char *source, const char *target)
{
    size_t i, l = strlen (source);
    
    for (i = 0; i < a->size; ++i)
    {
        int ch = 0, ch_inh = 0, ch_trm = 0, ch_prm = 0, ch_fex = 0;
        uid_t u1 = 0, u2 = 0;
        gid_t g1 = 0, g2 = 0; 
        char buffer[FGAC_LIMIT_PATH];
        if (!fgac_str_cat2 (buffer, target, a->data[i].rpath + l, FGAC_LIMIT_PATH)) return -ENOMEM;
        fgac_path dpath = fgac_path_init (buffer);
        
        fgac_path spath = fgac_path_init(a->data[i].rpath);    
            
        fgac_delete (state, &spath);
        
        fgac_set_inh (state, &dpath, 0);
        size_t j;
        for (j = 0; j < fgac_prms_size(&a->data[i].prms); ++j)
        {
              fgac_prm prm = * fgac_prms_get (&a->data[i].prms, j);
              prm.allow = prm.deny = 0;
              fgac_set_prm (state, &dpath, &prm);
        }
        
        fgacfs_add (fgac_get_path(&dpath), &a->data[i].st, 1, NULL);
        

        do
        {      
          ch = 0;    
          if (ch_inh != 2 &&  
              (fgac_is_dir (state, &dpath) ? fgac_check_prm (state, &dpath, prc, FGAC_PRM_DCI) : fgac_check_prm (state, &dpath, prc, FGAC_PRM_FCI))
             ) ch_inh = 1;
          
          if (ch_trm != 2 &&  
               fgac_is_dir (state, &dpath) && fgac_check_prm (state, &dpath, prc, FGAC_PRM_DCT)
             ) ch_trm = 1;
          
          if (ch_prm != 2 &&  
              (fgac_is_dir (state, &dpath) ? fgac_check_prm (state, &dpath, prc, FGAC_PRM_DCP) : fgac_check_prm (state, &dpath, prc, FGAC_PRM_FCP))
             ) ch_prm = 1;
          
          if (!fgac_is_dir (state, &dpath) && fgac_check_prm (state, &dpath, prc, FGAC_PRM_FSX)
             ) ch_fex = 1;

          
          u1 = u2 = g1 = g2 = 0;
          fgac_get_owner (state, &dpath, &u1);
          fgac_get_group (state, &dpath, &g1);
          fgacfs_chown (buffer, a->data[i].uid, a->data[i].gid);
          fgac_get_owner (state, &dpath, &u2);
          fgac_get_group (state, &dpath, &g2);
          ch = (u1 != u2) || (g1 != g2);
          
          if (ch_inh != 2 &&  
              (fgac_is_dir (state, &dpath) ? fgac_check_prm (state, &dpath, prc, FGAC_PRM_DCI) : fgac_check_prm (state, &dpath, prc, FGAC_PRM_FCI))
             ) ch_inh = 1;
          
          if (ch_trm != 2 &&  
               fgac_is_dir (state, &dpath) && fgac_check_prm (state, &dpath, prc, FGAC_PRM_DCT)
             ) ch_trm = 1;
          
          if (ch_prm != 2 &&  
              (fgac_is_dir (state, &dpath) ? fgac_check_prm (state, &dpath, prc, FGAC_PRM_DCP) : fgac_check_prm (state, &dpath, prc, FGAC_PRM_FCP))
             ) ch_prm = 1;
          
          if (!fgac_is_dir (state, &dpath) && fgac_check_prm (state, &dpath, prc, FGAC_PRM_FSX)
             ) ch_fex = 1;

          if (ch_inh == 1) { fgac_set_inh (state, &dpath, a->data[i].inh & FGAC_INH_INHS); ch_inh = 2; ch = 1; }
          if (ch_trm == 1) { fgac_set_inh (state, &dpath, a->data[i].inh & FGAC_INH_TRMS); ch_trm = 2; ch = 1; }
          
          if (ch_prm == 1)
          {
              size_t j;
              for (j = 0; j < fgac_prms_size(&a->data[i].prms); ++j)
              {
                  const fgac_prm * prm = fgac_prms_get (&a->data[i].prms, j);
                  fgac_set_prm (state, &dpath, prm);
              }
              ch_prm = 2;
              ch = 1;              
          }
        
        } while (ch);
        
        if (ch_fex == 1 && ch_prm != 2)
        {
            int fex = fgac_check_prm (state, &dpath, prc, FGAC_PRM_FEX);
            if (fex != a->data[i].fex)
            {
                fgac_prm prm;
                prm.cat = FGAC_CAT_UID;
                prm.prc.uid = prc->uid;
                
                if (!fgac_get_prm (state, &dpath, &prm))
                {
                    prm.allow |= FGAC_PRM_FEX;
                    fgac_set_prm (state, &dpath, &prm);
                } 
            }
        }
        
        
    }
    
    return 0;
    
}

int fgacfs_rename (const char *rpath, const char *newrpath)
{
//    struct stat *stat;
    fgac_path fgacnewpath = fgac_path_init((char *) newrpath), *newpath = &fgacnewpath;
    char newpbuffer[FGAC_LIMIT_PATH], 
         dirn[FGAC_LIMIT_PATH], newdirn[FGAC_LIMIT_PATH];
    fgac_path fgacnewparent = fgac_path_init(newpbuffer), *newparent = &fgacnewparent;
    char *newhostpath;
    FGACFS_INIT_PARENT
    FGACFS_HOSTPATH
    FGACFS_EXISTS
    if (!fgac_parent(path, newparent)) parent = NULL;
    if (!fgac_is_dir(state, newparent)) return -ENOTDIR;
    newhostpath = fgac_get_hostpath(state, newpath);
    
    if (rpath[strlen(rpath) - 1] == '/' || newrpath[strlen(newrpath) - 1] == '/') return FGAC_ERR_PATH;

    if (!fgac_str_cpy (dirn, rpath, FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
    if (!fgac_str_cpy (newdirn, newrpath, FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
    
    if (!strcmp(dirname(dirn), dirname(newdirn)) && fgac_check_prm2(state, path, prc, FGAC_PRM_FMV, FGAC_PRM_DMV))
/*    if (!strcmp(fgac_get_hostpath(state,parent), fgac_get_hostpath(state,newparent))) */
    {

        FGACFS_PRM2(MV);
        FGACFS_FAILCALL(rename(hostpath, newhostpath))
        if (fgac_rename (state, path, newrpath))
        {
            rename(newhostpath, hostpath);
            return -EACCES;
        }

        return 0;
    }
    
//    FGACFS_PRM2(RM);
    if (!check_rename (state, path, prc)) return -EACCES;
    
#ifndef NDEBUG
    printf ("rename allowed\n");    
#endif    

//    stat = fgac_stat(state, path, prc);

    mv_array a;
    
    a.size = 0;
    a.data = NULL;
    rc = load_mv_tree (state, fgac_get_path(path), prc, &a);
    if (rc) {free_mv_tree(&a); return rc; }

#ifndef NDEBUG
    printf ("tree loaded, p=%p\n", a.data);    
#endif    

    FGACFS_FAILCALL (rename(hostpath, newhostpath))
    
    rc = fgacfs_rename_tree (state, path, prc, &a, rpath, newrpath);
    free_mv_tree(&a);
    return rc;
}

#define CHMOD_CATCH(t,T)     \
    if (d##t##T)             \
    {                        \
        if (s##t##T)         \
        {                    \
            prm.allow |=  T; \
            prm.deny  &= ~T; \
        }                    \
        else                 \
        {                    \
            prm.allow &= ~T; \
            prm.deny  |=  T; \
        }                    \
    }

int fgacfs_chmod (const char *rpath, mode_t mode)
{
    struct stat *stat;
    fgac_prc prc2;
    fgac_prm prm;
    uint64_t r, w, x, u, g;
    int isdir;
    unsigned sur, suw, sux, sgr, sgw, sgx, sor, sow, sox, ssu, ssg,
             dur, duw, dux, dgr, dgw, dgx, dor, dow, dox, dsu, dsg;
    FGACFS_INIT
    FGACFS_EXISTS

    prc2 = *prc;
    prc2.cmd = NULL;

    stat = fgac_stat(state, path, &prc2);
    if (!stat) return -EACCES;

    isdir = fgac_is_dir(state, path);

    r = isdir ? FGAC_PRM_DREA : FGAC_PRM_FREA;
    w = isdir ? FGAC_PRM_DWRI : FGAC_PRM_FWRI;
    x = isdir ? FGAC_PRM_DEX  : FGAC_PRM_FEX;
    u = isdir ? 0             : FGAC_PRM_FSU;
    g = isdir ? 0             : FGAC_PRM_FSG;

    sur = mode & S_IRUSR;
    suw = mode & S_IWUSR;
    sux = mode & S_IXUSR;

    sgr = mode & S_IRGRP;
    sgw = mode & S_IWGRP;
    sgx = mode & S_IXGRP;

    sor = mode & S_IROTH;
    sow = mode & S_IWOTH;
    sox = mode & S_IXOTH;
    
    ssu = mode & S_ISUID;
    ssg = mode & S_ISGID;


    dur = (stat->st_mode & S_IRUSR) != sur;
    duw = (stat->st_mode & S_IWUSR) != suw;
    dux = (stat->st_mode & S_IXUSR) != sux;

    dgr = (stat->st_mode & S_IRGRP) != sgr;
    dgw = (stat->st_mode & S_IWGRP) != sgw;
    dgx = (stat->st_mode & S_IXGRP) != sgx;

    dor = (stat->st_mode & S_IROTH) != sor;
    dow = (stat->st_mode & S_IWOTH) != sow;
    dox = (stat->st_mode & S_IXOTH) != sox;

    dsu = (stat->st_mode & S_ISUID) != ssu;
    dsg = (stat->st_mode & S_ISGID) != ssg;

    if ((isdir && fgac_check_prm(state, path, prc, FGAC_PRM_DCP)) || (!isdir && fgac_check_prm(state, path, prc, FGAC_PRM_FCP)))
    {
        if (dur || duw || dux)
        {
            prm.cat = FGAC_CAT_OUS;
            if (fgac_get_prm(state, path, &prm)) return -EACCES;
            CHMOD_CATCH(u,r);
            CHMOD_CATCH(u,w);
            CHMOD_CATCH(u,x);
            if (fgac_set_prm(state, path, &prm)) return -EACCES;

            prm.cat = FGAC_CAT_UID;
            prm.prc.uid = stat->st_uid;
            if (fgac_get_prm(state, path, &prm)) return -EACCES;
            CHMOD_CATCH(u,r);
            CHMOD_CATCH(u,w);
            CHMOD_CATCH(u,x);
            if (fgac_set_prm(state, path, &prm)) return -EACCES;
        }

        if (dgr || dgw || dgx)
        {
            prm.cat = FGAC_CAT_OGR;
            if (fgac_get_prm(state, path, &prm)) return -EACCES;
            CHMOD_CATCH(g,r);
            CHMOD_CATCH(g,w);
            CHMOD_CATCH(g,x);
            if (fgac_set_prm(state, path, &prm)) return -EACCES;

            prm.cat = FGAC_CAT_GID;
            prm.prc.gid = stat->st_gid;
            if (fgac_get_prm(state, path, &prm)) return -EACCES;
            CHMOD_CATCH(g,r);
            CHMOD_CATCH(g,w);
            CHMOD_CATCH(g,x);
            if (fgac_set_prm(state, path, &prm)) return -EACCES;
        }

        if (dor || dow || dox)
        {
            prm.cat = FGAC_CAT_OTH;
            if (fgac_get_prm(state, path, &prm)) return -EACCES;
            CHMOD_CATCH(o,r);
            CHMOD_CATCH(o,w);
            CHMOD_CATCH(o,x);
            if (fgac_set_prm(state, path, &prm)) return -EACCES;
        }
        
        if (dsu || dsg)
        {
            prm.cat = FGAC_CAT_UID;
            prm.prc.uid = prc->uid;
            if (fgac_get_prm(state, path, &prm)) return -EACCES;
            CHMOD_CATCH(s,u);
            CHMOD_CATCH(s,g);
            if (fgac_set_prm(state, path, &prm)) return -EACCES;        
        }
        
    }
    else if (!isdir && fgac_check_prm(state, path, prc, FGAC_PRM_FSX))
    {

        if (dur || duw || dgr || dgw || dor || dow) return -EACCES;

        if (dux)
        {
            prm.cat = FGAC_CAT_OUS;
            if (fgac_get_prm(state, path, &prm)) return -EACCES;
            CHMOD_CATCH(u,x);
            if (fgac_set_prm(state, path, &prm)) return -EACCES;

            prm.cat = FGAC_CAT_UID;
            prm.prc.uid = stat->st_uid;
            if (fgac_get_prm(state, path, &prm)) return -EACCES;
            CHMOD_CATCH(u,x);
            if (fgac_set_prm(state, path, &prm)) return -EACCES;
        }
        if (dgx)
        {
            prm.cat = FGAC_CAT_OGR;
            if (fgac_get_prm(state, path, &prm)) return -EACCES;
            CHMOD_CATCH(g,x);
            if (fgac_set_prm(state, path, &prm)) return -EACCES;

            prm.cat = FGAC_CAT_GID;
            prm.prc.gid = stat->st_gid;
            if (fgac_get_prm(state, path, &prm)) return -EACCES;
            CHMOD_CATCH(u,x);
            if (fgac_set_prm(state, path, &prm)) return -EACCES;
        }
        if (dox)
        {
            prm.cat = FGAC_CAT_OTH;
            if (fgac_get_prm(state, path, &prm)) return -EACCES;
            CHMOD_CATCH(o,x);
            if (fgac_set_prm(state, path, &prm)) return -EACCES;
        }
    }
    else return -EACCES;
    return 0;
}

int fgacfs_chown (const char *rpath, uid_t uid, gid_t gid)
{
    uid_t ouid;
    gid_t ogid;

    FGACFS_INIT_PARENT
    FGACFS_EXISTS

    if (fgac_get_owner (state, path, &ouid)) return -EACCES;
    if (fgac_get_group (state, path, &ogid)) return -EACCES;

    if (ouid != uid)
    {
        if (!fgac_check_prm2(state, path, prc, FGAC_PRM_FCO, FGAC_PRM_DCO))
        {
            uid_t puid;
            if (!parent) return -EACCES;

            if (fgac_get_owner (state, parent, &puid)) return -EACCES;
            if (puid != uid || !fgac_check_prm2(state, path, prc, FGAC_PRM_FSP, FGAC_PRM_DSP)) return -EACCES;
        }
    }

    if (ogid != gid)
    {
        if (!fgac_check_prm2(state, path, prc, FGAC_PRM_FCG, FGAC_PRM_DCG))
        {
            gid_t pgid;
            if (!parent) return -EACCES;

            if (fgac_get_group (state, parent, &pgid)) return -EACCES;
            if (pgid != gid || !fgac_check_prm2(state, path, prc, FGAC_PRM_FSP, FGAC_PRM_DSP)) return -EACCES;
        }
    }

    if (ouid != uid) fgac_set_owner(state, path, uid);
    if (ogid != gid) fgac_set_group(state, path, gid);

    return 0;
}

int fgacfs_truncate (const char *rpath, off_t newsize)
{
    FGACFS_INIT
    FGACFS_EXISTS
/*    FGACFS_PRM(FWR) */
    FGACFS_HOSTPATH
    
#ifndef NDEBUG
    printf ("TRUNCATE %s to %li\n", rpath, (long int) newsize);
#endif    

    struct stat st;
    if (lstat (hostpath, &st)) return -errno;
    
    if ((st.st_size > newsize && !fgac_check_prm(state, path, prc, FGAC_PRM_FTR)) ||
        (st.st_size < newsize && !fgac_check_prm(state, path, prc, FGAC_PRM_FAP))
       ) return -EACCES;
    
    FGACFS_RETCALL(truncate(hostpath, newsize));
}

int fgacfs_utime (const char *rpath, struct utimbuf *ubuf)
{
    FGACFS_INIT
    FGACFS_EXISTS
    FGACFS_PRM2(CA)
    FGACFS_HOSTPATH

    FGACFS_RETCALL(utime(hostpath, ubuf))
}

typedef struct fgacfs_fh
{
    int fh;
    int rw, ap, tr;  /* NO rewrite, NO append, NO truncate */
} fgacfs_fh;

#define FH (((fgacfs_fh *)(long)(fi->fh))->fh)
#define RW (((fgacfs_fh *)(long)(fi->fh))->rw)
#define AP (((fgacfs_fh *)(long)(fi->fh))->ap)
#define TR (((fgacfs_fh *)(long)(fi->fh))->tr)

int fgacfs_open (const char *rpath, struct fuse_file_info *fi)
{
    FGACFS_INIT
    FGACFS_EXISTS
    FGACFS_HOSTPATH
    
#ifndef NDEBUG
    printf ("OPEN %s WR=%i RW=%i AP=%i TR=%i\n", rpath, (fi->flags & O_WRONLY) == O_WRONLY,
                                                        (fi->flags & O_RDWR) == O_RDWR,
                                                        (fi->flags & O_APPEND) == O_APPEND,
                                                        (fi->flags & O_TRUNC) == O_TRUNC     );
#endif    
    fi->fh = (uint64_t) (long) malloc (sizeof(fgacfs_fh));
    if (!fi->fh) return -ENOMEM;
    if (fi->flags & O_RDONLY || fi->flags & O_RDWR || !(fi->flags & O_WRONLY)) FGACFS_PRM(FRD);
    if (fi->flags & O_WRONLY || fi->flags & O_RDWR                           ) 
    {
        RW = !fgac_check_prm(state, path, prc, FGAC_PRM_FRW);
        AP = !fgac_check_prm(state, path, prc, FGAC_PRM_FAP);
        TR = !fgac_check_prm(state, path, prc, FGAC_PRM_FTR);
        
        if (RW && AP) return -EACCES;

        if (fi->flags & O_TRUNC)
        {
            struct stat st;
            if (!lstat (hostpath, &st) && st.st_size && TR) return -EACCES;            
        }
    }


    FGACFS_FAILCALL(open(hostpath, fi->flags))
    
    FH = rc;
    
    fi->direct_io = state->dio;
    return 0;
}

int fgacfs_read (const char *rpath, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    (void) rpath;
    return pread(FH, buf, size, offset);
}

off_t fsize (int fd)
{
    struct stat st;
    if (fstat(fd, &st)) return -errno;
    return st.st_size;
}

int fgacfs_write (const char *rpath, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    (void) rpath;

#ifndef NDEBUG
    printf ("WRITE to %s of size=%li offset=%li DIO=%i\n", rpath, (long int) fsize(FH), (long int) offset, fi->direct_io);
#endif

    off_t fs = fsize(FH);
    if (fs < 0) return -errno;
    
    if ((RW && offset < fs) ||
        (AP && offset + size >= fs)
       ) return -EACCES;

    return pwrite(FH, buf, size, offset);
}

int fgacfs_statfs (const char *rpath, struct statvfs *statv)
{
    FGACFS_INIT
    FGACFS_HOSTPATH
    (void) prc;
    FGACFS_RETCALL (statvfs(hostpath, statv));
}

int fgacfs_flush (const char *rpath, struct fuse_file_info *fi)
{
    (void) rpath;
    (void) fi;
    return 0;
}

int fgacfs_release (const char *rpath, struct fuse_file_info *fi)
{
    (void) rpath;
    int rc = close(FH); 
    if (rc < 0) return -errno;
    free ((void*)(long) fi->fh);
    return 0;
}

int fgacfs_fsync (const char *rpath, int datasync, struct fuse_file_info *fi)
{
    (void) rpath;
    (void) datasync;
#ifdef HAVE_FDATASYNC
    if (datasync)
        FGACFS_RETCALL(fdatasync(FH))
    else
#endif
	FGACFS_RETCALL(fsync(FH))

}

int int_to_hex (uint64_t i, char* hex);
int hex_to_int (const char* hex, uint64_t *i);


int fgacfs_setxattr (const char *rpath, const char *name, const char *value, size_t size, int flags)
{
    size_t len = strlen(name);
    FGACFS_INIT
    FGACFS_EXISTS

//    if (len >= 12 && !memcmp(name, "user.fgacfs.", 12)) return -EACCES;

    if (len >= 19 && !memcmp(name, "user.fgacfs.prm.", 16))
    {
        const char *id;
        FGACFS_PRM2(CP)
        if (size != 16) return -EACCES;
        fgac_prm prm;
        prm.allow = *(uint64_t *) value;
        prm.deny  = *(uint64_t *) (value + 8);
        name += 16;
        len -= 16;
        if (!memcmp(name, "all", 3))
            prm.cat = FGAC_CAT_ALL;
        else if (!memcmp(name, "ous", 3))
            prm.cat = FGAC_CAT_OUS;
        else if (!memcmp(name, "ogr", 3))
            prm.cat = FGAC_CAT_OGR;
        else if (!memcmp(name, "oth", 3))
            prm.cat = FGAC_CAT_OTH;
        else if (len >= 12 && !memcmp(name, "uid.", 4))
        {
            uint64_t uid;
            id = name + 4;
            if (!hex_to_int(id, &uid)) return -EACCES;
            prm.cat = FGAC_CAT_UID;
            prm.prc.uid = uid;
        }
        else if (len >= 12 && !memcmp(name, "gid.", 4))
        {
            uint64_t gid;
            id = name + 4;
            if (!hex_to_int(id, &gid)) return -EACCES;
            prm.prc.gid = gid;
            prm.cat = FGAC_CAT_GID;
        }
        else if (len >= 5 && !memcmp(name, "pex.", 4))
        {
            prm.cat = FGAC_CAT_PEX;
            prm.prc.cmd = (char *) name + 4;
        }
        else return -EACCES;

        if (fgac_set_prm (state, path, &prm)) return -EACCES;  
        return 0;
    }
    
    if (len == 15 && !memcmp(name, "user.fgacfs.inh", 15))
    {
        uint64_t inh;
        FGACFS_PRM2(CI)
        if (size != 8) return -EACCES;
        inh = *(uint64_t *) value;
        if (inh & FGAC_INH_TRMS) FGACFS_PRM(DCT);
        
        if (fgac_set_inh (state, path, inh)) return -EACCES;
#ifndef NDEBUG
        printf ("1: %s %s\n", name, value);
#endif        
        return 0;
    }

    FGACFS_PRM2(MX);
    FGACFS_HOSTPATH

    rc = lsetxattr(hostpath, name, value, size, flags);
    
    if (rc < 0) return -errno; else return 0;
}

int fgacfs_getxattr (const char *rpath, const char *name, char *value, size_t size)
{
    size_t len = strlen(name);
    FGACFS_INIT
    FGACFS_EXISTS

//    if (!memcmp(name, "user.fgacfs.", 12)) return -EACCES;

    if (len >= 19 && !memcmp(name, "user.fgacfs.prm.", 16))
    {
        const char *id;
        FGACFS_PRM2(RP)
        if (!size) return 16;
        if (size < 16) return -ERANGE;
        fgac_prm prm;
        name += 16;
        len -= 16;
        if (!memcmp(name, "all", 3))
            prm.cat = FGAC_CAT_ALL;
        else if (!memcmp(name, "ous", 3))
            prm.cat = FGAC_CAT_OUS;
        else if (!memcmp(name, "ogr", 3))
            prm.cat = FGAC_CAT_OGR;
        else if (!memcmp(name, "oth", 3))
            prm.cat = FGAC_CAT_OTH;
        else if (len >= 12 && !memcmp(name, "uid.", 4))
        {
            uint64_t uid;
            id = name + 4;
            if (!hex_to_int(id, &uid)) return -EACCES;
            prm.cat = FGAC_CAT_UID;
            prm.prc.uid = uid;
        }
        else if (len >= 12 && !memcmp(name, "gid.", 4))
        {
            uint64_t gid;
            id = name + 4;
            if (!hex_to_int(id, &gid)) return -EACCES;
            prm.prc.gid = gid;
            prm.cat = FGAC_CAT_GID;
        }
        else if (len >= 5 && !memcmp(name, "pex.", 4))
        {
            prm.cat = FGAC_CAT_PEX;
            prm.prc.cmd = (char *) name + 4;
        }
        else return -EACCES;

        if (fgac_get_prm (state, path, &prm)) return EACCES;
        
        memcpy (value,   &prm.allow, 8);
        memcpy (value+8, &prm.deny,  8);  
                
        return 16;
    }
    
    if (len == 15 && !memcmp(name, "user.fgacfs.inh", 15))
    {
        uint64_t inh;
        FGACFS_PRM2(RP)
        if (!size) return 8;
        if (size < 8) return -ERANGE;
        
        if (fgac_get_inh (state, path, &inh)) return EACCES;
        
        memcpy (value, &inh, 8);
        
        return 8;
            
    }

    FGACFS_PRM2(XA);
    FGACFS_HOSTPATH

    rc = lgetxattr(hostpath, name, value, size);
    if (rc < 0) return -errno; else return rc;
}

#define ADD_XATTR(asize,value)                   \
         r += asize;                             \
         if (size)                               \
         {                                       \
             if (r > size) return -ERANGE;       \
             else memcpy (list, value, asize);   \
         }                                       \
         list += asize;

int fgacfs_listxattr (const char *rpath, char *list, size_t size)
{
    ssize_t s = 0, r = 0;
    char *list2, *i, *e;
    size_t length;
    
    FGACFS_INIT
    FGACFS_EXISTS

    FGACFS_HOSTPATH


    if (fgac_check_prm2(state, path, prc, FGAC_PRM_FRP, FGAC_PRM_DRP)) 
    {
         fgac_prms prms = fgac_get_prms (state, path);
         size_t n = fgac_prms_size(&prms), i;
         if (fgac_prms_is_error(&prms)) return -ENOMEM;
         
         ADD_XATTR(16, "user.fgacfs.inh")
                  
         for (i = 0; i < n; ++i)
         {
             fgac_prm prm = *fgac_prms_get(&prms, i);
             switch (prm.cat)
             {
                 case FGAC_CAT_ALL: 
                     ADD_XATTR(20, "user.fgacfs.prm.all") break;
                 case FGAC_CAT_OUS: 
                     ADD_XATTR(20, "user.fgacfs.prm.ous") break;
                 case FGAC_CAT_OGR: 
                     ADD_XATTR(20, "user.fgacfs.prm.ogr") break;
                 case FGAC_CAT_OTH: 
                     ADD_XATTR(20, "user.fgacfs.prm.oth") break;
                 case FGAC_CAT_UID:
                 {
                     char name[37];
                     char id[17];
                     int_to_hex(prm.prc.uid, id);
                     fgac_str_cat2 (name, "user.fgacfs.prm.uid.", id, 37);
                     ADD_XATTR(37, name);
                     break;
                 }
                 case FGAC_CAT_GID:
                 {
                     char name[37];
                     char id[17];
                     int_to_hex(prm.prc.gid, id);
                     fgac_str_cat2 (name, "user.fgacfs.prm.gid.", id, 37);
                     ADD_XATTR(37, name);
                     break;
                 }
                 case FGAC_CAT_PEX:                 
                 {
                      size_t size = 20 + strlen(prm.prc.cmd);
                      char *name = malloc(size);
                      if (!name) return -ENOMEM;
                      fgac_str_cat2 (name, "user.fgacfs.prm.pex.", prm.prc.cmd, size);
                      ADD_XATTR(size, name);
                 }          
             }
        }     
    }

    if (!fgac_check_prm2(state, path, prc, FGAC_PRM_FXA, FGAC_PRM_DXA)) return r;

    if ((s = llistxattr(hostpath, list, 0)) < 0) return -errno;
    list2 = malloc(s);
    s = llistxattr(hostpath, list2, s);    
    if (s < 0) {free(list2); return -errno; }

    for (i = list2, e = list2 + s; i < e; i += length)
    {
        length = strlen (i) + 1;
        if (length < 12 || memcmp(i, "user.fgacfs.", 12))
        {
            ADD_XATTR(length, i);
        }
    }
    free(list2); 
    return r;
}

int fgacfs_removexattr (const char *rpath, const char *name)
{
    FGACFS_INIT
    FGACFS_EXISTS

    if (strlen(name) >= 12 && !memcmp(name, "user.fgacfs.", 12)) return -EACCES;

    FGACFS_PRM2(MX);
    FGACFS_HOSTPATH

    FGACFS_RETCALL(lremovexattr(hostpath, name))
}

int fgacfs_opendir (const char *rpath, struct fuse_file_info *fi)
{
    FGACFS_INIT
    FGACFS_EXISTS
    FGACFS_PRM(DRD)
    FGACFS_HOSTPATH
    rc = (intptr_t) opendir(hostpath);
    if (!rc) return -errno;
    fi->fh = rc;
    return 0;
}

int fgacfs_readdir (const char *rpath, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    (void)rpath;
    (void)offset;

    DIR *dp = (DIR *) (uintptr_t) fi->fh;
    struct dirent *de;

    errno = 0;
    while ((de = readdir(dp)))
        if (filler(buf, de->d_name, NULL, 0)) return -ENOMEM;

    return -errno;
}

int fgacfs_releasedir (const char *rpath, struct fuse_file_info *fi)
{
    (void)rpath;
    closedir((DIR *) (uintptr_t) fi->fh);
    return 0;
}

/*
int fgacfs_access (const char *rpath, int mode)
{
    int r = 1;

    FGACFS_INIT
    FGACFS_HOSTPATH

    if (mode & F_OK) r = access(hostpath, );

    if (mode != F_OK)
    {
        struct stat statbuf;

        int R, W, X;

        if (fgac_stat(state, path, prc, &statbuf) < 0) return -errno;

        if      (statbuf.st_uid == prc->uid)
        {
            R = S_IRUSR;
            W = S_IWUSR;
            X = S_IXUSR;
        }
        else if (statbuf.st_gid == prc->gid)
        {
            R = S_IRGRP;
            W = S_IWGRP;
            X = S_IXGRP;
        }
        else
        {
            R = S_IROTH;
            W = S_IWOTH;
            X = S_IXOTH;
        }

        if (mode & R_OK) r = r && statbuf.st_mode & R;
        if (mode & W_OK) r = r && statbuf.st_mode & W;
        if (mode & X_OK) r = r && statbuf.st_mode & X;

    }

    return r ? 0 : -1;
}
*/


int fgacfs_ftruncate (const char *rpath, off_t offset, struct fuse_file_info *fi)
{
    (void) rpath;
    
#ifndef NDEBUG
    printf ("FTRUNCATE %s to %li\n", rpath, (long int) offset);
#endif    
    
    if (TR || AP)
    {
        off_t fs = fsize(FH);
        if (fs < 0) return -errno;
        if ((TR && fs > offset) ||
            (AP && fs < offset)
           ) return -EACCES;
    }
    
    FGACFS_RETCALL(ftruncate(FH, offset));
}

int fgacfs_fgetattr (const char *rpath, struct stat *statbuf, struct fuse_file_info *fi)
{
    (void) fi;
    return fgacfs_getattr(rpath, statbuf);
}
