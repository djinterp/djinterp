/******************************************************************************
* djinterp [container]                                        table_options.hpp
*
*   The option vocabulary shared by the two table declaration front ends --
* table_builder (the compile-time TYPE DSL) and table_parser (the TEXT DSL).
* Both build the same table (Overlays: parse and compose are opposite legs of
* one prism; ch-parsing.tex), so both must read their configuration from ONE
* option surface.  This header supplies that surface: a graded set of policy
* and strictness options, expressed in the [option] subframework's own
* option<> / option_set<> machinery.
*
*   TWO LAYERS.
*   1. The VOCABULARY -- a family of plain enums (table_strictness, shape_policy,
*      ...) naming the graded choices.  Available on every standard, so a runtime
*      parse context may read the same grades a compile-time option pack carries.
*   2. The OPTION surface -- one key type (table_opt_key), a value carrier, the
*      option<> aliases, ready-made named options, defaults, and the compile-time
*      readers (table_option_value + the per-category *_of<> shorthands) the two
*      front ends consume.  This layer rides on option<>'s auto-NTTP key form and
*      so, like the rest of the option pack surface, requires C++17.
*
*   STRICTNESS IS GRADED, PER CATEGORY.  A table declaration answers several
* independent "how strict?" questions -- must a row's cell count match the
* declared width exactly, may the domain be jagged, must a text cell parse to its
* column type, and so on.  Each is its own key with its own grade enum, so they
* tune independently (the "strictness for certain/all categories" of the sketch).
* The open question a `| headers... ||||` (a multi-cell placeholder over five
* cells) raises -- error on a bad fit, ignore the overflow, or pad the shortfall
* -- is exactly table_strictness on the placeholder_fit key.
*
*   ONE KEY TYPE.  option_set requires every option in a set to share a key_type,
* so every table option is an enumerator of the single table_opt_key enum; the
* value rides in the option's arg as an integral_constant.  Container AXIS
* options (container_options.hpp) carry a different key_type and so form a
* SEPARATE pack -- table_builder partitions the two before consuming each.
*
*   PORTABILITY:
*   C++11 for the enum vocabulary; C++17 for the option<> surface (the auto-NTTP
* option key), matching the [option] pack form and container_options.hpp.
*
*   NOTE ON `D_CONSTEXPR_VAR`.  The variable templates below are spelled with the
* language keywords rather than D_CONSTEXPR_INLINE, which expands to
* D_CONSTEXPR D_INLINE -- a FUNCTION qualifier carrying always_inline, an attribute
* a variable cannot wear (it is diagnosed and dropped).  This surface is C++17-gated
* already, so the inline variable is available and exact.  Should the framework grow
* a variable-side qualifier (a D_INLINE_VAR / D_CONSTEXPR_VAR), these become it.
*
*
* path:      /inc/djinterp/core/container/table/table_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.14
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Strictness / policy vocabulary   (the graded enums; C++11+)
II.   defaults                         (the default grade per category; C++11+,
                                        resolved by cfg_table.h)
III.  table_opt_key                    (the single option key type)
IV.   option surface                   (value carrier + option<> aliases)
V.    named options                    (ready-made common choices)
VI.   consumption                      (table_option_value + *_of<> readers)
VII.  is_table_option                  (detection: an option keyed by table_opt_key)
VIII. table_option_set                 (the set of table options + pack check)
IX.   axis pass-through                (is_container_axis_option, the mixed-pack
                                        partition: select_table_options_t /
                                        select_axis_options_t)
X.    concepts                         (C++20 analogs)
*/

#ifndef DJINTERP_CONTAINER_TABLE_OPTIONS_
#define DJINTERP_CONTAINER_TABLE_OPTIONS_ 1

// djinterp (always: the namespace macros + the D_ENV_* version macros)
#include "../../djinterp.hpp"       // NS_*, D_CONSTEXPR, D_CONSTEXPR_VAR, clean_t

