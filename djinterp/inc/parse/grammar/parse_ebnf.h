/******************************************************************************
* djinterp [parse]                                                parse_ebnf.h
*
*   Extended BNF grammar parser. Supports standard BNF plus:
*     - { X }   : zero or more repetitions
*     - [ X ]   : optional (zero or one)
*     - ( X )   : grouping
*     - X*      : postfix zero or more
*     - X+      : postfix one or more
*     - X?      : postfix optional
*
*   EBNF constructs are desugared into standard BNF productions using
*   synthetic nonterminals (e.g., __synth_0_rep__).
*
* path:      \inc\parse\grammar\parse_ebnf.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.09
******************************************************************************/

#ifndef DJINTERP_PARSE_GRAMMAR_EBNF_
#define DJINTERP_PARSE_GRAMMAR_EBNF_ 1

#include "parse_bnf_common.h"

/* ============================================================================
 * Forward declarations
 * ========================================================================== */

static int
d_parse_ebnf_parser_parse_primary
(
    struct d_parse_parser* _parser
);

static void
d_parse_ebnf_parser_parse_alternative
(
    struct d_parse_parser* _parser,
    int                    _lhs_index
);

static int
d_parse_ebnf_create_repetition
(
    struct d_parse_grammar* _grammar,
    int                     _symbol_index
);

static int
d_parse_ebnf_create_one_or_more
(
    struct d_parse_grammar* _grammar,
    int                     _symbol_index
);

static int
d_parse_ebnf_create_optional
(
    struct d_parse_grammar* _grammar,
    int                     _symbol_index
);

static int
d_parse_ebnf_create_group
(
    struct d_parse_parser* _parser
);

/* ============================================================================
 * EBNF Helper: Create repetition (zero or more) synthetic rule
 *
 *   Given a symbol index for X, creates:
 *     __synth_N_rep__ ::= X __synth_N_rep__ | (epsilon)
 *
 *   Returns the index of the synthetic nonterminal.
 * ========================================================================== */

static int
d_parse_ebnf_create_repetition
(
    struct d_parse_grammar* _grammar,
    int                     _symbol_index
)
{
    int synth_index;
    int rhs_recursive[2];

    synth_index = d_parse_grammar_create_synthetic_symbol(_grammar, "rep");

    /* Production: synth -> symbol synth */
    rhs_recursive[0] = _symbol_index;
    rhs_recursive[1] = synth_index;

    d_parse_grammar_add_production(_grammar,
                                   synth_index,
                                   rhs_recursive,
                                   2u);

    /* Production: synth -> epsilon */
    d_parse_grammar_add_production(_grammar,
                                   synth_index,
                                   NULL,
                                   0u);

    return synth_index;
}

/* ============================================================================
 * EBNF Helper: Create one-or-more synthetic rule
 *
 *   Given a symbol index for X, creates:
 *     __synth_N_plus__ ::= X __synth_M_rep__
 *     __synth_M_rep__  ::= X __synth_M_rep__ | (epsilon)
 *
 *   Returns the index of the synthetic nonterminal for one-or-more.
 * ========================================================================== */

static int
d_parse_ebnf_create_one_or_more
(
    struct d_parse_grammar* _grammar,
    int                     _symbol_index
)
{
    int synth_plus_index;
    int synth_rep_index;
    int rhs[2];

    synth_rep_index  = d_parse_ebnf_create_repetition(_grammar, _symbol_index);
    synth_plus_index = d_parse_grammar_create_synthetic_symbol(_grammar, "plus");

    /* Production: synth_plus -> symbol synth_rep */
    rhs[0] = _symbol_index;
    rhs[1] = synth_rep_index;

    d_parse_grammar_add_production(_grammar,
                                   synth_plus_index,
                                   rhs,
                                   2u);

    return synth_plus_index;
}

