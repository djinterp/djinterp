/*******************************************************************************
* djinterp [core]                    container_conversion_traits_tests_paths.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_conversion_traits_tests_paths.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <type_traits>
// djinterp
#include "container_conversion_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_range_constructible
  is_range_constructible - To can be built from From's iterator range, To(begin,
  end).  The commonest construction path in the language, and the one every
  standard sequence and every standard associative container offers.
  Tests the following:
  - a To with a range constructor is detected
  - a To WITHOUT one is not - and does not diagnose
  - the standard sequences and associatives all have one
  - std::array does NOT.  It is an aggregate: no range constructor, no insert, and
    therefore NO CONSTRUCTIVE PATH AT ALL.  That is the fact behind the finding in
    tests_tier_of_no_path, and it is worth establishing here on its own
  - a non-container From is rejected by the FIRST conjunct, whatever To offers
*/
bool
tests_range_constructible()
{
    bool ok = true;

    // a range constructor is found
    D_CVT_CHECK((is_range_constructible<view_from, view_to>::value));
    D_CVT_CHECK((is_range_constructible<view_from, lever_elements>::value));
    D_CVT_CHECK((is_range_constructible<view_from, lever_layout>::value));
    D_CVT_CHECK((is_range_constructible<view_from, lossy_forget>::value));

    // ...and its absence is a false, not a diagnostic
    D_CVT_CHECK((!is_range_constructible<view_from, view_from>::value));
    D_CVT_CHECK((!is_range_constructible<view_from, insert_only>::value));
    D_CVT_CHECK((!is_range_constructible<view_from, no_path>::value));
    D_CVT_CHECK((!is_range_constructible<view_from, nc_runtime>::value));

    // the standard library
    D_CVT_CHECK((is_range_constructible<std_vector, std_list>::value));
    D_CVT_CHECK((is_range_constructible<std_vector, std_deque>::value));
    D_CVT_CHECK((is_range_constructible<std_vector, std_set>::value));
    D_CVT_CHECK((is_range_constructible<std_vector, std_multiset>::value));
    D_CVT_CHECK((is_range_constructible<std_set, std_vector>::value));
    D_CVT_CHECK((is_range_constructible<std_list, std_vector>::value));
    D_CVT_CHECK((is_range_constructible<std_vector, std_fwd_list>::value));

    // *** std::array has NO range constructor ***
    D_CVT_CHECK((!is_range_constructible<std_vector, std_array>::value));
    D_CVT_CHECK((!is_range_constructible<std_list, std_array>::value));
    D_CVT_CHECK((!is_range_constructible<std_array, std_array_64>::value));

    // ...and it is perfectly range-constructible FROM
    D_CVT_CHECK((is_range_constructible<std_array, std_vector>::value));

    // the first conjunct: a non-iterable From is rejected whatever To offers
    D_CVT_CHECK((!is_range_constructible<int, std_vector>::value));
    D_CVT_CHECK((!is_range_constructible<nc_runtime, view_to>::value));

    return ok;
}

/*
tests_range_insertable
  is_range_insertable - To accepts From's range through insert(begin, end).  The
  associative containers' path, and the one that catches a To with no range
  constructor.
  Tests the following:
  - a To with a two-argument insert(first, last) is detected
  - std::set and std::multiset have one; std::vector, std::list and std::deque do
    NOT

  The second half is the surprise, and it is correct.  A sequence's insert takes a
  POSITION first - insert(pos, first, last) is three arguments - and its
  two-argument overload is insert(pos, value).  So `vector.insert(begin, end)` is
  read as "insert the value `end` at position `begin`", which is ill-formed for
  vector<int> because an int* is not an int.  The probe therefore declines, exactly
  as it should, and has_constructive_path for every standard sequence rests
  ENTIRELY on the range constructor.
*/
bool
tests_range_insertable()
{
    bool ok = true;

    // a two-argument insert is found
    D_CVT_CHECK((is_range_insertable<view_from, insert_only>::value));
    D_CVT_CHECK((is_range_insertable<std_vector, std_set>::value));
    D_CVT_CHECK((is_range_insertable<std_vector, std_multiset>::value));
    D_CVT_CHECK((is_range_insertable<std_list, std_set>::value));

    // ...and the SEQUENCES do not have one.  Their two-argument insert is
    // insert(pos, value), and an iterator is not a value
    D_CVT_CHECK((!is_range_insertable<std_vector, std_vector>::value));
    D_CVT_CHECK((!is_range_insertable<std_vector, std_list>::value));
    D_CVT_CHECK((!is_range_insertable<std_vector, std_deque>::value));
    D_CVT_CHECK((!is_range_insertable<std_set, std_vector>::value));

    // nor does std::array, which has no insert at all
    D_CVT_CHECK((!is_range_insertable<std_vector, std_array>::value));

    // nor do the fixtures without one
    D_CVT_CHECK((!is_range_insertable<view_from, view_to>::value));
    D_CVT_CHECK((!is_range_insertable<view_from, no_path>::value));

    // the first conjunct again: a non-iterable From is rejected
    D_CVT_CHECK((!is_range_insertable<int, std_set>::value));
    D_CVT_CHECK((!is_range_insertable<nc_runtime, insert_only>::value));

    return ok;
}

