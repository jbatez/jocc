// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include "astman.h"
#include "decode_utf8.h"
#include "diagnostic.h"
#include "srcman.h"
#include "tmp_stack.h"

// Translation group.
// Contains all the data structures needed to store the
// intermediate and final results of JoC source file translation.
struct tgroup
{
    // Next unused source location.
    srcloc_t srcloc;

    // Abstract syntax tree manager.
    struct astman astman;

    // Diagnostics.
    struct diag_arr diag_arr;

    // Source manager.
    struct srcman srcman;

    // String manager.
    struct strman strman;

    // Temporary stack.
    struct tmp_stack tmp_stack;
};

// Initialize translation group.
static void tgroup_init(struct tgroup *tgroup)
{
    assert(tgroup != NULL);

    tgroup->srcloc = 1;
    astman_init(&tgroup->astman);
    diag_arr_init(&tgroup->diag_arr);
    srcman_init(&tgroup->srcman);
    strman_init(&tgroup->strman);
    tmp_stack_init(&tgroup->tmp_stack);
}

// Destroy translation group.
static void tgroup_destroy(struct tgroup *tgroup)
{
    assert(tgroup != NULL);

    tmp_stack_destroy(&tgroup->tmp_stack);
    strman_destroy(&tgroup->strman);
    srcman_destroy(&tgroup->srcman);
    diag_arr_destroy(&tgroup->diag_arr);
    astman_destroy(&tgroup->astman);
}

// Determine size of escaped decode_utf8_result.
static size_t _escaped_size(struct decode_utf8_result u)
{
    if (u.code_point >= ' ' && u.code_point <= '~')
    {
        return 1; // Just a normal ASCII character.
    }
    else if (u.code_point < 0)
    {
        return u.size * 4; // \xXX for each byte.
    }
    else if (u.code_point <= 0xFFFF)
    {
        return 6; // \uXXXX
    }
    else
    {
        return 10; // \UXXXXXXXX
    }
}

// Escape character/byte(s). Advance src and dst accordingly.
static void _escape(const char **src, char **dst)
{
    if (**src >= ' ' && **src <= '~')
    {
        **dst = **src;
        *dst += 1;
        *src += 1;
        return;
    }

    struct decode_utf8_result u = decode_utf8(*src);
    if (u.code_point < 0)
    {
        for (int i = 0; i < u.size; i++)
        {
            sprintf(*dst, "\\x%02"PRIX8, (uint8_t)(*src)[i]);
            *dst += 4;
        }
    }
    else if (u.code_point <= 0xFFFF)
    {
        sprintf(*dst, "\\u%04"PRIX16, (uint16_t)u.code_point);
        *dst += 6;
    }
    else
    {
        sprintf(*dst, "\\U%08"PRIX32, (uint32_t)u.code_point);
        *dst += 10;
    }

    *src += u.size;
}

// Add diagnostic with implicit line text.
static void tgroup_add_diag(
    struct tgroup *tgroup,
    srcloc_t start,
    srcloc_t end,
    enum diag_severity severity,
    enum diag_code code)
{
    assert(tgroup != NULL);
    assert(end >= start);

    struct srcman *srcman = &tgroup->srcman;

    // Get line and file info.
    srcloc_t line_start;
    struct srcline *line =
        srcman_get_line(srcman, start, &line_start);
    struct pres_file *pres_file =
        srcman_get_pres_file(srcman, line->pres_file_id);
    struct logi_file *logi_file =
        srcman_get_logi_file(srcman, pres_file->logi_file_id);
    struct phys_file *phys_file =
        srcman_get_phys_file(srcman, logi_file->phys_file_id);

    // Make sure start and end refer to the same logical file.
    #ifndef NDEBUG
    {
        srcloc_t end_line_start;
        struct srcline *end_line =
            srcman_get_line(srcman, end, &end_line_start);
        struct pres_file *end_pres_file =
            srcman_get_pres_file(srcman, end_line->pres_file_id);
        assert(end_pres_file->logi_file_id == pres_file->logi_file_id);
    }
    #endif

    // Get data pointers.
    const char *start_ptr =
        phys_file->data + (start - logi_file->start);
    const char *line_start_ptr =
        phys_file->data + (line_start - logi_file->start);
    const char *eof_ptr =
        phys_file->data + phys_file->size;

    // Count the number of characters from line_start to start.
    size_t left_size = 0;
    for (const char *ptr = line_start_ptr; ptr < start_ptr;)
    {
        struct decode_utf8_result u = decode_utf8(ptr);
        assert(start_ptr - ptr >= u.size);
        left_size += _escaped_size(u);
        ptr += u.size;
    }

    // Count the number of characters from start to EOF or EOL.
    size_t right_size = 0;
    const char *right_ptr = start_ptr;
    while (
        right_size < 80 && right_ptr < eof_ptr &&
        *right_ptr != '\n' && *right_ptr != '\r')
    {
        struct decode_utf8_result u = decode_utf8(right_ptr);
        assert(eof_ptr - right_ptr >= u.size);
        right_size += _escaped_size(u);
        right_ptr += u.size;

        if (u.code_point < 0)
        {
            break; // No more than one invalid byte sequence on the right.
        }
    }

    // Trim both sides until we're at <= 80 characters.
    const char *left_ptr = line_start_ptr;
    for (;;)
    {
        // Trim leading spaces and tabs.
        while (
            left_ptr < start_ptr &&
            (*left_ptr == ' ' || *left_ptr == '\t'))
        {
            left_size -= (*left_ptr == '\t') ? 6 : 1;
            left_ptr += 1;
        }

        // Trim trailing spaces and tabs.
        while (
            right_ptr > start_ptr &&
            (right_ptr[-1] == ' ' || right_ptr[-1] == '\t'))
        {
            right_size -= (right_ptr[-1] == '\t') ? 6 : 1;
            right_ptr -= 1;
        }

        // Make sure there's room for at least
        // one character for start to point at.
        if (right_size == 0)
        {
            if (right_ptr == eof_ptr)
            {
                right_size = 5; // <EOF>
            }
            else
            {
                right_size = 1; // An empty space.
            }
        }

        // If we're over 80 characters:
        if (left_size + right_size > 80)
        {
            if (left_size >= right_size)
            {
                // Trim from the left side.
                struct decode_utf8_result u = decode_utf8(left_ptr);
                assert(start_ptr - left_ptr >= u.size);
                left_size -= _escaped_size(u);
                left_ptr += u.size;
            }
            else
            {
                // Trim from the right side.
                struct decode_utf8_result u =
                    reverse_decode_utf8(start_ptr, right_ptr);
                assert(right_ptr - start_ptr >= u.size);
                right_size -= _escaped_size(u);
                right_ptr -= u.size;
            }
        }
        else
        {
            // Done once we're <= 80 characters.
            break;
        }
    }

    // Build line_text.
    char line_text[81];
    char *dst = line_text;
    const char *src = left_ptr;
    while (src < right_ptr)
    {
        _escape(&src, &dst);
    }

    if (right_ptr == eof_ptr)
    {
        strcpy(dst, "<EOF>");
    }
    else
    {
        *dst = 0;
    }

    // Append diagnostic.
    diag_arr_add(
        &tgroup->diag_arr, start, end, severity, code,
        (uint32_t)left_size, line_text);
}