/* ============================================================================
 * EBNF Helper: Create optional (zero or one) synthetic rule
 *
 *   Given a symbol index for X, creates:
 *     __synth_N_opt__ ::= X | (epsilon)
 *
 *   Returns the index of the synthetic nonterminal.
 * ========================================================================== */

static int
d_parse_ebnf_create_optional
(
    struct d_parse_grammar* _grammar,
    int                     _symbol_index
)
{
    int synth_index;

    synth_index = d_parse_grammar_create_synthetic_symbol(_grammar, "opt");

    /* Production: synth -> symbol */
    d_parse_grammar_add_production(_grammar,
                                   synth_index,
                                   &_symbol_index,
                                   1u);

    /* Production: synth -> epsilon */
    d_parse_grammar_add_production(_grammar,
                                   synth_index,
                                   NULL,
                                   0u);

    return synth_index;
}

/* ============================================================================
 * EBNF Helper: Create group synthetic rule
 *
 *   Creates a synthetic nonterminal and parses alternatives into it.
 *   Returns the index of the synthetic nonterminal.
 * ========================================================================== */

static int
d_parse_ebnf_create_group
(
    struct d_parse_parser* _parser
)
{
    int                   synth_index;
    struct d_parse_token* token;

    synth_index = d_parse_grammar_create_synthetic_symbol(_parser->grammar,
                                                          "group");

    /* Parse the grouped alternatives */
    d_parse_ebnf_parser_parse_alternative(_parser, synth_index);

    token = d_parse_parser_current(_parser);

    while (token->type == D_PARSE_TOKEN_TYPE_PIPE)
    {
        d_parse_parser_advance(_parser);

        d_parse_ebnf_parser_parse_alternative(_parser, synth_index);

        token = d_parse_parser_current(_parser);
    }

    return synth_index;
}

/* ============================================================================
 * EBNF Parser: Check for postfix operator and apply transformation
 * ========================================================================== */

static int
d_parse_ebnf_apply_postfix
(
    struct d_parse_parser* _parser,
    int                    _symbol_index
)
{
    struct d_parse_token* token;
    int                   result_index;

    result_index = _symbol_index;

    token = d_parse_parser_current(_parser);

    switch (token->type)
    {
        case D_PARSE_TOKEN_TYPE_STAR:
            d_parse_parser_advance(_parser);
            result_index = d_parse_ebnf_create_repetition(_parser->grammar,
                                                          _symbol_index);
            break;

        case D_PARSE_TOKEN_TYPE_PLUS:
            d_parse_parser_advance(_parser);
            result_index = d_parse_ebnf_create_one_or_more(_parser->grammar,
                                                           _symbol_index);
            break;

        case D_PARSE_TOKEN_TYPE_QUESTION:
            d_parse_parser_advance(_parser);
            result_index = d_parse_ebnf_create_optional(_parser->grammar,
                                                        _symbol_index);
            break;

        default:
            break;
    }

    return result_index;
}

/* ============================================================================
 * EBNF Parser: Parse a primary element
 *
 *   primary ::= IDENT
 *             | STRING
 *             | '(' alternatives ')'
 *             | '{' alternatives '}'   (zero or more)
 *             | '[' alternatives ']'   (optional)
 *
 *   Returns the symbol index of the parsed element (or synthetic).
 *   Returns -1 if no primary could be parsed.
 * ========================================================================== */

