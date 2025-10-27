#ifndef HYPERRECALL_MEDIA_H
#define HYPERRECALL_MEDIA_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file media.h
 * @brief Media cache and cross-platform resource loading helpers.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"
#include "model.h"

#ifndef HYPERRECALL_UI_QT6
#include <raylib.h>
#endif

/** Maximum number of characters persisted for media UUIDs. */
#define HR_MEDIA_MAX_UUID 64U
/** Maximum number of characters persisted for source paths. */
#define HR_MEDIA_MAX_PATH 260U
/** Maximum number of characters persisted for MIME/file hints. */
#define HR_MEDIA_MAX_HINT 32U

/** Represents the underlying runtime resource managed by the media cache. */
typedef enum HrMediaResourceKind {
    HR_MEDIA_RESOURCE_TEXTURE = 0,
    HR_MEDIA_RESOURCE_AUDIO,
    HR_MEDIA_RESOURCE_THUMBNAIL,
    HR_MEDIA_RESOURCE_OCCLUSION_MASK,
} HrMediaResourceKind;

/** Configures cache sizes and runtime loading behaviour. */
typedef struct HrMediaConfig {
    size_t max_texture_bytes;    /**< Maximum combined GPU budget for textures. */
    size_t max_audio_bytes;      /**< Maximum combined audio memory budget. */
    size_t max_thumbnail_bytes;  /**< Maximum combined thumbnail memory budget. */
    size_t max_mask_bytes;       /**< Maximum combined occlusion mask budget. */
    double eviction_grace_time;  /**< Minimum time (in seconds) before unused entries are evicted. */
} HrMediaConfig;

/** Identifies a source to load media from either disk or memory. */
typedef struct HrMediaSource {
    const char *uuid;        /**< Stable identifier for caching and analytics. */
    const char *path;        /**< Optional filesystem path. */
    const unsigned char *data; /**< Optional in-memory payload. */
    size_t data_size;        /**< Size of @p data when provided. */
    const char *file_hint;   /**< Optional file type hint (e.g. ".png"). */
    HrMediaType logical_type;/**< Logical type used for analytics/export. */
} HrMediaSource;

/** Metadata tracked for analytics and export manifests. */
typedef struct HrMediaMetadata {
    char uuid[HR_MEDIA_MAX_UUID];        /**< Stable identifier. */
    HrMediaType logical_type;            /**< Logical card media type. */
    HrMediaResourceKind resource_kind;   /**< Runtime resource type. */
    char source_path[HR_MEDIA_MAX_PATH]; /**< Normalised source path, if any. */
    char file_hint[HR_MEDIA_MAX_HINT];   /**< MIME/file hint for exporters. */
    uint64_t byte_size;                  /**< Size of the resource payload in bytes. */
    uint64_t access_count;               /**< Number of times the resource was acquired. */
    double created_time;                 /**< Time when the resource was first loaded. */
    double last_access_time;             /**< Time when the resource was last accessed. */
    bool from_memory;                    /**< True when loaded from an in-memory buffer. */
} HrMediaMetadata;

/** Event types emitted to analytics/import pipelines. */
typedef enum HrMediaEventType {
    HR_MEDIA_EVENT_LOADED = 0, /**< Resource was loaded and available. */
    HR_MEDIA_EVENT_RELEASED,   /**< Resource was released by the caller. */
    HR_MEDIA_EVENT_EVICTED,    /**< Resource was evicted to reclaim memory. */
    HR_MEDIA_EVENT_FAILED,     /**< Resource failed to load. */
} HrMediaEventType;

/** Event payload describing cache activity. */
typedef struct HrMediaEvent {
    HrMediaEventType type;     /**< Event classification. */
    HrMediaMetadata metadata;  /**< Snapshot of the resource metadata. */
    const char *reason;        /**< Optional human readable reason text. */
} HrMediaEvent;

/** Callback invoked whenever a media cache event occurs. */
typedef void (*HrMediaEventCallback)(const HrMediaEvent *event, void *user_data);

struct HrMediaCache;

/** Handle referencing an entry in the media cache. */
typedef struct HrMediaHandle {
    HrMediaResourceKind kind; /**< Resource kind to guard against misuse. */
    size_t slot;              /**< Slot index inside the cache. */
    uint32_t generation;      /**< Generation counter used to validate stale handles. */
} HrMediaHandle;

/** View returned to callers when a texture is acquired. */
typedef struct HrMediaTextureView {
    HrMediaHandle handle;            /**< Cache handle to release when finished. */
    const Texture2D *texture;        /**< Pointer to the GPU texture. */
    const HrMediaMetadata *metadata; /**< Associated metadata snapshot. */
} HrMediaTextureView;

/** View returned to callers when an audio clip is acquired. */
typedef struct HrMediaSoundView {
    HrMediaHandle handle;            /**< Cache handle to release when finished. */
    const Sound *sound;              /**< Pointer to the loaded sound. */
    const HrMediaMetadata *metadata; /**< Associated metadata snapshot. */
} HrMediaSoundView;

/** View returned to callers when an image-based payload is acquired. */
typedef struct HrMediaImageView {
    HrMediaHandle handle;            /**< Cache handle to release when finished. */
    const Image *image;              /**< Pointer to the CPU image data. */
    const HrMediaMetadata *metadata; /**< Associated metadata snapshot. */
} HrMediaImageView;

/**
 * @brief Runtime statistics reported for analytics overlays and debugging.
 */
typedef struct HrMediaCacheStats {
    size_t texture_count;
    size_t audio_count;
    size_t thumbnail_count;
    size_t mask_count;
    uint64_t texture_bytes;
    uint64_t audio_bytes;
    uint64_t thumbnail_bytes;
    uint64_t mask_bytes;
} HrMediaCacheStats;

/** Callback invoked while enumerating metadata entries. */
typedef void (*HrMediaMetadataCallback)(const HrMediaMetadata *metadata, void *user_data);

struct HrMediaCache *media_cache_create(const HrMediaConfig *config);

void media_cache_destroy(struct HrMediaCache *cache);

void media_cache_set_event_callback(struct HrMediaCache *cache, HrMediaEventCallback callback,
    void *user_data);

bool media_cache_acquire_texture(struct HrMediaCache *cache, const HrMediaSource *source,
    HrMediaTextureView *out_view);

bool media_cache_acquire_audio(struct HrMediaCache *cache, const HrMediaSource *source,
    HrMediaSoundView *out_view);

bool media_cache_acquire_thumbnail(struct HrMediaCache *cache, const HrMediaSource *source,
    int max_dimension, HrMediaImageView *out_view);

bool media_cache_acquire_occlusion_mask(struct HrMediaCache *cache, const HrMediaSource *source,
    HrMediaImageView *out_view);

void media_cache_release(struct HrMediaCache *cache, HrMediaHandle handle);

const HrMediaMetadata *media_cache_get_metadata(const struct HrMediaCache *cache,
    HrMediaHandle handle);

bool media_cache_handle_valid(const struct HrMediaCache *cache, HrMediaHandle handle);

void media_cache_get_stats(const struct HrMediaCache *cache, HrMediaCacheStats *out_stats);

void media_cache_enumerate(const struct HrMediaCache *cache, HrMediaMetadataCallback callback,
    void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_MEDIA_H */
