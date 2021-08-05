#pragma once

#include "strman.h"

// Physical file ID.
// Index into srcman phys_files.
typedef uint32_t phys_file_id_t;

// Logical file ID.
// Index into srcman logi_files.
typedef uint32_t logi_file_id_t;

// Presumed file ID.
// Index into srcman pres_files.
typedef uint32_t pres_file_id_t;

// Physical file.
// One for each actual file we care about.
struct phys_file
{
    strid_t name;
    const char *text;
    bool pragma_once;
    strid_t skip_ifdef;
};

// Logical file.
// One for each file instance whether or not it was #include'd and where.
struct logi_file
{
    phys_file_id_t phys_file_id;
    astid_t included_at;
};

// Presumed file.
// Generally just a proxy for a logi_file but enables #line overrides.
struct pres_file
{
    logi_file_id_t logi_file_id;
    uint32_t phys_line_num_base;
    strid_t pres_name;
    uint32_t pres_line_num_base;
};

// Source line relative to a pres_file.
struct srcline
{
    pres_file_id_t pres_file_id;
    uint32_t line_num_offset; // Relative to pres_file *_line_num_base.
};

// Source manager. Maintains file/line descriptions and maps srcloc's to lines.
// Each line is represented by a start srcloc (inclusive) and a srcline struct.
// Use srcman_get to find the line containing a given srcloc.
struct srcman
{
    uint32_t phys_file_count;
    uint32_t phys_file_capacity;
    struct phys_file *phys_files;

    uint32_t logi_file_count;
    uint32_t logi_file_capacity;
    struct logi_file *logi_files;

    uint32_t pres_file_count;
    uint32_t pres_file_capacity;
    struct pres_file *pres_files;

    uint32_t line_count;
    uint32_t line_capacity;
    srcloc_t *line_starts;
    struct srcline *lines;
};

// Initialize source manager.
static void srcman_init(struct srcman *srcman)
{
    assert(srcman != NULL);

    srcman->phys_file_count = 0;
    srcman->phys_file_capacity = 1;
    srcman->phys_files = JOCC_ALLOC(struct phys_file);

    srcman->logi_file_count = 0;
    srcman->logi_file_capacity = 1;
    srcman->logi_files = JOCC_ALLOC(struct logi_file);

    srcman->pres_file_count = 0;
    srcman->pres_file_capacity = 1;
    srcman->pres_files = JOCC_ALLOC(struct pres_file);

    srcman->line_count = 0;
    srcman->line_capacity = 1;
    srcman->line_starts = JOCC_ALLOC(srcloc_t);
    srcman->lines = JOCC_ALLOC(struct srcline);
}

// Destroy source manager.
static void srcman_destroy(struct srcman *srcman)
{
    assert(srcman != NULL);

    jocc_free(srcman->lines);
    jocc_free(srcman->line_starts);
    jocc_free(srcman->pres_files);
    jocc_free(srcman->logi_files);
    jocc_free(srcman->phys_files);
}

// Add physical file.
static phys_file_id_t srcman_add_phys_file(
    struct srcman *srcman,
    strid_t name,
    const char *text)
{
    assert(srcman != NULL);

    // Re-allocate if necessary.
    if (srcman->phys_file_capacity == srcman->phys_file_count)
    {
        if (srcman->phys_file_capacity > UINT32_MAX / 2)
        {
            exit_impl_limit_exceeded();
        }

        srcman->phys_file_capacity *= 2;
        srcman->phys_files = REALLOC_ARRAY(
            struct phys_file, srcman->phys_files, srcman->phys_file_capacity);
    }

    // Locate and initialize new element.
    phys_file_id_t id = srcman->phys_file_count++;
    struct phys_file *file = &srcman->phys_files[id];

    file->name = name;
    file->text = text;
    file->pragma_once = false;
    file->skip_ifdef = 0;

    // Return ID.
    return id;
}

// Add logical file.
static logi_file_id_t srcman_add_logi_file(
    struct srcman *srcman,
    phys_file_id_t phys_file_id,
    astid_t included_at)
{
    assert(srcman != NULL);

    // Re-allocate if necessary.
    if (srcman->logi_file_capacity == srcman->logi_file_count)
    {
        if (srcman->logi_file_capacity > UINT32_MAX / 2)
        {
            exit_impl_limit_exceeded();
        }

        srcman->logi_file_capacity *= 2;
        srcman->logi_files = REALLOC_ARRAY(
            struct logi_file, srcman->logi_files, srcman->logi_file_capacity);
    }

    // Locate and initialize new element.
    logi_file_id_t id = srcman->logi_file_count++;
    struct logi_file *file = &srcman->logi_files[id];

    file->phys_file_id = phys_file_id;
    file->included_at = included_at;

    // Return ID.
    return id;
}

