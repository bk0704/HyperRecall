#ifndef HYPERRECALL_APP_H
#define HYPERRECALL_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file app.h
 * @brief Declares the lifecycle management interfaces for the HyperRecall application core.
 */

#include <stdbool.h>

struct ConfigHandle;
struct PlatformHandle;
struct DatabaseHandle;
struct SrsHandle;
struct SessionManager;
struct UiContext;
struct AnalyticsHandle;
struct HrThemeManager;

/**
 * @brief Aggregates subsystem handles required to drive the application.
 */
typedef struct AppContext {
    struct ConfigHandle *config;      /**< Loaded configuration values. */
    struct PlatformHandle *platform;  /**< Platform/windowing state. */
    struct DatabaseHandle *database;  /**< Database connection handle. */
    struct SrsHandle *srs;            /**< Spaced repetition scheduler state. */
    struct SessionManager *sessions;  /**< Study session orchestration. */
    struct UiContext *ui;             /**< UI rendering subsystem. */
    struct AnalyticsHandle *analytics;/**< Analytics collection and export. */
    struct HrThemeManager *themes;    /**< Theme palette manager. */
    bool running;                     /**< Tracks whether the main loop is active. */
} AppContext;

/**
 * @brief Bootstraps the application and all of its subsystems.
 *
 * @return A fully initialized application context on success, otherwise NULL.
 */
AppContext *app_create(void);

/**
 * @brief Runs the main application loop until shutdown.
 *
 * @param app The application context previously created with app_create().
 *
 * @return 0 on success or a non-zero error code when execution fails.
 */
int app_run(AppContext *app);

/**
 * @brief Releases all resources owned by the application.
 *
 * @param app The application context to clean up (may be NULL).
 */
void app_destroy(AppContext *app);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_APP_H */
