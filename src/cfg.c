#include "cfg.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

struct ConfigHandle {
    HrConfig config;
    bool dirty;
};

struct ConfigParseState {
    bool data_dir_set;
    bool config_dir_set;
    bool cache_dir_set;
    bool db_path_set;
    bool backup_dir_set;
    bool window_state_path_set;
    bool autosave_dir_set;
    bool settings_path_set;
};

static int ascii_casecmp(const char *lhs, const char *rhs)
{
    if (lhs == NULL && rhs == NULL) {
        return 0;
    }
    if (lhs == NULL) {
        return -1;
    }
    if (rhs == NULL) {
        return 1;
    }

    while (*lhs != '\0' && *rhs != '\0') {
        unsigned char lc = (unsigned char)tolower((unsigned char)*lhs);
        unsigned char rc = (unsigned char)tolower((unsigned char)*rhs);
        if (lc != rc) {
            return (int)lc - (int)rc;
        }
        lhs++;
        rhs++;
    }
    return (int)(unsigned char)tolower((unsigned char)*lhs) -
           (int)(unsigned char)tolower((unsigned char)*rhs);
}

static void copy_path(char *dest, size_t dest_size, const char *src)
{
    if (dest_size == 0) {
        return;
    }

    if (src == NULL) {
        dest[0] = '\0';
        return;
    }

    strncpy(dest, src, dest_size - 1U);
    dest[dest_size - 1U] = '\0';
}

static void copy_string(char *dest, size_t dest_size, const char *src)
{
    if (dest_size == 0) {
        return;
    }

    if (src == NULL) {
        dest[0] = '\0';
        return;
    }

    strncpy(dest, src, dest_size - 1U);
    dest[dest_size - 1U] = '\0';
}

static const char *fallback_home(void)
{
    const char *home = getenv("HYPERRECALL_HOME");
    if (home == NULL) {
        home = getenv("HOME");
    }
#ifdef _WIN32
    if (home == NULL) {
        home = getenv("USERPROFILE");
    }
#endif
    return home != NULL ? home : ".";
}

static void join_path(char *dest, size_t dest_size, const char *base, const char *leaf)
{
    if (dest_size == 0) {
        return;
    }

    if (base == NULL || base[0] == '\0') {
        copy_path(dest, dest_size, leaf);
        return;
    }

    size_t base_len = strnlen(base, dest_size - 1U);
    bool needs_slash = base_len > 0U && base_len < dest_size - 1U && base[base_len - 1U] != '/' && base[base_len - 1U] != '\\';
    if (needs_slash) {
        snprintf(dest, dest_size, "%s/%s", base, leaf != NULL ? leaf : "");
    } else {
        snprintf(dest, dest_size, "%s%s", base, leaf != NULL ? leaf : "");
    }
}

static void derive_parent_directory(const char *path, char *dest, size_t dest_size)
{
    if (dest == NULL || dest_size == 0) {
        return;
    }

    if (path == NULL) {
        dest[0] = '\0';
        return;
    }

    const char *last_slash = strrchr(path, '/');
#ifdef _WIN32
    const char *last_backslash = strrchr(path, '\\');
    if (last_backslash != NULL && (last_slash == NULL || last_backslash > last_slash)) {
        last_slash = last_backslash;
    }
#endif

    if (last_slash == NULL) {
        copy_path(dest, dest_size, ".");
    } else if (last_slash == path) {
        copy_path(dest, dest_size, "/");
    } else {
        size_t length = (size_t)(last_slash - path);
        if (length >= dest_size) {
            length = dest_size - 1U;
        }
        memcpy(dest, path, length);
        dest[length] = '\0';
    }
}

