/*******************************************************************************
* djinterp [c/parse]                                    stress_tests_roundtrip.c
*
* Expression core v2's stress suite, block "roundtrip": seeded random terms,
* rendered and read back by the same signature, over two signatures that
* between them hold every form the core has.
*
*
* path:      /tests/djinterp/c/modules/parse/stress/stress_tests_roundtrip.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "./stress_tests.h"  // corresponding header

#if ( (D_INTERNAL_PARSE_EXPR_RENDER == 1) && \
      (D_INTERNAL_PARSE_EXPR_HEAP == 1) )

// the terms a round trip names before it stops naming them
#define D_INTERNAL_STRESS_SHOWN  5u

// the deepest a term is drawn to, each depth from 1 to it in turn
#define D_INTERNAL_STRESS_DEPTH  6u

// the signature a round trip reads by, and the generator it draws from
typedef bool     (*dj_tests_stress_make_fn)(struct dj_parse_expr_signature*);
typedef uint32_t (*dj_tests_stress_gen_fn)(struct dj_parse_expr_term*,
                                            uint32_t*,
                                            uint32_t);

/*
d_tests_stress_math_make
  Math's signature, its implied product included, as math reads.
*/
static bool
d_tests_stress_math_make(
    struct dj_parse_expr_signature* _s
)
{
    return d_tests_expr_math(_s, true);
}

/*
d_tests_stress_awk_leaf
  A random leaf of the awk-like language: a name of the atoms', or a number.
*/
static uint32_t
d_tests_stress_awk_leaf(
    struct dj_parse_expr_term* _t,
    uint32_t*                  _rng,
    bool                       _name
)
{
    const uint32_t pick = (_name) ? d_tests_expr_random(_rng, 14u)
                                  : d_tests_expr_random(_rng, 19u);

    return d_parse_expr_term_leaf(_t, (pick >= 14u) ? C_NUM : C_NAME, pick,
                                  0u, 0u);
}

static uint32_t d_tests_stress_awk_any(struct dj_parse_expr_term* _t,
                                       uint32_t*                  _rng,
                                       uint32_t                   _depth,
                                       bool                       _in_concat);

/*
d_tests_stress_awk_mixfix
  A random mixfix term of the awk-like language: `c ? a : b`,
`if c then a else b`, or a call of a name with up to two arguments.
*/
static uint32_t
d_tests_stress_awk_mixfix(
    struct dj_parse_expr_term* _t,
    uint32_t*                  _rng,
    uint32_t                   _depth,
    uint32_t                   _form
)
{
    uint32_t c[3];

    if (_form < 2u)
    {
        for (uint32_t i = 0u; i < 3u; ++i)
        {
            c[i] = d_tests_stress_awk_any(_t, _rng, _depth, false);
        }

        return d_parse_expr_term_apply(_t, (_form == 0u) ? O_COND : O_IF, c,
                                       3u, 0u, 0u);
    }

    const uint32_t items = d_tests_expr_random(_rng, 3u);

    c[0] = d_tests_stress_awk_leaf(_t, _rng, true);

    for (uint32_t i = 0u; i < items; ++i)
    {
        c[1u + i] = d_tests_stress_awk_any(_t, _rng, _depth, false);
    }

    return d_parse_expr_term_apply(_t, O_CALL, c, 1u + items, 0u, 0u);
}

/*
d_tests_stress_awk_any
  A random term of the awk-like language, of every form, to a depth. A
concatenation is never directly another's operand, since the language
flattens one into the other: `a b c` is one concatenation of three.
*/
static uint32_t
d_tests_stress_awk_any(
    struct dj_parse_expr_term* _t,
    uint32_t*                  _rng,
    uint32_t                   _depth,
    bool                       _in_concat
)
{
    static const uint32_t BINARY[2] = { O_SUB, O_GT };
    static const uint32_t UNARY[3]  = { O_NEG, O_PREINC, O_POSTINC };
    uint32_t              c[3];

    // leaves where the depth is spent, and now and then before
    if ( (_depth == 0u) ||
         (d_tests_expr_random(_rng, 4u) == 0u) )
    {
        return d_tests_stress_awk_leaf(_t, _rng, false);
    }

    uint32_t form = d_tests_expr_random(_rng, 6u);

    // a concatenation's operand is never one itself
    if ( (form == 0u) &&
         (_in_concat) )
    {
        form = 1u;
    }

    switch (form)
    {
        case 0u:
        {
            const uint32_t n = 2u + d_tests_expr_random(_rng, 2u);

            for (uint32_t i = 0u; i < n; ++i)
            {
                c[i] = d_tests_stress_awk_any(_t, _rng, _depth - 1u, true);
            }

            const uint32_t joined = d_parse_expr_term_apply(_t, O_CONCAT, c, n,
                                                            0u, 0u);

            // as the parser marks it: written with no token, and a run
            // where it collects more than two
            _t->nodes[joined].flags |= D_PARSE_EXPR_NODE_JUXTAPOSED;

            if (n > 2u)
            {
                _t->nodes[joined].flags |= D_PARSE_EXPR_NODE_FLATTENED;
            }

            return joined;
        }

        case 1u:
        case 2u:
            c[0] = d_tests_stress_awk_any(_t, _rng, _depth - 1u, false);
            c[1] = d_tests_stress_awk_any(_t, _rng, _depth - 1u, false);

            return d_parse_expr_term_apply(_t, BINARY[form - 1u], c, 2u, 0u,
                                           0u);

        case 3u:
            c[0] = d_tests_stress_awk_any(_t, _rng, _depth - 1u, false);

            return d_parse_expr_term_apply(
                       _t, UNARY[d_tests_expr_random(_rng, 3u)], c, 1u, 0u,
                       0u);

        default:
            return d_tests_stress_awk_mixfix(_t, _rng, _depth - 1u,
                                             d_tests_expr_random(_rng, 3u));
    }
}