static int
d_parse_ebnf_parser_parse_primary
(
    struct d_parse_parser* _parser
)
{
    struct d_parse_token* token;
    int                   symbol_index;

    token = d_parse_parser_current(_parser);

    /* Identifier */
    if (token->type == D_PARSE_TOKEN_TYPE_IDENT)
    {
        symbol_index =
            d_parse_grammar_add_symbol(_parser->grammar,
                                       token->lexeme,
                                       token->length);

        d_parse_parser_advance(_parser);

        return d_parse_ebnf_apply_postfix(_parser, symbol_index);
    }

    /* String literal */
    if (token->type == D_PARSE_TOKEN_TYPE_STRING)
    {
        symbol_index =
            d_parse_grammar_add_symbol(_parser->grammar,
                                       token->lexeme,
                                       token->length);

        _parser->grammar->symbols[symbol_index].kind = D_PARSE_SYMBOL_KIND_TERM;

        d_parse_parser_advance(_parser);

        return d_parse_ebnf_apply_postfix(_parser, symbol_index);
    }

    /* Grouping: ( ... ) */
    if (token->type == D_PARSE_TOKEN_TYPE_LPAREN)
    {
        d_parse_parser_advance(_parser);

        symbol_index = d_parse_ebnf_create_group(_parser);

        token = d_parse_parser_current(_parser);

        if (token->type != D_PARSE_TOKEN_TYPE_RPAREN)
        {
            fprintf(stderr,
                    "parse error at %d:%d: expected ')'\n",
                    token->line,
                    token->column);

            exit(EXIT_FAILURE);
        }

        d_parse_parser_advance(_parser);

        return d_parse_ebnf_apply_postfix(_parser, symbol_index);
    }

    /* Repetition: { ... } (zero or more) */
    if (token->type == D_PARSE_TOKEN_TYPE_LBRACE)
    {
        int group_index;
        int rep_index;

        d_parse_parser_advance(_parser);

        group_index = d_parse_ebnf_create_group(_parser);

        token = d_parse_parser_current(_parser);

        if (token->type != D_PARSE_TOKEN_TYPE_RBRACE)
        {
            fprintf(stderr,
                    "parse error at %d:%d: expected '}'\n",
                    token->line,
                    token->column);

            exit(EXIT_FAILURE);
        }

        d_parse_parser_advance(_parser);

        /* { X } is equivalent to X* */
        rep_index = d_parse_ebnf_create_repetition(_parser->grammar,
                                                   group_index);

        return d_parse_ebnf_apply_postfix(_parser, rep_index);
    }

    /* Optional: [ ... ] (zero or one) */
    if (token->type == D_PARSE_TOKEN_TYPE_LBRACKET)
    {
        int group_index;
        int opt_index;

        d_parse_parser_advance(_parser);

        group_index = d_parse_ebnf_create_group(_parser);

        token = d_parse_parser_current(_parser);

        if (token->type != D_PARSE_TOKEN_TYPE_RBRACKET)
        {
            fprintf(stderr,
                    "parse error at %d:%d: expected ']'\n",
                    token->line,
                    token->column);

            exit(EXIT_FAILURE);
        }

        d_parse_parser_advance(_parser);

        /* [ X ] is equivalent to X? */
        opt_index = d_parse_ebnf_create_optional(_parser->grammar,
                                                 group_index);

        return d_parse_ebnf_apply_postfix(_parser, opt_index);
    }

    /* No primary found */
    return -1;
}

/* ============================================================================
 * EBNF Parser: Check if token can start a primary
 * ========================================================================== */

static int
d_parse_ebnf_is_primary_start
(
    enum DParseTokenType _type
)
{
    return ( (_type == D_PARSE_TOKEN_TYPE_IDENT)
             || (_type == D_PARSE_TOKEN_TYPE_STRING)
             || (_type == D_PARSE_TOKEN_TYPE_LPAREN)
             || (_type == D_PARSE_TOKEN_TYPE_LBRACE)
             || (_type == D_PARSE_TOKEN_TYPE_LBRACKET) );
}

/* ============================================================================
 * EBNF Parser: Check if token ends an alternative
 * ========================================================================== */

