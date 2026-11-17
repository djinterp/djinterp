/*******************************************************************************
* djinterp [test]                                         array_tests_traits.cpp
*
*   Trait tests: array_traits.hpp's classification of the six cells, the
* standard containers, C arrays and the four wrappers, through cv and
* references, and the concepts over it.
*
*   The expected values are the intended ones.  Before 2026.11.14 the traits
* got several wrong; the suite header lists them.
*
*
* path:      /tests/djinterp/cpp/container/array/array_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

// std
#include <array>        // std::array
#include <string>       // std::string
#include <type_traits>  // std::is_same
#include <utility>      // std::pair
#include <vector>       // std::vector
#include <version>      // __cpp_lib_span
#if defined(__cpp_lib_span)
#include <span>         // std::span
#endif
// djinterp
#include "array_tests.hpp"  // suite header (normal face)


NS_DJINTERP
NS_TESTING

// ay_classification_mismatches
//   function: compares every classification of Type and Qualified, and
// returns a bit per trait that differs (0 when all agree).
template<typename Type,
         typename Qualified>
int ay_classification_mismatches()
{
    int m = 0;

    m |= (has_static_extent_v<Type> != has_static_extent_v<Qualified>) << 0;
    m |= (is_contiguous_array_v<Type> !=
          is_contiguous_array_v<Qualified>) << 1;
    m |= (is_iterable_array_v<Type> != is_iterable_array_v<Qualified>) << 2;
    m |= (is_mutable_array_v<Type> != is_mutable_array_v<Qualified>) << 3;
    m |= (is_immutable_array_v<Type> !=
          is_immutable_array_v<Qualified>) << 4;
    m |= (is_shiftable_array_v<Type> !=
          is_shiftable_array_v<Qualified>) << 5;
    m |= (array_strategy_v<Type> != array_strategy_v<Qualified>) << 6;
    m |= (capacity_model_of_v<Type> != capacity_model_of_v<Qualified>) << 7;
    m |= (array_lifetime_of_v<Type> != array_lifetime_of_v<Qualified>) << 8;
    m |= (is_trivially_relocatable_array_v<Type> !=
          is_trivially_relocatable_array_v<Qualified>) << 9;

    return m;
}

/*
tests_traits_extent_and_capacity
  The extent and capacity model.
  Tests the following:
  - every array<> cell has a static extent and a fixed capacity, and is not
    dynamic (its capacity() is its extent)
  - std::array and int[4] are fixed; std::vector and std::string dynamic
  - std::span<int, 4> is fixed; std::span<int> (dynamic_extent) has no
    static extent and is external storage
  - std::pair has no static extent, though it has std::tuple_size
*/
bool tests_traits_extent_and_capacity()
{
    D_AY_CHECK(has_static_extent_v<mi4> && has_static_extent_v<cni4>);
    D_AY_CHECK(is_fixed_capacity_v<mi4> && is_fixed_capacity_v<ini4>);
    D_AY_CHECK(!is_dynamic_capacity_v<mi4>);
    D_AY_CHECK(!is_dynamic_capacity_v<ii4>);
    D_AY_CHECK(!is_dynamic_capacity_v<cni4>);
    D_AY_CHECK(capacity_model_of_v<mi4> == DCapacityModel::fixed);
    D_AY_CHECK(capacity_model_of_v<ini4> == DCapacityModel::fixed);

    using stda = std::array<int, 4>;
    D_AY_CHECK(has_static_extent_v<stda> && is_fixed_capacity_v<stda>);
    D_AY_CHECK(capacity_model_of_v<int[4]> == DCapacityModel::fixed);

    D_AY_CHECK(!has_static_extent_v<std::vector<int>>);
    D_AY_CHECK(is_dynamic_capacity_v<std::vector<int>>);
    D_AY_CHECK(capacity_model_of_v<std::vector<int>> ==
               DCapacityModel::dynamic);
    D_AY_CHECK(capacity_model_of_v<std::string> == DCapacityModel::dynamic);

#if defined(__cpp_lib_span)
    D_AY_CHECK(has_static_extent_v<std::span<int, 4>>);
    D_AY_CHECK(capacity_model_of_v<std::span<int, 4>> ==
               DCapacityModel::fixed);
    D_AY_CHECK(!has_static_extent_v<std::span<int>>);
    D_AY_CHECK(!is_fixed_capacity_v<std::span<int>>);
    D_AY_CHECK(capacity_model_of_v<std::span<int>> ==
               DCapacityModel::external);
#endif

    D_AY_CHECK(!(has_static_extent_v<std::pair<int, int>>));
    D_AY_CHECK((capacity_model_of_v<std::pair<int, int>> ==
                DCapacityModel::none));

    return true;
}

