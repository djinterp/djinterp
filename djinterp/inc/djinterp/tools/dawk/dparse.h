/******************************************************************************
* djinterp [dawk]                                                     dparse.h
*
*   Syntax tree and recursive-descent parser for POSIX awk.
*     One node type carries every construct. Fixed children live in `a` to
* `d`; variadic ones live in `list`. That is less type-safe than a node per
* construct and very much shorter, which matters more while the shape of the
* interpreter is still settling.
*
*
* path:      /inc/djinterp/tools/dawk/dparse.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Nodes
         1.  d_awk_node_kind
         2.  d_awk_node
    2.  Program
         1.  d_awk_rule_kind
         2.  d_awk_rule
         3.  d_awk_function
         4.  d_awk_program
2.  OPERATIONS
    ----------
    1.  Parsing
    2.  Diagnostics
*/

#ifndef DJINTERP_TOOLS_DAWK_DPARSE_H
#define DJINTERP_TOOLS_DAWK_DPARSE_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "./dlex.h"   // d_awk_token_kind, used as the operator tag


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Nodes
//------------------------------------------------------------------------------
// 1.1.1
// d_awk_node_kind
//   enum: the constructs of the language.  Expressions and statements share
//   one enumeration because awk lets an expression stand as a statement.
enum d_awk_node_kind
{
    // expressions
    D_AWK_N_NUMBER = 0,
    D_AWK_N_STRING,
    D_AWK_N_REGEX,        // bare /re/, meaning a match against $0
    D_AWK_N_VAR,          // text
    D_AWK_N_FIELD,        // a
    D_AWK_N_INDEX,        // a = array name node, list = subscript parts
    D_AWK_N_ASSIGN,       // op, a = target, b = value
    D_AWK_N_TERNARY,      // a ? b : c
    D_AWK_N_BINARY,       // op, a, b
    D_AWK_N_UNARY,        // op, a
    D_AWK_N_CONCAT,       // a, b
    D_AWK_N_MATCH,        // op is MATCH or NOMATCH; a, b
    D_AWK_N_IN,           // list = subscript parts, b = array name node
    D_AWK_N_GROUPLIST,    // list = elements of a parenthesised comma list
    D_AWK_N_PREINCR,      // op is INCR or DECR; a
    D_AWK_N_POSTINCR,     // op is INCR or DECR; a
    D_AWK_N_CALL,         // text = name, list = arguments
    D_AWK_N_BUILTIN,      // text = name, list = arguments
    D_AWK_N_GETLINE,      // reserved for step 4
    // statements
    D_AWK_N_BLOCK,        // list
    D_AWK_N_EXPR_STMT,    // a
    D_AWK_N_PRINT,        // list = arguments, op = redirection, b = target
    D_AWK_N_PRINTF,       // list = arguments, op = redirection, b = target
    D_AWK_N_IF,           // a = condition, b = then, c = else
    D_AWK_N_WHILE,        // a = condition, b = body
    D_AWK_N_DO,           // a = body, b = condition
    D_AWK_N_FOR,          // a = init, b = condition, c = step, d = body
    D_AWK_N_FORIN,        // a = variable, b = array name node, d = body
    D_AWK_N_NEXT,
    D_AWK_N_NEXTFILE,
    D_AWK_N_EXIT,         // a, optional
    D_AWK_N_RETURN,       // a, optional
    D_AWK_N_BREAK,
    D_AWK_N_CONTINUE,
    D_AWK_N_DELETE,       // a = array name node, list = subscript parts
    D_AWK_N_DELETE_ALL    // a = array name node
};

// 1.1.2
// d_awk_node
//   struct: one syntax node.  `op` holds a token kind for the operator nodes;
//   `regex` holds the compiled pattern for a REGEX node so compilation happens
//   once rather than per record.
struct d_awk_node
{
    enum d_awk_node_kind kind;
    enum d_awk_token_kind op;
    double                number;
    char*                 text;
    size_t                length;
    struct d_regex*       regex;
    struct d_awk_node*    a;
    struct d_awk_node*    b;
    struct d_awk_node*    c;
    struct d_awk_node*    d;
    struct d_awk_node**   list;
    size_t                count;
    size_t                capacity;
    size_t                line;
};

// 1.2    Program
//------------------------------------------------------------------------------
// 1.2.1
// d_awk_rule_kind
//   enum: which part of the main loop a rule belongs to.
enum d_awk_rule_kind
{
    D_AWK_RULE_BEGIN = 0,
    D_AWK_RULE_END,
    D_AWK_RULE_MAIN
};

// 1.2.2
// d_awk_rule
//   struct: one pattern and its action.  A NULL pattern matches every record;
//   a non-NULL `pattern_end` makes it a range, whose state lives in `active`.
struct d_awk_rule
{
    enum d_awk_rule_kind kind;
    struct d_awk_node*   pattern;
    struct d_awk_node*   pattern_end;
    struct d_awk_node*   action;
    bool                 active;
};

// 1.2.3
// d_awk_function
//   struct: one user-defined function.  Parameters past those supplied at the
//   call are awk's way of declaring locals.
struct d_awk_function
{
    char*              name;
    char**             params;
    size_t             param_count;
    struct d_awk_node* body;
};

// 1.2.4
// d_awk_program
//   struct: a parsed program.
struct d_awk_program
{
    struct d_awk_rule*     rules;
    size_t                 rule_count;
    size_t                 rule_capacity;
    struct d_awk_function* functions;
    size_t                 function_count;
    size_t                 function_capacity;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Parsing
//------------------------------------------------------------------------------
struct d_awk_program* d_awk_parse(const char* _source,
                                  size_t      _length,
                                  const char* _origin);
void                  d_awk_program_free(struct d_awk_program* _program);

// 2.2    Diagnostics
//------------------------------------------------------------------------------
const char*           d_awk_parse_error(void);


#endif  // DJINTERP_TOOLS_DAWK_DPARSE_H
