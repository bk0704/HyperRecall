#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "import_export.h"

#include "db.h"
#include "json.h"
#include "model.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#if defined(_WIN32)
static char *hr_strtok_r(char *str, const char *delim, char **context)
{
    return strtok_s(str, delim, context);
}

#define strtok_r(str, delim, saveptr) hr_strtok_r((str), (delim), (saveptr))
#endif

/* Forward declarations for helper functions */
static HrJsonValue *serialize_card_to_json(const HrCard *card, bool include_srs_state);
static HrJsonValue *serialize_topic_to_json(const HrTopic *topic);
static bool deserialize_card_from_json(const HrJsonValue *json, HrCard *card, bool import_srs_state);
static bool deserialize_topic_from_json(const HrJsonValue *json, HrTopic *topic);

/* Serialize a card to JSON format */
static HrJsonValue *serialize_card_to_json(const HrCard *card, bool include_srs_state)
{
    if (!card) {
        return NULL;
    }

    HrJsonValue *obj = hr_json_object_new();
    if (!obj) {
        return NULL;
    }

    /* Basic fields */
    hr_json_object_set(obj, "id", hr_json_number_new((double)card->id));
    hr_json_object_set(obj, "topic_id", hr_json_number_new((double)card->topic_id));
    if (card->uuid) {
        hr_json_object_set(obj, "uuid", hr_json_string_new(card->uuid));
    }
    hr_json_object_set(obj, "type", hr_json_string_new(hr_card_type_to_string(card->type)));
    hr_json_object_set(obj, "prompt", hr_json_string_new(card->prompt ? card->prompt : ""));
    hr_json_object_set(obj, "response", hr_json_string_new(card->response ? card->response : ""));
    if (card->mnemonic) {
        hr_json_object_set(obj, "mnemonic", hr_json_string_new(card->mnemonic));
    }
    hr_json_object_set(obj, "created_at", hr_json_number_new((double)card->created_at));
    hr_json_object_set(obj, "updated_at", hr_json_number_new((double)card->updated_at));
    hr_json_object_set(obj, "suspended", hr_json_bool_new(card->suspended));

    /* SRS state fields (optional) */
    if (include_srs_state) {
        hr_json_object_set(obj, "due_at", hr_json_number_new((double)card->due_at));
        hr_json_object_set(obj, "interval", hr_json_number_new((double)card->interval));
        hr_json_object_set(obj, "ease_factor", hr_json_number_new((double)card->ease_factor));
        hr_json_object_set(obj, "review_state", hr_json_number_new((double)card->review_state));
    }

    /* TODO: Serialize card->extras and card->media if needed */
    /* For now, we'll skip these as they require more complex serialization */

    return obj;
}

/* Serialize a topic to JSON format */
static HrJsonValue *serialize_topic_to_json(const HrTopic *topic)
{
    if (!topic) {
        return NULL;
    }

    HrJsonValue *obj = hr_json_object_new();
    if (!obj) {
        return NULL;
    }

    hr_json_object_set(obj, "id", hr_json_number_new((double)topic->id));
    if (topic->parent_id > 0) {
        hr_json_object_set(obj, "parent_id", hr_json_number_new((double)topic->parent_id));
    } else {
        hr_json_object_set(obj, "parent_id", hr_json_null_new());
    }
    if (topic->uuid) {
        hr_json_object_set(obj, "uuid", hr_json_string_new(topic->uuid));
    }
    hr_json_object_set(obj, "title", hr_json_string_new(topic->title ? topic->title : ""));
    if (topic->summary) {
        hr_json_object_set(obj, "summary", hr_json_string_new(topic->summary));
    }
    hr_json_object_set(obj, "created_at", hr_json_number_new((double)topic->created_at));
    hr_json_object_set(obj, "updated_at", hr_json_number_new((double)topic->updated_at));
    hr_json_object_set(obj, "position", hr_json_number_new((double)topic->position));

    return obj;
}

