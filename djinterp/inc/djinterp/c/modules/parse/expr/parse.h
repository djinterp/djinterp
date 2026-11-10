/*******************************************************************************
* djinterp [c/parse]                                                     parse.h
*
* Expression core v2: the parser.
*   Text, as tokens, into a term, by a signature. The parse is operator
* precedence over two explicit stacks, an operand stack and a stack of open
* operators, in one pass: each token is pushed and popped at most once, so the
* parse is linear in the tokens, and nothing recurses, so the depth is bounded
* by memory and by the depth guard, never by the C stack.
*   Each token is read in its POSITION: in operand position, where an operand
* must begin, or in operator position, just after one ends. The token source
* is told the position too, so a lexer that must know it (awk's `/`, a bar
* that opens or closes) need not find it out for itself.
*   Every diagnostic goes into the foundation's sink in the domain
* D_PARSE_DIAG_DOMAIN_EXPR, with the span of the token or node it is about.
* The parse goes on after an error, to find one further left that it learns of
* only later, and names the leftmost as the error.
*
*
* path:      /inc/djinterp/c/modules/parse/expr/parse.h
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Diagnostic codes
         1.  dj_parse_expr_diag
    2.  Tokens
         1.  D_PARSE_EXPR_CLASS_END
         2.  Token flags
    3.  Settings
         1.  Error orders
         2.  D_PARSE_EXPR_DEPTH_UNBOUNDED, D_PARSE_EXPR_ERRORS_MAX
         3.  Status
         4.  Option flags
         5.  D_PARSE_EXPR_FLOOR_NONE
2.  TYPES
    -----
    1.  Tokens and sources
         1.  dj_parse_expr_token
         2.  dj_parse_expr_source
         3.  dj_parse_expr_array_source
    2.  Settings and results
         1.  dj_parse_expr_check_fn
         2.  dj_parse_expr_options
         3.  dj_parse_expr_result
3.  OPERATIONS
    ----------
    1.  Sources
    2.  Parsing
    3.  The hook interface
    4.  Places
*/

#ifndef DJINTERP_C_PARSE_EXPR_PARSE_H
#define DJINTERP_C_PARSE_EXPR_PARSE_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../../djinterp.h"                              // framework root
#include "../../../../config/c/modules/parse/cfg_parse.h"   // D_CFG_PARSE_EXPR_*
#include "../diagnostic.h"                                  // dj_parse_diag_sink,
                                                            // dj_parse_span
#include "./signature.h"                                    // dj_parse_expr_signature
#include "./term.h"                                         // dj_parse_expr_term
// re_std
#include "../../../../../re_std/c/cstdint/dstdint.h"  // uint8_t, uint16_t,
                                                      // int32_t, uint32_t


//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    Diagnostic codes
//------------------------------------------------------------------------------
// 1.1.1
// dj_parse_expr_diag
//   enum: the conditions D_PARSE_DIAG_DOMAIN_EXPR reports. Append-only, as
// every domain's codes are; a code means something only beside its domain.
enum dj_parse_expr_diag
{
    D_PARSE_EXPR_DIAG_UNEXPECTED      = 0,  // no role fits where it stands
    D_PARSE_EXPR_DIAG_MISSING_OPERAND = 1,  // where an operand must stand
    D_PARSE_EXPR_DIAG_UNCLOSED        = 2,  // a mixfix never closed
    D_PARSE_EXPR_DIAG_UNOPENED        = 3,  // a closing part, nothing open
    D_PARSE_EXPR_DIAG_MISMATCHED      = 4,  // closes an outer operator
    D_PARSE_EXPR_DIAG_NON_ASSOCIATIVE = 5,  // two `none` operators side by side
    D_PARSE_EXPR_DIAG_ARITY           = 6,  // a list outside its bounds
    D_PARSE_EXPR_DIAG_TOO_DEEP        = 7,  // past the depth guard
    D_PARSE_EXPR_DIAG_REFUSED         = 8,  // the language's check refused it
    D_PARSE_EXPR_DIAG_BAD_SIGNATURE   = 9   // sealing found the table unsound
};


// 1.2    Tokens
//------------------------------------------------------------------------------
// 1.2.1
// D_PARSE_EXPR_CLASS_END
//   constant: the token class of the end of the input; no role may have it.
#define D_PARSE_EXPR_CLASS_END          0u

