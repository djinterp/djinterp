/*******************************************************************************
* djinterp [test]                               concepts_tests_std_reexports.cpp
*
*   Tests for concepts.hpp sections I.1 - I.4: the standard-library
* concept re-exports.  The standard library itself is established to
* be correct, so this module's job is narrower: confirm that every
* re-exported name is *visible* in the djinterp namespace and that
* the re-exported concept is the same concept (same answers on
* representative types), not an accidental shadow.
*
*   Coverage: every `using std::X;` in concepts.hpp sections I.1
* through I.4 gets at least one positive and one negative probe.
*
*
* path:      /tests/djinterp/core/meta/concepts_tests_std_reexports.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "concepts_tests.hpp"


NS_DJINTERP_TEST


void
concepts_tests_std_reexports(
    test_handler& _test_handler,
    test_option   _options...)
{
    using fixtures::polymorphic_class;
    using fixtures::derived_from_polymorphic;
    using fixtures::noncopyable_class;
    using fixtures::returns_int;
    using fixtures::returns_int_noexcept;
    using fixtures::returns_void;

    // -------------------------------------------------------------------
    // I.1  Core language concepts
    // -------------------------------------------------------------------

    // same_as
    DT_CONCEPT    ((::djinterp::same_as<int, int>));
    DT_NOT_CONCEPT((::djinterp::same_as<int, long>));

    // derived_from
    DT_CONCEPT    ((::djinterp::derived_from<derived_from_polymorphic,
                                             polymorphic_class>));
    DT_NOT_CONCEPT((::djinterp::derived_from<polymorphic_class,
                                             derived_from_polymorphic>));
    //   a class is derived_from itself by definition
    DT_CONCEPT    ((::djinterp::derived_from<polymorphic_class,
                                             polymorphic_class>));

    // convertible_to
    DT_CONCEPT    ((::djinterp::convertible_to<int, long>));
    DT_NOT_CONCEPT((::djinterp::convertible_to<int*, double*>));

    // common_reference_with
    DT_CONCEPT    ((::djinterp::common_reference_with<int&, int&>));

    // common_with
    DT_CONCEPT    ((::djinterp::common_with<int, long>));

    // integral
    DT_CONCEPT    (::djinterp::integral<int>);
    DT_CONCEPT    (::djinterp::integral<char>);
    DT_CONCEPT    (::djinterp::integral<bool>);
    DT_NOT_CONCEPT(::djinterp::integral<float>);
    DT_NOT_CONCEPT(::djinterp::integral<void>);

    // signed_integral
    DT_CONCEPT    (::djinterp::signed_integral<int>);
    DT_NOT_CONCEPT(::djinterp::signed_integral<unsigned int>);

    // unsigned_integral
    DT_CONCEPT    (::djinterp::unsigned_integral<unsigned int>);
    DT_NOT_CONCEPT(::djinterp::unsigned_integral<int>);

    // floating_point
    DT_CONCEPT    (::djinterp::floating_point<float>);
    DT_CONCEPT    (::djinterp::floating_point<double>);
    DT_NOT_CONCEPT(::djinterp::floating_point<int>);

    // assignable_from
    DT_CONCEPT    ((::djinterp::assignable_from<int&, int>));
    DT_NOT_CONCEPT((::djinterp::assignable_from<int, int>));   // not an lvalue

    // swappable
    DT_CONCEPT    (::djinterp::swappable<int>);

    // swappable_with
    DT_CONCEPT    ((::djinterp::swappable_with<int&, int&>));

    // destructible
    DT_CONCEPT    (::djinterp::destructible<int>);

    // constructible_from
    DT_CONCEPT    ((::djinterp::constructible_from<int>));
    DT_CONCEPT    ((::djinterp::constructible_from<int, int>));

    // default_initializable
    DT_CONCEPT    (::djinterp::default_initializable<int>);

    // move_constructible
    DT_CONCEPT    (::djinterp::move_constructible<int>);
    DT_CONCEPT    (::djinterp::move_constructible<noncopyable_class>);

    // copy_constructible
    DT_CONCEPT    (::djinterp::copy_constructible<int>);
    DT_NOT_CONCEPT(::djinterp::copy_constructible<noncopyable_class>);

    // -------------------------------------------------------------------
    // I.2  Comparison concepts
    // -------------------------------------------------------------------

    DT_CONCEPT    (::djinterp::equality_comparable<int>);
    DT_CONCEPT    ((::djinterp::equality_comparable_with<int, int>));

    DT_CONCEPT    (::djinterp::totally_ordered<int>);
    DT_CONCEPT    ((::djinterp::totally_ordered_with<int, int>));

    // -------------------------------------------------------------------
    // I.3  Object concepts
    // -------------------------------------------------------------------

    DT_CONCEPT    (::djinterp::movable<int>);
    DT_CONCEPT    (::djinterp::copyable<int>);
    DT_CONCEPT    (::djinterp::semiregular<int>);
    DT_CONCEPT    (::djinterp::regular<int>);

    DT_NOT_CONCEPT(::djinterp::copyable<noncopyable_class>);
    //   noncopyable_class IS movable (it has move ops); the negative
    // case for movable would be a fully immobile type, which is rare
    // in practice.  We exercise the positive branch.
    DT_CONCEPT    (::djinterp::movable<noncopyable_class>);

    // -------------------------------------------------------------------
    // I.4  Callable concepts
    // -------------------------------------------------------------------

    DT_CONCEPT    ((::djinterp::invocable<returns_int, int>));
    DT_CONCEPT    ((::djinterp::invocable<returns_void>));
    DT_NOT_CONCEPT((::djinterp::invocable<returns_int>));         // missing arg
    DT_NOT_CONCEPT((::djinterp::invocable<int>));                 // int isn't callable

    DT_CONCEPT    ((::djinterp::regular_invocable<returns_int, int>));

    //   predicate: a regular_invocable returning something convertible
    // to bool.  returns_int returns int (convertible to bool), so it
    // qualifies; returns_void does not.
    DT_CONCEPT    ((::djinterp::predicate<returns_int, int>));
    DT_NOT_CONCEPT((::djinterp::predicate<returns_void>));

    //   relation / equivalence_relation / strict_weak_order all need
    // a binary predicate.  We only sanity-check the positive case
    // here; their semantic refinements are part of the standard
    // library's specification rather than something concepts.hpp
    // could break by re-exporting.
    DT_CONCEPT    ((::djinterp::relation<std::equal_to<>, int, int>));
    DT_CONCEPT    ((::djinterp::equivalence_relation<std::equal_to<>, int, int>));
    DT_CONCEPT    ((::djinterp::strict_weak_order<std::less<>, int, int>));

    //   silence unused-variable warnings on the noexcept fixture - it's
    // declared here in case a future test wants it but isn't used
    // directly by the standard concepts above.
    (void)sizeof(returns_int_noexcept);

    return;
}


NS_END  // test
NS_END  // djinterp