static int ensure_directory(const char *path)
{
    if (path == NULL || *path == '\0') {
        return 0;
    }

    char buffer[PATH_MAX];
    copy_path(buffer, sizeof(buffer), path);

    size_t len = strlen(buffer);
    if (len == 0U) {
        return 0;
    }

    if (buffer[len - 1U] == '/' || buffer[len - 1U] == '\\') {
        buffer[len - 1U] = '\0';
    }

    for (char *ptr = buffer + 1; *ptr != '\0'; ++ptr) {
        if (*ptr == '/' || *ptr == '\\') {
            *ptr = '\0';
            if (mkdir(buffer, 0700) != 0 && errno != EEXIST) {
                int err = errno;
                return -err;
            }
            *ptr = '/';
        }
    }

    if (mkdir(buffer, 0700) != 0 && errno != EEXIST) {
        int err = errno;
        return -err;
    }

    return 0;
}

static void set_default_paths(HrConfig *config)
{
    const char *data_override = getenv("HYPERRECALL_DATA_HOME");
    const char *config_override = getenv("HYPERRECALL_CONFIG_HOME");
    const char *cache_override = getenv("HYPERRECALL_CACHE_HOME");
    const char *xdg_data = getenv("XDG_DATA_HOME");
    const char *xdg_config = getenv("XDG_CONFIG_HOME");
    const char *xdg_cache = getenv("XDG_CACHE_HOME");
    const char *home = fallback_home();

    if (data_override != NULL && data_override[0] != '\0') {
        join_path(config->paths.data_dir, sizeof(config->paths.data_dir), data_override, "");
    } else if (xdg_data != NULL && xdg_data[0] != '\0') {
        join_path(config->paths.data_dir, sizeof(config->paths.data_dir), xdg_data, "/HyperRecall");
    } else {
        snprintf(config->paths.data_dir, sizeof(config->paths.data_dir), "%s/.local/share/HyperRecall", home);
    }

    if (config_override != NULL && config_override[0] != '\0') {
        join_path(config->paths.config_dir, sizeof(config->paths.config_dir), config_override, "");
    } else if (xdg_config != NULL && xdg_config[0] != '\0') {
        join_path(config->paths.config_dir, sizeof(config->paths.config_dir), xdg_config, "/HyperRecall");
    } else {
        snprintf(config->paths.config_dir, sizeof(config->paths.config_dir), "%s/.config/HyperRecall", home);
    }

    if (cache_override != NULL && cache_override[0] != '\0') {
        join_path(config->paths.cache_dir, sizeof(config->paths.cache_dir), cache_override, "");
    } else if (xdg_cache != NULL && xdg_cache[0] != '\0') {
        join_path(config->paths.cache_dir, sizeof(config->paths.cache_dir), xdg_cache, "/HyperRecall");
    } else {
        snprintf(config->paths.cache_dir, sizeof(config->paths.cache_dir), "%s/.cache/HyperRecall", home);
    }

    join_path(config->paths.settings_path, sizeof(config->paths.settings_path), config->paths.config_dir, "/settings.cfg");
    join_path(config->database.path, sizeof(config->database.path), config->paths.data_dir, "/hyperrecall.db");
    join_path(config->database.backup_dir, sizeof(config->database.backup_dir), config->paths.data_dir, "/backups");
    join_path(config->paths.window_state_path, sizeof(config->paths.window_state_path), config->paths.config_dir,
              "/window_state.json");
    join_path(config->paths.autosave_dir, sizeof(config->paths.autosave_dir), config->paths.data_dir, "/autosave");
}

static void set_default_values(HrConfig *config)
{
    memset(config, 0, sizeof(*config));
    set_default_paths(config);

    config->ui.scale_percent = 100U;
    config->ui.font_size_pt = 14U;
    copy_string(config->ui.theme_palette, sizeof(config->ui.theme_palette), "default");
    config->analytics.enabled = true;
    config->srs.daily_new_cards = 20U;
    config->srs.daily_review_limit = 200U;

    config->study.exam_date[0] = '\0';
    config->study.saved_filters[0] = '\0';

    config->workspace.autosave_minutes = 5U;

    config->database.backup.enable_auto = true;
    config->database.backup.keep_days = 30U;
    config->database.backup.max_files = 10U;
}

