// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include "tgroup.h"

// One unit of lexer output.
struct lexeme
{
    // Syntactic category.
    enum syncat syncat : 32;

    // Token spelling without line splices.
    // 0 for non-tokens (EOF, EOL, white-space, etc).
    strid_t spelling;
};

// Lexer. One for each file the preprocessor ends up processing.
//
// Make sure to call lexer_begin_line before each logical line. The lexer itself
// calls lexer_begin_line for newlines in block comments and line splices. 
//
// Otherwise, call lexer_next to get each lexeme until it returns SYNCAT_EOF.
struct lexer
{
    struct tgroup *tgroup;
    const char *pos;
    pres_file_id_t pres_file_id;
    uint32_t line_num_offset;
};

// Initialize lexer.
static void lexer_init(
    struct lexer *lexer,
    struct tgroup *tgroup,
    const char *text,
    pres_file_id_t pres_file_id)
{
    assert(lexer != NULL);
    assert(tgroup != NULL);
    assert(text != NULL);

    lexer->tgroup = tgroup;
    lexer->pos = text;
    lexer->pres_file_id = pres_file_id;
    lexer->line_num_offset = 0;
}

// Begin line.
static void lexer_begin_line(struct lexer *lexer)
{
    srcman_add_line(
        &lexer->tgroup->srcman, lexer->tgroup->srcloc,
        lexer->pres_file_id, lexer->line_num_offset);
}

// Consume character.
static char lexer_consume_char(struct lexer *lexer)
{
    lexer->pos++;
    lexer->tgroup->srcloc++;
    return lexer->pos[-1];
}

// Skip line splices.
static const char *lexer_skip_line_splices(const char *pos)
{
    for (;;)
    {
        if (*pos != '\\' || (pos[1] != '\n' && pos[1] != '\r'))
        {
            return pos;
        }

        pos++;
        char c = *pos++;
        if (c == '\r' && *pos == '\n')
        {
            pos++;
        }
    }
}

// Consume line splices.
static void lexer_consume_line_splices(struct lexer *lexer)
{
    for (;;)
    {
        const char *pos = lexer->pos;
        if (*pos != '\\' || (pos[1] != '\n' && pos[1] != '\r'))
        {
            return;
        }

        lexer_consume_char(lexer);
        char c = lexer_consume_char(lexer);
        if (c == '\r' && *lexer->pos == '\n')
        {
            lexer_consume_char(lexer);
        }

        lexer->line_num_offset++;
        lexer_begin_line(lexer);
    }
}

// Peek at next character after skipping line splices.
static char lexer_peek(struct lexer *lexer)
{
    return *lexer_skip_line_splices(lexer->pos);
}

// Consume next character after line splices.
static char lexer_consume_peek(struct lexer *lexer)
{
    lexer_consume_line_splices(lexer);
    return lexer_consume_char(lexer);
}

// Consume character and append to tmp_stack for spelling.
static char lexer_include_char(struct lexer *lexer)
{
    char c = lexer_consume_char(lexer);
    tmp_stack_push(&lexer->tgroup->tmp_stack, &c, sizeof(c));
    return c;
}

// Consume next character after skipping line
// splices and append to tmp_stack for spelling.
static char lexer_include_peek(struct lexer *lexer)
{
    char c = lexer_consume_peek(lexer);
    tmp_stack_push(&lexer->tgroup->tmp_stack, &c, sizeof(c));
    return c;
}

// Include characters up to and including delimiter.
// Used to build character-constant and string-literal tokens.
static void lexer_include_until_delimiter(struct lexer *lexer, char delimiter)
{
    for (;;)
    {
        lexer_consume_line_splices(lexer);

        if (*lexer->pos == delimiter)
        {
            lexer_include_char(lexer);
            break;
        }
        else if (*lexer->pos == '\\')
        {
            // Include the backslash in an escape sequence
            // and treat the character after it like any other.
            lexer_include_char(lexer);
            lexer_consume_line_splices(lexer);
        }

        // At this stage, we allow anything in character-constants
        // and string-literals other than EOF and EOL.
        char c = *lexer->pos;
        if (c == 0 || c == '\n' || c == '\r')
        {
            break;
        }
        else
        {
            lexer_include_char(lexer);
        }
    }
}

