#include "app.h"

#include <stdbool.h>
#include <stdlib.h>

#include "cfg.h"
#include "db.h"

struct PlatformHandle {
    bool initialized;
    unsigned int frame_budget;
    unsigned int frame_counter;
};

struct SrsHandle {
    unsigned int updates_processed;
};

struct SessionManager {
    unsigned int frames_processed;
};

struct UiContext {
    unsigned int frames_rendered;
};

struct AnalyticsHandle {
    bool enabled;
    unsigned int frames_tracked;
};

static struct PlatformHandle *platform_bootstrap(void)
{
    struct PlatformHandle *platform = malloc(sizeof(*platform));
    if (platform != NULL) {
        platform->initialized = true;
        platform->frame_budget = 8U;
        platform->frame_counter = 0U;
    }
    return platform;
}

static bool platform_process_events(struct PlatformHandle *platform)
{
    if (platform == NULL || !platform->initialized) {
        return false;
    }

    if (platform->frame_budget == 0U) {
        platform->initialized = false;
        return false;
    }

    platform->frame_budget--;
    platform->frame_counter++;
    return true;
}

static unsigned int platform_frame_index(const struct PlatformHandle *platform)
{
    return platform != NULL ? platform->frame_counter : 0U;
}

static void platform_shutdown(struct PlatformHandle *platform)
{
    if (platform != NULL) {
        platform->initialized = false;
        free(platform);
    }
}

static struct SrsHandle *srs_initialize(void)
{
    struct SrsHandle *srs = malloc(sizeof(*srs));
    if (srs != NULL) {
        srs->updates_processed = 0U;
    }
    return srs;
}

static bool srs_update(struct SrsHandle *srs)
{
    if (srs == NULL) {
        return false;
    }
    srs->updates_processed++;
    return true;
}

static void srs_shutdown(struct SrsHandle *srs)
{
    free(srs);
}

static struct SessionManager *sessions_create(void)
{
    struct SessionManager *sessions = malloc(sizeof(*sessions));
    if (sessions != NULL) {
        sessions->frames_processed = 0U;
    }
    return sessions;
}

static bool sessions_update(struct SessionManager *sessions)
{
    if (sessions == NULL) {
        return false;
    }
    sessions->frames_processed++;
    return true;
}

static void sessions_destroy(struct SessionManager *sessions)
{
    free(sessions);
}

static struct UiContext *ui_create(void)
{
    struct UiContext *ui = malloc(sizeof(*ui));
    if (ui != NULL) {
        ui->frames_rendered = 0U;
    }
    return ui;
}

static bool ui_render(struct UiContext *ui)
{
    if (ui == NULL) {
        return false;
    }
    ui->frames_rendered++;
    return true;
}

static void ui_destroy(struct UiContext *ui)
{
    free(ui);
}

static struct AnalyticsHandle *analytics_create(void)
{
    struct AnalyticsHandle *analytics = malloc(sizeof(*analytics));
    if (analytics != NULL) {
        analytics->enabled = true;
        analytics->frames_tracked = 0U;
    }
    return analytics;
}

static void analytics_record_frame(struct AnalyticsHandle *analytics, unsigned int frame_index)
{
    if (analytics == NULL || !analytics->enabled) {
        return;
    }
    (void)frame_index;
    analytics->frames_tracked++;
}

static void analytics_flush(struct AnalyticsHandle *analytics)
{
    if (analytics == NULL || !analytics->enabled) {
        return;
    }
    analytics->frames_tracked = 0U;
}

static void analytics_shutdown(struct AnalyticsHandle *analytics)
{
    if (analytics != NULL) {
        analytics->enabled = false;
        free(analytics);
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

    app->platform = platform_bootstrap();
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

    app->sessions = sessions_create();
    if (app->sessions == NULL) {
        app_destroy(app);
        return NULL;
    }

    app->ui = ui_create();
    if (app->ui == NULL) {
        app_destroy(app);
        return NULL;
    }

    app->analytics = analytics_create();
    if (app->analytics == NULL) {
        app_destroy(app);
        return NULL;
    }

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

    while (platform_process_events(app->platform)) {
        if (!srs_update(app->srs)) {
            result = 2;
            break;
        }

        if (!sessions_update(app->sessions)) {
            result = 3;
            break;
        }

        if (!ui_render(app->ui)) {
            result = 4;
            break;
        }

        analytics_record_frame(app->analytics, platform_frame_index(app->platform));
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

    sessions_destroy(app->sessions);
    app->sessions = NULL;

    srs_shutdown(app->srs);
    app->srs = NULL;

    db_close(app->database);
    app->database = NULL;

    platform_shutdown(app->platform);
    app->platform = NULL;

    cfg_unload(app->config);
    app->config = NULL;

    free(app);
}
