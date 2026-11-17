/*******************************************************************************
* djinterp [test]                                   array_tests_construction.cpp
*
*   Construction tests: value-initialization, the element-list constructor
* and its constraints and noexcept, copy and move, the factories, the options
* pack, the cells' constants, and constant-expression construction.
*
*
* path:      /tests/djinterp/cpp/container/array/array_tests_construction.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

// std
#include <string>       // std::string
#include <type_traits>  // std::is_constructible
#include <utility>      // std::move
// djinterp
#include "array_tests.hpp"  // suite header (normal face)


NS_DJINTERP
NS_TESTING

/*
tests_construction_default_value_initializes
  A default-constructed array value-initializes every element.
  Tests the following:
  - every element of a mutable int array is 0
  - every element of an immutable and a constexpr cell is 0
  - every element of a string array is empty
  - an extent-0 array default-constructs and is empty
*/
bool tests_construction_default_value_initializes()
{
    mi4 m;
    ii4 i;
    ci4 c;
    ms3 s;
    mi0 z;

    for (std::size_t k = 0; k < 4; ++k)
    {
        D_AY_CHECK(m[k] == 0);
        D_AY_CHECK(i[k] == 0);
        D_AY_CHECK(c[k] == 0);
    }

    for (std::size_t k = 0; k < 3; ++k)
    {
        D_AY_CHECK(s[k].empty());
    }

    D_AY_CHECK(z.empty());

    return true;
}

/*
tests_construction_element_list
  The element-list constructor stores, value-initializes and casts.
  Tests the following:
  - a full list lands in order
  - a short list value-initializes the remaining elements
  - each value is cast to value_type (double and long to int)
  - string elements are built from string literals
*/
bool tests_construction_element_list()
{
    mi4 full(1, 2, 3, 4);
    D_AY_CHECK(full[0] == 1 && full[1] == 2 && full[2] == 3 && full[3] == 4);

    mi4 part(7, 8);
    D_AY_CHECK(part[0] == 7 && part[1] == 8);
    D_AY_CHECK(part[2] == 0 && part[3] == 0);

    // the cast: 2.9 truncates to 2, a long narrows to int
    mi4 cast(1L, 2.9, 3u);
    D_AY_CHECK(cast[0] == 1 && cast[1] == 2 && cast[2] == 3);

    ms3 s("ab", "cd");
    D_AY_CHECK(s[0] == "ab" && s[1] == "cd" && s[2].empty());

    ii4 imm(5, 6, 7, 8);
    D_AY_CHECK(imm[0] == 5 && imm[3] == 8);

    return true;
}

/*
tests_construction_is_constrained
  The element-list constructor takes only what it can store.
  Tests the following:
  - up to extent castable values: constructible
  - one value more than the extent: not constructible
  - a value that doesn't cast to value_type: not constructible
  - an extent-0 array takes no values, but default-constructs
  - copy and move construction are still the implicit ones
*/
bool tests_construction_is_constrained()
{
    D_AY_CHECK(std::is_constructible<mi4, int, int, int, int>::value);
    D_AY_CHECK(std::is_constructible<mi4, int>::value);
    D_AY_CHECK(std::is_constructible<mi4, double, long>::value);
    D_AY_CHECK(std::is_constructible<ms3, const char*>::value);

    // too many
    D_AY_CHECK(!std::is_constructible<mi4, int, int, int, int, int>::value);

    // no cast from std::string to int
    D_AY_CHECK(!std::is_constructible<mi4, std::string>::value);
    D_AY_CHECK(!std::is_constructible<mi4, int, std::string>::value);

    // extent 0
    D_AY_CHECK(!std::is_constructible<mi0, int>::value);
    D_AY_CHECK(std::is_default_constructible<mi0>::value);

    // copy and move
    D_AY_CHECK(std::is_copy_constructible<mi4>::value);
    D_AY_CHECK(std::is_move_constructible<ms3>::value);
    D_AY_CHECK(std::is_copy_constructible<ii4>::value);

    return true;
}

