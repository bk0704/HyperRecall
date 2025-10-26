#ifndef HYPERRECALL_JSON_H
#define HYPERRECALL_JSON_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file json.h
 * @brief Minimal in-repo JSON parser and serializer for HyperRecall.
 * 
 * Provides a simple JSON library sufficient for export/import operations,
 * theme loading, and configuration persistence. Not a full-featured JSON
 * library - just enough for HyperRecall's needs.
 */

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief JSON value types.
 */
typedef enum HrJsonType {
    HR_JSON_NULL = 0,
    HR_JSON_BOOL,
    HR_JSON_NUMBER,
    HR_JSON_STRING,
    HR_JSON_ARRAY,
    HR_JSON_OBJECT
} HrJsonType;

/**
 * @brief Forward declaration for JSON value.
 */
typedef struct HrJsonValue HrJsonValue;

/**
 * @brief Parse JSON from a string.
 * 
 * @param json_text The JSON string to parse (null-terminated).
 * @return Parsed JSON value, or NULL on error. Free with hr_json_free().
 */
HrJsonValue *hr_json_parse(const char *json_text);

/**
 * @brief Free a JSON value and all its children.
 * 
 * @param value The JSON value to free (may be NULL).
 */
void hr_json_free(HrJsonValue *value);

/**
 * @brief Get the type of a JSON value.
 * 
 * @param value The JSON value.
 * @return The type, or HR_JSON_NULL if value is NULL.
 */
HrJsonType hr_json_type(const HrJsonValue *value);

/**
 * @brief Get a boolean value.
 * 
 * @param value The JSON value.
 * @param out_bool Pointer to store the boolean result.
 * @return true if value is a boolean, false otherwise.
 */
bool hr_json_get_bool(const HrJsonValue *value, bool *out_bool);

/**
 * @brief Get a number value.
 * 
 * @param value The JSON value.
 * @param out_number Pointer to store the number result.
 * @return true if value is a number, false otherwise.
 */
bool hr_json_get_number(const HrJsonValue *value, double *out_number);

/**
 * @brief Get a string value.
 * 
 * @param value The JSON value.
 * @return The string, or NULL if not a string. String is owned by the JSON value.
 */
const char *hr_json_get_string(const HrJsonValue *value);

/**
 * @brief Get the number of elements in an array.
 * 
 * @param value The JSON array.
 * @return The number of elements, or 0 if not an array.
 */
size_t hr_json_array_size(const HrJsonValue *value);

/**
 * @brief Get an element from an array by index.
 * 
 * @param value The JSON array.
 * @param index The index.
 * @return The element, or NULL if out of bounds or not an array.
 */
const HrJsonValue *hr_json_array_get(const HrJsonValue *value, size_t index);

/**
 * @brief Get a field from an object by key.
 * 
 * @param value The JSON object.
 * @param key The field name.
 * @return The field value, or NULL if not found or not an object.
 */
const HrJsonValue *hr_json_object_get(const HrJsonValue *value, const char *key);

/**
 * @brief Check if an object has a field.
 * 
 * @param value The JSON object.
 * @param key The field name.
 * @return true if the field exists, false otherwise.
 */
bool hr_json_object_has(const HrJsonValue *value, const char *key);

/**
 * @brief Create a new JSON object.
 * 
 * @return A new empty JSON object, or NULL on allocation failure.
 */
HrJsonValue *hr_json_object_new(void);

/**
 * @brief Create a new JSON array.
 * 
 * @return A new empty JSON array, or NULL on allocation failure.
 */
HrJsonValue *hr_json_array_new(void);

/**
 * @brief Create a JSON string value.
 * 
 * @param str The string (will be copied).
 * @return A new JSON string, or NULL on allocation failure.
 */
HrJsonValue *hr_json_string_new(const char *str);

/**
 * @brief Create a JSON number value.
 * 
 * @param number The number.
 * @return A new JSON number, or NULL on allocation failure.
 */
HrJsonValue *hr_json_number_new(double number);

/**
 * @brief Create a JSON boolean value.
 * 
 * @param value The boolean value.
 * @return A new JSON boolean, or NULL on allocation failure.
 */
HrJsonValue *hr_json_bool_new(bool value);

/**
 * @brief Create a JSON null value.
 * 
 * @return A new JSON null, or NULL on allocation failure.
 */
HrJsonValue *hr_json_null_new(void);

/**
 * @brief Set a field in a JSON object.
 * 
 * @param object The JSON object.
 * @param key The field name (will be copied).
 * @param value The value to set (ownership transferred to object).
 * @return true on success, false on error.
 */
bool hr_json_object_set(HrJsonValue *object, const char *key, HrJsonValue *value);

/**
 * @brief Append a value to a JSON array.
 * 
 * @param array The JSON array.
 * @param value The value to append (ownership transferred to array).
 * @return true on success, false on error.
 */
bool hr_json_array_append(HrJsonValue *array, HrJsonValue *value);

/**
 * @brief Serialize a JSON value to a string.
 * 
 * @param value The JSON value to serialize.
 * @param pretty If true, format with indentation.
 * @return Allocated string containing JSON, or NULL on error. Free with free().
 */
char *hr_json_serialize(const HrJsonValue *value, bool pretty);

#ifdef __cplusplus
}
#endif

#endif /* HYPERRECALL_JSON_H */
