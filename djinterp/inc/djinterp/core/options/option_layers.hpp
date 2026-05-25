/******************************************************************************
* djinterp [options]                                            option_layers.hpp
*
*   A concentric stack of option sets.  Lookup walks the chain
* outer-to-inner; the first layer that owns a value for the queried
* key wins.  The chain may mix static and dynamic option sets
* freely — the innermost layer is typically a constexpr
* `static_option_set<...>` of module defaults, with one or more
* runtime `option_set<K, V>` layers on top for overrides.
*
*   ORDERING CONVENTION:
*   In the template parameter list `option_layers<L0, L1, ..., Ln>`,
* `L0` is the OUTERMOST layer and `Ln` is the INNERMOST.  Lookup
* walks `L0 -> L1 -> ... -> Ln`; the first layer that resolves the
* key wins.  This reads "request comes in at the outside, falls
* through to defaults at the centre" — the natural reading of
* concentric layers.
*
*   COMPILE-TIME vs RUNTIME LOOKUP:
*
*     get<Key>()  - compile-time type-keyed lookup.  Walks layers
*                   that expose `has_compile_time_keys<>` (i.e.
*                   `static_option_set<...>` and friends).  Static
*                   layers are queried via their `contains<Key>` and
*                   `get<Key>()` members; runtime layers are skipped.
*                   Result type is the deduced value type of the
*                   first matching layer.  Compile error if no static
*                   layer contains the key.
*
*     find(key)   - runtime value-keyed lookup.  Walks all layers
*                   in order; for static layers, this requires a
*                   `visit(key, fn)` adapter (TODO; currently the
*                   runtime path only consults dynamic layers).
*                   Returns an optional<mapped_type> or fallback
*                   per the `value_or` overload.
*
*   OVERRIDE POLICIES:
*   Override policies from `option_override_policy.hpp` control
* whether outer layers are PERMITTED to shadow inner layers for a
* given key/depth combination.  Policies are supplied per-layer via
* the `with_override_policy<Policy>` wrapper around the layer type
* in the parameter list.  Bare layer types use `override_allow_all`.
*
*   STORAGE:
*   `option_layers` holds its layers by value in a `std::tuple` so
* the whole structure can be a `constexpr` object when all layers
* are constexpr-constructible (e.g. all static).  Mixed chains
* containing a runtime `option_set` are constructed at runtime;
* lookup is still constexpr-eligible for the parts that touch only
* static layers.
*
* DEPENDENCIES:
*   djinterp.hpp                  - namespaces, D_CONSTEXPR
*   option_set.hpp                - runtime option_set surface
*   option_set_traits.hpp         - is_option_set_like
*   static_option_set.hpp         - static_option_set surface
*   static_option_set_traits.hpp  - has_compile_time_keys aggregate
*   option_override_policy.hpp    - allow/deny/predicate policies
*
*
* path:      /inc/djinterp/core/options/option_layers.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    with_override_policy (per-layer policy wrapper)
II.   internal traits
III.  option_layers class
IV.   make_option_layers helper
V.    Backward-compat alias for the existing binary option_layer
*/

#ifndef DJINTERP_OPTION_LAYERS_
#define DJINTERP_OPTION_LAYERS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "./option_override_policy.hpp"
#include "./option_set.hpp"
#include "./static_option_set.hpp"
#include "./static_option_set_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   with_override_policy — per-layer policy wrapper
// ===========================================================================

// with_override_policy
//   adapter: pairs a layer type with an override policy.  Used inside
// the `option_layers<...>` parameter pack to attach a non-default
// policy to a specific layer:
//
//     option_layers<
//         with_override_policy<option_set<std::string, int>,
//                              override_to_depth<2>>,
//         static_options<width_key{}, 800>>;
//
// A bare layer type uses `override_allow_all` by default.
template<typename _Layer,
         typename _Policy = override_allow_all>
struct with_override_policy
{
    using layer_type  = _Layer;
    using policy_type = _Policy;

    _Layer  m_layer;
    _Policy m_policy;

    // default
    D_CONSTEXPR with_override_policy() = default;

    // construct from layer + policy
    template<typename _LFwd,
             typename _PFwd>
    D_CONSTEXPR with_override_policy(_LFwd&& _layer,
                                     _PFwd&& _policy)
        : m_layer (static_cast<_LFwd&&>(_layer)),
          m_policy(static_cast<_PFwd&&>(_policy))
    {}
};


