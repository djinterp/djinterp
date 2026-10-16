/* ===========================================================================
 * calculus.y - Bison grammar for the unified math + calculus + linear-algebra
 *              + logic + set-theory + discrete grammar.
 *
 * ARCHITECTURE NOTE
 * -----------------
 * The calculus.abnf source defines six parallel top-level non-terminals
 * (expression, calculus-expression, linear-algebra-expression,
 *  logical-expression, set-expression, discrete-expression). Implemented
 * as parallel non-terminals in LALR(1) they generate hundreds of R/R
 * conflicts because they share starting tokens (IDENT, '[', '{', etc.).
 *
 * This grammar collapses them into ONE unified `expression` grammar where
 * boolean operators, set operators, and set-membership relations live at
 * their natural precedence levels. Only constructs with truly distinct
 * syntax — quantifiers (∀ x, P), calculus operators (∫, ∂, lim, ∑, ∏, ∇),
 * and recurrence relations — sit alongside `expression` as top-level forms.
 *
 * Build with:
 *   flex calculus.l && bison -d calculus.y && cc lex.yy.c calculus.tab.c -o calc
 * =========================================================================*/

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int   yylex(void);
extern int   yylineno;
extern char *yytext;
void yyerror(const char *s);
%}

%union {
    long    ival;
    double  fval;
    char   *sval;
}

/* ===========================================================================
 * TERMINALS
 * =========================================================================*/

%token <ival> INTEGER
%token <fval> NUMBER
%token <sval> IDENT
%token <sval> DIFFERENTIAL    /* "dx", "dt", etc. — d immediately followed by var */

/* ASCII multi-char ops */
%token EQ NEQ LEQ GEQ ANDAND OROR
%token IFF IMPLIES ARROW
%token CARET_INV CARET_T CARET_C
%token PRIME BACKSLASH HASH

/* delimiters with state */
%token ABS_OPEN ABS_CLOSE
%token NORM_OPEN NORM_CLOSE

/* base-math constants */
%token PI EULER INF PHI TAU

/* base-math functions */
%token SIN COS TAN SEC CSC COT
%token ASIN ACOS ATAN ATAN2 ASEC ACSC ACOT
%token SINH COSH TANH SECH CSCH COTH
%token ASINH ACOSH ATANH ASECH ACSCH ACOTH
%token LOG LN LOG10 LOG2 EXP
%token SQRT CBRT ABS FLOOR CEIL ROUND MIN MAX GCD LCM
%token FACTORIAL GAMMA ERF ERFC SIGN
%token POLAR RECT THETA CYL RHO SPHER
%token RE_FN IM_FN ARG_FN CONJ_FN MAG_FN

/* calculus */
%token INTEGRAL INTEGRAL2 INTEGRAL3 PARTIAL SUMMATION PRODUCT NABLA
%token DELTA_CAP DELTA_LO
%token DIFF_FN PARTIAL_KW GRAD_KW INTEGRATE_FN
%token LIM LIMIT_FN SUM_FN PRODUCT_FN
%token DIV_KW CURL_KW
%token D_KW D_CAP

/* linear algebra */
%token TRANSPOSE_FN INV_FN DET_FN TRACE_FN TR_FN RANK_FN EIG_FN EIGVEC_FN
%token DOT_FN CROSS_FN NORM_FN NORMALIZE_FN
%token ZEROS_FN ONES_FN EYE_FN DIAG_FN RESHAPE_FN
%token SOLVE_FN LU_FN QR_FN SVD_FN CHOLESKY_FN

/* logic */
%token FORALL EXISTS OR_SYM AND_SYM NOT_SYM IFF_SYM TOP_SYM BOT_SYM
%token FORALL_KW EXISTS_KW AND_KW OR_KW NOT_KW IMPLIES_KW IFF_KW
%token TRUE_KW FALSE_KW

/* set theory */
%token IN_SYM NOTIN_SYM UNION_SYM INTER_SYM
%token SUBSETEQ SUPSETEQ SUBSET SUPSET
%token TIMES_SYM CDOT_SYM EMPTY_SYM POWERSET EQUIV_SYM
%token NAT_SET INT_SET RAT_SET REAL_SET CPX_SET
%token UNION_KW INTER_KW DIFF_KW COMP_KW CART_KW PWRSET_KW
%token SUBSET_KW SUPSET_KW PSUBSET_KW EMPTY_KW IN_KW
%token CARD_KW SUP_KW

