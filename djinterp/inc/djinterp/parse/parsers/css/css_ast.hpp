/******************************************************************************
* djinterp [css]                                                  css_ast.hpp
*
* CSS abstract syntax tree:
*   This header defines the data-only AST nodes produced by the css parser
* and consumed by the css matching engine.  No methods, no logic — just
* structurally-typed records describing a parsed stylesheet.
*
*   The AST is host-agnostic.  Selectors carry no concept of "an HTML
* element"; they describe predicates that the matching engine evaluates
* against any opaque `style_target` view supplied by the host application.
*
* Contents:
*   - css_combinator             selector combinators (>, +, ~, descendant)
*   - css_attribute_op           attribute selector operators
*   - css_simple_kind            tag for simple-selector variants
*   - css_simple_selector        a single atomic selector predicate
*   - css_compound_selector      simple selectors fused with no whitespace
*   - css_complex_step           combinator + compound (a step in a chain)
*   - css_complex_selector       full chain with packed specificity
*   - css_selector_list          comma-separated selectors
*   - css_value_kind             tag for value-component variants
*   - css_value                  a single value token (recursive for fns)
*   - css_declaration            property : value pair, optional !important
*   - css_style_rule             selectors + declarations
*   - css_at_rule                @name prelude { ... }
*   - css_stylesheet             top-level container
*
*
* path:      /inc/cpp/css/css_ast.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_CSS_AST_
#define DJINTERP_CSS_AST_ 1

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "../core/djinterp.hpp"


// D_KEYWORD_CSS
//   keyword: resolves to `css`.
// Used to specify that a unit of code pertains to the css
// parsing / matching subsystem.
#define D_KEYWORD_CSS               css

// NS_CSS
//   namespace: the css subsystem namespace.
#define NS_CSS                      D_NAMESPACE(D_KEYWORD_CSS)


NS_DJINTERP
NS_CSS


// ================================================================
//  combinators  /  operators  /  simple-selector tags
// ================================================================

// DCssCombinator
//   enum: kinds of selector combinator joining two compounds.
enum DCssCombinator
{
    DCssCombinatorDescendant       = 0,    // ' '   (whitespace)
    DCssCombinatorChild            = 1,    // '>'
    DCssCombinatorAdjacentSibling  = 2,    // '+'
    DCssCombinatorGeneralSibling   = 3     // '~'
};

// DCssAttributeOp
//   enum: kinds of attribute-selector matcher.
enum DCssAttributeOp
{
    DCssAttributeOpPresence    = 0,     // [attr]
    DCssAttributeOpExact       = 1,     // [attr="v"]
    DCssAttributeOpWord        = 2,     // [attr~="v"]
    DCssAttributeOpLang        = 3,     // [attr|="v"]
    DCssAttributeOpPrefix      = 4,     // [attr^="v"]
    DCssAttributeOpSuffix      = 5,     // [attr$="v"]
    DCssAttributeOpSubstring   = 6      // [attr*="v"]
};

// DCssSimpleKind
//   enum: discriminator for css_simple_selector variants.
enum DCssSimpleKind
{
    DCssSimpleKindUniversal     = 0,    // *
    DCssSimpleKindType          = 1,    // foo
    DCssSimpleKindId            = 2,    // #foo
    DCssSimpleKindClass         = 3,    // .foo
    DCssSimpleKindAttribute     = 4,    // [...]
    DCssSimpleKindPseudoClass   = 5,    // :foo  /  :foo(...)
    DCssSimpleKindPseudoElement = 6,    // ::foo
    DCssSimpleKindNegation      = 7     // :not(compound)
};

// DCssValueKind
//   enum: discriminator for css_value variants.
enum DCssValueKind
{
    DCssValueKindIdent       = 0,    // foo, auto, inherit
    DCssValueKindNumber      = 1,    // 42  /  42.5px  /  -3em
    DCssValueKindPercentage  = 2,    // 50%
    DCssValueKindString      = 3,    // "hello"
    DCssValueKindHash        = 4,    // #fff  /  #1a2b3c  (typically a colour)
    DCssValueKindFunction    = 5,    // ident( args )
    DCssValueKindDelim       = 6     // '/' or ',' as a structural delimiter
};


// ================================================================
//  forward declarations
// ================================================================

struct css_compound_selector;
struct css_value;


// ================================================================
//  css_simple_selector
// ================================================================

// css_simple_selector
//   struct: a single atomic selector predicate (one of the variants
// enumerated by DCssSimpleKind).  All fields are present in the
// struct but only those relevant to `kind` are meaningful.
struct css_simple_selector
{
    DCssSimpleKind  kind;

    // shared:
    //   - type:           name = element type (e.g. "button", "*" stored
    //                     as universal kind instead of "*" string)
    //   - id:             name = identifier
    //   - class_:         name = class identifier
    //   - attribute:      name = attribute name
    //   - pseudo_class:   name = pseudo-class identifier
    //   - pseudo_element: name = pseudo-element identifier
    std::string     name;

    // attribute-selector data:
    DCssAttributeOp attribute_op;
    std::string     attribute_value;
    bool            attribute_case_insensitive;

    // pseudo-class with arg, e.g. :nth-child(2n+1) -> "2n+1".
    // Empty string for argument-less pseudo-classes.
    std::string     pseudo_arg;

    // for negation (:not(...)). owned via shared_ptr to keep the
    // struct value-semantic and avoid hand-rolled deep-copy code.
    std::shared_ptr<css_compound_selector>      negated;

    css_simple_selector()
        : kind                        (DCssSimpleKindUniversal)
        , name                        ()
        , attribute_op                (DCssAttributeOpPresence)
        , attribute_value             ()
        , attribute_case_insensitive  (false)
        , pseudo_arg                  ()
        , negated                     ()
    {}
};


// ================================================================
//  css_compound_selector
// ================================================================

// css_compound_selector
//   struct: a sequence of simple selectors with no whitespace
// between them, e.g. `button.primary[disabled]:hover`.  At most
// one of the simples should be a type/universal selector (and
// it must come first if present); the parser enforces this.
struct css_compound_selector
{
    std::vector<css_simple_selector>    simples;

    css_compound_selector()
        : simples()
    {}
};


// ================================================================
//  css_complex_step  /  css_complex_selector
// ================================================================

// css_complex_step
//   struct: one step in a complex selector — a combinator paired
// with the compound that follows it.
struct css_complex_step
{
    DCssCombinator          combinator;
    css_compound_selector   compound;

    css_complex_step()
        : combinator (DCssCombinatorDescendant)
        , compound   ()
    {}
};

// css_complex_selector
//   struct: a chain of compound selectors joined by combinators,
// e.g. `nav > ul.main li:hover`.  The head compound has no
// preceding combinator; subsequent compounds are stored in
// `tail`, each carrying the combinator that introduced it.
//
//   `specificity` is computed once at parse time and packed into
// a uint32_t as 0x00aabbcc, where:
//   - aa: number of id selectors            (high byte)
//   - bb: number of class/attr/pseudo-class (mid byte)
//   - cc: number of type/pseudo-element     (low byte)
struct css_complex_selector
{
    css_compound_selector           head;
    std::vector<css_complex_step>   tail;
    std::uint32_t                   specificity;

    css_complex_selector()
        : head        ()
        , tail        ()
        , specificity (0u)
    {}
};


// ================================================================
//  css_selector_list
// ================================================================

// css_selector_list
//   struct: a comma-separated list of complex selectors.  Each
// selector is matched independently; a target matches the list
// if it matches any one of them.  Specificity is per-selector.
struct css_selector_list
{
    std::vector<css_complex_selector>   selectors;

    css_selector_list()
        : selectors()
    {}
};


// ================================================================
//  css_value
// ================================================================

// css_value
//   struct: a single value-list component.  Recursive: function
// calls hold their argument tokens as a vector of css_value.
//
//   Field meaning by `kind`:
//   - ident:       text holds the identifier
//   - number:      number holds the numeric value, unit holds the
//                  optional dimension (e.g. "px", "em", "")
//   - percentage:  number holds the percentage value
//   - string:      text holds the unquoted, unescaped contents
//   - hash:        text holds the hash body (e.g. "fff", "1a2b3c")
//   - function:    text holds the function name; args holds the
//                  argument value list
//   - delim:       text holds the delimiter character as a string
//                  of length 1 (e.g. "/", ",")
struct css_value
{
    DCssValueKind           kind;
    std::string             text;
    double                  number;
    std::string             unit;
    std::vector<css_value>  args;

    css_value()
        : kind   (DCssValueKindIdent)
        , text   ()
        , number (0.0)
        , unit   ()
        , args   ()
    {}
};

// css_value_list
//   type: alias for a sequence of value components forming the
// right-hand side of a declaration.
typedef std::vector<css_value>      css_value_list;


// ================================================================
//  css_declaration
// ================================================================

// css_declaration
//   struct: a single property: value pair, with the !important
// flag captured separately rather than left in the value list.
struct css_declaration
{
    std::string         property;
    css_value_list      value;
    bool                important;

    css_declaration()
        : property  ()
        , value     ()
        , important (false)
    {}
};


// ================================================================
//  css_style_rule
// ================================================================

// css_style_rule
//   struct: a top-level qualified rule — one or more selectors
// followed by a declaration block.  The vast majority of a real
// stylesheet consists of these.
struct css_style_rule
{
    css_selector_list               selectors;
    std::vector<css_declaration>    declarations;

    css_style_rule()
        : selectors    ()
        , declarations ()
    {}
};


// ================================================================
//  css_at_rule
// ================================================================

// css_at_rule
//   struct: an at-rule, e.g. `@media (min-width: 600px) { ... }`
// or `@import "x.css";`.  The body is captured structurally so
// that hosts can interpret each at-rule type as they see fit:
//
//   - has_block == false:        a statement-style at-rule with no
//                                body (e.g. @import, @charset).
//   - rules        non-empty:    a rule-list block (e.g. @media).
//   - declarations non-empty:    a declaration block (e.g.
//                                @font-face, @page).
//
//   The two body vectors are mutually exclusive in well-formed
// CSS, but the parser does not enforce a particular shape — it
// fills whichever one corresponds to the parsed content.
struct css_at_rule
{
    std::string                     name;
    css_value_list                  prelude;
    bool                            has_block;
    std::vector<css_style_rule>     rules;
    std::vector<css_declaration>    declarations;

    css_at_rule()
        : name         ()
        , prelude      ()
        , has_block    (false)
        , rules        ()
        , declarations ()
    {}
};


// ================================================================
//  css_stylesheet
// ================================================================

// css_stylesheet
//   struct: the top-level result of parsing a .css source.  Holds
// both bare style rules and at-rules in the order they appeared.
//
//   `order` parallels `rules` and `at_rules` and records each
// rule's source-order position; the cascade uses it as the
// final tie-breaker after specificity.  For convenience callers
// who do not need it can ignore the field.
struct css_stylesheet
{
    std::vector<css_style_rule>     rules;
    std::vector<css_at_rule>        at_rules;

    css_stylesheet()
        : rules    ()
        , at_rules ()
    {}
};


NS_END  // css
NS_END  // djinterp


#endif  // DJINTERP_CSS_AST_
