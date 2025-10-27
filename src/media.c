#include "media.h"

#ifndef HYPERRECALL_UI_QT6
#include <raylib.h>
#endif

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HYPERRECALL_UI_QT6
// Stub implementations for Qt6 backend
static inline double GetTime(void) { 
    return (double)time(NULL);
}
static inline const char* GetFileExtension(const char *fileName) {
    if (fileName == NULL) return "";
    const char *dot = strrchr(fileName, '.');
    return (dot == NULL) ? "" : dot;
}
static inline bool IsAudioDeviceReady(void) {
    return false;
}
static inline void InitAudioDevice(void) {
    // No-op for Qt6
}
static inline Texture2D LoadTexture(const char *fileName) {
    (void)fileName;
    return (Texture2D){0, 0, 0, 0, 0};
}
static inline Texture2D LoadTextureFromImage(Image image) {
    (void)image;
    return (Texture2D){0, 0, 0, 0, 0};
}
static inline void UnloadTexture(Texture2D texture) {
    (void)texture;
}
static inline Image LoadImage(const char *fileName) {
    (void)fileName;
    return (Image){NULL, 0, 0, 0, 0};
}
static inline Image LoadImageFromMemory(const char *fileType, const unsigned char *fileData, int dataSize) {
    (void)fileType; (void)fileData; (void)dataSize;
    return (Image){NULL, 0, 0, 0, 0};
}
static inline void UnloadImage(Image image) {
    (void)image;
}
static inline void ImageResize(Image *image, int newWidth, int newHeight) {
    (void)image; (void)newWidth; (void)newHeight;
}
static inline void ImageFormat(Image *image, int newFormat) {
    (void)image; (void)newFormat;
}
static inline Sound LoadSound(const char *fileName) {
    (void)fileName;
    return (Sound){NULL, 0};
}
static inline Sound LoadSoundFromWave(Wave wave) {
    (void)wave;
    return (Sound){NULL, 0};
}
static inline void UnloadSound(Sound sound) {
    (void)sound;
}
static inline Wave LoadWave(const char *fileName) {
    (void)fileName;
    return (Wave){NULL, 0, 0, 0, 0};
}
static inline Wave LoadWaveFromMemory(const char *fileType, const unsigned char *fileData, int dataSize) {
    (void)fileType; (void)fileData; (void)dataSize;
    return (Wave){NULL, 0, 0, 0, 0};
}
static inline void UnloadWave(Wave wave) {
    (void)wave;
}
#endif

#define HR_MEDIA_DEFAULT_TEXTURE_BUDGET (256ULL * 1024ULL * 1024ULL)
#define HR_MEDIA_DEFAULT_AUDIO_BUDGET (128ULL * 1024ULL * 1024ULL)
#define HR_MEDIA_DEFAULT_THUMBNAIL_BUDGET (64ULL * 1024ULL * 1024ULL)
#define HR_MEDIA_DEFAULT_MASK_BUDGET (32ULL * 1024ULL * 1024ULL)
#define HR_MEDIA_DEFAULT_EVICTION_GRACE 1.5

typedef struct HrMediaResource {
    bool active;
    HrMediaResourceKind kind;
    uint32_t generation;
    uint32_t ref_count;
    uint64_t byte_size;
    double last_release_time;
    HrMediaMetadata metadata;
    union {
        Texture2D texture;
        Sound sound;
        Image image;
    } payload;
} HrMediaResource;

struct HrMediaCache {
    HrMediaConfig config;
    HrMediaEventCallback callback;
    void *callback_user_data;
    HrMediaResource *entries;
    size_t entry_capacity;
    uint64_t texture_bytes;
    uint64_t audio_bytes;
    uint64_t thumbnail_bytes;
    uint64_t mask_bytes;
};

static double hr_media_now(void)
{
    return GetTime();
}

static void hr_media_strlcpy(char *dst, size_t dst_size, const char *src)
{
    if (dst == NULL || dst_size == 0U) {
        return;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    size_t length = strlen(src);
    if (length >= dst_size) {
        length = dst_size - 1U;
    }

    if (length > 0U) {
        memcpy(dst, src, length);
    }
    dst[length] = '\0';
}

static const char *hr_media_source_identifier(const HrMediaSource *source)
{
    if (source == NULL) {
        return NULL;
    }

    if (source->uuid != NULL && source->uuid[0] != '\0') {
        return source->uuid;
    }

    if (source->path != NULL && source->path[0] != '\0') {
        return source->path;
    }

    return NULL;
}

static const char *hr_media_resolve_hint(const HrMediaSource *source, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0U) {
        return "";
    }

    buffer[0] = '\0';

    if (source == NULL) {
        return buffer;
    }

    if (source->file_hint != NULL && source->file_hint[0] != '\0') {
        hr_media_strlcpy(buffer, buffer_size, source->file_hint);
        return buffer;
    }

    if (source->path != NULL && source->path[0] != '\0') {
        const char *extension = GetFileExtension(source->path);
        if (extension != NULL && extension[0] != '\0') {
            hr_media_strlcpy(buffer, buffer_size, extension);
        }
    }

    return buffer;
}

