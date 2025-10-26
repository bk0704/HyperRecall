#include "db.h"

#include "cfg.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef _WIN32
#include <strings.h>
#endif

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define access _access
#define F_OK 0
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

struct DatabaseHandle {
    sqlite3 *connection;
    char database_path[PATH_MAX];
    char backup_dir[PATH_MAX];
    HrBackupPolicy backup_policy;
};

struct Migration {
    unsigned int version;
    const char *sql;
};

static const struct Migration kMigrations[] = {
    {
        1U,
        "PRAGMA foreign_keys = ON;"
        "\nCREATE TABLE IF NOT EXISTS metadata (key TEXT PRIMARY KEY, value TEXT NOT NULL);"
        "\nINSERT INTO metadata(key, value) VALUES('schema_version', '0')"
        " ON CONFLICT(key) DO NOTHING;"
    },
    {
        2U,
        "DROP TABLE IF EXISTS decks;"
        "\nDROP TABLE IF EXISTS notes;"
        "\nDROP TABLE IF EXISTS media;"
        "\nDROP TABLE IF EXISTS tags;"
        "\nDROP TABLE IF EXISTS card_tags;"
        "\nDROP TABLE IF EXISTS sessions;"
        "\nDROP TABLE IF EXISTS analytics_events;"
        "\nDROP TABLE IF EXISTS reviews;"
        "\nDROP TABLE IF EXISTS cards;"
        "\nDROP TABLE IF EXISTS topics;"
        "\nCREATE TABLE IF NOT EXISTS topics ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " uuid TEXT NOT NULL UNIQUE,"
        " parent_id INTEGER REFERENCES topics(id) ON DELETE SET NULL,"
        " title TEXT NOT NULL,"
        " summary TEXT DEFAULT '',"
        " created_at INTEGER NOT NULL,"
        " updated_at INTEGER NOT NULL,"
        " position INTEGER NOT NULL DEFAULT 0"
        ");"
        "\nCREATE TABLE IF NOT EXISTS cards ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " topic_id INTEGER NOT NULL REFERENCES topics(id) ON DELETE CASCADE,"
        " uuid TEXT NOT NULL UNIQUE,"
        " prompt TEXT NOT NULL,"
        " response TEXT NOT NULL,"
        " mnemonic TEXT,"
        " created_at INTEGER NOT NULL,"
        " updated_at INTEGER NOT NULL,"
        " due_at INTEGER NOT NULL DEFAULT 0,"
        " interval INTEGER NOT NULL DEFAULT 0,"
        " ease_factor INTEGER NOT NULL DEFAULT 250,"
        " review_state INTEGER NOT NULL DEFAULT 0,"
        " suspended INTEGER NOT NULL DEFAULT 0"
        ");"
        "\nCREATE TABLE IF NOT EXISTS reviews ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " card_id INTEGER NOT NULL REFERENCES cards(id) ON DELETE CASCADE,"
        " reviewed_at INTEGER NOT NULL,"
        " rating INTEGER NOT NULL,"
        " duration_ms INTEGER NOT NULL,"
        " scheduled_interval INTEGER NOT NULL,"
        " actual_interval INTEGER NOT NULL,"
        " ease_factor INTEGER NOT NULL,"
        " review_state INTEGER NOT NULL"
        ");"
        "\nCREATE INDEX IF NOT EXISTS idx_topics_parent ON topics(parent_id);"
        "\nCREATE INDEX IF NOT EXISTS idx_topics_uuid ON topics(uuid);"
        "\nCREATE INDEX IF NOT EXISTS idx_cards_topic ON cards(topic_id);"
        "\nCREATE INDEX IF NOT EXISTS idx_cards_due ON cards(due_at, suspended);"
        "\nCREATE INDEX IF NOT EXISTS idx_cards_uuid ON cards(uuid);"
        "\nCREATE INDEX IF NOT EXISTS idx_reviews_card_time ON reviews(card_id, reviewed_at);"
        "\nCREATE INDEX IF NOT EXISTS idx_reviews_timestamp ON reviews(reviewed_at);"
    },
};

