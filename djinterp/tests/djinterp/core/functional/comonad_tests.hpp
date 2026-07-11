/******************************************************************************
* djinterp [test]                                          comonad_tests.hpp
*
*   Declarations and shared fixtures for the comonad.hpp unit suite.  The
* individual tests_* predicates and their block-providers are defined per
* translation unit (one .cpp per like-group semantic section of the header);
* this head carries only what those files and the runner share.
*
*   comonad.hpp ships two concrete instances of its own -- the Env (co-reader)
* comonad over std::pair<E, A> and kv_pair<K, V>, where the focus is the SECOND
* component and the first rides along as the environment -- so the generic
* extract / extend / duplicate are driven directly over those (they are
* themselves code under test).  In addition a small user-defined comonad,
* ident<A> (the Identity comonad: a bare focus, no environment), is provided so
* the generic operations are proven over a comonad of a different shape.
*
*   Generic co-Kleisli functors (co_extract / co_focus_x2 / co_focus_plus1 /
* co_focus_show / co_id) act on ANY comonad through extract, so one set drives
* pair, kv_pair, and ident; pair_env_focus / kv_key_focus additionally read the
* ENVIRONMENT, proving extend hands the whole context to its function.  All are
* named functors (not lambdas) so they may appear in trailing return types on
* every language floor.
*
*   has_co_value_type<T> is the SFINAE-safe detector (probing the undefined
* primary comonad_traits, as is_comonad does); comonad_value_type<T>::type is
* declared unconditionally, so a direct probe of it would be a hard error for a
* non-comonad.
*
*   A NOTE ON kv_pair: its operator== / operator< compare the KEY ONLY.  Since
* the Env comonad's environment IS the key, extend (which preserves the key /
* environment) leaves == intact even as it recomputes the value / focus; tests
* that must verify a recomputed value compare the m_value field directly.
*
*   Every predicate lives flat in djinterp::testing and returns true iff all of
* its checks pass.  Each section .cpp keeps its predicates file-local (internal
* linkage) and exposes exactly one block-provider, declared below.
*
*   SECTIONS (mirroring comonad.hpp's table of contents):
*     0 + I.  protocol & traits ......... comonad_tests_protocol.cpp
*     II.1    extract .................. comonad_tests_extract.cpp
*     II.2    extend ................... comonad_tests_extend.cpp
*     II.3    duplicate ................ comonad_tests_duplicate.cpp
*     III.    instances (Env comonad) .. comonad_tests_instances.cpp
*
*
* path:      /tests/djinterp/core/functional/comonad_tests.hpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_COMONAD_TESTS_
#define DJINTERP_FUNCTIONAL_COMONAD_TESTS_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
// djinterp -- the header under test (which pulls in meta/kv_pair.hpp), plus the
//   DTest authoring + runner surface.  NOTE: these two include paths are rooted
//   at the djinterp include directory (e.g. -I.../inc); adjust them to match
//   your build tree.
#include "djinterp/core/functional/comonad.hpp"
#include "djinterp/test/test_defaults.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                I.   CUSTOM COMONAD FIXTURE:  ident<A>  (Identity)        ///
///////////////////////////////////////////////////////////////////////////////

// ident
//   fixture: the Identity comonad -- a bare focus with no environment.  It
// proves the generic operations work over a comonad whose shape differs from
// the shipped Env (pair-based) instances.  Full structural equality.
template<typename _A>
struct ident
{
    _A value;

    ident(
        const _A& _v
    )
        : value(_v)
    {
    }

    friend bool
    operator==(
        const ident& _a,
        const ident& _b
    )
    {
        return (_a.value == _b.value);
    }

    friend bool
    operator!=(
        const ident& _a,
        const ident& _b
    )
    {
        return (!(_a == _b));
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  CO-KLEISLI FUNCTORS                                  ///
///////////////////////////////////////////////////////////////////////////////

// -- generic (any comonad, via extract) --

// co_extract
//   W -> A: the focus.  extend(w, co_extract) is the comonad left identity.
struct co_extract
{
    template<typename _W>
    auto operator()(
        const _W& _w
    ) const
    -> decltype(::djinterp::extract(_w))
    {
        return ::djinterp::extract(_w);
    }
};

// co_focus_x2
//   W -> int: twice the (int) focus.
struct co_focus_x2
{
    template<typename _W>
    int operator()(
        const _W& _w
    ) const
    {
        return (::djinterp::extract(_w) * 2);
    }
};

// co_focus_plus1
//   W -> int: the (int) focus plus one.
struct co_focus_plus1
{
    template<typename _W>
    int operator()(
        const _W& _w
    ) const
    {
        return (::djinterp::extract(_w) + 1);
    }
};

// co_focus_show
//   W -> std::string: the (int) focus rendered as decimal (proves extend may
// change the focus type).
struct co_focus_show
{
    template<typename _W>
    std::string operator()(
        const _W& _w
    ) const
    {
        return std::to_string(::djinterp::extract(_w));
    }
};

// co_id
//   W -> W: the whole context unchanged.  duplicate(w) == extend(w, co_id).
struct co_id
{
    template<typename _W>
    _W operator()(
        const _W& _w
    ) const
    {
        return _w;
    }
};

// -- environment-reading (prove extend sees the whole context) --

// pair_env_focus
//   std::pair<std::string,int> -> int: focus plus the environment's length, so
// its result depends on BOTH components.
struct pair_env_focus
{
    int operator()(
        const std::pair<std::string, int>& _w
    ) const
    {
        return (_w.second + static_cast<int>(_w.first.size()));
    }
};

// kv_key_focus
//   kv_pair<int,int> -> int: value plus key, again depending on both.
struct kv_key_focus
{
    int operator()(
        const ::djinterp::kv_pair<int, int>& _w
    ) const
    {
        return (_w.m_value + _w.m_key);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                III. COMPILE-TIME DETECTOR:  has_co_value_type<T>         ///
///////////////////////////////////////////////////////////////////////////////

// has_co_value_type
//   detector: true iff comonad_traits<decay<_Type>>::value_type is well-formed.
// SFINAE-safe: the primary comonad_traits is declared but undefined, so for a
// non-comonad the member access soft-fails in the immediate context (false)
// rather than a hard error.  Deliberately NOT written in terms of
// comonad_value_type<T>::type, which declares its member unconditionally.
template<typename _Type,
         typename _Enable = void>
struct has_co_value_type
    : std::false_type
{
};

template<typename _Type>
struct has_co_value_type<
    _Type,
    ::djinterp::void_t<
        typename ::djinterp::comonad_traits<
            typename std::decay<_Type>::type >::value_type> >
    : std::true_type
{
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  SECTION BLOCK-PROVIDERS  (the runner's surface)      ///
///////////////////////////////////////////////////////////////////////////////

::djinterp::test::block_spec comonad_protocol_block();
::djinterp::test::block_spec comonad_extract_block();
::djinterp::test::block_spec comonad_extend_block();
::djinterp::test::block_spec comonad_duplicate_block();
::djinterp::test::block_spec comonad_instances_block();


NS_END  // testing
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                V.   CUSTOM FIXTURE PROTOCOL SPECIALIZATION              ///
///////////////////////////////////////////////////////////////////////////////
//   ident<A> is registered as a comonad (in namespace djinterp, where the
// primary lives), in the explicit <W, void> form the shipped instances use.

NS_DJINTERP

// comonad_traits< ::djinterp::testing::ident<_A> >
//   instance: the Identity comonad -- extract returns the focus; extend applies
// f to the whole (bare) context and re-wraps.
template<typename _A>
struct comonad_traits< ::djinterp::testing::ident<_A>, void >
{
    using is_specialized = std::true_type;
    using value_type     = _A;

    static
    _A extract(
        const ::djinterp::testing::ident<_A>& _w
    )
    {
        return _w.value;
    }

    template<typename _Function>
    static
    ::djinterp::testing::ident<
        typename std::decay<decltype(std::declval<_Function&>()(
            std::declval<const ::djinterp::testing::ident<_A>&>()))>::type>
    extend(
        const ::djinterp::testing::ident<_A>& _w,
        _Function                             _function
    )
    {
        using mapped_t = typename std::decay<decltype(
            std::declval<_Function&>()(
                std::declval<const ::djinterp::testing::ident<_A>&>()))>::type;

        return ::djinterp::testing::ident<mapped_t>(_function(_w));
    }
};

NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_COMONAD_TESTS_
