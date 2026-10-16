/******************************************************************************
* djinterp [parser]                                   filter_grammar_prefix.y
*
* Bison/Yacc grammar for prefix-style filter expressions.
*   Defines parser for --and, --or, --not, --xor prefix operators.
* Generates expression trees compatible with filter_expr.hpp.
*
* COMPILATION:
*   bison -d filter_grammar_prefix.y -o filter_grammar_prefix.tab.cpp
*
* USAGE:
*   Combined with prefix lexer (whitespace-tokenized):
*   Input: "--and status:active priority>5"
*   Tokens: AND_OP PREDICATE PREDICATE
*   Output: and_expr(predicate(status, active), predicate(priority, 5))
*
* path:      \src\parser\filter_grammar_prefix.y
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

// global result
template<typename T>
std::unique_ptr<djinterp::filter_expr<T>>* g_prefix_parse_result = nullptr;

// type alias
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
    std::vector<djinterp::filter_expr<void>*>* expr_list_val;
}


/* ------------------------------------------------------------------------- */
/* Token declarations                                                        */
/* ------------------------------------------------------------------------- */

/* Prefix operators */
%token AND_OP              /* --and */
%token OR_OP               /* --or */
%token XOR_OP              /* --xor */
%token NOT_OP              /* --not */

/* Predicate token (complete predicate string) */
%token <str_val> PREDICATE

/* Optional grouping */
%token LPAREN              /* ( */
%token RPAREN              /* ) */


/* ------------------------------------------------------------------------- */
/* Non-terminal types                                                        */
/* ------------------------------------------------------------------------- */

%type <expr_val>      filter_expr
%type <expr_val>      logical_expr
%type <expr_val>      and_expr
%type <expr_val>      or_expr
%type <expr_val>      xor_expr
%type <expr_val>      not_expr
%type <expr_val>      predicate
%type <expr_list_val> expr_list


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
    : logical_expr
        {
            $$ = $1;
            
            if (g_prefix_parse_result) {
                *g_prefix_parse_result = expr_ptr<void>(
                    static_cast<djinterp::filter_expr<void>*>($1));
            }
        }
    | predicate
        {
            $$ = $1;
            
            if (g_prefix_parse_result) {
                *g_prefix_parse_result = expr_ptr<void>(
                    static_cast<djinterp::filter_expr<void>*>($1));
            }
        }
    ;


/* ------------------------------------------------------------------------- */
/* Logical expressions (prefix operators)                                    */
/* ------------------------------------------------------------------------- */

logical_expr
    : and_expr
        {
            $$ = $1;
        }
    | or_expr
        {
            $$ = $1;
        }
    | xor_expr
        {
            $$ = $1;
        }
    | not_expr
        {
            $$ = $1;
        }
    ;


/* ------------------------------------------------------------------------- */
/* AND expression (n-ary: 2+ operands)                                       */
/* ------------------------------------------------------------------------- */

and_expr
    : AND_OP expr_list
        {
            // check minimum arity
            if ($2->size() < 2) {
                yyerror("--and requires at least 2 operands");
                YYABORT;
            }
            
            // create n-ary AND expression
            std::vector<expr_ptr<void>> operands;
            
            for (auto* expr : *$2) {
                operands.push_back(expr_ptr<void>(
                    static_cast<djinterp::filter_expr<void>*>(expr)));
            }
            
            $$ = djinterp::make_and_all(std::move(operands)).release();
            
            delete $2;
        }
    ;


/* ------------------------------------------------------------------------- */
/* OR expression (n-ary: 2+ operands)                                        */
/* ------------------------------------------------------------------------- */

or_expr
    : OR_OP expr_list
        {
            // check minimum arity
            if ($2->size() < 2) {
                yyerror("--or requires at least 2 operands");
                YYABORT;
            }
            
            // create n-ary OR expression
            std::vector<expr_ptr<void>> operands;
            
            for (auto* expr : *$2) {
                operands.push_back(expr_ptr<void>(
                    static_cast<djinterp::filter_expr<void>*>(expr)));
            }
            
            $$ = djinterp::make_or_all(std::move(operands)).release();
            
            delete $2;
        }
    ;