static int ensure_directory(const char *path)
{
    if (path == NULL || *path == '\0') {
        return 0;
    }

    char buffer[PATH_MAX];
    strncpy(buffer, path, sizeof(buffer) - 1U);
    buffer[sizeof(buffer) - 1U] = '\0';

    size_t len = strlen(buffer);
    if (len == 0U) {
        return 0;
    }

    if (buffer[len - 1U] == '/' || buffer[len - 1U] == '\\') {
        buffer[len - 1U] = '\0';
    }

    for (char *ptr = buffer + 1; *ptr != '\0'; ++ptr) {
        if (*ptr == '/' || *ptr == '\\') {
            *ptr = '\0';
#ifdef _WIN32
            if (_mkdir(buffer) != 0 && errno != EEXIST) {
                int err = errno;
                return -err;
            }
#else
            if (mkdir(buffer, 0700) != 0 && errno != EEXIST) {
                int err = errno;
                return -err;
            }
#endif
            *ptr = '/';
        }
    }

#ifdef _WIN32
    if (_mkdir(buffer) != 0 && errno != EEXIST) {
        int err = errno;
        return -err;
    }
#else
    if (mkdir(buffer, 0700) != 0 && errno != EEXIST) {
        int err = errno;
        return -err;
    }
#endif

    return 0;
}

static int exec_simple(sqlite3 *db, const char *sql)
{
    if (db == NULL || sql == NULL) {
        return SQLITE_MISUSE;
    }
    char *errmsg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        if (errmsg != NULL) {
            fprintf(stderr, "SQLite exec error (%d): %s\n", rc, errmsg);
        }
        sqlite3_free(errmsg);
    }
    return rc;
}

static int bind_text_or_null(sqlite3_stmt *stmt, int index, const char *value)
{
    if (stmt == NULL) {
        return SQLITE_MISUSE;
    }

    if (value != NULL) {
        return sqlite3_bind_text(stmt, index, value, -1, SQLITE_TRANSIENT);
    }

    return sqlite3_bind_null(stmt, index);
}

static int bind_optional_int64(sqlite3_stmt *stmt, int index, sqlite3_int64 value)
{
    if (stmt == NULL) {
        return SQLITE_MISUSE;
    }

    if (value > 0) {
        return sqlite3_bind_int64(stmt, index, value);
    }

    return sqlite3_bind_null(stmt, index);
}

static int fetch_schema_version(sqlite3 *db, unsigned int *version)
{
    if (db == NULL || version == NULL) {
        return SQLITE_MISUSE;
    }

    int rc = exec_simple(db, "CREATE TABLE IF NOT EXISTS metadata (key TEXT PRIMARY KEY, value TEXT NOT NULL);");
    if (rc != SQLITE_OK) {
        return rc;
    }

    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2(db, "SELECT value FROM metadata WHERE key='schema_version'", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return rc;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char *text = sqlite3_column_text(stmt, 0);
        if (text != NULL) {
            *version = (unsigned int)strtoul((const char *)text, NULL, 10);
        }
    } else if (rc == SQLITE_DONE) {
        *version = 0U;
        rc = SQLITE_OK;
    }

    sqlite3_finalize(stmt);
    return rc;
}

static int set_schema_version(sqlite3 *db, unsigned int version)
{
    if (db == NULL) {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db,
                                "INSERT INTO metadata(key, value) VALUES('schema_version', ?1) "
                                "ON CONFLICT(key) DO UPDATE SET value=excluded.value;",
                                -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return rc;
    }

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%u", version);
    sqlite3_bind_text(stmt, 1, buffer, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        rc = SQLITE_OK;
    }

    sqlite3_finalize(stmt);
    return rc;
}

