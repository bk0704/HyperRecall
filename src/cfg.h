#ifndef HYPERRECALL_CFG_H
#define HYPERRECALL_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file cfg.h
 * @brief Declares configuration management interfaces for user and system settings.
 */

#include <stdbool.h>
#include <stddef.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

struct ConfigHandle;

/**
 * @brief Describes retention policy for database backups.
 */
typedef struct HrBackupPolicy {
    bool enable_auto;        /**< Enable automatic backups when opening the database. */
    unsigned int keep_days;  /**< Minimum number of days to retain backups (0 disables). */
    unsigned int max_files;  /**< Maximum number of backup files to retain (0 disables). */
} HrBackupPolicy;

/**
 * @brief Configuration for SQLite database persistence.
 */
typedef struct HrDatabaseConfig {
    char path[PATH_MAX];        /**< Absolute path to the primary database file. */
    char backup_dir[PATH_MAX];  /**< Directory where automatic backups are stored. */
    HrBackupPolicy backup;      /**< Backup retention policy. */
} HrDatabaseConfig;

/**
 * @brief Configuration for user interface behaviour.
 */
typedef struct HrUiConfig {
    unsigned int scale_percent; /**< Interface scale multiplier (percentage). */
} HrUiConfig;

/**
 * @brief Configuration for analytics capture.
 */
typedef struct HrAnalyticsConfig {
    bool enabled; /**< Whether analytics events should be recorded. */
} HrAnalyticsConfig;

/**
 * @brief Configuration for spaced repetition scheduling.
 */
typedef struct HrSrsConfig {
    unsigned int daily_new_cards;     /**< Maximum new cards introduced per day. */
    unsigned int daily_review_limit;  /**< Maximum review cards processed per day. */
} HrSrsConfig;

/**
 * @brief Commonly used filesystem paths.
 */
typedef struct HrPathConfig {
    char data_dir[PATH_MAX];     /**< Root directory for application data. */
    char config_dir[PATH_MAX];   /**< Root directory for configuration files. */
    char cache_dir[PATH_MAX];    /**< Directory for cache/temporary files. */
    char settings_path[PATH_MAX];/**< Fully qualified path to the settings file. */
} HrPathConfig;

/**
 * @brief Aggregate configuration shared across subsystems.
 */
typedef struct HrConfig {
    HrPathConfig paths;           /**< Filesystem paths used by the application. */
    HrDatabaseConfig database;    /**< Database configuration. */
    HrUiConfig ui;                /**< UI preferences. */
    HrAnalyticsConfig analytics;  /**< Analytics settings. */
    HrSrsConfig srs;              /**< Spaced repetition scheduler defaults. */
} HrConfig;

/**
 * @brief Loads configuration using defaults, disk persistence, and environment overrides.
 *
 * @param explicit_path Optional explicit configuration file path (may be NULL).
 *
 * @return A handle containing the loaded configuration on success, otherwise NULL.
 */
struct ConfigHandle *cfg_load(const char *explicit_path);

/**
 * @brief Reloads configuration from disk, retaining overrides and defaults.
 *
 * @param handle The configuration handle to refresh.
 *
 * @return 0 on success or a negative value if reloading fails.
 */
int cfg_reload(struct ConfigHandle *handle);

/**
 * @brief Persists configuration to disk at the configured settings path.
 *
 * @param handle The configuration handle to persist.
 *
 * @return 0 on success, otherwise a negative error code.
 */
int cfg_save(const struct ConfigHandle *handle);

/**
 * @brief Releases resources owned by the configuration handle.
 *
 * @param handle The configuration handle (may be NULL).
 */
void cfg_unload(struct ConfigHandle *handle);

/**
 * @brief Provides immutable access to the loaded configuration.
 *
 * @param handle The configuration handle.
 *
 * @return Pointer to the aggregate configuration, or NULL if handle is NULL.
 */
const HrConfig *cfg_data(const struct ConfigHandle *handle);

/**
 * @brief Provides mutable access to the loaded configuration for in-memory edits.
 *
 * @param handle The configuration handle.
 *
 * @return Pointer to the aggregate configuration, or NULL if handle is NULL.
 */
HrConfig *cfg_data_mutable(struct ConfigHandle *handle);

/**
 * @brief Marks the configuration as having in-memory modifications.
 *
 * @param handle The configuration handle to mark dirty.
 */
void cfg_mark_dirty(struct ConfigHandle *handle);

/**
 * @brief Queries whether the configuration has unsaved changes.
 *
 * @param handle The configuration handle to inspect.
 *
 * @return true if unsaved changes are present, otherwise false.
 */
bool cfg_is_dirty(const struct ConfigHandle *handle);

/**
 * @brief Convenience accessor for the resolved settings file path.
 */
const char *cfg_settings_path(const struct ConfigHandle *handle);

/**
 * @brief Convenience accessor for the resolved database path.
 */
const char *cfg_database_path(const struct ConfigHandle *handle);

/**
 * @brief Convenience accessor for the resolved database backup directory.
 */
const char *cfg_database_backup_dir(const struct ConfigHandle *handle);

/**
 * @brief Convenience accessor for the resolved database backup policy.
 */
const HrBackupPolicy *cfg_database_backup_policy(const struct ConfigHandle *handle);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_CFG_H */
