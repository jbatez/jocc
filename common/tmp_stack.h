// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include "alloc.h"

// Stack for runtime-sized temporaries.
struct tmp_stack
{
    size_t size;
    size_t capacity;
    unsigned char *data;
};

// Initialize temporary stack.
static void tmp_stack_init(struct tmp_stack *stack)
{
    assert(stack != NULL);

    stack->size = 0;
    stack->capacity = 0;
    stack->data = NULL;
}

// Destroy temporary stack.
static void tmp_stack_destroy(struct tmp_stack *stack)
{
    assert(stack != NULL);

    jocc_free(stack->data);
}

// Get pointer to end of temporary stack.
static unsigned char *tmp_stack_end(struct tmp_stack *stack)
{
    assert(stack != NULL);

    return stack->data + stack->size;
}

// Push to temporary stack.
static void tmp_stack_push(
    struct tmp_stack *stack,
    const void *data,
    size_t size)
{
    assert(stack != NULL);
    assert(data != NULL);

    // Check for overflow.
    size_t old_size = stack->size;
    stack->size = old_size + size;
    if (stack->size < old_size)
    {
        exit_impl_limit_exceeded();
    }

    // Re-allocate if necessary.
    if (stack->capacity < stack->size)
    {
        stack->capacity *= 2;
        if (stack->capacity < stack->size)
        {
            stack->capacity = stack->size;
        }

        stack->data = jocc_realloc(stack->data, stack->capacity);
    }

    // Copy the new data to the top of the stack.
    memcpy(stack->data + old_size, data, size);
}

// Pop bytes off top of temporary stack.
static void tmp_stack_pop(struct tmp_stack *stack, size_t size)
{
    assert(stack != NULL);
    assert(stack->size >= size);

    stack->size -= size;
}