static void hr_media_emit_event(const struct HrMediaCache *cache, const HrMediaMetadata *metadata,
    HrMediaEventType type, const char *reason)
{
    if (cache == NULL || cache->callback == NULL || metadata == NULL) {
        return;
    }

    HrMediaEvent event = {
        .type = type,
        .metadata = *metadata,
        .reason = reason,
    };

    cache->callback(&event, cache->callback_user_data);
}

static HrMediaResource *hr_media_entry_from_handle(struct HrMediaCache *cache, HrMediaHandle handle)
{
    if (cache == NULL || handle.slot >= cache->entry_capacity) {
        return NULL;
    }

    HrMediaResource *entry = &cache->entries[handle.slot];
    if (!entry->active || entry->generation != handle.generation || entry->kind != handle.kind) {
        return NULL;
    }

    return entry;
}

static const HrMediaResource *hr_media_entry_from_handle_const(const struct HrMediaCache *cache,
    HrMediaHandle handle)
{
    if (cache == NULL || handle.slot >= cache->entry_capacity) {
        return NULL;
    }

    const HrMediaResource *entry = &cache->entries[handle.slot];
    if (!entry->active || entry->generation != handle.generation || entry->kind != handle.kind) {
        return NULL;
    }

    return entry;
}

static HrMediaHandle hr_media_make_handle(const struct HrMediaCache *cache,
    const HrMediaResource *entry)
{
    HrMediaHandle handle = {0U};
    if (cache == NULL || entry == NULL) {
        return handle;
    }

    handle.kind = entry->kind;
    handle.generation = entry->generation;
    handle.slot = (size_t)(entry - cache->entries);
    return handle;
}

static uint64_t *hr_media_usage_counter(struct HrMediaCache *cache, HrMediaResourceKind kind)
{
    switch (kind) {
        case HR_MEDIA_RESOURCE_TEXTURE:
            return &cache->texture_bytes;
        case HR_MEDIA_RESOURCE_AUDIO:
            return &cache->audio_bytes;
        case HR_MEDIA_RESOURCE_THUMBNAIL:
            return &cache->thumbnail_bytes;
        case HR_MEDIA_RESOURCE_OCCLUSION_MASK:
            return &cache->mask_bytes;
    }

    return &cache->texture_bytes;
}

static size_t hr_media_budget_limit(const struct HrMediaCache *cache, HrMediaResourceKind kind)
{
    switch (kind) {
        case HR_MEDIA_RESOURCE_TEXTURE:
            return cache->config.max_texture_bytes;
        case HR_MEDIA_RESOURCE_AUDIO:
            return cache->config.max_audio_bytes;
        case HR_MEDIA_RESOURCE_THUMBNAIL:
            return cache->config.max_thumbnail_bytes;
        case HR_MEDIA_RESOURCE_OCCLUSION_MASK:
            return cache->config.max_mask_bytes;
    }

    return 0U;
}

static void hr_media_prepare_metadata(const HrMediaSource *source, HrMediaResourceKind kind,
    uint64_t byte_size, double now, bool from_memory, HrMediaMetadata *metadata)
{
    if (metadata == NULL) {
        return;
    }

    memset(metadata, 0, sizeof(*metadata));
    metadata->resource_kind = kind;
    metadata->logical_type = source != NULL ? source->logical_type : HR_MEDIA_TYPE_IMAGE;
    metadata->byte_size = byte_size;
    metadata->created_time = now;
    metadata->last_access_time = now;
    metadata->from_memory = from_memory;

    if (source != NULL) {
        const char *identifier = hr_media_source_identifier(source);
        if (identifier != NULL) {
            hr_media_strlcpy(metadata->uuid, sizeof(metadata->uuid), identifier);
        }
        if (source->path != NULL) {
            hr_media_strlcpy(metadata->source_path, sizeof(metadata->source_path), source->path);
        }
        char hint[HR_MEDIA_MAX_HINT];
        const char *resolved_hint = hr_media_resolve_hint(source, hint, sizeof(hint));
        if (resolved_hint != NULL) {
            hr_media_strlcpy(metadata->file_hint, sizeof(metadata->file_hint), resolved_hint);
        }
    }
}

