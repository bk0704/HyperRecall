#include "import_export.h"

#include "model.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
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

#ifndef HR_ARRAY_GROW
#define HR_ARRAY_GROW 32U
#endif

#ifndef HR_JSON_STRING_INITIAL
#define HR_JSON_STRING_INITIAL 64U
#endif

#ifndef HR_IO_BUFFER_SIZE
#define HR_IO_BUFFER_SIZE 4096U
#endif

#ifndef HR_JSON_MAX_DEPTH
#define HR_JSON_MAX_DEPTH 64U
#endif

#ifndef HR_CSV_FIELD_INITIAL
#define HR_CSV_FIELD_INITIAL 64U
#endif

#ifndef HR_CSV_FIELDS_INITIAL
#define HR_CSV_FIELDS_INITIAL 8U
#endif

#ifndef HR_IO_SCHEMA
#define HR_IO_SCHEMA "hyperrecall.deck/1"
#endif

#ifndef HR_IO_MANIFEST
#define HR_IO_MANIFEST "manifest.json"
#endif

#ifndef HR_IO_TOPICS_CSV
#define HR_IO_TOPICS_CSV "topics.csv"
#endif

#ifndef HR_IO_CARDS_CSV
#define HR_IO_CARDS_CSV "cards.csv"
#endif

#ifndef HR_IO_REVIEWS_CSV
#define HR_IO_REVIEWS_CSV "reviews.csv"
#endif

#ifndef HR_IO_MEDIA_DIR
#define HR_IO_MEDIA_DIR "media"
#endif

#ifndef HR_IO_DEFAULT_EXPORT
#define HR_IO_DEFAULT_EXPORT "hyperrecall-export"
#endif

#ifndef HR_UUID_BUFFER
#define HR_UUID_BUFFER 64U
#endif

#ifndef HR_STRCMP
#ifdef _WIN32
#define HR_STRCMP _stricmp
#else
#define HR_STRCMP strcasecmp
#endif
#endif

#ifndef HR_STRDUP
#ifdef _WIN32
#define HR_STRDUP _strdup
#else
#define HR_STRDUP strdup
#endif
#endif

typedef enum HrJsonType {
    HR_JSON_NULL = 0,
    HR_JSON_BOOLEAN,
    HR_JSON_NUMBER,
    HR_JSON_STRING,
    HR_JSON_ARRAY,
    HR_JSON_OBJECT,
} HrJsonType;

typedef struct HrJsonValue HrJsonValue;

typedef struct HrJsonString {
    char *data;
    size_t length;
} HrJsonString;

typedef struct HrJsonArray {
    HrJsonValue *items;
    size_t count;
    size_t capacity;
} HrJsonArray;

typedef struct HrJsonObjectEntry {
    char *key;
    HrJsonValue value;
} HrJsonObjectEntry;

typedef struct HrJsonObject {
    HrJsonObjectEntry *entries;
    size_t count;
    size_t capacity;
} HrJsonObject;

struct HrJsonValue {
    HrJsonType type;
    union {
        double number;
        int boolean;
        HrJsonString string;
        HrJsonArray array;
        HrJsonObject object;
    } data;
};

typedef struct TopicRow {
    sqlite3_int64 id;
    sqlite3_int64 parent_id;
    char uuid[HR_UUID_BUFFER];
    char parent_uuid[HR_UUID_BUFFER];
    char *title;
    char *summary;
    sqlite3_int64 created_at;
    sqlite3_int64 updated_at;
    int position;
} TopicRow;

typedef struct CardRow {
    sqlite3_int64 id;
    sqlite3_int64 topic_id;
    char uuid[HR_UUID_BUFFER];
    char topic_uuid[HR_UUID_BUFFER];
    char *prompt;
    char *response;
    char *mnemonic;
    sqlite3_int64 created_at;
    sqlite3_int64 updated_at;
    sqlite3_int64 due_at;
    int interval;
    int ease_factor;
    int review_state;
    int suspended;
} CardRow;

typedef struct ReviewRow {
    sqlite3_int64 card_id;
    char card_uuid[HR_UUID_BUFFER];
    sqlite3_int64 reviewed_at;
    int rating;
    int duration_ms;
    int scheduled_interval;
    int actual_interval;
    int ease_factor;
    int review_state;
} ReviewRow;

typedef struct TopicRowArray {
    TopicRow *items;
    size_t count;
    size_t capacity;
} TopicRowArray;

typedef struct CardRowArray {
    CardRow *items;
    size_t count;
    size_t capacity;
} CardRowArray;

typedef struct ReviewRowArray {
    ReviewRow *items;
    size_t count;
    size_t capacity;
} ReviewRowArray;

typedef struct TopicIdEntry {
    char uuid[HR_UUID_BUFFER];
    sqlite3_int64 id;
    struct TopicIdEntry *next;
} TopicIdEntry;

typedef struct CardIdEntry {
    char uuid[HR_UUID_BUFFER];
    sqlite3_int64 id;
    struct CardIdEntry *next;
} CardIdEntry;

struct ImportExportContext {
    DatabaseHandle *database;
    ImportExportConfig config;
    ImportExportProgressCallback progress_callback;
    void *progress_user_data;
};

static void io_set_error(char *buffer, size_t capacity, const char *message)
{
    if (buffer == NULL || capacity == 0U) {
        return;
    }
    if (message == NULL) {
        buffer[0] = '\0';
        return;
    }
    strncpy(buffer, message, capacity - 1U);
    buffer[capacity - 1U] = '\0';
}

static void io_set_errorf(char *buffer, size_t capacity, const char *fmt, ...)
{
    if (buffer == NULL || capacity == 0U || fmt == NULL) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, capacity, fmt, args);
    va_end(args);
}

static void io_report_progress(struct ImportExportContext *context,
                               const char *phase,
                               size_t current,
                               size_t total)
{
    if (context == NULL || context->progress_callback == NULL) {
        return;
    }

    ImportExportProgress progress;
    progress.phase = phase;
    progress.current = current;
    progress.total = total;
    context->progress_callback(&progress, context->progress_user_data);
}

static int io_ensure_directory(const char *path)
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

static bool io_path_join(const char *lhs, const char *rhs, char *out_path, size_t capacity)
{
    if (out_path == NULL || capacity == 0U) {
        return false;
    }
    if (lhs == NULL || *lhs == '\0') {
        return snprintf(out_path, capacity, "%s", rhs != NULL ? rhs : "") < (int)capacity;
    }
    if (rhs == NULL || *rhs == '\0') {
        return snprintf(out_path, capacity, "%s", lhs) < (int)capacity;
    }

    size_t lhs_len = strlen(lhs);
    bool needs_sep = lhs[lhs_len - 1U] != '/' && lhs[lhs_len - 1U] != '\\';
    return snprintf(out_path, capacity, "%s%s%s", lhs, needs_sep ? "/" : "", rhs) < (int)capacity;
}

static bool io_copy_file(const char *source, const char *destination)
{
    if (source == NULL || destination == NULL) {
        return false;
    }

    FILE *src = fopen(source, "rb");
    if (src == NULL) {
        return false;
    }

    FILE *dst = fopen(destination, "wb");
    if (dst == NULL) {
        fclose(src);
        return false;
    }

    unsigned char buffer[HR_IO_BUFFER_SIZE];
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1U, sizeof(buffer), src)) > 0U) {
        size_t written = fwrite(buffer, 1U, read_bytes, dst);
        if (written != read_bytes) {
            fclose(src);
            fclose(dst);
            return false;
        }
    }

    fclose(src);
    fclose(dst);
    return true;
}

static bool io_file_exists(const char *path)
{
    if (path == NULL || *path == '\0') {
        return false;
    }
    return access(path, F_OK) == 0;
}

static bool io_compute_hash64(const char *path, uint64_t *out_hash, uint64_t *out_size)
{
    if (path == NULL || out_hash == NULL) {
        return false;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }

    const uint64_t kOffsetBasis = 1469598103934665603ULL;
    const uint64_t kPrime = 1099511628211ULL;
    uint64_t hash = kOffsetBasis;
    uint64_t size = 0U;

    unsigned char buffer[HR_IO_BUFFER_SIZE];
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1U, sizeof(buffer), file)) > 0U) {
        for (size_t i = 0; i < read_bytes; ++i) {
            hash ^= buffer[i];
            hash *= kPrime;
        }
        size += (uint64_t)read_bytes;
    }

    fclose(file);

    *out_hash = hash;
    if (out_size != NULL) {
        *out_size = size;
    }
    return true;
}

static void json_value_init(HrJsonValue *value)
{
    if (value == NULL) {
        return;
    }
    value->type = HR_JSON_NULL;
    value->data.number = 0.0;
}

static void json_free_value(HrJsonValue *value);

static void json_free_array(HrJsonArray *array)
{
    if (array == NULL) {
        return;
    }
    for (size_t i = 0; i < array->count; ++i) {
        json_free_value(&array->items[i]);
    }
    free(array->items);
    array->items = NULL;
    array->count = 0U;
    array->capacity = 0U;
}

static void json_free_object(HrJsonObject *object)
{
    if (object == NULL) {
        return;
    }
    for (size_t i = 0; i < object->count; ++i) {
        free(object->entries[i].key);
        json_free_value(&object->entries[i].value);
    }
    free(object->entries);
    object->entries = NULL;
    object->count = 0U;
    object->capacity = 0U;
}

static void json_free_value(HrJsonValue *value)
{
    if (value == NULL) {
        return;
    }
    switch (value->type) {
    case HR_JSON_STRING:
        free(value->data.string.data);
        value->data.string.data = NULL;
        value->data.string.length = 0U;
        break;
    case HR_JSON_ARRAY:
        json_free_array(&value->data.array);
        break;
    case HR_JSON_OBJECT:
        json_free_object(&value->data.object);
        break;
    default:
        break;
    }
    value->type = HR_JSON_NULL;
}
static void json_skip_whitespace(const char **cursor)
{
    if (cursor == NULL || *cursor == NULL) {
        return;
    }
    while (**cursor != '\0' && isspace((unsigned char)**cursor)) {
        ++(*cursor);
    }
}

static bool json_parse_string(const char **cursor, HrJsonString *out_string)
{
    if (cursor == NULL || *cursor == NULL || **cursor != '"' || out_string == NULL) {
        return false;
    }

    ++(*cursor);
    size_t capacity = HR_JSON_STRING_INITIAL;
    char *buffer = (char *)malloc(capacity);
    if (buffer == NULL) {
        return false;
    }
    size_t length = 0U;

    while (**cursor != '\0') {
        char c = **cursor;
        if (c == '"') {
            ++(*cursor);
            break;
        }
        if (c == '\\') {
            ++(*cursor);
            c = **cursor;
            if (c == '\0') {
                free(buffer);
                return false;
            }
            switch (c) {
            case '\\':
            case '"':
            case '/':
                break;
            case 'b':
                c = '\b';
                break;
            case 'f':
                c = '\f';
                break;
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            default:
                free(buffer);
                return false;
            }
        }

        if (length + 1U >= capacity) {
            size_t new_capacity = capacity * 2U;
            char *new_buffer = (char *)realloc(buffer, new_capacity);
            if (new_buffer == NULL) {
                free(buffer);
                return false;
            }
            buffer = new_buffer;
            capacity = new_capacity;
        }

        buffer[length++] = c;
        ++(*cursor);
    }

    buffer[length] = '\0';
    out_string->data = buffer;
    out_string->length = length;
    return true;
}

static bool json_parse_literal(const char **cursor, const char *literal)
{
    size_t len = strlen(literal);
    if (strncmp(*cursor, literal, len) != 0) {
        return false;
    }
    *cursor += len;
    return true;
}

static bool json_parse_number(const char **cursor, double *out_number)
{
    if (cursor == NULL || *cursor == NULL || out_number == NULL) {
        return false;
    }

    char *endptr = NULL;
    double value = strtod(*cursor, &endptr);
    if (endptr == *cursor) {
        return false;
    }
    *out_number = value;
    *cursor = endptr;
    return true;
}