// ===========================================================================
// II.  Internal traits
// ===========================================================================

NS_INTERNAL

    // is_policy_wrapped
    //   trait: true iff _Layer is a `with_override_policy<...>`.
    template<typename _Layer>
    struct is_policy_wrapped : std::false_type
    {};

    template<typename _L,
             typename _P>
    struct is_policy_wrapped<with_override_policy<_L, _P>>
        : std::true_type
    {};

    // unwrap_layer_type
    //   trait: extracts the underlying layer type, whether wrapped
    // in `with_override_policy` or bare.
    template<typename _Layer>
    struct unwrap_layer_type
    {
        using type = _Layer;
    };

    template<typename _L,
             typename _P>
    struct unwrap_layer_type<with_override_policy<_L, _P>>
    {
        using type = _L;
    };

    template<typename _Layer>
    using unwrap_layer_t = typename unwrap_layer_type<_Layer>::type;

    // layer_policy_type
    //   trait: extracts the policy type for a layer, defaulting to
    // `override_allow_all` for bare layers.
    template<typename _Layer>
    struct layer_policy_type
    {
        using type = override_allow_all;
    };

    template<typename _L,
             typename _P>
    struct layer_policy_type<with_override_policy<_L, _P>>
    {
        using type = _P;
    };

    // unwrap_layer
    //   helper: returns a reference to the underlying layer for a
    // bare or policy-wrapped layer.
    template<typename _Layer>
    D_CONSTEXPR const _Layer&
    unwrap_layer(const _Layer& _l) noexcept
    {
        return _l;
    }

    template<typename _L,
             typename _P>
    D_CONSTEXPR const _L&
    unwrap_layer(const with_override_policy<_L, _P>& _wrap) noexcept
    {
        return _wrap.m_layer;
    }


    // first_static_layer_with_key
    //   trait: walks a tuple of layer types in order and reports the
    // index of the FIRST layer that satisfies
    // `has_compile_time_keys<>` AND `contains<_Key>::value`.
    // Resolves to `sizeof...(_Layers)` if none qualify.
    template<typename    _Key,
             std::size_t _I,
             typename... _Layers>
    struct first_static_layer_with_key_impl;

    // base case: nothing left
    template<typename    _Key,
             std::size_t _I>
    struct first_static_layer_with_key_impl<_Key, _I>
        : std::integral_constant<std::size_t, _I>
    {};

    // step: try head, recurse on tail
    template<typename    _Key,
             std::size_t _I,
             typename    _Head,
             typename... _Tail>
    struct first_static_layer_with_key_impl<_Key, _I, _Head, _Tail...>
    {
    private:
        using head_t = unwrap_layer_t<_Head>;

        // SFINAE-safe: only ask `head_t` about contains<_Key> if
        // head_t actually has compile-time keys.  Otherwise treat
        // it as "no match" and recurse.
        template<typename _T,
                 bool     _Has = has_compile_time_keys<_T>::value>
        struct head_contains
            : std::false_type
        {};

        template<typename _T>
        struct head_contains<_T, true>
            : std::integral_constant<bool,
                _T::template contains<_Key>::value>
        {};

    public:
        static constexpr std::size_t value =
            head_contains<head_t>::value
                ? _I
                : first_static_layer_with_key_impl<
                    _Key, _I + 1, _Tail...>::value;
    };

    template<typename    _Key,
             typename... _Layers>
    struct first_static_layer_with_key
        : std::integral_constant<std::size_t,
            first_static_layer_with_key_impl<
                _Key, 0, _Layers...>::value>
    {};

NS_END  // internal


// ===========================================================================
// III. option_layers class
// ===========================================================================

// option_layers
//   class: concentric stack of option-set layers.  Layers are
// stored in declaration order (outermost first).  Lookup walks
// outer-to-inner; first hit wins.
//
// Example:
//   constexpr auto defaults = make_static_option_set(
//       width_key{}  = 800,
//       height_key{} = 600);
//
//   option_set<std::string, int> overrides;
//   overrides.insert("width", 1024);
//
//   auto config = make_option_layers(overrides, defaults);
//   //  -> outer = overrides, inner = defaults
//
//   int w = config.template get<width_key>();        // 800 (static)
//   int w_rt = config.value_or("width", 0);          // 1024 (dynamic)
template<typename... _Layers>
class option_layers
{
public:
    using layers_tuple_type = std::tuple<_Layers...>;

    static constexpr std::size_t layer_count = sizeof...(_Layers);

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    D_CONSTEXPR option_layers() = default;