/* ------------------------------------------------------------------------- */
/* XOR expression (binary: exactly 2 operands)                               */
/* ------------------------------------------------------------------------- */

xor_expr
    : XOR_OP filter_expr filter_expr
        {
            auto left = expr_ptr<void>(
                static_cast<djinterp::filter_expr<void>*>($2));
            auto right = expr_ptr<void>(
                static_cast<djinterp::filter_expr<void>*>($3));
            
            $$ = djinterp::make_xor(std::move(left), 
                                     std::move(right)).release();
        }
    ;


/* ------------------------------------------------------------------------- */
/* NOT expression (unary: exactly 1 operand)                                 */
/* ------------------------------------------------------------------------- */

not_expr
    : NOT_OP filter_expr
        {
            auto operand = expr_ptr<void>(
                static_cast<djinterp::filter_expr<void>*>($2));
            
            $$ = djinterp::make_not(std::move(operand)).release();
        }
    ;


/* ------------------------------------------------------------------------- */
/* Expression list (for n-ary operators)                                     */
/* ------------------------------------------------------------------------- */

expr_list
    : filter_expr filter_expr
        {
            $$ = new std::vector<djinterp::filter_expr<void>*>();
            $$->push_back($1);
            $$->push_back($2);
        }
    | expr_list filter_expr
        {
            $$ = $1;
            $$->push_back($2);
        }
    ;


/* ------------------------------------------------------------------------- */
/* Predicate (atomic expression)                                             */
/* ------------------------------------------------------------------------- */

