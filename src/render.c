#include "render.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

// Stub implementations for Qt6 backend
static inline Font GetFontDefault(void) { return (Font){0}; }
static inline Vector2 MeasureTextEx(Font font, const char *text, float fontSize, float spacing) {
    (void)font; (void)text; (void)fontSize; (void)spacing;
    return (Vector2){0, 0};
}
static inline void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
    (void)font; (void)text; (void)position; (void)fontSize; (void)spacing; (void)tint;
}
static inline void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) {
    (void)rec; (void)roundness; (void)segments; (void)color;
}
static inline void DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, float lineThick, Color color) {
    (void)rec; (void)roundness; (void)segments; (void)lineThick; (void)color;
}
static inline void DrawTriangleFan(Vector2 *points, int pointCount, Color color) {
    (void)points; (void)pointCount; (void)color;
}
static inline void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) {
    (void)startPos; (void)endPos; (void)thick; (void)color;
}
static inline void DrawCircleV(Vector2 center, float radius, Color color) {
    (void)center; (void)radius; (void)color;
}

static Font resolve_font(Font candidate)
{
    (void)candidate;
    return GetFontDefault();
}

static float measure_text_range(Font font, const char *text, size_t length, float font_size)
{
    if (length == 0U) {
        return 0.0f;
    }

    char buffer[512];
    if (length + 1U >= sizeof(buffer)) {
        length = sizeof(buffer) - 1U;
    }
    memcpy(buffer, text, length);
    buffer[length] = '\0';

    Vector2 size = MeasureTextEx(font, buffer, font_size, font_size / 4.0f);
    return size.x;
}

static Color resolve_palette_color(const HrRenderContext *context, HrThemeColorRole role, Color fallback)
{
    if (context != NULL && context->palette != NULL) {
        return theme_palette_color(context->palette, role);
    }
    return fallback;
}

static float draw_wrapped_segment(Font font,
                                  const char *text,
                                  size_t length,
                                  Vector2 *pen,
                                  float origin_x,
                                  float line_height,
                                  float font_size,
                                  float wrap_width,
                                  Color color,
                                  bool code_background,
                                  Color code_background_color)
{
    if (length == 0U) {
        return 0.0f;
    }

    const float space_advance = MeasureTextEx(font, " ", font_size, font_size / 4.0f).x;
    float max_x = pen->x;
    size_t index = 0U;

    while (index < length) {
        while (index < length && text[index] == ' ') {
            pen->x += space_advance;
            index++;
        }
        if (index >= length) {
            break;
        }

        size_t word_start = index;
        while (index < length && text[index] != ' ' && text[index] != '\n') {
            index++;
        }
        size_t word_length = index - word_start;
        float word_width = measure_text_range(font, text + word_start, word_length, font_size);

        if (wrap_width > 0.0f) {
            float right_edge = origin_x + wrap_width;
            if (pen->x > origin_x && pen->x + word_width > right_edge) {
                pen->x = origin_x;
                pen->y += line_height;
            }
        }

        char buffer[512];
        if (word_length + 1U >= sizeof(buffer)) {
            word_length = sizeof(buffer) - 1U;
        }
        memcpy(buffer, text + word_start, word_length);
        buffer[word_length] = '\0';

        if (code_background) {
            Rectangle rect = {
                pen->x - 2.0f,
                pen->y - line_height * 0.1f,
                word_width + 4.0f,
                line_height,
            };
            DrawRectangleRounded(rect, 0.15f, 8, code_background_color);
        }

        DrawTextEx(font, buffer, *pen, font_size, font_size / 4.0f, color);
        pen->x += word_width;
        if (pen->x > max_x) {
            max_x = pen->x;
        }

        while (index < length && text[index] == ' ') {
            pen->x += space_advance;
            index++;
        }
    }

    return max_x;
}

void render_context_init(HrRenderContext *context,
                         const HrRenderFontSet *fonts,
                         float base_font_size)
{
    if (context == NULL) {
        return;
    }

    if (fonts != NULL) {
        context->fonts = *fonts;
    } else {
        context->fonts.regular = GetFontDefault();
        context->fonts.bold = GetFontDefault();
        context->fonts.italic = GetFontDefault();
        context->fonts.monospace = GetFontDefault();
    }

    context->base_font_size = base_font_size > 0.0f ? base_font_size : 18.0f;
    context->line_height = 1.45f;
    context->dpi_scale = 1.0f;
    context->palette = NULL;
}

