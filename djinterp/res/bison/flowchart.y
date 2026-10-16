/* ============================================================================
 * flowchart.y — bison specification for the FLOWCHART DSL
 *
 * Parses a flowchart-document: a header (dimension, optional time/defaults)
 * followed by elements (nodes, edges, connectors, groups, layers,
 * annotations). Most attribute *names* (shape names, color names, property
 * keys, routing types) are accepted as IDENT and validated semantically; the
 * grammar focuses on overall structure.
 *
 * Math expressions live in a dedicated chain at the bottom of the file. They
 * cover arithmetic (+, -, *, /, %, ^), comparison, logical ops, function
 * calls, and parens — enough for positions, transforms, and parametric
 * shapes. (Implicit multiplication and the rich math-function reservations
 * from math.y are intentionally omitted here so that flowchart values like
 * `padding: 10px 20px` parse as compound values rather than `10px * 20px`.)
 *
 * Two pragmatic deviations from the ABNF
 * --------------------------------------
 * 1. Only the long forms `dimension:` and `time:` are accepted as header
 *    introducers. The short alias `t:` is dropped because reserving `t`
 *    would break parametric shapes that use `t` as a parameter variable.
 * 2. Property values are uniformly `value-list` (CSS-like sequences with
 *    optional commas) rather than each property having its own bespoke
 *    grammar. Values like `border: 2px solid red`, `transform: translate(…),
 *    rotate(…)`, `gradient: linear(45deg, red 0%, blue 100%)` all parse
 *    cleanly under this single rule. Validation of which keys/value shapes
 *    are legal happens in the semantic pass.
 * ========================================================================== */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylex(void);
extern int yylineno;
extern char *yytext;
void yyerror(const char *s);
%}

%expect 6
/*
 * Six benign shift/reduce conflicts, all resolved by bison's default-shift:
 *
 *   1. `value_item: IDENT` vs `value_item: IDENT '(' arg_list_opt ')'` —
 *      lookahead `(` after an IDENT in a value position is treated as a
 *      function call (e.g. `rgb(255,0,0)`, `translate(10,20)`).
 *
 *   2,3. `label_spec: label_text [label_position] [style_block]` — the
 *      optional trailing parts attach to the label rather than starting a
 *      new edge_decoration.
 *
 *   4. `content_block: TEXT_KW ':' STRING [style_block]` — same pattern.
 *
 *   5,6. `value_seq` extends greedily on a following IDENT so compound
 *      values such as `border: 2px solid red` parse as a single value.
 *      Side-effect: properties whose value ends in an IDENT must be
 *      separated by `;` from the next property, otherwise the next
 *      property name is absorbed into the previous value:
 *
 *          color: red; border: solid;     // OK
 *          color: red  border: solid       // ERROR — `red border` is the
 *                                          //         value, then `:` is
 *                                          //         unexpected.
 *      `;` is a no-op separator inside a property block.
 */

%union {
    long    ival;
    double  fval;
    char   *sval;
}

%token <ival>  INTEGER
%token <fval>  NUMBER
%token <sval>  IDENT STRING HEX_COLOR
%token <sval>  MEASUREMENT PERCENTAGE ANGLE_DEG

%token DIM_2D DIM_3D
%token DEGREE IN_SYM
%token DOTDOT DOLLAR_DOLLAR
%token EQ NEQ LEQ GEQ ANDAND OROR

/* Structural keywords */
%token NODE_KW EDGE_KW CONNECTOR_KW GROUP_KW LAYER_KW ANNOTATION_KW
%token DIMENSION_KW DIM_KW TIME_KW DEFAULTS_KW
%token AT_KW FROM_KW TO_KW VIA_KW
%token STYLE_KW CONTENT_KW TEXT_KW LABEL_KW RICH_KW
%token EXPR_KW ANIMATE_KW IN_KW BR_KW
%token TRUE_KW FALSE_KW YES_KW NO_KW

