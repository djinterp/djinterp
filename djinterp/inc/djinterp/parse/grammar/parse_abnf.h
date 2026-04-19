/******************************************************************************
* djinterp [parse]                                                parse_abnf.h
*
*   Augmented BNF (ABNF) grammar parser per RFC 5234. Supports:
*     - Rule definition:     rulename = elements
*     - Incremental alt:     rulename =/ elements
*     - Alternation:         element / element
*     - Concatenation:       element element (whitespace-separated)
*     - Repetition:          *element, 1*element, *2element, 3*5element, 3element
*     - Optional:            [element]
*     - Grouping:            (element element)
*     - String literals:     "text" (case-insensitive), %s"text" (case-sensitive)
*     - Numeric values:      %d65, %x41, %b1000001
*     - Value ranges:        %x30-39
*     - Value concatenation: %x48.65.6C.6C.6F
*     - Comments:            ; to end of line
*     - Prose values:        <descriptive text>
*
* path:      \inc\parse\grammar\parse_abnf.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.09
******************************************************************************/

#ifndef DJINTERP_PARSE_GRAMMAR_ABNF_
#define DJINTERP_PARSE_GRAMMAR_ABNF_ 1

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse_bnf_common.h"


// ============================================================================
// ABNF token types
// ============================================================================

// DTokenType_ABNF
//   enum: identifies the semantic category of a token produced by the ABNF
// lexer during grammar parsing per RFC 5234.
//
// Values:
//   D_ABNF_TOKEN_TYPE_STOP             - end of input or error sentinel
//   D_ABNF_TOKEN_TYPE_RULENAME         - case-insensitive rule identifier
//   D_ABNF_TOKEN_TYPE_EQUALS           - rule definition operator =
//   D_ABNF_TOKEN_TYPE_EQUALS_SLASH     - incremental alternatives operator =/
//   D_ABNF_TOKEN_TYPE_SLASH            - alternation separator /
//   D_ABNF_TOKEN_TYPE_STAR             - repetition operator *
//   D_ABNF_TOKEN_TYPE_NUMBER           - decimal number for repetition bounds
//   D_ABNF_TOKEN_TYPE_LPAREN           - group open (
//   D_ABNF_TOKEN_TYPE_RPAREN           - group close )
//   D_ABNF_TOKEN_TYPE_LBRACKET         - optional open [
//   D_ABNF_TOKEN_TYPE_RBRACKET         - optional close ]
//   D_ABNF_TOKEN_TYPE_DQUOTE_STRING    - case-insensitive string "..."
//   D_ABNF_TOKEN_TYPE_SENSITIVE_STRING - case-sensitive string %s"..."
//   D_ABNF_TOKEN_TYPE_NUMERIC          - numeric value %d/%x/%b (range/concat)
//   D_ABNF_TOKEN_TYPE_PROSE            - prose description <...>
//   D_ABNF_TOKEN_TYPE_NEWLINE          - significant line break (rule boundary)
enum DTokenType_ABNF
{
    D_ABNF_TOKEN_TYPE_STOP = 0,
    D_ABNF_TOKEN_TYPE_RULENAME,
    D_ABNF_TOKEN_TYPE_EQUALS,
    D_ABNF_TOKEN_TYPE_EQUALS_SLASH,
    D_ABNF_TOKEN_TYPE_SLASH,
    D_ABNF_TOKEN_TYPE_STAR,
    D_ABNF_TOKEN_TYPE_NUMBER,
    D_ABNF_TOKEN_TYPE_LPAREN,
    D_ABNF_TOKEN_TYPE_RPAREN,
    D_ABNF_TOKEN_TYPE_LBRACKET,
    D_ABNF_TOKEN_TYPE_RBRACKET,
    D_ABNF_TOKEN_TYPE_DQUOTE_STRING,
    D_ABNF_TOKEN_TYPE_SENSITIVE_STRING,
    D_ABNF_TOKEN_TYPE_NUMERIC,
    D_ABNF_TOKEN_TYPE_PROSE,
    D_ABNF_TOKEN_TYPE_NEWLINE
};


// ============================================================================
// ABNF data structures
// ============================================================================

// d_token_abnf
//   struct: represents a single token produced by the ABNF lexer with extended
// fields for numeric values and repetition counts.
//
// Fields:
//   type           - semantic category of the token
//   lexeme         - pointer to first character in source (not null-terminated)
//   length         - length of lexeme in bytes
//   line           - 1-based line number of token start
//   column         - 1-based column number of token start
//   number_value   - parsed value for NUMBER tokens
//   numeric_values - heap-allocated array of parsed values for NUMERIC tokens
//   numeric_count  - number of values in numeric_values array
//   is_range       - non-zero if NUMERIC represents a range (e.g., %x30-39)
//   range_end      - end value if is_range is set
struct d_token_abnf
{
    enum DTokenType_ABNF type;
    const char* lexeme;
    size_t               length;
    int                  line;
    int                  column;
    int                  number_value;
    int* numeric_values;
    size_t               numeric_count;
    int                  is_range;
    int                  range_end;
};

