/******************************************************************************
* djinterp [containers]                                  registry_traits.hpp
*
* SFINAE-based traits for registry entry and registry container requirements.
*   Provides type traits that detect whether an entry type has optional
* column mixins (description, default value, type info, validator, etc.),
* whether two entry types are column-compatible, and whether a container
* type satisfies the registry interface contract (find, contains, iterate).
*
*   The top-level composite traits aggregate low-level detectors into
* single booleans that can gate template overloads or static assertions.
*
*   No tag dispatching or concepts are used; all detection is performed via
* the void_t / detector SFINAE idiom from type_traits.hpp.
*
*
* TABLE OF CONTENTS
* =================
* I.    ENTRY-LEVEL EXPRESSION ALIASES
*       1.  key_expr              (.key member)
*       2.  value_expr            (.value member)
*       3.  description_expr      (.description member)
*       4.  default_value_expr    (.default_value member)
*       5.  type_id_expr          (.type_id member)
*       6.  type_info_expr        (.type_info member)
*       7.  validate_expr         (.validate(const value&) callable)
*       8.  min_value_expr        (.min_value member)
*       9.  max_value_expr        (.max_value member)
*       10. units_expr            (.units member)
*       11. format_expr           (.format member)
*       12. permissions_expr      (.permissions member)
*
* II.   ENTRY-LEVEL has_* TRAITS
*       1.  has_key               (.key)
*       2.  has_value             (.value)
*       3.  has_description       (.description)
*       4.  has_default_value     (.default_value)
*       5.  has_type_id           (.type_id)
*       6.  has_type_info         (.type_info)
*       7.  has_validator         (.validate callable)
*       8.  has_min_value         (.min_value)
*       9.  has_max_value         (.max_value)
*       10. has_units             (.units)
*       11. has_format            (.format)
*       12. has_permissions       (.permissions)
*
* III.  ENTRY TYPE EXTRACTION
*       1.  entry_key_t           (key type or nonesuch)
*       2.  entry_value_t         (value type or nonesuch)
*       3.  entry_description_t   (description type or nonesuch)
*       4.  entry_default_t       (default_value type or nonesuch)
*       5.  entry_type_id_t       (type_id type or nonesuch)
*       6.  entry_type_info_t     (type_info type or nonesuch)
*
* IV.   COMPOSITE ENTRY TRAITS
*       1.  is_registry_entry     (key + value)
*       2.  is_described          (has description)
*       3.  is_defaultable        (has default_value)
*       4.  is_typed              (has type_id or type_info)
*       5.  is_validatable        (has validate callable on value)
*       6.  is_bounded            (has min_value and max_value)
*       7.  is_fully_described    (description + default + typed)
*       8.  is_self_validating    (validatable + bounded)
*       9.  entry_known_column_count  (count of detected columns)
*
* V.    ENTRY COMPATIBILITY TRAITS
*       1.  entries_share_key_type    (same key type)
*       2.  entries_share_value_type  (same value type)
*       3.  entries_are_compatible    (same key + value types)
*       4.  entry_is_superset_of     (all columns of _Sub + more)
*
* VI.   REGISTRY-LEVEL EXPRESSION ALIASES
*       1.  entry_type_alias_expr     (::entry_type nested type)
*       2.  key_type_alias_expr       (::key_type nested type)
*       3.  value_type_alias_expr     (::value_type nested type)
*       4.  extent_expr               (::extent static member)
*       5.  find_method_expr          (.find(key) -> const entry*)
*       6.  contains_method_expr      (.contains(key) -> bool)
*       7.  value_or_method_expr      (.value_or(key, fallback))
*       8.  size_method_expr          (.size())
*       9.  empty_method_expr         (.empty())
*       10. begin_method_expr         (.begin())
*       11. end_method_expr           (.end())
*       12. backing_array_method_expr (.backing_array())
*       13. count_if_method_expr      (.count_if(pred))
*       14. validate_all_method_expr  (.validate_all())
*
* VII.  REGISTRY-LEVEL has_* TRAITS
*       1.  has_entry_type_alias  (::entry_type)
*       2.  has_key_type_alias    (::key_type)
*       3.  has_value_type_alias  (::value_type)
*       4.  has_extent            (::extent)
*       5.  has_find_method       (.find)
*       6.  has_contains_method   (.contains)
*       7.  has_value_or_method   (.value_or)
*       8.  has_size_method       (.size)
*       9.  has_empty_method      (.empty)
*       10. has_begin_method      (.begin)
*       11. has_end_method        (.end)
*       12. has_backing_array     (.backing_array)
*       13. has_count_if_method   (.count_if)
*       14. has_validate_all      (.validate_all)
*
* VIII. COMPOSITE REGISTRY TRAITS
*       1.  is_registry           (entry_type + find + contains +
*                                  size + begin + end)
*       2.  is_iterable_registry  (begin + end)
*       3.  is_searchable_registry(find + contains)
*       4.  is_static_registry    (has backing_array)
*       5.  is_dynamic_registry   (is_registry + no backing_array)
*       6.  is_validatable_registry (is_registry + validate_all)
*
* IX.   CONVENIENCE ALIASES
*       1.  _v variable templates (gated on variable template support)
*
*
* path:      /inc/containers/registry_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.XX.XX
******************************************************************************/