/* Math expression precedence (lowest → highest) */
%right '='
%left  OROR
%left  ANDAND
%nonassoc EQ NEQ
%nonassoc '<' '>' LEQ GEQ
%left  '+' '-'
%left  '*' '/' '%'
%right '^'
%right UMINUS LNOT
%left  POSTFIX_BANG

%start flowchart_document

%%

/* ========================================================================
 * TOP-LEVEL DOCUMENT
 * ====================================================================== */

flowchart_document
    : flowchart_header element_list
    ;

flowchart_header
    : dimension_spec
    | dimension_spec time_spec
    | dimension_spec defaults_spec
    | dimension_spec time_spec defaults_spec
    | dimension_spec defaults_spec time_spec
    ;

element_list
    : /* empty */
    | element_list flowchart_element
    ;

flowchart_element
    : node_definition
    | edge_definition
    | connector_definition
    | group_definition
    | annotation_definition
    | layer_definition
    ;

/* ========================================================================
 * HEADER: DIMENSION / TIME / DEFAULTS
 * ====================================================================== */

dimension_spec
    : DIMENSION_KW ':' DIM_2D
    | DIMENSION_KW ':' DIM_3D
    | DIM_KW       ':' INTEGER          /* 2 or 3 — checked semantically */
    ;

time_spec
    : TIME_KW ':' IDENT                 /* static / animated / timeline */
    | TIME_KW ':' time_range
    ;

time_range
    : expression DOTDOT expression
    | '[' expression ',' expression ']'
    ;

defaults_spec
    : DEFAULTS_KW '{' property_list '}'
    ;

/* ========================================================================
 * NODE
 * ====================================================================== */

node_definition
    : NODE_KW IDENT node_type_opt position_spec node_decoration_list
    ;

node_type_opt
    : /* empty */
    | ':' shape_spec
    ;

shape_spec
    : IDENT                                 /* rectangle, circle, sphere, … */
    | IDENT '(' INTEGER ')'                 /* star(5), prism(6) */
    | IDENT '{' shape_definition '}'        /* custom { … } */
    ;

shape_definition
    : parametric_shape
    | implicit_shape
    | point_list_shape
    ;

parametric_shape
    : IDENT ':' expression ',' IDENT ':' expression                      ',' parameter_range
    | IDENT ':' expression ',' IDENT ':' expression ',' IDENT ':' expression ',' parameter_range
    ;

implicit_shape
    : IDENT ':' expression                  /* `equation: x^2 + y^2 < 4`,
                                               `equation: f(x, y) == 0`, etc.
                                               Comparisons are part of `expression`. */
    ;

point_list_shape
    : IDENT ':' point_list
    | IDENT ':' point_list ',' IDENT ':' boolean
    ;

point_list
    : '[' point_seq ']'
    ;

point_seq
    : point
    | point_seq ',' point
    ;

point
    : '(' expression ',' expression ')'
    | '(' expression ',' expression ',' expression ')'
    ;

parameter_range
    : IDENT IN_SYM '[' expression ',' expression ']'
    | IDENT IN_KW  range_expr
    ;

range_expr
    : expression DOTDOT expression
    | '[' expression ',' expression ']'
    ;

position_spec
    : AT_KW position
    | AT_KW position orientation_clause
    ;

position
    : '(' expression ',' expression ')'                      /* 2D */
    | '(' expression ',' expression ',' expression ')'       /* 3D */
    | IDENT '.' IDENT                                        /* node.anchor */
    | IDENT '.' IDENT '+' value_item                         /* with offset */
    | IDENT '.' IDENT '-' value_item
    | '[' INTEGER ',' INTEGER ']'                            /* grid 2D */
    | '[' INTEGER ',' INTEGER ',' INTEGER ']'                /* grid 3D */
    | IDENT '(' arg_list ')'                                 /* f(x,y), polar(r,θ), … */
    ;