predicate
    : PREDICATE
        {
            // parse predicate string into components
            std::string pred_str($1);
            std::string attr, op, value;
            
            if (parse_predicate_string(pred_str, attr, op, value)) {
                $$ = create_predicate(attr, op, value);
            } else {
                yyerror("Invalid predicate format");
                YYABORT;
            }
            
            free($1);
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
/* Helper: Parse predicate string                                            */
/* ------------------------------------------------------------------------- */

bool parse_predicate_string(const std::string& pred,
                            std::string&       attr,
                            std::string&       op,
                            std::string&       value)
{
    size_t pos;
    
    // try two-character operators first
    const char* two_char_ops[] = {"==", "!=", "<=", ">=", "~=", nullptr};
    
    for (int i = 0; two_char_ops[i]; ++i) {
        pos = pred.find(two_char_ops[i]);
        
        if (pos != std::string::npos) {
            attr  = pred.substr(0, pos);
            op    = two_char_ops[i];
            value = pred.substr(pos + 2);
            return true;
        }
    }
    
    // try single-character operators
    const char single_char_ops[] = {':', '<', '>', '~', '\0'};
    
    for (int i = 0; single_char_ops[i]; ++i) {
        pos = pred.find(single_char_ops[i]);
        
        if (pos != std::string::npos) {
            attr  = pred.substr(0, pos);
            op    = std::string(1, single_char_ops[i]);
            value = pred.substr(pos + 1);
            return true;
        }
    }
    
    // try 'in' operator (space-delimited)
    pos = pred.find(" in ");
    
    if (pos != std::string::npos) {
        attr  = pred.substr(0, pos);
        op    = "in";
        value = pred.substr(pos + 4);
        return true;
    }
    
    return false;
}


/* ------------------------------------------------------------------------- */
/* Helper: Create predicate from parts                                       */
/* ------------------------------------------------------------------------- */

djinterp::filter_expr<void>* 
create_predicate(const std::string& attr,
                 const std::string& op,
                 const std::string& value)
{
    if ((op == ":") || (op == "==")) {
        return djinterp::make_predicate<void>(
            attr, djinterp::eq_predicate(value)).release();
    }
    
    if (op == "!=") {
        return djinterp::make_predicate<void>(
            attr, djinterp::ne_predicate(value)).release();
    }
    
    if (op == "<") {
        return djinterp::make_predicate<void>(
            attr, djinterp::lt_predicate(value)).release();
    }
    
    if (op == "<=") {
        return djinterp::make_predicate<void>(
            attr, djinterp::le_predicate(value)).release();
    }
    
    if (op == ">") {
        return djinterp::make_predicate<void>(
            attr, djinterp::gt_predicate(value)).release();
    }
    
    if (op == ">=") {
        return djinterp::make_predicate<void>(
            attr, djinterp::ge_predicate(value)).release();
    }
    
    if (op == "~") {
        return djinterp::make_predicate<void>(
            attr, djinterp::matches_predicate(value)).release();
    }
    
    if (op == "in") {
        // parse set literal
        std::vector<std::string> values = parse_set_literal(value);
        
        return djinterp::make_predicate<void>(
            attr, djinterp::in_set_predicate(values)).release();
    }
    
    return nullptr;
}


/* ------------------------------------------------------------------------- */
/* Helper: Parse set literal                                                 */
/* ------------------------------------------------------------------------- */

std::vector<std::string> parse_set_literal(const std::string& set_str)
{
    std::vector<std::string> values;
    std::string clean = set_str;
    
    // remove braces
    if (!clean.empty() && (clean.front() == '{')) {
        clean = clean.substr(1);
    }
    
    if (!clean.empty() && (clean.back() == '}')) {
        clean = clean.substr(0, clean.length() - 1);
    }
    
    // split by comma
    size_t start = 0;
    size_t pos = 0;
    
    while ((pos = clean.find(',', start)) != std::string::npos) {
        std::string value = clean.substr(start, pos - start);
        
        // trim whitespace
        size_t first = value.find_first_not_of(" \t");
        size_t last = value.find_last_not_of(" \t");
        
        if (first != std::string::npos) {
            values.push_back(value.substr(first, last - first + 1));
        }
        
        start = pos + 1;
    }
    
    // add last value
    if (start < clean.length()) {
        std::string value = clean.substr(start);
        size_t first = value.find_first_not_of(" \t");
        size_t last = value.find_last_not_of(" \t");
        
        if (first != std::string::npos) {
            values.push_back(value.substr(first, last - first + 1));
        }
    }
    
    return values;
}


/* ========================================================================= */
/* EXAMPLE USAGE                                                             */
/* ========================================================================= */

/*

// Example C++ code using the parser:

#include "filter_grammar_prefix.tab.hpp"
#include "filter_expr.hpp"

int main() {
    const char* input = "--and status:active priority>5";
    
    auto filter = parse_prefix_filter_expression<MyElement>(input);
    
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

COMPILATION:
------------
bison -d filter_grammar_prefix.y -o filter_grammar_prefix.tab.cpp


OPERATOR SEMANTICS:
-------------------
--and: n-ary conjunction (2+ operands)
       --and a b c   ≡   a AND b AND c

--or:  n-ary disjunction (2+ operands)
       --or a b c    ≡   a OR b OR c

--xor: binary exclusive-or (exactly 2 operands)
       --xor a b     ≡   (a AND NOT b) OR (NOT a AND b)

--not: unary negation (exactly 1 operand)
       --not a       ≡   NOT a


PARSING STRATEGY:
-----------------
Prefix notation is parsed using a simple recursive descent approach:
1. Read operator token
2. Based on operator, read required number of operands
3. Recursively parse each operand
4. Build expression tree from operator and operands


N-ARY OPERATORS:
----------------
--and and --or use expr_list production to collect 2+ operands:
  expr_list : filter_expr filter_expr           (at least 2)
            | expr_list filter_expr              (more operands)

This allows greedy parsing: consumes as many operands as available.


EXAMPLE PARSE TREES:
--------------------
Input: --and a b c
Tree:  AND
       ├── a
       ├── b
       └── c

Input: --not --or a b
Tree:  NOT
       └── OR
           ├── a
           └── b

Input: --and --or a b c
Tree:  AND
       ├── OR
       │   ├── a
       │   └── b
       └── c


ADVANTAGES:
-----------
1. No precedence rules needed (structure is explicit)
2. No shift/reduce conflicts (unambiguous)
3. Simple grammar (fewer productions than infix)
4. Easy to extend (just add new operator tokens)


LEXER REQUIREMENTS:
-------------------
The lexer must:
1. Tokenize on whitespace boundaries
2. Recognize operator tokens: --and, --or, --xor, --not
3. Return complete predicate strings as PREDICATE tokens
4. Not tokenize within predicates (e.g., "status:active" is one token)

*/