static HrMediaResource *hr_media_find_entry(struct HrMediaCache *cache, HrMediaResourceKind kind,
    const char *identifier)
{
    if (cache == NULL || identifier == NULL || identifier[0] == '\0') {
        return NULL;
    }

    for (size_t i = 0U; i < cache->entry_capacity; ++i) {
        HrMediaResource *entry = &cache->entries[i];
        if (!entry->active || entry->kind != kind) {
            continue;
        }
        if (strncmp(entry->metadata.uuid, identifier, sizeof(entry->metadata.uuid)) == 0) {
            return entry;
        }
    }

    return NULL;
}

static HrMediaResource *hr_media_allocate_slot(struct HrMediaCache *cache)
{
    if (cache == NULL) {
        return NULL;
    }

    for (size_t i = 0U; i < cache->entry_capacity; ++i) {
        HrMediaResource *entry = &cache->entries[i];
        if (!entry->active) {
            entry->generation += 1U;
            if (entry->generation == 0U) {
                entry->generation = 1U;
            }
            entry->ref_count = 0U;
            entry->byte_size = 0U;
            entry->last_release_time = 0.0;
            memset(&entry->metadata, 0, sizeof(entry->metadata));
            entry->active = true;
            return entry;
        }
    }

    size_t new_capacity = cache->entry_capacity > 0U ? cache->entry_capacity * 2U : 16U;
    HrMediaResource *resized = realloc(cache->entries, new_capacity * sizeof(*resized));
    if (resized == NULL) {
        return NULL;
    }

    memset(resized + cache->entry_capacity, 0,
        (new_capacity - cache->entry_capacity) * sizeof(*resized));
    cache->entries = resized;
    HrMediaResource *entry = &cache->entries[cache->entry_capacity];
    cache->entry_capacity = new_capacity;

    entry->active = true;
    entry->generation += 1U;
    if (entry->generation == 0U) {
        entry->generation = 1U;
    }
    entry->ref_count = 0U;
    entry->byte_size = 0U;
    entry->last_release_time = 0.0;
    memset(&entry->metadata, 0, sizeof(entry->metadata));

    return entry;
}

static void hr_media_detach_entry(struct HrMediaCache *cache, HrMediaResource *entry)
{
    if (cache == NULL || entry == NULL || !entry->active) {
        return;
    }

    uint64_t *usage = hr_media_usage_counter(cache, entry->kind);
    if (*usage >= entry->byte_size) {
        *usage -= entry->byte_size;
    } else {
        *usage = 0U;
    }

    switch (entry->kind) {
        case HR_MEDIA_RESOURCE_TEXTURE:
            if (entry->payload.texture.id != 0U) {
                UnloadTexture(entry->payload.texture);
            }
            break;
        case HR_MEDIA_RESOURCE_AUDIO:
            UnloadSound(entry->payload.sound);
            break;
        case HR_MEDIA_RESOURCE_THUMBNAIL:
        case HR_MEDIA_RESOURCE_OCCLUSION_MASK:
            UnloadImage(entry->payload.image);
            break;
    }

    entry->generation += 1U;
    if (entry->generation == 0U) {
        entry->generation = 1U;
    }

    entry->active = false;
    entry->ref_count = 0U;
    entry->byte_size = 0U;
    entry->last_release_time = 0.0;
    memset(&entry->metadata, 0, sizeof(entry->metadata));
    memset(&entry->payload, 0, sizeof(entry->payload));
}

static HrMediaResource *hr_media_choose_evict_candidate(struct HrMediaCache *cache,
    HrMediaResourceKind kind, double now, bool ignore_grace)
{
    if (cache == NULL) {
        return NULL;
    }

    HrMediaResource *candidate = NULL;
    double oldest_time = now;

    for (size_t i = 0U; i < cache->entry_capacity; ++i) {
        HrMediaResource *entry = &cache->entries[i];
        if (!entry->active || entry->kind != kind || entry->ref_count > 0U) {
            continue;
        }

        const double age = now - entry->metadata.last_access_time;
        if (!ignore_grace && cache->config.eviction_grace_time > 0.0 &&
            age < cache->config.eviction_grace_time) {
            continue;
        }

        if (candidate == NULL || entry->metadata.last_access_time < oldest_time) {
            candidate = entry;
            oldest_time = entry->metadata.last_access_time;
        }
    }

    return candidate;
}