static bool json_parse_value_internal(const char **cursor, HrJsonValue *out_value, unsigned depth)
{
    if (cursor == NULL || *cursor == NULL || out_value == NULL) {
        return false;
    }
    if (depth > HR_JSON_MAX_DEPTH) {
        return false;
    }

    json_skip_whitespace(cursor);
    char c = **cursor;
    if (c == '\0') {
        return false;
    }

    if (c == '"') {
        HrJsonString string;
        if (!json_parse_string(cursor, &string)) {
            return false;
        }
        out_value->type = HR_JSON_STRING;
        out_value->data.string = string;
        return true;
    }
    if (c == '{') {
        ++(*cursor);
        out_value->type = HR_JSON_OBJECT;
        out_value->data.object.entries = NULL;
        out_value->data.object.count = 0U;
        out_value->data.object.capacity = 0U;

        json_skip_whitespace(cursor);
        if (**cursor == '}') {
            ++(*cursor);
            return true;
        }

        while (**cursor != '\0') {
            HrJsonString key;
            if (!json_parse_string(cursor, &key)) {
                json_free_value(out_value);
                return false;
            }
            json_skip_whitespace(cursor);
            if (**cursor != ':') {
                free(key.data);
                json_free_value(out_value);
                return false;
            }
            ++(*cursor);

            HrJsonValue value;
            json_value_init(&value);
            if (!json_parse_value_internal(cursor, &value, depth + 1U)) {
                free(key.data);
                json_free_value(&value);
                json_free_value(out_value);
                return false;
            }

            HrJsonObject *object = &out_value->data.object;
            if (object->count + 1U > object->capacity) {
                size_t new_capacity = object->capacity == 0U ? HR_ARRAY_GROW : object->capacity * 2U;
                HrJsonObjectEntry *new_entries = (HrJsonObjectEntry *)realloc(object->entries,
                                                                             new_capacity * sizeof(HrJsonObjectEntry));
                if (new_entries == NULL) {
                    free(key.data);
                    json_free_value(&value);
                    json_free_value(out_value);
                    return false;
                }
                object->entries = new_entries;
                object->capacity = new_capacity;
            }

            object->entries[object->count].key = key.data;
            object->entries[object->count].value = value;
            object->count += 1U;

            json_skip_whitespace(cursor);
            if (**cursor == ',') {
                ++(*cursor);
                json_skip_whitespace(cursor);
                continue;
            }
            if (**cursor == '}') {
                ++(*cursor);
                return true;
            }
            json_free_value(out_value);
            return false;
        }
        json_free_value(out_value);
        return false;
    }
    if (c == '[') {
        ++(*cursor);
        out_value->type = HR_JSON_ARRAY;
        out_value->data.array.items = NULL;
        out_value->data.array.count = 0U;
        out_value->data.array.capacity = 0U;

        json_skip_whitespace(cursor);
        if (**cursor == ']') {
            ++(*cursor);
            return true;
        }

        while (**cursor != '\0') {
            HrJsonValue element;
            json_value_init(&element);
            if (!json_parse_value_internal(cursor, &element, depth + 1U)) {
                json_free_value(&element);
                json_free_value(out_value);
                return false;
            }

            HrJsonArray *array = &out_value->data.array;
            if (array->count + 1U > array->capacity) {
                size_t new_capacity = array->capacity == 0U ? HR_ARRAY_GROW : array->capacity * 2U;
                HrJsonValue *new_items = (HrJsonValue *)realloc(array->items, new_capacity * sizeof(HrJsonValue));
                if (new_items == NULL) {
                    json_free_value(&element);
                    json_free_value(out_value);
                    return false;
                }
                array->items = new_items;
                array->capacity = new_capacity;
            }

            array->items[array->count] = element;
            array->count += 1U;

            json_skip_whitespace(cursor);
            if (**cursor == ',') {
                ++(*cursor);
                json_skip_whitespace(cursor);
                continue;
            }
            if (**cursor == ']') {
                ++(*cursor);
                return true;
            }
            json_free_value(out_value);
            return false;
        }
        json_free_value(out_value);
        return false;
    }
    if (c == 't') {
        if (!json_parse_literal(cursor, "true")) {
            return false;
        }
        out_value->type = HR_JSON_BOOLEAN;
        out_value->data.boolean = 1;
        return true;
    }
    if (c == 'f') {
        if (!json_parse_literal(cursor, "false")) {
            return false;
        }
        out_value->type = HR_JSON_BOOLEAN;
        out_value->data.boolean = 0;
        return true;
    }
    if (c == 'n') {
        if (!json_parse_literal(cursor, "null")) {
            return false;
        }
        out_value->type = HR_JSON_NULL;
        return true;
    }

    double number = 0.0;
    if (!json_parse_number(cursor, &number)) {
        return false;
    }
    out_value->type = HR_JSON_NUMBER;
    out_value->data.number = number;
    return true;
}

static bool json_parse(const char *text, HrJsonValue *out_value)
{
    if (text == NULL || out_value == NULL) {
        return false;
    }
    const char *cursor = text;
    json_value_init(out_value);
    if (!json_parse_value_internal(&cursor, out_value, 0U)) {
        json_free_value(out_value);
        return false;
    }
    json_skip_whitespace(&cursor);
    if (*cursor != '\0') {
        json_free_value(out_value);
        return false;
    }
    return true;
}

static const HrJsonValue *json_object_get(const HrJsonValue *object, const char *key)
{
    if (object == NULL || object->type != HR_JSON_OBJECT || key == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < object->data.object.count; ++i) {
        if (strcmp(object->data.object.entries[i].key, key) == 0) {
            return &object->data.object.entries[i].value;
        }
    }
    return NULL;
}

static const char *json_value_get_string(const HrJsonValue *value)
{
    if (value == NULL || value->type != HR_JSON_STRING) {
        return NULL;
    }
    return value->data.string.data;
}

static bool json_value_get_boolean(const HrJsonValue *value, bool *out_boolean)
{
    if (value == NULL || value->type != HR_JSON_BOOLEAN || out_boolean == NULL) {
        return false;
    }
    *out_boolean = value->data.boolean != 0;
    return true;
}

static bool json_value_get_int64(const HrJsonValue *value, sqlite3_int64 *out_value)
{
    if (value == NULL || out_value == NULL) {
        return false;
    }
    if (value->type == HR_JSON_NUMBER) {
        *out_value = (sqlite3_int64)value->data.number;
        return true;
    }
    if (value->type == HR_JSON_STRING && value->data.string.data != NULL) {
        char *endptr = NULL;
        long long parsed = strtoll(value->data.string.data, &endptr, 10);
        if (endptr != value->data.string.data) {
            *out_value = (sqlite3_int64)parsed;
            return true;
        }
    }
    return false;
}

static bool json_value_get_int(const HrJsonValue *value, int *out_value)
{
    sqlite3_int64 temp;
    if (!json_value_get_int64(value, &temp)) {
        return false;
    }
    *out_value = (int)temp;
    return true;
}

static bool json_value_get_array(const HrJsonValue *value, const HrJsonArray **out_array)
{
    if (value == NULL || value->type != HR_JSON_ARRAY || out_array == NULL) {
        return false;
    }
    *out_array = &value->data.array;
    return true;
}

static void json_write_escaped(FILE *file, const char *text)
{
    if (file == NULL) {
        return;
    }
    if (text == NULL) {
        fputs("null", file);
        return;
    }
    fputc('"', file);
    while (*text != '\0') {
        unsigned char c = (unsigned char)*text;
        switch (c) {
        case '"':
            fputs("\\\"", file);
            break;
        case '\\':
            fputs("\\\\", file);
            break;
        case '\b':
            fputs("\\b", file);
            break;
        case '\f':
            fputs("\\f", file);
            break;
        case '\n':
            fputs("\\n", file);
            break;
        case '\r':
            fputs("\\r", file);
            break;
        case '\t':
            fputs("\\t", file);
            break;
        default:
            if (c < 0x20U) {
                fprintf(file, "\\u%04x", c);
            } else {
                fputc(c, file);
            }
            break;
        }
        ++text;
    }
    fputc('"', file);
}

static void csv_write_field(FILE *file, const char *text)
{
    if (file == NULL) {
        return;
    }
    if (text == NULL) {
        return;
    }
    bool needs_quotes = false;
    for (const char *ptr = text; *ptr != '\0'; ++ptr) {
        if (*ptr == ',' || *ptr == '"' || *ptr == '\n' || *ptr == '\r') {
            needs_quotes = true;
            break;
        }
    }
    if (!needs_quotes) {
        fputs(text, file);
        return;
    }
    fputc('"', file);
    for (const char *ptr = text; *ptr != '\0'; ++ptr) {
        if (*ptr == '"') {
            fputc('"', file);
        }
        fputc(*ptr, file);
    }
    fputc('"', file);
}

static bool csv_write_row(FILE *file, const char **fields, size_t field_count)
{
    if (file == NULL) {
        return false;
    }
    for (size_t i = 0; i < field_count; ++i) {
        if (i > 0U) {
            fputc(',', file);
        }
        csv_write_field(file, fields[i]);
    }
    fputc('\n', file);
    return ferror(file) == 0;
}

typedef struct CsvFieldArray {
    char **items;
    size_t count;
    size_t capacity;
} CsvFieldArray;

typedef struct CsvRow {
    CsvFieldArray fields;
} CsvRow;

typedef struct CsvRowArray {
    CsvRow *rows;
    size_t count;
    size_t capacity;
} CsvRowArray;

static void csv_field_array_free(CsvFieldArray *array)
{
    if (array == NULL) {
        return;
    }
    for (size_t i = 0; i < array->count; ++i) {
        free(array->items[i]);
    }
    free(array->items);
    array->items = NULL;
    array->count = 0U;
    array->capacity = 0U;
}

static void csv_row_array_free(CsvRowArray *array)
{
    if (array == NULL) {
        return;
    }
    for (size_t i = 0; i < array->count; ++i) {
        csv_field_array_free(&array->rows[i].fields);
    }
    free(array->rows);
    array->rows = NULL;
    array->count = 0U;
    array->capacity = 0U;
}

static bool csv_row_array_push(CsvRowArray *array, CsvRow *row)
{
    if (array == NULL || row == NULL) {
        return false;
    }
    if (array->count + 1U > array->capacity) {
        size_t new_capacity = array->capacity == 0U ? HR_ARRAY_GROW : array->capacity * 2U;
        CsvRow *new_rows = (CsvRow *)realloc(array->rows, new_capacity * sizeof(CsvRow));
        if (new_rows == NULL) {
            return false;
        }
        array->rows = new_rows;
        array->capacity = new_capacity;
    }
    array->rows[array->count] = *row;
    array->count += 1U;
    return true;
}

static bool csv_field_array_push(CsvFieldArray *array, char *value)
{
    if (array == NULL) {
        return false;
    }
    if (array->count + 1U > array->capacity) {
        size_t new_capacity = array->capacity == 0U ? HR_CSV_FIELDS_INITIAL : array->capacity * 2U;
        char **new_items = (char **)realloc(array->items, new_capacity * sizeof(char *));
        if (new_items == NULL) {
            return false;
        }
        array->items = new_items;
        array->capacity = new_capacity;
    }
    array->items[array->count] = value;
    array->count += 1U;
    return true;
}

