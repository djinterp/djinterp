/******************************************************************************
* djinterp [dawk]                                                test_dparse.c
*
*   Unit tests for the scanner and parser.
*     Each case is a program and the shape its tree should have, written in a
* parenthesised form that makes precedence and associativity visible. That is
* the only property worth asserting at this layer: whether the tree groups the
* way POSIX says, not whether it evaluates, which nothing here can yet do.
*
*
* path:      /tests/djinterp/c/dawk/test_dparse.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dparse.h"  // corresponding header
// std
#include <stdio.h>   // printf, snprintf
#include <string.h>  // strlen, strcmp


static int  g_checks = 0;
static int  g_failed = 0;
static char g_out[4096];
static size_t g_used;


/*
d_tests_emit
  Appends formatted text to the rendering buffer.

Parameter(s):
  _format: a printf format.
Return:
  none.
*/
static void
d_tests_emit(
    const char* _format,
    ...
);


#include <stdarg.h>

static void
d_tests_emit(
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int written = vsnprintf(&g_out[g_used],
                                  sizeof(g_out) - g_used,
                                  _format,
                                  args);

    va_end(args);

    // a truncated rendering still leaves the buffer terminated
    if ((written > 0) && ((size_t)written < (sizeof(g_out) - g_used)))
    {
        g_used += (size_t)written;
    }

    return;
}


static void d_tests_show(struct d_awk_node* _node);


/*
d_tests_show_list
  Renders a node's variadic children, separated.

Parameter(s):
  _node:      the node whose list to render.
  _separator: the text placed between children.
Return:
  none.
*/
static void
d_tests_show_list(
    struct d_awk_node* _node,
    const char*        _separator
)
{
    for (size_t at = 0; at < _node->count; ++at)
    {
        // the separator joins children rather than leading them
        if (at > 0)
        {
            d_tests_emit("%s", _separator);
        }

        d_tests_show(_node->list[at]);
    }

    return;
}