void render_context_set_palette(HrRenderContext *context, const HrThemePalette *palette)
{
    if (context == NULL) {
        return;
    }
    context->palette = palette;
}

void render_context_set_dpi_scale(HrRenderContext *context, float dpi_scale)
{
    if (context == NULL) {
        return;
    }
    context->dpi_scale = (dpi_scale > 0.0f) ? dpi_scale : 1.0f;
}

float render_draw_rich_text(const HrRenderContext *context,
                            Vector2 origin,
                            const char *text,
                            const HrRenderRichTextOptions *options)
{
    if (context == NULL || text == NULL) {
        return 0.0f;
    }

    HrRenderRichTextOptions defaults = {
        .wrap_width = 0.0f,
        .allow_markup = true,
        .fallback_color = WHITE,
    };
    if (options == NULL) {
        options = &defaults;
    }

    const float font_size = context->base_font_size * context->dpi_scale;
    const float line_height = context->line_height * font_size;

    Font regular = resolve_font(context->fonts.regular);
    Font bold = resolve_font(context->fonts.bold);
    Font italic = resolve_font(context->fonts.italic);
    Font monospace = resolve_font(context->fonts.monospace);

    Vector2 pen = origin;
    float max_y = origin.y;

    bool bold_active = false;
    bool italic_active = false;
    bool code_active = false;

    Color base_color = resolve_palette_color(context, HR_THEME_COLOR_TEXT, options->fallback_color);
    Color code_background = resolve_palette_color(context, HR_THEME_COLOR_CODE_BACKGROUND, (Color){24, 27, 44, 255});
    Color code_color = resolve_palette_color(context, HR_THEME_COLOR_CODE_TEXT, base_color);

    char segment[512];
    size_t segment_len = 0U;

    const char *cursor = text;
    while (*cursor != '\0') {
        if (*cursor == '\n') {
            if (segment_len > 0U) {
                Font font = code_active ? monospace : (bold_active ? bold : regular);
                if (italic_active && !code_active) {
                    font = italic;
                }
                Color color = code_active ? code_color : base_color;
                draw_wrapped_segment(font,
                                     segment,
                                     segment_len,
                                     &pen,
                                     origin.x,
                                     line_height,
                                     font_size,
                                     options->wrap_width * context->dpi_scale,
                                     color,
                                     code_active,
                                     code_background);
                segment_len = 0U;
            }
            pen.x = origin.x;
            pen.y += line_height;
            cursor++;
            continue;
        }

        if (options->allow_markup) {
            if (*cursor == '*' && *(cursor + 1) == '*') {
                cursor += 2;
                bold_active = !bold_active;
                continue;
            }
            if (*cursor == '_' && *(cursor + 1) != '_') {
                cursor++;
                italic_active = !italic_active;
                continue;
            }
            if (*cursor == '`') {
                cursor++;
                code_active = !code_active;
                continue;
            }
        }

        if (segment_len + 1U >= sizeof(segment)) {
            Font font = code_active ? monospace : (bold_active ? bold : regular);
            if (italic_active && !code_active) {
                font = italic;
            }
            Color color = code_active ? code_color : base_color;
            draw_wrapped_segment(font,
                                 segment,
                                 segment_len,
                                 &pen,
                                 origin.x,
                                 line_height,
                                 font_size,
                                 options->wrap_width * context->dpi_scale,
                                 color,
                                 code_active,
                                 code_background);
            segment_len = 0U;
        }

        segment[segment_len++] = *cursor++;
    }

    if (segment_len > 0U) {
        Font font = code_active ? monospace : (bold_active ? bold : regular);
        if (italic_active && !code_active) {
            font = italic;
        }
        Color color = code_active ? code_color : base_color;
        draw_wrapped_segment(font,
                             segment,
                             segment_len,
                             &pen,
                             origin.x,
                             line_height,
                             font_size,
                             options->wrap_width * context->dpi_scale,
                             color,
                             code_active,
                             code_background);
    }

    max_y = pen.y + line_height;
    return max_y - origin.y;
}

