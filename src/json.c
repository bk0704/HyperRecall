/* Enable POSIX extensions for strdup */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "json.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Internal JSON value structure.
 */
struct HrJsonValue {
    HrJsonType type;
    union {
        bool bool_val;
        double number_val;
        char *string_val;
        struct {
            HrJsonValue **elements;
            size_t count;
            size_t capacity;
        } array_val;
        struct {
            char **keys;
            HrJsonValue **values;
            size_t count;
            size_t capacity;
        } object_val;
    } data;
};

/**
 * Parser state.
 */
typedef struct {
    const char *json;
    const char *ptr;
    char error[256];
} HrJsonParser;

static void skip_whitespace(HrJsonParser *parser)
{
    while (*parser->ptr && isspace((unsigned char)*parser->ptr)) {
        parser->ptr++;
    }
}

static bool parse_value(HrJsonParser *parser, HrJsonValue **out_value);

static bool parse_string(HrJsonParser *parser, char **out_string)
{
    if (*parser->ptr != '"') {
        snprintf(parser->error, sizeof(parser->error), "Expected '\"'");
        return false;
    }
    parser->ptr++; /* skip opening quote */

    const char *start = parser->ptr;
    size_t length = 0;
    while (*parser->ptr && *parser->ptr != '"') {
        if (*parser->ptr == '\\') {
            parser->ptr++; /* skip escape */
            if (!*parser->ptr) {
                snprintf(parser->error, sizeof(parser->error), "Unterminated string");
                return false;
            }
        }
        parser->ptr++;
        length++;
    }

    if (*parser->ptr != '"') {
        snprintf(parser->error, sizeof(parser->error), "Unterminated string");
        return false;
    }

    char *str = (char *)malloc(length + 1);
    if (!str) {
        snprintf(parser->error, sizeof(parser->error), "Out of memory");
        return false;
    }

    /* Simple copy without escape processing for now */
    size_t j = 0;
    const char *p = start;
    while (p < parser->ptr) {
        if (*p == '\\' && (p + 1) < parser->ptr) {
            p++; /* skip backslash */
            switch (*p) {
            case 'n':
                str[j++] = '\n';
                break;
            case 't':
                str[j++] = '\t';
                break;
            case 'r':
                str[j++] = '\r';
                break;
            case '"':
            case '\\':
            case '/':
                str[j++] = *p;
                break;
            default:
                str[j++] = *p; /* pass through unknown escapes */
                break;
            }
            p++;
        } else {
            str[j++] = *p++;
        }
    }
    str[j] = '\0';

    parser->ptr++; /* skip closing quote */
    *out_string = str;
    return true;
}

static bool parse_number(HrJsonParser *parser, double *out_number)
{
    char *end;
    *out_number = strtod(parser->ptr, &end);
    if (end == parser->ptr) {
        snprintf(parser->error, sizeof(parser->error), "Invalid number");
        return false;
    }
    parser->ptr = end;
    return true;
}

static bool parse_array(HrJsonParser *parser, HrJsonValue **out_value)
{
    if (*parser->ptr != '[') {
        snprintf(parser->error, sizeof(parser->error), "Expected '['");
        return false;
    }
    parser->ptr++;

    HrJsonValue *array = hr_json_array_new();
    if (!array) {
        snprintf(parser->error, sizeof(parser->error), "Out of memory");
        return false;
    }

    skip_whitespace(parser);
    if (*parser->ptr == ']') {
        parser->ptr++;
        *out_value = array;
        return true;
    }

    for (;;) {
        HrJsonValue *element = NULL;
        if (!parse_value(parser, &element)) {
            hr_json_free(array);
            return false;
        }
        if (!hr_json_array_append(array, element)) {
            hr_json_free(element);
            hr_json_free(array);
            snprintf(parser->error, sizeof(parser->error), "Out of memory");
            return false;
        }

        skip_whitespace(parser);
        if (*parser->ptr == ',') {
            parser->ptr++;
            skip_whitespace(parser);
        } else if (*parser->ptr == ']') {
            parser->ptr++;
            break;
        } else {
            hr_json_free(array);
            snprintf(parser->error, sizeof(parser->error), "Expected ',' or ']'");
            return false;
        }
    }

    *out_value = array;
    return true;
}