static void finalize_paths(HrConfig *config)
{
    if (config->paths.data_dir[0] == '\0') {
        const char *home = fallback_home();
        snprintf(config->paths.data_dir, sizeof(config->paths.data_dir), "%s/.local/share/HyperRecall", home);
    }

    if (config->paths.config_dir[0] == '\0') {
        const char *home = fallback_home();
        snprintf(config->paths.config_dir, sizeof(config->paths.config_dir), "%s/.config/HyperRecall", home);
    }

    if (config->paths.cache_dir[0] == '\0') {
        const char *home = fallback_home();
        snprintf(config->paths.cache_dir, sizeof(config->paths.cache_dir), "%s/.cache/HyperRecall", home);
    }

    if (config->paths.settings_path[0] == '\0') {
        join_path(config->paths.settings_path, sizeof(config->paths.settings_path), config->paths.config_dir,
                  "/settings.cfg");
    }

    if (config->database.path[0] == '\0') {
        join_path(config->database.path, sizeof(config->database.path), config->paths.data_dir, "/hyperrecall.db");
    }

    if (config->database.backup_dir[0] == '\0') {
        join_path(config->database.backup_dir, sizeof(config->database.backup_dir), config->paths.data_dir, "/backups");
    }

    if (config->paths.window_state_path[0] == '\0') {
        join_path(config->paths.window_state_path, sizeof(config->paths.window_state_path), config->paths.config_dir,
                  "/window_state.json");
    }

    if (config->paths.autosave_dir[0] == '\0') {
        join_path(config->paths.autosave_dir, sizeof(config->paths.autosave_dir), config->paths.data_dir, "/autosave");
    }
}

static void apply_environment_overrides(HrConfig *config, const char *explicit_path)
{
    const char *config_file = explicit_path;
    if (config_file == NULL || config_file[0] == '\0') {
        config_file = getenv("HYPERRECALL_CONFIG_FILE");
    }

    if (config_file != NULL && config_file[0] != '\0') {
        copy_path(config->paths.settings_path, sizeof(config->paths.settings_path), config_file);
        derive_parent_directory(config->paths.settings_path, config->paths.config_dir, sizeof(config->paths.config_dir));
    }

    const char *db_path = getenv("HYPERRECALL_DB_PATH");
    if (db_path != NULL && db_path[0] != '\0') {
        copy_path(config->database.path, sizeof(config->database.path), db_path);
    }

    const char *backup_dir = getenv("HYPERRECALL_BACKUP_DIR");
    if (backup_dir != NULL && backup_dir[0] != '\0') {
        copy_path(config->database.backup_dir, sizeof(config->database.backup_dir), backup_dir);
    }

    const char *auto_backup = getenv("HYPERRECALL_AUTO_BACKUP");
    if (auto_backup != NULL) {
        config->database.backup.enable_auto = (auto_backup[0] == '1' || auto_backup[0] == 't' || auto_backup[0] == 'T' ||
                                               auto_backup[0] == 'y' || auto_backup[0] == 'Y');
    }

    const char *keep_days = getenv("HYPERRECALL_BACKUP_KEEP_DAYS");
    if (keep_days != NULL && keep_days[0] != '\0') {
        config->database.backup.keep_days = (unsigned int)strtoul(keep_days, NULL, 10);
    }

    const char *max_files = getenv("HYPERRECALL_BACKUP_MAX_FILES");
    if (max_files != NULL && max_files[0] != '\0') {
        config->database.backup.max_files = (unsigned int)strtoul(max_files, NULL, 10);
    }

    const char *theme_palette = getenv("HYPERRECALL_THEME");
    if (theme_palette != NULL && theme_palette[0] != '\0') {
        copy_string(config->ui.theme_palette, sizeof(config->ui.theme_palette), theme_palette);
    }

    const char *font_size = getenv("HYPERRECALL_FONT_SIZE");
    if (font_size != NULL && font_size[0] != '\0') {
        config->ui.font_size_pt = (unsigned int)strtoul(font_size, NULL, 10);
    }

    const char *exam_date = getenv("HYPERRECALL_EXAM_DATE");
    if (exam_date != NULL && exam_date[0] != '\0') {
        copy_string(config->study.exam_date, sizeof(config->study.exam_date), exam_date);
    }

    const char *saved_filters = getenv("HYPERRECALL_SAVED_FILTERS");
    if (saved_filters != NULL && saved_filters[0] != '\0') {
        copy_string(config->study.saved_filters, sizeof(config->study.saved_filters), saved_filters);
    }

    const char *autosave_minutes = getenv("HYPERRECALL_AUTOSAVE_MINUTES");
    if (autosave_minutes != NULL && autosave_minutes[0] != '\0') {
        config->workspace.autosave_minutes = (unsigned int)strtoul(autosave_minutes, NULL, 10);
    }

    const char *window_state = getenv("HYPERRECALL_WINDOW_GEOMETRY");
    if (window_state != NULL && window_state[0] != '\0') {
        copy_path(config->paths.window_state_path, sizeof(config->paths.window_state_path), window_state);
    }

    const char *autosave_dir = getenv("HYPERRECALL_AUTOSAVE_DIR");
    if (autosave_dir != NULL && autosave_dir[0] != '\0') {
        copy_path(config->paths.autosave_dir, sizeof(config->paths.autosave_dir), autosave_dir);
    }
}