static bool csv_parse_file(const char *path, CsvRowArray *out_rows)
{
    if (path == NULL || out_rows == NULL) {
        return false;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }

    CsvRowArray rows = {0};
    CsvRow current_row = {0};
    CsvFieldArray current_fields = {0};

    size_t field_capacity = HR_CSV_FIELD_INITIAL;
    char *field_buffer = (char *)malloc(field_capacity);
    if (field_buffer == NULL) {
        fclose(file);
        return false;
    }

    size_t field_length = 0U;
    bool in_quotes = false;
    bool field_started = false;

    for (;;) {
        int c = fgetc(file);
        if (c == EOF) {
            if (field_started || in_quotes || field_length > 0U) {
                field_buffer[field_length] = '\0';
                if (!csv_field_array_push(&current_fields, HR_STRDUP(field_buffer))) {
                    csv_field_array_free(&current_fields);
                    csv_row_array_free(&rows);
                    free(field_buffer);
                    fclose(file);
                    return false;
                }
            }
            if (current_fields.count > 0U || rows.count == 0U) {
                current_row.fields = current_fields;
                if (!csv_row_array_push(&rows, &current_row)) {
                    csv_field_array_free(&current_fields);
                    csv_row_array_free(&rows);
                    free(field_buffer);
                    fclose(file);
                    return false;
                }
            }
            break;
        }

        if (in_quotes) {
            if (c == '"') {
                int next = fgetc(file);
                if (next == '"') {
                    if (field_length + 2U >= field_capacity) {
                        size_t new_capacity = field_capacity * 2U;
                        char *new_buffer = (char *)realloc(field_buffer, new_capacity);
                        if (new_buffer == NULL) {
                            csv_field_array_free(&current_fields);
                            csv_row_array_free(&rows);
                            free(field_buffer);
                            fclose(file);
                            return false;
                        }
                        field_buffer = new_buffer;
                        field_capacity = new_capacity;
                    }
                    field_buffer[field_length++] = '"';
                } else {
                    in_quotes = false;
                    if (next != EOF) {
                        ungetc(next, file);
                    }
                }
            } else {
                if (field_length + 2U >= field_capacity) {
                    size_t new_capacity = field_capacity * 2U;
                    char *new_buffer = (char *)realloc(field_buffer, new_capacity);
                    if (new_buffer == NULL) {
                        csv_field_array_free(&current_fields);
                        csv_row_array_free(&rows);
                        free(field_buffer);
                        fclose(file);
                        return false;
                    }
                    field_buffer = new_buffer;
                    field_capacity = new_capacity;
                }
                field_buffer[field_length++] = (char)c;
            }
            continue;
        }

        if (c == '"') {
            in_quotes = true;
            field_started = true;
            continue;
        }
        if (c == ',') {
            field_buffer[field_length] = '\0';
            if (!csv_field_array_push(&current_fields, HR_STRDUP(field_buffer))) {
                csv_field_array_free(&current_fields);
                csv_row_array_free(&rows);
                free(field_buffer);
                fclose(file);
                return false;
            }
            field_length = 0U;
            field_started = false;
            continue;
        }
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            field_buffer[field_length] = '\0';
            if (!csv_field_array_push(&current_fields, HR_STRDUP(field_buffer))) {
                csv_field_array_free(&current_fields);
                csv_row_array_free(&rows);
                free(field_buffer);
                fclose(file);
                return false;
            }
            current_row.fields = current_fields;
            if (!csv_row_array_push(&rows, &current_row)) {
                csv_field_array_free(&current_fields);
                csv_row_array_free(&rows);
                free(field_buffer);
                fclose(file);
                return false;
            }
            current_fields.items = NULL;
            current_fields.count = 0U;
            current_fields.capacity = 0U;
            field_length = 0U;
            field_started = false;
            continue;
        }

        if (field_length + 2U >= field_capacity) {
            size_t new_capacity = field_capacity * 2U;
            char *new_buffer = (char *)realloc(field_buffer, new_capacity);
            if (new_buffer == NULL) {
                csv_field_array_free(&current_fields);
                csv_row_array_free(&rows);
                free(field_buffer);
                fclose(file);
                return false;
            }
            field_buffer = new_buffer;
            field_capacity = new_capacity;
        }
        field_buffer[field_length++] = (char)c;
        field_started = true;
    }

    free(field_buffer);
    fclose(file);

    *out_rows = rows;
    return true;
}
static bool topic_id_map_find(const TopicIdEntry *head, const char *uuid, sqlite3_int64 *out_id)
{
    if (uuid == NULL || head == NULL) {
        return false;
    }
    const TopicIdEntry *entry = head;
    while (entry != NULL) {
        if (strcmp(entry->uuid, uuid) == 0) {
            if (out_id != NULL) {
                *out_id = entry->id;
            }
            return true;
        }
        entry = entry->next;
    }
    return false;
}

static bool topic_id_map_add(TopicIdEntry **head, const char *uuid, sqlite3_int64 id)
{
    if (head == NULL || uuid == NULL || *uuid == '\0') {
        return false;
    }

    TopicIdEntry *existing = *head;
    while (existing != NULL) {
        if (strcmp(existing->uuid, uuid) == 0) {
            existing->id = id;
            return true;
        }
        existing = existing->next;
    }

    TopicIdEntry *entry = (TopicIdEntry *)malloc(sizeof(TopicIdEntry));
    if (entry == NULL) {
        return false;
    }
    strncpy(entry->uuid, uuid, sizeof(entry->uuid) - 1U);
    entry->uuid[sizeof(entry->uuid) - 1U] = '\0';
    entry->id = id;
    entry->next = *head;
    *head = entry;
    return true;
}

static void topic_id_map_free(TopicIdEntry *head)
{
    TopicIdEntry *entry = head;
    while (entry != NULL) {
        TopicIdEntry *next = entry->next;
        free(entry);
        entry = next;
    }
}

static bool card_id_map_find(const CardIdEntry *head, const char *uuid, sqlite3_int64 *out_id)
{
    if (uuid == NULL || head == NULL) {
        return false;
    }
    const CardIdEntry *entry = head;
    while (entry != NULL) {
        if (strcmp(entry->uuid, uuid) == 0) {
            if (out_id != NULL) {
                *out_id = entry->id;
            }
            return true;
        }
        entry = entry->next;
    }
    return false;
}

static bool card_id_map_add(CardIdEntry **head, const char *uuid, sqlite3_int64 id)
{
    if (head == NULL || uuid == NULL || *uuid == '\0') {
        return false;
    }
    CardIdEntry *existing = *head;
    while (existing != NULL) {
        if (strcmp(existing->uuid, uuid) == 0) {
            existing->id = id;
            return true;
        }
        existing = existing->next;
    }

    CardIdEntry *entry = (CardIdEntry *)malloc(sizeof(CardIdEntry));
    if (entry == NULL) {
        return false;
    }
    strncpy(entry->uuid, uuid, sizeof(entry->uuid) - 1U);
    entry->uuid[sizeof(entry->uuid) - 1U] = '\0';
    entry->id = id;
    entry->next = *head;
    *head = entry;
    return true;
}

static void card_id_map_free(CardIdEntry *head)
{
    CardIdEntry *entry = head;
    while (entry != NULL) {
        CardIdEntry *next = entry->next;
        free(entry);
        entry = next;
    }
}

static bool load_text_file(const char *path, char **out_text, size_t *out_size)
{
    if (path == NULL || out_text == NULL) {
        return false;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }
    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return false;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }

    char *buffer = (char *)malloc((size_t)size + 1U);
    if (buffer == NULL) {
        fclose(file);
        return false;
    }

    size_t read_bytes = fread(buffer, 1U, (size_t)size, file);
    fclose(file);
    if (read_bytes != (size_t)size) {
        free(buffer);
        return false;
    }

    buffer[size] = '\0';
    *out_text = buffer;
    if (out_size != NULL) {
        *out_size = (size_t)size;
    }
    return true;
}

static bool copy_media_file(const char *source_path,
                            const char *destination_dir,
                            const char *file_name,
                            bool dedupe_existing,
                            ImportExportStats *stats)
{
    char destination_path[PATH_MAX];
    if (!io_path_join(destination_dir, file_name, destination_path, sizeof(destination_path))) {
        return false;
    }

    if (dedupe_existing && io_file_exists(destination_path)) {
        uint64_t src_hash = 0U;
        uint64_t dst_hash = 0U;
        if (io_compute_hash64(source_path, &src_hash, NULL) && io_compute_hash64(destination_path, &dst_hash, NULL) &&
            src_hash == dst_hash) {
            if (stats != NULL) {
                stats->media_deduplicated += 1U;
            }
            return true;
        }

        char base[PATH_MAX];
        char ext[PATH_MAX];
        const char *dot = strrchr(file_name, '.');
        if (dot != NULL) {
            size_t base_len = (size_t)(dot - file_name);
            if (base_len >= sizeof(base)) {
                base_len = sizeof(base) - 1U;
            }
            memcpy(base, file_name, base_len);
            base[base_len] = '\0';
            strncpy(ext, dot, sizeof(ext) - 1U);
            ext[sizeof(ext) - 1U] = '\0';
        } else {
            strncpy(base, file_name, sizeof(base) - 1U);
            base[sizeof(base) - 1U] = '\0';
            ext[0] = '\0';
        }

        for (unsigned counter = 1U; counter < 1000U; ++counter) {
            char candidate[PATH_MAX];
            snprintf(candidate, sizeof(candidate), "%s_%u%s", base, counter, ext);
            if (!io_path_join(destination_dir, candidate, destination_path, sizeof(destination_path))) {
                return false;
            }
            if (!io_file_exists(destination_path)) {
                file_name = candidate;
                break;
            }
        }
    }

    if (!io_copy_file(source_path, destination_path)) {
        return false;
    }

    if (stats != NULL) {
        stats->media_copied += 1U;
    }
    return true;
}

static bool copy_media_directory(struct ImportExportContext *context,
                                 const char *source_dir,
                                 const char *destination_dir,
                                 bool dedupe_existing,
                                 ImportExportStats *stats,
                                 char *error_message,
                                 size_t error_capacity)
{
    (void)context;
    if (source_dir == NULL || destination_dir == NULL) {
        io_set_error(error_message, error_capacity, "Invalid media directory");
        return false;
    }

    if (!io_file_exists(source_dir)) {
        return true;
    }

    if (io_ensure_directory(destination_dir) != 0) {
        io_set_errorf(error_message, error_capacity, "Failed to create %s", destination_dir);
        return false;
    }

#ifdef _WIN32
    char pattern[PATH_MAX];
    if (!io_path_join(source_dir, "*", pattern, sizeof(pattern))) {
        io_set_error(error_message, error_capacity, "Invalid media pattern");
        return false;
    }

    WIN32_FIND_DATAA find_data;
    HANDLE handle = FindFirstFileA(pattern, &find_data);
    if (handle == INVALID_HANDLE_VALUE) {
        return true;
    }

    do {
        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            continue;
        }
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }

        char source_path[PATH_MAX];
        if (!io_path_join(source_dir, find_data.cFileName, source_path, sizeof(source_path))) {
            FindClose(handle);
            io_set_error(error_message, error_capacity, "Failed to join media path");
            return false;
        }

        if (!copy_media_file(source_path, destination_dir, find_data.cFileName, dedupe_existing, stats)) {
            FindClose(handle);
            io_set_errorf(error_message, error_capacity, "Failed to copy media %s", find_data.cFileName);
            return false;
        }
    } while (FindNextFileA(handle, &find_data));

    FindClose(handle);
#else
    DIR *dir = opendir(source_dir);
    if (dir == NULL) {
        return false;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char source_path[PATH_MAX];
        if (!io_path_join(source_dir, entry->d_name, source_path, sizeof(source_path))) {
            closedir(dir);
            io_set_error(error_message, error_capacity, "Failed to join media path");
            return false;
        }

        struct stat st;
        if (stat(source_path, &st) != 0 || !S_ISREG(st.st_mode)) {
            continue;
        }

        if (!copy_media_file(source_path, destination_dir, entry->d_name, dedupe_existing, stats)) {
            closedir(dir);
            io_set_errorf(error_message, error_capacity, "Failed to copy media %s", entry->d_name);
            return false;
        }
    }

    closedir(dir);
#endif

    return true;
}

