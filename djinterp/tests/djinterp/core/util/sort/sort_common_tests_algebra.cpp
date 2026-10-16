/*******************************************************************************
* djinterp [core]                                  sort_common_tests_algebra.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_common_tests_algebra.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_common_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_common_is_comparator_trait
bool tests_sort_common_is_comparator_trait()
{
    verdict result;

    //   sort_common.hpp no longer defines its own is_comparator; it pulls in
    // functional/comparator.hpp so the sort subsystem and the functional
    // layer share ONE trait.  These checks are what would catch a return of
    // the two clashing primaries the header comment warns about.
    result.check((is_comparator<less<int>, int>::value));
    result.check((is_comparator<greater<int>, int>::value));
    result.check((is_comparator<less<boxed_int>, boxed_int>::value));
    result.check((is_comparator<even_first, int>::value));

    //   A type that is not callable as (const T&, const T&) -> bool is not a
    // comparator.
    result.check((!is_comparator<int, int>::value));
    result.check((!is_comparator<std::vector<int>, int>::value));

    //   The wrapped run-time comparator models the trait too, which is what
    // lets the *_sort_ordered entry points hand it to an algorithm.
    result.check((is_comparator<
                      ::djinterp::internal::order_comparator<less<int> >,
                      int>::value));

    return result.ok();
}


// tests_sort_common_natural_matches_less
bool tests_sort_common_natural_matches_less()
{
    verdict     result;
    less<int>   primitive;
    std::size_t i;
    std::size_t j;
    int         values[6];

    //   The header documents less<T> as "the value-type equivalent of
    // natural<T>()".  That is a behavioural claim, so it is checked
    // behaviourally, over every ordered pair of a set that contains
    // duplicates and negatives.
    values[0] = -7;
    values[1] = -1;
    values[2] = 0;
    values[3] = 4;
    values[4] = 4;
    values[5] = 11;

    for (i = 0; i < 6; ++i)
    {
        for (j = 0; j < 6; ++j)
        {
            result.check(::djinterp::natural<int>()(values[i], values[j]) ==
                         primitive(values[i], values[j]));
        }
    }

    result.check((is_comparator<
                      decltype(::djinterp::natural<int>()), int>::value));

    return result.ok();
}


// tests_sort_common_reversed_matches_greater
bool tests_sort_common_reversed_matches_greater()
{
    verdict      result;
    greater<int> primitive;
    std::size_t  i;
    std::size_t  j;
    int          values[6];

    values[0] = -7;
    values[1] = -1;
    values[2] = 0;
    values[3] = 4;
    values[4] = 4;
    values[5] = 11;

    //   The matching claim for greater<T>: "natural<T>() | reversed()".
    for (i = 0; i < 6; ++i)
    {
        for (j = 0; j < 6; ++j)
        {
            result.check(
                (::djinterp::natural<int>() |
                 ::djinterp::reversed())(values[i], values[j]) ==
                primitive(values[i], values[j]));
        }
    }

    return result.ok();
}


// tests_sort_common_composed_comparator_sorts
bool tests_sort_common_composed_comparator_sorts()
{
    verdict                  result;
    std::vector<keyed_value> data;
    std::size_t              i;

    //   The point of the functional integration: a comparator built from the
    // algebra drops into a sort entry point with no glue.  by_key extracts
    // the key; then() breaks the resulting ties on the tag, which turns a
    // partial order into a total one and makes the expected output unique
    // even for an unstable algorithm.
    data.push_back(make_keyed(2, 30));
    data.push_back(make_keyed(1, 20));
    data.push_back(make_keyed(2, 10));
    data.push_back(make_keyed(1, 40));
    data.push_back(make_keyed(3, 0));

    insertion_sort(data.begin(),
                   data.end(),
                   ::djinterp::by_key(int_key_of()) |
                       ::djinterp::then(keyed_tag_less()));

    result.check(data[0] == make_keyed(1, 20));
    result.check(data[1] == make_keyed(1, 40));
    result.check(data[2] == make_keyed(2, 10));
    result.check(data[3] == make_keyed(2, 30));
    result.check(data[4] == make_keyed(3, 0));

    //   Reversed composes on top of that, and the result is still a
    // comparator the algorithms accept.
    insertion_sort(data.begin(),
                   data.end(),
                   ::djinterp::by_key(int_key_of()) |
                       ::djinterp::reversed());

    for (i = 1; i < data.size(); ++i)
    {
        result.check(data[i - 1].key >= data[i].key);
    }

    result.check(data[0].key == 3);
    result.check(data[4].key == 1);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
