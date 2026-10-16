/******************************************************************************
* djinterp [parser]                                         filter_grammar.y
*
* Bison/Yacc grammar for filter expression DSL.
*   Defines LALR(1) parser specification with semantic actions that build
* expression trees compatible with filter_expr.hpp.
*
* COMPILATION:
*   bison -d filter_grammar.y -o filter_grammar.tab.cpp
*
* USAGE WITH FLEX:
*   flex -o filter_lexer.cpp filter_lexer.l
*   bison -d filter_grammar.y -o filter_grammar.tab.cpp
*   g++ -std=c++14 filter_grammar.tab.cpp filter_lexer.cpp -o filter_parser
*
* path:      \src\parser\filter_grammar.y
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.15
******************************************************************************/

/* ========================================================================= */
/* PROLOGUE - C/C++ DECLARATIONS                                             */
/* ========================================================================= */

%{
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include "filter_expr.hpp"

// forward declarations
void yyerror(const char* s);
int yylex(void);

// global result (set by parser)
template<typename T>
std::unique_ptr<djinterp::filter_expr<T>>* g_parse_result = nullptr;

// type alias for readability
template<typename T>
using expr_ptr = std::unique_ptr<djinterp::filter_expr<T>>;

%}


/* ========================================================================= */
/* BISON DECLARATIONS                                                        */
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/* Union for semantic values                                                 */
/* ------------------------------------------------------------------------- */

%union {
    char*                                  str_val;
    djinterp::filter_expr<void>*           expr_val;
    std::vector<std::string>*              str_list_val;
}


/* ------------------------------------------------------------------------- */
/* Token declarations                                                        */
/* ------------------------------------------------------------------------- */

/* Literals and identifiers */
%token <str_val> IDENTIFIER
%token <str_val> STRING_LITERAL
%token <str_val> NUMBER_LITERAL

/* Comparison operators */
%token EQ              /* == */
%token NE              /* != */
%token LT              /* <  */
%token LE              /* <= */
%token GT              /* >  */
%token GE              /* >= */
%token MATCH           /* ~ or ~= */
%token COLON           /* :  */

/* Keywords */
%token AND
%token OR
%token NOT
%token XOR             /* optional */
%token IN
%token HAS

/* Punctuation */
%token LPAREN          /* ( */
%token RPAREN          /* ) */
%token LBRACE          /* { */
%token RBRACE          /* } */
%token COMMA           /* , */


/* ------------------------------------------------------------------------- */
/* Non-terminal types                                                        */
/* ------------------------------------------------------------------------- */

%type <expr_val>     filter_expr
%type <expr_val>     or_expr
%type <expr_val>     and_expr
%type <expr_val>     unary_expr
%type <expr_val>     primary_expr
%type <expr_val>     predicate
%type <str_val>      value
%type <str_list_val> value_list
%type <str_list_val> set_literal


/* ------------------------------------------------------------------------- */
/* Operator precedence and associativity                                     */
/* ------------------------------------------------------------------------- */

%left OR               /* lowest precedence, left-associative */
%left XOR
%left AND
%right NOT             /* highest precedence, right-associative */


/* ------------------------------------------------------------------------- */
/* Start symbol                                                              */
/* ------------------------------------------------------------------------- */

%start filter_expr


/* ========================================================================= */
/* GRAMMAR RULES                                                             */
/* ========================================================================= */

%%

/* ------------------------------------------------------------------------- */
/* Top-level filter expression                                               */
/* ------------------------------------------------------------------------- */

filter_expr
    : or_expr
        {
            $$ = $1;
            
            // store result in global variable
            if (g_parse_result) {
                *g_parse_result = expr_ptr<void>(static_cast<djinterp::filter_expr<void>*>($1));
            }
        }
    ;


/* ------------------------------------------------------------------------- */
/* Logical OR expression (lowest precedence)                                 */
/* ------------------------------------------------------------------------- */

or_expr
    : and_expr
        {
            $$ = $1;
        }
    | or_expr OR and_expr
        {
            // create OR expression
            auto left = expr_ptr<void>(static_cast<djinterp::filter_expr<void>*>($1));
            auto right = expr_ptr<void>(static_cast<djinterp::filter_expr<void>*>($3));
            
            $$ = djinterp::make_or(std::move(left), std::move(right)).release();
        }
    ;


/* ------------------------------------------------------------------------- */
/* Logical AND expression (medium precedence)                                */
/* ------------------------------------------------------------------------- */

and_expr
    : unary_expr
        {
            $$ = $1;
        }
    | and_expr AND unary_expr
        {
            // create AND expression
            auto left = expr_ptr<void>(static_cast<djinterp::filter_expr<void>*>($1));
            auto right = expr_ptr<void>(static_cast<djinterp::filter_expr<void>*>($3));
            
            $$ = djinterp::make_and(std::move(left), std::move(right)).release();
        }
    ;


/* ------------------------------------------------------------------------- */
/* Unary NOT expression (highest precedence)                                 */
/* ------------------------------------------------------------------------- */

unary_expr
    : primary_expr
        {
            $$ = $1;
        }
    | NOT primary_expr
        {
            // create NOT expression
            auto operand = expr_ptr<void>(static_cast<djinterp::filter_expr<void>*>($2));
            
            $$ = djinterp::make_not(std::move(operand)).release();
        }
    ;


/* ------------------------------------------------------------------------- */
/* Primary expression (atoms and grouping)                                   */
/* ------------------------------------------------------------------------- */

primary_expr
    : predicate
        {
            $$ = $1;
        }
    | LPAREN filter_expr RPAREN
        {
            $$ = $2;
        }
    ;


