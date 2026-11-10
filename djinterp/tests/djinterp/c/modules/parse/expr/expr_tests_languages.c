/*******************************************************************************
* djinterp [c/parse]                                      expr_tests_languages.c
*
* Expression core v2's C suite, block "languages": the cases of math's,
* the command language's, an awk-like language's and dawk's tables.
*
*
* path:      /tests/djinterp/c/modules/parse/expr/expr_tests_languages.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "./expr_tests.h"  // corresponding header

/*
d_tests_expr_math_cases
  Math's cases, the design's section 6.3, each as its term.
  Tests the following:
  - indexing, the transpose and the factorial after any operand (S252)
  - calls only after an ungrouped name, products otherwise (S201, S196)
  - bars opening and closing by where they stand (S207)
  - chains joined by `and`, a grouped relation no link (S214, S215)
  - logic below the relations (S213); literals with rows; empty lists
*/
bool
d_tests_expr_math_cases(void)
{
    static const struct dj_tests_expr_case CASES[] =
    {
        { "-v[1]",            "neg(index(v, 1))" },
        { "2 ^ v[1]",         "pow(2, index(v, 1))" },
        { "A[1][2]",          "index(index(A, 1), 2)" },
        { "x'[1]",            "index(trans(x), 1)" },
        { "f(x)[1]",          "index(call(f, x), 1)" },
        { "(a + b)[1]",       "index(add(a, b), 1)" },
        { "A' ^ 2",           "pow(trans(A), 2)" },
        { "A ^ 2'",           "trans(pow(A, 2))" },
        { "-A'",              "neg(trans(A))" },
        { "x!'",              "trans(fact(x))" },
        { "2 ^ 3!",           "pow(2, fact(3))" },
        { "-3!",              "neg(fact(3))" },
        { "n!!",              "fact(fact(n))" },
        { "sin(x) ^ 2",       "pow(call(sin, x), 2)" },
        { "2 sin(x)",         "mul(2, call(sin, x))" },
        { "2(y)",             "mul(2, y)" },
        { "(x)(y)",           "mul(x, y)" },
        { "x(y)",             "call(x, y)" },
        { "|x| |y|",          "mul(abs(x), abs(y))" },
        { "||x| - 1|",        "abs(sub(abs(x), 1))" },
        { "a < b <= c",       "and*(lt(a, b), le(b, c))" },
        { "a < b < c < d",    "and*(and*(lt(a, b), lt(b, c)), lt(c, d))" },
        { "(a < b) = c",      "eq(lt(a, b), c)" },
        { "x > 0 and x < 1",  "and(gt(x, 0), lt(x, 1))" },
        { "not x = y",        "not(eq(x, y))" },
        { "[1, 2; 3, 4]",     "matrix(row(1, 2), row(3, 4))" },
        { "[a, b]",           "vector(a, b)" },
        { "[]",               "vector()" },
        { "f()",              "call(f)" },
        { "1/2x",             "mul(div(1, 2), x)" },
        { "-x ^ 2",           "neg(pow(x, 2))" },
        { "-x * y",           "mul(neg(x), y)" },
        { "a - b - c",        "sub(sub(a, b), c)" },
        { "a ^ b ^ c",        "pow(a, pow(b, c))" },
        { "a mod b * c",      "mul(mod(a, b), c)" }
    };
    struct dj_parse_expr_signature s;
    bool                           ok = d_tests_expr_math(&s, true);

    ok = d_tests_expr_expect(ok && (d_tests_expr_cases(
                                 &s, CASES, sizeof(CASES) / sizeof(CASES[0]),
                                 0u, 0u)),
                             "every case reads as its term", __LINE__);
    d_parse_expr_signature_release(&s);

    return ok;
}