static bool collect_topics(struct ImportExportContext *context,
                           TopicRowArray *topics,
                           char *error_message,
                           size_t error_capacity)
{
    if (context == NULL || topics == NULL) {
        io_set_error(error_message, error_capacity, "Invalid context");
        return false;
    }
    sqlite3 *db = db_connection(context->database);
    if (db == NULL) {
        io_set_error(error_message, error_capacity, "Database unavailable");
        return false;
    }

    const char *sql = "SELECT id, parent_id, uuid, title, summary, created_at, updated_at, position FROM topics ORDER BY id;";
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        io_set_errorf(error_message, error_capacity, "Failed to prepare topic query (%d)", rc);
        return false;
    }

    TopicRowArray array = {0};

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        TopicRow row;
        memset(&row, 0, sizeof(row));
        row.id = sqlite3_column_int64(stmt, 0);
        row.parent_id = sqlite3_column_type(stmt, 1) == SQLITE_NULL ? 0 : sqlite3_column_int64(stmt, 1);
        const unsigned char *uuid_text = sqlite3_column_text(stmt, 2);
        const unsigned char *title_text = sqlite3_column_text(stmt, 3);
        const unsigned char *summary_text = sqlite3_column_text(stmt, 4);
        row.created_at = sqlite3_column_int64(stmt, 5);
        row.updated_at = sqlite3_column_int64(stmt, 6);
        row.position = sqlite3_column_int(stmt, 7);

        if (uuid_text != NULL) {
            strncpy(row.uuid, (const char *)uuid_text, sizeof(row.uuid) - 1U);
            row.uuid[sizeof(row.uuid) - 1U] = '\0';
        } else {
            row.uuid[0] = '\0';
        }

        row.title = title_text != NULL ? HR_STRDUP((const char *)title_text) : HR_STRDUP("");
        row.summary = summary_text != NULL ? HR_STRDUP((const char *)summary_text) : HR_STRDUP("");

        if (array.count + 1U > array.capacity) {
            size_t new_capacity = array.capacity == 0U ? HR_ARRAY_GROW : array.capacity * 2U;
            TopicRow *new_items = (TopicRow *)realloc(array.items, new_capacity * sizeof(TopicRow));
            if (new_items == NULL) {
                sqlite3_finalize(stmt);
                for (size_t i = 0; i < array.count; ++i) {
                    free(array.items[i].title);
                    free(array.items[i].summary);
                }
                free(array.items);
                io_set_error(error_message, error_capacity, "Out of memory");
                return false;
            }
            array.items = new_items;
            array.capacity = new_capacity;
        }
        array.items[array.count++] = row;
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        for (size_t i = 0; i < array.count; ++i) {
            free(array.items[i].title);
            free(array.items[i].summary);
        }
        free(array.items);
        io_set_errorf(error_message, error_capacity, "Topic query failed (%d)", rc);
        return false;
    }

    for (size_t i = 0; i < array.count; ++i) {
        array.items[i].parent_uuid[0] = '\0';
        if (array.items[i].parent_id <= 0) {
            continue;
        }
        for (size_t j = 0; j < array.count; ++j) {
            if (array.items[j].id == array.items[i].parent_id) {
                strncpy(array.items[i].parent_uuid, array.items[j].uuid, sizeof(array.items[i].parent_uuid) - 1U);
                array.items[i].parent_uuid[sizeof(array.items[i].parent_uuid) - 1U] = '\0';
                break;
            }
        }
    }

    *topics = array;
    return true;
}

static void free_topics(TopicRowArray *topics)
{
    if (topics == NULL) {
        return;
    }
    for (size_t i = 0; i < topics->count; ++i) {
        free(topics->items[i].title);
        free(topics->items[i].summary);
    }
    free(topics->items);
    topics->items = NULL;
    topics->count = 0U;
    topics->capacity = 0U;
}

static bool collect_cards(struct ImportExportContext *context,
                          CardRowArray *cards,
                          char *error_message,
                          size_t error_capacity)
{
    if (context == NULL || cards == NULL) {
        io_set_error(error_message, error_capacity, "Invalid context");
        return false;
    }
    sqlite3 *db = db_connection(context->database);
    if (db == NULL) {
        io_set_error(error_message, error_capacity, "Database unavailable");
        return false;
    }

    const char *sql =
        "SELECT c.id, c.topic_id, c.uuid, t.uuid, c.prompt, c.response, c.mnemonic, c.created_at, c.updated_at, c.due_at, c.interval, "
        "c.ease_factor, c.review_state, c.suspended FROM cards c JOIN topics t ON t.id = c.topic_id ORDER BY c.id;";
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        io_set_errorf(error_message, error_capacity, "Failed to prepare card query (%d)", rc);
        return false;
    }

    CardRowArray array = {0};

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        CardRow row;
        memset(&row, 0, sizeof(row));
        row.id = sqlite3_column_int64(stmt, 0);
        row.topic_id = sqlite3_column_int64(stmt, 1);
        const unsigned char *uuid_text = sqlite3_column_text(stmt, 2);
        const unsigned char *topic_uuid_text = sqlite3_column_text(stmt, 3);
        const unsigned char *prompt_text = sqlite3_column_text(stmt, 4);
        const unsigned char *response_text = sqlite3_column_text(stmt, 5);
        const unsigned char *mnemonic_text = sqlite3_column_text(stmt, 6);
        row.created_at = sqlite3_column_int64(stmt, 7);
        row.updated_at = sqlite3_column_int64(stmt, 8);
        row.due_at = sqlite3_column_int64(stmt, 9);
        row.interval = sqlite3_column_int(stmt, 10);
        row.ease_factor = sqlite3_column_int(stmt, 11);
        row.review_state = sqlite3_column_int(stmt, 12);
        row.suspended = sqlite3_column_int(stmt, 13);

        if (uuid_text != NULL) {
            strncpy(row.uuid, (const char *)uuid_text, sizeof(row.uuid) - 1U);
            row.uuid[sizeof(row.uuid) - 1U] = '\0';
        } else {
            row.uuid[0] = '\0';
        }
        if (topic_uuid_text != NULL) {
            strncpy(row.topic_uuid, (const char *)topic_uuid_text, sizeof(row.topic_uuid) - 1U);
            row.topic_uuid[sizeof(row.topic_uuid) - 1U] = '\0';
        } else {
            row.topic_uuid[0] = '\0';
        }

        row.prompt = prompt_text != NULL ? HR_STRDUP((const char *)prompt_text) : HR_STRDUP("");
        row.response = response_text != NULL ? HR_STRDUP((const char *)response_text) : HR_STRDUP("");
        row.mnemonic = mnemonic_text != NULL ? HR_STRDUP((const char *)mnemonic_text) : NULL;

        if (array.count + 1U > array.capacity) {
            size_t new_capacity = array.capacity == 0U ? HR_ARRAY_GROW : array.capacity * 2U;
            CardRow *new_items = (CardRow *)realloc(array.items, new_capacity * sizeof(CardRow));
            if (new_items == NULL) {
                sqlite3_finalize(stmt);
                for (size_t i = 0; i < array.count; ++i) {
                    free(array.items[i].prompt);
                    free(array.items[i].response);
                    free(array.items[i].mnemonic);
                }
                free(array.items);
                io_set_error(error_message, error_capacity, "Out of memory");
                return false;
            }
            array.items = new_items;
            array.capacity = new_capacity;
        }
        array.items[array.count++] = row;
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        for (size_t i = 0; i < array.count; ++i) {
            free(array.items[i].prompt);
            free(array.items[i].response);
            free(array.items[i].mnemonic);
        }
        free(array.items);
        io_set_errorf(error_message, error_capacity, "Card query failed (%d)", rc);
        return false;
    }

    *cards = array;
    return true;
}

static void free_cards(CardRowArray *cards)
{
    if (cards == NULL) {
        return;
    }
    for (size_t i = 0; i < cards->count; ++i) {
        free(cards->items[i].prompt);
        free(cards->items[i].response);
        free(cards->items[i].mnemonic);
    }
    free(cards->items);
    cards->items = NULL;
    cards->count = 0U;
    cards->capacity = 0U;
}

static bool collect_reviews(struct ImportExportContext *context,
                            ReviewRowArray *reviews,
                            char *error_message,
                            size_t error_capacity)
{
    if (context == NULL || reviews == NULL) {
        io_set_error(error_message, error_capacity, "Invalid context");
        return false;
    }
    if (!context->config.include_reviews) {
        reviews->items = NULL;
        reviews->count = 0U;
        reviews->capacity = 0U;
        return true;
    }

    sqlite3 *db = db_connection(context->database);
    if (db == NULL) {
        io_set_error(error_message, error_capacity, "Database unavailable");
        return false;
    }

    const char *sql =
        "SELECT r.card_id, c.uuid, r.reviewed_at, r.rating, r.duration_ms, r.scheduled_interval, r.actual_interval, r.ease_factor, r.review_state "
        "FROM reviews r JOIN cards c ON c.id = r.card_id ORDER BY r.reviewed_at;";
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        io_set_errorf(error_message, error_capacity, "Failed to prepare review query (%d)", rc);
        return false;
    }

    ReviewRowArray array = {0};

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        ReviewRow row;
        memset(&row, 0, sizeof(row));
        row.card_id = sqlite3_column_int64(stmt, 0);
        const unsigned char *uuid_text = sqlite3_column_text(stmt, 1);
        row.reviewed_at = sqlite3_column_int64(stmt, 2);
        row.rating = sqlite3_column_int(stmt, 3);
        row.duration_ms = sqlite3_column_int(stmt, 4);
        row.scheduled_interval = sqlite3_column_int(stmt, 5);
        row.actual_interval = sqlite3_column_int(stmt, 6);
        row.ease_factor = sqlite3_column_int(stmt, 7);
        row.review_state = sqlite3_column_int(stmt, 8);
        if (uuid_text != NULL) {
            strncpy(row.card_uuid, (const char *)uuid_text, sizeof(row.card_uuid) - 1U);
            row.card_uuid[sizeof(row.card_uuid) - 1U] = '\0';
        } else {
            row.card_uuid[0] = '\0';
        }

        if (array.count + 1U > array.capacity) {
            size_t new_capacity = array.capacity == 0U ? HR_ARRAY_GROW : array.capacity * 2U;
            ReviewRow *new_items = (ReviewRow *)realloc(array.items, new_capacity * sizeof(ReviewRow));
            if (new_items == NULL) {
                sqlite3_finalize(stmt);
                free(array.items);
                io_set_error(error_message, error_capacity, "Out of memory");
                return false;
            }
            array.items = new_items;
            array.capacity = new_capacity;
        }
        array.items[array.count++] = row;
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        free(array.items);
        io_set_errorf(error_message, error_capacity, "Review query failed (%d)", rc);
        return false;
    }

    *reviews = array;
    return true;
}

static void free_reviews(ReviewRowArray *reviews)
{
    if (reviews == NULL) {
        return;
    }
    free(reviews->items);
    reviews->items = NULL;
    reviews->count = 0U;
    reviews->capacity = 0U;
}

static bool write_manifest(const char *path,
                           const TopicRowArray *topics,
                           const CardRowArray *cards,
                           const ReviewRowArray *reviews,
                           char *error_message,
                           size_t error_capacity)
{
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        io_set_errorf(error_message, error_capacity, "Failed to open manifest %s", path);
        return false;
    }

    time_t now = time(NULL);
    fprintf(file, "{\n");
    fprintf(file, "  \"schema\": ");
    json_write_escaped(file, HR_IO_SCHEMA);
    fprintf(file, ",\n");
    fprintf(file, "  \"exported_at\": %lld,\n", (long long)now);

    fprintf(file, "  \"topics\": [\n");
    for (size_t i = 0; i < topics->count; ++i) {
        const TopicRow *row = &topics->items[i];
        fprintf(file, "    {\"uuid\": ");
        json_write_escaped(file, row->uuid);
        fprintf(file, ", \"parent_uuid\": ");
        if (row->parent_uuid[0] != '\0') {
            json_write_escaped(file, row->parent_uuid);
        } else {
            fputs("null", file);
        }
        fprintf(file, ", \"title\": ");
        json_write_escaped(file, row->title);
        fprintf(file, ", \"summary\": ");
        json_write_escaped(file, row->summary);
        fprintf(file, ", \"created_at\": %lld, \"updated_at\": %lld, \"position\": %d}",
                (long long)row->created_at,
                (long long)row->updated_at,
                row->position);
        if (i + 1U < topics->count) {
            fputs(",", file);
        }
        fputs("\n", file);
    }
    fprintf(file, "  ],\n");

    fprintf(file, "  \"cards\": [\n");
    for (size_t i = 0; i < cards->count; ++i) {
        const CardRow *row = &cards->items[i];
        fprintf(file, "    {\"uuid\": ");
        json_write_escaped(file, row->uuid);
        fprintf(file, ", \"topic_uuid\": ");
        json_write_escaped(file, row->topic_uuid);
        fprintf(file, ", \"type\": ");
        json_write_escaped(file, hr_card_type_to_string(HR_CARD_TYPE_SHORT_ANSWER));
        fprintf(file, ", \"prompt\": ");
        json_write_escaped(file, row->prompt);
        fprintf(file, ", \"response\": ");
        json_write_escaped(file, row->response);
        fprintf(file, ", \"mnemonic\": ");
        if (row->mnemonic != NULL) {
            json_write_escaped(file, row->mnemonic);
        } else {
            fputs("null", file);
        }
        fprintf(file, ", \"created_at\": %lld, \"updated_at\": %lld, \"due_at\": %lld, \"interval\": %d, \"ease_factor\": %d, \"review_state\": %d, \"suspended\": %s}",
                (long long)row->created_at,
                (long long)row->updated_at,
                (long long)row->due_at,
                row->interval,
                row->ease_factor,
                row->review_state,
                row->suspended ? "true" : "false");
        if (i + 1U < cards->count) {
            fputs(",", file);
        }
        fputs("\n", file);
    }
    fprintf(file, "  ]");

    if (reviews != NULL && reviews->count > 0U) {
        fprintf(file, ",\n  \"reviews\": [\n");
        for (size_t i = 0; i < reviews->count; ++i) {
            const ReviewRow *row = &reviews->items[i];
            fprintf(file, "    {\"card_uuid\": ");
            json_write_escaped(file, row->card_uuid);
            fprintf(file,
                    ", \"reviewed_at\": %lld, \"rating\": %d, \"duration_ms\": %d, \"scheduled_interval\": %d, \"actual_interval\": %d, \"ease_factor\": %d, \"review_state\": %d}",
                    (long long)row->reviewed_at,
                    row->rating,
                    row->duration_ms,
                    row->scheduled_interval,
                    row->actual_interval,
                    row->ease_factor,
                    row->review_state);
            if (i + 1U < reviews->count) {
                fputs(",", file);
            }
            fputs("\n", file);
        }
        fprintf(file, "  ]");
    }

    fprintf(file, "\n}\n");

    if (ferror(file)) {
        fclose(file);
        io_set_error(error_message, error_capacity, "Failed to write manifest");
        return false;
    }

    fclose(file);
    return true;
}

