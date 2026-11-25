/*******************************************************************************
* djinterp [cpp][meta]                          math_vocabulary_tests_values.cpp
*
*   Section I of math_vocabulary_tests.hpp: each enum's unknown member and
* underlying type, the order strength's ladder, the sentinels and the
* compile-time pairs.
*
*
* path:      /tests/djinterp/cpp/meta/math_vocabulary_tests_values.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "math_vocabulary_tests.hpp"  // the suite


NS_DJINTERP
NS_TESTING

namespace
{
    // is_zero_default
    //   function (template): true when the enum's member is 0 and a
    // value-initialized enum equals it.
    template<typename Enum>
    bool is_zero_default(
        Enum _unknown
    )
    {
        using U = typename std::underlying_type<Enum>::type;

        return ( (static_cast<U>(_unknown) == 0) &&
                 (Enum{} == _unknown) );
    }
}

/*
tests_values_unknown_is_zero_and_default
  Every enum's "not classified" member is 0 and its default.
  Tests the following:
  - unknown is 0 in each of the twelve enums with one
  - DRelationProperty's none is 0
  - a value-initialized enum equals that member
*/
bool tests_values_unknown_is_zero_and_default()
{
    D_MV_CHECK(is_zero_default(DParity::unknown));
    D_MV_CHECK(is_zero_default(DSign::unknown));
    D_MV_CHECK(is_zero_default(DHandedness::unknown));
    D_MV_CHECK(is_zero_default(DCardinality::unknown));
    D_MV_CHECK(is_zero_default(DSelfLoopPolicy::unknown));
    D_MV_CHECK(is_zero_default(DDirectionality::unknown));
    D_MV_CHECK(is_zero_default(DOrientation::unknown));
    D_MV_CHECK(is_zero_default(DClosure::unknown));
    D_MV_CHECK(is_zero_default(DOrderStrength::unknown));
    D_MV_CHECK(is_zero_default(DStrictness::unknown));
    D_MV_CHECK(is_zero_default(DRelationKind::unknown));
    D_MV_CHECK(is_zero_default(DAlgebraicStructure::unknown));
    D_MV_CHECK(is_zero_default(DRelationProperty::none));

    return true;
}

/*
tests_values_underlying_types
  Each enum's underlying type is the documented one.
  Tests the following:
  - DSign is signed, int8_t: negative is -1
  - DRelationProperty is uint16_t, wide enough for its bits
  - the others are uint8_t
  - each is an enum class: no implicit conversion to its underlying type
*/
bool tests_values_underlying_types()
{
    D_MV_CHECK(std::is_same<std::underlying_type<DSign>::type,
                            re_std::int8_t>::value);
    D_MV_CHECK(static_cast<int>(DSign::negative) == -1);
    D_MV_CHECK(std::is_same<std::underlying_type<DRelationProperty>::type,
                            re_std::uint16_t>::value);

    D_MV_CHECK(std::is_same<std::underlying_type<DParity>::type,
                            re_std::uint8_t>::value);
    D_MV_CHECK(std::is_same<std::underlying_type<DCardinality>::type,
                            re_std::uint8_t>::value);
    D_MV_CHECK(std::is_same<std::underlying_type<DDirectionality>::type,
                            re_std::uint8_t>::value);
    D_MV_CHECK(std::is_same<std::underlying_type<DOrderStrength>::type,
                            re_std::uint8_t>::value);
    D_MV_CHECK(std::is_same<std::underlying_type<DAlgebraicStructure>::type,
                            re_std::uint8_t>::value);

    D_MV_CHECK(!std::is_convertible<DOrderStrength, int>::value);
    D_MV_CHECK(!std::is_convertible<DRelationProperty, unsigned>::value);

    return true;
}

/*
tests_values_order_strength_is_a_ladder
  DOrderStrength rises from no order to a well-order.
  Tests the following:
  - unordered < sequenced < partial < total < well_ordered, by value
  - unknown is below them all
  - the members are distinct
*/
bool tests_values_order_strength_is_a_ladder()
{
    using U = std::underlying_type<DOrderStrength>::type;
    const U ladder[] = {
        static_cast<U>(DOrderStrength::unknown),
        static_cast<U>(DOrderStrength::unordered),
        static_cast<U>(DOrderStrength::sequenced),
        static_cast<U>(DOrderStrength::partial),
        static_cast<U>(DOrderStrength::total),
        static_cast<U>(DOrderStrength::well_ordered)
    };
    std::size_t i;

    for (i = 1; i < (sizeof(ladder) / sizeof(ladder[0])); ++i)
    {
        D_MV_CHECK(ladder[i - 1] < ladder[i]);
    }

    return true;
}

/*
tests_values_sentinels_and_sizes
  The size types and the sentinels.
  Tests the following:
  - arity_t and degree_t are std::size_t
  - kVariadicArity, kInvalidSize and kInvalidIndex are std::size_t(-1)
  - each is a constant expression
*/
bool tests_values_sentinels_and_sizes()
{
    D_MV_CHECK(std::is_same<arity_t, std::size_t>::value);
    D_MV_CHECK(std::is_same<degree_t, std::size_t>::value);

    D_MV_CHECK(kVariadicArity == static_cast<std::size_t>(-1));
    D_MV_CHECK(kInvalidSize == static_cast<std::size_t>(-1));
    D_MV_CHECK(kInvalidIndex == static_cast<std::size_t>(-1));

    static_assert(kInvalidIndex == kInvalidSize,
                  "the sentinels are constant expressions");

    return true;
}

/*
tests_values_compile_time_pairs
  The compile-time pair and triple helpers.
  Tests the following:
  - DTypePair and DTypeTriple name their types first, second and third
  - DValuePair carries two values and their type
  - DIndexedValue carries an index, a value and its type
*/
bool tests_values_compile_time_pairs()
{
    using P = DTypePair<int, char>;
    using T = DTypeTriple<int, char, double>;
    using V = DValuePair<int, 3, 7>;
    using I = DIndexedValue<4, long, -2>;

    D_MV_CHECK(std::is_same<P::first, int>::value);
    D_MV_CHECK(std::is_same<P::second, char>::value);
    D_MV_CHECK(std::is_same<T::third, double>::value);

    D_MV_CHECK(std::is_same<V::value_type, int>::value);
    D_MV_CHECK(V::first == 3);
    D_MV_CHECK(V::second == 7);

    D_MV_CHECK(I::index == 4);
    D_MV_CHECK(std::is_same<I::value_type, long>::value);
    D_MV_CHECK(I::value == -2);

    return true;
}

NS_END  // testing
NS_END  // djinterp