static int apply_migrations(sqlite3 *db)
{
    unsigned int current_version = 0U;
    int rc = fetch_schema_version(db, &current_version);
    if (rc != SQLITE_OK) {
        return rc;
    }

    for (size_t i = 0; i < sizeof(kMigrations) / sizeof(kMigrations[0]); ++i) {
        const struct Migration *migration = &kMigrations[i];
        if (migration->version <= current_version) {
            continue;
        }

        rc = exec_simple(db, "BEGIN IMMEDIATE;");
        if (rc != SQLITE_OK) {
            return rc;
        }

        rc = exec_simple(db, migration->sql);
        if (rc != SQLITE_OK) {
            (void)exec_simple(db, "ROLLBACK;");
            return rc;
        }

        rc = set_schema_version(db, migration->version);
        if (rc != SQLITE_OK) {
            (void)exec_simple(db, "ROLLBACK;");
            return rc;
        }

        rc = exec_simple(db, "COMMIT;");
        if (rc != SQLITE_OK) {
            return rc;
        }

        current_version = migration->version;
    }

    return SQLITE_OK;
}

static int apply_pragmas(sqlite3 *db)
{
    static const char *const pragmas[] = {
        "PRAGMA foreign_keys = ON;",
        "PRAGMA journal_mode = WAL;",
        "PRAGMA synchronous = NORMAL;",
        "PRAGMA temp_store = MEMORY;",
    };

    for (size_t i = 0; i < sizeof(pragmas) / sizeof(pragmas[0]); ++i) {
        int rc = exec_simple(db, pragmas[i]);
        if (rc != SQLITE_OK) {
            return rc;
        }
    }

    sqlite3_busy_timeout(db, 5000);
    return SQLITE_OK;
}

static int compare_dirent_names(const void *lhs, const void *rhs)
{
#ifndef _WIN32
    const struct dirent *const *a = lhs;
    const struct dirent *const *b = rhs;
    return strcasecmp((*a)->d_name, (*b)->d_name);
#else
    (void)lhs;
    (void)rhs;
    return 0;
#endif
}

static int prune_backups(const char *backup_dir, const HrBackupPolicy *policy)
{
    if (policy == NULL || backup_dir == NULL) {
        return 0;
    }

#ifdef _WIN32
    (void)backup_dir;
    (void)policy;
    return 0;
#else
    DIR *dir = opendir(backup_dir);
    if (dir == NULL) {
        return -errno;
    }

    struct dirent **entries = NULL;
    size_t count = 0U;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        struct dirent *copy = malloc(sizeof(*copy));
        if (copy == NULL) {
            continue;
        }
        memcpy(copy, entry, sizeof(*copy));
        struct dirent **tmp = realloc(entries, (count + 1U) * sizeof(*entries));
        if (tmp == NULL) {
            free(copy);
            continue;
        }
        entries = tmp;
        entries[count++] = copy;
    }
    closedir(dir);

    if (count == 0U) {
        free(entries);
        return 0;
    }

    qsort(entries, count, sizeof(entries[0]), compare_dirent_names);

    time_t now = time(NULL);
    for (size_t i = 0; i < count; ++i) {
        if (entries[i] == NULL) {
            continue;
        }
        bool keep = true;
        if (policy->max_files > 0U) {
            size_t remaining = count - i;
            if (remaining > policy->max_files) {
                keep = false;
            }
        }
        if (keep && policy->keep_days > 0U) {
            int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
            if (sscanf(entries[i]->d_name, "%4d%2d%2d%2d%2d%2d", &year, &month, &day, &hour, &minute, &second) == 6) {
                struct tm tm_info;
                memset(&tm_info, 0, sizeof(tm_info));
                tm_info.tm_year = year - 1900;
                tm_info.tm_mon = month - 1;
                tm_info.tm_mday = day;
                tm_info.tm_hour = hour;
                tm_info.tm_min = minute;
                tm_info.tm_sec = second;
                time_t backup_time = mktime(&tm_info);
                double age_days = difftime(now, backup_time) / (60.0 * 60.0 * 24.0);
                if (age_days > (double)policy->keep_days) {
                    keep = false;
                }
            }
        }

        if (!keep) {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", backup_dir, entries[i]->d_name);
            remove(path);
        }
        free(entries[i]);
    }
    free(entries);

    return 0;
