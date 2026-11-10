/*******************************************************************************
* djinterp [c/parse]                                           expr_tests_term.c
*
* Expression core v2's C suite, block "term": the arena and the
* signature.
*
*
* path:      /tests/djinterp/c/modules/parse/expr/expr_tests_term.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "./expr_tests.h"  // corresponding header

/*
d_tests_expr_term
  The arena and its builders.
  Tests the following:
  - leaves, applications and holes are appended in order, with their depths
  - an application of a child not yet in the term is refused
  - verify holds a built term and refuses a broken one
  - equal compares meaning, spans aside unless asked
  - copy keeps a shared node shared
  - caller storage, once full, refuses another node
*/
bool
d_tests_expr_term(void)
{
    struct dj_parse_expr_term t;
    struct dj_parse_expr_term u;
    bool                      ok = true;

    d_parse_expr_term_init_heap(&t);
    d_parse_expr_term_init_heap(&u);

    const uint32_t a     = d_parse_expr_term_leaf(&t, C_NAME, 0u, 0u, 1u);
    const uint32_t b     = d_parse_expr_term_leaf(&t, C_NAME, 1u, 4u, 1u);
    const uint32_t pair[2] = { a, b };
    const uint32_t lt    = d_parse_expr_term_apply(&t, O_LT, pair, 2u, 0u, 5u);
    const uint32_t ahead[2] = { lt, 99u };

    ok = d_tests_expr_expect(ok && (a == 0u) && (b == 1u) && (lt == 2u),
                             "nodes are appended in order", __LINE__);
    ok = d_tests_expr_expect(ok && (t.nodes[lt].depth == 2u) &&
                             (t.depth == 2u), "depth is kept", __LINE__);
    ok = d_tests_expr_expect(ok && (d_parse_expr_term_apply(
                                 &t, O_AND, ahead, 2u, 0u, 1u) ==
                             D_PARSE_EXPR_NO_NODE),
                             "a child past the term is refused", __LINE__);

    const uint32_t shared[2] = { lt, lt };
    const uint32_t both  = d_parse_expr_term_apply(&t, O_AND, shared, 2u, 0u,
                                                   5u);

    t.root = both;
    ok = d_tests_expr_expect(ok && (d_parse_expr_term_verify(&t)),
                             "a built term verifies", __LINE__);

    const uint32_t copy = d_parse_expr_term_copy(&u, &t, both);

    ok = d_tests_expr_expect(ok && (copy == 3u) && (u.node_count == 4u),
                             "a shared node is copied once", __LINE__);

    // the same copy in the caller's scratch; one byte short is refused
    {
        uint32_t                  room[8];
        struct dj_parse_expr_term w;
        const size_t              need = d_parse_expr_term_copy_scratch(both);

        d_parse_expr_term_init_heap(&w);

        const uint32_t short_copy =
            d_parse_expr_term_copy_with(&w, &t, both, room, need - 1u);
        const uint32_t made =
            d_parse_expr_term_copy_with(&w, &t, both, room, need);

        ok = d_tests_expr_expect(
                 (ok) && (need <= sizeof(room))                           &&
                 (short_copy == D_PARSE_EXPR_NO_NODE)                     &&
                 (made == copy)                                           &&
                 (d_parse_expr_term_equal(&t, both, &w, made, true)),
                 "a copy in the caller's scratch", __LINE__);
        d_parse_expr_term_release(&w);
    }
    ok = d_tests_expr_expect(ok && (d_parse_expr_term_equal(&t, both, &u,
                                                            copy, true)),
                             "the copy is equal, spans and all", __LINE__);
    u.nodes[0].offset = 7u;
    ok = d_tests_expr_expect(ok && (d_parse_expr_term_equal(&t, both, &u,
                                                            copy, false)) &&
                             (!d_parse_expr_term_equal(&t, both, &u, copy,
                                                       true)),
                             "spans count only when asked", __LINE__);
    t.args[0] = 3u;
    ok = d_tests_expr_expect(ok && (!d_parse_expr_term_verify(&t)),
                             "a child after its parent fails verify",
                             __LINE__);

    struct dj_parse_expr_node nodes[2];
    struct dj_parse_expr_term fixed;

    d_parse_expr_term_init(&fixed, nodes, 2u, NULL, 0u);
    (void)d_parse_expr_term_leaf(&fixed, C_NUM, 0u, 0u, 1u);
    (void)d_parse_expr_term_leaf(&fixed, C_NUM, 1u, 0u, 1u);
    ok = d_tests_expr_expect(ok && (d_parse_expr_term_leaf(&fixed, C_NUM, 2u,
                                                           0u, 1u) ==
                                    D_PARSE_EXPR_NO_NODE),
                             "full caller storage refuses a node", __LINE__);

    d_parse_expr_term_release(&t);
    d_parse_expr_term_release(&u);

    return ok;
}

