#ifndef HYPERRECALL_MODEL_H
#define HYPERRECALL_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file model.h
 * @brief Declares domain models representing study items, decks, and user progress.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "db.h"

/**
 * @brief Represents validation errors produced while converting or validating
 * domain objects.
 */
typedef struct HrValidationError {
    const char* field;
    char        message[128];
} HrValidationError;

typedef enum HrCardType {
    HR_CARD_TYPE_SHORT_ANSWER = 0,
    HR_CARD_TYPE_CLOZE,
    HR_CARD_TYPE_MULTIPLE_CHOICE_SINGLE,
    HR_CARD_TYPE_MULTIPLE_CHOICE_MULTI,
    HR_CARD_TYPE_TRUE_FALSE,
    HR_CARD_TYPE_IMAGE_OCCLUSION,
    HR_CARD_TYPE_AUDIO_RECALL,
    HR_CARD_TYPE_TYPING,
    HR_CARD_TYPE_ORDERING,
    HR_CARD_TYPE_MATCHING,
    HR_CARD_TYPE_CODE_OUTPUT,
    HR_CARD_TYPE_DEBUG_FIX,
    HR_CARD_TYPE_COMPARE,
} HrCardType;

typedef enum HrMediaType {
    HR_MEDIA_TYPE_IMAGE = 0,
    HR_MEDIA_TYPE_AUDIO,
    HR_MEDIA_TYPE_VIDEO,
    HR_MEDIA_TYPE_LATEX,
} HrMediaType;

typedef struct HrCardShortAnswerExtras {
    bool        case_sensitive;
    const char* alternate_answer;
    bool        strip_whitespace;
} HrCardShortAnswerExtras;

typedef struct HrCardClozeExtras {
    size_t cloze_count;
    bool   strict;
} HrCardClozeExtras;

typedef struct HrCardChoiceOption {
    const char* value;
    bool        correct;
} HrCardChoiceOption;

typedef struct HrCardChoiceExtras {
    const HrCardChoiceOption* options;
    size_t                    option_count;
    bool                      allow_multiple;
    bool                      shuffle;
} HrCardChoiceExtras;

typedef struct HrCardTrueFalseExtras {
    bool        answer_true;
    const char* explanation;
} HrCardTrueFalseExtras;

typedef struct HrCardImageOcclusionExtras {
    const char* image_uuid;
    size_t      mask_count;
    bool        require_order;
} HrCardImageOcclusionExtras;

typedef struct HrCardAudioExtras {
    const char* audio_uuid;
    bool        require_transcript;
    const char* reference_text;
} HrCardAudioExtras;

typedef struct HrCardTypingExtras {
    const char* regex_pattern;
    bool        case_sensitive;
    const char* sample_answer;
} HrCardTypingExtras;

typedef struct HrCardOrderingItem {
    const char* value;
    int         correct_position;
} HrCardOrderingItem;

typedef struct HrCardOrderingExtras {
    const HrCardOrderingItem* items;
    size_t                    item_count;
    bool                      partial_credit;
} HrCardOrderingExtras;

typedef struct HrCardMatchingPair {
    const char* left;
    const char* right;
} HrCardMatchingPair;

typedef struct HrCardMatchingExtras {
    const HrCardMatchingPair* pairs;
    size_t                    pair_count;
    bool                      shuffle_right;
} HrCardMatchingExtras;

typedef struct HrCardCodeOutputExtras {
    const char* code;
    const char* language;
    const char* expected_output;
    bool        ignore_whitespace;
} HrCardCodeOutputExtras;

typedef struct HrCardDebugFixExtras {
    const char* buggy_code;
    const char* language;
    const char* error_description;
    const char* fixed_code;
} HrCardDebugFixExtras;

typedef struct HrCardCompareExtras {
    const char* item_a;
    const char* item_b;
    const char* aspect;
    const char* expected_comparison;
} HrCardCompareExtras;

typedef struct HrCardExtras {
    HrCardType type;
    union {
        HrCardShortAnswerExtras    short_answer;
        HrCardClozeExtras          cloze;
        HrCardChoiceExtras         choice;
        HrCardTrueFalseExtras      true_false;
        HrCardImageOcclusionExtras image;
        HrCardAudioExtras          audio;
        HrCardTypingExtras         typing;
        HrCardOrderingExtras       ordering;
        HrCardMatchingExtras       matching;
        HrCardCodeOutputExtras     code_output;
        HrCardDebugFixExtras       debug_fix;
        HrCardCompareExtras        compare;
    } data;
} HrCardExtras;

typedef struct HrCardMediaLink {
    HrMediaType type;
    const char* identifier;
    const char* meta;
} HrCardMediaLink;

typedef struct HrCardMediaList {
    const HrCardMediaLink* items;
    size_t                 count;
} HrCardMediaList;

typedef struct HrCardPayload {
    HrCardType      type;
    const char*     prompt;
    const char*     response;
    const char*     mnemonic;
    HrCardExtras    extras;
    HrCardMediaList media;
} HrCardPayload;

typedef struct HrCard {
    sqlite3_int64   id;
    sqlite3_int64   topic_id;
    const char*     uuid;
    HrCardType      type;
    const char*     prompt;
    const char*     response;
    const char*     mnemonic;
    sqlite3_int64   created_at;
    sqlite3_int64   updated_at;
    sqlite3_int64   due_at;
    int             interval;
    int             ease_factor;
    int             review_state;
    bool            suspended;
    HrCardExtras    extras;
    HrCardMediaList media;
} HrCard;

typedef struct HrTopic {
    sqlite3_int64 id;
    sqlite3_int64 parent_id;
    const char*   uuid;
    const char*   title;
    const char*   summary;
    sqlite3_int64 created_at;
    sqlite3_int64 updated_at;
    int           position;
} HrTopic;

typedef struct HrTopicPayload {
    const char* title;
    const char* summary;
} HrTopicPayload;

const char* hr_card_type_to_string(HrCardType type);

bool hr_card_type_from_string(const char* text, HrCardType* out_type);

const char* hr_media_type_to_string(HrMediaType type);

bool hr_media_type_from_string(const char* text, HrMediaType* out_type);

void hr_card_extras_init(HrCardExtras* extras, HrCardType type);

bool hr_card_extras_validate(const HrCardExtras* extras, HrValidationError* error);

bool hr_card_media_list_validate(const HrCardMediaList* media, HrValidationError* error);

bool hr_card_payload_validate(const HrCardPayload* payload, HrValidationError* error);

bool hr_card_validate(const HrCard* card, HrValidationError* error);

void hr_card_from_record(HrCard* card, const HrCardRecord* record, HrCardType type);

void hr_card_to_record(HrCardRecord* record, const HrCard* card);

void hr_card_payload_from_card(HrCardPayload* payload, const HrCard* card);

bool hr_card_apply_payload(HrCard* card, const HrCardPayload* payload, HrValidationError* error);

void hr_topic_from_record(HrTopic* topic, const HrTopicRecord* record);

void hr_topic_to_record(HrTopicRecord* record, const HrTopic* topic);

bool hr_topic_payload_validate(const HrTopicPayload* payload, HrValidationError* error);

bool hr_topic_apply_payload(HrTopic* topic, const HrTopicPayload* payload,
                            HrValidationError* error);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_MODEL_H */