/*
d_tests_stress_awk_gen
  A random term of the awk-like language, to a depth.
*/
static uint32_t
d_tests_stress_awk_gen(
    struct dj_parse_expr_term* _t,
    uint32_t*                  _rng,
    uint32_t                   _depth
)
{
    return d_tests_stress_awk_any(_t, _rng, _depth, false);
}

/*
d_tests_stress_roundtrip
  Draws a count of random terms from a seed, each to a depth from 1 to the
deepest in turn, renders each, reads the text back by the same signature,
and holds the term read to the term drawn. It names the first few that
differ, and how many bytes the renderings came to.
*/
static bool
d_tests_stress_roundtrip(
    dj_tests_stress_make_fn _make,
    dj_tests_stress_gen_fn  _gen,
    uint32_t                _seed,
    const char*             _what
)
{
    struct dj_parse_expr_signature      s;
    struct dj_parse_expr_render_options o;
    static struct dj_tests_expr_run     run;
    static char                         text[4096];
    uint32_t                            rng   = _seed;
    uint32_t                            lost  = 0u;
    unsigned long                       bytes = 0u;
    bool                                ok    = _make(&s);

    memset(&o, 0, sizeof(o));
    o.atom = d_tests_expr_atom_name;

    for (uint32_t i = 0u; ( (ok) && (i < D_TESTS_PARSE_STRESS_TERMS) ); ++i)
    {
        struct dj_parse_expr_term t;

        d_parse_expr_term_init_heap(&t);

        const uint32_t root = _gen(&t, &rng,
                                   1u + (i % D_INTERNAL_STRESS_DEPTH));
        const size_t   n    = d_parse_expr_render_buffer(&s, &t, root, &o,
                                                          text, sizeof(text));

        // the round trip reads back what it rendered, not the depth guard,
        // which a build may set as low as 1
        memset(&run, 0, sizeof(run));
        run.options.max_depth = D_PARSE_EXPR_DEPTH_UNBOUNDED;
        bytes += (unsigned long)n;

        const bool same =
            ( (n < sizeof(text))                                       &&
              (d_tests_expr_parse(&run, &s, text, 0u) ==
               D_PARSE_EXPR_OK)                                        &&
              (d_parse_expr_term_verify(&run.term))                    &&
              (d_parse_expr_term_equal(&t, root, &run.term,
                                       run.result.root, false)) );

        if (!same)
        {
            if (lost < D_INTERNAL_STRESS_SHOWN)
            {
                D_TEST_NOTE("%s: term %lu rendered as '%s' read back "
                            "otherwise", _what, (unsigned long)i, text);
            }

            ++lost;
        }

        d_parse_expr_term_release(&run.term);
        d_parse_expr_term_release(&t);
    }

    D_TEST_NOTE("%s: %lu terms from seed %lu, %lu bytes rendered, %lu read "
                "back otherwise", _what,
                (unsigned long)D_TESTS_PARSE_STRESS_TERMS,
                (unsigned long)_seed, bytes, (unsigned long)lost);

    d_parse_expr_signature_release(&s);

    return ( (ok) && (lost == 0u) );
}

/*
d_tests_stress_math_roundtrip
  Math's random terms of every form, rendered and read back.
  Tests the following:
  - each of D_TESTS_PARSE_STRESS_TERMS terms, drawn from a fixed seed to a
    depth from 1 to 6, reads back as itself
  - each term read back verifies
*/
bool
d_tests_stress_math_roundtrip(void)
{
    return d_tests_expr_expect(
               d_tests_stress_roundtrip(d_tests_stress_math_make,
                                        d_tests_expr_gen, 20261118u,
                                        "math"),
               "every random term of math's reads back as itself",
               __LINE__);
}

/*
d_tests_stress_awk_roundtrip
  The awk-like language's random terms of every form, rendered and read
back.
  Tests the following:
  - each of D_TESTS_PARSE_STRESS_TERMS terms, drawn from a fixed seed to a
    depth from 1 to 6, reads back as itself: concatenations, calls that
    touch their names, `++` before and after, `?:` and `if` with their
    tails
  - each term read back verifies
*/
bool
d_tests_stress_awk_roundtrip(void)
{
    return d_tests_expr_expect(
               d_tests_stress_roundtrip(d_tests_expr_awk,
                                        d_tests_stress_awk_gen, 11182026u,
                                        "awk"),
               "every random term of the awk-like language reads back as "
               "itself",
               __LINE__);
}

#else

/*
d_tests_stress_math_roundtrip
  The renderer or the heap compiled out: skipped.
*/
bool
d_tests_stress_math_roundtrip(void)
{
    D_TEST_SKIP("the renderer or the term's heap is compiled out "
                "(D_CFG_PARSE_EXPR_RENDER or _HEAP is 0)");
}

/*
d_tests_stress_awk_roundtrip
  The renderer or the heap compiled out: skipped.
*/
bool
d_tests_stress_awk_roundtrip(void)
{
    D_TEST_SKIP("the renderer or the term's heap is compiled out "
                "(D_CFG_PARSE_EXPR_RENDER or _HEAP is 0)");
}

#endif  // D_INTERNAL_PARSE_EXPR_RENDER, D_INTERNAL_PARSE_EXPR_HEAP