/*
d_tests_expr_cli_cases
  The command language's cases, the design's section 6.4.
  Tests the following:
  - a command takes its options and operands as leaves
  - `|` and `;` flatten into one node each, `;` the looser
  - `not` binds above `and`; groups
  - an operator only a hook builds (the command) seals, and renders by its
    form: each case is written back as it was read (lane G, 11-05)
*/
bool
d_tests_expr_cli_cases(void)
{
    static const struct dj_tests_expr_case CASES[] =
    {
        { "ls -l | grep x ; echo done",
          "seq(pipe(cmd(ls, -l), cmd(grep, x)), cmd(echo, done))" },
        { "a | b | c",          "pipe(cmd(a), cmd(b), cmd(c))" },
        { "not a and b",        "and(not(cmd(a)), cmd(b))" },
        { "(a ; b) | c",        "pipe(seq(cmd(a), cmd(b)), cmd(c))" },
        { "a == 1 or b =~ c",   "or(eqeq(cmd(a), 1), match(cmd(b), cmd(c)))" }
    };
    struct dj_parse_expr_signature s;
    bool                           ok = d_tests_expr_cli(&s);

    ok = d_tests_expr_expect(ok && (d_tests_expr_cases(
                                 &s, CASES, sizeof(CASES) / sizeof(CASES[0]),
                                 D_TESTS_EXPR_LEX_OPTIONS, 0u)),
                             "every case reads as its term", __LINE__);

    for (size_t i = 0u; ( (ok) && (i < sizeof(CASES) / sizeof(CASES[0])) );
         ++i)
    {
        static struct dj_tests_expr_run     run;
        struct dj_parse_expr_render_options plain;
        char                                text[128];

        memset(&run, 0, sizeof(run));
        memset(&plain, 0, sizeof(plain));
        (void)d_tests_expr_parse(&run, &s, CASES[i].text,
                                 D_TESTS_EXPR_LEX_OPTIONS);
        plain.text        = CASES[i].text;
        plain.text_length = strlen(CASES[i].text);
        (void)d_parse_expr_render_buffer(&s, &run.term, run.result.root,
                                         &plain, text, sizeof(text));
        ok = d_tests_expr_expect(strcmp(text, CASES[i].text) == 0,
                                 CASES[i].text, __LINE__);
        d_parse_expr_term_release(&run.term);
    }

    d_parse_expr_signature_release(&s);

    return ok;
}

/*
d_tests_expr_awk_cases
  The awk-like language's cases, the design's section 6.4.
  Tests the following:
  - concatenation by a flattening juxtaposition; `-` after an operand still
    subtracts
  - `?:` right-associative with a tail; `if ... then ... else` of three parts
  - `++` prefix and postfix by position
  - a call only with no space before its bracket
  - a role a mode turns off
*/
bool
d_tests_expr_awk_cases(void)
{
    static const struct dj_tests_expr_case CASES[] =
    {
        { "a b c",                 "concat(a, b, c)" },
        { "a - 1",                 "sub(a, 1)" },
        { "a -1 b",                "concat(sub(a, 1), b)" },
        { "c ? a : d ? b : x",     "cond(c, a, cond(d, b, x))" },
        { "if c then a else b c",  "if(c, a, concat(b, c))" },
        { "++i i++",               "concat(preinc(i), postinc(i))" },
        { "f(x)",                  "call(f, x)" },
        { "f (x)",                 "concat(f, x)" },
        { "a > b",                 "gt(a, b)" }
    };
    static const struct dj_tests_expr_case MODE2[] =
    {
        { "a b",                   "concat(a, b)" }
    };
    struct dj_parse_expr_signature s;
    static struct dj_tests_expr_run run;
    bool                           ok = d_tests_expr_awk(&s);

    ok = d_tests_expr_expect(ok && (d_tests_expr_cases(
                                 &s, CASES, sizeof(CASES) / sizeof(CASES[0]),
                                 0u, 0u)),
                             "every case reads as its term", __LINE__);
    ok = d_tests_expr_expect(ok && (d_tests_expr_cases(&s, MODE2, 1u, 0u,
                                                       2u)),
                             "mode 2 reads what it has", __LINE__);

    memset(&run, 0, sizeof(run));
    run.options.mode = 2u;
    (void)d_tests_expr_parse(&run, &s, "a > b", 0u);
    ok = d_tests_expr_expect(
             (ok) && (run.status == D_PARSE_EXPR_ERROR) &&
             (run.result.code == D_PARSE_EXPR_DIAG_UNEXPECTED) &&
             (run.result.span.offset == 2u),
             "mode 2 has no `>`", __LINE__);
    d_parse_expr_term_release(&run.term);
    d_parse_expr_signature_release(&s);

    return ok;
}