// 1.2.2
// Token flags
//   constant: SPACE_BEFORE: blank space precedes the token; LINE_BEFORE: a
// line break does.
#define D_PARSE_EXPR_TOKEN_SPACE_BEFORE 0x01u
#define D_PARSE_EXPR_TOKEN_LINE_BEFORE  0x02u


// 1.3    Settings
//------------------------------------------------------------------------------
// 1.3.1
// Error orders
//   constant: which of a parse's errors is the error. LEFTMOST: the one
// further left, ties to the first found; STRUCTURE_FIRST: the leftmost of the
// unclosed, unopened and mismatched where there is one, as math's parser
// layer orders them today. DEFAULT: the knob's.
#define D_PARSE_EXPR_ORDER_DEFAULT          0u
#define D_PARSE_EXPR_ORDER_LEFTMOST         1u
#define D_PARSE_EXPR_ORDER_STRUCTURE_FIRST  2u

// 1.3.2
// D_PARSE_EXPR_DEPTH_UNBOUNDED, D_PARSE_EXPR_ERRORS_MAX
//   constant: a `max_depth` that leaves only memory as the bound; the most
// errors a parse may be allowed, the bound of lane E's setting for it.
#define D_PARSE_EXPR_DEPTH_UNBOUNDED    0xFFFFFFFFu
#define D_PARSE_EXPR_ERRORS_MAX         0xFFFFu

// 1.3.3
// Status
//   constant: what d_parse_expr_parse returns. OK: a whole term, no error;
// ERROR: the text has errors, each reported, the term holding what was read;
// FATAL: the parse could not go on (no memory, a full arena, a failing
// source, the depth guard, bad arguments), with what was found reported.
#define D_PARSE_EXPR_OK                 0
#define D_PARSE_EXPR_ERROR              1
#define D_PARSE_EXPR_FATAL              2

// 1.3.4
// Option flags
//   constant: STOP: a token the expression cannot go on with ends it, left
// unread in the result's `next`, where it would otherwise be an error. That is
// how a statement's parser hands an expression to the core and takes back the
// `;`, the newline, the `)` of its own `if (` or the next statement's word.
// Inside an open bracket it is still an error: the bracket is never closed.
#define D_PARSE_EXPR_OPTION_STOP        0x01u

// 1.3.5
// D_PARSE_EXPR_FLOOR_NONE
//   constant: a sub-parse's floor that holds no operator back: it reads on
// until a token it cannot go on with.
#define D_PARSE_EXPR_FLOOR_NONE         (-2147483647 - 1)


//==============================================================================
// 2.  TYPES
//==============================================================================


// 2.1    Tokens and sources
//------------------------------------------------------------------------------
// 2.1.1
// dj_parse_expr_token
//   struct: one token: its class, the language's; its flags; its span in the
// text's bytes; and the value a leaf made from it holds.
struct dj_parse_expr_token
{
    uint32_t cls;
    uint32_t flags;
    uint32_t offset;
    uint32_t length;
    uint32_t value;
};

// 2.1.2
// dj_parse_expr_source
//   struct: where tokens come from, one at a time. `next` is told the
// position the token will be read in and the parse's mode, fills `_out`, and
// returns 1 for a token, 0 at the end (with an END token whose offset is the
// end of the text), or -1 on a failure it has reported.
struct dj_parse_expr_source
{
    int   (*next)(void*                       _ctx,
                  uint8_t                     _position,
                  uint32_t                    _mode,
                  struct dj_parse_expr_token* _out);
    void* ctx;
};

// 2.1.3
// dj_parse_expr_array_source
//   struct: the state of a source over tokens already in an array.
struct dj_parse_expr_array_source
{
    const struct dj_parse_expr_token* tokens;
    uint32_t                          count;
    uint32_t                          next;
    uint32_t                          end;
};


// 2.2    Settings and results
//------------------------------------------------------------------------------
// 2.2.1
// dj_parse_expr_check_fn
//   type: the language's check of each application as it is built. Returns
// true to keep it; false to refuse it, having reported why into the sink
// (the parser then counts it as an error).
typedef bool (*dj_parse_expr_check_fn)(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    uint32_t                         _node,
    struct dj_parse_diag_sink*       _sink);

