#include "ui.h"

#include "analytics.h"

#include <ctype.h>
#include <math.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef UI_MAX_TOASTS
#define UI_MAX_TOASTS 8
#endif

#ifndef UI_MAX_COMMANDS
#define UI_MAX_COMMANDS 8
#endif

typedef struct UiTopicNode {
    sqlite3_int64 id;
    sqlite3_int64 parent_id;
    int depth;
    char title[128];
    char summary[160];
} UiTopicNode;

typedef struct UiCardRow {
    sqlite3_int64 id;
    sqlite3_int64 topic_id;
    char prompt[192];
    char topic_title[96];
    time_t due_at;
    int ease_factor;
    int interval;
    int review_state;
    bool suspended;
} UiCardRow;

typedef struct UiToast {
    char message[192];
    Color background;
    float ttl;
} UiToast;

typedef struct UiCommand {
    const char *label;
    const char *description;
} UiCommand;

typedef struct UiReviewMetrics {
    size_t total_reviews;
    size_t rating_counts[SRS_RESPONSE_CRAM + 1];
    double average_interval_minutes;
    float recent_intervals[64];
    size_t recent_count;
} UiReviewMetrics;

struct UiContext {
    UiConfig config;
    HrRenderContext render;

    struct HrThemeManager *themes;
    struct SessionManager *sessions;
    DatabaseHandle *database;
    struct ImportExportContext *import_export;
    struct AnalyticsHandle *analytics;

    SessionCallbacks chained_callbacks;

    UiScreenId screen;

    UiTopicNode *topics;
    size_t topic_count;
    bool topics_dirty;
    size_t selected_topic;

    UiCardRow *cards;
    size_t card_count;
    bool cards_dirty;
    float card_scroll_offset;
    float card_row_height;

    UiToast toasts[UI_MAX_TOASTS];
    size_t toast_count;

    struct {
        bool visible;
        char title[64];
        char body[256];
    } modal;

    struct {
        bool open;
        char filter[64];
        size_t selected_index;
    } command_palette;

    UiReviewMetrics metrics;

    HrPlatformFrame last_frame;
    double elapsed_time;

    float sidebar_scroll;
    bool theme_editor_open;
};

static const UiCommand kUiCommands[] = {
    {"Start mastery session", "Begin a mastery-oriented queue"},
    {"Start cram session", "Launch a cram-focused study run"},
    {"Toggle analytics", "Jump to the analytics dashboard"},
    {"Toggle theme editor", "Open the live theme editor"},
    {"Import deck", "Launch the import workflow"},
    {"Export deck", "Export the current topic"},
    {"Show shortcuts", "Display keyboard shortcut help"},
    {"Clear toasts", "Dismiss all active notifications"},
};

static Color ui_palette_color(const UiContext *ui, HrThemeColorRole role, Color fallback)
{
    if (ui != NULL && ui->render.palette != NULL) {
        return theme_palette_color(ui->render.palette, role);
    }
    return fallback;
}

static void ui_push_toast_internal(UiContext *ui, const char *message, Color background, float ttl)
{
    if (ui == NULL || message == NULL) {
        return;
    }

    if (ui->toast_count == UI_MAX_TOASTS) {
        memmove(&ui->toasts[0], &ui->toasts[1], (UI_MAX_TOASTS - 1) * sizeof(UiToast));
        ui->toast_count--;
    }

    UiToast *toast = &ui->toasts[ui->toast_count++];
    snprintf(toast->message, sizeof(toast->message), "%s", message);
    toast->background = background;
    toast->ttl = ttl;
}

static int ui_compare_topic(const void *lhs, const void *rhs)
{
    const UiTopicNode *a = (const UiTopicNode *)lhs;
    const UiTopicNode *b = (const UiTopicNode *)rhs;
    if (a->parent_id == b->parent_id) {
        if (a->id == b->id) {
            return 0;
        }
        return (a->id < b->id) ? -1 : 1;
    }
    if (a->parent_id == 0) {
        return -1;
    }
    if (b->parent_id == 0) {
        return 1;
    }
    return (a->parent_id < b->parent_id) ? -1 : 1;
}

static void ui_reload_topics(UiContext *ui)
{
    if (ui == NULL || ui->database == NULL) {
        return;
    }

    sqlite3 *connection = db_connection(ui->database);
    if (connection == NULL) {
        return;
    }

    const char *sql = "SELECT id, COALESCE(parent_id, 0), title, summary FROM topics ORDER BY parent_id, position, id";

    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(connection, sql, -1, &statement, NULL) != SQLITE_OK) {
        return;
    }

    free(ui->topics);
    ui->topics = NULL;
    ui->topic_count = 0U;

    size_t capacity = 0U;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        if (ui->topic_count == capacity) {
            size_t new_capacity = capacity == 0U ? 16U : capacity * 2U;
            UiTopicNode *resized = (UiTopicNode *)realloc(ui->topics, new_capacity * sizeof(UiTopicNode));
            if (resized == NULL) {
                break;
            }
            ui->topics = resized;
            capacity = new_capacity;
        }

        UiTopicNode *node = &ui->topics[ui->topic_count++];
        node->id = sqlite3_column_int64(statement, 0);
        node->parent_id = sqlite3_column_int64(statement, 1);
        const unsigned char *title = sqlite3_column_text(statement, 2);
        const unsigned char *summary = sqlite3_column_text(statement, 3);
        snprintf(node->title, sizeof(node->title), "%s", title != NULL ? (const char *)title : "Untitled Topic");
        snprintf(node->summary, sizeof(node->summary), "%s",
                 summary != NULL ? (const char *)summary : "");
        node->depth = 0;
    }

    sqlite3_finalize(statement);

    qsort(ui->topics, ui->topic_count, sizeof(UiTopicNode), ui_compare_topic);

    for (size_t i = 0; i < ui->topic_count; ++i) {
        UiTopicNode *node = &ui->topics[i];
        if (node->parent_id == 0) {
            node->depth = 0;
            continue;
        }
        for (size_t j = 0; j < ui->topic_count; ++j) {
            if (ui->topics[j].id == node->parent_id) {
                node->depth = ui->topics[j].depth + 1;
                break;
            }
        }
    }

    ui->topics_dirty = false;
}

