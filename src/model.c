#include "model.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const char* const CARD_TYPE_NAMES[] = {
    [HR_CARD_TYPE_SHORT_ANSWER] = "ShortAnswer",
    [HR_CARD_TYPE_CLOZE] = "Cloze",
    [HR_CARD_TYPE_MULTIPLE_CHOICE_SINGLE] = "MultipleChoice",
    [HR_CARD_TYPE_MULTIPLE_CHOICE_MULTI] = "MultipleResponse",
    [HR_CARD_TYPE_TRUE_FALSE] = "TrueFalse",
    [HR_CARD_TYPE_IMAGE_OCCLUSION] = "ImageOcclusion",
    [HR_CARD_TYPE_AUDIO_RECALL] = "AudioRecall",
    [HR_CARD_TYPE_TYPING] = "Typing",
    [HR_CARD_TYPE_ORDERING] = "Ordering",
    [HR_CARD_TYPE_MATCHING] = "Matching",
};

static const char* const MEDIA_TYPE_NAMES[] = {
    [HR_MEDIA_TYPE_IMAGE] = "Image",
    [HR_MEDIA_TYPE_AUDIO] = "Audio",
    [HR_MEDIA_TYPE_VIDEO] = "Video",
    [HR_MEDIA_TYPE_LATEX] = "Latex",
};

static int hr_ascii_casecmp(const char* lhs, const char* rhs) {
    if (!lhs || !rhs) {
        return (lhs == rhs) ? 0 : (lhs ? 1 : -1);
    }
    while (*lhs && *rhs) {
        int dl = tolower((unsigned char)*lhs);
        int dr = tolower((unsigned char)*rhs);
        if (dl != dr) {
            return dl - dr;
        }
        lhs++;
        rhs++;
    }
    return tolower((unsigned char)*lhs) - tolower((unsigned char)*rhs);
}

static bool hr_string_is_blank(const char* text) {
    if (!text) {
        return true;
    }
    while (*text) {
        if (!isspace((unsigned char)*text)) {
            return false;
        }
        text++;
    }
    return true;
}

static void hr_validation_error_set(HrValidationError* error, const char* field,
                                    const char* message) {
    if (!error) {
        return;
    }
    error->field = field;
    if (message) {
        snprintf(error->message, sizeof(error->message), "%s", message);
    } else {
        error->message[0] = '\0';
    }
}

const char* hr_card_type_to_string(HrCardType type) {
    if (type < 0 || (size_t)type >= (sizeof CARD_TYPE_NAMES / sizeof CARD_TYPE_NAMES[0])) {
        return "Unknown";
    }
    const char* name = CARD_TYPE_NAMES[type];
    return name ? name : "Unknown";
}

bool hr_card_type_from_string(const char* text, HrCardType* out_type) {
    if (!text || !out_type) {
        return false;
    }
    for (size_t i = 0; i < (sizeof CARD_TYPE_NAMES / sizeof CARD_TYPE_NAMES[0]); ++i) {
        const char* name = CARD_TYPE_NAMES[i];
        if (name && hr_ascii_casecmp(name, text) == 0) {
            *out_type = (HrCardType)i;
            return true;
        }
    }
    return false;
}

const char* hr_media_type_to_string(HrMediaType type) {
    if (type < 0 || (size_t)type >= (sizeof MEDIA_TYPE_NAMES / sizeof MEDIA_TYPE_NAMES[0])) {
        return "Unknown";
    }
    const char* name = MEDIA_TYPE_NAMES[type];
    return name ? name : "Unknown";
}

bool hr_media_type_from_string(const char* text, HrMediaType* out_type) {
    if (!text || !out_type) {
        return false;
    }
    for (size_t i = 0; i < (sizeof MEDIA_TYPE_NAMES / sizeof MEDIA_TYPE_NAMES[0]); ++i) {
        const char* name = MEDIA_TYPE_NAMES[i];
        if (name && hr_ascii_casecmp(name, text) == 0) {
            *out_type = (HrMediaType)i;
            return true;
        }
    }
    return false;
}