//   The option surface (sections II onward) is C++17, and it names the option_set
// queries in template definitions, so its dependencies are pulled in here under
// the same guard.  The enum vocabulary above them stays usable on any standard.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // std
    #include <tuple>
    #include <type_traits>
    // djinterp
    #include "../../option/option.hpp"       // option<>, is_option, is_option_v
    #include "../../option/option_set.hpp"    // option_set<>, option_set_find / contains
    #include "../container_options.hpp"        // container_axis (the axis key type)
#endif

//   The policy DEFAULTS and the concepts gate below are configuration, so they
// are resolved in cfg_table.h and merely READ here -- the localization rule:
// a module contains no config logic, it consumes the answer.
#include "../../../config/core/container/table/cfg_table.h"


NS_DJINTERP


// ===========================================================================
// I.   Strictness / policy vocabulary
// ===========================================================================
//   The graded choices, as plain enums.  These name WHAT may vary; the option
// keys of section II bind a grade to a category.  Kept free of the option<>
// machinery so a runtime parse context can speak the same grades on any standard.

// table_strictness
//   enum: how a supplied count must match a required count -- the shared grade
// of the count-matching categories (a row's width against the declared columns;
// a multi-cell placeholder against the cells it fills).
enum class table_strictness
{
    exact,      // counts must match one-to-one; a mismatch is an error
    truncate,   // a surplus is dropped (the required count wins)
    pad,        // a shortfall is filled with the cell default (empty)
    lenient     // either a surplus or a shortfall is accepted silently
};

// type_policy
//   enum: whether a text cell must yield its declared column type.  Governs the
// text front end's cell-to-type step (an axis-typed table only).
enum class type_policy
{
    require,    // the cell text must parse to the column type, else an error
    coerce,     // parse if possible, otherwise fall back to the raw cell
    ignore      // do not type cells (a cell-homogeneous / string table)
};

// shape_policy
//   enum: which domain shapes I_T a declaration admits (Rectangular, jagged, and
// sparse tables; containers.tex).  Rectangular is the strictest and the shape of
// the existing table trio; jagged and sparse are opt-in.
enum class shape_policy
{
    rectangular,   // every row the same width; I_T a box
    jagged,        // rows of differing widths permitted
    sparse         // interior holes permitted (absent cells, not blank)
};

// header_policy
//   enum: how header rows and the header/body separator are treated.
enum class header_policy
{
    require_separator,   // a separator line is mandatory to open a header block
    optional_separator,  // headers allowed but not required
    no_headers           // reject header rows outright
};

// domain_policy
//   enum: whether a cell-value domain (a closed interval delta_I; Overlays) is
// enforced.  Off by default, mirroring constrained_table's optional domain.
enum class domain_policy
{
    ignore,    // no value-domain restriction
    enforce    // every cell must lie in the declared interval, else an error
};

// trim_policy
//   enum: whether surrounding whitespace is stripped from a text cell.
enum class trim_policy
{
    trim,      // strip leading and trailing whitespace from each cell
    keep       // preserve cell text verbatim
};

// pipe_policy
//   enum: whether the leading and trailing row delimiters are mandatory.
enum class pipe_policy
{
    require_borders,   // a row must open and close with the delimiter
    optional_borders   // outer delimiters may be omitted
};

// anchor_policy
//   enum: which position names a merged cell -- its anchor (containers.tex:
// anchor(C) = min_lex R_C; covered positions defer to it).
enum class anchor_policy
{
    lexicographic_least,   // the formal default: the lex-least covered index
    declared               // the position at which the merge is declared
};


// ===========================================================================
// II.  defaults
// ===========================================================================
//   The grade a category assumes when nothing sets it.  Each is READ from
// cfg_table.h, which owns the choice (D_CFG_TABLE_DEFAULT_*) and resolves it into
// the enumerator pasted below: one knob, one canonical home.
//
//   These sit with the VOCABULARY, not with the option surface, and deliberately:
// they are plain enum constants needing no option<> machinery, and BOTH surfaces
// must read the same ones -- the compile-time readers and the text front end's
// runtime dialect (parse_grid_options), which is C++11.  Gating them on C++17
// would have forced the runtime side to spell its own copy, which is exactly the
// drift a single source of truth exists to prevent.

