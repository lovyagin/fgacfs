/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

/**
*  @file      fgacfs.h
*  @brief     basic include file for fgacfs library
*  @authors   Roman Y. Dayneko, <dayneko3000@gmail.com>,
*             Nikita Yu. Lovyagin, <lovyagin@mail.com>
*  @copyright GNU GPLv3.
*/


#ifndef FGACFS_H_INCLUDED
#define FGACFS_H_INCLUDED

#include <sys/stat.h>

#include <sqlite3.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

/*********************  General constants  *************************/
//{
#define FGAC_LIMIT_PATH   8192
#define FGAC_LIMIT_XHASH  8192
#define FGAC_LIMIT_GROUPS 8192
#define FGAC_LOCK_NAME    "/mount.lock"
#define FGAC_FIFO_NAME    "/mount.fifo"

/** type of FGACFS permission storing **/
typedef enum
{
    FGAC_XATTR,   /** safe (ASCII) xattr  **/
    FGAC_SQLITE,  /** sqlite3 database    **/
    FGAC_FXATTR   /** fast (binary) xattr **/
} fgac_type;

extern const char fgac_name[];
extern const char fgac_version[];
extern char fgac_prg_name[];
extern char fgac_prg_version[];

//}

/**********************  FGACFS Permissions  *************************/
//{
/** file permission inheritance **/
#define FGAC_INH_FPI (1uLL <<  0) /** Inherit file permission **/
#define FGAC_INH_FPK (1uLL <<  1) /** File permission keep (when FPI removed) **/
#define FGAC_INH_FPC (1uLL <<  2) /** File permission copy (like inherited) **/

/** file permission inheritance flag set **/
#define FGAC_INH_SFI (1uLL << 16) /** Set FPI to child **/
#define FGAC_INH_SFK (1uLL << 17) /** Set FPK to child **/
#define FGAC_INH_SFC (1uLL << 18) /** Set FPI to child **/

/** directory permission inheritance **/
#define FGAC_INH_DPI (1uLL << 32) /** Inherit directory permission **/
#define FGAC_INH_DPK (1uLL << 33) /** Directory permission keep (when DPI removed) **/
#define FGAC_INH_DPC (1uLL << 34) /** Directory permission copy (like inherited) **/

/** directory permission inheritance flag set **/
#define FGAC_INH_SDI (1uLL << 48) /** Set DPI to child **/
#define FGAC_INH_SDK (1uLL << 49) /** Set DPK to child **/
#define FGAC_INH_SDC (1uLL << 50) /** Set DPI to child **/

#define FGAC_INH_FILE (FGAC_INH_FPI | FGAC_INH_FPK)                 /** File-assigned inheritance **/
#define FGAC_INH_DIRS (FGAC_INH_FPI | FGAC_INH_FPK | FGAC_INH_FPC | \
                       FGAC_INH_SFI | FGAC_INH_SFK | FGAC_INH_SFC | \
                       FGAC_INH_DPI | FGAC_INH_DPK | FGAC_INH_DPC | \
                       FGAC_INH_SDI | FGAC_INH_SDK | FGAC_INH_SDC ) /** Dir-assigned inheritance **/

#define FGAC_INH_INHS (FGAC_INH_FPI | FGAC_INH_FPK | FGAC_INH_DPI | FGAC_INH_DPK) /** Inheritance flag **/
#define FGAC_INH_TRMS (FGAC_INH_FPC | FGAC_INH_DPC |                \
                       FGAC_INH_SFI | FGAC_INH_SFK | FGAC_INH_SFC | \
                       FGAC_INH_SDI | FGAC_INH_SDK | FGAC_INH_SDC )               /** Transmission flags **/

/** File permissions **/

#define FGAC_PRM_FRD (1uLL <<  0) /** Read file **/
#define FGAC_PRM_FRA (1uLL <<  1) /** Stat file permissions (getattr) **/
#define FGAC_PRM_FRP (1uLL <<  2) /** Read file permissions **/
#define FGAC_PRM_FXA (1uLL <<  3) /** Read file extended attributes **/
#define FGAC_PRM_FSL (1uLL <<  4) /** Read (follow) symlink **/