void hr_card_extras_init(HrCardExtras* extras, HrCardType type) {
    if (!extras) {
        return;
    }
    memset(extras, 0, sizeof(*extras));
    extras->type = type;
    switch (type) {
    case HR_CARD_TYPE_SHORT_ANSWER:
        extras->data.short_answer.case_sensitive = false;
        extras->data.short_answer.strip_whitespace = true;
        extras->data.short_answer.alternate_answer = NULL;
        break;
    case HR_CARD_TYPE_CLOZE:
        extras->data.cloze.cloze_count = 1;
        extras->data.cloze.strict = false;
        break;
    case HR_CARD_TYPE_MULTIPLE_CHOICE_SINGLE:
    case HR_CARD_TYPE_MULTIPLE_CHOICE_MULTI:
        extras->data.choice.options = NULL;
        extras->data.choice.option_count = 0;
        extras->data.choice.allow_multiple = (type == HR_CARD_TYPE_MULTIPLE_CHOICE_MULTI);
        extras->data.choice.shuffle = true;
        break;
    case HR_CARD_TYPE_TRUE_FALSE:
        extras->data.true_false.answer_true = true;
        extras->data.true_false.explanation = NULL;
        break;
    case HR_CARD_TYPE_IMAGE_OCCLUSION:
        extras->data.image.image_uuid = NULL;
        extras->data.image.mask_count = 0;
        extras->data.image.require_order = false;
        break;
    case HR_CARD_TYPE_AUDIO_RECALL:
        extras->data.audio.audio_uuid = NULL;
        extras->data.audio.require_transcript = false;
        extras->data.audio.reference_text = NULL;
        break;
    case HR_CARD_TYPE_TYPING:
        extras->data.typing.regex_pattern = NULL;
        extras->data.typing.case_sensitive = false;
        extras->data.typing.sample_answer = NULL;
        break;
    case HR_CARD_TYPE_ORDERING:
        extras->data.ordering.items = NULL;
        extras->data.ordering.item_count = 0;
        extras->data.ordering.partial_credit = true;
        break;
    case HR_CARD_TYPE_MATCHING:
        extras->data.matching.pairs = NULL;
        extras->data.matching.pair_count = 0;
        extras->data.matching.shuffle_right = true;
        break;
    default:
        break;
    }
}

static bool hr_card_choice_options_validate(const HrCardChoiceExtras* choice,
                                            HrValidationError*        error) {
    if (!choice) {
        hr_validation_error_set(error, "extras", "Choice extras missing");
        return false;
    }
    if (!choice->options || choice->option_count < 2) {
        hr_validation_error_set(error, "options",
                                "Multiple choice cards require at least two options");
        return false;
    }
    size_t correct_count = 0;
    for (size_t i = 0; i < choice->option_count; ++i) {
        const HrCardChoiceOption* option = &choice->options[i];
        if (!option->value || hr_string_is_blank(option->value)) {
            hr_validation_error_set(error, "options", "Option text cannot be blank");
            return false;
        }
        if (option->correct) {
            correct_count++;
        }
    }
    if (correct_count == 0) {
        hr_validation_error_set(error, "options", "At least one option must be marked correct");
        return false;
    }
    if (!choice->allow_multiple && correct_count > 1) {
        hr_validation_error_set(error, "options",
                                "Single answer questions cannot have multiple correct options");
        return false;
    }
    return true;
}

static bool hr_card_short_answer_validate(const HrCardShortAnswerExtras* extras,
                                          HrValidationError*             error) {
    if (!extras) {
        hr_validation_error_set(error, "extras", "Short answer extras missing");
        return false;
    }
    (void)extras; /* defaults are always valid */
    return true;
}

