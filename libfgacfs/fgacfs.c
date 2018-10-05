/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#include <config.h>
#include "fgacfs.h"
#include "xattr.h"
#include "fxattr.h"
#include "db.h"
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cache.h>
#include <stdio.h>
#include <sys/xattr.h>

const char fgac_version[] = "1.0.0";
const char fgac_name[] = "libfgacfs";
char fgac_prg_name[FGAC_LIMIT_PATH];
char fgac_prg_version[FGAC_LIMIT_PATH];

int exists (const char *name)
{
    struct stat statbuf;
    return lstat (name, &statbuf) == 0;
}

void fgac_init (const char *name, const char *version)
{
    fgac_str_cpy(fgac_prg_name,    name,    FGAC_LIMIT_PATH);
    fgac_str_cpy(fgac_prg_version, version, FGAC_LIMIT_PATH);
}

int fgac_open   (const char *hostdir, int check_prexec, int dio, fgac_state **state, size_t cache_size, int stat_cache)
{
    struct stat statbuf;
    
#ifndef NDEBUG
    printf ("Opening: hostdir=%s cmd=%i, csize=%u\n", hostdir, check_prexec, (unsigned) cache_size);
#endif    

    if (!hostdir) return FGAC_ERR_HOSTERROR;

    if (!(*state = malloc(sizeof(fgac_state))))         return FGAC_ERR_MALLOC;

    if (!fgac_str_cpy ((*state)->hostdir, hostdir, FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
    if (!fgac_str_cat2 ((*state)->datadir, (*state)->hostdir, "/data", FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;

    if (lstat ((*state)->datadir, &statbuf)) return FGAC_ERR_NODATA;

    if (lstat ((*state)->hostdir, &statbuf)) return FGAC_ERR_HOSTERROR;

    (*state)->uid = statbuf.st_uid;
    (*state)->gid = statbuf.st_gid;

    if (geteuid() != statbuf.st_uid || getegid() != statbuf.st_gid)
    {
        if (geteuid() != 0 || getegid() != 0) return FGAC_ERR_BADUID;
        if (setegid(statbuf.st_gid))          return FGAC_ERR_BADUID;
        if (seteuid(statbuf.st_uid))          return FGAC_ERR_BADUID;
    }

    if (db_open(*state)) { return FGAC_ERR_DBOPEN; }
    if (!(**state).db && fxattr_open(*state) && xattr_open(*state)) return FGAC_ERR_XATTROPEN;
    
    (**state).cache = cache_size ? cache_init (cache_size, stat_cache) : NULL;
    
    (**state).check_prexec = check_prexec;
    (**state).dio          = dio;
    
    (**state).fd_lock = -1;
    (**state).fd_fifo = -1;
    (**state).bpos    = 0;
    return FGAC_OK;

}

int fgac_mount (fgac_state *state, const char *mountpoint)
{
    char lockname[FGAC_LIMIT_PATH];
    if (!fgac_str_cat2 (lockname, state->hostdir, FGAC_LOCK_NAME, FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
    
    if (exists(lockname))
    {
        int fd;
        char emountpoint[FGAC_LIMIT_PATH];
        size_t p = 0;
        if ((fd = open (lockname, O_RDONLY)) < 0) return FGAC_ERR_LOCK;

        while (read(fd, &emountpoint[p], 1) == 1 && p < FGAC_LIMIT_PATH) ++p;
        if (p < FGAC_LIMIT_PATH)
        {
            char value[1];
#ifndef NDEBUG            
            printf("emountpoint=%s\n", emountpoint);
#endif            
            getxattr(emountpoint, "user.fgacfs.mount", value, 1);
            if (errno == EACCES) return FGAC_ERR_LOCK;
        }

        close(fd);
    }

    if ((state->fd_lock = open (lockname, O_WRONLY | O_CREAT, 0600)) < 0) return FGAC_ERR_LOCK;
    write(state->fd_lock, mountpoint, strlen(mountpoint) + 1);

    if (state->cache)
    {
        char fifoname[FGAC_LIMIT_PATH];
        if (!fgac_str_cat2 (fifoname, state->hostdir, FGAC_FIFO_NAME, FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
        
        if (exists(fifoname) && unlink (fifoname) < 0) return FGAC_ERR_LOCK;
        
        if (mkfifo (fifoname, 0666) < 0 || 
            (state->fd_fifo = open(fifoname, O_RDONLY | O_NONBLOCK | O_SYNC)) < 0
           ) return FGAC_ERR_LOCK;
    }
    
    return FGAC_OK;
}

int fgac_attach (fgac_state *state, char *mountpoint)
{
    char lockname[FGAC_LIMIT_PATH];
    char fifoname[FGAC_LIMIT_PATH];
    
    if (!fgac_str_cat2 (lockname, state->hostdir, FGAC_LOCK_NAME, FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
    if (!fgac_str_cat2 (fifoname, state->hostdir, FGAC_FIFO_NAME, FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;        

    *mountpoint = '\0';
    
    if (exists(lockname))
    {
        size_t p = 0;
        char value[1];

        state->fd_lock = open (lockname, O_RDONLY);
        if (state->fd_lock < 0) return FGAC_ERR_LOCK;
        
        while (read(state->fd_lock, &mountpoint[p], 1) == 1 && p < FGAC_LIMIT_PATH) ++p;
        if (p >= FGAC_LIMIT_PATH && mountpoint[FGAC_LIMIT_PATH-1]) return FGAC_ERR_PATH;

#ifndef NDEBUG        
        printf("attach mountpoint=%s\n", mountpoint);
#endif        

        getxattr(mountpoint, "user.fgacfs.mount", value, 1);
        if (errno == EACCES)
        {
           if (exists(fifoname) && 
               (state->fd_fifo = open(fifoname, O_WRONLY | O_NONBLOCK | O_SYNC)) < 0
              ) return FGAC_ERR_LOCK;
        }
        else
        {
            unlink(lockname);
            if (exists(fifoname)) unlink(fifoname);
        }

        close (state->fd_lock);
        state->fd_lock=-1;
    }        
    
#ifndef NDEBUG
    printf ("Attach: mointpoint=%s\n", mountpoint);
#endif    
    
    return FGAC_OK;
}

void fgac_close (fgac_state **state)
{
    switch ((*state)->type)
    {
        case FGAC_XATTR:  xattr_close(*state);  break;
        case FGAC_FXATTR: fxattr_close(*state); break;
        case FGAC_SQLITE: db_close(*state);     break;
    }

    if ((*state)->cache) cache_free((*state)->cache);
    if ((*state)->fd_lock != -1) 
    {
        char lockname[FGAC_LIMIT_PATH];
        close ((*state)->fd_lock);
        if (fgac_str_cat2 (lockname, (*state)->hostdir, FGAC_LOCK_NAME, FGAC_LIMIT_PATH))
            unlink(lockname);
        if ((*state)->fd_fifo != -1)
        {
            char fifoname[FGAC_LIMIT_PATH];
            close ((*state)->fd_fifo);
            if (fgac_str_cat2 (fifoname, (*state)->hostdir, FGAC_FIFO_NAME, FGAC_LIMIT_PATH))
                unlink(fifoname);
        }
    }
    else if ((*state)->fd_fifo != -1)
    {
        close ((*state)->fd_fifo);
    }

    free (*state);

    *state = NULL;
}

int fgac_create (const char *hostdir, fgac_type type)
{
    char datadir[FGAC_LIMIT_PATH];
    DIR* dir = opendir(hostdir);

    if (dir)
    {
        struct dirent *d;
        d = readdir(dir);
        d = readdir(dir);
        d = readdir(dir);
        closedir(dir);
        if (d) return FGAC_ERR_HOSTEMPTY;
        if (chmod(hostdir, 0700)) return FGAC_ERR_HOSTERROR;
    }
    else if (errno == ENOENT)
    {
        if (mkdir(hostdir, 0700)) return FGAC_ERR_HOSTERROR;
    }
    else
    {
        return FGAC_ERR_HOSTDIR;
    }

    if (!fgac_str_cat2 (datadir, hostdir, "/data", FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
    if (mkdir(datadir, 0700)) return FGAC_ERR_HOSTERROR;

    switch (type)
    {
        case FGAC_XATTR : return xattr_create(hostdir);
        case FGAC_FXATTR: return fxattr_create(hostdir);
        case FGAC_SQLITE: return db_create(hostdir);
        default:          return FGAC_ERR_TYPE;
    }

}

struct stat * fgac_prestat (fgac_state *state, fgac_path *path)
{
    if (path->statset == UNSET)
    {
        char *host = FGAC_HOSTPATH;
        if (!host || lstat (host, &path->statbuf) < 0) return NULL;
        path->statset = FALSE;
        return &path->statbuf;
    }
    return &path->statbuf;
}

void swappath (fgac_path **path1, fgac_path **path2)
{
    fgac_path *t = *path1;
    *path1       = *path2;
    *path2       = t;
}

#define FGAC_CPC \
    if (fgac_get_prm(state, path, &p) != FGAC_OK) return -1; \
    if (p.deny & prm) return -1;                             \
    allow |= p.allow;
int fgac_check_entry_prm (fgac_state *state, fgac_path *path, const fgac_prc *prc, uint64_t prm)
{
    uid_t uid; gid_t gid;
    fgac_prm p;
    uint64_t allow = 0;
    const gid_t *i = prc->groups, *e = i + prc->ngroups;

    p.cat = FGAC_CAT_ALL; FGAC_CPC
    if (prc->uid != (uid_t) -1) { p.cat = FGAC_CAT_UID; p.prc.uid = prc->uid; FGAC_CPC }
    if (prc->gid != (uid_t) -1) { p.cat = FGAC_CAT_GID; p.prc.gid = prc->gid; FGAC_CPC }

    p.cat = FGAC_CAT_GID;
    for ( ; i < e; ++i)
    {
        p.prc.gid = *i;
        FGAC_CPC
    }

    if (state->check_prexec && prc->cmd != NULL)
    {
        p.cat = FGAC_CAT_PEX; p.prc.cmd = prc->cmd; FGAC_CPC
    }

    if (fgac_get_group(state, path, &gid) != FGAC_OK) return -1;
    if (fgac_get_owner(state, path, &uid) != FGAC_OK) return -1;

    if (uid == prc->uid)
    {
        p.cat = FGAC_CAT_OUS; FGAC_CPC
    }
    else if (gid == prc->gid)
    {
        p.cat = FGAC_CAT_OGR; FGAC_CPC
    }
    else
    {
        p.cat = FGAC_CAT_OTH; FGAC_CPC
    }

    return (allow & prm) == prm;
}

#define FGAC_APC \
    if (fgac_get_prm(state, path, &p) != FGAC_OK) { *allow = *deny = 0; return; } \
    *deny  |= p.deny;                                                             \
    *allow |= p.allow;
void fgac_get_entry_ad (fgac_state *state, fgac_path *path, const fgac_prc *prc, uint64_t *allow, uint64_t *deny)
{
    fgac_prm p;
    const gid_t *i = prc->groups, *e = i + prc->ngroups;
    uid_t uid;
    gid_t gid;
    
    int b = 0;
/*
    if (prc->uid == 0 || prc->uid == state->uid)
    {
        *allow = fgac_is_dir(state, path) ? FGAC_PRM_ALLS : FGAC_PRM_FILE;
        *deny  = 0;
        return;
    }
*/
    *allow = *deny = 0;

    p.cat = FGAC_CAT_ALL; FGAC_APC
    if (prc->uid != (uid_t) -1) { b = 1; p.cat = FGAC_CAT_UID; p.prc.uid = prc->uid; FGAC_APC }
    if (prc->gid != (gid_t) -1) { b = 1; p.cat = FGAC_CAT_GID; p.prc.gid = prc->gid; FGAC_APC }

    for ( ; i < e; ++i)
    {
        p.prc.gid = *i;
        FGAC_APC
        b = 1;
    }

    if (state->check_prexec && prc->cmd != NULL)
    {
        p.cat = FGAC_CAT_PEX; p.prc.cmd = prc->cmd; FGAC_APC
    }

    if (!fgac_get_owner(state, path, &uid) && uid == prc->uid)
    {
        p.cat = FGAC_CAT_OUS; FGAC_APC
    }
    else if (!fgac_get_group(state, path, &gid) && gid  == prc->gid)
    {
        p.cat = FGAC_CAT_OGR; FGAC_APC
    }
    else if (b)
    {
        p.cat = FGAC_CAT_OTH; FGAC_APC
    }
}

int fgac_check_inh_prm (fgac_state *state, fgac_path *path, const fgac_prc *prc, uint64_t prm)
{
    int go_fprm, go_dprm, r, q;

    char buffer1 [FGAC_LIMIT_PATH], buffer2 [FGAC_LIMIT_PATH];
    fgac_path path1 = fgac_path_init(buffer1), path2 = fgac_path_init(buffer2),
              *curpath = &path1, *oldpath = &path2;

    go_dprm = fgac_is_dir(state, path) ? fgac_check_inh (state, path, FGAC_INH_DPI) : 0;
    go_fprm = fgac_check_inh (state, path, FGAC_INH_FPI);

    if (!fgac_str_cpy(fgac_get_path(oldpath), FGAC_PATH, FGAC_LIMIT_PATH)) return ENOMEM;

    if ((r = fgac_check_entry_prm(state, oldpath, prc, prm)) == -1) return 0;
    
    while ((go_dprm || go_fprm) && fgac_parent(oldpath, curpath))
    {
        if (!go_fprm) prm &= ~FGAC_PRM_FILE;
        if (!go_dprm) prm &= ~FGAC_PRM_DIRS;

        if ((q = fgac_check_entry_prm(state, curpath, prc, prm)) == -1) return 0;
        r = r || q;

        go_dprm = go_dprm && fgac_check_inh (state, curpath, FGAC_INH_DPI);
        go_fprm = go_fprm && fgac_check_inh (state, curpath, FGAC_INH_FPI);

        swappath(&oldpath, &curpath);
    }

    return r;
}

uint64_t fgac_check_inh_prms (fgac_state *state, fgac_path *path, const fgac_prc *prc)
{
    int go_fprm, go_dprm;

    char buffer1 [FGAC_LIMIT_PATH], buffer2 [FGAC_LIMIT_PATH];
    fgac_path path1 = fgac_path_init(buffer1), path2 = fgac_path_init(buffer2),
              *curpath = &path1, *oldpath = &path2;
    uint64_t allow, deny;

    go_dprm = fgac_is_dir(state, path) ? fgac_check_inh (state, path, FGAC_INH_DPI) : 0;
    go_fprm = fgac_check_inh (state, path, FGAC_INH_FPI);

    if (!fgac_str_cpy(fgac_get_path(oldpath), FGAC_PATH, FGAC_LIMIT_PATH)) return ENOMEM;

    fgac_get_entry_ad(state, oldpath, prc, &allow, &deny);

    while ((go_dprm || go_fprm) && fgac_parent(oldpath, curpath))
    {
        uint64_t a, d;
        fgac_get_entry_ad(state, curpath, prc, &a, &d);
        if (go_fprm)
        {
            allow |= a & FGAC_PRM_FILE;
            deny  |= d & FGAC_PRM_FILE;
        }
        if (go_dprm)
        {
            allow |= a & FGAC_PRM_DIRS;
            deny  |= d & FGAC_PRM_DIRS;
        }

        go_dprm = go_dprm && fgac_check_inh (state, curpath, FGAC_INH_DPI);
        go_fprm = go_fprm && fgac_check_inh (state, curpath, FGAC_INH_FPI);

        swappath(&oldpath, &curpath);
    }

    return allow & ~deny;
}


int fgac_check_dex (fgac_state *state, fgac_path *path, const fgac_prc *prc)
{
    char buffer1 [FGAC_LIMIT_PATH], buffer2 [FGAC_LIMIT_PATH];
    fgac_path path1 = fgac_path_init(buffer1), path2 = fgac_path_init(buffer2),
              *oldpath = &path1, *curpath = &path2;
    int dex;

    if (!fgac_exists(state, path)) return 0;
    
    if (cache_get_dex(state, path, prc, &dex)) return dex;

    if (prc->uid == 0 || prc->uid == state->uid) return 1;

    if (!fgac_str_cpy(fgac_get_path(oldpath), FGAC_PATH, FGAC_LIMIT_PATH)) return 0;

    while (fgac_parent(oldpath, curpath))
    {
#ifndef NDEBUG            
        printf("!checking DEX: '%s'\n", curpath->path);
#endif            

        if (!cache_get_dex(state, path, prc, &dex)) dex = fgac_check_inh_prm (state, curpath, prc, FGAC_PRM_DEX);
     
        if (dex) 
            cache_set_dex(state, curpath, prc, 1);
        else
        {
            cache_set_dex(state, curpath, prc, 0);
            return 0;
        }
        
        swappath(&oldpath, &curpath);
    }

#ifndef NDEBUG            
        printf("PATH DEX OK: '%s'\n", path->path);
#endif            

    cache_set_dex(state, path, prc, 1);

    return 1;
}

int fgac_check_prm (fgac_state *state, fgac_path *path, const fgac_prc *prc, uint64_t prm)
{
    if (!fgac_exists(state, path)) return 0;

    return prc->uid == 0 ||
           prc->uid == state->uid ||
           (fgac_check_dex(state, path, prc) && fgac_check_inh_prm(state, path, prc, prm));
}

uint64_t fgac_check_prms (fgac_state *state, fgac_path *path, const fgac_prc *prc, int susr)
{
    if (!fgac_exists(state, path)) return 0;

    if (susr && (prc->uid == 0 || prc->uid == state->uid)) return FGAC_PRM_ALLS;

    if (!fgac_check_dex(state, path, prc)) return 0;

    return fgac_check_inh_prms(state, path, prc);
}

typedef enum {OWNER, GROUP, OTHER} ugo_t;

int fgac_adjust_mode_inh (fgac_state *state, fgac_path *path, const fgac_prc *prc, ugo_t ugo)
{
    uint64_t rp = fgac_is_dir(state, path) ? FGAC_PRM_DREA : FGAC_PRM_FREA,
             wp = fgac_is_dir(state, path) ? FGAC_PRM_DWRI : FGAC_PRM_FWRI,
             xp = fgac_is_dir(state, path) ? FGAC_PRM_DEX  : FGAC_PRM_FEX,
             r, w, x,
             prm;

    if (!fgac_exists(state, path)/* || !fgac_check_dex(state, path, prc)*/)
    {
        switch (ugo)
        {
            case OWNER:
                path->statbuf.st_mode &= ~S_IRUSR;
                path->statbuf.st_mode &= ~S_IWUSR;
                path->statbuf.st_mode &= ~S_IXUSR;
                break;
            case GROUP:
                path->statbuf.st_mode &= ~S_IRGRP;
                path->statbuf.st_mode &= ~S_IWGRP;
                path->statbuf.st_mode &= ~S_IXGRP;
                break;
            case OTHER:
                path->statbuf.st_mode &= ~S_IROTH;
                path->statbuf.st_mode &= ~S_IWOTH;
                path->statbuf.st_mode &= ~S_IXOTH;
                break;
        }

        return 0;
    }

    prm = fgac_check_prms(state, path, prc, 0);

    r = prm & rp;
    w = prm & wp;
    x = prm & xp;

    switch (ugo)
    {
        case OWNER:
             r ? (path->statbuf.st_mode |= S_IRUSR) : (path->statbuf.st_mode &= ~S_IRUSR);
             w ? (path->statbuf.st_mode |= S_IWUSR) : (path->statbuf.st_mode &= ~S_IWUSR);
             x ? (path->statbuf.st_mode |= S_IXUSR) : (path->statbuf.st_mode &= ~S_IXUSR);
             break;
        case GROUP:
             r ? (path->statbuf.st_mode |= S_IRGRP) : (path->statbuf.st_mode &= ~S_IRGRP);
             w ? (path->statbuf.st_mode |= S_IWGRP) : (path->statbuf.st_mode &= ~S_IWGRP);
             x ? (path->statbuf.st_mode |= S_IXGRP) : (path->statbuf.st_mode &= ~S_IXGRP);
             break;
        case OTHER:
             r ? (path->statbuf.st_mode |= S_IROTH) : (path->statbuf.st_mode &= ~S_IROTH);
             w ? (path->statbuf.st_mode |= S_IWOTH) : (path->statbuf.st_mode &= ~S_IWOTH);
             x ? (path->statbuf.st_mode |= S_IXOTH) : (path->statbuf.st_mode &= ~S_IXOTH);
             break;
    }

    return 0;
}

int fgac_adjust_mode (fgac_state *state, fgac_path *path, const fgac_prc *prc)
{
    int rc;

    fgac_prc uprc, gprc, oprc;
    mode_t mode;
    
    if (cache_get_mode(state, path, prc, &mode))
    {
        path->statbuf.st_mode = mode;
        return 0;
    }

    if (path->statbuf.st_uid == prc->uid)
    {
        uprc.uid = prc->uid;
        uprc.gid = prc->gid;
        uprc.cmd = prc->cmd;
        uprc.ngroups = prc->ngroups;
        memcpy (uprc.groups, prc->groups, sizeof(*uprc.groups) * uprc.ngroups);

        gprc.uid = -1;
        gprc.gid = path->statbuf.st_gid;
        gprc.cmd = NULL;
        gprc.ngroups = 0;

        oprc.uid = -1;
        oprc.gid = -1;
        oprc.cmd = NULL;
        oprc.ngroups = 0;
    }
    else
    {
        uprc.uid = path->statbuf.st_uid;
        uprc.gid = -1;
        uprc.cmd = NULL;
        uprc.ngroups = 0;

        if (path->statbuf.st_gid == prc->gid)
        {
            gprc.uid = prc->uid;
            gprc.gid = prc->gid;
            gprc.cmd = prc->cmd;
            gprc.ngroups = prc->ngroups;
            memcpy (gprc.groups, prc->groups, sizeof(*gprc.groups) * gprc.ngroups);

            oprc.uid = -1;
            oprc.gid = -1;
            oprc.cmd = NULL;
            oprc.ngroups = 0;
        }
        else
        {
            gprc.uid = -1;
            gprc.gid = path->statbuf.st_gid;
            gprc.cmd = NULL;
            gprc.ngroups = 0;

            oprc.uid = prc->uid;
            oprc.gid = prc->gid;
            oprc.cmd = prc->cmd;
            oprc.ngroups = prc->ngroups;
            memcpy (oprc.groups, prc->groups, sizeof(*oprc.groups) * oprc.ngroups);
        }
    }

    if ((rc = fgac_adjust_mode_inh (state, path, &uprc, OWNER))) return rc;
    if ((rc = fgac_adjust_mode_inh (state, path, &gprc, GROUP))) return rc;
    if ((rc = fgac_adjust_mode_inh (state, path, &oprc, OTHER))) return rc;

    if (path->statbuf.st_uid == state->uid)
    {
        path->statbuf.st_mode |= S_IRUSR;
        path->statbuf.st_mode |= S_IWUSR;
        if (S_ISDIR(path->statbuf.st_mode)) path->statbuf.st_mode |= S_IXUSR;
    }
    else if (prc->uid == state->uid)
    {
        if (prc->gid == path->statbuf.st_gid)
        {
            path->statbuf.st_mode |= S_IRGRP;
            path->statbuf.st_mode |= S_IWGRP;
            if (S_ISDIR(path->statbuf.st_mode)) path->statbuf.st_mode |= S_IXGRP;
        }
        else
        {
            path->statbuf.st_mode |= S_IROTH;
            path->statbuf.st_mode |= S_IWOTH;
            if (S_ISDIR(path->statbuf.st_mode)) path->statbuf.st_mode |= S_IXOTH;
        }
    }
    
    if (!S_ISDIR(path->statbuf.st_mode))
    {
        if (fgac_check_dex(state, path, prc) && fgac_check_inh_prm(state, path, prc, FGAC_PRM_FSU)) path->statbuf.st_mode |= S_ISUID;
        if (fgac_check_dex(state, path, prc) && fgac_check_inh_prm(state, path, prc, FGAC_PRM_FSG)) path->statbuf.st_mode |= S_ISGID;
    }    
    
    cache_set_mode(state, path, prc, path->statbuf.st_mode);


    return 0;

}

struct stat * fgac_stat (fgac_state *state, fgac_path *path, const fgac_prc *prc)
{
    if (!fgac_prestat(state, path)) return NULL;
    if (path->statset != TRUE)
    {
        if (fgac_get_owner(state, path, &path->statbuf.st_uid)) {errno = EACCES; return NULL; }
        if (fgac_get_group(state, path, &path->statbuf.st_gid)) {errno = EACCES; return NULL; }
        if ((errno = fgac_adjust_mode(state, path, prc))) return NULL;
        path->statset = TRUE;
    }

    return &path->statbuf;
}

int fgac_check_prm2 (fgac_state *state, fgac_path *path, const fgac_prc *prc, uint64_t fprm, uint64_t dprm)
{
    if (!fgac_exists(state, path)) return 0;
    
    return  fgac_check_prm(state, path, prc, fgac_is_dir (state, path) ? dprm : fprm);
}

int fgac_check_inh (fgac_state *state, fgac_path *path, uint64_t inh)
{
    uint64_t cinh;
    if (fgac_get_inh (state, path, &cinh)) return 0;
    return (cinh & inh) == inh;
}

int fgac_set_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm)
{
    int rc;
    switch (state->type)
    {
        case FGAC_XATTR : rc =  xattr_set_prm(state, path, prm); break;
        case FGAC_FXATTR: rc = fxattr_set_prm(state, path, prm); break;
        case FGAC_SQLITE: rc =     db_set_prm(state, path, prm); break;
        default:          return FGAC_ERR_TYPE;
    }
    
    if (!rc) { cache_set_prm(state, path, prm); cache_stat_cleanup(state, path->path); }
    return rc;
}

int fgac_get_prm (fgac_state *state, fgac_path *path, fgac_prm *prm)
{
    int rc;
    
    if (cache_get_prm(state, path, prm)) return 0;
    
    switch (state->type)
    {
        case FGAC_XATTR : rc =  xattr_get_prm(state, path, prm); break;
        case FGAC_FXATTR: rc = fxattr_get_prm(state, path, prm); break;
        case FGAC_SQLITE: rc =     db_get_prm(state, path, prm); break;
        default:          return FGAC_ERR_TYPE;
    }
    
    if (!rc) cache_set_prm(state, path, prm);
    return rc;
}

int fgac_unset_prm (fgac_state *state, fgac_path *path, fgac_prm *prm)
{
    int rc;
    switch (state->type)
    {
        case FGAC_XATTR : rc =  xattr_unset_prm(state, path, prm); break;
        case FGAC_FXATTR: rc = fxattr_unset_prm(state, path, prm); break;
        case FGAC_SQLITE: rc =     db_unset_prm(state, path, prm); break;
        default:          return FGAC_ERR_TYPE;
    }
    
    if (!rc) { cache_unset_prm(state, path, prm); cache_stat_cleanup(state, path->path); }
    return rc;
}

int fgac_get_inh (fgac_state *state, fgac_path *path, uint64_t *inh)
{
    int rc;
    
    if (cache_get_inh(state, path, inh)) return 0;
    
    if (!path->inhset)
    {
        switch (state->type)
        {
            case FGAC_XATTR : rc =  xattr_get_inh(state, path, inh); break;
            case FGAC_FXATTR: rc = fxattr_get_inh(state, path, inh); break;
            case FGAC_SQLITE: rc =     db_get_inh(state, path, inh); break;
            default:          return FGAC_ERR_TYPE;
        }
        if (rc) return rc;
        path->inhset = 1;
        path->inh = *inh;
    }
    else
        *inh = path->inh;
       
    cache_set_inh(state, path, *inh);
    return FGAC_OK;
}

int fgac_set_inh (fgac_state *state, fgac_path *path, uint64_t inh)
{
    int rc;
    path->inhset = 0;
    switch (state->type)
    {
        case FGAC_XATTR : rc =  xattr_set_inh(state, path, inh); break;
        case FGAC_FXATTR: rc = fxattr_set_inh(state, path, inh); break;
        case FGAC_SQLITE: rc =     db_set_inh(state, path, inh); break;
        default:          return FGAC_ERR_TYPE;
    }
    
    if (!rc) { cache_set_inh(state, path, inh); cache_stat_cleanup(state, path->path); }
    return rc;
}

int fgac_get_owner (fgac_state *state, fgac_path *path, uid_t *uid)
{
    int rc;
    
    if (cache_get_owner(state, path, uid)) return 0;

    if (!path->uidset)
    {
        switch (state->type)
        {
            case FGAC_XATTR : rc =  xattr_get_owner(state, path, uid); break;
            case FGAC_FXATTR: rc = fxattr_get_owner(state, path, uid); break;
            case FGAC_SQLITE: rc =     db_get_owner(state, path, uid); break;
            default:          return FGAC_ERR_TYPE;
        }
        if (rc) return rc;
        path->uid = *uid;
    }
    else
        *uid = path->uid;

    cache_set_owner(state, path, *uid);
    return FGAC_OK;
}

int fgac_set_owner (fgac_state *state, fgac_path *path, uid_t uid)
{
    int rc;
    path->uidset = 0;
    path->statset = FALSE;
    switch (state->type)
    {
        case FGAC_XATTR : rc =  xattr_set_owner(state, path, uid); break;
        case FGAC_FXATTR: rc = fxattr_set_owner(state, path, uid); break;
        case FGAC_SQLITE: rc =     db_set_owner(state, path, uid); break;
        default:          return FGAC_ERR_TYPE;
    }
    
    if (!rc) { cache_set_owner(state, path, uid); cache_stat_cleanup(state, path->path); }
    return rc;
}

int fgac_get_group (fgac_state *state, fgac_path *path, gid_t *gid)
{
    int rc;
    
    if (cache_get_group(state, path, gid)) return 0;
    
    if (!path->gidset)
    {
        switch (state->type)
        {
            case FGAC_XATTR : rc =  xattr_get_group(state, path, gid); break;
            case FGAC_FXATTR: rc = fxattr_get_group(state, path, gid); break;
            case FGAC_SQLITE: rc =     db_get_group(state, path, gid); break;
            default:          return FGAC_ERR_TYPE;
        }
        if (rc) return rc;
        path->gidset = 1;
        path->gid = *gid;
    }
    else
        *gid = path->gid;

    cache_set_group(state, path, *gid);
    return FGAC_OK;
}

int fgac_set_group (fgac_state *state, fgac_path *path, gid_t gid)
{
    int rc;
    path->gidset = 0;
    path->statset = FALSE;
    switch (state->type)
    {
        case FGAC_XATTR : rc =  xattr_set_group(state, path, gid); break;
        case FGAC_FXATTR: rc = fxattr_set_group(state, path, gid); break;
        case FGAC_SQLITE: rc =     db_set_group(state, path, gid); break;
        default:          return FGAC_ERR_TYPE;
    }
    
    if (!rc) { cache_set_group(state, path, gid); cache_stat_cleanup(state, path->path); }
    return rc;
}

#define CLEARPATH(path)         \
    *((path)->hostpath) = '\0'; \
    (path)->is_dir    = UNSET;  \
    (path)->exists    = UNSET;  \
    (path)->inhset    = 0;      \
    (path)->uidset    = 0;      \
    (path)->gidset    = 0;      \
    (path)->statset   = UNSET;  \
    (path)->cache     = NULL;


int fgac_add (fgac_state *state, fgac_path *path, uid_t uid, gid_t gid)
{
    int rc;
    CLEARPATH(path);
    switch (state->type)
    {
        case FGAC_XATTR : rc =  xattr_add(state, path, uid, gid); break;
        case FGAC_FXATTR: rc = fxattr_add(state, path, uid, gid); break;
        case FGAC_SQLITE: rc =     db_add(state, path, uid, gid); break;
        default:          return FGAC_ERR_TYPE;
    }
    if (!rc) cache_add(state, path, uid, gid);
    return rc;
}

int fgac_delete (fgac_state *state, fgac_path *path)
{
    int rc;
    CLEARPATH(path);
    switch (state->type)
    {
        case FGAC_XATTR : rc =  xattr_delete(state, path); break;
        case FGAC_FXATTR: rc = fxattr_delete(state, path); break;
        case FGAC_SQLITE: rc =     db_delete(state, path); break;
        default:          return FGAC_ERR_TYPE;
    }
    if (!rc) cache_delete(state, path);
    return rc;
}

int fgac_rename (fgac_state *state, fgac_path *path, const char* newpath)
{
    int rc;
    CLEARPATH(path);
    switch (state->type)
    {
        case FGAC_XATTR : rc =  xattr_rename(state, path, newpath); break;
        case FGAC_FXATTR: rc = fxattr_rename(state, path, newpath); break;
        case FGAC_SQLITE: rc =     db_rename(state, path, newpath); break;
        default:          return FGAC_ERR_TYPE;
    }
    if (!rc) cache_rename(state, path, newpath);
    return rc;
}

fgac_prms fgac_get_prms (fgac_state *state, fgac_path *path)
{
    switch (state->type)
    {
        case FGAC_XATTR:  return xattr_get_prms(state,path);
        case FGAC_FXATTR: return fxattr_get_prms(state,path);
        case FGAC_SQLITE: return db_get_prms(state,path);
        default:          return fgac_prms_error();
    }
}

fgac_prms fgac_get_inhprms (fgac_state *state, fgac_path *path)
{
    fgac_prms r = fgac_prms_init();

    int go_fprm, go_dprm;

    char buffer1 [FGAC_LIMIT_PATH], buffer2 [FGAC_LIMIT_PATH];
    fgac_path path1 = fgac_path_init(buffer1), path2 = fgac_path_init(buffer2),
              *curpath = &path1, *oldpath = &path2;

    go_dprm = fgac_is_dir(state, path) ? fgac_check_inh (state, path, FGAC_INH_DPI) : 0;
    go_fprm = fgac_check_inh (state, path, FGAC_INH_FPI);

    if (!fgac_str_cpy(fgac_get_path(oldpath), FGAC_PATH, FGAC_LIMIT_PATH)) return fgac_prms_error();

    do
    {
        size_t i, s;
        fgac_prms q;

        if (!fgac_parent(oldpath, curpath)) break;

        q = fgac_get_prms(state, curpath);
        if (fgac_prms_is_error(&q)) return q;

        for (i = 0, s = fgac_prms_size(&q); i < s; ++i)
        {
            fgac_prm *p = fgac_prms_element(&q, i);
            if (!go_fprm) {p->allow &= ~FGAC_PRM_FILE; p->deny &= ~FGAC_PRM_FILE;}
            if (!go_dprm) {p->allow &= ~FGAC_PRM_DIRS; p->deny &= ~FGAC_PRM_DIRS;}
            if (p->allow || p->deny) fgac_prms_add(&r, p);
            if (fgac_prms_is_error(&r)) return r;
        }
        fgac_prms_free(&q);

        go_dprm = go_dprm && fgac_check_inh (state, curpath, FGAC_INH_FPI);
        go_fprm = go_fprm && fgac_check_inh (state, curpath, FGAC_INH_DPI);

        swappath(&oldpath, &curpath);
    } while (go_dprm || go_fprm);

    return r;
}

fgac_prms fgac_get_allprms (fgac_state *state, fgac_path *path)
{
    fgac_prms oprms = fgac_get_prms   (state, path),
              iprms = fgac_get_inhprms(state, path);

    fgac_prms_join(&oprms, &iprms);

    fgac_prms_free(&iprms);

    return oprms;
}

int fgac_copy_file_prm (fgac_state *state, fgac_path *path, fgac_path *parent)
{
    size_t i;
    int rc;
    fgac_prms prms = fgac_get_allprms(state, parent);

    if (fgac_prms_is_error(&prms)) return FGAC_ERR_NOPARENT;

    for (i = 0; i < fgac_prms_size(&prms); ++i)
    {
        fgac_prm *prm = fgac_prms_element(&prms, i);
        prm->allow &= FGAC_PRM_FILE;
        prm->deny  &= FGAC_PRM_FILE;
        if ((prm->allow || prm->deny) && (rc = fgac_set_prm(state, path, prm))) return rc;
    }
    return FGAC_OK;
}

int fgac_copy_dir_prm (fgac_state *state, fgac_path *path, fgac_path *parent)
{
    size_t i;
    int rc;
    fgac_prms prms = fgac_get_allprms(state, parent);

    if (fgac_prms_is_error(&prms)) return FGAC_ERR_NOPARENT;

    for (i = 0; i < fgac_prms_size(&prms); ++i)
    {
        fgac_prm *prm = fgac_prms_element(&prms, i);
        prm->allow &= FGAC_PRM_DIRS;
        prm->deny  &= FGAC_PRM_DIRS;
        if ((prm->allow || prm->deny) && (rc = fgac_set_prm(state, path, prm))) return rc;
    }
    return FGAC_OK;
}

int fgac_copy_prm (fgac_state *state, fgac_path *path, fgac_path *parent)
{
    size_t i;
    int rc;
    fgac_prms prms = fgac_get_allprms(state, parent);

    if (fgac_prms_is_error(&prms)) return FGAC_ERR_NOPARENT;

    for (i = 0; i < fgac_prms_size(&prms); ++i)
    {
        fgac_prm *prm = fgac_prms_element(&prms, i);
        if ((prm->allow || prm->deny) && (rc = fgac_set_prm(state, path, prm))) return rc;
    }
    return FGAC_OK;
}

int fgac_set_mkfile_prm (fgac_state *state, fgac_path *path)
{
    char pbuffer[FGAC_LIMIT_PATH];
    fgac_path parent = fgac_path_init(pbuffer);
    int rc;
    uint64_t inh = 0, pinh = 0;

    if (!fgac_parent(path, &parent)) return FGAC_ERR_NOPARENT;

    if ((rc = fgac_get_inh(state, &parent, &pinh))) return rc;

    if (pinh & FGAC_INH_SFI) inh |= FGAC_INH_FPI;
    if (pinh & FGAC_INH_SFK) inh |= FGAC_INH_FPK;

    if ((rc = fgac_set_inh(state, path, inh))) return rc;

    if (pinh & FGAC_INH_FPC) return fgac_copy_file_prm(state, path, &parent);

    return FGAC_OK;
}

int fgac_set_mkdir_prm (fgac_state *state, fgac_path *path)
{
    char buffer[FGAC_LIMIT_PATH];
    fgac_path parent = fgac_path_init(buffer);
    int rc;
    uint64_t inh = 0, pinh = 0;

    if (!fgac_parent(path, &parent)) return FGAC_ERR_NOPARENT;

    if ((rc = fgac_get_inh(state, &parent, &pinh))) return rc;

    if (pinh & FGAC_INH_SFI) inh |= FGAC_INH_FPI | FGAC_INH_SFI;
    if (pinh & FGAC_INH_SDI) inh |= FGAC_INH_DPI | FGAC_INH_SDI;

    if (pinh & FGAC_INH_SFK) inh |= FGAC_INH_FPK | FGAC_INH_SFK;
    if (pinh & FGAC_INH_SDK) inh |= FGAC_INH_DPK | FGAC_INH_SDK;

    if (pinh & FGAC_INH_SFC) inh |= FGAC_INH_FPC | FGAC_INH_SFC;
    if (pinh & FGAC_INH_SDC) inh |= FGAC_INH_DPC | FGAC_INH_SFC;


    if ((rc = fgac_set_inh(state, path, inh))) return rc;

    if ((pinh & FGAC_INH_DPC) && (pinh & FGAC_INH_FPC))
        return fgac_copy_prm (state, path, &parent);
    else if (pinh & FGAC_INH_DPC)
        return fgac_copy_dir_prm (state, path, &parent);
    else if (pinh & FGAC_INH_FPC)
        return fgac_copy_file_prm (state, path, &parent);
    else
        return FGAC_OK;
}

int fgac_unset_fpi (fgac_state *state, fgac_path *path)
{
    uint64_t inh;
    int rc;
    char buffer[FGAC_LIMIT_PATH];
    fgac_path parent = fgac_path_init(buffer);

    if ((rc = fgac_get_inh(state, path, &inh))) return rc;

    if (!(inh & FGAC_INH_FPI)) return FGAC_OK;

    inh &= ~FGAC_INH_FPI;

    if ((rc = fgac_set_inh(state, path, inh))) return rc;

    if (!(inh & FGAC_INH_SFI)) return FGAC_OK;

    if (!fgac_parent(path, &parent)) return FGAC_OK;

    return fgac_copy_file_prm(state, path, &parent);
}

int fgac_unset_dpi (fgac_state *state, fgac_path *path)
{
    uint64_t inh;
    int rc;
    char buffer[FGAC_LIMIT_PATH];
    fgac_path parent = fgac_path_init(buffer);

    if ((rc = fgac_get_inh(state, path, &inh))) return rc;

    if (!(inh & FGAC_INH_DPI)) return FGAC_OK;

    inh &= ~FGAC_INH_DPI;

    if ((rc = fgac_set_inh(state, path, inh))) return rc;

    if (!(inh & FGAC_INH_SDI)) return FGAC_OK;

    if (!fgac_parent(path, &parent)) return FGAC_OK;

    if (fgac_is_dir(state, path))
        return fgac_copy_dir_prm(state, path, &parent);
    else
        return FGAC_OK;

}

fgac_path fgac_path_init(char *path)
{
    fgac_path r;
    r.path = path;
    CLEARPATH(&r);
    return r;
}

char *fgac_get_path (fgac_path *path)
{
    return path->path;
}

char *fgac_get_hostpath (fgac_state *state, fgac_path *path)
{
    if (!path->hostpath[0])
        if (!fgac_str_cat2 (path->hostpath, state->datadir, path->path, FGAC_LIMIT_PATH)) return NULL;
    return path->hostpath;
}

int fgac_parent(fgac_path *path, fgac_path *parent)
{
    if (path->path[0] == '/' && !(path->path[1])) return 0;
    fgac_str_cpy (parent->path, path->path, FGAC_LIMIT_PATH);
    dirname(parent->path);
    CLEARPATH(parent);
    return 1;
}

int fgac_exists (fgac_state *state, fgac_path *path)
{
    if (path->exists == UNSET)
    {
        char *host = FGAC_HOSTPATH;
        struct stat buf;
        path->exists = host && lstat (host, &buf) == 0;
    }
    return path->exists;
}

int fgac_is_dir (fgac_state *state, fgac_path* path)
{
    if (path->is_dir == UNSET)
    {
        struct stat *stat = fgac_prestat (state, path);
        if (stat)
            path->is_dir = S_ISDIR(stat->st_mode);
        else
            path->is_dir = FALSE;
    }
    return path->is_dir;
}
