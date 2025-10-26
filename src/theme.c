#include "theme.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#define strcasecmp _stricmp
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Forward declarations */
struct HrThemeManager {
    HrThemePalette *palettes;
    size_t palette_count;
    size_t palette_capacity;
    size_t active_index;

    char preferences_path[PATH_MAX];
    char user_directory[PATH_MAX];

    HrThemeChangedCallback changed_callback;
    void *changed_user_data;

    HrThemeAnalyticsCallback analytics_callback;
    void *analytics_user_data;

    struct {
        bool active;
        size_t palette_index;
        HrThemePalette original;
        HrThemePalette working;
        bool dirty;
    } editor;
};

typedef struct HrThemeColorDescriptor {
    HrThemeColorRole role;
    const char *name;
    Color fallback;
} HrThemeColorDescriptor;

static const HrThemeColorDescriptor kThemeColorDescriptors[] = {
    {HR_THEME_COLOR_BACKGROUND, "background", {12, 14, 26, 255}},
    {HR_THEME_COLOR_BACKGROUND_ALT, "backgroundAlt", {18, 21, 36, 255}},
    {HR_THEME_COLOR_SURFACE, "surface", {24, 27, 44, 255}},
    {HR_THEME_COLOR_SURFACE_ALT, "surfaceAlt", {38, 42, 65, 255}},
    {HR_THEME_COLOR_TEXT, "text", {240, 244, 255, 255}},
    {HR_THEME_COLOR_TEXT_MUTED, "textMuted", {160, 168, 194, 255}},
    {HR_THEME_COLOR_ACCENT, "accent", {0, 220, 220, 255}},
    {HR_THEME_COLOR_ACCENT_ALT, "accentAlt", {255, 99, 247, 255}},
    {HR_THEME_COLOR_BORDER, "border", {62, 69, 98, 255}},
    {HR_THEME_COLOR_SUCCESS, "success", {0, 200, 120, 255}},
    {HR_THEME_COLOR_WARNING, "warning", {255, 183, 0, 255}},
    {HR_THEME_COLOR_DANGER, "danger", {255, 82, 82, 255}},
    {HR_THEME_COLOR_INFO, "info", {0, 168, 255, 255}},
    {HR_THEME_COLOR_CLOZE_GAP, "clozeGap", {0, 220, 220, 64}},
    {HR_THEME_COLOR_CLOZE_TEXT, "clozeText", {240, 244, 255, 255}},
    {HR_THEME_COLOR_CODE_BACKGROUND, "codeBackground", {18, 21, 36, 255}},
    {HR_THEME_COLOR_CODE_TEXT, "codeText", {220, 223, 238, 255}},
    {HR_THEME_COLOR_ANALYTICS_PRIMARY, "analyticsPrimary", {0, 220, 220, 255}},
    {HR_THEME_COLOR_ANALYTICS_SECONDARY, "analyticsSecondary", {255, 99, 247, 255}},
    {HR_THEME_COLOR_TOAST_BACKGROUND, "toastBackground", {24, 27, 44, 240}},
    {HR_THEME_COLOR_TOAST_TEXT, "toastText", {240, 244, 255, 255}},
};

static inline unsigned int theme_color_to_uint(Color color)
{
    return ((unsigned int)color.a << 24U) |
           ((unsigned int)color.r << 16U) |
           ((unsigned int)color.g << 8U) |
           ((unsigned int)color.b);
}

static inline bool is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

static const char *skip_whitespace(const char *cursor, const char *end)
{
    while (cursor < end && is_whitespace(*cursor)) {
        cursor++;
    }
    return cursor;
}

static const char *find_matching_brace(const char *start, const char *end)
{
    int depth = 0;
    const char *cursor = start;
    while (cursor < end) {
        char ch = *cursor;
        if (ch == '{') {
            depth++;
        } else if (ch == '}') {
            depth--;
            if (depth == 0) {
                return cursor;
            }
        } else if (ch == '"') {
            cursor++;
            while (cursor < end) {
                if (*cursor == '\\') {
                    cursor += 2;
                    continue;
                }
                if (*cursor == '"') {
                    break;
                }
                cursor++;
            }
        }
        cursor++;
    }
    return NULL;
}