static int
d_parse_ebnf_is_alternative_end
(
    struct d_parse_parser* _parser,
    struct d_parse_token*  _token
)
{
    const struct d_parse_bnf_config* config;
    enum DParseTokenType             type;

    config = _parser->lexer->config;
    type   = _token->type;

    /* End of input */
    if (type == D_PARSE_TOKEN_TYPE_STOP)
    {
        return 1;
    }

    /* Pipe separates alternatives */
    if (type == D_PARSE_TOKEN_TYPE_PIPE)
    {
        return 1;
    }

    /* Closing brackets */
    if ( (type == D_PARSE_TOKEN_TYPE_RPAREN)
         || (type == D_PARSE_TOKEN_TYPE_RBRACE)
         || (type == D_PARSE_TOKEN_TYPE_RBRACKET) )
    {
        return 1;
    }

    /* Rule terminator token (if configured) */
    if (config
        && (config->rule_end_mode == D_PARSE_BNF_RULE_END_MODE_TOKEN)
        && d_parse_token_matches_cstring(_token,
                                         config->rule_end_token_text))
    {
        return 1;
    }

    /* Next rule head: IDENT followed by ::= */
    if (type == D_PARSE_TOKEN_TYPE_IDENT)
    {
        struct d_parse_token* next;

        next = d_parse_parser_peek(_parser);

        if (next->type == D_PARSE_TOKEN_TYPE_COLON_COLON_EQ)
        {
            return 1;
        }
    }

    return 0;
}

/* ============================================================================
 * EBNF Parser: Parse an alternative (sequence of primaries)
 * ========================================================================== */

static void
d_parse_ebnf_parser_parse_alternative
(
    struct d_parse_parser* _parser,
    int                    _lhs_index
)
{
    int*                       tmp_rhs;
    size_t                     tmp_count;
    size_t                     tmp_capacity;
    struct d_parse_token*      token;
    struct d_parse_production* production;

    tmp_rhs      = NULL;
    tmp_count    = 0u;
    tmp_capacity = 0u;

    token = d_parse_parser_current(_parser);

    while (!d_parse_ebnf_is_alternative_end(_parser, token))
    {
        int    symbol_index;
        size_t new_capacity;

        if (!d_parse_ebnf_is_primary_start(token->type))
        {
            fprintf(stderr,
                    "parse error at %d:%d: unexpected token in alternative\n",
                    token->line,
                    token->column);

            exit(EXIT_FAILURE);
        }

        symbol_index = d_parse_ebnf_parser_parse_primary(_parser);

        if (symbol_index < 0)
        {
            break;
        }

        if (tmp_count == tmp_capacity)
        {
            new_capacity =
                (tmp_capacity != 0u) ? (tmp_capacity * 2u) : 4u;

            tmp_rhs =
                d_parse_realloc(tmp_rhs, new_capacity * sizeof(int));

            tmp_capacity = new_capacity;
        }

        tmp_rhs[tmp_count] = symbol_index;
        tmp_count         += 1u;

        token = d_parse_parser_current(_parser);
    }

    production = d_parse_grammar_new_production(_parser->grammar);
    production->lhs_index  = _lhs_index;
    production->rhs_length = tmp_count;

    if (tmp_count > 0u)
    {
        production->rhs_indices =
            (int*)malloc(tmp_count * sizeof(int));

        if (!production->rhs_indices)
        {
            fprintf(stderr,
                    "d_parse_ebnf_parser_parse_alternative: out of memory\n");

            exit(EXIT_FAILURE);
        }

        memcpy(production->rhs_indices,
               tmp_rhs,
               tmp_count * sizeof(int));
    }
    else
    {
        production->rhs_indices = NULL;  /* epsilon */
    }

    free(tmp_rhs);

    return;
}

/* ============================================================================
 * EBNF Parser: Parse the RHS of a rule
 * ========================================================================== */