/*
d_tests_show
  Renders one node in the parenthesised form the expectations use.

Parameter(s):
  _node: the node to render; may be NULL.
Return:
  none.
*/
static void
d_tests_show(
    struct d_awk_node* _node
)
{
    // an absent child renders as a placeholder rather than vanishing
    if (!_node)
    {
        d_tests_emit("_");
        return;
    }

    switch (_node->kind)
    {
        case D_AWK_N_NUMBER: d_tests_emit("%g", _node->number);   break;
        case D_AWK_N_STRING: d_tests_emit("\"%s\"", _node->text); break;
        case D_AWK_N_REGEX:  d_tests_emit("/%s/", _node->text);   break;
        case D_AWK_N_VAR:    d_tests_emit("%s", _node->text);     break;
        case D_AWK_N_NEXT:   d_tests_emit("next");                break;
        case D_AWK_N_BREAK:  d_tests_emit("break");               break;
        case D_AWK_N_CONTINUE: d_tests_emit("continue");          break;

        case D_AWK_N_FIELD:
            d_tests_emit("$(");
            d_tests_show(_node->a);
            d_tests_emit(")");
            break;

        case D_AWK_N_INDEX:
            d_tests_emit("%s[", _node->a->text);
            d_tests_show_list(_node, ",");
            d_tests_emit("]");
            break;

        case D_AWK_N_ASSIGN:
        case D_AWK_N_BINARY:
        case D_AWK_N_MATCH:
            d_tests_emit("(%s ", d_awk_token_name(_node->op));
            d_tests_show(_node->a);
            d_tests_emit(" ");
            d_tests_show(_node->b);
            d_tests_emit(")");
            break;

        case D_AWK_N_UNARY:
            d_tests_emit("(%s ", d_awk_token_name(_node->op));
            d_tests_show(_node->a);
            d_tests_emit(")");
            break;

        case D_AWK_N_CONCAT:
            d_tests_emit("(cat ");
            d_tests_show(_node->a);
            d_tests_emit(" ");
            d_tests_show(_node->b);
            d_tests_emit(")");
            break;

        case D_AWK_N_TERNARY:
            d_tests_emit("(?: ");
            d_tests_show(_node->a);
            d_tests_emit(" ");
            d_tests_show(_node->b);
            d_tests_emit(" ");
            d_tests_show(_node->c);
            d_tests_emit(")");
            break;

        case D_AWK_N_GROUPLIST:
            d_tests_emit("list[");
            d_tests_show_list(_node, ",");
            d_tests_emit("]");
            break;

        case D_AWK_N_IN:
            d_tests_emit("(in [");
            d_tests_show_list(_node, ",");
            d_tests_emit("] %s)", _node->b->text);
            break;

        case D_AWK_N_PREINCR:
            d_tests_emit("(pre%s ", d_awk_token_name(_node->op));
            d_tests_show(_node->a);
            d_tests_emit(")");
            break;

        case D_AWK_N_POSTINCR:
            d_tests_emit("(post%s ", d_awk_token_name(_node->op));
            d_tests_show(_node->a);
            d_tests_emit(")");
            break;

        case D_AWK_N_CALL:
            d_tests_emit("%s(", _node->text);
            d_tests_show_list(_node, ",");
            d_tests_emit(")");
            break;

        case D_AWK_N_BUILTIN:
            d_tests_emit("@%s(", _node->text);
            d_tests_show_list(_node, ",");
            d_tests_emit(")");
            break;

        case D_AWK_N_BLOCK:
            d_tests_emit("{");
            d_tests_show_list(_node, "; ");
            d_tests_emit("}");
            break;

        case D_AWK_N_EXPR_STMT:
            d_tests_show(_node->a);
            break;

        case D_AWK_N_PRINT:
        case D_AWK_N_PRINTF:
            d_tests_emit((_node->kind == D_AWK_N_PRINT) ? "print("
                                                        : "printf(");
            d_tests_show_list(_node, ",");
            d_tests_emit(")");

            // a redirection is rendered so that dropping one is visible
            if (_node->b)
            {
                d_tests_emit("%s", d_awk_token_name(_node->op));
                d_tests_show(_node->b);
            }

            break;

        case D_AWK_N_IF:
            d_tests_emit("if(");
            d_tests_show(_node->a);
            d_tests_emit(")");
            d_tests_show(_node->b);

            // the alternative is optional
            if (_node->c)
            {
                d_tests_emit("else");
                d_tests_show(_node->c);
            }

            break;

        case D_AWK_N_WHILE:
            d_tests_emit("while(");
            d_tests_show(_node->a);
            d_tests_emit(")");
            d_tests_show(_node->b);
            break;

        case D_AWK_N_DO:
            d_tests_emit("do");
            d_tests_show(_node->a);
            d_tests_emit("while(");
            d_tests_show(_node->b);
            d_tests_emit(")");
            break;

        case D_AWK_N_FOR:
            d_tests_emit("for(");
            d_tests_show(_node->a);
            d_tests_emit(";");
            d_tests_show(_node->b);
            d_tests_emit(";");
            d_tests_show(_node->c);
            d_tests_emit(")");
            d_tests_show(_node->d);
            break;

        case D_AWK_N_FORIN:
            d_tests_emit("forin(%s in %s)", _node->a->text, _node->b->text);
            d_tests_show(_node->d);
            break;

        case D_AWK_N_EXIT:
            d_tests_emit("exit(");
            d_tests_show(_node->a);
            d_tests_emit(")");
            break;

        case D_AWK_N_RETURN:
            d_tests_emit("return(");
            d_tests_show(_node->a);
            d_tests_emit(")");
            break;

        case D_AWK_N_DELETE:
            d_tests_emit("del %s[", _node->a->text);
            d_tests_show_list(_node, ",");
            d_tests_emit("]");
            break;

        case D_AWK_N_GETLINE:
            d_tests_emit("getline");

            // the target is optional and defaults to $0
            if (_node->a)
            {
                d_tests_emit(" ");
                d_tests_show(_node->a);
            }

            // the source is a file, a command, or the main input
            if (_node->op == D_AWK_TOK_LT)
            {
                d_tests_emit(" <");
                d_tests_show(_node->b);
            }
            else if (_node->op == D_AWK_TOK_PIPE)
            {
                d_tests_emit(" |");
                d_tests_show(_node->b);
            }

            break;

        case D_AWK_N_DELETE_ALL:
            d_tests_emit("del %s", _node->a->text);
            break;

        default:
            d_tests_emit("?%d", (int)_node->kind);
            break;
    }

    return;
}