float render_draw_cloze_text(const HrRenderContext *context,
                             Vector2 origin,
                             const char *text,
                             const HrRenderClozeOptions *options)
{
    if (context == NULL || text == NULL) {
        return 0.0f;
    }

    HrRenderClozeOptions defaults = {
        .wrap_width = 0.0f,
    };
    if (options == NULL) {
        options = &defaults;
    }

    const float font_size = context->base_font_size * context->dpi_scale;
    const float line_height = context->line_height * font_size;

    Font font = resolve_font(context->fonts.regular);
    Color text_color = resolve_palette_color(context, HR_THEME_COLOR_TEXT, WHITE);
    Color gap_background = resolve_palette_color(context, HR_THEME_COLOR_CLOZE_GAP, (Color){0, 200, 200, 80});
    Color gap_text = resolve_palette_color(context, HR_THEME_COLOR_CLOZE_TEXT, text_color);

    Vector2 pen = origin;
    char segment[512];
    size_t segment_len = 0U;

    bool in_gap = false;
    const char *cursor = text;

    while (*cursor != '\0') {
        if (strncmp(cursor, "{{", 2) == 0) {
            if (segment_len > 0U) {
                draw_wrapped_segment(font,
                                     segment,
                                     segment_len,
                                     &pen,
                                     origin.x,
                                     line_height,
                                     font_size,
                                     options->wrap_width * context->dpi_scale,
                                     in_gap ? gap_text : text_color,
                                     in_gap,
                                     gap_background);
                segment_len = 0U;
            }
            in_gap = true;
            cursor += 2;
            continue;
        }
        if (strncmp(cursor, "}}", 2) == 0) {
            if (segment_len > 0U) {
                draw_wrapped_segment(font,
                                     segment,
                                     segment_len,
                                     &pen,
                                     origin.x,
                                     line_height,
                                     font_size,
                                     options->wrap_width * context->dpi_scale,
                                     in_gap ? gap_text : text_color,
                                     in_gap,
                                     gap_background);
                segment_len = 0U;
            }
            in_gap = false;
            cursor += 2;
            continue;
        }

        if (*cursor == '\n') {
            if (segment_len > 0U) {
                draw_wrapped_segment(font,
                                     segment,
                                     segment_len,
                                     &pen,
                                     origin.x,
                                     line_height,
                                     font_size,
                                     options->wrap_width * context->dpi_scale,
                                     in_gap ? gap_text : text_color,
                                     in_gap,
                                     gap_background);
                segment_len = 0U;
            }
            pen.x = origin.x;
            pen.y += line_height;
            cursor++;
            continue;
        }

        if (segment_len + 1U >= sizeof(segment)) {
            draw_wrapped_segment(font,
                                 segment,
                                 segment_len,
                                 &pen,
                                 origin.x,
                                 line_height,
                                 font_size,
                                 options->wrap_width * context->dpi_scale,
                                 in_gap ? gap_text : text_color,
                                 in_gap,
                                 gap_background);
            segment_len = 0U;
        }

        segment[segment_len++] = *cursor++;
    }

    if (segment_len > 0U) {
        draw_wrapped_segment(font,
                             segment,
                             segment_len,
                             &pen,
                             origin.x,
                             line_height,
                             font_size,
                             options->wrap_width * context->dpi_scale,
                             in_gap ? gap_text : text_color,
                             in_gap,
                             gap_background);
    }

    return pen.y + line_height - origin.y;
}

