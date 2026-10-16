/* ===========================================================================
 * calculus.l - Flex lexer for the full math + calculus + linear-algebra +
 *              logic + set theory + discrete structures grammar.
 * Build with:
 *   flex calculus.l && bison -d calculus.y && cc lex.yy.c calculus.tab.c -o calc
 * =========================================================================*/

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "calculus.tab.h"

/* Stateful disambiguation for | and || */
static int abs_open  = 0;   /* single-bar |x|     */
static int norm_open = 0;   /* double-bar ||v||   */
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

    /* ---------------------------------------------------------------------
     * Numeric literals
     * ------------------------------------------------------------------- */
{SCI}                       { yylval.fval = atof(yytext); return NUMBER; }
{DEC}                       { yylval.fval = atof(yytext); return NUMBER; }
{INT}                       { yylval.ival = atol(yytext); return INTEGER; }

    /* ---------------------------------------------------------------------
     * Multi-char ASCII operators  (longest match first)
     * ------------------------------------------------------------------- */
"<=>"                       { return IFF;        }
"<="                        { return LEQ;        }
">="                        { return GEQ;        }
"=="                        { return EQ;         }
"!="                        { return NEQ;        }
"=>"                        { return IMPLIES;    }
"^-1"                       { return CARET_INV;  }
"^(-1)"                     { return CARET_INV;  }
"^T"                        { return CARET_T;    }
"^c"                        { return CARET_C;    }
"->"                        { return ARROW;      }

"&&"                        { return ANDAND;     }
"||"                        { /* could be logical-or OR opening/closing
                                 norm bars. We track open/close. The parser
                                 treats NORM_BARS as a delimiter; OROR is the
                                 boolean operator. We always emit NORM_BARS
                                 here and let the parser handle "logical or"
                                 via the spelled-out keyword `or` instead.
                                 If you want symbolic `||` for boolean OR,
                                 comment out norm tracking below.
                              */
                              if (norm_open) { norm_open = 0; return NORM_CLOSE; }
                              else           { norm_open = 1; return NORM_OPEN;  }
                            }

    /* ---------------------------------------------------------------------
     * Single-char operators / punctuation
     * ------------------------------------------------------------------- */
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
"{"                         { return '{'; }
"}"                         { return '}'; }
","                         { return ','; }
"_"                         { return '_'; }
";"                         { return ';'; }
":"                         { return ':'; }
"."                         { return '.'; }   /* dot product (ASCII) */
"'"                         { return PRIME;     }   /* prime: deriv/transpose/complement */
"\\"                        { return BACKSLASH; }   /* set-difference / left-division */
"#"                         { /* set cardinality #S — falls through to '#' lit */ return HASH; }

"|"                         { /* single-bar: |x| abs / cardinality / set-builder */
                              if (abs_open) { abs_open = 0; return ABS_CLOSE; }
                              else          { abs_open = 1; return ABS_OPEN;  }
                            }

    /* ---------------------------------------------------------------------
     * Multi-byte UTF-8 unicode operators
     *   We match each codepoint by its raw UTF-8 bytes so flex stays
     *   single-byte and dependency-free.
     * ------------------------------------------------------------------- */

    /* calculus */
"\xE2\x88\xAB\xE2\x88\xAB\xE2\x88\xAB" { return INTEGRAL3; }   /* ∫∫∫ */
"\xE2\x88\xAB\xE2\x88\xAB"             { return INTEGRAL2; }   /* ∫∫  */
"\xE2\x88\xAB"                         { return INTEGRAL;  }   /* ∫   */
"\xE2\x88\x82"                         { return PARTIAL;   }   /* ∂   */
"\xE2\x88\x91"                         { return SUMMATION; }   /* ∑   */
"\xE2\x88\x8F"                         { return PRODUCT;   }   /* ∏   */
"\xE2\x88\x87"                         { return NABLA;     }   /* ∇   */
"\xE2\x86\x92"                         { return ARROW;     }   /* →   */
"\xE2\x88\x86"                         { return DELTA_CAP; }   /* ∆   */
"\xCE\xB4"                             { return DELTA_LO;  }   /* δ   */

    /* logic */
"\xE2\x88\x80"                         { return FORALL;    }   /* ∀   */
"\xE2\x88\x83"                         { return EXISTS;    }   /* ∃   */
"\xE2\x88\xA8"                         { return OR_SYM;    }   /* ∨   */
"\xE2\x88\xA7"                         { return AND_SYM;   }   /* ∧   */
"\xC2\xAC"                             { return NOT_SYM;   }   /* ¬   */
"\xE2\x86\x94"                         { return IFF_SYM;   }   /* ↔   */
"\xE2\x8A\xA4"                         { return TOP_SYM;   }   /* ⊤   */
"\xE2\x8A\xA5"                         { return BOT_SYM;   }   /* ⊥   */

    /* set theory */