static bool json_extract_string(const char *start,
                                const char *end,
                                const char *key,
                                char *buffer,
                                size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0U) {
        return false;
    }

    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char *cursor = start;
    size_t pattern_len = strlen(pattern);

    while (cursor < end) {
        const char *found = strstr(cursor, pattern);
        if (found == NULL || found >= end) {
            break;
        }
        cursor = found + pattern_len;
        cursor = skip_whitespace(cursor, end);
        if (cursor >= end || *cursor != ':') {
            continue;
        }
        cursor++;
        cursor = skip_whitespace(cursor, end);
        if (cursor >= end || *cursor != '"') {
            continue;
        }
        cursor++;
        char *out = buffer;
        size_t remaining = buffer_size - 1U;
        while (cursor < end) {
            char ch = *cursor;
            if (ch == '\\' && (cursor + 1) < end) {
                cursor++;
                ch = *cursor;
            } else if (ch == '"') {
                break;
            }
            if (remaining > 0U) {
                *out++ = ch;
                remaining--;
            }
            cursor++;
        }
        *out = '\0';
        return true;
    }

    return false;
}

static bool json_extract_bool(const char *start,
                              const char *end,
                              const char *key,
                              bool *out_value)
{
    if (out_value == NULL) {
        return false;
    }

    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *cursor = start;
    size_t pattern_len = strlen(pattern);

    while (cursor < end) {
        const char *found = strstr(cursor, pattern);
        if (found == NULL || found >= end) {
            break;
        }
        cursor = found + pattern_len;
        cursor = skip_whitespace(cursor, end);
        if (cursor >= end || *cursor != ':') {
            continue;
        }
        cursor++;
        cursor = skip_whitespace(cursor, end);
        if (cursor + 4 <= end && strncmp(cursor, "true", 4) == 0) {
            *out_value = true;
            return true;
        }
        if (cursor + 5 <= end && strncmp(cursor, "false", 5) == 0) {
            *out_value = false;
            return true;
        }
    }

    return false;
}

static bool json_extract_object_bounds(const char *start,
                                       const char *end,
                                       const char *key,
                                       const char **out_begin,
                                       const char **out_end)
{
    if (out_begin == NULL || out_end == NULL) {
        return false;
    }

    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *cursor = start;
    size_t pattern_len = strlen(pattern);

    while (cursor < end) {
        const char *found = strstr(cursor, pattern);
        if (found == NULL || found >= end) {
            break;
        }
        cursor = found + pattern_len;
        cursor = skip_whitespace(cursor, end);
        if (cursor >= end || *cursor != ':') {
            continue;
        }
        cursor++;
        cursor = skip_whitespace(cursor, end);
        if (cursor >= end || *cursor != '{') {
            continue;
        }
        const char *object_start = cursor;
        const char *object_end = find_matching_brace(cursor, end);
        if (object_end == NULL) {
            break;
        }
        *out_begin = object_start + 1; /* Skip leading '{' */
        *out_end = object_end;
        return true;
    }

    return false;
}

static Color parse_color_hex(const char *value)
{
    if (value == NULL) {
        Color color = {0, 0, 0, 255};
        return color;
    }

    while (*value != '\0' && *value != '#') {
        value++;
    }
    if (*value == '#') {
        value++;
    }

    unsigned int rgba = 0U;
    size_t length = strlen(value);
    if (length >= 8U) {
        sscanf(value, "%08x", &rgba);
        Color color = {
            (unsigned char)((rgba >> 24U) & 0xFFU),
            (unsigned char)((rgba >> 16U) & 0xFFU),
            (unsigned char)((rgba >> 8U) & 0xFFU),
            (unsigned char)(rgba & 0xFFU),
        };
        return color;
    }

    unsigned int rgb = 0U;
    if (sscanf(value, "%06x", &rgb) == 1) {
        Color color = {
            (unsigned char)((rgb >> 16U) & 0xFFU),
            (unsigned char)((rgb >> 8U) & 0xFFU),
            (unsigned char)(rgb & 0xFFU),
            255,
        };
        return color;
    }

    Color fallback = {0, 0, 0, 255};
    return fallback;
}

