/*******************************************************************************
* djinterp [testing]                 container_traits_tests_method_detection.cpp
*
*   Section I - the structural member/alias detectors: size & capacity
* accessors, comparator/hasher and interval type aliases, the hierarchical
* protocol (node/depth types + parent/children/root/depth accessors), the
* mutation surface (push_ / insert / erase / clear), the iteration surface
* (begin/end/cbegin/cend/rbegin/rend), and the invariant probes (unique /
* value_comp / key_comp / multiplicity accessors).
*
*   Library facts these pin down: std::forward_list has no size() (nor a hinted
* insert, nor erase(iterator)); std::array has no clear/insert/erase; only
* std::vector and std::string expose data() among these plus capacity();
* std::list and std::forward_list expose unique(); the ordered associatives
* expose key_compare/value_compare and key_comp()/value_comp() while the
* unordered ones expose hasher; and reverse iteration is absent on forward_list
* and unordered_set.
*
*
* path:      /tests/djinterp/core/container/traits/container_traits_tests_method_detection.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_size_capacity_detection
//   data() / size() / max_size() / min_size() / capacity() detection.
bool
tests_size_capacity_detection()
{
    bool _ct_ok = true;

    // data(): contiguous containers only
    D_CT_CHECK( (has_data_method<std::vector<int>>::value) );
    D_CT_CHECK( (has_data_method<std::string>::value) );
    D_CT_CHECK( (has_data_method<std::array<int, 4>>::value) );
    D_CT_CHECK( (!has_data_method<std::deque<int>>::value) );
    D_CT_CHECK( (!has_data_method<std::list<int>>::value) );
    D_CT_CHECK( (!has_data_method<std::set<int>>::value) );

    // size(): every std container EXCEPT forward_list (and a C-array)
    D_CT_CHECK( (has_size_accessor<std::vector<int>>::value) );
    D_CT_CHECK( (has_size_accessor<std::map<int, int>>::value) );
    D_CT_CHECK( (has_size_accessor<std::array<int, 4>>::value) );
    D_CT_CHECK( (!has_size_accessor<std::forward_list<int>>::value) );  // no size()
    D_CT_CHECK( (!has_size_accessor<int[4]>::value) );
    D_CT_CHECK( (!has_size_accessor<int>::value) );

    // max_size(): every std container (incl. forward_list)
    D_CT_CHECK( (has_max_size_accessor<std::vector<int>>::value) );
    D_CT_CHECK( (has_max_size_accessor<std::forward_list<int>>::value) );
    D_CT_CHECK( (has_max_size_accessor<fixed4>::value) );
    D_CT_CHECK( (!has_max_size_accessor<empty_ct>::value) );   // has size() only
    D_CT_CHECK( (!has_max_size_accessor<int>::value) );

    // min_size(): no std container; the custom min5 has it
    D_CT_CHECK( (has_min_size_accessor<min5>::value) );
    D_CT_CHECK( (!has_min_size_accessor<std::vector<int>>::value) );
    D_CT_CHECK( (!has_min_size_accessor<std::array<int, 4>>::value) );

    // capacity(): vector and string only
    D_CT_CHECK( (has_capacity_accessor<std::vector<int>>::value) );
    D_CT_CHECK( (has_capacity_accessor<std::string>::value) );
    D_CT_CHECK( (!has_capacity_accessor<std::deque<int>>::value) );
    D_CT_CHECK( (!has_capacity_accessor<std::array<int, 4>>::value) );

    return _ct_ok;
}


// tests_type_member_detection
//   comparator / hasher / interval / underlying / depth type aliases.
bool
tests_type_member_detection()
{
    bool _ct_ok = true;

    // key_compare: ordered associatives only
    D_CT_CHECK( (has_key_compare<std::set<int>>::value) );
    D_CT_CHECK( (has_key_compare<std::map<int, int>>::value) );
    D_CT_CHECK( (!has_key_compare<std::unordered_set<int>>::value) );
    D_CT_CHECK( (!has_key_compare<std::vector<int>>::value) );

    // value_compare: ordered associatives (libstdc++ set defines it too)
    D_CT_CHECK( (has_value_compare_type<std::set<int>>::value) );
    D_CT_CHECK( (has_value_compare_type<std::map<int, int>>::value) );
    D_CT_CHECK( (!has_value_compare_type<std::vector<int>>::value) );

    // hasher: unordered associatives only
    D_CT_CHECK( (has_hasher_type<std::unordered_set<int>>::value) );
    D_CT_CHECK( (!has_hasher_type<std::set<int>>::value) );
    D_CT_CHECK( (!has_hasher_type<std::vector<int>>::value) );

    // underlying_container_type: the adaptor fixture
    D_CT_CHECK( (has_underlying_container_type<adaptor>::value) );
    D_CT_CHECK( (!has_underlying_container_type<std::vector<int>>::value) );

    // depth_type: the hierarchical fixture
    D_CT_CHECK( (has_depth_type<hier>::value) );
    D_CT_CHECK( (!has_depth_type<std::vector<int>>::value) );

    // interval type aliases: the intervals_bogus fixture
    D_CT_CHECK( (has_size_interval_type<intervals_bogus>::value) );
    D_CT_CHECK( (has_depth_interval_type<intervals_bogus>::value) );
    D_CT_CHECK( (has_multiplicity_interval_type<intervals_bogus>::value) );
    D_CT_CHECK( (!has_size_interval_type<std::vector<int>>::value) );
    D_CT_CHECK( (!has_multiplicity_interval_type<std::set<int>>::value) );

    return _ct_ok;
}


// tests_hierarchical_detection
//   parent() / children() / root() / depth() / max_depth() / min_depth().
bool
tests_hierarchical_detection()
{
    bool _ct_ok = true;

    D_CT_CHECK( (has_parent_accessor<hier>::value) );
    D_CT_CHECK( (has_children_accessor<hier>::value) );
    D_CT_CHECK( (has_root_accessor<hier>::value) );
    D_CT_CHECK( (has_depth_accessor<hier>::value) );
    D_CT_CHECK( (has_max_depth_accessor<hier>::value) );
    D_CT_CHECK( (has_min_depth_accessor<hier>::value) );

    // flat containers expose none of these
    D_CT_CHECK( (!has_parent_accessor<std::vector<int>>::value) );
    D_CT_CHECK( (!has_children_accessor<std::vector<int>>::value) );
    D_CT_CHECK( (!has_root_accessor<std::map<int, int>>::value) );
    D_CT_CHECK( (!has_depth_accessor<std::set<int>>::value) );

    return _ct_ok;
}


// tests_mutation_detection
//   push_ / insert(hint,value) / erase(iterator) / clear.
bool
tests_mutation_detection()
{
    bool _ct_ok = true;

    // push_ : the custom pusher (no std container has this exact member)
    D_CT_CHECK( (has_push_<pusher>::value) );
    D_CT_CHECK( (!has_push_<std::vector<int>>::value) );

    // hinted insert(const_iterator, value_type)
    D_CT_CHECK( (has_insert<std::vector<int>>::value) );
    D_CT_CHECK( (has_insert<std::set<int>>::value) );
    D_CT_CHECK( (has_insert<std::string>::value) );
    D_CT_CHECK( (!has_insert<std::forward_list<int>>::value) );  // insert_after only
    D_CT_CHECK( (!has_insert<std::array<int, 4>>::value) );

    // erase(const_iterator)
    D_CT_CHECK( (has_erase<std::vector<int>>::value) );
    D_CT_CHECK( (has_erase<std::map<int, int>>::value) );
    D_CT_CHECK( (!has_erase<std::forward_list<int>>::value) );   // erase_after only
    D_CT_CHECK( (!has_erase<std::array<int, 4>>::value) );

    // clear() : every std container except array
    D_CT_CHECK( (has_clear<std::vector<int>>::value) );
    D_CT_CHECK( (has_clear<std::forward_list<int>>::value) );
    D_CT_CHECK( (has_clear<std::set<int>>::value) );
    D_CT_CHECK( (!has_clear<std::array<int, 4>>::value) );

    return _ct_ok;
}


// tests_iteration_detection
//   begin/end/cbegin/cend (forward) + rbegin/rend (reverse).
bool
tests_iteration_detection()
{
    bool _ct_ok = true;

    // begin()/end(): member accessors (a C-array has none)
    D_CT_CHECK( (has_begin_accessor<std::vector<int>>::value) );
    D_CT_CHECK( (has_end_accessor<std::vector<int>>::value) );
    D_CT_CHECK( (has_begin_accessor<std::forward_list<int>>::value) );
    D_CT_CHECK( (!has_begin_accessor<int[4]>::value) );
    D_CT_CHECK( (!has_begin_accessor<int>::value) );

    // cbegin()/cend()
    D_CT_CHECK( (has_cbegin_accessor<std::vector<int>>::value) );
    D_CT_CHECK( (has_cend_accessor<std::map<int, int>>::value) );
    D_CT_CHECK( (!has_cbegin_accessor<int[4]>::value) );

    // rbegin()/rend(): absent on forward_list and unordered_set
    D_CT_CHECK( (has_rbegin_accessor<std::vector<int>>::value) );
    D_CT_CHECK( (has_rend_accessor<std::set<int>>::value) );
    D_CT_CHECK( (has_rbegin_accessor<std::array<int, 4>>::value) );
    D_CT_CHECK( (!has_rbegin_accessor<std::forward_list<int>>::value) );
    D_CT_CHECK( (!has_rbegin_accessor<std::unordered_set<int>>::value) );

    return _ct_ok;
}


// tests_invariant_detection
//   unique() / value_comp() / key_comp() / multiplicity_min() / multiplicity_max().
bool
tests_invariant_detection()
{
    bool _ct_ok = true;

    // unique(): the list families
    D_CT_CHECK( (has_unique_accessor<std::list<int>>::value) );
    D_CT_CHECK( (has_unique_accessor<std::forward_list<int>>::value) );
    D_CT_CHECK( (!has_unique_accessor<std::vector<int>>::value) );
    D_CT_CHECK( (!has_unique_accessor<std::set<int>>::value) );

    // value_comp() / key_comp(): ordered associatives
    D_CT_CHECK( (has_value_comp_method<std::set<int>>::value) );
    D_CT_CHECK( (has_value_comp_method<std::map<int, int>>::value) );
    D_CT_CHECK( (has_key_comp_method<std::set<int>>::value) );
    D_CT_CHECK( (has_key_comp_method<std::map<int, int>>::value) );
    D_CT_CHECK( (!has_value_comp_method<std::vector<int>>::value) );
    D_CT_CHECK( (!has_key_comp_method<std::unordered_set<int>>::value) );

    // multiplicity accessors: the custom fixture
    D_CT_CHECK( (has_multiplicity_min_accessor<mult_accessors>::value) );
    D_CT_CHECK( (has_multiplicity_max_accessor<mult_accessors>::value) );
    D_CT_CHECK( (!has_multiplicity_min_accessor<std::set<int>>::value) );

    return _ct_ok;
}


NS_END  // testing
NS_END  // djinterp
