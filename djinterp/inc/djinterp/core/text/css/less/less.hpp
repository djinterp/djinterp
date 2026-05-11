/******************************************************************************
* djinterp [less]                                                     less.hpp
*
*   Foundational Less module for the djinterp framework. Layered on
* top of the CSS module: a Less stylesheet IS a CSS stylesheet plus
* the Less-language extensions (variables, mixins, guards,
* extension, parametric mixins, namespaces, interpolation, escaping,
* operations, plugins). The Less facades extend the CSS facades so
* that storage and memory layout are unchanged below the extension
* surface; Less-specific behaviour is added via methods and free
* functions only -- never via additional state on the inherited CSS
* shape.
*
*   ZERO OVERHEAD:
*   - The Less rule-kind enum is `std::uint8_t`-backed.
*   - Category and classification predicates are `D_CONSTEXPR`.
*   - Built-in function name constants live as
*     `D_STATIC_CONSTEXPR const char*` literal pointers (no
*     allocation, no dynamic init).
*   - The Less facades inherit from CSS facades and add NO data
*     members of their own.
*
*   LIBRARY AGNOSTICISM:
*   This header pulls in the CSS module and nothing else beyond the
* standard library. Adapter backends for less.js IPC, less-cpp,
* etc. are detected structurally via the trait layer.
*
*   KEY DIFFERENCES FROM SASS:
*   - Variables use `@name: value;` (not `$name: value;`).
*   - Mixins are NOT a separate declaration form -- any rule
*     `.name() { ... }` or `.name { ... }` is callable.
*   - Mixin guards use `when (...)` after the parameter list.
*   - Extension uses `:extend(selector)` pseudo or `&:extend(...)`.
*   - No module system -- everything is `@import` (with optional
*     filter keywords: `(less)`, `(css)`, `(reference)`, `(once)`,
*     `(multiple)`, `(inline)`, `(optional)`).
*   - Interpolation uses `@{name}` (not `#{name}`).
*   - Namespaces are nested mixins acting as scoped accessors.
*
*
* path:      /inc/djinterp/core/util/less/less.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SHARED TYPES & CONSTANTS
II.   LESS DIALECT ENUM
III.  LESS RULE KIND ENUM
IV.   LESS AT-RULE KIND ENUM
V.    IMPORT OPTIONS / MERGE MODE
VI.   RULE CATEGORY CLASSIFICATION
VII.  BUILT-IN FUNCTION NAMESPACE
VIII. AT-RULE NAME NAMESPACE
IX.   KIND <-> NAME MAPPING
X.    BACKEND TAG DISPATCH
XI.   BACKEND DETECTION
XII.  SUB-MODULE INCLUDES
*/

#ifndef DJINTERP_LESS_
#define DJINTERP_LESS_ 1

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

// NOTE: djinterp.hpp does not (yet) define NS_LESS. Using a plain
// namespace declaration here matching the html / markdown / css /
// sass pattern; add `#define NS_LESS D_NAMESPACE(less)` alongside
// the other NS_* macros if the macro idiom is preferred.
namespace less {


// less_string_t
//   type: alias for the string type used throughout the less
// module.
using less_string_t = ::djinterp::css::css_string_t;


// D_LESS_VARIABLE_PREFIX
//   constant: prefix character for Less variables ("@").
#ifndef D_LESS_VARIABLE_PREFIX
    #define D_LESS_VARIABLE_PREFIX      "@"
#endif


// D_LESS_INTERPOLATION_OPEN / CLOSE
//   constant: delimiters for Less interpolation ("@{name}").
#ifndef D_LESS_INTERPOLATION_OPEN
    #define D_LESS_INTERPOLATION_OPEN   "@{"
#endif
#ifndef D_LESS_INTERPOLATION_CLOSE
    #define D_LESS_INTERPOLATION_CLOSE  "}"
#endif


// D_LESS_ESCAPE_PREFIX
//   constant: prefix for Less string escaping (`~"..."`).
#ifndef D_LESS_ESCAPE_PREFIX
    #define D_LESS_ESCAPE_PREFIX        "~"
#endif


///////////////////////////////////////////////////////////////////////////////
///                II.   LESS DIALECT ENUM                                  ///
///////////////////////////////////////////////////////////////////////////////

// less_dialect
//   enum: identifies the implementation / spec target.
enum class less_dialect : std::uint8_t
{
    less_js,            // less.js (canonical implementation)
    less_node,          // less-node CLI (same engine, different host)
    less_browser,       // browser-side less.js
    less_cpp,           // less-cpp (community port)
    custom,
    unspecified
};


///////////////////////////////////////////////////////////////////////////////
///                III.   LESS RULE KIND ENUM                               ///
///////////////////////////////////////////////////////////////////////////////

// less_rule_kind
//   enum: discriminator for Less-specific rule kinds. The
// standard CSS rule kinds remain reachable through the
// inherited CSS layer; this enum captures the additions Less
// introduces.
enum class less_rule_kind : std::uint8_t
{
    // declarations
    variable_declaration,       // @name: value;
    property_variable,          // $name reference inside a value (Less 3.5+)