/* Deserialize a card from JSON format */
static bool deserialize_card_from_json(const HrJsonValue *json, HrCard *card, bool import_srs_state)
{
    if (!json || !card) {
        return false;
    }

    memset(card, 0, sizeof(*card));

    /* Parse basic fields */
    const HrJsonValue *val;
    double num;

    val = hr_json_object_get(json, "id");
    if (val && hr_json_get_number(val, &num)) {
        card->id = (sqlite3_int64)num;
    }

    val = hr_json_object_get(json, "topic_id");
    if (val && hr_json_get_number(val, &num)) {
        card->topic_id = (sqlite3_int64)num;
    }

    val = hr_json_object_get(json, "uuid");
    if (val) {
        card->uuid = hr_json_get_string(val);
    }

    val = hr_json_object_get(json, "type");
    if (val) {
        const char *type_str = hr_json_get_string(val);
        if (type_str) {
            hr_card_type_from_string(type_str, &card->type);
        }
    }

    val = hr_json_object_get(json, "prompt");
    if (val) {
        card->prompt = hr_json_get_string(val);
    }

    val = hr_json_object_get(json, "response");
    if (val) {
        card->response = hr_json_get_string(val);
    }

    val = hr_json_object_get(json, "mnemonic");
    if (val) {
        card->mnemonic = hr_json_get_string(val);
    }

    val = hr_json_object_get(json, "created_at");
    if (val && hr_json_get_number(val, &num)) {
        card->created_at = (sqlite3_int64)num;
    }

    val = hr_json_object_get(json, "updated_at");
    if (val && hr_json_get_number(val, &num)) {
        card->updated_at = (sqlite3_int64)num;
    }

    val = hr_json_object_get(json, "suspended");
    if (val) {
        hr_json_get_bool(val, &card->suspended);
    }

    /* Parse SRS state fields if requested */
    if (import_srs_state) {
        val = hr_json_object_get(json, "due_at");
        if (val && hr_json_get_number(val, &num)) {
            card->due_at = (sqlite3_int64)num;
        }

        val = hr_json_object_get(json, "interval");
        if (val && hr_json_get_number(val, &num)) {
            card->interval = (int)num;
        }

        val = hr_json_object_get(json, "ease_factor");
        if (val && hr_json_get_number(val, &num)) {
            card->ease_factor = (int)num;
        }

        val = hr_json_object_get(json, "review_state");
        if (val && hr_json_get_number(val, &num)) {
            card->review_state = (int)num;
        }
    } else {
        /* Initialize with default SRS state */
        card->due_at = 0;
        card->interval = 0;
        card->ease_factor = 250;
        card->review_state = 0;
    }

    return true;
}

/* Deserialize a topic from JSON format */
static bool deserialize_topic_from_json(const HrJsonValue *json, HrTopic *topic)
{
    if (!json || !topic) {
        return false;
    }

    memset(topic, 0, sizeof(*topic));

    const HrJsonValue *val;
    double num;

    val = hr_json_object_get(json, "id");
    if (val && hr_json_get_number(val, &num)) {
        topic->id = (sqlite3_int64)num;
    }

    val = hr_json_object_get(json, "parent_id");
    if (val && hr_json_type(val) != HR_JSON_NULL && hr_json_get_number(val, &num)) {
        topic->parent_id = (sqlite3_int64)num;
    }

    val = hr_json_object_get(json, "uuid");
    if (val) {
        topic->uuid = hr_json_get_string(val);
    }

    val = hr_json_object_get(json, "title");
    if (val) {
        topic->title = hr_json_get_string(val);
    }

    val = hr_json_object_get(json, "summary");
    if (val) {
        topic->summary = hr_json_get_string(val);
    }

    val = hr_json_object_get(json, "created_at");
    if (val && hr_json_get_number(val, &num)) {
        topic->created_at = (sqlite3_int64)num;
    }

    val = hr_json_object_get(json, "updated_at");
    if (val && hr_json_get_number(val, &num)) {
        topic->updated_at = (sqlite3_int64)num;
    }

    val = hr_json_object_get(json, "position");
    if (val && hr_json_get_number(val, &num)) {
        topic->position = (int)num;
    }

    return true;
}

static bool read_file_contents(const char *path, char **out_contents, size_t *out_size)
{
    FILE *file = fopen(path, "rb");
    if (!file) {
        return false;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size < 0) {
        fclose(file);
        return false;
    }

    char *contents = (char *)malloc((size_t)size + 1);
    if (!contents) {
        fclose(file);
        return false;
    }

    size_t read = fread(contents, 1, (size_t)size, file);
    fclose(file);

    if (read != (size_t)size) {
        free(contents);
        return false;
    }

    contents[size] = '\0';
    *out_contents = contents;
    if (out_size) {
        *out_size = (size_t)size;
    }
    return true;
}

