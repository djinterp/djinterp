/******************************************************************************
* djinterp [container]                                        table_common.hpp
*
*   Shared vocabulary for the entire table family.  Houses the option keys,
* default tags, and the `table_base` mixin that turns an option pack into
* the canonical alias surface (`value_type`, `allocator_type`, `key_compare`,
* `hasher`, `size_interval`, `lock_type`, `underlying_container_type`, ...)
* that the twelve-axis container trait machinery already detects.
*   This header defines no concrete container.  It defines the contract:
*   1. Axis option keys (one tag type per classification axis)
*   2. Default tags (placeholder shapes for unset axes)
*   3. `table_base<_Type, _Allocator, _Options...>` - resolver mixin
*   Every table-family header (table.hpp, table_overlay.hpp, database_table,
* lookup_table, registry_table) depends on this single vocabulary.  Types
* derived from `table_base` automatically light up the twelve-axis trait
* surface without any extra wiring - the alias surface is the wiring.
*   AXIS / KEY MAP:
*     axis 1  lifetime       -> lifetime_key
*     axis 3  ordering       -> comparator_key (sorted) | hasher_key (hashed)
*     axis 4  bounds         -> bounds_key | size_interval_key
*     axis 5  multiplicity   -> multiplicity_key | multiplicity_interval_key
*     axis 6  structure      -> structure_key (flat / hierarchical)
*                               + node_type_key, depth_type_key (hierarchical)
*     axis 7  storage        -> storage_key (static / dynamic)
*     axis 8  thread safety  -> thread_safety_key
*                               + lock_type_key, mutex_type_key
*     axis 9  underlying     -> underlying_key
*     <table specific>       -> config_key (header / footer / span layout)
*                               rank_key  (multi-dimensional extension)
*     <foundational>         -> size_type_key, difference_type_key,
*                               iterator_key, const_iterator_key
*   `value_type` and `allocator_type` are positional template parameters of
* `table_base`, not keyed options - they match the std::vector convention
* and account for >90% of call sites.  Everything else is keyed.
*   CONDITIONAL ALIAS SURFACING:
*   Each axis whose alias is a *detection probe* in the trait machinery is
* surfaced via its own mixin (`with_comparator`, `with_hasher`, ...).  The
* mixin defines the alias only when the user opted into that axis - so a
* table without a comparator_key has no `::key_compare`, which means
* `is_sorted_container_v` correctly returns false on it.
* DEPENDENCIES:
*   djinterp.hpp           - NS_DJINTERP, D_CONSTEXPR
*   meta/type_traits.hpp   - clean_t, void_t
*   options/option.hpp     - option_list, option<>
*   options/with_options.hpp - with_options_pack<>
*
*
* path:      /inc/djinterp/container/table/table_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    axis option keys
II.   default tags
III.  empty config
IV.   conditional alias mixins
      1. with_comparator
      2. with_hasher
      3. with_size_interval
      4. with_multiplicity_interval
      5. with_depth_interval
      6. with_node_type
      7. with_depth_type
      8. with_lock_type
      9. with_mutex_type
     10. with_underlying
     11. with_config
V.    table_base (always-on aliases + mixin aggregation)
VI.   convenience aliases
*/

#ifndef DJINTERP_TABLE_COMMON_
#define DJINTERP_TABLE_COMMON_ 1

#include <cstddef>
#include <memory>
#include <type_traits>
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../options/option.hpp"
#include "../../options/with_options.hpp"


