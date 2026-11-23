/*******************************************************************************
* djinterp [cpp][functional]                         polynomial_tests_shapes.cpp
*
*
* path:      /tests/djinterp/cpp/functional/polynomial_tests_shapes.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  polynomial_tests_shapes.cpp
//   Section II - type-level shapes (poly_*_t, documentation-only builders).

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "polynomial_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_poly_constant_t
  Exercises the type-level constant shape poly_constant_t<C> (F(X) = K_C).
  Tests the following:
  - the constant_type alias echoes C, for several C
*/
bool tests_poly_constant_t()
{
    static_assert(std::is_same<poly_constant_t<char>::constant_type,
                               char>::value,
                  "poly_constant_t<char>");
    static_assert(std::is_same<poly_constant_t<int>::constant_type,
                               int>::value,
                  "poly_constant_t<int>");
    static_assert(std::is_same<
                      poly_constant_t<DPolyVar<int> >::constant_type,
                      DPolyVar<int> >::value,
                  "poly_constant_t of a carrier");

    return true;
}


/*
tests_poly_recursion_t
  Exercises the type-level recursion marker poly_recursion_t (F(X) = X).
  Tests the following:
  - it is a complete, empty, default-constructible class type
*/
bool tests_poly_recursion_t()
{
    static_assert(std::is_class<poly_recursion_t>::value,
                  "poly_recursion_t is a class");
    static_assert(std::is_empty<poly_recursion_t>::value,
                  "poly_recursion_t is empty");
    static_assert(std::is_default_constructible<poly_recursion_t>::value,
                  "poly_recursion_t is default-constructible");

    poly_recursion_t r;
    (void)r;

    return true;
}


/*
tests_poly_sum_t
  Exercises the type-level sum shape poly_sum_t<L, R> (F(X) = L(X) + R(X)).
  Tests the following:
  - the left and right aliases echo the arms
*/
bool tests_poly_sum_t()
{
    typedef poly_sum_t<DPolyVar<int>, DPolyUnit<int> > s;

    static_assert(std::is_same<s::left,  DPolyVar<int> >::value,
                  "poly_sum_t left");
    static_assert(std::is_same<s::right, DPolyUnit<int> >::value,
                  "poly_sum_t right");

    return true;
}


/*
tests_poly_product_t
  Exercises the type-level product shape poly_product_t<Variants...>.
  Tests the following:
  - children is the tuple of the variant list
  - arity equals the pack size for 0, 1, 2, and 3 variants
  - arity is a compile-time constant
*/
bool tests_poly_product_t()
{
    // children is the variant tuple.
    static_assert(
        std::is_same<
            poly_product_t<DPolyVar<int>, DPolyUnit<int> >::children,
            std::tuple<DPolyVar<int>, DPolyUnit<int> > >::value,
        "poly_product_t children");

    // arity mirrors the pack size (and is a constant expression).
    static_assert(poly_product_t<>::arity == 0u,
                  "arity 0");
    static_assert(poly_product_t<DPolyVar<int> >::arity == 1u,
                  "arity 1");
    static_assert(poly_product_t<DPolyVar<int>, DPolyUnit<int> >::arity == 2u,
                  "arity 2");
    static_assert(
        poly_product_t<DPolyVar<int>, DPolyUnit<int>,
                       DPolyConst<char, int> >::arity == 3u,
        "arity 3");

    // usable as a constant expression at runtime too.
    bool ok = true;
    const std::size_t n =
        poly_product_t<DPolyVar<int>, DPolyUnit<int> >::arity;
    ok = ok && (n == 2u);

    return ok;
}


/*
tests_poly_compose_t
  Exercises the type-level composition shape poly_compose_t<Outer, Inner>.
  Tests the following:
  - the outer and inner aliases echo the operands
*/
bool tests_poly_compose_t()
{
    typedef poly_compose_t<DPolyVar<int>, DPolyUnit<int> > c;

    static_assert(std::is_same<c::outer, DPolyVar<int> >::value,
                  "poly_compose_t outer");
    static_assert(std::is_same<c::inner, DPolyUnit<int> >::value,
                  "poly_compose_t inner");

    return true;
}


/*
tests_poly_mu_t
  Exercises the type-level initial-algebra shape poly_mu_t<F>.
  Tests the following:
  - the functor alias echoes F
*/
bool tests_poly_mu_t()
{
    static_assert(
        std::is_same<poly_mu_t<DPolyVar<int> >::functor,
                     DPolyVar<int> >::value,
        "poly_mu_t functor");

    return true;
}


NS_END  // testing
NS_END  // djinterp
