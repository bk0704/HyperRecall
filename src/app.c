#include "app.h"

#ifndef HYPERRECALL_UI_QT6
#include <raylib.h>
#endif

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(_WIN32)
#include <direct.h>
#endif

#include "analytics.h"
#include "cfg.h"
#include "db.h"
#include "platform.h"
#include "sessions.h"
#include "srs.h"
#include "theme.h"
#include "ui.h"

// Define color constants for Qt backend compatibility
#ifdef HYPERRECALL_UI_QT6
#define RED ((Color){230, 41, 55, 255})
#define GREEN ((Color){0, 228, 48, 255})
#define RAYWHITE ((Color){245, 245, 245, 255})
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

struct SrsHandle {
    double time_accumulator;
    uint64_t updates_processed;
};

static struct SrsHandle *srs_initialize(void)
{
    return calloc(1U, sizeof(struct SrsHandle));
}

static bool srs_update(struct SrsHandle *srs, const HrPlatformFrame *frame)
{
    if (srs == NULL || frame == NULL) {
        return false;
    }

    srs->time_accumulator += frame->delta_time;
    srs->updates_processed++;
    return true;
}

static void srs_shutdown(struct SrsHandle *srs)
{
    free(srs);
}

static Color app_theme_color(const AppContext *app, HrThemeColorRole role, Color fallback)
{
    if (app == NULL || app->ui == NULL) {
        return fallback;
    }

    const HrThemePalette *palette = ui_active_palette(app->ui);
    if (palette != NULL) {
        return theme_palette_color(palette, role);
    }

    return fallback;
}

static void app_push_toast(AppContext *app, const char *message, HrThemeColorRole role, Color fallback, float duration)
{
    if (app == NULL || app->ui == NULL || message == NULL) {
        return;
    }

    Color background = app_theme_color(app, role, fallback);
    ui_push_toast(app->ui, message, background, duration);
}

static bool ensure_directory_exists(const char *path)
{
    if (path == NULL || path[0] == '\0') {
        return false;
    }

    struct stat info;
    if (stat(path, &info) == 0) {
        return S_ISDIR(info.st_mode);
    }

#if defined(_WIN32)
    int rc = _mkdir(path);
#else
    int rc = mkdir(path, 0700);
#endif
    if (rc == 0) {
        return true;
    }

    if (errno == EEXIST) {
        return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
    }

    return false;
}

static bool app_prepare_autosave_directory(AppContext *app)
{
    if (app == NULL || app->config == NULL) {
        return false;
    }

    const HrConfig *config = cfg_data(app->config);
    if (config == NULL || config->paths.autosave_dir[0] == '\0') {
        return false;
    }

    return ensure_directory_exists(config->paths.autosave_dir);
}

static bool compose_autosave_path(char *buffer, size_t capacity, const char *directory, uint64_t card_id)
{
    if (buffer == NULL || capacity == 0U || directory == NULL || directory[0] == '\0') {
        return false;
    }

    int written = snprintf(buffer, capacity, "%s/autosave-%" PRIu64 ".json", directory, card_id);
    if (written < 0 || (size_t)written >= capacity) {
        return false;
    }

    return true;
}

static bool app_write_autosave_snapshot(AppContext *app,
                                        const SessionReviewEvent *event,
                                        const SRSPersistedState *persisted)
{
    if (app == NULL || app->config == NULL || event == NULL || persisted == NULL) {
        return false;
    }

    const HrConfig *config = cfg_data(app->config);
    if (config == NULL || config->paths.autosave_dir[0] == '\0') {
        return false;
    }

    if (!app->autosave.directory_ready) {
        app->autosave.directory_ready = app_prepare_autosave_directory(app);
        if (!app->autosave.directory_ready) {
            return false;
        }
    }

    char path[PATH_MAX];
    if (!compose_autosave_path(path, sizeof(path), config->paths.autosave_dir, event->card_id)) {
        return false;
    }

    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return false;
    }

    int rc = fprintf(file,
                     "{\n"
                     "  \"card_id\": %" PRIu64 ",\n"
                     "  \"version\": %u,\n"
                     "  \"mode\": %u,\n"
                     "  \"consecutive_correct\": %u,\n"
                     "  \"due_unix\": %" PRId64 ",\n"
                     "  \"last_review_unix\": %" PRId64 ",\n"
                     "  \"ease_factor\": %.9g,\n"
                     "  \"interval_days\": %.9g,\n"
                     "  \"cram_interval_minutes\": %.9g,\n"
                     "  \"cram_bleed_minutes\": %.9g,\n"
                     "  \"topic_adjustment\": %.9g\n"
                     "}\n",
                     event->card_id,
                     persisted->version,
                     persisted->mode,
                     persisted->consecutive_correct,
                     persisted->due_unix,
                     persisted->last_review_unix,
                     persisted->ease_factor,
                     persisted->interval_days,
                     persisted->cram_interval_minutes,
                     persisted->cram_bleed_minutes,
                     persisted->topic_adjustment);

    fclose(file);
    return rc >= 0;
}

