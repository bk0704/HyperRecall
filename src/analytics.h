#ifndef HYPERRECALL_ANALYTICS_H
#define HYPERRECALL_ANALYTICS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file analytics.h
 * @brief Declares analytics collection, aggregation, and reporting interfaces.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "cfg.h"
#include "srs.h"

struct HrPlatformFrame;
struct SessionReviewEvent;

/** Number of rating buckets captured for review analytics. */
#define HR_ANALYTICS_RATING_BUCKETS (SRS_RESPONSE_CRAM + 1)

/** Number of samples retained when plotting recent review intervals. */
#define HR_ANALYTICS_MAX_RECENT_INTERVALS 64U

/** Maximum number of daily buckets captured for the heatmap view. */
#define HR_ANALYTICS_MAX_HEATMAP_SAMPLES 365U

/** Number of buckets used when computing retention/forgetting curves. */
#define HR_ANALYTICS_RETENTION_BUCKETS 5U

/** Tracks basic frame timing statistics for performance dashboards. */
typedef struct HrAnalyticsFrameStats {
    uint64_t frames_tracked;      /**< Total frames sampled while analytics was enabled. */
    double total_time_seconds;    /**< Wall-clock time corresponding to the sampled frames. */
    uint64_t last_frame_index;    /**< Index of the last processed frame. */
} HrAnalyticsFrameStats;

/** Aggregates review activity metrics used by multiple dashboards. */
typedef struct HrAnalyticsReviewSummary {
    size_t total_reviews;                                      /**< Total number of reviews captured. */
    size_t rating_counts[HR_ANALYTICS_RATING_BUCKETS];         /**< Distribution of responses. */
    double average_interval_minutes;                           /**< Mean interval scheduled by the SRS. */
    float recent_intervals[HR_ANALYTICS_MAX_RECENT_INTERVALS]; /**< Sliding window of recent intervals. */
    size_t recent_count;                                       /**< Active length of @p recent_intervals. */
} HrAnalyticsReviewSummary;

/** Represents a single day inside the activity heatmap. */
typedef struct HrAnalyticsHeatmapSample {
    time_t day_start_utc;    /**< Start of day (midnight UTC). */
    uint32_t total_reviews;  /**< Number of reviews completed on the day. */
    uint32_t successful_reviews; /**< Reviews marked GOOD/EASY/CRAM. */
} HrAnalyticsHeatmapSample;

/** Tracks streak statistics derived from the activity heatmap. */
typedef struct HrAnalyticsStreakMetrics {
    size_t current_streak;         /**< Ongoing streak of active days. */
    size_t longest_streak;         /**< Longest streak observed. */
    time_t current_streak_start;   /**< Day the current streak began. */
    time_t longest_streak_start;   /**< Day the longest streak began. */
} HrAnalyticsStreakMetrics;

/** Summarises a retention bucket used by forgetting-curve charts. */
typedef struct HrAnalyticsRetentionSample {
    double min_interval_days; /**< Inclusive lower bound of the bucket. */
    double max_interval_days; /**< Exclusive upper bound of the bucket. */
    double success_rate;      /**< Successful reviews divided by total reviews. */
    uint32_t total_reviews;   /**< Total number of reviews in the bucket. */
    uint32_t successful_reviews; /**< Successful reviews contributing to @p success_rate. */
} HrAnalyticsRetentionSample;

/** Aggregate view combining all analytics surfaces exposed to the UI. */
typedef struct HrAnalyticsDashboard {
    HrAnalyticsFrameStats frames;                                        /**< Frame timing metrics. */
    HrAnalyticsReviewSummary reviews;                                    /**< Review activity summary. */
    HrAnalyticsStreakMetrics streaks;                                    /**< Streak information. */
    HrAnalyticsHeatmapSample heatmap[HR_ANALYTICS_MAX_HEATMAP_SAMPLES];  /**< Heatmap day buckets. */
    size_t heatmap_count;                                                /**< Active heatmap entries. */
    HrAnalyticsRetentionSample retention[HR_ANALYTICS_RETENTION_BUCKETS];/**< Retention curve buckets. */
    size_t retention_count;                                              /**< Active retention buckets. */
} HrAnalyticsDashboard;

struct AnalyticsHandle;

/** Allocates a new analytics handle using the supplied configuration. */
struct AnalyticsHandle *analytics_create(const HrAnalyticsConfig *config);

/** Releases resources owned by the analytics handle. */
void analytics_shutdown(struct AnalyticsHandle *handle);

/** Updates the analytics subsystem with a new configuration snapshot. */
void analytics_apply_config(struct AnalyticsHandle *handle, const HrAnalyticsConfig *config);

/** Explicitly toggles analytics capture on or off. */
void analytics_set_enabled(struct AnalyticsHandle *handle, bool enabled);

/** Returns whether analytics capture is currently enabled. */
bool analytics_is_enabled(const struct AnalyticsHandle *handle);

/** Records frame timing metrics for performance analytics. */
void analytics_record_frame(struct AnalyticsHandle *handle, const struct HrPlatformFrame *frame);

/** Resets transient frame statistics accumulated during the main loop. */
void analytics_flush(struct AnalyticsHandle *handle);

/** Captures a completed review event emitted by the session manager. */
void analytics_record_review(struct AnalyticsHandle *handle, const struct SessionReviewEvent *event);

/** Convenience wrapper matching the session_manager analytics callback signature. */
void analytics_record_session_event(const struct SessionReviewEvent *event, void *user_data);

/** Returns an immutable snapshot of the aggregated analytics dashboard. */
const HrAnalyticsDashboard *analytics_dashboard(const struct AnalyticsHandle *handle);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_ANALYTICS_H */