static bool write_topics_csv(const char *path,
                             const TopicRowArray *topics,
                             char *error_message,
                             size_t error_capacity)
{
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        io_set_errorf(error_message, error_capacity, "Unable to open %s", path);
        return false;
    }

    const char *header[] = {"uuid", "parent_uuid", "title", "summary", "created_at", "updated_at", "position"};
    csv_write_row(file, header, sizeof(header) / sizeof(header[0]));

    for (size_t i = 0; i < topics->count; ++i) {
        const TopicRow *row = &topics->items[i];
        char created_buffer[32];
        char updated_buffer[32];
        char position_buffer[16];
        snprintf(created_buffer, sizeof(created_buffer), "%lld", (long long)row->created_at);
        snprintf(updated_buffer, sizeof(updated_buffer), "%lld", (long long)row->updated_at);
        snprintf(position_buffer, sizeof(position_buffer), "%d", row->position);

        const char *fields[] = {row->uuid,
                                row->parent_uuid[0] != '\0' ? row->parent_uuid : "",
                                row->title,
                                row->summary,
                                created_buffer,
                                updated_buffer,
                                position_buffer};
        if (!csv_write_row(file, fields, sizeof(fields) / sizeof(fields[0]))) {
            fclose(file);
            io_set_error(error_message, error_capacity, "Failed to write topics CSV");
            return false;
        }
    }

    fclose(file);
    return true;
}

static bool write_cards_csv(const char *path,
                            const CardRowArray *cards,
                            char *error_message,
                            size_t error_capacity)
{
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        io_set_errorf(error_message, error_capacity, "Unable to open %s", path);
        return false;
    }

    const char *header[] = {"uuid",
                             "topic_uuid",
                             "prompt",
                             "response",
                             "mnemonic",
                             "created_at",
                             "updated_at",
                             "due_at",
                             "interval",
                             "ease_factor",
                             "review_state",
                             "suspended"};
    csv_write_row(file, header, sizeof(header) / sizeof(header[0]));

    for (size_t i = 0; i < cards->count; ++i) {
        const CardRow *row = &cards->items[i];
        char created_buffer[32];
        char updated_buffer[32];
        char due_buffer[32];
        char interval_buffer[16];
        char ease_buffer[16];
        char review_buffer[16];
        snprintf(created_buffer, sizeof(created_buffer), "%lld", (long long)row->created_at);
        snprintf(updated_buffer, sizeof(updated_buffer), "%lld", (long long)row->updated_at);
        snprintf(due_buffer, sizeof(due_buffer), "%lld", (long long)row->due_at);
        snprintf(interval_buffer, sizeof(interval_buffer), "%d", row->interval);
        snprintf(ease_buffer, sizeof(ease_buffer), "%d", row->ease_factor);
        snprintf(review_buffer, sizeof(review_buffer), "%d", row->review_state);
        const char *fields[] = {row->uuid,
                                row->topic_uuid,
                                row->prompt,
                                row->response,
                                row->mnemonic != NULL ? row->mnemonic : "",
                                created_buffer,
                                updated_buffer,
                                due_buffer,
                                interval_buffer,
                                ease_buffer,
                                review_buffer,
                                row->suspended ? "1" : "0"};
        if (!csv_write_row(file, fields, sizeof(fields) / sizeof(fields[0]))) {
            fclose(file);
            io_set_error(error_message, error_capacity, "Failed to write cards CSV");
            return false;
        }
    }

    fclose(file);
    return true;
}

static bool write_reviews_csv(const char *path,
                              const ReviewRowArray *reviews,
                              char *error_message,
                              size_t error_capacity)
{
    if (reviews == NULL || reviews->count == 0U) {
        return true;
    }

    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        io_set_errorf(error_message, error_capacity, "Unable to open %s", path);
        return false;
    }

    const char *header[] = {"card_uuid",
                             "reviewed_at",
                             "rating",
                             "duration_ms",
                             "scheduled_interval",
                             "actual_interval",
                             "ease_factor",
                             "review_state"};
    csv_write_row(file, header, sizeof(header) / sizeof(header[0]));

    for (size_t i = 0; i < reviews->count; ++i) {
        const ReviewRow *row = &reviews->items[i];
        char reviewed_buffer[32];
        char rating_buffer[16];
        char duration_buffer[16];
        char scheduled_buffer[16];
        char actual_buffer[16];
        char ease_buffer[16];
        char state_buffer[16];
        snprintf(reviewed_buffer, sizeof(reviewed_buffer), "%lld", (long long)row->reviewed_at);
        snprintf(rating_buffer, sizeof(rating_buffer), "%d", row->rating);
        snprintf(duration_buffer, sizeof(duration_buffer), "%d", row->duration_ms);
        snprintf(scheduled_buffer, sizeof(scheduled_buffer), "%d", row->scheduled_interval);
        snprintf(actual_buffer, sizeof(actual_buffer), "%d", row->actual_interval);
        snprintf(ease_buffer, sizeof(ease_buffer), "%d", row->ease_factor);
        snprintf(state_buffer, sizeof(state_buffer), "%d", row->review_state);
        const char *fields[] = {row->card_uuid,
                                reviewed_buffer,
                                rating_buffer,
                                duration_buffer,
                                scheduled_buffer,
                                actual_buffer,
                                ease_buffer,
                                state_buffer};
        if (!csv_write_row(file, fields, sizeof(fields) / sizeof(fields[0]))) {
            fclose(file);
            io_set_error(error_message, error_capacity, "Failed to write reviews CSV");
            return false;
        }
    }

    fclose(file);
    return true;
}

static bool parse_topics_json(const HrJsonArray *array,
                              TopicRowArray *topics,
                              char *error_message,
                              size_t error_capacity)
{
    TopicRowArray out = {0};
    for (size_t i = 0; i < array->count; ++i) {
        const HrJsonValue *value = &array->items[i];
        if (value->type != HR_JSON_OBJECT) {
            free_topics(&out);
            io_set_error(error_message, error_capacity, "Topic entry must be an object");
            return false;
        }
        const HrJsonValue *uuid_value = json_object_get(value, "uuid");
        const HrJsonValue *parent_value = json_object_get(value, "parent_uuid");
        const HrJsonValue *title_value = json_object_get(value, "title");
        const HrJsonValue *summary_value = json_object_get(value, "summary");
        const HrJsonValue *created_value = json_object_get(value, "created_at");
        const HrJsonValue *updated_value = json_object_get(value, "updated_at");
        const HrJsonValue *position_value = json_object_get(value, "position");
        if (uuid_value == NULL || title_value == NULL || created_value == NULL || updated_value == NULL ||
            position_value == NULL) {
            free_topics(&out);
            io_set_error(error_message, error_capacity, "Topic entry missing required fields");
            return false;
        }

        TopicRow row;
        memset(&row, 0, sizeof(row));
        const char *uuid_text = json_value_get_string(uuid_value);
        if (uuid_text == NULL) {
            free_topics(&out);
            io_set_error(error_message, error_capacity, "Topic uuid must be a string");
            return false;
        }
        strncpy(row.uuid, uuid_text, sizeof(row.uuid) - 1U);
        row.uuid[sizeof(row.uuid) - 1U] = '\0';

        if (parent_value != NULL && parent_value->type == HR_JSON_STRING && parent_value->data.string.data != NULL) {
            strncpy(row.parent_uuid, parent_value->data.string.data, sizeof(row.parent_uuid) - 1U);
            row.parent_uuid[sizeof(row.parent_uuid) - 1U] = '\0';
        } else {
            row.parent_uuid[0] = '\0';
        }

        const char *title_text = json_value_get_string(title_value);
        row.title = title_text != NULL ? HR_STRDUP(title_text) : HR_STRDUP("");
        const char *summary_text = summary_value != NULL ? json_value_get_string(summary_value) : NULL;
        row.summary = summary_text != NULL ? HR_STRDUP(summary_text) : HR_STRDUP("");
        if (!json_value_get_int64(created_value, &row.created_at) || !json_value_get_int64(updated_value, &row.updated_at) ||
            !json_value_get_int(position_value, &row.position)) {
            free(row.title);
            free(row.summary);
            free_topics(&out);
            io_set_error(error_message, error_capacity, "Topic numeric field invalid");
            return false;
        }

        HrTopicPayload payload;
        payload.title = row.title;
        payload.summary = row.summary;
        HrValidationError validation_error;
        if (!hr_topic_payload_validate(&payload, &validation_error)) {
            free(row.title);
            free(row.summary);
            free_topics(&out);
            io_set_errorf(error_message, error_capacity, "Topic validation failed: %s", validation_error.message);
            return false;
        }

        if (out.count + 1U > out.capacity) {
            size_t new_capacity = out.capacity == 0U ? HR_ARRAY_GROW : out.capacity * 2U;
            TopicRow *new_items = (TopicRow *)realloc(out.items, new_capacity * sizeof(TopicRow));
            if (new_items == NULL) {
                free(row.title);
                free(row.summary);
                free_topics(&out);
                io_set_error(error_message, error_capacity, "Out of memory");
                return false;
            }
            out.items = new_items;
            out.capacity = new_capacity;
        }
        out.items[out.count++] = row;
    }

    *topics = out;
    return true;
}