bool hr_export_json(struct DatabaseHandle *db, const HrExportOptions *options, HrExportResult *result)
{
    if (!db || !options || !result) {
        return false;
    }

    memset(result, 0, sizeof(*result));

    /* Create root JSON object */
    HrJsonValue *root = hr_json_object_new();
    if (!root) {
        snprintf(result->error, sizeof(result->error), "Failed to create JSON object");
        return false;
    }

    /* Add metadata */
    HrJsonValue *meta = hr_json_object_new();
    if (meta) {
        hr_json_object_set(meta, "version", hr_json_string_new("1.0"));
        hr_json_object_set(meta, "exported_at", hr_json_number_new((double)time(NULL)));
        hr_json_object_set(root, "metadata", meta);
    }

    /* Export topics if requested */
    HrJsonValue *topics_array = hr_json_array_new();
    if (!topics_array) {
        hr_json_free(root);
        snprintf(result->error, sizeof(result->error), "Failed to create topics array");
        return false;
    }

    if (options->include_topics) {
        sqlite3_stmt *stmt = NULL;
        const char *sql = "SELECT id, parent_id, uuid, title, summary, created_at, updated_at, position FROM topics ORDER BY id";
        
        int rc = db_prepare(db, &stmt, sql);
        if (rc == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                HrTopic topic;
                memset(&topic, 0, sizeof(topic));
                
                topic.id = sqlite3_column_int64(stmt, 0);
                if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
                    topic.parent_id = sqlite3_column_int64(stmt, 1);
                }
                topic.uuid = (const char *)sqlite3_column_text(stmt, 2);
                topic.title = (const char *)sqlite3_column_text(stmt, 3);
                topic.summary = (const char *)sqlite3_column_text(stmt, 4);
                topic.created_at = sqlite3_column_int64(stmt, 5);
                topic.updated_at = sqlite3_column_int64(stmt, 6);
                topic.position = sqlite3_column_int(stmt, 7);

                HrJsonValue *topic_json = serialize_topic_to_json(&topic);
                if (topic_json) {
                    hr_json_array_append(topics_array, topic_json);
                    result->topics_exported++;
                }
            }
            sqlite3_finalize(stmt);
        }
    }

    hr_json_object_set(root, "topics", topics_array);

    /* Export cards */
    HrJsonValue *cards_array = hr_json_array_new();
    if (!cards_array) {
        hr_json_free(root);
        snprintf(result->error, sizeof(result->error), "Failed to create cards array");
        return false;
    }

    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id, topic_id, uuid, prompt, response, mnemonic, created_at, updated_at, "
                      "due_at, interval, ease_factor, review_state, suspended FROM cards ORDER BY id";
    
    int rc = db_prepare(db, &stmt, sql);
    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            HrCard card;
            memset(&card, 0, sizeof(card));
            
            card.id = sqlite3_column_int64(stmt, 0);
            card.topic_id = sqlite3_column_int64(stmt, 1);
            card.uuid = (const char *)sqlite3_column_text(stmt, 2);
            card.prompt = (const char *)sqlite3_column_text(stmt, 3);
            card.response = (const char *)sqlite3_column_text(stmt, 4);
            if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
                card.mnemonic = (const char *)sqlite3_column_text(stmt, 5);
            }
            card.created_at = sqlite3_column_int64(stmt, 6);
            card.updated_at = sqlite3_column_int64(stmt, 7);
            card.due_at = sqlite3_column_int64(stmt, 8);
            card.interval = sqlite3_column_int(stmt, 9);
            card.ease_factor = sqlite3_column_int(stmt, 10);
            card.review_state = sqlite3_column_int(stmt, 11);
            card.suspended = sqlite3_column_int(stmt, 12) != 0;
            
            /* Default to ShortAnswer type - in a real implementation, 
               this should be stored in the database or inferred from extras */
            card.type = HR_CARD_TYPE_SHORT_ANSWER;

            HrJsonValue *card_json = serialize_card_to_json(&card, options->include_srs_state);
            if (card_json) {
                hr_json_array_append(cards_array, card_json);
                result->cards_exported++;
            }
        }
        sqlite3_finalize(stmt);
    } else {
        hr_json_free(cards_array);
        hr_json_free(root);
        snprintf(result->error, sizeof(result->error), "Failed to query cards");
        return false;
    }

    hr_json_object_set(root, "cards", cards_array);

    /* Serialize to file */
    char *json_text = hr_json_serialize(root, options->pretty_print);
    hr_json_free(root);

    if (!json_text) {
        snprintf(result->error, sizeof(result->error), "Failed to serialize JSON");
        return false;
    }

    FILE *file = fopen(options->output_path, "w");
    if (!file) {
        free(json_text);
        snprintf(result->error, sizeof(result->error), "Failed to open output file");
        return false;
    }

    fputs(json_text, file);
    fclose(file);
    free(json_text);

    result->success = true;
    /* Media copying would be implemented here if needed */
    result->media_files_copied = 0;

    return true;
}