static bool parse_object(HrJsonParser *parser, HrJsonValue **out_value)
{
    if (*parser->ptr != '{') {
        snprintf(parser->error, sizeof(parser->error), "Expected '{'");
        return false;
    }
    parser->ptr++;

    HrJsonValue *object = hr_json_object_new();
    if (!object) {
        snprintf(parser->error, sizeof(parser->error), "Out of memory");
        return false;
    }

    skip_whitespace(parser);
    if (*parser->ptr == '}') {
        parser->ptr++;
        *out_value = object;
        return true;
    }

    for (;;) {
        skip_whitespace(parser);
        char *key = NULL;
        if (!parse_string(parser, &key)) {
            hr_json_free(object);
            return false;
        }

        skip_whitespace(parser);
        if (*parser->ptr != ':') {
            free(key);
            hr_json_free(object);
            snprintf(parser->error, sizeof(parser->error), "Expected ':'");
            return false;
        }
        parser->ptr++;
        skip_whitespace(parser);

        HrJsonValue *value = NULL;
        if (!parse_value(parser, &value)) {
            free(key);
            hr_json_free(object);
            return false;
        }

        if (!hr_json_object_set(object, key, value)) {
            free(key);
            hr_json_free(value);
            hr_json_free(object);
            snprintf(parser->error, sizeof(parser->error), "Out of memory");
            return false;
        }
        free(key);

        skip_whitespace(parser);
        if (*parser->ptr == ',') {
            parser->ptr++;
        } else if (*parser->ptr == '}') {
            parser->ptr++;
            break;
        } else {
            hr_json_free(object);
            snprintf(parser->error, sizeof(parser->error), "Expected ',' or '}'");
            return false;
        }
    }

    *out_value = object;
    return true;
}

static bool parse_value(HrJsonParser *parser, HrJsonValue **out_value)
{
    skip_whitespace(parser);

    if (!*parser->ptr) {
        snprintf(parser->error, sizeof(parser->error), "Unexpected end of input");
        return false;
    }

    /* null */
    if (strncmp(parser->ptr, "null", 4) == 0) {
        parser->ptr += 4;
        *out_value = hr_json_null_new();
        return *out_value != NULL;
    }

    /* true */
    if (strncmp(parser->ptr, "true", 4) == 0) {
        parser->ptr += 4;
        *out_value = hr_json_bool_new(true);
        return *out_value != NULL;
    }

    /* false */
    if (strncmp(parser->ptr, "false", 5) == 0) {
        parser->ptr += 5;
        *out_value = hr_json_bool_new(false);
        return *out_value != NULL;
    }

    /* string */
    if (*parser->ptr == '"') {
        char *str = NULL;
        if (!parse_string(parser, &str)) {
            return false;
        }
        *out_value = hr_json_string_new(str);
        free(str);
        return *out_value != NULL;
    }

    /* array */
    if (*parser->ptr == '[') {
        return parse_array(parser, out_value);
    }

    /* object */
    if (*parser->ptr == '{') {
        return parse_object(parser, out_value);
    }

    /* number */
    if (*parser->ptr == '-' || isdigit((unsigned char)*parser->ptr)) {
        double num;
        if (!parse_number(parser, &num)) {
            return false;
        }
        *out_value = hr_json_number_new(num);
        return *out_value != NULL;
    }

    snprintf(parser->error, sizeof(parser->error), "Unexpected character '%c'", *parser->ptr);
    return false;
}

HrJsonValue *hr_json_parse(const char *json_text)
{
    if (!json_text) {
        return NULL;
    }

    HrJsonParser parser = {.json = json_text, .ptr = json_text, .error = {0}};
    HrJsonValue *value = NULL;

    if (!parse_value(&parser, &value)) {
        fprintf(stderr, "JSON parse error: %s\n", parser.error);
        return NULL;
    }

    return value;
}

void hr_json_free(HrJsonValue *value)
{
    if (!value) {
        return;
    }

    switch (value->type) {
    case HR_JSON_STRING:
        free(value->data.string_val);
        break;
    case HR_JSON_ARRAY:
        for (size_t i = 0; i < value->data.array_val.count; i++) {
            hr_json_free(value->data.array_val.elements[i]);
        }
        free(value->data.array_val.elements);
        break;
    case HR_JSON_OBJECT:
        for (size_t i = 0; i < value->data.object_val.count; i++) {
            free(value->data.object_val.keys[i]);
            hr_json_free(value->data.object_val.values[i]);
        }
        free(value->data.object_val.keys);
        free(value->data.object_val.values);
        break;
    default:
        break;
    }

    free(value);
}