static bool parse_cards_json(const HrJsonArray *array,
                             CardRowArray *cards,
                             char *error_message,
                             size_t error_capacity)
{
    CardRowArray out = {0};
    for (size_t i = 0; i < array->count; ++i) {
        const HrJsonValue *value = &array->items[i];
        if (value->type != HR_JSON_OBJECT) {
            free_cards(&out);
            io_set_error(error_message, error_capacity, "Card entry must be an object");
            return false;
        }
        const HrJsonValue *uuid_value = json_object_get(value, "uuid");
        const HrJsonValue *topic_uuid_value = json_object_get(value, "topic_uuid");
        const HrJsonValue *prompt_value = json_object_get(value, "prompt");
        const HrJsonValue *response_value = json_object_get(value, "response");
        const HrJsonValue *mnemonic_value = json_object_get(value, "mnemonic");
        const HrJsonValue *created_value = json_object_get(value, "created_at");
        const HrJsonValue *updated_value = json_object_get(value, "updated_at");
        const HrJsonValue *due_value = json_object_get(value, "due_at");
        const HrJsonValue *interval_value = json_object_get(value, "interval");
        const HrJsonValue *ease_value = json_object_get(value, "ease_factor");
        const HrJsonValue *review_value = json_object_get(value, "review_state");
        const HrJsonValue *suspended_value = json_object_get(value, "suspended");
        const HrJsonValue *type_value = json_object_get(value, "type");

        if (uuid_value == NULL || topic_uuid_value == NULL || prompt_value == NULL || response_value == NULL ||
            created_value == NULL || updated_value == NULL || due_value == NULL || interval_value == NULL ||
            ease_value == NULL || review_value == NULL || suspended_value == NULL) {
            free_cards(&out);
            io_set_error(error_message, error_capacity, "Card entry missing required fields");
            return false;
        }

        CardRow row;
        memset(&row, 0, sizeof(row));
        const char *uuid_text = json_value_get_string(uuid_value);
        const char *topic_uuid_text = json_value_get_string(topic_uuid_value);
        if (uuid_text == NULL || topic_uuid_text == NULL) {
            free_cards(&out);
            io_set_error(error_message, error_capacity, "Card uuid must be a string");
            return false;
        }
        strncpy(row.uuid, uuid_text, sizeof(row.uuid) - 1U);
        row.uuid[sizeof(row.uuid) - 1U] = '\0';
        strncpy(row.topic_uuid, topic_uuid_text, sizeof(row.topic_uuid) - 1U);
        row.topic_uuid[sizeof(row.topic_uuid) - 1U] = '\0';

        const char *prompt_text = json_value_get_string(prompt_value);
        const char *response_text = json_value_get_string(response_value);
        const char *mnemonic_text = mnemonic_value != NULL ? json_value_get_string(mnemonic_value) : NULL;
        row.prompt = prompt_text != NULL ? HR_STRDUP(prompt_text) : HR_STRDUP("");
        row.response = response_text != NULL ? HR_STRDUP(response_text) : HR_STRDUP("");
        row.mnemonic = mnemonic_text != NULL ? HR_STRDUP(mnemonic_text) : NULL;

        if (!json_value_get_int64(created_value, &row.created_at) || !json_value_get_int64(updated_value, &row.updated_at) ||
            !json_value_get_int64(due_value, &row.due_at) || !json_value_get_int(interval_value, &row.interval) ||
            !json_value_get_int(ease_value, &row.ease_factor) || !json_value_get_int(review_value, &row.review_state)) {
            free(row.prompt);
            free(row.response);
            free(row.mnemonic);
            free_cards(&out);
            io_set_error(error_message, error_capacity, "Card numeric field invalid");
            return false;
        }

        bool suspended = false;
        if (suspended_value->type == HR_JSON_BOOLEAN) {
            suspended = suspended_value->data.boolean != 0;
        } else if (suspended_value->type == HR_JSON_NUMBER) {
            suspended = suspended_value->data.number != 0.0;
        } else if (suspended_value->type == HR_JSON_STRING && suspended_value->data.string.data != NULL) {
            suspended = suspended_value->data.string.data[0] != '\0' && suspended_value->data.string.data[0] != '0';
        } else {
            free(row.prompt);
            free(row.response);
            free(row.mnemonic);
            free_cards(&out);
            io_set_error(error_message, error_capacity, "Card suspended field invalid");
            return false;
        }
        row.suspended = suspended ? 1 : 0;

        HrCardType card_type = HR_CARD_TYPE_SHORT_ANSWER;
        if (type_value != NULL) {
            const char *type_text = json_value_get_string(type_value);
            if (type_text != NULL && !hr_card_type_from_string(type_text, &card_type)) {
                free(row.prompt);
                free(row.response);
                free(row.mnemonic);
                free_cards(&out);
                io_set_errorf(error_message, error_capacity, "Unknown card type %s", type_text);
                return false;
            }
        }

        HrCardExtras extras;
        hr_card_extras_init(&extras, card_type);
        HrCardPayload payload;
        payload.type = card_type;
        payload.prompt = row.prompt;
        payload.response = row.response;
        payload.mnemonic = row.mnemonic;
        payload.extras = extras;
        payload.media.items = NULL;
        payload.media.count = 0U;
        HrValidationError validation_error;
        if (!hr_card_payload_validate(&payload, &validation_error)) {
            free(row.prompt);
            free(row.response);
            free(row.mnemonic);
            free_cards(&out);
            io_set_errorf(error_message, error_capacity, "Card validation failed: %s", validation_error.message);
            return false;
        }

        if (out.count + 1U > out.capacity) {
            size_t new_capacity = out.capacity == 0U ? HR_ARRAY_GROW : out.capacity * 2U;
            CardRow *new_items = (CardRow *)realloc(out.items, new_capacity * sizeof(CardRow));
            if (new_items == NULL) {
                free(row.prompt);
                free(row.response);
                free(row.mnemonic);
                free_cards(&out);
                io_set_error(error_message, error_capacity, "Out of memory");
                return false;
            }
            out.items = new_items;
            out.capacity = new_capacity;
        }
        out.items[out.count++] = row;
    }

    *cards = out;
    return true;
}

static bool parse_reviews_json(const HrJsonArray *array,
                               ReviewRowArray *reviews,
                               char *error_message,
                               size_t error_capacity)
{
    ReviewRowArray out = {0};
    for (size_t i = 0; i < array->count; ++i) {
        const HrJsonValue *value = &array->items[i];
        if (value->type != HR_JSON_OBJECT) {
            free_reviews(&out);
            io_set_error(error_message, error_capacity, "Review entry must be an object");
            return false;
        }
        const HrJsonValue *uuid_value = json_object_get(value, "card_uuid");
        const HrJsonValue *reviewed_value = json_object_get(value, "reviewed_at");
        const HrJsonValue *rating_value = json_object_get(value, "rating");
        const HrJsonValue *duration_value = json_object_get(value, "duration_ms");
        const HrJsonValue *scheduled_value = json_object_get(value, "scheduled_interval");
        const HrJsonValue *actual_value = json_object_get(value, "actual_interval");
        const HrJsonValue *ease_value = json_object_get(value, "ease_factor");
        const HrJsonValue *state_value = json_object_get(value, "review_state");
        if (uuid_value == NULL || reviewed_value == NULL || rating_value == NULL || duration_value == NULL ||
            scheduled_value == NULL || actual_value == NULL || ease_value == NULL || state_value == NULL) {
            free_reviews(&out);
            io_set_error(error_message, error_capacity, "Review entry missing required fields");
            return false;
        }
        ReviewRow row;
        memset(&row, 0, sizeof(row));
        const char *uuid_text = json_value_get_string(uuid_value);
        if (uuid_text == NULL) {
            free_reviews(&out);
            io_set_error(error_message, error_capacity, "Review card_uuid must be a string");
            return false;
        }
        strncpy(row.card_uuid, uuid_text, sizeof(row.card_uuid) - 1U);
        row.card_uuid[sizeof(row.card_uuid) - 1U] = '\0';
        if (!json_value_get_int64(reviewed_value, &row.reviewed_at) || !json_value_get_int(rating_value, &row.rating) ||
            !json_value_get_int(duration_value, &row.duration_ms) || !json_value_get_int(scheduled_value, &row.scheduled_interval) ||
            !json_value_get_int(actual_value, &row.actual_interval) || !json_value_get_int(ease_value, &row.ease_factor) ||
            !json_value_get_int(state_value, &row.review_state)) {
            free_reviews(&out);
            io_set_error(error_message, error_capacity, "Review numeric field invalid");
            return false;
        }

        if (out.count + 1U > out.capacity) {
            size_t new_capacity = out.capacity == 0U ? HR_ARRAY_GROW : out.capacity * 2U;
            ReviewRow *new_items = (ReviewRow *)realloc(out.items, new_capacity * sizeof(ReviewRow));
            if (new_items == NULL) {
                free_reviews(&out);
                io_set_error(error_message, error_capacity, "Out of memory");
                return false;
            }
            out.items = new_items;
            out.capacity = new_capacity;
        }
        out.items[out.count++] = row;
    }

    *reviews = out;
    return true;
}

static bool parse_manifest(const HrJsonValue *root,
                           TopicRowArray *topics,
                           CardRowArray *cards,
                           ReviewRowArray *reviews,
                           char *error_message,
                           size_t error_capacity)
{
    if (root == NULL || root->type != HR_JSON_OBJECT) {
        io_set_error(error_message, error_capacity, "Manifest root must be an object");
        return false;
    }
    const HrJsonValue *schema_value = json_object_get(root, "schema");
    const char *schema_text = schema_value != NULL ? json_value_get_string(schema_value) : NULL;
    if (schema_text == NULL || strcmp(schema_text, HR_IO_SCHEMA) != 0) {
        io_set_error(error_message, error_capacity, "Manifest schema mismatch");
        return false;
    }

    const HrJsonValue *topics_value = json_object_get(root, "topics");
    if (topics_value == NULL || topics_value->type != HR_JSON_ARRAY) {
        io_set_error(error_message, error_capacity, "Manifest missing topics array");
        return false;
    }
    if (!parse_topics_json(&topics_value->data.array, topics, error_message, error_capacity)) {
        return false;
    }

    const HrJsonValue *cards_value = json_object_get(root, "cards");
    if (cards_value == NULL || cards_value->type != HR_JSON_ARRAY) {
        free_topics(topics);
        io_set_error(error_message, error_capacity, "Manifest missing cards array");
        return false;
    }
    if (!parse_cards_json(&cards_value->data.array, cards, error_message, error_capacity)) {
        free_topics(topics);
        return false;
    }

    const HrJsonValue *reviews_value = json_object_get(root, "reviews");
    if (reviews != NULL && reviews_value != NULL && reviews_value->type == HR_JSON_ARRAY) {
        if (!parse_reviews_json(&reviews_value->data.array, reviews, error_message, error_capacity)) {
            free_topics(topics);
            free_cards(cards);
            return false;
        }
    } else if (reviews != NULL) {
        reviews->items = NULL;
        reviews->count = 0U;
        reviews->capacity = 0U;
    }

    return true;
}

static bool load_manifest(const char *path,
                          TopicRowArray *topics,
                          CardRowArray *cards,
                          ReviewRowArray *reviews,
                          char *error_message,
                          size_t error_capacity)
{
    char *text = NULL;
    if (!load_text_file(path, &text, NULL)) {
        io_set_errorf(error_message, error_capacity, "Unable to read manifest %s", path);
        return false;
    }

    HrJsonValue root;
    json_value_init(&root);
    bool parsed = json_parse(text, &root);
    free(text);
    if (!parsed) {
        io_set_error(error_message, error_capacity, "Manifest JSON parsing failed");
        return false;
    }

    bool ok = parse_manifest(&root, topics, cards, reviews, error_message, error_capacity);
    json_free_value(&root);
    return ok;
}

