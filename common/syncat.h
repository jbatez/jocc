// Copyright (c) Jo Bates 2021.
// Distributed under the MIT License.
// See accompanying file LICENSE.txt

#pragma once

#include "prelude.h"

// Syntactic category.
enum syncat
{
    SYNCAT_NONE = 0,

    SYNCAT_EOF, // end-of-file
    SYNCAT_EOL, // end-of-line
    SYNCAT_WS,  // white-space

    SYNCAT_CHAR_CONST, // character-constant
    SYNCAT_STRING_LIT, // string-literal
    SYNCAT_IDENT,      // identifier
    SYNCAT_PP_NUMBER,  // pp-number

    SYNCAT_BLOCK_COMMENT,
    SYNCAT_LINE_COMMENT,

    SYNCAT_INCOMPLETE_CHAR_CONST,
    SYNCAT_INCOMPLETE_STRING_LIT,
    SYNCAT_INCOMPLETE_BLOCK_COMMENT,

    SYNCAT_EXCLAIM,     // !
    SYNCAT_NE,          // !=
    SYNCAT_HASH,        // #
    SYNCAT_HASH_HASH,   // ##
    SYNCAT_PERCENT,     // %
    SYNCAT_MOD_ASSIGN,  // %=
    SYNCAT_AMPERSAND,   // &
    SYNCAT_AND_AND,     // &&
    SYNCAT_AND_ASSIGN,  // &=
    SYNCAT_LPAREN,      // (
    SYNCAT_RPAREN,      // )
    SYNCAT_ASTERISK,    // *
    SYNCAT_MUL_ASSIGN,  // *=
    SYNCAT_PLUS,        // +
    SYNCAT_INC,         // ++
    SYNCAT_ADD_ASSIGN,  // +=
    SYNCAT_COMMA,       // ,
    SYNCAT_MINUS,       // -
    SYNCAT_DEC,         // --
    SYNCAT_SUB_ASSIGN,  // -=
    SYNCAT_ARROW,       // ->
    SYNCAT_DOT,         // .
    SYNCAT_ELLIPSIS,    // ...
    SYNCAT_SLASH,       // /
    SYNCAT_DIV_ASSIGN,  // /=
    SYNCAT_COLON,       // :
    SYNCAT_COLON_COLON, // ::
    SYNCAT_SEMICOLON,   // ;
    SYNCAT_LT,          // <
    SYNCAT_LE,          // <=
    SYNCAT_SHL,         // <<
    SYNCAT_SHL_ASSIGN,  // <<=
    SYNCAT_ASSIGN,      // =
    SYNCAT_EQ_EQ,       // ==
    SYNCAT_GT,          // >
    SYNCAT_GE,          // >=
    SYNCAT_SHR,         // >>
    SYNCAT_SHR_ASSIGN,  // >>=
    SYNCAT_QMARK,       // ?
    SYNCAT_LBRACK,      // [
    SYNCAT_RBRACK,      // ]
    SYNCAT_CARET,       // ^
    SYNCAT_XOR_ASSIGN,  // ^=
    SYNCAT_LBRACE,      // {
    SYNCAT_VBAR,        // |
    SYNCAT_OR_OR,       // ||
    SYNCAT_OR_ASSIGN,   // |=
    SYNCAT_RBRACE,      // }
    SYNCAT_TILDE,       // ~

    SYNCAT_LINE_SPLICE,
    SYNCAT_OTHER_CHAR,
    SYNCAT_ILLEGAL_BYTES,

    SYNCAT_SUBLIST, // Sub-list of a node with over UINT16_MAX children.
};