static bool app_session_autosave_callback(const SessionReviewEvent *event,
                                          const SRSPersistedState *persisted,
                                          void *user_data)
{
    AppContext *app = (AppContext *)user_data;
    if (app == NULL || event == NULL || persisted == NULL) {
        return false;
    }

    if (!app->autosave.enabled) {
        return true;
    }

    if (!app_write_autosave_snapshot(app, event, persisted)) {
        app_push_toast(app,
                       "Failed to persist autosave snapshot",
                       HR_THEME_COLOR_DANGER,
                       RED,
                       4.0f);
        return false;
    }

    return true;
}

static void app_update_autosave_timer(AppContext *app, double delta_time)
{
    if (app == NULL || !app->autosave.enabled || app->autosave.interval_seconds <= 0.0) {
        return;
    }

    app->autosave.elapsed_seconds += delta_time;
    if (app->autosave.elapsed_seconds < app->autosave.interval_seconds) {
        return;
    }

    app->autosave.elapsed_seconds = 0.0;

    if (app->database == NULL) {
        return;
    }

    int rc = db_create_backup(app->database, "autosave");
    if (rc != SQLITE_OK) {
        char message[128];
        snprintf(message, sizeof(message), "Autosave backup failed (rc=%d)", rc);
        app_push_toast(app, message, HR_THEME_COLOR_DANGER, RED, 4.0f);
        app->autosave.last_backup_failed = true;
        return;
    }

    app->autosave.backups_completed++;
    if (app->autosave.last_backup_failed || app->autosave.backups_completed == 1U) {
        char message[128];
        snprintf(message,
                 sizeof(message),
                 "Workspace autosaved (%zu total)",
                 app->autosave.backups_completed);
        app_push_toast(app, message, HR_THEME_COLOR_SUCCESS, GREEN, 2.5f);
    }

    app->autosave.last_backup_failed = false;
}

static void theme_usage_callback(const HrThemePalette *palette, void *user_data)
{
    AppContext *app = (AppContext *)user_data;
    if (app == NULL || palette == NULL || app->config == NULL) {
        return;
    }

    HrConfig *config = cfg_data_mutable(app->config);
    if (config != NULL) {
        snprintf(config->ui.theme_palette, sizeof(config->ui.theme_palette), "%s", palette->id);
    }
}

