/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#include <config.h>
#include "fgacfsctl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>

extern char mountpoint[];

void putfifo (fgac_state *state, const char *path)
{
    struct stat statbuf;
    write(state->fd_fifo, path, strlen(path)+1);
//    printf("%p %s %s (%i)\n", mountpoint, mountpoint, path, (int) rc);
    if (*mountpoint) lstat(mountpoint, &statbuf);
}


// nopar
int fgacfsctl_type (fgac_state *state, fgac_prc *prc, int argc, char *argv[])
{
    const char *type;
    (void) argv;
    if (argc > 0) return FGACFSCTL_MSG_USAGE;

    if (prc->uid != 0 && (prc->uid != state->uid || prc->gid != state->gid)) return FGACFSCTL_ERR_PRM;

    switch (state->type)
    {
        case FGAC_SQLITE: type = "db";     break;
        case FGAC_XATTR:  type = "xattr";  break;
        case FGAC_FXATTR: type = "fxattr"; break;
        default: return FGAC_ERR_TYPE;
    }

    printf ("%s", type);

    return FGAC_OK;

}

int fgacfsctl_str_to_int (const char *str, unsigned long *value)
{
    char *end;
    if (!str || !*str) return 0;
    *value = strtoul (str, &end, 10);
    return *end ? 0 : 1;
}

int fgacfsctl_mk_path (const char *source, char *buffer, fgac_path *path)
{
    size_t l;
    if (source[0] == '/')
        { if (!fgac_str_cpy (buffer, source, FGAC_LIMIT_PATH))       return FGAC_ERR_PATH; }
    else
        { if (!fgac_str_cat2 (buffer, "/", source, FGAC_LIMIT_PATH)) return FGAC_ERR_PATH; }
    l = strlen(buffer);
    if (l != 1 && buffer[l-1] == '/') buffer[l-1] = '\0';

    *path = fgac_path_init (buffer);
    return FGAC_OK;
}

// entry
int fgacfsctl_get_uid (fgac_state *state, fgac_prc *prc, int argc, char *argv[])
{
    uid_t uid;
    fgac_path path;
    int rc;
    unsigned long v;
    char buffer [FGAC_LIMIT_PATH];

    if (argc != 1) return FGACFSCTL_MSG_USAGE;

    if ((rc = fgacfsctl_mk_path(argv[0], buffer, &path))) return rc;

    if (!fgac_check_prm2(state, &path, prc, FGAC_PRM_FRA, FGAC_PRM_DRA)) return FGACFSCTL_ERR_PRM;

    if ((rc = fgac_get_owner(state, &path, &uid))) return rc;

    v = uid;
    printf ("%lu", v);

    return FGAC_OK;
}

// entry
int fgacfsctl_get_gid (fgac_state *state, fgac_prc *prc, int argc, char *argv[])
{
    gid_t gid;
    fgac_path path;
    int rc;
    unsigned long v;
    char buffer [FGAC_LIMIT_PATH];

    if (argc != 1) return FGACFSCTL_MSG_USAGE;

    if ((rc = fgacfsctl_mk_path(argv[0], buffer, &path))) return rc;

    if (!fgac_check_prm2(state, &path, prc, FGAC_PRM_FRA, FGAC_PRM_DRA)) return FGACFSCTL_ERR_PRM;

    if ((rc = fgac_get_group(state, &path, &gid))) return rc;

    v = gid;
    printf ("%lu", v);

    return FGAC_OK;
}