static void ui_reload_cards(UiContext *ui)
{
    if (ui == NULL || ui->database == NULL) {
        return;
    }

    sqlite3 *connection = db_connection(ui->database);
    if (connection == NULL) {
        return;
    }

    const char *sql =
        "SELECT cards.id, cards.topic_id, cards.prompt, cards.due_at, cards.ease_factor, cards.interval, "
        "cards.review_state, cards.suspended, topics.title "
        "FROM cards LEFT JOIN topics ON cards.topic_id = topics.id "
        "ORDER BY cards.due_at, cards.id LIMIT 512";

    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(connection, sql, -1, &statement, NULL) != SQLITE_OK) {
        return;
    }

    free(ui->cards);
    ui->cards = NULL;
    ui->card_count = 0U;

    size_t capacity = 0U;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        if (ui->card_count == capacity) {
            size_t new_capacity = capacity == 0U ? 32U : capacity * 2U;
            UiCardRow *resized = (UiCardRow *)realloc(ui->cards, new_capacity * sizeof(UiCardRow));
            if (resized == NULL) {
                break;
            }
            ui->cards = resized;
            capacity = new_capacity;
        }

        UiCardRow *row = &ui->cards[ui->card_count++];
        row->id = sqlite3_column_int64(statement, 0);
        row->topic_id = sqlite3_column_int64(statement, 1);
        const unsigned char *prompt = sqlite3_column_text(statement, 2);
        row->due_at = (time_t)sqlite3_column_int64(statement, 3);
        row->ease_factor = sqlite3_column_int(statement, 4);
        row->interval = sqlite3_column_int(statement, 5);
        row->review_state = sqlite3_column_int(statement, 6);
        row->suspended = sqlite3_column_int(statement, 7) != 0;
        const unsigned char *topic_title = sqlite3_column_text(statement, 8);
        snprintf(row->prompt, sizeof(row->prompt), "%s",
                 prompt != NULL ? (const char *)prompt : "(No prompt)" );
        snprintf(row->topic_title, sizeof(row->topic_title), "%s",
                 topic_title != NULL ? (const char *)topic_title : "Unknown Topic");
    }

    sqlite3_finalize(statement);
    ui->cards_dirty = false;
}

static void ui_draw_text_line(const HrRenderContext *render,
                              Vector2 position,
                              const char *text,
                              Color color,
                              float font_scale)
{
    if (render == NULL || text == NULL) {
        return;
    }

    Font font = render->fonts.regular.texture.id != 0 ? render->fonts.regular : GetFontDefault();
    float font_size = render->base_font_size * render->dpi_scale * font_scale;
    DrawTextEx(font, text, position, font_size, font_size / 4.0f, color);
}

static void ui_draw_sidebar(UiContext *ui, Rectangle bounds)
{
    Color background = ui_palette_color(ui, HR_THEME_COLOR_BACKGROUND_ALT, (Color){20, 24, 36, 255});
    Color text_color = ui_palette_color(ui, HR_THEME_COLOR_TEXT, RAYWHITE);
    Color muted_color = ui_palette_color(ui, HR_THEME_COLOR_TEXT_MUTED, GRAY);
    Color accent = ui_palette_color(ui, HR_THEME_COLOR_ACCENT, SKYBLUE);

    DrawRectangleRec(bounds, background);

    Vector2 title_pos = {bounds.x + 16.0f, bounds.y + 16.0f};
    ui_draw_text_line(&ui->render, title_pos, "Topics", accent, 1.1f);

    Rectangle list_bounds = {
        bounds.x,
        bounds.y + 56.0f,
        bounds.width,
        bounds.height - 56.0f,
    };

    if (ui->topics_dirty) {
        ui_reload_topics(ui);
    }

    const float row_height = 32.0f * ui->render.dpi_scale;
    const Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, list_bounds);
    if (hovered) {
        float wheel = GetMouseWheelMove();
        ui->sidebar_scroll -= wheel * row_height;
        if (ui->sidebar_scroll < 0.0f) {
            ui->sidebar_scroll = 0.0f;
        }
    }

    size_t first_index = (size_t)(ui->sidebar_scroll / row_height);
    float offset_y = list_bounds.y - fmodf(ui->sidebar_scroll, row_height);

    for (size_t i = first_index; i < ui->topic_count; ++i) {
        if (offset_y > list_bounds.y + list_bounds.height) {
            break;
        }

        UiTopicNode *node = &ui->topics[i];
        Rectangle row_rect = {list_bounds.x, offset_y, list_bounds.width, row_height};
        bool selected = (i == ui->selected_topic);
        if (selected) {
            Color selected_bg = ui_palette_color(ui, HR_THEME_COLOR_SURFACE_ALT, (Color){46, 51, 72, 255});
            DrawRectangleRec(row_rect, selected_bg);
        } else if (CheckCollisionPointRec(mouse, row_rect)) {
            Color hover = ui_palette_color(ui, HR_THEME_COLOR_SURFACE, (Color){36, 40, 60, 255});
            DrawRectangleRec(row_rect, hover);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                ui->selected_topic = i;
                ui_push_toast_internal(ui, "Selected topic", accent, 2.5f);
            }
        }

        char buffer[256];
        memset(buffer, ' ', node->depth * 2);
        buffer[node->depth * 2] = '\0';
        strncat(buffer, node->title, sizeof(buffer) - strlen(buffer) - 1U);

        Vector2 text_pos = {row_rect.x + 12.0f, row_rect.y + 6.0f};
        DrawTextEx(ui->render.fonts.regular, buffer, text_pos,
                   ui->render.base_font_size * 0.95f * ui->render.dpi_scale,
                   2.0f, selected ? accent : text_color);

        if (node->summary[0] != '\0') {
            Vector2 summary_pos = {row_rect.x + 12.0f, row_rect.y + row_height * 0.55f};
            DrawTextEx(ui->render.fonts.italic.texture.id != 0 ? ui->render.fonts.italic : ui->render.fonts.regular,
                       node->summary,
                       summary_pos,
                       ui->render.base_font_size * 0.65f * ui->render.dpi_scale,
                       1.5f,
                       muted_color);
        }

        offset_y += row_height;
    }
}

