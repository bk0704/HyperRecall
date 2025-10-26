#include "platform.h"

#include <raylib.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HR_MAX_WINDOW_TITLE 256

struct PlatformHandle {
    char window_title[HR_MAX_WINDOW_TITLE];
    int window_width;
    int window_height;
    int target_fps;
    bool resizable;
    bool enable_vsync;
    bool window_ready;
    bool frame_in_progress;
    bool close_requested;
    uint64_t frame_index;
    double previous_time;
};

static void hr_apply_window_config(const HrPlatformConfig *config, HrPlatformConfig *out_effective)
{
    const HrPlatformConfig defaults = {
        .window_title = "HyperRecall",
        .window_width = 1280,
        .window_height = 720,
        .target_fps = 60,
        .resizable = true,
        .enable_vsync = true,
    };

    if (out_effective == NULL) {
        return;
    }

    *out_effective = defaults;

    if (config == NULL) {
        return;
    }

    if (config->window_title != NULL) {
        out_effective->window_title = config->window_title;
    }
    if (config->window_width > 0) {
        out_effective->window_width = config->window_width;
    }
    if (config->window_height > 0) {
        out_effective->window_height = config->window_height;
    }
    if (config->target_fps >= 0) {
        out_effective->target_fps = config->target_fps;
    }
    out_effective->resizable = config->resizable;
    out_effective->enable_vsync = config->enable_vsync;
}

struct PlatformHandle *platform_create(const HrPlatformConfig *config)
{
    HrPlatformConfig effective_config;
    hr_apply_window_config(config, &effective_config);

    struct PlatformHandle *handle = calloc(1U, sizeof(*handle));
    if (handle == NULL) {
        return NULL;
    }

    snprintf(handle->window_title, sizeof(handle->window_title), "%s",
        effective_config.window_title != NULL ? effective_config.window_title : "HyperRecall");
    handle->window_width = effective_config.window_width;
    handle->window_height = effective_config.window_height;
    handle->target_fps = effective_config.target_fps;
    handle->resizable = effective_config.resizable;
    handle->enable_vsync = effective_config.enable_vsync;

    unsigned int flags = 0U;
    if (handle->resizable) {
        flags |= FLAG_WINDOW_RESIZABLE;
    }
    if (handle->enable_vsync) {
        flags |= FLAG_VSYNC_HINT;
    }
    SetConfigFlags(flags);
    SetTraceLogLevel(LOG_WARNING);
    SetExitKey(KEY_NULL);

    InitWindow(handle->window_width, handle->window_height, handle->window_title);
    if (!IsWindowReady()) {
        free(handle);
        return NULL;
    }

    handle->window_ready = true;
    handle->previous_time = GetTime();

    if (handle->target_fps > 0) {
        SetTargetFPS(handle->target_fps);
    }

    return handle;
}

void platform_destroy(struct PlatformHandle *handle)
{
    if (handle == NULL) {
        return;
    }

    if (handle->frame_in_progress) {
        EndDrawing();
        handle->frame_in_progress = false;
    }

    if (handle->window_ready) {
        CloseWindow();
        handle->window_ready = false;
    }

    free(handle);
}

bool platform_begin_frame(struct PlatformHandle *handle, HrPlatformFrame *out_frame)
{
    if (handle == NULL || !handle->window_ready) {
        return false;
    }

    if (handle->close_requested || WindowShouldClose()) {
        handle->close_requested = true;
        return false;
    }

    const double now = GetTime();
    double delta = 0.0;
    if (handle->frame_index > 0U) {
        delta = now - handle->previous_time;
    }
    handle->previous_time = now;

    handle->frame_index++;

    if (out_frame != NULL) {
        out_frame->index = handle->frame_index;
        out_frame->delta_time = delta;
        out_frame->render_width = GetRenderWidth();
        out_frame->render_height = GetRenderHeight();
        out_frame->resized = IsWindowResized();
    }

    BeginDrawing();
    handle->frame_in_progress = true;

    return true;
}

void platform_end_frame(struct PlatformHandle *handle)
{
    if (handle == NULL || !handle->frame_in_progress) {
        return;
    }

    EndDrawing();
    handle->frame_in_progress = false;
}

void platform_request_close(struct PlatformHandle *handle)
{
    if (handle == NULL) {
        return;
    }

    handle->close_requested = true;
}

bool platform_is_active(const struct PlatformHandle *handle)
{
    if (handle == NULL || !handle->window_ready) {
        return false;
    }

    if (handle->close_requested) {
        return false;
    }

    return !WindowShouldClose();
}
