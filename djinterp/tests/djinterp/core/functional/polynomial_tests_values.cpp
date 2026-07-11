// djinterp [test]  polynomial_tests_values.cpp
//   Section I -- value-level poly_* vocabulary (var/unit/const/sum/product).

// std
#include <type_traits>
// djinterp
#include "polynomial_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_poly_var
  Exercises the recursive-position carrier poly_var<X> (F(X) = X).
  Tests the following:
  - the value_type alias is X
  - default construction value-initialises content
  - the explicit ctor stores its argument
  - content is a public, mutable member
  - the value ctor is explicit (no implicit X -> poly_var<X>)
*/
bool
tests_poly_var()
{
    bool ok = true;

    // value_type alias.
    ok = ok && (std::is_same<poly_var<int>::value_type, int>::value);

    // default construction value-initialises content (int() == 0).
    poly_var<int> a;
    ok = ok && (a.content == 0);

    // explicit ctor stores the value.
    poly_var<int> b(7);
    ok = ok && (b.content == 7);

    // content is public and mutable.
    b.content = -3;
    ok = ok && (b.content == -3);

    // the value ctor is explicit.
    ok = ok && (std::is_constructible<poly_var<int>, int>::value);
    ok = ok && (!std::is_convertible<int, poly_var<int> >::value);

    return ok;
}


/*
tests_poly_unit
  Exercises the unit carrier poly_unit<X> (F(X) = 1, X phantom).
  Tests the following:
  - the value_type alias is X
  - it is default-constructible and carries no payload
  - the phantom X distinguishes poly_unit<int> from poly_unit<double>
*/
bool
tests_poly_unit()
{
    bool ok = true;

    // value_type alias.
    ok = ok && (std::is_same<poly_unit<int>::value_type, int>::value);

    // default-constructible; the type exists purely as a marker.
    ok = ok && (std::is_default_constructible<poly_unit<int> >::value);
    poly_unit<int> u;
    (void)u;

    // the phantom parameter still distinguishes instantiations.
    ok = ok && (!std::is_same<poly_unit<int>, poly_unit<double> >::value);

    return ok;
}


/*
tests_poly_const
  Exercises the constant carrier poly_const<C, X> (F(X) = K_C, X phantom).
  Tests the following:
  - the constant_type and value_type aliases
  - default construction value-initialises the payload
  - the explicit ctor stores the constant payload
  - content is public and mutable
  - the value ctor (from C) is explicit
*/
bool
tests_poly_const()
{
    bool ok = true;

    // aliases.
    ok = ok && (std::is_same<poly_const<char, int>::constant_type,
                             char>::value);
    ok = ok && (std::is_same<poly_const<char, int>::value_type,
                             int>::value);

    // default construction value-initialises content.
    poly_const<int, double> a;
    ok = ok && (a.content == 0);

    // explicit ctor stores the constant.
    poly_const<char, int> b('a');
    ok = ok && (b.content == 'a');

    // public, mutable payload.
    b.content = 'z';
    ok = ok && (b.content == 'z');

    // the ctor from C is explicit.
    ok = ok && (std::is_constructible<poly_const<char, int>, char>::value);
    ok = ok && (!std::is_convertible<char, poly_const<char, int> >::value);

    return ok;
}


/*
tests_poly_sum
  Exercises the binary-sum carrier poly_sum<L, R> (F = L + R).
  Tests the following:
  - value_type is the arms' shared X
  - default construction selects the left arm
  - inj_left builds a left-tagged value holding the left arm
  - inj_right builds a right-tagged value holding the right arm
*/
bool
tests_poly_sum()
{
    bool ok = true;

    typedef poly_var<int>       arm_l;
    typedef poly_const<char,int> arm_r;
    typedef poly_sum<arm_l, arm_r> sum_t;

    // value_type is the shared X of the two arms.
    ok = ok && (std::is_same<sum_t::value_type, int>::value);

    // default construction is left-tagged.
    sum_t d;
    ok = ok && (d.is_left == true);

    // inj_left: left-tagged, holds the given left arm.
    sum_t l = sum_t::inj_left(arm_l(5));
    ok = ok && (l.is_left == true);
    ok = ok && (l.left.content == 5);

    // inj_right: right-tagged, holds the given right arm.
    sum_t r = sum_t::inj_right(arm_r('q'));
    ok = ok && (r.is_left == false);
    ok = ok && (r.right.content == 'q');

    return ok;
}


/*
tests_poly_product
  Exercises the binary-product carrier poly_product<F, G> (F = First x Second).
  Tests the following:
  - value_type is the components' shared X
  - default construction value-initialises both components
  - the two-argument ctor stores both components
  - first / second are public and mutable
*/
bool
tests_poly_product()
{
    bool ok = true;

    typedef poly_var<int>        comp_a;
    typedef poly_const<char, int> comp_b;
    typedef poly_product<comp_a, comp_b> prod_t;

    // value_type is the shared X of the components.
    ok = ok && (std::is_same<prod_t::value_type, int>::value);

    // default construction value-initialises both components.
    prod_t d;
    ok = ok && (d.first.content == 0);

    // the two-argument ctor stores both components.
    prod_t p(comp_a(4), comp_b('m'));
    ok = ok && (p.first.content  == 4);
    ok = ok && (p.second.content == 'm');

    // components are public and mutable.
    p.first.content = 9;
    ok = ok && (p.first.content == 9);

    return ok;
}


/*
tests_value_level_value_types
  Confirms the value_type / constant_type contracts across the vocabulary,
  including propagation of the shared X through the composite carriers.
  Tests the following:
  - the inner value_type of each leaf carrier
  - poly_sum / poly_product inherit value_type from their arms/components
  - nesting composites still propagates the shared X
*/
bool
tests_value_level_value_types()
{
    // leaf carriers.
    static_assert(std::is_same<poly_var<int>::value_type, int>::value,
                  "poly_var value_type");
    static_assert(std::is_same<poly_unit<int>::value_type, int>::value,
                  "poly_unit value_type");
    static_assert(std::is_same<poly_const<char, int>::value_type, int>::value,
                  "poly_const value_type");
    static_assert(std::is_same<poly_const<char, int>::constant_type,
                               char>::value,
                  "poly_const constant_type");

    // composites inherit the shared X from their arms / components.
    static_assert(
        std::is_same<poly_sum<poly_var<int>, poly_unit<int> >::value_type,
                     int>::value,
        "poly_sum value_type");
    static_assert(
        std::is_same<poly_product<poly_var<int>, poly_const<char, int> >
                         ::value_type,
                     int>::value,
        "poly_product value_type");

    // nested composite still propagates X.
    static_assert(
        std::is_same<
            poly_sum<poly_product<poly_var<int>, poly_unit<int> >,
                     poly_const<char, int> >::value_type,
            int>::value,
        "nested composite value_type");

    return true;
}


NS_END  // testing
NS_END  // djinterp
