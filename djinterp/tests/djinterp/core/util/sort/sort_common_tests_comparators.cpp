/*******************************************************************************
* djinterp [core]                              sort_common_tests_comparators.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_common_tests_comparators.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_common_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_common_less_semantics
bool tests_sort_common_less_semantics()
{
    verdict     result;
    less<int>   comparator;

    //   less<T> is the subsystem's ascending primitive and the default for
    // every algorithm's two-argument entry point.
    result.check(comparator(1, 2));
    result.check(!comparator(2, 1));
    result.check(!comparator(2, 2));

    //   Negative and boundary values: the comparator must be a plain
    // operator< forward, with no unsigned arithmetic creeping in.
    result.check(comparator(-5, 0));
    result.check(comparator(-5, -1));
    result.check(!comparator(0, -5));

    //   Instances are stateless and cheap to copy, which is what lets every
    // algorithm take the comparator by value.
    result.check(std::is_empty<less<int> >::value);
    result.check((std::is_copy_constructible<less<int> >::value));

    return result.ok();
}


// tests_sort_common_greater_semantics
bool tests_sort_common_greater_semantics()
{
    verdict      result;
    greater<int> comparator;

    result.check(comparator(2, 1));
    result.check(!comparator(1, 2));
    result.check(!comparator(2, 2));

    result.check(comparator(0, -5));
    result.check(!comparator(-5, 0));

    result.check(std::is_empty<greater<int> >::value);

    return result.ok();
}


// tests_sort_common_comparator_irreflexivity
bool tests_sort_common_comparator_irreflexivity()
{
    verdict      result;
    less<int>    ascending;
    greater<int> descending;
    int          values[7];
    std::size_t  i;
    std::size_t  j;
    std::size_t  k;

    values[0] = -3;
    values[1] = -1;
    values[2] = 0;
    values[3] = 1;
    values[4] = 1;
    values[5] = 2;
    values[6] = 9;

    //   Both primitives must be genuine strict weak orderings, because every
    // algorithm in the subsystem relies on that and several would loop or
    // read out of bounds without it.  The three laws are checked
    // exhaustively over a small set that contains a duplicate pair.
    for (i = 0; i < 7; ++i)
    {
        // irreflexive
        result.check(!ascending(values[i], values[i]));
        result.check(!descending(values[i], values[i]));

        for (j = 0; j < 7; ++j)
        {
            // asymmetric
            if (ascending(values[i], values[j]))
            {
                result.check(!ascending(values[j], values[i]));
            }

            if (descending(values[i], values[j]))
            {
                result.check(!descending(values[j], values[i]));
            }

            for (k = 0; k < 7; ++k)
            {
                // transitive
                if ( (ascending(values[i], values[j])) &&
                     (ascending(values[j], values[k])) )
                {
                    result.check(ascending(values[i], values[k]));
                }

                if ( (descending(values[i], values[j])) &&
                     (descending(values[j], values[k])) )
                {
                    result.check(descending(values[i], values[k]));
                }
            }
        }
    }

    //   The two primitives are each other's mirror image on distinct
    // values, and agree that equivalents are equivalent.
    for (i = 0; i < 7; ++i)
    {
        for (j = 0; j < 7; ++j)
        {
            result.check(ascending(values[i], values[j]) ==
                         descending(values[j], values[i]));
        }
    }

    return result.ok();
}


// tests_sort_common_comparator_on_user_type
bool tests_sort_common_comparator_on_user_type()
{
    verdict                result;
    less<boxed_int>        ascending;
    greater<boxed_int>     descending;
    std::vector<boxed_int> data;
    std::size_t            i;

    //   less<T> and greater<T> reach a user type's operator< and operator>
    // through ordinary overload resolution -- they are not specialised for
    // built-ins.
    result.check(ascending(make_boxed(1), make_boxed(2)));
    result.check(!ascending(make_boxed(2), make_boxed(1)));
    result.check(!ascending(make_boxed(2), make_boxed(2)));

    result.check(descending(make_boxed(2), make_boxed(1)));
    result.check(!descending(make_boxed(1), make_boxed(2)));

    //   And they drive a real sort of that user type end to end.
    data.push_back(make_boxed(5));
    data.push_back(make_boxed(1));
    data.push_back(make_boxed(4));
    data.push_back(make_boxed(1));
    data.push_back(make_boxed(9));

    insertion_sort(data.begin(), data.end(), ascending);

    for (i = 1; i < data.size(); ++i)
    {
        result.check(!ascending(data[i], data[i - 1]));
    }

    result.check(data[0].value == 1);
    result.check(data[4].value == 9);

    insertion_sort(data.begin(), data.end(), descending);

    result.check(data[0].value == 9);
    result.check(data[4].value == 1);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
