#ifndef HYPERRECALL_SRS_H
#define HYPERRECALL_SRS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

/**
 * @file srs.h
 * @brief Hybrid Mastery/Cram spaced repetition scheduler for HyperRecall.
 */

/** Version tag stored with persisted SRS state. */
#define SRS_STATE_VERSION 1u

/** Possible learner feedback ratings for a review. */
typedef enum SRSReviewRating {
    SRS_RESPONSE_FAIL = 0,  /**< The learner could not recall the prompt. */
    SRS_RESPONSE_HARD = 1,  /**< The learner recalled with difficulty. */
    SRS_RESPONSE_GOOD = 2,  /**< The learner recalled confidently. */
    SRS_RESPONSE_EASY = 3,  /**< The learner recalled effortlessly. */
    SRS_RESPONSE_CRAM = 4   /**< The learner wants another short-term cram pass. */
} SRSReviewRating;

/** Scheduler mode describing whether the item is being crammed or mastered. */
typedef enum SRSMode {
    SRS_MODE_MASTERY = 0,
    SRS_MODE_CRAM = 1
} SRSMode;

/**
 * Tunable configuration for the HyperSRS scheduler.
 */
typedef struct SRSConfig {
    double starting_interval_days;        /**< Initial mastery interval when unseen. */
    double minimum_interval_minutes;      /**< Lower bound for any scheduled interval. */
    double maximum_interval_days;         /**< Upper bound to avoid runaway growth. */
    double ease_default;                  /**< Default ease factor assigned on init. */
    double ease_min;                      /**< Minimum ease factor after penalties. */
    double ease_max;                      /**< Maximum ease factor after bonuses. */
    double ease_step_easy;                /**< Increment applied on EASY answers. */
    double ease_step_hard;                /**< Penalty applied on HARD answers. */
    double ease_step_fail;                /**< Penalty applied on FAIL answers. */
    double easy_bonus;                    /**< Additional multiplier for EASY answers. */
    double hard_interval_factor;          /**< Multiplier for HARD answers. */
    double lapse_reset_interval_days;     /**< Interval applied after a lapse/FAIL. */
    double cram_initial_interval_minutes; /**< First cram interval when starting from scratch. */
    double cram_growth_multiplier;        /**< Growth multiplier for repeated cram passes. */
    double cram_hard_penalty;             /**< Penalty applied to cram interval on HARD answers. */
    double cram_bleed_ratio;              /**< Portion of cram spacing bleeding into mastery. */
    double exam_override_window_days;     /**< Window before exam to compress intervals. */
    double exam_override_multiplier;      /**< Compression factor during exam week. */
    double topic_modifier_floor;          /**< Minimum topic multiplier allowed. */
    double topic_modifier_ceiling;        /**< Maximum topic multiplier allowed. */
} SRSConfig;

/**
 * Persistable spaced repetition state for a single study unit.
 */
typedef struct SRSState {
    uint32_t version;            /**< Version of the serialized state. */
    SRSMode mode;                /**< Last scheduling mode. */
    double ease_factor;          /**< Smoothed difficulty multiplier. */
    double interval_days;        /**< Last mastery interval in days. */
    double cram_interval_minutes;/**< Last cram interval in minutes. */
    double cram_bleed_minutes;   /**< Smoothed cram carry-over buffer. */
    double topic_adjustment;     /**< Persisted topic multiplier. */
    uint32_t consecutive_correct;/**< Consecutive mastery successes. */
    time_t due;                  /**< Next scheduled review timestamp. */
    time_t last_review;          /**< When the previous review was recorded. */
} SRSState;

/**
 * Lightweight struct used to persist scheduler state to storage.
 */
typedef struct SRSPersistedState {
    uint32_t version;
    uint32_t mode;
    uint32_t consecutive_correct;
    int64_t due_unix;
    int64_t last_review_unix;
    double ease_factor;
    double interval_days;
    double cram_interval_minutes;
    double cram_bleed_minutes;
    double topic_adjustment;
} SRSPersistedState;