static void trim(char *value)
{
    if (value == NULL) {
        return;
    }

    size_t len = strlen(value);
    while (len > 0U && isspace((unsigned char)value[len - 1U])) {
        value[--len] = '\0';
    }

    size_t start = 0U;
    while (value[start] != '\0' && isspace((unsigned char)value[start])) {
        start++;
    }

    if (start > 0U) {
        memmove(value, value + start, len - start + 1U);
    }
}

static void parse_bool(bool *target, const char *value)
{
    if (target == NULL || value == NULL) {
        return;
    }

    if (ascii_casecmp(value, "1") == 0 || ascii_casecmp(value, "true") == 0 || ascii_casecmp(value, "yes") == 0 ||
        ascii_casecmp(value, "on") == 0) {
        *target = true;
    } else if (ascii_casecmp(value, "0") == 0 || ascii_casecmp(value, "false") == 0 || ascii_casecmp(value, "no") == 0 ||
               ascii_casecmp(value, "off") == 0) {
        *target = false;
    }
}

static void parse_unsigned(unsigned int *target, const char *value)
{
    if (target == NULL || value == NULL) {
        return;
    }
    char *endptr = NULL;
    unsigned long parsed = strtoul(value, &endptr, 10);
    if (endptr != value) {
        *target = (unsigned int)parsed;
    }
}

