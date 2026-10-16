/* ===========================================================================
 * math.l - Flex lexer for the math grammar (math.abnf, base subset).
 * Build with:  flex math.l && bison -d math.y && cc lex.yy.c math.tab.c -o math
 * =========================================================================*/

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "math.tab.h"

/* Track nesting depth of |...| absolute-value bars so we can tell an opening
 * '|' from a closing one. A simple toggle works because bars don't nest in
 * the base grammar; for proper nesting use a counter stack. */
static int abs_open = 0;
%}

%option noyywrap nounput noinput
%option yylineno

DIGIT       [0-9]
ALPHA       [A-Za-z]
IDCHAR      [A-Za-z0-9_]
INT         {DIGIT}+
DEC         {INT}"."{DIGIT}+
SCI         ({DEC}|{INT})[eE][+-]?{INT}

%%

[ \t\r\n]+                  { /* skip whitespace */ }
"#".*                       { /* line comment */ }

    /* ---- numeric literals (longest-match order matters) ---- */
{SCI}                       { yylval.fval = atof(yytext); return NUMBER; }
{DEC}                       { yylval.fval = atof(yytext); return NUMBER; }
{INT}                       { yylval.ival = atol(yytext); return INTEGER; }

    /* ---- multi-char operators ---- */
"=="                        { return EQ;     }
"!="                        { return NEQ;    }
"<="                        { return LEQ;    }
">="                        { return GEQ;    }
"&&"                        { return ANDAND; }
"||"                        { return OROR;   }

    /* ---- single-char operators / punctuation ---- */
"+"                         { return '+'; }
"-"                         { return '-'; }
"*"                         { return '*'; }
"/"                         { return '/'; }
"%"                         { return '%'; }
"^"                         { return '^'; }
"!"                         { return '!'; }
"="                         { return '='; }
"<"                         { return '<'; }
">"                         { return '>'; }
"("                         { return '('; }
")"                         { return ')'; }
"["                         { return '['; }
"]"                         { return ']'; }
","                         { return ','; }
"_"                         { return '_'; }

"|"                         { /* toggle: open vs close abs-value bar */
                              if (abs_open) { abs_open = 0; return ABS_CLOSE; }
                              else          { abs_open = 1; return ABS_OPEN;  }
                            }

    /* ---- mathematical constants (must precede generic IDENT) ---- */
"pi"                        { return PI;    }
"\xCF\x80"                  { return PI;    }  /* π U+03C0 */
"tau"                       { return TAU;   }
"\xCF\x84"                  { return TAU;   }  /* τ U+03C4 */
"phi"                       { return PHI;   }
"\xCF\x86"                  { return PHI;   }  /* φ U+03C6 */
"inf"                       { return INF;   }
"\xE2\x88\x9E"              { return INF;   }  /* ∞ U+221E */

    /* ---- trig ---- */
"sin"                       { return SIN;  }
"cos"                       { return COS;  }
"tan"                       { return TAN;  }
"sec"                       { return SEC;  }
"csc"                       { return CSC;  }
"cot"                       { return COT;  }

    /* ---- inverse trig ---- */
"asin"|"arcsin"             { return ASIN;  }
"acos"|"arccos"             { return ACOS;  }
"atan"|"arctan"             { return ATAN;  }
"atan2"                     { return ATAN2; }
"asec"|"arcsec"             { return ASEC;  }
"acsc"|"arccsc"             { return ACSC;  }
"acot"|"arccot"             { return ACOT;  }

    /* ---- hyperbolic + inverse hyperbolic ---- */
"sinh"                      { return SINH;  }
"cosh"                      { return COSH;  }
"tanh"                      { return TANH;  }
"sech"                      { return SECH;  }
"csch"                      { return CSCH;  }
"coth"                      { return COTH;  }
"asinh"|"arcsinh"           { return ASINH; }
"acosh"|"arccosh"           { return ACOSH; }
"atanh"|"arctanh"           { return ATANH; }
"asech"|"arcsech"           { return ASECH; }
"acsch"|"arccsch"           { return ACSCH; }
"acoth"|"arccoth"           { return ACOTH; }

    /* ---- logarithmic / exponential ---- */
"log10"                     { return LOG10; }
"log2"                      { return LOG2;  }
"log"                       { return LOG;   }
"ln"                        { return LN;    }
"exp"                       { return EXP;   }

    /* ---- special functions ---- */
"sqrt"                      { return SQRT;     }
"cbrt"                      { return CBRT;     }
"abs"                       { return ABS;      }
"floor"                     { return FLOOR;    }
"ceil"                      { return CEIL;     }
"round"                     { return ROUND;    }
"min"                       { return MIN;      }
"max"                       { return MAX;      }
"gcd"                       { return GCD;      }
"lcm"                       { return LCM;      }
"factorial"                 { return FACTORIAL;}
"gamma"                     { return GAMMA;    }
"erfc"                      { return ERFC;     }
"erf"                       { return ERF;      }
"sign"|"sgn"                { return SIGN;     }

    /* ---- coordinate / complex helpers ---- */
"polar"                     { return POLAR;     }
"rect"                      { return RECT;      }
"theta"                     { return THETA;     }
"\xCE\xB8"                  { return THETA;     }  /* θ U+03B8 */
"cylindrical"               { return CYL;       }
"rho"                       { return RHO;       }
"\xCF\x81"                  { return RHO;       }  /* ρ U+03C1 */
"spherical"                 { return SPHER;     }
"real"|"Re"                 { return RE_FN;     }
"imag"|"Im"                 { return IM_FN;     }
"arg"|"angle"               { return ARG_FN;    }
"conj"|"conjugate"          { return CONJ_FN;   }
"mag"|"modulus"             { return MAG_FN;    }

    /* ---- imaginary / Euler constants come AFTER the keyword soup so that
            "exp", "erf", etc. take precedence over a bare "e"            ---- */
"e"                         { return EULER;     }
    /* `i` and `j` are NOT reserved as imaginary unit at the syntactic
       level — they are too commonly used as loop / index variables.
       Semantic analysis can recognize them by context if desired.       */

    /* ---- generic identifier (variables, user functions) ---- */
{ALPHA}{IDCHAR}*            { yylval.sval = strdup(yytext); return IDENT; }

.                           { fprintf(stderr,
                                "math.l: unexpected character '%s' on line %d\n",
                                yytext, yylineno);
                            }

%%

/* Driver `main()` lives in math.y — building with bison's main avoids
 * a duplicate-symbol link error. Define your own externally if you prefer. */