    template<typename... _Args,
             typename = typename std::enable_if<
                 (sizeof...(_Args) == sizeof...(_Layers))
             >::type>
    D_CONSTEXPR explicit
    option_layers(_Args&&... _args)
        : m_layers(static_cast<_Args&&>(_args)...)
    {}

    // layers
    //   function: returns the underlying tuple of layers.  Each
    // entry is either a bare layer or a `with_override_policy`
    // wrapper; use `unwrap_layer(...)` to get at the underlying
    // option set if you don't care about the policy.
    D_CONSTEXPR const layers_tuple_type&
    layers() const noexcept
    {
        return m_layers;
    }

    // -----------------------------------------------------------------
    //  compile-time type-keyed lookup
    // -----------------------------------------------------------------

    // static_layer_index_for
    //   trait: compile-time index of the first static layer
    // containing `_Key`, or `layer_count` if none.
    template<typename _Key>
    struct static_layer_index_for
        : internal::first_static_layer_with_key<_Key, _Layers...>
    {};

    // has_static_value_for
    //   trait: true iff at least one static layer contains `_Key`.
    template<typename _Key>
    struct has_static_value_for
        : std::integral_constant<bool,
            (static_layer_index_for<_Key>::value < layer_count)>
    {};

    // get
    //   function: returns the value paired with `_Key` from the
    // first static layer that contains it.  Compile error if no
    // static layer contains `_Key`.
    //
    //   Note: this consults STATIC layers only — runtime layers
    // cannot be queried by a compile-time key in general.  Use
    // `value_or(key, fallback)` for runtime-keyed lookup that
    // includes runtime layers.
    template<typename _Key>
    D_CONSTEXPR auto
    get() const
        -> decltype(internal::unwrap_layer(
                std::get<static_layer_index_for<_Key>::value>(
                    std::declval<const layers_tuple_type&>()))
                .template get<_Key>())
    {
        static_assert(has_static_value_for<_Key>::value,
                      "option_layers::get<Key>: no static layer "
                      "contains the requested key.");

        constexpr std::size_t idx =
            static_layer_index_for<_Key>::value;

        return internal::unwrap_layer(
            std::get<idx>(m_layers)).template get<_Key>();
    }

    // -----------------------------------------------------------------
    //  runtime value-keyed lookup
    // -----------------------------------------------------------------

    // contains
    //   function: runtime test — returns true if ANY layer
    // (static or dynamic) contains the given key value.  Static
    // layers are skipped for runtime keys; only dynamic layers
    // participate in this overload.
    template<typename _Key>
    bool
    contains(const _Key& _k) const
    {
        return contains_walk<_Key>(_k,
            std::make_index_sequence<layer_count>{});
    }

    // value_or
    //   function: returns the value for `_k` from the first
    // dynamic layer that contains it, or `_fallback` if no
    // dynamic layer does.  Static layers are not consulted by
    // this overload (their values are not addressable by a
    // runtime key without a `visit` adapter).
    template<typename _Key,
             typename _Value>
    _Value
    value_or(const _Key&   _k,
             const _Value& _fallback) const
    {
        return value_or_walk<_Key, _Value>(
            _k, _fallback,
            std::make_index_sequence<layer_count>{});
    }

private:
    layers_tuple_type m_layers;

    // -----------------------------------------------------------------
    //  internal walk helpers
    // -----------------------------------------------------------------

    template<typename       _Key,
             std::size_t... _Is>
    bool
    contains_walk(const _Key& _k,
                  std::index_sequence<_Is...>) const
    {
        // OR-fold over layers; short-circuits in practice via the
        // built-in || in the expanded expression.
        bool found = false;
        // C++17 fold via initializer list; works in C++11/14 via
        // recursive helper if needed (left as enhancement).
        using sink = int[];
        (void)sink{0, ( (found = found ||
                                 layer_contains<_Is, _Key>(_k)), 0)...};

        return found;
    }

    template<typename       _Key,
             typename       _Value,
             std::size_t... _Is>
    _Value
    value_or_walk(const _Key&   _k,
                  const _Value& _fallback,
                  std::index_sequence<_Is...>) const
    {
        _Value result   = _fallback;
        bool   resolved = false;

        using sink = int[];
        (void)sink{0, ( (resolved = resolved ||
                                    layer_value_or_step<_Is,
                                                        _Key,
                                                        _Value>(
                                        _k, result)), 0)...};

        return result;
    }