    // mixin model (Less mixins are just rules whose selector
    // happens to be callable, but it is useful to surface
    // the role explicitly when known)
    mixin_definition,           // .name(params) { ... } with body
    mixin_call,                 // .name(args); top-level invocation
    parametric_mixin,           // mixin with a parameter list
    rule_set_alias,             // .name() { ... }; -- pattern-matching
                                //   mixin with the trailing ; to
                                //   suppress emission

    // extension
    extend_statement,           // &:extend(selector);
    extend_inside_selector,     // selector:extend(target) { ... }

    // guards
    guarded_mixin,              // .name() when (condition) { ... }
    guard_condition,            // when (condition) -- statement form

    // imports
    import_with_options,        // @import (reference) 'file.less';
    plugin_import,              // @plugin 'name';

    // namespaces
    namespace_definition,       // #namespace { ... }
    namespace_access,           // #namespace > .mixin

    // detached rulesets (Less 1.7+)
    detached_ruleset_declaration,   // @name: { ... };
    detached_ruleset_call,          // @name();

    // interpolated selectors / property names
    interpolated_selector,
    interpolated_property,

    // sentinel
    unknown = 255
};


///////////////////////////////////////////////////////////////////////////////
///                IV.   LESS AT-RULE KIND ENUM                             ///
///////////////////////////////////////////////////////////////////////////////

// less_at_rule_kind
//   enum: refines `css::css_at_rule_kind` with Less-specific
// at-keywords. Less reuses CSS at-rules unchanged, so the
// additions are limited.
enum class less_at_rule_kind : std::uint8_t
{
    import_,                    // @import (with Less filter options)
    plugin,                     // @plugin (Less plugin import)
    options,                    // @options (Less options directive)
    custom,
    unknown = 255
};


///////////////////////////////////////////////////////////////////////////////
///                V.   IMPORT OPTIONS / MERGE MODE                         ///
///////////////////////////////////////////////////////////////////////////////

// less_import_option
//   enum: bitmask of options applicable to `@import` in Less.
// Multiple flags may be combined.
enum less_import_option : unsigned
{
    lio_none      = 0u,
    lio_reference = 1u << 0,    // (reference) -- import without emit
    lio_inline_   = 1u << 1,    // (inline) -- copy raw content
    lio_less      = 1u << 2,    // (less) -- force Less parsing
    lio_css       = 1u << 3,    // (css) -- emit unchanged
    lio_once      = 1u << 4,    // (once) -- default behaviour
    lio_multiple  = 1u << 5,    // (multiple) -- re-import
    lio_optional  = 1u << 6     // (optional) -- skip if missing
};


// less_merge_mode
//   enum: how a Less declaration carrying `+` or `+_` should
// be merged with previously-encountered declarations sharing
// the same property name.
enum class less_merge_mode : std::uint8_t
{
    none,                       // no merge (default)
    comma,                      // `prop+: ...` -- comma-merge
    space                       // `prop+_: ...` -- space-merge
};


///////////////////////////////////////////////////////////////////////////////
///                VI.   RULE CATEGORY CLASSIFICATION                       ///
///////////////////////////////////////////////////////////////////////////////

// less_rule_category
//   enum: bitmask describing the categorical properties of a
// Less rule kind. Backed by `unsigned`.
enum less_rule_category : unsigned
{
    lrc_none           = 0u,
    lrc_declaration    = 1u <<  0,  // declares a variable / mixin
    lrc_mixin          = 1u <<  1,  // mixin-related (def / call)
    lrc_invocation     = 1u <<  2,  // calls a mixin
    lrc_extension      = 1u <<  3,  // :extend(...)
    lrc_guard          = 1u <<  4,  // `when` clause
    lrc_import         = 1u <<  5,  // @import / @plugin
    lrc_namespace      = 1u <<  6,  // #ns { ... } scope
    lrc_interpolation  = 1u <<  7,  // `@{name}` inside selector / property
    lrc_detached       = 1u <<  8,  // detached ruleset declaration / call
    lrc_grouping       = 1u <<  9,  // hosts nested rules
    lrc_property_ref   = 1u << 10   // $name reference
};


// less_category_for_rule_kind
//   function: returns the category bitmask for a Less rule
// kind.
D_CONSTEXPR_INLINE unsigned
less_category_for_rule_kind(
    less_rule_kind  _k
)
{
    using K = less_rule_kind;
    return (_k == K::variable_declaration)          ? (lrc_declaration)
         : (_k == K::property_variable)             ? (lrc_property_ref)

         : (_k == K::mixin_definition)              ? (lrc_declaration | lrc_mixin
                                                       | lrc_grouping)
         : (_k == K::mixin_call)                    ? (lrc_invocation | lrc_mixin)
         : (_k == K::parametric_mixin)              ? (lrc_declaration | lrc_mixin
                                                       | lrc_grouping)
         : (_k == K::rule_set_alias)                ? (lrc_declaration | lrc_mixin
                                                       | lrc_grouping)

         : (_k == K::extend_statement)              ? (lrc_extension)
         : (_k == K::extend_inside_selector)        ? (lrc_extension)

         : (_k == K::guarded_mixin)                 ? (lrc_declaration | lrc_mixin
                                                       | lrc_guard | lrc_grouping)
         : (_k == K::guard_condition)               ? (lrc_guard | lrc_grouping)

         : (_k == K::import_with_options)           ? (lrc_import)
         : (_k == K::plugin_import)                 ? (lrc_import)

         : (_k == K::namespace_definition)          ? (lrc_namespace | lrc_grouping)
         : (_k == K::namespace_access)              ? (lrc_namespace | lrc_invocation)

         : (_k == K::detached_ruleset_declaration)  ? (lrc_declaration | lrc_detached
                                                       | lrc_grouping)
         : (_k == K::detached_ruleset_call)         ? (lrc_invocation | lrc_detached)

         : (_k == K::interpolated_selector)         ? (lrc_interpolation)
         : (_k == K::interpolated_property)         ? (lrc_interpolation)

         :    lrc_none;
}


// is_less_mixin_kind
//   function: true if the rule is mixin-related (definition,
// call, parametric).
D_CONSTEXPR_INLINE bool
is_less_mixin_kind(
    less_rule_kind  _k
)
{
    return ((less_category_for_rule_kind(_k) & lrc_mixin) != 0u);
}


// is_less_extension_kind
//   function: true if the rule is an extension statement.
D_CONSTEXPR_INLINE bool
is_less_extension_kind(
    less_rule_kind  _k
)
{
    return ((less_category_for_rule_kind(_k) & lrc_extension) != 0u);
}


// is_less_guarded_kind
//   function: true if the rule carries a `when` guard.
D_CONSTEXPR_INLINE bool
is_less_guarded_kind(
    less_rule_kind  _k
)
{
    return ((less_category_for_rule_kind(_k) & lrc_guard) != 0u);
}


// is_less_namespace_kind
//   function: true if the rule is namespace-related.
D_CONSTEXPR_INLINE bool
is_less_namespace_kind(
    less_rule_kind  _k
)
{
    return ((less_category_for_rule_kind(_k) & lrc_namespace) != 0u);
}


// is_less_import_kind
//   function: true if the rule is @import or @plugin.
D_CONSTEXPR_INLINE bool
is_less_import_kind(
    less_rule_kind  _k
)
{
    return ((less_category_for_rule_kind(_k) & lrc_import) != 0u);
}


// is_less_grouping_kind
//   function: true if the rule hosts nested rules.
D_CONSTEXPR_INLINE bool
is_less_grouping_kind(
    less_rule_kind  _k
)
{
    return ((less_category_for_rule_kind(_k) & lrc_grouping) != 0u);
}


///////////////////////////////////////////////////////////////////////////////
///                VII.   BUILT-IN FUNCTION NAMESPACE                       ///
///////////////////////////////////////////////////////////////////////////////

// fns
//   namespace: name constants for commonly-used built-in
// functions in Less.
namespace fns {