void render_draw_code_block(const HrRenderContext *context,
                            Rectangle bounds,
                            const char *code)
{
    if (context == NULL || code == NULL) {
        return;
    }

    Color background = resolve_palette_color(context, HR_THEME_COLOR_CODE_BACKGROUND, (Color){24, 27, 44, 255});
    Color border = resolve_palette_color(context, HR_THEME_COLOR_BORDER, (Color){62, 69, 98, 255});
    Color text_color = resolve_palette_color(context, HR_THEME_COLOR_CODE_TEXT, WHITE);

    DrawRectangleRounded(bounds, 0.08f, 8, background);
    DrawRectangleRoundedLines(bounds, 0.08f, 8, 1.0f, border);

    Font monospace = resolve_font(context->fonts.monospace);
    const float font_size = (context->base_font_size * 0.9f) * context->dpi_scale;
    const float line_height = context->line_height * font_size;

    Vector2 pen = {
        bounds.x + 12.0f,
        bounds.y + 8.0f,
    };

    char buffer[512];
    size_t buffer_len = 0U;
    const char *cursor = code;

    while (*cursor != '\0') {
        if (*cursor == '\n' || buffer_len + 1U >= sizeof(buffer)) {
            buffer[buffer_len] = '\0';
            DrawTextEx(monospace, buffer, pen, font_size, font_size / 4.0f, text_color);
            pen.y += line_height;
            buffer_len = 0U;
            if (*cursor == '\n') {
                cursor++;
            }
            continue;
        }
        buffer[buffer_len++] = *cursor++;
    }

    if (buffer_len > 0U) {
        buffer[buffer_len] = '\0';
        DrawTextEx(monospace, buffer, pen, font_size, font_size / 4.0f, text_color);
    }
}

float render_draw_line_chart(const HrRenderContext *context,
                             Rectangle bounds,
                             const float *samples,
                             size_t count,
                             const HrRenderChartOptions *options)
{
    if (context == NULL || samples == NULL || count == 0U) {
        return 0.0f;
    }

    HrRenderChartOptions defaults = {
        .line_color = resolve_palette_color(context, HR_THEME_COLOR_ANALYTICS_PRIMARY, (Color){0, 220, 220, 255}),
        .fill_color = resolve_palette_color(context, HR_THEME_COLOR_ANALYTICS_PRIMARY, (Color){0, 220, 220, 96}),
        .axis_color = resolve_palette_color(context, HR_THEME_COLOR_TEXT_MUTED, (Color){160, 168, 194, 255}),
        .draw_markers = true,
        .fill_under_curve = true,
    };
    if (options == NULL) {
        options = &defaults;
    }

    DrawRectangleRoundedLines(bounds, 0.05f, 6, 1.0f, options->axis_color);

    float min_value = samples[0];
    float max_value = samples[0];
    for (size_t i = 1; i < count; ++i) {
        if (samples[i] < min_value) {
            min_value = samples[i];
        }
        if (samples[i] > max_value) {
            max_value = samples[i];
        }
    }
    if (fabsf(max_value - min_value) < 1e-6f) {
        max_value = min_value + 1.0f;
    }

    float vertical_range = max_value - min_value;
    float step_x = (count > 1U) ? (bounds.width / (float)(count - 1U)) : bounds.width;

    Vector2 previous = {bounds.x, bounds.y + bounds.height};
    previous.y -= ((samples[0] - min_value) / vertical_range) * bounds.height;

    if (options->fill_under_curve) {
        for (size_t i = 1; i < count; ++i) {
            Vector2 current = {
                bounds.x + step_x * (float)i,
                bounds.y + bounds.height,
            };
            current.y -= ((samples[i] - min_value) / vertical_range) * bounds.height;

            Vector2 triangle[3] = {
                {previous.x, bounds.y + bounds.height},
                previous,
                current,
            };
            DrawTriangleFan(triangle, 3, options->fill_color);
            previous = current;
        }
    }

    previous.x = bounds.x;
    previous.y = bounds.y + bounds.height - ((samples[0] - min_value) / vertical_range) * bounds.height;

    for (size_t i = 1; i < count; ++i) {
        Vector2 current = {
            bounds.x + step_x * (float)i,
            bounds.y + bounds.height - ((samples[i] - min_value) / vertical_range) * bounds.height,
        };
        DrawLineEx(previous, current, 2.0f, options->line_color);
        if (options->draw_markers) {
            DrawCircleV(previous, 3.0f, options->line_color);
        }
        previous = current;
    }

    if (options->draw_markers) {
        DrawCircleV(previous, 3.0f, options->line_color);
    }

    return vertical_range;
}