static bool hr_media_ensure_budget(struct HrMediaCache *cache, HrMediaResourceKind kind,
    uint64_t needed_bytes, double now)
{
    if (cache == NULL) {
        return false;
    }

    size_t budget = hr_media_budget_limit(cache, kind);
    if (budget == 0U) {
        return true;
    }

    uint64_t *usage = hr_media_usage_counter(cache, kind);
    while (*usage + needed_bytes > budget) {
        HrMediaResource *candidate = hr_media_choose_evict_candidate(cache, kind, now, false);
        if (candidate == NULL) {
            candidate = hr_media_choose_evict_candidate(cache, kind, now, true);
        }
        if (candidate == NULL) {
            return false;
        }

        HrMediaMetadata metadata = candidate->metadata;
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_EVICTED, "budget");
        hr_media_detach_entry(cache, candidate);
    }

    return true;
}

static bool hr_media_load_image(const HrMediaSource *source, Image *out_image)
{
    if (out_image == NULL) {
        return false;
    }

    memset(out_image, 0, sizeof(*out_image));

    if (source == NULL) {
        return false;
    }

    if (source->data != NULL && source->data_size > 0U) {
        char hint[HR_MEDIA_MAX_HINT];
        const char *file_type = hr_media_resolve_hint(source, hint, sizeof(hint));
        if (file_type == NULL || file_type[0] == '\0') {
            file_type = ".png";
        }
        *out_image = LoadImageFromMemory(file_type, source->data, (int)source->data_size);
    } else if (source->path != NULL && source->path[0] != '\0') {
        *out_image = LoadImage(source->path);
    }

    return out_image->data != NULL;
}

static bool hr_media_resize_thumbnail(Image *image, int max_dimension)
{
    if (image == NULL || image->data == NULL || max_dimension <= 0) {
        return false;
    }

    int width = image->width;
    int height = image->height;
    int largest = width > height ? width : height;
    if (largest <= max_dimension) {
        return true;
    }

    float scale = (float)max_dimension / (float)largest;
    int new_width = (int)fmaxf(1.0f, floorf((float)width * scale + 0.5f));
    int new_height = (int)fmaxf(1.0f, floorf((float)height * scale + 0.5f));

    ImageResize(image, new_width, new_height);
    return true;
}

static bool hr_media_load_wave(const HrMediaSource *source, Wave *out_wave)
{
    if (out_wave == NULL) {
        return false;
    }

    memset(out_wave, 0, sizeof(*out_wave));

    if (source == NULL) {
        return false;
    }

    if (source->data != NULL && source->data_size > 0U) {
        char hint[HR_MEDIA_MAX_HINT];
        const char *file_type = hr_media_resolve_hint(source, hint, sizeof(hint));
        if (file_type == NULL || file_type[0] == '\0') {
            file_type = ".wav";
        }
        *out_wave = LoadWaveFromMemory(file_type, source->data, (int)source->data_size);
    } else if (source->path != NULL && source->path[0] != '\0') {
        *out_wave = LoadWave(source->path);
    }

    return out_wave->data != NULL;
}

static bool hr_media_populate_texture(struct HrMediaCache *cache, HrMediaResource *entry,
    const HrMediaSource *source, Image *image, double now)
{
    if (cache == NULL || entry == NULL || source == NULL || image == NULL) {
        return false;
    }

    Texture2D texture = LoadTextureFromImage(*image);
    if (texture.id == 0U) {
        return false;
    }

    const uint64_t byte_size = (uint64_t)image->width * (uint64_t)image->height * 4ULL;
    if (!hr_media_ensure_budget(cache, HR_MEDIA_RESOURCE_TEXTURE, byte_size, now)) {
        UnloadTexture(texture);
        return false;
    }

    entry->kind = HR_MEDIA_RESOURCE_TEXTURE;
    entry->byte_size = byte_size;
    entry->payload.texture = texture;
    hr_media_prepare_metadata(source, entry->kind, byte_size, now, source->data != NULL,
        &entry->metadata);
    entry->metadata.access_count = 1U;
    uint64_t *usage = hr_media_usage_counter(cache, entry->kind);
    *usage += byte_size;

    return true;
}

