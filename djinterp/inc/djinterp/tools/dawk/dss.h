/*******************************************************************************
* djinterp [dawk]                                                          dss.h
*
* DSS front end:
*   Parses the declarative subset of djinterp-dss.peg -- at-rules, style rules,
* selectors and declarations -- into an index-addressed tree.  Statements,
* expressions and calc() are not accepted here; a sheet that needs them needs
* the awk side of the grammar and is rejected with a located error.
*   Every node is a fixed-size record in a flat array and every reference is a
* uint32_t index, so the whole sheet is three allocations and may be walked
* without chasing pointers.  Identifiers are interned, which makes selector
* matching an integer compare rather than a strcmp.
*   There is no cascade and no specificity: rules are held in source order and
* the first match wins, as the grammar's committed ordered choice requires.
*
* path:      /inc/djinterp/tools/dawk/dss.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Constants
         1.  D_DSS_NO_INDEX
    2.  Enumerations
         1.  d_dss_simple_kind
         2.  d_dss_combinator
         3.  d_dss_attr_op
         4.  d_dss_value_kind
    3.  Records
         1.  d_dss_simple
         2.  d_dss_compound
         3.  d_dss_step
         4.  d_dss_selector
         5.  d_dss_value
         6.  d_dss_declaration
         7.  d_dss_rule
         8.  d_dss_at_rule
         9.  d_dss_error
    4.  Opaque types
         1.  d_dss_sheet
2.  OPERATIONS
    ----------
    1.  Parsing
    2.  Inspection
    3.  Text
*/

#ifndef DJINTERP_TOOLS_DAWK_DSS_H
#define DJINTERP_TOOLS_DAWK_DSS_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint32_t, uint8_t


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_DSS_NO_INDEX
//   constant: the index reported for an absent reference.  Zero is a valid
// index into every array, so absence needs a value of its own.
#define D_DSS_NO_INDEX ((uint32_t)-1)

// 1.2    Enumerations
//------------------------------------------------------------------------------
// 1.2.1
// d_dss_simple_kind
//   enum: which variant of simple selector a record holds.
enum d_dss_simple_kind
{
    D_DSS_SIMPLE_UNIVERSAL = 0,   // *
    D_DSS_SIMPLE_TYPE,            // banner
    D_DSS_SIMPLE_CLASS,           // .open
    D_DSS_SIMPLE_ATTRIBUTE,       // [name=created]
    D_DSS_SIMPLE_PSEUDO_CLASS,    // :first-child
    D_DSS_SIMPLE_PSEUDO_ELEMENT   // ::before
};

// 1.2.2
// d_dss_combinator
//   enum: how a step joins to the compound before it.  The general sibling
// combinator is spelled `...` rather than `~`, which awk keeps for regex
// match.
enum d_dss_combinator
{
    D_DSS_COMBINATOR_DESCENDANT = 0,  // whitespace
    D_DSS_COMBINATOR_CHILD,           // >
    D_DSS_COMBINATOR_ADJACENT,        // +
    D_DSS_COMBINATOR_SIBLING          // ...
};

// 1.2.3
// d_dss_attr_op
//   enum: the comparison an attribute selector performs.  The numeric four
// are the ones CSS lacks and the guide's rules need, e.g. [size>=4k].
enum d_dss_attr_op
{
    D_DSS_ATTR_PRESENCE = 0,  // [attr]
    D_DSS_ATTR_EQUAL,         // [attr=v]  /  [attr==v]
    D_DSS_ATTR_NOT_EQUAL,     // [attr!=v]
    D_DSS_ATTR_PREFIX,        // [attr^=v]
    D_DSS_ATTR_SUFFIX,        // [attr$=v]
    D_DSS_ATTR_SUBSTRING,     // [attr*=v]
    D_DSS_ATTR_WORD,          // [attr~=v]
    D_DSS_ATTR_LANG,          // [attr|=v]
    D_DSS_ATTR_LESS,          // [attr<v]
    D_DSS_ATTR_LESS_EQUAL,    // [attr<=v]
    D_DSS_ATTR_GREATER,       // [attr>v]
    D_DSS_ATTR_GREATER_EQUAL  // [attr>=v]
};

// 1.2.4
// d_dss_value_kind
//   enum: which variant of declaration value a record holds.
enum d_dss_value_kind
{
    D_DSS_VALUE_IDENT = 0,  // one-per-line
    D_DSS_VALUE_STRING,     // "TBA"
    D_DSS_VALUE_NUMBER,     // 80  /  4k
    D_DSS_VALUE_FUNCTION,   // date("YYYY.MM.DD")
    D_DSS_VALUE_COMMA,      // a separator inside a value list
    D_DSS_VALUE_FLAG        // !fixable
};