/*
tests_construction_noexcept_follows_conversion
  The element-list constructor is noexcept when no cast can throw.
  Tests the following:
  - int values into an int array: noexcept
  - string literals into a string array: not noexcept (it allocates)
  - an int into ay_throws, whose converting constructor may throw: not
    noexcept
  - an ay_throws into ay_throws (its noexcept move): noexcept
  - default construction of an int array: noexcept
*/
bool tests_construction_noexcept_follows_conversion()
{
    using throws2 = DMutableIterableArray<ay_throws, 2>;

    D_AY_CHECK(std::is_nothrow_constructible<mi4, int, int>::value);
    D_AY_CHECK(!std::is_nothrow_constructible<ms3, const char*>::value);
    D_AY_CHECK(!std::is_nothrow_constructible<throws2, int>::value);
    D_AY_CHECK(std::is_nothrow_constructible<throws2, ay_throws>::value);
    D_AY_CHECK(std::is_nothrow_default_constructible<mi4>::value);

    // and the throwing path still builds the values
    throws2 t(3, 4);
    D_AY_CHECK(t[0].value == 3 && t[1].value == 4);

    return true;
}

/*
tests_construction_copy_and_move
  Copies and moves construct and assign.
  Tests the following:
  - copy from a const lvalue
  - copy from a non-const lvalue (not taken by the element-list
    constructor)
  - move of a string array carries its strings
  - copy and move assignment
  - an immutable array copy-constructs
*/
bool tests_construction_copy_and_move()
{
    const mi4 src(1, 2, 3, 4);
    mi4       a(src);
    D_AY_CHECK(a[0] == 1 && a[3] == 4);

    mi4 b(a);
    D_AY_CHECK(b[1] == 2);

    ms3 s("x", "y", "z");
    ms3 moved(std::move(s));
    D_AY_CHECK(moved[0] == "x" && moved[2] == "z");

    ms3 assigned;
    assigned = moved;
    D_AY_CHECK(assigned[1] == "y");

    ms3 move_assigned;
    move_assigned = std::move(assigned);
    D_AY_CHECK(move_assigned[1] == "y");

    const ii4 isrc(9, 8, 7, 6);
    ii4       icopy(isrc);
    D_AY_CHECK(icopy[0] == 9 && icopy[3] == 6);

    return true;
}

/*
tests_construction_make_array
  The two factories deduce the type and extent.
  Tests the following:
  - make_array(1, 2, 3) is DMutableIterableArray<int, 3>
  - the element type comes from the first value; the rest are cast
  - make_immutable_array yields DImmutableIterableArray
  - the values land in order
*/
bool tests_construction_make_array()
{
    auto a = make_array(1, 2, 3);
    D_AY_CHECK((std::is_same<decltype(a),
                             DMutableIterableArray<int, 3>>::value));
    D_AY_CHECK(a[0] == 1 && a[1] == 2 && a[2] == 3);

    // first value decides: int, so 2.5 becomes 2
    auto b = make_array(1, 2.5);
    D_AY_CHECK((std::is_same<decltype(b),
                             DMutableIterableArray<int, 2>>::value));
    D_AY_CHECK(b[1] == 2);

    auto c = make_immutable_array(4L, 5L);
    D_AY_CHECK((std::is_same<decltype(c),
                             DImmutableIterableArray<long, 2>>::value));
    D_AY_CHECK(c[0] == 4L && c[1] == 5L);

    auto d = make_array(std::string("p"), "q");
    D_AY_CHECK(d[0] == "p" && d[1] == "q");

    return true;
}