static bool hr_media_populate_thumbnail(struct HrMediaCache *cache, HrMediaResource *entry,
    const HrMediaSource *source, Image *image, int max_dimension, double now)
{
    if (cache == NULL || entry == NULL || source == NULL || image == NULL) {
        return false;
    }

    if (max_dimension > 0) {
        hr_media_resize_thumbnail(image, max_dimension);
    }

    const uint64_t byte_size = (uint64_t)image->width * (uint64_t)image->height * 4ULL;
    if (!hr_media_ensure_budget(cache, HR_MEDIA_RESOURCE_THUMBNAIL, byte_size, now)) {
        UnloadImage(*image);
        return false;
    }

    entry->kind = HR_MEDIA_RESOURCE_THUMBNAIL;
    entry->byte_size = byte_size;
    entry->payload.image = *image;
    hr_media_prepare_metadata(source, entry->kind, byte_size, now, source->data != NULL,
        &entry->metadata);
    entry->metadata.access_count = 1U;
    uint64_t *usage = hr_media_usage_counter(cache, entry->kind);
    *usage += byte_size;

    return true;
}

static bool hr_media_populate_mask(struct HrMediaCache *cache, HrMediaResource *entry,
    const HrMediaSource *source, Image *image, double now)
{
    if (cache == NULL || entry == NULL || source == NULL || image == NULL) {
        return false;
    }

    ImageFormat(image, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);

    const uint64_t byte_size = (uint64_t)image->width * (uint64_t)image->height;
    if (!hr_media_ensure_budget(cache, HR_MEDIA_RESOURCE_OCCLUSION_MASK, byte_size, now)) {
        UnloadImage(*image);
        return false;
    }

    entry->kind = HR_MEDIA_RESOURCE_OCCLUSION_MASK;
    entry->byte_size = byte_size;
    entry->payload.image = *image;
    hr_media_prepare_metadata(source, entry->kind, byte_size, now, source->data != NULL,
        &entry->metadata);
    entry->metadata.access_count = 1U;
    uint64_t *usage = hr_media_usage_counter(cache, entry->kind);
    *usage += byte_size;

    return true;
}

static bool hr_media_populate_sound(struct HrMediaCache *cache, HrMediaResource *entry,
    const HrMediaSource *source, Wave *wave, double now)
{
    if (cache == NULL || entry == NULL || source == NULL || wave == NULL) {
        return false;
    }

    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }

    if (!IsAudioDeviceReady()) {
        return false;
    }

    Sound sound = LoadSoundFromWave(*wave);
    if (sound.frameCount == 0U) {
        return false;
    }

    const uint64_t byte_size = (uint64_t)(wave->frameCount * wave->channels * wave->sampleSize / 8);
    if (!hr_media_ensure_budget(cache, HR_MEDIA_RESOURCE_AUDIO, byte_size, now)) {
        UnloadSound(sound);
        return false;
    }

    entry->kind = HR_MEDIA_RESOURCE_AUDIO;
    entry->byte_size = byte_size;
    entry->payload.sound = sound;
    hr_media_prepare_metadata(source, entry->kind, byte_size, now, source->data != NULL,
        &entry->metadata);
    entry->metadata.access_count = 1U;
    uint64_t *usage = hr_media_usage_counter(cache, entry->kind);
    *usage += byte_size;

    return true;
}

static bool hr_media_validate_identifier(struct HrMediaCache *cache, const HrMediaSource *source,
    HrMediaResourceKind kind)
{
    (void)kind;
    if (cache == NULL || source == NULL) {
        return false;
    }

    const char *identifier = hr_media_source_identifier(source);
    if (identifier == NULL || identifier[0] == '\0') {
        HrMediaMetadata metadata;
        hr_media_prepare_metadata(source, kind, 0U, hr_media_now(), source->data != NULL, &metadata);
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_FAILED, "missing_identifier");
        return false;
    }

    return true;
}

struct HrMediaCache *media_cache_create(const HrMediaConfig *config)
{
    HrMediaConfig defaults = {
        .max_texture_bytes = HR_MEDIA_DEFAULT_TEXTURE_BUDGET,
        .max_audio_bytes = HR_MEDIA_DEFAULT_AUDIO_BUDGET,
        .max_thumbnail_bytes = HR_MEDIA_DEFAULT_THUMBNAIL_BUDGET,
        .max_mask_bytes = HR_MEDIA_DEFAULT_MASK_BUDGET,
        .eviction_grace_time = HR_MEDIA_DEFAULT_EVICTION_GRACE,
    };

    struct HrMediaCache *cache = calloc(1U, sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }

    if (config != NULL) {
        defaults = *config;
        if (defaults.eviction_grace_time < 0.0) {
            defaults.eviction_grace_time = 0.0;
        }
    }

    cache->config = defaults;

    return cache;
}

