/******************************************************************************
* djinterp [test]                                         bifunctor_tests.hpp
*
*   Declarations and shared fixtures for the bifunctor.hpp unit suite.  The
* individual tests_* predicates and their block-providers are defined per
* translation unit (one .cpp per like-group semantic section of the header);
* this head carries only what those files and the runner share.
*
*   bifunctor.hpp ships two concrete instances of its own -- std::pair and
* kv_pair -- so the generic operations are driven directly over those (they are
* themselves code under test).  In addition a small user-defined bifunctor,
* two<A, B>, is provided (its own bifunctor_traits specialization) so bimap /
* map_first / map_second are proven generic over a bifunctor beyond the shipped
* pair-like ones.
*
*   NAMED mapping functors (idf / inc_int / dbl_int / show_int / real_int) are
* used rather than lambdas so they may appear in trailing return types and
* decltype on every language floor (a lambda in an unevaluated context is
* C++20-only).  idf is a generic identity mirroring the header's own
* bifunctor_identity_helper.
*
*   has_bi_types<T> is the SFINAE-safe detector (probing the undefined primary
* bifunctor_traits, as is_bifunctor does); bifunctor_first_type<T>::type is
* declared unconditionally, so a direct probe of it would be a hard error for a
* non-bifunctor.
*
*   A NOTE ON kv_pair: its operator== / operator< compare the KEY ONLY.  So a
* result whose value was mapped but whose key was not still compares equal to
* the original; tests that must verify a mapped VALUE compare the m_value field
* directly rather than relying on ==.
*
*   Every predicate lives flat in djinterp::testing and returns true iff all of
* its checks pass.  Each section .cpp keeps its predicates file-local (internal
* linkage) and exposes exactly one block-provider, declared below.
*
*   SECTIONS (mirroring bifunctor.hpp's table of contents):
*     0 + I.  protocol & traits ......... bifunctor_tests_protocol.cpp
*     II.1    bimap .................... bifunctor_tests_bimap.cpp
*     II.2-3  map_first / map_second ... bifunctor_tests_onesided.cpp
*     III.    instances (pair, kv_pair)  bifunctor_tests_instances.cpp
*
*
* path:      /tests/djinterp/core/functional/bifunctor_tests.hpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_BIFUNCTOR_TESTS_
#define DJINTERP_FUNCTIONAL_BIFUNCTOR_TESTS_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
// djinterp -- the header under test (which pulls in meta/kv_pair.hpp), plus the
//   DTest authoring + runner surface.  NOTE: these two include paths are rooted
//   at the djinterp include directory (e.g. -I.../inc); adjust them to match
//   your build tree.
#include "djinterp/core/functional/bifunctor.hpp"
#include "djinterp/test/test_defaults.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                I.   CUSTOM BIFUNCTOR FIXTURE:  two<A, B>                 ///
///////////////////////////////////////////////////////////////////////////////

// two
//   fixture: a minimal user-defined two-parameter product, distinct from
// std::pair / kv_pair, so the generic bimap / map_first / map_second are proven
// generic over a bifunctor other than the shipped instances.  Full structural
// equality (both components), unlike kv_pair.
template<typename _A,
         typename _B>
struct two
{
    _A a;
    _B b;

    // two
    //   constructor: wraps a first and a second value.
    two(
        const _A& _first,
        const _B& _second
    )
        : a(_first),
          b(_second)
    {
    }

    // operator==
    //   equal iff both components are equal.
    friend bool
    operator==(
        const two& _x,
        const two& _y
    )
    {
        return ( (_x.a == _y.a) &&
                 (_x.b == _y.b) );
    }

    // operator!=
    //   the negation of operator==.
    friend bool
    operator!=(
        const two& _x,
        const two& _y
    )
    {
        return (!(_x == _y));
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  NAMED MAPPING FUNCTORS                               ///
///////////////////////////////////////////////////////////////////////////////

// idf
//   functor: generic identity (mirrors the header's bifunctor_identity_helper),
// used to check that the untouched side of a one-sided map is preserved.
struct idf
{
    template<typename _X>
    _X operator()(_X _x) const
    {
        return _x;
    }
};

// inc_int
//   functor: int -> int, add one.
struct inc_int
{
    int operator()(int _x) const
    {
        return (_x + 1);
    }
};

// dbl_int
//   functor: int -> int, double.
struct dbl_int
{
    int operator()(int _x) const
    {
        return (_x * 2);
    }
};

// show_int
//   functor: int -> std::string, render as decimal (proves a mapped side may
// change type).
struct show_int
{
    std::string operator()(int _x) const
    {
        return std::to_string(_x);
    }
};

// real_int
//   functor: int -> double (proves the second side may change type
// independently of the first).
struct real_int
{
    double operator()(int _x) const
    {
        return static_cast<double>(_x);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                III. COMPILE-TIME DETECTOR:  has_bi_types<T>              ///
///////////////////////////////////////////////////////////////////////////////

// has_bi_types
//   detector: true iff both bifunctor_traits<decay<_Type>>::first_type and
// ::second_type are well-formed -- i.e. iff _Type carries the Bifunctor
// protocol's parameter types.  SFINAE-safe: the primary bifunctor_traits is
// declared but undefined, so for a non-bifunctor the member access soft-fails in
// the immediate context (false) rather than a hard error.  Deliberately NOT
// written in terms of bifunctor_first_type<T>::type, which declares its member
// unconditionally.
template<typename _Type,
         typename _Enable = void>
struct has_bi_types
    : std::false_type
{
};

// has_bi_types (well-formed specialization)
//   detector: the arm chosen when both parameter-type members exist.
template<typename _Type>
struct has_bi_types<
    _Type,
    ::djinterp::void_t<
        typename ::djinterp::bifunctor_traits<
            typename std::decay<_Type>::type >::first_type,
        typename ::djinterp::bifunctor_traits<
            typename std::decay<_Type>::type >::second_type> >
    : std::true_type
{
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  SECTION BLOCK-PROVIDERS  (the runner's surface)      ///
///////////////////////////////////////////////////////////////////////////////

::djinterp::test::block_spec bifunctor_protocol_block();
::djinterp::test::block_spec bifunctor_bimap_block();
::djinterp::test::block_spec bifunctor_onesided_block();
::djinterp::test::block_spec bifunctor_instances_block();


NS_END  // testing
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                V.   CUSTOM FIXTURE PROTOCOL SPECIALIZATION              ///
///////////////////////////////////////////////////////////////////////////////
//   Specialized in namespace djinterp (where the primary lives), in the same
// explicit <F, void> form and declval-based return-type style the shipped
// std::pair / kv_pair instances use.

NS_DJINTERP

// bifunctor_traits< ::djinterp::testing::two<_A, _B> >
//   instance: maps each component of the custom product.
template<typename _A,
         typename _B>
struct bifunctor_traits< ::djinterp::testing::two<_A, _B>, void >
{
    using is_specialized = std::true_type;
    using first_type     = _A;
    using second_type    = _B;

    template<typename _First,
             typename _Second>
    static
    ::djinterp::testing::two<
        typename std::decay<decltype(std::declval<_First&>()(
            std::declval<const _A&>()))>::type,
        typename std::decay<decltype(std::declval<_Second&>()(
            std::declval<const _B&>()))>::type>
    bimap(
        const ::djinterp::testing::two<_A, _B>& _t,
        _First                                  _f,
        _Second                                 _g
    )
    {
        using mapped_first_t = typename std::decay<decltype(
            std::declval<_First&>()(std::declval<const _A&>()))>::type;
        using mapped_second_t = typename std::decay<decltype(
            std::declval<_Second&>()(std::declval<const _B&>()))>::type;

        return ::djinterp::testing::two<mapped_first_t, mapped_second_t>(
            _f(_t.a), _g(_t.b));
    }
};

NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_BIFUNCTOR_TESTS_