/*
d_tests_expect
  Parses a program and compares its rendered tree against an expectation.

Parameter(s):
  _source:   the program text.
  _expected: the expected rendering, or NULL to expect a parse failure.
Return:
  none.
*/
static void
d_tests_expect(
    const char* _source,
    const char* _expected
)
{
    g_used    = 0;
    g_out[0]  = '\0';
    g_checks++;

    struct d_awk_program* program = d_awk_parse(_source,
                                                strlen(_source),
                                                "-");

    // a program that fails to parse matches only a NULL expectation
    if (!program)
    {
        // report the failure only when a tree was expected
        if (_expected)
        {
            g_failed++;
            printf("  FAIL  %s\n        did not parse: %s\n",
                   _source,
                   d_awk_parse_error());
        }

        return;
    }

    // a program that parses matches only a non-NULL expectation
    if (!_expected)
    {
        g_failed++;
        printf("  FAIL  %s\n        parsed but should have been rejected\n",
               _source);
        d_awk_program_free(program);

        return;
    }

    // functions are rendered before the rules that may call them
    for (size_t at = 0; at < program->function_count; ++at)
    {
        d_tests_emit("fn %s/%zu",
                     program->functions[at].name,
                     program->functions[at].param_count);
        d_tests_show(program->functions[at].body);
        d_tests_emit(" ");
    }

    for (size_t at = 0; at < program->rule_count; ++at)
    {
        struct d_awk_rule* const rule = &program->rules[at];

        // the pattern renders as its kind, its expression, or a wildcard
        if (rule->kind == D_AWK_RULE_BEGIN)
        {
            d_tests_emit("BEGIN");
        }
        else if (rule->kind == D_AWK_RULE_END)
        {
            d_tests_emit("END");
        }
        else if (rule->pattern)
        {
            d_tests_show(rule->pattern);

            // a range carries a second pattern that ends it
            if (rule->pattern_end)
            {
                d_tests_emit(",");
                d_tests_show(rule->pattern_end);
            }
        }
        else
        {
            d_tests_emit("*");
        }

        // a main rule may omit its action
        if (rule->action)
        {
            d_tests_show(rule->action);
        }

        d_tests_emit(" ");
    }

    // trim the separator the loop leaves behind
    if ((g_used > 0) && (g_out[g_used - 1u] == ' '))
    {
        g_out[--g_used] = '\0';
    }

    // compare the rendering against the expectation
    if (strcmp(g_out, _expected) != 0)
    {
        g_failed++;
        printf("  FAIL  %s\n        expected %s\n        got      %s\n",
               _source,
               _expected,
               g_out);
    }

    d_awk_program_free(program);

    return;
}


/*
d_tests_precedence
  brief description
  Tests the following:
  - multiplication binds tighter than addition
  - exponentiation associates to the right and outranks unary minus
  - concatenation binds looser than addition and tighter than comparison
  - the relational operators do not associate
  - assignment associates to the right
*/
static void
d_tests_precedence(void)
{
    printf("precedence\n");

    d_tests_expect("BEGIN { print 1 + 2 * 3 }",
                   "BEGIN{print((+ 1 (* 2 3)))}");
    d_tests_expect("BEGIN { x = 2 ^ 3 ^ 2 }",
                   "BEGIN{(= x (^ 2 (^ 3 2)))}");
    d_tests_expect("BEGIN { x = -2 ^ 2 }",
                   "BEGIN{(= x (- (^ 2 2)))}");
    d_tests_expect("BEGIN { print 1 2, 3 }",
                   "BEGIN{print((cat 1 2),3)}");
    d_tests_expect("BEGIN { print 1 + 2 3 }",
                   "BEGIN{print((cat (+ 1 2) 3))}");
    d_tests_expect("BEGIN { x = a < b c }",
                   "BEGIN{(= x (< a (cat b c)))}");
    d_tests_expect("BEGIN { x = y = 1 }",
                   "BEGIN{(= x (= y 1))}");
    d_tests_expect("BEGIN { x = 1 ? 2 : 3 ? 4 : 5 }",
                   "BEGIN{(= x (?: 1 2 (?: 3 4 5)))}");

    // a < b < c is not a comparison against a truth value
    d_tests_expect("BEGIN { x = 1 < 2 < 3 }", NULL);

    return;
}


