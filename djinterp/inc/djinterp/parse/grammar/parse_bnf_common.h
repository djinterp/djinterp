/******************************************************************************
* djinterp [parse]                                          parse_bnf_common.h
*
*   Common infrastructure for BNF-style grammar parsers. Contains:
*     - Token and symbol enums
*     - Configuration structures
*     - Lexer implementation
*     - Grammar data structures and helpers
*
* 
* path:      \inc\parse\grammar\parse_bnf_common.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.09
******************************************************************************/

#ifndef DJINTERP_PARSE_GRAMMAR_BNF_COMMON_
#define DJINTERP_PARSE_GRAMMAR_BNF_COMMON_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "djinterp.h"


// ============================================================================
// token / symbol enums
// ============================================================================

// DParseTokenType
//   enum: identifies the semantic category of a token produced by the BNF
// lexer during grammar parsing.
//
// Values:
//   D_PARSE_TOKEN_TYPE_STOP          - end of input or error sentinel
//   D_PARSE_TOKEN_TYPE_IDENT         - identifier/nonterminal name
//   D_PARSE_TOKEN_TYPE_STRING        - single-quoted string literal 'literal'
//   D_PARSE_TOKEN_TYPE_COLON_COLON_EQ- production operator ::=
//   D_PARSE_TOKEN_TYPE_PIPE          - alternative separator |
//   D_PARSE_TOKEN_TYPE_SEMI          - rule terminator ;
//   D_PARSE_TOKEN_TYPE_LBRACE        - EBNF left brace {
//   D_PARSE_TOKEN_TYPE_RBRACE        - EBNF right brace }
//   D_PARSE_TOKEN_TYPE_LBRACKET      - EBNF left bracket [
//   D_PARSE_TOKEN_TYPE_RBRACKET      - EBNF right bracket ]
//   D_PARSE_TOKEN_TYPE_LPAREN        - EBNF left parenthesis (
//   D_PARSE_TOKEN_TYPE_RPAREN        - EBNF right parenthesis )
//   D_PARSE_TOKEN_TYPE_STAR          - EBNF zero-or-more *
//   D_PARSE_TOKEN_TYPE_PLUS          - EBNF one-or-more +
//   D_PARSE_TOKEN_TYPE_QUESTION      - EBNF optional ?
enum DParseTokenType
{
    D_PARSE_TOKEN_TYPE_STOP = 0,
    D_PARSE_TOKEN_TYPE_IDENT,
    D_PARSE_TOKEN_TYPE_STRING,
    D_PARSE_TOKEN_TYPE_COLON_COLON_EQ,
    D_PARSE_TOKEN_TYPE_PIPE,
    D_PARSE_TOKEN_TYPE_SEMI,

    // ebnf extensions
    D_PARSE_TOKEN_TYPE_LBRACE,
    D_PARSE_TOKEN_TYPE_RBRACE,
    D_PARSE_TOKEN_TYPE_LBRACKET,
    D_PARSE_TOKEN_TYPE_RBRACKET,
    D_PARSE_TOKEN_TYPE_LPAREN,
    D_PARSE_TOKEN_TYPE_RPAREN,
    D_PARSE_TOKEN_TYPE_STAR,
    D_PARSE_TOKEN_TYPE_PLUS,
    D_PARSE_TOKEN_TYPE_QUESTION
};

// DParseSymbolKind
//   enum: classifies grammar symbols by their role in the grammar.
//
// Values:
//   D_PARSE_SYMBOL_KIND_UNKNOWN   - not yet classified
//   D_PARSE_SYMBOL_KIND_NONTERM   - nonterminal (appears on LHS of production)
//   D_PARSE_SYMBOL_KIND_TERM      - terminal (leaf symbol)
//   D_PARSE_SYMBOL_KIND_SYNTHETIC - auto-generated for EBNF desugaring
enum DParseSymbolKind
{
    D_PARSE_SYMBOL_KIND_UNKNOWN = 0,
    D_PARSE_SYMBOL_KIND_NONTERM,
    D_PARSE_SYMBOL_KIND_TERM,
    D_PARSE_SYMBOL_KIND_SYNTHETIC
};

