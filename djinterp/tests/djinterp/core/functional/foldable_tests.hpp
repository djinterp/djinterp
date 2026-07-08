/******************************************************************************
* djinterp [test]                                            foldable_tests.hpp
*
*   Declarations and shared fixtures for the foldable.hpp unit suite.  The
* individual tests_* predicates and their block-providers are defined per
* translation unit (one .cpp per like-group semantic section of the header);
* this head carries only what those files and the runner share:
*
*     - the six section block-providers (the public surface the runner links),
*     - a second, non-std foldable fixture (`seq<T>`) with its own
*       foldable_traits specialization, so the generic folds are exercised on a
*       type OTHER than std::vector (the header's own materialized instance) and
*       the protocol's dispatch is proven generic rather than vector-specific,
*     - `has_value_type<T>`, a compile-time detector that drives the SFINAE
*       (soft-failure) arm of foldable_value_type for a non-foldable type.
*
*   Every predicate lives flat in djinterp::testing and returns true iff all of
* its checks pass (the bool() shape the DTest block model records one verdict
* from).  Each section .cpp keeps its predicates file-local (internal linkage)
* and exposes exactly one block-provider, declared below.
*
*   SECTIONS (mirroring foldable.hpp's table of contents):
*     0 + I.  protocol & traits ......... foldable_tests_protocol.cpp
*     II.1    fold_left ................. foldable_tests_fold_left.cpp
*     II.2    fold_right ............... foldable_tests_fold_right.cpp
*     II.3    fold_map ................. foldable_tests_fold_map.cpp
*     II.4-6  collect / length / empty . foldable_tests_collect.cpp
*     II.7    fold_any / fold_all ...... foldable_tests_predicates.cpp
*
*
* path:      /tests/djinterp/core/functional/foldable_tests.hpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_FOLDABLE_TESTS_
#define DJINTERP_FUNCTIONAL_FOLDABLE_TESTS_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp -- the header under test, plus the DTest authoring + runner surface
//   (module_spec / block_spec / test_spec, the option set, and run_module).
//   NOTE: these two include paths are rooted at the djinterp include directory
//   (e.g. -I.../inc); adjust them to match your build tree.
#include "djinterp/core/functional/foldable.hpp"
#include "djinterp/test/test_defaults.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED FIXTURE:  seq<T>  (a non-std foldable)        ///
///////////////////////////////////////////////////////////////////////////////

// seq
//   fixture: a minimal, ordered, zero-or-more-element container distinct from
// std::vector.  It exists solely so the generic folds can be driven over a
// foldable that is NOT the header's own std::vector instance, proving the
// protocol dispatch is generic.  Backed by a std::vector for storage; the fold
// order is first-to-last, matching the vector instance's contract.
template<typename _Type>
struct seq
{
    std::vector<_Type> data;

    // seq
    //   constructor: an empty sequence.
    seq()
        : data()
    {
    }

    // seq
    //   constructor: a sequence seeded from a braced list, in order.
    seq(
        std::initializer_list<_Type> _xs
    )
        : data(_xs)
    {
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  COMPILE-TIME DETECTOR:  has_traits_value_type<T>     ///
///////////////////////////////////////////////////////////////////////////////

// has_traits_value_type
//   detector: true iff foldable_traits<decay<_Type>>::value_type is well-formed
// -- i.e. iff _Type carries the foldable protocol's inner value type.  This is
// the SFINAE-safe surface the framework itself detects on: the primary
// foldable_traits is declared but undefined, so for a non-foldable the member
// access soft-fails in the immediate context (false) rather than triggering a
// hard error.  It is deliberately NOT written in terms of
// foldable_value_type<T>::type: that trait declares `type` unconditionally, so
// instantiating it for a non-foldable is a hard error (a nested-instantiation
// failure), unusable for detection.  Note the input is decayed, matching
// foldable_value_type's own behavior.
template<typename _Type,
         typename _Enable = void>
struct has_traits_value_type
    : std::false_type
{
};

// has_traits_value_type (well-formed specialization)
//   detector: the arm chosen when the trait's value_type exists.
template<typename _Type>
struct has_traits_value_type<
    _Type,
    ::djinterp::void_t<
        typename ::djinterp::foldable_traits<
            typename std::decay<_Type>::type >::value_type> >
    : std::true_type
{
};


///////////////////////////////////////////////////////////////////////////////
///                III. SECTION BLOCK-PROVIDERS  (the runner's surface)      ///
///////////////////////////////////////////////////////////////////////////////

::djinterp::test::block_spec foldable_protocol_block();
::djinterp::test::block_spec foldable_fold_left_block();
::djinterp::test::block_spec foldable_fold_right_block();
::djinterp::test::block_spec foldable_fold_map_block();
::djinterp::test::block_spec foldable_collect_block();
::djinterp::test::block_spec foldable_predicate_block();


NS_END  // testing
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                IV.  seq<T> FOLDABLE PROTOCOL SPECIALIZATION              ///
///////////////////////////////////////////////////////////////////////////////
//   Specialized in namespace djinterp (where the primary lives), written in the
// same explicit <F, void> form the std::vector instance uses.  One obligation --
// the strict left fold -- is enough for every generic fold to work over seq<T>.

NS_DJINTERP

// foldable_traits< ::djinterp::testing::seq<_Type> >
//   instance: makes the test fixture seq<T> a first-class foldable, folding
// first-to-last with the accumulator threaded by move (so collecting folds stay
// O(n)), mirroring the std::vector instance.
template<typename _Type>
struct foldable_traits< ::djinterp::testing::seq<_Type>, void >
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    // fold_left
    //   strict left fold over the sequence's elements.
    template<typename _Acc,
             typename _Function>
    static
    _Acc fold_left(
        const ::djinterp::testing::seq<_Type>& _xs,
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


#endif  // DJINTERP_FUNCTIONAL_FOLDABLE_TESTS_
