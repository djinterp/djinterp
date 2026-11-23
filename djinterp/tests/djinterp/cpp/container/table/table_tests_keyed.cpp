/*******************************************************************************
* djinterp [cpp][container]                                table_tests_keyed.cpp
*
* keyed_table.hpp: the record table kept in key order -- construction,
* lookup, insertion and erasure as a map and as a multimap, the projection
* forms, and detection.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_keyed.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <cstddef>      // std::size_t
#include <functional>   // std::greater
#include <stdexcept>    // std::out_of_range
#include <string>       // std::string
#include <type_traits>  // std::is_same
#include <vector>       // std::vector
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <djinterp/cpp/container/table/keyed_table.hpp>  // under test


NS_DJINTERP
NS_TESTING

namespace
{

// tb_person
//   type: a record keyed by id (pointer-to-member) or by name (functor).
struct tb_person
{
    int         id;
    std::string name;
};

// tb_by_name
//   type: a projection functor onto tb_person::name.
struct tb_by_name
{
    const std::string& operator()(const tb_person& _p) const
    {
        return _p.name;
    }
};

using tb_by_id = int tb_person::*;

// tb_ids / tb_names
//   function: the keys or names of a keyed table, in its order.
template<typename Table>
std::vector<int> tb_ids(const Table& _t)
{
    std::vector<int> out;

    for (const tb_person& p : _t)
    {
        out.push_back(p.id);
    }

    return out;
}

template<typename Table>
std::vector<std::string> tb_names(const Table& _t)
{
    std::vector<std::string> out;

    for (const tb_person& p : _t)
    {
        out.push_back(p.name);
    }

    return out;
}

}  // namespace


/*
tests_keyed_table_construction
  construction and the declared positions
  Tests the following:
  - an empty table, from a projection; from a record list in any order,
    which comes out in key order; make_keyed_table
  - a repeated key in the list keeps the last record (map assignment)
  - size, empty, operator[], at (std::out_of_range), the iterators
  - the member constants and key_type
*/
bool tests_keyed_table_construction()
{
    using map_type = DKeyedTable<tb_person, tb_by_id>;

    map_type none(&tb_person::id);
    D_TBL_CHECK(none.empty());
    D_TBL_CHECK(none.size() == 0);

    auto t = make_keyed_table<tb_person>(
        { { 3, "c" }, { 1, "a" }, { 2, "b" }, { 1, "A" } }, &tb_person::id);
    D_TBL_CHECK(t.size() == 3);
    D_TBL_CHECK(tb_ids(t) == std::vector<int>({ 1, 2, 3 }));
    D_TBL_CHECK(t[0].name == "A");
    D_TBL_CHECK(t.at(2).name == "c");
    D_TBL_CHECK(tbl_throws<std::out_of_range>([&t] { (void)t.at(3); }));
    D_TBL_CHECK((t.cend() - t.cbegin()) == 3);

    D_TBL_CHECK((std::is_same<map_type::key_type, int>::value));
    D_TBL_CHECK((std::is_same<map_type::record_type, tb_person>::value));
    D_TBL_CHECK(map_type::keyed);
    D_TBL_CHECK(map_type::sorted_invariant);
    D_TBL_CHECK(map_type::unique_keys);
    D_TBL_CHECK(map_type::multiplicity_grade == DContainerMultiplicity::unique);
    D_TBL_CHECK(map_type::npos == static_cast<std::size_t>(-1));

    D_TBL_CHECK(is_keyed_table<map_type>::value);
    D_TBL_CHECK(is_keyed_table_v<const map_type&>);
    D_TBL_CHECK(!is_keyed_table<std::vector<tb_person>>::value);

    return true;
}

/*
tests_keyed_table_lookup
  key_of, the bounds, find, index_of, contains_key, count_key
  Tests the following:
  - key_of applies the projection
  - lower_bound and upper_bound bracket a key, present or not
  - find returns the record or nullptr; index_of its position or npos
  - contains_key and count_key (0 or 1 for a map)
*/
bool tests_keyed_table_lookup()
{
    auto t = make_keyed_table<tb_person>(
        { { 10, "j" }, { 30, "l" }, { 20, "k" } }, &tb_person::id);

    D_TBL_CHECK(t.key_of(tb_person{ 7, "x" }) == 7);

    D_TBL_CHECK(t.lower_bound(20) == 1);
    D_TBL_CHECK(t.upper_bound(20) == 2);
    D_TBL_CHECK(t.lower_bound(25) == 2);
    D_TBL_CHECK(t.upper_bound(25) == 2);
    D_TBL_CHECK(t.lower_bound(5) == 0);
    D_TBL_CHECK(t.upper_bound(99) == 3);

    const tb_person* k = t.find(20);
    D_TBL_CHECK(k != nullptr);
    D_TBL_CHECK(k->name == "k");
    D_TBL_CHECK(t.find(25) == nullptr);
    D_TBL_CHECK(t.index_of(30) == 2);
    D_TBL_CHECK(t.index_of(31) == t.npos);

    D_TBL_CHECK(t.contains_key(10));
    D_TBL_CHECK(!t.contains_key(11));
    D_TBL_CHECK(t.count_key(10) == 1);
    D_TBL_CHECK(t.count_key(11) == 0);

    return true;
}

