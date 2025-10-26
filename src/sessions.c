#include "sessions.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct SessionCardEntry {
    SessionCard card;
} SessionCardEntry;

struct SessionManager {
    SRSConfig config;
    SRSCalibrationHooks calibration_hooks;
    bool calibration_enabled;

    SRSCallbacks srs_callbacks;
    bool srs_callbacks_enabled;

    SessionCallbacks callbacks;

    SessionCardEntry *queue;
    size_t queue_count;
    size_t queue_index;

    SessionMode mode;
    bool in_session;

#if HYPERRECALL_ENABLE_DEVTOOLS
    SessionReviewEvent *trace_frames;
    size_t trace_count;
    size_t trace_capacity;
#endif
};

static void session_manager_reset_queue(struct SessionManager *manager)
{
    if (manager == NULL) {
        return;
    }

    free(manager->queue);
    manager->queue = NULL;
    manager->queue_count = 0u;
    manager->queue_index = 0u;
    manager->in_session = false;
}

static void session_card_from_spec(SessionCard *card,
                                   const SessionCardSpec *spec,
                                   const SRSConfig *config)
{
    if (card == NULL) {
        return;
    }

    memset(card, 0, sizeof(*card));
    card->card_id = (spec != NULL) ? spec->card_id : 0u;
    card->user_data = (spec != NULL) ? spec->user_data : NULL;

    SRSTopicContext topic = {0};
    if (spec != NULL && spec->has_topic) {
        topic = spec->topic;
    }
    if (topic.weight <= 0.0) {
        topic.weight = 1.0;
    }
    card->topic = topic;

    SRSState state;
    if (spec != NULL && spec->has_state) {
        state = spec->state;
    } else if (spec != NULL && spec->has_persisted_state) {
        srs_state_unpack(&state, &spec->persisted_state, config);
    } else {
        srs_state_init(&state, config);
    }
    card->card_id = (spec != NULL) ? spec->card_id : 0u;
    card->state = state;

    if (spec != NULL && spec->has_context) {
        card->custom_context = spec->context;
        if (card->custom_context.topic.weight <= 0.0) {
            card->custom_context.topic.weight = topic.weight;
        }
        if (card->custom_context.topic.topic_id == NULL) {
            card->custom_context.topic.topic_id = topic.topic_id;
        }
        card->has_custom_context = true;
    } else {
        memset(&card->custom_context, 0, sizeof(card->custom_context));
        card->has_custom_context = false;
    }
}

static int compare_due_time(const void *lhs, const void *rhs)
{
    const SessionCardEntry *a = (const SessionCardEntry *)lhs;
    const SessionCardEntry *b = (const SessionCardEntry *)rhs;

    const time_t due_a = a->card.state.due;
    const time_t due_b = b->card.state.due;

    if (due_a == due_b) {
        if (a->card.card_id == b->card.card_id) {
            return 0;
        }
        return (a->card.card_id < b->card.card_id) ? -1 : 1;
    }

    if (due_a == 0) {
        return -1;
    }
    if (due_b == 0) {
        return 1;
    }

    return (due_a < due_b) ? -1 : 1;
}

static SRSReviewContext session_compose_context(struct SessionManager *manager,
                                                const SessionCard *card,
                                                const SRSReviewContext *override_context)
{
    SRSReviewContext context;
    memset(&context, 0, sizeof(context));

    if (card != NULL && card->has_custom_context) {
        context = card->custom_context;
    }

    if (override_context != NULL) {
        if (override_context->now != 0) {
            context.now = override_context->now;
        }
        if (override_context->exam_date != 0) {
            context.exam_date = override_context->exam_date;
        }
        if (override_context->cram_session) {
            context.cram_session = true;
        }
        if (override_context->topic.topic_id != NULL) {
            context.topic.topic_id = override_context->topic.topic_id;
        }
        if (override_context->topic.weight > 0.0) {
            context.topic.weight = override_context->topic.weight;
        }
    }

    if (context.topic.topic_id == NULL && card != NULL) {
        context.topic.topic_id = card->topic.topic_id;
    }

    if (context.topic.weight <= 0.0) {
        context.topic.weight = (card != NULL && card->topic.weight > 0.0) ? card->topic.weight : 1.0;
    }

    if (manager != NULL) {
        switch (manager->mode) {
        case SESSION_MODE_CRAM:
            context.cram_session = true;
            break;
        case SESSION_MODE_MASTERY:
            context.cram_session = false;
            break;
        case SESSION_MODE_CUSTOM:
            /* Honor card or override preferences. */
            break;
        case SESSION_MODE_EXAM_SIM:
            context.cram_session = false;
            break;
        }
    }

    if (context.now == 0) {
        context.now = time(NULL);
    }

    return context;
}

