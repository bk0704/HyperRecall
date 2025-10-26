#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "analytics.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "platform.h"
#include "sessions.h"

#ifndef HR_ANALYTICS_SECONDS_PER_DAY
#define HR_ANALYTICS_SECONDS_PER_DAY 86400
#endif

typedef struct HrAnalyticsRetentionBucketSpec {
    double min_days;
    double max_days;
} HrAnalyticsRetentionBucketSpec;

static const HrAnalyticsRetentionBucketSpec kRetentionBuckets[HR_ANALYTICS_RETENTION_BUCKETS] = {
    {0.0, 1.0},
    {1.0, 3.0},
    {3.0, 7.0},
    {7.0, 30.0},
    {30.0, DBL_MAX},
};

struct AnalyticsHandle {
    bool enabled;
    HrAnalyticsDashboard dashboard;
    double interval_sum_minutes;
    time_t last_activity_day;
};

static void analytics_reset(struct AnalyticsHandle *handle)
{
    if (handle == NULL) {
        return;
    }

    memset(&handle->dashboard, 0, sizeof(handle->dashboard));
    handle->dashboard.retention_count = HR_ANALYTICS_RETENTION_BUCKETS;
    for (size_t i = 0; i < HR_ANALYTICS_RETENTION_BUCKETS; ++i) {
        handle->dashboard.retention[i].min_interval_days = kRetentionBuckets[i].min_days;
        handle->dashboard.retention[i].max_interval_days = kRetentionBuckets[i].max_days;
    }
    handle->interval_sum_minutes = 0.0;
    handle->last_activity_day = 0;
}

static time_t truncate_to_day(time_t timestamp)
{
    if (timestamp <= 0) {
        return 0;
    }

    struct tm components;
#if defined(_WIN32)
    if (gmtime_s(&components, &timestamp) != 0) {
        return timestamp;
    }
#else
    if (gmtime_r(&timestamp, &components) == NULL) {
        return timestamp;
    }
#endif

    long seconds = (long)components.tm_hour * 3600L + (long)components.tm_min * 60L + (long)components.tm_sec;
    return timestamp - seconds;
}

static HrAnalyticsHeatmapSample *ensure_heatmap_sample(struct AnalyticsHandle *handle, time_t day_start)
{
    if (handle == NULL || day_start <= 0) {
        return NULL;
    }

    HrAnalyticsDashboard *dashboard = &handle->dashboard;

    for (size_t i = 0; i < dashboard->heatmap_count; ++i) {
        if (dashboard->heatmap[i].day_start_utc == day_start) {
            return &dashboard->heatmap[i];
        }
    }

    if (dashboard->heatmap_count > 0) {
        HrAnalyticsHeatmapSample *latest = &dashboard->heatmap[dashboard->heatmap_count - 1U];
        if (day_start == latest->day_start_utc) {
            return latest;
        }
        if (day_start < latest->day_start_utc) {
            if (dashboard->heatmap_count == HR_ANALYTICS_MAX_HEATMAP_SAMPLES) {
                return NULL;
            }
            size_t insert_index = dashboard->heatmap_count;
            while (insert_index > 0U && dashboard->heatmap[insert_index - 1U].day_start_utc > day_start) {
                insert_index--;
            }
            if (insert_index < dashboard->heatmap_count) {
                memmove(&dashboard->heatmap[insert_index + 1U],
                        &dashboard->heatmap[insert_index],
                        (dashboard->heatmap_count - insert_index) * sizeof(HrAnalyticsHeatmapSample));
            }
            HrAnalyticsHeatmapSample *sample = &dashboard->heatmap[insert_index];
            sample->day_start_utc = day_start;
            sample->total_reviews = 0U;
            sample->successful_reviews = 0U;
            dashboard->heatmap_count++;
            return sample;
        }
    }

    if (dashboard->heatmap_count == HR_ANALYTICS_MAX_HEATMAP_SAMPLES) {
        memmove(&dashboard->heatmap[0],
                &dashboard->heatmap[1],
                (HR_ANALYTICS_MAX_HEATMAP_SAMPLES - 1U) * sizeof(HrAnalyticsHeatmapSample));
        dashboard->heatmap_count--;
    }

    HrAnalyticsHeatmapSample *sample = &dashboard->heatmap[dashboard->heatmap_count++];
    sample->day_start_utc = day_start;
    sample->total_reviews = 0U;
    sample->successful_reviews = 0U;
    return sample;
}

static void update_streaks(struct AnalyticsHandle *handle, time_t day_start)
{
    if (handle == NULL || day_start <= 0) {
        return;
    }

    HrAnalyticsStreakMetrics *streaks = &handle->dashboard.streaks;

    if (streaks->current_streak == 0U) {
        streaks->current_streak = 1U;
        streaks->longest_streak = 1U;
        streaks->current_streak_start = day_start;
        streaks->longest_streak_start = day_start;
        handle->last_activity_day = day_start;
        return;
    }

    if (handle->last_activity_day == day_start) {
        return;
    }

    if (handle->last_activity_day > 0 && day_start < handle->last_activity_day) {
        return;
    }

    size_t gap = (size_t)((day_start - handle->last_activity_day) / HR_ANALYTICS_SECONDS_PER_DAY);
    if (gap == 0U) {
        handle->last_activity_day = day_start;
        return;
    }

    if (gap == 1U) {
        streaks->current_streak++;
    } else {
        streaks->current_streak = 1U;
        streaks->current_streak_start = day_start;
    }

    handle->last_activity_day = day_start;

    if (streaks->current_streak > streaks->longest_streak) {
        streaks->longest_streak = streaks->current_streak;
        streaks->longest_streak_start = streaks->current_streak_start;
    }
}