// ============================================================================
// whitespace and BNF mode enums
// ============================================================================

// DParseWhitespaceKind
//   enum: identifies individual whitespace character types for configurable
// whitespace handling via bitmask.
//
// Values:
//   D_PARSE_WHITESPACE_KIND_SPACE           - ASCII space (0x20)
//   D_PARSE_WHITESPACE_KIND_TAB             - horizontal tab (0x09)
//   D_PARSE_WHITESPACE_KIND_NEWLINE         - line feed (0x0A)
//   D_PARSE_WHITESPACE_KIND_CARRIAGE_RETURN - carriage return (0x0D)
//   D_PARSE_WHITESPACE_KIND_FORM_FEED       - form feed (0x0C)
//   D_PARSE_WHITESPACE_KIND_VERTICAL_TAB    - vertical tab (0x0B)
//   D_PARSE_WHITESPACE_KIND_COUNT           - sentinel/count value
enum DParseWhitespaceKind
{
    D_PARSE_WHITESPACE_KIND_SPACE = 0,
    D_PARSE_WHITESPACE_KIND_TAB,
    D_PARSE_WHITESPACE_KIND_NEWLINE,
    D_PARSE_WHITESPACE_KIND_CARRIAGE_RETURN,
    D_PARSE_WHITESPACE_KIND_FORM_FEED,
    D_PARSE_WHITESPACE_KIND_VERTICAL_TAB,
    D_PARSE_WHITESPACE_KIND_COUNT
};

// D_PARSE_WHITESPACE_MASK
//   macro: converts a DParseWhitespaceKind enum value to its corresponding
// single-bit mask for use in whitespace_mask configuration field.
#define D_PARSE_WHITESPACE_MASK(kind) \
    ( (unsigned int)1u << (unsigned int)(kind) )

// DParseBnfStartMode
//   enum: controls how the grammar's start symbol is determined.
//
// Values:
//   D_PARSE_BNF_START_MODE_INFER - infer from first rule's LHS
//   D_PARSE_BNF_START_MODE_TOKEN - require explicit %START directive
enum DParseBnfStartMode
{
    D_PARSE_BNF_START_MODE_INFER = 0,
    D_PARSE_BNF_START_MODE_TOKEN
};

// DParseBnfEndMode
//   enum: controls how end-of-grammar is detected.
//
// Values:
//   D_PARSE_BNF_END_MODE_INFER - end at EOF
//   D_PARSE_BNF_END_MODE_TOKEN - require explicit %END directive
enum DParseBnfEndMode
{
    D_PARSE_BNF_END_MODE_INFER = 0,
    D_PARSE_BNF_END_MODE_TOKEN
};

// DParseBnfRuleBeginMode
//   enum: controls how rule definitions are recognized.
//
// Values:
//   D_PARSE_BNF_RULE_BEGIN_MODE_INFER - recognize IDENT ::= pattern
//   D_PARSE_BNF_RULE_BEGIN_MODE_TOKEN - require %RULE prefix before each rule
enum DParseBnfRuleBeginMode
{
    D_PARSE_BNF_RULE_BEGIN_MODE_INFER = 0,
    D_PARSE_BNF_RULE_BEGIN_MODE_TOKEN
};

// DParseBnfRuleEndMode
//   enum: controls how rule termination is detected.
//
// Values:
//   D_PARSE_BNF_RULE_END_MODE_INFER - rule ends at next rule or EOF
//   D_PARSE_BNF_RULE_END_MODE_TOKEN - require terminator token (e.g., ';')
enum DParseBnfRuleEndMode
{
    D_PARSE_BNF_RULE_END_MODE_INFER = 0,
    D_PARSE_BNF_RULE_END_MODE_TOKEN
};

// ============================================================================
// bnf parser configuration
// ============================================================================

