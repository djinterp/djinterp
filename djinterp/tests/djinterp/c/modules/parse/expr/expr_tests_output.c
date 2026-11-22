/*******************************************************************************
* djinterp [c][parse]                                        expr_tests_output.c
*
* Expression core v2's C suite, block "output": round trips through the
* renderer, folds and walks, and places in a text.
*
*
* path:      /tests/djinterp/c/modules/parse/expr/expr_tests_output.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "./expr_tests.h"  // corresponding header

#if ( (D_INTERNAL_PARSE_EXPR_HEAP == 1) && \
      (D_INTERNAL_PARSE_EXPR_RENDER == 1) )

/*
d_tests_expr_spans
  A leaf is its span's text, an empty span nothing, and a span the text does
not hold its value (lane G, 11-06: awk's empty block read raw).
*/
static bool
d_tests_expr_spans(
    const struct dj_parse_expr_signature* _s
)
{
    static const char* const            TEXT = "ab cd";
    struct dj_parse_expr_term           t;
    struct dj_parse_expr_render_options plain;
    char                                empty[8];
    char                                outside[8];

    memset(&plain, 0, sizeof(plain));
    plain.text        = TEXT;
    plain.text_length = strlen(TEXT);
    d_parse_expr_term_init_heap(&t);

    const uint32_t none = d_parse_expr_term_leaf(&t, C_NAME, 7u, 2u, 0u);
    const uint32_t far  = d_parse_expr_term_leaf(&t, C_NAME, 7u, 9u, 1u);
    const size_t   n    = d_parse_expr_render_buffer(_s, &t, none, &plain,
                                                     empty, sizeof(empty));
    const size_t   m    = d_parse_expr_render_buffer(_s, &t, far, &plain,
                                                     outside,
                                                     sizeof(outside));
    const bool     ok   = d_tests_expr_expect(
                              (n == 0u) && (empty[0] == '\0') && (m == 1u) &&
                              (strcmp(outside, "7") == 0),
                              "an empty span renders as nothing, a span "
                              "outside the text as its value", __LINE__);

    d_parse_expr_term_release(&t);

    return ok;
}

/*
d_tests_expr_roundtrip
  Rendering and reading back.
  Tests the following:
  - each of 3,000 random terms of every form, rendered, reads back as the
    same term, its brackets only where the parser needs them
  - the design's renderings: `- -x`, `A ^ (2')`, `x'[1]`, `(-x)[1]`
  - a buffer too small is cut and NUL-terminated, the whole length returned
  - a leaf is its span's text, nothing for an empty span, its value for a
    span the text does not hold
*/
bool
d_tests_expr_roundtrip(void)
{
    struct dj_parse_expr_signature      s;
    struct dj_parse_expr_render_options o;
    static struct dj_tests_expr_run     run;
    char                                text[2048];
    uint32_t                            rng  = 2026u;
    uint32_t                            lost = 0u;
    bool                                ok   = d_tests_expr_math(&s, true);

    memset(&o, 0, sizeof(o));
    o.atom = d_tests_expr_atom_name;

    for (uint32_t i = 0u; ( (ok) && (i < 3000u) ); ++i)
    {
        struct dj_parse_expr_term t;

        d_parse_expr_term_init_heap(&t);

        const uint32_t root = d_tests_expr_gen(&t, &rng, 5u);
        const size_t   n    = d_parse_expr_render_buffer(&s, &t, root, &o,
                                                          text, sizeof(text));

        memset(&run, 0, sizeof(run));

        const bool same =
            ( (n < sizeof(text))                                       &&
              (d_tests_expr_parse(&run, &s, text, 0u) ==
               D_PARSE_EXPR_OK)                                        &&
              (d_parse_expr_term_equal(&t, root, &run.term,
                                       run.result.root, false)) );

        if (!same)
        {
            D_TEST_NOTE("a random term rendered as '%s' read back otherwise",
                        text);
            ++lost;
            ok = (lost < 5u);
        }

        d_parse_expr_term_release(&run.term);
        d_parse_expr_term_release(&t);
    }

    ok = d_tests_expr_expect(ok && (lost == 0u),
                             "every random term reads back as itself",
                             __LINE__);

    // the renderings the design names
    static const char* const SHOWN[5][2] =
    {
        { "-(-x)", "- -x" }, { "A ^ (2')", "A ^ (2')" },
        { "(x')[1]", "x'[1]" }, { "(-x)[1]", "(-x)[1]" },
        { "(a + not b) <= n", "(a + not b) <= n" }
    };

    for (int i = 0; i < 5; ++i)
    {
        struct dj_parse_expr_render_options plain;

        memset(&run, 0, sizeof(run));
        memset(&plain, 0, sizeof(plain));
        (void)d_tests_expr_parse(&run, &s, SHOWN[i][0], 0u);
        plain.text        = SHOWN[i][0];
        plain.text_length = strlen(SHOWN[i][0]);
        (void)d_parse_expr_render_buffer(&s, &run.term, run.result.root,
                                         &plain, text, sizeof(text));
        ok = d_tests_expr_expect(ok && (strcmp(text, SHOWN[i][1]) == 0),
                                 SHOWN[i][1], __LINE__);

        if ( (ok) && (i == 3) )
        {
            const size_t whole = d_parse_expr_render_buffer(
                &s, &run.term, run.result.root, &plain, text, 4u);

            ok = d_tests_expr_expect((whole == 7u) &&
                                     (strcmp(text, "(-x") == 0),
                                     "a short buffer is cut", __LINE__);
        }

        d_parse_expr_term_release(&run.term);
    }

    ok = (ok) && (d_tests_expr_spans(&s));

    d_parse_expr_signature_release(&s);

    return ok;
}


