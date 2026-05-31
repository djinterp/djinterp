#include "accumulator_tests.hpp"

// std
#include <map>
#include <string>
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_collectors_joining
  Concatenates stringified elements with a separator.
  Tests the following:
  - the separator appears only between elements
  - a single element produces no separator
  - an empty input produces the empty string
*/
bool
test_collectors_joining(
)
{
    std::vector<int> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);

    D_INTERNAL_ACC_CHECK(joining<int>(", ").run(values) == "1, 2, 3");

    std::vector<int> single;
    single.push_back(9);

    D_INTERNAL_ACC_CHECK(joining<int>(", ").run(single) == "9");

    std::vector<int> empty_vec;

    D_INTERNAL_ACC_CHECK(joining<int>(", ").run(empty_vec) == "");

    return true;
}


/*
test_collectors_to_vector
  Materialises the stream into a vector.
  Tests the following:
  - element order is preserved
  - duplicates are retained
  - an empty input yields an empty vector
*/
bool
test_collectors_to_vector(
)
{
    std::vector<int> values;
    values.push_back(1);
    values.push_back(1);
    values.push_back(2);

    std::vector<int> result = to_vector<int>().run(values);

    D_INTERNAL_ACC_CHECK(result.size() == 3);
    D_INTERNAL_ACC_CHECK(result[0] == 1);
    D_INTERNAL_ACC_CHECK(result[1] == 1);
    D_INTERNAL_ACC_CHECK(result[2] == 2);

    std::vector<int> empty_vec;

    D_INTERNAL_ACC_CHECK(to_vector<int>().run(empty_vec).empty());

    return true;
}


/*
test_collectors_histogram
  Counts occurrences per distinct key.
  Tests the following:
  - repeated keys accumulate counts
  - the resulting map size equals the distinct-key count
*/
bool
test_collectors_histogram(
)
{
    std::vector<int> values;
    values.push_back(1);
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);
    values.push_back(3);
    values.push_back(3);

    std::map<int, std::size_t> hist = histogram<int>().run(values);

    D_INTERNAL_ACC_CHECK(hist.size() == 3);
    D_INTERNAL_ACC_CHECK(hist[1] == 2);
    D_INTERNAL_ACC_CHECK(hist[2] == 1);
    D_INTERNAL_ACC_CHECK(hist[3] == 3);

    return true;
}


/*
test_collectors_to_map_by
  Indexes elements by a projected key, last-write-wins.
  Tests the following:
  - each element is stored under its key
  - a duplicate key is overwritten by the later element
*/
bool
test_collectors_to_map_by(
)
{
    std::vector<point> values;
    values.push_back(point{ 1, 10 });
    values.push_back(point{ 2, 20 });
    values.push_back(point{ 1, 99 });

    std::map<int, point> indexed = to_map_by<point>(by_x()).run(values);

    D_INTERNAL_ACC_CHECK(indexed.size() == 2);
    D_INTERNAL_ACC_CHECK(indexed[1].y == 99);
    D_INTERNAL_ACC_CHECK(indexed[2].y == 20);

    return true;
}


/*
test_collectors_group_by
  Buckets elements by a projected key, preserving arrival order.
  Tests the following:
  - elements sharing a key land in one bucket
  - within-bucket order matches arrival order
*/
bool
test_collectors_group_by(
)
{
    std::vector<point> values;
    values.push_back(point{ 1, 10 });
    values.push_back(point{ 2, 20 });
    values.push_back(point{ 1, 99 });

    std::map<int, std::vector<point> > grouped =
        group_by<point>(by_x()).run(values);

    D_INTERNAL_ACC_CHECK(grouped.size() == 2);
    D_INTERNAL_ACC_CHECK(grouped[1].size() == 2);
    D_INTERNAL_ACC_CHECK(grouped[1][0].y == 10);
    D_INTERNAL_ACC_CHECK(grouped[1][1].y == 99);
    D_INTERNAL_ACC_CHECK(grouped[2].size() == 1);
    D_INTERNAL_ACC_CHECK(grouped[2][0].y == 20);

    return true;
}


/*
test_collectors_top_k
  Keeps the k largest elements in descending order.
  Tests the following:
  - the k largest values are retained and sorted high-to-low
  - a smaller-than-k stream is fully retained, still sorted
  - k == 1 reduces to the maximum
*/
bool
test_collectors_top_k(
)
{
    std::vector<int> values;
    values.push_back(3);
    values.push_back(1);
    values.push_back(4);
    values.push_back(1);
    values.push_back(5);
    values.push_back(9);
    values.push_back(2);
    values.push_back(6);

    std::vector<int> top3 = top_k<int>(3).run(values);

    D_INTERNAL_ACC_CHECK(top3.size() == 3);
    D_INTERNAL_ACC_CHECK(top3[0] == 9);
    D_INTERNAL_ACC_CHECK(top3[1] == 6);
    D_INTERNAL_ACC_CHECK(top3[2] == 5);

    std::vector<int> few;
    few.push_back(3);
    few.push_back(1);
    few.push_back(2);

    std::vector<int> top_all = top_k<int>(10).run(few);

    D_INTERNAL_ACC_CHECK(top_all.size() == 3);
    D_INTERNAL_ACC_CHECK(top_all[0] == 3);
    D_INTERNAL_ACC_CHECK(top_all[1] == 2);
    D_INTERNAL_ACC_CHECK(top_all[2] == 1);

    std::vector<int> top1 = top_k<int>(1).run(values);

    D_INTERNAL_ACC_CHECK(top1.size() == 1);
    D_INTERNAL_ACC_CHECK(top1[0] == 9);

    return true;
}


/*
run_collectors_tests
  Aggregates every collectors-section test.
  Tests the following:
  - all joining/to_vector/histogram/to_map_by/group_by/top_k tests pass
*/
bool
run_collectors_tests(
)
{
    return ( test_collectors_joining()    &&
             test_collectors_to_vector()  &&
             test_collectors_histogram()  &&
             test_collectors_to_map_by()  &&
             test_collectors_group_by()   &&
             test_collectors_top_k() );
}


NS_END  // testing
NS_END  // djinterp
