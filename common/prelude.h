// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A unique ID number for each abstract syntax tree node.
// Index into astman data of node header plus 1.
// Alternatively: Index into astman data of the first child ID or extra entry.
// Use astman_get_*() to read the header.
// Use astman.data[astid + offset] to access child ID's and extra entries.
// 0 is reserved for null.
typedef uint32_t astid_t;

// A unique ID number for each byte in every logi_file.
// Use srcman_get() to dereference.
// 0 is reserved for null.
typedef uint32_t srcloc_t;

// Exit because out of memory.
static void out_of_memory(void)
{
    fprintf(stderr, "fatal error: out of memory\n");
    exit(EXIT_FAILURE);
}

// Exit because a translation limit was exceeded.
static void translation_limit_exceeded(void)
{
    fprintf(stderr, "fatal error: translation limit exceeded\n");
    exit(EXIT_FAILURE);
}