/** Metadata for the topic associated with the review. */
typedef struct SRSTopicContext {
    const char *topic_id;  /**< Identifier used for analytics hooks. */
    double weight;         /**< External difficulty multiplier (1.0 = neutral). */
} SRSTopicContext;

/** Context surrounding an individual review application. */
typedef struct SRSReviewContext {
    time_t now;                 /**< Timestamp of the review (UTC). */
    time_t exam_date;           /**< Target exam date (0 if none). */
    bool cram_session;          /**< True if the learner is currently cramming. */
    SRSTopicContext topic;      /**< Topic metadata influencing modifiers. */
} SRSReviewContext;

/**
 * Result produced after applying a review outcome to the scheduler.
 */
typedef struct SRSReviewResult {
    time_t review_time;        /**< Timestamp when the review occurred. */
    time_t due;                /**< Computed due date for the next review. */
    double previous_interval_days; /**< Interval before the update (days). */
    double interval_days;      /**< New mastery interval in days. */
    double interval_minutes;   /**< New interval in minutes (cram friendly). */
    double topic_modifier;     /**< Multiplier applied from topic adjustments. */
    double applied_ease_factor;/**< Ease factor after the update. */
    uint32_t consecutive_correct; /**< Updated streak of successful recalls. */
    bool used_cram;            /**< True if the review used cram scheduling. */
    bool exam_override;        /**< True if exam compression was applied. */
    SRSMode mode;              /**< Scheduling mode selected for this review. */
    SRSReviewRating rating;    /**< Rating provided by the learner. */
} SRSReviewResult;

/** Callback invoked to adjust proposed intervals before commitment. */
typedef double (*srs_interval_calibration_hook)(const SRSState *state,
                                                double proposed_interval_days,
                                                void *user_data);

/** Callback invoked to adjust the ease factor after it has been updated. */
typedef double (*srs_ease_calibration_hook)(const SRSState *state,
                                            double proposed_ease_factor,
                                            void *user_data);

/** Callback invoked to supply custom topic-based multipliers. */
typedef double (*srs_topic_modifier_hook)(const char *topic_id,
                                          double base_modifier,
                                          void *user_data);

/** Container for optional calibration hooks. */
typedef struct SRSCalibrationHooks {
    srs_interval_calibration_hook interval_hook;
    void *interval_hook_user_data;
    srs_ease_calibration_hook ease_hook;
    void *ease_hook_user_data;
    srs_topic_modifier_hook topic_hook;
    void *topic_hook_user_data;
} SRSCalibrationHooks;

/** Event payload emitted whenever a review completes. */
typedef struct SRSReviewEvent {
    const SRSState *state; /**< Pointer to the updated state. */
    SRSReviewContext context;     /**< Snapshot of the supplied context. */
    SRSReviewResult result;       /**< Scheduler result. */
} SRSReviewEvent;

/** Callback signature for review completion events. */
typedef void (*srs_review_event_callback)(const SRSReviewEvent *event,
                                          void *user_data);

/** Pair of callbacks interested in review completion events. */
typedef struct SRSCallbacks {
    srs_review_event_callback session_callback;
    void *session_user_data;
    srs_review_event_callback analytics_callback;
    void *analytics_user_data;
} SRSCallbacks;

void srs_default_config(SRSConfig *config);
void srs_state_init(SRSState *state, const SRSConfig *config);
void srs_state_pack(const SRSState *state, SRSPersistedState *out);
void srs_state_unpack(SRSState *state, const SRSPersistedState *in,
                      const SRSConfig *config);
SRSReviewResult srs_apply_review(const SRSConfig *config,
                                 SRSState *state,
                                 SRSReviewRating rating,
                                 const SRSReviewContext *context,
                                 const SRSCalibrationHooks *hooks,
                                 const SRSCallbacks *callbacks);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_SRS_H */