/*
d_tests_patterns
  brief description
  Tests the following:
  - a rule may carry a pattern, an action, or both
  - a comma between patterns makes a range
  - a bare regular expression is a pattern in its own right
*/
static void
d_tests_patterns(void)
{
    printf("patterns\n");

    d_tests_expect("{ print }", "*{print()}");
    d_tests_expect("/x/", "/x/");
    d_tests_expect("NR==1, NR==3 { print }",
                   "(== NR 1),(== NR 3){print()}");
    d_tests_expect("$1 ~ /a/ && $2 !~ /b/ { n++ }",
                   "(&& (~ $(1) /a/) (!~ $(2) /b/)){(post++ n)}");
    d_tests_expect("BEGIN { x = 1 } { y = 2 } END { z = 3 }",
                   "BEGIN{(= x 1)} *{(= y 2)} END{(= z 3)}");

    // BEGIN and END are not patterns and cannot stand alone
    d_tests_expect("BEGIN", NULL);

    return;
}


/*
d_tests_statements
  brief description
  Tests the following:
  - the control structures, including both forms of for
  - a redirection on print is captured rather than dropped
  - delete of one element and of a whole array
  - functions, whose extra parameters are awk's locals
*/
static void
d_tests_statements(void)
{
    printf("statements\n");

    d_tests_expect("{ if ($1 > 2) print \"b\"; else print \"s\" }",
                   "*{if((> $(1) 2))print(\"b\")elseprint(\"s\")}");
    d_tests_expect("{ while (i<3) i++ }",
                   "*{while((< i 3))(post++ i)}");
    d_tests_expect("{ do i++; while (i<3) }",
                   "*{do(post++ i)while((< i 3))}");
    d_tests_expect("{ for (i=0; i<3; i++) s+=i }",
                   "*{for((= i 0);(< i 3);(post++ i))(+= s i)}");
    d_tests_expect("{ for (k in a) print k }",
                   "*{forin(k in a)print(k)}");
    d_tests_expect("{ print $1 > \"f\" }",
                   "*{print($(1))>\"f\"}");
    d_tests_expect("{ delete a[1]; delete a }",
                   "*{del a[1]; del a}");
    d_tests_expect("{ a[$1,$2]++ }",
                   "*{(post++ a[$(1),$(2)])}");
    d_tests_expect("{ print (1,2) in a }",
                   "*{print((in [1,2] a))}");

    // a parenthesised list with no `in` after it is print's argument list
    d_tests_expect("{ print (1,2) }",       "*{print(1,2)}");
    d_tests_expect("{ print ($1,$2) > \"f\" }",
                   "*{print($(1),$(2))>\"f\"}");
    d_tests_expect("{ printf (\"%s-%s\", $1, $2) }",
                   "*{printf(\"%s-%s\",$(1),$(2))}");
    d_tests_expect("function f(x, y,  t) { return x+y }\n{ print f(1,2) }",
                   "fn f/3{return((+ x y))} *{print(f(1,2))}");
    d_tests_expect("{ print substr($0,1,3), length }",
                   "*{print(@substr($(0),1,3),@length())}");

    return;
}