// flag entry
int fgacfsctl_get_inh (fgac_state *state, fgac_prc *prc, int argc, char *argv[])
{
    uint64_t inh;
    fgac_path path;
    int rc;
    char buffer [FGAC_LIMIT_PATH];

    if (argc != 2) return FGACFSCTL_MSG_USAGE;

    if ((rc = fgacfsctl_mk_path(argv[1], buffer, &path))) return rc;

    if (!fgac_check_prm2(state, &path, prc, FGAC_PRM_FRP, FGAC_PRM_DRP)) return FGACFSCTL_ERR_PRM;

    if ((rc = fgac_get_inh (state, &path, &inh))) return rc;

    if      (!strcmp (argv[0], "INH")) inh &= FGAC_INH_INH;
    else if (!strcmp (argv[0], "SET")) inh &= FGAC_INH_SET;
    else if (!strcmp (argv[0], "IFP")) inh &= FGAC_INH_IFP;
    else if (!strcmp (argv[0], "IFS")) inh &= FGAC_INH_IFS;
    else if (!strcmp (argv[0], "SPI")) inh &= FGAC_INH_SPI;
    else if (!strcmp (argv[0], "SPS")) inh &= FGAC_INH_SPS;
    else if (!strcmp (argv[0], "CPR")) inh &= FGAC_INH_CPR;
    else if (!strcmp (argv[0], "SFP")) inh &= FGAC_INH_SFP;
    else if (!strcmp (argv[0], "SFS")) inh &= FGAC_INH_SFS;
    else if (!strcmp (argv[0], "CFP")) inh &= FGAC_INH_CFP;
    else if (!strcmp (argv[0], "SCP")) inh &= FGAC_INH_SCP;
    else if (!strcmp (argv[0], "SCF")) inh &= FGAC_INH_SCF;
    else    return FGACFSCTL_ERR_INH;

    printf (inh ? "1" : "0");
    return FGAC_OK;

}

int fgacfsctl_get_cat (const char *source, fgac_cat *cat)
{
    if      (!strcmp (source, "ALL")) *cat = FGAC_CAT_ALL;
    else if (!strcmp (source, "OUS")) *cat = FGAC_CAT_OUS;
    else if (!strcmp (source, "OGR")) *cat = FGAC_CAT_OGR;
    else if (!strcmp (source, "OTH")) *cat = FGAC_CAT_OTH;
    else if (!strcmp (source, "UID")) *cat = FGAC_CAT_UID;
    else if (!strcmp (source, "GID")) *cat = FGAC_CAT_GID;
    else if (!strcmp (source, "PEX")) *cat = FGAC_CAT_PEX;
    else    return FGACFSCTL_ERR_CAT;
    return FGAC_OK;

}

#define IFPRMGET(arg) if (!strcmp (source, #arg)) *prm = FGAC_PRM_##arg;
#define EFPRMGET(arg) else IFPRMGET(arg)
int fgacfsctl_get_prmf (const char *source, uint64_t *prm)
{
    IFPRMGET(FRD)
    EFPRMGET(FRA)
    EFPRMGET(FRP)
    EFPRMGET(FXA)
    EFPRMGET(FSL)
    EFPRMGET(FWR)
    EFPRMGET(FCA)
    EFPRMGET(FCP)
    EFPRMGET(FCI)
    EFPRMGET(FCO)
    EFPRMGET(FCG)
    EFPRMGET(FOP)
    EFPRMGET(FRM)
    EFPRMGET(FMV)
    EFPRMGET(FSX)
    EFPRMGET(FEX)
    EFPRMGET(DRD)
    EFPRMGET(DRA)
    EFPRMGET(DRP)
    EFPRMGET(DXA)
    EFPRMGET(DAF)
    EFPRMGET(DAD)
    EFPRMGET(DAL)
    EFPRMGET(DMK)
    EFPRMGET(DMD)
    EFPRMGET(DSL)
    EFPRMGET(DCA)
    EFPRMGET(DCP)
    EFPRMGET(DCI)
    EFPRMGET(DCT)
    EFPRMGET(DCO)
    EFPRMGET(DCG)
    EFPRMGET(DOP)
    EFPRMGET(DRM)
    EFPRMGET(DMX)
    EFPRMGET(DEX)
    EFPRMGET(DCH)
    EFPRMGET(DBL)
    EFPRMGET(DSC)
    EFPRMGET(DFF)
    EFPRMGET(DAC)
    EFPRMGET(DAB)
    EFPRMGET(DAO)
    EFPRMGET(DAP)
    else    return FGACFSCTL_ERR_PFL;
    return FGAC_OK;

}

