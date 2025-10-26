#ifndef HYPERRECALL_TYPES_H
#define HYPERRECALL_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file types.h
 * @brief Common type definitions shared across the application.
 */

#ifndef HYPERRECALL_UI_QT6
#include <raylib.h>
#else
// Define raylib-compatible types for Qt backend

typedef struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Color;

typedef struct Vector2 {
    float x;
    float y;
} Vector2;

typedef struct Rectangle {
    float x;
    float y;
    float width;
    float height;
} Rectangle;

typedef struct Font {
    int baseSize;
    int glyphCount;
    int glyphPadding;
    void *texture;
    void *recs;
    void *glyphs;
} Font;

#endif

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_TYPES_H */
