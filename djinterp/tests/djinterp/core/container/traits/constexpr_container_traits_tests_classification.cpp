/*******************************************************************************
* djinterp [core]            constexpr_container_traits_tests_classification.cpp
*
*
* path:      /tests/djinterp/core/container/traits/constexpr_container_traits_tests_classification.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <list>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
// djinterp
#include "constexpr_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_is_constexpr_positive
  is_constexpr_container - the types that ARE usable in constant evaluation.
  Tests the following:
  - a fixed array of literal elements
  - a literal type with a constexpr size()
  - a type promoted by tuple_size alone, whose contents are literal
  - a type with constexpr iteration and literal contents
  - a type whose contents are declared compile-time-only (lifetime::compile_time
    is still a compile-time capability - `is_constexpr_container` is the
    compile-time BIT, not the `both` value)
  - a tagged type - including one whose contents are not literal at all (the
    tag is an unchecked assertion; see tests_container_lifetime_tag_bypass)
*/
bool
tests_is_constexpr_positive()
{
    bool ok = true;

    D_CCT_CHECK(is_constexpr_container<std::array<int, 3>>::value);
    D_CCT_CHECK(is_constexpr_container<std::array<int, 0>>::value);
    D_CCT_CHECK(is_constexpr_container<size_constexpr>::value);
    D_CCT_CHECK(is_constexpr_container<tuple_conformer>::value);
    D_CCT_CHECK(is_constexpr_container<iter_alias>::value);
    D_CCT_CHECK(is_constexpr_container<iter_pair>::value);
    D_CCT_CHECK(is_constexpr_container<extent_static>::value);
    D_CCT_CHECK(is_constexpr_container<extent_zero>::value);

    // compile-time-ONLY contents still carry the compile-time bit
    D_CCT_CHECK(is_constexpr_container<life_compile_sized>::value);
    D_CCT_CHECK(is_constexpr_container<life_both_sized>::value);

    // the tag, believed
    D_CCT_CHECK(is_constexpr_container<tag_true>::value);
    D_CCT_CHECK(is_constexpr_container<tag_true_nonliteral>::value);

    return ok;
}

/*
tests_is_constexpr_negative
  The types that are NOT.
  Tests the following:
  - a growable container: its contents are not constexpr-constructible, so the
    meet clears the compile-time bit even where the SIZE probe fires (which, on
    a C++20 library, it does - see tests_stdlib_vector_string)
  - a fixed array of NON-literal elements: compile-time size, runtime contents
  - an empty struct: literal CONTENTS, but no compile-time size - so not a
    constexpr CONTAINER.  This is the distinction the trait exists to draw, and
    it is the one most likely to be misread as a bug
  - a plain int, for the same reason
  - a type whose declared lifetime is `none`, and one whose compile-time-only
    contents sit in a runtime-sized container (the two routes to the bottom)
*/
bool
tests_is_constexpr_negative()
{
    bool ok = true;

    D_CCT_CHECK(!is_constexpr_container<std::vector<int>>::value);
    D_CCT_CHECK(!is_constexpr_container<std::list<int>>::value);
    D_CCT_CHECK(!is_constexpr_container<std::string>::value);

    // compile-time size, runtime contents
    D_CCT_CHECK(!is_constexpr_container<extent_nonliteral>::value);
    D_CCT_CHECK(!is_constexpr_container<tuple_conformer_nonliteral>::value);

    // literal contents, runtime size - constexpr-CAPABLE, but not a constexpr
    // CONTAINER
    D_CCT_CHECK(!is_constexpr_container<no_signal>::value);
    D_CCT_CHECK(!is_constexpr_container<int>::value);
    D_CCT_CHECK(is_literal_type<no_signal>::value);
    D_CCT_CHECK(is_literal_type<int>::value);

    // both routes to the bottom of the lattice
    D_CCT_CHECK(!is_constexpr_container<life_none_sized>::value);
    D_CCT_CHECK(!is_constexpr_container<life_compile_unsized>::value);

    // and a declared runtime lifetime
    D_CCT_CHECK(!is_constexpr_container<life_runtime_sized>::value);

    D_CCT_CHECK(!is_constexpr_container<no_signal_nonliteral>::value);
    D_CCT_CHECK(!is_constexpr_container<size_runtime>::value);

    return ok;
}

/*
tests_is_constexpr_is_the_compile_time_bit
  is_constexpr_container is defined as is_compile_time(container_lifetime<T>),
  and nothing else.
  Tests the following:
  - the biconditional holds across the whole fixture set
  - in particular it is TRUE for lifetime::compile_time as well as for
    lifetime::both - it is the BIT, not the value.  A rewrite to
    `container_lifetime<T>::value == lifetime::both` would pass every test that
    only uses ordinary literal types and would silently exclude every
    compile-time-only container
*/
bool
tests_is_constexpr_is_the_compile_time_bit()
{
    bool ok = true;

    #define D_CCT_IS_BIT(...)                                                 \
        D_CCT_CHECK(is_constexpr_container<__VA_ARGS__>::value ==             \
                    is_compile_time(                                          \
                        container_lifetime<__VA_ARGS__>::value))

    D_CCT_IS_BIT(std::array<int, 3>);
    D_CCT_IS_BIT(std::vector<int>);
    D_CCT_IS_BIT(size_constexpr);
    D_CCT_IS_BIT(tuple_conformer);
    D_CCT_IS_BIT(tuple_conformer_nonliteral);
    D_CCT_IS_BIT(extent_static);
    D_CCT_IS_BIT(extent_nonliteral);
    D_CCT_IS_BIT(iter_pair);
    D_CCT_IS_BIT(tag_true);
    D_CCT_IS_BIT(tag_true_nonliteral);
    D_CCT_IS_BIT(tag_false_with_extent);
    D_CCT_IS_BIT(life_both_sized);
    D_CCT_IS_BIT(life_compile_sized);
    D_CCT_IS_BIT(life_runtime_sized);
    D_CCT_IS_BIT(life_none_sized);
    D_CCT_IS_BIT(life_compile_unsized);
    D_CCT_IS_BIT(no_signal);
    D_CCT_IS_BIT(int);

    #undef D_CCT_IS_BIT

    // the bit, not the value: compile_time counts, and it is not `both`
    D_CCT_CHECK(container_lifetime<life_compile_sized>::value !=
                lifetime::both);
    D_CCT_CHECK(is_constexpr_container<life_compile_sized>::value);

    return ok;
}