static void theme_palette_fill_defaults(HrThemePalette *palette)
{
    if (palette == NULL) {
        return;
    }
    memset(palette, 0, sizeof(*palette));
    snprintf(palette->id, sizeof(palette->id), "%s", "neon-dark");
    snprintf(palette->name, sizeof(palette->name), "%s", "Neon Dark");
    snprintf(palette->description, sizeof(palette->description), "%s",
             "HyperRecall neon-dark default palette");

    for (size_t i = 0; i < ARRAY_SIZE(kThemeColorDescriptors); ++i) {
        HrThemeColorDescriptor descriptor = kThemeColorDescriptors[i];
        if (descriptor.role >= 0 && (size_t)descriptor.role < HR_THEME_COLOR_COUNT) {
            palette->colors[descriptor.role] = descriptor.fallback;
        }
    }
    palette->user_defined = false;
}

static void theme_palette_compute_style(HrThemePalette *palette)
{
    if (palette == NULL) {
        return;
    }

    memset(palette->style, 0, sizeof(palette->style));

    palette->style[0] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_BACKGROUND));
    palette->style[1] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_SURFACE));
    palette->style[2] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_ACCENT));
    palette->style[3] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_TEXT));
    palette->style[4] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_TEXT_MUTED));
    palette->style[5] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_BORDER));
    palette->style[6] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_SURFACE_ALT));
    palette->style[7] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_ACCENT_ALT));
    palette->style[8] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_SUCCESS));
    palette->style[9] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_WARNING));
    palette->style[10] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_DANGER));
    palette->style[11] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_INFO));
    palette->style[12] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_CLOZE_GAP));
    palette->style[13] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_CODE_BACKGROUND));
    palette->style[14] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_CODE_TEXT));
    palette->style[15] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_TOAST_BACKGROUND));
    palette->style[16] = theme_color_to_uint(theme_palette_color(palette, HR_THEME_COLOR_TOAST_TEXT));
}

static bool theme_manager_reserve(struct HrThemeManager *manager, size_t capacity)
{
    if (manager->palette_capacity >= capacity) {
        return true;
    }

    size_t new_capacity = manager->palette_capacity == 0 ? 4 : manager->palette_capacity;
    while (new_capacity < capacity) {
        new_capacity *= 2;
    }

    HrThemePalette *resized = (HrThemePalette *)realloc(manager->palettes,
                                                        new_capacity * sizeof(HrThemePalette));
    if (resized == NULL) {
        return false;
    }

    manager->palettes = resized;
    manager->palette_capacity = new_capacity;
    return true;
}

static void theme_manager_on_palette_changed(struct HrThemeManager *manager)
{
    if (manager == NULL || manager->active_index >= manager->palette_count) {
        return;
    }

    const HrThemePalette *active = &manager->palettes[manager->active_index];
    if (manager->analytics_callback != NULL) {
        manager->analytics_callback(active, manager->analytics_user_data);
    }
    if (manager->changed_callback != NULL) {
        manager->changed_callback(active, manager->changed_user_data);
    }
}

static bool theme_manager_set_active_index(struct HrThemeManager *manager, size_t index)
{
    if (manager == NULL || index >= manager->palette_count) {
        return false;
    }

    if (manager->active_index == index) {
        return true;
    }

    manager->active_index = index;
    theme_manager_on_palette_changed(manager);
    return theme_manager_write_preferences(manager);
}

static bool ensure_directory_exists(const char *path)
{
    if (path == NULL || *path == '\0') {
        return false;
    }

#if defined(_WIN32)
    int result = _mkdir(path);
    if (result == 0 || errno == EEXIST) {
        return true;
    }
    return false;
#else
    char buffer[PATH_MAX];
    snprintf(buffer, sizeof(buffer), "%s", path);
    size_t length = strlen(buffer);
    if (length == 0U) {
        return false;
    }
    if (buffer[length - 1U] == '/') {
        buffer[length - 1U] = '\0';
    }

    char *cursor = buffer;
    if (*cursor == '/') {
        cursor++;
    }
    while (*cursor != '\0') {
        if (*cursor == '/') {
            *cursor = '\0';
            if (mkdir(buffer, 0755) != 0 && errno != EEXIST) {
                return false;
            }
            *cursor = '/';
        }
        cursor++;
    }
    if (mkdir(buffer, 0755) != 0 && errno != EEXIST) {
        return false;
    }
    return true;
#endif
}