//   The grade assumed for a category no option sets.  Each is READ from
// cfg_table.h, which owns the choice (D_CFG_TABLE_DEFAULT_*) and resolves it into
// the enumerator pasted below.  There is no default spelled here: one knob, one
// canonical home, per the config subframework's single-source-of-truth rule.
//
//   The shipped choices fail loud and stay rectangular -- so leniency is always
// opted into, never inherited by accident -- but a project may set any of them
// build-wide (-DD_CFG_TABLE_DEFAULT_SHAPE=D_CFG_TABLE_SHAPE_JAGGED, say).

D_CONSTEXPR_VAR table_strictness default_cell_count =
    table_strictness::D_INTERNAL_TABLE_DEFAULT_CELL_COUNT;
D_CONSTEXPR_VAR table_strictness default_placeholder_fit =
    table_strictness::D_INTERNAL_TABLE_DEFAULT_PLACEHOLDER_FIT;
D_CONSTEXPR_VAR type_policy default_type_conformance =
    type_policy::D_INTERNAL_TABLE_DEFAULT_TYPE_CONFORMANCE;
D_CONSTEXPR_VAR shape_policy default_shape =
    shape_policy::D_INTERNAL_TABLE_DEFAULT_SHAPE;
D_CONSTEXPR_VAR header_policy default_header =
    header_policy::D_INTERNAL_TABLE_DEFAULT_HEADER;
D_CONSTEXPR_VAR domain_policy default_domain =
    domain_policy::D_INTERNAL_TABLE_DEFAULT_DOMAIN;
D_CONSTEXPR_VAR trim_policy default_trim =
    trim_policy::D_INTERNAL_TABLE_DEFAULT_TRIM;
D_CONSTEXPR_VAR pipe_policy default_pipe =
    pipe_policy::D_INTERNAL_TABLE_DEFAULT_PIPE;
D_CONSTEXPR_VAR anchor_policy default_anchor =
    anchor_policy::D_INTERNAL_TABLE_DEFAULT_ANCHOR;


// ---------------------------------------------------------------------------
//   The option surface (sections II-IX) rides on option<>'s auto-NTTP key form,
// which -- like the option pack surface and container_options.hpp -- is C++17.
// The vocabulary above remains available below it.
// ---------------------------------------------------------------------------
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


// ===========================================================================
// III. table_opt_key
// ===========================================================================

// table_opt_key
//   enum: the single key type of every table option.  option_set requires one
// key_type across a set, so all table policies key off this one enum; the value
// travels in the option's arg (section III).
enum class table_opt_key
{
    cell_count,        // table_strictness: a row's width vs the declared columns
    placeholder_fit,   // table_strictness: a multi-cell placeholder vs its cells
    type_conformance,  // type_policy:      text cell -> column type
    shape,             // shape_policy:     admissible domain I_T
    header,            // header_policy:    header rows and separator
    domain,            // domain_policy:    cell-value interval
    trim,              // trim_policy:      cell whitespace
    pipe,              // pipe_policy:      row border delimiters
    anchor             // anchor_policy:    merged-cell anchor
};


// ===========================================================================
// IV.  option surface
// ===========================================================================
//   The value carrier and the per-category option<> aliases.  An option's value
// is a compile-time grade, carried as an integral_constant arg so the option
// stays a pure type (no runtime state) and reads back through section VI.

// table_opt_arg
//   type: carries a scalar grade _V (an enum of section I) as an option arg.
template<auto _V>
using table_opt_arg = std::integral_constant<decltype(_V), _V>;

