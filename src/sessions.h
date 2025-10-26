#ifndef HYPERRECALL_SESSIONS_H
#define HYPERRECALL_SESSIONS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file sessions.h
 * @brief Declares study session orchestration and progress tracking interfaces.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "srs.h"

/** Supported study loop orchestration modes. */
typedef enum SessionMode {
    SESSION_MODE_MASTERY = 0, /**< Standard mastery-based scheduling. */
    SESSION_MODE_CRAM = 1,    /**< Short-term cram scheduling. */
    SESSION_MODE_CUSTOM = 2,  /**< Caller-controlled hybrid behaviour. */
    SESSION_MODE_EXAM_SIM = 3 /**< Exam simulation that avoids persistence. */
} SessionMode;

/**
 * Source data used when constructing a study queue entry.
 */
typedef struct SessionCardSpec {
    uint64_t card_id;                  /**< Unique identifier for the card. */
    bool has_state;                    /**< True when @p state contains valid data. */
    SRSState state;                    /**< In-memory scheduler state (optional). */
    bool has_persisted_state;          /**< True when @p persisted_state should be unpacked. */
    SRSPersistedState persisted_state; /**< Serialized scheduler state (optional). */
    bool has_topic;                    /**< True when @p topic is provided. */
    SRSTopicContext topic;             /**< Topic metadata for analytics and modifiers. */
    bool has_context;                  /**< True when @p context overrides are supplied. */
    SRSReviewContext context;          /**< Optional per-card context defaults. */
    void *user_data;                   /**< Caller-provided handle associated with the card. */
} SessionCardSpec;

/**
 * Fully-realized session queue entry exposed to the study loop.
 */
typedef struct SessionCard {
    uint64_t card_id;             /**< Unique identifier for the queued card. */
    SRSState state;               /**< Active spaced repetition state. */
    SRSTopicContext topic;        /**< Topic metadata applied during reviews. */
    bool has_custom_context;      /**< True when @p custom_context should be used. */
    SRSReviewContext custom_context; /**< Caller supplied context overrides. */
    void *user_data;              /**< User data pointer mirrored from spec. */
} SessionCard;

/**
 * Summary describing a single review processed inside the session manager.
 */
typedef struct SessionReviewEvent {
    uint64_t card_id;                  /**< Card identifier used for analytics hooks. */
    SessionMode mode;                  /**< Session mode driving the review. */
    bool simulated;                    /**< True when the session avoided persistence. */
    size_t queue_position;             /**< Zero-based index of the processed card. */
    size_t remaining;                  /**< Cards remaining after completing the review. */
    const SRSState *state;             /**< Pointer to the (possibly updated) card state. */
    SRSReviewContext context;          /**< Context supplied to the scheduler. */
    SRSReviewResult result;            /**< Scheduler output from srs_apply_review(). */
} SessionReviewEvent;

/** Callback used to persist updated spaced repetition state. */
typedef bool (*session_autosave_callback)(const SessionReviewEvent *event,
                                          const SRSPersistedState *persisted,
                                          void *user_data);

/** Callback invoked for session/analytics consumers after a review completes. */
typedef void (*session_review_callback)(const SessionReviewEvent *event,
                                        void *user_data);

#if HYPERRECALL_ENABLE_DEVTOOLS
/** Callback emitted when developer tracing is enabled. */
typedef void (*session_devtools_callback)(const SessionReviewEvent *event,
                                          void *user_data);
#endif /* HYPERRECALL_ENABLE_DEVTOOLS */

/** Bundles optional callbacks interested in session lifecycle events. */
typedef struct SessionCallbacks {
    session_review_callback session_event;   /**< Invoked for UI/session consumers. */
    void *session_user_data;                 /**< User data for @p session_event. */
    session_review_callback analytics_event; /**< Invoked for analytics pipelines. */
    void *analytics_user_data;               /**< User data for @p analytics_event. */
    session_autosave_callback autosave_event;/**< Invoked to persist SRS state. */
    void *autosave_user_data;                /**< User data for @p autosave_event. */
#if HYPERRECALL_ENABLE_DEVTOOLS
    session_devtools_callback devtools_event;/**< Optional developer tooling hook. */
    void *devtools_user_data;                /**< User data for @p devtools_event. */
#endif /* HYPERRECALL_ENABLE_DEVTOOLS */
} SessionCallbacks;

struct SessionManager;

/** Allocates a new session manager instance. */
struct SessionManager *session_manager_create(void);

/** Releases memory and resources owned by the session manager. */
void session_manager_destroy(struct SessionManager *manager);

/** Applies the supplied scheduler configuration (or defaults when NULL). */
void session_manager_set_config(struct SessionManager *manager,
                                const SRSConfig *config);

/** Supplies calibration hooks used when invoking the HyperSRS scheduler. */
void session_manager_set_calibration(struct SessionManager *manager,
                                     const SRSCalibrationHooks *hooks);

/** Registers review callbacks that should be forwarded to srs_apply_review(). */
void session_manager_set_srs_callbacks(struct SessionManager *manager,
                                       const SRSCallbacks *callbacks);

/** Registers session, analytics, autosave, and developer tooling callbacks. */
void session_manager_set_callbacks(struct SessionManager *manager,
                                   const SessionCallbacks *callbacks);

/**
 * Initializes a session queue for the requested mode.
 *
 * Existing queue contents are cleared. When @p cards is NULL the function
 * succeeds only if @p count is zero.
 */
bool session_manager_begin(struct SessionManager *manager,
                           SessionMode mode,
                           const SessionCardSpec *cards,
                           size_t count);

/** Clears any in-flight session state and releases queued cards. */
void session_manager_end(struct SessionManager *manager);

/** Returns the card currently at the front of the session queue. */
const SessionCard *session_manager_current(const struct SessionManager *manager);

/** Returns the active session mode. */
SessionMode session_manager_mode(const struct SessionManager *manager);

/** Returns how many cards remain (including the current card when any). */
size_t session_manager_remaining(const struct SessionManager *manager);

/**
 * Grades the current card using the supplied rating and optional context
 * overrides, advancing the session queue on success.
 */
bool session_manager_grade(struct SessionManager *manager,
                           SRSReviewRating rating,
                           const SRSReviewContext *override_context,
                           SRSReviewResult *out_result);

#if HYPERRECALL_ENABLE_DEVTOOLS
/** Returns the number of developer trace frames recorded for the session. */
size_t session_manager_trace_count(const struct SessionManager *manager);

/** Copies the trace frame at @p index into @p out_event when available. */
bool session_manager_trace_get(const struct SessionManager *manager,
                               size_t index,
                               SessionReviewEvent *out_event);

/** Clears any accumulated developer trace frames without ending the session. */
void session_manager_trace_clear(struct SessionManager *manager);
#endif /* HYPERRECALL_ENABLE_DEVTOOLS */

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_SESSIONS_H */