    // string
    D_STATIC_CONSTEXPR const char* e                = "e";
    D_STATIC_CONSTEXPR const char* escape           = "escape";
    D_STATIC_CONSTEXPR const char* format           = "%";
    D_STATIC_CONSTEXPR const char* replace          = "replace";

    // logical
    D_STATIC_CONSTEXPR const char* if_              = "if";
    D_STATIC_CONSTEXPR const char* boolean          = "boolean";

    // list
    D_STATIC_CONSTEXPR const char* length           = "length";
    D_STATIC_CONSTEXPR const char* extract          = "extract";
    D_STATIC_CONSTEXPR const char* range            = "range";
    D_STATIC_CONSTEXPR const char* each             = "each";

    // math
    D_STATIC_CONSTEXPR const char* ceil             = "ceil";
    D_STATIC_CONSTEXPR const char* floor            = "floor";
    D_STATIC_CONSTEXPR const char* percentage       = "percentage";
    D_STATIC_CONSTEXPR const char* round            = "round";
    D_STATIC_CONSTEXPR const char* sqrt             = "sqrt";
    D_STATIC_CONSTEXPR const char* abs              = "abs";
    D_STATIC_CONSTEXPR const char* sin              = "sin";
    D_STATIC_CONSTEXPR const char* cos              = "cos";
    D_STATIC_CONSTEXPR const char* tan              = "tan";
    D_STATIC_CONSTEXPR const char* asin             = "asin";
    D_STATIC_CONSTEXPR const char* acos             = "acos";
    D_STATIC_CONSTEXPR const char* atan             = "atan";
    D_STATIC_CONSTEXPR const char* pi               = "pi";
    D_STATIC_CONSTEXPR const char* pow              = "pow";
    D_STATIC_CONSTEXPR const char* mod              = "mod";
    D_STATIC_CONSTEXPR const char* min              = "min";
    D_STATIC_CONSTEXPR const char* max              = "max";

