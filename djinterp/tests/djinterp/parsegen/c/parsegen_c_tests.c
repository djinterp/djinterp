/*******************************************************************************
* djinterp [parsegen]                                         parsegen_c_tests.c
*
* Parsegen's C suite: a grammar built, rendered, analysed and routed, each
* through the C API alone and on storage the sections declare.
*   ONE GAP IN THE C API SHOWS HERE. Facts, pools, programs, registries,
* operator sets and sinks each have an initializer that binds caller storage; a
* grammar has none, so d_tests_parsegen_c_scratch_bind binds its fields by
* hand -- the same fields fixed_grammar binds in C++. The day C gains
* d_parsegen_grammar_init_fixed, that function is one call.
*
*
* path:      /tests/djinterp/parsegen/c/parsegen_c_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./parsegen_c_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <string.h>   // strcmp
// djinterp
#include "../../../../inc/djinterp/parse/c/charset.h"      // d_parse_charset
#include "../../../../inc/djinterp/parse/c/diagnostic.h"   // the sink
#include "../../../../inc/djinterp/parsegen/c/analysis.h"  // facts, analyze
#include "../../../../inc/djinterp/parsegen/c/grammar.h"   // the grammar
#include "../../../../inc/djinterp/parsegen/c/registry.h"  // stage selection
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // int32_t, uint8_t,
                                                     // uint16_t, uint32_t


// d_tests_parsegen_c_scratch
//   struct: a grammar over storage the struct itself carries, so that a
// section needs no allocator.
struct d_tests_parsegen_c_scratch
{
    struct d_parsegen_node    nodes[32];
    struct d_parsegen_rule    rules[4];
    char                      bytes[256];
    struct d_parse_pool_entry entries[16];
    struct d_parsegen_grammar grammar;
};

// d_tests_parsegen_c_facts
//   struct: the facts analysis fills for a scratch grammar, over storage the
// struct itself carries.
struct d_tests_parsegen_c_facts
{
    struct d_parsegen_node_facts nodes[32];
    struct d_parsegen_rule_facts rules[4];
    uint32_t                     work[8];
    uint8_t                      mark[4];
    int32_t                      stack[32];
    struct d_parsegen_facts      facts;
};

/*
d_tests_parsegen_c_scratch_bind
  Gives a scratch grammar its storage. C has no fixed-storage initializer for
a grammar, so this binds the fields fixed_grammar binds in C++.
*/
static void
d_tests_parsegen_c_scratch_bind(
    struct d_tests_parsegen_c_scratch* _scratch
)
{
    d_parsegen_grammar_init(&_scratch->grammar);

    _scratch->grammar.nodes         = _scratch->nodes;
    _scratch->grammar.node_capacity = 32u;
    _scratch->grammar.rules         = _scratch->rules;
    _scratch->grammar.rule_capacity = 4u;

    d_parse_pool_init(&_scratch->grammar.pool,
                      _scratch->bytes,
                      (uint32_t)sizeof(_scratch->bytes),
                      _scratch->entries,
                      16u);

    return;
}

/*
d_tests_parsegen_c_facts_bind
  Gives scratch facts their storage, sized for a scratch grammar.
*/
static void
d_tests_parsegen_c_facts_bind(
    struct d_tests_parsegen_c_facts* _facts
)
{
    d_parsegen_facts_init(&_facts->facts,
                          _facts->nodes,
                          32u,
                          _facts->rules,
                          4u,
                          _facts->work,
                          _facts->mark,
                          _facts->stack);

    return;
}

/*
d_tests_parsegen_c_define_digits
  Defines `S = [0-9]+ ;` in a grammar and reports the body it built.
*/
static int32_t
d_tests_parsegen_c_define_digits(
    struct d_parsegen_grammar* _grammar
)
{
    struct d_parse_charset digits = { 0 };

    if (d_parse_charset_parse(&digits, "0-9") != 0)
    {
        return D_PARSEGEN_NO_NODE;
    }

    const int32_t digit = d_parsegen_class(_grammar, &digits);
    const int32_t body  = d_parsegen_repeat(_grammar,
                                            digit,
                                            1u,
                                            D_PARSEGEN_UNBOUNDED);

    if ( (body == D_PARSEGEN_NO_NODE)                         ||
         (d_parsegen_rule_add(_grammar, "S", body, 0u) != 0)  )
    {
        return D_PARSEGEN_NO_NODE;
    }

    return body;
}