#define FGAC_PRM_FRW (1uLL <<  8) /** ReWrite file (modify existing content, no append) **/
#define FGAC_PRM_FAP (1uLL << 19) /** Append to file (write to the end) **/
#define FGAC_PRM_FTR (1uLL << 20) /** Truncate (reduce file size, enlarging affected by FAP **/
#define FGAC_PRM_FCA (1uLL <<  9) /** Change attributes (utime) **/
#define FGAC_PRM_FCP (1uLL << 10) /** Chmod (change permission) file **/
#define FGAC_PRM_FCI (1uLL << 11) /** Change permission inheritance of the file **/
#define FGAC_PRM_FCO (1uLL << 12) /** Chown file (inside group) **/
#define FGAC_PRM_FCG (1uLL << 13) /** Change owner group **/
#define FGAC_PRM_FSP (1uLL << 14) /** Chown and chgroup to parent dir owner:group **/
#define FGAC_PRM_FRM (1uLL << 15) /** Remove file **/
#define FGAC_PRM_FMV (1uLL << 16) /** Rename file (inside parent directory) **/
#define FGAC_PRM_FMX (1uLL << 17) /** Modify file extended attributes **/
#define FGAC_PRM_FSX (1uLL << 18) /** (Un)set file execution for bit for current user **/

#define FGAC_PRM_FEX (1uLL << 24) /** Execute file **/
#define FGAC_PRM_FSU (1uLL << 25) /** Setuid file **/
#define FGAC_PRM_FSG (1uLL << 26) /** Setgid file **/

#define FGAC_PRM_FREA (FGAC_PRM_FRD | FGAC_PRM_FRA | FGAC_PRM_FRP | FGAC_PRM_FXA | FGAC_PRM_FSL)   /** File read permission **/
#define FGAC_PRM_FWRI (FGAC_PRM_FRW | FGAC_PRM_FCA | FGAC_PRM_FCP | FGAC_PRM_FCI | FGAC_PRM_FCO | \
                       FGAC_PRM_FCG | FGAC_PRM_FSP | FGAC_PRM_FRM | FGAC_PRM_FMV | FGAC_PRM_FMX | \
                       FGAC_PRM_FSX | FGAC_PRM_FAP | FGAC_PRM_FTR )                                /** File write permission **/
#define FGAC_PRM_FEXE (FGAC_PRM_FEX)                                                               /** File exec permission **/
#define FGAC_PRM_SUGI (FGAC_PRM_FSU | FGAC_PRM_FSG)                                                /** Setuid and Setgid */
#define FGAC_PRM_FILE (FGAC_PRM_FREA | FGAC_PRM_FWRI | FGAC_PRM_FEXE | FGAC_PRM_SUGI)              /** File permission **/


/** Directory permissions **/

#define FGAC_PRM_DRD (1uLL << 32) /** Read directory context **/
#define FGAC_PRM_DRA (1uLL << 33) /** Stat directory (getattr) **/
#define FGAC_PRM_DRP (1uLL << 34) /** Read directory permissions **/
#define FGAC_PRM_DXA (1uLL << 35) /** Read directory extended attributes **/

#define FGAC_PRM_DAF (1uLL << 37) /** Create (add) files (mknod), same owner as parent directory's one **/
#define FGAC_PRM_DAD (1uLL << 38) /** Create (add) subdirectory (mkdir), same owner as parent directory's one **/
#define FGAC_PRM_DAL (1uLL << 39) /** Create (add) symbolic link in the directory, same owner as parent directory's one **/
#define FGAC_PRM_DAC (1uLL << 40) /** Create (add) symbolic device **/
#define FGAC_PRM_DAB (1uLL << 41) /** Create (add) block device **/
#define FGAC_PRM_DAS (1uLL << 42) /** Create (add) socket **/
#define FGAC_PRM_DAP (1uLL << 43) /** Create (add) named pipe (fifo) **/
#define FGAC_PRM_DOF (1uLL << 44) /** Create own files (mknod) **/
#define FGAC_PRM_DOD (1uLL << 45) /** Create own subdirectory (mkdir) **/
#define FGAC_PRM_DOL (1uLL << 46) /** Create own symbolic link in the directory **/
#define FGAC_PRM_DOC (1uLL << 47) /** Create own symbolic device **/
#define FGAC_PRM_DOB (1uLL << 48) /** Create own block device **/
#define FGAC_PRM_DOS (1uLL << 49) /** Create own socket **/
#define FGAC_PRM_DOP (1uLL << 50) /** Create own named pipe fifo **/
#define FGAC_PRM_DCA (1uLL << 53) /** Change attributes (utime) **/
#define FGAC_PRM_DCP (1uLL << 54) /** Chmod (change permission) directory **/
#define FGAC_PRM_DCI (1uLL << 55) /** Change permission inheritance rules of the directory **/
#define FGAC_PRM_DCT (1uLL << 56) /** Change permission transmit rules of the directory **/
#define FGAC_PRM_DCO (1uLL << 57) /** Chown directory (inside group) **/
#define FGAC_PRM_DCG (1uLL << 58) /** Change owner group **/
#define FGAC_PRM_DSP (1uLL << 59) /** Chown and chgroup to parent dir owner:group **/
#define FGAC_PRM_DRM (1uLL << 60) /** Remove directory **/
#define FGAC_PRM_DMV (1uLL << 61) /** Rename directory (inside parent directory) **/
#define FGAC_PRM_DMX (1uLL << 62) /** Modify directory extended attributes **/

