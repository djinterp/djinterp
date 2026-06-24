/******************************************************************************
* djinterp [sass]                                                     sass.hpp
*
*   Foundational Sass / SCSS module for the djinterp framework.
* Layered on top of the CSS module: a Sass stylesheet IS a CSS
* stylesheet plus the Sass-language extensions (variables, mixins,
* includes, extension, control flow, modules, interpolation,
* placeholder selectors, parent reference). The Sass facades extend
* the CSS facades so that storage and memory layout are unchanged
* below the extension surface; Sass-specific behaviour is added via
* methods and free functions only -- never via additional state on
* the inherited CSS shape.
*
*   COVERS BOTH DIALECTS:
*   - SCSS (curly-brace, semicolon-terminated)
*   - Sass (indented, newline-terminated)
*   Both share the same AST. The `sass_syntax` enum on stylesheets
* drives emission style; parsing-side adapter backends pick it up
* automatically from file extension (.scss vs .sass).
*
*   ZERO OVERHEAD:
*   - The Sass rule-kind enum is `std::uint8_t`-backed.
*   - Category and classification predicates are `D_CONSTEXPR`.
*   - Variable / mixin / function name constants live as
*     `D_STATIC_CONSTEXPR const char*` literal pointers in
*     `built_ins::` (no allocation, no dynamic init).
*   - The Sass facades inherit from CSS facades and add NO data
*     members of their own.
*
*   LIBRARY AGNOSTICISM:
*   This header pulls in the CSS module and nothing else beyond the
* standard library. Adapter backends for libsass / dart-sass IPC /
* sass-embedded / etc. are detected structurally via the trait
* layer.
*
*
* path:      /inc/djinterp/core/util/sass/sass.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SHARED TYPES & CONSTANTS
II.   SASS SYNTAX & DIALECT ENUMS
III.  SASS RULE KIND ENUM
IV.   SASS AT-RULE KIND ENUM
V.    SASS VALUE / EXPRESSION ENUMS
VI.   RULE CATEGORY CLASSIFICATION
VII.  BUILT-IN MODULE / FUNCTION NAMESPACES
VIII. KIND <-> NAME MAPPING
IX.   BACKEND TAG DISPATCH
X.    BACKEND DETECTION
XI.   FOLDED TRAITS & CONCEPTS
*/

#ifndef DJINTERP_SASS_
#define DJINTERP_SASS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <ostream>
// djinterp
#include "../../djinterp.hpp"
#include "../css/css.hpp"


///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED TYPES & CONSTANTS                            ///
///////////////////////////////////////////////////////////////////////////////

NS_DJINTERP

// NOTE: djinterp.hpp does not (yet) define NS_SASS. Using a plain
// namespace declaration here matching the html / markdown / css
// pattern; add `#define NS_SASS D_NAMESPACE(sass)` alongside the
// other NS_* macros if the macro idiom is preferred.
namespace sass {


// sass_string_t
//   type: alias for the string type used throughout the sass
// module. Same as `css::css_string_t` but re-exported for
// convenience.
using sass_string_t = ::djinterp::css::css_string_t;


// D_SASS_VARIABLE_PREFIX
//   constant: prefix character for Sass variables ("$").
#ifndef D_SASS_VARIABLE_PREFIX
    #define D_SASS_VARIABLE_PREFIX      "$"
#endif


// D_SASS_PLACEHOLDER_PREFIX
//   constant: prefix character for Sass placeholder selectors
// ("%name" -- not emitted in compiled output, only used as
// extension targets).
#ifndef D_SASS_PLACEHOLDER_PREFIX
    #define D_SASS_PLACEHOLDER_PREFIX   "%"
#endif


// D_SASS_INTERPOLATION_OPEN / CLOSE
//   constant: delimiters for Sass interpolation ("#{...}").
#ifndef D_SASS_INTERPOLATION_OPEN
    #define D_SASS_INTERPOLATION_OPEN   "#{"
#endif
#ifndef D_SASS_INTERPOLATION_CLOSE
    #define D_SASS_INTERPOLATION_CLOSE  "}"
#endif


///////////////////////////////////////////////////////////////////////////////
///                II.   SASS SYNTAX & DIALECT ENUMS                        ///
///////////////////////////////////////////////////////////////////////////////

// sass_syntax
//   enum: identifies which surface syntax the stylesheet uses.
enum class sass_syntax : std::uint8_t
{
    scss,           // curly-brace, semicolon-terminated (.scss)
    indented,       // indentation-significant (.sass)
    auto_detect,    // detect from file extension or contents
    unspecified
};


// sass_dialect
//   enum: identifies the implementation / spec target. Drives
// which features (modern modules, calc-functions inside
// expressions, color-function math) are accepted.
enum class sass_dialect : std::uint8_t
{
    libsass_3,      // libsass 3.x (legacy import system)
    dart_sass,      // dart-sass (modern module system,
                    //   @use / @forward, sass:* built-in modules)
    sass_embedded,  // sass-embedded (host-controlled dart-sass)
    custom,
    unspecified
};


///////////////////////////////////////////////////////////////////////////////
///                III.   SASS RULE KIND ENUM                               ///
///////////////////////////////////////////////////////////////////////////////

// sass_rule_kind
//   enum: discriminator for Sass-specific rule kinds. The
// standard CSS rule kinds remain reachable through the
// inherited CSS layer (`css::css_rule_kind`); this enum
// captures the additions Sass introduces. Values that shadow
// CSS kinds use the `sass_` prefix to avoid confusion at the
// switch site.
enum class sass_rule_kind : std::uint8_t
{
    // declaration of a Sass entity
    variable_declaration,       // $name: value;
    mixin_declaration,          // @mixin name(args) { ... }
    function_declaration,       // @function name(args) { ... }
    return_statement,           // @return expr;

    // invocation
    include_statement,          // @include name(args);
    extend_statement,           // @extend selector;
    debug_statement,            // @debug expr;
    warn_statement,             // @warn expr;
    error_statement,            // @error expr;

    // control flow
    if_statement,               // @if condition { ... }
    else_statement,             // @else { ... } / @else if ...
    each_statement,             // @each $x in list { ... }
    for_statement,              // @for $i from a through/to b { ... }
    while_statement,            // @while condition { ... }

    // module system (dart-sass)
    use_rule,                   // @use 'module' (with overrides)
    forward_rule,               // @forward 'module'
    at_root_rule,               // @at-root selector { ... }

    // legacy / dialect
    import_rule_legacy,         // @import 'partial' (libsass-style)

    // placeholder selectors are style rules whose selector
    // begins with `%`; not a separate rule kind, but exposed
    // here for the trait layer.
    placeholder_rule,

