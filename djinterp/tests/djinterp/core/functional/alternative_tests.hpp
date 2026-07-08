/******************************************************************************
* djinterp [test]                                         alternative_tests.hpp
*
*   Declarations and shared fixtures for the alternative.hpp unit suite.  The
* individual tests_* predicates and their block-providers are defined per
* translation unit (one .cpp per like-group semantic section of the header);
* this head carries only what those files and the runner share.
*
*   alternative.hpp ships NO concrete instance of its own (maybe / view /
* producer specialize it in their own headers), so a suite must SUPPLY one.
* Two fixtures are provided here:
*
*     - opt<T> : a maybe-like Alternative -- engaged (holds a value) or empty --
*       with its own alternative_traits specialization (empty() = the disengaged
*       value; choice(a, b) keeps the first that is engaged).  This is the
*       uniform alternative every generic op (aempty / alt / asum) is driven on.
*     - bag<T> : a minimal, ordered Foldable (its own foldable_traits
*       specialization) distinct from std::vector.  asum sits at the
*       Foldable x Alternative intersection -- it folds a Foldable of
*       Alternatives -- so bag<opt<T>> lets asum be exercised over a Foldable
*       OTHER than the std::vector instance foldable.hpp ships, proving asum is
*       generic over the Foldable, not vector-specific.
*
*   has_alt_value_type<T> is the SFINAE-safe detector (probing the undefined
* primary alternative_traits, exactly as is_alternative does) used for the
* value-type present/absent checks; alternative_value_type<T>::type declares its
* member unconditionally, so a direct probe of it would be a hard error for a
* non-alternative.
*
*   Every predicate lives flat in djinterp::testing and returns true iff all of
* its checks pass.  Each section .cpp keeps its predicates file-local (internal
* linkage) and exposes exactly one block-provider, declared below.
*
*   SECTIONS (mirroring alternative.hpp's table of contents):
*     0 + I.  protocol & traits ......... alternative_tests_protocol.cpp
*     II.1    aempty ................... alternative_tests_aempty.cpp
*     II.2    alt ...................... alternative_tests_alt.cpp
*     II.3    asum ..................... alternative_tests_asum.cpp
*
*
* path:      /tests/djinterp/core/functional/alternative_tests.hpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_ALTERNATIVE_TESTS_
#define DJINTERP_FUNCTIONAL_ALTERNATIVE_TESTS_ 1

// std
#include <cstddef>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp -- the header under test (which pulls in foldable.hpp), plus the
//   DTest authoring + runner surface.  NOTE: these two include paths are rooted
//   at the djinterp include directory (e.g. -I.../inc); adjust them to match
//   your build tree.
#include "djinterp/core/functional/alternative.hpp"
#include "djinterp/test/test_defaults.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                I.   ALTERNATIVE FIXTURE:  opt<T>  (maybe-like)           ///
///////////////////////////////////////////////////////////////////////////////

// opt
//   fixture: a minimal maybe-like Alternative.  It is either engaged (carrying
// a value) or empty; choice keeps the first engaged operand, and the empty
// value is the identity for that choice -- the smallest concrete Alternative
// the generic operations can be exercised on.  Equality is by engagement and,
// when engaged, by value (so opt<T> requires an equality-comparable T; the
// suite instantiates it at int / double / std::string).
template<typename _Type>
struct opt
{
    bool  engaged;
    _Type value;

    // opt
    //   constructor: the empty / disengaged value.
    opt()
        : engaged(false),
          value()
    {
    }

    // opt
    //   constructor: an engaged value carrying _v.
    opt(
        const _Type& _v
    )
        : engaged(true),
          value(_v)
    {
    }

    // operator==
    //   equal iff both are empty, or both engaged with equal values.
    friend bool
    operator==(
        const opt& _a,
        const opt& _b
    )
    {
        return ( (_a.engaged == _b.engaged) &&
                 ( (!_a.engaged) || (_a.value == _b.value) ) );
    }

    // operator!=
    //   the negation of operator==.
    friend bool
    operator!=(
        const opt& _a,
        const opt& _b
    )
    {
        return (!(_a == _b));
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  FOLDABLE FIXTURE:  bag<T>  (a non-std Foldable)      ///
///////////////////////////////////////////////////////////////////////////////

// bag
//   fixture: a minimal, ordered, zero-or-more-element Foldable distinct from
// std::vector, so asum can be driven over a Foldable that is NOT the std::vector
// instance foldable.hpp ships.  Backed by a std::vector for storage; the fold
// order is first-to-last.
template<typename _Type>
struct bag
{
    std::vector<_Type> data;

    // bag
    //   constructor: an empty bag.
    bag()
        : data()
    {
    }

    // bag
    //   constructor: a bag seeded from a braced list, in order.
    bag(
        std::initializer_list<_Type> _xs
    )
        : data(_xs)
    {
    }
};


///////////////////////////////////////////////////////////////////////////////
///                III. COMPILE-TIME DETECTOR:  has_alt_value_type<T>        ///
///////////////////////////////////////////////////////////////////////////////

// has_alt_value_type
//   detector: true iff alternative_traits<decay<_Type>>::value_type is
// well-formed -- i.e. iff _Type carries the Alternative protocol's inner value
// type.  SFINAE-safe: the primary alternative_traits is declared but undefined,
// so for a non-alternative the member access soft-fails in the immediate context
// (false) rather than triggering a hard error.  Deliberately NOT written in
// terms of alternative_value_type<T>::type: that trait declares `type`
// unconditionally, so instantiating it for a non-alternative is a hard error.
template<typename _Type,
         typename _Enable = void>
struct has_alt_value_type
    : std::false_type
{
};

// has_alt_value_type (well-formed specialization)
//   detector: the arm chosen when the trait's value_type exists.
template<typename _Type>
struct has_alt_value_type<
    _Type,
    ::djinterp::void_t<
        typename ::djinterp::alternative_traits<
            typename std::decay<_Type>::type >::value_type> >
    : std::true_type
{
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  SECTION BLOCK-PROVIDERS  (the runner's surface)      ///
///////////////////////////////////////////////////////////////////////////////

::djinterp::test::block_spec alternative_protocol_block();
::djinterp::test::block_spec alternative_aempty_block();
::djinterp::test::block_spec alternative_alt_block();
::djinterp::test::block_spec alternative_asum_block();


NS_END  // testing
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                V.   FIXTURE PROTOCOL SPECIALIZATIONS                     ///
///////////////////////////////////////////////////////////////////////////////
//   Specialized in namespace djinterp (where the primaries live), each in the
// explicit <F, void> form the shipped instances use.

NS_DJINTERP

// alternative_traits< ::djinterp::testing::opt<_Type> >
//   instance: makes opt<T> a uniform Alternative.  empty() is the disengaged
// value; choice(a, b) is the associative "first engaged wins" that models
// maybe's or_else, returning the same opt<T> (so aempty / alt / asum are
// well-typed and the fold accumulator is stable).
template<typename _Type>
struct alternative_traits< ::djinterp::testing::opt<_Type>, void >
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    // empty
    //   the failure / identity for choice: the disengaged value.
    static
    ::djinterp::testing::opt<_Type> empty()
    {
        return ::djinterp::testing::opt<_Type>();
    }

    // choice
    //   associative choice: keep the first operand if engaged, else the second.
    static
    ::djinterp::testing::opt<_Type> choice(
        const ::djinterp::testing::opt<_Type>& _a,
        const ::djinterp::testing::opt<_Type>& _b
    )
    {
        return _a.engaged ? _a : _b;
    }
};


// foldable_traits< ::djinterp::testing::bag<_Type> >
//   instance: makes the bag<T> fixture a Foldable, folding first-to-last with
// the accumulator threaded by move, mirroring the std::vector instance.  Present
// so asum can be exercised over a Foldable other than std::vector.
template<typename _Type>
struct foldable_traits< ::djinterp::testing::bag<_Type>, void >
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    // fold_left
    //   strict left fold over the bag's elements.
    template<typename _Acc,
             typename _Function>
    static
    _Acc fold_left(
        const ::djinterp::testing::bag<_Type>& _xs,
        _Acc                                   _init,
        _Function                              _function
    )
    {
        typename std::vector<_Type>::const_iterator _it = _xs.data.begin();

        for (; _it != _xs.data.end(); ++_it)
        {
            _init = _function(std::move(_init), *_it);
        }

        return _init;
    }
};

NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_ALTERNATIVE_TESTS_