void media_cache_destroy(struct HrMediaCache *cache)
{
    if (cache == NULL) {
        return;
    }

    if (cache->entries != NULL) {
        for (size_t i = 0U; i < cache->entry_capacity; ++i) {
            HrMediaResource *entry = &cache->entries[i];
            if (entry->active) {
                HrMediaMetadata metadata = entry->metadata;
                hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_EVICTED, "shutdown");
                hr_media_detach_entry(cache, entry);
            }
        }
        free(cache->entries);
        cache->entries = NULL;
    }

    free(cache);
}

void media_cache_set_event_callback(struct HrMediaCache *cache, HrMediaEventCallback callback,
    void *user_data)
{
    if (cache == NULL) {
        return;
    }

    cache->callback = callback;
    cache->callback_user_data = user_data;
}

bool media_cache_acquire_texture(struct HrMediaCache *cache, const HrMediaSource *source,
    HrMediaTextureView *out_view)
{
    if (!hr_media_validate_identifier(cache, source, HR_MEDIA_RESOURCE_TEXTURE)) {
        return false;
    }

    double now = hr_media_now();
    const char *identifier = hr_media_source_identifier(source);
    HrMediaResource *entry = hr_media_find_entry(cache, HR_MEDIA_RESOURCE_TEXTURE, identifier);

    if (entry != NULL) {
        entry->ref_count += 1U;
        entry->metadata.access_count += 1U;
        entry->metadata.last_access_time = now;
        if (out_view != NULL) {
            out_view->handle = hr_media_make_handle(cache, entry);
            out_view->texture = &entry->payload.texture;
            out_view->metadata = &entry->metadata;
        }
        return true;
    }

    Image image;
    if (!hr_media_load_image(source, &image)) {
        HrMediaMetadata metadata;
        hr_media_prepare_metadata(source, HR_MEDIA_RESOURCE_TEXTURE, 0U, now, source->data != NULL,
            &metadata);
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_FAILED, "load_image");
        return false;
    }

    HrMediaResource *slot = hr_media_allocate_slot(cache);
    if (slot == NULL) {
        HrMediaMetadata metadata;
        hr_media_prepare_metadata(source, HR_MEDIA_RESOURCE_TEXTURE, 0U, now, source->data != NULL,
            &metadata);
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_FAILED, "allocate_slot");
        UnloadImage(image);
        return false;
    }

    if (!hr_media_populate_texture(cache, slot, source, &image, now)) {
        HrMediaMetadata metadata;
        hr_media_prepare_metadata(source, HR_MEDIA_RESOURCE_TEXTURE, 0U, now, source->data != NULL,
            &metadata);
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_FAILED, "populate_texture");
        UnloadImage(image);
        slot->active = false;
        slot->ref_count = 0U;
        slot->byte_size = 0U;
        memset(&slot->metadata, 0, sizeof(slot->metadata));
        return false;
    }

    UnloadImage(image);

    slot->ref_count = 1U;
    slot->metadata.last_access_time = now;

    HrMediaMetadata metadata = slot->metadata;
    hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_LOADED, "loaded");

    if (out_view != NULL) {
        out_view->handle = hr_media_make_handle(cache, slot);
        out_view->texture = &slot->payload.texture;
        out_view->metadata = &slot->metadata;
    }

    return true;
}

bool media_cache_acquire_audio(struct HrMediaCache *cache, const HrMediaSource *source,
    HrMediaSoundView *out_view)
{
    if (!hr_media_validate_identifier(cache, source, HR_MEDIA_RESOURCE_AUDIO)) {
        return false;
    }

    double now = hr_media_now();
    const char *identifier = hr_media_source_identifier(source);
    HrMediaResource *entry = hr_media_find_entry(cache, HR_MEDIA_RESOURCE_AUDIO, identifier);

    if (entry != NULL) {
        entry->ref_count += 1U;
        entry->metadata.access_count += 1U;
        entry->metadata.last_access_time = now;
        if (out_view != NULL) {
            out_view->handle = hr_media_make_handle(cache, entry);
            out_view->sound = &entry->payload.sound;
            out_view->metadata = &entry->metadata;
        }
        return true;
    }

    Wave wave;
    if (!hr_media_load_wave(source, &wave)) {
        HrMediaMetadata metadata;
        hr_media_prepare_metadata(source, HR_MEDIA_RESOURCE_AUDIO, 0U, now, source->data != NULL,
            &metadata);
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_FAILED, "load_wave");
        return false;
    }

    HrMediaResource *slot = hr_media_allocate_slot(cache);
    if (slot == NULL) {
        HrMediaMetadata metadata;
        hr_media_prepare_metadata(source, HR_MEDIA_RESOURCE_AUDIO, 0U, now, source->data != NULL,
            &metadata);
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_FAILED, "allocate_slot");
        UnloadWave(wave);
        return false;
    }

    if (!hr_media_populate_sound(cache, slot, source, &wave, now)) {
        HrMediaMetadata metadata;
        hr_media_prepare_metadata(source, HR_MEDIA_RESOURCE_AUDIO, 0U, now, source->data != NULL,
            &metadata);
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_FAILED, "populate_sound");
        UnloadWave(wave);
        slot->active = false;
        slot->ref_count = 0U;
        slot->byte_size = 0U;
        memset(&slot->metadata, 0, sizeof(slot->metadata));
        return false;
    }

    UnloadWave(wave);

    slot->ref_count = 1U;
    slot->metadata.last_access_time = now;

    HrMediaMetadata metadata = slot->metadata;
    hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_LOADED, "loaded");

    if (out_view != NULL) {
        out_view->handle = hr_media_make_handle(cache, slot);
        out_view->sound = &slot->payload.sound;
        out_view->metadata = &slot->metadata;
    }

    return true;
}

