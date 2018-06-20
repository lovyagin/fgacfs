/*
  Copyright (C) 2016      Roman Y. Dayneko, <dayneko3000@gmail.com>
                2017-2018 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#include "fgacfs.h"
#include "db.h"
#include <sqlite3.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int db_open (fgac_state *state)
{
    char dbfile[FGAC_LIMIT_PATH];
    struct stat statbuf;
    if (!fgac_str_cat2 (dbfile, state->hostdir, "/fgac.db", FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;

    if (!lstat (dbfile, &statbuf))
    {
        if (sqlite3_open(dbfile, &(state->db)) != SQLITE_OK ||
            sqlite3_exec(state->db, "PRAGMA foreign_keys = ON", NULL, NULL, NULL) != SQLITE_OK
           ) return FGAC_ERR_DBOPEN;
        state->type = FGAC_SQLITE;
    }
    else
        state->db = NULL;
    return FGAC_OK;
}

int db_close (fgac_state *state)
{
    if (state->db)
    {
        sqlite3_close(state->db);
        state->db = NULL;
    }
    return FGAC_OK;
}

int db_create (const char *hostdir)
{
    char dbfile[FGAC_LIMIT_PATH];

    if (!fgac_str_cat2 (dbfile, hostdir, "/fgac.db", FGAC_LIMIT_PATH)) return FGAC_ERR_PATH;
    sqlite3 *db;
    if (sqlite3_open(dbfile, &db) != SQLITE_OK) return FGAC_ERR_DBOPEN;

    if (sqlite3_exec(db,
                     "PRAGMA foreign_keys = ON; "
                     "CREATE TABLE entry "
                     "("
                        "id   INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "path TEXT, "
                        "uid  INTEGER, "
                        "gid  INTEGER, "
                        "inh  INTEGER "
                     "); "
                     "CREATE TABLE prm "
                     "("
                        "id    INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "entry INTEGER, "
                        "type  INTEGER, "
                        "prmid INTEGER, "
                        "allow INTEGER, "
                        "deny  INTEGER, "
                        "FOREIGN KEY (entry) references entry(id) ON DELETE CASCADE"
                     "); "
                     "CREATE TABLE exec "
                     "("
                        "id    INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "exec  TEXT "
                     "); "
                     "CREATE UNIQUE INDEX entry_path_index ON entry(path); "
                     "CREATE INDEX prm_entry_index  ON prm(entry); "
                     "CREATE INDEX prm_type_index   ON prm(type); "
                     "CREATE UNIQUE INDEX exec_exec_index  ON exec(exec); ",
                     NULL, NULL, NULL) != SQLITE_OK) return FGAC_ERR_DBCREATE;
    sqlite3_close(db);
    return FGAC_OK;

}


int db_get_owner (fgac_state *state, fgac_path *path, uid_t *uid)
{
    sqlite3_stmt *stmt; int rc;

    sqlite3_prepare_v2(state->db, "SELECT uid FROM entry WHERE path = ?1", -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, FGAC_PATH, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) *uid = sqlite3_column_int64(stmt, 0);

    sqlite3_finalize(stmt);

    return rc != SQLITE_ROW ? FGAC_ERR_DBQUERY : FGAC_OK;
}

int db_get_group (fgac_state *state, fgac_path *path, gid_t *gid)
{
    sqlite3_stmt *stmt; int rc;
    sqlite3_prepare_v2(state->db, "SELECT gid FROM entry WHERE path = ?1", -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, FGAC_PATH, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) *gid = sqlite3_column_int64(stmt, 0);

    sqlite3_finalize(stmt);
    return rc != SQLITE_ROW ? FGAC_ERR_DBQUERY : FGAC_OK;
}


int db_set_group (fgac_state *state, fgac_path *path, gid_t gid)
{
    sqlite3_stmt *stmt; int rc;
    sqlite3_prepare_v2(state->db, "UPDATE entry SET gid = ?1 WHERE path = ?2", -1, &stmt, NULL);

    sqlite3_bind_int64 (stmt, 1, gid);
    sqlite3_bind_text  (stmt, 2, FGAC_PATH, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc != SQLITE_DONE ? FGAC_ERR_DBQUERY : FGAC_OK;
}

int db_set_owner (fgac_state *state, fgac_path *path, uid_t uid)
{
    sqlite3_stmt *stmt; int rc;
    sqlite3_prepare_v2(state->db, "UPDATE entry SET uid = ?1 WHERE path = ?2", -1, &stmt, NULL);

    sqlite3_bind_int64 (stmt, 1, uid);
    sqlite3_bind_text  (stmt, 2, FGAC_PATH, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc != SQLITE_DONE ? FGAC_ERR_DBQUERY : FGAC_OK;
}

int db_get_inh (fgac_state *state, fgac_path *path, uint64_t *inh)
{
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(state->db, "SELECT inh FROM entry WHERE path = ?1", -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, FGAC_PATH, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) *inh = sqlite3_column_int64(stmt, 0);

    sqlite3_finalize(stmt);
    return rc != SQLITE_ROW ? FGAC_ERR_DBQUERY : FGAC_OK;
}


int db_set_inh (fgac_state *state, fgac_path *path, uint64_t inh)
{
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(state->db, "UPDATE entry SET inh = ?1 WHERE path = ?2", -1, &stmt, NULL);

    sqlite3_bind_int64 (stmt, 1, inh);
    sqlite3_bind_text  (stmt, 2, FGAC_PATH, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) return FGAC_ERR_DBQUERY;

    sqlite3_finalize(stmt);
    return FGAC_OK;
}

int db_add (fgac_state *state, fgac_path *path, uid_t uid, gid_t gid)
{
    sqlite3_stmt *stmt; int rc;
    sqlite3_prepare_v2(state->db, "INSERT INTO entry (path, uid, gid, inh) VALUES (?1, ?2, ?3, ?4)", -1, &stmt, NULL);

    sqlite3_bind_text  (stmt, 1, FGAC_PATH, -1, SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, uid);
    sqlite3_bind_int64 (stmt, 3, gid);
    sqlite3_bind_int64 (stmt, 4, 0  );

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc != SQLITE_DONE ? FGAC_ERR_DBQUERY : FGAC_OK;

}

int db_exec_cleanup (fgac_state *state)
{
    sqlite3_stmt *stmt;
    int rc;
    sqlite3_prepare_v2(state->db,
                       "DELETE FROM exec "
                       "WHERE id NOT IN (SELECT prmid FROM prm WHERE type = ?1)",
                       -1, &stmt, NULL
                      );

    sqlite3_bind_int(stmt, 1, FGAC_CAT_PEX);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc != SQLITE_DONE ? FGAC_ERR_DBQUERY : FGAC_OK;
}

int db_delete (fgac_state *state, fgac_path *path)
{
    sqlite3_stmt *stmt; int rc;
    sqlite3_prepare_v2(state->db, "DELETE FROM entry WHERE path = ?1", -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, FGAC_PATH, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    db_exec_cleanup (state);

    return rc != SQLITE_DONE ? FGAC_ERR_DBQUERY : FGAC_OK;

}

int db_rename (fgac_state *state, fgac_path *path, const char* newfgac_path)
{
    sqlite3_stmt *stmt; int rc;
    sqlite3_prepare_v2(state->db, "UPDATE entry SET fgac_path = ?1 WHERE fgac_path = ?2", -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, FGAC_PATH, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, newfgac_path, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) return FGAC_ERR_DBQUERY;

    sqlite3_prepare_v2(state->db, 
                       "UPDATE entry "
                       "SET fgac_path = ?2 + '/' + substr(fgac_path, length(?1)) "
                       "WHERE fgac_path LIKE ?2+'%'", 
                       -1, &stmt, NULL
                      );

    sqlite3_bind_text(stmt, 1, FGAC_PATH, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, newfgac_path, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc != SQLITE_DONE ? FGAC_ERR_DBQUERY : FGAC_OK;
}


int db_get_prm (fgac_state *state, fgac_path *path, fgac_prm *prm)
{

    sqlite3_stmt *stmt;
    uint64_t prmid = 0;
    int rc;

    prm->allow = prm->deny = 0;

    if (prm->cat == FGAC_CAT_PEX)
    {
        sqlite3_prepare_v2(state->db,
                           "SELECT allow, deny FROM prm "
                           "JOIN entry ON entry.id = prm.entry "
                           "JOIN exec  ON  exec.id = prm.prmid "
                           "WHERE entry.path = ?1 AND prm.type = ?2 AND exec.exec = ?3",
                           -1, &stmt, NULL
                          );

        sqlite3_bind_text(stmt, 3, prm->prc.cmd, -1, SQLITE_STATIC);

    }
    else
    {
        if (prm->cat == FGAC_CAT_ALL ||
            prm->cat == FGAC_CAT_OUS ||
            prm->cat == FGAC_CAT_OGR ||
            prm->cat == FGAC_CAT_OTH
           )
        {
            sqlite3_prepare_v2(state->db,
                               "SELECT allow, deny FROM prm "
                               "JOIN entry ON entry.id = prm.entry "
                               "WHERE entry.path = ?1 AND prm.type = ?2 AND prm.prmid = 0",
                               -1, &stmt, NULL
                              );
        }
        else
        {
            if (prm->cat == FGAC_CAT_UID)
                prmid = prm->prc.uid;
            else if (prm->cat == FGAC_CAT_GID)
                prmid = prm->prc.gid;
            else return FGAC_ERR_PRMCAT;

            sqlite3_prepare_v2(state->db,
                               "SELECT allow, deny FROM prm "
                               "JOIN entry ON entry.id = prm.entry "
                               "WHERE entry.path = ?1 AND prm.type = ?2 AND prm.prmid = ?3",
                               -1, &stmt, NULL
                              );
            sqlite3_bind_int64(stmt, 3, prmid);
        }

    }

    sqlite3_bind_text (stmt, 1, FGAC_PATH, -1, SQLITE_STATIC);
    sqlite3_bind_int  (stmt, 2, prm->cat);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        prm->allow = sqlite3_column_int64(stmt, 0);
        prm->deny  = sqlite3_column_int64(stmt, 1);
    }

    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE || rc == SQLITE_ROW ? FGAC_OK : FGAC_ERR_DBQUERY;

}

int db_set_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm)
{
    int rc;
    sqlite3_stmt *stmt;
    uint64_t prmid = 0;

    if (!(prm->allow | prm->deny)) return db_unset_prm(state, path, prm);

    if (prm->cat == FGAC_CAT_PEX)
    {
        sqlite3_prepare_v2(state->db,
                           "SELECT id FROM exec WHERE exec = ?1",
                           -1, &stmt, NULL
                          );
        sqlite3_bind_text(stmt, 1, prm->prc.cmd, -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);

        if (rc == SQLITE_ROW)
           prmid = sqlite3_column_int64(stmt, 0);

        sqlite3_finalize(stmt);

        if (rc == SQLITE_DONE)
        {
            sqlite3_prepare_v2(state->db,
                               "INSERT INTO exec (exec) VALUES (?1)",
                               -1, &stmt, NULL
                              );
            sqlite3_bind_text(stmt, 1, prm->prc.cmd, -1, SQLITE_STATIC);
            rc = sqlite3_step(stmt);
            if (rc == SQLITE_DONE) prmid = sqlite3_last_insert_rowid(state->db);

            sqlite3_finalize(stmt);
            if (rc != SQLITE_DONE) return FGAC_ERR_DBQUERY;
        }
        else
            return FGAC_ERR_DBQUERY;
    }
    else
    {
        if (prm->cat == FGAC_CAT_ALL ||
            prm->cat == FGAC_CAT_OUS ||
            prm->cat == FGAC_CAT_OGR ||
            prm->cat == FGAC_CAT_OTH
           ) prmid = 0;
        else
        {
            if (prm->cat == FGAC_CAT_UID)
                prmid = prm->prc.uid;
            else if (prm->cat == FGAC_CAT_GID)
                prmid = prm->prc.gid;
            else return FGAC_ERR_PRMCAT;
        }
    }

    rc = sqlite3_prepare_v2(state->db,
                       "SELECT allow, deny FROM prm "
                       "JOIN entry ON entry.id = prm.entry "
                       "WHERE entry.path = ?1 AND prm.type = ?2 AND prm.prmid = ?3",
                       -1, &stmt, NULL
                      );
    sqlite3_bind_text (stmt, 1, FGAC_PATH, -1, SQLITE_STATIC);
    sqlite3_bind_int  (stmt, 2, prm->cat);
    sqlite3_bind_int64(stmt, 3, prmid);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_ROW)
    {
        sqlite3_prepare_v2(state->db,
                           "UPDATE prm SET allow = ?4, deny = ?5 "
                           "WHERE entry IN (SELECT id FROM entry WHERE path = ?1) AND "
                                 "type = ?2 AND prmid = ?3",
                           -1, &stmt, NULL
                          );
        sqlite3_bind_text (stmt, 1, FGAC_PATH, -1, SQLITE_STATIC);
        sqlite3_bind_int  (stmt, 2, prm->cat);
        sqlite3_bind_int64(stmt, 3, prmid);
        sqlite3_bind_int64(stmt, 4, prm->allow);
        sqlite3_bind_int64(stmt, 5, prm->deny);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE) return FGAC_ERR_DBQUERY;
    }
    else if (rc == SQLITE_DONE)
    {
        sqlite3_prepare_v2(state->db,
                           "INSERT INTO prm (entry, type, prmid, allow, deny) "
                           "SELECT id, ?2, ?3, ?4, ?5 FROM entry WHERE path = ?1",
                           -1, &stmt, NULL
                          );
        sqlite3_bind_text (stmt, 1, FGAC_PATH, -1, SQLITE_STATIC);
        sqlite3_bind_int  (stmt, 2, prm->cat);
        sqlite3_bind_int64(stmt, 3, prmid);
        sqlite3_bind_int64(stmt, 4, prm->allow);
        sqlite3_bind_int64(stmt, 5, prm->deny);

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE) return FGAC_ERR_DBQUERY;

    }
    else return FGAC_ERR_DBQUERY;



    return FGAC_OK;



}

int db_unset_prm (fgac_state *state, fgac_path *path, const fgac_prm *prm)
{
    sqlite3_stmt *stmt;
    int rc;
    uint64_t prmid = 0;

    if (prm->cat == FGAC_CAT_PEX)
    {
        sqlite3_prepare_v2(state->db,
                           "DELETE FROM prm WHERE prm.id IN "
                           "(SELECT prm.id FROM prm "
                            "JOIN entry ON entry.id = prm.entry "
                            "JOIN exec  ON  exec.id = prm.prmid "
                            "WHERE entry.path = ?1 AND prm.type = ?2 AND exec.exec = ?3"
                           ")",
                           -1, &stmt, NULL
                          );

        sqlite3_bind_text(stmt, 3, prm->prc.cmd, -1, SQLITE_STATIC);

    }
    else
    {
        if (prm->cat == FGAC_CAT_ALL ||
            prm->cat == FGAC_CAT_OUS ||
            prm->cat == FGAC_CAT_OGR ||
            prm->cat == FGAC_CAT_OTH
           )
        {
            sqlite3_prepare_v2(state->db,
                               "DELETE FROM prm WHERE prm.id IN "
                               "(SELECT prm.id FROM prm "
                                "JOIN entry ON entry.id = prm.entry "
                                "WHERE entry.path = ?1 AND prm.type = ?2 AND prm.prmid = 0"
                               ")",
                               -1, &stmt, NULL
                              );
        }
        else
        {
            if (prm->cat == FGAC_CAT_UID)
                prmid = prm->prc.uid;
            else if (prm->cat == FGAC_CAT_GID)
                prmid = prm->prc.gid;
            else return FGAC_ERR_PRMCAT;

            sqlite3_prepare_v2(state->db,
                               "DELETE FROM prm WHERE prm.id IN "
                               "(SELECT prm.id FROM prm "
                                "JOIN entry ON entry.id = prm.entry "
                                "WHERE entry.path = ?1 AND prm.type = ?2 AND prm.prmid = ?3"
                               ")",
                               -1, &stmt, NULL
                              );
            sqlite3_bind_int64(stmt, 3, prmid);
        }

    }

    sqlite3_bind_text (stmt, 1, FGAC_PATH, -1, SQLITE_STATIC);
    sqlite3_bind_int  (stmt, 2, prm->cat);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return FGAC_ERR_DBQUERY;

    if (prm->cat == FGAC_CAT_PEX) db_exec_cleanup(state);

    return FGAC_OK;

}

fgac_prms db_get_prms (fgac_state *state, fgac_path *path)
{
    sqlite3_stmt *stmt;
    fgac_prms r = fgac_prms_init();
    fgac_prm prm;

    sqlite3_prepare_v2(state->db,
                       "SELECT prm.allow, prm.deny, exec.exec FROM prm "
                       "JOIN entry ON entry.id = prm.entry "
                       "JOIN exec  ON  exec.id = prm.prmid "
                       "WHERE entry.path = ?1 AND prm.type = ?2",
                       -1, &stmt, NULL
                      );

    sqlite3_bind_text(stmt, 1, FGAC_PATH, -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, FGAC_CAT_PEX);

    prm.cat = FGAC_CAT_PEX;
    while (sqlite3_step (stmt) == SQLITE_ROW)
    {
        prm.allow   = sqlite3_column_int64 (stmt, 1);
        prm.deny    = sqlite3_column_int64 (stmt, 2);
        prm.prc.cmd = (char *) sqlite3_column_text (stmt, 3);

        fgac_prms_push (&r, &prm);
        
        if (fgac_prms_is_error(&r))
        {
            sqlite3_finalize(stmt);
            return r;
        }
    }

    sqlite3_prepare_v2(state->db,
                       "SELECT prm.allow, prm.deny, prm.type, prm.prmid FROM prm "
                       "JOIN entry ON entry.id = prm.entry "
                       "WHERE entry.path = ?1 AND prm.type != ?2",
                       -1, &stmt, NULL
                      );

    sqlite3_bind_text(stmt, 1, FGAC_PATH, -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, FGAC_CAT_PEX);

    while (sqlite3_step (stmt) == SQLITE_ROW)
    {
        prm.allow = sqlite3_column_int64 (stmt, 0);
        prm.deny  = sqlite3_column_int64 (stmt, 1);
        prm.cat   = sqlite3_column_int64 (stmt, 2);
        if (prm.cat == FGAC_CAT_ALL ||
            prm.cat == FGAC_CAT_OUS ||
            prm.cat == FGAC_CAT_OGR ||
            prm.cat == FGAC_CAT_OTH
           )
            prm.prc.uid = 0;
        else if (prm.cat == FGAC_CAT_UID)
            prm.prc.uid = sqlite3_column_int64 (stmt, 3);
        else if (prm.cat == FGAC_CAT_GID)
            prm.prc.gid = sqlite3_column_int64 (stmt, 3);
        else
        {
            fgac_prms_free(&r);
            sqlite3_finalize(stmt);
            return fgac_prms_error ();
        }

        fgac_prms_push (&r, &prm);
        if (fgac_prms_is_error (&r))
        {
            sqlite3_finalize(stmt);
            return r;
        }
    }
    sqlite3_finalize(stmt);
    return r;
}