#endif
}

static int create_backup_file(DatabaseHandle *handle, const char *tag)
{
    if (handle == NULL || handle->database_path[0] == '\0' || handle->connection == NULL) {
        return -EINVAL;
    }

    int dir_rc = ensure_directory(handle->backup_dir);
    if (dir_rc != 0) {
        return dir_rc;
    }

    if (access(handle->database_path, F_OK) != 0) {
        return 0;
    }

    char timestamp[32];
    time_t now = time(NULL);
    struct tm tm_info;
#ifdef _WIN32
    localtime_s(&tm_info, &now);
#else
    localtime_r(&now, &tm_info);
#endif
    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", &tm_info);

    char filename[PATH_MAX];
    if (tag != NULL && tag[0] != '\0') {
        snprintf(filename, sizeof(filename), "%s/%s-%s.db", handle->backup_dir, timestamp, tag);
    } else {
        snprintf(filename, sizeof(filename), "%s/%s.db", handle->backup_dir, timestamp);
    }

    sqlite3 *backup_db = NULL;
    int rc = sqlite3_open_v2(filename,
                             &backup_db,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                             NULL);
    if (rc != SQLITE_OK) {
        if (backup_db != NULL) {
            sqlite3_close(backup_db);
        }
        remove(filename);
        return rc > 0 ? -rc : rc;
    }

    sqlite3_backup *backup = sqlite3_backup_init(backup_db, "main", handle->connection, "main");
    if (backup == NULL) {
        rc = sqlite3_errcode(backup_db);
    } else {
        do {
            rc = sqlite3_backup_step(backup, 128);
            if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
                sqlite3_sleep(25);
            }
        } while (rc == SQLITE_BUSY || rc == SQLITE_LOCKED);

        int finish_rc = sqlite3_backup_finish(backup);
        if (rc == SQLITE_DONE) {
            rc = SQLITE_OK;
        }
        if (finish_rc != SQLITE_OK) {
            rc = finish_rc;
        }
    }

    int close_rc = sqlite3_close(backup_db);
    if (rc == SQLITE_OK && close_rc != SQLITE_OK) {
        rc = close_rc;
    }

    if (rc != SQLITE_OK) {
        remove(filename);
        return rc > 0 ? -rc : rc;
    }

    rc = prune_backups(handle->backup_dir, &handle->backup_policy);
    return rc;
}

DatabaseHandle *db_open(const struct ConfigHandle *config)
{
    if (config == NULL) {
        return NULL;
    }

    DatabaseHandle *handle = calloc(1U, sizeof(*handle));
    if (handle == NULL) {
        return NULL;
    }

    const HrConfig *cfg = cfg_data(config);
    if (cfg == NULL) {
        free(handle);
        return NULL;
    }

    strncpy(handle->database_path, cfg->database.path, sizeof(handle->database_path) - 1U);
    strncpy(handle->backup_dir, cfg->database.backup_dir, sizeof(handle->backup_dir) - 1U);
    handle->backup_policy = cfg->database.backup;

    int dir_rc = ensure_directory(cfg->paths.data_dir);
    if (dir_rc != 0) {
        free(handle);
        return NULL;
    }

    int rc = sqlite3_open_v2(handle->database_path,
                             &handle->connection,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                             NULL);
    if (rc != SQLITE_OK) {
        const char *errmsg = handle->connection != NULL ? sqlite3_errmsg(handle->connection) : NULL;
        fprintf(stderr, "Failed to open database %s (%d): %s\n", handle->database_path, rc,
                errmsg != NULL ? errmsg : "unknown error");
        sqlite3_close(handle->connection);
        free(handle);
        return NULL;
    }

    rc = apply_pragmas(handle->connection);
    if (rc != SQLITE_OK) {
        db_close(handle);
        return NULL;
    }

    rc = apply_migrations(handle->connection);
    if (rc != SQLITE_OK) {
        db_close(handle);
        return NULL;
    }

    if (handle->backup_policy.enable_auto) {
        (void)create_backup_file(handle, "auto");
    }

    return handle;
}