bool media_cache_acquire_thumbnail(struct HrMediaCache *cache, const HrMediaSource *source,
    int max_dimension, HrMediaImageView *out_view)
{
    if (!hr_media_validate_identifier(cache, source, HR_MEDIA_RESOURCE_THUMBNAIL)) {
        return false;
    }

    double now = hr_media_now();
    const char *identifier = hr_media_source_identifier(source);
    HrMediaResource *entry = hr_media_find_entry(cache, HR_MEDIA_RESOURCE_THUMBNAIL, identifier);

    if (entry != NULL) {
        entry->ref_count += 1U;
        entry->metadata.access_count += 1U;
        entry->metadata.last_access_time = now;
        if (out_view != NULL) {
            out_view->handle = hr_media_make_handle(cache, entry);
            out_view->image = &entry->payload.image;
            out_view->metadata = &entry->metadata;
        }
        return true;
    }

    Image image;
    if (!hr_media_load_image(source, &image)) {
        HrMediaMetadata metadata;
        hr_media_prepare_metadata(source, HR_MEDIA_RESOURCE_THUMBNAIL, 0U, now,
            source->data != NULL, &metadata);
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_FAILED, "load_image");
        return false;
    }

    HrMediaResource *slot = hr_media_allocate_slot(cache);
    if (slot == NULL) {
        HrMediaMetadata metadata;
        hr_media_prepare_metadata(source, HR_MEDIA_RESOURCE_THUMBNAIL, 0U, now,
            source->data != NULL, &metadata);
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_FAILED, "allocate_slot");
        UnloadImage(image);
        return false;
    }

    if (!hr_media_populate_thumbnail(cache, slot, source, &image, max_dimension, now)) {
        HrMediaMetadata metadata;
        hr_media_prepare_metadata(source, HR_MEDIA_RESOURCE_THUMBNAIL, 0U, now,
            source->data != NULL, &metadata);
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_FAILED, "populate_thumbnail");
        if (image.data != NULL) {
            UnloadImage(image);
        }
        slot->active = false;
        slot->ref_count = 0U;
        slot->byte_size = 0U;
        memset(&slot->metadata, 0, sizeof(slot->metadata));
        return false;
    }

    slot->ref_count = 1U;
    slot->metadata.last_access_time = now;

    HrMediaMetadata metadata = slot->metadata;
    hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_LOADED, "loaded");

    if (out_view != NULL) {
        out_view->handle = hr_media_make_handle(cache, slot);
        out_view->image = &slot->payload.image;
        out_view->metadata = &slot->metadata;
    }

    return true;
}