static bool import_topics_into_db(struct ImportExportContext *context,
                                  const TopicRowArray *topics,
                                  bool merge_existing,
                                  TopicIdEntry **out_map,
                                  ImportExportStats *stats,
                                  char *error_message,
                                  size_t error_capacity)
{
    if (topics == NULL || topics->count == 0U) {
        *out_map = NULL;
        return true;
    }

    sqlite3 *db = db_connection(context->database);
    if (db == NULL) {
        io_set_error(error_message, error_capacity, "Database unavailable");
        return false;
    }

    sqlite3_stmt *insert_stmt = NULL;
    sqlite3_stmt *update_stmt = NULL;
    sqlite3_stmt *select_stmt = NULL;
    sqlite3_stmt *existing_stmt = NULL;

    if (db_topic_prepare_insert(context->database, &insert_stmt) != SQLITE_OK ||
        db_topic_prepare_update(context->database, &update_stmt) != SQLITE_OK ||
        db_topic_prepare_select_by_uuid(context->database, &select_stmt) != SQLITE_OK ||
        sqlite3_prepare_v2(db, "SELECT id, uuid FROM topics;", -1, &existing_stmt, NULL) != SQLITE_OK) {
        io_set_error(error_message, error_capacity, "Failed to prepare topic statements");
        sqlite3_finalize(insert_stmt);
        sqlite3_finalize(update_stmt);
        sqlite3_finalize(select_stmt);
        sqlite3_finalize(existing_stmt);
        return false;
    }

    TopicIdEntry *map = NULL;
    if (merge_existing) {
        while (sqlite3_step(existing_stmt) == SQLITE_ROW) {
            sqlite3_int64 id = sqlite3_column_int64(existing_stmt, 0);
            const unsigned char *uuid_text = sqlite3_column_text(existing_stmt, 1);
            if (uuid_text != NULL) {
                topic_id_map_add(&map, (const char *)uuid_text, id);
            }
        }
        sqlite3_reset(existing_stmt);
    }

    for (size_t i = 0; i < topics->count; ++i) {
        const TopicRow *row = &topics->items[i];
        sqlite3_int64 parent_id = 0;
        if (row->parent_uuid[0] != '\0') {
            if (!topic_id_map_find(map, row->parent_uuid, &parent_id)) {
                sqlite3_bind_text(select_stmt, 1, row->parent_uuid, -1, SQLITE_TRANSIENT);
                if (sqlite3_step(select_stmt) == SQLITE_ROW) {
                    parent_id = sqlite3_column_int64(select_stmt, 0);
                    topic_id_map_add(&map, row->parent_uuid, parent_id);
                } else {
                    sqlite3_finalize(insert_stmt);
                    sqlite3_finalize(update_stmt);
                    sqlite3_finalize(select_stmt);
                    sqlite3_finalize(existing_stmt);
                    topic_id_map_free(map);
                    io_set_errorf(error_message, error_capacity, "Missing parent topic %s", row->parent_uuid);
                    return false;
                }
                sqlite3_reset(select_stmt);
            }
        }

        sqlite3_int64 existing_id = 0;
        bool has_existing = topic_id_map_find(map, row->uuid, &existing_id);
        if (has_existing && !merge_existing) {
            sqlite3_finalize(insert_stmt);
            sqlite3_finalize(update_stmt);
            sqlite3_finalize(select_stmt);
            sqlite3_finalize(existing_stmt);
            topic_id_map_free(map);
            io_set_errorf(error_message, error_capacity, "Duplicate topic %s", row->uuid);
            return false;
        }

        HrTopicRecord record;
        memset(&record, 0, sizeof(record));
        record.parent_id = parent_id;
        record.title = row->title;
        record.summary = row->summary;
        record.created_at = row->created_at;
        record.updated_at = row->updated_at;
        record.position = row->position;

        if (has_existing) {
            record.id = existing_id;
            if (db_topic_bind_update(update_stmt, &record) != SQLITE_OK || sqlite3_step(update_stmt) != SQLITE_DONE) {
                sqlite3_finalize(insert_stmt);
                sqlite3_finalize(update_stmt);
                sqlite3_finalize(select_stmt);
                sqlite3_finalize(existing_stmt);
                topic_id_map_free(map);
                io_set_errorf(error_message, error_capacity, "Failed to update topic %s", row->uuid);
                return false;
            }
            sqlite3_reset(update_stmt);
            sqlite3_clear_bindings(update_stmt);
        } else {
            record.uuid = row->uuid;
            if (db_topic_bind_insert(insert_stmt, &record) != SQLITE_OK || sqlite3_step(insert_stmt) != SQLITE_DONE) {
                sqlite3_finalize(insert_stmt);
                sqlite3_finalize(update_stmt);
                sqlite3_finalize(select_stmt);
                sqlite3_finalize(existing_stmt);
                topic_id_map_free(map);
                io_set_errorf(error_message, error_capacity, "Failed to insert topic %s", row->uuid);
                return false;
            }
            sqlite3_reset(insert_stmt);
            sqlite3_clear_bindings(insert_stmt);
            existing_id = sqlite3_last_insert_rowid(db);
            topic_id_map_add(&map, row->uuid, existing_id);
        }

        if (stats != NULL) {
            stats->topic_count += 1U;
        }
    }

    sqlite3_finalize(insert_stmt);
    sqlite3_finalize(update_stmt);
    sqlite3_finalize(select_stmt);
    sqlite3_finalize(existing_stmt);

    *out_map = map;
    return true;
}

static bool import_cards_into_db(struct ImportExportContext *context,
                                 const CardRowArray *cards,
                                 bool merge_existing,
                                 TopicIdEntry *topic_map,
                                 CardIdEntry **out_card_map,
                                 ImportExportStats *stats,
                                 char *error_message,
                                 size_t error_capacity)
{
    if (cards == NULL || cards->count == 0U) {
        *out_card_map = NULL;
        return true;
    }

    sqlite3 *db = db_connection(context->database);
    if (db == NULL) {
        io_set_error(error_message, error_capacity, "Database unavailable");
        return false;
    }

    sqlite3_stmt *insert_stmt = NULL;
    sqlite3_stmt *update_stmt = NULL;
    sqlite3_stmt *select_stmt = NULL;
    sqlite3_stmt *topic_select_stmt = NULL;
    sqlite3_stmt *existing_stmt = NULL;

    if (db_card_prepare_insert(context->database, &insert_stmt) != SQLITE_OK ||
        db_card_prepare_update(context->database, &update_stmt) != SQLITE_OK ||
        sqlite3_prepare_v2(db, "SELECT id FROM cards WHERE uuid=?1;", -1, &select_stmt, NULL) != SQLITE_OK ||
        sqlite3_prepare_v2(db, "SELECT id FROM topics WHERE uuid=?1;", -1, &topic_select_stmt, NULL) != SQLITE_OK ||
        sqlite3_prepare_v2(db, "SELECT id, uuid FROM cards;", -1, &existing_stmt, NULL) != SQLITE_OK) {
        io_set_error(error_message, error_capacity, "Failed to prepare card statements");
        sqlite3_finalize(insert_stmt);
        sqlite3_finalize(update_stmt);
        sqlite3_finalize(select_stmt);
        sqlite3_finalize(topic_select_stmt);
        sqlite3_finalize(existing_stmt);
        return false;
    }

    CardIdEntry *card_map = NULL;
    if (merge_existing) {
        while (sqlite3_step(existing_stmt) == SQLITE_ROW) {
            sqlite3_int64 id = sqlite3_column_int64(existing_stmt, 0);
            const unsigned char *uuid_text = sqlite3_column_text(existing_stmt, 1);
            if (uuid_text != NULL) {
                card_id_map_add(&card_map, (const char *)uuid_text, id);
            }
        }
        sqlite3_reset(existing_stmt);
    }

    for (size_t i = 0; i < cards->count; ++i) {
        const CardRow *row = &cards->items[i];
        sqlite3_int64 topic_id = 0;
        if (!topic_id_map_find(topic_map, row->topic_uuid, &topic_id)) {
            sqlite3_bind_text(topic_select_stmt, 1, row->topic_uuid, -1, SQLITE_TRANSIENT);
            if (sqlite3_step(topic_select_stmt) == SQLITE_ROW) {
                topic_id = sqlite3_column_int64(topic_select_stmt, 0);
                topic_id_map_add(&topic_map, row->topic_uuid, topic_id);
            } else {
                sqlite3_finalize(insert_stmt);
                sqlite3_finalize(update_stmt);
                sqlite3_finalize(select_stmt);
                sqlite3_finalize(topic_select_stmt);
                sqlite3_finalize(existing_stmt);
                card_id_map_free(card_map);
                io_set_errorf(error_message, error_capacity, "Missing topic %s for card", row->topic_uuid);
                return false;
            }
            sqlite3_reset(topic_select_stmt);
        }

        sqlite3_int64 existing_id = 0;
        bool has_existing = card_id_map_find(card_map, row->uuid, &existing_id);
        if (!has_existing) {
            sqlite3_bind_text(select_stmt, 1, row->uuid, -1, SQLITE_TRANSIENT);
            if (sqlite3_step(select_stmt) == SQLITE_ROW) {
                existing_id = sqlite3_column_int64(select_stmt, 0);
                has_existing = true;
                card_id_map_add(&card_map, row->uuid, existing_id);
            }
            sqlite3_reset(select_stmt);
        }

        if (has_existing && !merge_existing) {
            sqlite3_finalize(insert_stmt);
            sqlite3_finalize(update_stmt);
            sqlite3_finalize(select_stmt);
            sqlite3_finalize(topic_select_stmt);
            sqlite3_finalize(existing_stmt);
            card_id_map_free(card_map);
            io_set_errorf(error_message, error_capacity, "Duplicate card %s", row->uuid);
            return false;
        }

        HrCardRecord record;
        memset(&record, 0, sizeof(record));
        record.topic_id = topic_id;
        record.prompt = row->prompt;
        record.response = row->response;
        record.mnemonic = row->mnemonic;
        record.created_at = row->created_at;
        record.updated_at = row->updated_at;
        record.due_at = row->due_at;
        record.interval = row->interval;
        record.ease_factor = row->ease_factor;
        record.review_state = row->review_state;
        record.suspended = row->suspended != 0;

        if (has_existing) {
            record.id = existing_id;
            if (db_card_bind_update(update_stmt, &record) != SQLITE_OK || sqlite3_step(update_stmt) != SQLITE_DONE) {
                sqlite3_finalize(insert_stmt);
                sqlite3_finalize(update_stmt);
                sqlite3_finalize(select_stmt);
                sqlite3_finalize(topic_select_stmt);
                sqlite3_finalize(existing_stmt);
                card_id_map_free(card_map);
                io_set_errorf(error_message, error_capacity, "Failed to update card %s", row->uuid);
                return false;
            }
            sqlite3_reset(update_stmt);
            sqlite3_clear_bindings(update_stmt);
        } else {
            record.uuid = row->uuid;
            if (db_card_bind_insert(insert_stmt, &record) != SQLITE_OK || sqlite3_step(insert_stmt) != SQLITE_DONE) {
                sqlite3_finalize(insert_stmt);
                sqlite3_finalize(update_stmt);
                sqlite3_finalize(select_stmt);
                sqlite3_finalize(topic_select_stmt);
                sqlite3_finalize(existing_stmt);
                card_id_map_free(card_map);
                io_set_errorf(error_message, error_capacity, "Failed to insert card %s", row->uuid);
                return false;
            }
            sqlite3_reset(insert_stmt);
            sqlite3_clear_bindings(insert_stmt);
            existing_id = sqlite3_last_insert_rowid(db);
            card_id_map_add(&card_map, row->uuid, existing_id);
        }

        if (stats != NULL) {
            stats->card_count += 1U;
        }
    }

    sqlite3_finalize(insert_stmt);
    sqlite3_finalize(update_stmt);
    sqlite3_finalize(select_stmt);
    sqlite3_finalize(topic_select_stmt);
    sqlite3_finalize(existing_stmt);

    *out_card_map = card_map;
    return true;
}

static bool import_reviews_into_db(struct ImportExportContext *context,
                                   const ReviewRowArray *reviews,
                                   CardIdEntry *card_map,
                                   char *error_message,
                                   size_t error_capacity)
{
    if (reviews == NULL || reviews->count == 0U) {
        return true;
    }

    sqlite3 *db = db_connection(context->database);
    if (db == NULL) {
        io_set_error(error_message, error_capacity, "Database unavailable");
        return false;
    }

    sqlite3_stmt *insert_stmt = NULL;
    sqlite3_stmt *card_select_stmt = NULL;
    if (db_review_prepare_bulk_insert(context->database, &insert_stmt) != SQLITE_OK ||
        sqlite3_prepare_v2(db, "SELECT id FROM cards WHERE uuid=?1;", -1, &card_select_stmt, NULL) != SQLITE_OK) {
        io_set_error(error_message, error_capacity, "Failed to prepare review statements");
        sqlite3_finalize(insert_stmt);
        sqlite3_finalize(card_select_stmt);
        return false;
    }

    for (size_t i = 0; i < reviews->count; ++i) {
        const ReviewRow *row = &reviews->items[i];
        sqlite3_int64 card_id = 0;
        if (!card_id_map_find(card_map, row->card_uuid, &card_id)) {
            sqlite3_bind_text(card_select_stmt, 1, row->card_uuid, -1, SQLITE_TRANSIENT);
            if (sqlite3_step(card_select_stmt) == SQLITE_ROW) {
                card_id = sqlite3_column_int64(card_select_stmt, 0);
                card_id_map_add(&card_map, row->card_uuid, card_id);
            } else {
                sqlite3_finalize(insert_stmt);
                sqlite3_finalize(card_select_stmt);
                io_set_errorf(error_message, error_capacity, "Unknown card for review %s", row->card_uuid);
                return false;
            }
            sqlite3_reset(card_select_stmt);
        }

        HrReviewRecord record;
        record.card_id = card_id;
        record.reviewed_at = row->reviewed_at;
        record.rating = row->rating;
        record.duration_ms = row->duration_ms;
        record.scheduled_interval = row->scheduled_interval;
        record.actual_interval = row->actual_interval;
        record.ease_factor = row->ease_factor;
        record.review_state = row->review_state;

        if (db_review_bind_bulk_insert(insert_stmt, &record) != SQLITE_OK || sqlite3_step(insert_stmt) != SQLITE_DONE) {
            sqlite3_finalize(insert_stmt);
            sqlite3_finalize(card_select_stmt);
            io_set_error(error_message, error_capacity, "Failed to insert review");
            return false;
        }
        sqlite3_reset(insert_stmt);
        sqlite3_clear_bindings(insert_stmt);
    }

    sqlite3_finalize(insert_stmt);
    sqlite3_finalize(card_select_stmt);
    return true;
}