/* ------------------------------------------------------------------------- */
/* Atomic predicate                                                          */
/* ------------------------------------------------------------------------- */

predicate
    : IDENTIFIER EQ value
        {
            $$ = djinterp::make_predicate<void>(
                std::string($1),
                djinterp::eq_predicate(std::string($3))
            ).release();
            
            free($1);
            free($3);
        }
    | IDENTIFIER NE value
        {
            $$ = djinterp::make_predicate<void>(
                std::string($1),
                djinterp::ne_predicate(std::string($3))
            ).release();
            
            free($1);
            free($3);
        }
    | IDENTIFIER LT value
        {
            $$ = djinterp::make_predicate<void>(
                std::string($1),
                djinterp::lt_predicate(std::string($3))
            ).release();
            
            free($1);
            free($3);
        }
    | IDENTIFIER LE value
        {
            $$ = djinterp::make_predicate<void>(
                std::string($1),
                djinterp::le_predicate(std::string($3))
            ).release();
            
            free($1);
            free($3);
        }
    | IDENTIFIER GT value
        {
            $$ = djinterp::make_predicate<void>(
                std::string($1),
                djinterp::gt_predicate(std::string($3))
            ).release();
            
            free($1);
            free($3);
        }
    | IDENTIFIER GE value
        {
            $$ = djinterp::make_predicate<void>(
                std::string($1),
                djinterp::ge_predicate(std::string($3))
            ).release();
            
            free($1);
            free($3);
        }
    | IDENTIFIER MATCH value
        {
            $$ = djinterp::make_predicate<void>(
                std::string($1),
                djinterp::matches_predicate(std::string($3))
            ).release();
            
            free($1);
            free($3);
        }
    | IDENTIFIER COLON value
        {
            // colon is shorthand for ==
            $$ = djinterp::make_predicate<void>(
                std::string($1),
                djinterp::eq_predicate(std::string($3))
            ).release();
            
            free($1);
            free($3);
        }
    | IDENTIFIER IN set_literal
        {
            $$ = djinterp::make_predicate<void>(
                std::string($1),
                djinterp::in_set_predicate(*$3)
            ).release();
            
            free($1);
            delete $3;
        }
    ;


/* ------------------------------------------------------------------------- */
/* Value (string, number, or set literal)                                    */
/* ------------------------------------------------------------------------- */

value
    : STRING_LITERAL
        {
            $$ = $1;
        }
    | NUMBER_LITERAL
        {
            $$ = $1;
        }
    ;


/* ------------------------------------------------------------------------- */
/* Set literal                                                               */
/* ------------------------------------------------------------------------- */

set_literal
    : LBRACE value_list RBRACE
        {
            $$ = $2;
        }
    | LBRACE RBRACE
        {
            $$ = new std::vector<std::string>();
        }
    ;


/* ------------------------------------------------------------------------- */
/* Value list (comma-separated values)                                       */
/* ------------------------------------------------------------------------- */

value_list
    : value
        {
            $$ = new std::vector<std::string>();
            $$->push_back(std::string($1));
            free($1);
        }
    | value_list COMMA value
        {
            $$ = $1;
            $$->push_back(std::string($3));
            free($3);
        }
    ;


%%


/* ========================================================================= */
/* EPILOGUE - C/C++ CODE                                                     */
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/* Error handling                                                            */
/* ------------------------------------------------------------------------- */

void yyerror(const char* s)
{
    fprintf(stderr, "Parse error: %s\n", s);
}


/* ------------------------------------------------------------------------- */
/* Parse function (to be called from C++ code)                               */
/* ------------------------------------------------------------------------- */

template<typename T>
std::unique_ptr<djinterp::filter_expr<T>>
parse_filter_expression(const char* input)
{
    std::unique_ptr<djinterp::filter_expr<T>> result;
    
    // set up global result pointer
    g_parse_result = reinterpret_cast<expr_ptr<void>**>(&result);
    
    // parse input (would need to set up input buffer for flex/bison)
    // yy_scan_string(input);
    // yyparse();
    
    // clear global
    g_parse_result = nullptr;
    
    return result;
}


/* ========================================================================= */
/* EXAMPLE USAGE                                                             */
/* ========================================================================= */

/*

// Example C++ code using the parser:

#include "filter_grammar.tab.hpp"
#include "filter_expr.hpp"

int main() {
    const char* input = "status == \"active\" AND priority > 5";
    
    auto filter = parse_filter_expression<MyElement>(input);
    
    if (filter) {
        // use filter
        std::vector<MyElement> elements = get_elements();
        auto filtered = djinterp::apply_filter(elements, *filter);
    }
    
    return 0;
}

*/


/* ========================================================================= */
/* NOTES                                                                     */
/* ========================================================================= */

/*

COMPILATION NOTES:
------------------
1. Bison generates .tab.cpp and .tab.hpp files
2. Flex generates lexer code
3. Link together with filter_expr.hpp implementation

OPERATOR PRECEDENCE:
--------------------
Lowest:  OR
         XOR (optional)
         AND
Highest: NOT

All binary operators are left-associative.
NOT is right-associative.

SEMANTIC ACTIONS:
-----------------
Each rule builds the corresponding expression tree node.
Memory management uses unique_ptr for safety.
Raw pointers released from unique_ptr during parsing.

LIMITATIONS:
------------
- Generic void type used; needs templating for specific element types
- Error recovery is basic (can be enhanced)
- No location tracking (can add with %locations)

ENHANCEMENTS:
-------------
To improve this grammar, consider:
1. Add %locations for position tracking
2. Implement better error recovery
3. Add semantic error checking (type compatibility)
4. Support for XOR operator
5. Function call syntax
6. Aggregate operators

*/
