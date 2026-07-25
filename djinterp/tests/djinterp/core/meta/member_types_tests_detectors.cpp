#include "member_types_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_detectors_all_ten_fire_on_has_all
  Every detector is true for a struct exposing all ten typedefs.
  Tests the following:
  - the four parser/scanner/token contract detectors fire
  - has_value_type fires
  - the associative detectors (key/mapped) fire
  - the container detectors (size/difference/allocator) fire
*/
bool
tests_detectors_all_ten_fire_on_has_all()
{
    D_MT_CHECK(has_input_type<has_all>::value);
    D_MT_CHECK(has_result_type<has_all>::value);
    D_MT_CHECK(has_item_type<has_all>::value);
    D_MT_CHECK(has_kind_type<has_all>::value);
    D_MT_CHECK(has_value_type<has_all>::value);
    D_MT_CHECK(has_key_type<has_all>::value);
    D_MT_CHECK(has_mapped_type<has_all>::value);
    D_MT_CHECK(has_size_type<has_all>::value);
    D_MT_CHECK(has_difference_type<has_all>::value);
    D_MT_CHECK(has_allocator_type<has_all>::value);

    return true;
}

/*
tests_detectors_all_ten_silent_on_has_none
  Every detector is false for a struct exposing none of them.
  Tests the following:
  - the contract detectors are false
  - has_value_type is false
  - the associative detectors are false
  - the container detectors are false
*/
bool
tests_detectors_all_ten_silent_on_has_none()
{
    D_MT_CHECK(!has_input_type<has_none>::value);
    D_MT_CHECK(!has_result_type<has_none>::value);
    D_MT_CHECK(!has_item_type<has_none>::value);
    D_MT_CHECK(!has_kind_type<has_none>::value);
    D_MT_CHECK(!has_value_type<has_none>::value);
    D_MT_CHECK(!has_key_type<has_none>::value);
    D_MT_CHECK(!has_mapped_type<has_none>::value);
    D_MT_CHECK(!has_size_type<has_none>::value);
    D_MT_CHECK(!has_difference_type<has_none>::value);
    D_MT_CHECK(!has_allocator_type<has_none>::value);

    return true;
}

/*
tests_detectors_each_fires_only_on_its_own_typedef
  Each detector keys on exactly its own typedef, never a sibling's.
  Tests the following:
  - a fixture exposing only input_type trips only has_input_type
  - a fixture exposing only value_type trips only has_value_type
  - the associative and container detectors are likewise isolated
  - no detector reports a member a fixture does not declare
*/
bool
tests_detectors_each_fires_only_on_its_own_typedef()
{
    // only_input: has_input_type true, everything else false
    D_MT_CHECK(has_input_type<only_input>::value);
    D_MT_CHECK(!has_result_type<only_input>::value);
    D_MT_CHECK(!has_value_type<only_input>::value);
    D_MT_CHECK(!has_allocator_type<only_input>::value);

    // only_result
    D_MT_CHECK(has_result_type<only_result>::value);
    D_MT_CHECK(!has_input_type<only_result>::value);
    D_MT_CHECK(!has_item_type<only_result>::value);

    // only_item
    D_MT_CHECK(has_item_type<only_item>::value);
    D_MT_CHECK(!has_kind_type<only_item>::value);

    // only_kind
    D_MT_CHECK(has_kind_type<only_kind>::value);
    D_MT_CHECK(!has_value_type<only_kind>::value);

    // only_value
    D_MT_CHECK(has_value_type<only_value>::value);
    D_MT_CHECK(!has_key_type<only_value>::value);
    D_MT_CHECK(!has_mapped_type<only_value>::value);

    // only_key vs only_mapped -- the associative pair are independent
    D_MT_CHECK(has_key_type<only_key>::value);
    D_MT_CHECK(!has_mapped_type<only_key>::value);
    D_MT_CHECK(has_mapped_type<only_mapped>::value);
    D_MT_CHECK(!has_key_type<only_mapped>::value);

    // only_size / only_difference / only_allocator -- container trio isolated
    D_MT_CHECK(has_size_type<only_size>::value);
    D_MT_CHECK(!has_difference_type<only_size>::value);
    D_MT_CHECK(has_difference_type<only_difference>::value);
    D_MT_CHECK(!has_allocator_type<only_difference>::value);
    D_MT_CHECK(has_allocator_type<only_allocator>::value);
    D_MT_CHECK(!has_size_type<only_allocator>::value);

    return true;
}

