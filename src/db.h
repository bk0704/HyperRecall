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

typedef struct HrTopicRecord {
    sqlite3_int64 id;
    sqlite3_int64 parent_id;
    const char *uuid;
    const char *title;
    const char *summary;
    sqlite3_int64 created_at;
    sqlite3_int64 updated_at;
    int position;
} HrTopicRecord;

typedef struct HrCardRecord {
    sqlite3_int64 id;
    sqlite3_int64 topic_id;
    const char *uuid;
    const char *prompt;
    const char *response;
    const char *mnemonic;
    sqlite3_int64 created_at;
    sqlite3_int64 updated_at;
    sqlite3_int64 due_at;
    int interval;
    int ease_factor;
    int review_state;
    bool suspended;
} HrCardRecord;

typedef struct HrCardDueQuery {
    sqlite3_int64 latest_due_at;
    int limit;
} HrCardDueQuery;

typedef struct HrReviewRecord {
    sqlite3_int64 card_id;
    sqlite3_int64 reviewed_at;
    int rating;
    int duration_ms;
    int scheduled_interval;
    int actual_interval;
    int ease_factor;
    int review_state;
} HrReviewRecord;

typedef struct HrReviewSummaryQuery {
    sqlite3_int64 start_at;
    sqlite3_int64 end_at;
} HrReviewSummaryQuery;

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

int db_topic_prepare_insert(DatabaseHandle *handle, sqlite3_stmt **statement);

int db_topic_bind_insert(sqlite3_stmt *statement, const HrTopicRecord *record);

int db_topic_prepare_update(DatabaseHandle *handle, sqlite3_stmt **statement);

int db_topic_bind_update(sqlite3_stmt *statement, const HrTopicRecord *record);

int db_topic_prepare_delete(DatabaseHandle *handle, sqlite3_stmt **statement);

int db_topic_bind_delete(sqlite3_stmt *statement, sqlite3_int64 topic_id);

int db_topic_prepare_select_by_uuid(DatabaseHandle *handle, sqlite3_stmt **statement);

int db_topic_bind_select_by_uuid(sqlite3_stmt *statement, const char *uuid);

int db_card_prepare_insert(DatabaseHandle *handle, sqlite3_stmt **statement);

int db_card_bind_insert(sqlite3_stmt *statement, const HrCardRecord *record);

int db_card_prepare_update(DatabaseHandle *handle, sqlite3_stmt **statement);

int db_card_bind_update(sqlite3_stmt *statement, const HrCardRecord *record);

int db_card_prepare_delete(DatabaseHandle *handle, sqlite3_stmt **statement);

int db_card_bind_delete(sqlite3_stmt *statement, sqlite3_int64 card_id);

int db_card_prepare_select_due(DatabaseHandle *handle, sqlite3_stmt **statement);

int db_card_bind_select_due(sqlite3_stmt *statement, const HrCardDueQuery *query);

int db_review_prepare_bulk_insert(DatabaseHandle *handle, sqlite3_stmt **statement);

int db_review_bind_bulk_insert(sqlite3_stmt *statement, const HrReviewRecord *record);

int db_analytics_prepare_review_summary(DatabaseHandle *handle, sqlite3_stmt **statement);

int db_analytics_bind_review_summary(sqlite3_stmt *statement, const HrReviewSummaryQuery *query);

int db_analytics_prepare_topic_card_totals(DatabaseHandle *handle, sqlite3_stmt **statement);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_DB_H */