// d_parse_bnf_config
//   struct: configuration settings controlling BNF/EBNF grammar parsing
// behavior including whitespace handling, comment syntax, directive tokens,
// and EBNF extension support.
//
// Fields:
//   whitespace_mask        - bitmask of whitespace characters to skip
//   line_comment_char      - character starting line comments (0 to disable)
//   start_mode             - how start symbol is determined
//   start_token_text       - directive text for explicit start (e.g., "%START")
//   end_mode               - how grammar end is detected
//   end_token_text         - directive text for explicit end (e.g., "%END")
//   rule_begin_mode        - how rules are introduced
//   rule_begin_token_text  - directive text for rule prefix (e.g., "%RULE")
//   rule_end_mode          - how rules are terminated
//   rule_end_token_text    - terminator token text (e.g., ";")
//   ebnf_extensions        - non-zero to enable EBNF syntax ({}, [], *, +, ?)
struct d_parse_bnf_config
{
    unsigned int                  whitespace_mask;
    char                          line_comment_char;

    enum DParseBnfStartMode       start_mode;
    const char*                   start_token_text;

    enum DParseBnfEndMode         end_mode;
    const char*                   end_token_text;

    enum DParseBnfRuleBeginMode   rule_begin_mode;
    const char*                   rule_begin_token_text;

    enum DParseBnfRuleEndMode     rule_end_mode;
    const char*                   rule_end_token_text;

    int                           ebnf_extensions;
};


// ============================================================================
// core data structures
// ============================================================================

// d_parse_token
//   struct: represents a single token produced by the BNF lexer, containing
// its type, source location, and a non-owning reference to the lexeme text
// within the source buffer.
//
// Fields:
//   type   - semantic category of the token
//   lexeme - pointer to first character in source buffer (not null-terminated)
//   length - length of lexeme in bytes
//   line   - 1-based line number of token start
//   column - 1-based column number of token start
struct d_parse_token
{
    enum DParseTokenType type;
    const char*          lexeme;
    size_t               length;
    int                  line;
    int                  column;
};

// d_parse_lexer
//   struct: maintains lexer state during tokenization of a BNF grammar source.
// Contains the source buffer reference, current read position, location
// tracking, configuration reference, and currently active token.
//
// Fields:
//   source   - pointer to source buffer (not owned)
//   length   - total length of source buffer in bytes
//   position - current read position (byte offset from source start)
//   line     - current 1-based line number
//   column   - current 1-based column number
//   config   - pointer to configuration (not owned, may be NULL)
//   current  - most recently lexed token
struct d_parse_lexer
{
    const char*                      source;
    size_t                           length;
    size_t                           position;
    int                              line;
    int                              column;
    const struct d_parse_bnf_config* config;
    struct d_parse_token             current;
};

// d_parse_symbol
//   struct: represents a grammar symbol (terminal or nonterminal) in the
// symbol table. Contains the symbol's name, classification, and whether it
// appears as a left-hand side of any production.
//
// Fields:
//   name   - null-terminated symbol name (heap-allocated, owned)
//   kind   - symbol classification (terminal, nonterminal, synthetic, unknown)
//   is_lhs - non-zero if symbol appears on LHS of at least one production
struct d_parse_symbol
{
    char*                 name;
    enum DParseSymbolKind kind;
    int                   is_lhs;
};

// d_parse_production
//   struct: represents a single production rule in the grammar with a
// left-hand side symbol and zero or more right-hand side symbols.
//
// Fields:
//   lhs_index   - index of LHS symbol in grammar's symbol table
//   rhs_indices - heap-allocated array of RHS symbol indices (NULL if epsilon)
//   rhs_length  - number of symbols in RHS (0 for epsilon production)
struct d_parse_production
{
    int    lhs_index;
    int*   rhs_indices;
    size_t rhs_length;
};