/*
tests_detectors_on_standard_containers
  Real STL sequence types trip exactly the expected detectors.
  Tests the following:
  - std::vector<int> has value_type, size_type, difference_type, allocator_type
  - std::vector<int> has no mapped_type and no key_type
  - std::string has value_type and size_type
  - the contract detectors (input/result/item/kind) are false for both
*/
bool
tests_detectors_on_standard_containers()
{
    using vec = std::vector<int>;

    D_MT_CHECK(has_value_type<vec>::value);
    D_MT_CHECK(has_size_type<vec>::value);
    D_MT_CHECK(has_difference_type<vec>::value);
    D_MT_CHECK(has_allocator_type<vec>::value);

    D_MT_CHECK(!has_mapped_type<vec>::value);
    D_MT_CHECK(!has_key_type<vec>::value);

    // contract typedefs are not a container concept
    D_MT_CHECK(!has_input_type<vec>::value);
    D_MT_CHECK(!has_result_type<vec>::value);
    D_MT_CHECK(!has_item_type<vec>::value);
    D_MT_CHECK(!has_kind_type<vec>::value);

    // std::string
    D_MT_CHECK(has_value_type<std::string>::value);
    D_MT_CHECK(has_size_type<std::string>::value);
    D_MT_CHECK(!has_mapped_type<std::string>::value);

    return true;
}

/*
tests_detectors_associative_only_typedefs
  Associative containers expose key/mapped where sequences do not.
  Tests the following:
  - std::map has key_type and mapped_type
  - std::map also has value_type (the pair) and the container typedefs
  - a std::vector has neither key_type nor mapped_type
  - the map's key/mapped detectors distinguish it from the vector
*/
bool
tests_detectors_associative_only_typedefs()
{
    using map = std::map<int, long>;
    using vec = std::vector<int>;

    D_MT_CHECK(has_key_type<map>::value);
    D_MT_CHECK(has_mapped_type<map>::value);
    D_MT_CHECK(has_value_type<map>::value);
    D_MT_CHECK(has_size_type<map>::value);

    D_MT_CHECK(!has_key_type<vec>::value);
    D_MT_CHECK(!has_mapped_type<vec>::value);

    // the distinguishing pair
    D_MT_CHECK(has_mapped_type<map>::value != has_mapped_type<vec>::value);

    return true;
}

/*
tests_detectors_value_companions_agree
  The has_<name>_v companions match their traits (C++14+).
  Tests the following:
  - a representative _v equals ::value in the true case
  - and in the false case
  - several distinct detectors' companions agree with their traits
  - a companion is usable in a constant expression
*/
bool
tests_detectors_value_companions_agree()
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // true and false cases for one detector
    D_MT_CHECK(has_value_type_v<has_all> == has_value_type<has_all>::value);
    D_MT_CHECK(has_value_type_v<has_none> == has_value_type<has_none>::value);

    // several detectors' companions
    D_MT_CHECK(has_input_type_v<only_input> == has_input_type<only_input>::value);
    D_MT_CHECK(has_key_type_v<only_key> == has_key_type<only_key>::value);
    D_MT_CHECK(has_allocator_type_v<std::vector<int> > ==
               has_allocator_type<std::vector<int> >::value);

    // constant-expression usability
    static_assert(has_value_type_v<has_all>, "companion is a constant expression");
    static_assert(!has_value_type_v<has_none>, "companion rejects absence");
#else
    // no variable templates on this dialect; the ::value members still hold
    D_MT_CHECK(has_value_type<has_all>::value);
    D_MT_CHECK(!has_value_type<has_none>::value);
#endif

    return true;
}

NS_END  // testing
NS_END  // djinterp
