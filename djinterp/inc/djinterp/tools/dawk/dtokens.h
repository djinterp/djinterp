/*******************************************************************************
* djinterp [c][tools]                                                  dtokens.h
*
* A file's tokens as the fixer reads them, and the fixer's two safety checks
* over a file before and after a repair (dawk's decision 45).
*   TOKENS. Parse's lexer reads the file, as C or C++ by its extension, with
* the options the fixer's extensions use, and each token is named as Clang's
* raw lexer names it: raw_identifier for an identifier or a keyword, l_brace,
* plus, numeric_constant, string_literal, comment. So what the checks say
* reads as the tools they replace said it, and a test can lex a file with
* Clang too and compare the two, token for token (d_tokens_agree).
*   SAME TOKENS. Two files are the same to the compiler when they hold the
* same tokens in the same order, each spelled the same, and the same
* directives, each ending after the same token. Comments and layout may
* differ, but not where white space is part of the meaning: before a
* `#define`'s `(`, which makes a macro function-like; between the tokens of a
* replacement list; and between the tokens of an argument a macro may turn
* into a string (one of `assert`, a macro the file defines with `#`, or any
* name in capitals, which is a macro by the style guide's naming).
*   MOVED LINES. The two files must hold the same tokens. Each line's first
* token in the second is then held to what it belongs with:
*     - a token inside a `(` or `[` opened on an earlier line moves as that
*       line does, or stands where the layout puts it; so does a closing
*       bracket, as its opener's line does;
*     - a `{` on a line of its own moves as the line its head begins on;
*     - a line that continues a statement moves as the statement's first
*       line does;
*     - statements that stood in one column of a block stand in one column
*       still, labels apart from statements (a label is not a statement);
*     - a comment on a line of its own that stood in the column of the line
*       after it moves as that line does, and otherwise, if it stood in the
*       column of the line before it, as that one does;
*     - a token given a line of its own stands where the layout puts it: a
*       level in from the line its bracket opens on, or under the first token
*       after a bracket that keeps its first token beside it; a `{` under the
*       line its head begins on; a closing bracket under the line its opener
*       is on; an `else` under the `}` before it.
* Each finding is a line of the report, `after:line: what`, worded as the
* tools' tests expect (the .says files of tests/dawk/moved_lines/wrong).
*
* HOW TO USE
* ----------
*   dcheck --tokens FILE                    the tokens, one a line
*   dcheck --same-tokens BEFORE AFTER       exit 0 when the same
*   dcheck --moved-lines BEFORE AFTER       exit 0 when everything moved
*                                           stands in its place
*   dcheck --clang-tokens=DUMP FILE         exit 0 when DUMP, Clang's
*                                           `-Xclang -dump-raw-tokens` of
*                                           FILE, agrees with parse's lexer
*
* OPTIONS
* -------
*   None: each call takes the paths it compares and a stream to report on.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/tools/dawk/dtokens.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
*                                                            revised: 2026.11.19
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  dj_tokens_file
2.  OPERATIONS
    ----------
    1.  Reading
    2.  Names
    3.  The checks
*/

#ifndef DJINTERP_TOOLS_DAWK_DTOKENS_H
#define DJINTERP_TOOLS_DAWK_DTOKENS_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // FILE
// djinterp
#include "../../c/modules/parse/lex/lex_token.h"          // dj_token
#include "../../c/modules/parse/source/source_reader.h"  // dj_source


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1
// dj_tokens_file
//   struct: a file read whole, and every token the fixer's lexer finds in it,
// comments included and the end left out. `text` is the file's bytes and
// `source` reads them; both are freed by d_tokens_free.
struct dj_tokens_file
{
    const char*       path;
    char*             text;
    size_t            length;
    struct dj_source  source;
    struct dj_token*  tokens;
    size_t            count;
    bool              cpp;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Reading
//------------------------------------------------------------------------------
/**
 * @brief Reads a file and lexes it as the fixer does.
 *
 * @param[out] _file    receives the file and its tokens.
 * @param[in]  _path    the file; C++ by its extension (d_ext_cpp_accepts),
 *                      C otherwise.
 * @param[in]  _report  where a failure is said; may be `NULL`.
 * @return `true` if read and lexed; `false`, with `_file` empty, when it
 *         can't be read or memory runs out.
 */
bool d_tokens_read(
    struct dj_tokens_file* _file,
    const char*            _path,
    FILE*                  _report
);
void d_tokens_free(
    struct dj_tokens_file* _file
);

// 2.2    Names
//------------------------------------------------------------------------------
/**
 * @brief The name Clang's raw lexer gives a token.
 *
 * @note A header name in angle brackets has none, since Clang's raw lexer
 *       reads it as the tokens it holds; it is `header_name` here.
 *
 * @param[in] _file   the file the token is in.
 * @param[in] _token  one of its tokens.
 * @return the name; `unknown` for ill-formed text and stray characters.
 */
const char* d_tokens_clang_name(
    const struct dj_tokens_file* _file,
    const struct dj_token*       _token
);

// 2.3    The checks
//------------------------------------------------------------------------------
// Each returns 0 when what it checks holds, 1 when it doesn't, having said why
// on `_report`, and 2 when a file can't be read.
int d_tokens_dump(
    const char* _path,
    FILE*       _out,
    FILE*       _report
);
int d_tokens_agree(
    const char* _path,
    const char* _clang_dump,
    FILE*       _report
);
int d_tokens_same(
    const char* _before,
    const char* _after,
    FILE*       _report
);
int d_tokens_moved(
    const char* _before,
    const char* _after,
    FILE*       _report
);


#endif  // DJINTERP_TOOLS_DAWK_DTOKENS_H
