/******************************************************************************
* djinterp [parser]                                           filter_lexer.l
*
* Flex/Lex lexer for filter expression DSL.
*   Tokenizes filter expressions for use with Bison parser (filter_grammar.y).
* Handles keywords, operators, identifiers, strings, numbers, and whitespace.
*
* COMPILATION:
*   flex -o filter_lexer.cpp filter_lexer.l
*
* USAGE WITH BISON:
*   flex -o filter_lexer.cpp filter_lexer.l
*   bison -d filter_grammar.y -o filter_grammar.tab.cpp
*   g++ -std=c++14 filter_grammar.tab.cpp filter_lexer.cpp -o filter_parser
*
* path:      \src\parser\filter_lexer.l
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
#include "filter_grammar.tab.hpp"

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
%option case-insensitive


/* ------------------------------------------------------------------------- */
/* Named patterns                                                            */
/* ------------------------------------------------------------------------- */

DIGIT           [0-9]
ALPHA           [a-zA-Z]
ALNUM           [a-zA-Z0-9]
WHITESPACE      [ \t\r\n]

IDENTIFIER      [a-zA-Z_][a-zA-Z0-9_\-]*
INTEGER         \-?{DIGIT}+
FLOAT           \-?{DIGIT}+\.{DIGIT}+
NUMBER          {INTEGER}|{FLOAT}

UNQUOTED_CHAR   [a-zA-Z0-9_\-\.\/]
UNQUOTED_STRING {UNQUOTED_CHAR}+


/* ========================================================================= */
/* RULES SECTION                                                             */
/* ========================================================================= */

%%

/* ------------------------------------------------------------------------- */
/* Whitespace (skip)                                                         */
/* ------------------------------------------------------------------------- */

{WHITESPACE}+       { /* skip whitespace */ }


/* ------------------------------------------------------------------------- */
/* Comments (optional - uncomment if needed)                                 */
/* ------------------------------------------------------------------------- */

#[^\n]*             { /* skip line comments */ }
\/\/[^\n]*          { /* skip C++ style comments */ }


/* ------------------------------------------------------------------------- */
/* Keywords (case-insensitive due to %option case-insensitive)               */
/* ------------------------------------------------------------------------- */

(?i:AND)            { return AND; }
(?i:OR)             { return OR; }
(?i:NOT)            { return NOT; }
(?i:XOR)            { return XOR; }
(?i:IN)             { return IN; }
(?i:HAS)            { return HAS; }


/* ------------------------------------------------------------------------- */
/* Operators                                                                 */
/* ------------------------------------------------------------------------- */

"=="                { return EQ; }
"!="                { return NE; }
"<="                { return LE; }
">="                { return GE; }
"<"                 { return LT; }
">"                 { return GT; }
"~="                { return MATCH; }
"~"                 { return MATCH; }
":"                 { return COLON; }


/* ------------------------------------------------------------------------- */
/* Punctuation                                                               */
/* ------------------------------------------------------------------------- */

"("                 { return LPAREN; }
")"                 { return RPAREN; }
"{"                 { return LBRACE; }
"}"                 { return RBRACE; }
","                 { return COMMA; }


/* ------------------------------------------------------------------------- */
/* String literals (quoted)                                                  */
/* ------------------------------------------------------------------------- */

\"([^\\\"]|\\.)*\"  {
                        // double-quoted string
                        yylval.str_val = strndup(yytext + 1, yyleng - 2);
                        
                        // process escape sequences
                        process_escapes(yylval.str_val);
                        
                        return STRING_LITERAL;
                    }

'([^\\']|\\.)*'     {
                        // single-quoted string
                        yylval.str_val = strndup(yytext + 1, yyleng - 2);
                        
                        // process escape sequences
                        process_escapes(yylval.str_val);
                        
                        return STRING_LITERAL;
                    }


/* ------------------------------------------------------------------------- */
/* Number literals                                                           */
/* ------------------------------------------------------------------------- */

{NUMBER}            {
                        yylval.str_val = strdup(yytext);
                        return NUMBER_LITERAL;
                    }