"\xE2\x88\x88"                         { return IN_SYM;    }   /* ∈   */
"\xE2\x88\x89"                         { return NOTIN_SYM; }   /* ∉   */
"\xE2\x88\xAA"                         { return UNION_SYM; }   /* ∪   */
"\xE2\x88\xA9"                         { return INTER_SYM; }   /* ∩   */
"\xE2\x8A\x86"                         { return SUBSETEQ;  }   /* ⊆   */
"\xE2\x8A\x87"                         { return SUPSETEQ;  }   /* ⊇   */
"\xE2\x8A\x82"                         { return SUBSET;    }   /* ⊂   */
"\xE2\x8A\x83"                         { return SUPSET;    }   /* ⊃   */
"\xC3\x97"                             { return TIMES_SYM; }   /* ×   */
"\xC2\xB7"                             { return CDOT_SYM;  }   /* ·   */
"\xE2\x88\x85"                         { return EMPTY_SYM; }   /* ∅   */
"\xE2\x84\x98"                         { return POWERSET;  }   /* ℘   */
"\xE2\x89\xA1"                         { return EQUIV_SYM; }   /* ≡   */

    /* number sets */
"\xE2\x84\x95"                         { return NAT_SET;   }   /* ℕ   */
"\xE2\x84\xA4"                         { return INT_SET;   }   /* ℤ   */
"\xE2\x84\x9A"                         { return RAT_SET;   }   /* ℚ   */
"\xE2\x84\x9D"                         { return REAL_SET;  }   /* ℝ   */
"\xE2\x84\x82"                         { return CPX_SET;   }   /* ℂ   */

    /* floor / ceil */
"\xE2\x8C\x8A"                         { return LFLOOR;    }   /* ⌊   */
"\xE2\x8C\x8B"                         { return RFLOOR;    }   /* ⌋   */
"\xE2\x8C\x88"                         { return LCEIL;     }   /* ⌈   */
"\xE2\x8C\x89"                         { return RCEIL;     }   /* ⌉   */

    /* greek constants used as identifiers/coordinates */
"\xCF\x80"                             { return PI;        }   /* π   */
"\xCF\x84"                             { return TAU;       }   /* τ   */
"\xCF\x86"                             { return PHI;       }   /* φ   */
"\xCE\xB8"                             { return THETA;     }   /* θ   */
"\xCF\x81"                             { return RHO;       }   /* ρ   */
"\xE2\x88\x9E"                         { return INF;       }   /* ∞   */

    /* ---------------------------------------------------------------------
     * Reserved words — must precede generic IDENT
     * ------------------------------------------------------------------- */

    /* calculus */
"diff"|"derivative"         { return DIFF_FN;     }
"partial"                   { return PARTIAL_KW;  }
"grad"                      { return GRAD_KW;     }
"integrate"                 { return INTEGRATE_FN;}
"lim"                       { return LIM;         }
"limit"                     { return LIMIT_FN;    }
"sum"                       { return SUM_FN;      }
"product"                   { return PRODUCT_FN;  }
"div"                       { return DIV_KW;      }
"curl"                      { return CURL_KW;     }

    /* linear algebra */
"transpose"                 { return TRANSPOSE_FN;}
"inv"                       { return INV_FN;      }
"det"                       { return DET_FN;      }
"trace"                     { return TRACE_FN;    }
"tr"                        { return TR_FN;       }
"rank"                      { return RANK_FN;     }
"eigenvalues"|"eig"         { return EIG_FN;      }
"eigenvectors"              { return EIGVEC_FN;   }
"dot"                       { return DOT_FN;      }
"cross"                     { return CROSS_FN;    }
"norm"                      { return NORM_FN;     }
"normalize"|"unit"          { return NORMALIZE_FN;}
"zeros"                     { return ZEROS_FN;    }
"ones"                      { return ONES_FN;     }
"eye"                       { return EYE_FN;      }
"diag"                      { return DIAG_FN;     }
"reshape"                   { return RESHAPE_FN;  }
"solve"                     { return SOLVE_FN;    }
"lu"                        { return LU_FN;       }
"qr"                        { return QR_FN;       }
"svd"                       { return SVD_FN;      }
"cholesky"                  { return CHOLESKY_FN; }

    /* logic */