// 2.2.2
// dj_parse_expr_options
//   struct: the run-time settings, each 0 for the knob's default. `max_depth`
// is 1 to D_PARSE_EXPR_NODE_DEPTH_MAX, or D_PARSE_EXPR_DEPTH_UNBOUNDED: a
// guard between them is refused, as no node records a depth past the first.
// `max_errors` is 1 to D_PARSE_EXPR_ERRORS_MAX; more is refused. `mode` is
// the mode the parse runs in, one bit, and 1 where 0 is given: a role's
// `modes` is a set of these bits, so a hook that reads in a mode of its own
// gives another bit (2, 4, ...). `text`, where given, is the source text, for
// the messages' quotations and the default renderer; `source_id` goes into
// every span. `scratch`, where given, is `scratch_size` bytes of the
// caller's, aligned as malloc's, which the parser's stacks start in when it
// holds more than their own rooms in its frame: with the heap they grow past
// it, and without the heap a scratch of d_parse_expr_scratch_size(tokens)
// lets a parse of that many tokens finish. It must not be shared by two
// parses at once.
struct dj_parse_expr_options
{
    uint32_t               max_depth;
    uint32_t               max_errors;
    uint32_t               mode;
    uint8_t                error_order;
    uint8_t                flags;
    uint8_t                reserved[2];
    uint32_t               source_id;
    const char*            text;
    size_t                 text_length;
    dj_parse_expr_check_fn check;
    void*                  check_ctx;
    void*                  scratch;
    size_t                 scratch_size;
};

// 2.2.3
// dj_parse_expr_result
//   struct: what a parse found: its status; the term's root; how many errors;
// the error -- its domain, code and span, and its index in the sink, or
// 0xFFFFFFFF where the sink did not store it; and `next`, the token the parse
// ended at, unread: the end of the input, or the token D_PARSE_EXPR_OPTION_STOP
// ended it at (zero where a fatal failure left no token at hand).
struct dj_parse_expr_result
{
    int                        status;
    uint32_t                   root;
    uint32_t                   errors;
    uint16_t                   domain;
    uint16_t                   code;
    struct dj_parse_span       span;
    uint32_t                   diag_index;
    struct dj_parse_expr_token next;
};


//==============================================================================
// 3.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 3.1    Sources
//------------------------------------------------------------------------------
/**
 * @brief Makes a source over tokens already lexed.
 *
 * @param[out] _source  the source; may not be NULL, nor `_state`.
 * @param[out] _state   its state, which must outlive the parse.
 * @param[in]  _tokens  the tokens, without an END token; may be NULL with 0.
 * @param[in]  _count   how many.
 * @param[in]  _end     the offset of the end of the text, the END token's.
 */
void d_parse_expr_source_array(
         struct dj_parse_expr_source*       _source,
         struct dj_parse_expr_array_source* _state,
         const struct dj_parse_expr_token*  _tokens,
         uint32_t                           _count,
         uint32_t                           _end);

// 3.2    Parsing
//------------------------------------------------------------------------------
/**
 * @brief Parses one expression from a source into a term.
 *
 * @param[in]     _signature  a sealed signature.
 * @param[in,out] _source     the tokens; read to the end, or to where the
 *                            parse stopped.
 * @param[in]     _options    the settings; may be NULL for every default.
 * @param[in,out] _term       the term the nodes are appended to.
 * @param[in,out] _sink       where diagnostics go; may be NULL, and the
 *                            result still says the error.
 * @param[out]    _result     what was found; may not be NULL.
 * @return the status, as `_result->status`.
 * @post On D_PARSE_EXPR_OK, `_result->root` is the whole expression's node
 *       and `_term->root` is set to it. On D_PARSE_EXPR_ERROR the term holds
 *       what was read, holes where operands were missing.
 */
int  d_parse_expr_parse(const struct dj_parse_expr_signature* _signature,
                        struct dj_parse_expr_source*          _source,
                        const struct dj_parse_expr_options*   _options,
                        struct dj_parse_expr_term*            _term,
                        struct dj_parse_diag_sink*            _sink,
                        struct dj_parse_expr_result*          _result);

/**
 * @brief The scratch a parse of a number of tokens needs, so that its
 *        stacks never run out.
 *
 * @param[in] _tokens  the tokens the source will give, the end not counted.
 * @return the bytes, or 0 where the count's size does not fit a size_t.
 */
size_t d_parse_expr_scratch_size(uint32_t _tokens);

// 3.3    The hook interface
//------------------------------------------------------------------------------
// for a custom role's hook alone, while the parser is running it