// d_abnf_lexer
//   struct: maintains lexer state during tokenization of an ABNF grammar source.
//
// Fields:
//   source   - pointer to source buffer (not owned)
//   length   - total length of source buffer in bytes
//   position - current read position (byte offset)
//   line     - current 1-based line number
//   column   - current 1-based column number
//   current  - most recently lexed token
struct d_abnf_lexer
{
    const char* source;
    size_t               length;
    size_t               position;
    int                  line;
    int                  column;
    struct d_token_abnf  current;
};

// d_parser_abnf
//   struct: maintains parser state during ABNF grammar analysis.
//
// Fields:
//   lexer         - pointer to associated lexer (not owned)
//   grammar       - pointer to target grammar being built (not owned)
//   current       - current token being processed
//   lookahead     - cached lookahead token
//   has_lookahead - non-zero if lookahead contains valid cached token
struct d_parser_abnf
{
    struct d_abnf_lexer* lexer;
    struct d_parse_grammar* grammar;
    struct d_token_abnf     current;
    struct d_token_abnf     lookahead;
    int                     has_lookahead;
};


// token management
struct d_token_abnf* d_abnf_token_create(enum DTokenType_ABNF _type, const char* _start, size_t _length, int _line, int _column);
void                 d_abnf_token_destroy(struct d_token_abnf* _token);
// lexer - core operations
int                  d_abnf_lexer_init(struct d_abnf_lexer* _lexer, const char* _source);
int                  d_abnf_lexer_peek(struct d_abnf_lexer* _lexer);
int                  d_abnf_lexer_peek_ahead(struct d_abnf_lexer* _lexer, size_t _offset);
int                  d_abnf_lexer_advance(struct d_abnf_lexer* _lexer);
void                 d_abnf_lexer_skip_whitespace(struct d_abnf_lexer* _lexer, int _skip_newlines);
struct d_token_abnf* d_abnf_lexer_next(struct d_abnf_lexer* _lexer);
// lexer - character classification and numeric parsing
int                  d_abnf_is_rulename_start(int _ch);
int                  d_abnf_is_rulename_char(int _ch);
int                  d_abnf_parse_int(struct d_abnf_lexer* _lexer, int _base);
struct d_token_abnf* d_abnf_lexer_scan_numeric(struct d_abnf_lexer* _lexer, int _start_line, int _start_column);
// parser - core operations
struct d_token_abnf* d_parser_abnf_current(struct d_parser_abnf* _parser);
struct d_token_abnf* d_parser_abnf_peek(struct d_parser_abnf* _parser);
void                 d_parser_abnf_advance(struct d_parser_abnf* _parser);
void                 d_parser_abnf_skip_newlines(struct d_parser_abnf* _parser);
// parser - token classification
int                  d_abnf_is_element_start(enum DTokenType_ABNF _type);
int                  d_abnf_is_concatenation_end(struct d_token_abnf* _token);
// grammar - symbol creation
int                  d_abnf_add_rulename_symbol(struct d_parse_grammar* _grammar, const char* _name, size_t _length);
int                  d_abnf_add_string_terminal(struct d_parse_grammar* _grammar, const char* _str, size_t _length, int _case_sensitive);
int                  d_abnf_add_numeric_terminal(struct d_parse_grammar* _grammar, int _value);
int                  d_abnf_add_numeric_range(struct d_parse_grammar* _grammar, int _start, int _end);
int                  d_abnf_add_numeric_concat(struct d_parse_grammar* _grammar, const int* _values, size_t _count);
// grammar - production helpers
int                  d_abnf_create_repetition(struct d_parse_grammar* _grammar, int _symbol_index, int _min, int _max);
int                  d_abnf_create_group(struct d_parser_abnf* _parser);
// parser - grammar construction
int                  d_parser_abnf_parse_element(struct d_parser_abnf* _parser);
int                  d_parser_abnf_parse_concatenation(struct d_parser_abnf* _parser, int _lhs_index);
int                  d_parser_abnf_parse_alternation(struct d_parser_abnf* _parser, int _lhs_index);
int                  d_parser_abnf_parse_rule(struct d_parser_abnf* _parser);
// public interface
int                  d_parse_grammar_from_abnf(struct d_parse_grammar* _grammar, const char* _source);


#endif  // DJINTERP_PARSE_GRAMMAR_ABNF_