static void ui_draw_card_row(const UiContext *ui,
                             const UiCardRow *row,
                             Rectangle bounds,
                             bool hovered)
{
    Color surface = ui_palette_color(ui, HR_THEME_COLOR_SURFACE, (Color){32, 36, 54, 255});
    Color surface_alt = ui_palette_color(ui, HR_THEME_COLOR_SURFACE_ALT, (Color){44, 49, 70, 255});
    Color text_color = ui_palette_color(ui, HR_THEME_COLOR_TEXT, WHITE);
    Color muted_color = ui_palette_color(ui, HR_THEME_COLOR_TEXT_MUTED, GRAY);
    Color suspended_color = ui_palette_color(ui, HR_THEME_COLOR_WARNING, ORANGE);

    DrawRectangleRec(bounds, hovered ? surface_alt : surface);

    char title_buffer[256];
    snprintf(title_buffer, sizeof(title_buffer), "#%lld  %s",
             (long long)row->id, row->prompt);
    DrawTextEx(ui->render.fonts.regular,
               title_buffer,
               (Vector2){bounds.x + 12.0f, bounds.y + 8.0f},
               ui->render.base_font_size * ui->render.dpi_scale,
               2.0f,
               text_color);

    char meta_buffer[256];
    time_t now = time(NULL);
    double minutes = difftime(row->due_at, now) / 60.0;
    const char *due_label = minutes <= 0.0 ? "due now" : "due in";
    snprintf(meta_buffer, sizeof(meta_buffer), "%s %0.0f mins  |  EF %d  |  Interval %d", due_label,
             fabs(minutes), row->ease_factor, row->interval);

    Color meta_color = row->suspended ? suspended_color : muted_color;
    DrawTextEx(ui->render.fonts.italic.texture.id != 0 ? ui->render.fonts.italic : ui->render.fonts.regular,
               meta_buffer,
               (Vector2){bounds.x + 12.0f, bounds.y + bounds.height - 24.0f},
               ui->render.base_font_size * 0.75f * ui->render.dpi_scale,
               1.5f,
               meta_color);
}

static void ui_draw_card_table(UiContext *ui, Rectangle bounds)
{
    if (ui->cards_dirty) {
        ui_reload_cards(ui);
    }

    Color border = ui_palette_color(ui, HR_THEME_COLOR_BORDER, (Color){60, 66, 90, 255});
    DrawRectangleLinesEx(bounds, 1.0f, border);

    const Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    if (hovered) {
        float wheel = GetMouseWheelMove();
        ui->card_scroll_offset -= wheel * ui->card_row_height;
        if (ui->card_scroll_offset < 0.0f) {
            ui->card_scroll_offset = 0.0f;
        }
    }

    size_t first_index = (size_t)(ui->card_scroll_offset / ui->card_row_height);
    float offset_y = bounds.y - fmodf(ui->card_scroll_offset, ui->card_row_height);

    for (size_t i = first_index; i < ui->card_count; ++i) {
        if (offset_y > bounds.y + bounds.height) {
            break;
        }

        Rectangle row_bounds = {bounds.x, offset_y, bounds.width, ui->card_row_height - 4.0f};
        bool row_hovered = CheckCollisionPointRec(mouse, row_bounds);
        ui_draw_card_row(ui, &ui->cards[i], row_bounds, row_hovered);
        offset_y += ui->card_row_height;
    }
}