// d_parse_grammar
//   struct: complete grammar representation containing symbol table,
// production rules, start symbol designation, and bookkeeping for synthetic
// symbol generation during EBNF desugaring.
//
// Fields:
//   symbols             - heap-allocated array of grammar symbols
//   symbol_count        - number of symbols currently in table
//   symbol_capacity     - allocated capacity of symbols array
//   productions         - heap-allocated array of production rules
//   production_count    - number of productions in grammar
//   production_capacity - allocated capacity of productions array
//   start_symbol_index  - index of start symbol (-1 if unset)
//   synthetic_counter   - counter for generating unique synthetic names
struct d_parse_grammar
{
    struct d_parse_symbol*     symbols;
    size_t                     symbol_count;
    size_t                     symbol_capacity;

    struct d_parse_production* productions;
    size_t                     production_count;
    size_t                     production_capacity;

    int                        start_symbol_index;

    size_t                     synthetic_counter;
};

// d_parse_parser
//   struct: maintains parser state during grammar analysis including lexer
// reference, target grammar, current token, and one-token lookahead cache.
//
// Fields:
//   lexer         - pointer to associated lexer (not owned)
//   grammar       - pointer to target grammar being built (not owned)
//   current       - current token being processed
//   lookahead     - cached lookahead token (valid if has_lookahead is non-zero)
//   has_lookahead - non-zero if lookahead contains a valid cached token
struct d_parse_parser
{
    struct d_parse_lexer*   lexer;
    struct d_parse_grammar* grammar;

    struct d_parse_token    current;
    struct d_parse_token    lookahead;
    int                     has_lookahead;
};


// ============================================================================
// function declarations
// ============================================================================

// configuration
void                       d_parse_bnf_config_init_default(struct d_parse_bnf_config* _config);
void                       d_parse_bnf_config_init_ebnf(struct d_parse_bnf_config* _config);

// utility
void*                      d_parse_realloc(void* _ptr, size_t _size);
struct d_parse_token       d_parse_make_token(enum DParseTokenType _type, const char* _start, size_t _length, int _line, int _column);
unsigned int               d_parse_whitespace_bit_for_char(int _ch);
int                        d_parse_is_config_whitespace(const struct d_parse_lexer* _lexer, int _ch);
int                        d_parse_token_matches_cstring(const struct d_parse_token* _token, const char* _text);

// lexer
int                        d_parse_lexer_peek(struct d_parse_lexer* _lexer);
int                        d_parse_lexer_advance(struct d_parse_lexer* _lexer);
void                       d_parse_lexer_skip_whitespace_and_comments(struct d_parse_lexer* _lexer);
int                        d_parse_lexer_is_ebnf_char(const struct d_parse_lexer* _lexer, int _ch);
struct d_parse_token       d_parse_lexer_next(struct d_parse_lexer* _lexer);
int                        d_parse_lexer_init(struct d_parse_lexer* _lexer, const char* _source, const struct d_parse_bnf_config* _config);

// grammar
void                       d_parse_grammar_init(struct d_parse_grammar* _grammar);
int                        d_parse_grammar_add_symbol(struct d_parse_grammar* _grammar, const char* _name, size_t _length);
int                        d_parse_grammar_add_symbol_cstr(struct d_parse_grammar* _grammar, const char* _name);
int                        d_parse_grammar_create_synthetic_symbol(struct d_parse_grammar* _grammar, const char* _suffix);
struct d_parse_production* d_parse_grammar_new_production(struct d_parse_grammar* _grammar);
int                        d_parse_grammar_add_production(struct d_parse_grammar* _grammar, int _lhs_index, const int* _rhs_indices, size_t _rhs_length);
void                       d_parse_grammar_classify_symbols(struct d_parse_grammar* _grammar);
void                       d_parse_grammar_destroy(struct d_parse_grammar* _grammar);

// parser
struct d_parse_token*      d_parse_parser_current(struct d_parse_parser* _parser);
struct d_parse_token*      d_parse_parser_peek(struct d_parse_parser* _parser);
void                       d_parse_parser_advance(struct d_parse_parser* _parser);
int                        d_parse_parser_expect(struct d_parse_parser* _parser, enum DParseTokenType _expected_type, const char* _message);


#endif  // DJINTERP_PARSE_GRAMMAR_BNF_COMMON_