static void sanitise_identifier(const char *source, char *destination, size_t capacity)
{
    if (destination == NULL || capacity == 0U) {
        return;
    }
    destination[0] = '\0';
    if (source == NULL) {
        return;
    }
    size_t dest_index = 0;
    for (const char *cursor = source; *cursor != '\0' && dest_index + 1U < capacity; ++cursor) {
        unsigned char ch = (unsigned char)*cursor;
        if (isalnum(ch)) {
            destination[dest_index++] = (char)tolower(ch);
        } else if (ch == ' ' || ch == '-' || ch == '_') {
            if (dest_index > 0 && destination[dest_index - 1] != '-') {
                destination[dest_index++] = '-';
            }
        }
    }
    if (dest_index == 0U) {
        snprintf(destination, capacity, "%s", "theme");
    } else {
        destination[dest_index] = '\0';
    }
}

static bool parse_theme_object(const char *object_start,
                               const char *object_end,
                               HrThemePalette *out_palette)
{
    if (object_start == NULL || object_end == NULL || out_palette == NULL) {
        return false;
    }

    HrThemePalette palette;
    theme_palette_fill_defaults(&palette);

    char name_buffer[HR_THEME_MAX_NAME_LENGTH];
    if (json_extract_string(object_start, object_end, "name", name_buffer, sizeof(name_buffer))) {
        snprintf(palette.name, sizeof(palette.name), "%s", name_buffer);
        sanitise_identifier(name_buffer, palette.id, sizeof(palette.id));
    }

    char id_buffer[HR_THEME_MAX_ID_LENGTH];
    if (json_extract_string(object_start, object_end, "id", id_buffer, sizeof(id_buffer))) {
        snprintf(palette.id, sizeof(palette.id), "%s", id_buffer);
    }

    char description_buffer[HR_THEME_MAX_DESCRIPTION_LENGTH];
    if (json_extract_string(object_start, object_end, "description", description_buffer,
                            sizeof(description_buffer))) {
        snprintf(palette.description, sizeof(palette.description), "%s", description_buffer);
    }

    bool user_defined = false;
    if (json_extract_bool(object_start, object_end, "user", &user_defined) ||
        json_extract_bool(object_start, object_end, "userDefined", &user_defined)) {
        palette.user_defined = user_defined;
    }

    const char *colors_start = NULL;
    const char *colors_end = NULL;
    if (json_extract_object_bounds(object_start, object_end, "colors", &colors_start, &colors_end)) {
        for (size_t i = 0; i < ARRAY_SIZE(kThemeColorDescriptors); ++i) {
            HrThemeColorDescriptor descriptor = kThemeColorDescriptors[i];
            char color_buffer[64];
            if (json_extract_string(colors_start, colors_end, descriptor.name, color_buffer,
                                    sizeof(color_buffer))) {
                palette.colors[descriptor.role] = parse_color_hex(color_buffer);
            } else {
                palette.colors[descriptor.role] = descriptor.fallback;
            }
        }
    }

    theme_palette_compute_style(&palette);
    *out_palette = palette;
    return true;
}

static bool theme_manager_register_palette(struct HrThemeManager *manager,
                                           const HrThemePalette *palette)
{
    if (manager == NULL || palette == NULL) {
        return false;
    }

    if (!theme_manager_reserve(manager, manager->palette_count + 1U)) {
        return false;
    }

    manager->palettes[manager->palette_count++] = *palette;
    return true;
}

static size_t theme_manager_find_index(const struct HrThemeManager *manager, const char *palette_id)
{
    if (manager == NULL || palette_id == NULL) {
        return SIZE_MAX;
    }

    for (size_t i = 0; i < manager->palette_count; ++i) {
        if (strcasecmp(manager->palettes[i].id, palette_id) == 0) {
            return i;
        }
    }

    return SIZE_MAX;
}

