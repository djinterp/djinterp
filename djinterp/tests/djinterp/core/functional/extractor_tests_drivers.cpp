#include "extractor_tests.hpp"

// std
#include <map>
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_drivers_extract_all
  Verifies extract_all applies an extractor to every element and collects the
  results in container order.
  Tests the following:
  - extract_all(get_age, people) yields the ages in order
*/
bool
test_drivers_extract_all(
)
{
    std::vector<person> people = make_people();
    std::vector<int> ages = extract_all(get_age(), people);

    D_INTERNAL_EXT_CHECK(ages.size() == 4);
    D_INTERNAL_EXT_CHECK(ages[0] == 30);
    D_INTERNAL_EXT_CHECK(ages[1] == 15);
    D_INTERNAL_EXT_CHECK(ages[2] == 30);
    D_INTERNAL_EXT_CHECK(ages[3] == 40);

    return true;
}


/*
test_drivers_extract_all_empty
  Verifies extract_all on an empty container yields an empty vector (the
  result type is still well-formed via the unevaluated decltype).
  Tests the following:
  - extract_all(get_age, {}) yields an empty vector
*/
bool
test_drivers_extract_all_empty(
)
{
    std::vector<person> empty;
    std::vector<int> ages = extract_all(get_age(), empty);

    D_INTERNAL_EXT_CHECK(ages.empty());

    return true;
}


/*
test_drivers_extract_first
  Verifies extract_first returns just(extract(front)) for a non-empty
  container.
  Tests the following:
  - extract_first(get_age, people) yields a present maybe holding the first
    age
*/
bool
test_drivers_extract_first(
)
{
    std::vector<person> people = make_people();
    maybe<int> m = extract_first(get_age(), people);

    D_INTERNAL_EXT_CHECK(m.has_value());
    D_INTERNAL_EXT_CHECK(m.value() == 30);

    return true;
}


/*
test_drivers_extract_first_empty
  Verifies extract_first yields nothing for an empty container.
  Tests the following:
  - extract_first(get_age, {}) yields an empty maybe
*/
bool
test_drivers_extract_first_empty(
)
{
    std::vector<person> empty;
    maybe<int> m = extract_first(get_age(), empty);

    D_INTERNAL_EXT_CHECK(m.is_nothing());

    return true;
}


/*
test_drivers_extract_unique
  Verifies extract_unique returns distinct extracted values in first-seen
  order.
  Tests the following:
  - ages {30, 15, 30, 40} dedupe to {30, 15, 40} (order preserved, the
    second 30 dropped)
  - depts {10, 20, 10, 20} dedupe to {10, 20}
*/
bool
test_drivers_extract_unique(
)
{
    std::vector<person> people = make_people();

    std::vector<int> ages = extract_unique(get_age(), people);
    D_INTERNAL_EXT_CHECK(ages.size() == 3);
    D_INTERNAL_EXT_CHECK(ages[0] == 30);
    D_INTERNAL_EXT_CHECK(ages[1] == 15);
    D_INTERNAL_EXT_CHECK(ages[2] == 40);

    std::vector<int> depts = extract_unique(get_dept(), people);
    D_INTERNAL_EXT_CHECK(depts.size() == 2);
    D_INTERNAL_EXT_CHECK(depts[0] == 10);
    D_INTERNAL_EXT_CHECK(depts[1] == 20);

    return true;
}


/*
test_drivers_extract_into_map
  Verifies extract_into_map builds a map from a key extractor and a value
  extractor.
  Tests the following:
  - keying by unique id maps each id to its age, with all four entries present
*/
bool
test_drivers_extract_into_map(
)
{
    std::vector<person> people = make_people();
    std::map<int, int> by_id = extract_into_map(get_id(), get_age(), people);

    D_INTERNAL_EXT_CHECK(by_id.size() == 4);
    D_INTERNAL_EXT_CHECK(by_id[1] == 30);
    D_INTERNAL_EXT_CHECK(by_id[2] == 15);
    D_INTERNAL_EXT_CHECK(by_id[3] == 30);
    D_INTERNAL_EXT_CHECK(by_id[4] == 40);

    return true;
}


/*
test_drivers_extract_into_map_overwrite
  Verifies later duplicates overwrite earlier entries (last-write-wins) when
  the key is not unique.
  Tests the following:
  - keying by dept collapses to two entries; each holds the age of the LAST
    person seen in that dept (dept 10 -> id 3 age 30; dept 20 -> id 4 age 40)
*/
bool
test_drivers_extract_into_map_overwrite(
)
{
    std::vector<person> people = make_people();
    std::map<int, int> by_dept =
        extract_into_map(get_dept(), get_age(), people);

    D_INTERNAL_EXT_CHECK(by_dept.size() == 2);
    // dept 10: persons id1(age30) then id3(age30) -> last age 30
    D_INTERNAL_EXT_CHECK(by_dept[10] == 30);
    // dept 20: persons id2(age15) then id4(age40) -> last age 40
    D_INTERNAL_EXT_CHECK(by_dept[20] == 40);

    return true;
}


/*
test_drivers_group_by_extractor
  Verifies group_by_extractor buckets whole sources by an extracted key,
  preserving insertion order within each bucket.
  Tests the following:
  - grouping by dept yields two buckets
  - bucket 10 holds persons id 1 then 3; bucket 20 holds id 2 then 4
*/
bool
test_drivers_group_by_extractor(
)
{
    std::vector<person> people = make_people();
    std::map<int, std::vector<person> > groups =
        group_by_extractor(get_dept(), people);

    D_INTERNAL_EXT_CHECK(groups.size() == 2);

    const std::vector<person>& g10 = groups[10];
    D_INTERNAL_EXT_CHECK(g10.size() == 2);
    D_INTERNAL_EXT_CHECK(g10[0].id == 1);
    D_INTERNAL_EXT_CHECK(g10[1].id == 3);

    const std::vector<person>& g20 = groups[20];
    D_INTERNAL_EXT_CHECK(g20.size() == 2);
    D_INTERNAL_EXT_CHECK(g20[0].id == 2);
    D_INTERNAL_EXT_CHECK(g20[1].id == 4);

    return true;
}


/*
run_drivers_tests
  Aggregates every container-driver test.
  Tests the following:
  - all extract_all / extract_first / extract_unique / extract_into_map /
    group_by_extractor tests pass, including the empty-container edges and
    the last-write-wins map behaviour
*/
bool
run_drivers_tests(
)
{
    return ( test_drivers_extract_all()             &&
             test_drivers_extract_all_empty()        &&
             test_drivers_extract_first()            &&
             test_drivers_extract_first_empty()      &&
             test_drivers_extract_unique()           &&
             test_drivers_extract_into_map()         &&
             test_drivers_extract_into_map_overwrite() &&
             test_drivers_group_by_extractor() );
}


NS_END  // testing
NS_END  // djinterp
