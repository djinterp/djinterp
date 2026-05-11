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
XI.   SUB-MODULE INCLUDES
*/

#ifndef DJINTERP_SASS_
#define DJINTERP_SASS_ 1

// std
#include <cstddef>
#include <cstdint>
// djinterp
#include "../../../djinterp.hpp"
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
///                XI.   SUB-MODULE INCLUDES                                ///
///////////////////////////////////////////////////////////////////////////////

#include "./sass_template_traits.hpp"
#include "./sass_template.hpp"
#include "./sass_template_concepts.hpp"


#endif  // DJINTERP_SASS_