static bool load_theme_json(struct HrThemeManager *manager, const char *json_path)
{
    FILE *file = fopen(json_path, "rb");
    if (file == NULL) {
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }
    long length = ftell(file);
    if (length < 0) {
        fclose(file);
        return false;
    }
    rewind(file);

    char *buffer = (char *)malloc((size_t)length + 1U);
    if (buffer == NULL) {
        fclose(file);
        return false;
    }

    size_t read_count = fread(buffer, 1, (size_t)length, file);
    fclose(file);
    if (read_count != (size_t)length) {
        free(buffer);
        return false;
    }
    buffer[length] = '\0';

    const char *start = buffer;
    const char *end = buffer + length;

    const char *themes_key = strstr(start, "\"themes\"");
    if (themes_key == NULL) {
        free(buffer);
        return false;
    }

    const char *cursor = strchr(themes_key, '[');
    if (cursor == NULL) {
        free(buffer);
        return false;
    }
    cursor++;

    while (cursor < end) {
        cursor = skip_whitespace(cursor, end);
        if (cursor >= end || *cursor == ']') {
            break;
        }
        if (*cursor != '{') {
            cursor++;
            continue;
        }
        const char *object_start = cursor;
        const char *object_end = find_matching_brace(cursor, end);
        if (object_end == NULL) {
            break;
        }
        HrThemePalette palette;
        if (parse_theme_object(object_start, object_end + 1, &palette)) {
            theme_manager_register_palette(manager, &palette);
        }
        cursor = object_end + 1;
    }

    free(buffer);
    return true;
}

static void theme_manager_load_preferences(struct HrThemeManager *manager)
{
    if (manager == NULL || manager->preferences_path[0] == '\0') {
        return;
    }

    FILE *file = fopen(manager->preferences_path, "rb");
    if (file == NULL) {
        return;
    }

    char buffer[HR_THEME_MAX_ID_LENGTH];
    if (fgets(buffer, sizeof(buffer), file) != NULL) {
        size_t len = strlen(buffer);
        while (len > 0U && (buffer[len - 1U] == '\n' || buffer[len - 1U] == '\r')) {
            buffer[--len] = '\0';
        }
        size_t index = theme_manager_find_index(manager, buffer);
        if (index != SIZE_MAX) {
            manager->active_index = index;
        }
    }

    fclose(file);
}

struct HrThemeManager *theme_manager_create(void)
{
    struct HrThemeManager *manager = (struct HrThemeManager *)calloc(1U, sizeof(*manager));
    if (manager == NULL) {
        return NULL;
    }

    HrThemePalette default_palette;
    theme_palette_fill_defaults(&default_palette);
    theme_palette_compute_style(&default_palette);
    theme_manager_register_palette(manager, &default_palette);
    manager->active_index = 0U;

    return manager;
}

void theme_manager_destroy(struct HrThemeManager *manager)
{
    if (manager == NULL) {
        return;
    }

    free(manager->palettes);
    free(manager);
}

void theme_manager_set_preferences_file(struct HrThemeManager *manager, const char *path)
{
    if (manager == NULL) {
        return;
    }
    if (path != NULL) {
        snprintf(manager->preferences_path, sizeof(manager->preferences_path), "%s", path);
        theme_manager_load_preferences(manager);
        theme_manager_set_active_index(manager, manager->active_index);
    } else {
        manager->preferences_path[0] = '\0';
    }
}

void theme_manager_set_user_directory(struct HrThemeManager *manager, const char *directory)
{
    if (manager == NULL) {
        return;
    }
    if (directory != NULL) {
        snprintf(manager->user_directory, sizeof(manager->user_directory), "%s", directory);
    } else {
        manager->user_directory[0] = '\0';
    }
}

bool theme_manager_load_palettes(struct HrThemeManager *manager, const char *json_path)
{
    if (manager == NULL || json_path == NULL) {
        return false;
    }

    size_t previous_count = manager->palette_count;
    bool ok = load_theme_json(manager, json_path);
    if (ok && manager->palette_count > previous_count && manager->changed_callback != NULL) {
        manager->changed_callback(theme_manager_active(manager), manager->changed_user_data);
    }
    return ok;
}

