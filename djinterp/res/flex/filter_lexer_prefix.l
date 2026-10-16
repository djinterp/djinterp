/******************************************************************************
* djinterp [parser]                                     filter_lexer_prefix.l
*
* Flex/Lex lexer for prefix-style filter expressions.
*   Tokenizes --and, --or, --not, --xor operators and complete predicates.
* Designed for whitespace-separated tokens.
*
* COMPILATION:
*   flex -o filter_lexer_prefix.cpp filter_lexer_prefix.l
*
* USAGE WITH BISON:
*   flex -o filter_lexer_prefix.cpp filter_lexer_prefix.l
*   bison -d filter_grammar_prefix.y -o filter_grammar_prefix.tab.cpp
*   g++ -std=c++14 filter_grammar_prefix.tab.cpp filter_lexer_prefix.cpp
*
* path:      \src\parser\filter_lexer_prefix.l
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.15
******************************************************************************/

/* ========================================================================= */
/* DEFINITIONS SECTION                                                       */
/* ========================================================================= */

%{
#include <cstdlib>
#include <cstring>
#include <string>
#include "filter_grammar_prefix.tab.hpp"

// line and column tracking
int yylineno = 1;
int yycolumn = 1;

// update position tracking
#define YY_USER_ACTION                      \
    yylloc.first_line = yylineno;           \
    yylloc.first_column = yycolumn;         \
    yycolumn += yyleng;                     \
    yylloc.last_line = yylineno;            \
    yylloc.last_column = yycolumn;

%}

/* ------------------------------------------------------------------------- */
/* Options                                                                   */
/* ------------------------------------------------------------------------- */

%option noyywrap
%option yylineno


/* ------------------------------------------------------------------------- */
/* Named patterns                                                            */
/* ------------------------------------------------------------------------- */

WHITESPACE      [ \t\r\n]+

OPERATOR_PREFIX "--"

PREDICATE_CHAR  [a-zA-Z0-9_\-\.\/:<>=~!{},"]
PREDICATE       {PREDICATE_CHAR}+


/* ========================================================================= */
/* RULES SECTION                                                             */
/* ========================================================================= */

%%

/* ------------------------------------------------------------------------- */
/* Whitespace (required separator)                                           */
/* ------------------------------------------------------------------------- */

{WHITESPACE}        { /* skip but track newlines */ 
                      for (int i = 0; i < yyleng; ++i) {
                          if (yytext[i] == '\n') {
                              ++yylineno;
                              yycolumn = 1;
                          }
                      }
                    }


/* ------------------------------------------------------------------------- */
/* Prefix operators                                                          */
/* ------------------------------------------------------------------------- */

"--and"             { return AND_OP; }
"--or"              { return OR_OP; }
"--xor"             { return XOR_OP; }
"--not"             { return NOT_OP; }


/* ------------------------------------------------------------------------- */
/* Optional: Parentheses for grouping                                        */
/* ------------------------------------------------------------------------- */

"("                 { return LPAREN; }
")"                 { return RPAREN; }


/* ------------------------------------------------------------------------- */
/* Predicates (complete predicate strings)                                   */
/* ------------------------------------------------------------------------- */

{PREDICATE}         {
                        yylval.str_val = strdup(yytext);
                        return PREDICATE;
                    }


/* ------------------------------------------------------------------------- */
/* Invalid characters                                                        */
/* ------------------------------------------------------------------------- */

.                   {
                        fprintf(stderr, 
                                "Lexical error: unexpected character '%c' "
                                "at line %d, column %d\n",
                                yytext[0], yylineno, yycolumn);
                        return yytext[0];
                    }


%%


/* ========================================================================= */
/* USER CODE SECTION                                                         */
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/* Utility functions                                                         */
/* ------------------------------------------------------------------------- */

// reset lexer state
void reset_prefix_lexer()
{
    yylineno = 1;
    yycolumn = 1;
    YY_FLUSH_BUFFER;
}

// set input string
void set_prefix_lexer_input(const char* input)
{
    yy_scan_string(input);
}

// set input file
void set_prefix_lexer_file(FILE* file)
{
    yyin = file;
}


/* ========================================================================= */
/* EXAMPLE USAGE                                                             */
/* ========================================================================= */

/*

// Example C code using the lexer:

#include "filter_lexer_prefix.h"
#include "filter_grammar_prefix.tab.hpp"

int main() {
    const char* input = "--and status:active priority>5";
    
    // set input
    set_prefix_lexer_input(input);
    
    // tokenize
    int token;
    while ((token = yylex()) != 0) {
        printf("Token: %d, Text: %s\n", token, yytext);
    }
    
    return 0;
}

*/


/* ========================================================================= */
/* NOTES                                                                     */
/* ========================================================================= */

/*

TOKENIZATION STRATEGY:
----------------------
Unlike the infix lexer, this lexer treats complete predicates as single tokens:

  Input:  --and status:active priority>5
  Tokens: AND_OP PREDICATE("status:active") PREDICATE("priority>5")

This simplifies the grammar since predicates don't need to be parsed as
separate attribute, operator, and value tokens.


PREDICATE FORMAT:
-----------------
Predicates are sequences of alphanumeric characters and special characters:
  - Letters, digits, underscore, hyphen
  - Operators: : < > = ! ~
  - Delimiters: { } ,
  - Quotes: "

Examples:
  status:active
  priority>5
  name~"test.*"
  type in {bug,task}


WHITESPACE HANDLING:
--------------------
Whitespace is REQUIRED to separate tokens:
  ✓ --and status:active priority>5
  ✗ --andstatus:activepriority>5

This is natural for CLI usage where arguments are space-separated.


OPERATOR RECOGNITION:
---------------------
Operators are recognized by their "--" prefix:
  --and  → AND_OP
  --or   → OR_OP
  --xor  → XOR_OP
  --not  → NOT_OP

The "--" prefix distinguishes operators from predicates, so there's no
ambiguity between:
  --and (operator)
  and   (predicate attribute name)


LEXER SIMPLICITY:
-----------------
This lexer is simpler than the infix version because:
1. Fewer token types (just operators and predicates)
2. No need to parse within predicates
3. Whitespace always delimits tokens
4. No quoting issues (predicates are atomic)


COMPARISON WITH INFIX LEXER:
-----------------------------
Infix lexer:
  Input:  status == "active" AND priority > 5
  Tokens: IDENTIFIER EQ STRING AND IDENTIFIER GT NUMBER

Prefix lexer:
  Input:  --and status:active priority>5
  Tokens: AND_OP PREDICATE PREDICATE

The prefix lexer defers predicate parsing to the parser (or semantic actions),
while the infix lexer fully tokenizes predicates.


ADVANTAGES:
-----------
1. Simpler lexer (fewer rules)
2. Natural for whitespace-separated input
3. Easy to extend (just add operator patterns)
4. No quoting ambiguity


DISADVANTAGES:
--------------
1. Predicate validation deferred to parser
2. Less granular error messages for malformed predicates
3. Parser must handle predicate parsing


SHELL INTEGRATION:
------------------
This tokenization strategy works well with shell arguments:

  bash$ myapp --and status:active priority>5

The shell splits on whitespace:
  argv[0] = "myapp"
  argv[1] = "--and"
  argv[2] = "status:active"
  argv[3] = "priority>5"

Each argv element maps directly to a lexer token.

*/
