// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include "tgroup.h"

// Tracking for an AST ID list being built on the temporary stack.
struct astlst
{
    uint16_t direct_count;
    uint16_t sublist_count;
};

// Initialize list.
static void astlst_init(struct astlst *astlst)
{
    assert(astlst != NULL);

    astlst->direct_count = 0;
    astlst->sublist_count = 0;
}

// Convert all direct ID's to a sublist.
static void _astlst_to_sublist(
    struct tgroup *tgroup,
    uint16_t child_count)
{
    struct astman *astman = &tgroup->astman;
    struct tmp_stack *tmp_stack = &tgroup->tmp_stack;

    size_t children_size = sizeof(astid_t) * child_count;
    void *children = tmp_stack_end(tmp_stack) - children_size;
    astid_t sublist = astman_alloc_node(astman, SYNCAT_SUBLIST, child_count, 0);
    memcpy(astman->data + sublist, children, children_size);
    tmp_stack_pop(tmp_stack, children_size);

    tmp_stack_push(tmp_stack, &sublist, sizeof(sublist));
}

// Push an ID.
static void astlst_push(
    struct tgroup *tgroup,
    struct astlst *astlst,
    astid_t astid)
{
    assert(tgroup != NULL);
    assert(astlst != NULL);

    // Increment direct ID count.
    astlst->direct_count++;
    if (astlst->direct_count == 0)
    {
        // Convert existing direct ID's to a sublist on
        // uint16_t overflow before pushing the new ID.
        astlst->direct_count = 1;
        astlst->sublist_count++;
        _astlst_to_sublist(tgroup, UINT16_MAX);
    }

    // Push the new ID.
    tmp_stack_push(&tgroup->tmp_stack, &astid, sizeof(astid));
}

// Make sure total child count fits in uint16_t. Doesn't bother
// to leave astlst in a meaningful state; stop using it after this.
static uint16_t astlst_finalize(struct tgroup *tgroup, struct astlst *astlst)
{
    assert(tgroup != NULL);
    assert(astlst != NULL);

    uint32_t total = (uint32_t)astlst->direct_count + astlst->sublist_count;
    if (total <= UINT16_MAX)
    {
        return (uint16_t)total;
    }
    else
    {
        _astlst_to_sublist(tgroup, astlst->direct_count);
        return astlst->sublist_count + 1;
    }
}