HrJsonType hr_json_type(const HrJsonValue *value)
{
    return value ? value->type : HR_JSON_NULL;
}

bool hr_json_get_bool(const HrJsonValue *value, bool *out_bool)
{
    if (!value || value->type != HR_JSON_BOOL || !out_bool) {
        return false;
    }
    *out_bool = value->data.bool_val;
    return true;
}

bool hr_json_get_number(const HrJsonValue *value, double *out_number)
{
    if (!value || value->type != HR_JSON_NUMBER || !out_number) {
        return false;
    }
    *out_number = value->data.number_val;
    return true;
}

const char *hr_json_get_string(const HrJsonValue *value)
{
    if (!value || value->type != HR_JSON_STRING) {
        return NULL;
    }
    return value->data.string_val;
}

size_t hr_json_array_size(const HrJsonValue *value)
{
    if (!value || value->type != HR_JSON_ARRAY) {
        return 0;
    }
    return value->data.array_val.count;
}

const HrJsonValue *hr_json_array_get(const HrJsonValue *value, size_t index)
{
    if (!value || value->type != HR_JSON_ARRAY || index >= value->data.array_val.count) {
        return NULL;
    }
    return value->data.array_val.elements[index];
}

const HrJsonValue *hr_json_object_get(const HrJsonValue *value, const char *key)
{
    if (!value || value->type != HR_JSON_OBJECT || !key) {
        return NULL;
    }

    for (size_t i = 0; i < value->data.object_val.count; i++) {
        if (strcmp(value->data.object_val.keys[i], key) == 0) {
            return value->data.object_val.values[i];
        }
    }
    return NULL;
}

bool hr_json_object_has(const HrJsonValue *value, const char *key)
{
    return hr_json_object_get(value, key) != NULL;
}

HrJsonValue *hr_json_object_new(void)
{
    HrJsonValue *value = (HrJsonValue *)calloc(1, sizeof(HrJsonValue));
    if (!value) {
        return NULL;
    }
    value->type = HR_JSON_OBJECT;
    value->data.object_val.capacity = 16;
    value->data.object_val.keys = (char **)calloc(16, sizeof(char *));
    value->data.object_val.values = (HrJsonValue **)calloc(16, sizeof(HrJsonValue *));
    if (!value->data.object_val.keys || !value->data.object_val.values) {
        free(value->data.object_val.keys);
        free(value->data.object_val.values);
        free(value);
        return NULL;
    }
    return value;
}

HrJsonValue *hr_json_array_new(void)
{
    HrJsonValue *value = (HrJsonValue *)calloc(1, sizeof(HrJsonValue));
    if (!value) {
        return NULL;
    }
    value->type = HR_JSON_ARRAY;
    value->data.array_val.capacity = 16;
    value->data.array_val.elements = (HrJsonValue **)calloc(16, sizeof(HrJsonValue *));
    if (!value->data.array_val.elements) {
        free(value);
        return NULL;
    }
    return value;
}

HrJsonValue *hr_json_string_new(const char *str)
{
    if (!str) {
        return NULL;
    }
    HrJsonValue *value = (HrJsonValue *)calloc(1, sizeof(HrJsonValue));
    if (!value) {
        return NULL;
    }
    value->type = HR_JSON_STRING;
#if defined(_WIN32)
    value->data.string_val = _strdup(str);
#else
    value->data.string_val = strdup(str);
#endif
    if (!value->data.string_val) {
        free(value);
        return NULL;
    }
    return value;
}

HrJsonValue *hr_json_number_new(double number)
{
    HrJsonValue *value = (HrJsonValue *)calloc(1, sizeof(HrJsonValue));
    if (!value) {
        return NULL;
    }
    value->type = HR_JSON_NUMBER;
    value->data.number_val = number;
    return value;
}

HrJsonValue *hr_json_bool_new(bool bool_val)
{
    HrJsonValue *value = (HrJsonValue *)calloc(1, sizeof(HrJsonValue));
    if (!value) {
        return NULL;
    }
    value->type = HR_JSON_BOOL;
    value->data.bool_val = bool_val;
    return value;
}

HrJsonValue *hr_json_null_new(void)
{
    HrJsonValue *value = (HrJsonValue *)calloc(1, sizeof(HrJsonValue));
    if (!value) {
        return NULL;
    }
    value->type = HR_JSON_NULL;
    return value;
}

