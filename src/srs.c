#include "srs.h"

#include <math.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

static double clamp_double(double value, double min_value, double max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static double ensure_min_days(double interval_days, double min_minutes)
{
    const double min_days = min_minutes / 1440.0;
    if (interval_days < min_days) {
        return min_days;
    }
    return interval_days;
}

static time_t compute_due_time(time_t now, double interval_minutes)
{
    const double seconds = interval_minutes * 60.0;
    const double rounded = seconds >= 0.0 ? floor(seconds + 0.5) : ceil(seconds - 0.5);
    return now + (time_t)rounded;
}

void srs_default_config(SRSConfig *config)
{
    if (config == NULL) {
        return;
    }

    config->starting_interval_days = 1.0;
    config->minimum_interval_minutes = 10.0;
    config->maximum_interval_days = 365.0;
    config->ease_default = 2.5;
    config->ease_min = 1.3;
    config->ease_max = 3.0;
    config->ease_step_easy = 0.15;
    config->ease_step_hard = 0.15;
    config->ease_step_fail = 0.35;
    config->easy_bonus = 1.5;
    config->hard_interval_factor = 0.5;
    config->lapse_reset_interval_days = 0.7;
    config->cram_initial_interval_minutes = 5.0;
    config->cram_growth_multiplier = 2.0;
    config->cram_hard_penalty = 0.5;
    config->cram_bleed_ratio = 0.25;
    config->exam_override_window_days = 7.0;
    config->exam_override_multiplier = 0.35;
    config->topic_modifier_floor = 0.5;
    config->topic_modifier_ceiling = 2.0;
}

void srs_state_init(SRSState *state, const SRSConfig *config)
{
    if (state == NULL) {
        return;
    }

    const double ease = (config != NULL) ? config->ease_default : 2.5;
    const double start_interval = (config != NULL) ? config->starting_interval_days : 1.0;

    state->version = SRS_STATE_VERSION;
    state->mode = SRS_MODE_MASTERY;
    state->ease_factor = ease;
    state->interval_days = start_interval;
    state->cram_interval_minutes = (config != NULL) ? config->cram_initial_interval_minutes : 5.0;
    state->cram_bleed_minutes = 0.0;
    state->topic_adjustment = 1.0;
    state->consecutive_correct = 0u;
    state->due = 0;
    state->last_review = 0;
}

void srs_state_pack(const SRSState *state, SRSPersistedState *out)
{
    if (state == NULL || out == NULL) {
        return;
    }

    out->version = state->version;
    out->mode = (uint32_t)state->mode;
    out->consecutive_correct = state->consecutive_correct;
    out->due_unix = (int64_t)state->due;
    out->last_review_unix = (int64_t)state->last_review;
    out->ease_factor = state->ease_factor;
    out->interval_days = state->interval_days;
    out->cram_interval_minutes = state->cram_interval_minutes;
    out->cram_bleed_minutes = state->cram_bleed_minutes;
    out->topic_adjustment = state->topic_adjustment;
}

void srs_state_unpack(SRSState *state, const SRSPersistedState *in,
                      const SRSConfig *config)
{
    if (state == NULL) {
        return;
    }

    if (in == NULL) {
        srs_state_init(state, config);
        return;
    }

    const double ease_default = (config != NULL) ? config->ease_default : 2.5;
    const double starting_interval = (config != NULL) ? config->starting_interval_days : 1.0;

    state->version = (in->version != 0u) ? in->version : SRS_STATE_VERSION;
    state->mode = (in->mode <= (uint32_t)SRS_MODE_CRAM) ? (SRSMode)in->mode : SRS_MODE_MASTERY;
    state->consecutive_correct = in->consecutive_correct;
    state->due = (time_t)in->due_unix;
    state->last_review = (time_t)in->last_review_unix;
    state->ease_factor = (in->ease_factor > 0.0) ? in->ease_factor : ease_default;
    state->interval_days = (in->interval_days > 0.0) ? in->interval_days : starting_interval;
    state->cram_interval_minutes = (in->cram_interval_minutes > 0.0)
                                       ? in->cram_interval_minutes
                                       : ((config != NULL) ? config->cram_initial_interval_minutes : 5.0);
    state->cram_bleed_minutes = (in->cram_bleed_minutes >= 0.0) ? in->cram_bleed_minutes : 0.0;
    state->topic_adjustment = (in->topic_adjustment > 0.0) ? in->topic_adjustment : 1.0;
}

static double resolve_topic_modifier(const SRSConfig *config,
                                     const SRSState *state,
                                     const SRSReviewContext *context,
                                     const SRSCalibrationHooks *hooks)
{
    double modifier = 1.0;

    if (state != NULL && state->topic_adjustment > 0.0) {
        modifier *= state->topic_adjustment;
    }

    if (context != NULL && context->topic.weight > 0.0) {
        modifier *= context->topic.weight;
    }

    if (hooks != NULL && hooks->topic_hook != NULL) {
        const char *topic_id = (context != NULL) ? context->topic.topic_id : NULL;
        double hook_value = hooks->topic_hook(topic_id, modifier, hooks->topic_hook_user_data);
        if (hook_value > 0.0) {
            modifier = hook_value;
        }
    }

    const double floor_value = (config != NULL) ? config->topic_modifier_floor : 0.5;
    const double ceiling_value = (config != NULL) ? config->topic_modifier_ceiling : 2.0;
    modifier = clamp_double(modifier, floor_value, ceiling_value);
    return modifier;
}

static double adjust_ease_factor(const SRSConfig *config,
                                 const SRSState *state,
                                 double proposed_ease,
                                 const SRSCalibrationHooks *hooks)
{
    double ease = proposed_ease;
    if (hooks != NULL && hooks->ease_hook != NULL) {
        double hook_value = hooks->ease_hook(state, ease, hooks->ease_hook_user_data);
        if (hook_value > 0.0) {
            ease = hook_value;
        }
    }

    const double ease_min = (config != NULL) ? config->ease_min : 1.3;
    const double ease_max = (config != NULL) ? config->ease_max : 3.0;
    return clamp_double(ease, ease_min, ease_max);
}

static double adjust_interval_days(const SRSConfig *config,
                                   const SRSState *state,
                                   double proposed_days,
                                   const SRSCalibrationHooks *hooks)
{
    double days = proposed_days;
    if (hooks != NULL && hooks->interval_hook != NULL) {
        double hook_value = hooks->interval_hook(state, days, hooks->interval_hook_user_data);
        if (hook_value > 0.0) {
            days = hook_value;
        }
    }

    const double max_days = (config != NULL) ? config->maximum_interval_days : 365.0;
    days = clamp_double(days, 0.0, max_days);
    return ensure_min_days(days, (config != NULL) ? config->minimum_interval_minutes : 10.0);
}

static double apply_cram_bleed(const SRSConfig *config,
                               SRSState *state,
                               double interval_days)
{
    if (state == NULL || config == NULL) {
        return interval_days;
    }

    if (config->cram_bleed_ratio <= 0.0) {
        state->cram_bleed_minutes = 0.0;
        return interval_days;
    }

    if (state->cram_bleed_minutes <= 0.0) {
        return interval_days;
    }

    const double bleed_ratio = clamp_double(config->cram_bleed_ratio, 0.0, 1.0);
    const double bleed_days = (state->cram_bleed_minutes / 1440.0);
    const double blended = (interval_days * (1.0 - bleed_ratio)) + (bleed_days * bleed_ratio);
    state->cram_bleed_minutes *= (1.0 - bleed_ratio);
    return blended;
}

static bool is_exam_override_active(const SRSConfig *config,
                                    const SRSReviewContext *context,
                                    double *out_days_until_exam)
{
    if (config == NULL || context == NULL) {
        return false;
    }

    if (context->exam_date <= 0 || context->now <= 0) {
        return false;
    }

    const double seconds_until = difftime(context->exam_date, context->now);
    const double days_until = seconds_until / 86400.0;
    if (out_days_until_exam != NULL) {
        *out_days_until_exam = days_until;
    }

    return days_until >= 0.0 && days_until <= config->exam_override_window_days;
}

SRSReviewResult srs_apply_review(const SRSConfig *config,
                                 SRSState *state,
                                 SRSReviewRating rating,
                                 const SRSReviewContext *context,
                                 const SRSCalibrationHooks *hooks,
                                 const SRSCallbacks *callbacks)
{
    SRSReviewResult result;
    memset(&result, 0, sizeof(result));
    result.rating = rating;

    if (state == NULL) {
        return result;
    }

    SRSConfig local_config;
    if (config == NULL) {
        srs_default_config(&local_config);
        config = &local_config;
    }

    SRSReviewContext ctx;
    if (context != NULL) {
        ctx = *context;
    } else {
        memset(&ctx, 0, sizeof(ctx));
    }

    if (ctx.now == 0) {
        ctx.now = time(NULL);
    }

    if (ctx.topic.weight <= 0.0) {
        ctx.topic.weight = 1.0;
    }

    double days_until_exam = 0.0;
    const bool exam_override = is_exam_override_active(config, &ctx, &days_until_exam);
    const double exam_multiplier = exam_override
                                      ? clamp_double(config->exam_override_multiplier, 0.05, 1.0)
                                      : 1.0;

    const double topic_modifier = resolve_topic_modifier(config, state, &ctx, hooks);
    state->topic_adjustment = topic_modifier;

    const double previous_interval_days = state->interval_days;
    result.previous_interval_days = previous_interval_days;

    double ease = state->ease_factor;
    double interval_days = state->interval_days;
    double interval_minutes = state->cram_interval_minutes;

    if (interval_days <= 0.0) {
        interval_days = config->starting_interval_days;
    }
    if (interval_minutes <= 0.0) {
        interval_minutes = config->cram_initial_interval_minutes;
    }

    bool used_cram = ctx.cram_session || rating == SRS_RESPONSE_CRAM;

    if (used_cram) {
        switch (rating) {
        case SRS_RESPONSE_FAIL:
            interval_minutes = config->cram_initial_interval_minutes;
            state->consecutive_correct = 0;
            break;
        case SRS_RESPONSE_HARD:
            interval_minutes = ensure_min_days(interval_minutes / 1440.0,
                                               config->minimum_interval_minutes) * 1440.0;
            interval_minutes *= config->cram_hard_penalty;
            if (interval_minutes < config->cram_initial_interval_minutes) {
                interval_minutes = config->cram_initial_interval_minutes;
            }
            state->consecutive_correct = 0;
            break;
        case SRS_RESPONSE_GOOD:
        case SRS_RESPONSE_CRAM:
            interval_minutes *= config->cram_growth_multiplier;
            state->consecutive_correct += 1;
            break;
        case SRS_RESPONSE_EASY:
            interval_minutes *= (config->cram_growth_multiplier * 1.5);
            state->consecutive_correct += 1;
            break;
        default:
            break;
        }

        interval_minutes *= topic_modifier;
        interval_minutes *= exam_multiplier;
        if (interval_minutes < config->minimum_interval_minutes) {
            interval_minutes = config->minimum_interval_minutes;
        }

        state->cram_interval_minutes = interval_minutes;
        state->cram_bleed_minutes = (state->cram_bleed_minutes * 0.5) + (interval_minutes * 0.5);
        interval_days = interval_minutes / 1440.0;
        ease = adjust_ease_factor(config, state, ease, hooks);
        state->ease_factor = ease;
        state->mode = SRS_MODE_CRAM;
    } else {
        switch (rating) {
        case SRS_RESPONSE_FAIL:
            ease = adjust_ease_factor(config, state, ease - config->ease_step_fail, hooks);
            interval_days = config->lapse_reset_interval_days;
            state->consecutive_correct = 0;
            break;
        case SRS_RESPONSE_HARD:
            ease = adjust_ease_factor(config, state, ease - config->ease_step_hard, hooks);
            interval_days *= config->hard_interval_factor;
            state->consecutive_correct = 0;
            break;
        case SRS_RESPONSE_GOOD:
            interval_days *= ease;
            state->consecutive_correct += 1;
            ease = adjust_ease_factor(config, state, ease, hooks);
            break;
        case SRS_RESPONSE_EASY:
            ease = adjust_ease_factor(config, state, ease + config->ease_step_easy, hooks);
            interval_days *= ease * config->easy_bonus;
            state->consecutive_correct += 1;
            break;
        case SRS_RESPONSE_CRAM:
            /* Already handled by used_cram flag; keep compiler happy. */
            break;
        }

        interval_days *= topic_modifier;
        interval_days *= exam_multiplier;
        interval_days = apply_cram_bleed(config, state, interval_days);
        interval_days = adjust_interval_days(config, state, interval_days, hooks);
        interval_minutes = interval_days * 1440.0;
        double baseline_cram = state->cram_interval_minutes;
        if (baseline_cram <= 0.0) {
            baseline_cram = config->cram_initial_interval_minutes;
        }
        baseline_cram = (baseline_cram * (1.0 - config->cram_bleed_ratio)) +
                        (config->cram_initial_interval_minutes * config->cram_bleed_ratio);
        state->cram_interval_minutes = baseline_cram;
        state->mode = SRS_MODE_MASTERY;
        state->ease_factor = ease;
    }

    interval_days = ensure_min_days(interval_days, config->minimum_interval_minutes);
    interval_minutes = interval_days * 1440.0;

    const time_t due_time = compute_due_time(ctx.now, interval_minutes);
    state->interval_days = interval_days;
    state->due = due_time;
    state->last_review = ctx.now;
    state->version = SRS_STATE_VERSION;

    result.review_time = ctx.now;
    result.due = due_time;
    result.interval_days = interval_days;
    result.interval_minutes = interval_minutes;
    result.topic_modifier = topic_modifier;
    result.applied_ease_factor = state->ease_factor;
    result.consecutive_correct = state->consecutive_correct;
    result.used_cram = used_cram;
    result.exam_override = exam_override;
    result.mode = state->mode;

    if (callbacks != NULL) {
        SRSReviewEvent event;
        event.state = state;
        event.context = ctx;
        event.result = result;

        if (callbacks->session_callback != NULL) {
            callbacks->session_callback(&event, callbacks->session_user_data);
        }
        if (callbacks->analytics_callback != NULL) {
            callbacks->analytics_callback(&event, callbacks->analytics_user_data);
        }
    }

    return result;
}