    // sentinel
    unknown = 255
};


///////////////////////////////////////////////////////////////////////////////
///                IV.   SASS AT-RULE KIND ENUM                             ///
///////////////////////////////////////////////////////////////////////////////

// sass_at_rule_kind
//   enum: refines `css::css_at_rule_kind` with Sass-specific
// at-keywords. Backends that store all at-rules under a single
// discriminator can use this to further classify.
enum class sass_at_rule_kind : std::uint8_t
{
    mixin,
    include,
    function,
    return_,                    // trailing _ (C++ keyword)
    extend,
    debug,
    warn,
    error,
    if_,
    else_,
    each,
    for_,
    while_,
    use,
    forward,
    at_root,
    import_legacy,              // @import in libsass-classic
    custom,
    unknown = 255
};


///////////////////////////////////////////////////////////////////////////////
///                V.   SASS VALUE / EXPRESSION ENUMS                       ///
///////////////////////////////////////////////////////////////////////////////

// sass_value_kind
//   enum: extends CSS's value kinds with Sass-specific value
// types. Sass values include all CSS values plus typed values
// the Sass evaluator produces (booleans, lists with separator
// type, maps).
enum class sass_value_kind : std::uint8_t
{
    null_,                      // null literal
    boolean,                    // true / false
    number,                     // number with optional unit
    string_,                    // quoted or unquoted string
    color,                      // a color value
    list,                       // (a, b, c) or a b c
    map,                        // (k1: v1, k2: v2)
    function_ref,               // get-function('name')
    calculation,                // calc() expression as Sass value
    arglist,                    // ...$args
    interpolation,              // #{...} fragment
    custom,
    unknown = 255
};


// sass_list_separator
//   enum: identifies the separator style of a Sass list.
enum class sass_list_separator : std::uint8_t
{
    comma,                      // a, b, c
    space,                      // a b c
    slash,                      // a / b / c (CSS Values 4)
    bracketed_comma,            // [a, b, c]
    bracketed_space,            // [a b c]
    none                        // single-element list
};


///////////////////////////////////////////////////////////////////////////////
///                VI.   RULE CATEGORY CLASSIFICATION                       ///
///////////////////////////////////////////////////////////////////////////////

// sass_rule_category
//   enum: bitmask describing the categorical properties of a
// Sass rule kind. Backed by `unsigned`. Distinct from CSS
// categories because Sass rules are evaluated, not just
// emitted.
enum sass_rule_category : unsigned
{
    src_none           = 0u,
    src_declaration    = 1u <<  0,  // declares a variable / mixin / fn
    src_invocation     = 1u <<  1,  // calls a mixin / inserts an include
    src_control_flow   = 1u <<  2,  // @if / @each / @for / @while
    src_module         = 1u <<  3,  // @use / @forward / @import
    src_diagnostic     = 1u <<  4,  // @debug / @warn / @error
    src_extension      = 1u <<  5,  // @extend / placeholder consumer
    src_placeholder    = 1u <<  6,  // %name selector
    src_grouping       = 1u <<  7,  // hosts nested rules / statements
    src_returns_value  = 1u <<  8,  // @return inside @function
    src_at_root        = 1u <<  9   // @at-root: escapes nesting
};


// sass_category_for_rule_kind
//   function: returns the category bitmask for a Sass rule
// kind.
D_CONSTEXPR_INLINE unsigned
sass_category_for_rule_kind(
    sass_rule_kind  _k
)
{
    using K = sass_rule_kind;
    return (_k == K::variable_declaration) ? (src_declaration)
         : (_k == K::mixin_declaration)    ? (src_declaration | src_grouping)
         : (_k == K::function_declaration) ? (src_declaration | src_grouping)
         : (_k == K::return_statement)     ? (src_returns_value)

         : (_k == K::include_statement)    ? (src_invocation)
         : (_k == K::extend_statement)     ? (src_extension)
         : (_k == K::debug_statement)      ? (src_diagnostic)
         : (_k == K::warn_statement)       ? (src_diagnostic)
         : (_k == K::error_statement)      ? (src_diagnostic)

         : (_k == K::if_statement)         ? (src_control_flow | src_grouping)
         : (_k == K::else_statement)       ? (src_control_flow | src_grouping)
         : (_k == K::each_statement)       ? (src_control_flow | src_grouping)
         : (_k == K::for_statement)        ? (src_control_flow | src_grouping)
         : (_k == K::while_statement)      ? (src_control_flow | src_grouping)

         : (_k == K::use_rule)             ? (src_module)
         : (_k == K::forward_rule)         ? (src_module)
         : (_k == K::at_root_rule)         ? (src_grouping | src_at_root)
         : (_k == K::import_rule_legacy)   ? (src_module)

         : (_k == K::placeholder_rule)     ? (src_placeholder | src_grouping)

         :    src_none;
}


// is_sass_declaration_kind
//   function: true if the rule declares a Sass entity
// (variable, mixin, function).
D_CONSTEXPR_INLINE bool
is_sass_declaration_kind(
    sass_rule_kind  _k
)
{
    return ((sass_category_for_rule_kind(_k) & src_declaration) != 0u);
}


// is_sass_control_flow_kind
//   function: true for @if / @else / @each / @for / @while.
D_CONSTEXPR_INLINE bool
is_sass_control_flow_kind(
    sass_rule_kind  _k
)
{
    return ((sass_category_for_rule_kind(_k) & src_control_flow) != 0u);
}


// is_sass_module_kind
//   function: true for @use / @forward / @import.
D_CONSTEXPR_INLINE bool
is_sass_module_kind(
    sass_rule_kind  _k
)
{
    return ((sass_category_for_rule_kind(_k) & src_module) != 0u);
}


// is_sass_diagnostic_kind
//   function: true for @debug / @warn / @error.
D_CONSTEXPR_INLINE bool
is_sass_diagnostic_kind(
    sass_rule_kind  _k
)
{
    return ((sass_category_for_rule_kind(_k) & src_diagnostic) != 0u);
}


// is_sass_grouping_kind
//   function: true if the rule hosts nested rules / statements.
D_CONSTEXPR_INLINE bool
is_sass_grouping_kind(
    sass_rule_kind  _k
)
{
    return ((sass_category_for_rule_kind(_k) & src_grouping) != 0u);
}


///////////////////////////////////////////////////////////////////////////////
///                VII.   BUILT-IN MODULE / FUNCTION NAMESPACES             ///
///////////////////////////////////////////////////////////////////////////////

// modules
//   namespace: `D_STATIC_CONSTEXPR const char*` constants for
// the built-in dart-sass modules accessible via `@use`.
namespace modules {

    D_STATIC_CONSTEXPR const char* color    = "sass:color";
    D_STATIC_CONSTEXPR const char* list     = "sass:list";
    D_STATIC_CONSTEXPR const char* map      = "sass:map";
    D_STATIC_CONSTEXPR const char* math     = "sass:math";
    D_STATIC_CONSTEXPR const char* meta     = "sass:meta";
    D_STATIC_CONSTEXPR const char* selector = "sass:selector";
    D_STATIC_CONSTEXPR const char* string   = "sass:string";

}   // namespace modules


// fns
//   namespace: name constants for commonly-used built-in
// functions. Names are unqualified; modern (dart-sass) code
// prefers the namespaced form (`color.adjust(...)` vs
// `adjust-color(...)`), but the unqualified names remain
// reachable when the module is imported with `@use ... as *`.
namespace fns {

