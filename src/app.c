#include "app.h"

#include <raylib.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cfg.h"
#include "db.h"
#include "platform.h"

struct SrsHandle {
    double time_accumulator;
    uint64_t updates_processed;
};

struct SessionManager {
    double elapsed_time;
    uint64_t frames_processed;
};

struct UiContext {
    uint64_t frames_rendered;
    double elapsed_time;
};

struct AnalyticsHandle {
    bool enabled;
    uint64_t frames_tracked;
    double total_time;
    uint64_t last_frame_index;
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

static struct SessionManager *sessions_create(void)
{
    return calloc(1U, sizeof(struct SessionManager));
}

static bool sessions_update(struct SessionManager *sessions, const HrPlatformFrame *frame)
{
    if (sessions == NULL || frame == NULL) {
        return false;
    }

    sessions->elapsed_time += frame->delta_time;
    sessions->frames_processed++;
    return true;
}

static void sessions_destroy(struct SessionManager *sessions)
{
    free(sessions);
}

static struct UiContext *ui_create(void)
{
    return calloc(1U, sizeof(struct UiContext));
}

static bool ui_render(struct UiContext *ui, const HrPlatformFrame *frame)
{
    if (ui == NULL || frame == NULL) {
        return false;
    }

    ui->frames_rendered++;
    ui->elapsed_time += frame->delta_time;

    const int fps_x = frame->render_width > 120 ? frame->render_width - 120 : 0;
    DrawFPS(fps_x, 16);
    DrawText("HyperRecall scaffolding build", 24, 24, 28, DARKGRAY);

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Frame %llu  |  %.2f ms",
        (unsigned long long)frame->index, frame->delta_time * 1000.0);
    DrawText(buffer, 24, 64, 20, GRAY);

    return true;
}

static void ui_destroy(struct UiContext *ui)
{
    free(ui);
}

static struct AnalyticsHandle *analytics_create(void)
{
    struct AnalyticsHandle *analytics = calloc(1U, sizeof(*analytics));
    if (analytics != NULL) {
        analytics->enabled = true;
    }
    return analytics;
}

static void analytics_record_frame(struct AnalyticsHandle *analytics, const HrPlatformFrame *frame)
{
    if (analytics == NULL || frame == NULL || !analytics->enabled) {
        return;
    }

    analytics->frames_tracked++;
    analytics->total_time += frame->delta_time;
    analytics->last_frame_index = frame->index;
}

static void analytics_flush(struct AnalyticsHandle *analytics)
{
    if (analytics == NULL || !analytics->enabled) {
        return;
    }

    analytics->frames_tracked = 0U;
    analytics->total_time = 0.0;
    analytics->last_frame_index = 0U;
}

static void analytics_shutdown(struct AnalyticsHandle *analytics)
{
    if (analytics != NULL) {
        analytics->enabled = false;
    }
    free(analytics);
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

    HrPlatformFrame frame_info = {0};

    while (platform_begin_frame(app->platform, &frame_info)) {
        bool frame_ok = true;

        ClearBackground(RAYWHITE);

        if (!srs_update(app->srs, &frame_info)) {
            result = 2;
            frame_ok = false;
        } else if (!sessions_update(app->sessions, &frame_info)) {
            result = 3;
            frame_ok = false;
        } else if (!ui_render(app->ui, &frame_info)) {
            result = 4;
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

    sessions_destroy(app->sessions);
    app->sessions = NULL;

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