// cell_count_opt / placeholder_fit_opt
//   option: bind a table_strictness grade to a count-matching category.
template<table_strictness _S>
using cell_count_opt =
    option<table_opt_key::cell_count, table_opt_arg<_S>>;

template<table_strictness _S>
using placeholder_fit_opt =
    option<table_opt_key::placeholder_fit, table_opt_arg<_S>>;

// type_opt / shape_opt / header_opt / domain_opt / trim_opt / pipe_opt / anchor_opt
//   option: bind a policy grade to its category.
template<type_policy _P>
using type_opt =
    option<table_opt_key::type_conformance, table_opt_arg<_P>>;

template<shape_policy _P>
using shape_opt =
    option<table_opt_key::shape, table_opt_arg<_P>>;

template<header_policy _P>
using header_opt =
    option<table_opt_key::header, table_opt_arg<_P>>;

template<domain_policy _P>
using domain_opt =
    option<table_opt_key::domain, table_opt_arg<_P>>;

template<trim_policy _P>
using trim_opt =
    option<table_opt_key::trim, table_opt_arg<_P>>;

template<pipe_policy _P>
using pipe_opt =
    option<table_opt_key::pipe, table_opt_arg<_P>>;

template<anchor_policy _P>
using anchor_opt =
    option<table_opt_key::anchor, table_opt_arg<_P>>;


// ===========================================================================
// V.   named options
// ===========================================================================
//   Ready-made options for the common choices, so a declaration reads plainly:
//     table_builder<columns<...>, allow_jagged, lenient_counts>
//   Only the frequent picks are named; the section-III aliases cover the rest.

using strict_counts     = cell_count_opt<table_strictness::exact>;
using truncate_counts   = cell_count_opt<table_strictness::truncate>;
using pad_counts        = cell_count_opt<table_strictness::pad>;
using lenient_counts    = cell_count_opt<table_strictness::lenient>;

using strict_types      = type_opt<type_policy::require>;
using coerce_types      = type_opt<type_policy::coerce>;
using untyped_cells     = type_opt<type_policy::ignore>;

using rectangular_only  = shape_opt<shape_policy::rectangular>;
using allow_jagged      = shape_opt<shape_policy::jagged>;
using allow_sparse      = shape_opt<shape_policy::sparse>;

using require_headers   = header_opt<header_policy::require_separator>;
using optional_headers  = header_opt<header_policy::optional_separator>;
using no_headers        = header_opt<header_policy::no_headers>;

using enforce_domain    = domain_opt<domain_policy::enforce>;
using ignore_domain     = domain_opt<domain_policy::ignore>;

using trim_cells        = trim_opt<trim_policy::trim>;
using keep_whitespace   = trim_opt<trim_policy::keep>;

using require_borders   = pipe_opt<pipe_policy::require_borders>;
using optional_borders  = pipe_opt<pipe_policy::optional_borders>;


// ===========================================================================
// VI.  consumption
// ===========================================================================
//   How the two front ends read a policy out of an option set.  table_option_value
// looks the key up in the set and returns the carried grade, or _Default when the
// key is absent; the *_of<> shorthands pin each key to its grade type and default
// so a call site reads e.g. shape_of<Opts>.  _Set must be an option_set<...>.

NS_INTERNAL

    // table_opt_extract
    //   trait: the scalar grade carried by an option whose sole arg is an
    // integral_constant (every table option, per section III).
    template<typename _Option>
    struct table_opt_extract
    {
        using arg0 = std::tuple_element_t<0, typename _Option::args_type>;

        static D_CONSTEXPR auto value = arg0::value;
    };

    // table_opt_lookup
    //   trait: dispatch on presence.  The primary (absent) yields the default;
    // the specialization (present) extracts.  Only the taken branch instantiates
    // the extractor, so an absent key never touches option_set_find_t.
    template<bool           _Found,
             typename       _Set,
             table_opt_key  _Key,
             auto           _Default>
    struct table_opt_lookup
    {
        static D_CONSTEXPR auto value = _Default;
    };

    template<typename       _Set,
             table_opt_key  _Key,
             auto           _Default>
    struct table_opt_lookup<true, _Set, _Key, _Default>
    {
        static D_CONSTEXPR auto value =
            table_opt_extract<option_set_find_t<_Set, _Key>>::value;
    };