/*
tests_is_not_constexpr
  is_not_constexpr_container - the explicit negation.
  Tests the following:
  - it is the strict complement of is_constexpr_container, for every fixture
  - it is a real trait, not a synonym: it derives from std::integral_constant
    and so can be used where a bool trait is required
*/
bool
tests_is_not_constexpr()
{
    bool ok = true;

    D_CCT_CHECK(is_not_constexpr_container<std::vector<int>>::value);
    D_CCT_CHECK(is_not_constexpr_container<no_signal>::value);
    D_CCT_CHECK(is_not_constexpr_container<extent_nonliteral>::value);
    D_CCT_CHECK(is_not_constexpr_container<life_none_sized>::value);
    D_CCT_CHECK(is_not_constexpr_container<int>::value);

    D_CCT_CHECK(!is_not_constexpr_container<std::array<int, 3>>::value);
    D_CCT_CHECK(!is_not_constexpr_container<size_constexpr>::value);
    D_CCT_CHECK(!is_not_constexpr_container<tuple_conformer>::value);
    D_CCT_CHECK(!is_not_constexpr_container<tag_true>::value);
    D_CCT_CHECK(!is_not_constexpr_container<life_compile_sized>::value);

    return ok;
}

/*
tests_classification_exhaustive
  The two verdicts partition every type: exactly one of them is true, always.
  Tests the following:
  - is_constexpr_container XOR is_not_constexpr_container, over the whole
    fixture set and the whole hostile zoo

  Unlike the boundedness axis - where `unknown` is a real third state - this
  classification is total, because it is the projection of a lattice onto one
  bit.  Worth checking rather than assuming: is_not_constexpr_container is
  written as `!is_constexpr_container<_Type>::value` and passes _Type UNCLEANED,
  relying on is_constexpr_container to clean it.  If either ever stops cleaning,
  the two stop being complements for cv-ref-qualified arguments and nothing else
  would say so.
*/
bool
tests_classification_exhaustive()
{
    bool ok = true;

    #define D_CCT_XOR(...)                                                    \
        D_CCT_CHECK(is_constexpr_container<__VA_ARGS__>::value !=             \
                    is_not_constexpr_container<__VA_ARGS__>::value)

    D_CCT_XOR(std::array<int, 3>);
    D_CCT_XOR(std::vector<int>);
    D_CCT_XOR(std::tuple<int, char>);
    D_CCT_XOR(size_constexpr);
    D_CCT_XOR(tuple_conformer);
    D_CCT_XOR(extent_nonliteral);
    D_CCT_XOR(tag_true_nonliteral);
    D_CCT_XOR(life_compile_unsized);
    D_CCT_XOR(no_signal);
    D_CCT_XOR(int);
    D_CCT_XOR(void);

    // and through the cv-ref forms, where the uncleaned argument would show
    D_CCT_XOR(const std::array<int, 3>&);
    D_CCT_XOR(std::array<int, 3>&&);
    D_CCT_XOR(const volatile std::vector<int>&);

    #undef D_CCT_XOR

    // stated over the whole zoo at once: nothing is a constexpr container, and
    // therefore everything is not one
    D_CCT_CHECK((holds_for_none<is_constexpr_container,
                                D_TEST_HOSTILE_TYPES_COMPLETE>::value));
    D_CCT_CHECK((holds_for_all<is_not_constexpr_container,
                               D_TEST_HOSTILE_TYPES_COMPLETE>::value));

    return ok;
}

/*
tests_classification_value_companions
  The `_v` companions of the two verdicts.
  Tests the following:
  - is_constexpr_container_v and is_not_constexpr_container_v track their traits
*/
bool
tests_classification_value_companions()
{
    bool ok = true;

    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(is_constexpr_container,
                                      std::array<int, 3>));
    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(is_constexpr_container,
                                      std::vector<int>));
    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(is_constexpr_container, no_signal));

    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(is_not_constexpr_container,
                                      std::vector<int>));
    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(is_not_constexpr_container,
                                      std::array<int, 3>));

    D_CCT_CHECK(is_constexpr_container_v<std::array<int, 3>>);
    D_CCT_CHECK(!is_constexpr_container_v<std::vector<int>>);
    D_CCT_CHECK(is_not_constexpr_container_v<std::vector<int>>);
    D_CCT_CHECK(!is_not_constexpr_container_v<std::array<int, 3>>);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