#else

/*
d_tests_expr_roundtrip
  The heap or the renderer compiled out: skipped.
*/
bool
d_tests_expr_roundtrip(void)
{
    D_TEST_SKIP(D_TESTS_EXPR_NO_RENDER);
}

#endif  // D_INTERNAL_PARSE_EXPR_HEAP, D_INTERNAL_PARSE_EXPR_RENDER

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)

// dj_tests_expr_eval
//   struct: an evaluation's context: how many leaves it folded.
struct dj_tests_expr_eval
{
    uint32_t leaves;
};

/*
d_tests_expr_eval
  An algebra: integer arithmetic over numbers 0 to 4, relations as 0 or 1,
`and` as both; anything else fails the fold.
*/
static int
d_tests_expr_eval(
    void*                                 _ctx,
    const struct dj_parse_expr_fold_view* _view,
    void*                                 _out)
{
    const struct dj_parse_expr_node* node = &_view->term->nodes[_view->node];
    long*                            out  = (long*)_out;

    if (node->kind == D_PARSE_EXPR_NODE_LEAF)
    {
        ++((struct dj_tests_expr_eval*)_ctx)->leaves;
        *out = (long)node->first - 14L;

        return (node->op == C_NUM) ? 0 : 1;
    }

    if (node->kind == D_PARSE_EXPR_NODE_HOLE)
    {
        *out = 0L;

        return 0;
    }

    const long a = *(const long*)d_parse_expr_fold_child(_view, 0u);
    const long b = (node->count > 1u)
                   ? *(const long*)d_parse_expr_fold_child(_view, 1u)
                   : 0L;

    *out = (node->op == O_ADD) ? (a + b)
         : (node->op == O_SUB) ? (a - b)
         : (node->op == O_MUL) ? (a * b)
         : (node->op == O_NEG) ? (-a)
         : (node->op == O_LT)  ? (long)(a < b)
         : (node->op == O_AND) ? (long)( (a != 0L) && (b != 0L) )
                               : 0L;

    return ( (node->op == O_ADD) || (node->op == O_SUB) ||
             (node->op == O_MUL) || (node->op == O_NEG) ||
             (node->op == O_LT)  || (node->op == O_AND) ) ? 0 : 2;
}

/*
d_tests_expr_skip_all
  A walk's entry that skips every node's children.
*/
static int
d_tests_expr_skip_all(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    uint32_t                         _node,
    uint32_t                         _parent
)
{
    (void)_term;
    (void)_node;
    (void)_parent;
    ++*(int*)_ctx;

    return D_PARSE_EXPR_WALK_SKIP;
}