// Handle one-or-two-character punctuator.
// Example: + or +=
static enum syncat lexer_one_or_two_char_punc(
    struct lexer *lexer, enum syncat one_syncat,
    char two_char2, enum syncat two_syncat)
{
    lexer_include_char(lexer);

    char peek = lexer_peek(lexer);
    if (peek == two_char2)
    {
        lexer_include_peek(lexer);
        return two_syncat;
    }
    else
    {
        return one_syncat;
    }
}

// Handle one-or-two-or-two-character punctuator.
// Example: & or && or &=
static enum syncat lexer_one_or_two_or_two_char_punc(
    struct lexer *lexer, enum syncat one_syncat,
    char two_char2_1, enum syncat two_syncat_1,
    char two_char2_2, enum syncat two_syncat_2)
{
    lexer_include_char(lexer);

    char peek = lexer_peek(lexer);
    if (peek == two_char2_1)
    {
        lexer_include_peek(lexer);
        return two_syncat_1;
    }
    else if (peek == two_char2_2)
    {
        lexer_include_peek(lexer);
        return two_syncat_2;
    }
    else
    {
        return one_syncat;
    }
}

// Handle one-or-two-or-two-or-two-character punctuator.
// Example: - or -- or -= or ->
static enum syncat lexer_one_or_two_or_two_or_two_char_punc(
    struct lexer *lexer, enum syncat one_syncat,
    char two_char2_1, enum syncat two_syncat_1,
    char two_char2_2, enum syncat two_syncat_2,
    char two_char2_3, enum syncat two_syncat_3)
{
    lexer_include_char(lexer);

    char peek = lexer_peek(lexer);
    if (peek == two_char2_1)
    {
        lexer_include_peek(lexer);
        return two_syncat_1;
    }
    else if (peek == two_char2_2)
    {
        lexer_include_peek(lexer);
        return two_syncat_2;
    }
    else if (peek == two_char2_3)
    {
        lexer_include_peek(lexer);
        return two_syncat_3;
    }
    else
    {
        return one_syncat;
    }
}

// Handle one-or-two-or-two-or-three-character punctuator.
// Example: < or <= or << or <<=
static enum syncat lexer_one_or_two_or_two_or_three_char_punc(
    struct lexer *lexer, enum syncat one_syncat,
    char two_char2_1, enum syncat two_syncat_1,
    char two_char2_2, enum syncat two_syncat_2,
    char three_char3, enum syncat three_syncat)
{
    lexer_include_char(lexer);

    char peek = lexer_peek(lexer);
    if (peek == two_char2_1)
    {
        lexer_include_peek(lexer);
        return two_syncat_1;
    }
    else if (peek == two_char2_2)
    {
        lexer_include_peek(lexer);

        peek = lexer_peek(lexer);
        if (peek == three_char3)
        {
            lexer_include_peek(lexer);
            return three_syncat;
        }
        else
        {
            return two_syncat_2;
        }
    }
    else
    {
        return one_syncat;
    }
}

// Next lexeme.
static struct lexeme lexer_next(struct lexer *lexer)
{
    assert(lexer != NULL);

    // Save initial tmp_stack position. If we're lexing a token, individual
    // characters (excluding line splices) will get pushed to the tmp_stack
    // so we can generate a spelling strid_t at the end.
    size_t spelling_start = lexer->tgroup->tmp_stack.size;