bool hr_import_json(struct DatabaseHandle *db, const HrImportOptions *options, HrImportResult *result)
{
    if (!db || !options || !result) {
        return false;
    }

    memset(result, 0, sizeof(*result));

    /* Read file */
    char *contents = NULL;
    if (!read_file_contents(options->input_path, &contents, NULL)) {
        snprintf(result->error, sizeof(result->error), "Failed to read input file");
        return false;
    }

    /* Parse JSON */
    HrJsonValue *root = hr_json_parse(contents);
    free(contents);

    if (!root) {
        snprintf(result->error, sizeof(result->error), "Failed to parse JSON");
        return false;
    }

    /* Validate structure */
    const HrJsonValue *cards = hr_json_object_get(root, "cards");
    const HrJsonValue *topics = hr_json_object_get(root, "topics");

    if (!cards || hr_json_type(cards) != HR_JSON_ARRAY) {
        hr_json_free(root);
        snprintf(result->error, sizeof(result->error), "Invalid JSON structure: missing 'cards' array");
        return false;
    }

    if (options->validate_only) {
        result->success = true;
        hr_json_free(root);
        return true;
    }

    /* Begin transaction */
    if (db_begin(db) != SQLITE_OK) {
        hr_json_free(root);
        snprintf(result->error, sizeof(result->error), "Failed to begin transaction");
        return false;
    }

    /* Import topics first (if present and merge_topics is enabled) */
    if (topics && hr_json_type(topics) == HR_JSON_ARRAY && options->merge_topics) {
        size_t topic_count = hr_json_array_size(topics);
        
        for (size_t i = 0; i < topic_count; i++) {
            const HrJsonValue *topic_json = hr_json_array_get(topics, i);
            if (!topic_json) continue;

            HrTopic topic;
            if (!deserialize_topic_from_json(topic_json, &topic)) {
                continue;
            }

            /* Try to insert topic, skip if UUID already exists */
            sqlite3_stmt *stmt = NULL;
            const char *sql = "INSERT OR IGNORE INTO topics (uuid, parent_id, title, summary, created_at, updated_at, position) "
                              "VALUES (?, ?, ?, ?, ?, ?, ?)";
            
            if (db_prepare(db, &stmt, sql) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, topic.uuid ? topic.uuid : "", -1, SQLITE_TRANSIENT);
                if (topic.parent_id > 0) {
                    sqlite3_bind_int64(stmt, 2, topic.parent_id);
                } else {
                    sqlite3_bind_null(stmt, 2);
                }
                sqlite3_bind_text(stmt, 3, topic.title ? topic.title : "", -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 4, topic.summary ? topic.summary : "", -1, SQLITE_TRANSIENT);
                sqlite3_bind_int64(stmt, 5, topic.created_at ? topic.created_at : time(NULL));
                sqlite3_bind_int64(stmt, 6, topic.updated_at ? topic.updated_at : time(NULL));
                sqlite3_bind_int(stmt, 7, topic.position);

                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    result->topics_imported++;
                }
                sqlite3_finalize(stmt);
            }
        }
    }

    /* Import cards */
    size_t card_count = hr_json_array_size(cards);
    
    for (size_t i = 0; i < card_count; i++) {
        const HrJsonValue *card_json = hr_json_array_get(cards, i);
        if (!card_json) continue;

        HrCard card;
        if (!deserialize_card_from_json(card_json, &card, options->import_srs_state)) {
            continue;
        }

        /* Check if card with this UUID already exists */
        if (card.uuid && card.uuid[0] != '\0') {
            sqlite3_stmt *check_stmt = NULL;
            const char *check_sql = "SELECT COUNT(*) FROM cards WHERE uuid = ?";
            
            if (db_prepare(db, &check_stmt, check_sql) == SQLITE_OK) {
                sqlite3_bind_text(check_stmt, 1, card.uuid, -1, SQLITE_TRANSIENT);
                if (sqlite3_step(check_stmt) == SQLITE_ROW) {
                    int count = sqlite3_column_int(check_stmt, 0);
                    if (count > 0) {
                        sqlite3_finalize(check_stmt);
                        result->cards_skipped++;
                        continue;
                    }
                }
                sqlite3_finalize(check_stmt);
            }
        }

        /* Insert card */
        sqlite3_stmt *stmt = NULL;
        const char *sql = "INSERT INTO cards (uuid, topic_id, prompt, response, mnemonic, created_at, updated_at, "
                          "due_at, interval, ease_factor, review_state, suspended) "
                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        
        if (db_prepare(db, &stmt, sql) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, card.uuid ? card.uuid : "", -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 2, card.topic_id);
            sqlite3_bind_text(stmt, 3, card.prompt ? card.prompt : "", -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, card.response ? card.response : "", -1, SQLITE_TRANSIENT);
            if (card.mnemonic && card.mnemonic[0] != '\0') {
                sqlite3_bind_text(stmt, 5, card.mnemonic, -1, SQLITE_TRANSIENT);
            } else {
                sqlite3_bind_null(stmt, 5);
            }
            sqlite3_bind_int64(stmt, 6, card.created_at ? card.created_at : time(NULL));
            sqlite3_bind_int64(stmt, 7, card.updated_at ? card.updated_at : time(NULL));
            sqlite3_bind_int64(stmt, 8, card.due_at);
            sqlite3_bind_int(stmt, 9, card.interval);
            sqlite3_bind_int(stmt, 10, card.ease_factor);
            sqlite3_bind_int(stmt, 11, card.review_state);
            sqlite3_bind_int(stmt, 12, card.suspended ? 1 : 0);

            if (sqlite3_step(stmt) == SQLITE_DONE) {
                result->cards_imported++;
            }
            sqlite3_finalize(stmt);
        }
    }

    hr_json_free(root);

    /* Commit transaction */
    if (db_commit(db) != SQLITE_OK) {
        db_rollback(db);
        snprintf(result->error, sizeof(result->error), "Failed to commit transaction");
        return false;
    }

    result->success = true;
    return true;
}