// 1.3    Records
//------------------------------------------------------------------------------
// 1.3.1
// d_dss_simple
//   struct: one atomic selector predicate.  Only the fields relevant to
// `kind` carry meaning; the rest hold their defaults.
struct d_dss_simple
{
    uint8_t   kind;        // d_dss_simple_kind
    uint8_t   op;          // d_dss_attr_op, for attributes
    bool      numeric;     // the attribute value parsed as a number
    uint32_t  name;        // interned: type, class, attribute or pseudo name
    uint32_t  value;       // interned: the attribute value, or NO_INDEX
    double    number;      // the attribute value as a number, when numeric
};

// 1.3.2
// d_dss_compound
//   struct: simple selectors fused with no whitespace between them.
struct d_dss_compound
{
    uint32_t  first_simple;
    uint32_t  simple_count;
};

// 1.3.3
// d_dss_step
//   struct: a combinator and the compound it introduces.
struct d_dss_step
{
    uint8_t   combinator;  // d_dss_combinator
    uint32_t  compound;
};

// 1.3.4
// d_dss_selector
//   struct: a chain of compounds.  The head has no preceding combinator.
struct d_dss_selector
{
    uint32_t  head;
    uint32_t  first_step;
    uint32_t  step_count;
};

// 1.3.5
// d_dss_value
//   struct: one term of a declaration value.  Function arguments are a
// contiguous run of further values, so the record stays fixed-size.
struct d_dss_value
{
    uint8_t   kind;        // d_dss_value_kind
    uint32_t  text;        // interned: ident, string body or function name
    double    number;      // the numeric value, for numbers
    uint32_t  first_arg;
    uint32_t  arg_count;
};

// 1.3.6
// d_dss_declaration
//   struct: a property and its value terms.  The line is kept so that a
// report can name the sheet position that produced it.
struct d_dss_declaration
{
    uint32_t  property;    // interned
    uint32_t  first_value;
    uint32_t  value_count;
    uint32_t  line;
};

// 1.3.7
// d_dss_rule
//   struct: a selector list and the declarations it carries.
struct d_dss_rule
{
    uint32_t  first_selector;
    uint32_t  selector_count;
    uint32_t  first_declaration;
    uint32_t  declaration_count;
    uint32_t  line;
};

// 1.3.8
// d_dss_at_rule
//   struct: an at-rule.  A block body is parsed as declarations, which is
// what @table rows are; a prelude is kept as interned text for the host to
// interpret.
struct d_dss_at_rule
{
    uint32_t  name;        // interned, without the leading @
    uint32_t  prelude;     // interned, or NO_INDEX
    bool      has_block;
    uint32_t  first_declaration;
    uint32_t  declaration_count;
    uint32_t  line;
};

// 1.3.9
// d_dss_error
//   struct: where parsing stopped and why.  A zero line means no error.
struct d_dss_error
{
    uint32_t     line;
    uint32_t     column;
    const char*  message;
};

// 1.4    Opaque types
//------------------------------------------------------------------------------
// 1.4.1
// d_dss_sheet
//   struct: a parsed stylesheet.  Owns its arrays and its intern table.
struct d_dss_sheet;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Parsing
//------------------------------------------------------------------------------
struct d_dss_sheet* d_dss_parse(const char*         _text,
                                size_t              _length,
                                struct d_dss_error* _out_error);
void                d_dss_free(struct d_dss_sheet* _sheet);

// 2.2    Inspection
//------------------------------------------------------------------------------
size_t                        d_dss_rule_count(const struct d_dss_sheet* _sheet);
const struct d_dss_rule*      d_dss_rule_at(const struct d_dss_sheet* _sheet,
                                            size_t                    _at);
size_t                        d_dss_at_rule_count(
                                  const struct d_dss_sheet* _sheet);
const struct d_dss_at_rule*   d_dss_at_rule_at(const struct d_dss_sheet* _sheet,
                                               size_t                    _at);
const struct d_dss_selector*  d_dss_selector_at(const struct d_dss_sheet* _sheet,
                                                uint32_t                  _at);
const struct d_dss_compound*  d_dss_compound_at(const struct d_dss_sheet* _sheet,
                                                uint32_t                  _at);
const struct d_dss_step*      d_dss_step_at(const struct d_dss_sheet* _sheet,
                                            uint32_t                  _at);
const struct d_dss_simple*    d_dss_simple_at(const struct d_dss_sheet* _sheet,
                                              uint32_t                  _at);
const struct d_dss_declaration*
                              d_dss_declaration_at(
                                  const struct d_dss_sheet* _sheet,
                                  uint32_t                  _at);
const struct d_dss_value*     d_dss_value_at(const struct d_dss_sheet* _sheet,
                                             uint32_t                  _at);

// 2.3    Text
//------------------------------------------------------------------------------
const char*         d_dss_text(const struct d_dss_sheet* _sheet,
                               uint32_t                  _id);


#endif  // DJINTERP_TOOLS_DAWK_DSS_H