static bool hr_card_cloze_validate(const HrCardClozeExtras* extras, HrValidationError* error) {
    if (!extras) {
        hr_validation_error_set(error, "extras", "Cloze extras missing");
        return false;
    }
    if (extras->cloze_count == 0) {
        hr_validation_error_set(error, "extras", "Cloze cards require at least one blank");
        return false;
    }
    return true;
}

static bool hr_card_true_false_validate(const HrCardTrueFalseExtras* extras,
                                        HrValidationError*           error) {
    if (!extras) {
        hr_validation_error_set(error, "extras", "True/False extras missing");
        return false;
    }
    (void)extras;
    return true;
}

static bool hr_card_image_validate(const HrCardImageOcclusionExtras* extras,
                                   HrValidationError*                error) {
    if (!extras) {
        hr_validation_error_set(error, "extras", "Image occlusion extras missing");
        return false;
    }
    if (!extras->image_uuid || hr_string_is_blank(extras->image_uuid)) {
        hr_validation_error_set(error, "image_uuid",
                                "Image occlusion cards must reference an image");
        return false;
    }
    if (extras->mask_count == 0) {
        hr_validation_error_set(error, "mask_count",
                                "Image occlusion cards require at least one mask");
        return false;
    }
    return true;
}

static bool hr_card_audio_validate(const HrCardAudioExtras* extras, HrValidationError* error) {
    if (!extras) {
        hr_validation_error_set(error, "extras", "Audio recall extras missing");
        return false;
    }
    if (!extras->audio_uuid || hr_string_is_blank(extras->audio_uuid)) {
        hr_validation_error_set(error, "audio_uuid",
                                "Audio recall cards must reference an audio asset");
        return false;
    }
    if (extras->require_transcript && hr_string_is_blank(extras->reference_text)) {
        hr_validation_error_set(error, "reference_text",
                                "A transcript is required when transcription is enforced");
        return false;
    }
    return true;
}

static bool hr_card_typing_validate(const HrCardTypingExtras* extras, HrValidationError* error) {
    if (!extras) {
        hr_validation_error_set(error, "extras", "Typing extras missing");
        return false;
    }
    if (!extras->regex_pattern || hr_string_is_blank(extras->regex_pattern)) {
        hr_validation_error_set(error, "regex_pattern", "Typing cards require a regex pattern");
        return false;
    }
    return true;
}

static bool hr_card_ordering_validate(const HrCardOrderingExtras* extras,
                                      HrValidationError*          error) {
    if (!extras) {
        hr_validation_error_set(error, "extras", "Ordering extras missing");
        return false;
    }
    if (!extras->items || extras->item_count < 2) {
        hr_validation_error_set(error, "items", "Ordering cards require at least two items");
        return false;
    }
    for (size_t i = 0; i < extras->item_count; ++i) {
        const HrCardOrderingItem* item = &extras->items[i];
        if (!item->value || hr_string_is_blank(item->value)) {
            hr_validation_error_set(error, "items", "Ordering item text cannot be blank");
            return false;
        }
        if (item->correct_position < 0 || (size_t)item->correct_position >= extras->item_count) {
            hr_validation_error_set(error, "items", "Ordering item has invalid position");
            return false;
        }
    }
    return true;
}

static bool hr_card_matching_validate(const HrCardMatchingExtras* extras,
                                      HrValidationError*          error) {
    if (!extras) {
        hr_validation_error_set(error, "extras", "Matching extras missing");
        return false;
    }
    if (!extras->pairs || extras->pair_count < 2) {
        hr_validation_error_set(error, "pairs", "Matching cards require at least two pairs");
        return false;
    }
    for (size_t i = 0; i < extras->pair_count; ++i) {
        const HrCardMatchingPair* pair = &extras->pairs[i];
        if (!pair->left || hr_string_is_blank(pair->left)) {
            hr_validation_error_set(error, "pairs", "Matching pair left side cannot be blank");
            return false;
        }
        if (!pair->right || hr_string_is_blank(pair->right)) {
            hr_validation_error_set(error, "pairs", "Matching pair right side cannot be blank");
            return false;
        }
    }
    return true;
}