#ifndef DJINTERP_CONTAINER_REGISTRY_TRAITS_
#define DJINTERP_CONTAINER_REGISTRY_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include "../djinterp.hpp"
#include "../type_traits.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///        I.    ENTRY-LEVEL EXPRESSION ALIASES                             ///
///////////////////////////////////////////////////////////////////////////////
// Each alias attempts to form a type from a member access
// expression on _Type.  If the member does not exist, the
// expression is ill-formed and the detector maps it to
// false_type / nonesuch.  Users add new column detectors by
// following this pattern with their own member names.

NS_INTERNAL

    // ----------------------------------------------------------------
    // mandatory base members
    // ----------------------------------------------------------------

    // key_expr
    //   trait: expression alias for .key member detection.
    template<typename _Type>
    using key_expr = decltype(std::declval<const _Type&>().key);

    // value_expr
    //   trait: expression alias for .value member detection.
    template<typename _Type>
    using value_expr = decltype(std::declval<const _Type&>().value);

    // ----------------------------------------------------------------
    // optional column members
    // ----------------------------------------------------------------

    // description_expr
    //   trait: expression alias for .description member.
    template<typename _Type>
    using description_expr = decltype(std::declval<const _Type&>().description);

    // default_value_expr
    //   trait: expression alias for .default_value member.
    template<typename _Type>
    using default_value_expr = decltype(std::declval<const _Type&>().default_value);

    // type_id_expr
    //   trait: expression alias for .type_id member.
    template<typename _Type>
    using type_id_expr = decltype(std::declval<const _Type&>().type_id);

    // type_info_expr
    //   trait: expression alias for .type_info member
    // (d_type_info16 / d_type_info32 / d_type_info64).
    template<typename _Type>
    using type_info_expr = decltype(std::declval<const _Type&>().type_info);

    // validate_expr
    //   trait: expression alias detecting a .validate callable
    // that accepts the entry's own value type by const ref.
    // Ill-formed when either .value or .validate is absent.
    template<typename _Type>
    using validate_expr = decltype(
        std::declval<const _Type&>().validate(
            std::declval<
                const decltype(
                    std::declval<const _Type&>().value)&>()));

    // min_value_expr
    //   trait: expression alias for .min_value member.
    template<typename _Type>
    using min_value_expr = decltype(std::declval<const _Type&>().min_value);

    // max_value_expr
    //   trait: expression alias for .max_value member.
    template<typename _Type>
    using max_value_expr = decltype(std::declval<const _Type&>().max_value);

    // units_expr
    //   trait: expression alias for .units member.
    template<typename _Type>
    using units_expr = decltype(std::declval<const _Type&>().units);

    // format_expr
    //   trait: expression alias for .format member.
    template<typename _Type>
    using format_expr = decltype(std::declval<const _Type&>().format);

    // permissions_expr
    //   trait: expression alias for .permissions member.
    template<typename _Type>
    using permissions_expr = decltype(std::declval<const _Type&>().permissions);

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///        II.   ENTRY-LEVEL has_* TRAITS                                   ///
///////////////////////////////////////////////////////////////////////////////

