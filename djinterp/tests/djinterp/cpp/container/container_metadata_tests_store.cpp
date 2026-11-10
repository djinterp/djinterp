/*******************************************************************************
* djinterp [testing]                          container_metadata_tests_store.cpp
*
*   The store section of container_metadata's suite: DContainerMetadata as
* an ordered collection of named entries - construction, set, lookup, erase
* and clear, iteration, and a backing store other than the default.
*
*
* path:      /tests/djinterp/cpp/container/container_metadata_tests_store.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/
#include "container_metadata_tests.hpp"  // corresponding header
// std
#include <stdexcept>


NS_DJINTERP
NS_TESTING

/*
tests_container_metadata_construction
  Construction and the member types.
  Tests the following:
  - default construction is empty
  - an initializer list is kept verbatim, a duplicate key included, and
    find returns the first entry for that key
  - key_type, mapped_type, store_type and value_type name what they say
*/
bool tests_container_metadata_construction()
{
    const DCmStrings none;

    D_CM_CHECK(none.empty());
    D_CM_CHECK(none.size() == 0);
    D_CM_CHECK(none.begin() == none.end());

    const DCmStrings listed = { { "title", "Q3" },
                                { "owner", "ops" },
                                { "title", "Q4" } };

    D_CM_CHECK(listed.size() == 3);
    D_CM_CHECK(listed.find("title") != nullptr);
    D_CM_CHECK(*listed.find("title") == "Q3");

    D_CM_CHECK(std::is_same<DCmStrings::key_type, std::string>::value);
    D_CM_CHECK(std::is_same<DCmStrings::mapped_type, std::string>::value);
    D_CM_CHECK(std::is_same<DCmStrings::value_type,
                            std::pair<std::string, std::string>>::value);
    D_CM_CHECK(std::is_same<DCmStrings::store_type,
                            std::vector<std::pair<std::string,
                                                  std::string>>>::value);
    D_CM_CHECK(std::is_same<DCmNumbers::key_type, int>::value);
    D_CM_CHECK(std::is_same<DCmNumbers::mapped_type, double>::value);

    return true;
}

/*
tests_container_metadata_set
  set as the one way to bind a key.
  Tests the following:
  - a new key is appended, in first-seen order
  - an existing key is overwritten in place: the size and the order stay
  - a value set from an rvalue arrives intact
*/
bool tests_container_metadata_set()
{
    DCmStrings meta;
    std::string long_value(64, 'x');

    meta.set("title", "Q3");
    meta.set("owner", "ops");
    meta.set("notes", static_cast<std::string&&>(long_value));

    D_CM_CHECK(meta.size() == 3);
    D_CM_CHECK(meta.entries()[0].first == "title");
    D_CM_CHECK(meta.entries()[1].first == "owner");
    D_CM_CHECK(meta.entries()[2].first == "notes");
    D_CM_CHECK(meta.at("notes") == std::string(64, 'x'));

    meta.set("owner", "finance");

    D_CM_CHECK(meta.size() == 3);
    D_CM_CHECK(meta.entries()[1].first == "owner");
    D_CM_CHECK(meta.entries()[1].second == "finance");
    D_CM_CHECK(meta.at("title") == "Q3");

    return true;
}

/*
tests_container_metadata_lookup
  find, at and contains.
  Tests the following:
  - find returns a pointer to the bound value, or nullptr for an absent key,
    from a const and from a non-const store
  - a write through the non-const find lands in the store
  - at returns the bound value, and throws std::out_of_range for an absent
    key, leaving the store as it was
  - contains agrees with find
*/
bool tests_container_metadata_lookup()
{
    DCmStrings meta = { { "title", "Q3" } };
    const DCmStrings& view = meta;
    bool threw;

    D_CM_CHECK(view.find("title") != nullptr);
    D_CM_CHECK(*view.find("title") == "Q3");
    D_CM_CHECK(view.find("owner") == nullptr);
    D_CM_CHECK(meta.find("owner") == nullptr);

    *meta.find("title") = "Q4";
    D_CM_CHECK(view.at("title") == "Q4");

    threw = false;

    try
    {
        (void)view.at("owner");
    }
    catch (const std::out_of_range&)
    {
        threw = true;
    }

    D_CM_CHECK(threw);
    D_CM_CHECK(meta.size() == 1);

    D_CM_CHECK(view.contains("title"));
    D_CM_CHECK(!view.contains("owner"));

    return true;
}