#define FGAC_PRM_DEX (1uLL << 63) /** Execute directory (chdir) **/

#define FGAC_PRM_DREA (FGAC_PRM_DRD | FGAC_PRM_DRA | FGAC_PRM_DRP | FGAC_PRM_DXA)                   /** Directory read permission **/
#define FGAC_PRM_DWRI (FGAC_PRM_DAF | FGAC_PRM_DAD | FGAC_PRM_DAL | FGAC_PRM_DAC | FGAC_PRM_DAB | FGAC_PRM_DAS | FGAC_PRM_DAP | \
                       FGAC_PRM_DOF | FGAC_PRM_DOD | FGAC_PRM_DOL | FGAC_PRM_DOC | FGAC_PRM_DOB | FGAC_PRM_DOS | FGAC_PRM_DOP | \
                       FGAC_PRM_DCA | FGAC_PRM_DCP | FGAC_PRM_DCI | FGAC_PRM_DCT | FGAC_PRM_DCO | FGAC_PRM_DCG | FGAC_PRM_DSP | \
                       FGAC_PRM_DRM | FGAC_PRM_DMV | FGAC_PRM_DMX )                               /** Directory write permission **/
#define FGAC_PRM_DEXE (FGAC_PRM_DEX)                                                                /** Directory exec permission **/
#define FGAC_PRM_DIRS (FGAC_PRM_DREA | FGAC_PRM_DWRI | FGAC_PRM_DEXE)                               /** Directory permission **/

#define FGAC_PRM_ALLS FGAC_PRM_FILE | FGAC_PRM_DIRS                                                 /** All permissions **/
//}

/***********************  FGACFS Data types  *************************/
//{

struct cache;

/**
* @struct fgac_state
* @brief  handler for fgac filesystem operated
* @var    fgac_state::hostdir
*         Field 'hostdir' keeps real (absolute) path for host (real) fgacfs directory
* @var    fgac_state::datadir
*         Field 'datadir' keeps real (absolute) path for data subdirectory of hostdir
*         (where actual files and directories of filesystem is kept in host)
* @var    fgac_state::db
*         Field 'db' is database handler (of NULL if xattr version is used)
* @var    fgac_state::check_prexec
*         Field 'check_prexec' is 1 if executable process permission checking required, 0 otherwise
* @var    fgac_state::type
*         Field 'type' store FGACFS permission storing type
* @var    fgac_state::uid
*         Field 'uid' store FGACFS owner UID
* @var    fgac_state::gid
*         Field 'gid' store FGACFS owner GID
* @var    cache::cache
*         Field 'cache' store FGACFS cache pointer
* @var    fd_lock::fd_lock
*         Field 'fd_lock' store FGACFS mount lock fd
* @var    fd_fifo::fd_fifo
*         Field 'fd_fifo' store FGACFS IPC fifo read end fd
* @var    dio::dio
*         Field 'dio' set to 1 if direct_io user, 0 otherwise
*/
typedef struct
{
    char          hostdir[FGAC_LIMIT_PATH];
    char          datadir[FGAC_LIMIT_PATH];
    sqlite3      *db;
    int           check_prexec;
    fgac_type     type;
    uid_t         uid;
    gid_t         gid;
    struct cache *cache;
    int           fd_lock;
    int           fd_fifo;
    char          buffer[FGAC_LIMIT_PATH];
    size_t        bpos;
    int           dio;
} fgac_state;

