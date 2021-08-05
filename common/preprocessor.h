// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include "lexer.h"

// Preprocessor (work-in-progress).
static void preprocess(struct tgroup *tgroup, logi_file_id_t logi_file_id)
{
    struct logi_file *logi_file =
        srcman_get_logi_file(&tgroup->srcman, logi_file_id);
    struct phys_file *phys_file =
        srcman_get_phys_file(&tgroup->srcman, logi_file->phys_file_id);

    pres_file_id_t pres_file_id = srcman_add_pres_file(
        &tgroup->srcman, logi_file_id, 1, phys_file->name, 1);

    struct lexer lexer;
    lexer_init(&lexer, tgroup, phys_file->text, pres_file_id);

    // For each line.
    for (;;)
    {
        lexer_begin_line(&lexer);

        // Convert lexemes to AST nodes and push ID's to tmp_stack.
        size_t lexeme_astids_start = tgroup->tmp_stack.size;
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
            else
            {
                astid_t astid = astman_alloc(
                    &tgroup->astman, lexeme.syncat, 0,
                    lexeme.spelling == 0 ? 2 : 3);

                tgroup->astman.data[astid + 0] = start_srcloc;
                tgroup->astman.data[astid + 1] = end_srcloc;

                if (lexeme.spelling != 0)
                {
                    tgroup->astman.data[astid + 2] = lexeme.spelling;
                }

                tmp_stack_push(&tgroup->tmp_stack, &astid, sizeof(astid));
            }
        }

        // TODO.

        // Restore tmp_stack and continue to next line.
        tgroup->tmp_stack.size = lexeme_astids_start;
        if (eof)
        {
            break;
        }
    }
}