static void ui_draw_study_view(UiContext *ui, Rectangle bounds)
{
    Color surface = ui_palette_color(ui, HR_THEME_COLOR_SURFACE, (Color){32, 36, 54, 255});
    Color border = ui_palette_color(ui, HR_THEME_COLOR_BORDER, (Color){62, 69, 98, 255});

    DrawRectangleRec(bounds, surface);
    DrawRectangleLinesEx(bounds, 1.0f, border);

    const SessionCard *current = session_manager_current(ui->sessions);
    if (current == NULL) {
        const char *message = "No active card. Start a session to begin reviewing.";
        Vector2 text_size = MeasureTextEx(ui->render.fonts.regular,
                                          message,
                                          ui->render.base_font_size * ui->render.dpi_scale,
                                          2.0f);
        Vector2 position = {
            bounds.x + (bounds.width - text_size.x) * 0.5f,
            bounds.y + (bounds.height - text_size.y) * 0.5f,
        };
        DrawTextEx(ui->render.fonts.regular,
                   message,
                   position,
                   ui->render.base_font_size * ui->render.dpi_scale,
                   2.0f,
                   ui_palette_color(ui, HR_THEME_COLOR_TEXT_MUTED, RAYWHITE));
        return;
    }

    Rectangle prompt_bounds = {
        bounds.x + 24.0f,
        bounds.y + 24.0f,
        bounds.width - 48.0f,
        bounds.height * 0.45f,
    };

    Rectangle response_bounds = {
        bounds.x + 24.0f,
        prompt_bounds.y + prompt_bounds.height + 16.0f,
        bounds.width - 48.0f,
        bounds.height * 0.45f - 48.0f,
    };

    DrawRectangleLinesEx(prompt_bounds, 1.0f, border);
    DrawRectangleLinesEx(response_bounds, 1.0f, border);

    HrRenderRichTextOptions rich_options = {
        .wrap_width = prompt_bounds.width,
        .allow_markup = true,
        .fallback_color = ui_palette_color(ui, HR_THEME_COLOR_TEXT, WHITE),
    };

    render_draw_rich_text(&ui->render,
                          (Vector2){prompt_bounds.x + 12.0f, prompt_bounds.y + 12.0f},
                          current->state.mode == SRS_MODE_CRAM ? "[CRAM]" : "Prompt",
                          &rich_options);

    render_draw_rich_text(&ui->render,
                          (Vector2){response_bounds.x + 12.0f, response_bounds.y + 12.0f},
                          "Flip the card to reveal the answer...",
                          &rich_options);
}

static void ui_draw_analytics(UiContext *ui, Rectangle bounds)
{
    Color surface = ui_palette_color(ui, HR_THEME_COLOR_SURFACE, (Color){30, 34, 52, 255});
    Color border = ui_palette_color(ui, HR_THEME_COLOR_BORDER, (Color){62, 69, 98, 255});
    Color text_color = ui_palette_color(ui, HR_THEME_COLOR_TEXT, WHITE);
    Color muted_color = ui_palette_color(ui, HR_THEME_COLOR_TEXT_MUTED, GRAY);

    DrawRectangleRec(bounds, surface);
    DrawRectangleLinesEx(bounds, 1.0f, border);

    Vector2 header_pos = {bounds.x + 18.0f, bounds.y + 18.0f};
    ui_draw_text_line(&ui->render, header_pos, "Analytics Dashboard", text_color, 1.1f);

    const HrAnalyticsDashboard *dashboard = analytics_dashboard(ui->analytics);
    bool analytics_ready = dashboard != NULL && analytics_is_enabled(ui->analytics);

    size_t total_reviews = ui->metrics.total_reviews;
    double average_interval = ui->metrics.average_interval_minutes;
    const float *recent_intervals = ui->metrics.recent_intervals;
    size_t recent_count = ui->metrics.recent_count;
    size_t rating_counts[HR_ANALYTICS_RATING_BUCKETS] = {0};
    memcpy(rating_counts, ui->metrics.rating_counts, sizeof(rating_counts));

    size_t current_streak = 0U;
    size_t longest_streak = 0U;

    if (analytics_ready) {
        total_reviews = dashboard->reviews.total_reviews;
        average_interval = dashboard->reviews.average_interval_minutes;
        recent_intervals = dashboard->reviews.recent_intervals;
        recent_count = dashboard->reviews.recent_count;
        memcpy(rating_counts, dashboard->reviews.rating_counts, sizeof(rating_counts));
        current_streak = dashboard->streaks.current_streak;
        longest_streak = dashboard->streaks.longest_streak;
    }

    char summary[128];
    if (analytics_ready && current_streak > 0U) {
        snprintf(summary,
                 sizeof(summary),
                 "Reviews: %zu  |  Avg Interval: %.1f mins  |  Streak: %zu day%s (Best %zu)",
                 total_reviews,
                 average_interval,
                 current_streak,
                 current_streak == 1U ? "" : "s",
                 longest_streak);
    } else {
        snprintf(summary,
                 sizeof(summary),
                 "Reviews: %zu  |  Avg Interval: %.1f mins",
                 total_reviews,
                 average_interval);
    }
    ui_draw_text_line(&ui->render, (Vector2){bounds.x + 18.0f, bounds.y + 54.0f}, summary, muted_color, 0.8f);

    Rectangle chart_bounds = {
        bounds.x + 18.0f,
        bounds.y + 84.0f,
        bounds.width - 36.0f,
        bounds.height * 0.5f,
    };

    HrRenderChartOptions options = {
        .line_color = ui_palette_color(ui, HR_THEME_COLOR_ANALYTICS_PRIMARY, (Color){0, 220, 220, 255}),
        .fill_color = ui_palette_color(ui, HR_THEME_COLOR_ANALYTICS_PRIMARY, (Color){0, 220, 220, 96}),
        .axis_color = border,
        .draw_markers = true,
        .fill_under_curve = true,
    };

    render_draw_line_chart(&ui->render,
                           chart_bounds,
                           recent_intervals,
                           recent_count,
                           &options);

    Rectangle ratings_bounds = {
        bounds.x + 18.0f,
        chart_bounds.y + chart_bounds.height + 24.0f,
        bounds.width - 36.0f,
        bounds.height - (chart_bounds.height + 128.0f),
    };

    Color rating_colors[] = {
        ui_palette_color(ui, HR_THEME_COLOR_DANGER, RED),
        ui_palette_color(ui, HR_THEME_COLOR_WARNING, ORANGE),
        ui_palette_color(ui, HR_THEME_COLOR_SUCCESS, GREEN),
        ui_palette_color(ui, HR_THEME_COLOR_ACCENT, SKYBLUE),
        ui_palette_color(ui, HR_THEME_COLOR_INFO, (Color){120, 220, 255, 255}),
    };

    const char *rating_labels[] = {"Fail", "Hard", "Good", "Easy", "Cram"};

    for (size_t i = 0; i < ARRAY_SIZE(rating_labels); ++i) {
        float share = (total_reviews > 0U)
                          ? (float)rating_counts[i] / (float)total_reviews
                          : 0.0f;
        Rectangle bar = {
            ratings_bounds.x,
            ratings_bounds.y + (float)i * 28.0f,
            ratings_bounds.width * share,
            20.0f,
        };
        DrawRectangleRec(bar, rating_colors[i]);
        char label_buffer[64];
        snprintf(label_buffer, sizeof(label_buffer), "%s (%zu)", rating_labels[i], rating_counts[i]);
        DrawTextEx(ui->render.fonts.regular,
                   label_buffer,
                   (Vector2){ratings_bounds.x + 6.0f, ratings_bounds.y + (float)i * 28.0f},
                   ui->render.base_font_size * 0.75f * ui->render.dpi_scale,
                   1.5f,
                   text_color);
    }
}

