// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include "prelude.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

// Hash code.
typedef XXH64_hash_t hash_t;

// Calculate hash code.
static hash_t jocc_hash(const void *data, size_t size)
{
    return XXH3_64bits(data, size);
}