bool hr_json_object_set(HrJsonValue *object, const char *key, HrJsonValue *value)
{
    if (!object || object->type != HR_JSON_OBJECT || !key || !value) {
        return false;
    }

    /* Check if key exists */
    for (size_t i = 0; i < object->data.object_val.count; i++) {
        if (strcmp(object->data.object_val.keys[i], key) == 0) {
            hr_json_free(object->data.object_val.values[i]);
            object->data.object_val.values[i] = value;
            return true;
        }
    }

    /* Grow if needed */
    if (object->data.object_val.count >= object->data.object_val.capacity) {
        size_t new_capacity = object->data.object_val.capacity * 2;
        char **new_keys = (char **)realloc(object->data.object_val.keys, new_capacity * sizeof(char *));
        HrJsonValue **new_values =
            (HrJsonValue **)realloc(object->data.object_val.values, new_capacity * sizeof(HrJsonValue *));
        if (!new_keys || !new_values) {
            return false;
        }
        object->data.object_val.keys = new_keys;
        object->data.object_val.values = new_values;
        object->data.object_val.capacity = new_capacity;
    }

    char *key_copy = strdup(key);
    if (!key_copy) {
        return false;
    }

    object->data.object_val.keys[object->data.object_val.count] = key_copy;
    object->data.object_val.values[object->data.object_val.count] = value;
    object->data.object_val.count++;
    return true;
}

bool hr_json_array_append(HrJsonValue *array, HrJsonValue *value)
{
    if (!array || array->type != HR_JSON_ARRAY || !value) {
        return false;
    }

    if (array->data.array_val.count >= array->data.array_val.capacity) {
        size_t new_capacity = array->data.array_val.capacity * 2;
        HrJsonValue **new_elements =
            (HrJsonValue **)realloc(array->data.array_val.elements, new_capacity * sizeof(HrJsonValue *));
        if (!new_elements) {
            return false;
        }
        array->data.array_val.elements = new_elements;
        array->data.array_val.capacity = new_capacity;
    }

    array->data.array_val.elements[array->data.array_val.count++] = value;
    return true;
}

static void serialize_string(const char *str, char **out, size_t *out_len, size_t *out_cap)
{
    size_t needed = *out_len + strlen(str) * 6 + 3; /* worst case with unicode escapes + quotes */
    if (needed > *out_cap) {
        size_t new_cap = *out_cap * 2;
        while (new_cap < needed) {
            new_cap *= 2;
        }
        char *new_out = (char *)realloc(*out, new_cap);
        if (!new_out) {
            return;
        }
        *out = new_out;
        *out_cap = new_cap;
    }

    (*out)[(*out_len)++] = '"';
    while (*str) {
        unsigned char c = (unsigned char)*str++;
        switch (c) {
            case '"':
            case '\\':
                (*out)[(*out_len)++] = '\\';
                (*out)[(*out_len)++] = (char)c;
                break;
            case '\b':
                (*out)[(*out_len)++] = '\\';
                (*out)[(*out_len)++] = 'b';
                break;
            case '\f':
                (*out)[(*out_len)++] = '\\';
                (*out)[(*out_len)++] = 'f';
                break;
            case '\n':
                (*out)[(*out_len)++] = '\\';
                (*out)[(*out_len)++] = 'n';
                break;
            case '\r':
                (*out)[(*out_len)++] = '\\';
                (*out)[(*out_len)++] = 'r';
                break;
            case '\t':
                (*out)[(*out_len)++] = '\\';
                (*out)[(*out_len)++] = 't';
                break;
            default: {
                if (c < 0x20) {
                    static const char hex_digits[] = "0123456789ABCDEF";
                    (*out)[(*out_len)++] = '\\';
                    (*out)[(*out_len)++] = 'u';
                    (*out)[(*out_len)++] = '0';
                    (*out)[(*out_len)++] = '0';
                    (*out)[(*out_len)++] = hex_digits[(c >> 4) & 0xF];
                    (*out)[(*out_len)++] = hex_digits[c & 0xF];
                } else {
                    (*out)[(*out_len)++] = (char)c;
                }
                break;
            }
        }
    }
    (*out)[(*out_len)++] = '"';
}

static void serialize_value(const HrJsonValue *value, char **out, size_t *out_len, size_t *out_cap, int indent,
                            bool pretty);

