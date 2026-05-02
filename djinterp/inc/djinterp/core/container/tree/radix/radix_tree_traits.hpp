/*
radix_tree_traits.hpp
  Standalone detection traits for radix tree key types, node shape, and
  container interface members. Depends only on standard library headers.
  Shared across text_radix_tree and binary_radix_tree.

  Trait summary:
    is_text_key<T>        — T is convertible to std::string_view
    is_binary_key<T>      — T is an integral or pointer scalar
    key_bit_width<T>      — bit width of a binary key type
    has_radix_value<T>    — T exposes ::value_type
    has_radix_children<T> — T exposes a .children member
    is_radix_node<T>      — composite: value_type + children present
    DRadixKeyCategory     — discriminant enum (text / binary / custom)
    radix_key_category<T> — maps T to DRadixKeyCategory
    has_options_type<T>   — T::options_type alias exists
    has_option_flags<T>   — T::option_flags static member exists
    is_radix_tree_base<T> — composite container interface check
    enable_if_text_key<T>   — SFINAE guard for text keys
    enable_if_binary_key<T> — SFINAE guard for binary keys
*/

#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace djinterp
{

// --------------------------------------------------------------------------
// void_t — maps any sequence of types to void (SFINAE utility).
// --------------------------------------------------------------------------

// void_t
//   type: maps any sequence of types to void for SFINAE detection.
template<typename...>
using void_t = void;

// --------------------------------------------------------------------------
// Text key detection
// --------------------------------------------------------------------------

namespace internal
{

// is_text_key_helper
//   trait: primary template (failure case) for text key detection.
template<typename _T,
         typename = void>
struct is_text_key_helper
{
    static constexpr bool value = false;
};

// is_text_key_helper (success case)
//   trait: specialization when _T is constructible as std::string_view.
template<typename _T>
struct is_text_key_helper<
    _T,
    void_t<decltype(std::string_view(std::declval<const _T&>()))>>
{
    static constexpr bool value = true;
};

}   // internal

// is_text_key
//   trait: detects key types valid for text radix tree traversal.
//   A type qualifies when it is implicitly convertible to
//   std::string_view (e.g. std::string, const char*, string literals).
template<typename _T>
struct is_text_key
    : std::bool_constant<internal::is_text_key_helper<_T>::value>
{
};

// is_text_key_v
//   value: convenience alias for is_text_key<_T>::value.
template<typename _T>
inline constexpr bool is_text_key_v = is_text_key<_T>::value;

// --------------------------------------------------------------------------
// Binary key detection
// --------------------------------------------------------------------------

// is_binary_key
//   trait: detects key types valid for binary (bitwise) radix tree
//   traversal. A type qualifies when it is a trivially-copyable integral
//   or pointer scalar on which bitwise shift and AND are defined.
template<typename _T>
struct is_binary_key
    : std::bool_constant<
        ( ( std::is_integral<_T>::value ||
            std::is_pointer<_T>::value ) &&
          std::is_trivially_copyable<_T>::value )>
{
};

// is_binary_key_v
//   value: convenience alias for is_binary_key<_T>::value.
template<typename _T>
inline constexpr bool is_binary_key_v = is_binary_key<_T>::value;

// --------------------------------------------------------------------------
// Key bit width
// --------------------------------------------------------------------------

namespace internal
{

// key_bit_width_helper
//   trait: primary template (failure case) for bit-width deduction.
template<typename _T,
         typename = void>
struct key_bit_width_helper
{
    static constexpr std::size_t value = 0u;
};

// key_bit_width_helper (success case)
//   trait: specialization for integral and pointer binary key types.
template<typename _T>
struct key_bit_width_helper<
    _T,
    typename std::enable_if<is_binary_key<_T>::value>::type>
{
    static constexpr std::size_t value = sizeof(_T) * 8u;
};

}   // internal

// key_bit_width
//   trait: yields the bit width of a binary key type _T.
//   Yields 0 for non-binary-key types.
template<typename _T>
struct key_bit_width
{
    static constexpr std::size_t value =
        internal::key_bit_width_helper<_T>::value;
};

// key_bit_width_v
//   value: convenience alias for key_bit_width<_T>::value.
template<typename _T>
inline constexpr std::size_t key_bit_width_v = key_bit_width<_T>::value;

// --------------------------------------------------------------------------
// Node shape detection
// --------------------------------------------------------------------------

namespace internal
{

// has_radix_value_helper
//   trait: primary template (failure case) for value_type detection.
template<typename _T,
         typename = void>
struct has_radix_value_helper
{
    static constexpr bool value = false;
};

// has_radix_value_helper (success case)
//   trait: specialization when _T exposes a ::value_type member alias.
template<typename _T>
struct has_radix_value_helper<
    _T,
    void_t<typename _T::value_type>>
{
    static constexpr bool value = true;
};

}   // internal

// has_radix_value
//   trait: detects node-like types that expose a ::value_type member
//   alias, which is required by the container interface.
template<typename _T>
struct has_radix_value
    : std::bool_constant<internal::has_radix_value_helper<_T>::value>
{
};

// has_radix_value_v
//   value: convenience alias for has_radix_value<_T>::value.
template<typename _T>
inline constexpr bool has_radix_value_v = has_radix_value<_T>::value;

namespace internal
{

// has_radix_children_helper
//   trait: primary template (failure case) for .children detection.
template<typename _T,
         typename = void>
struct has_radix_children_helper
{
    static constexpr bool value = false;
};

// has_radix_children_helper (success case)
//   trait: specialization when _T exposes a .children data member.
template<typename _T>
struct has_radix_children_helper<
    _T,
    void_t<decltype(std::declval<_T>().children)>>
{
    static constexpr bool value = true;
};

}   // internal

// has_radix_children
//   trait: detects node-like types that expose a .children array or
//   equivalent member, used to navigate to descendant nodes.
template<typename _T>
struct has_radix_children
    : std::bool_constant<internal::has_radix_children_helper<_T>::value>
{
};

// has_radix_children_v
//   value: convenience alias for has_radix_children<_T>::value.
template<typename _T>
inline constexpr bool has_radix_children_v = has_radix_children<_T>::value;

// is_radix_node
//   trait: composite — detects a valid radix tree node shape.
//   A type satisfies this trait when it exposes both ::value_type and
//   a .children member (text or binary child array).
template<typename _T>
struct is_radix_node
    : std::bool_constant<
        ( has_radix_value<_T>::value    &&
          has_radix_children<_T>::value )>
{
};

// is_radix_node_v
//   value: convenience alias for is_radix_node<_T>::value.
template<typename _T>
inline constexpr bool is_radix_node_v = is_radix_node<_T>::value;

// --------------------------------------------------------------------------
// Key category discriminant
// --------------------------------------------------------------------------

// DRadixKeyCategory
//   enum: discriminant between the two radix tree specializations.
//   Used by radix_key_category<T> and runtime dispatch helpers.
enum class DRadixKeyCategory : unsigned
{
    text   = 0u,   // character-indexed (std::string_view keys)
    binary = 1u,   // bit-indexed (integral / pointer keys)
    custom = 2u    // user-supplied radix extractor (reserved)
};

namespace internal
{

// radix_key_category_helper
//   trait: primary template, maps unknown types to custom.
template<typename _T,
         typename = void>
struct radix_key_category_helper
{
    static constexpr DRadixKeyCategory value = DRadixKeyCategory::custom;
};

// radix_key_category_helper (text case)
//   trait: specialization when _T is a valid text radix key.
template<typename _T>
struct radix_key_category_helper<
    _T,
    typename std::enable_if<is_text_key<_T>::value>::type>
{
    static constexpr DRadixKeyCategory value = DRadixKeyCategory::text;
};

// radix_key_category_helper (binary case)
//   trait: specialization when _T is a binary key and not a text key.
//   Integer types that are also text-convertible prefer text.
template<typename _T>
struct radix_key_category_helper<
    _T,
    typename std::enable_if<
        (  is_binary_key<_T>::value &&
          !is_text_key<_T>::value   )>::type>
{
    static constexpr DRadixKeyCategory value = DRadixKeyCategory::binary;
};

}   // internal

// radix_key_category
//   trait: maps key type _T to its DRadixKeyCategory discriminant.
//   Text-convertible types take the text branch; raw integral/pointer
//   types (not text-convertible) take the binary branch.
template<typename _T>
struct radix_key_category
{
    static constexpr DRadixKeyCategory value =
        internal::radix_key_category_helper<_T>::value;
};

// radix_key_category_v
//   value: convenience alias for radix_key_category<_T>::value.
template<typename _T>
inline constexpr DRadixKeyCategory radix_key_category_v =
    radix_key_category<_T>::value;

// --------------------------------------------------------------------------
// Container option interface detection
// --------------------------------------------------------------------------

namespace internal
{

// has_options_type_helper
//   trait: primary template (failure case) for ::options_type detection.
template<typename _T,
         typename = void>
struct has_options_type_helper
{
    static constexpr bool value = false;
};

// has_options_type_helper (success case)
//   trait: specialization when _T exposes a ::options_type member alias.
template<typename _T>
struct has_options_type_helper<
    _T,
    void_t<typename _T::options_type>>
{
    static constexpr bool value = true;
};

}   // internal

// has_options_type
//   trait: detects T::options_type alias, required by the djinterp
//   option system for CLI trait detection and option resolution.
template<typename _T>
struct has_options_type
    : std::bool_constant<internal::has_options_type_helper<_T>::value>
{
};

// has_options_type_v
//   value: convenience alias for has_options_type<_T>::value.
template<typename _T>
inline constexpr bool has_options_type_v = has_options_type<_T>::value;

namespace internal
{

// has_option_flags_helper
//   trait: primary template (failure case) for ::option_flags detection.
template<typename _T,
         typename = void>
struct has_option_flags_helper
{
    static constexpr bool value = false;
};

// has_option_flags_helper (success case)
//   trait: specialization when _T exposes a ::option_flags static member.
template<typename _T>
struct has_option_flags_helper<
    _T,
    void_t<decltype(_T::option_flags)>>
{
    static constexpr bool value = true;
};

}   // internal

// has_option_flags
//   trait: detects T::option_flags static member, required by the
//   djinterp option system and cli_traits.hpp detection machinery.
template<typename _T>
struct has_option_flags
    : std::bool_constant<internal::has_option_flags_helper<_T>::value>
{
};

// has_option_flags_v
//   value: convenience alias for has_option_flags<_T>::value.
template<typename _T>
inline constexpr bool has_option_flags_v = has_option_flags<_T>::value;

// is_radix_tree_base
//   trait: composite — detects a type satisfying the radix tree
//   container interface. Requires: options_type alias, option_flags
//   static member, and value_type alias (via has_radix_value).
template<typename _T>
struct is_radix_tree_base
    : std::bool_constant<
        ( has_options_type<_T>::value &&
          has_option_flags<_T>::value &&
          has_radix_value<_T>::value  )>
{
};

// is_radix_tree_base_v
//   value: convenience alias for is_radix_tree_base<_T>::value.
template<typename _T>
inline constexpr bool is_radix_tree_base_v = is_radix_tree_base<_T>::value;

// --------------------------------------------------------------------------
// SFINAE guards
// --------------------------------------------------------------------------

// enable_if_text_key
//   type: SFINAE guard that enables a template only when _T is a valid
//   text radix key (convertible to std::string_view).
template<typename _T>
using enable_if_text_key =
    typename std::enable_if<is_text_key<_T>::value>::type;

// enable_if_binary_key
//   type: SFINAE guard that enables a template only when _T is a valid
//   binary radix key (integral or pointer scalar).
template<typename _T>
using enable_if_binary_key =
    typename std::enable_if<is_binary_key<_T>::value>::type;

}   // djinterp
