#ifndef HYPERRECALL_THEME_H
#define HYPERRECALL_THEME_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file theme.h
 * @brief Declares theme management abstractions controlling colors, fonts, and layouts.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/** Maximum length used for theme identifiers (including the null terminator). */
#define HR_THEME_MAX_ID_LENGTH 64

/** Maximum length used for human readable theme names. */
#define HR_THEME_MAX_NAME_LENGTH 96

/** Maximum length used for theme descriptions. */
#define HR_THEME_MAX_DESCRIPTION_LENGTH 256

/** Number of entries exposed in the raygui style table produced by a theme. */
#define HR_THEME_STYLE_TABLE_SIZE 64

/** Enumerates color roles exposed by the UI theme system. */
typedef enum HrThemeColorRole {
    HR_THEME_COLOR_BACKGROUND = 0,      /**< Primary window background. */
    HR_THEME_COLOR_BACKGROUND_ALT = 1,  /**< Alternate background accent. */
    HR_THEME_COLOR_SURFACE = 2,         /**< Card/table surface color. */
    HR_THEME_COLOR_SURFACE_ALT = 3,     /**< Hovered/selected surface color. */
    HR_THEME_COLOR_TEXT = 4,            /**< Default foreground text color. */
    HR_THEME_COLOR_TEXT_MUTED = 5,      /**< Muted/secondary text color. */
    HR_THEME_COLOR_ACCENT = 6,          /**< Primary accent color. */
    HR_THEME_COLOR_ACCENT_ALT = 7,      /**< Secondary accent color. */
    HR_THEME_COLOR_BORDER = 8,          /**< Border/outline color. */
    HR_THEME_COLOR_SUCCESS = 9,         /**< Success/"good" feedback color. */
    HR_THEME_COLOR_WARNING = 10,        /**< Warning feedback color. */
    HR_THEME_COLOR_DANGER = 11,         /**< Error/"fail" feedback color. */
    HR_THEME_COLOR_INFO = 12,           /**< Informational highlight color. */
    HR_THEME_COLOR_CLOZE_GAP = 13,      /**< Cloze deletion highlight background. */
    HR_THEME_COLOR_CLOZE_TEXT = 14,     /**< Cloze deletion text color. */
    HR_THEME_COLOR_CODE_BACKGROUND = 15,/**< Code block background color. */
    HR_THEME_COLOR_CODE_TEXT = 16,      /**< Code block text color. */
    HR_THEME_COLOR_ANALYTICS_PRIMARY = 17, /**< Primary analytics chart color. */
    HR_THEME_COLOR_ANALYTICS_SECONDARY = 18, /**< Secondary analytics chart color. */
    HR_THEME_COLOR_TOAST_BACKGROUND = 19, /**< Toast/notification background. */
    HR_THEME_COLOR_TOAST_TEXT = 20,        /**< Toast/notification text. */
    HR_THEME_COLOR_COUNT
} HrThemeColorRole;

/**
 * Describes a fully resolved theme palette including raygui style values.
 */
typedef struct HrThemePalette {
    char id[HR_THEME_MAX_ID_LENGTH];                         /**< Stable identifier. */
    char name[HR_THEME_MAX_NAME_LENGTH];                     /**< Display name. */
    char description[HR_THEME_MAX_DESCRIPTION_LENGTH];       /**< Description shown in UI. */
    Color colors[HR_THEME_COLOR_COUNT];                      /**< Color table indexed by HrThemeColorRole. */
    unsigned int style[HR_THEME_STYLE_TABLE_SIZE];           /**< Packed raygui style values. */
    bool user_defined;                                       /**< True when persisted by the user. */
} HrThemePalette;

struct HrThemeManager;

/** Callback invoked whenever the active palette changes. */
typedef void (*HrThemeChangedCallback)(const HrThemePalette *palette, void *user_data);

/** Callback invoked after a palette passes validation for analytics tracking. */
typedef void (*HrThemeAnalyticsCallback)(const HrThemePalette *palette, void *user_data);

/** Allocates a new theme manager instance populated with the built-in palette. */
struct HrThemeManager *theme_manager_create(void);

/** Releases resources owned by the theme manager. */
void theme_manager_destroy(struct HrThemeManager *manager);

/** Sets the path used to persist the active theme preference (optional). */
void theme_manager_set_preferences_file(struct HrThemeManager *manager, const char *path);

/** Sets the directory used when saving user defined palettes (optional). */
void theme_manager_set_user_directory(struct HrThemeManager *manager, const char *directory);

/** Loads additional palettes from the supplied JSON file. */
bool theme_manager_load_palettes(struct HrThemeManager *manager, const char *json_path);

/** Returns how many palettes are currently registered. */
size_t theme_manager_palette_count(const struct HrThemeManager *manager);

/** Returns the palette at @p index when available. */
const HrThemePalette *theme_manager_palette(const struct HrThemeManager *manager, size_t index);

/** Finds a palette by identifier (case insensitive). */
const HrThemePalette *theme_manager_find(const struct HrThemeManager *manager, const char *palette_id);

/** Applies the palette with the supplied identifier making it the active theme. */
bool theme_manager_apply(struct HrThemeManager *manager, const char *palette_id);

/** Returns the currently active palette (never NULL after successful initialization). */
const HrThemePalette *theme_manager_active(const struct HrThemeManager *manager);

/** Persists the active palette selection to disk (if a preference path was supplied). */
bool theme_manager_write_preferences(struct HrThemeManager *manager);

/** Persists the supplied palette to disk inside the configured user directory. */
bool theme_manager_save_palette(const struct HrThemeManager *manager,
                                const HrThemePalette *palette,
                                const char *override_path);

/** Registers a callback invoked whenever the active palette changes. */
void theme_manager_set_changed_callback(struct HrThemeManager *manager,
                                        HrThemeChangedCallback callback,
                                        void *user_data);

/** Registers a callback invoked after palette validation for analytics tracking. */
void theme_manager_set_analytics_callback(struct HrThemeManager *manager,
                                          HrThemeAnalyticsCallback callback,
                                          void *user_data);

/** Begins editing the palette with the supplied identifier. */
bool theme_manager_begin_edit(struct HrThemeManager *manager, const char *palette_id);

/** Cancels any in-progress palette edit reverting to the original values. */
void theme_manager_cancel_edit(struct HrThemeManager *manager);

/** Commits the in-progress palette edit, optionally persisting the changes. */
bool theme_manager_commit_edit(struct HrThemeManager *manager, bool persist_changes);

/** Sets a color override for the palette currently being edited. */
bool theme_manager_edit_set_color(struct HrThemeManager *manager,
                                  HrThemeColorRole role,
                                  Color color);

/** Fetches a color from the palette currently being edited. */
bool theme_manager_edit_get_color(const struct HrThemeManager *manager,
                                  HrThemeColorRole role,
                                  Color *out_color);

/** Returns the palette currently being edited, or NULL when not in edit mode. */
const HrThemePalette *theme_manager_edit_palette(const struct HrThemeManager *manager);

/** Returns true when the in-progress edit differs from the original palette. */
bool theme_manager_edit_dirty(const struct HrThemeManager *manager);

/** Utility returning the canonical name for a color role (never NULL). */
const char *theme_color_role_name(HrThemeColorRole role);

/** Returns the color associated with @p role in the supplied palette. */
Color theme_palette_color(const HrThemePalette *palette, HrThemeColorRole role);

/** Copies the palette's raygui style table into @p out_values (clamped to @p count entries). */
size_t theme_palette_style_table(const HrThemePalette *palette,
                                 unsigned int *out_values,
                                 size_t count);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_THEME_H */
