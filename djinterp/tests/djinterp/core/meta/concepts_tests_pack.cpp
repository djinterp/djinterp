/*******************************************************************************
* djinterp [test]                                        concepts_tests_pack.cpp
*
*   Tests for concepts.hpp section II.11: parameter pack concepts.
* Covers: single_type, empty_pack, nonempty_pack, all_same,
* all_convertible_to, all_derived_from, single_type_arg,
* single_tuple_arg, nonvoid_pack.
*
*   Edge cases of particular interest:
*     - single_type / empty_pack / nonempty_pack partition every pack
*       by arity (0, 1, >1).
*     - all_same requires a leading type parameter (`_First`) plus the
*       rest pack, so the empty case is the one-type-only case
*       <_First> with an empty rest pack - which vacuously satisfies
*       the concept.
*     - single_tuple_arg is the new concept added during the audit;
*       it parallels djinterp::is_single_tuple_arg and accepts exactly
*       one parameter that must be a std::tuple specialization.
*     - all_derived_from / all_convertible_to vacuously hold on empty
*       packs because the fold expression's identity is `true`.
*
*
* path:      /tests/djinterp/core/meta/concepts_tests_pack.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "concepts_tests.hpp"


NS_DJINTERP_TEST


void
concepts_tests_pack(
    test_handler& _test_handler,
    test_option   _options...)
{
    using fixtures::polymorphic_class;
    using fixtures::derived_from_polymorphic;
    using fixtures::trivial_class;
    using fixtures::empty_tuple_t;
    using fixtures::single_element_tuple_t;
    using fixtures::heterogeneous_tuple_t;

    // -------------------------------------------------------------------
    // single_type -- exactly one element in the pack
    // -------------------------------------------------------------------

    DT_CONCEPT    ((single_type<int>));
    DT_CONCEPT    ((single_type<void>));                     // void counts
    DT_CONCEPT    ((single_type<trivial_class>));
    DT_NOT_CONCEPT((single_type<>));                         // empty
    DT_NOT_CONCEPT((single_type<int, double>));              // two
    DT_NOT_CONCEPT((single_type<int, int, int>));            // three

    // -------------------------------------------------------------------
    // empty_pack
    // -------------------------------------------------------------------

    DT_CONCEPT    ((empty_pack<>));
    DT_NOT_CONCEPT((empty_pack<int>));
    DT_NOT_CONCEPT((empty_pack<int, double>));

    // -------------------------------------------------------------------
    // nonempty_pack
    // -------------------------------------------------------------------

    DT_NOT_CONCEPT((nonempty_pack<>));
    DT_CONCEPT    ((nonempty_pack<int>));
    DT_CONCEPT    ((nonempty_pack<int, double>));
    DT_CONCEPT    ((nonempty_pack<int, double, char>));

    //   structural invariants over the arity-partition trio
    DT_CONCEPT    (empty_pack<>          != nonempty_pack<>);
    DT_CONCEPT    (empty_pack<int>       != nonempty_pack<int>);
    DT_NOT_CONCEPT((single_type<>        && nonempty_pack<>));
    DT_CONCEPT    ((single_type<int>     && nonempty_pack<int>));

    // -------------------------------------------------------------------
    // all_same -- leading type + rest pack, all identical
    // -------------------------------------------------------------------
    //   The concept is single-arg-minimum (the leading _First is not
    // optional); rest is a variadic pack.

    //   one arg: vacuously all_same (no rest to disagree with)
    DT_CONCEPT    ((all_same<int>));
    DT_CONCEPT    ((all_same<void>));

    //   two-plus: every entry must equal _First
    DT_CONCEPT    ((all_same<int, int>));
    DT_CONCEPT    ((all_same<int, int, int>));
    DT_CONCEPT    ((all_same<int, int, int, int>));
    DT_NOT_CONCEPT((all_same<int, long>));
    DT_NOT_CONCEPT((all_same<int, int, long>));
    DT_NOT_CONCEPT((all_same<int, long, int>));

    //   cv-qualifiers and references count - all_same uses same_as
    DT_NOT_CONCEPT((all_same<int, const int>));
    DT_NOT_CONCEPT((all_same<int, int&>));

    // -------------------------------------------------------------------
    // all_convertible_to -- every type convertible to target
    // -------------------------------------------------------------------

    //   empty pack: vacuously true
    DT_CONCEPT    ((all_convertible_to<int>));

    //   one or more: convertibility holds
    DT_CONCEPT    ((all_convertible_to<long,    int>));
    DT_CONCEPT    ((all_convertible_to<long,    int, short, char>));
    DT_CONCEPT    ((all_convertible_to<double,  int, float, char>));

    //   one element breaks the chain
    DT_NOT_CONCEPT((all_convertible_to<int*,    int, double>));         // none of these is int*
    DT_NOT_CONCEPT((all_convertible_to<int,     int, trivial_class>));  // trivial_class not int-convertible

    // -------------------------------------------------------------------
    // all_derived_from -- every type derived from base
    // -------------------------------------------------------------------

    //   empty: vacuously true
    DT_CONCEPT    ((all_derived_from<polymorphic_class>));

    //   the base is derived_from itself; a derived type is too
    DT_CONCEPT    ((all_derived_from<polymorphic_class, polymorphic_class>));
    DT_CONCEPT    ((all_derived_from<polymorphic_class, derived_from_polymorphic>));
    DT_CONCEPT    ((all_derived_from<polymorphic_class,
                                     polymorphic_class,
                                     derived_from_polymorphic>));

    //   trivial_class is unrelated to polymorphic_class
    DT_NOT_CONCEPT((all_derived_from<polymorphic_class,
                                     derived_from_polymorphic,
                                     trivial_class>));
    DT_NOT_CONCEPT((all_derived_from<polymorphic_class, int>));

    // -------------------------------------------------------------------
    // single_type_arg -- pack of exactly one element of type _Type
    // -------------------------------------------------------------------

    DT_CONCEPT    ((single_type_arg<int, int>));
    DT_NOT_CONCEPT((single_type_arg<int, long>));            // wrong type
    DT_NOT_CONCEPT((single_type_arg<int>));                  // empty rest pack
    DT_NOT_CONCEPT((single_type_arg<int, int, int>));        // two args
    DT_NOT_CONCEPT((single_type_arg<int, const int>));       // cv counts

    // -------------------------------------------------------------------
    // single_tuple_arg -- pack of exactly one element that IS a tuple
    // -------------------------------------------------------------------
    //   New concept added during the audit; parallels
    // djinterp::is_single_tuple_arg.

    DT_CONCEPT    ((single_tuple_arg<empty_tuple_t>));
    DT_CONCEPT    ((single_tuple_arg<single_element_tuple_t>));
    DT_CONCEPT    ((single_tuple_arg<heterogeneous_tuple_t>));

    //   empty pack: fails the size check
    DT_NOT_CONCEPT((single_tuple_arg<>));

    //   single non-tuple element: fails the content check
    DT_NOT_CONCEPT((single_tuple_arg<int>));
    DT_NOT_CONCEPT((single_tuple_arg<trivial_class>));
    DT_NOT_CONCEPT((single_tuple_arg<void>));

    //   two-plus elements: fails the size check, even if both ARE tuples
    DT_NOT_CONCEPT((single_tuple_arg<empty_tuple_t, empty_tuple_t>));
    DT_NOT_CONCEPT((single_tuple_arg<heterogeneous_tuple_t, int>));

    // -------------------------------------------------------------------
    // nonvoid_pack -- every type is non-void
    // -------------------------------------------------------------------

    //   empty pack: vacuously true (no void to find)
    DT_CONCEPT    ((nonvoid_pack<>));

    //   all non-void
    DT_CONCEPT    ((nonvoid_pack<int>));
    DT_CONCEPT    ((nonvoid_pack<int, double, char>));
    DT_CONCEPT    ((nonvoid_pack<void*>));                   // pointer-to-void is non-void

    //   any void breaks it
    DT_NOT_CONCEPT((nonvoid_pack<void>));
    DT_NOT_CONCEPT((nonvoid_pack<int, void>));
    DT_NOT_CONCEPT((nonvoid_pack<void, int>));
    DT_NOT_CONCEPT((nonvoid_pack<int, void, char>));
    DT_NOT_CONCEPT((nonvoid_pack<const void>));              // cv-qualified void counts

    return;
}


NS_END  // test
NS_END  // djinterp