// has_key
//   trait: detects whether _Type has a .key member.
template<typename _Type>
struct has_key
{
    static constexpr bool value =
        is_detected<internal::key_expr, _Type>::value;
};

// has_value
//   trait: detects whether _Type has a .value member.
template<typename _Type>
struct has_value
{
    static constexpr bool value =
        is_detected<internal::value_expr, _Type>::value;
};

// has_description
//   trait: detects whether _Type has a .description member.
template<typename _Type>
struct has_description
{
    static constexpr bool value =
        is_detected<internal::description_expr, _Type>::value;
};

// has_default_value
//   trait: detects whether _Type has a .default_value member.
template<typename _Type>
struct has_default_value
{
    static constexpr bool value =
        is_detected<internal::default_value_expr, _Type>::value;
};

// has_type_id
//   trait: detects whether _Type has a .type_id member.
template<typename _Type>
struct has_type_id
{
    static constexpr bool value =
        is_detected<internal::type_id_expr, _Type>::value;
};

// has_type_info
//   trait: detects whether _Type has a .type_info member.
template<typename _Type>
struct has_type_info
{
    static constexpr bool value =
        is_detected<internal::type_info_expr, _Type>::value;
};

// has_validator
//   trait: detects whether _Type has a .validate callable that
// accepts const value& and returns bool.
template<typename _Type>
struct has_validator
{
    static constexpr bool value =
        is_detected<internal::validate_expr, _Type>::value;
};

// has_min_value
//   trait: detects whether _Type has a .min_value member.
template<typename _Type>
struct has_min_value
{
    static constexpr bool value =
        is_detected<internal::min_value_expr, _Type>::value;
};

// has_max_value
//   trait: detects whether _Type has a .max_value member.
template<typename _Type>
struct has_max_value
{
    static constexpr bool value =
        is_detected<internal::max_value_expr, _Type>::value;
};

// has_units
//   trait: detects whether _Type has a .units member.
template<typename _Type>
struct has_units
{
    static constexpr bool value =
        is_detected<internal::units_expr, _Type>::value;
};

// has_format
//   trait: detects whether _Type has a .format member.
template<typename _Type>
struct has_format
{
    static constexpr bool value =
        is_detected<internal::format_expr, _Type>::value;
};

// has_permissions
//   trait: detects whether _Type has a .permissions member.
template<typename _Type>
struct has_permissions
{
    static constexpr bool value =
        is_detected<internal::permissions_expr, _Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        III.  ENTRY TYPE EXTRACTION                                      ///
///////////////////////////////////////////////////////////////////////////////

// entry_key_t
//   type: extracts the key type from a registry entry, or
// nonesuch if unavailable.
template<typename _Type>
using entry_key_t =
    detected_or_t<nonesuch, internal::key_expr, _Type>;

// entry_value_t
//   type: extracts the value type from a registry entry, or
// nonesuch if unavailable.
template<typename _Type>
using entry_value_t =
    detected_or_t<nonesuch, internal::value_expr, _Type>;

// entry_description_t
//   type: extracts the description type from a registry entry,
// or nonesuch if unavailable.
template<typename _Type>
using entry_description_t =
    detected_or_t<nonesuch, internal::description_expr, _Type>;

// entry_default_t
//   type: extracts the default_value type from a registry
// entry, or nonesuch if unavailable.
template<typename _Type>
using entry_default_t =
    detected_or_t<nonesuch, internal::default_value_expr, _Type>;

// entry_type_id_t
//   type: extracts the type_id type from a registry entry,
// or nonesuch if unavailable.
template<typename _Type>
using entry_type_id_t =
    detected_or_t<nonesuch, internal::type_id_expr, _Type>;

// entry_type_info_t
//   type: extracts the type_info type from a registry entry,
// or nonesuch if unavailable.
template<typename _Type>
using entry_type_info_t =
    detected_or_t<nonesuch, internal::type_info_expr, _Type>;


///////////////////////////////////////////////////////////////////////////////
///        IV.   COMPOSITE ENTRY TRAITS                                     ///
///////////////////////////////////////////////////////////////////////////////

// is_registry_entry
//   trait: true when _Type satisfies the minimum registry entry
// contract (has .key and .value members).
template<typename _Type>
struct is_registry_entry
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_key<clean_type>::value &&
          has_value<clean_type>::value );
};