// cat [id] flag {A|D} entry
int fgacfsctl_get_prm (fgac_state *state, fgac_prc *prc, int argc, char *argv[])
{
    fgac_path path;
    int rc;
    long unsigned v;
    char buffer [FGAC_LIMIT_PATH],
         *flag, *pad, *entry;
    int ad;
    uint64_t perm;
    fgac_prm prm;

    if (argc < 1) return FGACFSCTL_MSG_USAGE;

    if ((rc = fgacfsctl_get_cat (argv[0], &prm.cat))) return rc;


    if (prm.cat == FGAC_CAT_UID || prm.cat == FGAC_CAT_GID || prm.cat == FGAC_CAT_PEX)
    {
        if (argc != 5) return FGACFSCTL_MSG_USAGE;
        flag  = argv[2];
        pad   = argv[3];
        entry = argv[4];

        if (prm.cat == FGAC_CAT_UID || prm.cat == FGAC_CAT_GID)
        {
            if (!fgacfsctl_str_to_int(argv[1], &v)) return FGACFSCTL_ERR_NUMBER;
            if (prm.cat == FGAC_CAT_UID) prm.prc.uid = v; else prm.prc.gid = v;
        }
        else
            prm.prc.cmd = argv[1];
    }
    else
    {
        if (argc != 4) return FGACFSCTL_MSG_USAGE;
        flag  = argv[1];
        pad   = argv[2];
        entry = argv[3];

    }

    if ((rc = fgacfsctl_mk_path(entry, buffer, &path))) return rc;
    if (!fgac_check_prm2(state, &path, prc, FGAC_PRM_FRP, FGAC_PRM_DRP)) return FGACFSCTL_ERR_PRM;

    if      (!strcmp (pad, "A")     ||
             !strcmp (pad, "a")     ||
             !strcmp (pad, "allow") ||
             !strcmp (pad, "Allow") ||
             !strcmp (pad, "ALLOW")
            ) ad = 0;
    else if (!strcmp (pad, "D")     ||
             !strcmp (pad, "d")     ||
             !strcmp (pad, "deny")  ||
             !strcmp (pad, "Deny")  ||
             !strcmp (pad, "DENY")
            ) ad = 1;
    else return FGACFSCTL_ERR_AD;

    if ((rc = fgacfsctl_get_prmf (flag, &perm))) return rc;

    prm.allow = prm.deny = 0;

    if ((rc = fgac_get_prm(state, &path, &prm))) return rc;

    printf ((ad ? prm.deny & perm : prm.allow & perm) ? "1" : "0");

    return FGAC_OK;


}