/*
d_tests_expr_fold_walk
  Folds and walks.
  Tests the following:
  - a fold computes bottom-up in one pass, the operand a chain shares folded
    once
  - an algebra's failure stops the fold and is its answer
  - a term with holes is refused unless the fold asks for them
  - a walk's skip leaves a node's children unvisited, and leave still runs
*/
bool
d_tests_expr_fold_walk(void)
{
    struct dj_parse_expr_signature  s;
    static struct dj_tests_expr_run run;
    struct dj_tests_expr_eval       eval;
    long                            value   = 0L;
    int                             entered = 0;
    bool                            ok      = d_tests_expr_math(&s, true);

    memset(&run, 0, sizeof(run));
    memset(&eval, 0, sizeof(eval));
    (void)d_tests_expr_parse(&run, &s, "-(1 + 2) * 3 + 4", 0u);
    ok = d_tests_expr_expect(ok && (d_parse_expr_fold(
                                 &run.term, run.result.root, sizeof(long),
                                 NULL, NULL, d_tests_expr_eval, &eval, 0u,
                                 &value) == 0) && (value == -5L),
                             "a fold computes", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    memset(&eval, 0, sizeof(eval));
    (void)d_tests_expr_parse(&run, &s, "1 < 2 < 3", 0u);
    ok = d_tests_expr_expect(ok && (d_parse_expr_fold(
                                 &run.term, run.result.root, sizeof(long),
                                 NULL, NULL, d_tests_expr_eval, &eval, 0u,
                                 &value) == 0) && (value == 1L) &&
                             (eval.leaves == 3u),
                             "a shared operand is folded once", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    (void)d_tests_expr_parse(&run, &s, "1 + x", 0u);
    ok = d_tests_expr_expect(ok && (d_parse_expr_fold(
                                 &run.term, run.result.root, sizeof(long),
                                 NULL, NULL, d_tests_expr_eval, &eval, 0u,
                                 &value) == 1),
                             "an algebra's failure is the answer", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    (void)d_tests_expr_parse(&run, &s, "1 +", 0u);
    ok = d_tests_expr_expect(ok && (d_parse_expr_fold(
                                 &run.term, run.result.root, sizeof(long),
                                 NULL, NULL, d_tests_expr_eval, &eval, 0u,
                                 &value) == D_PARSE_EXPR_FOLD_HAS_HOLES) &&
                             (d_parse_expr_fold(
                                 &run.term, run.result.root, sizeof(long),
                                 NULL, NULL, d_tests_expr_eval, &eval,
                                 D_PARSE_EXPR_FOLD_HOLES, &value) == 0) &&
                             (value == 1L),
                             "holes are folded only when asked", __LINE__);
    ok = d_tests_expr_expect(ok && (d_parse_expr_walk(
                                 &run.term, run.result.root,
                                 d_tests_expr_skip_all, NULL, &entered)) &&
                             (entered == 1),
                             "a skip leaves the children unvisited",
                             __LINE__);
    d_parse_expr_term_release(&run.term);
    d_parse_expr_signature_release(&s);

    return ok;
}

#else

/*
d_tests_expr_fold_walk
  The heap compiled out: skipped.
*/
bool
d_tests_expr_fold_walk(void)
{
    D_TEST_SKIP(D_TESTS_EXPR_NO_HEAP);
}

#endif  // D_INTERNAL_PARSE_EXPR_HEAP

/*
d_tests_expr_locate
  Lines and columns.
  Tests the following:
  - `\n`, `\r\n` and a lone `\r` each end one line
  - a column counts characters, a two-byte UTF-8 character as one
  - an offset past the end is the end
*/
bool
d_tests_expr_locate(void)
{
    static const char TEXT[] = "ab\ncd\r\nef\rg\xc3\xa9";
    static const uint32_t WHERE[7][3] =
    {
        { 0u, 1u, 1u }, { 3u, 2u, 1u }, { 5u, 2u, 3u }, { 7u, 3u, 1u },
        { 10u, 4u, 1u }, { 13u, 4u, 3u }, { 99u, 4u, 3u }
    };
    bool ok = true;

    for (int i = 0; i < 7; ++i)
    {
        uint32_t line   = 0u;
        uint32_t column = 0u;

        d_parse_expr_locate(TEXT, sizeof(TEXT) - 1u, WHERE[i][0], &line,
                            &column);
        ok = d_tests_expr_expect(ok && (line == WHERE[i][1]) &&
                                 (column == WHERE[i][2]),
                                 "a line and a column", __LINE__);
    }

    return ok;
}