static void
d_parse_ebnf_parser_parse_rhs
(
    struct d_parse_parser* _parser,
    int                    _lhs_index
)
{
    const struct d_parse_bnf_config* config;
    struct d_parse_token*            token;

    config = _parser->lexer->config;

    d_parse_ebnf_parser_parse_alternative(_parser, _lhs_index);

    token = d_parse_parser_current(_parser);

    while (token->type == D_PARSE_TOKEN_TYPE_PIPE)
    {
        d_parse_parser_advance(_parser);

        d_parse_ebnf_parser_parse_alternative(_parser, _lhs_index);

        token = d_parse_parser_current(_parser);
    }

    /* Consume rule-end token if configured */
    if (config && (config->rule_end_mode == D_PARSE_BNF_RULE_END_MODE_TOKEN))
    {
        token = d_parse_parser_current(_parser);

        if (d_parse_token_matches_cstring(token,
                                          config->rule_end_token_text))
        {
            d_parse_parser_advance(_parser);
        }
    }

    return;
}

/* ============================================================================
 * EBNF Parser: Parse a rule
 * ========================================================================== */

static void
d_parse_ebnf_parser_parse_rule
(
    struct d_parse_parser* _parser
)
{
    struct d_parse_token*  token;
    int                    lhs_index;
    struct d_parse_symbol* lhs_symbol;

    token = d_parse_parser_current(_parser);

    d_parse_parser_expect(_parser,
                          D_PARSE_TOKEN_TYPE_IDENT,
                          "nonterminal name");

    lhs_index =
        d_parse_grammar_add_symbol(_parser->grammar,
                                   token->lexeme,
                                   token->length);

    lhs_symbol = &_parser->grammar->symbols[lhs_index];
    lhs_symbol->kind   = D_PARSE_SYMBOL_KIND_NONTERM;
    lhs_symbol->is_lhs = 1;

    d_parse_parser_advance(_parser);

    d_parse_parser_expect(_parser,
                          D_PARSE_TOKEN_TYPE_COLON_COLON_EQ,
                          "'::='");

    d_parse_parser_advance(_parser);

    d_parse_ebnf_parser_parse_rhs(_parser, lhs_index);

    return;
}

/* ============================================================================
 * EBNF Parser: Parse a start directive
 * ========================================================================== */

static void
d_parse_ebnf_parser_parse_start
(
    struct d_parse_parser* _parser
)
{
    const struct d_parse_bnf_config* config;
    struct d_parse_token*            token;
    int                              index;
    struct d_parse_symbol*           symbol;

    config = _parser->lexer->config;

    /* At entry, current token is the START directive */
    d_parse_parser_advance(_parser);

    token = d_parse_parser_current(_parser);

    d_parse_parser_expect(_parser,
                          D_PARSE_TOKEN_TYPE_IDENT,
                          "start symbol name");

    index =
        d_parse_grammar_add_symbol(_parser->grammar,
                                   token->lexeme,
                                   token->length);

    symbol = &_parser->grammar->symbols[index];
    symbol->kind   = D_PARSE_SYMBOL_KIND_NONTERM;
    symbol->is_lhs = 1;

    _parser->grammar->start_symbol_index = index;

    d_parse_parser_advance(_parser);

    if (config && config->rule_end_mode == D_PARSE_BNF_RULE_END_MODE_TOKEN)
    {
        token = d_parse_parser_current(_parser);

        if (d_parse_token_matches_cstring(token,
                                          config->rule_end_token_text))
        {
            d_parse_parser_advance(_parser);
        }
    }

    return;
}

/* ============================================================================
 * Public interface
 * ========================================================================== */

