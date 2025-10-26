#ifndef HYPERRECALL_DB_H
#define HYPERRECALL_DB_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file db.h
 * @brief Declares persistence layer interfaces built on SQLite3.
 */

#include <stdbool.h>
#include <sqlite3.h>

struct ConfigHandle;

typedef struct DatabaseHandle DatabaseHandle;

typedef int (*HrDbTxnCallback)(sqlite3 *db, void *user_data);

DatabaseHandle *db_open(const struct ConfigHandle *config);

void db_close(DatabaseHandle *handle);

sqlite3 *db_connection(DatabaseHandle *handle);

const char *db_path(const DatabaseHandle *handle);

int db_prepare(DatabaseHandle *handle, sqlite3_stmt **statement, const char *sql);

int db_exec(DatabaseHandle *handle, const char *sql);

int db_begin(DatabaseHandle *handle);

int db_commit(DatabaseHandle *handle);

int db_rollback(DatabaseHandle *handle);

int db_run_in_transaction(DatabaseHandle *handle, HrDbTxnCallback callback, void *user_data);

int db_create_backup(DatabaseHandle *handle, const char *tag);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_DB_H */