/* floor/ceil unicode */
%token LFLOOR RFLOOR LCEIL RCEIL

/* discrete */
%token CHOOSE_KW BINOM_KW PERM_KW COMB_KW MOD_KW
%token TOTIENT_KW MOBIUS_KW ISPRIME_KW NEXTPRIME_KW PRIMES_KW FACTORS_KW
%token SEQUENCE_KW
%token DEGREE_KW NEIGHBORS_KW ADJ_KW CONN_KW CYCLE_KW PATH_KW GRAPH_KW

/* ===========================================================================
 * PRECEDENCE  (lowest first)
 * =========================================================================*/

%right '='
%right IFF IFF_SYM IFF_KW
%right IMPLIES IMPLIES_KW ARROW
%left  OROR OR_SYM OR_KW
%left  ANDAND AND_SYM AND_KW
%right NOT_SYM NOT_KW
%nonassoc EQ NEQ EQUIV_SYM
%nonassoc '<' '>' LEQ GEQ
%nonassoc IN_SYM NOTIN_SYM SUBSETEQ SUPSETEQ SUBSET SUPSET
%left  UNION_SYM INTER_SYM
%left  '+' '-' BACKSLASH
%left  MOD_KW
%left  '*' '/' '%' CDOT_SYM TIMES_SYM
%right '^'
%right UMINUS UPLUS UNOT
%left  PRIME POSTFIX_BANG CARET_INV CARET_T CARET_C
%left  '.'

%start math_expression

/* The remaining benign conflicts are the f(x) shift-vs-juxtaposition pattern
 * (carried over from math.y, multiplied by the larger primary set) plus the
 * matrix-row/vector ambiguity at '['. All resolve correctly via default-shift. */
%expect 222

%%

/* ============================================================================
 * TOP-LEVEL UNIFIED RULE
 * ==========================================================================*/

math_expression
    : top_form                       { printf("ok\n"); }
    ;

top_form
    : expression
    | quantified_expression
    | calculus_form
    | recurrence_relation
    ;

/* ============================================================================
 * EXPRESSION GRAMMAR
 * Layered low→high: =, ↔, →, ∨, ∧, ¬, comparison/membership, ∪/∩,
 *                   +/-/\, mod, *,/,%,·,×, ^, unary, postfix, primary.
 * ==========================================================================*/

expression
    : assignment
    | conditional
    ;

assignment
    : IDENT '=' expression
    ;

conditional
    : iff_form
    ;

iff_form
    : implication
    | iff_form IFF      implication
    | iff_form IFF_SYM  implication
    | iff_form IFF_KW   implication
    ;

implication
    : logical_or
    | implication IMPLIES     logical_or
    | implication ARROW       logical_or
    | implication IMPLIES_KW  logical_or
    ;

logical_or
    : logical_and
    | logical_or OROR    logical_and
    | logical_or OR_SYM  logical_and
    | logical_or OR_KW   logical_and
    ;

logical_and
    : logical_not
    | logical_and ANDAND   logical_not
    | logical_and AND_SYM  logical_not
    | logical_and AND_KW   logical_not
    ;

logical_not
    : comparison
    | NOT_SYM logical_not
    | NOT_KW  logical_not
    ;

comparison
    : set_union
    | comparison comp_op set_union
    ;

comp_op
    : EQ | NEQ | LEQ | GEQ | '<' | '>'
    | IN_SYM | NOTIN_SYM
    | SUBSETEQ | SUPSETEQ | SUBSET | SUPSET
    | EQUIV_SYM
    ;

set_union
    : additive
    | set_union UNION_SYM additive
    | set_union INTER_SYM additive
    ;

additive
    : modular
    | additive '+' modular
    | additive '-' modular
    | additive BACKSLASH modular
    ;

modular
    : multiplicative
    | modular MOD_KW multiplicative
    ;

multiplicative
    : exponential
    | multiplicative '*'        exponential
    | multiplicative '/'        exponential
    | multiplicative '%'        exponential
    | multiplicative CDOT_SYM   exponential
    | multiplicative TIMES_SYM  exponential
    ;

exponential
    : implicit_mult
    | implicit_mult '^' exponential
    ;

implicit_mult
    : unary
    | implicit_mult juxt_factor
    ;