    // color
    D_STATIC_CONSTEXPR const char* rgb              = "rgb";
    D_STATIC_CONSTEXPR const char* rgba             = "rgba";
    D_STATIC_CONSTEXPR const char* hsl              = "hsl";
    D_STATIC_CONSTEXPR const char* hsla             = "hsla";
    D_STATIC_CONSTEXPR const char* mix              = "mix";
    D_STATIC_CONSTEXPR const char* lighten          = "lighten";
    D_STATIC_CONSTEXPR const char* darken           = "darken";
    D_STATIC_CONSTEXPR const char* saturate         = "saturate";
    D_STATIC_CONSTEXPR const char* desaturate       = "desaturate";
    D_STATIC_CONSTEXPR const char* adjust_hue       = "adjust-hue";
    D_STATIC_CONSTEXPR const char* invert           = "invert";
    D_STATIC_CONSTEXPR const char* grayscale        = "grayscale";
    D_STATIC_CONSTEXPR const char* alpha            = "alpha";
    D_STATIC_CONSTEXPR const char* opacify          = "opacify";
    D_STATIC_CONSTEXPR const char* transparentize   = "transparentize";
    D_STATIC_CONSTEXPR const char* fade_in          = "fade-in";
    D_STATIC_CONSTEXPR const char* fade_out         = "fade-out";
    D_STATIC_CONSTEXPR const char* color_adjust     = "adjust-color";
    D_STATIC_CONSTEXPR const char* color_change     = "change-color";
    D_STATIC_CONSTEXPR const char* color_scale      = "scale-color";

    // string
    D_STATIC_CONSTEXPR const char* unquote          = "unquote";
    D_STATIC_CONSTEXPR const char* quote            = "quote";
    D_STATIC_CONSTEXPR const char* str_length       = "str-length";
    D_STATIC_CONSTEXPR const char* str_insert       = "str-insert";
    D_STATIC_CONSTEXPR const char* str_index        = "str-index";
    D_STATIC_CONSTEXPR const char* str_slice        = "str-slice";
    D_STATIC_CONSTEXPR const char* to_upper_case    = "to-upper-case";
    D_STATIC_CONSTEXPR const char* to_lower_case    = "to-lower-case";

    // math
    D_STATIC_CONSTEXPR const char* percentage       = "percentage";
    D_STATIC_CONSTEXPR const char* round            = "round";
    D_STATIC_CONSTEXPR const char* ceil             = "ceil";
    D_STATIC_CONSTEXPR const char* floor            = "floor";
    D_STATIC_CONSTEXPR const char* abs              = "abs";
    D_STATIC_CONSTEXPR const char* min              = "min";
    D_STATIC_CONSTEXPR const char* max              = "max";
    D_STATIC_CONSTEXPR const char* random           = "random";

    // list
    D_STATIC_CONSTEXPR const char* length           = "length";
    D_STATIC_CONSTEXPR const char* nth              = "nth";
    D_STATIC_CONSTEXPR const char* set_nth          = "set-nth";
    D_STATIC_CONSTEXPR const char* join             = "join";
    D_STATIC_CONSTEXPR const char* append           = "append";
    D_STATIC_CONSTEXPR const char* zip              = "zip";
    D_STATIC_CONSTEXPR const char* index            = "index";
    D_STATIC_CONSTEXPR const char* list_separator   = "list-separator";

    // map
    D_STATIC_CONSTEXPR const char* map_get          = "map-get";
    D_STATIC_CONSTEXPR const char* map_merge        = "map-merge";
    D_STATIC_CONSTEXPR const char* map_remove       = "map-remove";
    D_STATIC_CONSTEXPR const char* map_keys         = "map-keys";
    D_STATIC_CONSTEXPR const char* map_values       = "map-values";
    D_STATIC_CONSTEXPR const char* map_has_key      = "map-has-key";

    // selector
    D_STATIC_CONSTEXPR const char* selector_nest    = "selector-nest";
    D_STATIC_CONSTEXPR const char* selector_append  = "selector-append";
    D_STATIC_CONSTEXPR const char* selector_extend  = "selector-extend";
    D_STATIC_CONSTEXPR const char* selector_replace = "selector-replace";
    D_STATIC_CONSTEXPR const char* selector_unify   = "selector-unify";
    D_STATIC_CONSTEXPR const char* is_superselector = "is-superselector";
    D_STATIC_CONSTEXPR const char* simple_selectors = "simple-selectors";
    D_STATIC_CONSTEXPR const char* selector_parse   = "selector-parse";

    // meta
    D_STATIC_CONSTEXPR const char* type_of          = "type-of";
    D_STATIC_CONSTEXPR const char* unit             = "unit";
    D_STATIC_CONSTEXPR const char* unitless         = "unitless";
    D_STATIC_CONSTEXPR const char* comparable       = "comparable";
    D_STATIC_CONSTEXPR const char* feature_exists   = "feature-exists";
    D_STATIC_CONSTEXPR const char* variable_exists  = "variable-exists";
    D_STATIC_CONSTEXPR const char* function_exists  = "function-exists";
    D_STATIC_CONSTEXPR const char* mixin_exists     = "mixin-exists";
    D_STATIC_CONSTEXPR const char* inspect          = "inspect";
    D_STATIC_CONSTEXPR const char* if_function      = "if";
    D_STATIC_CONSTEXPR const char* call             = "call";
    D_STATIC_CONSTEXPR const char* get_function     = "get-function";

}   // namespace fns


// at_keywords
//   namespace: name constants for Sass-specific at-keywords
// (without the leading `@`).
namespace at_keywords {

