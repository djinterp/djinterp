/*******************************************************************************
* djinterp [testing]             container_transform_traits_tests_capability.cpp
*
*   Section I - the capability split.  is_transform_source is the READ capability
* (iterable + a value_type); is_container_transformable adds the BUILD capability
* (push_back or associative insert); is_transform_input_only is a source that
* cannot build (transforming it needs an external target); has_native_transform
* detects a container's own transform(map) primitive.
*
*   The edges the standard library supplies: std::forward_list and std::array are
* SOURCES but not transformable - forward_list has neither push_back nor a
* single-argument insert (only push_front / insert_after), and array is fixed - so
* both are input-only.  A non-container (int) is not even a source, so it is NOT
* input-only either (that predicate requires a source).
*
*
* path:      /tests/djinterp/core/container/traits/container_transform_traits_tests_capability.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_transform_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_is_transform_source
//   iterable AND typed - the read capability.
bool
tests_is_transform_source()
{
    bool _tx_ok = true;

    // every standard container is a source (incl. the non-buildable ones)
    D_TX_CHECK( (is_transform_source<std::vector<int>>::value) );
    D_TX_CHECK( (is_transform_source<std::set<int>>::value) );
    D_TX_CHECK( (is_transform_source<std::unordered_set<int>>::value) );
    D_TX_CHECK( (is_transform_source<std::forward_list<int>>::value) );
    D_TX_CHECK( (is_transform_source<std::array<int, 4>>::value) );
    // custom sources
    D_TX_CHECK( (is_transform_source<native_box>::value) );
    D_TX_CHECK( (is_transform_source<read_only_seq>::value) );
    D_TX_CHECK( (is_transform_source<Box<int>>::value) );

    // a non-container is not a source
    D_TX_CHECK( (!is_transform_source<int>::value) );

    return _tx_ok;
}


// tests_is_container_transformable
//   a source that can also gather the images (read + build).
bool
tests_is_container_transformable()
{
    bool _tx_ok = true;

    // read + build
    D_TX_CHECK( (is_container_transformable<std::vector<int>>::value) );
    D_TX_CHECK( (is_container_transformable<std::deque<int>>::value) );
    D_TX_CHECK( (is_container_transformable<std::list<int>>::value) );
    D_TX_CHECK( (is_container_transformable<std::set<int>>::value) );       // insert(value)
    D_TX_CHECK( (is_container_transformable<std::map<int, int>>::value) );  // insert(value)
    D_TX_CHECK( (is_container_transformable<std::unordered_set<int>>::value) );
    D_TX_CHECK( (is_container_transformable<std::string>::value) );
    D_TX_CHECK( (is_container_transformable<native_box>::value) );
    D_TX_CHECK( (is_container_transformable<Box<int>>::value) );

    // sources that cannot build
    D_TX_CHECK( (!is_container_transformable<std::forward_list<int>>::value) );  // no push_back / insert(value)
    D_TX_CHECK( (!is_container_transformable<std::array<int, 4>>::value) );      // fixed
    D_TX_CHECK( (!is_container_transformable<read_only_seq>::value) );
    // not even a source
    D_TX_CHECK( (!is_container_transformable<int>::value) );

    return _tx_ok;
}


// tests_is_transform_input_only
//   a source WITHOUT the build capability.
bool
tests_is_transform_input_only()
{
    bool _tx_ok = true;

    // sources that cannot build
    D_TX_CHECK( (is_transform_input_only<std::forward_list<int>>::value) );
    D_TX_CHECK( (is_transform_input_only<std::array<int, 4>>::value) );
    D_TX_CHECK( (is_transform_input_only<read_only_seq>::value) );

    // transformable containers are NOT input-only
    D_TX_CHECK( (!is_transform_input_only<std::vector<int>>::value) );
    D_TX_CHECK( (!is_transform_input_only<std::set<int>>::value) );
    D_TX_CHECK( (!is_transform_input_only<Box<int>>::value) );
    // a non-source is not input-only (the predicate requires a source)
    D_TX_CHECK( (!is_transform_input_only<int>::value) );

    return _tx_ok;
}


// tests_has_native_transform
//   detects a transform(map) member of the container's own.
bool
tests_has_native_transform()
{
    bool _tx_ok = true;

    D_TX_CHECK( (has_native_transform<native_box>::value) );

    // no standard container exposes a transform() member
    D_TX_CHECK( (!has_native_transform<std::vector<int>>::value) );
    D_TX_CHECK( (!has_native_transform<std::set<int>>::value) );
    D_TX_CHECK( (!has_native_transform<Box<int>>::value) );
    D_TX_CHECK( (!has_native_transform<read_only_seq>::value) );
    D_TX_CHECK( (!has_native_transform<int>::value) );

    return _tx_ok;
}


NS_END  // testing
NS_END  // djinterp