orientation_clause
    : IDENT ':' angle                                        /* rotate: 45deg */
    | IDENT ':' angle ',' IDENT ':' angle ',' IDENT ':' angle  /* pitch/yaw/roll */
    ;

angle
    : MEASUREMENT
    | ANGLE_DEG
    | NUMBER
    | INTEGER
    | NUMBER  DEGREE
    | INTEGER DEGREE
    | NUMBER  IDENT                                          /* 45 deg */
    | INTEGER IDENT
    ;

node_decoration_list
    : /* empty */
    | node_decoration_list node_decoration
    ;

node_decoration
    : style_block
    | content_block
    ;

/* ========================================================================
 * EDGE
 * ====================================================================== */

edge_definition
    : EDGE_KW IDENT edge_spec edge_decoration_list
    ;

edge_spec
    : FROM_KW node_reference TO_KW node_reference
    | FROM_KW node_reference TO_KW node_reference VIA_KW waypoint_list
    ;

node_reference
    : IDENT
    | IDENT '.' IDENT
    ;

waypoint_list
    : waypoint
    | waypoint_list ',' waypoint
    ;

waypoint
    : position
    | IDENT                                                  /* manhattan, smooth, … */
    ;

edge_decoration_list
    : /* empty */
    | edge_decoration_list edge_decoration
    ;

edge_decoration
    : style_block
    | edge_label
    ;

edge_label
    : LABEL_KW label_spec
    ;

label_spec
    : label_text
    | label_text label_position
    | label_text style_block
    | label_text label_position style_block
    ;

label_text
    : STRING
    | EXPR_KW ':' expression
    ;

label_position
    : AT_KW IDENT                                            /* start, middle, … */
    | AT_KW NUMBER                                           /* 0–1 along edge   */
    | AT_KW INTEGER
    | AT_KW position
    ;

/* ========================================================================
 * CONNECTOR
 * ====================================================================== */

connector_definition
    : CONNECTOR_KW IDENT connector_spec connector_decoration_list
    ;

connector_spec
    : FROM_KW connector_end TO_KW connector_end
    | FROM_KW connector_end TO_KW connector_end routing_clause
    ;

connector_end
    : node_reference
    | point
    | IDENT '(' arg_list ')'                                 /* polar/cylindrical/… */
    ;

routing_clause
    : IDENT ':' value_seq                                    /* routing: <type> [params] */
    ;

connector_decoration_list
    : /* empty */
    | connector_decoration_list style_block
    ;

/* ========================================================================
 * GROUP / LAYER
 * ====================================================================== */

group_definition
    : GROUP_KW IDENT                                  '{' element_list '}'
    | GROUP_KW IDENT AT_KW position                   '{' element_list '}'
    | GROUP_KW IDENT             style_block          '{' element_list '}'
    | GROUP_KW IDENT AT_KW position style_block       '{' element_list '}'
    ;

layer_definition
    : LAYER_KW IDENT                                  '{' element_list '}'
    | LAYER_KW IDENT             style_block          '{' element_list '}'
    ;

/* ========================================================================
 * ANNOTATION
 * ====================================================================== */

annotation_definition
    : ANNOTATION_KW IDENT IDENT annotation_position                       content_block
    | ANNOTATION_KW IDENT IDENT annotation_position style_block           content_block
    ;

annotation_position
    : position                                  /* per ABNF: position alone */
    | AT_KW position                            /* `at`-prefixed for consistency */
    ;

/* ========================================================================
 * STYLE BLOCK / PROPERTY LIST
 *
 * Generic IDENT ':' value-list pattern handles the bulk of the spec —
 * border, font, color, transform, gradient, shadow, padding, margin, etc.
 * ====================================================================== */

style_block
    : STYLE_KW '{' property_list '}'
    ;

property_list
    : /* empty */
    | property_list property
    | property_list ';'                         /* `;` is a no-op separator */
    ;

