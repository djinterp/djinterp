/* ===========================================================================
 * math.y - Bison grammar for the math grammar (math.abnf, base subset).
 * Build with:  flex math.l && bison -d math.y && cc lex.yy.c math.tab.c -o math
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

/* ---- terminals ---------------------------------------------------------- */
%token <ival> INTEGER
%token <fval> NUMBER
%token <sval> IDENT

%token EQ NEQ LEQ GEQ ANDAND OROR
%token ABS_OPEN ABS_CLOSE

/* constants */
%token PI EULER INF PHI TAU

/* trig + inverse + hyperbolic */
%token SIN COS TAN SEC CSC COT
%token ASIN ACOS ATAN ATAN2 ASEC ACSC ACOT
%token SINH COSH TANH SECH CSCH COTH
%token ASINH ACOSH ATANH ASECH ACSCH ACOTH

/* logs / specials */
%token LOG LN LOG10 LOG2 EXP
%token SQRT CBRT ABS FLOOR CEIL ROUND MIN MAX GCD LCM
%token FACTORIAL GAMMA ERF ERFC SIGN

/* coordinate / complex helpers */
%token POLAR RECT THETA CYL RHO SPHER
%token RE_FN IM_FN ARG_FN CONJ_FN MAG_FN

/* ---- precedence (lowest first) ----------------------------------------- */
%right '='
%left  OROR
%left  ANDAND
%nonassoc EQ NEQ
%nonassoc '<' '>' LEQ GEQ
%left  '+' '-'
%left  '*' '/' '%'
%right '^'
%right UMINUS UPLUS UNOT
%left  POSTFIX

%start math_expression

/* Two benign shift/reduce conflicts on '(' after IDENT — `f(x)` should be
 * parsed as a user-function call (shift), not as `f` juxtaposed with `(x)`
 * (reduce). Default-shift gives the correct interpretation. */
%expect 2

%%

/* ============================================================================
 * Top-level unifying rule. In the base grammar this is just `expression`;
 * the calculus grammar (calculus.y) widens this with extra alternatives.
 * ==========================================================================*/
math_expression
    : expression                            { printf("ok: parsed expression\n"); }
    ;

expression
    : assignment
    | conditional
    ;

assignment
    : IDENT '=' expression
    ;

conditional
    : logical_or
    ;

/* ---- boolean / comparison layers --------------------------------------- */
logical_or
    : logical_and
    | logical_or OROR logical_and
    ;

logical_and
    : comparison
    | logical_and ANDAND comparison
    ;

comparison
    : additive
    | comparison comp_op additive
    ;

comp_op
    : EQ | NEQ | LEQ | GEQ | '<' | '>'
    ;

/* ---- arithmetic by precedence ------------------------------------------ */
additive
    : multiplicative
    | additive '+' multiplicative
    | additive '-' multiplicative
    ;

multiplicative
    : exponential
    | multiplicative '*' exponential
    | multiplicative '/' exponential
    | multiplicative '%' exponential
    ;

exponential
    : implicit_mult
    | implicit_mult '^' exponential          /* right-assoc: 2^3^4 = 2^(3^4) */
    ;

/* implicit-mult: juxtaposition. The first factor is a full `unary` so things
 * like `-2x` work, but each subsequent juxtaposed factor is a `postfix` only.
 * Restricting the right-hand side keeps `2-x` parsing as `2 - x` (subtraction)
 * rather than `2 * (-x)`, and similarly `2!3` parses as `(2!) * 3`. */
implicit_mult
    : unary
    | implicit_mult juxt_factor
    ;

juxt_factor
    : postfix
    ;

/* ---- unary / postfix --------------------------------------------------- */
unary
    : postfix
    | '+' postfix       %prec UPLUS
    | '-' postfix       %prec UMINUS
    | '!' postfix       %prec UNOT
    ;

postfix
    : primary
    | primary '!'       %prec POSTFIX        /* factorial */
    ;

/* ---- primary expressions ----------------------------------------------
 * Note: `IDENT` (variable, subscripted variable, or user-function call) is
 * lifted directly into `primary` so the single-token lookahead — '_' vs '('
 * vs anything else — disambiguates without an LALR(1) reduce/reduce. */
primary
    : number
    | constant
    | known_function_call
    | IDENT                                  /* variable: x, foo, ... */
    | IDENT '_' INTEGER                      /* subscripted: x_1     */
    | IDENT '_' IDENT                        /* subscripted: x_n     */
    | IDENT '(' argument_list ')'            /* user function: f(...) */
    | '(' expression ')'
    | '(' argument_list_2plus ')'            /* tuple / point */
    | vector_literal
    | ABS_OPEN expression ABS_CLOSE          /* |x|  absolute value */
    ;

/* tuple form: at least 2 comma-separated expressions inside parens */
argument_list_2plus
    : expression ',' expression
    | argument_list_2plus ',' expression
    ;

vector_literal
    : '[' argument_list ']'
    ;

/* ---- numbers ----------------------------------------------------------- */
number
    : INTEGER
    | NUMBER
    ;

/* ---- constants --------------------------------------------------------- */
constant
    : PI | TAU | PHI | EULER | INF
    ;

/* ---- built-in function calls -------------------------------------------
 * User-defined function calls (`f(x)`) live in `primary` to avoid an
 * R/R conflict between IDENT-as-variable and IDENT-as-function-name. */
known_function_call
    : known_function_name '(' argument_list ')'
    ;

argument_list
    : expression
    | argument_list ',' expression
    ;

known_function_name
    : trig_function
    | inverse_trig_function
    | hyperbolic_function
    | inverse_hyperbolic_function
    | logarithmic_function
    | special_function
    | coordinate_function
    | complex_function
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
coordinate_function
    : POLAR | RECT | THETA | CYL | RHO | SPHER
    ;
complex_function
    : RE_FN | IM_FN | ARG_FN | CONJ_FN | MAG_FN
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "math.y: %s near '%s' on line %d\n", s, yytext, yylineno);
}

#ifndef MATH_NO_MAIN
int main(void) {
    return yyparse();
}
#endif
