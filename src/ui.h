#ifndef HYPERRECALL_UI_H
#define HYPERRECALL_UI_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file ui.h
 * @brief Declares the primary UI rendering and interaction layer built on raylib/raygui.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"
#include "db.h"
#include "platform.h"
#include "render.h"
#include "sessions.h"
#include "theme.h"

struct ImportExportContext;
struct AnalyticsHandle;

/**
 * Enumerates the high level screen groupings shown by the UI.
 */
typedef enum UiScreenId {
    UI_SCREEN_STUDY = 0,
    UI_SCREEN_ANALYTICS = 1,
    UI_SCREEN_LIBRARY = 2,
} UiScreenId;

/**
 * Describes configuration flags used to control UI behaviour.
 */
typedef struct UiConfig {
    bool enable_devtools; /**< Enables developer overlays and trace viewers. */
} UiConfig;

/** Forward declaration for the UI context. */
typedef struct UiContext UiContext;

/** Creates a new UI context instance. */
UiContext *ui_create(const UiConfig *config);

/** Releases resources owned by the UI context. */
void ui_destroy(UiContext *ui);

/** Provides the theme manager powering palette selection and live editing. */
void ui_attach_theme_manager(UiContext *ui, struct HrThemeManager *themes);

/** Provides the session manager used to drive the study loop. */
void ui_attach_session_manager(UiContext *ui,
                               struct SessionManager *sessions,
                               const SessionCallbacks *forward_callbacks);

/** Provides the analytics handle used for dashboards and charts. */
void ui_attach_analytics(UiContext *ui, struct AnalyticsHandle *analytics);

/** Provides the database handle used for topic/card listings. */
void ui_attach_database(UiContext *ui, DatabaseHandle *database);

/** Provides an import/export context for deck interactions (optional). */
void ui_attach_import_export(UiContext *ui, struct ImportExportContext *io_context);

/** Sets the fonts used by the render context (NULL uses defaults). */
void ui_set_fonts(UiContext *ui, const HrRenderFontSet *fonts, float base_font_size);

/** Returns the immutable render context used by the UI. */
const HrRenderContext *ui_render_context(const UiContext *ui);

/** Returns the active theme palette currently applied to the UI. */
const HrThemePalette *ui_active_palette(const UiContext *ui);

/** Processes a frame: handles input, issues draw calls, and updates analytics overlays. */
bool ui_process_frame(UiContext *ui, const HrPlatformFrame *frame);

/** Toggles the visibility of the command palette overlay. */
void ui_toggle_command_palette(UiContext *ui);

/** Enqueues a toast notification message. */
void ui_push_toast(UiContext *ui, const char *message, Color background, float duration_seconds);

/** Displays a modal dialog with the supplied title/body content. */
void ui_show_modal(UiContext *ui, const char *title, const char *body);

/** Dismisses the currently visible modal dialog. */
void ui_close_modal(UiContext *ui);

/** Requests a high level screen change (study/library/analytics). */
void ui_request_screen(UiContext *ui, UiScreenId screen);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_UI_H */