// cat [id] flag {A|D} [val] entry
int fgacfsctl_set_prm (fgac_state *state, fgac_prc *prc, int argc, char *argv[])
{
    fgac_path path;
    int rc;
    long unsigned v;
    char buffer [FGAC_LIMIT_PATH],
         *flag, *pad, *entry, *pval;
    int ad, val;
    uint64_t perm;
    fgac_prm prm;

    if (argc < 1) return FGACFSCTL_MSG_USAGE;

    if ((rc = fgacfsctl_get_cat (argv[0], &prm.cat))) return rc;


    if (prm.cat == FGAC_CAT_UID || prm.cat == FGAC_CAT_GID || prm.cat == FGAC_CAT_PEX)
    {
        if (argc == 5)
        {
            pval = NULL;
            entry = argv[4];
        }
        else if (argc == 6)
        {
            pval  = argv[4];
            entry = argv[5];
        }
        else return FGACFSCTL_MSG_USAGE;

        flag  = argv[2];
        pad   = argv[3];

        if (prm.cat == FGAC_CAT_UID || prm.cat == FGAC_CAT_GID)
        {
            if (!fgacfsctl_str_to_int(argv[1], &v)) return FGACFSCTL_ERR_NUMBER;
            if (prm.cat == FGAC_CAT_UID) prm.prc.uid = v; else prm.prc.gid = v;
        }
        else
            prm.prc.cmd = argv[1];
    }
    else
    {
        if (argc == 4)
        {
            pval = NULL;
            entry = argv[3];
        }
        else if (argc == 5)
        {
            pval  = argv[3];
            entry = argv[4];
        }
        else return FGACFSCTL_MSG_USAGE;
        flag  = argv[1];
        pad   = argv[2];
    }

    if      (!pval)               val = 1;
    else if (!strcmp (pval, "1")) val = 1;
    else if (!strcmp (pval, "0")) val = 0;
    else return FGACFSCTL_ERR_SET;

    if ((rc = fgacfsctl_mk_path(entry, buffer, &path))) return rc;
    if (!fgac_check_prm2(state, &path, prc, FGAC_PRM_FCP, FGAC_PRM_DCP)) return FGACFSCTL_ERR_PRM;

    if      (!strcmp (pad, "A")     ||
             !strcmp (pad, "a")     ||
             !strcmp (pad, "allow") ||
             !strcmp (pad, "Allow") ||
             !strcmp (pad, "ALLOW")
            ) ad = 0;
    else if (!strcmp (pad, "D")     ||
             !strcmp (pad, "d")     ||
             !strcmp (pad, "deny")  ||
             !strcmp (pad, "Deny")  ||
             !strcmp (pad, "DENY")
            ) ad = 1;
    else return FGACFSCTL_ERR_AD;

    if ((rc = fgacfsctl_get_prmf (flag, &perm))) return rc;

    prm.allow = prm.deny = 0;

    if ((rc = fgac_get_prm(state, &path, &prm))) return rc;

    if (!fgac_is_dir(state, &path) && (perm & FGAC_PRM_DIRS)) return FGACFSCTL_ERR_DFP;

    if (ad)
        if (val) prm.deny  |= perm; else prm.deny  &= ~perm;
    else
        if (val) prm.allow |= perm; else prm.allow &= ~perm;

//    printf ((ad ? prm.deny & perm : prm.allow & perm) ? "1" : "0");

    putfifo(state, path.path);

    return fgac_set_prm(state, &path, &prm);

//    return FGAC_OK;

}

// flag [val] entry
int fgacfsctl_set_inh (fgac_state *state, fgac_prc *prc, int argc, char *argv[])
{
    uint64_t cinh, inh;
    fgac_path path;
    int rc;
    char buffer [FGAC_LIMIT_PATH];
    int val;

    if      (argc == 2)
    {
        val = 1;
        if ((rc = fgacfsctl_mk_path(argv[1], buffer, &path))) return rc;
    }
    else if (argc == 3)
    {
        if      (!strcmp(argv[1], "1")) val = 1;
        else if (!strcmp(argv[1], "0")) val = 0;
        else return FGACFSCTL_ERR_SET;
        if ((rc = fgacfsctl_mk_path(argv[2], buffer, &path))) return rc;
    }
    else return FGACFSCTL_MSG_USAGE;

    if ((rc = fgac_get_inh (state, &path, &cinh))) return rc;

    if      (!strcmp (argv[0], "INH")) inh = FGAC_INH_INH;
    else if (!strcmp (argv[0], "SET")) inh = FGAC_INH_SET;
    else if (!strcmp (argv[0], "IFP")) inh = FGAC_INH_IFP;
    else if (!strcmp (argv[0], "IFS")) inh = FGAC_INH_IFS;
    else if (!strcmp (argv[0], "SPI")) inh = FGAC_INH_SPI;
    else if (!strcmp (argv[0], "SPS")) inh = FGAC_INH_SPS;
    else if (!strcmp (argv[0], "CPR")) inh = FGAC_INH_CPR;
    else if (!strcmp (argv[0], "SFP")) inh = FGAC_INH_SFP;
    else if (!strcmp (argv[0], "SFS")) inh = FGAC_INH_SFS;
    else if (!strcmp (argv[0], "CFP")) inh = FGAC_INH_CFP;
    else if (!strcmp (argv[0], "SCP")) inh = FGAC_INH_SCP;
    else if (!strcmp (argv[0], "SCF")) inh = FGAC_INH_SCF;
    else    return FGACFSCTL_ERR_INH;

    if (!fgac_check_prm2(state, &path, prc, FGAC_PRM_FCI, inh & FGAC_INH_TRMS ? FGAC_PRM_DCT : FGAC_PRM_DCI)) return FGACFSCTL_ERR_PRM;

    if (!fgac_is_dir (state, &path) && (inh & FGAC_INH_DIRS)) return FGACFSCTL_ERR_DFI;

    if      (!val && inh == FGAC_INH_INH && (cinh & FGAC_INH_INH)) return fgac_unset_inh (state, &path);
    else if (!val && inh == FGAC_INH_IFP && (cinh & FGAC_INH_IFP)) return fgac_unset_ifp (state, &path);
    else 
    {
        putfifo(state, path.path);
        return fgac_set_inh(state, &path, val ? cinh | inh : cinh & ~inh);
    }

}