juxt_factor
    : postfix
    ;

unary
    : postfix
    | '+' postfix     %prec UPLUS
    | '-' postfix     %prec UMINUS
    | '!' postfix     %prec UNOT
    ;

postfix
    : primary
    | postfix '!'         %prec POSTFIX_BANG
    | postfix PRIME       %prec PRIME
    | postfix CARET_T
    | postfix CARET_INV
    | postfix CARET_C
    ;

primary
    : number
    | constant
    | known_function_call
    | IDENT
    | IDENT '_' INTEGER
    | IDENT '_' IDENT
    | IDENT '[' expression ']'
    | IDENT '(' argument_list ')'
    | '(' expression ')'
    | '(' argument_list_2plus ')'
    | '[' bracket_body ']'
    | '{' '}'
    | '{' set_elements '}'
    | '{' set_builder_body '}'
    | ABS_OPEN expression ABS_CLOSE
    | NORM_OPEN expression NORM_CLOSE
    | NORM_OPEN expression NORM_CLOSE '_' INTEGER
    | LFLOOR expression RFLOOR
    | LCEIL  expression RCEIL
    | HASH primary
    | POWERSET '(' expression ')'
    | indefinite_integral
    | definite_integral
    | summation_form
    | product_form
    | limit_form
    | derivative_form
    | nabla_form
    | differential_form
    | INTEGER PERM_KW INTEGER
    | INTEGER COMB_KW INTEGER
    | '(' combinatoric_atom CHOOSE_KW combinatoric_atom ')'
    ;

combinatoric_atom
    : INTEGER
    | IDENT
    ;

bracket_body
    : argument_list
    | matrix_rows_2plus
    ;

matrix_rows_2plus
    : argument_list ';' argument_list
    | matrix_rows_2plus ';' argument_list
    ;

argument_list_2plus
    : expression ',' expression
    | argument_list_2plus ',' expression
    ;

set_elements
    : expression
    | set_elements ',' expression
    ;

set_builder_body
    : IDENT IN_SYM expression ':' expression
    | IDENT IN_SYM expression '|' expression
    | IDENT IN_KW  expression ':' expression
    | IDENT IN_KW  expression '|' expression
    ;

argument_list
    : expression
    | argument_list ',' expression
    ;

number    : INTEGER | NUMBER ;

constant
    : PI | TAU | PHI | EULER | INF
    | TRUE_KW | FALSE_KW | TOP_SYM | BOT_SYM
    | NAT_SET | INT_SET | RAT_SET | REAL_SET | CPX_SET
    | EMPTY_SYM | EMPTY_KW
    ;

known_function_call
    : known_function_name '(' argument_list ')'
    ;

known_function_name
    : trig_function | inverse_trig_function
    | hyperbolic_function | inverse_hyperbolic_function
    | logarithmic_function | special_function
    | coordinate_function | complex_function
    | linalg_function_name | calculus_function_name
    | set_function_name | discrete_function_name
    ;

trig_function              : SIN | COS | TAN | SEC | CSC | COT ;
inverse_trig_function      : ASIN | ACOS | ATAN | ATAN2 | ASEC | ACSC | ACOT ;
hyperbolic_function        : SINH | COSH | TANH | SECH | CSCH | COTH ;
inverse_hyperbolic_function: ASINH | ACOSH | ATANH | ASECH | ACSCH | ACOTH ;
logarithmic_function       : LOG | LN | LOG10 | LOG2 | EXP ;
special_function
    : SQRT | CBRT | ABS | FLOOR | CEIL | ROUND
    | MIN  | MAX  | GCD | LCM   | FACTORIAL
    | GAMMA | ERF | ERFC | SIGN
    ;
coordinate_function : POLAR | RECT | THETA | CYL | RHO | SPHER ;
complex_function    : RE_FN | IM_FN | ARG_FN | CONJ_FN | MAG_FN ;

linalg_function_name
    : TRANSPOSE_FN | INV_FN | DET_FN | TRACE_FN | TR_FN | RANK_FN
    | EIG_FN | EIGVEC_FN | DOT_FN | CROSS_FN | NORM_FN | NORMALIZE_FN
    | ZEROS_FN | ONES_FN | EYE_FN | DIAG_FN | RESHAPE_FN
    | SOLVE_FN | LU_FN | QR_FN | SVD_FN | CHOLESKY_FN
    ;