    // Determine syntactic category and consume characters.
    enum syncat syncat;
    switch (*lexer->pos)
    {
    case 0:
        // Consume EOF. The caller should stop using this lexer after this.
        lexer_consume_char(lexer);
        syncat = SYNCAT_EOF;
        break;

    case '\n':
        // Consume LF. It's up to the caller to
        // invoke lexer_begin_line when it's ready.
        lexer_consume_char(lexer);
        lexer->line_num_offset++;
        syncat = SYNCAT_EOL;
        break;

    case '\r':
        // Consume CR or CRLF. It's up to the caller
        // to invoke lexer_begin_line when it's ready.
        lexer_consume_char(lexer);
        if (*lexer->pos == '\n')
        {
            lexer_consume_char(lexer);
        }
        lexer->line_num_offset++;
        syncat = SYNCAT_EOL;
        break;

    case ' ': case '\t':
        // Consume all spaces and tabs. JoC doesn't consider VT
        // and FF to be valid whitespace like Standard C does.
        for (;;)
        {
            lexer_consume_char(lexer);
            if (*lexer->pos != ' ' && *lexer->pos != '\t')
            {
                break;
            }
        }
        syncat = SYNCAT_WS;
        break;

    case 'L': case 'U': case 'u':
        {
            // Include the first character no matter
            // what and peek at the next character.
            char c = lexer_include_char(lexer);
            char d = lexer_peek(lexer);

            // Check for u8.
            if (c == 'u' && d == '8')
            {
                lexer_include_peek(lexer);
                d = lexer_peek(lexer);
            }

            // L, U, u, and u8 might be character-constant or string
            // literal-prefixes. If not, they're just the beginnings of
            // identifiers.
            if (d == '\'')
            {
                lexer_include_peek(lexer);
                lexer_include_until_delimiter(lexer, d);
                syncat = SYNCAT_CHAR_CONST;
            }
            else if (d == '"')
            {
                lexer_include_peek(lexer);
                lexer_include_until_delimiter(lexer, d);
                syncat = SYNCAT_STRING_LIT;
            }
            else
            {
                goto identifier;
            }
        }
        break;

    case '\'':
        // Character-constant.
        lexer_include_char(lexer);
        lexer_include_until_delimiter(lexer, '\'');
        syncat = SYNCAT_CHAR_CONST;
        break;

    case '"':
        // String-literal.
        lexer_include_char(lexer);
        lexer_include_until_delimiter(lexer, '"');
        syncat = SYNCAT_STRING_LIT;
        break;

    case 'A': case 'B': case 'C': case 'D': case 'E':
    case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '_':
    case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r': case 's': case 't':
    case 'v': case 'w': case 'x': case 'y': case 'z':
        lexer_include_char(lexer);
    identifier:
        for (;;)
        {
            char c = lexer_peek(lexer);
            if ((c >= '0' && c <= '9') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z') ||
                c == '_')
            {
                lexer_include_peek(lexer);
            }
            else
            {
                break;
            }
        }
        syncat = SYNCAT_IDENT;
        break;

    case '.':
        lexer_include_char(lexer);
        {
            // .<digit> begins a pp-number.
            // ... is an ellipsis.
            // .<anything else> is just a dot.
            const char *peek = lexer_skip_line_splices(lexer->pos);
            if (*peek >= '0' && *peek <= '9')
            {
                lexer_include_peek(lexer);
                goto pp_number;
            }
            else if (*peek == '.' && *lexer_skip_line_splices(peek + 1) == '.')
            {
                lexer_include_peek(lexer);
                lexer_include_peek(lexer);
                syncat = SYNCAT_ELLIPSIS;
            }
            else
            {
                syncat = SYNCAT_DOT;
            }
        }
        break;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        lexer_include_char(lexer);
    pp_number:
        for (;;)
        {
            // [EePp] can be followed by sign characters in pp-numbers.
            // Otherwise, pp-numbers just consist of dots and identifier
            // characters.
            char c = lexer_peek(lexer);
            if (c == 'E' || c == 'e' || c == 'P' || c == 'p')
            {
                lexer_include_peek(lexer);
                char s = lexer_peek(lexer);
                if (s == '+' || s == '-')
                {
                    lexer_include_peek(lexer);
                }
            }
            else if (
                c == '.' ||
                (c >= '0' && c <= '9') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z') ||
                c == '_')
            {
                lexer_include_peek(lexer);
            }
            else
            {
                break;
            }
        }
        syncat = SYNCAT_PP_NUMBER;
        break;

    case '/':
        {
            // /* begins a block comment.
            // // begins a line comment.
            // /= is the division assignment operator.
            // /<anything else> is just the division operator.
            const char *peek = lexer_skip_line_splices(lexer->pos + 1);
            if (*peek == '*')
            {
                // Consume /*
                lexer_consume_char(lexer);
                lexer_consume_peek(lexer);

                // Consume everything else up to and including */
                for (;;)
                {
                    // Terminate early on EOF.
                    // TODO: Generate diagnostic.
                    if (*lexer->pos == 0)
                    {
                        break;
                    }

                    // Terminate on */
                    // Handle EOL especially.
                    // Just consume everything else.
                    char c = lexer_consume_char(lexer);
                    if (c == '*' && lexer_peek(lexer) == '/')
                    {
                        lexer_consume_peek(lexer);
                        break;
                    }
                    else if (c == '\r' || c == '\n')
                    {
                        if (c == '\r' && *lexer->pos == '\n')
                        {
                            lexer_consume_char(lexer);
                        }
                        lexer->line_num_offset++;
                        lexer_begin_line(lexer);
                    }
                }
                syncat = SYNCAT_BLOCK_COMMENT;
            }
            else if (*peek == '/')
            {
                // Consume //
                lexer_consume_char(lexer);
                lexer_consume_peek(lexer);

                // Consume everything else up to but excluding EOF and EOL.
                for (;;)
                {
                    lexer_consume_line_splices(lexer);

                    char c = *lexer->pos;
                    if (c == 0 || c == '\r' || c == '\n')
                    {
                        break;
                    }
                    else
                    {
                        lexer_consume_char(lexer);
                    }
                }
                syncat = SYNCAT_LINE_COMMENT;
            }
            else if (*peek == '=')
            {
                // Include /=
                lexer_include_char(lexer);
                lexer_include_peek(lexer);
                syncat = SYNCAT_DIV_ASSIGN;
            }
            else
            {
                // Include just the /
                lexer_include_char(lexer);
                syncat = SYNCAT_SLASH;
            }
        }
        break;

    case '!':
        // ! or !=
        syncat = lexer_one_or_two_char_punc(
            lexer, SYNCAT_EXCLAIM,
            '=', SYNCAT_NE);
        break;

    case '#':
        // # or ##
        syncat = lexer_one_or_two_char_punc(
            lexer, SYNCAT_HASH,
            '#', SYNCAT_HASH_HASH);
        break;

    case '%':
        // % or %=
        syncat = lexer_one_or_two_char_punc(
            lexer, SYNCAT_PERCENT,
            '=', SYNCAT_MOD_ASSIGN);
        break;

    case '&':
        // & or && or &=
        syncat = lexer_one_or_two_or_two_char_punc(
            lexer, SYNCAT_AMPERSAND,
            '&', SYNCAT_AND_AND,
            '=', SYNCAT_AND_ASSIGN);
        break;

    case '(':
        // Just (
        lexer_include_char(lexer);
        syncat = SYNCAT_LPAREN;
        break;

    case ')':
        // Just )
        lexer_include_char(lexer);
        syncat = SYNCAT_RPAREN;
        break;

    case '*':
        // * or *=
        syncat = lexer_one_or_two_char_punc(
            lexer, SYNCAT_ASTERISK,
            '=', SYNCAT_MUL_ASSIGN);
        break;

    case '+':
        // + or ++ or +=
        syncat = lexer_one_or_two_or_two_char_punc(
            lexer, SYNCAT_PLUS,
            '+', SYNCAT_INC,
            '=', SYNCAT_ADD_ASSIGN);
        break;

    case ',':
        // Just ,
        lexer_include_char(lexer);
        syncat = SYNCAT_COMMA;
        break;

    case '-':
        // - or -- or -= or ->
        syncat = lexer_one_or_two_or_two_or_two_char_punc(
            lexer, SYNCAT_MINUS,
            '-', SYNCAT_DEC,
            '=', SYNCAT_SUB_ASSIGN,
            '>', SYNCAT_ARROW);
        break;

    case ':':
        // : or ::
        syncat = lexer_one_or_two_char_punc(
            lexer, SYNCAT_COLON,
            ':', SYNCAT_COLON_COLON);
        break;

    case ';':
        // Just ;
        lexer_include_char(lexer);
        syncat = SYNCAT_SEMICOLON;
        break;

    case '<':
        // < or <= or << or <<=
        syncat = lexer_one_or_two_or_two_or_three_char_punc(
            lexer, SYNCAT_LT,
            '=', SYNCAT_LE,
            '<', SYNCAT_SHL,
            '=', SYNCAT_SHL_ASSIGN);
        break;

    case '=':
        // = or ==
        syncat = lexer_one_or_two_char_punc(
            lexer, SYNCAT_ASSIGN,
            '=', SYNCAT_EQ_EQ);
        break;

    case '>':
        // > or >= or >> or >>=
        syncat = lexer_one_or_two_or_two_or_three_char_punc(
            lexer, SYNCAT_GT,
            '=', SYNCAT_GE,
            '>', SYNCAT_SHR,
            '=', SYNCAT_SHR_ASSIGN);
        break;

    case '?':
        // Just ?
        lexer_include_char(lexer);
        syncat = SYNCAT_QMARK;
        break;

    case '[':
        // Just [
        lexer_include_char(lexer);
        syncat = SYNCAT_LBRACK;
        break;

    case ']':
        // Just ]
        lexer_include_char(lexer);
        syncat = SYNCAT_RBRACK;
        break;

    case '^':
        // ^ or ^=
        syncat = lexer_one_or_two_char_punc(
            lexer, SYNCAT_CARET,
            '=', SYNCAT_XOR_ASSIGN);
        break;

    case '{':
        // Just {
        lexer_include_char(lexer);
        syncat = SYNCAT_LBRACE;
        break;

    case '|':
        // | or || or |=
        syncat = lexer_one_or_two_or_two_char_punc(
            lexer, SYNCAT_VBAR,
            '|', SYNCAT_OR_OR,
            '=', SYNCAT_OR_ASSIGN);
        break;

    case '}':
        // Just }
        lexer_include_char(lexer);
        syncat = SYNCAT_RBRACE;
        break;

    case '~':
        // Just ~
        lexer_include_char(lexer);
        syncat = SYNCAT_TILDE;
        break;

    case '\\':
        // Either a line splice or just a stray backslash.
        if (lexer->pos[1] == '\r' || lexer->pos[1] == '\n')
        {
            // Consume line splice.
            lexer_consume_char(lexer);
            char c = lexer_consume_char(lexer);
            if (c == '\r' && *lexer->pos == '\n')
            {
                lexer_consume_char(lexer);
            }

            // We're responsible for calling lexer_begin_line on line splices,
            // unlike logical EOL's where the caller is responsible.
            lexer->line_num_offset++;
            lexer_begin_line(lexer);
            syncat = SYNCAT_LINE_SPLICE;
        }
        else
        {
            // Just a stray backslash.
            lexer_include_char(lexer);
            syncat = SYNCAT_OTHER_CHAR;
        }
        break;

    default:
        // Just pass through any other ASCII printable character.
        if (*lexer->pos >= ' ' && *lexer->pos <= '~')
        {
            lexer_include_char(lexer);
            syncat = SYNCAT_OTHER_CHAR;
        }
        else
        {
            // An earlier stage should've rejected this.
            abort();
        }
        break;
    }

    // Generate spelling strid_t from characters pushed to tmp_stack.
    struct tmp_stack *tmp_stack = &lexer->tgroup->tmp_stack;
    char *string = (char *)(tmp_stack->data + spelling_start);
    uint32_t len = (uint32_t)(tmp_stack->size - spelling_start);
    strid_t spelling = strman_getid(&lexer->tgroup->strman, string, len);

    // Restore tmp_stack.
    tmp_stack->size = spelling_start;

    // Done.
    return (struct lexeme){syncat, spelling};
}