NS_END  // internal


// table_option_value
//   trait: the grade bound to _Key in _Set, or _Default when _Set does not carry
// _Key.  The single read the front ends route every policy query through.
template<typename       _Set,
         table_opt_key  _Key,
         auto           _Default>
struct table_option_value
{
    static D_CONSTEXPR auto value =
        internal::table_opt_lookup<option_set_contains_v<_Set, _Key>,
                                   _Set,
                                   _Key,
                                   _Default>::value;
};

// table_option_value_v
//   value: shorthand for table_option_value<...>::value.
template<typename       _Set,
         table_opt_key  _Key,
         auto           _Default>
D_CONSTEXPR_VAR auto table_option_value_v =
    table_option_value<_Set, _Key, _Default>::value;


// per-category readers
//   each pins a key to its grade type and default, so a call site names only the
// set: cell_count_of<Opts>, shape_of<Opts>, and so on.

template<typename _Set>
D_CONSTEXPR_VAR table_strictness cell_count_of =
    table_option_value_v<_Set, table_opt_key::cell_count, default_cell_count>;

template<typename _Set>
D_CONSTEXPR_VAR table_strictness placeholder_fit_of =
    table_option_value_v<_Set,
                         table_opt_key::placeholder_fit,
                         default_placeholder_fit>;

template<typename _Set>
D_CONSTEXPR_VAR type_policy type_conformance_of =
    table_option_value_v<_Set,
                         table_opt_key::type_conformance,
                         default_type_conformance>;

template<typename _Set>
D_CONSTEXPR_VAR shape_policy shape_of =
    table_option_value_v<_Set, table_opt_key::shape, default_shape>;

template<typename _Set>
D_CONSTEXPR_VAR header_policy header_of =
    table_option_value_v<_Set, table_opt_key::header, default_header>;

template<typename _Set>
D_CONSTEXPR_VAR domain_policy domain_of =
    table_option_value_v<_Set, table_opt_key::domain, default_domain>;

template<typename _Set>
D_CONSTEXPR_VAR trim_policy trim_of =
    table_option_value_v<_Set, table_opt_key::trim, default_trim>;

template<typename _Set>
D_CONSTEXPR_VAR pipe_policy pipe_of =
    table_option_value_v<_Set, table_opt_key::pipe, default_pipe>;

template<typename _Set>
D_CONSTEXPR_VAR anchor_policy anchor_of =
    table_option_value_v<_Set, table_opt_key::anchor, default_anchor>;


// ===========================================================================
// VII. is_table_option
// ===========================================================================

NS_INTERNAL

    // is_table_option_helper
    //   trait: primary (a non-option, or an option keyed by another type).
    template<typename _Type,
             bool     _IsOption = is_option_v<_Type>>
    struct is_table_option_helper : std::false_type
    {};

    // is_table_option_helper (an option): true iff its key_type is table_opt_key.
    template<typename _Type>
    struct is_table_option_helper<_Type, true>
        : std::integral_constant<bool,
            std::is_same<typename _Type::key_type, table_opt_key>::value>
    {};

NS_END  // internal

// is_table_option
//   trait: true iff _Type is an option<> keyed by table_opt_key -- a member of
// this vocabulary, as opposed to a container axis option or a foreign option.
// table_builder partitions a mixed option pack on this trait.
template<typename _Type>
struct is_table_option
    : internal::is_table_option_helper<clean_t<_Type>>
{};

// is_table_option_v
//   value: shorthand for is_table_option<_Type>::value.
template<typename _Type>
D_CONSTEXPR_VAR bool is_table_option_v = is_table_option<_Type>::value;


// ===========================================================================
// VIII. table_option_set
// ===========================================================================

