// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include "alloc.h"
#include "hash.h"

// String ID.
// Index into strman data.
// 0 is reserved for the empty string.
typedef uint32_t strid_t;

// String manager entry.
struct strman_entry
{
    uint32_t hash;
    strid_t strid;
};

// String manager.
// Effectively a hash set of strings.
// Stores a single copy of each unique string and generates a small ID that
// can be used to efficiently store references and compare for equality.
struct strman
{
    uint32_t entry_count;
    uint32_t entry_capacity; // Must be a power of two.
    struct strman_entry *entries;

    uint32_t data_size;
    uint32_t data_capacity;
    char *data;
};

// Initialize string manager.
static void strman_init(struct strman *strman)
{
    assert(strman != NULL);

    strman->entry_count = 0;
    strman->entry_capacity = 1;
    strman->entries = JOCC_ZALLOC(struct strman_entry);

    strman->data_size = 1;
    strman->data_capacity = 1;
    strman->data = JOCC_ZALLOC(char);
}

// Destroy string manager.
static void strman_destroy(struct strman *strman)
{
    assert(strman != NULL);

    jocc_free(strman->data);
    jocc_free(strman->entries);
}

// Get ID for string.
static strid_t strman_get_id(
    struct strman *strman,
    const char *string,
    uint32_t len)
{
    assert(strman != NULL);
    assert(string != NULL || len == 0);

    if (len == 0)
    {
        return 0; // The empty string.
    }

    // Try to find an existing entry.
    uint32_t hash = (uint32_t)jocc_hash(string, len);
    uint32_t mask = strman->entry_capacity - 1;

    struct strman_entry *entry;
    for (uint32_t i = hash & mask;; i = (i + 1) & mask)
    {
        entry = &strman->entries[i];
        if (entry->hash == hash)
        {
            // The hash of this entry matches.
            // Make extra sure the string actually matches too.
            const char *data = strman->data + entry->strid;
            if (strncmp(data, string, len) == 0 && data[len] == 0)
            {
                return entry->strid;
            }
        }

        // Empty entry. This must be the first
        // time we've encountered this string.
        if (entry->strid == 0)
        {
            break;
        }
    }

    // No existing entry. Create a new one.
    strman->entry_count++;

    // Make sure entry_capacity is at least double entry_count.
    if (strman->entry_count > strman->entry_capacity / 2)
    {
        uint32_t old_capacity = strman->entry_capacity;
        if (old_capacity > UINT32_MAX / 2)
        {
            translation_limit_exceeded();
        }

        strman->entry_capacity = old_capacity * 2;
        mask = strman->entry_capacity - 1;

        struct strman_entry *old_entries = strman->entries;
        strman->entries = ZALLOC_ARRAY(
            struct strman_entry, strman->entry_capacity);

        // Migrate from old_entries.
        for (uint32_t i = 0; i < old_capacity; i++)
        {
            struct strman_entry *old_entry = &old_entries[i];

            // Skip empty old_entries.
            if (old_entry->strid == 0)
            {
                continue;
            }

            // Copy non-empty old_entries into empty slots in the new array.
            for (uint32_t j = old_entry->hash & mask;; j = (j + 1) & mask)
            {
                struct strman_entry *new_entry = &strman->entries[j];
                if (new_entry->strid == 0)
                {
                    *new_entry = *old_entry;
                    break;
                }
            }
        }

        jocc_free(old_entries);

        // Find an empty slot for the new entry.
        for (uint32_t i = hash & mask;; i = (i + 1) & mask)
        {
            entry = &strman->entries[i];
            if (entry->strid == 0)
            {
                break;
            }
        }
    }

    // Initialize new entry.
    strid_t strid = strman->data_size;
    entry->strid = strid;
    entry->hash = hash;

    // Append new data.
    strman->data_size = strid + len + 1;
    if (strman->data_size <= strid)
    {
        translation_limit_exceeded();
    }

    if (strman->data_capacity < strman->data_size)
    {
        strman->data_capacity *= 2;
        if (strman->data_capacity < strman->data_size)
        {
            strman->data_capacity = strman->data_size;
        }

        strman->data = REALLOC_ARRAY(
            char, strman->data, strman->data_capacity);
    }

    memcpy(strman->data + strid, string, len);
    strman->data[strid + len] = 0;

    // Done.
    return strid;
}

// Get string by ID.
static const char *strman_get_str(struct strman *strman, strid_t strid)
{
    assert(strman != NULL);
    assert(strid < strman->data_size);

    return strman->data + strid;
}