/*
tests_keyed_table_insert_erase
  insert, erase_at, erase_key, clear (a map)
  Tests the following:
  - insert returns the position the record takes
  - inserting an existing key replaces that record in place
  - erase_at removes by position; erase_key by key, returning the count
  - clear empties the table
*/
bool tests_keyed_table_insert_erase()
{
    DKeyedTable<tb_person, tb_by_id> t(&tb_person::id);

    D_TBL_CHECK(t.insert({ 5, "e" }) == 0);
    D_TBL_CHECK(t.insert({ 1, "a" }) == 0);
    D_TBL_CHECK(t.insert({ 9, "i" }) == 2);
    D_TBL_CHECK(t.insert({ 5, "E" }) == 1);
    D_TBL_CHECK(t.size() == 3);
    D_TBL_CHECK(tb_names(t) == std::vector<std::string>({ "a", "E", "i" }));

    t.erase_at(0);
    D_TBL_CHECK(tb_ids(t) == std::vector<int>({ 5, 9 }));
    D_TBL_CHECK(t.erase_key(9) == 1);
    D_TBL_CHECK(t.erase_key(9) == 0);
    D_TBL_CHECK(tb_ids(t) == std::vector<int>({ 5 }));

    t.clear();
    D_TBL_CHECK(t.empty());

    return true;
}

/*
tests_keyed_table_multi
  the multimap: repeated keys
  Tests the following:
  - make_multikeyed_table keeps every record; equal keys keep their
    insertion order, and a new equal key goes after them
  - count_key counts the run; the bounds bracket it
  - erase_key removes the whole run and returns its length
*/
bool tests_keyed_table_multi()
{
    auto t = make_multikeyed_table<tb_person>(
        { { 2, "b1" }, { 1, "a" }, { 2, "b2" } }, &tb_person::id);

    D_TBL_CHECK(!decltype(t)::unique_keys);
    D_TBL_CHECK(decltype(t)::multiplicity_grade
                == DContainerMultiplicity::multi);
    D_TBL_CHECK(tb_names(t) == std::vector<std::string>({ "a", "b1", "b2" }));

    D_TBL_CHECK(t.insert({ 2, "b3" }) == 3);
    D_TBL_CHECK(t.count_key(2) == 3);
    D_TBL_CHECK(t.lower_bound(2) == 1);
    D_TBL_CHECK(t.upper_bound(2) == 4);

    D_TBL_CHECK(t.erase_key(2) == 3);
    D_TBL_CHECK(tb_names(t) == std::vector<std::string>({ "a" }));

    return true;
}

/*
tests_keyed_table_projection
  projection by functor, and a custom comparator
  Tests the following:
  - a functor projection keys on its result type (std::string here)
  - std::greater<> keeps the table in descending key order
*/
bool tests_keyed_table_projection()
{
    DKeyedTable<tb_person, tb_by_name> by_name{
        { 1, "kiwi" }, { 2, "apple" }, { 3, "fig" } };
    D_TBL_CHECK((std::is_same<decltype(by_name)::key_type,
                std::string>::value));
    D_TBL_CHECK(tb_ids(by_name) == std::vector<int>({ 2, 3, 1 }));
    D_TBL_CHECK(by_name.find("fig")->id == 3);
    D_TBL_CHECK(by_name.find(std::string("pear")) == nullptr);

    DKeyedTable<tb_person, tb_by_id, std::greater<>> down(
        { { 1, "a" }, { 3, "c" }, { 2, "b" } }, &tb_person::id);
    D_TBL_CHECK(tb_ids(down) == std::vector<int>({ 3, 2, 1 }));
    D_TBL_CHECK(down.lower_bound(2) == 1);
    D_TBL_CHECK(down.index_of(1) == 2);

    return true;
}

NS_END  // testing
NS_END  // djinterp