NS_INTERNAL

    // all_table_options
    //   trait: every type in the pack is a table option.
    template<typename...>
    struct all_table_options : std::true_type
    {};

    template<typename    _First,
             typename... _Rest>
    struct all_table_options<_First, _Rest...>
        : std::integral_constant<bool,
            ( is_table_option_v<_First> &&
              all_table_options<_Rest...>::value )>
    {};

NS_END  // internal

// is_table_option_pack
//   trait: true iff every type in the pack is a table option.  table_builder
// asserts this before treating a pack as a table option set, giving a specific
// diagnostic where option_set's own single-key-type check would give a general
// one.  (A pack that mixes a foreign key_type in among table options is caught by
// option_set itself; this pins the intent -- "these are meant to be table
// options" -- for the pure-foreign-pack case that check cannot see.)
template<typename... _Options>
struct is_table_option_pack
    : internal::all_table_options<_Options...>
{};

// is_table_option_pack_v
//   value: shorthand for is_table_option_pack<_Options...>::value.
template<typename... _Options>
D_CONSTEXPR_VAR bool is_table_option_pack_v =
    is_table_option_pack<_Options...>::value;

// table_option_set
//   type: the set of table options.  A plain alias over option_set<> -- IDENTITY,
// not a subclass -- so the section-VI readers (which pattern-match option_set<>)
// consume it directly.  Every entry shares key_type table_opt_key, so option_set's
// single-key-type check rejects a stray foreign option for free; is_table_option_pack
// above is the tighter, intent-pinning check for table_builder to assert on.
//
// Example:
//   using opts = table_option_set<allow_jagged, lenient_counts, trim_cells>;
//   static_assert(shape_of<opts>      == shape_policy::jagged);
//   static_assert(cell_count_of<opts> == table_strictness::lenient);
template<typename... _Options>
using table_option_set = option_set<_Options...>;


// ===========================================================================
// IX.  axis pass-through (the mixed-pack partition)
// ===========================================================================
//   A declaration configures TWO things at once: how the DSL reads it (the table
// policies above) and what the realized container is (the universal axis
// positions of container_options.hpp -- lifetime, bounds, ordering, ...).  They
// cannot share a set: option_set resolves keys with == and so requires one
// key_type per set, and the two vocabularies key off different enums
// (table_opt_key vs container_axis).  A mixed pack is therefore PARTITIONED --
// each half lifted into its own set -- and each half read by its own reader
// (the section-VI *_of<> shorthands here; container_axis_value_v there).
//
//   The axis aliases themselves need no re-export: container_opt_bounds<>,
// container_opt_lifetime<>, and their siblings are already in this namespace and
// drop straight into a declaration beside the table options.
//
// Example:
//   //   a jagged-tolerant declaration whose container must read as bounded
//   using pack = ...<allow_jagged, container_opt_bounds<container_bounds::bounded>>;
//   using t_opts = select_table_options_t<pack...>;   // -> the table policies
//   using a_opts = select_axis_options_t<pack...>;    // -> the container axes
//   shape_of<t_opts>;                                  // jagged
//   container_axis_value_v<a_opts, container_axis::bounds, container_bounds::unbounded>;

NS_INTERNAL

    // is_axis_option_helper
    //   trait: primary (a non-option, or an option keyed by another type).
    template<typename _Type,
             bool     _IsOption = is_option_v<_Type>>
    struct is_axis_option_helper : std::false_type
    {};

    // is_axis_option_helper (an option): true iff its key_type is container_axis.
    template<typename _Type>
    struct is_axis_option_helper<_Type, true>
        : std::integral_constant<bool,
            std::is_same<typename _Type::key_type, container_axis>::value>
    {};

NS_END  // internal

// is_container_axis_option
//   trait: true iff _Type is an option<> keyed by container_axis -- a universal
// axis position (container_options.hpp), as opposed to a table policy.
template<typename _Type>
struct is_container_axis_option
    : internal::is_axis_option_helper<clean_t<_Type>>
{};