static void session_emit_callbacks(struct SessionManager *manager,
                                   const SessionReviewEvent *event)
{
    if (manager == NULL || event == NULL) {
        return;
    }

    if (manager->callbacks.session_event != NULL) {
        manager->callbacks.session_event(event, manager->callbacks.session_user_data);
    }
    if (manager->callbacks.analytics_event != NULL) {
        manager->callbacks.analytics_event(event, manager->callbacks.analytics_user_data);
    }

#if HYPERRECALL_ENABLE_DEVTOOLS
    if (manager->callbacks.devtools_event != NULL) {
        manager->callbacks.devtools_event(event, manager->callbacks.devtools_user_data);
    }

    if (manager->trace_count == manager->trace_capacity) {
        size_t new_capacity = (manager->trace_capacity == 0u) ? 16u : (manager->trace_capacity * 2u);
        SessionReviewEvent *resized = (SessionReviewEvent *)realloc(manager->trace_frames,
                                                                    new_capacity * sizeof(SessionReviewEvent));
        if (resized != NULL) {
            manager->trace_frames = resized;
            manager->trace_capacity = new_capacity;
        }
    }

    if (manager->trace_count < manager->trace_capacity && manager->trace_frames != NULL) {
        manager->trace_frames[manager->trace_count++] = *event;
    }
#endif
}

struct SessionManager *session_manager_create(void)
{
    struct SessionManager *manager = (struct SessionManager *)calloc(1u, sizeof(struct SessionManager));
    if (manager == NULL) {
        return NULL;
    }

    srs_default_config(&manager->config);
    memset(&manager->calibration_hooks, 0, sizeof(manager->calibration_hooks));
    manager->calibration_enabled = false;
    memset(&manager->srs_callbacks, 0, sizeof(manager->srs_callbacks));
    manager->srs_callbacks_enabled = false;
    memset(&manager->callbacks, 0, sizeof(manager->callbacks));
    manager->mode = SESSION_MODE_MASTERY;
    manager->in_session = false;

#if HYPERRECALL_ENABLE_DEVTOOLS
    manager->trace_frames = NULL;
    manager->trace_count = 0u;
    manager->trace_capacity = 0u;
#endif

    return manager;
}

void session_manager_destroy(struct SessionManager *manager)
{
    if (manager == NULL) {
        return;
    }

    session_manager_end(manager);

#if HYPERRECALL_ENABLE_DEVTOOLS
    free(manager->trace_frames);
    manager->trace_frames = NULL;
    manager->trace_count = 0u;
    manager->trace_capacity = 0u;
#endif

    free(manager);
}

void session_manager_set_config(struct SessionManager *manager, const SRSConfig *config)
{
    if (manager == NULL) {
        return;
    }

    if (config != NULL) {
        manager->config = *config;
    } else {
        srs_default_config(&manager->config);
    }
}

void session_manager_set_calibration(struct SessionManager *manager,
                                     const SRSCalibrationHooks *hooks)
{
    if (manager == NULL) {
        return;
    }

    if (hooks != NULL) {
        manager->calibration_hooks = *hooks;
        manager->calibration_enabled = true;
    } else {
        memset(&manager->calibration_hooks, 0, sizeof(manager->calibration_hooks));
        manager->calibration_enabled = false;
    }
}

void session_manager_set_srs_callbacks(struct SessionManager *manager,
                                       const SRSCallbacks *callbacks)
{
    if (manager == NULL) {
        return;
    }

    if (callbacks != NULL) {
        manager->srs_callbacks = *callbacks;
        manager->srs_callbacks_enabled = true;
    } else {
        memset(&manager->srs_callbacks, 0, sizeof(manager->srs_callbacks));
        manager->srs_callbacks_enabled = false;
    }
}

void session_manager_set_callbacks(struct SessionManager *manager,
                                   const SessionCallbacks *callbacks)
{
    if (manager == NULL) {
        return;
    }

    if (callbacks != NULL) {
        manager->callbacks = *callbacks;
    } else {
        memset(&manager->callbacks, 0, sizeof(manager->callbacks));
    }
}

