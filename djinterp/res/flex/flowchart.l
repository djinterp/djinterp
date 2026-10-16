/* ============================================================================
 * flowchart.l — flex specification for the FLOWCHART DSL
 *
 * The DSL describes 2D/3D flowcharts: nodes, edges, connectors, groups,
 * layers, annotations, plus style/content blocks and embedded math
 * expressions for positions, transforms, and parametric shapes.
 *
 * Lexing strategy
 * ---------------
 * - Only *structural* keywords are reserved (node, edge, connector, group,
 *   layer, annotation, dimension, dim, time, defaults, at, from, to, via,
 *   style, content, text, label, rich, animate, expr, in, br, true, false,
 *   yes, no).
 * - Shape names (rectangle, circle, …), color names (red, blue, …), property
 *   keys (width, font_size, …), routing types (manhattan, bezier, …) etc.
 *   all fall through to IDENT and are validated semantically.
 * - Measurements are lexed as a single token: `2px`, `45deg`, `100ms`,
 *   `1.5em`, `90°`, `50%`. This avoids implicit-multiplication ambiguity in
 *   the math expression chain.
 * - Hex colors `#abc`, `#aabbcc`, `#aabbccdd` are single HEX_COLOR tokens.
 * - `2D` and `3D` are special tokens (they cannot match the IDENT pattern,
 *   which must start with a letter).
 * - Comments: `//` and `;` to end of line.
 * ========================================================================== */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flowchart.tab.h"

void yyerror(const char *s);
%}

%option noyywrap yylineno

DIGIT       [0-9]
ALPHA       [a-zA-Z]
IDCHAR      [a-zA-Z0-9_\-]
INT         {DIGIT}+
DEC         {DIGIT}+"."{DIGIT}+
SCI         ({DEC}|{INT})[eE][+-]?{DIGIT}+
NUM         ({SCI}|{DEC}|{INT})
HEX         [0-9a-fA-F]

LEN_UNIT    "px"|"pt"|"em"|"rem"|"cm"|"mm"|"in"|"pc"
ANG_UNIT    "deg"|"rad"|"grad"|"turn"
TIME_UNIT   "ms"|"s"|"frames"|"f"
ANY_UNIT    {LEN_UNIT}|{ANG_UNIT}|{TIME_UNIT}

%%

    /* ---- whitespace and comments ---------------------------------------- */
[ \t\r\n]+              { /* skip */ }
"//".*                  { /* line comment */ }

    /* ---- string literals ------------------------------------------------ */
\"([^"\\\n]|\\.)*\"     { yylval.sval = strdup(yytext); return STRING; }

    /* ---- hex colors (must come before number / # punctuation) ---------- */
"#"{HEX}{8}             { yylval.sval = strdup(yytext); return HEX_COLOR; }
"#"{HEX}{6}             { yylval.sval = strdup(yytext); return HEX_COLOR; }
"#"{HEX}{3}             { yylval.sval = strdup(yytext); return HEX_COLOR; }

    /* ---- compound number tokens (number+unit are one token) ------------ */
{NUM}({LEN_UNIT}|{TIME_UNIT}|{ANG_UNIT})  {
                              yylval.sval = strdup(yytext);
                              return MEASUREMENT;
                          }
{NUM}"%"                  { yylval.sval = strdup(yytext); return PERCENTAGE; }
{NUM}"\xC2\xB0"           { yylval.sval = strdup(yytext); return ANGLE_DEG; }

    /* ---- bare numbers (after measurements so units win when present) --- */
{SCI}                   { yylval.fval = atof(yytext); return NUMBER; }
{DEC}                   { yylval.fval = atof(yytext); return NUMBER; }
{INT}                   { yylval.ival = atol(yytext); return INTEGER; }

    /* ---- special dimension literals (cannot match IDENT) --------------- */
"2D"                    { return DIM_2D; }
"3D"                    { return DIM_3D; }

    /* ---- multi-character operators ------------------------------------- */
".."                    { return DOTDOT; }
"=="                    { return EQ;     }
"!="                    { return NEQ;    }
"<="                    { return LEQ;    }
">="                    { return GEQ;    }
"&&"                    { return ANDAND; }
"||"                    { return OROR;   }
"$$"                    { return DOLLAR_DOLLAR; }

    /* ---- single-character punctuation ---------------------------------- */
[(){}\[\],:;.+\-*/%^=<>$_!] { return yytext[0]; }

    /* ---- unicode operators --------------------------------------------- */
"\xC2\xB0"              { return DEGREE; }    /* ° standalone */
"\xE2\x88\x88"          { return IN_SYM; }    /* ∈           */
"\xE2\x89\xA4"          { return LEQ;    }    /* ≤           */
"\xE2\x89\xA5"          { return GEQ;    }    /* ≥           */

    /* ---- structural reserved keywords ---------------------------------- */
"node"                  { return NODE_KW;       }
"edge"                  { return EDGE_KW;       }
"connector"             { return CONNECTOR_KW;  }
"group"                 { return GROUP_KW;      }
"layer"                 { return LAYER_KW;      }
"annotation"            { return ANNOTATION_KW; }
"dimension"             { return DIMENSION_KW;  }
"dim"                   { return DIM_KW;        }
"time"                  { return TIME_KW;       }
"defaults"              { return DEFAULTS_KW;   }
"at"                    { return AT_KW;         }
"from"                  { return FROM_KW;       }
"to"                    { return TO_KW;         }
"via"                   { return VIA_KW;        }
"style"                 { return STYLE_KW;      }
"content"               { return CONTENT_KW;    }
"text"                  { return TEXT_KW;       }
"label"                 { return LABEL_KW;      }
"rich"                  { return RICH_KW;       }
"animate"               { return ANIMATE_KW;    }
"expr"                  { return EXPR_KW;       }
"in"                    { return IN_KW;         }
"br"                    { return BR_KW;         }
"true"                  { return TRUE_KW;       }
"false"                 { return FALSE_KW;      }
"yes"                   { return YES_KW;        }
"no"                    { return NO_KW;         }

    /* ---- generic identifier (variables, shape/color/property names) --- */
{ALPHA}{IDCHAR}*        { yylval.sval = strdup(yytext); return IDENT; }

    /* ---- catch-all ----------------------------------------------------- */
.                       { fprintf(stderr,
                                  "flowchart.l: unexpected '%s' at line %d\n",
                                  yytext, yylineno); }

%%