property
    : IDENT       ':' value_list
    | ANIMATE_KW  ':' animation_block
    ;

/* Animation has its own structure (keyframes are blocks, not values) */
animation_block
    : '{' keyframe_list '}'
    ;

keyframe_list
    : /* empty */
    | keyframe_list keyframe
    ;

keyframe
    : AT_KW       value_seq      '{' property_list '}'
    | NUMBER      ':'            '{' property_list '}'
    | INTEGER     ':'            '{' property_list '}'
    | MEASUREMENT ':'            '{' property_list '}'
    ;

/* value_list is one or more comma-separated value_seqs.
   value_seq is a space-separated sequence of value_items.   */
value_list
    : value_seq
    | value_list ',' value_seq
    ;

value_seq
    : value_item
    | value_seq value_item
    ;

value_item
    : NUMBER
    | INTEGER
    | MEASUREMENT
    | PERCENTAGE
    | ANGLE_DEG
    | HEX_COLOR
    | STRING
    | IDENT
    | IDENT '(' arg_list_opt ')'                /* function-like values */
    | '(' value_inner ')'                       /* grouped or tuple */
    | '[' value_list_opt ']'                    /* list value */
    | TRUE_KW | FALSE_KW | YES_KW | NO_KW
    ;

value_inner
    : expression                                /* '(' expr ')' — math grouping */
    | expression ',' expression                 /* 2-tuple, e.g. (10, 20) */
    | expression ',' expression ',' expression  /* 3-tuple */
    | expression ',' expression ',' expression ',' expression  /* 4-tuple (rgba …) */
    ;

value_list_opt
    : /* empty */
    | value_list
    ;

arg_list_opt
    : /* empty */
    | arg_list
    ;

arg_list
    : arg
    | arg_list ',' arg
    ;

arg
    : value_seq                                 /* allows 'red 0%' style stops */
    ;

/* ========================================================================
 * CONTENT BLOCK
 * ====================================================================== */

content_block
    : CONTENT_KW '{' content_body '}'
    | TEXT_KW    ':' STRING
    | TEXT_KW    ':' STRING style_block
    ;

content_body
    : text_content
    | text_content style_block
    ;

text_content
    : STRING
    | RICH_KW '{' rich_text_list '}'
    | EXPR_KW ':' expression
    ;

rich_text_list
    : /* empty */
    | rich_text_list rich_text_element
    ;

rich_text_element
    : STRING
    | STRING '[' property_list ']'              /* inline style */
    | BR_KW
    | '$'           expression           '$'    /* inline math */
    | DOLLAR_DOLLAR expression DOLLAR_DOLLAR    /* display math */
    ;

/* ========================================================================
 * BOOLEAN
 * ====================================================================== */

boolean
    : TRUE_KW | FALSE_KW | YES_KW | NO_KW | INTEGER     /* 0 / 1 also accepted */
    ;

/* ========================================================================
 * MATH EXPRESSION (simplified)
 *
 * A clean arithmetic chain. No implicit multiplication, no reserved math
 * function names — function calls are just IDENT '(' arg_list ')'.
 * ====================================================================== */

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
    : unary
    | unary '^' exponential
    ;

unary
    : postfix
    | '-' unary  %prec UMINUS
    | '+' unary  %prec UMINUS
    | '!' unary  %prec LNOT
    ;

postfix
    : primary
    | postfix '!'  %prec POSTFIX_BANG
    ;

primary
    : NUMBER
    | INTEGER
    | MEASUREMENT                               /* numeric literal with unit */
    | PERCENTAGE
    | ANGLE_DEG
    | STRING
    | IDENT
    | IDENT '(' arg_list_opt ')'                /* function call */
    | '(' expression ')'
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "flowchart.y: %s near '%s' on line %d\n",
            s, yytext, yylineno);
}

int main(void) {
    int rc = yyparse();
    if (rc == 0) printf("ok\n");
    return rc;
}