// is_described
//   trait: true when _Type has a .description member.
template<typename _Type>
struct is_described
{
    static constexpr bool value = has_description<_Type>::value;
};

// is_defaultable
//   trait: true when _Type has a .default_value member.
template<typename _Type>
struct is_defaultable
{
    static constexpr bool value = has_default_value<_Type>::value;
};

// is_typed
//   trait: true when _Type carries type metadata via either a
// .type_id or a .type_info member (or both).
template<typename _Type>
struct is_typed
{
    static constexpr bool value =
        ( has_type_id<_Type>::value ||
          has_type_info<_Type>::value );
};

// is_validatable
//   trait: true when _Type has both a .value and a callable
// .validate member.
template<typename _Type>
struct is_validatable
{
    static constexpr bool value =
        ( has_value<_Type>::value &&
          has_validator<_Type>::value );
};

// is_bounded
//   trait: true when _Type has both .min_value and .max_value
// members, enabling range-checked validation.
template<typename _Type>
struct is_bounded
{
    static constexpr bool value =
        ( has_min_value<_Type>::value &&
          has_max_value<_Type>::value );
};

// is_fully_described
//   trait: true when _Type has description, default value, and
// type metadata — sufficient for automatic documentation or
// help-text generation.
template<typename _Type>
struct is_fully_described
{
    static constexpr bool value =
        ( is_described<_Type>::value   &&
          is_defaultable<_Type>::value &&
          is_typed<_Type>::value );
};

// is_self_validating
//   trait: true when _Type has both a callable validator and
// range bounds, enabling complete automated validation.
template<typename _Type>
struct is_self_validating
{
    static constexpr bool value =
        ( is_validatable<_Type>::value &&
          is_bounded<_Type>::value );
};

// entry_known_column_count
//   trait: counts the number of detected optional columns.
// NOTE: this counts only the columns known to this header;
// user-defined columns outside this set are not counted.
template<typename _Type>
struct entry_known_column_count
{
    static constexpr std::size_t value =
        has_description<_Type>::value   +
        has_default_value<_Type>::value +
        has_type_id<_Type>::value       +
        has_type_info<_Type>::value     +
        has_validator<_Type>::value     +
        has_min_value<_Type>::value     +
        has_max_value<_Type>::value     +
        has_units<_Type>::value         +
        has_format<_Type>::value        +
        has_permissions<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        V.    ENTRY COMPATIBILITY TRAITS                                 ///
///////////////////////////////////////////////////////////////////////////////

// entries_share_key_type
//   trait: true when both _E1 and _E2 have .key members of
// the same type.
template<typename _E1,
         typename _E2>
struct entries_share_key_type
{
    static constexpr bool value =
        ( has_key<_E1>::value &&
          has_key<_E2>::value &&
          std::is_same<entry_key_t<_E1>,
                       entry_key_t<_E2>>::value );
};

// entries_share_value_type
//   trait: true when both _E1 and _E2 have .value members of
// the same type.
template<typename _E1,
         typename _E2>
struct entries_share_value_type
{
    static constexpr bool value =
        ( has_value<_E1>::value &&
          has_value<_E2>::value &&
          std::is_same<entry_value_t<_E1>,
                       entry_value_t<_E2>>::value );
};

// entries_are_compatible
//   trait: true when _E1 and _E2 share both key and value
// types.  Compatible entries can coexist in concatenated or
// merged registries (provided they share the same entry type
// or a common supertype).
template<typename _E1,
         typename _E2>
struct entries_are_compatible
{
    static constexpr bool value =
        ( entries_share_key_type<_E1, _E2>::value &&
          entries_share_value_type<_E1, _E2>::value );
};

// entry_is_superset_of
//   trait: true when _Super has every optional column that
// _Sub has.  Useful for verifying that a richer entry type
// can substitute for a leaner one without losing data.
// NOTE: detection is limited to columns known to this header.
template<typename _Super,
         typename _Sub>
struct entry_is_superset_of
{
private:
    // for each column in _Sub, _Super must also have it
    static constexpr bool check_description =
        ( !has_description<_Sub>::value ||
           has_description<_Super>::value );