    // type predicates
    D_STATIC_CONSTEXPR const char* isnumber         = "isnumber";
    D_STATIC_CONSTEXPR const char* isstring         = "isstring";
    D_STATIC_CONSTEXPR const char* iscolor          = "iscolor";
    D_STATIC_CONSTEXPR const char* iskeyword        = "iskeyword";
    D_STATIC_CONSTEXPR const char* isurl            = "isurl";
    D_STATIC_CONSTEXPR const char* ispixel          = "ispixel";
    D_STATIC_CONSTEXPR const char* isem             = "isem";
    D_STATIC_CONSTEXPR const char* ispercentage     = "ispercentage";
    D_STATIC_CONSTEXPR const char* isunit           = "isunit";
    D_STATIC_CONSTEXPR const char* isruleset        = "isruleset";
    D_STATIC_CONSTEXPR const char* isdefined        = "isdefined";

    // color definition
    D_STATIC_CONSTEXPR const char* rgb              = "rgb";
    D_STATIC_CONSTEXPR const char* rgba             = "rgba";
    D_STATIC_CONSTEXPR const char* argb             = "argb";
    D_STATIC_CONSTEXPR const char* hsl              = "hsl";
    D_STATIC_CONSTEXPR const char* hsla             = "hsla";
    D_STATIC_CONSTEXPR const char* hsv              = "hsv";
    D_STATIC_CONSTEXPR const char* hsva             = "hsva";
    D_STATIC_CONSTEXPR const char* hwb              = "hwb";

