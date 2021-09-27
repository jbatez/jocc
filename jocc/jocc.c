// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#include "../common/preprocessor.h"

// Read file.
static char *read_file(const char *path, size_t *size_out)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        abort();
    }

    size_t size = 0;
    char *data = NULL;
    for (;;)
    {
        data = jocc_realloc(data, size + 4096);
        size_t ret = fread(data + size, 1, 4096, file);
        if (ret > 0)
        {
            size += ret;
        }
        else if (ferror(file))
        {
            abort();
        }
        else
        {
            *size_out = size;
            data[size] = 0;
            fclose(file);
            return data;
        }
    }
}

// Entry point.
int main(void)
{
    // Initialize translation group.
    struct tgroup tgroup;
    tgroup_init(&tgroup);

    // Read file and generate corresponding phys_file and logi_file.
    const char *path = "example.joc";
    strid_t name = strman_get_id(&tgroup.strman, path, strlen(path));

    size_t size;
    char *data = read_file(path, &size);

    phys_file_id_t phys_file_id =
        srcman_add_phys_file(&tgroup.srcman, name, size, data);
    logi_file_id_t logi_file_id =
        srcman_add_logi_file(&tgroup.srcman, phys_file_id, 0);

    // Preprocess.
    preprocess(&tgroup, logi_file_id);

    // Put some unused functions through their
    // paces to elide -Wunused-function for now.
    astman_get_syncat(&tgroup.astman, 1);
    astman_get_child_count(&tgroup.astman, 1);

    diag_arr_add(
        &tgroup.diag_arr, 1, 1,
        DIAG_SEVERITY_ERROR, DIAG_CODE_TODO,
        0, NULL);

    srcman_get_pres_file(&tgroup.srcman, 0);

    srcloc_t line_start;
    srcman_get_line(&tgroup.srcman, 1, &line_start);

    strman_get_str(&tgroup.strman, name);

    // Cleanup.
    jocc_free(data);
    tgroup_destroy(&tgroup);
}