/*
d_tests_expr_signature
  Sealing.
  Tests the following:
  - each of math's, the command language's and the awk-like tables seals
  - an operator defined twice, a chain whose join is no infix operator, a
    role of an undefined operator, a role whose operator does not fit its
    position, and a mixfix of one part each fail the seal, with a report
  - a sealed signature refuses more definitions, and finds by id and class
*/
bool
d_tests_expr_signature(void)
{
    struct dj_parse_expr_signature s;
    struct dj_parse_diagnostic     items[8];
    char                           text[1024];
    struct dj_parse_diag_sink      sink;
    struct dj_parse_expr_mixfix    one = d_tests_expr_mixfix_of(0u, C_LP, "(",
                                                               C_RP, ")");
    uint32_t                       count = 0u;
    bool                           ok    = true;

    ok = d_tests_expr_expect(ok && (d_tests_expr_math(&s, true)),
                             "math's table seals", __LINE__);
    ok = d_tests_expr_expect(ok && (!d_parse_expr_define_atom(&s, C_NUM, 0u)),
                             "a sealed signature refuses more", __LINE__);
    ok = d_tests_expr_expect(ok && (d_parse_expr_find_op(&s, O_POW)) &&
                             (d_parse_expr_find_op(&s, O_POW)->precedence ==
                              80) &&
                             (d_parse_expr_find_roles(&s, C_LP,
                                                      D_PARSE_EXPR_OPERATOR,
                                                      &count)) &&
                             (count == 1u),
                             "operators and roles are found", __LINE__);
    d_parse_expr_signature_release(&s);
    ok = d_tests_expr_expect(ok && (d_tests_expr_cli(&s)),
                             "the command language seals", __LINE__);
    d_parse_expr_signature_release(&s);
    ok = d_tests_expr_expect(ok && (d_tests_expr_awk(&s)),
                             "the awk-like table seals", __LINE__);
    d_parse_expr_signature_release(&s);

    one.parts = 1u;
    d_parse_diag_sink_init(&sink, items, 8u, text, (uint32_t)sizeof(text));
    d_parse_expr_signature_init_heap(&s);
    (void)d_parse_expr_define_op(&s, O_ADD, D_PARSE_EXPR_FORM_INFIX, 50,
                                 D_PARSE_EXPR_ASSOC_LEFT, 0u, "+");
    (void)d_parse_expr_define_op(&s, O_ADD, D_PARSE_EXPR_FORM_INFIX, 50,
                                 D_PARSE_EXPR_ASSOC_LEFT, 0u, "+");
    (void)d_parse_expr_define_chain(&s, O_LT, 40, O_NOT, "<");
    (void)d_parse_expr_define_token(&s, C_STAR, D_PARSE_EXPR_OPERATOR, O_MUL,
                                    0u);
    (void)d_parse_expr_define_token(&s, C_PLUS, D_PARSE_EXPR_OPERAND, O_ADD,
                                    0u);
    (void)d_parse_expr_define_mixfix(&s, O_GROUP, 0, 0u, 0u, &one);
    ok = d_tests_expr_expect(ok && (!d_parse_expr_signature_seal(&s, &sink)),
                             "an unsound table does not seal", __LINE__);
    ok = d_tests_expr_expect(ok && (sink.count == 5u) &&
                             (items[0].domain == D_PARSE_DIAG_DOMAIN_EXPR) &&
                             (items[0].code ==
                              D_PARSE_EXPR_DIAG_BAD_SIGNATURE),
                             "each of its five faults is reported", __LINE__);
    d_parse_expr_signature_release(&s);

    return ok;
}
