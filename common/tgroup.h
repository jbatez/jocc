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

// Determine size of a decode_utf8_result after escaping.
static size_t _escaped_size(struct decode_utf8_result u)
{
    if (u.code_point >= ' ' && u.code_point <= '~')
    {
        return 1; // Just a normal ASCII character.
    }
    else if (u.code_point == '\t')
    {
        return 2; // \t
    }
    else if (u.code_point < 0)
    {
        return 4 * u.size; // \xXX for each byte.
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

// Escape a character or byte(s).
static void _escape(char *dst, const char *src, struct decode_utf8_result u)
{
    if (u.code_point >= ' ' && u.code_point <= '~')
    {
        *dst = (char)u.code_point;
    }
    else if (u.code_point == '\t')
    {
        dst[0] = '\\';
        dst[1] = 't';
    }
    else if (u.code_point < 0)
    {
        for (int i = 0; i < u.size; i++)
        {
            sprintf(dst, "\\x%02"PRIX8, (uint8_t)src[i]);
            dst += 4;
        }
    }
    else if (u.code_point <= 0xFFFF)
    {
        sprintf(dst, "\\u%04"PRIX16, (uint16_t)u.code_point);
    }
    else
    {
        sprintf(dst, "\\U%08"PRIX32, (uint32_t)u.code_point);
    }
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

    // Count the number of characters from line_start to start with escaping.
    size_t len = 0;
    for (const char *src = line_start_ptr; src < start_ptr;)
    {
        struct decode_utf8_result u = decode_utf8(src);
        size_t escaped_size = _escaped_size(u);
        assert(u.size <= start_ptr - src);
        len += escaped_size;
        src += u.size;
    }

    // Trim leading characters until there's less than 40.
    const char *src = line_start_ptr;
    while (len >= 40)
    {
        struct decode_utf8_result u = decode_utf8(src);
        size_t escaped_size = _escaped_size(u);
        assert(u.size <= start_ptr - src);
        len -= escaped_size;
        src += u.size;
    }

    // Trim leading spaces and tabs.
    while (src < start_ptr && (*src == ' ' || *src == '\t'))
    {
        len -= (*src == '\t') ? 2 : 1;
        src += 1;
    }

    // Start building line_text.
    char line_text[81];
    assert(len < sizeof(line_text));

    char *dst = line_text;
    while (src < start_ptr)
    {
        struct decode_utf8_result u = decode_utf8(src);
        size_t escaped_size = _escaped_size(u);
        assert(u.size <= start_ptr - src);
        _escape(dst, src, u);
        dst += escaped_size;
        src += u.size;
    }

    assert(src == start_ptr);

    // Append additional characters until EOF or EOL.
    uint32_t line_text_offset = (uint32_t)len;
    while (src < eof_ptr && *src != '\n' && *src != '\r')
    {
        struct decode_utf8_result u = decode_utf8(src);
        size_t escaped_size = _escaped_size(u);
        assert(u.size <= eof_ptr - src);

        if (len + escaped_size >= sizeof(line_text))
        {
            break;
        }

        _escape(dst, src, u);
        len += escaped_size;
        dst += escaped_size;
        src += u.size;
    }

    // Trim trailing spaces and tabs.
    while (src > start_ptr && (src[-1] == ' ' || src[-1] == '\t'))
    {
        len -= (src[-1] == '\t') ? 2 : 1;
        dst -= (src[-1] == '\t') ? 2 : 1;
        src -= 1;
    }

    // Terminate dst.
    if (start_ptr == eof_ptr)
    {
        assert(src == eof_ptr);
        strcpy(dst, "<EOF>");
    }
    else
    {
        *dst = 0;
    }

    // Append diagnostic.
    diag_arr_add(
        &tgroup->diag_arr, start, end, severity, code,
        line_text_offset, line_text);
}