void
d_parse_grammar_from_ebnf_with_config
(
    struct d_parse_grammar*          _grammar,
    const char*                      _source,
    const struct d_parse_bnf_config* _config
)
{
    struct d_parse_parser            parser;
    struct d_parse_lexer             lexer;
    struct d_parse_token*            token;
    struct d_parse_bnf_config        ebnf_config;
    const struct d_parse_bnf_config* effective_config;
    size_t                           i;

    if (!_grammar || !_source)
    {
        fprintf(stderr,
                "d_parse_grammar_from_ebnf_with_config: invalid argument\n");

        exit(EXIT_FAILURE);
    }

    /* Ensure EBNF extensions are enabled */
    if (_config)
    {
        ebnf_config = *_config;
        ebnf_config.ebnf_extensions = 1;
        effective_config = &ebnf_config;
    }
    else
    {
        d_parse_bnf_config_init_ebnf(&ebnf_config);
        effective_config = &ebnf_config;
    }

    d_parse_grammar_init(_grammar);

    memset(&parser, 0, sizeof(parser));
    memset(&lexer,  0, sizeof(lexer));

    parser.grammar       = _grammar;
    parser.lexer         = &lexer;
    parser.has_lookahead = 0;

    d_parse_lexer_init(parser.lexer, _source, effective_config);

    parser.current =
        d_parse_lexer_next(parser.lexer);

    token = d_parse_parser_current(&parser);

    while (token->type != D_PARSE_TOKEN_TYPE_STOP)
    {
        const struct d_parse_bnf_config* config;

        config = parser.lexer->config;

        /* END-of-grammar directive (if enabled) */
        if (config
            && (config->end_mode == D_PARSE_BNF_END_MODE_TOKEN)
            && (token->type == D_PARSE_TOKEN_TYPE_IDENT)
            && d_parse_token_matches_cstring(token,
                                             config->end_token_text))
        {
            d_parse_parser_advance(&parser);
            break;
        }

        /* START directive (if enabled) */
        if (config
            && (config->start_mode == D_PARSE_BNF_START_MODE_TOKEN)
            && (token->type == D_PARSE_TOKEN_TYPE_IDENT)
            && d_parse_token_matches_cstring(token,
                                             config->start_token_text))
        {
            d_parse_ebnf_parser_parse_start(&parser);

            token = d_parse_parser_current(&parser);
            continue;
        }

        /* Rule-begin directive (if enabled) */
        if (config
            && (config->rule_begin_mode == D_PARSE_BNF_RULE_BEGIN_MODE_TOKEN)
            && (token->type == D_PARSE_TOKEN_TYPE_IDENT)
            && d_parse_token_matches_cstring(token,
                                             config->rule_begin_token_text))
        {
            d_parse_parser_advance(&parser);

            d_parse_parser_expect(&parser,
                                  D_PARSE_TOKEN_TYPE_IDENT,
                                  "nonterminal name after rule-begin token");

            d_parse_ebnf_parser_parse_rule(&parser);

            token = d_parse_parser_current(&parser);
            continue;
        }

        /* Default: inferred rule head "IDENT ::=" */
        if (token->type == D_PARSE_TOKEN_TYPE_IDENT)
        {
            struct d_parse_token* next;

            next = d_parse_parser_peek(&parser);

            if (next->type == D_PARSE_TOKEN_TYPE_COLON_COLON_EQ)
            {
                d_parse_ebnf_parser_parse_rule(&parser);
            }
            else
            {
                fprintf(stderr,
                        "parse error at %d:%d: expected '::=' after rule head\n",
                        token->line,
                        token->column);

                exit(EXIT_FAILURE);
            }
        }
        else
        {
            fprintf(stderr,
                    "parse error at %d:%d: unexpected token at top level\n",
                    token->line,
                    token->column);

            exit(EXIT_FAILURE);
        }

        token = d_parse_parser_current(&parser);
    }

    /* If no explicit START directive was given, infer from first LHS. */
    if (_grammar->start_symbol_index < 0)
    {
        for (i = 0u; i < _grammar->symbol_count; ++i)
        {
            if (_grammar->symbols[i].is_lhs)
            {
                _grammar->start_symbol_index = (int)i;
                break;
            }
        }
    }

    d_parse_grammar_classify_symbols(_grammar);

    return;
}

void
d_parse_grammar_from_ebnf
(
    struct d_parse_grammar* _grammar,
    const char*             _source
)
{
    d_parse_grammar_from_ebnf_with_config(_grammar, _source, NULL);

    return;
}

#endif  /* DJINTERP_PARSE_GRAMMAR_EBNF_ */