// is_container_axis_option_v
//   value: shorthand for is_container_axis_option<_Type>::value.
template<typename _Type>
D_CONSTEXPR_VAR bool is_container_axis_option_v =
    is_container_axis_option<_Type>::value;

NS_INTERNAL

    // lift_to_set
    //   trait: lift a std::tuple of options into an option_set.  Local to keep
    // this header's dependency footprint at option.hpp + option_set.hpp (the
    // option subframework's own tuple_to_option_set_t lives in option_builder.hpp,
    // which drags in the meta partition machinery this header does not need).
    template<typename _Tuple>
    struct lift_to_set;

    template<typename... _Options>
    struct lift_to_set<std::tuple<_Options...>>
    {
        using type = option_set<_Options...>;
    };

    // filter_options
    //   trait: the entries of a pack satisfying _Pred, accumulated in order.  The
    // conditional picks the recursive step WITHOUT instantiating the branch not
    // taken, so a rejected entry costs nothing.
    template<template<typename> class _Pred,
             typename                 _Acc,
             typename...              _Rest>
    struct filter_options;

    template<template<typename> class _Pred,
             typename...              _Acc>
    struct filter_options<_Pred, std::tuple<_Acc...>>
    {
        using type = std::tuple<_Acc...>;
    };

    template<template<typename> class _Pred,
             typename...              _Acc,
             typename                 _Head,
             typename...              _Tail>
    struct filter_options<_Pred, std::tuple<_Acc...>, _Head, _Tail...>
    {
        using type =
            typename std::conditional<
                _Pred<_Head>::value,
                filter_options<_Pred, std::tuple<_Acc..., _Head>, _Tail...>,
                filter_options<_Pred, std::tuple<_Acc...>,        _Tail...>
            >::type::type;
    };

NS_END  // internal

// select_table_options_t
//   type: the TABLE options of a mixed pack, as a set.  Everything that is not a
// table option -- an axis option, a declarator, anything else -- is dropped, so a
// declaration may interleave the two vocabularies freely and each consumer takes
// only its own half.
template<typename... _Pack>
using select_table_options_t =
    typename internal::lift_to_set<
        typename internal::filter_options<
            is_table_option, std::tuple<>, _Pack...>::type>::type;

// select_axis_options_t
//   type: the container AXIS options of a mixed pack, as a set -- the half
// container_axis_value_v reads (container_options.hpp).
template<typename... _Pack>
using select_axis_options_t =
    typename internal::lift_to_set<
        typename internal::filter_options<
            is_container_axis_option, std::tuple<>, _Pack...>::type>::type;

// is_any_table_config
//   trait: true iff _Type is an option this header knows how to route -- a table
// policy or a container axis position.  The test a declarator fold uses to tell
// a configuration entry from a structural one.
template<typename _Type>
struct is_any_table_config
    : std::integral_constant<bool,
        ( is_table_option_v<_Type> ||
          is_container_axis_option_v<_Type> )>
{};

// is_any_table_config_v
//   value: shorthand for is_any_table_config<_Type>::value.
template<typename _Type>
D_CONSTEXPR_VAR bool is_any_table_config_v = is_any_table_config<_Type>::value;


// ===========================================================================
// X.   concepts   (C++20 analogs)
// ===========================================================================

#if D_INTERNAL_TABLE_CONCEPTS

// TableOption
//   concept: satisfied iff _Type is a table option (keyed by table_opt_key).
// Parallels is_table_option_v.
template<typename _Type>
concept TableOption = is_table_option_v<_Type>;

// ContainerAxisOption
//   concept: satisfied iff _Type is a universal axis option (keyed by
// container_axis).  Parallels is_container_axis_option_v.
template<typename _Type>
concept ContainerAxisOption = is_container_axis_option_v<_Type>;

#endif  // D_INTERNAL_TABLE_CONCEPTS


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TABLE_OPTIONS_