static bool ui_command_matches(const UiCommand *command, const char *filter)
{
    if (filter == NULL || filter[0] == '\0') {
        return true;
    }

    char lowercase_filter[64];
    size_t filter_len = strlen(filter);
    for (size_t i = 0; i < filter_len && i + 1U < sizeof(lowercase_filter); ++i) {
        lowercase_filter[i] = (char)tolower((unsigned char)filter[i]);
    }
    lowercase_filter[filter_len] = '\0';

    char lowercase_label[128];
    size_t label_len = strlen(command->label);
    for (size_t i = 0; i < label_len && i + 1U < sizeof(lowercase_label); ++i) {
        lowercase_label[i] = (char)tolower((unsigned char)command->label[i]);
    }
    lowercase_label[label_len] = '\0';

    return strstr(lowercase_label, lowercase_filter) != NULL;
}

static void ui_execute_command(UiContext *ui, const UiCommand *command)
{
    if (ui == NULL || command == NULL) {
        return;
    }

    Color info = ui_palette_color(ui, HR_THEME_COLOR_INFO, SKYBLUE);

    if (strcmp(command->label, "Toggle analytics") == 0) {
        ui->screen = (ui->screen == UI_SCREEN_ANALYTICS) ? UI_SCREEN_STUDY : UI_SCREEN_ANALYTICS;
        ui_push_toast_internal(ui, "Toggled analytics dashboard", info, 2.5f);
    } else if (strcmp(command->label, "Toggle theme editor") == 0) {
        ui->theme_editor_open = !ui->theme_editor_open;
        ui_push_toast_internal(ui, ui->theme_editor_open ? "Theme editor enabled" : "Theme editor hidden", info, 2.0f);
    } else if (strcmp(command->label, "Clear toasts") == 0) {
        ui->toast_count = 0U;
    } else {
        ui_push_toast_internal(ui, command->label, info, 2.0f);
    }
}

static void ui_draw_command_palette(UiContext *ui, Rectangle bounds)
{
    if (!ui->command_palette.open) {
        return;
    }

    Color overlay = {0, 0, 0, 160};
    DrawRectangleRec(bounds, overlay);

    Rectangle panel = {
        bounds.x + bounds.width * 0.15f,
        bounds.y + bounds.height * 0.2f,
        bounds.width * 0.7f,
        bounds.height * 0.6f,
    };

    Color surface = ui_palette_color(ui, HR_THEME_COLOR_SURFACE, (Color){36, 40, 60, 255});
    Color border = ui_palette_color(ui, HR_THEME_COLOR_BORDER, (Color){62, 69, 98, 255});
    Color text_color = ui_palette_color(ui, HR_THEME_COLOR_TEXT, WHITE);
    Color muted = ui_palette_color(ui, HR_THEME_COLOR_TEXT_MUTED, GRAY);
    Color accent = ui_palette_color(ui, HR_THEME_COLOR_ACCENT, SKYBLUE);

    DrawRectangleRec(panel, surface);
    DrawRectangleLinesEx(panel, 2.0f, border);

    Vector2 input_pos = {panel.x + 16.0f, panel.y + 16.0f};
    DrawRectangleRec((Rectangle){panel.x + 12.0f, panel.y + 12.0f, panel.width - 24.0f, 36.0f}, border);
    DrawTextEx(ui->render.fonts.regular,
               ui->command_palette.filter,
               input_pos,
               ui->render.base_font_size * ui->render.dpi_scale,
               2.0f,
               text_color);

    Rectangle list_bounds = {
        panel.x + 12.0f,
        panel.y + 60.0f,
        panel.width - 24.0f,
        panel.height - 72.0f,
    };

    size_t visible_index = 0U;
    for (size_t i = 0; i < ARRAY_SIZE(kUiCommands); ++i) {
        if (!ui_command_matches(&kUiCommands[i], ui->command_palette.filter)) {
            continue;
        }
        Rectangle row = {
            list_bounds.x,
            list_bounds.y + (float)visible_index * 48.0f,
            list_bounds.width,
            46.0f,
        };
        if (row.y > list_bounds.y + list_bounds.height) {
            break;
        }
        bool selected = (visible_index == ui->command_palette.selected_index);
        DrawRectangleRec(row, selected ? accent : surface);
        DrawRectangleLinesEx(row, 1.0f, border);

        DrawTextEx(ui->render.fonts.regular,
                   kUiCommands[i].label,
                   (Vector2){row.x + 12.0f, row.y + 6.0f},
                   ui->render.base_font_size * ui->render.dpi_scale,
                   2.0f,
                   selected ? surface : text_color);
        DrawTextEx(ui->render.fonts.italic.texture.id != 0 ? ui->render.fonts.italic : ui->render.fonts.regular,
                   kUiCommands[i].description,
                   (Vector2){row.x + 12.0f, row.y + 26.0f},
                   ui->render.base_font_size * 0.75f * ui->render.dpi_scale,
                   1.5f,
                   selected ? surface : muted);
        visible_index++;
    }
}

