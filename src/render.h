#ifndef HYPERRECALL_RENDER_H
#define HYPERRECALL_RENDER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file render.h
 * @brief Declares rendering utilities supporting layered composition and effects.
 */

#include <stdbool.h>
#include <stddef.h>

#ifndef HYPERRECALL_UI_QT6
#include <raylib.h>
#else
// Forward declare raylib types for Qt backend compatibility
typedef struct Font {
    int baseSize;
    int glyphCount;
    int glyphPadding;
    void *texture;
    void *recs;
    void *glyphs;
} Font;

typedef struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Color;
#endif

#include "theme.h"

/** Describes the font set used when rendering complex text. */
typedef struct HrRenderFontSet {
    Font regular;   /**< Primary UI font. */
    Font bold;      /**< Bold variant used for emphasis. */
    Font italic;    /**< Italic variant used in rich text. */
    Font monospace; /**< Monospace font used for code blocks. */
} HrRenderFontSet;

/** Captures global rendering parameters shared across UI surfaces. */
typedef struct HrRenderContext {
    HrRenderFontSet fonts;           /**< Font collection used for drawing. */
    float base_font_size;            /**< Base font size in pixels. */
    float line_height;               /**< Default line height multiplier. */
    float dpi_scale;                 /**< UI scale multiplier. */
    const HrThemePalette *palette;   /**< Active theme palette (may be NULL). */
} HrRenderContext;

/** Options controlling how rich text is drawn. */
typedef struct HrRenderRichTextOptions {
    float wrap_width;    /**< Optional wrapping width in pixels (<=0 disables wrapping). */
    bool allow_markup;   /**< When true, lightweight markup is processed. */
    Color fallback_color;/**< Color used when no palette is available. */
} HrRenderRichTextOptions;

/** Options controlling how cloze deletions are highlighted. */
typedef struct HrRenderClozeOptions {
    float wrap_width; /**< Optional wrapping width in pixels (<=0 disables wrapping). */
} HrRenderClozeOptions;

/** Options controlling analytics chart rendering. */
typedef struct HrRenderChartOptions {
    Color line_color;      /**< Primary series color. */
    Color fill_color;      /**< Fill color under the chart (alpha respected). */
    Color axis_color;      /**< Axis/label color. */
    bool draw_markers;     /**< Draw markers for each sample when true. */
    bool fill_under_curve; /**< Fill the area under the curve when true. */
} HrRenderChartOptions;

/** Initializes a render context using the supplied font set and defaults. */
void render_context_init(HrRenderContext *context,
                         const HrRenderFontSet *fonts,
                         float base_font_size);

/** Updates the render context with the active theme palette. */
void render_context_set_palette(HrRenderContext *context, const HrThemePalette *palette);

/** Adjusts the UI scale applied during rendering. */
void render_context_set_dpi_scale(HrRenderContext *context, float dpi_scale);

/** Draws a block of rich text and returns the rendered height in pixels. */
float render_draw_rich_text(const HrRenderContext *context,
                            Vector2 origin,
                            const char *text,
                            const HrRenderRichTextOptions *options);

/** Draws cloze deletions using highlighted spans and returns the rendered height. */
float render_draw_cloze_text(const HrRenderContext *context,
                             Vector2 origin,
                             const char *text,
                             const HrRenderClozeOptions *options);

/** Draws a syntax highlighted code block inside the supplied bounds. */
void render_draw_code_block(const HrRenderContext *context,
                            Rectangle bounds,
                            const char *code);

/** Draws a simple analytics line chart using @p samples and returns the y-range. */
float render_draw_line_chart(const HrRenderContext *context,
                             Rectangle bounds,
                             const float *samples,
                             size_t count,
                             const HrRenderChartOptions *options);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_RENDER_H */