    static constexpr bool check_default =
        ( !has_default_value<_Sub>::value ||
           has_default_value<_Super>::value );

    static constexpr bool check_type_id =
        ( !has_type_id<_Sub>::value ||
           has_type_id<_Super>::value );

    static constexpr bool check_type_info =
        ( !has_type_info<_Sub>::value ||
           has_type_info<_Super>::value );

    static constexpr bool check_validator =
        ( !has_validator<_Sub>::value ||
           has_validator<_Super>::value );

    static constexpr bool check_min =
        ( !has_min_value<_Sub>::value ||
           has_min_value<_Super>::value );

    static constexpr bool check_max =
        ( !has_max_value<_Sub>::value ||
           has_max_value<_Super>::value );

    static constexpr bool check_units =
        ( !has_units<_Sub>::value ||
           has_units<_Super>::value );

    static constexpr bool check_format =
        ( !has_format<_Sub>::value ||
           has_format<_Super>::value );

    static constexpr bool check_permissions =
        ( !has_permissions<_Sub>::value ||
           has_permissions<_Super>::value );

public:
    static constexpr bool value =
        ( entries_are_compatible<_Super, _Sub>::value &&
          check_description  &&
          check_default      &&
          check_type_id      &&
          check_type_info    &&
          check_validator    &&
          check_min          &&
          check_max          &&
          check_units        &&
          check_format       &&
          check_permissions );
};


///////////////////////////////////////////////////////////////////////////////
///        VI.   REGISTRY-LEVEL EXPRESSION ALIASES                         ///
///////////////////////////////////////////////////////////////////////////////
// Detects whether a container type exposes the interface
// expected of a registry: nested type aliases, lookup methods,
// iteration, and optional features like validation.  Each
// expression is probed in a void_t context; an ill-formed
// expression maps to false_type via the detector.

NS_INTERNAL

    // ----------------------------------------------------------------
    // nested type aliases
    // ----------------------------------------------------------------

    // entry_type_alias_expr
    //   trait: expression alias for ::entry_type.
    template<typename _R>
    using entry_type_alias_expr = typename _R::entry_type;

    // key_type_alias_expr
    //   trait: expression alias for ::key_type.
    template<typename _R>
    using key_type_alias_expr = typename _R::key_type;

    // value_type_alias_expr
    //   trait: expression alias for ::value_type.
    template<typename _R>
    using value_type_alias_expr = typename _R::value_type;

    // extent_expr
    //   trait: expression alias for ::extent static member.
    template<typename _R>
    using extent_expr = decltype(_R::extent);

    // ----------------------------------------------------------------
    // lookup methods
    // ----------------------------------------------------------------

    // find_method_expr
    //   trait: expression alias for .find(key) method.
    // Probes the full call expression including the key_type
    // argument.  Ill-formed when ::key_type is absent.
    template<typename _R>
    using find_method_expr = decltype(
        std::declval<const _R&>().find(
            std::declval<const typename _R::key_type&>()));

    // contains_method_expr
    //   trait: expression alias for .contains(key) method.
    template<typename _R>
    using contains_method_expr = decltype(
        std::declval<const _R&>().contains(
            std::declval<const typename _R::key_type&>()));

    // value_or_method_expr
    //   trait: expression alias for .value_or(key, fallback).
    template<typename _R>
    using value_or_method_expr = decltype(
        std::declval<const _R&>().value_or(
            std::declval<const typename _R::key_type&>(),
            std::declval<const typename _R::value_type&>()));

    // ----------------------------------------------------------------
    // capacity methods
    // ----------------------------------------------------------------

    // size_method_expr
    //   trait: expression alias for .size() method.
    template<typename _R>
    using size_method_expr =
        decltype(std::declval<const _R&>().size());

    // empty_method_expr
    //   trait: expression alias for .empty() method.
    template<typename _R>
    using empty_method_expr =
        decltype(std::declval<const _R&>().empty());

    // ----------------------------------------------------------------
    // iteration methods
    // ----------------------------------------------------------------