/*
d_tests_parsegen_c_define_sum
  Defines the left-recursive `E = E '+' T / T ;` and `T = [0-9] ;` in a
grammar. Each node is built in a statement of its own, since C leaves the
order of the calls in an initializer list unspecified.
*/
static bool
d_tests_parsegen_c_define_sum(
    struct d_parsegen_grammar* _grammar
)
{
    struct d_parse_charset digits = { 0 };

    if (d_parse_charset_parse(&digits, "0-9") != 0)
    {
        return false;
    }

    const int32_t e_ref   = d_parsegen_ref(_grammar, "E");
    const int32_t plus    = d_parsegen_literal(_grammar, "+");
    const int32_t t_ref   = d_parsegen_ref(_grammar, "T");
    const int32_t left[3] = { e_ref, plus, t_ref };
    const int32_t seq     = d_parsegen_sequence(_grammar, left, 3u);
    const int32_t t_alt   = d_parsegen_ref(_grammar, "T");
    const int32_t alts[2] = { seq, t_alt };
    const int32_t body    = d_parsegen_ordered(_grammar, alts, 2u);
    const int32_t digit   = d_parsegen_class(_grammar, &digits);

    return ( (d_parsegen_rule_add(_grammar, "E", body, 0u) == 0)    &&
             (d_parsegen_rule_add(_grammar, "T", digit, 0u) == 0)   &&
             (d_parsegen_grammar_resolve(_grammar, NULL) == 0)      );
}

/*
d_tests_parsegen_c_grammar_build
  A grammar, built from C.
  Tests the following:
  - construction declares the capabilities it uses
  - `+` is a bound, not a counted repetition
  - the result resolves and verifies
*/
static bool
d_tests_parsegen_c_grammar_build(void)
{
    struct d_tests_parsegen_c_scratch scratch = { 0 };

    d_tests_parsegen_c_scratch_bind(&scratch);

    if ( (d_tests_parsegen_c_define_digits(&scratch.grammar) ==
          D_PARSEGEN_NO_NODE)                                           ||
         (d_parsegen_grammar_resolve(&scratch.grammar, NULL) != 0)      ||
         (d_parsegen_grammar_verify(&scratch.grammar, NULL) != 0)       )
    {
        return false;
    }

    return ( (d_parsegen_features_has(scratch.grammar.features,
                                      D_PARSEGEN_CHARACTER_CLASS))      &&
             (!d_parsegen_features_any(scratch.grammar.features,
                                       D_PARSEGEN_BOUNDED_REPEAT))      );
}

/*
d_tests_parsegen_c_grammar_render
  A grammar, as canonical text.
  Tests the following:
  - `S = [0-9]+ ;` renders as exactly that, one rule to a line
*/
static bool
d_tests_parsegen_c_grammar_render(void)
{
    struct d_tests_parsegen_c_scratch scratch  = { 0 };
    char                              text[64] = { 0 };

    d_tests_parsegen_c_scratch_bind(&scratch);

    if (d_tests_parsegen_c_define_digits(&scratch.grammar) ==
        D_PARSEGEN_NO_NODE)
    {
        return false;
    }

    (void)d_parsegen_grammar_render(&scratch.grammar, text, sizeof(text));

    return (strcmp(text, "S = [0-9]+ ;\n") == 0);
}

/*
d_tests_parsegen_c_grammar_unresolved
  A reference to a rule nobody defined.
  Tests the following:
  - resolution fails
  - the failure is found by the grammar's domain and its own code
*/
static bool
d_tests_parsegen_c_grammar_unresolved(void)
{
    struct d_tests_parsegen_c_scratch scratch   = { 0 };
    struct d_parse_diagnostic         items[4]  = { 0 };
    char                              text[256] = { 0 };
    struct d_parse_diag_sink          sink      = { 0 };

    d_tests_parsegen_c_scratch_bind(&scratch);
    d_parse_diag_sink_init(&sink, items, 4u, text, (uint32_t)sizeof(text));

    const int32_t missing = d_parsegen_ref(&scratch.grammar, "Missing");

    return ( (missing != D_PARSEGEN_NO_NODE)                                &&
             (d_parsegen_rule_add(&scratch.grammar,
                                  "R",
                                  missing,
                                  0u) == 0)                                 &&
             (d_parsegen_grammar_resolve(&scratch.grammar, &sink) != 0)     &&
             (d_parse_diag_find(
                  &sink,
                  (uint16_t)D_PARSEGEN_DIAG_DOMAIN_GRAMMAR,
                  (uint16_t)D_PARSEGEN_DIAG_GRAMMAR_UNRESOLVED_REF,
                  0u) >= 0)                                                 );
}