static void parse_config_line(HrConfig *config, char *line, struct ConfigParseState *state)
{
    char *equals = strchr(line, '=');
    if (equals == NULL) {
        return;
    }

    *equals = '\0';
    char *key = line;
    char *value = equals + 1;
    trim(key);
    trim(value);

    if (ascii_casecmp(key, "analytics_enabled") == 0) {
        parse_bool(&config->analytics.enabled, value);
    } else if (ascii_casecmp(key, "ui_scale_percent") == 0) {
        parse_unsigned(&config->ui.scale_percent, value);
    } else if (ascii_casecmp(key, "ui_font_size_pt") == 0) {
        parse_unsigned(&config->ui.font_size_pt, value);
    } else if (ascii_casecmp(key, "ui_theme_palette") == 0) {
        copy_string(config->ui.theme_palette, sizeof(config->ui.theme_palette), value);
    } else if (ascii_casecmp(key, "srs_daily_new_cards") == 0) {
        parse_unsigned(&config->srs.daily_new_cards, value);
    } else if (ascii_casecmp(key, "srs_daily_review_limit") == 0) {
        parse_unsigned(&config->srs.daily_review_limit, value);
    } else if (ascii_casecmp(key, "db_auto_backup") == 0) {
        parse_bool(&config->database.backup.enable_auto, value);
    } else if (ascii_casecmp(key, "db_backup_keep_days") == 0) {
        parse_unsigned(&config->database.backup.keep_days, value);
    } else if (ascii_casecmp(key, "db_backup_max_files") == 0) {
        parse_unsigned(&config->database.backup.max_files, value);
    } else if (ascii_casecmp(key, "db_path") == 0) {
        copy_path(config->database.path, sizeof(config->database.path), value);
        if (state != NULL) {
            state->db_path_set = true;
        }
    } else if (ascii_casecmp(key, "db_backup_dir") == 0) {
        copy_path(config->database.backup_dir, sizeof(config->database.backup_dir), value);
        if (state != NULL) {
            state->backup_dir_set = true;
        }
    } else if (ascii_casecmp(key, "data_dir") == 0) {
        copy_path(config->paths.data_dir, sizeof(config->paths.data_dir), value);
        if (state != NULL) {
            state->data_dir_set = true;
        }
    } else if (ascii_casecmp(key, "config_dir") == 0) {
        copy_path(config->paths.config_dir, sizeof(config->paths.config_dir), value);
        if (state != NULL) {
            state->config_dir_set = true;
        }
    } else if (ascii_casecmp(key, "cache_dir") == 0) {
        copy_path(config->paths.cache_dir, sizeof(config->paths.cache_dir), value);
        if (state != NULL) {
            state->cache_dir_set = true;
        }
    } else if (ascii_casecmp(key, "window_state_path") == 0) {
        copy_path(config->paths.window_state_path, sizeof(config->paths.window_state_path), value);
        if (state != NULL) {
            state->window_state_path_set = true;
        }
    } else if (ascii_casecmp(key, "autosave_dir") == 0) {
        copy_path(config->paths.autosave_dir, sizeof(config->paths.autosave_dir), value);
        if (state != NULL) {
            state->autosave_dir_set = true;
        }
    } else if (ascii_casecmp(key, "study_exam_date") == 0) {
        copy_string(config->study.exam_date, sizeof(config->study.exam_date), value);
    } else if (ascii_casecmp(key, "study_saved_filters") == 0) {
        copy_string(config->study.saved_filters, sizeof(config->study.saved_filters), value);
    } else if (ascii_casecmp(key, "workspace_autosave_minutes") == 0) {
        parse_unsigned(&config->workspace.autosave_minutes, value);
    } else if (ascii_casecmp(key, "settings_path") == 0) {
        copy_path(config->paths.settings_path, sizeof(config->paths.settings_path), value);
        if (state != NULL) {
            state->settings_path_set = true;
        }
    }
}

static void harmonize_derived_paths(HrConfig *config, const struct ConfigParseState *state)
{
    if (state == NULL) {
        return;
    }

    if (state->data_dir_set) {
        if (!state->db_path_set) {
            config->database.path[0] = '\0';
        }
        if (!state->backup_dir_set) {
            config->database.backup_dir[0] = '\0';
        }
        if (!state->autosave_dir_set) {
            config->paths.autosave_dir[0] = '\0';
        }
    }

    if (state->config_dir_set) {
        if (!state->settings_path_set) {
            config->paths.settings_path[0] = '\0';
        }
        if (!state->window_state_path_set) {
            config->paths.window_state_path[0] = '\0';
        }
    }
}

static int ensure_file_parent(const char *file_path)
{
    if (file_path == NULL || file_path[0] == '\0') {
        return 0;
    }

    char parent[PATH_MAX];
    derive_parent_directory(file_path, parent, sizeof(parent));
    return ensure_directory(parent);
}

static int ensure_file_exists(const char *file_path)
{
    if (file_path == NULL || file_path[0] == '\0') {
        return 0;
    }

    FILE *file = fopen(file_path, "ab+");
    if (file == NULL) {
        return -errno;
    }

    if (fclose(file) != 0) {
        return -errno;
    }

    return 0;
}

