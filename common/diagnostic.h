// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include "alloc.h"

// Diagnostic severity.
enum diag_severity
{
    DIAG_SEVERITY_ERROR,
    DIAG_SEVERITY_WARNING,
};

// Diagnostic code.
enum diag_code
{
    DIAG_CODE_TODO,
};

// Diagnostic (e.g. error or warning).
struct diagnostic
{
    struct srcloc_range srcloc_range;

    enum diag_severity severity : 16;
    enum diag_code code : 16;

    // Offset in line_text to point at.
    uint32_t line_text_offset;

    // Up to 80 characters around srcloc_range.start;
    // Owned and optional.
    char *line_text;
};

// Dynamic array of diagnostics.
struct diag_arr
{
    uint32_t len;
    uint32_t capacity;
    struct diagnostic *data;
};

// Initialize diagnostic array.
static void diag_arr_init(struct diag_arr *arr)
{
    assert(arr != NULL);

    arr->len = 0;
    arr->capacity = 1;
    arr->data = JOCC_ALLOC(struct diagnostic);
}

// Destroy diagnostic array.
static void diag_arr_destroy(struct diag_arr *arr)
{
    assert(arr != NULL);

    for (uint32_t i = 0; i < arr->len; i++)
    {
        jocc_free(arr->data[i].line_text);
    }

    jocc_free(arr->data);
}

// Add diagnostic to array.
static void diag_arr_add(
    struct diag_arr *arr,
    srcloc_t start,
    srcloc_t end,
    enum diag_severity severity,
    enum diag_code code,
    uint32_t line_text_offset,
    const char *line_text)
{
    assert(arr != NULL);

    // Re-allocate if necessary.
    if (arr->capacity == arr->len)
    {
        if (arr->capacity > UINT32_MAX / 2)
        {
            exit_impl_limit_exceeded();
        }

        arr->capacity *= 2;
        arr->data = REALLOC_ARRAY(struct diagnostic, arr->data, arr->capacity);
    }

    // Locate and initialize new element.
    uint32_t idx = arr->len++;
    struct diagnostic *diag = &arr->data[idx];

    diag->srcloc_range.start = start;
    diag->srcloc_range.end = end;
    diag->severity = severity;
    diag->code = code;
    diag->line_text_offset = line_text_offset;

    // Copy line_text if provided.
    if (line_text == NULL)
    {
        diag->line_text = NULL;
    }
    else
    {
        size_t line_text_size = strlen(line_text) + 1;
        diag->line_text = ALLOC_ARRAY(char, line_text_size);
        memcpy(diag->line_text, line_text, line_text_size);
    }
}
