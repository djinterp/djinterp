/*******************************************************************************
* djinterp [core]                                      sort_monoid_tests_run.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_monoid_tests_run.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_monoid_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_monoid_run_default_construction
bool tests_sort_monoid_run_default_construction()
{
    verdict result;
    int_run run;

    //   The default-constructed run is the monoid identity: no elements, a
    // default-constructed comparator.
    result.check(run.empty());
    result.check(run.size() == 0);
    result.check(run.data().empty());

    //   The comparator is present and usable even though the run is empty --
    // combine() reaches for the left operand's comparator, so an empty left
    // operand must still carry one.
    result.check(run.compare()(1, 2));
    result.check(!run.compare()(2, 1));

    //   The published member types.
    result.check((std::is_same<int_run::value_type, int>::value));
    result.check((std::is_same<int_run::compare_type, less<int> >::value));
    result.check((std::is_same<int_run::container_type,
                               std::vector<int> >::value));
    result.check((std::is_same<int_run::compare_type,
                               default_comparator_t<int> >::value));

    return result.ok();
}


// tests_sort_monoid_run_comparator_construction
bool tests_sort_monoid_run_comparator_construction()
{
    verdict result;

    sorted_run<int, greater<int> > descending_run((greater<int>()));

    //   The comparator-only constructor builds an empty run that remembers a
    // specific ordering.  This matters because combine() propagates the LEFT
    // operand's comparator, so the ordering is carried by the value and not
    // by the type alone.
    result.check(descending_run.empty());
    result.check(descending_run.size() == 0);
    result.check(descending_run.compare()(2, 1));
    result.check(!descending_run.compare()(1, 2));

    //   It is explicit, so a comparator never silently becomes a run.
    result.check((!std::is_convertible<
                      greater<int>,
                      sorted_run<int, greater<int> > >::value));
    result.check((std::is_constructible<
                      sorted_run<int, greater<int> >,
                      greater<int> >::value));

    return result.ok();
}


// tests_sort_monoid_run_adopting_construction
bool tests_sort_monoid_run_adopting_construction()
{
    verdict result;

    static const int ordered[]   = { 1, 3, 3, 7, 9 };
    static const int unordered[] = { 9, 1, 7 };

    int_run adopted(ints_of(ordered, 5), less<int>(), true);
    int_run misused(ints_of(unordered, 3), less<int>(), true);
    int_run adopted_empty((std::vector<int>()), less<int>(), true);

    //   The three-argument constructor adopts a vector that the CALLER
    // guarantees is already ordered.  It performs no sort, which is the
    // whole point -- it is the zero-cost path.
    result.check(adopted.size() == 5);
    result.check(ranges_equal(adopted.data(), ints_of(ordered, 5)));
    result.check(!adopted.empty());

    //   The boolean tag exists only to disambiguate this constructor from a
    // sorting factory; its value is never read.  Passing unsorted data is a
    // precondition violation, and the run keeps it verbatim.  That is
    // asserted rather than avoided, because it is the behaviour a caller who
    // reaches for this constructor by mistake will actually observe -- and
    // because it is the sharpest available statement that no sort happens
    // here.
    result.check(ranges_equal(misused.data(), ints_of(unordered, 3)));
    result.check(misused.data()[0] == 9);

    //   An adopted empty vector is still the identity.
    result.check(adopted_empty.empty());
    result.check(adopted_empty.size() == 0);

    return result.ok();
}


// tests_sort_monoid_run_accessors
bool tests_sort_monoid_run_accessors()
{
    verdict result;

    static const int values[] = { 2, 4, 6, 8 };

    int_run run(ints_of(values, 4), less<int>(), true);

    const int_run& frozen = run;

    //   size() and data().size() are the same number, and empty() agrees
    // with both.
    result.check(run.size() == 4);
    result.check(run.size() == run.data().size());
    result.check(!run.empty());
    result.check(run.empty() == run.data().empty());

    result.check(run.data()[0] == 2);
    result.check(run.data()[3] == 8);

    //   The accessors hand back const references, not copies: a run is
    // value-semantic but its contents are not duplicated on every read.
    result.check((std::is_same<decltype(run.data()),
                               const std::vector<int>&>::value));
    result.check((std::is_same<decltype(run.compare()),
                               const less<int>&>::value));

    //   And they are const member functions, so a const run is fully
    // readable.
    result.check(frozen.size() == 4);
    result.check(!frozen.empty());
    result.check(frozen.data()[1] == 4);
    result.check(frozen.compare()(1, 2));

    return result.ok();
}


// tests_sort_monoid_run_value_semantics
bool tests_sort_monoid_run_value_semantics()
{
    verdict result;

    static const int first_values[]  = { 1, 2, 3 };
    static const int second_values[] = { 7, 8 };

    int_run original(ints_of(first_values, 3), less<int>(), true);
    int_run  copy(original);
    int_run  assigned;
    int_run* self;

    //   sorted_run is a value type -- the header calls it a newtype over a
    // vector.  A copy is independent of its source, which is what makes the
    // algebra safe: combine() takes both operands by const reference and
    // returns a new run, so a caller's runs are never mutated underneath
    // them.
    result.check(copy.size() == 3);
    result.check(ranges_equal(copy.data(), original.data()));

    assigned = original;

    result.check(assigned.size() == 3);
    result.check(ranges_equal(assigned.data(), original.data()));

    //   Reassigning one leaves the other alone.
    assigned = int_run(ints_of(second_values, 2), less<int>(), true);

    result.check(assigned.size() == 2);
    result.check(assigned.data()[0] == 7);
    result.check(original.size() == 3);
    result.check(original.data()[0] == 1);
    result.check(copy.size() == 3);
    result.check(copy.data()[0] == 1);

    //   Self-assignment is safe.  Routed through a pointer so the compiler
    // does not simply diagnose it and elide the case being tested.
    self = &original;

    original = *self;

    result.check(original.size() == 3);
    result.check(ranges_equal(original.data(), ints_of(first_values, 3)));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