/*
tests_construction_from_options
  The options pack and its factory.
  Tests the following:
  - array_from_options_t<Opts...> is array<Opts...>
  - make_array_from_options appends DArrayOptExtent<count>
  - its defaults are the mutable, iterable cell
  - a lifetime option in the pack is kept
*/
bool tests_construction_from_options()
{
    using opts_t =
        array_from_options_t<array_opt_type<int>, DArrayOptExtent<2>>;
    D_AY_CHECK((std::is_same<opts_t,
                             array<array_opt_type<int>,
                                   DArrayOptExtent<2>>>::value));

    auto a = make_array_from_options<array_opt_type<int>>(4, 5, 6);
    D_AY_CHECK((std::is_same<decltype(a),
                             array<array_opt_type<int>,
                                   DArrayOptExtent<3>>>::value));
    D_AY_CHECK(decltype(a)::extent == 3);
    D_AY_CHECK(decltype(a)::lifetime == DArrayLifetime::mutable_lifetime);
    D_AY_CHECK(decltype(a)::iterable);
    D_AY_CHECK(a[0] == 4 && a[2] == 6);

    auto i = make_array_from_options<
        array_opt_type<int>,
        DContainerOptLifetime<container_lifetime::immutable>>(1, 2);
    D_AY_CHECK(decltype(i)::lifetime == DArrayLifetime::immutable_lifetime);
    D_AY_CHECK(i[1] == 2);

    return true;
}

/*
tests_construction_cell_constants
  Each alias's static constants.
  Tests the following:
  - extent is 4 for the six cells and 0 for mi0
  - lifetime and iterability per cell, and iterable as a bool
  - value_type is the element type
  - option order doesn't change the constants
*/
bool tests_construction_cell_constants()
{
    D_AY_CHECK(mi4::extent == 4 && ini4::extent == 4 && mi0::extent == 0);

    D_AY_CHECK(mi4::lifetime  == DArrayLifetime::mutable_lifetime);
    D_AY_CHECK(mni4::lifetime == DArrayLifetime::mutable_lifetime);
    D_AY_CHECK(ii4::lifetime  == DArrayLifetime::immutable_lifetime);
    D_AY_CHECK(ini4::lifetime == DArrayLifetime::immutable_lifetime);
    D_AY_CHECK(ci4::lifetime  == DArrayLifetime::constexpr_lifetime);
    D_AY_CHECK(cni4::lifetime == DArrayLifetime::constexpr_lifetime);

    D_AY_CHECK(mi4::iterability  == DArrayIterability::iterable);
    D_AY_CHECK(mni4::iterability == DArrayIterability::non_iterable);
    D_AY_CHECK(mi4::iterable && ii4::iterable && ci4::iterable);
    D_AY_CHECK(!mni4::iterable && !ini4::iterable && !cni4::iterable);

    D_AY_CHECK((std::is_same<mi4::value_type, int>::value));
    D_AY_CHECK((std::is_same<ms3::value_type, std::string>::value));

    // option order
    using fwd = array<array_opt_type<int>, DArrayOptExtent<4>>;
    using rev = array<DArrayOptExtent<4>, array_opt_type<int>>;
    D_AY_CHECK(fwd::extent == rev::extent);
    D_AY_CHECK((std::is_same<fwd::value_type, rev::value_type>::value));
    D_AY_CHECK(fwd::lifetime == rev::lifetime);
    D_AY_CHECK(fwd::iterability == rev::iterability);

    return true;
}

/*
tests_construction_is_constexpr
  Arrays in constant expressions.
  Tests the following:
  - a constexpr array is built from values and read by subscript
  - size() and back() are constant expressions
  - make_array is usable in a constant expression
  - an immutable cell is a constexpr variable too
*/
bool tests_construction_is_constexpr()
{
    constexpr mi4 k(1, 2, 3, 4);
    static_assert(k[2] == 3, "constexpr subscript");
    static_assert(k.size() == 4, "constexpr size");

    constexpr auto m = make_array(5, 6);
    static_assert(m.back() == 6, "constexpr make_array");

    constexpr ii4 i(7, 8, 9, 10);
    static_assert(i.front() == 7, "constexpr immutable cell");

    constexpr mi4 d;
    static_assert(d[3] == 0, "constexpr default construction");

    D_AY_CHECK(k[2] == 3 && m.back() == 6 && i.front() == 7 && d[3] == 0);

    return true;
}

NS_END  // testing
NS_END  // djinterp