/*
d_tests_getline
  brief description
  Tests the following:
  - all six forms build a tree, so the grammar can be run over real programs
  - `<` after getline opens a source file rather than comparing
  - `|` is this operator only when getline follows, leaving print's pipe alone
  - the pipe binds tighter than a comparison, as a reference awk runs it
  - a target that parses but cannot be assigned to is rejected
  - a token that cannot be a target concatenates rather than failing
*/
static void
d_tests_getline(void)
{
    printf("getline\n");

    d_tests_expect("{ r = getline }",
                   "*{(= r getline)}");
    d_tests_expect("{ r = getline x }",
                   "*{(= r getline x)}");
    d_tests_expect("{ r = (getline < \"f\") }",
                   "*{(= r getline <\"f\")}");
    d_tests_expect("{ r = (getline x < \"f\") }",
                   "*{(= r getline x <\"f\")}");
    d_tests_expect("{ r = (\"c\" | getline) }",
                   "*{(= r getline |\"c\")}");
    d_tests_expect("{ r = (\"c\" | getline x) }",
                   "*{(= r getline x |\"c\")}");

    // the pipe binds tighter than the comparison beside it
    d_tests_expect("{ while (\"c\" | getline line > 0) n++ }",
                   "*{while((> getline line |\"c\" 0))(post++ n)}");

    // a bare pipe after print is still redirection, not this operator
    d_tests_expect("{ print $1 | \"sort\" }",
                   "*{print($(1))|\"sort\"}");

    // parentheses restore `>` as a comparison inside a print argument list
    d_tests_expect("{ print $1 > \"f\" }",  "*{print($(1))>\"f\"}");
    d_tests_expect("{ print ($1 > 2) }",    "*{print((> $(1) 2))}");
    d_tests_expect("{ print length(a) > 0 }", "*{print(@length(a))>0}");
    d_tests_expect("{ print (length(a) > 0) }", "*{print((> @length(a) 0))}");

    // a token that cannot be a target is not an error: it concatenates,
    // which is what a reference awk does with `getline 3`
    d_tests_expect("{ x = getline 3 }", "*{(= x (cat getline 3))}");

    // a target that parses but cannot be assigned to is an error
    d_tests_expect("{ getline a++ }", NULL);

    return;
}


/*
d_tests_lexical
  brief description
  Tests the following:
  - a slash divides after an operand and opens a regexp otherwise
  - a newline terminates a rule but not a continued expression
  - comments and escaped newlines are invisible to the parser
  - getline is rejected with a diagnostic rather than silently mis-parsed
*/
static void
d_tests_lexical(void)
{
    printf("lexical rules\n");

    d_tests_expect("BEGIN { x = a / b / c }",
                   "BEGIN{(= x (/ (/ a b) c))}");
    d_tests_expect("BEGIN { x = 1; y /= 2 }",
                   "BEGIN{(= x 1); (/= y 2)}");
    d_tests_expect("$0 ~ /a\\/b/ { print }",
                   "(~ $(0) /a\\/b/){print()}");
    d_tests_expect("BEGIN { x = 1 &&\n 2 }",
                   "BEGIN{(= x (&& 1 2))}");
    d_tests_expect("BEGIN { x = 1 # note\n y = 2 }",
                   "BEGIN{(= x 1); (= y 2)}");
    d_tests_expect("BEGIN { x = 1 + \\\n 2 }",
                   "BEGIN{(= x (+ 1 2))}");
    d_tests_expect("{ n = split($0, a, /,/) }",
                   "*{(= n @split($(0),a,/,/))}");

    // POSIX reserves `function` and not `func`, so this program is legal
    d_tests_expect("BEGIN { func = 5 }", "BEGIN{(= func 5)}");

    // a malformed pattern is a program error, not a runtime one
    d_tests_expect("$0 ~ /[a/ { print }", NULL);
    d_tests_expect("{ 1 +* 2 }", NULL);
    d_tests_expect("{ print \"unterminated }", NULL);

    return;
}


/*
main
  Runs every test group and reports the aggregate result.

Parameter(s):
  none.
Return:
  Zero when every check passed, and one otherwise.
*/
int
main(void)
{
    d_tests_precedence();
    d_tests_patterns();
    d_tests_statements();
    d_tests_getline();
    d_tests_lexical();

    printf("\n%d checks, %d failed\n", g_checks, g_failed);

    return (g_failed == 0) ? 0 : 1;
}