/*
tests_container_metadata_erase_clear
  Removing entries.
  Tests the following:
  - erase of a bound key returns true and removes that key only, the other
    entries keeping their order
  - erase of an absent key returns false and changes nothing
  - clear empties the store, and a cleared store takes new entries
*/
bool tests_container_metadata_erase_clear()
{
    DCmStrings meta = { { "a", "1" }, { "b", "2" }, { "c", "3" } };

    D_CM_CHECK(meta.erase("b"));
    D_CM_CHECK(meta.size() == 2);
    D_CM_CHECK(!meta.contains("b"));
    D_CM_CHECK(meta.entries()[0].first == "a");
    D_CM_CHECK(meta.entries()[1].first == "c");

    D_CM_CHECK(!meta.erase("b"));
    D_CM_CHECK(!meta.erase("z"));
    D_CM_CHECK(meta.size() == 2);

    meta.clear();

    D_CM_CHECK(meta.empty());
    D_CM_CHECK(!meta.contains("a"));

    meta.set("d", "4");
    D_CM_CHECK(meta.size() == 1);
    D_CM_CHECK(meta.at("d") == "4");

    return true;
}

/*
tests_container_metadata_iteration
  The store as a range.
  Tests the following:
  - begin/end, cbegin/cend and a range-for visit the entries in insertion
    order
  - a write through a non-const iterator lands in the store
  - entries() is the backing store, in the same order
*/
bool tests_container_metadata_iteration()
{
    DCmStrings meta = { { "a", "1" }, { "b", "2" }, { "c", "3" } };
    const DCmStrings& view = meta;
    std::string keys;
    std::size_t n;

    for (const DCmStrings::value_type& entry : view)
    {
        keys += entry.first;
    }

    D_CM_CHECK(keys == "abc");

    n = 0;

    for (DCmStrings::const_iterator it = view.cbegin();
         it != view.cend();
         ++it)
    {
        ++n;
    }

    D_CM_CHECK(n == 3);

    meta.begin()->second = "one";
    D_CM_CHECK(meta.at("a") == "one");

    D_CM_CHECK(view.entries().size() == view.size());
    D_CM_CHECK(view.entries().front().first == "a");

    return true;
}

/*
tests_container_metadata_custom_store
  The open backing store and entry types.
  Tests the following:
  - a std::list of pairs serves as the store: set, find, erase and the
    order all behave as with the default
  - int keys and double values work as well as strings
*/
bool tests_container_metadata_custom_store()
{
    DCmListed listed;
    DCmNumbers numbers = { { 1, 0.5 } };

    listed.set("rows", 3);
    listed.set("cols", 4);
    listed.set("rows", 5);

    D_CM_CHECK(listed.size() == 2);
    D_CM_CHECK(listed.at("rows") == 5);
    D_CM_CHECK(listed.begin()->first == "rows");
    D_CM_CHECK(listed.erase("rows"));
    D_CM_CHECK(listed.begin()->first == "cols");
    D_CM_CHECK(std::is_same<DCmListed::store_type,
                            std::list<std::pair<std::string,
                                                int>>>::value);

    numbers.set(2, 1.5);

    D_CM_CHECK(numbers.size() == 2);
    D_CM_CHECK(numbers.at(1) == 0.5);
    D_CM_CHECK(numbers.at(2) == 1.5);
    D_CM_CHECK(numbers.find(3) == nullptr);

    return true;
}

NS_END  // testing
NS_END  // djinterp