AppContext *app_create(void)
{
    AppContext *app = calloc(1U, sizeof(*app));
    if (app == NULL) {
        return NULL;
    }

    app->config = cfg_load(NULL);
    if (app->config == NULL) {
        app_destroy(app);
        return NULL;
    }

    app->platform = platform_create(NULL);
    if (app->platform == NULL) {
        app_destroy(app);
        return NULL;
    }

    app->database = db_open(app->config);
    if (app->database == NULL) {
        app_destroy(app);
        return NULL;
    }

    app->srs = srs_initialize();
    if (app->srs == NULL) {
        app_destroy(app);
        return NULL;
    }

    app->sessions = session_manager_create();
    if (app->sessions == NULL) {
        app_destroy(app);
        return NULL;
    }

    app->themes = theme_manager_create();
    if (app->themes == NULL) {
        app_destroy(app);
        return NULL;
    }

    const HrConfig *config_data = cfg_data(app->config);
    if (config_data != NULL) {
        char theme_prefs_path[PATH_MAX];
        int written = snprintf(theme_prefs_path, sizeof(theme_prefs_path), "%s/theme_palette.json",
                 config_data->paths.config_dir);
        if (written < 0 || (size_t)written >= sizeof(theme_prefs_path)) {
            fprintf(stderr, "Theme preferences path too long\n");
        } else {
            theme_manager_set_preferences_file(app->themes, theme_prefs_path);
        }
        theme_manager_set_user_directory(app->themes, config_data->paths.config_dir);
    }

    theme_manager_load_palettes(app->themes, "assets/themes.json");
    if (config_data != NULL && config_data->ui.theme_palette[0] != '\0') {
        theme_manager_apply(app->themes, config_data->ui.theme_palette);
    }
    theme_manager_set_analytics_callback(app->themes, theme_usage_callback, app);

    UiConfig ui_config = {
        .enable_devtools = false,
    };
    app->ui = ui_create(&ui_config);
    if (app->ui == NULL) {
        app_destroy(app);
        return NULL;
    }

    HrAnalyticsConfig analytics_config = {.enabled = true};
    if (config_data != NULL) {
        analytics_config = config_data->analytics;
        app->autosave.enabled = config_data->workspace.autosave_minutes > 0U;
        app->autosave.interval_seconds = app->autosave.enabled
                                             ? (double)config_data->workspace.autosave_minutes * 60.0
                                             : 0.0;
    } else {
        app->autosave.enabled = false;
        app->autosave.interval_seconds = 0.0;
    }
    app->autosave.elapsed_seconds = 0.0;
    app->autosave.directory_ready = false;
    app->autosave.last_backup_failed = false;
    app->autosave.backups_completed = 0U;

    app->analytics = analytics_create(&analytics_config);
    if (app->analytics == NULL) {
        app_destroy(app);
        return NULL;
    }

    ui_attach_analytics(app->ui, app->analytics);

    SessionCallbacks session_callbacks;
    memset(&session_callbacks, 0, sizeof(session_callbacks));
    session_callbacks.analytics_event = analytics_record_session_event;
    session_callbacks.analytics_user_data = app->analytics;
    session_callbacks.autosave_event = app_session_autosave_callback;
    session_callbacks.autosave_user_data = app;

    ui_attach_theme_manager(app->ui, app->themes);
    ui_attach_session_manager(app->ui, app->sessions, &session_callbacks);
    ui_attach_database(app->ui, app->database);

    float base_font_size = 20.0f;
    if (config_data != NULL && config_data->ui.font_size_pt > 0U) {
        base_font_size = (float)config_data->ui.font_size_pt;
    }
    ui_set_fonts(app->ui, NULL, base_font_size);

    app->running = false;
    return app;
}

int app_run(AppContext *app)
{
    if (app == NULL || app->config == NULL || app->platform == NULL || app->database == NULL ||
        app->srs == NULL || app->sessions == NULL || app->ui == NULL || app->analytics == NULL) {
        return 1;
    }

    app->running = true;
    int result = 0;

    HrPlatformFrame frame_info = {0};

    while (platform_begin_frame(app->platform, &frame_info)) {
        bool frame_ok = true;

#ifndef HYPERRECALL_UI_QT6
        ClearBackground(RAYWHITE);
#endif

        if (!srs_update(app->srs, &frame_info)) {
            result = 2;
            frame_ok = false;
        } else if (!ui_process_frame(app->ui, &frame_info)) {
            result = 3;
            frame_ok = false;
        }

        analytics_record_frame(app->analytics, &frame_info);
        app_update_autosave_timer(app, frame_info.delta_time);

        platform_end_frame(app->platform);

        if (!frame_ok) {
            platform_request_close(app->platform);
            break;
        }
    }

    analytics_flush(app->analytics);
    app->running = false;
    return result;
}

void app_destroy(AppContext *app)
{
    if (app == NULL) {
        return;
    }

    analytics_shutdown(app->analytics);
    app->analytics = NULL;

    ui_destroy(app->ui);
    app->ui = NULL;

    session_manager_destroy(app->sessions);
    app->sessions = NULL;

    if (app->themes != NULL) {
        theme_manager_write_preferences(app->themes);
        theme_manager_destroy(app->themes);
        app->themes = NULL;
    }

    srs_shutdown(app->srs);
    app->srs = NULL;

    db_close(app->database);
    app->database = NULL;

    platform_destroy(app->platform);
    app->platform = NULL;

    cfg_unload(app->config);
    app->config = NULL;

    free(app);
}