size_t theme_manager_palette_count(const struct HrThemeManager *manager)
{
    return manager != NULL ? manager->palette_count : 0U;
}

const HrThemePalette *theme_manager_palette(const struct HrThemeManager *manager, size_t index)
{
    if (manager == NULL || index >= manager->palette_count) {
        return NULL;
    }
    return &manager->palettes[index];
}

const HrThemePalette *theme_manager_find(const struct HrThemeManager *manager, const char *palette_id)
{
    size_t index = theme_manager_find_index(manager, palette_id);
    return (index != SIZE_MAX) ? &manager->palettes[index] : NULL;
}

bool theme_manager_apply(struct HrThemeManager *manager, const char *palette_id)
{
    if (manager == NULL || palette_id == NULL) {
        return false;
    }
    size_t index = theme_manager_find_index(manager, palette_id);
    if (index == SIZE_MAX) {
        return false;
    }
    return theme_manager_set_active_index(manager, index);
}

const HrThemePalette *theme_manager_active(const struct HrThemeManager *manager)
{
    if (manager == NULL || manager->palette_count == 0U) {
        return NULL;
    }
    size_t index = manager->active_index < manager->palette_count ? manager->active_index : 0U;
    return &manager->palettes[index];
}

bool theme_manager_write_preferences(struct HrThemeManager *manager)
{
    if (manager == NULL || manager->preferences_path[0] == '\0') {
        return true;
    }
    if (manager->active_index >= manager->palette_count) {
        return false;
    }

    FILE *file = fopen(manager->preferences_path, "wb");
    if (file == NULL) {
        return false;
    }

    const HrThemePalette *palette = &manager->palettes[manager->active_index];
    fprintf(file, "%s\n", palette->id);
    fclose(file);
    return true;
}

static bool write_palette_to_file(const HrThemePalette *palette, const char *path)
{
    if (palette == NULL || path == NULL) {
        return false;
    }

    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }

    fprintf(file, "{\n");
    fprintf(file, "  \"name\": \"%s\",\n", palette->name);
    fprintf(file, "  \"id\": \"%s\",\n", palette->id);
    fprintf(file, "  \"description\": \"%s\",\n", palette->description);
    fprintf(file, "  \"user\": %s,\n", palette->user_defined ? "true" : "false");
    fprintf(file, "  \"colors\": {\n");

    for (size_t i = 0; i < ARRAY_SIZE(kThemeColorDescriptors); ++i) {
        HrThemeColorDescriptor descriptor = kThemeColorDescriptors[i];
        Color color = palette->colors[descriptor.role];
        fprintf(file,
                "    \"%s\": \"#%02X%02X%02X%02X\"%s\n",
                descriptor.name,
                color.r,
                color.g,
                color.b,
                color.a,
                (i + 1U < ARRAY_SIZE(kThemeColorDescriptors)) ? "," : "");
    }

    fprintf(file, "  }\n");
    fprintf(file, "}\n");

    fclose(file);
    return true;
}

bool theme_manager_save_palette(const struct HrThemeManager *manager,
                                const HrThemePalette *palette,
                                const char *override_path)
{
    if (manager == NULL || palette == NULL) {
        return false;
    }

    char path_buffer[PATH_MAX];
    if (override_path != NULL) {
        snprintf(path_buffer, sizeof(path_buffer), "%s", override_path);
    } else {
        if (manager->user_directory[0] == '\0') {
            return false;
        }
        if (!ensure_directory_exists(manager->user_directory)) {
            return false;
        }
        snprintf(path_buffer, sizeof(path_buffer), "%s/%s.json", manager->user_directory, palette->id);
    }

    return write_palette_to_file(palette, path_buffer);
}

void theme_manager_set_changed_callback(struct HrThemeManager *manager,
                                        HrThemeChangedCallback callback,
                                        void *user_data)
{
    if (manager == NULL) {
        return;
    }
    manager->changed_callback = callback;
    manager->changed_user_data = user_data;
    theme_manager_on_palette_changed(manager);
}