static void append_indent(char **out, size_t *out_len, size_t *out_cap, int indent)
{
    size_t needed = *out_len + (size_t)indent * 2 + 1;
    if (needed > *out_cap) {
        size_t new_cap = *out_cap * 2;
        while (new_cap < needed) {
            new_cap *= 2;
        }
        char *new_out = (char *)realloc(*out, new_cap);
        if (!new_out) {
            return;
        }
        *out = new_out;
        *out_cap = new_cap;
    }
    for (int i = 0; i < indent; i++) {
        (*out)[(*out_len)++] = ' ';
        (*out)[(*out_len)++] = ' ';
    }
}

static void append_char(char **out, size_t *out_len, size_t *out_cap, char c)
{
    if (*out_len + 1 >= *out_cap) {
        size_t new_cap = *out_cap * 2;
        char *new_out = (char *)realloc(*out, new_cap);
        if (!new_out) {
            return;
        }
        *out = new_out;
        *out_cap = new_cap;
    }
    (*out)[(*out_len)++] = c;
}

static void append_str(char **out, size_t *out_len, size_t *out_cap, const char *str)
{
    size_t len = strlen(str);
    size_t needed = *out_len + len + 1;
    if (needed > *out_cap) {
        size_t new_cap = *out_cap * 2;
        while (new_cap < needed) {
            new_cap *= 2;
        }
        char *new_out = (char *)realloc(*out, new_cap);
        if (!new_out) {
            return;
        }
        *out = new_out;
        *out_cap = new_cap;
    }
    memcpy(*out + *out_len, str, len);
    *out_len += len;
}

static void serialize_value(const HrJsonValue *value, char **out, size_t *out_len, size_t *out_cap, int indent,
                            bool pretty)
{
    if (!value) {
        return;
    }

    switch (value->type) {
    case HR_JSON_NULL:
        append_str(out, out_len, out_cap, "null");
        break;
    case HR_JSON_BOOL:
        append_str(out, out_len, out_cap, value->data.bool_val ? "true" : "false");
        break;
    case HR_JSON_NUMBER: {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.17g", value->data.number_val);
        append_str(out, out_len, out_cap, buf);
        break;
    }
    case HR_JSON_STRING:
        serialize_string(value->data.string_val, out, out_len, out_cap);
        break;
    case HR_JSON_ARRAY:
        append_char(out, out_len, out_cap, '[');
        if (pretty && value->data.array_val.count > 0) {
            append_char(out, out_len, out_cap, '\n');
        }
        for (size_t i = 0; i < value->data.array_val.count; i++) {
            if (pretty) {
                append_indent(out, out_len, out_cap, indent + 1);
            }
            serialize_value(value->data.array_val.elements[i], out, out_len, out_cap, indent + 1, pretty);
            if (i < value->data.array_val.count - 1) {
                append_char(out, out_len, out_cap, ',');
            }
            if (pretty) {
                append_char(out, out_len, out_cap, '\n');
            }
        }
        if (pretty && value->data.array_val.count > 0) {
            append_indent(out, out_len, out_cap, indent);
        }
        append_char(out, out_len, out_cap, ']');
        break;
    case HR_JSON_OBJECT:
        append_char(out, out_len, out_cap, '{');
        if (pretty && value->data.object_val.count > 0) {
            append_char(out, out_len, out_cap, '\n');
        }
        for (size_t i = 0; i < value->data.object_val.count; i++) {
            if (pretty) {
                append_indent(out, out_len, out_cap, indent + 1);
            }
            serialize_string(value->data.object_val.keys[i], out, out_len, out_cap);
            append_char(out, out_len, out_cap, ':');
            if (pretty) {
                append_char(out, out_len, out_cap, ' ');
            }
            serialize_value(value->data.object_val.values[i], out, out_len, out_cap, indent + 1, pretty);
            if (i < value->data.object_val.count - 1) {
                append_char(out, out_len, out_cap, ',');
            }
            if (pretty) {
                append_char(out, out_len, out_cap, '\n');
            }
        }
        if (pretty && value->data.object_val.count > 0) {
            append_indent(out, out_len, out_cap, indent);
        }
        append_char(out, out_len, out_cap, '}');
        break;
    }
}

char *hr_json_serialize(const HrJsonValue *value, bool pretty)
{
    if (!value) {
        return NULL;
    }

    size_t capacity = 1024;
    char *out = (char *)malloc(capacity);
    if (!out) {
        return NULL;
    }

    size_t length = 0;
    serialize_value(value, &out, &length, &capacity, 0, pretty);
    out[length] = '\0';

    return out;
}