struct ImportExportContext *import_export_create(DatabaseHandle *database,
                                                 const ImportExportConfig *config)
{
    if (database == NULL) {
        return NULL;
    }
    struct ImportExportContext *context = (struct ImportExportContext *)calloc(1, sizeof(*context));
    if (context == NULL) {
        return NULL;
    }
    context->database = database;
    if (config != NULL) {
        context->config = *config;
        if (context->config.default_export_name == NULL) {
            context->config.default_export_name = HR_IO_DEFAULT_EXPORT;
        }
    } else {
        context->config.media_root = NULL;
        context->config.default_export_name = HR_IO_DEFAULT_EXPORT;
        context->config.include_reviews = false;
    }
    return context;
}

void import_export_destroy(struct ImportExportContext *context)
{
    if (context == NULL) {
        return;
    }
    free(context);
}

void import_export_set_progress_callback(struct ImportExportContext *context,
                                         ImportExportProgressCallback callback,
                                         void *user_data)
{
    if (context == NULL) {
        return;
    }
    context->progress_callback = callback;
    context->progress_user_data = user_data;
}

bool import_export_export_collection(struct ImportExportContext *context,
                                     const char *destination_dir,
                                     ImportExportStats *stats,
                                     char *error_message,
                                     size_t error_capacity)
{
    if (context == NULL || destination_dir == NULL) {
        io_set_error(error_message, error_capacity, "Invalid parameters");
        return false;
    }

    ImportExportStats local_stats = {0};
    if (stats == NULL) {
        stats = &local_stats;
    } else {
        memset(stats, 0, sizeof(*stats));
    }

    TopicRowArray topics = {0};
    CardRowArray cards = {0};
    ReviewRowArray reviews = {0};

    io_report_progress(context, "Collecting topics", 0U, 0U);
    if (!collect_topics(context, &topics, error_message, error_capacity)) {
        return false;
    }

    io_report_progress(context, "Collecting cards", 0U, 0U);
    if (!collect_cards(context, &cards, error_message, error_capacity)) {
        free_topics(&topics);
        return false;
    }

    if (!collect_reviews(context, &reviews, error_message, error_capacity)) {
        free_topics(&topics);
        free_cards(&cards);
        return false;
    }

    if (io_ensure_directory(destination_dir) != 0) {
        free_topics(&topics);
        free_cards(&cards);
        free_reviews(&reviews);
        io_set_errorf(error_message, error_capacity, "Unable to create %s", destination_dir);
        return false;
    }

    char manifest_path[PATH_MAX];
    if (!io_path_join(destination_dir, HR_IO_MANIFEST, manifest_path, sizeof(manifest_path))) {
        free_topics(&topics);
        free_cards(&cards);
        free_reviews(&reviews);
        io_set_error(error_message, error_capacity, "Manifest path too long");
        return false;
    }

    io_report_progress(context, "Writing manifest", 0U, 0U);
    if (!write_manifest(manifest_path, &topics, &cards, &reviews, error_message, error_capacity)) {
        free_topics(&topics);
        free_cards(&cards);
        free_reviews(&reviews);
        return false;
    }

    char topics_csv[PATH_MAX];
    if (!io_path_join(destination_dir, HR_IO_TOPICS_CSV, topics_csv, sizeof(topics_csv))) {
        free_topics(&topics);
        free_cards(&cards);
        free_reviews(&reviews);
        io_set_error(error_message, error_capacity, "topics.csv path too long");
        return false;
    }
    if (!write_topics_csv(topics_csv, &topics, error_message, error_capacity)) {
        free_topics(&topics);
        free_cards(&cards);
        free_reviews(&reviews);
        return false;
    }

    char cards_csv[PATH_MAX];
    if (!io_path_join(destination_dir, HR_IO_CARDS_CSV, cards_csv, sizeof(cards_csv))) {
        free_topics(&topics);
        free_cards(&cards);
        free_reviews(&reviews);
        io_set_error(error_message, error_capacity, "cards.csv path too long");
        return false;
    }
    if (!write_cards_csv(cards_csv, &cards, error_message, error_capacity)) {
        free_topics(&topics);
        free_cards(&cards);
        free_reviews(&reviews);
        return false;
    }

    if (reviews.count > 0U) {
        char reviews_csv[PATH_MAX];
        if (!io_path_join(destination_dir, HR_IO_REVIEWS_CSV, reviews_csv, sizeof(reviews_csv))) {
            free_topics(&topics);
            free_cards(&cards);
            free_reviews(&reviews);
            io_set_error(error_message, error_capacity, "reviews.csv path too long");
            return false;
        }
        if (!write_reviews_csv(reviews_csv, &reviews, error_message, error_capacity)) {
            free_topics(&topics);
            free_cards(&cards);
            free_reviews(&reviews);
            return false;
        }
    }

    if (context->config.media_root != NULL && context->config.media_root[0] != '\0') {
        char destination_media[PATH_MAX];
        if (!io_path_join(destination_dir, HR_IO_MEDIA_DIR, destination_media, sizeof(destination_media))) {
            free_topics(&topics);
            free_cards(&cards);
            free_reviews(&reviews);
            io_set_error(error_message, error_capacity, "Media path too long");
            return false;
        }
        io_report_progress(context, "Copying media", 0U, 0U);
        if (!copy_media_directory(context,
                                  context->config.media_root,
                                  destination_media,
                                  false,
                                  stats,
                                  error_message,
                                  error_capacity)) {
            free_topics(&topics);
            free_cards(&cards);
            free_reviews(&reviews);
            return false;
        }
    }

    stats->topic_count = topics.count;
    stats->card_count = cards.count;

    free_topics(&topics);
    free_cards(&cards);
    free_reviews(&reviews);

    return true;
}

bool import_export_export_csv(struct ImportExportContext *context,
                              const char *destination_dir,
                              ImportExportStats *stats,
                              char *error_message,
                              size_t error_capacity)
{
    if (context == NULL || destination_dir == NULL) {
        io_set_error(error_message, error_capacity, "Invalid parameters");
        return false;
    }

    ImportExportStats local_stats = {0};
    if (stats == NULL) {
        stats = &local_stats;
    } else {
        memset(stats, 0, sizeof(*stats));
    }

    TopicRowArray topics = {0};
    CardRowArray cards = {0};

    if (!collect_topics(context, &topics, error_message, error_capacity)) {
        return false;
    }
    if (!collect_cards(context, &cards, error_message, error_capacity)) {
        free_topics(&topics);
        return false;
    }

    if (io_ensure_directory(destination_dir) != 0) {
        free_topics(&topics);
        free_cards(&cards);
        io_set_errorf(error_message, error_capacity, "Unable to create %s", destination_dir);
        return false;
    }

    char topics_csv[PATH_MAX];
    char cards_csv[PATH_MAX];
    if (!io_path_join(destination_dir, HR_IO_TOPICS_CSV, topics_csv, sizeof(topics_csv)) ||
        !io_path_join(destination_dir, HR_IO_CARDS_CSV, cards_csv, sizeof(cards_csv))) {
        free_topics(&topics);
        free_cards(&cards);
        io_set_error(error_message, error_capacity, "CSV path too long");
        return false;
    }

    if (!write_topics_csv(topics_csv, &topics, error_message, error_capacity) ||
        !write_cards_csv(cards_csv, &cards, error_message, error_capacity)) {
        free_topics(&topics);
        free_cards(&cards);
        return false;
    }

    stats->topic_count = topics.count;
    stats->card_count = cards.count;

    free_topics(&topics);
    free_cards(&cards);

    return true;
}

bool import_export_import_collection(struct ImportExportContext *context,
                                     const char *source_dir,
                                     bool merge_existing,
                                     ImportExportStats *stats,
                                     char *error_message,
                                     size_t error_capacity)
{
    if (context == NULL || source_dir == NULL) {
        io_set_error(error_message, error_capacity, "Invalid parameters");
        return false;
    }

    ImportExportStats local_stats = {0};
    if (stats == NULL) {
        stats = &local_stats;
    } else {
        memset(stats, 0, sizeof(*stats));
    }

    char manifest_path[PATH_MAX];
    if (!io_path_join(source_dir, HR_IO_MANIFEST, manifest_path, sizeof(manifest_path))) {
        io_set_error(error_message, error_capacity, "Manifest path too long");
        return false;
    }

    TopicRowArray topics = {0};
    CardRowArray cards = {0};
    ReviewRowArray reviews = {0};

    if (!load_manifest(manifest_path, &topics, &cards, &reviews, error_message, error_capacity)) {
        return false;
    }

    sqlite3 *db = db_connection(context->database);
    if (db == NULL) {
        free_topics(&topics);
        free_cards(&cards);
        free_reviews(&reviews);
        io_set_error(error_message, error_capacity, "Database unavailable");
        return false;
    }

    if (db_begin(context->database) != SQLITE_OK) {
        free_topics(&topics);
        free_cards(&cards);
        free_reviews(&reviews);
        io_set_error(error_message, error_capacity, "Failed to begin transaction");
        return false;
    }

    bool success = false;
    TopicIdEntry *topic_map = NULL;
    CardIdEntry *card_map = NULL;

    do {
        io_report_progress(context, "Importing topics", 0U, topics.count);
        if (!import_topics_into_db(context, &topics, merge_existing, &topic_map, stats, error_message, error_capacity)) {
            break;
        }

        io_report_progress(context, "Importing cards", 0U, cards.count);
        if (!import_cards_into_db(context,
                                  &cards,
                                  merge_existing,
                                  topic_map,
                                  &card_map,
                                  stats,
                                  error_message,
                                  error_capacity)) {
            break;
        }

        if (reviews.count > 0U) {
            io_report_progress(context, "Importing reviews", 0U, reviews.count);
            if (!import_reviews_into_db(context, &reviews, card_map, error_message, error_capacity)) {
                break;
            }
        }

        if (db_commit(context->database) != SQLITE_OK) {
            io_set_error(error_message, error_capacity, "Failed to commit transaction");
            break;
        }

        success = true;
    } while (false);

    if (!success) {
        db_rollback(context->database);
    }

    topic_id_map_free(topic_map);
    card_id_map_free(card_map);
    free_topics(&topics);
    free_cards(&cards);
    free_reviews(&reviews);

    if (!success) {
        return false;
    }

    if (context->config.media_root != NULL && context->config.media_root[0] != '\0') {
        char source_media[PATH_MAX];
        if (!io_path_join(source_dir, HR_IO_MEDIA_DIR, source_media, sizeof(source_media))) {
            io_set_error(error_message, error_capacity, "Media path too long");
            return false;
        }
        if (!copy_media_directory(context,
                                  source_media,
                                  context->config.media_root,
                                  true,
                                  stats,
                                  error_message,
                                  error_capacity)) {
            return false;
        }
    }

    return true;
}

bool import_export_export_apkg(struct ImportExportContext *context,
                               const char *destination_path,
                               char *error_message,
                               size_t error_capacity)
{
    (void)context;
    (void)destination_path;
    io_set_error(error_message, error_capacity, "Anki .apkg export is not yet supported. Please export as a HyperRecall bundle.");
    return false;
}

