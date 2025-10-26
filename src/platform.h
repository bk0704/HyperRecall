#ifndef HYPERRECALL_PLATFORM_H
#define HYPERRECALL_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file platform.h
 * @brief Declares the platform abstraction layer for windowing and frame timing.
 */

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Describes the configuration required to initialize the platform layer.
 */
typedef struct HrPlatformConfig {
    const char *window_title; /**< Title used for the application window. */
    int window_width;         /**< Initial window width in pixels. */
    int window_height;        /**< Initial window height in pixels. */
    int target_fps;           /**< Desired frame rate when V-Sync is disabled (0 for uncapped). */
    bool resizable;           /**< Whether the window should be user resizable. */
    bool enable_vsync;        /**< Enable V-Sync hints during initialization. */
} HrPlatformConfig;

/**
 * @brief Captures timing and window state for the currently processed frame.
 */
typedef struct HrPlatformFrame {
    uint64_t index;      /**< Sequential frame index starting at 1. */
    double delta_time;   /**< Time elapsed since the previous frame in seconds. */
    int render_width;    /**< Width of the current render surface in pixels. */
    int render_height;   /**< Height of the current render surface in pixels. */
    bool resized;        /**< Indicates whether the window was resized this frame. */
} HrPlatformFrame;

struct PlatformHandle;

/**
 * @brief Initializes the platform layer and opens the primary application window.
 *
 * @param config Optional configuration describing the window parameters. When NULL,
 *        reasonable defaults are applied.
 *
 * @return A valid platform handle on success, otherwise NULL.
 */
struct PlatformHandle *platform_create(const HrPlatformConfig *config);

/**
 * @brief Releases resources owned by the platform layer and shuts down the window.
 */
void platform_destroy(struct PlatformHandle *handle);

/**
 * @brief Begins processing for a new frame.
 *
 * @param handle    The platform handle returned by platform_create().
 * @param out_frame Optional pointer that receives timing and window metrics.
 *
 * @return true if the frame should be processed, otherwise false when the application
 *         should terminate.
 */
bool platform_begin_frame(struct PlatformHandle *handle, HrPlatformFrame *out_frame);

/**
 * @brief Completes the active frame and presents the rendered contents.
 */
void platform_end_frame(struct PlatformHandle *handle);

/**
 * @brief Signals that the application should exit after the current frame.
 */
void platform_request_close(struct PlatformHandle *handle);

/**
 * @brief Queries whether the platform window is currently active.
 */
bool platform_is_active(const struct PlatformHandle *handle);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_PLATFORM_H */
