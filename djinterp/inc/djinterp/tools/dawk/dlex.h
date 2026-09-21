/******************************************************************************
* djinterp [dawk]                                                       dlex.h
*
*   Tokeniser for POSIX awk.
*     Two rules here are not obvious from the grammar. A slash begins a regular
* expression where an operand is expected and divides where an operator is,
* which the scanner decides from the previous token rather than from context
* supplied by the parser. And a newline terminates a statement except after the
* tokens that cannot end one, which the scanner swallows so the parser never
* sees them.
*
*
* path:      /inc/djinterp/tools/dawk/dlex.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Token kinds
         1.  d_awk_token_kind
    2.  Token and scanner
         1.  d_awk_token
         2.  d_awk_lexer
2.  OPERATIONS
    ----------
    1.  Scanning
    2.  Diagnostics
*/

#ifndef DJINTERP_TOOLS_DAWK_DLEX_H
#define DJINTERP_TOOLS_DAWK_DLEX_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


//==============================================================================
// 1.  TYPES
//==============================================================================
// The token kinds, one token, and the scanner that produces them.


// 1.1    Token kinds
//------------------------------------------------------------------------------
// 1.1.1
// d_awk_token_kind
//   enum: the lexical classes of POSIX awk.  Compound assignment and the
//   two-character operators are distinct kinds so the parser never re-reads
//   text.
enum d_awk_token_kind
{
    D_AWK_TOK_EOF = 0,
    D_AWK_TOK_NEWLINE,
    D_AWK_TOK_NUMBER,
    D_AWK_TOK_STRING,
    D_AWK_TOK_ERE,
    D_AWK_TOK_NAME,
    D_AWK_TOK_FUNC_NAME,     // a NAME immediately followed by '('
    D_AWK_TOK_BUILTIN,
    // keywords
    D_AWK_TOK_BEGIN,
    D_AWK_TOK_END,
    D_AWK_TOK_FUNCTION,
    D_AWK_TOK_IF,
    D_AWK_TOK_ELSE,
    D_AWK_TOK_WHILE,
    D_AWK_TOK_FOR,
    D_AWK_TOK_DO,
    D_AWK_TOK_BREAK,
    D_AWK_TOK_CONTINUE,
    D_AWK_TOK_NEXT,
    D_AWK_TOK_NEXTFILE,
    D_AWK_TOK_EXIT,
    D_AWK_TOK_RETURN,
    D_AWK_TOK_DELETE,
    D_AWK_TOK_IN,
    D_AWK_TOK_PRINT,
    D_AWK_TOK_PRINTF,
    D_AWK_TOK_GETLINE,
    // punctuation
    D_AWK_TOK_LBRACE,
    D_AWK_TOK_RBRACE,
    D_AWK_TOK_LPAREN,
    D_AWK_TOK_RPAREN,
    D_AWK_TOK_LBRACKET,
    D_AWK_TOK_RBRACKET,
    D_AWK_TOK_SEMI,
    D_AWK_TOK_COMMA,
    D_AWK_TOK_QUESTION,
    D_AWK_TOK_COLON,
    D_AWK_TOK_DOLLAR,
    // operators
    D_AWK_TOK_ASSIGN,
    D_AWK_TOK_ADD_ASSIGN,
    D_AWK_TOK_SUB_ASSIGN,
    D_AWK_TOK_MUL_ASSIGN,
    D_AWK_TOK_DIV_ASSIGN,
    D_AWK_TOK_MOD_ASSIGN,
    D_AWK_TOK_POW_ASSIGN,
    D_AWK_TOK_OR,
    D_AWK_TOK_AND,
    D_AWK_TOK_NOT,
    D_AWK_TOK_MATCH,
    D_AWK_TOK_NOMATCH,
    D_AWK_TOK_EQ,
    D_AWK_TOK_NE,
    D_AWK_TOK_LT,
    D_AWK_TOK_LE,
    D_AWK_TOK_GT,
    D_AWK_TOK_GE,
    D_AWK_TOK_PLUS,
    D_AWK_TOK_MINUS,
    D_AWK_TOK_STAR,
    D_AWK_TOK_SLASH,
    D_AWK_TOK_PERCENT,
    D_AWK_TOK_CARET,
    D_AWK_TOK_INCR,
    D_AWK_TOK_DECR,
    D_AWK_TOK_APPEND,        // '>>'
    D_AWK_TOK_PIPE
};

// 1.2    Token and scanner
//------------------------------------------------------------------------------
// 1.2.1
// d_awk_token
//   struct: one token.  `text` points into a buffer owned by the lexer for a
//   NAME, STRING or ERE, and is already unescaped for a STRING.
struct d_awk_token
{
    enum d_awk_token_kind kind;
    const char*           text;
    size_t                length;
    double                number;
    size_t                line;
    size_t                column;
};

// 1.2.2
// d_awk_lexer
//   struct: scanner state.  Layout is private.
struct d_awk_lexer;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Scanning
//------------------------------------------------------------------------------
struct d_awk_lexer* d_awk_lexer_new(const char* _source,
                                    size_t      _length,
                                    const char* _origin);
void                d_awk_lexer_free(struct d_awk_lexer* _lexer);
bool                d_awk_lexer_next(struct d_awk_lexer* _lexer,
                                     struct d_awk_token* _out_token);

// 2.2    Diagnostics
//------------------------------------------------------------------------------
const char*         d_awk_lexer_error(const struct d_awk_lexer* _lexer);
size_t              d_awk_lexer_line(const struct d_awk_lexer* _lexer);
size_t              d_awk_lexer_column(const struct d_awk_lexer* _lexer);
const char*         d_awk_token_name(enum d_awk_token_kind _kind);


#endif  // DJINTERP_TOOLS_DAWK_DLEX_H
