// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include "astman.h"
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
