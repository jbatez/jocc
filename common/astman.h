// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include "alloc.h"
#include "syncat.h"

// Abstract syntax tree manager.
//
// The abstract syntax tree is stored as an array of of uint32_t entries. Each
// node is a contiguous sub-array of these entries. The first entry in each node
// is its "header" which packs the node's syncat (syntactic category) and
// child_count. After that are the 32-bit ID's of all its child nodes, followed
// by optional extra 32-bit entries, the interpretation of which depends on the
// syncat. For example, token nodes don't have any children, but their "extra"
// entries include a starting srcloc_t, ending srcloc_t, and a spelling strid_t.
struct astman
{
    uint32_t data_len;
    uint32_t data_capacity;
    uint32_t *data;
};

// Initialize abstract syntax tree manager.
static void astman_init(struct astman *astman)
{
    assert(astman != NULL);

    astman->data_len = 0;
    astman->data_capacity = 0;
    astman->data = NULL;
}

// Destroy abstract syntax tree manager.
static void astman_destroy(struct astman *astman)
{
    assert(astman != NULL);

    jocc_free(astman->data);
}

// Allocate abstract syntax tree node.
static astid_t astman_alloc_node(
    struct astman *astman,
    enum syncat syncat,
    uint16_t child_count,
    uint32_t extra_count)
{
    assert(astman != NULL);

    // Make sure header + child_count doesn't cause overflow.
    uint32_t old_len = astman->data_len;
    uint32_t tmp_len = old_len + 1 + child_count;
    if (tmp_len <= old_len)
    {
        exit_impl_limit_exceeded();
    }

    // Make sure extra_count doesn't cause overflow.
    astman->data_len = tmp_len + extra_count;
    if (astman->data_len < tmp_len)
    {
        exit_impl_limit_exceeded();
    }

    // Re-allocate if necessary.
    if (astman->data_capacity < astman->data_len)
    {
        astman->data_capacity *= 2;
        if (astman->data_capacity < astman->data_len)
        {
            astman->data_capacity = astman->data_len;
        }

        astman->data = REALLOC_ARRAY(
            uint32_t, astman->data, astman->data_capacity);
    }

    // Initialize header.
    astman->data[old_len] = syncat | ((uint32_t)child_count << 16);

    // Return ID.
    return old_len + 1;
}

// Get abstract syntax tree node syntactic category.
static enum syncat astman_get_syncat(struct astman *astman, astid_t id)
{
    assert(astman != NULL);
    assert(id > 0);
    assert(id <= astman->data_len);

    return astman->data[id - 1] & 0xFFFF;
}

// Get abstract syntax tree node child count.
static uint16_t astman_get_child_count(struct astman *astman, astid_t id)
{
    assert(astman != NULL);
    assert(id > 0);
    assert(id <= astman->data_len);

    return astman->data[id - 1] >> 16;
}