bool hr_export_csv(struct DatabaseHandle *db, const char *output_path, HrExportResult *result)
{
    if (!db || !output_path || !result) {
        return false;
    }

    memset(result, 0, sizeof(*result));

    FILE *file = fopen(output_path, "w");
    if (!file) {
        snprintf(result->error, sizeof(result->error), "Failed to open output file");
        return false;
    }

    /* Write CSV header */
    fprintf(file, "id,topic_id,prompt,response,mnemonic,created_at,due_at\n");

    /* Query and write cards */
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id, topic_id, prompt, response, mnemonic, created_at, due_at FROM cards ORDER BY id";
    
    int rc = db_prepare(db, &stmt, sql);
    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            sqlite3_int64 id = sqlite3_column_int64(stmt, 0);
            sqlite3_int64 topic_id = sqlite3_column_int64(stmt, 1);
            const char *prompt = (const char *)sqlite3_column_text(stmt, 2);
            const char *response = (const char *)sqlite3_column_text(stmt, 3);
            const char *mnemonic = (const char *)sqlite3_column_text(stmt, 4);
            sqlite3_int64 created_at = sqlite3_column_int64(stmt, 5);
            sqlite3_int64 due_at = sqlite3_column_int64(stmt, 6);

            /* Simple CSV output - in a real implementation, would need proper escaping */
            fprintf(file, "%lld,%lld,\"%s\",\"%s\",\"%s\",%lld,%lld\n",
                    (long long)id,
                    (long long)topic_id,
                    prompt ? prompt : "",
                    response ? response : "",
                    mnemonic ? mnemonic : "",
                    (long long)created_at,
                    (long long)due_at);
            
            result->cards_exported++;
        }
        sqlite3_finalize(stmt);
    } else {
        fclose(file);
        snprintf(result->error, sizeof(result->error), "Failed to query cards");
        return false;
    }

    fclose(file);
    result->success = true;

    return true;
}