/** permission category **/
typedef enum
{
    FGAC_CAT_ALL, /**< ALL users                                  **/
    FGAC_CAT_OUS, /**< Owner USer                                 **/
    FGAC_CAT_OGR, /**< Owner Group                                **/
    FGAC_CAT_OTH, /**< OTHer (not owner's group)                  **/
    FGAC_CAT_UID, /**< User IDentifier                            **/
    FGAC_CAT_GID, /**< Group IDentifier                           **/
    FGAC_CAT_PEX  /**< Process EXecutable                         **/
} fgac_cat;

/**
* @struct fgac_prm
* @brief  represent single fgacfs permission record
* @var    fgac_prm::cat
*         Field 'cat' keeps permission category
* @var    fgac_prm::uid
*         Union field 'srcs.uid' keeps user id, iff cat = FGAC_CTG_UID
* @var    fgac_prm::gid
*         Union field 'srca.gid' keeps group id, iff cat = FGAC_CTG_GID
* @var    fgac_prm::cmd
*         Union field 'srca.cmd' keeps process executable name, iff cat = FGAC_CTG_PEX
* @var    fgac_prm::allow
*         Bitfield 'allow' keeps allowed permissions
* @var    fgac_prm::deny
*         Bitfield 'deny' keeps allowed permissions
*/

typedef struct
{
    fgac_cat cat;
    union
    {
        uid_t uid;
        gid_t gid;
        char *cmd;
    } prc;
    uint64_t allow;
    uint64_t deny;
} fgac_prm;

/**
* @struct fgac_prms
* @brief  represent array of permissions (dynamic)
* @var    fgac_prms::data
*         Field 'data' is array of permissions
* @var    fgac_prm::size
*         Field 'size' keeps number of permissions in array
*/
typedef struct
{
    fgac_prm *array;
    size_t length;
} fgac_prms;

/**
* @struct fgac_prc
* @brief  fgacfs accessing process detail
* @var    fgac_prc::uid
*         Field 'uid' keeps user id
* @var    fgac_prc::gid
*         Field 'gid' keeps group id
* @var    fgac_prc::cmd
*         Field 'cmd' keeps process executable name
*/
typedef struct
{
    uid_t uid;
    gid_t gid;
    char *cmd;
    gid_t groups[FGAC_LIMIT_GROUPS];
    int   ngroups;
} fgac_prc;


/**
* @struct  fgac_path
* @brief   fgacfs file or directory path
* @warning incomplete type, not to be used directly
*/
typedef struct fgac_path fgac_path;

//}

/*********************  FGACFS initialization  ***********************/
//{


/**
* @brief      initialize library
* @param[in]  name    program name
* @param[in]  version program version string
*/
void fgac_init (const char *name, const char *version);

/**
* @brief      open existing fgacfs system for operating
* @param[in]  hostdir path to host directory
* @param[in]  check_prexec should be non-zero if executable process permission checking required, 0 otherwise
* @param[in]  dio should be non-zero if direct_io is required
* @param[in]  cache_size cache size (number of entries), uncached if 0
* @param[in]  state_cache should be non-zero to enable stat caching
* @param[out] state   pointer to pointer to a newly allocated fgacfs handler or to NULL on fail
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
*/
int fgac_open   (const char *hostdir, int check_prexec, int dio, fgac_state **state, size_t cache_size, int stat_cache);

/**
* @brief         close fgacfs
* @param[in,out] state fgacfs handler, NULLs on exit
*/
void fgac_close (fgac_state **state);

/**
* @brief     do mount stuff (set mount lock, open fifo for listening if required)
* @param[in] state      fgacfs handler
* @param[in] mountpoint fgacfs mount directory (VFS dir not affected, used for fgac_attach only)
* @return    FGAC_OK (0) on success
*            fgacfs error code on fail
*/
int fgac_mount (fgac_state *state, const char *mountpoint);

/**
* @brief      attach to either mounted on not FGACFS (open fifo for writting)
* @param[in]  state      fgacfs handler
* @param[out] mountpoint mount directory (empty string for umounted filesystem)
* @warning    mountpoint should be at least FGAT_LIMIT_PATH size
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
*/
int fgac_attach (fgac_state *state, char *mountpoint);