/*
d_tests_parsegen_c_analysis
  What analysis derives that construction cannot see.
  Tests the following:
  - building and resolving a left-recursive grammar declares no left recursion
  - analysis succeeds on it, and finds its first rule left-recursive
  - analysis folds what it derived into the grammar's capability mask
*/
static bool
d_tests_parsegen_c_analysis(void)
{
    struct d_tests_parsegen_c_scratch scratch = { 0 };
    struct d_tests_parsegen_c_facts   facts   = { 0 };

    d_tests_parsegen_c_scratch_bind(&scratch);
    d_tests_parsegen_c_facts_bind(&facts);

    if ( (!d_tests_parsegen_c_define_sum(&scratch.grammar))            ||
         (d_parsegen_features_any(scratch.grammar.features,
                                  D_PARSEGEN_LEFT_RECURSION))          )
    {
        return false;
    }

    return ( (d_parsegen_analyze(&scratch.grammar,
                                 &facts.facts,
                                 NULL) == 0)                           &&
             (d_parsegen_left_recursive(&facts.facts, 0))              &&
             (d_parsegen_features_any(scratch.grammar.features,
                                      D_PARSEGEN_LEFT_RECURSION))      );
}

/*
d_tests_parsegen_c_routing
  A derived capability, routed on.
  Tests the following:
  - a stage is a plain declaration: designated initializers spell it in C
  - once analysed, a left-recursive grammar is passed over by the family that
    rejects left recursion and selects the one that provides it
*/
static bool
d_tests_parsegen_c_routing(void)
{
    struct d_tests_parsegen_c_scratch scratch   = { 0 };
    struct d_tests_parsegen_c_facts   facts     = { 0 };
    struct d_parsegen_stage           stages[2] = { 0 };
    struct d_parsegen_registry        registry  = { 0 };

    const struct d_parsegen_stage peg =
    {
        .name    = "peg",
        .kind    = D_PARSEGEN_STAGE_FAMILY,
        .rejects = D_PARSEGEN_LEFT_RECURSION | D_PARSEGEN_UNORDERED_CHOICE
    };

    const struct d_parsegen_stage lr =
    {
        .name     = "lr",
        .kind     = D_PARSEGEN_STAGE_FAMILY,
        .provides = D_PARSEGEN_LEFT_RECURSION,
        .rejects  = D_PARSEGEN_SYNTACTIC_PREDICATE
    };

    d_tests_parsegen_c_scratch_bind(&scratch);
    d_tests_parsegen_c_facts_bind(&facts);
    d_parsegen_registry_init(&registry, stages, 2u);

    if ( (d_parsegen_registry_add(&registry, &peg) != 0)             ||
         (d_parsegen_registry_add(&registry, &lr) != 0)              ||
         (!d_tests_parsegen_c_define_sum(&scratch.grammar))          ||
         (d_parsegen_analyze(&scratch.grammar,
                             &facts.facts,
                             NULL) != 0)                             )
    {
        return false;
    }

    const struct d_parsegen_stage* const chosen =
        d_parsegen_registry_select(&registry,
                                   D_PARSEGEN_STAGE_FAMILY,
                                   scratch.grammar.features,
                                   NULL);

    return ( (chosen != NULL)                   &&
             (strcmp(chosen->name, "lr") == 0)  );
}

// d_tests_parsegen_c
//   constant: the suite -- every section this build has, the grammar before
// the analysis of it, and the analysis before what is routed on it.
const struct d_tests_section d_tests_parsegen_c[] =
{
    { "parsegen_c_grammar_build",      d_tests_parsegen_c_grammar_build      },
    { "parsegen_c_grammar_render",     d_tests_parsegen_c_grammar_render     },
    { "parsegen_c_grammar_unresolved", d_tests_parsegen_c_grammar_unresolved },
    { "parsegen_c_analysis",           d_tests_parsegen_c_analysis           },
    { "parsegen_c_routing",            d_tests_parsegen_c_routing            }
};

// d_tests_parsegen_c_count
//   constant: how many sections the table above holds in this build.
const size_t d_tests_parsegen_c_count =
    sizeof(d_tests_parsegen_c) / sizeof(d_tests_parsegen_c[0]);
