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

#include "db.h"

/** Forward declaration for the import/export context. */
struct ImportExportContext;

/** Describes configuration options for the import/export subsystem. */
typedef struct ImportExportConfig {
    const char *media_root;       /**< Directory where media assets are stored. */
    const char *default_export_name; /**< Optional default export stem (e.g. "deck"). */
    bool include_reviews;         /**< Whether review history should be exported when available. */
} ImportExportConfig;

/** Tracks aggregate statistics collected while importing or exporting data. */
typedef struct ImportExportStats {
    size_t topic_count;         /**< Number of topics processed. */
    size_t card_count;          /**< Number of cards processed. */
    size_t media_copied;        /**< Number of media assets copied to the destination. */
    size_t media_deduplicated;  /**< Number of media assets skipped due to dedupe. */
} ImportExportStats;

/** High level phases emitted during long running import/export tasks. */
typedef struct ImportExportProgress {
    const char *phase; /**< Human readable stage description. */
    size_t current;    /**< Current progress units completed. */
    size_t total;      /**< Total progress units expected (0 when unknown). */
} ImportExportProgress;

/** Callback invoked as progress updates occur. */
typedef void (*ImportExportProgressCallback)(const ImportExportProgress *progress, void *user_data);

/**
 * Creates a new import/export context bound to the supplied database handle.
 */
struct ImportExportContext *import_export_create(DatabaseHandle *database,
                                                 const ImportExportConfig *config);

/** Releases resources owned by the import/export context. */
void import_export_destroy(struct ImportExportContext *context);

/** Assigns a progress callback invoked during import/export operations. */
void import_export_set_progress_callback(struct ImportExportContext *context,
                                         ImportExportProgressCallback callback,
                                         void *user_data);

/**
 * Exports the active collection into a JSON manifest plus CSV tables.
 *
 * The destination directory is created when necessary. When @p stats is not NULL
 * it is populated with aggregate counters describing the export operation.
 */
bool import_export_export_collection(struct ImportExportContext *context,
                                     const char *destination_dir,
                                     ImportExportStats *stats,
                                     char *error_message,
                                     size_t error_capacity);

/**
 * Imports a collection previously exported with import_export_export_collection().
 *
 * When @p merge_existing is true existing records are updated in-place, otherwise
 * conflicting UUIDs cause the import to abort.
 */
bool import_export_import_collection(struct ImportExportContext *context,
                                     const char *source_dir,
                                     bool merge_existing,
                                     ImportExportStats *stats,
                                     char *error_message,
                                     size_t error_capacity);

/** Writes CSV snapshots of topics and cards to @p destination_dir. */
bool import_export_export_csv(struct ImportExportContext *context,
                              const char *destination_dir,
                              ImportExportStats *stats,
                              char *error_message,
                              size_t error_capacity);

/**
 * Placeholder for future Anki .apkg export support. Always returns false with
 * a detailed message.
 */
bool import_export_export_apkg(struct ImportExportContext *context,
                               const char *destination_path,
                               char *error_message,
                               size_t error_capacity);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_IMPORT_EXPORT_H */