/**
* @brief      create fgacfs filesystem
* @param[in]  hostdir   host directory on real filesystem
* @param[in]  type      FGAC_XATTR  for xattr  version
*                       FGAC_SQLITE for sqlite version
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
*/
int fgac_create (const char *hostdir, fgac_type type);

//}

/************  Permission check and inherit functions  *************/
//{

/**
* @brief      check specific permission
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @param[in]  prc   accessing process attributes
* @param[in]  prm   permission to check (bitmask)
* @return     non-zero if all operations are permitted, 0 otherwise
* @warning    FGAC_PRM_DEX for all path is pre-checked before checking entry permission
*/
int fgac_check_prm (fgac_state *state, fgac_path *path, const fgac_prc *prc, uint64_t prm);

/**
* @brief      check process permissions
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @param[in]  prc   accessing process attributes
* @param[in]  susr  set to 1 if superuser and owner should got FGAC_PRMS_ALL, 0 to get actual permissions
* @return     allowed permission bitmask
* @warning    FGAC_PRM_DEX for all path is pre-checked before checking entry permission
*/
uint64_t fgac_check_prms (fgac_state *state, fgac_path *path, const fgac_prc *prc, int susr);


/**
* @brief      stat fgacfs entry
* @param[in]  state   fgacfs handler
* @param[in]  path    path to file or directory
* @param[in]  prc     accessing process attributes
* @return     pointer to stat buffer on success or NULL and set errno appropriately fail
*/
struct stat * fgac_stat (fgac_state *state, fgac_path *path, const fgac_prc *prc);

/**
* @brief      сheck inheritance or transmit flag
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @param[in]  inh   inheritance bit to check
* @return     non-zero if flag is set, 0 otherwise
*/
int fgac_check_inh (fgac_state *state, fgac_path *path, uint64_t inh);

/**
* @brief      сheck if entry exists in FGACFS
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @return     non-zero if file exists, 0 otherwise
* @warning    fgac_check_prm and fgac_check_inh returns 0 on any file/dir access error
*             fgac_exists is only way to pre-check file existence and separate no access and no file situation
*/
int fgac_exists (fgac_state *state, fgac_path *path);

/**
* @brief      сheck if entry is directory in FGACFS
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @return     1 if it is a directory, 0 otherwise
* @warning    if the entry doesn't exists, result in unpredictable
*/
int fgac_is_dir (fgac_state *state, fgac_path* path);

/**
* @brief      check if dir or file operation is permitted
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @param[in]  prc   accessing process attributes
* @param[in]  dprm  permission to check, if entry is a directory
* @return     non-zero if operation permitted, 0 otherwise
* @warning    FGAC_PRM_DEX for all path is pre-checked before checking entry permission
*/
int fgac_check_prm2 (fgac_state *state, fgac_path *path, const fgac_prc *prc, uint64_t fprm, uint64_t dprm);

//}

/*****************  Permission reading functions  ******************/
//{

/**
* @brief      get permissions (no inheritance check at this point)
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @param[in]  prm   permission record defining type, category and source of permission
* @param[out] prm   permission record with allow and deny bitfield refined
* @return     FGAC_OK on success or errorcode on fail
*/
int fgac_get_prm (fgac_state *state, fgac_path *path, fgac_prm *prm);

/**
* @brief      get permissions (no inheritance check at this point)
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @return     array of permission records
* @warning    return array should be free'd by fgac_prms_free()
*/
fgac_prms fgac_get_prms (fgac_state *state, fgac_path *path);

/**
* @brief      get inherited permissions (no recorded permissions loaded at this point)
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @return     array of permission records
* @warning    return array should be free'd by fgac_prms_free()
*/
fgac_prms fgac_get_inhprms (fgac_state *state, fgac_path *path);

/**
* @brief      get joined (own and inherited permissions)
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @return     array of permission records
* @warning    return array should be free'd by fgac_prms_free()
*/
fgac_prms fgac_get_allprms (fgac_state *state, fgac_path *path);

/**
* @brief      get inheritance flags
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @param[out] inh   inheritance flags
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
*/
int fgac_get_inh (fgac_state *state, fgac_path *path, uint64_t *inh);

//}

/*****************  Permission setting functions  ******************/
//{