static void ui_draw_modal(const UiContext *ui, Rectangle bounds)
{
    if (!ui->modal.visible) {
        return;
    }

    Color overlay = {0, 0, 0, 200};
    DrawRectangleRec(bounds, overlay);

    Rectangle dialog = {
        bounds.x + bounds.width * 0.2f,
        bounds.y + bounds.height * 0.25f,
        bounds.width * 0.6f,
        bounds.height * 0.4f,
    };

    Color surface = ui_palette_color(ui, HR_THEME_COLOR_SURFACE, (Color){36, 40, 60, 255});
    Color border = ui_palette_color(ui, HR_THEME_COLOR_BORDER, (Color){62, 69, 98, 255});
    Color text_color = ui_palette_color(ui, HR_THEME_COLOR_TEXT, WHITE);

    DrawRectangleRec(dialog, surface);
    DrawRectangleLinesEx(dialog, 2.0f, border);

    DrawTextEx(ui->render.fonts.bold.texture.id != 0 ? ui->render.fonts.bold : ui->render.fonts.regular,
               ui->modal.title,
               (Vector2){dialog.x + 24.0f, dialog.y + 24.0f},
               ui->render.base_font_size * 1.1f * ui->render.dpi_scale,
               2.0f,
               text_color);

    render_draw_rich_text(&ui->render,
                          (Vector2){dialog.x + 24.0f, dialog.y + 72.0f},
                          ui->modal.body,
                          &(HrRenderRichTextOptions){.wrap_width = dialog.width - 48.0f,
                                                     .allow_markup = false,
                                                     .fallback_color = text_color});
}

static void ui_draw_toasts(UiContext *ui, Rectangle bounds, float delta_time)
{
    if (ui->toast_count == 0U) {
        return;
    }

    float y = bounds.y + 24.0f;
    for (size_t i = 0; i < ui->toast_count;) {
        UiToast *toast = &ui->toasts[i];
        toast->ttl -= delta_time;
        if (toast->ttl <= 0.0f) {
            memmove(toast, toast + 1, (ui->toast_count - i - 1) * sizeof(UiToast));
            ui->toast_count--;
            continue;
        }

        float alpha_scale = fminf(toast->ttl, 1.0f);
        Color background = toast->background;
        background.a = (unsigned char)((float)background.a * alpha_scale);
        Rectangle toast_bounds = {
            bounds.x + bounds.width - 340.0f,
            y,
            320.0f,
            44.0f,
        };
        DrawRectangleRounded(toast_bounds, 0.25f, 8, background);
        DrawTextEx(ui->render.fonts.regular,
                   toast->message,
                   (Vector2){toast_bounds.x + 12.0f, toast_bounds.y + 12.0f},
                   ui->render.base_font_size * 0.8f * ui->render.dpi_scale,
                   1.5f,
                   ui_palette_color(ui, HR_THEME_COLOR_TOAST_TEXT, RAYWHITE));
        y += toast_bounds.height + 12.0f;
        ++i;
    }
}

static void ui_update_metrics(UiContext *ui, const SessionReviewEvent *event)
{
    if (ui == NULL || event == NULL) {
        return;
    }

    ui->metrics.total_reviews++;
    if (event->result.rating >= SRS_RESPONSE_FAIL && event->result.rating <= SRS_RESPONSE_CRAM) {
        ui->metrics.rating_counts[event->result.rating]++;
    }

    float interval_minutes = (float)(event->result.interval_minutes);
    if (ui->metrics.recent_count == ARRAY_SIZE(ui->metrics.recent_intervals)) {
        memmove(&ui->metrics.recent_intervals[0], &ui->metrics.recent_intervals[1],
                (ARRAY_SIZE(ui->metrics.recent_intervals) - 1U) * sizeof(float));
        ui->metrics.recent_count--;
    }
    ui->metrics.recent_intervals[ui->metrics.recent_count++] = interval_minutes;

    double total = ui->metrics.average_interval_minutes * (double)(ui->metrics.total_reviews - 1U);
    ui->metrics.average_interval_minutes = (total + interval_minutes) / (double)ui->metrics.total_reviews;
}