    // begin_method_expr
    //   trait: expression alias for .begin() method.
    template<typename _R>
    using begin_method_expr =
        decltype(std::declval<const _R&>().begin());

    // end_method_expr
    //   trait: expression alias for .end() method.
    template<typename _R>
    using end_method_expr =
        decltype(std::declval<const _R&>().end());

    // ----------------------------------------------------------------
    // optional registry features
    // ----------------------------------------------------------------

    // backing_array_method_expr
    //   trait: expression alias for .backing_array() method.
    // Present only on static-extent registries.
    template<typename _R>
    using backing_array_method_expr =
        decltype(std::declval<const _R&>().backing_array());

    // count_if_method_expr
    //   trait: expression alias for .count_if(pred) method.
    // Uses a trivial always-true lambda shape for detection.
    template<typename _R>
    using count_if_method_expr = decltype(
        std::declval<const _R&>().count_if(
            std::declval<bool(*)(
                const typename _R::entry_type&)>()));

    // validate_all_method_expr
    //   trait: expression alias for .validate_all() method.
    // Present only when entries carry a validator column.
    template<typename _R>
    using validate_all_method_expr =
        decltype(std::declval<const _R&>().validate_all());

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///        VII.  REGISTRY-LEVEL has_* TRAITS                                ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------
// nested type aliases
// ----------------------------------------------------------------

// has_entry_type_alias
//   trait: detects whether _R has a ::entry_type nested type.
template<typename _R>
struct has_entry_type_alias
{
    static constexpr bool value =
        is_detected<internal::entry_type_alias_expr, _R>::value;
};

// has_key_type_alias
//   trait: detects whether _R has a ::key_type nested type.
template<typename _R>
struct has_key_type_alias
{
    static constexpr bool value =
        is_detected<internal::key_type_alias_expr, _R>::value;
};

// has_value_type_alias
//   trait: detects whether _R has a ::value_type nested type.
template<typename _R>
struct has_value_type_alias
{
    static constexpr bool value =
        is_detected<internal::value_type_alias_expr, _R>::value;
};

// has_extent
//   trait: detects whether _R has a ::extent static member.
template<typename _R>
struct has_extent
{
    static constexpr bool value =
        is_detected<internal::extent_expr, _R>::value;
};

// ----------------------------------------------------------------
// lookup methods
// ----------------------------------------------------------------

// has_find_method
//   trait: detects whether _R has a .find(key) method.
template<typename _R>
struct has_find_method
{
    static constexpr bool value =
        is_detected<internal::find_method_expr, _R>::value;
};

// has_contains_method
//   trait: detects whether _R has a .contains(key) method.
template<typename _R>
struct has_contains_method
{
    static constexpr bool value =
        is_detected<internal::contains_method_expr, _R>::value;
};

// has_value_or_method
//   trait: detects whether _R has a .value_or(key, fallback)
// method.
template<typename _R>
struct has_value_or_method
{
    static constexpr bool value =
        is_detected<internal::value_or_method_expr, _R>::value;
};

// ----------------------------------------------------------------
// capacity methods
// ----------------------------------------------------------------

// has_size_method
//   trait: detects whether _R has a .size() method.
template<typename _R>
struct has_size_method
{
    static constexpr bool value =
        is_detected<internal::size_method_expr, _R>::value;
};

// has_empty_method
//   trait: detects whether _R has an .empty() method.
template<typename _R>
struct has_empty_method
{
    static constexpr bool value =
        is_detected<internal::empty_method_expr, _R>::value;
};

// ----------------------------------------------------------------
// iteration methods
// ----------------------------------------------------------------

// has_begin_method
//   trait: detects whether _R has a .begin() method.
template<typename _R>
struct has_begin_method
{
    static constexpr bool value =
        is_detected<internal::begin_method_expr, _R>::value;
};

// has_end_method
//   trait: detects whether _R has an .end() method.
template<typename _R>
struct has_end_method
{
    static constexpr bool value =
        is_detected<internal::end_method_expr, _R>::value;
};

// ----------------------------------------------------------------
// optional features
// ----------------------------------------------------------------

// has_backing_array
//   trait: detects whether _R has a .backing_array() method.
// This member exists only on static-extent registries,
// providing a tagless distinction between static and dynamic.
template<typename _R>
struct has_backing_array
{
    static constexpr bool value =
        is_detected<internal::backing_array_method_expr, _R>::value;
};

// has_count_if_method
//   trait: detects whether _R has a .count_if(pred) method.
template<typename _R>
struct has_count_if_method
{
    static constexpr bool value =
        is_detected<internal::count_if_method_expr, _R>::value;
};

// has_validate_all
//   trait: detects whether _R has a .validate_all() method.
// This member exists only when the entry type carries a
// validator column — another tagless distinction.
template<typename _R>
struct has_validate_all
{
    static constexpr bool value =
        is_detected<internal::validate_all_method_expr, _R>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        VIII. COMPOSITE REGISTRY TRAITS                                  ///
///////////////////////////////////////////////////////////////////////////////

// is_registry
//   trait: true when _R satisfies the minimum registry
// container contract: exposes ::entry_type, supports keyed
// lookup via .find and .contains, reports size via .size,
// and is iterable via .begin/.end.
template<typename _R>
struct is_registry
{
private:
    using clean_type = clean_t<_R>;

public:
    static constexpr bool value =
        ( has_entry_type_alias<clean_type>::value  &&
          has_find_method<clean_type>::value        &&
          has_contains_method<clean_type>::value    &&
          has_size_method<clean_type>::value        &&
          has_begin_method<clean_type>::value       &&
          has_end_method<clean_type>::value );
};

// is_iterable_registry
//   trait: true when _R supports range-based iteration
// via .begin() and .end().
template<typename _R>
struct is_iterable_registry
{
    static constexpr bool value =
        ( has_begin_method<_R>::value &&
          has_end_method<_R>::value );
};

// is_searchable_registry
//   trait: true when _R supports keyed lookup via .find()
// and .contains().
template<typename _R>
struct is_searchable_registry
{
    static constexpr bool value =
        ( has_find_method<_R>::value &&
          has_contains_method<_R>::value );
};

// is_static_registry
//   trait: true when _R exposes .backing_array(), which
// is present only on static-extent (compile-time-sized)
// registries.  Tagless: no _Sorted or _N inspection.
template<typename _R>
struct is_static_registry
{
    static constexpr bool value =
        ( is_registry<_R>::value &&
          has_backing_array<_R>::value );
};

// is_dynamic_registry
//   trait: true when _R satisfies the registry contract but
// does NOT expose .backing_array(), indicating a heap-backed
// dynamic-extent instance.
template<typename _R>
struct is_dynamic_registry
{
    static constexpr bool value =
        ( is_registry<_R>::value &&
          !has_backing_array<_R>::value );
};

// is_validatable_registry
//   trait: true when _R satisfies the registry contract and
// exposes .validate_all(), indicating that the entry type
// carries a validator column.
template<typename _R>
struct is_validatable_registry
{
    static constexpr bool value =
        ( is_registry<_R>::value &&
          has_validate_all<_R>::value );
};


///////////////////////////////////////////////////////////////////////////////
///        IX.   CONVENIENCE ALIASES                                        ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // ----------------------------------------------------------------
    // entry-level _v aliases
    // ----------------------------------------------------------------