// uid entry
int fgacfsctl_set_uid (fgac_state *state, fgac_prc *prc, int argc, char *argv[])
{
    uid_t uid;
    fgac_path path;
    int rc;
    char buffer [FGAC_LIMIT_PATH];
    unsigned long v;

    if (argc != 2) return FGACFSCTL_MSG_USAGE;
    if (!fgacfsctl_str_to_int(argv[0], &v)) return FGACFSCTL_ERR_NUMBER;
    uid = v;

    if ((rc = fgacfsctl_mk_path(argv[1], buffer, &path))) return rc;

    if (!fgac_check_prm2(state, &path, prc, FGAC_PRM_FCO, FGAC_PRM_DCO))
    {
        char pbuffer [FGAC_LIMIT_PATH];
        uid_t puid;
        fgac_path parent = fgac_path_init(pbuffer);

        if (!fgac_parent(&path, &parent)) return FGACFSCTL_ERR_PRM;

        if ((rc == fgac_get_owner(state, &parent, &puid))) return rc;
        if (puid != uid || !fgac_check_prm2(state, &path, prc, FGAC_PRM_FOP, FGAC_PRM_DOP)) return FGACFSCTL_ERR_PRM;
    }

    putfifo(state, path.path);

    if ((rc = fgac_set_owner(state, &path, uid))) return rc;

    return FGAC_OK;
}

// gid entry
int fgacfsctl_set_gid (fgac_state *state, fgac_prc *prc, int argc, char *argv[])
{
    gid_t gid;
    fgac_path path;
    int rc;
    char buffer [FGAC_LIMIT_PATH];
    unsigned long v;

    if (argc != 2) return FGACFSCTL_MSG_USAGE;
    if (!fgacfsctl_str_to_int(argv[0], &v)) return FGACFSCTL_ERR_NUMBER;
    gid = v;

    if ((rc = fgacfsctl_mk_path(argv[1], buffer, &path))) return rc;

    if (!fgac_check_prm2(state, &path, prc, FGAC_PRM_FCG, FGAC_PRM_DCG))
    {
        char pbuffer [FGAC_LIMIT_PATH];
        gid_t pgid;
        fgac_path parent = fgac_path_init (pbuffer);

        if (!fgac_parent(&path, &parent)) return FGACFSCTL_ERR_PRM;

        if ((rc == fgac_get_group (state, &parent, &pgid))) return rc;
        if (pgid != gid || !fgac_check_prm2(state, &path, prc, FGAC_PRM_FOP, FGAC_PRM_DOP)) return FGACFSCTL_ERR_PRM;
    }

    putfifo(state, path.path);

    if ((rc = fgac_set_group(state, &path, gid))) return rc;

    return FGAC_OK;
}