"forall"                    { return FORALL_KW;   }
"exists"                    { return EXISTS_KW;   }
"and"                       { return AND_KW;      }
"or"                        { return OR_KW;       }
"not"                       { return NOT_KW;      }
"implies"                   { return IMPLIES_KW;  }
"iff"                       { return IFF_KW;      }
"true"                      { return TRUE_KW;     }
"false"                     { return FALSE_KW;    }

    /* set theory */
"union"                     { return UNION_KW;    }
"intersection"              { return INTER_KW;    }
"difference"                { return DIFF_KW;     }
"complement"                { return COMP_KW;     }
"cartesian"                 { return CART_KW;     }
"powerset"                  { return PWRSET_KW;   }
"subset"                    { return SUBSET_KW;   }
"superset"                  { return SUPSET_KW;   }
"proper_subset"             { return PSUBSET_KW;  }
"emptyset"                  { return EMPTY_KW;    }
"in"                        { return IN_KW;       }
"card"|"cardinality"        { return CARD_KW;     }
"sup"                       { return SUP_KW;      }

    /* discrete */
"choose"                    { return CHOOSE_KW;   }
"binom"                     { return BINOM_KW;    }
"permutation"               { return PERM_KW;     }
"combination"               { return COMB_KW;     }
"mod"                       { return MOD_KW;      }
"totient"                   { return TOTIENT_KW;  }
"mobius"                    { return MOBIUS_KW;   }
"isprime"                   { return ISPRIME_KW;  }
"nextprime"                 { return NEXTPRIME_KW;}
"primes"                    { return PRIMES_KW;   }
"factors"                   { return FACTORS_KW;  }
"sequence"                  { return SEQUENCE_KW; }
"degree"                    { return DEGREE_KW;   }
"neighbors"                 { return NEIGHBORS_KW;}
"adjacent"                  { return ADJ_KW;      }
"connected"                 { return CONN_KW;     }
"cycle"                     { return CYCLE_KW;    }
"path"                      { return PATH_KW;     }
"graph"                     { return GRAPH_KW;    }

    /* ---------------------------------------------------------------------
     * Math base-grammar keywords (subset of math.l)
     * ------------------------------------------------------------------- */
"pi"                        { return PI;     }
"tau"                       { return TAU;    }
"phi"                       { return PHI;    }
"inf"                       { return INF;    }

"sin"                       { return SIN;  }
"cos"                       { return COS;  }
"tan"                       { return TAN;  }
"sec"                       { return SEC;  }
"csc"                       { return CSC;  }
"cot"                       { return COT;  }
"asin"|"arcsin"             { return ASIN;  }
"acos"|"arccos"             { return ACOS;  }
"atan2"                     { return ATAN2; }
"atan"|"arctan"             { return ATAN;  }
"asec"|"arcsec"             { return ASEC;  }
"acsc"|"arccsc"             { return ACSC;  }
"acot"|"arccot"             { return ACOT;  }
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
"log10"                     { return LOG10; }
"log2"                      { return LOG2;  }
"log"                       { return LOG;   }
"ln"                        { return LN;    }
"exp"                       { return EXP;   }
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
"polar"                     { return POLAR;    }
"rect"                      { return RECT;     }
"theta"                     { return THETA;    }
"cylindrical"               { return CYL;      }
"rho"                       { return RHO;      }
"spherical"                 { return SPHER;    }
"real"|"Re"                 { return RE_FN;    }
"imag"|"Im"                 { return IM_FN;    }
"arg"|"angle"               { return ARG_FN;   }
"conj"|"conjugate"          { return CONJ_FN;  }
"mag"|"modulus"             { return MAG_FN;   }
"d"[a-zA-Z][a-zA-Z0-9_]*    { /* `dx`, `dt`, `du2` etc. — single DIFFERENTIAL token */
                              yylval.sval = strdup(yytext + 1);
                              return DIFFERENTIAL; }
"d"                         { return D_KW;     }   /* `d` alone: d/dx form */
"D"                         { return D_CAP;    }   /* D_x(...) */

"e"                         { return EULER;    }
    /* `i` and `j` are NOT reserved — they are too commonly used as loop and
       index variables. Semantic analysis can still recognize them as the
       imaginary unit when appropriate.                                   */

    /* ---------------------------------------------------------------------
     * Generic identifier (variables, user functions)
     * ------------------------------------------------------------------- */
{ALPHA}{IDCHAR}*            { yylval.sval = strdup(yytext); return IDENT; }

    /* ---------------------------------------------------------------------
     * Unknown
     * ------------------------------------------------------------------- */
.                           { fprintf(stderr,
                                "calculus.l: unexpected byte 0x%02x ('%s') line %d\n",
                                (unsigned char)yytext[0], yytext, yylineno);
                            }

%%