void db_close(DatabaseHandle *handle)
{
    if (handle == NULL) {
        return;
    }

    if (handle->connection != NULL) {
        sqlite3_close(handle->connection);
        handle->connection = NULL;
    }
    free(handle);
}

sqlite3 *db_connection(DatabaseHandle *handle)
{
    return handle != NULL ? handle->connection : NULL;
}

const char *db_path(const DatabaseHandle *handle)
{
    return handle != NULL ? handle->database_path : NULL;
}

int db_prepare(DatabaseHandle *handle, sqlite3_stmt **statement, const char *sql)
{
    if (handle == NULL || handle->connection == NULL || statement == NULL || sql == NULL) {
        return SQLITE_MISUSE;
    }

    return sqlite3_prepare_v2(handle->connection, sql, -1, statement, NULL);
}

int db_exec(DatabaseHandle *handle, const char *sql)
{
    if (handle == NULL) {
        return SQLITE_MISUSE;
    }
    return exec_simple(handle->connection, sql);
}

int db_begin(DatabaseHandle *handle)
{
    return db_exec(handle, "BEGIN IMMEDIATE;");
}

int db_commit(DatabaseHandle *handle)
{
    return db_exec(handle, "COMMIT;");
}

int db_rollback(DatabaseHandle *handle)
{
    return db_exec(handle, "ROLLBACK;");
}

int db_run_in_transaction(DatabaseHandle *handle, HrDbTxnCallback callback, void *user_data)
{
    if (handle == NULL || callback == NULL) {
        return SQLITE_MISUSE;
    }

    int rc = db_begin(handle);
    if (rc != SQLITE_OK) {
        return rc;
    }

    rc = callback(handle->connection, user_data);
    if (rc == SQLITE_OK) {
        int commit_rc = db_commit(handle);
        if (commit_rc != SQLITE_OK) {
            return commit_rc;
        }
    } else {
        (void)db_rollback(handle);
    }

    return rc;
}

int db_create_backup(DatabaseHandle *handle, const char *tag)
{
    if (handle == NULL) {
        return -EINVAL;
    }

    return create_backup_file(handle, tag);
}

int db_topic_prepare_insert(DatabaseHandle *handle, sqlite3_stmt **statement)
{
    static const char *sql =
        "INSERT INTO topics(uuid, parent_id, title, summary, created_at, updated_at, position) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7);";
    return db_prepare(handle, statement, sql);
}