// uid:gid[:cmd] flag entry
int fgacfsctl_test (fgac_state *state, fgac_prc *prc, int argc, char *argv[])
{
    char *puid, *pgid, *pcmd;
    long unsigned iuid, igid;
    fgac_prc prc2;
    struct passwd *pwd;
    uint64_t prm;
    int rc;
    fgac_path path;
    char buffer [FGAC_LIMIT_PATH];

    if (prc->uid != 0 && (prc->uid != state->uid || prc->gid != state->gid)) return FGACFSCTL_ERR_PRM;

    if (argc != 3) return FGACFSCTL_MSG_USAGE;

    puid = argv[0];

    pgid = puid;
    while (*pgid && *pgid != ':') pgid++;
    if (!*pgid) return FGACFSCTL_MSG_USAGE;
    *pgid = '\0';
    ++pgid;

    pcmd = pgid;
    while (*pcmd && *pcmd != ':') pcmd++;
    if (!*pcmd)
        pcmd = NULL;
    else
    {
        *pcmd = '\0';
        ++pcmd;
    }


    if (!fgacfsctl_str_to_int(puid, &iuid)) return FGACFSCTL_ERR_NUMBER;
    if (!fgacfsctl_str_to_int(pgid, &igid)) return FGACFSCTL_ERR_NUMBER;

    prc2.uid = iuid;
    prc2.gid = igid;
    prc2.cmd = pcmd;

    pwd = getpwuid(prc2.uid);

    prc2.ngroups = FGAC_LIMIT_GROUPS;
    prc2.ngroups = getgrouplist (pwd->pw_name, prc2.gid, prc2.groups, &prc2.ngroups);

    if (prc2.ngroups == -1) return FGACFSCTL_ERR_GROUPS;

    if ((rc = fgacfsctl_get_prmf (argv[1], &prm))) return rc;

    if ((rc = fgacfsctl_mk_path(argv[2], buffer, &path))) return rc;

    printf (fgac_check_prm(state, &path, &prc2, prm) ? "1" : "0");

    return FGAC_OK;

}

#define SHOW_INH(INH) printf(inh & FGAC_INH_##INH ? #INH " " : "    ");

void fgacfsctl_show_inhs (uint64_t inh, int isdir)
{
    printf("INHERITANCE:\t");

    SHOW_INH(INH)
    SHOW_INH(SET)

    if (isdir)
    {
        SHOW_INH(IFP)
        SHOW_INH(IFS)
        printf ("\tTRANSMISSION:\t");

        SHOW_INH(SPI)
        SHOW_INH(SPS)
        SHOW_INH(CPR)
        SHOW_INH(SCP)
        SHOW_INH(SFP)
        SHOW_INH(SFS)
        SHOW_INH(CFP)
        SHOW_INH(SCF)
    }
    printf ("\n");
}

#define SHOW_PRM(SRC,PRM) printf(SRC & FGAC_PRM_##PRM ? #PRM " " : "    ");
#define SHOW_FIL_PRMS(SRC) \
        printf ("R: ");    \
        SHOW_PRM(SRC,FRD)  \
        SHOW_PRM(SRC,FRA)  \
        SHOW_PRM(SRC,FRP)  \
        SHOW_PRM(SRC,FXA)  \
        SHOW_PRM(SRC,FSL)  \
        printf ("W: ");    \
        SHOW_PRM(SRC,FWR)  \
        SHOW_PRM(SRC,FCA)  \
        SHOW_PRM(SRC,FCP)  \
        SHOW_PRM(SRC,FCI)  \
        SHOW_PRM(SRC,FCO)  \
        SHOW_PRM(SRC,FCG)  \
        SHOW_PRM(SRC,FOP)  \
        SHOW_PRM(SRC,FRM)  \
        SHOW_PRM(SRC,FMV)  \
        SHOW_PRM(SRC,FMX)  \
        SHOW_PRM(SRC,FSX)  \
        printf ("X: ");    \
        SHOW_PRM(SRC,FEX)

#define SHOW_DIR_PRMS(SRC) \
        printf ("R: ");    \
        SHOW_PRM(SRC,DRD)  \
        SHOW_PRM(SRC,DRA)  \
        SHOW_PRM(SRC,DRP)  \
        SHOW_PRM(SRC,DXA)  \
        printf ("W: ");    \
        SHOW_PRM(SRC,DAF)  \
        SHOW_PRM(SRC,DAD)  \
        SHOW_PRM(SRC,DAL)  \
        SHOW_PRM(SRC,DAC)  \
        SHOW_PRM(SRC,DAB)  \
        SHOW_PRM(SRC,DAO)  \
        SHOW_PRM(SRC,DAP)  \
        SHOW_PRM(SRC,DMK)  \
        SHOW_PRM(SRC,DMD)  \
        SHOW_PRM(SRC,DSL)  \
        SHOW_PRM(SRC,DCH)  \
        SHOW_PRM(SRC,DBL)  \
        SHOW_PRM(SRC,DSC)  \
        SHOW_PRM(SRC,DFF)  \
        SHOW_PRM(SRC,DCA)  \
        SHOW_PRM(SRC,DCP)  \
        SHOW_PRM(SRC,DCI)  \
        SHOW_PRM(SRC,DCT)  \
        SHOW_PRM(SRC,DCO)  \
        SHOW_PRM(SRC,DCG)  \
        SHOW_PRM(SRC,DOP)  \
        SHOW_PRM(SRC,DRM)  \
        SHOW_PRM(SRC,DMV)  \
        SHOW_PRM(SRC,DMX)  \
        printf ("X: ");    \
        SHOW_PRM(SRC,DEX)