static int ensure_filesystem_layout(const HrConfig *config)
{
    int rc = ensure_directory(config->paths.data_dir);
    if (rc != 0) {
        return rc;
    }

    rc = ensure_directory(config->paths.config_dir);
    if (rc != 0) {
        return rc;
    }

    rc = ensure_directory(config->paths.cache_dir);
    if (rc != 0) {
        return rc;
    }

    rc = ensure_directory(config->database.backup_dir);
    if (rc != 0) {
        return rc;
    }

    rc = ensure_directory(config->paths.autosave_dir);
    if (rc != 0) {
        return rc;
    }

    rc = ensure_file_parent(config->database.path);
    if (rc != 0) {
        return rc;
    }

    rc = ensure_file_parent(config->paths.settings_path);
    if (rc != 0) {
        return rc;
    }

    rc = ensure_file_parent(config->paths.window_state_path);
    if (rc != 0) {
        return rc;
    }

    rc = ensure_file_exists(config->paths.window_state_path);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

static int load_from_disk(HrConfig *config, const char *settings_path, struct ConfigParseState *state)
{
    if (settings_path == NULL || settings_path[0] == '\0') {
        return -1;
    }

    FILE *file = fopen(settings_path, "r");
    if (file == NULL) {
        return -errno;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file) != NULL) {
        trim(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        parse_config_line(config, line, state);
    }

    fclose(file);
    return 0;
}

static int write_to_disk(const HrConfig *config, const char *settings_path)
{
    if (settings_path == NULL || settings_path[0] == '\0') {
        return -EINVAL;
    }

    int dir_rc = ensure_filesystem_layout(config);
    if (dir_rc != 0) {
        return dir_rc;
    }

    FILE *file = fopen(settings_path, "w");
    if (file == NULL) {
        return -errno;
    }

    fprintf(file, "# HyperRecall configuration\n");
    fprintf(file, "# Generated on %ld\n\n", (long)time(NULL));

    fprintf(file, "analytics_enabled=%s\n", config->analytics.enabled ? "true" : "false");
    fprintf(file, "ui_scale_percent=%u\n", config->ui.scale_percent);
    fprintf(file, "ui_font_size_pt=%u\n", config->ui.font_size_pt);
    fprintf(file, "ui_theme_palette=%s\n", config->ui.theme_palette);
    fprintf(file, "srs_daily_new_cards=%u\n", config->srs.daily_new_cards);
    fprintf(file, "srs_daily_review_limit=%u\n", config->srs.daily_review_limit);
    fprintf(file, "db_auto_backup=%s\n", config->database.backup.enable_auto ? "true" : "false");
    fprintf(file, "db_backup_keep_days=%u\n", config->database.backup.keep_days);
    fprintf(file, "db_backup_max_files=%u\n", config->database.backup.max_files);
    fprintf(file, "db_path=%s\n", config->database.path);
    fprintf(file, "db_backup_dir=%s\n", config->database.backup_dir);
    fprintf(file, "data_dir=%s\n", config->paths.data_dir);
    fprintf(file, "config_dir=%s\n", config->paths.config_dir);
    fprintf(file, "cache_dir=%s\n", config->paths.cache_dir);
    fprintf(file, "window_state_path=%s\n", config->paths.window_state_path);
    fprintf(file, "autosave_dir=%s\n", config->paths.autosave_dir);
    fprintf(file, "study_exam_date=%s\n", config->study.exam_date);
    fprintf(file, "study_saved_filters=%s\n", config->study.saved_filters);
    fprintf(file, "workspace_autosave_minutes=%u\n", config->workspace.autosave_minutes);

    if (fclose(file) != 0) {
        return -errno;
    }

    return 0;
}

struct ConfigHandle *cfg_load(const char *explicit_path)
{
    struct ConfigHandle *handle = calloc(1U, sizeof(*handle));
    if (handle == NULL) {
        return NULL;
    }

    set_default_values(&handle->config);
    apply_environment_overrides(&handle->config, explicit_path);
    finalize_paths(&handle->config);

    struct ConfigParseState parse_state = {0};
    int rc = load_from_disk(&handle->config, handle->config.paths.settings_path, &parse_state);
    if (rc == -ENOENT) {
        /* Persist defaults to help users discover the configuration file. */
        rc = ensure_filesystem_layout(&handle->config);
        if (rc == 0) {
            rc = write_to_disk(&handle->config, handle->config.paths.settings_path);
        }
        if (rc == 0) {
            rc = ensure_file_exists(handle->config.paths.window_state_path);
        }
    }

    if (rc == 0) {
        harmonize_derived_paths(&handle->config, &parse_state);
        finalize_paths(&handle->config);
        apply_environment_overrides(&handle->config, explicit_path);
        finalize_paths(&handle->config);
        rc = ensure_filesystem_layout(&handle->config);
        if (rc != 0) {
            free(handle);
            return NULL;
        }
        handle->dirty = false;
        return handle;
    }

    free(handle);
    return NULL;
}

int cfg_reload(struct ConfigHandle *handle)
{
    if (handle == NULL) {
        return -EINVAL;
    }

    HrConfig fresh;
    set_default_values(&fresh);
    apply_environment_overrides(&fresh, handle->config.paths.settings_path);
    finalize_paths(&fresh);

    struct ConfigParseState parse_state = {0};
    int rc = load_from_disk(&fresh, fresh.paths.settings_path, &parse_state);
    if (rc != 0 && rc != -ENOENT) {
        return rc;
    }

    if (rc == -ENOENT) {
        rc = ensure_filesystem_layout(&fresh);
        if (rc == 0) {
            rc = write_to_disk(&fresh, fresh.paths.settings_path);
        }
        if (rc == 0) {
            rc = ensure_file_exists(fresh.paths.window_state_path);
        }
    }

    if (rc == 0) {
        harmonize_derived_paths(&fresh, &parse_state);
        finalize_paths(&fresh);
        apply_environment_overrides(&fresh, handle->config.paths.settings_path);
        finalize_paths(&fresh);
        rc = ensure_filesystem_layout(&fresh);
        if (rc == 0) {
            handle->config = fresh;
            handle->dirty = false;
        }
    }

    return rc;
}

int cfg_save(const struct ConfigHandle *handle)
{
    if (handle == NULL) {
        return -EINVAL;
    }

    int rc = ensure_filesystem_layout(&handle->config);
    if (rc != 0) {
        return rc;
    }

    rc = write_to_disk(&handle->config, handle->config.paths.settings_path);
    if (rc == 0) {
        ((struct ConfigHandle *)handle)->dirty = false;
    }
    return rc;
}

void cfg_unload(struct ConfigHandle *handle)
{
    free(handle);
}

const HrConfig *cfg_data(const struct ConfigHandle *handle)
{
    return handle != NULL ? &handle->config : NULL;
}

HrConfig *cfg_data_mutable(struct ConfigHandle *handle)
{
    if (handle == NULL) {
        return NULL;
    }
    handle->dirty = true;
    return &handle->config;
}

void cfg_mark_dirty(struct ConfigHandle *handle)
{
    if (handle != NULL) {
        handle->dirty = true;
    }
}

bool cfg_is_dirty(const struct ConfigHandle *handle)
{
    return handle != NULL && handle->dirty;
}

const char *cfg_settings_path(const struct ConfigHandle *handle)
{
    return handle != NULL ? handle->config.paths.settings_path : NULL;
}

const char *cfg_database_path(const struct ConfigHandle *handle)
{
    return handle != NULL ? handle->config.database.path : NULL;
}

const char *cfg_database_backup_dir(const struct ConfigHandle *handle)
{
    return handle != NULL ? handle->config.database.backup_dir : NULL;
}

const HrBackupPolicy *cfg_database_backup_policy(const struct ConfigHandle *handle)
{
    return handle != NULL ? &handle->config.database.backup : NULL;
}
