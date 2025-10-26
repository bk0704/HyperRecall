#include "import_export.h"

#include "db.h"
#include "json.h"
#include "model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

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

    /* Export would query database and populate arrays here */
    /* For now, create empty arrays as placeholders */
    HrJsonValue *cards_array = hr_json_array_new();
    HrJsonValue *topics_array = hr_json_array_new();

    if (cards_array) {
        hr_json_object_set(root, "cards", cards_array);
    }
    if (options->include_topics && topics_array) {
        hr_json_object_set(root, "topics", topics_array);
    } else {
        hr_json_free(topics_array);
    }

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
    /* Would set actual counts here */
    result->cards_exported = 0;
    result->topics_exported = 0;
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

    /* Import would process cards and topics here */
    size_t card_count = hr_json_array_size(cards);
    size_t topic_count = topics ? hr_json_array_size(topics) : 0;

    hr_json_free(root);

    result->success = true;
    result->cards_imported = card_count;
    result->topics_imported = topic_count;
    result->cards_skipped = 0;

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
    fprintf(file, "id,topic,prompt,response,mnemonic,created_at,due_at\n");

    /* Would query and write cards here */

    fclose(file);

    result->success = true;
    result->cards_exported = 0;

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

    while (fgets(line, sizeof(line), file)) {
        if (first_line) {
            first_line = false;
            continue; /* skip header */
        }

        /* Would parse and import card here */
        result->cards_imported++;
    }

    fclose(file);

    result->success = true;
    return true;
}