int db_topic_bind_insert(sqlite3_stmt *statement, const HrTopicRecord *record)
{
    if (statement == NULL || record == NULL || record->uuid == NULL || record->title == NULL) {
        return SQLITE_MISUSE;
    }

    int rc = sqlite3_bind_text(statement, 1, record->uuid, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = bind_optional_int64(statement, 2, record->parent_id);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_text(statement, 3, record->title, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = bind_text_or_null(statement, 4, record->summary);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int64(statement, 5, record->created_at);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int64(statement, 6, record->updated_at);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 7, record->position);
    return rc;
}

int db_topic_prepare_update(DatabaseHandle *handle, sqlite3_stmt **statement)
{
    static const char *sql =
        "UPDATE topics SET parent_id=?2, title=?3, summary=?4, created_at=?5, updated_at=?6, position=?7 "
        "WHERE id=?1;";
    return db_prepare(handle, statement, sql);
}

int db_topic_bind_update(sqlite3_stmt *statement, const HrTopicRecord *record)
{
    if (statement == NULL || record == NULL || record->id <= 0 || record->title == NULL) {
        return SQLITE_MISUSE;
    }

    int rc = sqlite3_bind_int64(statement, 1, record->id);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = bind_optional_int64(statement, 2, record->parent_id);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_text(statement, 3, record->title, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = bind_text_or_null(statement, 4, record->summary);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int64(statement, 5, record->created_at);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int64(statement, 6, record->updated_at);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 7, record->position);
    return rc;
}

int db_topic_prepare_delete(DatabaseHandle *handle, sqlite3_stmt **statement)
{
    static const char *sql = "DELETE FROM topics WHERE id=?1;";
    return db_prepare(handle, statement, sql);
}

int db_topic_bind_delete(sqlite3_stmt *statement, sqlite3_int64 topic_id)
{
    if (statement == NULL || topic_id <= 0) {
        return SQLITE_MISUSE;
    }
    return sqlite3_bind_int64(statement, 1, topic_id);
}

int db_topic_prepare_select_by_uuid(DatabaseHandle *handle, sqlite3_stmt **statement)
{
    static const char *sql =
        "SELECT id, parent_id, uuid, title, summary, created_at, updated_at, position "
        "FROM topics WHERE uuid=?1;";
    return db_prepare(handle, statement, sql);
}

int db_topic_bind_select_by_uuid(sqlite3_stmt *statement, const char *uuid)
{
    if (statement == NULL || uuid == NULL) {
        return SQLITE_MISUSE;
    }
    return sqlite3_bind_text(statement, 1, uuid, -1, SQLITE_TRANSIENT);
}

int db_card_prepare_insert(DatabaseHandle *handle, sqlite3_stmt **statement)
{
    static const char *sql =
        "INSERT INTO cards(uuid, topic_id, prompt, response, mnemonic, created_at, updated_at, due_at, interval, ease_factor, "
        "review_state, suspended) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12);";
    return db_prepare(handle, statement, sql);
}

int db_card_bind_insert(sqlite3_stmt *statement, const HrCardRecord *record)
{
    if (statement == NULL || record == NULL || record->uuid == NULL || record->prompt == NULL ||
        record->response == NULL || record->topic_id <= 0) {
        return SQLITE_MISUSE;
    }

    int rc = sqlite3_bind_text(statement, 1, record->uuid, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int64(statement, 2, record->topic_id);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_text(statement, 3, record->prompt, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_text(statement, 4, record->response, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = bind_text_or_null(statement, 5, record->mnemonic);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int64(statement, 6, record->created_at);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int64(statement, 7, record->updated_at);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int64(statement, 8, record->due_at);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 9, record->interval);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 10, record->ease_factor);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 11, record->review_state);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 12, record->suspended ? 1 : 0);
    return rc;
}

int db_card_prepare_update(DatabaseHandle *handle, sqlite3_stmt **statement)
{
    static const char *sql =
        "UPDATE cards SET topic_id=?2, prompt=?3, response=?4, mnemonic=?5, created_at=?6, updated_at=?7, due_at=?8, interval=?9, "
        "ease_factor=?10, review_state=?11, suspended=?12 WHERE id=?1;";
    return db_prepare(handle, statement, sql);
}

int db_card_bind_update(sqlite3_stmt *statement, const HrCardRecord *record)
{
    if (statement == NULL || record == NULL || record->id <= 0 || record->prompt == NULL || record->response == NULL ||
        record->topic_id <= 0) {
        return SQLITE_MISUSE;
    }

    int rc = sqlite3_bind_int64(statement, 1, record->id);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int64(statement, 2, record->topic_id);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_text(statement, 3, record->prompt, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_text(statement, 4, record->response, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = bind_text_or_null(statement, 5, record->mnemonic);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int64(statement, 6, record->created_at);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int64(statement, 7, record->updated_at);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int64(statement, 8, record->due_at);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 9, record->interval);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 10, record->ease_factor);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 11, record->review_state);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 12, record->suspended ? 1 : 0);
    return rc;
}