/*
tests_traits_contiguity_and_metrics
  Contiguity and the element metrics.
  Tests the following:
  - all six cells are contiguous, iterable or not
  - std::array, std::vector, std::string and int[4] are contiguous
  - array_element_type_of names the element type, for array<> and int[4]
  - element size, alignment and stride follow the element type
*/
bool tests_traits_contiguity_and_metrics()
{
    D_AY_CHECK(is_contiguous_array_v<mi4> && is_contiguous_array_v<mni4>);
    D_AY_CHECK(is_contiguous_array_v<ii4> && is_contiguous_array_v<ini4>);
    D_AY_CHECK(is_contiguous_array_v<ci4> && is_contiguous_array_v<cni4>);
    D_AY_CHECK((is_contiguous_array_v<std::array<int, 4>>));
    D_AY_CHECK(is_contiguous_array_v<std::vector<int>>);
    D_AY_CHECK(is_contiguous_array_v<std::string>);
    D_AY_CHECK(is_contiguous_array_v<int[4]>);
    D_AY_CHECK(!(is_contiguous_array_v<std::pair<int, int>>));

    D_AY_CHECK((std::is_same<array_element_type_of_t<mi4>, int>::value));
    D_AY_CHECK((std::is_same<array_element_type_of_t<ms3>,
                             std::string>::value));
    D_AY_CHECK((std::is_same<array_element_type_of_t<int[4]>, int>::value));

    D_AY_CHECK(element_size_of_v<mi4> == sizeof(int));
    D_AY_CHECK(element_size_of_v<ms3> == sizeof(std::string));
    D_AY_CHECK(element_alignment_of_v<mi4> == alignof(int));
    D_AY_CHECK(element_stride_of_v<ms3> == sizeof(std::string));

    return true;
}

/*
tests_traits_strategy
  Trivial relocation and the bulk-operation strategy.
  Tests the following:
  - int arrays, in every cell, are trivially relocatable: bulk_memcpy
  - a string array is not: element_move
  - std::vector<int> is bulk_memcpy; std::pair is generic
*/
bool tests_traits_strategy()
{
    D_AY_CHECK(is_trivially_relocatable_array_v<mi4>);
    D_AY_CHECK(is_trivially_relocatable_array_v<mni4>);
    D_AY_CHECK(!is_trivially_relocatable_array_v<ms3>);

    D_AY_CHECK(array_strategy_v<mi4> == DArrayOperationsStrategy::bulk_memcpy);
    D_AY_CHECK(array_strategy_v<ini4> ==
               DArrayOperationsStrategy::bulk_memcpy);
    D_AY_CHECK(array_strategy_v<cni4> ==
               DArrayOperationsStrategy::bulk_memcpy);
    D_AY_CHECK(array_strategy_v<ms3> ==
               DArrayOperationsStrategy::element_move);
    D_AY_CHECK(array_strategy_v<std::vector<int>> ==
               DArrayOperationsStrategy::bulk_memcpy);
    D_AY_CHECK((array_strategy_v<std::pair<int, int>> ==
                DArrayOperationsStrategy::generic));

    return true;
}

/*
tests_traits_lifetime
  The lifetime classification.
  Tests the following:
  - each cell's lifetime marker decides: exactly one of constexpr, mutable
    and immutable holds, and array_lifetime_of names it
  - std::array and std::vector, unmarked, are mutable
*/
bool tests_traits_lifetime()
{
    D_AY_CHECK(is_mutable_array_v<mi4> && is_mutable_array_v<mni4>);
    D_AY_CHECK(!is_immutable_array_v<mi4> && !is_constexpr_array_v<mi4>);

    D_AY_CHECK(is_immutable_array_v<ii4> && is_immutable_array_v<ini4>);
    D_AY_CHECK(!is_mutable_array_v<ii4> && !is_constexpr_array_v<ii4>);

    D_AY_CHECK(is_constexpr_array_v<ci4> && is_constexpr_array_v<cni4>);
    D_AY_CHECK(!is_mutable_array_v<ci4> && !is_immutable_array_v<ci4>);

    D_AY_CHECK(array_lifetime_of_v<mni4> == DArrayLifetime::mutable_lifetime);
    D_AY_CHECK(array_lifetime_of_v<ii4> ==
               DArrayLifetime::immutable_lifetime);
    D_AY_CHECK(array_lifetime_of_v<cni4> ==
               DArrayLifetime::constexpr_lifetime);

    D_AY_CHECK((is_mutable_array_v<std::array<int, 4>>));
    D_AY_CHECK(is_mutable_array_v<std::vector<int>>);
    D_AY_CHECK(array_lifetime_of_v<std::vector<int>> ==
               DArrayLifetime::mutable_lifetime);

    return true;
}