    D_STATIC_CONSTEXPR const char* mixin            = "mixin";
    D_STATIC_CONSTEXPR const char* include          = "include";
    D_STATIC_CONSTEXPR const char* function         = "function";
    D_STATIC_CONSTEXPR const char* return_          = "return";
    D_STATIC_CONSTEXPR const char* extend           = "extend";
    D_STATIC_CONSTEXPR const char* debug            = "debug";
    D_STATIC_CONSTEXPR const char* warn             = "warn";
    D_STATIC_CONSTEXPR const char* error            = "error";
    D_STATIC_CONSTEXPR const char* if_              = "if";
    D_STATIC_CONSTEXPR const char* else_            = "else";
    D_STATIC_CONSTEXPR const char* each             = "each";
    D_STATIC_CONSTEXPR const char* for_             = "for";
    D_STATIC_CONSTEXPR const char* while_           = "while";
    D_STATIC_CONSTEXPR const char* use              = "use";
    D_STATIC_CONSTEXPR const char* forward          = "forward";
    D_STATIC_CONSTEXPR const char* at_root          = "at-root";
    D_STATIC_CONSTEXPR const char* import_          = "import";

}   // namespace at_keywords


///////////////////////////////////////////////////////////////////////////////
///                VIII.   KIND <-> NAME MAPPING                            ///
///////////////////////////////////////////////////////////////////////////////

// sass_internal_streq
//   function: constexpr null-terminated string equality.
// Provided locally so this header stays dependency-free of
// the html / markdown / css internal_streq variants.
D_CONSTEXPR_INLINE bool
sass_internal_streq(
    const char*     _a,
    const char*     _b
)
{
    return ( (*_a == *_b) &&
             ( (*_a == '\0') ||
               (sass_internal_streq(_a + 1, _b + 1)) ) );
}


// sass_at_rule_kind_from_name
//   function: maps an at-keyword (without the leading `@`) to
// its `sass_at_rule_kind`. Returns `unknown` for keywords not
// in the Sass extension set.
D_CONSTEXPR_INLINE sass_at_rule_kind
sass_at_rule_kind_from_name(
    const char*     _name
)
{
    using K = sass_at_rule_kind;
    return (!_name)                                                 ? K::unknown
         : (sass_internal_streq(_name, at_keywords::mixin))         ? K::mixin
         : (sass_internal_streq(_name, at_keywords::include))       ? K::include
         : (sass_internal_streq(_name, at_keywords::function))      ? K::function
         : (sass_internal_streq(_name, at_keywords::return_))       ? K::return_
         : (sass_internal_streq(_name, at_keywords::extend))        ? K::extend
         : (sass_internal_streq(_name, at_keywords::debug))         ? K::debug
         : (sass_internal_streq(_name, at_keywords::warn))          ? K::warn
         : (sass_internal_streq(_name, at_keywords::error))         ? K::error
         : (sass_internal_streq(_name, at_keywords::if_))           ? K::if_
         : (sass_internal_streq(_name, at_keywords::else_))         ? K::else_
         : (sass_internal_streq(_name, at_keywords::each))          ? K::each
         : (sass_internal_streq(_name, at_keywords::for_))          ? K::for_
         : (sass_internal_streq(_name, at_keywords::while_))        ? K::while_
         : (sass_internal_streq(_name, at_keywords::use))           ? K::use
         : (sass_internal_streq(_name, at_keywords::forward))       ? K::forward
         : (sass_internal_streq(_name, at_keywords::at_root))       ? K::at_root
         : (sass_internal_streq(_name, at_keywords::import_))       ? K::import_legacy
         :    K::unknown;
}


// name_from_sass_at_rule_kind
//   function: maps a `sass_at_rule_kind` to its at-keyword
// string (without the leading `@`).
D_CONSTEXPR_INLINE const char*
name_from_sass_at_rule_kind(
    sass_at_rule_kind   _k
)
{
    using K = sass_at_rule_kind;
    return (_k == K::mixin)         ? at_keywords::mixin
         : (_k == K::include)       ? at_keywords::include
         : (_k == K::function)      ? at_keywords::function
         : (_k == K::return_)       ? at_keywords::return_
         : (_k == K::extend)        ? at_keywords::extend
         : (_k == K::debug)         ? at_keywords::debug
         : (_k == K::warn)          ? at_keywords::warn
         : (_k == K::error)         ? at_keywords::error
         : (_k == K::if_)           ? at_keywords::if_
         : (_k == K::else_)         ? at_keywords::else_
         : (_k == K::each)          ? at_keywords::each
         : (_k == K::for_)          ? at_keywords::for_
         : (_k == K::while_)        ? at_keywords::while_
         : (_k == K::use)           ? at_keywords::use
         : (_k == K::forward)       ? at_keywords::forward
         : (_k == K::at_root)       ? at_keywords::at_root
         : (_k == K::import_legacy) ? at_keywords::import_
         :    "";
}


// sass_rule_kind_from_at_rule_kind
//   function: maps a `sass_at_rule_kind` to the corresponding
// `sass_rule_kind`. Returns `unknown` for kinds without a
// dedicated rule kind.
D_CONSTEXPR_INLINE sass_rule_kind
sass_rule_kind_from_at_rule_kind(
    sass_at_rule_kind   _k
)
{
    using A = sass_at_rule_kind;
    using R = sass_rule_kind;
    return (_k == A::mixin)         ? R::mixin_declaration
         : (_k == A::include)       ? R::include_statement
         : (_k == A::function)      ? R::function_declaration
         : (_k == A::return_)       ? R::return_statement
         : (_k == A::extend)        ? R::extend_statement
         : (_k == A::debug)         ? R::debug_statement
         : (_k == A::warn)          ? R::warn_statement
         : (_k == A::error)         ? R::error_statement
         : (_k == A::if_)           ? R::if_statement
         : (_k == A::else_)         ? R::else_statement
         : (_k == A::each)          ? R::each_statement
         : (_k == A::for_)          ? R::for_statement
         : (_k == A::while_)        ? R::while_statement
         : (_k == A::use)           ? R::use_rule
         : (_k == A::forward)       ? R::forward_rule
         : (_k == A::at_root)       ? R::at_root_rule
         : (_k == A::import_legacy) ? R::import_rule_legacy
         :    R::unknown;
}


// is_sass_variable_name
//   function: true if `_name` begins with the Sass variable
// prefix (`$`).
D_CONSTEXPR_INLINE bool
is_sass_variable_name(
    const char*     _name
)
{
    return ( (_name != nullptr) && (_name[0] == '$') );
}


// is_sass_placeholder_name
//   function: true if `_name` begins with the Sass placeholder
// prefix (`%`).
D_CONSTEXPR_INLINE bool
is_sass_placeholder_name(
    const char*     _name
)
{
    return ( (_name != nullptr) && (_name[0] == '%') );
}


///////////////////////////////////////////////////////////////////////////////
///                IX.   BACKEND TAG DISPATCH                               ///
///////////////////////////////////////////////////////////////////////////////

// sass_backend_tag
//   struct: empty base tag for all Sass backend tag types.
struct sass_backend_tag
{};

// sass_default_backend_tag
struct sass_default_backend_tag : sass_backend_tag
{};

// sass_libsass_backend_tag
//   struct: tag identifying a libsass-based adapter backend.
struct sass_libsass_backend_tag : sass_backend_tag
{};

// sass_dart_backend_tag
//   struct: tag identifying a dart-sass / sass-embedded
// adapter backend.
struct sass_dart_backend_tag : sass_backend_tag
{};


///////////////////////////////////////////////////////////////////////////////
///                X.   BACKEND DETECTION                                   ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace sass
NS_INTERNAL

    // has_sass_backend_tag_helper
    //   trait: SFINAE helper detecting a nested
    // `sass_backend_tag` alias. Primary template (failure).
    template<typename _Type,
             typename = void>
    struct has_sass_backend_tag_helper
    {
        D_STATIC_CONSTEXPR bool value = false;
    };

    // has_sass_backend_tag_helper (specialization)
    template<typename _Type>
    struct has_sass_backend_tag_helper<
        _Type,
        void_t<typename _Type::sass_backend_tag>
    >
    {
        D_STATIC_CONSTEXPR bool value = true;
    };

NS_END  // internal
namespace sass {


// is_sass_backend
//   trait: true if `_Type` has a nested `sass_backend_tag`
// type.
template<typename    _Type>
struct is_sass_backend
{
    D_STATIC_CONSTEXPR bool value =
        ::djinterp::internal::has_sass_backend_tag_helper<
            clean_t<_Type>>::value;
};


// is_sass_backend_v
//   constant: convenience accessor.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_STATIC_CONSTEXPR bool is_sass_backend_v =
        is_sass_backend<_Type>::value;
#endif


}   // namespace sass
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                XI.   FOLDED TRAITS & CONCEPTS                                ///
///////////////////////////////////////////////////////////////////////////////

// The structural traits and C++20 concepts (formerly
// sass_template_traits.hpp / sass_template_concepts.hpp) are folded in
// directly below.  Their CSS counterparts live in ../css/css.hpp
// (already included above), so the dead ../css/css_template_*.hpp
// includes are dropped.  The builder engine (sass_template.hpp) is a
// separate header that includes this one.

NS_DJINTERP

namespace sass {


///////////////////////////////////////////////////////////////////////////////
///                I.   SASS RULE KIND ACCESSOR DETECTION                   ///
///////////////////////////////////////////////////////////////////////////////

// has_sass_rule_kind_method
//   trait: true if `_Type` exposes `sass_rule_kind()` const.
template<typename _Type, typename = void>
struct has_sass_rule_kind_method : std::false_type {};
template<typename _Type>
struct has_sass_rule_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().sass_rule_kind())
>> : std::true_type {};


// has_get_sass_rule_kind_method
template<typename _Type, typename = void>
struct has_get_sass_rule_kind_method : std::false_type {};
template<typename _Type>
struct has_get_sass_rule_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_sass_rule_kind())
>> : std::true_type {};