int db_card_prepare_delete(DatabaseHandle *handle, sqlite3_stmt **statement)
{
    static const char *sql = "DELETE FROM cards WHERE id=?1;";
    return db_prepare(handle, statement, sql);
}

int db_card_bind_delete(sqlite3_stmt *statement, sqlite3_int64 card_id)
{
    if (statement == NULL || card_id <= 0) {
        return SQLITE_MISUSE;
    }
    return sqlite3_bind_int64(statement, 1, card_id);
}

int db_card_prepare_select_due(DatabaseHandle *handle, sqlite3_stmt **statement)
{
    static const char *sql =
        "SELECT id, topic_id, uuid, prompt, response, mnemonic, created_at, updated_at, due_at, interval, ease_factor, "
        "review_state, suspended FROM cards WHERE suspended=0 AND due_at <= ?1 ORDER BY due_at ASC LIMIT ?2;";
    return db_prepare(handle, statement, sql);
}

int db_card_bind_select_due(sqlite3_stmt *statement, const HrCardDueQuery *query)
{
    if (statement == NULL || query == NULL || query->limit <= 0) {
        return SQLITE_MISUSE;
    }

    int rc = sqlite3_bind_int64(statement, 1, query->latest_due_at);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 2, query->limit);
    return rc;
}

int db_review_prepare_bulk_insert(DatabaseHandle *handle, sqlite3_stmt **statement)
{
    static const char *sql =
        "INSERT INTO reviews(card_id, reviewed_at, rating, duration_ms, scheduled_interval, actual_interval, ease_factor, "
        "review_state) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8);";
    return db_prepare(handle, statement, sql);
}

int db_review_bind_bulk_insert(sqlite3_stmt *statement, const HrReviewRecord *record)
{
    if (statement == NULL || record == NULL || record->card_id <= 0) {
        return SQLITE_MISUSE;
    }

    int rc = sqlite3_bind_int64(statement, 1, record->card_id);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int64(statement, 2, record->reviewed_at);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 3, record->rating);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 4, record->duration_ms);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 5, record->scheduled_interval);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 6, record->actual_interval);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 7, record->ease_factor);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int(statement, 8, record->review_state);
    return rc;
}

int db_analytics_prepare_review_summary(DatabaseHandle *handle, sqlite3_stmt **statement)
{
    static const char *sql =
        "SELECT strftime('%Y-%m-%d', reviewed_at, 'unixepoch') AS day, COUNT(*) AS total_reviews, "
        "SUM(CASE WHEN rating >= 3 THEN 1 ELSE 0 END) AS successful_reviews, AVG(duration_ms) AS avg_duration_ms "
        "FROM reviews WHERE reviewed_at BETWEEN ?1 AND ?2 GROUP BY day ORDER BY day;";
    return db_prepare(handle, statement, sql);
}

int db_analytics_bind_review_summary(sqlite3_stmt *statement, const HrReviewSummaryQuery *query)
{
    if (statement == NULL || query == NULL) {
        return SQLITE_MISUSE;
    }

    sqlite3_int64 start = query->start_at >= 0 ? query->start_at : 0;
    sqlite3_int64 end = query->end_at > 0 ? query->end_at : (sqlite3_int64)INT64_MAX;

    int rc = sqlite3_bind_int64(statement, 1, start);
    if (rc != SQLITE_OK) {
        return rc;
    }
    rc = sqlite3_bind_int64(statement, 2, end);
    return rc;
}

int db_analytics_prepare_topic_card_totals(DatabaseHandle *handle, sqlite3_stmt **statement)
{
    static const char *sql =
        "SELECT t.id, t.uuid, t.title, COUNT(c.id) AS card_count FROM topics t "
        "LEFT JOIN cards c ON c.topic_id = t.id GROUP BY t.id ORDER BY t.position, t.title;";
    return db_prepare(handle, statement, sql);
}