/*
tests_traits_c_arrays
  A C array's lifetime follows its elements' const.
  Tests the following:
  - int[4] is mutable, not immutable, and its lifetime is mutable
  - const int[4] is immutable, not mutable, and its lifetime is immutable
  - the same through references to each
  - both have a static extent and are contiguous and iterable
*/
bool tests_traits_c_arrays()
{
    using c_int  = int[4];
    using c_cint = const int[4];

    D_AY_CHECK(is_mutable_array_v<c_int> && !is_immutable_array_v<c_int>);
    D_AY_CHECK(array_lifetime_of_v<c_int> ==
               DArrayLifetime::mutable_lifetime);

    D_AY_CHECK(is_immutable_array_v<c_cint> && !is_mutable_array_v<c_cint>);
    D_AY_CHECK(array_lifetime_of_v<c_cint> ==
               DArrayLifetime::immutable_lifetime);

    D_AY_CHECK(is_mutable_array_v<c_int&>);
    D_AY_CHECK(is_immutable_array_v<c_cint&>);
    D_AY_CHECK(array_lifetime_of_v<c_cint&> ==
               DArrayLifetime::immutable_lifetime);

    D_AY_CHECK(has_static_extent_v<c_cint> && is_contiguous_array_v<c_cint>);
    D_AY_CHECK(is_iterable_array_v<c_int> && is_iterable_array_v<c_cint>);

    return true;
}

/*
tests_traits_iterability
  is_iterable_array and is_non_iterable_array.
  Tests the following:
  - the iterable cells are iterable and not non-iterable
  - the non-iterable cells are the reverse
  - std::vector and int[4] are iterable; std::pair is neither
*/
bool tests_traits_iterability()
{
    D_AY_CHECK(is_iterable_array_v<mi4> && !is_non_iterable_array_v<mi4>);
    D_AY_CHECK(is_iterable_array_v<ii4> && is_iterable_array_v<ci4>);

    D_AY_CHECK(!is_iterable_array_v<mni4> && is_non_iterable_array_v<mni4>);
    D_AY_CHECK(!is_iterable_array_v<ini4> && is_non_iterable_array_v<ini4>);
    D_AY_CHECK(!is_iterable_array_v<cni4> && is_non_iterable_array_v<cni4>);

    D_AY_CHECK(is_iterable_array_v<std::vector<int>>);
    D_AY_CHECK(is_iterable_array_v<int[4]>);
    D_AY_CHECK(!(is_iterable_array_v<std::pair<int, int>>));
    D_AY_CHECK(!(is_non_iterable_array_v<std::pair<int, int>>));

    return true;
}

/*
tests_traits_shiftable
  is_shiftable_array: contiguous, sized, and not immutable.
  Tests the following:
  - the mutable and constexpr cells are shiftable, iterable or not
  - the immutable cells are not
  - int[4] is; const int[4] is not
  - std::vector is
*/
bool tests_traits_shiftable()
{
    D_AY_CHECK(is_shiftable_array_v<mi4> && is_shiftable_array_v<mni4>);
    D_AY_CHECK(is_shiftable_array_v<ci4> && is_shiftable_array_v<cni4>);
    D_AY_CHECK(!is_shiftable_array_v<ii4> && !is_shiftable_array_v<ini4>);

    D_AY_CHECK(is_shiftable_array_v<int[4]>);
    D_AY_CHECK(!is_shiftable_array_v<const int[4]>);
    D_AY_CHECK(is_shiftable_array_v<std::vector<int>>);

    return true;
}

/*
tests_traits_qualified_types
  const, & and && change no answer.
  Tests the following:
  - a cell against its const, &, const & and && forms
  - std::vector against const std::vector&
  - each wrapper against const W& and W&& (their specializations answer
    through the qualifiers)
*/
bool tests_traits_qualified_types()
{
    D_AY_CHECK((ay_classification_mismatches<mi4, const mi4>() == 0));
    D_AY_CHECK((ay_classification_mismatches<mi4, mi4&>() == 0));
    D_AY_CHECK((ay_classification_mismatches<mi4, const mi4&>() == 0));
    D_AY_CHECK((ay_classification_mismatches<mi4, mi4&&>() == 0));
    D_AY_CHECK((ay_classification_mismatches<ini4, const ini4&>() == 0));
    D_AY_CHECK((ay_classification_mismatches<cni4, cni4&&>() == 0));

    D_AY_CHECK((ay_classification_mismatches<std::vector<int>,
                                             const std::vector<int>&>()
                == 0));

    D_AY_CHECK((ay_classification_mismatches<atomic4,
                                             const atomic4&>() == 0));
    D_AY_CHECK((ay_classification_mismatches<cow4, const cow4&>() == 0));
    D_AY_CHECK((ay_classification_mismatches<rcu4, rcu4&&>() == 0));
    D_AY_CHECK((ay_classification_mismatches<ts4, const ts4&>() == 0));
    D_AY_CHECK((ay_classification_mismatches<ts4_ni, ts4_ni&>() == 0));

    return true;
}

