#ifndef HYPERRECALL_TYPES_H
#define HYPERRECALL_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file types.h
 * @brief Common type definitions shared across the application.
 */

// Define common types for Qt6 C++ backend

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

typedef struct Texture2D {
    unsigned int id;
    int width;
    int height;
    int mipmaps;
    int format;
} Texture2D;

typedef struct Image {
    void *data;
    int width;
    int height;
    int mipmaps;
    int format;
} Image;

typedef struct Sound {
    void *stream;
    unsigned int frameCount;
} Sound;

typedef struct Wave {
    void *data;
    unsigned int frameCount;
    unsigned int sampleRate;
    unsigned int sampleSize;
    unsigned int channels;
} Wave;

// Color constants
#define WHITE      (Color){ 255, 255, 255, 255 }
#define BLACK      (Color){ 0, 0, 0, 255 }
#define RAYWHITE   (Color){ 245, 245, 245, 255 }
#define GRAY       (Color){ 130, 130, 130, 255 }

// Pixel format constants
#define PIXELFORMAT_UNCOMPRESSED_GRAYSCALE 1

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_TYPES_H */
