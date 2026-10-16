/******************************************************************************
* djinterp [parse]                                                 parse_bnf.h
*
*   Simple BNF-style grammar parser. Supports:
*     - %start Nonterminal ;
*     - Nonterminal ::= alt ('|' alt)* ;
*     - Alternative = sequence of IDENT and 'string' symbols (or empty)
*
* path:      \inc\parse\grammar\parse_bnf.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.09
******************************************************************************/

#ifndef DJINTERP_PARSE_GRAMMAR_BNF_
#define DJINTERP_PARSE_GRAMMAR_BNF_ 1

#include "parse_bnf_common.h"

/* ============================================================================
 * BNF Parser implementation
 * ========================================================================== */

static int
d_parse_bnf_parser_parse_symbol
(
    struct d_parse_parser* _parser
)
{
    struct d_parse_token* token;
    int                   symbol_index;
    enum DParseTokenType  type;

    token = d_parse_parser_current(_parser);
    type  = token->type;

    if ( (type == D_PARSE_TOKEN_TYPE_IDENT)
         || (type == D_PARSE_TOKEN_TYPE_STRING)
         || (type == D_PARSE_TOKEN_TYPE_SEMI) )
    {
        symbol_index =
            d_parse_grammar_add_symbol(_parser->grammar,
                                       token->lexeme,
                                       token->length);

        if ( (type == D_PARSE_TOKEN_TYPE_STRING)
             || (type == D_PARSE_TOKEN_TYPE_SEMI) )
        {
            _parser->grammar->symbols[symbol_index].kind =
                D_PARSE_SYMBOL_KIND_TERM;
        }

        d_parse_parser_advance(_parser);

        return symbol_index;
    }

    fprintf(stderr,
            "parse error at %d:%d: expected identifier, string, or ';'\n",
            token->line,
            token->column);

    exit(EXIT_FAILURE);
}

static void
d_parse_bnf_parser_parse_alternative
(
    struct d_parse_parser* _parser,
    int                    _lhs_index
)
{
    const struct d_parse_bnf_config* config;
    int*                             tmp_rhs;
    size_t                           tmp_count;
    size_t                           tmp_capacity;
    struct d_parse_token*            token;
    enum DParseTokenType             type;
    struct d_parse_production*       production;

    config       = _parser->lexer->config;
    tmp_rhs      = NULL;
    tmp_count    = 0u;
    tmp_capacity = 0u;

    for (;;)
    {
        struct d_parse_token* next;

        token = d_parse_parser_current(_parser);
        type  = token->type;

        if (type == D_PARSE_TOKEN_TYPE_STOP)
        {
            break;
        }

        /* rule terminator token (if configured) */
        if (config
            && (config->rule_end_mode == D_PARSE_BNF_RULE_END_MODE_TOKEN)
            && d_parse_token_matches_cstring(token,
                                             config->rule_end_token_text))
        {
            break;
        }

        /* next rule head? IDENT followed by ::= */
        if (type == D_PARSE_TOKEN_TYPE_IDENT)
        {
            next = d_parse_parser_peek(_parser);

            if (next->type == D_PARSE_TOKEN_TYPE_COLON_COLON_EQ)
            {
                break;
            }
        }
        else if ( (type != D_PARSE_TOKEN_TYPE_STRING)
                  && (type != D_PARSE_TOKEN_TYPE_SEMI) )
        {
            /* anything else ends this alternative */
            break;
        }

        {
            int     symbol_index;
            size_t  new_capacity;

            symbol_index =
                d_parse_bnf_parser_parse_symbol(_parser);

            if (tmp_count == tmp_capacity)
            {
                new_capacity =
                    (tmp_capacity != 0u)
                        ? (tmp_capacity * 2u)
                        : 4u;

                tmp_rhs =
                    d_parse_realloc(tmp_rhs,
                                    new_capacity * sizeof(int));

                tmp_capacity = new_capacity;
            }

            tmp_rhs[tmp_count] = symbol_index;
            tmp_count         += 1u;
        }
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
                    "d_parse_bnf_parser_parse_alternative: out of memory\n");

            exit(EXIT_FAILURE);
        }

        memcpy(production->rhs_indices,
               tmp_rhs,
               tmp_count * sizeof(int));
    }
    else
    {
        production->rhs_indices = NULL;    /* epsilon */
    }

    free(tmp_rhs);

    return;
}

static void
d_parse_bnf_parser_parse_rhs
(
    struct d_parse_parser* _parser,
    int                    _lhs_index
)
{
    const struct d_parse_bnf_config* config;
    struct d_parse_token*            token;

    config = _parser->lexer->config;

    d_parse_bnf_parser_parse_alternative(_parser, _lhs_index);

    token = d_parse_parser_current(_parser);

    while (token->type == D_PARSE_TOKEN_TYPE_PIPE)
    {
        d_parse_parser_advance(_parser);

        d_parse_bnf_parser_parse_alternative(_parser, _lhs_index);

        token = d_parse_parser_current(_parser);
    }

    /* consume rule-end token if configured */
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

static void
d_parse_bnf_parser_parse_rule
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

    d_parse_bnf_parser_parse_rhs(_parser, lhs_index);

    return;
}

static void
d_parse_bnf_parser_parse_start
(
    struct d_parse_parser* _parser
)
{
    const struct d_parse_bnf_config* config;
    struct d_parse_token*            token;
    int                              index;
    struct d_parse_symbol*           symbol;

    config = _parser->lexer->config;

    /* at entry, current token is the START directive */
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
d_parse_grammar_from_bnf_with_config
(
    struct d_parse_grammar*          _grammar,
    const char*                      _source,
    const struct d_parse_bnf_config* _config
)
{
    struct d_parse_parser parser;
    struct d_parse_lexer  lexer;
    struct d_parse_token* token;
    size_t                i;

    if (!_grammar || !_source)
    {
        fprintf(stderr,
                "d_parse_grammar_from_bnf_with_config: invalid argument\n");

        exit(EXIT_FAILURE);
    }

    d_parse_grammar_init(_grammar);

    memset(&parser, 0, sizeof(parser));
    memset(&lexer,  0, sizeof(lexer));

    parser.grammar       = _grammar;
    parser.lexer         = &lexer;
    parser.has_lookahead = 0;

    d_parse_lexer_init(parser.lexer, _source, _config);

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
            d_parse_bnf_parser_parse_start(&parser);

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

            d_parse_bnf_parser_parse_rule(&parser);

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
                d_parse_bnf_parser_parse_rule(&parser);
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
d_parse_grammar_from_bnf
(
    struct d_parse_grammar* _grammar,
    const char*             _source
)
{
    struct d_parse_bnf_config config;

    d_parse_bnf_config_init_default(&config);

    d_parse_grammar_from_bnf_with_config(_grammar,
                                         _source,
                                         &config);

    return;
}

#endif  /* DJINTERP_PARSE_GRAMMAR_BNF_ */