/*
tests_traits_wrappers
  The wrappers keep the wrapped array's axes.
  Tests the following:
  - each wrapper is contiguous, iterable, of static extent and mutable
  - the non-iterable cow and threadsafe wrappers are not iterable, and
    still contiguous
  - each registers its own concurrency strategy and no other's
  - a plain array registers none
*/
bool tests_traits_wrappers()
{
    D_AY_CHECK(is_contiguous_array_v<atomic4> && is_iterable_array_v<atomic4>);
    D_AY_CHECK(is_contiguous_array_v<cow4> && is_iterable_array_v<cow4>);
    D_AY_CHECK(is_contiguous_array_v<rcu4> && is_iterable_array_v<rcu4>);
    D_AY_CHECK(is_contiguous_array_v<ts4> && is_iterable_array_v<ts4>);
    D_AY_CHECK(has_static_extent_v<atomic4> && has_static_extent_v<ts4>);
    D_AY_CHECK(array_lifetime_of_v<cow4> == DArrayLifetime::mutable_lifetime);
    D_AY_CHECK(array_lifetime_of_v<rcu4> == DArrayLifetime::mutable_lifetime);

    D_AY_CHECK(!is_iterable_array_v<cow4_ni> && !is_iterable_array_v<ts4_ni>);
    D_AY_CHECK(is_contiguous_array_v<cow4_ni>);
    D_AY_CHECK(is_contiguous_array_v<ts4_ni>);

    D_AY_CHECK(is_atomic_container_v<atomic4>);
    D_AY_CHECK(!is_locked_container_v<atomic4>);
    D_AY_CHECK(is_cow_container_v<cow4> && !is_rcu_container_v<cow4>);
    D_AY_CHECK(is_rcu_container_v<rcu4> && !is_cow_container_v<rcu4>);
    D_AY_CHECK(is_locked_container_v<ts4> && !is_atomic_container_v<ts4>);

    D_AY_CHECK(!is_atomic_container_v<mi4> && !is_locked_container_v<mi4>);
    D_AY_CHECK(!is_cow_container_v<mi4> && !is_rcu_container_v<mi4>);

    return true;
}

/*
tests_traits_concepts
  The concepts agree with the traits (C++20 with concepts).
  Tests the following:
  - contiguous_array, fixed_capacity_array, static_extent_array hold for a
    cell; dynamic_capacity_array doesn't, but does for std::vector
  - iterable_array / non_iterable_array split the cells
  - mutable_array, immutable_array and constexpr_array follow the
    lifetime, for int[4] and const int[4] too
  - shiftable_array and bulk_memcpy_array; element_move_array for strings
  Below C++20 the block reports a pass with nothing to check.
*/
bool tests_traits_concepts()
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    D_AY_CHECK(contiguous_array<mni4> && contiguous_array<const ii4&>);
    D_AY_CHECK(fixed_capacity_array<mi4> && static_extent_array<mi4>);
    D_AY_CHECK(!dynamic_capacity_array<mi4>);
    D_AY_CHECK(dynamic_capacity_array<std::vector<int>>);

    D_AY_CHECK(iterable_array<mi4> && !iterable_array<mni4>);
    D_AY_CHECK(non_iterable_array<cni4> && !non_iterable_array<ci4>);

    D_AY_CHECK(mutable_array<mi4> && !mutable_array<ii4>);
    D_AY_CHECK(immutable_array<ini4> && constexpr_array<ci4>);
    D_AY_CHECK(mutable_array<int[4]> && !mutable_array<const int[4]>);
    D_AY_CHECK(immutable_array<const int[4]> && !immutable_array<int[4]>);

    D_AY_CHECK(shiftable_array<mi4> && !shiftable_array<ii4>);
    D_AY_CHECK(bulk_memcpy_array<mi4> && element_move_array<ms3>);
#endif

    return true;
}

NS_END  // testing
NS_END  // djinterp
