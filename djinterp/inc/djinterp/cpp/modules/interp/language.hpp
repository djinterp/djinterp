/*******************************************************************************
* djinterp [cpp][interp]                                            language.hpp
*
* The command language read: a line or a command line into tokens, and the
* tokens into a term, on expression core v2's C API (docs/djinterp/interp/
* DESIGN.md, section 3). The interpreter's skeleton, its second step:
* sequences, pipelines, `and`, `or`, `not`, the comparisons and literals.
*   A line is lexed as the grammar's section I has it, with sh's quoting: a
* word is a run of bare, single-quoted and double-quoted parts, `\"` and
* `\\` escaped in double quotes; `#` begins a comment where a token could;
* a newline ends a pipeline, as `;` does. A command line the shell has
* already split is a token a word, whatever its characters: only a word
* that is wholly an operator, a keyword or a number is one (decision 2). A
* word that is wholly a number is a NUMBER; on a line, one that is wholly
* one quoted part is a STRING; either, standing where a command could, is a
* literal (the grammar's note 4).
*   The grammar of this step, lowest first, as djinterp.peg's section III
* has it:
*
*     sequence   pipeline ((';' | newline) pipeline)*       10
*     pipeline   or ('|' or)*                               20
*     or         and ('or' and)*                            30
*     and        not ('and' not)*                           40
*     not        'not' not | compare                        50, prefix
*     compare    primary (COMPARE primary)?                 60, none
*     primary    WORD (WORD | NUMBER | STRING)*  - a verb and its arguments
*              | NUMBER | STRING                 - a literal
*              | '(' sequence ')'
*
* A command takes the words after its verb until an operator, a keyword, a
* bracket or the end; each keeps its text as written, for the command's own
* parser to read. `;` and a newline make one flattened `seq` node, `|` one
* `pipe`, `or` one `or` and `and` one `and`; `not` a node of one child; a
* comparison a node of two, which a second beside it refuses; the command a
* `cmd` node over its words' leaves, and a literal a leaf; brackets leave no
* node. A separator may end a sequence, and blank lines are passed over.
* The braces are read as tokens of their own and have no role yet: each is
* a parse error, until the step that gives blocks one.
*
* HOW TO USE
* ----------
*   Read a text with parse_line, or a command line with parse_argv; check
* the script's ok(), and its errors() where it is not; then evaluate it
* (evaluate.hpp). sexpr() writes the term as an s-expression, for tests and
* for -v.
*
* OPTIONS
* -------
*   None yet: the depth guard and the error allowance are the parse
* settings' in force (`parse.expr_max_depth`, `parse.expr_max_errors`).
*
* CONFIGURATION
* -------------
*   C++11, the spine's floor: below it the header is empty. The library is
* a preview, built and tested by its own leaf and kept out of the install.
*
*
* path:      /inc/djinterp/cpp/modules/interp/language.hpp
* link(s):   /docs/djinterp/interp/DESIGN.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.22
*                                                            revised: 2026.11.25
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  token_class
    2.  op_id
2.  RECORDS
    -------
    1.  token
    2.  diagnostic
    3.  script
3.  READING
    -------
    1.  parse_line
    2.  parse_argv
    3.  is_number
*/

#ifndef DJINTERP_INTERP_LANGUAGE_HPP
#define DJINTERP_INTERP_LANGUAGE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error, as
// the spine is.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <memory>   // std::shared_ptr
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../djinterp.hpp"  // framework root
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint32_t

struct dj_parse_expr_term;


NS_DJINTERP
NS_INTERP


//==============================================================================
// 1.  VOCABULARY
//==============================================================================