// Add presumed file.
static pres_file_id_t srcman_add_pres_file(
    struct srcman *srcman,
    logi_file_id_t logi_file_id,
    uint32_t phys_line_num_base,
    strid_t pres_name,
    uint32_t pres_line_num_base)
{
    assert(srcman != NULL);

    // Re-allocate if necessary.
    if (srcman->pres_file_capacity == srcman->pres_file_count)
    {
        if (srcman->pres_file_capacity > UINT32_MAX / 2)
        {
            exit_impl_limit_exceeded();
        }

        srcman->pres_file_capacity *= 2;
        srcman->pres_files = REALLOC_ARRAY(
            struct pres_file, srcman->pres_files, srcman->pres_file_capacity);
    }

    // Locate and initialize new element.
    pres_file_id_t id = srcman->pres_file_count++;
    struct pres_file *file = &srcman->pres_files[id];

    file->logi_file_id = logi_file_id;
    file->phys_line_num_base = phys_line_num_base;
    file->pres_name = pres_name;
    file->pres_line_num_base = pres_line_num_base;

    // Return ID.
    return id;
}

// Add line.
static void srcman_add_line(
    struct srcman *srcman,
    srcloc_t start,
    pres_file_id_t pres_file_id,
    uint32_t line_num_offset)
{
    assert(srcman != NULL);
    uint32_t old_count = srcman->line_count;
    assert(old_count == 0 || start > srcman->line_starts[old_count - 1]);

    // Re-allocate if necessary.
    if (srcman->line_capacity == old_count)
    {
        if (srcman->line_capacity > UINT32_MAX / 2)
        {
            exit_impl_limit_exceeded();
        }

        srcman->line_capacity *= 2;

        srcman->line_starts = REALLOC_ARRAY(
            srcloc_t, srcman->line_starts, srcman->line_capacity);

        srcman->lines = REALLOC_ARRAY(
            struct srcline, srcman->lines, srcman->line_capacity);
    }

    // Locate and initialize new elements.
    uint32_t idx = srcman->line_count++;
    srcman->line_starts[idx] = start;

    struct srcline *line = &srcman->lines[idx];
    line->pres_file_id = pres_file_id;
    line->line_num_offset = line_num_offset;
}

// Get physical file.
static struct phys_file *srcman_get_phys_file(
    struct srcman *srcman,
    phys_file_id_t id)
{
    assert(srcman != NULL);
    assert(id < srcman->phys_file_count);

    return &srcman->phys_files[id];
}

// Get logical file.
static struct logi_file *srcman_get_logi_file(
    struct srcman *srcman,
    logi_file_id_t id)
{
    assert(srcman != NULL);
    assert(id < srcman->logi_file_count);

    return &srcman->logi_files[id];
}

// // Get presumed file.
// static struct pres_file *srcman_get_pres_file(
//     struct srcman *srcman,
//     pres_file_id_t id)
// {
//     assert(srcman != NULL);
//     assert(id < srcman->pres_file_count);
//
//     return &srcman->pres_files[id];
// }

// Get source location information.
static struct srcline *srcman_get(
    struct srcman *srcman,
    srcloc_t srcloc,
    srcloc_t *line_start_out)
{
    assert(srcman != NULL);
    assert(srcman->line_count > 0);
    assert(srcloc >= srcman->line_starts[0]);
    assert(line_start_out != NULL);

    // Binary search.
    uint32_t hi = srcman->line_count; // Upper index (exclusive)
    uint32_t lo = 0;                  // Lower index (inclusive)

    for (;;)
    {
        uint32_t diff = hi - lo;
        if (diff == 1)
        {
            *line_start_out = srcman->line_starts[lo];
            return &srcman->lines[lo];
        }

        uint32_t mid = lo + diff / 2;
        if (srcloc < srcman->line_starts[mid])
        {
            hi = mid;
        }
        else
        {
            lo = mid;
        }
    }
}
