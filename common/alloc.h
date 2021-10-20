// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include "prelude.h"

// Allocate.
static void *jocc_alloc(size_t size)
{
    assert(size != 0);

    void *ptr = malloc(size);
    if (ptr == NULL)
    {
        out_of_memory();
    }

    return ptr;
}

// Allocate and zero.
static void *jocc_zalloc(size_t size)
{
    assert(size != 0);

    void *ptr = jocc_alloc(size);
    memset(ptr, 0, size);
    return ptr;
}

// Re-allocate.
static void *jocc_realloc(void *ptr, size_t size)
{
    assert(size != 0);

    ptr = realloc(ptr, size);
    if (ptr == NULL)
    {
        out_of_memory();
    }

    return ptr;
}

// Allocate array.
static void *alloc_array(size_t len, size_t element_size)
{
    assert(len != 0);
    assert(element_size != 0);

    if (len > SIZE_MAX / element_size)
    {
        out_of_memory();
    }

    return jocc_alloc(len * element_size);
}

// Allocate array and zero.
static void *zalloc_array(size_t len, size_t element_size)
{
    assert(len != 0);
    assert(element_size != 0);

    void *ptr = calloc(len, element_size);
    if (ptr == NULL)
    {
        out_of_memory();
    }

    return ptr;
}

// Re-allocate array.
static void *realloc_array(void *ptr, size_t len, size_t element_size)
{
    assert(len != 0);
    assert(element_size != 0);

    if (len > SIZE_MAX / element_size)
    {
        out_of_memory();
    }

    return jocc_realloc(ptr, len * element_size);
}

// Free.
static void jocc_free(void *ptr)
{
    free(ptr);
}

// Allocate.
#define JOCC_ALLOC(T) ((T *)jocc_alloc(sizeof(T)))

// Allocate and zero.
#define JOCC_ZALLOC(T) ((T *)jocc_zalloc(sizeof(T)))

// Allocate array.
#define ALLOC_ARRAY(T, len) ((T *)alloc_array(len, sizeof(T)))

// Allocate array and zero.
#define ZALLOC_ARRAY(T, len) ((T *)zalloc_array(len, sizeof(T)))

// Re-allocate array.
#define REALLOC_ARRAY(T, ptr, len) ((T *)realloc_array(ptr, len, sizeof(T)))