bool media_cache_acquire_occlusion_mask(struct HrMediaCache *cache, const HrMediaSource *source,
    HrMediaImageView *out_view)
{
    if (!hr_media_validate_identifier(cache, source, HR_MEDIA_RESOURCE_OCCLUSION_MASK)) {
        return false;
    }

    double now = hr_media_now();
    const char *identifier = hr_media_source_identifier(source);
    HrMediaResource *entry = hr_media_find_entry(cache, HR_MEDIA_RESOURCE_OCCLUSION_MASK, identifier);

    if (entry != NULL) {
        entry->ref_count += 1U;
        entry->metadata.access_count += 1U;
        entry->metadata.last_access_time = now;
        if (out_view != NULL) {
            out_view->handle = hr_media_make_handle(cache, entry);
            out_view->image = &entry->payload.image;
            out_view->metadata = &entry->metadata;
        }
        return true;
    }

    Image image;
    if (!hr_media_load_image(source, &image)) {
        HrMediaMetadata metadata;
        hr_media_prepare_metadata(source, HR_MEDIA_RESOURCE_OCCLUSION_MASK, 0U, now,
            source->data != NULL, &metadata);
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_FAILED, "load_image");
        return false;
    }

    HrMediaResource *slot = hr_media_allocate_slot(cache);
    if (slot == NULL) {
        HrMediaMetadata metadata;
        hr_media_prepare_metadata(source, HR_MEDIA_RESOURCE_OCCLUSION_MASK, 0U, now,
            source->data != NULL, &metadata);
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_FAILED, "allocate_slot");
        UnloadImage(image);
        return false;
    }

    if (!hr_media_populate_mask(cache, slot, source, &image, now)) {
        HrMediaMetadata metadata;
        hr_media_prepare_metadata(source, HR_MEDIA_RESOURCE_OCCLUSION_MASK, 0U, now,
            source->data != NULL, &metadata);
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_FAILED, "populate_mask");
        if (image.data != NULL) {
            UnloadImage(image);
        }
        slot->active = false;
        slot->ref_count = 0U;
        slot->byte_size = 0U;
        memset(&slot->metadata, 0, sizeof(slot->metadata));
        return false;
    }

    slot->ref_count = 1U;
    slot->metadata.last_access_time = now;

    HrMediaMetadata metadata = slot->metadata;
    hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_LOADED, "loaded");

    if (out_view != NULL) {
        out_view->handle = hr_media_make_handle(cache, slot);
        out_view->image = &slot->payload.image;
        out_view->metadata = &slot->metadata;
    }

    return true;
}

void media_cache_release(struct HrMediaCache *cache, HrMediaHandle handle)
{
    HrMediaResource *entry = hr_media_entry_from_handle(cache, handle);
    if (entry == NULL) {
        return;
    }

    if (entry->ref_count > 0U) {
        entry->ref_count -= 1U;
    }

    double now = hr_media_now();
    entry->metadata.last_access_time = now;

    if (entry->ref_count == 0U) {
        entry->last_release_time = now;
        HrMediaMetadata metadata = entry->metadata;
        hr_media_emit_event(cache, &metadata, HR_MEDIA_EVENT_RELEASED, "release");
    }
}

const HrMediaMetadata *media_cache_get_metadata(const struct HrMediaCache *cache,
    HrMediaHandle handle)
{
    const HrMediaResource *entry = hr_media_entry_from_handle_const(cache, handle);
    if (entry == NULL) {
        return NULL;
    }

    return &entry->metadata;
}

bool media_cache_handle_valid(const struct HrMediaCache *cache, HrMediaHandle handle)
{
    return hr_media_entry_from_handle_const(cache, handle) != NULL;
}

void media_cache_get_stats(const struct HrMediaCache *cache, HrMediaCacheStats *out_stats)
{
    if (out_stats == NULL) {
        return;
    }

    memset(out_stats, 0, sizeof(*out_stats));

    if (cache == NULL || cache->entries == NULL) {
        return;
    }

    for (size_t i = 0U; i < cache->entry_capacity; ++i) {
        const HrMediaResource *entry = &cache->entries[i];
        if (!entry->active) {
            continue;
        }

        switch (entry->kind) {
            case HR_MEDIA_RESOURCE_TEXTURE:
                out_stats->texture_count += 1U;
                out_stats->texture_bytes += entry->byte_size;
                break;
            case HR_MEDIA_RESOURCE_AUDIO:
                out_stats->audio_count += 1U;
                out_stats->audio_bytes += entry->byte_size;
                break;
            case HR_MEDIA_RESOURCE_THUMBNAIL:
                out_stats->thumbnail_count += 1U;
                out_stats->thumbnail_bytes += entry->byte_size;
                break;
            case HR_MEDIA_RESOURCE_OCCLUSION_MASK:
                out_stats->mask_count += 1U;
                out_stats->mask_bytes += entry->byte_size;
                break;
        }
    }
}

void media_cache_enumerate(const struct HrMediaCache *cache, HrMediaMetadataCallback callback,
    void *user_data)
{
    if (cache == NULL || callback == NULL || cache->entries == NULL) {
        return;
    }

    for (size_t i = 0U; i < cache->entry_capacity; ++i) {
        const HrMediaResource *entry = &cache->entries[i];
        if (!entry->active) {
            continue;
        }
        callback(&entry->metadata, user_data);
    }
}