/*
tests_path_iterable_gate
  THE FIRST CONJUNCT.  Both path traits are gated on
  is_iterable_container<From> BEFORE the helper is consulted, and the gate is not
  redundant.

  A RAW C ARRAY is the case that shows it.  std::begin and std::end work on `int[3]`
  - that is the whole point of the free functions - so

      std::vector<int>( std::begin(a), std::end(a) )

  is perfectly valid C++ and the helper alone would report it.  But
  is_iterable_container<int[3]> is FALSE (an array is not a CONTAINER), so
  is_range_constructible<int[3], std::vector<int>> is false as well, and the module
  will not admit a conversion the language performs every day.

  Whether that is a defect or a deliberate scoping to class types is a design
  question the suite does not answer.  What it does is pin it: the gate is live,
  it fires on exactly this shape, and the helper underneath it disagrees.

  Tests the following:
  - is_iterable_container<int[3]> is false, so both path traits are false
  - the underlying HELPER, named directly, is TRUE for the same pair - so the gate
    is what rejected it, and not the construction
  - the gate fires for every non-container From: an int, a void, a function type
*/
bool
tests_path_iterable_gate()
{
    bool ok = true;

    // the gate is closed for a raw array
    D_CVT_CHECK(!is_iterable_container<raw_array>::value);
    D_CVT_CHECK((!is_range_constructible<raw_array, std_vector>::value));
    D_CVT_CHECK((!is_range_insertable<raw_array, std_set>::value));
    D_CVT_CHECK((!has_constructive_path<raw_array, std_vector>::value));

    // ...and the HELPER underneath it says the construction is fine
    D_CVT_CHECK((internal::range_constructible_helper<std_vector,
                                                      raw_array>::value));
    D_CVT_CHECK((internal::range_insertable_helper<std_set,
                                                   raw_array>::value));

    // so the gate is what rejected it, and nothing else
    D_CVT_CHECK((internal::range_constructible_helper<std_vector,
                                                      raw_array>::value !=
                 is_range_constructible<raw_array, std_vector>::value));

    // the gate fires for every non-container From
    D_CVT_CHECK((!has_constructive_path<int, std_vector>::value));
    D_CVT_CHECK((!has_constructive_path<void, std_vector>::value));
    D_CVT_CHECK((!has_constructive_path<fn_type, std_vector>::value));
    D_CVT_CHECK((!has_constructive_path<nc_runtime, view_to>::value));

    // ...and opens for a real container
    D_CVT_CHECK(is_iterable_container<std_vector>::value);
    D_CVT_CHECK((has_constructive_path<std_vector, std_list>::value));

    return ok;
}

/*
tests_has_constructive_path
  has_constructive_path - a data-preserving construction of To from From exists.
  The disjunction of the two path traits.
  Tests the following:
  - either disjunct alone carries it: a range-constructible To, or a
    range-insertable one
  - the insert-only fixture is what proves the SECOND disjunct is doing work.  Every
    other To in the suite is range-constructible, so the disjunction would
    short-circuit before ever reaching the insert, and a suite without this fixture
    would have covered one half of an `||` and called it done
  - neither disjunct: no path
  - a From that is not a container: no path, whatever To offers
*/
bool
tests_has_constructive_path()
{
    bool ok = true;

    // the FIRST disjunct alone
    D_CVT_CHECK((is_range_constructible<view_from, view_to>::value));
    D_CVT_CHECK((!is_range_insertable<view_from, view_to>::value));
    D_CVT_CHECK((has_constructive_path<view_from, view_to>::value));

    // the SECOND disjunct alone - the fixture that exists for exactly this
    D_CVT_CHECK((!is_range_constructible<view_from, insert_only>::value));
    D_CVT_CHECK((is_range_insertable<view_from, insert_only>::value));
    D_CVT_CHECK((has_constructive_path<view_from, insert_only>::value));

    // BOTH disjuncts - the associative containers
    D_CVT_CHECK((is_range_constructible<std_vector, std_set>::value));
    D_CVT_CHECK((is_range_insertable<std_vector, std_set>::value));
    D_CVT_CHECK((has_constructive_path<std_vector, std_set>::value));

    // NEITHER
    D_CVT_CHECK((!is_range_constructible<view_from, no_path>::value));
    D_CVT_CHECK((!is_range_insertable<view_from, no_path>::value));
    D_CVT_CHECK((!has_constructive_path<view_from, no_path>::value));

    // *** std::array: neither, so no path exists to it at all ***
    D_CVT_CHECK((!has_constructive_path<std_vector, std_array>::value));
    D_CVT_CHECK((!has_constructive_path<std_list, std_array>::value));
    D_CVT_CHECK((!has_constructive_path<std_array, std_array_64>::value));

    // ...and it is the identity that is impossible too, which is the sharpest form
    D_CVT_CHECK((!has_constructive_path<std_array, std_array>::value));
    D_CVT_CHECK((has_constructive_path<std_vector, std_vector>::value));

    // it is exactly the disjunction, restated
    #define D_CVT_IS_DISJUNCTION(FROM, TO)                                    \
        D_CVT_CHECK((has_constructive_path<FROM, TO>::value) ==               \
                    ((is_range_constructible<FROM, TO>::value) ||             \
                     (is_range_insertable<FROM, TO>::value)))

    D_CVT_IS_DISJUNCTION(view_from,  view_to);
    D_CVT_IS_DISJUNCTION(view_from,  insert_only);
    D_CVT_IS_DISJUNCTION(view_from,  no_path);
    D_CVT_IS_DISJUNCTION(std_vector, std_set);
    D_CVT_IS_DISJUNCTION(std_vector, std_array);
    D_CVT_IS_DISJUNCTION(int,        std_vector);

    #undef D_CVT_IS_DISJUNCTION

    return ok;
}


NS_END  // testing
NS_END  // djinterp