bool hr_card_extras_validate(const HrCardExtras* extras, HrValidationError* error) {
    if (!extras) {
        hr_validation_error_set(error, "extras", "Card extras missing");
        return false;
    }
    switch (extras->type) {
    case HR_CARD_TYPE_SHORT_ANSWER:
        return hr_card_short_answer_validate(&extras->data.short_answer, error);
    case HR_CARD_TYPE_CLOZE:
        return hr_card_cloze_validate(&extras->data.cloze, error);
    case HR_CARD_TYPE_MULTIPLE_CHOICE_SINGLE:
    case HR_CARD_TYPE_MULTIPLE_CHOICE_MULTI:
        return hr_card_choice_options_validate(&extras->data.choice, error);
    case HR_CARD_TYPE_TRUE_FALSE:
        return hr_card_true_false_validate(&extras->data.true_false, error);
    case HR_CARD_TYPE_IMAGE_OCCLUSION:
        return hr_card_image_validate(&extras->data.image, error);
    case HR_CARD_TYPE_AUDIO_RECALL:
        return hr_card_audio_validate(&extras->data.audio, error);
    case HR_CARD_TYPE_TYPING:
        return hr_card_typing_validate(&extras->data.typing, error);
    case HR_CARD_TYPE_ORDERING:
        return hr_card_ordering_validate(&extras->data.ordering, error);
    case HR_CARD_TYPE_MATCHING:
        return hr_card_matching_validate(&extras->data.matching, error);
    default:
        hr_validation_error_set(error, "extras", "Unsupported card type for extras validation");
        return false;
    }
}

bool hr_card_media_list_validate(const HrCardMediaList* media, HrValidationError* error) {
    if (!media) {
        hr_validation_error_set(error, "media", "Media list missing");
        return false;
    }
    if (media->count > 0 && !media->items) {
        hr_validation_error_set(error, "media", "Media entries missing");
        return false;
    }
    for (size_t i = 0; i < media->count; ++i) {
        const HrCardMediaLink* link = &media->items[i];
        if (!link->identifier || hr_string_is_blank(link->identifier)) {
            hr_validation_error_set(error, "media", "Media identifier cannot be blank");
            return false;
        }
        if (link->type < HR_MEDIA_TYPE_IMAGE || link->type > HR_MEDIA_TYPE_LATEX) {
            hr_validation_error_set(error, "media", "Invalid media type");
            return false;
        }
    }
    return true;
}

bool hr_card_payload_validate(const HrCardPayload* payload, HrValidationError* error) {
    if (!payload) {
        hr_validation_error_set(error, "payload", "Payload missing");
        return false;
    }
    if (payload->extras.type != payload->type) {
        hr_validation_error_set(error, "type", "Payload extras do not match the card type");
        return false;
    }
    if (hr_string_is_blank(payload->prompt)) {
        hr_validation_error_set(error, "prompt", "Prompt cannot be empty");
        return false;
    }
    if (hr_string_is_blank(payload->response)) {
        hr_validation_error_set(error, "response", "Response cannot be empty");
        return false;
    }
    if (!hr_card_extras_validate(&payload->extras, error)) {
        return false;
    }
    if (!hr_card_media_list_validate(&payload->media, error)) {
        return false;
    }
    return true;
}

bool hr_card_validate(const HrCard* card, HrValidationError* error) {
    if (!card) {
        hr_validation_error_set(error, "card", "Card missing");
        return false;
    }
    if (card->topic_id <= 0) {
        hr_validation_error_set(error, "topic_id", "Card must belong to a topic");
        return false;
    }
    if (hr_string_is_blank(card->prompt)) {
        hr_validation_error_set(error, "prompt", "Prompt cannot be empty");
        return false;
    }
    if (hr_string_is_blank(card->response)) {
        hr_validation_error_set(error, "response", "Response cannot be empty");
        return false;
    }
    if (!hr_card_extras_validate(&card->extras, error)) {
        return false;
    }
    if (!hr_card_media_list_validate(&card->media, error)) {
        return false;
    }
    return true;
}