static void ui_session_event_dispatch(const SessionReviewEvent *event, void *user_data)
{
    UiContext *ui = (UiContext *)user_data;
    if (ui == NULL) {
        return;
    }

    ui_update_metrics(ui, event);

    Color toast_color;
    switch (event->result.rating) {
    case SRS_RESPONSE_FAIL:
        toast_color = ui_palette_color(ui, HR_THEME_COLOR_DANGER, RED);
        break;
    case SRS_RESPONSE_HARD:
        toast_color = ui_palette_color(ui, HR_THEME_COLOR_WARNING, ORANGE);
        break;
    case SRS_RESPONSE_GOOD:
        toast_color = ui_palette_color(ui, HR_THEME_COLOR_SUCCESS, GREEN);
        break;
    case SRS_RESPONSE_EASY:
        toast_color = ui_palette_color(ui, HR_THEME_COLOR_ACCENT, SKYBLUE);
        break;
    case SRS_RESPONSE_CRAM:
    default:
        toast_color = ui_palette_color(ui, HR_THEME_COLOR_INFO, SKYBLUE);
        break;
    }

    char message[128];
    snprintf(message, sizeof(message), "Card %llu reviewed (%s)",
             (unsigned long long)event->card_id,
             event->result.rating == SRS_RESPONSE_FAIL   ? "Fail"
             : event->result.rating == SRS_RESPONSE_HARD ? "Hard"
             : event->result.rating == SRS_RESPONSE_GOOD ? "Good"
             : event->result.rating == SRS_RESPONSE_EASY ? "Easy"
                                                        : "Cram");
    ui_push_toast_internal(ui, message, toast_color, 2.5f);

    ui->cards_dirty = true;

    if (ui->chained_callbacks.session_event != NULL) {
        ui->chained_callbacks.session_event(event, ui->chained_callbacks.session_user_data);
    }
}

static void ui_theme_changed_callback(const HrThemePalette *palette, void *user_data)
{
    UiContext *ui = (UiContext *)user_data;
    if (ui == NULL) {
        return;
    }
    render_context_set_palette(&ui->render, palette);
    Color toast_bg = ui_palette_color(ui, HR_THEME_COLOR_INFO, SKYBLUE);
    char message[128];
    snprintf(message, sizeof(message), "Theme switched to %s", palette != NULL ? palette->name : "Unknown");
    ui_push_toast_internal(ui, message, toast_bg, 2.0f);
}

UiContext *ui_create(const UiConfig *config)
{
    UiContext *ui = (UiContext *)calloc(1U, sizeof(UiContext));
    if (ui == NULL) {
        return NULL;
    }

    if (config != NULL) {
        ui->config = *config;
    }

    render_context_init(&ui->render, NULL, 20.0f);
    ui->card_row_height = 88.0f;
    ui->screen = UI_SCREEN_STUDY;
    return ui;
}

void ui_destroy(UiContext *ui)
{
    if (ui == NULL) {
        return;
    }

    free(ui->topics);
    free(ui->cards);
    free(ui);
}

void ui_attach_theme_manager(UiContext *ui, struct HrThemeManager *themes)
{
    if (ui == NULL) {
        return;
    }

    ui->themes = themes;
    if (themes != NULL) {
        theme_manager_set_changed_callback(themes, ui_theme_changed_callback, ui);
        const HrThemePalette *palette = theme_manager_active(themes);
        render_context_set_palette(&ui->render, palette);
    }
}

void ui_attach_session_manager(UiContext *ui,
                               struct SessionManager *sessions,
                               const SessionCallbacks *forward_callbacks)
{
    if (ui == NULL) {
        return;
    }

    ui->sessions = sessions;
    if (forward_callbacks != NULL) {
        ui->chained_callbacks = *forward_callbacks;
    } else {
        memset(&ui->chained_callbacks, 0, sizeof(ui->chained_callbacks));
    }

    if (sessions != NULL) {
        SessionCallbacks callbacks = ui->chained_callbacks;
        callbacks.session_event = ui_session_event_dispatch;
        callbacks.session_user_data = ui;
        session_manager_set_callbacks(sessions, &callbacks);
    }
}

void ui_attach_analytics(UiContext *ui, struct AnalyticsHandle *analytics)
{
    if (ui == NULL) {
        return;
    }
    ui->analytics = analytics;
}

void ui_attach_database(UiContext *ui, DatabaseHandle *database)
{
    if (ui == NULL) {
        return;
    }

    ui->database = database;
    ui->topics_dirty = true;
    ui->cards_dirty = true;
}

void ui_attach_import_export(UiContext *ui, struct ImportExportContext *io_context)
{
    if (ui == NULL) {
        return;
    }
    ui->import_export = io_context;
}

void ui_set_fonts(UiContext *ui, const HrRenderFontSet *fonts, float base_font_size)
{
    if (ui == NULL) {
        return;
    }

    render_context_init(&ui->render, fonts, base_font_size);
    if (ui->themes != NULL) {
        render_context_set_palette(&ui->render, theme_manager_active(ui->themes));
    }
}

const HrRenderContext *ui_render_context(const UiContext *ui)
{
    if (ui == NULL) {
        return NULL;
    }
    return &ui->render;
}

const HrThemePalette *ui_active_palette(const UiContext *ui)
{
    if (ui == NULL) {
        return NULL;
    }
    return ui->render.palette;
}

void ui_toggle_command_palette(UiContext *ui)
{
    if (ui == NULL) {
        return;
    }

    ui->command_palette.open = !ui->command_palette.open;
    if (ui->command_palette.open) {
        ui->command_palette.selected_index = 0U;
        ui->command_palette.filter[0] = '\0';
    }
}

void ui_push_toast(UiContext *ui, const char *message, Color background, float duration_seconds)
{
    ui_push_toast_internal(ui, message, background, duration_seconds);
}