    template<typename _Type>
    constexpr bool has_key_v = has_key<_Type>::value;

    template<typename _Type>
    constexpr bool has_value_v = has_value<_Type>::value;

    template<typename _Type>
    constexpr bool has_description_v = has_description<_Type>::value;

    template<typename _Type>
    constexpr bool has_default_value_v = has_default_value<_Type>::value;

    template<typename _Type>
    constexpr bool has_type_id_v = has_type_id<_Type>::value;

    template<typename _Type>
    constexpr bool has_type_info_v = has_type_info<_Type>::value;

    template<typename _Type>
    constexpr bool has_validator_v = has_validator<_Type>::value;

    template<typename _Type>
    constexpr bool has_min_value_v = has_min_value<_Type>::value;

    template<typename _Type>
    constexpr bool has_max_value_v = has_max_value<_Type>::value;

    template<typename _Type>
    constexpr bool has_units_v = has_units<_Type>::value;

    template<typename _Type>
    constexpr bool has_format_v = has_format<_Type>::value;

    template<typename _Type>
    constexpr bool has_permissions_v = has_permissions<_Type>::value;

    // ----------------------------------------------------------------
    // composite entry _v aliases
    // ----------------------------------------------------------------

    template<typename _Type>
    constexpr bool is_registry_entry_v =
        is_registry_entry<_Type>::value;