// has_sass_rule_kind_access
template<typename _Type>
struct has_sass_rule_kind_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_sass_rule_kind_method<_Type>::value ||
          has_get_sass_rule_kind_method<_Type>::value );
};


// has_sass_at_rule_kind_method
template<typename _Type, typename = void>
struct has_sass_at_rule_kind_method : std::false_type {};
template<typename _Type>
struct has_sass_at_rule_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().sass_at_rule_kind())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                II.   VARIABLE ACCESSOR DETECTION                        ///
///////////////////////////////////////////////////////////////////////////////

// has_variable_name_method / has_get_variable_name_method
template<typename _Type, typename = void>
struct has_variable_name_method : std::false_type {};
template<typename _Type>
struct has_variable_name_method<_Type, void_t<
    decltype(std::declval<const _Type&>().variable_name())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_variable_name_method : std::false_type {};
template<typename _Type>
struct has_get_variable_name_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_variable_name())
>> : std::true_type {};

template<typename _Type>
struct has_variable_name_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_variable_name_method<_Type>::value ||
          has_get_variable_name_method<_Type>::value );
};


// has_is_default_method
//   trait: true if `_Type` exposes `is_default()` const --
// used for variable declarations carrying `!default`.
template<typename _Type, typename = void>
struct has_is_default_method : std::false_type {};
template<typename _Type>
struct has_is_default_method<_Type, void_t<
    decltype(std::declval<const _Type&>().is_default())
>> : std::true_type {};


// has_is_global_method
//   trait: true if `_Type` exposes `is_global()` const -- used
// for variable declarations carrying `!global`.
template<typename _Type, typename = void>
struct has_is_global_method : std::false_type {};
template<typename _Type>
struct has_is_global_method<_Type, void_t<
    decltype(std::declval<const _Type&>().is_global())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                III.   MIXIN / FUNCTION ACCESSOR DETECTION               ///
///////////////////////////////////////////////////////////////////////////////

// has_mixin_name_method / has_get_mixin_name_method
template<typename _Type, typename = void>
struct has_mixin_name_method : std::false_type {};
template<typename _Type>
struct has_mixin_name_method<_Type, void_t<
    decltype(std::declval<const _Type&>().mixin_name())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_mixin_name_method : std::false_type {};
template<typename _Type>
struct has_get_mixin_name_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_mixin_name())
>> : std::true_type {};

template<typename _Type>
struct has_mixin_name_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_mixin_name_method<_Type>::value ||
          has_get_mixin_name_method<_Type>::value );
};


// has_function_name_method / has_get_function_name_method
template<typename _Type, typename = void>
struct has_function_name_method : std::false_type {};
template<typename _Type>
struct has_function_name_method<_Type, void_t<
    decltype(std::declval<const _Type&>().function_name())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_function_name_method : std::false_type {};
template<typename _Type>
struct has_get_function_name_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_function_name())
>> : std::true_type {};

template<typename _Type>
struct has_function_name_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_function_name_method<_Type>::value ||
          has_get_function_name_method<_Type>::value );
};


// has_parameters_method
//   trait: true if `_Type` exposes `parameters()` const.
template<typename _Type, typename = void>
struct has_parameters_method : std::false_type {};
template<typename _Type>
struct has_parameters_method<_Type, void_t<
    decltype(std::declval<const _Type&>().parameters())
>> : std::true_type {};


// has_get_parameters_method
template<typename _Type, typename = void>
struct has_get_parameters_method : std::false_type {};
template<typename _Type>
struct has_get_parameters_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_parameters())
>> : std::true_type {};


// has_parameters_access
template<typename _Type>
struct has_parameters_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_parameters_method<_Type>::value ||
          has_get_parameters_method<_Type>::value );
};


// has_accepts_content_block_method
//   trait: true if `_Type` exposes `accepts_content_block()`
// const -- mixins receiving an `@content` block.
template<typename _Type, typename = void>
struct has_accepts_content_block_method : std::false_type {};
template<typename _Type>
struct has_accepts_content_block_method<_Type, void_t<
    decltype(std::declval<const _Type&>().accepts_content_block())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                IV.   INCLUDE / EXTEND ACCESSOR DETECTION                ///
///////////////////////////////////////////////////////////////////////////////

// has_include_target_method
//   trait: true if `_Type` exposes `include_target()` const --
// the mixin name being included.
template<typename _Type, typename = void>
struct has_include_target_method : std::false_type {};
template<typename _Type>
struct has_include_target_method<_Type, void_t<
    decltype(std::declval<const _Type&>().include_target())
>> : std::true_type {};


// has_arguments_method
//   trait: true if `_Type` exposes `arguments()` const.
template<typename _Type, typename = void>
struct has_arguments_method : std::false_type {};
template<typename _Type>
struct has_arguments_method<_Type, void_t<
    decltype(std::declval<const _Type&>().arguments())
>> : std::true_type {};


// has_extend_target_method
//   trait: true if `_Type` exposes `extend_target()` const --
// the selector being extended.
template<typename _Type, typename = void>
struct has_extend_target_method : std::false_type {};
template<typename _Type>
struct has_extend_target_method<_Type, void_t<
    decltype(std::declval<const _Type&>().extend_target())
>> : std::true_type {};


// has_extend_optional_method
//   trait: true if `_Type` exposes `extend_optional()` const
// -- @extend ... !optional flag.
template<typename _Type, typename = void>
struct has_extend_optional_method : std::false_type {};
template<typename _Type>
struct has_extend_optional_method<_Type, void_t<
    decltype(std::declval<const _Type&>().extend_optional())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                V.   CONTROL FLOW ACCESSOR DETECTION                     ///
///////////////////////////////////////////////////////////////////////////////

// has_condition_method / has_get_condition_method
template<typename _Type, typename = void>
struct has_condition_method : std::false_type {};
template<typename _Type>
struct has_condition_method<_Type, void_t<
    decltype(std::declval<const _Type&>().condition())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_condition_method : std::false_type {};
template<typename _Type>
struct has_get_condition_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_condition())
>> : std::true_type {};

template<typename _Type>
struct has_condition_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_condition_method<_Type>::value ||
          has_get_condition_method<_Type>::value );
};