void ui_show_modal(UiContext *ui, const char *title, const char *body)
{
    if (ui == NULL) {
        return;
    }
    ui->modal.visible = true;
    snprintf(ui->modal.title, sizeof(ui->modal.title), "%s", title != NULL ? title : "");
    snprintf(ui->modal.body, sizeof(ui->modal.body), "%s", body != NULL ? body : "");
}

void ui_close_modal(UiContext *ui)
{
    if (ui == NULL) {
        return;
    }
    ui->modal.visible = false;
}

void ui_request_screen(UiContext *ui, UiScreenId screen)
{
    if (ui == NULL) {
        return;
    }
    ui->screen = screen;
}

static void ui_handle_command_palette_input(UiContext *ui)
{
    if (!ui->command_palette.open) {
        return;
    }

    int key = GetCharPressed();
    while (key > 0) {
        size_t len = strlen(ui->command_palette.filter);
        if (len + 1U < sizeof(ui->command_palette.filter) && !iscntrl(key)) {
            ui->command_palette.filter[len] = (char)key;
            ui->command_palette.filter[len + 1U] = '\0';
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        size_t len = strlen(ui->command_palette.filter);
        if (len > 0U) {
            ui->command_palette.filter[len - 1U] = '\0';
        }
    }

    size_t match_count = 0U;
    for (size_t i = 0; i < ARRAY_SIZE(kUiCommands); ++i) {
        if (ui_command_matches(&kUiCommands[i], ui->command_palette.filter)) {
            if (match_count == ui->command_palette.selected_index) {
                break;
            }
            match_count++;
        }
    }

    if (IsKeyPressed(KEY_DOWN)) {
        ui->command_palette.selected_index++;
    }
    if (IsKeyPressed(KEY_UP)) {
        if (ui->command_palette.selected_index > 0U) {
            ui->command_palette.selected_index--;
        }
    }

    if (IsKeyPressed(KEY_ENTER)) {
        match_count = 0U;
        for (size_t i = 0; i < ARRAY_SIZE(kUiCommands); ++i) {
            if (!ui_command_matches(&kUiCommands[i], ui->command_palette.filter)) {
                continue;
            }
            if (match_count == ui->command_palette.selected_index) {
                ui_execute_command(ui, &kUiCommands[i]);
                break;
            }
            match_count++;
        }
        ui->command_palette.open = false;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        ui->command_palette.open = false;
    }
}

static void ui_handle_hotkeys(UiContext *ui)
{
    if (IsKeyPressed(KEY_F1)) {
        ui_toggle_command_palette(ui);
    }
    if (IsKeyPressed(KEY_F2)) {
        ui_request_screen(ui, UI_SCREEN_STUDY);
    }
    if (IsKeyPressed(KEY_F3)) {
        ui_request_screen(ui, UI_SCREEN_LIBRARY);
    }
    if (IsKeyPressed(KEY_F4)) {
        ui_request_screen(ui, UI_SCREEN_ANALYTICS);
    }
    if (IsKeyPressed(KEY_T) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))) {
        ui->theme_editor_open = !ui->theme_editor_open;
    }
}

bool ui_process_frame(UiContext *ui, const HrPlatformFrame *frame)
{
    if (ui == NULL || frame == NULL) {
        return false;
    }

    ui->last_frame = *frame;
    ui->elapsed_time += frame->delta_time;

    if (ui->themes != NULL) {
        const HrThemePalette *active = theme_manager_active(ui->themes);
        if (active != ui->render.palette) {
            render_context_set_palette(&ui->render, active);
        }
    }

    ui_handle_hotkeys(ui);
    ui_handle_command_palette_input(ui);

    Color background = ui_palette_color(ui, HR_THEME_COLOR_BACKGROUND, (Color){18, 20, 32, 255});
    ClearBackground(background);

    float sidebar_width = fminf(320.0f * ui->render.dpi_scale, (float)frame->render_width * 0.32f);
    Rectangle sidebar_bounds = {0.0f, 0.0f, sidebar_width, (float)frame->render_height};
    Rectangle content_bounds = {sidebar_width, 0.0f,
                                (float)frame->render_width - sidebar_width,
                                (float)frame->render_height};

    ui_draw_sidebar(ui, sidebar_bounds);

    Rectangle table_bounds = {
        content_bounds.x + 24.0f,
        content_bounds.y + 24.0f,
        content_bounds.width - 48.0f,
        content_bounds.height * 0.35f,
    };
    ui_draw_card_table(ui, table_bounds);

    Rectangle detail_bounds = {
        content_bounds.x + 24.0f,
        table_bounds.y + table_bounds.height + 24.0f,
        content_bounds.width - 48.0f,
        content_bounds.height - (table_bounds.height + 64.0f),
    };

    switch (ui->screen) {
    case UI_SCREEN_STUDY:
        ui_draw_study_view(ui, detail_bounds);
        break;
    case UI_SCREEN_ANALYTICS:
        ui_draw_analytics(ui, detail_bounds);
        break;
    case UI_SCREEN_LIBRARY:
        ui_draw_card_table(ui, detail_bounds);
        break;
    }

    ui_draw_command_palette(ui, (Rectangle){0, 0, (float)frame->render_width, (float)frame->render_height});
    ui_draw_modal(ui, (Rectangle){0, 0, (float)frame->render_width, (float)frame->render_height});
    ui_draw_toasts(ui, (Rectangle){0, 0, (float)frame->render_width, (float)frame->render_height}, (float)frame->delta_time);

    return true;
}