    template<typename _Type>
    constexpr bool is_described_v = is_described<_Type>::value;

    template<typename _Type>
    constexpr bool is_defaultable_v = is_defaultable<_Type>::value;

    template<typename _Type>
    constexpr bool is_typed_v = is_typed<_Type>::value;

    template<typename _Type>
    constexpr bool is_validatable_v = is_validatable<_Type>::value;

    template<typename _Type>
    constexpr bool is_bounded_v = is_bounded<_Type>::value;

    template<typename _Type>
    constexpr bool is_fully_described_v =
        is_fully_described<_Type>::value;

    template<typename _Type>
    constexpr bool is_self_validating_v =
        is_self_validating<_Type>::value;

    template<typename _Type>
    constexpr std::size_t entry_known_column_count_v =
        entry_known_column_count<_Type>::value;

    // ----------------------------------------------------------------
    // entry compatibility _v aliases
    // ----------------------------------------------------------------

    template<typename _E1, typename _E2>
    constexpr bool entries_share_key_type_v =
        entries_share_key_type<_E1, _E2>::value;

    template<typename _E1, typename _E2>
    constexpr bool entries_share_value_type_v =
        entries_share_value_type<_E1, _E2>::value;

    template<typename _E1, typename _E2>
    constexpr bool entries_are_compatible_v =
        entries_are_compatible<_E1, _E2>::value;

    template<typename _Super, typename _Sub>
    constexpr bool entry_is_superset_of_v =
        entry_is_superset_of<_Super, _Sub>::value;

    // ----------------------------------------------------------------
    // registry-level _v aliases
    // ----------------------------------------------------------------

    template<typename _R>
    constexpr bool has_entry_type_alias_v =
        has_entry_type_alias<_R>::value;

    template<typename _R>
    constexpr bool has_key_type_alias_v =
        has_key_type_alias<_R>::value;

    template<typename _R>
    constexpr bool has_value_type_alias_v =
        has_value_type_alias<_R>::value;

    template<typename _R>
    constexpr bool has_extent_v = has_extent<_R>::value;

    template<typename _R>
    constexpr bool has_find_method_v =
        has_find_method<_R>::value;

    template<typename _R>
    constexpr bool has_contains_method_v =
        has_contains_method<_R>::value;

    template<typename _R>
    constexpr bool has_value_or_method_v =
        has_value_or_method<_R>::value;

    template<typename _R>
    constexpr bool has_size_method_v =
        has_size_method<_R>::value;

    template<typename _R>
    constexpr bool has_empty_method_v =
        has_empty_method<_R>::value;

    template<typename _R>
    constexpr bool has_begin_method_v =
        has_begin_method<_R>::value;

    template<typename _R>
    constexpr bool has_end_method_v =
        has_end_method<_R>::value;

    template<typename _R>
    constexpr bool has_backing_array_v =
        has_backing_array<_R>::value;

    template<typename _R>
    constexpr bool has_count_if_method_v =
        has_count_if_method<_R>::value;

    template<typename _R>
    constexpr bool has_validate_all_v =
        has_validate_all<_R>::value;

    // ----------------------------------------------------------------
    // composite registry _v aliases
    // ----------------------------------------------------------------

    template<typename _R>
    constexpr bool is_registry_v = is_registry<_R>::value;

    template<typename _R>
    constexpr bool is_iterable_registry_v =
        is_iterable_registry<_R>::value;

    template<typename _R>
    constexpr bool is_searchable_registry_v =
        is_searchable_registry<_R>::value;

    template<typename _R>
    constexpr bool is_static_registry_v =
        is_static_registry<_R>::value;

    template<typename _R>
    constexpr bool is_dynamic_registry_v =
        is_dynamic_registry<_R>::value;

    template<typename _R>
    constexpr bool is_validatable_registry_v =
        is_validatable_registry<_R>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_REGISTRY_TRAITS_
