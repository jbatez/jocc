// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include "astlst.h"
#include "lexer.h"

// Preprocessor (TODO: work-in-progress).
static int preprocess(
    struct tgroup *tgroup,
    phys_file_id_t phys_file_id,
    astid_t included_at)
{
    int ret = 0;

    struct phys_file *phys_file =
        srcman_get_phys_file(&tgroup->srcman, phys_file_id);

    logi_file_id_t logi_file_id = srcman_add_logi_file(
        &tgroup->srcman, phys_file_id, included_at, tgroup->srcloc);

    pres_file_id_t pres_file_id = srcman_add_pres_file(
        &tgroup->srcman, logi_file_id, 1, phys_file->name, 1);

    struct lexer lexer;
    const char *end = phys_file->data + phys_file->size;
    lexer_init(&lexer, tgroup, phys_file->data, end, pres_file_id);

    // For each line.
    for (;;)
    {
        lexer_begin_line(&lexer);

        // Convert lexemes to AST nodes.
        struct astlst lexemes;
        astlst_init(&lexemes);
        bool eof = false;
        for (;;)
        {
            srcloc_t start_srcloc = tgroup->srcloc;
            struct lexeme lexeme = lexer_next(&lexer);
            srcloc_t end_srcloc = tgroup->srcloc;

            if (lexeme.syncat == SYNCAT_EOF)
            {
                eof = true;
                break;
            }
            else if (lexeme.syncat == SYNCAT_EOL)
            {
                break;
            }
            else if (lexeme.syncat == SYNCAT_ILLEGAL_BYTES)
            {
                eof = true;
                ret = 1;
                break;
            }
            else
            {
                astid_t astid = astman_alloc_node(
                    &tgroup->astman, lexeme.syncat, 0,
                    lexeme.spelling == 0 ? 2 : 3);

                tgroup->astman.data[astid + 0] = start_srcloc;
                tgroup->astman.data[astid + 1] = end_srcloc;

                if (lexeme.spelling != 0)
                {
                    tgroup->astman.data[astid + 2] = lexeme.spelling;
                }

                astlst_push(tgroup, &lexemes, astid);
            }
        }

        // Finalize lexemes.
        uint16_t child_count = astlst_finalize(tgroup, &lexemes);
        size_t children_size = sizeof(astid_t) * child_count;

        // TODO.
        tmp_stack_pop(&tgroup->tmp_stack, children_size);

        // Stop after EOF.
        if (eof)
        {
            return ret;
        }
    }
}