/**
* @brief      add, modify or delete (if allow=deny=0) permission record
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @param[in]  prm   permission record to set
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
*/
int fgac_set_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm);

/**
* @brief      set file inheritance flags
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @param[in]  inh   inheritance flags
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
*/
int fgac_set_inh (fgac_state *state, fgac_path *path, uint64_t inh);


//}


/*********************  Errors and messages  ***********************/
//{

/** Error codes **/
#define FGAC_ERR (1 << 13)   /** Error-code bit (message, if not set)                      **/
#define FGAC_PRG (1 << 14)   /** Program-level bit (this library level if not set)         **/


#define FGAC_ERR_MALLOC    (FGAC_ERR |  0)
#define FGAC_ERR_DBOPEN    (FGAC_ERR |  1)
#define FGAC_ERR_XATTROPEN (FGAC_ERR |  2)
#define FGAC_ERR_NODATA    (FGAC_ERR |  4)
#define FGAC_ERR_XATTRCRT  (FGAC_ERR |  5)
#define FGAC_ERR_XATTRFMT  (FGAC_ERR |  6)
#define FGAC_ERR_HOSTDIR   (FGAC_ERR |  7)
#define FGAC_ERR_HOSTEMPTY (FGAC_ERR |  8)
#define FGAC_ERR_HOSTERROR (FGAC_ERR |  9)
#define FGAC_ERR_TYPE      (FGAC_ERR | 10)
#define FGAC_ERR_DBCREATE  (FGAC_ERR | 11)
#define FGAC_ERR_DBQUERY   (FGAC_ERR | 12)
#define FGAC_ERR_PRMCAT    (FGAC_ERR | 13)
#define FGAC_ERR_XHASH     (FGAC_ERR | 14)
#define FGAC_ERR_PATH      (FGAC_ERR | 15)
#define FGAC_ERR_NOPARENT  (FGAC_ERR | 16)
#define FGAC_ERR_LOCK      (FGAC_ERR | 17)
#define FGAC_ERR_BADUID    (FGAC_ERR | 18)

/** Message codes **/

#define FGAC_OK                     0
#define FGAC_MSG_VERSION            1



extern const char * const fgac_errlist[];
extern const char * const fgac_msglist[];

extern const char * const fgacprg_errlist[];
extern const char * const fgacprg_msglist[];


/**
* @brief      get a message string by index
* @param[in]  idx is an index in fgac_errlist or fgac_msglist array if FGAC_PRG bit is unset
*                 is an index in fgacprg_errlist or fgacprg_msglist array otherwise
* @return     pointer to message (constant data, not needed to be freed
*/
const char * fgac_msg (size_t idx);

/**
* @brief      print a formatted message by index
*             prints to stderr, if FGAC_ERR of idx is set to stdout  otherwise
* @param[in]  idx is an index in fgac_errlist or fgac_msglist array if FGAC_PRG bit is unset
*                 is an index in fgacprg_errlist or fgacprg_msglist array otherwise
* @return     pointer to message (constant data, not needed to be freed
*/
void fgac_put_msg (size_t idx, ...);


/**
* @brief      print program and library version
*/
void fgac_msg_version (void);


//}

/********************  Entry adding and info  **********************/
//{
/**
* @brief      get file owner id (no permission check at this point)
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @param[out] uid   owner uid
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
*/
int fgac_get_owner (fgac_state *state, fgac_path *path, uid_t *uid);

/**
* @brief      set file owner id (no permission check at this point)
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @param[in]  uid   new owner uid
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
*/
int fgac_set_owner (fgac_state *state, fgac_path *path, uid_t uid);

/**
* @brief      get file owner group id (no permission check at this point)
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @param[out] uid   owner gid
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
*/
int fgac_get_group (fgac_state *state, fgac_path *path, gid_t *gid);

/**
* @brief      set file owner group id (no permission check at this point)
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @param[in]  gid   new owner gid
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
*/
int fgac_set_group (fgac_state *state, fgac_path *path, gid_t gid);

/**
* @brief      create file or directory (fgacfs only)
* @param[in]  state fgacfs handler
* @param[in]  path    path to file or directory to create
* @param[in]  uid     owner user id
* @param[in]  oid     owner group id
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
* @warning    only fgacfs synchronization provided, actual host fs operation should be done separately
*/
int fgac_add (fgac_state *state, fgac_path *path, uid_t uid, gid_t gid);