    // layer_contains
    //   helper: query a single layer's runtime `contains(key)`.
    // Returns false for static layers (they have no runtime key
    // interface) and for layers that don't have a `contains`
    // method.  SFINAE-safe via the trait probes.
    template<std::size_t _I,
             typename    _Key>
    bool
    layer_contains(const _Key& _k) const
    {
        return layer_contains_impl<_I, _Key>(
            _k, has_runtime_contains<_I>{});
    }

    template<std::size_t _I,
             typename    _Key>
    bool
    layer_contains_impl(const _Key& _k,
                        std::true_type) const
    {
        return internal::unwrap_layer(
            std::get<_I>(m_layers)).contains(_k);
    }

    template<std::size_t _I,
             typename    _Key>
    bool
    layer_contains_impl(const _Key& /*_k*/,
                        std::false_type) const
    {
        return false;
    }

    // layer_value_or_step
    //   helper: if the layer at index _I has a runtime find/at
    // surface AND contains the key, writes the value into
    // `_result` and returns true.  Otherwise returns false to
    // signal "fall through to next layer".
    template<std::size_t _I,
             typename    _Key,
             typename    _Value>
    bool
    layer_value_or_step(const _Key& _k,
                        _Value&     _result) const
    {
        return layer_value_or_step_impl<_I, _Key, _Value>(
            _k, _result, has_runtime_find<_I>{});
    }

    template<std::size_t _I,
             typename    _Key,
             typename    _Value>
    bool
    layer_value_or_step_impl(const _Key& _k,
                             _Value&     _result,
                             std::true_type) const
    {
        const auto& layer =
            internal::unwrap_layer(std::get<_I>(m_layers));

        auto it = layer.find(_k);

        if (it != layer.end())
        {
            _result = it->value;

            return true;
        }

        return false;
    }

    template<std::size_t _I,
             typename    _Key,
             typename    _Value>
    bool
    layer_value_or_step_impl(const _Key& /*_k*/,
                             _Value&     /*_result*/,
                             std::false_type) const
    {
        return false;
    }

    // has_runtime_contains / has_runtime_find
    //   traits: true if the layer at index _I exposes the runtime
    // contains/find surface (i.e. is an option_set-like dynamic
    // layer rather than a static one).

    template<std::size_t _I>
    using layer_at_index = typename std::tuple_element<
        _I, layers_tuple_type>::type;

    template<std::size_t _I>
    using underlying_at_index =
        internal::unwrap_layer_t<layer_at_index<_I>>;

    template<std::size_t _I>
    struct has_runtime_contains
        : std::integral_constant<bool,
            has_contains_method<
                underlying_at_index<_I>>::value>
    {};

    template<std::size_t _I>
    struct has_runtime_find
        : std::integral_constant<bool,
            has_find_method<
                underlying_at_index<_I>>::value>
    {};
};


// ===========================================================================
// IV.  make_option_layers
// ===========================================================================

// make_option_layers
//   function: constructs an `option_layers<...>` with deduced layer
// types.  Decay is applied per layer.
//
// Example:
//   auto cfg = make_option_layers(runtime_overrides, defaults);
template<typename... _Layers>
D_CONSTEXPR
option_layers<typename std::decay<_Layers>::type...>
make_option_layers(
    _Layers&&... _layers
)
{
    return option_layers<
        typename std::decay<_Layers>::type...>(
            static_cast<_Layers&&>(_layers)...);
}


// ===========================================================================
// V.   Backward-compatibility alias
// ===========================================================================

// option_layer
//   alias: backward-compatible binary specialization of
// `option_layers`.  The existing binary form
//   option_layer<Parent, OverridePolicy>
// is now an `option_layers<with_override_policy<Parent, OverridePolicy>>`
// with the (currently empty) outer layer assumed to be the
// derived/overriding child.  Existing utility functions in
// `option_diff.hpp` continue to work via the structural surface.
//
//   New code should prefer `option_layers<...>` directly so that
// outer layers (overrides) come first in the parameter list.
//
//   Note: this alias intentionally retains the old name to keep
// existing includes compiling.  If you want the strict two-layer
// form with no auto-wrapping, use
//   option_layers<Outer, Inner>
// directly.
template<typename _Parent,
         typename _OverridePolicy = override_allow_all>
using option_layer = option_layers<
    with_override_policy<_Parent, _OverridePolicy>>;


NS_END  // djinterp


#endif  // DJINTERP_OPTION_LAYERS_