bool session_manager_begin(struct SessionManager *manager,
                           SessionMode mode,
                           const SessionCardSpec *cards,
                           size_t count)
{
    if (manager == NULL) {
        return false;
    }

#if HYPERRECALL_ENABLE_DEVTOOLS
    session_manager_trace_clear(manager);
#endif

    session_manager_reset_queue(manager);

    manager->mode = mode;

    if (count == 0u) {
        manager->in_session = false;
        return cards == NULL;
    }

    if (cards == NULL) {
        return false;
    }

    SessionCardEntry *entries = (SessionCardEntry *)calloc(count, sizeof(SessionCardEntry));
    if (entries == NULL) {
        return false;
    }

    for (size_t i = 0; i < count; ++i) {
        session_card_from_spec(&entries[i].card, &cards[i], &manager->config);
    }

    if (mode != SESSION_MODE_CUSTOM) {
        qsort(entries, count, sizeof(SessionCardEntry), compare_due_time);
    }

    manager->queue = entries;
    manager->queue_count = count;
    manager->queue_index = 0u;
    manager->in_session = (count > 0u);

    return true;
}

void session_manager_end(struct SessionManager *manager)
{
    if (manager == NULL) {
        return;
    }

    session_manager_reset_queue(manager);
}

const SessionCard *session_manager_current(const struct SessionManager *manager)
{
    if (manager == NULL || !manager->in_session) {
        return NULL;
    }

    if (manager->queue_index >= manager->queue_count) {
        return NULL;
    }

    return &manager->queue[manager->queue_index].card;
}

SessionMode session_manager_mode(const struct SessionManager *manager)
{
    if (manager == NULL) {
        return SESSION_MODE_MASTERY;
    }
    return manager->mode;
}

size_t session_manager_remaining(const struct SessionManager *manager)
{
    if (manager == NULL || !manager->in_session) {
        return 0u;
    }

    if (manager->queue_index >= manager->queue_count) {
        return 0u;
    }

    return manager->queue_count - manager->queue_index;
}

bool session_manager_grade(struct SessionManager *manager,
                           SRSReviewRating rating,
                           const SRSReviewContext *override_context,
                           SRSReviewResult *out_result)
{
    if (manager == NULL || !manager->in_session) {
        return false;
    }

    if (manager->queue_index >= manager->queue_count) {
        return false;
    }

    SessionCardEntry *entry = &manager->queue[manager->queue_index];
    SessionCard *card = &entry->card;

    SRSReviewContext context = session_compose_context(manager, card, override_context);

    const bool simulate_only = (manager->mode == SESSION_MODE_EXAM_SIM);

    SRSState working_state = card->state;
    SRSState *state_ptr = simulate_only ? &working_state : &card->state;

    const SRSCalibrationHooks *hooks = manager->calibration_enabled ? &manager->calibration_hooks : NULL;
    const SRSCallbacks *callbacks = manager->srs_callbacks_enabled ? &manager->srs_callbacks : NULL;

    SRSReviewResult result = srs_apply_review(&manager->config,
                                              state_ptr,
                                              rating,
                                              &context,
                                              hooks,
                                              callbacks);

    if (!simulate_only) {
        card->state = *state_ptr;
    }

    SessionReviewEvent event;
    memset(&event, 0, sizeof(event));
    event.card_id = card->card_id;
    event.mode = manager->mode;
    event.simulated = simulate_only;
    event.queue_position = manager->queue_index;
    event.remaining = (manager->queue_count > (manager->queue_index + 1u))
                          ? (manager->queue_count - (manager->queue_index + 1u))
                          : 0u;
    event.state = &card->state;
    event.context = context;
    event.result = result;

    bool autosave_ok = true;
    SRSPersistedState persisted_state;
    memset(&persisted_state, 0, sizeof(persisted_state));

    if (!simulate_only) {
        srs_state_pack(&card->state, &persisted_state);

        if (manager->callbacks.autosave_event != NULL) {
            autosave_ok = manager->callbacks.autosave_event(&event,
                                                            &persisted_state,
                                                            manager->callbacks.autosave_user_data);
        }
    }

    if (!autosave_ok) {
        /* Restore the previous state if persistence fails. */
        card->state = working_state;
        return false;
    }

    session_emit_callbacks(manager, &event);

    if (out_result != NULL) {
        *out_result = result;
    }

    manager->queue_index += 1u;
    if (manager->queue_index >= manager->queue_count) {
        manager->in_session = false;
    }

    return true;
}

#if HYPERRECALL_ENABLE_DEVTOOLS
size_t session_manager_trace_count(const struct SessionManager *manager)
{
    if (manager == NULL) {
        return 0u;
    }

    return manager->trace_count;
}

bool session_manager_trace_get(const struct SessionManager *manager,
                               size_t index,
                               SessionReviewEvent *out_event)
{
    if (manager == NULL || out_event == NULL) {
        return false;
    }

    if (index >= manager->trace_count) {
        return false;
    }

    *out_event = manager->trace_frames[index];
    return true;
}

void session_manager_trace_clear(struct SessionManager *manager)
{
    if (manager == NULL) {
        return;
    }

    if (manager->trace_frames != NULL) {
        manager->trace_count = 0u;
    }
}
#endif