// has_loop_variable_method
//   trait: true if `_Type` exposes `loop_variable()` const --
// `$x` in `@each $x in ...` or `@for $x from ...`.
template<typename _Type, typename = void>
struct has_loop_variable_method : std::false_type {};
template<typename _Type>
struct has_loop_variable_method<_Type, void_t<
    decltype(std::declval<const _Type&>().loop_variable())
>> : std::true_type {};


// has_loop_iterable_method
//   trait: true if `_Type` exposes `loop_iterable()` const --
// the list / range being iterated.
template<typename _Type, typename = void>
struct has_loop_iterable_method : std::false_type {};
template<typename _Type>
struct has_loop_iterable_method<_Type, void_t<
    decltype(std::declval<const _Type&>().loop_iterable())
>> : std::true_type {};


// has_loop_from_method / has_loop_to_method
template<typename _Type, typename = void>
struct has_loop_from_method : std::false_type {};
template<typename _Type>
struct has_loop_from_method<_Type, void_t<
    decltype(std::declval<const _Type&>().loop_from())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_loop_to_method : std::false_type {};
template<typename _Type>
struct has_loop_to_method<_Type, void_t<
    decltype(std::declval<const _Type&>().loop_to())
>> : std::true_type {};


// has_loop_through_method
//   trait: true if `_Type` exposes `loop_is_through()` const
// -- distinguishes `@for $i from 1 through 5` (inclusive)
// from `@for $i from 1 to 5` (exclusive).
template<typename _Type, typename = void>
struct has_loop_through_method : std::false_type {};
template<typename _Type>
struct has_loop_through_method<_Type, void_t<
    decltype(std::declval<const _Type&>().loop_is_through())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                VI.   MODULE ACCESSOR DETECTION                          ///
///////////////////////////////////////////////////////////////////////////////

// has_module_url_method
//   trait: true if `_Type` exposes `module_url()` const --
// the URL string of `@use 'name'` or `@forward 'name'`.
template<typename _Type, typename = void>
struct has_module_url_method : std::false_type {};
template<typename _Type>
struct has_module_url_method<_Type, void_t<
    decltype(std::declval<const _Type&>().module_url())
>> : std::true_type {};


// has_module_namespace_method
//   trait: true if `_Type` exposes `module_namespace()` const
// -- the namespace alias (`@use 'foo' as bar`).
template<typename _Type, typename = void>
struct has_module_namespace_method : std::false_type {};
template<typename _Type>
struct has_module_namespace_method<_Type, void_t<
    decltype(std::declval<const _Type&>().module_namespace())
>> : std::true_type {};


// has_module_configuration_method
//   trait: true if `_Type` exposes `module_configuration()`
// const -- the `with (...)` overrides on `@use`.
template<typename _Type, typename = void>
struct has_module_configuration_method : std::false_type {};
template<typename _Type>
struct has_module_configuration_method<_Type, void_t<
    decltype(std::declval<const _Type&>().module_configuration())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                VII.   STYLESHEET SYNTAX / DIALECT DETECTION             ///
///////////////////////////////////////////////////////////////////////////////

// has_sass_syntax_method / has_get_sass_syntax_method
template<typename _Type, typename = void>
struct has_sass_syntax_method : std::false_type {};
template<typename _Type>
struct has_sass_syntax_method<_Type, void_t<
    decltype(std::declval<const _Type&>().sass_syntax())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_sass_syntax_method : std::false_type {};
template<typename _Type>
struct has_get_sass_syntax_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_sass_syntax())
>> : std::true_type {};

template<typename _Type>
struct has_sass_syntax_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_sass_syntax_method<_Type>::value ||
          has_get_sass_syntax_method<_Type>::value );
};


// has_sass_dialect_method
template<typename _Type, typename = void>
struct has_sass_dialect_method : std::false_type {};
template<typename _Type>
struct has_sass_dialect_method<_Type, void_t<
    decltype(std::declval<const _Type&>().sass_dialect())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                VIII.   RENDER METHOD DETECTION                          ///
///////////////////////////////////////////////////////////////////////////////