void hr_card_from_record(HrCard* card, const HrCardRecord* record, HrCardType type) {
    if (!card || !record) {
        return;
    }
    card->id = record->id;
    card->topic_id = record->topic_id;
    card->uuid = record->uuid;
    card->type = type;
    card->prompt = record->prompt;
    card->response = record->response;
    card->mnemonic = record->mnemonic;
    card->created_at = record->created_at;
    card->updated_at = record->updated_at;
    card->due_at = record->due_at;
    card->interval = record->interval;
    card->ease_factor = record->ease_factor;
    card->review_state = record->review_state;
    card->suspended = record->suspended;
    hr_card_extras_init(&card->extras, type);
    card->media.items = NULL;
    card->media.count = 0;
}

void hr_card_to_record(HrCardRecord* record, const HrCard* card) {
    if (!record || !card) {
        return;
    }
    record->id = card->id;
    record->topic_id = card->topic_id;
    record->uuid = card->uuid;
    record->prompt = card->prompt;
    record->response = card->response;
    record->mnemonic = card->mnemonic;
    record->created_at = card->created_at;
    record->updated_at = card->updated_at;
    record->due_at = card->due_at;
    record->interval = card->interval;
    record->ease_factor = card->ease_factor;
    record->review_state = card->review_state;
    record->suspended = card->suspended;
}

void hr_card_payload_from_card(HrCardPayload* payload, const HrCard* card) {
    if (!payload || !card) {
        return;
    }
    payload->type = card->type;
    payload->prompt = card->prompt;
    payload->response = card->response;
    payload->mnemonic = card->mnemonic;
    payload->extras = card->extras;
    payload->media = card->media;
}

bool hr_card_apply_payload(HrCard* card, const HrCardPayload* payload, HrValidationError* error) {
    if (!card) {
        hr_validation_error_set(error, "card", "Card missing");
        return false;
    }
    if (!hr_card_payload_validate(payload, error)) {
        return false;
    }
    card->type = payload->type;
    card->prompt = payload->prompt;
    card->response = payload->response;
    card->mnemonic = payload->mnemonic;
    card->extras = payload->extras;
    card->media = payload->media;
    return true;
}

void hr_topic_from_record(HrTopic* topic, const HrTopicRecord* record) {
    if (!topic || !record) {
        return;
    }
    topic->id = record->id;
    topic->parent_id = record->parent_id;
    topic->uuid = record->uuid;
    topic->title = record->title;
    topic->summary = record->summary;
    topic->created_at = record->created_at;
    topic->updated_at = record->updated_at;
    topic->position = record->position;
}

void hr_topic_to_record(HrTopicRecord* record, const HrTopic* topic) {
    if (!record || !topic) {
        return;
    }
    record->id = topic->id;
    record->parent_id = topic->parent_id;
    record->uuid = topic->uuid;
    record->title = topic->title;
    record->summary = topic->summary;
    record->created_at = topic->created_at;
    record->updated_at = topic->updated_at;
    record->position = topic->position;
}

bool hr_topic_payload_validate(const HrTopicPayload* payload, HrValidationError* error) {
    if (!payload) {
        hr_validation_error_set(error, "payload", "Payload missing");
        return false;
    }
    if (hr_string_is_blank(payload->title)) {
        hr_validation_error_set(error, "title", "Title cannot be empty");
        return false;
    }
    return true;
}

bool hr_topic_apply_payload(HrTopic* topic, const HrTopicPayload* payload,
                            HrValidationError* error) {
    if (!topic) {
        hr_validation_error_set(error, "topic", "Topic missing");
        return false;
    }
    if (!hr_topic_payload_validate(payload, error)) {
        return false;
    }
    topic->title = payload->title;
    topic->summary = payload->summary;
    return true;
}
