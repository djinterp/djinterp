/*******************************************************************************
* djinterp [core]              container_comparison_traits_tests_realization.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_comparison_traits_tests_realization.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <list>
#include <set>
#include <vector>
// djinterp
#include "container_comparison_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_lifetime_relation
  lifetime_relation - staticity, as a TWO-POINT total order with the compile-time
  stage the stronger.
  Tests the following:
  - equal stages are equivalent, both ways
  - a compile-time type is `greater` than a runtime one, and the runtime one is
    `less` than it
  - the fixtures really do differ on the stage and on nothing else the lifetime
    axis reads
*/
bool
tests_lifetime_relation()
{
    bool ok = true;

    // equal stages
    D_CCM_REL(order_relation::equivalent,
              (lifetime_relation<runtime_seq, runtime_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (lifetime_relation<compile_seq, compile_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (lifetime_relation<runtime_seq, const_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (lifetime_relation<compile_seq, compile_const_seq>::value));

    // compile-time is the stronger end
    D_CCM_REL(order_relation::greater,
              (lifetime_relation<compile_seq, runtime_seq>::value));
    D_CCM_REL(order_relation::less,
              (lifetime_relation<runtime_seq, compile_seq>::value));

    D_CCM_REL(order_relation::greater,
              (lifetime_relation<compile_const_seq, const_seq>::value));
    D_CCM_REL(order_relation::less,
              (lifetime_relation<const_seq, compile_const_seq>::value));

    // and the stages really are what they say
    D_CCM_CHECK(is_compile_time(container_lifetime<compile_seq>::value));
    D_CCM_CHECK(!is_compile_time(container_lifetime<runtime_seq>::value));

    // a non-container is runtime, so it is `less` than any compile-time container
    D_CCM_REL(order_relation::less,
              (lifetime_relation<not_a_container, compile_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (lifetime_relation<not_a_container, runtime_seq>::value));

    return ok;
}

/*
tests_lifetime_is_total
  lifetime_relation is a TOTAL order on two points - it can never be incomparable.

  That is a structural property, not a coincidence: the axis reads a single BOOL
  (is_compile_time of the container lifetime), and two bools are always ordered.
  It matters because the refinement meets four axes and incomparability is
  absorbing - an axis that could spuriously return `incomparable` would poison
  every verdict that passed through it.

  Tests the following:
  - over every ordered pair of fixtures in the suite, and over the hostile zoo,
    lifetime_relation never yields `incomparable`
  - the underlying lattice has FOUR values (none, compile_time, runtime, both) and
    the axis projects them onto ONE BIT.  So a container whose lifetime is `none`
    - the bottom - is treated as identical to a runtime one.  Worth pinning: the
    Lifetime axis is richer than what this module reads of it
*/
bool
tests_lifetime_is_total()
{
    bool ok = true;

    #define D_CCM_NOT_IC(LEFT, RIGHT)                                         \
        D_CCM_CHECK((lifetime_relation<LEFT, RIGHT>::value) !=                \
                    order_relation::incomparable);                            \
        D_CCM_CHECK((lifetime_relation<RIGHT, LEFT>::value) !=                \
                    order_relation::incomparable)

    D_CCM_NOT_IC(runtime_seq,       compile_seq);
    D_CCM_NOT_IC(runtime_seq,       const_seq);
    D_CCM_NOT_IC(compile_seq,       compile_const_seq);
    D_CCM_NOT_IC(not_a_container,   compile_seq);
    D_CCM_NOT_IC(not_iterable,      compile_const_seq);
    D_CCM_NOT_IC(unique_seq,        sorted_seq);
    D_CCM_NOT_IC(static_seq,        dynamic_seq);
    D_CCM_NOT_IC(int,               std_vector);
    D_CCM_NOT_IC(std_array, std_vector);
    D_CCM_NOT_IC(void,              std_set);

    #undef D_CCM_NOT_IC

    // ONE BIT out of four lattice values.  A container declared to have NO
    // lifetime at all reads the same as a runtime one on this axis
    D_CCM_CHECK(!is_compile_time(lifetime::none));
    D_CCM_CHECK(!is_compile_time(lifetime::runtime));
    D_CCM_CHECK(is_compile_time(lifetime::compile_time));
    D_CCM_CHECK(is_compile_time(lifetime::both));

    return ok;
}

/*
tests_iterability_bottom
  iterability_relation - the BOTTOM of the lattice: not-iterable.
  Tests the following:
  - two non-iterable types are equivalent - there is nothing to compare
  - an iterable type is `greater` than a non-iterable one, whatever the iterable
    one's stage and mode, because the bottom is below all four iterable kinds
  - the direction, both ways
  - a NON-CONTAINER and a countable-but-not-traversable CONTAINER are both at the
    bottom, and are therefore equivalent on this axis.  That is correct - and it is
    the half of the finding that lets the discipline axis's bogus `greater` survive
    the meet.  See tests_refinement_non_container
*/
bool
tests_iterability_bottom()
{
    bool ok = true;

    // both at the bottom
    D_CCM_REL(order_relation::equivalent,
              (iterability_relation<not_a_container, not_a_container>::value));
    D_CCM_REL(order_relation::equivalent,
              (iterability_relation<not_iterable, not_iterable>::value));
    D_CCM_REL(order_relation::equivalent,
              (iterability_relation<not_a_container, not_iterable>::value));
    D_CCM_REL(order_relation::equivalent,
              (iterability_relation<int, void>::value));

    // the bottom is below all four iterable kinds
    #define D_CCM_ABOVE_BOTTOM(ITERABLE)                                      \
        D_CCM_REL(order_relation::greater,                                    \
                  (iterability_relation<ITERABLE, not_iterable>::value));     \
        D_CCM_REL(order_relation::less,                                       \
                  (iterability_relation<not_iterable, ITERABLE>::value))

    D_CCM_ABOVE_BOTTOM(const_seq);          // (runtime, const)
    D_CCM_ABOVE_BOTTOM(runtime_seq);        // (runtime, non-const)
    D_CCM_ABOVE_BOTTOM(compile_const_seq);  // (compile, const)
    D_CCM_ABOVE_BOTTOM(compile_seq);        // (compile, non-const)

    #undef D_CCM_ABOVE_BOTTOM

    // and the fixtures really are what they claim
    D_CCM_CHECK(!is_iterable_container<not_iterable>::value);
    D_CCM_CHECK(!is_iterable_container<not_a_container>::value);
    D_CCM_CHECK(is_iterable_container<const_seq>::value);
    D_CCM_CHECK(iteration_mode_of<not_iterable>::value == iteration_mode::none);

    return ok;
}

/*
tests_iterability_lattice
  iterability_relation - the four iterable kinds, as the PRODUCT of two bits:

      compile-reachable  x  non-const-reachable

                              const          non-const
      runtime stage        (F, F)  const_seq       (F, T)  runtime_seq
      compile stage        (T, F)  compile_const   (T, T)  compile_seq

  Tests the following:
  - the diagonal: (T, T) is greater than (F, F) - strictly more capable on both
    bits
  - each single-bit step: (F, T) > (F, F), (T, F) > (F, F), (T, T) > (F, T),
    (T, T) > (T, F)
  - equality on both bits is equivalence, even between DIFFERENT types
  - the direction, at every site

  The two MIXED kinds are the interesting pair and have their own test; see
  tests_iterability_mixed_kinds.
*/
bool
tests_iterability_lattice()
{
    bool ok = true;

    #define D_CCM_ITER(EXPECTED, LEFT, RIGHT)                                 \
        D_CCM_REL((EXPECTED), (iterability_relation<LEFT, RIGHT>::value));    \
        D_CCM_REL(ccm_dual(EXPECTED),                                         \
                  (iterability_relation<RIGHT, LEFT>::value))

    // the diagonal: both bits up
    D_CCM_ITER(order_relation::greater, compile_seq, const_seq);

    // one bit at a time, up from the bottom iterable kind
    D_CCM_ITER(order_relation::greater, runtime_seq,       const_seq);
    D_CCM_ITER(order_relation::greater, compile_const_seq, const_seq);

    // one bit at a time, up to the top
    D_CCM_ITER(order_relation::greater, compile_seq, runtime_seq);
    D_CCM_ITER(order_relation::greater, compile_seq, compile_const_seq);

    #undef D_CCM_ITER

    // equal on both bits - and between different types
    D_CCM_REL(order_relation::equivalent,
              (iterability_relation<runtime_seq, runtime_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (iterability_relation<runtime_seq, nested_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (iterability_relation<runtime_seq, sorted_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (iterability_relation<const_seq, const_seq>::value));

    // the four cells really are the four cells
    D_CCM_CHECK(iteration_mode_of<const_seq>::value ==
                iteration_mode::const_only);
    D_CCM_CHECK(iteration_mode_of<runtime_seq>::value ==
                iteration_mode::non_const);
    D_CCM_CHECK(iteration_mode_of<compile_const_seq>::value ==
                iteration_mode::const_only);
    D_CCM_CHECK(iteration_mode_of<compile_seq>::value ==
                iteration_mode::non_const);

    D_CCM_CHECK(!is_compile_time(container_lifetime<const_seq>::value));
    D_CCM_CHECK(!is_compile_time(container_lifetime<runtime_seq>::value));
    D_CCM_CHECK(is_compile_time(container_lifetime<compile_const_seq>::value));
    D_CCM_CHECK(is_compile_time(container_lifetime<compile_seq>::value));

    return ok;
}

/*
tests_iterability_mixed_kinds
  THE REASON THE AXIS IS A LATTICE.

  The two MIXED kinds - compile/const and runtime/non-const - are INCOMPARABLE.
  One can be traversed at compile time but not written through; the other can be
  written through but only at run time.  Neither capability contains the other, so
  neither type refines the other, and the axis must say so.

  This is the single most easily lost property in the module.  Any implementation
  that scored capability as ONE NUMBER - two bits added, or a rank over the five
  kinds - would order this pair, and would report that a compile-time-traversable
  const container is "more capable than" or "less capable than" a runtime mutable
  one.  It is neither.

  Tests the following:
  - the two mixed kinds are incomparable, both ways
  - each is nevertheless strictly ABOVE the bottom iterable kind and strictly BELOW
    the top one - so they really are the middle of a lattice and not two points off
    to one side
  - the meet consequence: an incomparable axis poisons the whole refinement, which
    is what a product order is FOR
*/
bool
tests_iterability_mixed_kinds()
{
    bool ok = true;

    // *** the mixed kinds ***
    D_CCM_REL(order_relation::incomparable,
              (iterability_relation<compile_const_seq, runtime_seq>::value));
    D_CCM_REL(order_relation::incomparable,
              (iterability_relation<runtime_seq, compile_const_seq>::value));

    // ...and they really are mixed: each has one bit the other lacks
    D_CCM_CHECK(is_compile_time(container_lifetime<compile_const_seq>::value) &&
                !is_compile_time(container_lifetime<runtime_seq>::value));
    D_CCM_CHECK((iteration_mode_of<runtime_seq>::value ==
                 iteration_mode::non_const) &&
                (iteration_mode_of<compile_const_seq>::value ==
                 iteration_mode::const_only));

    // each is strictly between the bottom and the top iterable kind, so the
    // lattice really has a middle
    D_CCM_REL(order_relation::greater,
              (iterability_relation<compile_const_seq, const_seq>::value));
    D_CCM_REL(order_relation::less,
              (iterability_relation<compile_const_seq, compile_seq>::value));
    D_CCM_REL(order_relation::greater,
              (iterability_relation<runtime_seq, const_seq>::value));
    D_CCM_REL(order_relation::less,
              (iterability_relation<runtime_seq, compile_seq>::value));

    // and the meet consequence: one incomparable axis is enough
    D_CCM_REL(order_relation::incomparable,
              (refinement_of<compile_const_seq, runtime_seq>::value));

    // ...even though the LIFETIME axis, taken alone, would have ordered them
    D_CCM_REL(order_relation::greater,
              (lifetime_relation<compile_const_seq, runtime_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<compile_const_seq, runtime_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (storage_relation<compile_const_seq, runtime_seq>::value));

    // meet(equivalent, greater, incomparable, equivalent) = incomparable.  A
    // module that had summed the axes, or taken a majority, would have said
    // `greater` - and would have licensed substituting a const container where a
    // mutable one was wanted
    return ok;
}

/*
tests_storage_relation
  storage_relation - an EQUIVALENCE axis.  Content is invariant under siting, so
  two types either share a storage class or they do not; there is no ordering.
  Tests the following:
  - the same declared siting is equivalent
  - different sitings are INCOMPARABLE - never `greater`, never `less`, even for
    hybrid against static, where a "contains" reading would be tempting (hybrid IS
    static | dynamic as a bit pattern)
  - the axis yields ONLY equivalent and incomparable, over every pair in the suite
*/
bool
tests_storage_relation()
{
    bool ok = true;

    // same siting
    D_CCM_REL(order_relation::equivalent,
              (storage_relation<static_seq, static_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (storage_relation<dynamic_seq, dynamic_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (storage_relation<hybrid_seq, hybrid_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (storage_relation<runtime_seq, const_seq>::value));   // both unknown

    // different siting: INCOMPARABLE, both ways, always
    #define D_CCM_STOR_IC(LEFT, RIGHT)                                        \
        D_CCM_REL(order_relation::incomparable,                               \
                  (storage_relation<LEFT, RIGHT>::value));                    \
        D_CCM_REL(order_relation::incomparable,                               \
                  (storage_relation<RIGHT, LEFT>::value))

    D_CCM_STOR_IC(static_seq,  dynamic_seq);
    D_CCM_STOR_IC(static_seq,  hybrid_seq);
    D_CCM_STOR_IC(dynamic_seq, hybrid_seq);
    D_CCM_STOR_IC(static_seq,  runtime_seq);    // declared vs unknown
    D_CCM_STOR_IC(hybrid_seq,  runtime_seq);

    #undef D_CCM_STOR_IC

    // hybrid CONTAINS static as a bit pattern - and the axis still does not order
    // them, because it is an equivalence and not a lattice
    D_CCM_CHECK((static_cast<unsigned>(storage_duration::hybrid_storage) &
                 static_cast<unsigned>(storage_duration::static_storage)) != 0u);
    D_CCM_REL(order_relation::incomparable,
              (storage_relation<hybrid_seq, static_seq>::value));

    // the axis NEVER orders
    #define D_CCM_NEVER_ORDERED(LEFT, RIGHT)                                  \
        D_CCM_CHECK((storage_relation<LEFT, RIGHT>::value) !=                 \
                    order_relation::greater);                                 \
        D_CCM_CHECK((storage_relation<LEFT, RIGHT>::value) !=                 \
                    order_relation::less)

    D_CCM_NEVER_ORDERED(static_seq, dynamic_seq);
    D_CCM_NEVER_ORDERED(hybrid_seq, static_seq);
    D_CCM_NEVER_ORDERED(runtime_seq, static_seq);
    D_CCM_NEVER_ORDERED(std_vector, std_array);

    #undef D_CCM_NEVER_ORDERED

    return ok;
}

/*
tests_storage_inert
  THE FINDING.  The storage axis contributes NOTHING for any type anyone actually
  has.

  storage_of reads an opt-in `storage_duration_category` member and reports
  `unknown` when it is absent.  Nothing in the standard library declares it, and
  neither does anything in djinterp's own container set - so every real type
  reports `unknown`, every pair of real types compares `equivalent`, and the axis
  is the IDENTITY of the meet for all of them.

  The consequence is not that the axis is wrong.  It is that

      storage_relation<std_vector, std_array>   ==  equivalent

  - a heap-allocated growable vector and an inline fixed array are declared to
  SHARE A STORAGE CLASS.  And worse:

      storage_relation<std_vector, not_a_container>      ==  equivalent

  - a vector and an empty struct.  Two things about which the module knows nothing
  are reported as agreeing, rather than as unknown.  `unknown == unknown` is being
  read as agreement, when the honest reading is that there is no information.

  The header's own comment on storage_of says as much - "the structural inference
  (an extent, an allocator, a reserve()) that resolves the unknown case lives with
  the container storage traits, which feed this opt-in priority" - so the hook is
  known to be waiting for a supplier.  Until it has one, an axis that is always
  `equivalent` is an axis that is not there, and a caller reading the profile has
  no way to tell "these agree" from "nobody has said".

  A third relation - `unknown` against anything, including itself, yielding
  incomparable - would be the conservative reading and would cost one clause.

  Tests the following:
  - every standard container reports storage `unknown`
  - so every pair of them is `equivalent` on this axis, including pairs that
    manifestly are not
  - the axis is therefore the meet's identity for all of them, and drops out of
    every refinement over real types
  - it is NOT inert for the fixtures, which opt in - so the axis works, and simply
    has no data
*/
bool
tests_storage_inert()
{
    bool ok = true;

    // nothing in the standard library declares a siting
    D_CCM_CHECK(storage_of<std_vector>::value ==
                storage_duration::unknown);
    D_CCM_CHECK(storage_of<std_array>::value ==
                storage_duration::unknown);
    D_CCM_CHECK(storage_of<std_list>::value ==
                storage_duration::unknown);
    D_CCM_CHECK(storage_of<std_set>::value == storage_duration::unknown);
    D_CCM_CHECK(storage_of<int>::value == storage_duration::unknown);

    // ...so every pair of them "agrees"
    D_CCM_REL(order_relation::equivalent,
              (storage_relation<std_vector, std_array>::value));
    D_CCM_REL(order_relation::equivalent,
              (storage_relation<std_vector, std_list>::value));
    D_CCM_REL(order_relation::equivalent,
              (storage_relation<std_vector, not_a_container>::value));
    D_CCM_REL(order_relation::equivalent,
              (storage_relation<int, void>::value));

    // and the axis is the meet's IDENTITY for them, so it drops out entirely
    D_CCM_CHECK(order_relation_meet(
                    order_relation::greater,
                    storage_relation<std_vector,
                                     std_array>::value) ==
                order_relation::greater);

    // it is not BROKEN - it works perfectly for a type that opts in.  It simply
    // has no data
    D_CCM_CHECK(storage_of<static_seq>::value ==
                storage_duration::static_storage);
    D_CCM_REL(order_relation::incomparable,
              (storage_relation<static_seq, dynamic_seq>::value));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