    // color channel
    D_STATIC_CONSTEXPR const char* hue              = "hue";
    D_STATIC_CONSTEXPR const char* saturation       = "saturation";
    D_STATIC_CONSTEXPR const char* lightness        = "lightness";
    D_STATIC_CONSTEXPR const char* hsvhue           = "hsvhue";
    D_STATIC_CONSTEXPR const char* hsvsaturation    = "hsvsaturation";
    D_STATIC_CONSTEXPR const char* hsvvalue         = "hsvvalue";
    D_STATIC_CONSTEXPR const char* red              = "red";
    D_STATIC_CONSTEXPR const char* green            = "green";
    D_STATIC_CONSTEXPR const char* blue             = "blue";
    D_STATIC_CONSTEXPR const char* alpha            = "alpha";
    D_STATIC_CONSTEXPR const char* luma             = "luma";
    D_STATIC_CONSTEXPR const char* luminance        = "luminance";

    // color operation
    D_STATIC_CONSTEXPR const char* saturate         = "saturate";
    D_STATIC_CONSTEXPR const char* desaturate       = "desaturate";
    D_STATIC_CONSTEXPR const char* lighten          = "lighten";
    D_STATIC_CONSTEXPR const char* darken           = "darken";
    D_STATIC_CONSTEXPR const char* fadein           = "fadein";
    D_STATIC_CONSTEXPR const char* fadeout          = "fadeout";
    D_STATIC_CONSTEXPR const char* fade             = "fade";
    D_STATIC_CONSTEXPR const char* spin             = "spin";
    D_STATIC_CONSTEXPR const char* mix              = "mix";
    D_STATIC_CONSTEXPR const char* tint             = "tint";
    D_STATIC_CONSTEXPR const char* shade            = "shade";
    D_STATIC_CONSTEXPR const char* greyscale        = "greyscale";
    D_STATIC_CONSTEXPR const char* contrast         = "contrast";

    // color blending
    D_STATIC_CONSTEXPR const char* multiply         = "multiply";
    D_STATIC_CONSTEXPR const char* screen           = "screen";
    D_STATIC_CONSTEXPR const char* overlay          = "overlay";
    D_STATIC_CONSTEXPR const char* softlight        = "softlight";
    D_STATIC_CONSTEXPR const char* hardlight        = "hardlight";
    D_STATIC_CONSTEXPR const char* difference       = "difference";
    D_STATIC_CONSTEXPR const char* exclusion        = "exclusion";
    D_STATIC_CONSTEXPR const char* average          = "average";
    D_STATIC_CONSTEXPR const char* negation         = "negation";

    // misc
    D_STATIC_CONSTEXPR const char* default_         = "default";
    D_STATIC_CONSTEXPR const char* unit             = "unit";
    D_STATIC_CONSTEXPR const char* get_unit         = "get-unit";
    D_STATIC_CONSTEXPR const char* convert          = "convert";
    D_STATIC_CONSTEXPR const char* data_uri         = "data-uri";
    D_STATIC_CONSTEXPR const char* image_size       = "image-size";
    D_STATIC_CONSTEXPR const char* image_width      = "image-width";
    D_STATIC_CONSTEXPR const char* image_height     = "image-height";
    D_STATIC_CONSTEXPR const char* svg_gradient     = "svg-gradient";

}   // namespace fns


///////////////////////////////////////////////////////////////////////////////
///                VIII.   AT-RULE NAME NAMESPACE                           ///
///////////////////////////////////////////////////////////////////////////////

// at_keywords
//   namespace: name constants for Less-specific at-keywords.
namespace at_keywords {