bool hr_import_csv(struct DatabaseHandle *db, const char *input_path, HrImportResult *result)
{
    if (!db || !input_path || !result) {
        return false;
    }

    memset(result, 0, sizeof(*result));

    FILE *file = fopen(input_path, "r");
    if (!file) {
        snprintf(result->error, sizeof(result->error), "Failed to open input file");
        return false;
    }

    char line[4096];
    bool first_line = true;

    /* Begin transaction */
    if (db_begin(db) != SQLITE_OK) {
        fclose(file);
        snprintf(result->error, sizeof(result->error), "Failed to begin transaction");
        return false;
    }

    while (fgets(line, sizeof(line), file)) {
        if (first_line) {
            first_line = false;
            continue; /* skip header */
        }

        /* Simple CSV parsing - in a real implementation, would need proper CSV parser */
        /* Format: id,topic_id,prompt,response,mnemonic,created_at,due_at */
        
        char *saveptr = NULL;
        char *id_str = strtok_r(line, ",", &saveptr);
        (void)id_str; /* Unused - we generate new IDs on import */
        char *topic_id_str = strtok_r(NULL, ",", &saveptr);
        char *prompt = strtok_r(NULL, ",", &saveptr);
        char *response = strtok_r(NULL, ",", &saveptr);
        char *mnemonic = strtok_r(NULL, ",", &saveptr);
        char *created_at_str = strtok_r(NULL, ",", &saveptr);
        char *due_at_str = strtok_r(NULL, ",\n", &saveptr);

        if (!topic_id_str || !prompt || !response) {
            continue; /* Invalid line */
        }

        /* Remove quotes from strings if present */
        if (prompt[0] == '"') prompt++;
        if (response[0] == '"') response++;
        if (mnemonic && mnemonic[0] == '"') mnemonic++;
        
        size_t len;
        len = strlen(prompt);
        if (len > 0 && prompt[len - 1] == '"') prompt[len - 1] = '\0';
        len = strlen(response);
        if (len > 0 && response[len - 1] == '"') response[len - 1] = '\0';
        if (mnemonic) {
            len = strlen(mnemonic);
            if (len > 0 && mnemonic[len - 1] == '"') mnemonic[len - 1] = '\0';
        }

        /* Insert card */
        sqlite3_stmt *stmt = NULL;
        const char *sql = "INSERT INTO cards (topic_id, uuid, prompt, response, mnemonic, created_at, updated_at, "
                          "due_at, interval, ease_factor, review_state, suspended) "
                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 0, 250, 0, 0)";
        
        if (db_prepare(db, &stmt, sql) == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, atoll(topic_id_str));
            sqlite3_bind_text(stmt, 2, "", -1, SQLITE_TRANSIENT); /* Generate UUID if needed */
            sqlite3_bind_text(stmt, 3, prompt, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, response, -1, SQLITE_TRANSIENT);
            if (mnemonic && mnemonic[0] != '\0') {
                sqlite3_bind_text(stmt, 5, mnemonic, -1, SQLITE_TRANSIENT);
            } else {
                sqlite3_bind_null(stmt, 5);
            }
            sqlite3_bind_int64(stmt, 6, created_at_str ? atoll(created_at_str) : time(NULL));
            sqlite3_bind_int64(stmt, 7, time(NULL));
            sqlite3_bind_int64(stmt, 8, due_at_str ? atoll(due_at_str) : 0);

            if (sqlite3_step(stmt) == SQLITE_DONE) {
                result->cards_imported++;
            }
            sqlite3_finalize(stmt);
        }
    }

    fclose(file);

    /* Commit transaction */
    if (db_commit(db) != SQLITE_OK) {
        db_rollback(db);
        snprintf(result->error, sizeof(result->error), "Failed to commit transaction");
        return false;
    }

    result->success = true;
    return true;
}