NS_DJINTERP


    // =========================================================================
    // I.   AXIS OPTION KEYS
    // =========================================================================
    //   Empty tag types.  Used as keys with the options machinery.  Every
    // call site reads as `key_name` (no value) when used as a flag, or as
    // `key_name, value_type` when used as a positional K-V pair.

    // ------ foundational ----------------------------------------------------

    // size_type_key
    //   tag: identifies the option carrying ::size_type (default std::size_t).
    struct size_type_key
    {};

    // difference_type_key
    //   tag: identifies the option carrying ::difference_type
    // (default std::ptrdiff_t).
    struct difference_type_key
    {};

    // iterator_key
    //   tag: identifies the option carrying ::iterator.
    struct iterator_key
    {};

    // const_iterator_key
    //   tag: identifies the option carrying ::const_iterator.
    struct const_iterator_key
    {};


    // ------ axis 1 - lifetime -----------------------------------------------

    // lifetime_key
    //   tag: identifies the option carrying the lifetime tag
    // (constexpr_storage / immutable / mutable_storage).
    struct lifetime_key
    {};


    // ------ axis 3 - ordering -----------------------------------------------

    // comparator_key
    //   tag: identifies the option carrying ::key_compare.  Surfacing this
    // alias makes the type detect as `is_sorted_container_v` (when no
    // hasher is also surfaced).
    struct comparator_key
    {};

    // hasher_key
    //   tag: identifies the option carrying ::hasher.  Surfacing this alias
    // makes the type detect as unordered-hashed and BLOCKS sorted
    // detection.
    struct hasher_key
    {};


    // ------ axis 4 - bounds -------------------------------------------------

    // bounds_key
    //   tag: identifies the option carrying a coarse bounds tag
    // (bounded_tag / unbounded_tag).  Useful when a full size_interval is
    // not needed.
    struct bounds_key
    {};

    // size_interval_key
    //   tag: identifies the option carrying ::size_interval (an instance
    // of maths::interval).  Surfacing this alias activates the
    // interval-aware bounded detection path.
    struct size_interval_key
    {};


    // ------ axis 5 - multiplicity -------------------------------------------

    // multiplicity_key
    //   tag: identifies the option carrying a coarse multiplicity tag
    // (unique_tag / multi_tag).
    struct multiplicity_key
    {};

    // multiplicity_interval_key
    //   tag: identifies the option carrying ::multiplicity_interval.
    // Required for bounded-N multiplicity detection.
    struct multiplicity_interval_key
    {};


    // ------ axis 6 - structure ----------------------------------------------

    // structure_key
    //   tag: identifies the option carrying the structure tag (flat_tag /
    // hierarchical_tag).
    struct structure_key
    {};

    // node_type_key
    //   tag: identifies the option carrying ::node_type (hierarchical
    // detection).
    struct node_type_key
    {};

    // depth_type_key
    //   tag: identifies the option carrying ::depth_type.
    struct depth_type_key
    {};

    // depth_interval_key
    //   tag: identifies the option carrying ::depth_interval (bounded
    // depth).
    struct depth_interval_key
    {};


    // ------ axis 7 - storage ------------------------------------------------

    // storage_key
    //   tag: identifies the option carrying the storage tag
    // (static_storage_tag / dynamic_storage_tag).
    struct storage_key
    {};


    // ------ axis 8 - thread safety ------------------------------------------

    // thread_safety_key
    //   tag: identifies the option carrying the thread-safety level enum
    // value (DThreadSafetyLevel::none ... shared_timed).
    struct thread_safety_key
    {};

    // lock_type_key
    //   tag: identifies the option carrying ::lock_type.
    struct lock_type_key
    {};

    // mutex_type_key
    //   tag: identifies the option carrying ::mutex_type.
    struct mutex_type_key
    {};


    // ------ axis 9 - underlying ---------------------------------------------

    // underlying_key
    //   tag: identifies the option carrying ::underlying_container_type.
    // Surfacing this alias classifies the type as an overlay.
    struct underlying_key
    {};


    // ------ table-specific --------------------------------------------------

    // config_key
    //   tag: identifies the option carrying the table region/layout config
    // (headers, footers, spans, splits, partitions).
    struct config_key
    {};

    // rank_key
    //   tag: identifies the option carrying the multi-dimensional rank
    // (std::integral_constant<std::size_t, N>).  Default rank is 1.
    struct rank_key
    {};


    // =========================================================================
    // II.  DEFAULT TAGS
    // =========================================================================
    //   Concrete placeholder shapes for axes the user did not configure.
    // These tags are detected by the trait system as "explicitly default"
    // (vs. the "alias absent" case which is detected as "axis not opted
    // into").

    // ------ axis 1 ----------------------------------------------------------

    // mutable_lifetime_tag
    //   tag: lifetime axis - dynamically mutable.  Default for tables.
    struct mutable_lifetime_tag
    {};

    // immutable_lifetime_tag
    //   tag: lifetime axis - immutable after construction.
    struct immutable_lifetime_tag
    {};

    // constexpr_lifetime_tag
    //   tag: lifetime axis - constexpr storage.
    struct constexpr_lifetime_tag
    {};


    // ------ axis 4 ----------------------------------------------------------

    // bounded_tag
    //   tag: bounds axis - container has a size cap (full interval lives
    // in size_interval_key).
    struct bounded_tag
    {};

    // unbounded_tag
    //   tag: bounds axis - container has no size cap.  Default.
    struct unbounded_tag
    {};


    // ------ axis 5 ----------------------------------------------------------

    // unique_tag
    //   tag: multiplicity axis - duplicates not allowed.
    struct unique_tag
    {};

    // multi_tag
    //   tag: multiplicity axis - duplicates allowed.  Default.
    struct multi_tag
    {};


    // ------ axis 6 ----------------------------------------------------------

    // flat_tag
    //   tag: structure axis - flat sequence.  Default.
    struct flat_tag
    {};

    // hierarchical_tag
    //   tag: structure axis - tree / graph.
    struct hierarchical_tag
    {};


    // ------ axis 7 ----------------------------------------------------------

    // dynamic_storage_tag
    //   tag: storage axis - heap-allocated, growable.  Default.
    struct dynamic_storage_tag
    {};

    // static_storage_tag
    //   tag: storage axis - compile-time-fixed capacity.
    struct static_storage_tag
    {};


    // =========================================================================
    // III. EMPTY CONFIG
    // =========================================================================

    // empty_config
    //   struct: minimal config used as the default for config_key.  No
    // headers, footers, totals, spans, splits, or partitions.
    //   This is the same `empty_config` already referenced by
    // database_table; included here for self-containment so that headers
    // depending only on table_common.hpp can resolve config_type without
    // pulling in table_traits.hpp.
    struct empty_config
    {};


    // =========================================================================
    // IV.  CONDITIONAL ALIAS MIXINS
    // =========================================================================
    //   Each mixin surfaces exactly ONE alias and only when the
    // corresponding option key is present in the option pack.  This keeps
    // the trait surface honest: a table that never opted into a
    // comparator has no `::key_compare`, so `is_sorted_container_v`
    // correctly returns false on it.

    NS_INTERNAL

        // -----------------------------------------------------------------
        //  1. with_comparator -> ::key_compare
        // -----------------------------------------------------------------

        // with_comparator
        //   trait: empty primary - user did not opt into comparator_key.
        template<typename _Opts,
                 bool     _Has = _Opts::template has_option_v<comparator_key>>
        struct with_comparator
        {};

        // with_comparator (specialization)
        //   trait: surfaces ::key_compare from comparator_key.
        template<typename _Opts>
        struct with_comparator<_Opts, true>
        {
            using key_compare =
                typename _Opts::template option_t<comparator_key>;
        };


        // -----------------------------------------------------------------
        //  2. with_hasher -> ::hasher
        // -----------------------------------------------------------------

        // with_hasher
        //   trait: empty primary.
        template<typename _Opts,
                 bool     _Has = _Opts::template has_option_v<hasher_key>>
        struct with_hasher
        {};

        // with_hasher (specialization)
        //   trait: surfaces ::hasher from hasher_key.
        template<typename _Opts>
        struct with_hasher<_Opts, true>
        {
            using hasher =
                typename _Opts::template option_t<hasher_key>;
        };


        // -----------------------------------------------------------------
        //  3. with_size_interval -> ::size_interval
        // -----------------------------------------------------------------

        // with_size_interval
        //   trait: empty primary.
        template<typename _Opts,
                 bool     _Has = _Opts::template has_option_v<size_interval_key>>
        struct with_size_interval
        {};

        // with_size_interval (specialization)
        //   trait: surfaces ::size_interval from size_interval_key.
        template<typename _Opts>
        struct with_size_interval<_Opts, true>
        {
            using size_interval =
                typename _Opts::template option_t<size_interval_key>;
        };


        // -----------------------------------------------------------------
        //  4. with_multiplicity_interval -> ::multiplicity_interval
        // -----------------------------------------------------------------

        // with_multiplicity_interval
        //   trait: empty primary.
        template<typename _Opts,
                 bool     _Has = _Opts::template has_option_v<
                     multiplicity_interval_key>>
        struct with_multiplicity_interval
        {};

        // with_multiplicity_interval (specialization)
        //   trait: surfaces ::multiplicity_interval.
        template<typename _Opts>
        struct with_multiplicity_interval<_Opts, true>
        {
            using multiplicity_interval =
                typename _Opts::template option_t<multiplicity_interval_key>;
        };


        // -----------------------------------------------------------------
        //  5. with_depth_interval -> ::depth_interval
        // -----------------------------------------------------------------

        // with_depth_interval
        //   trait: empty primary.
        template<typename _Opts,
                 bool     _Has = _Opts::template has_option_v<depth_interval_key>>
        struct with_depth_interval
        {};

        // with_depth_interval (specialization)
        //   trait: surfaces ::depth_interval.
        template<typename _Opts>
        struct with_depth_interval<_Opts, true>
        {
            using depth_interval =
                typename _Opts::template option_t<depth_interval_key>;
        };


        // -----------------------------------------------------------------
        //  6. with_node_type -> ::node_type
        // -----------------------------------------------------------------

        // with_node_type
        //   trait: empty primary.
        template<typename _Opts,
                 bool     _Has = _Opts::template has_option_v<node_type_key>>
        struct with_node_type
        {};

        // with_node_type (specialization)
        //   trait: surfaces ::node_type for hierarchical detection.
        template<typename _Opts>
        struct with_node_type<_Opts, true>
        {
            using node_type =
                typename _Opts::template option_t<node_type_key>;
        };


        // -----------------------------------------------------------------
        //  7. with_depth_type -> ::depth_type
        // -----------------------------------------------------------------

        // with_depth_type
        //   trait: empty primary.
        template<typename _Opts,
                 bool     _Has = _Opts::template has_option_v<depth_type_key>>
        struct with_depth_type
        {};

        // with_depth_type (specialization)
        //   trait: surfaces ::depth_type.
        template<typename _Opts>
        struct with_depth_type<_Opts, true>
        {
            using depth_type =
                typename _Opts::template option_t<depth_type_key>;
        };


        // -----------------------------------------------------------------
        //  8. with_lock_type -> ::lock_type
        // -----------------------------------------------------------------

        // with_lock_type
        //   trait: empty primary.
        template<typename _Opts,
                 bool     _Has = _Opts::template has_option_v<lock_type_key>>
        struct with_lock_type
        {};

        // with_lock_type (specialization)
        //   trait: surfaces ::lock_type for thread-safety detection.
        template<typename _Opts>
        struct with_lock_type<_Opts, true>
        {
            using lock_type =
                typename _Opts::template option_t<lock_type_key>;
        };


        // -----------------------------------------------------------------
        //  9. with_mutex_type -> ::mutex_type
        // -----------------------------------------------------------------

        // with_mutex_type
        //   trait: empty primary.
        template<typename _Opts,
                 bool     _Has = _Opts::template has_option_v<mutex_type_key>>
        struct with_mutex_type
        {};

        // with_mutex_type (specialization)
        //   trait: surfaces ::mutex_type.
        template<typename _Opts>
        struct with_mutex_type<_Opts, true>
        {
            using mutex_type =
                typename _Opts::template option_t<mutex_type_key>;
        };


        // -----------------------------------------------------------------
        // 10. with_underlying -> ::underlying_container_type
        // -----------------------------------------------------------------

        // with_underlying
        //   trait: empty primary.
        template<typename _Opts,
                 bool     _Has = _Opts::template has_option_v<underlying_key>>
        struct with_underlying
        {};

        // with_underlying (specialization)
        //   trait: surfaces ::underlying_container_type.  Activates
        // axis-9 (overlay) classification.
        template<typename _Opts>
        struct with_underlying<_Opts, true>
        {
            using underlying_container_type =
                typename _Opts::template option_t<underlying_key>;
        };


        // -----------------------------------------------------------------
        // 11. with_config -> ::config_type
        // -----------------------------------------------------------------

        // with_config
        //   trait: surfaces ::config_type.  Always present (defaults to
        // empty_config) so generic table code can rely on it.
        template<typename _Opts>
        struct with_config
        {
            using config_type =
                typename _Opts::template option_t<config_key, empty_config>;
        };


        // -----------------------------------------------------------------
        //  axes_always
        // -----------------------------------------------------------------

        // axes_always
        //   trait: surfaces the foundational always-on alias surface.
        // value_type and allocator_type come from the positional template
        // parameters; size_type, difference_type, iterator, and
        // const_iterator come from the option pack with sensible defaults.
        template<typename    _Type,
                 typename    _Allocator,
                 typename    _Opts,
                 typename    _DefaultIterator,
                 typename    _DefaultConstIterator>
        struct axes_always
        {
            using value_type      = _Type;
            using allocator_type  = _Allocator;
            using size_type       =
                typename _Opts::template option_t<size_type_key,
                                                  std::size_t>;
            using difference_type =
                typename _Opts::template option_t<difference_type_key,
                                                  std::ptrdiff_t>;
            using iterator        =
                typename _Opts::template option_t<iterator_key,
                                                  _DefaultIterator>;
            using const_iterator  =
                typename _Opts::template option_t<const_iterator_key,
                                                  _DefaultConstIterator>;

            using reference       = value_type&;
            using const_reference = const value_type&;
            using pointer         =
                typename std::allocator_traits<_Allocator>::pointer;
            using const_pointer   =
                typename std::allocator_traits<_Allocator>::const_pointer;
        };

    NS_END  // internal


    // =========================================================================
    // V.   table_base
    // =========================================================================
    //   Stateless mixin that takes a positional `_Type` and `_Allocator`
    // plus a variadic `_Options...` pack and publishes the canonical alias
    // surface picked up by the twelve-axis trait machinery.
    //
    //   `_DefaultIterator` and `_DefaultConstIterator` let derived classes
    // (table, table_overlay, lookup_table, etc.) plug in their preferred
    // default iterator types without forcing a pull-in from this header.

    // table_base
    //   class: aggregates the always-on alias mixin and all conditional
    // alias mixins under a single public surface.  Inherits publicly from
    // with_options_pack so that ::options_type, ::option_count,
    // ::has_option_v<>, and ::option_t<> are all directly available.
    template<typename _Type,
             typename _Allocator           = std::allocator<_Type>,
             typename _DefaultIterator     = _Type*,
             typename _DefaultConstIterator = const _Type*,
             typename... _Options>
    class table_base
        : public with_options_pack<_Options...>,
          public internal::axes_always<_Type,
                                       _Allocator,
                                       with_options_pack<_Options...>,
                                       _DefaultIterator,
                                       _DefaultConstIterator>,
          public internal::with_comparator<with_options_pack<_Options...>>,
          public internal::with_hasher<with_options_pack<_Options...>>,
          public internal::with_size_interval<with_options_pack<_Options...>>,
          public internal::with_multiplicity_interval<
              with_options_pack<_Options...>>,
          public internal::with_depth_interval<with_options_pack<_Options...>>,
          public internal::with_node_type<with_options_pack<_Options...>>,
          public internal::with_depth_type<with_options_pack<_Options...>>,
          public internal::with_lock_type<with_options_pack<_Options...>>,
          public internal::with_mutex_type<with_options_pack<_Options...>>,
          public internal::with_underlying<with_options_pack<_Options...>>,
          public internal::with_config<with_options_pack<_Options...>>
    {
    public:
        // -----------------------------------------------------------------
        //  rank
        //   value: multi-dimensional rank.  1 by default.
        //   Resolved via ::option_t<rank_key, integral_constant<size_t,1>>.
        // -----------------------------------------------------------------
        using rank_constant =
            typename with_options_pack<_Options...>::template option_t<
                rank_key,
                std::integral_constant<std::size_t, 1>>;

        static constexpr std::size_t rank = rank_constant::value;
    };


    // =========================================================================
    // VI.  CONVENIENCE ALIASES
    // =========================================================================

    // resolve_option_t
    //   type: shorthand for "look up _Key in the normalized form of
    // _Options..., falling back to _Default".  Useful inside derived
    // classes that need to peek at axes without going through inheritance.
    template<typename    _Key,
             typename    _Default,
             typename... _Options>
    using resolve_option_t =
        option_list_lookup_t<normalize_options_t<_Options...>,
                             _Key,
                             _Default>;

    // has_option_in
    //   trait: shorthand for "is _Key present in the normalized form of
    // _Options...".
    template<typename    _Key,
             typename... _Options>
    struct has_option_in
        : std::integral_constant<bool,
            option_list_contains<normalize_options_t<_Options...>,
                                 _Key>::value>
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename    _Key,
             typename... _Options>
    constexpr bool has_option_in_v = has_option_in<_Key, _Options...>::value;
#endif


NS_END  // djinterp


#endif  // DJINTERP_TABLE_COMMON_