void fgacfsctl_show_prm (const fgac_prm *prm, int isdir)
{
    if (!prm->allow && !prm->deny) return;
    switch (prm->cat)
    {
        case FGAC_CAT_ALL: printf ("ALL");                    break;
        case FGAC_CAT_OUS: printf ("OUS (%i)", prm->prc.uid); break;
        case FGAC_CAT_OGR: printf ("OGR (%i)", prm->prc.gid); break;
        case FGAC_CAT_OTH: printf ("OTH");                    break;
        case FGAC_CAT_UID: printf ("UID (%i)", prm->prc.uid); break;
        case FGAC_CAT_GID: printf ("GID (%i)", prm->prc.gid); break;
        case FGAC_CAT_PEX: printf ("PEX (%s)", prm->prc.cmd); break;
    }

    if (isdir)
    {
        printf("\n\tDIR ALLOW:\t");  SHOW_DIR_PRMS(prm->allow);
        printf("\n\tDIR DENY:\t");   SHOW_DIR_PRMS(prm->deny);
        printf("\n\tFILE ALLOW:\t"); SHOW_FIL_PRMS(prm->allow);
        printf("\n\tFILE DENY:\t");  SHOW_FIL_PRMS(prm->deny);
        printf("\n");
    }
    else
    {
        printf("\n\tALLOW:\t"); SHOW_FIL_PRMS(prm->allow);
        printf("\n\tDENY:\t");  SHOW_FIL_PRMS(prm->deny);
        printf("\n");
    }

}

