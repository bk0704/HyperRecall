#include "app.h"

#include <raylib.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "analytics.h"
#include "cfg.h"
#include "db.h"
#include "platform.h"
#include "sessions.h"
#include "theme.h"
#include "ui.h"

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
        snprintf(theme_prefs_path, sizeof(theme_prefs_path), "%s/theme_palette.json",
                 config_data->paths.config_dir);
        theme_manager_set_preferences_file(app->themes, theme_prefs_path);
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
    }

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

        ClearBackground(RAYWHITE);

        if (!srs_update(app->srs, &frame_info)) {
            result = 2;
            frame_ok = false;
        } else if (!ui_process_frame(app->ui, &frame_info)) {
            result = 3;
            frame_ok = false;
        }

        analytics_record_frame(app->analytics, &frame_info);

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