// has_render_to_scss_source_method
//   trait: true if `_Type` exposes
// `render_to_scss_source(std::ostream&)` const -- emits SCSS
// source rather than compiled CSS.
template<typename _Type, typename = void>
struct has_render_to_scss_source_method : std::false_type {};
template<typename _Type>
struct has_render_to_scss_source_method<_Type, void_t<
    decltype(std::declval<const _Type&>().render_to_scss_source(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_render_to_sass_source_method
//   trait: true if `_Type` exposes
// `render_to_sass_source(std::ostream&)` const -- emits Sass
// (indented) source.
template<typename _Type, typename = void>
struct has_render_to_sass_source_method : std::false_type {};
template<typename _Type>
struct has_render_to_sass_source_method<_Type, void_t<
    decltype(std::declval<const _Type&>().render_to_sass_source(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_compile_to_css_method
//   trait: true if `_Type` exposes `compile_to_css(...)` --
// evaluates the Sass tree and emits compiled CSS.
template<typename _Type, typename = void>
struct has_compile_to_css_method : std::false_type {};
template<typename _Type>
struct has_compile_to_css_method<_Type, void_t<
    decltype(std::declval<const _Type&>().compile_to_css(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_any_sass_render_method
template<typename _Type>
struct has_any_sass_render_method
{
    D_STATIC_CONSTEXPR bool value = (
           has_render_to_scss_source_method<_Type>::value
        || has_render_to_sass_source_method<_Type>::value
        || has_compile_to_css_method<_Type>::value
    );
};


///////////////////////////////////////////////////////////////////////////////
///                IX.   COMPOSITE CLASSIFIERS                              ///
///////////////////////////////////////////////////////////////////////////////

// is_sass_rule
//   trait: true if `_Type` satisfies the CSS rule protocol
// AND exposes a Sass rule-kind discriminator. The minimum bar
// is the CSS rule protocol; the Sass kind accessor marks it
// as Sass-aware.
template<typename _Type>
struct is_sass_rule
{
    D_STATIC_CONSTEXPR bool value =
        ( ::djinterp::css::is_css_rule<_Type>::value &&
          has_sass_rule_kind_access<_Type>::value );
};


// is_sass_rule_loose
//   trait: looser detection -- any CSS rule is treated as a
// candidate Sass rule.
template<typename _Type>
struct is_sass_rule_loose
{
    D_STATIC_CONSTEXPR bool value =
        ::djinterp::css::is_css_rule<_Type>::value;
};


// is_sass_variable_declaration
//   trait: true if `_Type` exposes the variable-declaration
// surface (variable name + value access).
template<typename _Type>
struct is_sass_variable_declaration
{
    D_STATIC_CONSTEXPR bool value =
        ( has_variable_name_access<_Type>::value &&
          ::djinterp::css::has_value_access<_Type>::value );
};


// is_sass_mixin_declaration
//   trait: true if `_Type` exposes mixin-name and parameter
// access.
template<typename _Type>
struct is_sass_mixin_declaration
{
    D_STATIC_CONSTEXPR bool value =
        ( has_mixin_name_access<_Type>::value &&
          has_parameters_access<_Type>::value );
};


// is_sass_function_declaration
//   trait: true if `_Type` exposes function-name and parameter
// access.
template<typename _Type>
struct is_sass_function_declaration
{
    D_STATIC_CONSTEXPR bool value =
        ( has_function_name_access<_Type>::value &&
          has_parameters_access<_Type>::value );
};


// is_sass_include_statement
//   trait: true if `_Type` exposes include-target access.
template<typename _Type>
struct is_sass_include_statement
{
    D_STATIC_CONSTEXPR bool value =
        has_include_target_method<_Type>::value;
};


// is_sass_extend_statement
//   trait: true if `_Type` exposes extend-target access.
template<typename _Type>
struct is_sass_extend_statement
{
    D_STATIC_CONSTEXPR bool value =
        has_extend_target_method<_Type>::value;
};


// is_sass_module_rule
//   trait: true if `_Type` exposes module-url access.
template<typename _Type>
struct is_sass_module_rule
{
    D_STATIC_CONSTEXPR bool value =
        has_module_url_method<_Type>::value;
};


// is_sass_stylesheet
//   trait: true if `_Type` satisfies the CSS stylesheet
// protocol AND exposes a Sass syntax accessor.
template<typename _Type>
struct is_sass_stylesheet
{
    D_STATIC_CONSTEXPR bool value =
        ( ::djinterp::css::is_css_stylesheet<_Type>::value &&
          has_sass_syntax_access<_Type>::value );
};


// is_sass_stylesheet_loose
//   trait: looser detection -- any CSS stylesheet qualifies.
template<typename _Type>
struct is_sass_stylesheet_loose
{
    D_STATIC_CONSTEXPR bool value =
        ::djinterp::css::is_css_stylesheet<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                X.   CLASSIFICATION STRUCTS                              ///
///////////////////////////////////////////////////////////////////////////////

// sass_rule_class
//   struct: comprehensive classification of a Sass-rule-shaped
// type. Aggregates the CSS rule classification with Sass
// extensions.
template<typename _Type>
struct sass_rule_class
{
    D_STATIC_CONSTEXPR bool is_sass_rule_  = is_sass_rule<_Type>::value;
    D_STATIC_CONSTEXPR bool is_css_rule_   =
        ::djinterp::css::is_css_rule<_Type>::value;

    // Sass-specific surface
    D_STATIC_CONSTEXPR bool has_kind       =
        has_sass_rule_kind_access<_Type>::value;

    D_STATIC_CONSTEXPR bool is_variable    =
        is_sass_variable_declaration<_Type>::value;
    D_STATIC_CONSTEXPR bool is_mixin       =
        is_sass_mixin_declaration<_Type>::value;
    D_STATIC_CONSTEXPR bool is_function    =
        is_sass_function_declaration<_Type>::value;
    D_STATIC_CONSTEXPR bool is_include     =
        is_sass_include_statement<_Type>::value;
    D_STATIC_CONSTEXPR bool is_extend      =
        is_sass_extend_statement<_Type>::value;
    D_STATIC_CONSTEXPR bool is_module      =
        is_sass_module_rule<_Type>::value;

    // capability flags
    D_STATIC_CONSTEXPR bool has_default    =
        has_is_default_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_global     =
        has_is_global_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_content    =
        has_accepts_content_block_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_condition  =
        has_condition_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_loop_var   =
        has_loop_variable_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_namespace_ =
        has_module_namespace_method<_Type>::value;
};


// sass_stylesheet_class
//   struct: comprehensive classification of a Sass-stylesheet
// shaped type.
template<typename _Type>
struct sass_stylesheet_class
{
    D_STATIC_CONSTEXPR bool is_sass_sheet  = is_sass_stylesheet<_Type>::value;
    D_STATIC_CONSTEXPR bool is_css_sheet   =
        ::djinterp::css::is_css_stylesheet<_Type>::value;
    D_STATIC_CONSTEXPR bool has_syntax     =
        has_sass_syntax_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_dialect    =
        has_sass_dialect_method<_Type>::value;
    D_STATIC_CONSTEXPR bool can_render_scss =
        has_render_to_scss_source_method<_Type>::value;
    D_STATIC_CONSTEXPR bool can_render_sass =
        has_render_to_sass_source_method<_Type>::value;
    D_STATIC_CONSTEXPR bool can_compile    =
        has_compile_to_css_method<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                XI.   BACKEND COMPLETENESS                               ///
///////////////////////////////////////////////////////////////////////////////

// has_sass_rule_type_alias
//   trait: true if `_Type` exposes a nested `sass_rule_type`
// alias (in addition to the inherited CSS `rule_type`).
template<typename _Type, typename = void>
struct has_sass_rule_type_alias : std::false_type {};
template<typename _Type>
struct has_sass_rule_type_alias<_Type, void_t<
    typename clean_t<_Type>::sass_rule_type
>> : std::true_type {};


// has_sass_stylesheet_type_alias
template<typename _Type, typename = void>
struct has_sass_stylesheet_type_alias : std::false_type {};
template<typename _Type>
struct has_sass_stylesheet_type_alias<_Type, void_t<
    typename clean_t<_Type>::sass_stylesheet_type
>> : std::true_type {};


// has_make_sass_stylesheet_method
//   trait: true if `_Type` exposes a static factory
// `make_sass_stylesheet()`.
template<typename _Type, typename = void>
struct has_make_sass_stylesheet_method : std::false_type {};
template<typename _Type>
struct has_make_sass_stylesheet_method<_Type, void_t<
    decltype(clean_t<_Type>::make_sass_stylesheet())
>> : std::true_type {};


// is_sass_backend_complete
//   trait: true if `_Type` exposes the full Sass backend
// protocol -- nested type aliases plus factory.
template<typename _Type>
struct is_sass_backend_complete
{
    D_STATIC_CONSTEXPR bool value =
        ( has_sass_rule_type_alias<_Type>::value       &&
          has_sass_stylesheet_type_alias<_Type>::value &&
          has_make_sass_stylesheet_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                XII.   VARIABLE TEMPLATES                                ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool has_sass_rule_kind_access_v =
        has_sass_rule_kind_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_variable_name_access_v =
        has_variable_name_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_mixin_name_access_v =
        has_mixin_name_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_parameters_access_v =
        has_parameters_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_condition_access_v =
        has_condition_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_module_url_method_v =
        has_module_url_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_rule_v =
        is_sass_rule<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_rule_loose_v =
        is_sass_rule_loose<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_variable_declaration_v =
        is_sass_variable_declaration<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_mixin_declaration_v =
        is_sass_mixin_declaration<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_function_declaration_v =
        is_sass_function_declaration<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_include_statement_v =
        is_sass_include_statement<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_extend_statement_v =
        is_sass_extend_statement<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_module_rule_v =
        is_sass_module_rule<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_stylesheet_v =
        is_sass_stylesheet<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_stylesheet_loose_v =
        is_sass_stylesheet_loose<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_backend_complete_v =
        is_sass_backend_complete<_Type>::value;

#endif  // variable templates


}   // namespace sass
NS_END  // djinterp


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// std
#include <concepts>


NS_DJINTERP

namespace sass {


///////////////////////////////////////////////////////////////////////////////
///                I.   RULE CONCEPTS                                       ///
///////////////////////////////////////////////////////////////////////////////

// sass_rule_type
//   concept: satisfied by any type that satisfies the CSS rule
// protocol AND exposes a Sass rule-kind discriminator.
template<typename _Type>
concept sass_rule_type =
    is_sass_rule<_Type>::value;


// sass_rule_loose_type
//   concept: looser variant -- any CSS rule qualifies.
template<typename _Type>
concept sass_rule_loose_type =
    is_sass_rule_loose<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                II.   VARIABLE / MIXIN / FUNCTION CONCEPTS               ///
///////////////////////////////////////////////////////////////////////////////

// sass_variable_declaration_type
//   concept: a rule exposing variable-name and value access.
template<typename _Type>
concept sass_variable_declaration_type =
    is_sass_variable_declaration<_Type>::value;


// defaulted_variable_declaration
//   concept: a variable declaration that exposes the
// `!default` flag.
template<typename _Type>
concept defaulted_variable_declaration =
       ( sass_variable_declaration_type<_Type> )
    && ( has_is_default_method<_Type>::value );


// global_variable_declaration
//   concept: a variable declaration that exposes the
// `!global` flag.
template<typename _Type>
concept global_variable_declaration =
       ( sass_variable_declaration_type<_Type> )
    && ( has_is_global_method<_Type>::value );


// sass_mixin_declaration_type
//   concept: a rule exposing mixin-name and parameter access.
template<typename _Type>
concept sass_mixin_declaration_type =
    is_sass_mixin_declaration<_Type>::value;


// content_aware_mixin
//   concept: a mixin declaration exposing the
// `accepts_content_block()` predicate.
template<typename _Type>
concept content_aware_mixin =
       ( sass_mixin_declaration_type<_Type> )
    && ( has_accepts_content_block_method<_Type>::value );


// sass_function_declaration_type
//   concept: a rule exposing function-name and parameter
// access.
template<typename _Type>
concept sass_function_declaration_type =
    is_sass_function_declaration<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                III.   INCLUDE / EXTEND CONCEPTS                         ///
///////////////////////////////////////////////////////////////////////////////

// sass_include_statement_type
//   concept: a rule exposing include-target access.
template<typename _Type>
concept sass_include_statement_type =
    is_sass_include_statement<_Type>::value;


// sass_extend_statement_type
//   concept: a rule exposing extend-target access.
template<typename _Type>
concept sass_extend_statement_type =
    is_sass_extend_statement<_Type>::value;


// optional_extend_statement
//   concept: an extend statement exposing the `!optional`
// flag.
template<typename _Type>
concept optional_extend_statement =
       ( sass_extend_statement_type<_Type> )
    && ( has_extend_optional_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                IV.   CONTROL FLOW CONCEPTS                              ///
///////////////////////////////////////////////////////////////////////////////

// conditional_sass_rule
//   concept: a rule exposing a condition expression (i.e.
// classifiable as @if / @while).
template<typename _Type>
concept conditional_sass_rule =
       ( sass_rule_type<_Type> )
    && ( has_condition_access<_Type>::value );


// loop_sass_rule
//   concept: a rule exposing a loop binding (i.e.
// classifiable as @each / @for).
template<typename _Type>
concept loop_sass_rule =
       ( sass_rule_type<_Type> )
    && ( has_loop_variable_method<_Type>::value );


// each_sass_rule
//   concept: a loop rule whose iterable is a list / map.
template<typename _Type>
concept each_sass_rule =
       ( loop_sass_rule<_Type> )
    && ( has_loop_iterable_method<_Type>::value );


// numeric_loop_sass_rule
//   concept: a loop rule with from / to numeric bounds (@for).
template<typename _Type>
concept numeric_loop_sass_rule =
       ( loop_sass_rule<_Type> )
    && ( has_loop_from_method<_Type>::value )
    && ( has_loop_to_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                V.   MODULE CONCEPTS                                     ///
///////////////////////////////////////////////////////////////////////////////

// sass_module_rule_type
//   concept: a rule exposing module-url access.
template<typename _Type>
concept sass_module_rule_type =
    is_sass_module_rule<_Type>::value;


// namespaced_module_rule
//   concept: a module rule exposing namespace-alias access.
template<typename _Type>
concept namespaced_module_rule =
       ( sass_module_rule_type<_Type> )
    && ( has_module_namespace_method<_Type>::value );


// configured_module_rule
//   concept: a module rule exposing `with (...)` configuration
// access.
template<typename _Type>
concept configured_module_rule =
       ( sass_module_rule_type<_Type> )
    && ( has_module_configuration_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                VI.   STYLESHEET CONCEPTS                                ///
///////////////////////////////////////////////////////////////////////////////

// sass_stylesheet_type
//   concept: a stylesheet exposing syntax accessor plus the
// inherited CSS stylesheet protocol.
template<typename _Type>
concept sass_stylesheet_type =
    is_sass_stylesheet<_Type>::value;


// sass_stylesheet_loose_type
//   concept: looser variant -- any CSS stylesheet qualifies.
template<typename _Type>
concept sass_stylesheet_loose_type =
    is_sass_stylesheet_loose<_Type>::value;


// flavoured_sass_stylesheet
//   concept: a stylesheet exposing the dialect accessor.
template<typename _Type>
concept flavoured_sass_stylesheet =
       ( sass_stylesheet_type<_Type> )
    && ( has_sass_dialect_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                VII.   RENDER-TARGET CONCEPTS                            ///
///////////////////////////////////////////////////////////////////////////////

// scss_renderable_sass_stylesheet
//   concept: a stylesheet that emits SCSS source.
template<typename _Type>
concept scss_renderable_sass_stylesheet =
    has_render_to_scss_source_method<_Type>::value;


// indented_renderable_sass_stylesheet
//   concept: a stylesheet that emits Sass-indented source.
template<typename _Type>
concept indented_renderable_sass_stylesheet =
    has_render_to_sass_source_method<_Type>::value;


// compilable_sass_stylesheet
//   concept: a stylesheet that compiles to CSS.
template<typename _Type>
concept compilable_sass_stylesheet =
    has_compile_to_css_method<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                VIII.   COMPOSITE CONCEPTS                               ///
///////////////////////////////////////////////////////////////////////////////

// full_sass_stylesheet
//   concept: a stylesheet exposing every Sass render target
// plus syntax and dialect metadata.
template<typename _Type>
concept full_sass_stylesheet =
       ( sass_stylesheet_type<_Type> )
    && ( flavoured_sass_stylesheet<_Type> )
    && ( scss_renderable_sass_stylesheet<_Type> )
    && ( indented_renderable_sass_stylesheet<_Type> )
    && ( compilable_sass_stylesheet<_Type> );


///////////////////////////////////////////////////////////////////////////////
///                IX.   BACKEND CONCEPTS                                   ///
///////////////////////////////////////////////////////////////////////////////

// sass_backend_type
//   concept: satisfied by any type tagged with
// `sass_backend_tag`.
template<typename _Type>
concept sass_backend_type =
    is_sass_backend<_Type>::value;


// complete_sass_backend
//   concept: a Sass backend that additionally exposes the
// full nested-type-alias protocol and the
// `make_sass_stylesheet` factory.
template<typename _Type>
concept complete_sass_backend =
       ( sass_backend_type<_Type> )
    && ( is_sass_backend_complete<_Type>::value );


}   // namespace sass
NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_SASS_
