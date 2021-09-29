// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include "prelude.h"

// decode_utf8 result.
struct decode_utf8_result
{
    // Negative on invalid byte sequence.
    int32_t code_point;
    int size;
};

// Decode UTF-8 code point.
static struct decode_utf8_result decode_utf8(const char *bytes)
{
    assert(bytes != NULL);

    unsigned char b0 = (unsigned char)bytes[0];
    if (b0 < 0x80)
    {
        return (struct decode_utf8_result){.code_point = b0, .size = 1};
    }

    int size;
    uint32_t code_point;
    if ((b0 & 0xE0) == 0xC0)
    {
        size = 2;
        code_point = b0 & 0x1F;
    }
    else if ((b0 & 0xF0) == 0xE0)
    {
        size = 3;
        code_point = b0 & 0x0F;
    }
    else if ((b0 & 0xF8) == 0xF0)
    {
        size = 4;
        code_point = b0 & 0x07;
    }
    else
    {
        return (struct decode_utf8_result){.code_point = -1, .size = 1};
    }

    for (int i = 1; i < size; i++)
    {
        unsigned char bi = (unsigned char)bytes[i];
        if ((bi & 0xC0) == 0x80)
        {
            code_point <<= 6;
            code_point |= bi & 0x3F;
        }
        else
        {
            return (struct decode_utf8_result){.code_point = -1, .size = i};
        }
    }

    if (size == 2)
    {
        if (code_point < 0x80)
        {
            code_point = -1;
        }
    }
    else if (size == 3)
    {
        if (code_point < 0x0800 || (code_point > 0xD7FF && code_point < 0xE000))
        {
            code_point = -1;
        }
    }
    else // (size == 4)
    {
        if (code_point < 0x10000 || code_point > 0x10FFFF)
        {
            code_point = -1;
        }
    }

    return (struct decode_utf8_result){(int32_t)code_point, size};
}

// Reverse decode UTF-8 code point.
static struct decode_utf8_result reverse_decode_utf8(
    const char *begin,
    const char *end)
{
    assert(begin != NULL);
    assert(end != NULL);
    assert(end > begin);

    ptrdiff_t offset = -1;
    while (end + offset > begin && (end[offset] & 0xC0) == 0x80)
    {
        offset -= 1;
    }

    struct decode_utf8_result u = decode_utf8(end + offset);
    if (u.size == -offset)
    {
        return u;
    }
    else
    {
        return (struct decode_utf8_result){.code_point = -1, .size = 1};
    }
}