/**
* @brief      remove file or directory (fgacfs only)
* @param[in]  state fgacfs handler
* @param[in]  path    path to file or directory to remove
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
* @warning    only fgacfs synchronization provided, actual host fs operation should be done separately
*/
int fgac_delete (fgac_state *state, fgac_path *path);

/**
* @brief      rename file or directory inside filesystem (fgacfs only)
* @param[in]  state fgacfs handler
* @param[in]  path    path to file or directory to rename
* @param[in]  newpath new path of the file or directory
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
* @warning    only fgacfs synchronization provided, actual host renaming should be done separately
*/
int fgac_rename (fgac_state *state, fgac_path *path, const char* newpath);

/**
* @brief      set inheritance and permission of newly created file
*             according to parent inheritance
* @param[in]  state fgacfs handler
* @param[in]  path  path to file
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
* @warning    fgac_add should be called before and separately
*/
int fgac_set_mkfile_prm (fgac_state *state, fgac_path *path);

/**
* @brief      set inheritance and permission of newly created directory
*             according to parent inheritance
* @param[in]  state fgacfs handler
* @param[in]  path  path to directory
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
* @warning    fgac_add should be called before and separately
*/
int fgac_set_mkdir_prm (fgac_state *state, fgac_path *path);

/**
* @brief      unset file permissions inheritance flag (FGAC_INH_FPI)
*             and keep (copy) inherited permissions if needed
* @param[in]  state fgacfs handler
* @param[in]  path  path to file or directory
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
*/
int fgac_unset_fpi (fgac_state *state, fgac_path *path);

/**
* @brief      unset directory permissions inheritance flag (FGAC_INH_DPI)
*             and keep (copy) inherited permissions if needed
* @param[in]  state fgacfs handler
* @param[in]  path  path to directory
* @return     FGAC_OK (0) on success
*             fgacfs error code on fail
*/
int fgac_unset_dpi (fgac_state *state, fgac_path *path);
//}

/************************  Misc function  **************************/
//{

/**
* @brief      create correct fgac_path variable
* @param[in]  path    static or preallocated path buffer containing fgacfs-relative path (or initialized later)
* @return     fgac_path variable
*/
fgac_path fgac_path_init(char *path);

/**
* @brief      get relative path from fgac_path
* @param[in]  path    fgac_path variable
* @return     relative FGACFS entry path
*/
char *fgac_get_path (fgac_path *path);

/**
* @brief      get host (real FS) path from fgac_path
* @param[in]  state   fgacfs state
* @param[in]  path    fgac_path variable
* @return     real (absolute FS) entry path
*/
char *fgac_get_hostpath (fgac_state *state, fgac_path *path);


/**
 * @brief  fast type macro to get FGACFS path in usual situation (from `path' variable)
 */
#define FGAC_PATH \
    fgac_get_path(path)

/**
 * @brief  fast type macro to get FGACFS hostpath in usual situation (from `state' and `path' variables)
 */
#define FGAC_HOSTPATH \
    fgac_get_hostpath(state, path)

/**
* @brief      find parent fgacfs directory
* @param[in]  path     file or directory
* @param[out] parent   parent directory (should be preinitialized properly)
* @return     1 if parent found, 0 if there is no parent, i.e. path is "/"
*/
int fgac_parent(fgac_path *path, fgac_path *parent);

/**
* @brief      safe copy strings
* @param[in]  dest     destination string buffer
* @param[in]  source   source string
* @param[in]  size     buffer size (at most size-1 chars plus '\0' would be copied)
* @return     1 on success, 0 on buffer overflow (safe, dest is zero-length string)
*/
int fgac_str_cpy (char *dest, const char *source, size_t size);

/**
* @brief      safe add string to the chosen position of another string
* @param[in]  dest     destination string buffer
* @param[in]  shifgac    starting position
* @param[in]  source   added string
* @param[in]  size     buffer size (including zero-char)
* @return     1 on success, 0 on buffer overflow (safe, dest is zero-length string)
*/
int fgac_str_add (char *dest, size_t shifgac, const char *source, size_t size);

/**
* @brief      safe add string to the end of another string (concatenate)
* @param[in]  dest     destination string buffer
* @param[in]  source   added string
* @param[in]  size     buffer size (including zero-char)
* @return     1 on success, 0 on buffer overflow (safe, dest is zero-length string)
*/
int fgac_str_cat (char *dest, const char *source, size_t size);