calculus_function_name
    : DIFF_FN | INTEGRATE_FN | LIMIT_FN | SUM_FN | PRODUCT_FN
    | PARTIAL_KW | GRAD_KW | DIV_KW | CURL_KW
    ;
set_function_name
    : UNION_KW | INTER_KW | DIFF_KW | COMP_KW | CART_KW | PWRSET_KW
    | SUBSET_KW | SUPSET_KW | PSUBSET_KW | IN_KW | CARD_KW | SUP_KW
    ;
discrete_function_name
    : BINOM_KW | PERM_KW | COMB_KW
    | TOTIENT_KW | MOBIUS_KW | ISPRIME_KW | NEXTPRIME_KW
    | PRIMES_KW | FACTORS_KW | SEQUENCE_KW
    | DEGREE_KW | NEIGHBORS_KW | ADJ_KW | CONN_KW
    | CYCLE_KW | PATH_KW | GRAPH_KW
    ;

/* ============================================================================
 * CALCULUS PRIMITIVES
 * ==========================================================================*/

/* Differentials in integrals must be written without a space between
 * `d` and the variable (`dx`, `dt`, ...) so the lexer captures them as a
 * single DIFFERENTIAL token. The spaced form `d x` is lexically ambiguous
 * with the start of `d/dx` (derivative_form) and is therefore not
 * supported. Users who prefer spaces can wrap the integrand: `∫ (x^2) dx`. */
indefinite_integral
    : INTEGRAL expression DIFFERENTIAL
    ;

definite_integral
    : INTEGRAL '_' primary '^' primary expression DIFFERENTIAL
    ;

summation_form
    : SUMMATION '_' '(' IDENT '=' expression ')' '^' primary expression
    ;

product_form
    : PRODUCT '_' '(' IDENT '=' expression ')' '^' primary expression
    ;

limit_form
    : LIM '_' '(' IDENT ARROW expression ')' expression
    ;

derivative_form
    : D_KW '/' DIFFERENTIAL                                      /* d/dx        */
    | D_KW '/' D_KW IDENT                                        /* d / d x     */
    | D_KW '^' INTEGER '/' DIFFERENTIAL '^' INTEGER              /* d^2/dx^2    */
    | D_KW '^' INTEGER '/' D_KW IDENT '^' INTEGER                /* d^2/d x^2   */
    | D_CAP '_' IDENT '(' expression ')'
    | D_CAP '(' expression ')'
    | PARTIAL IDENT '/' PARTIAL IDENT                            /* ∂f/∂x       */
    | PARTIAL '/' PARTIAL IDENT                                  /* ∂/∂x        */
    | PARTIAL '^' INTEGER '/' PARTIAL IDENT '^' INTEGER
    | PARTIAL '_' IDENT
    ;

nabla_form
    : NABLA primary                           %prec UNOT
    | NABLA CDOT_SYM primary                  %prec UNOT
    | NABLA TIMES_SYM primary                 %prec UNOT
    ;

differential_form
    : DELTA_CAP IDENT
    | DELTA_LO  IDENT
    ;

/* ============================================================================
 * QUANTIFIERS, MULTIPLE INTEGRALS, RECURRENCE
 * ==========================================================================*/

quantified_expression
    : FORALL    IDENT ',' quantifier_body
    | FORALL    IDENT IN_SYM expression ',' quantifier_body
    | FORALL_KW IDENT ',' quantifier_body
    | FORALL_KW IDENT IN_KW  expression ',' quantifier_body
    | EXISTS    IDENT ',' quantifier_body
    | EXISTS    IDENT IN_SYM expression ',' quantifier_body
    | EXISTS_KW IDENT ',' quantifier_body
    | EXISTS_KW IDENT IN_KW  expression ',' quantifier_body
    ;

quantifier_body
    : expression
    | quantified_expression
    ;

calculus_form
    : INTEGRAL2 expression D_KW IDENT D_KW IDENT
    | INTEGRAL3 expression D_KW IDENT D_KW IDENT D_KW IDENT
    ;

recurrence_relation
    : sequence_term '=' expression
    ;

sequence_term
    : IDENT '_' INTEGER
    | IDENT '_' IDENT
    | IDENT '[' expression ']'
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "calculus.y: %s near '%s' on line %d\n",
            s, yytext, yylineno);
}

int main(void) {
    return yyparse();
}