void theme_manager_set_analytics_callback(struct HrThemeManager *manager,
                                          HrThemeAnalyticsCallback callback,
                                          void *user_data)
{
    if (manager == NULL) {
        return;
    }
    manager->analytics_callback = callback;
    manager->analytics_user_data = user_data;
}

bool theme_manager_begin_edit(struct HrThemeManager *manager, const char *palette_id)
{
    if (manager == NULL || palette_id == NULL) {
        return false;
    }

    size_t index = theme_manager_find_index(manager, palette_id);
    if (index == SIZE_MAX) {
        return false;
    }

    manager->editor.active = true;
    manager->editor.palette_index = index;
    manager->editor.original = manager->palettes[index];
    manager->editor.working = manager->palettes[index];
    manager->editor.dirty = false;
    return true;
}

void theme_manager_cancel_edit(struct HrThemeManager *manager)
{
    if (manager == NULL || !manager->editor.active) {
        return;
    }
    manager->editor.active = false;
    manager->editor.dirty = false;
}

bool theme_manager_commit_edit(struct HrThemeManager *manager, bool persist_changes)
{
    if (manager == NULL || !manager->editor.active) {
        return false;
    }

    size_t index = manager->editor.palette_index;
    manager->palettes[index] = manager->editor.working;
    theme_palette_compute_style(&manager->palettes[index]);

    manager->editor.active = false;
    manager->editor.dirty = false;

    if (persist_changes && manager->palettes[index].user_defined) {
        theme_manager_save_palette(manager, &manager->palettes[index], NULL);
    }

    if (index == manager->active_index) {
        theme_manager_on_palette_changed(manager);
    }

    return true;
}

bool theme_manager_edit_set_color(struct HrThemeManager *manager,
                                  HrThemeColorRole role,
                                  Color color)
{
    if (manager == NULL || !manager->editor.active) {
        return false;
    }
    if ((size_t)role >= HR_THEME_COLOR_COUNT) {
        return false;
    }

    HrThemePalette *working = &manager->editor.working;
    if (memcmp(&working->colors[role], &color, sizeof(color)) != 0) {
        working->colors[role] = color;
        theme_palette_compute_style(working);
        manager->editor.dirty = true;
    }
    return true;
}

bool theme_manager_edit_get_color(const struct HrThemeManager *manager,
                                  HrThemeColorRole role,
                                  Color *out_color)
{
    if (manager == NULL || !manager->editor.active || out_color == NULL) {
        return false;
    }
    if ((size_t)role >= HR_THEME_COLOR_COUNT) {
        return false;
    }
    *out_color = manager->editor.working.colors[role];
    return true;
}

const HrThemePalette *theme_manager_edit_palette(const struct HrThemeManager *manager)
{
    if (manager == NULL || !manager->editor.active) {
        return NULL;
    }
    return &manager->editor.working;
}

bool theme_manager_edit_dirty(const struct HrThemeManager *manager)
{
    if (manager == NULL || !manager->editor.active) {
        return false;
    }
    return manager->editor.dirty;
}

const char *theme_color_role_name(HrThemeColorRole role)
{
    if ((size_t)role >= ARRAY_SIZE(kThemeColorDescriptors)) {
        return "unknown";
    }
    return kThemeColorDescriptors[role].name;
}

Color theme_palette_color(const HrThemePalette *palette, HrThemeColorRole role)
{
    if (palette == NULL || (size_t)role >= HR_THEME_COLOR_COUNT) {
        Color fallback = {0, 0, 0, 255};
        return fallback;
    }
    return palette->colors[role];
}

size_t theme_palette_style_table(const HrThemePalette *palette,
                                 unsigned int *out_values,
                                 size_t count)
{
    if (palette == NULL || out_values == NULL || count == 0U) {
        return 0U;
    }
    size_t to_copy = HR_THEME_STYLE_TABLE_SIZE;
    if (to_copy > count) {
        to_copy = count;
    }
    memcpy(out_values, palette->style, to_copy * sizeof(unsigned int));
    return to_copy;
}