// 1.1
// token_class
//   enum: the language's token classes, core v2's `cls`; 0 is the end.
enum token_class
{
    token_end      = 0,
    token_word     = 1,   // a verb or an argument
    token_semi     = 2,   // ;
    token_newline  = 3,   // a newline, read as ;
    token_bar      = 4,   // |
    token_lparen   = 5,   // (
    token_rparen   = 6,   // )
    token_lbrace   = 7,   // {, a block's: a later step
    token_rbrace   = 8,   // }
    token_and      = 9,   // and, standing alone
    token_or       = 10,  // or
    token_not      = 11,  // not
    token_eq       = 12,  // ==, standing alone
    token_ne       = 13,  // !=
    token_lt       = 14,  // <
    token_le       = 15,  // <=
    token_gt       = 16,  // >
    token_ge       = 17,  // >=
    token_match    = 18,  // =~
    token_number   = 19,  // a word that is wholly a number
    token_string   = 20   // a word that is wholly one quoted part
};

// 1.2
// op_id
//   enum: the term's operators, core v2's ids; 0 is none.
enum op_id
{
    op_seq   = 1,   // ; and newline, flattened, at 10
    op_pipe  = 2,   // |, flattened, at 20
    op_cmd   = 3,   // a command: its verb and arguments, built by its role
    op_group = 4,   // ( ), transparent: it leaves no node
    op_or    = 5,   // or, flattened, at 30
    op_and   = 6,   // and, flattened, at 40
    op_not   = 7,   // not, prefix, at 50
    op_eq    = 8,   // ==, at 60, as each comparison: none beside another
    op_ne    = 9,   // !=
    op_lt    = 10,  // <
    op_le    = 11,  // <=
    op_gt    = 12,  // >
    op_ge    = 13,  // >=
    op_match = 14   // =~
};


//==============================================================================
// 2.  RECORDS
//==============================================================================

// 2.1
// token
//   struct: one token: its class, its text as the command is to see it --
// quotes taken off, escapes read - and its span in the source; whether a
// blank, or a line, came before it.
struct token
{
    token_class      cls;
    std::string      text;
    re_std::uint32_t offset;
    re_std::uint32_t length;
    bool             space_before;
    bool             line_before;
};

// 2.2
// diagnostic
//   struct: an error found reading: its words, and its span in the source.
struct diagnostic
{
    std::string      message;
    re_std::uint32_t offset;
    re_std::uint32_t length;
};

// 2.3
// script
//   class: a text read: its tokens, the term core v2 made of them, and the
// errors; a script with errors has no root to evaluate. Copies share the
// term, which is never changed once read.
class script
{
public:
    script();

    // whether it was read without an error, and so has a root
    bool ok() const;
    // the errors, the reader's and the parser's, in the order found
    const std::vector<diagnostic>& errors() const;
    // the tokens, the end not among them
    const std::vector<token>& tokens() const;
    // the text it was read from: a line, or the words joined by a space
    const std::string& text() const;
    // the term, and its root; NULL and none for a script with errors
    const struct dj_parse_expr_term* term() const;
    re_std::uint32_t root() const;
    // a leaf's word: the text of the token it was made from
    const std::string& word(re_std::uint32_t _node) const;
    // the term as an s-expression: seq(pipe(cmd(ls, -l), cmd(wc)), cmd(x)),
    // and(cmd(a), ==(cmd(b), "x"), not(2)): a STRING literal in quotes
    std::string sexpr() const;

private:
    friend script parse_tokens(std::vector<token>&& _tokens,
                               std::string&&        _text,
                               std::vector<diagnostic>&& _errors);

    struct held;

    std::shared_ptr<held> m_held;
};


//==============================================================================
// 3.  READING
//==============================================================================

// 3.1
// parse_line
//   reading: a text, a line or a script of lines, lexed by the grammar's
// section I, then parsed.
script parse_line(const std::string& _text);

// 3.2
// parse_argv
//   reading: a command line the shell has split, a token a word.
script parse_argv(const std::vector<std::string>& _words);

// 3.3
// is_number
//   query: whether a word is wholly a number, as the grammar's NUMBER is:
// an optional '-', digits, and a fraction's '.' and digits.
bool is_number(const std::string& _word);

// parse_tokens
//   reading: tokens already lexed, from `_text`, the reader's errors with
// them; what parse_line and parse_argv end with.
script parse_tokens(std::vector<token>&&      _tokens,
                    std::string&&             _text,
                    std::vector<diagnostic>&& _errors);


NS_END  // interp
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_INTERP_LANGUAGE_HPP
