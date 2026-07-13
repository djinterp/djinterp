/******************************************************************************
* djinterp [meta]                                                 lifetime.hpp
*
*   The framework's foundational LIFETIME vocabulary - the compile-time /
* runtime classification of a type, factored out so every subsystem draws its
* "when does this come to be fixed?" answer from one place.  This is the C++
* embodiment of the Lifetime axis: the earliest stage at which a quantity is
* determined, and the lattice those stages form.
*
*   THE MODEL (two stages, c precedes r):
*   A quantity may become determined at one of two stages, ordered by
* precedence:
*       compile-time (c)   the program text and types alone;
*       runtime      (r)   additionally the concrete instance from execution.
*   Compile-time is the STRONGER guarantee: anything determined at c is a
* fortiori determined at r, so a value fixed at compile time is also usable at
* runtime.  A TYPE's lifetime is the SET of stages at which its values can be
* determined - one of:
*       lifetime::compile_time   the c stage only (constexpr, e.g. consteval-
*                                only constructs that opt in);
*       lifetime::runtime        the r stage only (heap-backed, virtual, or
*                                otherwise not constant-evaluable);
*       lifetime::both           either stage (a literal type - the common
*                                "constexpr-capable" case: std::array, our
*                                fixed_array);
*       lifetime::none           neither (the bottom / identity element).
*   The set is encoded as one bit per stage, so the four values form a lattice
* under bitwise combination.
*
*   COMPOSITION (meet == the formal max-over-stages):
*   A composite (a container of its size and its elements; an aggregate of its
* members) is constexpr-capable if, and ONLY if, EVERY constituent is, and
* runtime-capable if, and only if, every constituent is.  That is exactly the
* meet (bitwise AND) of the constituents' lifetimes, and it reproduces the
* Lifetime-axis composition stage(c) = max(stage|c|, max_p stage val_c(p))
* under c < r (max selects the later, weaker stage).  lifetime_meet is the
* canonical composition operator; the container traits build on it.
*
*   CONTAINER-AGNOSTIC:
*   This header owns only the lifetime vocabulary, its algebra, a type-level
* carrier, a portable literal-type probe, and an opt-in classifier.  It knows
* nothing of containers: structural probes (a constexpr size(), an extent, an
* allocator, a reserve()) are container-domain and live with the container
* traits, which express their findings AS a lifetime and feed it through
* lifetime_meet.
*
*   DETECTION PRIORITY of lifetime_of<T>:
*     1. opt-in `lifetime_category` static member on T;
*     2. portable is_literal_type<T>  -> lifetime::both;
*     3. conservative fallback        -> lifetime::runtime.
*
*   PORTABILITY:
*   C++11 baseline.  The `_v` companions degrade with the language (inline
* variable on C++17+, plain variable template on C++14, absent on C++11 -
* where the `::value` member is always present).  std::is_literal_type is used
* on C++11/14 and replaced by a heuristic on C++17+ (deprecated in C++17,
* removed in C++20).
*
*
* path:      /inc/djinterp/core/meta/lifetime.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_META_LIFETIME_
#define DJINTERP_META_LIFETIME_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"      // clean_t, void_t, NS_*, D_ENV_* feature macros
#include "./trait_detect.hpp"   // D_VOID_T, D_TYPE_TRAIT_* detection macros


NS_DJINTERP


// ===========================================================================
// I.   The lifetime lattice
// ===========================================================================

// lifetime
//   enum: a type's determination capability across the two evaluation stages
// - compile-time (constexpr) and runtime - with one bit per stage so the four
// values form a lattice.  compile-time precedes runtime (c < r); a value fixed
// at compile time is a fortiori available at runtime.
enum class lifetime : unsigned
{
    none         = 0u,                       // neither stage (bottom)
    compile_time = (1u << 0),                // constexpr stage only
    runtime      = (1u << 1),                // runtime stage only
    both         = (compile_time | runtime)  // either stage (top)
};


// ===========================================================================
// II.  Lifetime algebra
// ===========================================================================
//   meet/join over the lattice, the stage predicates, and a builder.  Plain
// `constexpr` (not D_CONSTEXPR) so the algebra is always usable in constant
// expressions - including the static initializers of the traits below -
// regardless of the constexpr-instrumentation toggle.

// lifetime_meet
//   function: the lattice meet (intersection of stages) - the lifetime of a
// composite whose parts have lifetimes _a and _b.  The composite is
// constexpr-capable only if both parts are, runtime-capable only if both
// parts are.  This is the canonical COMPOSITION operator; it reproduces the
// Lifetime-axis max-over-stages.
constexpr lifetime
lifetime_meet(
    lifetime _a,
    lifetime _b
) noexcept
{
    return static_cast<lifetime>(
        static_cast<unsigned>(_a) & static_cast<unsigned>(_b) );
}

// lifetime_join
//   function: the lattice join (union of stages) - the lifetime of an
// alternative that may be either _a or _b (a sum / variant), capable at any
// stage either operand is.
constexpr lifetime
lifetime_join(
    lifetime _a,
    lifetime _b
) noexcept
{
    return static_cast<lifetime>(
        static_cast<unsigned>(_a) | static_cast<unsigned>(_b) );
}

// make_lifetime
//   function: builds a lifetime from two independent stage facts (is the type
// usable at compile time? at runtime?).
constexpr lifetime
make_lifetime(
    bool _is_compile_time,
    bool _is_runtime
) noexcept
{
    return static_cast<lifetime>(
        ( _is_compile_time ? static_cast<unsigned>(lifetime::compile_time) : 0u ) |
        ( _is_runtime      ? static_cast<unsigned>(lifetime::runtime)      : 0u ) );
}

// is_compile_time
//   function: true iff the lifetime includes the compile-time (constexpr)
// stage - i.e. the type is constexpr-capable (compile_time or both).
constexpr bool
is_compile_time(lifetime _life) noexcept
{
    return ( ( static_cast<unsigned>(_life) &
               static_cast<unsigned>(lifetime::compile_time) ) != 0u );
}

// is_runtime
//   function: true iff the lifetime includes the runtime stage (runtime or
// both).
constexpr bool
is_runtime(lifetime _life) noexcept
{
    return ( ( static_cast<unsigned>(_life) &
               static_cast<unsigned>(lifetime::runtime) ) != 0u );
}

// is_both
//   function: true iff the lifetime spans both stages.
constexpr bool
is_both(lifetime _life) noexcept
{
    return ( _life == lifetime::both );
}

// is_none
//   function: true iff the lifetime spans neither stage (the bottom).
constexpr bool
is_none(lifetime _life) noexcept
{
    return ( _life == lifetime::none );
}

// is_compile_time_only
//   function: true iff the lifetime is the compile-time stage exclusively
// (constexpr, not runtime).
constexpr bool
is_compile_time_only(lifetime _life) noexcept
{
    return ( _life == lifetime::compile_time );
}

// is_runtime_only
//   function: true iff the lifetime is the runtime stage exclusively (not
// constant-evaluable).
constexpr bool
is_runtime_only(lifetime _life) noexcept
{
    return ( _life == lifetime::runtime );
}

// lifetime_name
//   function: a stable human-readable spelling of a lifetime, for diagnostics
// and agent-facing summaries.
constexpr const char*
lifetime_name(lifetime _life) noexcept
{
    return ( _life == lifetime::both         ? "both"
           : _life == lifetime::compile_time ? "compile_time"
           : _life == lifetime::runtime      ? "runtime"
           :                                   "none" );
}


// ===========================================================================
// III. Type-level carrier
// ===========================================================================
//   Lets a lifetime travel through templates as a type and serve as the
// `::type` of the classifier below.

// lifetime_constant
//   type: an integral_constant specialized to a lifetime value (the lifetime
// analogue of std::bool_constant).
template<lifetime _Life>
using lifetime_constant = std::integral_constant<lifetime, _Life>;

// none_lifetime / compile_time_lifetime / runtime_lifetime / both_lifetime
//   type: named carriers for the four lattice values, for tag dispatch.
using none_lifetime         = lifetime_constant<lifetime::none>;
using compile_time_lifetime = lifetime_constant<lifetime::compile_time>;
using runtime_lifetime      = lifetime_constant<lifetime::runtime>;
using both_lifetime         = lifetime_constant<lifetime::both>;


// ===========================================================================
// IV.  Portable literal-type probe
// ===========================================================================

// is_literal_type
//   trait: portable "is _Type a literal type?" - the general structural
// signal that a type is constexpr-capable.  Strips cv-ref first.  On C++11/14
// it delegates to std::is_literal_type; on C++17+ (where that trait is
// deprecated, then removed) it falls back to a conservative heuristic:
// scalars, plus trivially default-constructible AND trivially destructible
// class types.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    template<typename _Type>
    struct is_literal_type
        : std::integral_constant<bool,
              ( std::is_scalar<clean_t<_Type>>::value ||
                ( std::is_trivially_destructible<clean_t<_Type>>::value &&
                  std::is_trivially_default_constructible<
                      clean_t<_Type>>::value ) )>
    {};

#else  // C++11 / C++14

    template<typename _Type>
    struct is_literal_type
        : std::is_literal_type<clean_t<_Type>>
    {};

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

D_TYPE_TRAIT_VALUE_BOOL(is_literal_type)


// ===========================================================================
// V.   Opt-in detection
// ===========================================================================
//   A type declares its own lifetime by exposing a static member
//       static constexpr djinterp::lifetime lifetime_category = ...;
// This is the highest-priority signal: it overrides the structural default.

// has_lifetime_category
//   trait: detects the opt-in `lifetime_category` static member (cv-ref
// stripped via clean_t, so the answer agrees for T, const T, T&).
D_TYPE_TRAIT_TRUE(has_lifetime_category,
                  decltype(clean_t<_Type>::lifetime_category))

NS_INTERNAL

    // lifetime_category_member
    //   trait: yields the opt-in `lifetime_category` when declared, else
    // lifetime::none (primary template - member absent).  The fallback is
    // observed only when has_lifetime_category is false, where the classifier
    // discards it.
    template<typename _Type,
             typename = void>
    struct lifetime_category_member
    {
        static constexpr lifetime value = lifetime::none;
    };

    // lifetime_category_member (opt-in present)
    //   trait: reads clean_t<_Type>::lifetime_category.
    template<typename _Type>
    struct lifetime_category_member<_Type,
        D_VOID_T<decltype(clean_t<_Type>::lifetime_category)>>
    {
        static constexpr lifetime value = clean_t<_Type>::lifetime_category;
    };

NS_END  // internal


// ===========================================================================
// VI.  Classification umbrella
// ===========================================================================

// lifetime_of
//   trait: classifies a type's lifetime.  Priority: an opt-in
// `lifetime_category` member, else a literal type (lifetime::both), else the
// conservative fallback lifetime::runtime.  Exposes the value plus a
// lifetime_constant carrier as `::type`.
template<typename _Type>
struct lifetime_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr lifetime value =
        ( has_lifetime_category<clean_type>::value
              ? internal::lifetime_category_member<clean_type>::value
              : ( is_literal_type<clean_type>::value
                      ? lifetime::both
                      : lifetime::runtime ) );

    using type = lifetime_constant<value>;
};

// lifetime_of_t
//   type: convenience alias for lifetime_of<_Type>::type (a carrier).
template<typename _Type>
using lifetime_of_t = typename lifetime_of<_Type>::type;

// lifetime_of_v
//   value: the `_v` companion of lifetime_of (a lifetime, not a bool, so it is
// emitted by hand rather than via D_TYPE_TRAIT_VALUE_BOOL - same degradation).
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr lifetime lifetime_of_v = lifetime_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr lifetime lifetime_of_v = lifetime_of<_Type>::value;
#endif


// ===========================================================================
// VII. Derived lifetime predicates
// ===========================================================================
//   Boolean trait projections of lifetime_of, for SFINAE branches and
// requires-clauses.  Each emits its `_v` companion through the canonical
// trait_detect macro.

// is_constexpr_lifetime
//   trait: true iff _Type is constexpr-capable - its lifetime includes the
// compile-time stage (compile_time or both).
template<typename _Type>
struct is_constexpr_lifetime
    : std::integral_constant<bool,
          is_compile_time(lifetime_of<_Type>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_constexpr_lifetime)

// is_runtime_only_lifetime
//   trait: true iff _Type's lifetime is the runtime stage exclusively - it is
// NOT constant-evaluable.
template<typename _Type>
struct is_runtime_only_lifetime
    : std::integral_constant<bool,
          is_runtime_only(lifetime_of<_Type>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_runtime_only_lifetime)

// is_dual_lifetime
//   trait: true iff _Type spans both stages (constexpr-capable AND usable at
// runtime - the literal-type case).
template<typename _Type>
struct is_dual_lifetime
    : std::integral_constant<bool,
          is_both(lifetime_of<_Type>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_dual_lifetime)


NS_END  // djinterp


#endif  // DJINTERP_META_LIFETIME_