/**
* @brief      safe concatenate two strings
* @param[in]  dest     destination string buffer
* @param[in]  s1       first string
* @param[in]  s2       second string
* @param[in]  size     buffer size (including zero-char)
* @return     1 on success, 0 on buffer overflow (safe, dest is zero-length string)
*/
int fgac_str_cat2 (char *dest, const char *s1, const char *s2, size_t size);

/**
* @brief      safe concatenate three strings
* @param[in]  dest     destination string buffer
* @param[in]  s1       first string
* @param[in]  s2       second string
* @param[in]  s2       third string
* @param[in]  size     buffer size (including zero-char)
* @return     1 on success, 0 on buffer overflow (safe, dest is zero-length string)
*/
int fgac_str_cat3 (char *dest, const char *s1, const char *s2, const char *s3, size_t size);


/**
* @brief      initialize empty fgac_prms array
* @return     empty fgac_prms array
*/
fgac_prms fgac_prms_init (void);

/**
* @brief         add element to the end of fgac_prms array
* @param[in,out] prms array to be altered
* @param[in]     prm  record to add
* @warning       in case of error (out of memory) prms sets to error-state array
*/
void fgac_prms_add (fgac_prms *prms, const fgac_prm *prm);

/**
* @brief         remove element from fgac_prms array
* @param[in,out] prms array to be altered
* @param[in]     i    index of record to remove
* @warning       in case of error (out of memory, index out of bounds) prms sets to error-state array
*/
void fgac_prms_remove (fgac_prms *prms, size_t i);

/**
* @brief         add element to the end of fgac_prms array
* @param[in,out] prms array to be altered
* @param[in]     prm  record to add
* @warning       in case of error (out of memory) prms sets to error-state array
* @warning       if the record of the same type (cat and prmid) already exists,
*                record no added, allow and deny of existing record are joined using XOR
*/
void fgac_prms_push (fgac_prms *prms, const fgac_prm *prm);

/**
* @brief         add all elements from source array to the end of prms array
* @param[in,out] prms   array to be altered
* @param[in]     source array of rerecords to be added
* @warning       in case of error (out of memory) prms sets to error-state array
* @warning       in case of records of duplicate cat and prmid,
*                one record with allow and deny are joined using XOR occurs
*/
void fgac_prms_join (fgac_prms *prms, const fgac_prms *source);

/**
* @brief      free and set to empty prm array
*/
void fgac_prms_free (fgac_prms *prms);

/**
* @brief      create prms array in error state
* @return     prms array in error state
*/
fgac_prms fgac_prms_error (void);

/**
* @brief      check if prms array is in error state
* @return     1 if prms array is in error state or 0 otherwise
*/
#define fgac_prms_is_error(prms) (((prms)->length == ~(size_t)0))

/**
* @brief     get element of non-constant prms array (safe)
* @param[in] prms array of elements
* @param[in] i    index of elements
* @return    pointer to element or NULL if element doesn't exists
*/
fgac_prm * fgac_prms_element (fgac_prms *prms, size_t i);

/**
* @brief     get element of constant prms array (safe)
* @param[in] prms array of elements
* @param[in] i    index of elements
* @return    pointer to element or NULL if element doesn't exists
*/
const fgac_prm * fgac_prms_get (const fgac_prms *prms, size_t i);

/**
* @brief     get size of prms array
* @param[in] prms array of elements
* @return    number of elements
*/
size_t fgac_prms_size (const fgac_prms *prms);

//}

/** cashed boolean **/
typedef enum
{
    FALSE = 0, /**< known to be false         */
    TRUE  = 1, /**< known to be true          */
    UNSET = -1 /**< unknown, to be calculated */
} fgac_cbool;

struct cache_entry;

struct fgac_path
{
    char *path;
    char hostpath[FGAC_LIMIT_PATH];

    fgac_cbool is_dir;

    fgac_cbool exists;

    int inhset;
    uint64_t inh;

    fgac_cbool statset;
    struct stat statbuf;

    int uidset;
    uid_t uid;

    int gidset;
    gid_t gid;
    
    struct cache_entry *cache;
};


#endif // FGACFS_H_INCLUDED