int fgacfsctl_show_prms (fgac_prms prms, fgac_prc *prc, int isdir)
{
    fgac_prms uid = fgac_prms_init(), gid = fgac_prms_init(), pex = fgac_prms_init();
    fgac_prm all, ous, ogr, oth; const fgac_prm *prm;
    all.allow = all.deny = ous.allow = ous.deny = ogr.allow = ogr.deny = oth.allow = oth.deny = 0;
    all.cat = FGAC_CAT_ALL;
    ous.cat = FGAC_CAT_OUS;
    ogr.cat = FGAC_CAT_OGR;
    oth.cat = FGAC_CAT_OTH;

    ous.prc.uid = prc->uid;
    ogr.prc.gid = prc->gid;

    size_t i, j, s, l;

    for (i = 0, s = fgac_prms_size(&prms); i < s; ++i)
    {
        prm = fgac_prms_get(&prms, i);
        switch (prm->cat)
        {
            case FGAC_CAT_ALL: all.allow |= prm->allow; all.deny |= prm->deny; break;
            case FGAC_CAT_OUS: ous.allow |= prm->allow; ous.deny |= prm->deny; break;
            case FGAC_CAT_OGR: ogr.allow |= prm->allow; ogr.deny |= prm->deny; break;
            case FGAC_CAT_OTH: oth.allow |= prm->allow; oth.deny |= prm->deny; break;
            case FGAC_CAT_UID:
                fgac_prms_add(&uid, prm);
                if (fgac_prms_is_error(&uid)) return FGAC_ERR_MALLOC;
                l = fgac_prms_size(&uid);
                if (l > 2)
                {
                    for (j = l - 1; j > 0 && fgac_prms_get(&uid, j - 1)->prc.uid > prm->prc.uid; --j)
                        *fgac_prms_element(&uid, j) = *fgac_prms_get(&uid, j - 1);
                    *fgac_prms_element(&uid, j) = *prm;
                }
                break;
            case FGAC_CAT_GID:
                fgac_prms_add(&gid, prm);
                if (fgac_prms_is_error(&gid)) return FGAC_ERR_MALLOC;
                l = fgac_prms_size(&gid);
                if (l > 2)
                {
                    for (j = l - 1; j > 0 && fgac_prms_get(&gid, j - 1)->prc.gid > prm->prc.gid; --j)
                        *fgac_prms_element(&gid, j) = *fgac_prms_get(&gid, j - 1);
                    *fgac_prms_element(&gid, j) = *prm;
                }
                break;
            case FGAC_CAT_PEX:
                fgac_prms_add(&pex, prm);
                if (fgac_prms_is_error(&pex)) return FGAC_ERR_MALLOC;
                l = fgac_prms_size(&pex);
                if (l > 2)
                {
                    for (j = l - 1; j > 0 && strcmp (fgac_prms_get(&pex, j - 1)->prc.cmd, prm->prc.cmd) > 0; --j)
                        *fgac_prms_element(&pex, j) = *fgac_prms_get(&pex, j - 1);
                    *fgac_prms_element(&pex, j) = *prm;
                }
                break;
        }
    }

    fgacfsctl_show_prm (&all, isdir);
    fgacfsctl_show_prm (&ous, isdir);
    for (i = 0, s = fgac_prms_size(&uid); i < s; ++i) fgacfsctl_show_prm (fgac_prms_get(&uid, i), isdir);

    fgacfsctl_show_prm (&ogr, isdir);
    for (i = 0, s = fgac_prms_size(&gid); i < s; ++i) fgacfsctl_show_prm (fgac_prms_get(&gid, i), isdir);

    for (i = 0, s = fgac_prms_size(&pex); i < s; ++i) fgacfsctl_show_prm (fgac_prms_get(&pex, i), isdir);
    fgacfsctl_show_prm (&oth, isdir);

    return FGAC_OK;
}

// entry
int fgacfsctl_show (fgac_state *state, fgac_prc *prc, int argc, char *argv[])
{
    fgac_path path;
    int rc, isdir;
    char buffer [FGAC_LIMIT_PATH];
    fgac_prms oprms, iprms, jprms = fgac_prms_init();
    uint64_t inh;
    uid_t uid; gid_t gid;

    if (argc != 1) return FGACFSCTL_MSG_USAGE;
    if ((rc = fgacfsctl_mk_path(argv[0], buffer, &path))) return rc;

    if (!fgac_check_prm2 (state, &path, prc, FGAC_PRM_FRP, FGAC_PRM_DRP)) return FGACFSCTL_ERR_PRM;

    oprms = fgac_get_prms   (state, &path);
    iprms = fgac_get_inhprms(state, &path);

    fgac_prms_join(&jprms, &oprms);
    fgac_prms_join(&jprms, &iprms);

    if (fgac_prms_is_error(&oprms) || fgac_prms_is_error(&iprms) || fgac_prms_is_error(&jprms)) return FGACFSCTL_ERR_PRMGET;

    if ((rc = fgac_get_inh   (state, &path, &inh))) return rc;
    if ((rc = fgac_get_owner (state, &path, &uid))) return rc;
    if ((rc = fgac_get_group (state, &path, &gid))) return rc;

    isdir = fgac_is_dir(state, &path);

    printf ("FGACFS \"%s\" %s OWNER = %i GROUP = %i\n", path.path, isdir ? "DIR" : "FILE", uid, gid);

    fgacfsctl_show_inhs (inh, isdir);

    printf ("\nJoined permissions:\n");
    if ((rc = fgacfsctl_show_prms (jprms, prc, isdir))) return rc;

    printf ("\nOwn permissions:\n");
    if ((rc = fgacfsctl_show_prms (oprms, prc, isdir))) return rc;

    printf ("\nInherited permissions:\n");
    if ((rc = fgacfsctl_show_prms (iprms, prc, isdir))) return rc;

    return FGAC_OK;

}