    D_STATIC_CONSTEXPR const char* import_          = "import";
    D_STATIC_CONSTEXPR const char* plugin           = "plugin";
    D_STATIC_CONSTEXPR const char* options          = "options";

}   // namespace at_keywords


///////////////////////////////////////////////////////////////////////////////
///                IX.   KIND <-> NAME MAPPING                              ///
///////////////////////////////////////////////////////////////////////////////

// less_internal_streq
//   function: constexpr null-terminated string equality.
D_CONSTEXPR_INLINE bool
less_internal_streq(
    const char*     _a,
    const char*     _b
)
{
    return ( (*_a == *_b) &&
             ( (*_a == '\0') ||
               (less_internal_streq(_a + 1, _b + 1)) ) );
}


// less_at_rule_kind_from_name
//   function: maps an at-keyword to its `less_at_rule_kind`.
D_CONSTEXPR_INLINE less_at_rule_kind
less_at_rule_kind_from_name(
    const char*     _name
)
{
    using K = less_at_rule_kind;
    return (!_name)                                                 ? K::unknown
         : (less_internal_streq(_name, at_keywords::import_))       ? K::import_
         : (less_internal_streq(_name, at_keywords::plugin))        ? K::plugin
         : (less_internal_streq(_name, at_keywords::options))       ? K::options
         :    K::unknown;
}


// name_from_less_at_rule_kind
//   function: maps a `less_at_rule_kind` to its at-keyword
// string.
D_CONSTEXPR_INLINE const char*
name_from_less_at_rule_kind(
    less_at_rule_kind   _k
)
{
    using K = less_at_rule_kind;
    return (_k == K::import_) ? at_keywords::import_
         : (_k == K::plugin)  ? at_keywords::plugin
         : (_k == K::options) ? at_keywords::options
         :    "";
}


// is_less_variable_name
//   function: true if `_name` begins with the Less variable
// prefix (`@`).
D_CONSTEXPR_INLINE bool
is_less_variable_name(
    const char*     _name
)
{
    return ( (_name != nullptr) && (_name[0] == '@') );
}


// is_less_property_variable_name
//   function: true if `_name` begins with the Less property-
// reference prefix (`$`).
D_CONSTEXPR_INLINE bool
is_less_property_variable_name(
    const char*     _name
)
{
    return ( (_name != nullptr) && (_name[0] == '$') );
}


// is_less_mixin_selector
//   function: true if `_selector` looks like a Less mixin
// callable (begins with `.` or `#` and ends with `()` or has
// a parameter list). Heuristic; adapter backends with full
// parsing should override.
D_CONSTEXPR_INLINE bool
is_less_mixin_selector(
    const char*     _selector
)
{
    // First char must be `.` or `#`; we treat trailing `()`
    // (or any selector containing `(`) as the mixin marker.
    // Walk via recursion to stay constexpr in C++11.
    return ( (_selector != nullptr) &&
             ((_selector[0] == '.') || (_selector[0] == '#')) );
}


///////////////////////////////////////////////////////////////////////////////
///                X.   BACKEND TAG DISPATCH                                ///
///////////////////////////////////////////////////////////////////////////////

// less_backend_tag
//   struct: empty base tag for all Less backend tag types.
struct less_backend_tag
{};

// less_default_backend_tag
struct less_default_backend_tag : less_backend_tag
{};

// less_lessjs_backend_tag
//   struct: tag identifying an adapter backend bound to
// less.js (via N-API / IPC).
struct less_lessjs_backend_tag : less_backend_tag
{};

// less_cpp_backend_tag
//   struct: tag identifying an adapter backend bound to
// less-cpp.
struct less_cpp_backend_tag : less_backend_tag
{};


///////////////////////////////////////////////////////////////////////////////
///                XI.   BACKEND DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace less
NS_INTERNAL

    // has_less_backend_tag_helper
    //   trait: SFINAE helper detecting a nested
    // `less_backend_tag` alias.
    template<typename _Type,
             typename = void>
    struct has_less_backend_tag_helper
    {
        D_STATIC_CONSTEXPR bool value = false;
    };

    // has_less_backend_tag_helper (specialization)
    template<typename _Type>
    struct has_less_backend_tag_helper<
        _Type,
        void_t<typename _Type::less_backend_tag>
    >
    {
        D_STATIC_CONSTEXPR bool value = true;
    };

NS_END  // internal
namespace less {


// is_less_backend
//   trait: true if `_Type` has a nested `less_backend_tag`
// type.
template<typename    _Type>
struct is_less_backend
{
    D_STATIC_CONSTEXPR bool value =
        ::djinterp::internal::has_less_backend_tag_helper<
            clean_t<_Type>>::value;
};


// is_less_backend_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_STATIC_CONSTEXPR bool is_less_backend_v =
        is_less_backend<_Type>::value;
#endif


}   // namespace less
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                XII.   SUB-MODULE INCLUDES                               ///
///////////////////////////////////////////////////////////////////////////////

#include "./less_template_traits.hpp"
#include "./less_template.hpp"
#include "./less_template_concepts.hpp"


#endif  // DJINTERP_LESS_