/*
d_tests_expr_dawk_cases
  dawk's expressions, the cases of lane H's review of 11-05.
  Tests the following:
  - a postfix step after something that cannot be assigned begins the next
    operand instead (point 1), and `$` is above it (`$i++`, `$++i`)
  - a bracketed list is a node of its own, one item a group (point 2); the
    check refuses the list anywhere but before `in`
  - every level of the ladder, `| getline` between the relations and
    concatenation, its left the whole concatenation (point 3)
  - `getline` and `getline var` (point 4, the rest with the sub-parse)
  - `^` above a sign, its right operand free to begin with one (point 6)
  - a call only where `(` touches its name (point 7)
  - what may and may not begin a concatenated operand (point 8)
*/
bool
d_tests_expr_dawk_cases(void)
{
    static const struct dj_tests_expr_case CASES[] =
    {
        { "1 ++i",               "concat(1, preinc(i))" },
        { "a ++b",               "concat(postinc(a), b)" },
        { "$i++",                "postinc(field(i))" },
        { "$++i",                "field(preinc(i))" },
        { "++$i",                "preinc(field(i))" },
        { "arr[i, j]--",         "postdec(index(arr, i, j))" },
        { "(a)++ b",             "concat(a, preinc(b))" },
        { "(i, j) in arr",       "in(group(i, j), arr)" },
        { "(a)",                 "a" },
        { "a = b = c",           "assign(a, assign(b, c))" },
        { "a = b ? c : d ? x : y", "assign(a, cond(b, c, cond(d, x, y)))" },
        { "a || b && c",         "or(a, and(b, c))" },
        { "a && b in arr",       "and(a, in(b, arr))" },
        { "a in arr && b",       "and(in(a, arr), b)" },
        { "a ~ b < c",           "match(a, lt(b, c))" },
        { "a !~ b",              "nomatch(a, b)" },
        { "a < b c",             "lt(a, concat(b, c))" },
        { "cmd | getline line > 0", "gt(pipeget(cmd, line), 0)" },
        { "a b | getline",       "pipeget(concat(a, b))" },
        { "a b + c",             "concat(a, add(b, c))" },
        { "a + b * c % d",       "add(a, mod(mul(b, c), d))" },
        { "getline line",        "getline(line)" },
        { "x = getline",         "assign(x, getline())" },
        { "-2^2",                "neg(pow(2, 2))" },
        { "2^-3",                "pow(2, neg(3))" },
        { "2^-3^2",              "pow(2, neg(pow(3, 2)))" },
        { "f(x)",                "call(f, x)" },
        { "f (x)",               "concat(f, x)" },
        { "a b -1",              "concat(a, sub(b, 1))" },
        { "a -1",                "sub(a, 1)" },
        { "!a b",                "concat(not(a), b)" },
        { "a !b",                "concat(a, not(b))" },
        { "a $1",                "concat(a, field(1))" },
        { "a (b)",               "concat(a, b)" }
    };
    static const struct dj_tests_expr_error ERRORS[] =
    {
        { "a < b < c", D_PARSE_EXPR_DIAG_NON_ASSOCIATIVE, 6u, 1u, 1u },
        { "a getline", D_PARSE_EXPR_DIAG_UNEXPECTED,      2u, 7u, 1u },
        { "a | b",     D_PARSE_EXPR_DIAG_UNEXPECTED,      2u, 1u, 1u }
    };
    struct dj_parse_expr_signature  s;
    static struct dj_tests_expr_run run;
    bool                            ok = d_tests_expr_dawk(&s);

    ok = d_tests_expr_expect(ok && (d_tests_expr_cases(
                                 &s, CASES, sizeof(CASES) / sizeof(CASES[0]),
                                 D_TESTS_EXPR_LEX_DAWK, 0u)),
                             "every case reads as its term", __LINE__);
    ok = d_tests_expr_expect(ok && (d_tests_expr_errors(
                                 &s, ERRORS,
                                 sizeof(ERRORS) / sizeof(ERRORS[0]), 0u)),
                             "each error is the one expected", __LINE__);

    // the check keeps a bracketed list before `in`, and refuses it elsewhere
    memset(&run, 0, sizeof(run));
    run.options.check = d_tests_expr_dawk_list;
    (void)d_tests_expr_parse(&run, &s, "(i, j) in arr",
                             D_TESTS_EXPR_LEX_DAWK);
    ok = d_tests_expr_expect(ok && (run.status == D_PARSE_EXPR_OK),
                             "a list before `in` is kept", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    run.options.check = d_tests_expr_dawk_list;
    (void)d_tests_expr_parse(&run, &s, "x + (i, j)",
                             D_TESTS_EXPR_LEX_DAWK);
    ok = d_tests_expr_expect(
             (ok) && (run.status == D_PARSE_EXPR_ERROR) &&
             (run.result.domain == D_PARSE_DIAG_DOMAIN_USER) &&
             (run.result.code == 8u) && (run.result.span.offset == 4u) &&
             (run.result.span.length == 6u),
             "a list anywhere else is refused, at the list", __LINE__);
    d_parse_expr_term_release(&run.term);
    d_parse_expr_signature_release(&s);

    return ok;
}