/* ------------------------------------------------------------------------- */
/* Unquoted strings (for simple values)                                      */
/* ------------------------------------------------------------------------- */

{UNQUOTED_STRING}   {
                        // check if it's a keyword first
                        // (this won't match keywords due to rule ordering)
                        yylval.str_val = strdup(yytext);
                        return STRING_LITERAL;
                    }


/* ------------------------------------------------------------------------- */
/* Identifiers (attribute names)                                             */
/* ------------------------------------------------------------------------- */

{IDENTIFIER}        {
                        yylval.str_val = strdup(yytext);
                        return IDENTIFIER;
                    }


/* ------------------------------------------------------------------------- */
/* Invalid characters                                                        */
/* ------------------------------------------------------------------------- */

.                   {
                        fprintf(stderr, 
                                "Lexical error: unexpected character '%c' at line %d, column %d\n",
                                yytext[0], yylineno, yycolumn);
                        return yytext[0];
                    }


%%


/* ========================================================================= */
/* USER CODE SECTION                                                         */
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/* Escape sequence processing                                                */
/* ------------------------------------------------------------------------- */

void process_escapes(char* str)
{
    char* src;
    char* dst;
    
    src = str;
    dst = str;
    
    while (*src)
    {
        if (*src == '\\')
        {
            ++src;
            
            switch (*src)
            {
                case 'n':  *dst++ = '\n'; break;
                case 't':  *dst++ = '\t'; break;
                case 'r':  *dst++ = '\r'; break;
                case '\\': *dst++ = '\\'; break;
                case '"':  *dst++ = '"';  break;
                case '\'': *dst++ = '\''; break;
                case '0':  *dst++ = '\0'; break;
                default:   *dst++ = *src; break;
            }
            
            ++src;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    
    *dst = '\0';
}


/* ------------------------------------------------------------------------- */
/* Utility functions                                                         */
/* ------------------------------------------------------------------------- */

// reset lexer state
void reset_lexer()
{
    yylineno = 1;
    yycolumn = 1;
    YY_FLUSH_BUFFER;
}

// set input string
void set_lexer_input(const char* input)
{
    yy_scan_string(input);
}

// set input file
void set_lexer_file(FILE* file)
{
    yyin = file;
}


/* ========================================================================= */
/* EXAMPLE USAGE                                                             */
/* ========================================================================= */

/*

// Example C code using the lexer:

#include "filter_lexer.h"
#include "filter_grammar.tab.hpp"

int main() {
    const char* input = "status == \"active\" AND priority > 5";
    
    // set input
    set_lexer_input(input);
    
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

COMPILATION:
------------
flex -o filter_lexer.cpp filter_lexer.l

This generates filter_lexer.cpp which can be compiled with the Bison-generated
parser.


FEATURES:
---------
1. Case-insensitive keywords (AND, and, And all work)
2. Double and single quoted strings
3. Escape sequence processing (\n, \t, \\, \", \', etc.)
4. Unquoted strings for simple values
5. Integer and floating-point numbers
6. Line and column tracking
7. Comment support (optional)


TOKEN ORDERING:
---------------
Rules are matched in order, so:
1. Keywords matched before identifiers
2. Multi-character operators before single-character
3. Quoted strings before unquoted strings


ESCAPE SEQUENCES:
-----------------
Supported in quoted strings:
  \n  - newline
  \t  - tab
  \r  - carriage return
  \\  - backslash
  \"  - double quote
  \'  - single quote
  \0  - null character


ERROR HANDLING:
---------------
Invalid characters produce error messages with line/column information.


INTEGRATION WITH BISON:
------------------------
This lexer is designed to work with filter_grammar.y.
Token values are returned via yylval.
String memory is allocated with strdup() and must be freed by parser.


EXTENSIONS:
-----------
To extend the lexer, consider adding:
1. More escape sequences
2. Unicode support
3. Better error recovery
4. String interpolation
5. Multi-line strings
6. Hexadecimal numbers
7. Scientific notation

*/
