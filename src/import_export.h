#ifndef HYPERRECALL_IMPORT_EXPORT_H
#define HYPERRECALL_IMPORT_EXPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file import_export.h
 * @brief Declares import/export mechanisms for decks, media, and configuration data.
 */

#include <stdbool.h>
#include <stddef.h>

struct DatabaseHandle;
struct HrCard;
struct HrTopic;

/**
 * @brief Export options for deck export.
 */
typedef struct HrExportOptions {
    const char *output_path;         /**< Path to output JSON file. */
    const char *media_dir;           /**< Directory to copy media files to (relative to JSON). */
    bool include_srs_state;          /**< Include SRS scheduling state in export. */
    bool include_topics;             /**< Include topic tree in export. */
    bool pretty_print;               /**< Format JSON with indentation. */
} HrExportOptions;

/**
 * @brief Import options for deck import.
 */
typedef struct HrImportOptions {
    const char *input_path;          /**< Path to input JSON file. */
    bool merge_topics;               /**< Merge with existing topics rather than replace. */
    bool import_srs_state;           /**< Import SRS scheduling state. */
    bool validate_only;              /**< Only validate, don't actually import. */
} HrImportOptions;

/**
 * @brief Export result information.
 */
typedef struct HrExportResult {
    bool success;                    /**< True if export succeeded. */
    size_t cards_exported;           /**< Number of cards exported. */
    size_t topics_exported;          /**< Number of topics exported. */
    size_t media_files_copied;       /**< Number of media files copied. */
    char error[256];                 /**< Error message if failed. */
} HrExportResult;

/**
 * @brief Import result information.
 */
typedef struct HrImportResult {
    bool success;                    /**< True if import succeeded. */
    size_t cards_imported;           /**< Number of cards imported. */
    size_t topics_imported;          /**< Number of topics imported. */
    size_t cards_skipped;            /**< Number of cards skipped (duplicates). */
    char error[256];                 /**< Error message if failed. */
} HrImportResult;

/**
 * @brief Export cards and topics to JSON format.
 * 
 * @param db Database handle.
 * @param options Export options.
 * @param result Result information (output).
 * @return true on success, false on error.
 */
bool hr_export_json(struct DatabaseHandle *db, const HrExportOptions *options, HrExportResult *result);

/**
 * @brief Import cards and topics from JSON format.
 * 
 * @param db Database handle.
 * @param options Import options.
 * @param result Result information (output).
 * @return true on success, false on error.
 */
bool hr_import_json(struct DatabaseHandle *db, const HrImportOptions *options, HrImportResult *result);

/**
 * @brief Export cards to CSV format (basic fields only).
 * 
 * @param db Database handle.
 * @param output_path Path to output CSV file.
 * @param result Result information (output).
 * @return true on success, false on error.
 */
bool hr_export_csv(struct DatabaseHandle *db, const char *output_path, HrExportResult *result);

/**
 * @brief Import cards from CSV format (basic fields only).
 * 
 * @param db Database handle.
 * @param input_path Path to input CSV file.
 * @param result Result information (output).
 * @return true on success, false on error.
 */
bool hr_import_csv(struct DatabaseHandle *db, const char *input_path, HrImportResult *result);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_IMPORT_EXPORT_H */