static size_t retention_bucket_index(double previous_interval_days)
{
    if (previous_interval_days < 0.0) {
        previous_interval_days = 0.0;
    }

    for (size_t i = 0; i < HR_ANALYTICS_RETENTION_BUCKETS; ++i) {
        double min_days = kRetentionBuckets[i].min_days;
        double max_days = kRetentionBuckets[i].max_days;
        if (previous_interval_days >= min_days && (previous_interval_days < max_days ||
                                                   i == HR_ANALYTICS_RETENTION_BUCKETS - 1U)) {
            return i;
        }
    }

    return HR_ANALYTICS_RETENTION_BUCKETS - 1U;
}

static void update_retention(struct AnalyticsHandle *handle, double previous_interval_days, bool success)
{
    if (handle == NULL) {
        return;
    }

    size_t bucket = retention_bucket_index(previous_interval_days);
    HrAnalyticsRetentionSample *sample = &handle->dashboard.retention[bucket];
    sample->total_reviews++;
    if (success) {
        sample->successful_reviews++;
    }
    if (sample->total_reviews > 0U) {
        sample->success_rate = (double)sample->successful_reviews / (double)sample->total_reviews;
    } else {
        sample->success_rate = 0.0;
    }
}

struct AnalyticsHandle *analytics_create(const HrAnalyticsConfig *config)
{
    struct AnalyticsHandle *handle = (struct AnalyticsHandle *)calloc(1U, sizeof(struct AnalyticsHandle));
    if (handle == NULL) {
        return NULL;
    }

    handle->enabled = config != NULL ? config->enabled : true;
    analytics_reset(handle);
    return handle;
}

void analytics_shutdown(struct AnalyticsHandle *handle)
{
    if (handle == NULL) {
        return;
    }

    analytics_reset(handle);
    free(handle);
}

void analytics_apply_config(struct AnalyticsHandle *handle, const HrAnalyticsConfig *config)
{
    if (handle == NULL) {
        return;
    }

    bool enabled = config != NULL ? config->enabled : true;
    analytics_set_enabled(handle, enabled);
}

void analytics_set_enabled(struct AnalyticsHandle *handle, bool enabled)
{
    if (handle == NULL) {
        return;
    }

    if (handle->enabled == enabled) {
        return;
    }

    handle->enabled = enabled;
    analytics_reset(handle);
}

bool analytics_is_enabled(const struct AnalyticsHandle *handle)
{
    return handle != NULL && handle->enabled;
}

void analytics_record_frame(struct AnalyticsHandle *handle, const struct HrPlatformFrame *frame)
{
    if (handle == NULL || frame == NULL || !handle->enabled) {
        return;
    }

    HrAnalyticsFrameStats *frames = &handle->dashboard.frames;
    frames->frames_tracked++;
    frames->total_time_seconds += frame->delta_time;
    frames->last_frame_index = frame->index;
}

void analytics_flush(struct AnalyticsHandle *handle)
{
    if (handle == NULL) {
        return;
    }

    handle->dashboard.frames.frames_tracked = 0U;
    handle->dashboard.frames.total_time_seconds = 0.0;
    handle->dashboard.frames.last_frame_index = 0U;
}

void analytics_record_review(struct AnalyticsHandle *handle, const struct SessionReviewEvent *event)
{
    if (handle == NULL || event == NULL || !handle->enabled) {
        return;
    }

    HrAnalyticsReviewSummary *reviews = &handle->dashboard.reviews;

    reviews->total_reviews++;

    SRSReviewRating rating = event->result.rating;
    if (rating >= SRS_RESPONSE_FAIL && rating <= SRS_RESPONSE_CRAM) {
        reviews->rating_counts[rating]++;
    }

    float interval_minutes = (float)event->result.interval_minutes;
    if (interval_minutes < 0.0f || isnan(interval_minutes)) {
        interval_minutes = 0.0f;
    }

    if (reviews->recent_count == HR_ANALYTICS_MAX_RECENT_INTERVALS) {
        memmove(&reviews->recent_intervals[0],
                &reviews->recent_intervals[1],
                (HR_ANALYTICS_MAX_RECENT_INTERVALS - 1U) * sizeof(float));
        reviews->recent_count--;
    }
    reviews->recent_intervals[reviews->recent_count++] = interval_minutes;

    handle->interval_sum_minutes += (double)interval_minutes;
    if (reviews->total_reviews > 0U) {
        reviews->average_interval_minutes = handle->interval_sum_minutes / (double)reviews->total_reviews;
    } else {
        reviews->average_interval_minutes = 0.0;
    }

    bool success = rating >= SRS_RESPONSE_GOOD && rating <= SRS_RESPONSE_CRAM;

    time_t timestamp = event->result.review_time;
    if (timestamp <= 0) {
        timestamp = event->context.now;
    }
    if (timestamp <= 0) {
        timestamp = time(NULL);
    }

    time_t day_start = truncate_to_day(timestamp);
    HrAnalyticsHeatmapSample *sample = ensure_heatmap_sample(handle, day_start);
    if (sample != NULL) {
        sample->total_reviews++;
        if (success) {
            sample->successful_reviews++;
        }
    }

    update_streaks(handle, day_start);
    update_retention(handle, event->result.previous_interval_days, success);
}

void analytics_record_session_event(const struct SessionReviewEvent *event, void *user_data)
{
    if (user_data == NULL) {
        return;
    }
    analytics_record_review((struct AnalyticsHandle *)user_data, event);
}

const HrAnalyticsDashboard *analytics_dashboard(const struct AnalyticsHandle *handle)
{
    if (handle == NULL) {
        return NULL;
    }
    return &handle->dashboard;
}