/**
 * @brief The next token, not taken.
 *
 * @param[in,out] _parser    the parser that called the hook.
 * @param[in]     _position  the position to read it in.
 * @param[in]     _mode      the mode to read it in, one bit; 0 for the
 *                           parse's own, which is 1 unless its options gave
 *                           another.
 * @param[out]    _out       the token.
 * @return 1 for a token, 0 at the end, -1 where the source failed or the
 *         token is already peeked in another mode.
 * @note The parser holds one token peeked, and cannot read it again: a hook
 *       takes every token it reads in another mode than the parse's before
 *       it returns, and a hook that leaves one stops the parse (a
 *       bad-argument failure).
 */
int      d_parse_expr_peek(struct dj_parse_expr_parser* _parser,
                           uint8_t                      _position,
                           uint32_t                     _mode,
                           struct dj_parse_expr_token*  _out);
/**
 * @brief The next token, taken.
 *
 * @param[in,out] _parser    the parser that called the hook.
 * @param[in]     _position  the position to read it in.
 * @param[in]     _mode      the mode to read it in, as d_parse_expr_peek's.
 * @param[out]    _out       the token.
 * @return 1 for a token, 0 at the end (not taken), -1 as d_parse_expr_peek.
 */
int      d_parse_expr_take(struct dj_parse_expr_parser* _parser,
                           uint8_t                      _position,
                           uint32_t                     _mode,
                           struct dj_parse_expr_token*  _out);
/**
 * @brief Reads one expression inside a hook: a sub-parse (design 6.5).
 *
 * The operators it takes are those that bind tighter than `_floor`, while no
 * bracket of its own is open; it ends without an error at the first token it
 * cannot go on with, which stays read, for the hook to peek. Inside its own
 * brackets every operator is taken and every error is one, as in the parse.
 * Its nesting counts against the depth guard, and its errors against the
 * parse's allowance. `getline < file` reads the file with the floor at
 * concatenation's precedence.
 *
 * @param[in,out] _parser  the parser that called the hook.
 * @param[in]     _floor   the precedence an operator must bind tighter than,
 *                         or D_PARSE_EXPR_FLOOR_NONE.
 * @param[in]     _mode    the mode to read it in, one bit; 0 for the
 *                         parse's own. A mixfix inside it reads its holes in
 *                         its `hole_mode`, as in the parse.
 * @param[out]    _node    the expression's node, of the parser's term.
 * @return 1 with the node, which may hold holes where operands were missing
 *         (each reported); -1 where the parse stopped (its allowance spent,
 *         a fatal failure), or a token the hook left was read in another
 *         mode than `_mode`.
 * @note A hook that ran a sub-parse may not decline (return 0): the tokens
 *       it read cannot be read again. Doing so stops the parse with a
 *       bad-argument failure, as a token left in another mode does.
 */
int      d_parse_expr_sub(struct dj_parse_expr_parser* _parser,
                          int32_t                      _floor,
                          uint32_t                     _mode,
                          uint32_t*                    _node);
/**
 * @brief The term the parser is building.
 *
 * @param[in] _parser  the parser that called the hook.
 * @return the term, to build nodes in with term.h's builders.
 */
struct dj_parse_expr_term*
         d_parse_expr_parser_term(struct dj_parse_expr_parser* _parser);
/**
 * @brief Reports an error at a span, counted as the parse's.
 *
 * @param[in,out] _parser   the parser that called the hook.
 * @param[in]     _domain   the diagnostic's domain: the language's own.
 * @param[in]     _code     its code in that domain.
 * @param[in]     _offset   its span's first byte.
 * @param[in]     _length   its span's length.
 * @param[in]     _message  its text.
 */
void     d_parse_expr_report(struct dj_parse_expr_parser* _parser,
                             uint16_t                     _domain,
                             uint16_t                     _code,
                             uint32_t                     _offset,
                             uint32_t                     _length,
                             const char*                  _message);

// 3.4    Places
//------------------------------------------------------------------------------
/**
 * @brief The line and the column of a byte offset in a text.
 *
 * @param[in]  _text    the text; may be NULL only with a length 0.
 * @param[in]  _length  its length in bytes.
 * @param[in]  _offset  the offset; past the end means the end.
 * @param[out] _line    receives the 1-based line: `\n`, `\r\n` and a lone
 *                      `\r` each end one.
 * @param[out] _column  receives the 1-based column, in characters (Unicode
 *                      code points, read as UTF-8).
 */
void     d_parse_expr_locate(const char* _text,
                             size_t      _length,
                             size_t      _offset,
                             uint32_t*   _line,
                             uint32_t*   _column);

D_EXTERN_C_END


#endif  // DJINTERP_C_PARSE_EXPR_PARSE_H
