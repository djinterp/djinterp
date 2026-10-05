/*******************************************************************************
* djinterp [core]                                          radix_tree_common.hpp
*
* Radix tree shared core  (the traits module is FOLDED IN):
*   Everything text_radix_tree and binary_radix_tree share: key traits, node
* shape traits, the option surface, the prefix utilities, the node bases, and
* the traversal frame.  radix_tree_traits.hpp is GONE -- its contents are
* sections I-IV below.
*
* ============================================================================
* WHAT CHANGED, AND WHY
* ============================================================================
*   1. THE TRAITS HEADER COULD NOT COEXIST WITH THE FRAMEWORK.  It opened
*      `namespace djinterp` and redefined void_t and clean_t there, on the
*      stated grounds of staying "standalone (no dependency on djinterp.hpp)".
*      But djinterp.hpp ALREADY defines both, in that same namespace.  Any
*      translation unit including djinterp.hpp and radix_tree_traits.hpp is
*      ill-formed:
*
*          error: conflicting declaration of template
*                 'template<class ...> using djinterp::void_t = void'
*
*      Standalone bought a header that cannot be used with the framework it is
*      part of.  Folded in; both names now come from djinterp.hpp.
*
*   2. A SECOND, PARALLEL, FRAMEWORK-WIDE OPTION SYSTEM.  DContainerOption
*      described itself as "the shared bitmask controlling mutability,
*      ordering, and storage strategy for EVERY djinterp container" -- but
*      container_options.hpp already holds that office, with nine type-level
*      axes.  The three DContainerOption axes are not merely similar to three
*      of them, they are the SAME THREE, re-encoded as bits:
*
*          writable / immutable / compile_time
*                            ==  container_lifetime{mutable_storage,
*                                                   immutable,
*                                                   constexpr_storage}
*          ordered / unordered
*                            ==  container_ordering{ordered, unordered}
*          fixed_size / dynamic_size
*                            ==  container_storage_kind{static_storage,
*                                                       dynamic_storage}
*
*      And it is not just duplication.  A bitmask over three axes CANNOT SAY
*      THE ONE THING A RADIX TREE MOST NEEDS TO SAY -- that a key maps to at
*      most one value.  That is the MULTIPLICITY axis, DContainerOption has no
*      such axis, and multiplicity is exactly the condition under which this
*      container is addressable at all (see THE SPEC).  The framework's axes
*      are now the source of truth; DContainerOption is retained as a bridge
*      (section V) so the two trees keep compiling while they are ported.
*
*   3. A POINTER KEY PASSED THE TRAIT AND THEN FAILED TO COMPILE.
*      is_binary_key admitted pointers --
*
*          ( is_integral<T> || is_pointer<T> ) && is_trivially_copyable<T>
*
*      -- and enable_if_binary_key therefore ENABLED bit_at<T*> and
*      binary_prefix_length<T*>, whose bodies do `_key >> n`. You cannot shift
*      a pointer:
*
*          error: invalid 'static_cast' from type 'long unsigned int'
*                 to type 'int*'
*
*      The trait's contract and the utilities' contract disagreed, and the
*      disagreement was a hard error at the first pointer key -- which is a
*      shame, because interning pointers is one of the things a binary radix
*      tree is FOR.  Bit work now goes through key_bits_t (section I), which
*      carries a key's bit pattern as an unsigned integer: integral keys keep
*      their bits, pointers become uintptr_t.  Pointer keys now work.
*
*   4. C++17 WAS REQUIRED FOR NO REASON.  std::bool_constant (8 uses) and
*      inline variables (27) are C++17; re_std::bool_constant is C++11+ and the
*      framework already gates inline variables.  The binary half of this
* module now builds at C++14. The TEXT half still needs re_std::string_view,
* which is genuinely C++17 -- see the note at section VI. 5. HOUSE STYLE.
* #pragma once -> include guards; raw `namespace djinterp {` -> NS_DJINTERP;
* PascalCase DContainerOption / DRadixKeyCategory -> snake_case (the old
* spellings are kept as aliases); the two D_RADIX_* object-like macros ->
* constants that respect scope.
* ============================================================================
* THE SPEC: A RADIX TREE IS THE ADDRESSABILITY AXIS, MADE LITERAL
* ============================================================================
*   Every other container has to be ASKED what a node's address is.  Here you
* do not ask, because THE KEY IS THE ADDRESS.  Not "maps to" it, not "encodes"
* it -- a node's address is the concatenation of the edge labels from the
* root,
* and that concatenation IS the key you looked it up with.  find() is resolve.
* The round trip, find(key_of(n)) == n, is not a property this container has;
* it is the container's entire reason to exist.
*
*   SEPARATION IS STRUCTURAL.  Children are indexed BY FIRST BYTE (text) or BY
* NEXT BIT (binary).  Two children of one node therefore CANNOT share a first
* byte -- not "must not", cannot: there is one slot per byte and a second
* child
* would have to occupy the same one.  The multiplicity restriction mu_1 is not
* enforced here, it is UNREPRESENTABLE to violate.  That is the strongest form
* separation can take, and it is precisely why a trie resolves in a single
* descent with no backtracking: at every node, the next byte of the key picks
* the one child that can possibly match.
*
*   common_prefix_length IS THE MEET. The longest common prefix of two keys is
* their greatest lower bound in the prefix order -- their lca.  It is not a
* string-comparison helper that happens to be useful; it is the meet,
* computed.
* text_prefix_compare's four flags (key_consumed / edge_consumed / exact /
* neither) are the four possible positions of that meet relative to the two
* words, and they are exactly the four cases a descent must distinguish.
*
*   PATH COMPRESSION PRESERVES ADDRESSES -- WHICH IS WHY IT IS ALLOWED.
* Collapsing a chain of single-child nodes into one edge with a longer label
* re-BRACKETS the concatenation of labels; it does not change the
* concatenation.  The address is the concatenation.  So compression is an
* address-preserving transformation, and that -- not the memory saving -- is
* what makes it sound.  Edge SPLITTING is its inverse, and the place you split
* is THE MEET: exactly where the shared prefix ends, which is exactly what
* common_prefix_length returns.  The two operations are the same theorem read
* in opposite directions.
*
* Contents:
*   I.    key traits            (text / binary / key_bits_t / bit width)
*   II.   key category
*   III.  node shape traits
*   IV.   container interface traits
*   V.    options              (the framework's axes; DContainerOption bridge)
*   VI.   text prefix utilities   -- THE MEET
*   VII.  binary prefix utilities -- THE MEET, bitwise
*   VIII. node types
*   IX.   traversal frame
*   X.    constants
*
*
* path:      /inc/djinterp/core/container/tree/radix/radix_tree_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.29
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_RADIX_RADIX_TREE_COMMON_HPP
#define DJINTERP_CONTAINER_TREE_RADIX_RADIX_TREE_COMMON_HPP 1

// FLOOR, FOR NOW: below C++14 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

#ifndef __cplusplus
    #error "radix_tree_common.hpp can only be used in C++ compilation mode"
#endif

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../../../djinterp.hpp"                            // void_t, clean_t
#include "../../container_options.hpp"                      // the nine axes
// re_std
#include "../../../../../re_std/cstdint/cstdint.hpp"  // re_std::uintptr_t
#include "../../../../../re_std/type_traits/bool_constant.hpp"  // portable,
                                                                // C++11+
// re_std::string_view -- basic_string_view<char>, available from C++11. This
// used to be re_std::string_view, which is C++17, and the TEXT half of this
// module was gated away below that. THE GATE IS GONE: re_std back-ports the
// view, so text keys now work everywhere binary keys do, and nothing here has
// a C++17 floor any more.
#include "../../../../../re_std/string_view/string_view_typedefs.hpp"

// D_RADIX_HAVE_TEXT_KEYS
//   retained, and now always 1, so a downstream #if on it still compiles.
// There is nothing left for it to gate.
#define D_RADIX_HAVE_TEXT_KEYS 1


NS_DJINTERP


// ===========================================================================
// I.   KEY TRAITS
// ===========================================================================
//   NOTE.  void_t and clean_t are NOT redefined here.  They come from
// djinterp.hpp, which has always had them; redefining them in this same
// namespace made the old traits header impossible to include alongside the
// framework.

#if D_RADIX_HAVE_TEXT_KEYS

NS_INTERNAL

    // is_text_key_helper
    //   trait: primary template (failure case).
    template<typename Type,
             typename = void>
    struct is_text_key_helper
    {
        static D_CONSTEXPR bool value = false;
    };

    // is_text_key_helper (success)
    //   trait: Type is constructible as a re_std::string_view.
    template<typename Type>
    struct is_text_key_helper<
        Type,
        void_t<decltype(
            re_std::string_view(std::declval<const Type&>()))>>
    {
        static D_CONSTEXPR bool value = true;
    };

NS_END  // internal

// is_text_key
//   trait: a key the TEXT tree can descend -- one convertible to a
// re_std::string_view (std::string, const char*, a literal). The label alphabet
// is then the byte, and the child slot is the byte's value.
template<typename Type>
struct is_text_key
    : re_std::bool_constant<internal::is_text_key_helper<Type>::value>
{};

#endif  // D_RADIX_HAVE_TEXT_KEYS


// is_binary_key
//   trait: a key the BINARY tree can descend -- an integral or pointer scalar,
// whose bit pattern is the label word and whose next bit is the child slot.
//
//   POINTERS ARE ADMITTED, AND NOW ACTUALLY WORK. They always passed this
// trait; what they could not do was survive bit_at(), which shifts the key.
// Bit work goes through key_bits_t (below), so a pointer key is now shifted as
// its uintptr_t image rather than as a pointer.
template<typename Type>
struct is_binary_key
    : re_std::bool_constant<
          ( ( std::is_integral<Type>::value ||
              std::is_pointer<Type>::value ) &&
            std::is_trivially_copyable<Type>::value )>
{};


// ---------------------------------------------------------------------------
//  SFINAE guards
// ---------------------------------------------------------------------------

#if D_RADIX_HAVE_TEXT_KEYS

template<typename Type>
using enable_if_text_key =
    typename std::enable_if<is_text_key<Type>::value>::type;

#endif

template<typename Type>
using enable_if_binary_key =
    typename std::enable_if<is_binary_key<Type>::value>::type;


// ---------------------------------------------------------------------------
//  key_bits_t  -- the bit carrier
// ---------------------------------------------------------------------------

NS_INTERNAL

    // key_bits_of
    //   trait: the unsigned integer that carries Type's bit pattern. An
    // integral key keeps its own bits (as unsigned, so the shift is LOGICAL
    // and a signed key's sign bit does not smear); a pointer key becomes
    // uintptr_t.
    template<typename Type,
             bool IsPointer = std::is_pointer<Type>::value>
    struct key_bits_of
    {
        using type = typename std::make_unsigned<Type>::type;
    };

    // key_bits_of<Type, true>
    //   helper: the case where `std::is_pointer<Type>::value` is true; it
    // maps to `re_std::uintptr_t`.
    template<typename Type>
    struct key_bits_of<Type, true>
    {
        using type = re_std::uintptr_t;
    };

    // bool has no make_unsigned; carry it as unsigned char.
    // key_bits_of<bool, false>
    //   trait: the `bool` case; it maps to `unsigned char`.
    template<>
    struct key_bits_of<bool, false>
    {
        using type = unsigned char;
    };

NS_END  // internal

// key_bits_t
//   type: the unsigned integer carrying Type's bit pattern. Every bitwise
// operation in this module goes through it.
template<typename Type>
using key_bits_t = typename internal::key_bits_of<clean_t<Type>>::type;

NS_INTERNAL

    // key_bits_impl
    //   TAG DISPATCH, not if-constexpr: reinterpret_cast is only well-formed
    // on a pointer, and an integral key must never see it. An overload that is
    // not selected is never instantiated, so it is never checked -- C++11.
    template<typename Key>
    D_CONSTEXPR_INLINE key_bits_t<Key>
    key_bits_impl(Key _key, std::true_type /*pointer*/) D_NOEXCEPT
    {
        return static_cast<key_bits_t<Key>>(
            reinterpret_cast<re_std::uintptr_t>(_key));
    }

    template<typename Key>
    D_CONSTEXPR_INLINE key_bits_t<Key>
    key_bits_impl(Key _key, std::false_type /*integral*/) D_NOEXCEPT
    {
        return static_cast<key_bits_t<Key>>(_key);
    }

NS_END  // internal

// key_bits
//   _key's bit pattern as an unsigned integer. A pointer becomes its uintptr_t
// image; an integral keeps its bits, UNSIGNED, so the shift below is LOGICAL
// and a signed key's sign bit cannot smear across the high labels. This is the
// ONE place a key stops being a key and becomes a word of labels.
template<typename Key,
         typename = enable_if_binary_key<Key>>
D_CONSTEXPR_INLINE key_bits_t<Key>
key_bits(
    Key _key
) D_NOEXCEPT
{
    return internal::key_bits_impl(
        _key, typename std::is_pointer<Key>::type());
}


// ---------------------------------------------------------------------------
//  key_bit_width
// ---------------------------------------------------------------------------

NS_INTERNAL

    template<typename Type,
             typename = void>
    struct key_bit_width_helper
    {
        static D_CONSTEXPR std::size_t value = 0u;
    };

    template<typename Type>
    struct key_bit_width_helper<
        Type,
        typename std::enable_if<is_binary_key<Type>::value>::type>
    {
        static D_CONSTEXPR std::size_t value = sizeof(Type) * 8u;
    };

NS_END  // internal

// key_bit_width
//   trait: the number of labels in a binary key's address -- its bit width.
// Zero for a type that is not a binary key.
template<typename Type>
struct key_bit_width
{
    static D_CONSTEXPR std::size_t value =
        internal::key_bit_width_helper<Type>::value;
};


// ===========================================================================
// II.  KEY CATEGORY
// ===========================================================================

// radix_key_category
//   enum: which of the two descent alphabets a key uses.
enum class radix_key_category : unsigned
{
    text   = 0u,   // byte-indexed  (string_view keys)
    binary = 1u,   // bit-indexed   (integral / pointer keys)
    custom = 2u    // user-supplied radix extractor (reserved)
};

// DRadixKeyCategory
//   alias: the old PascalCase spelling. Retained; prefer the snake_case name.
using DRadixKeyCategory = radix_key_category;

NS_INTERNAL

    template<typename Type,
             typename = void>
    struct radix_key_category_helper
    {
        static D_CONSTEXPR radix_key_category value =
            radix_key_category::custom;
    };

#if D_RADIX_HAVE_TEXT_KEYS

    // text wins when a key is both (an integer that is also string-convertible
    // is a string as far as this tree is concerned).
    template<typename Type>
    struct radix_key_category_helper<
        Type,
        typename std::enable_if<is_text_key<Type>::value>::type>
    {
        static D_CONSTEXPR radix_key_category value =
            radix_key_category::text;
    };

    template<typename Type>
    struct radix_key_category_helper<
        Type,
        typename std::enable_if<
            ( is_binary_key<Type>::value &&
             !is_text_key<Type>::value )>::type>
    {
        static D_CONSTEXPR radix_key_category value =
            radix_key_category::binary;
    };

#else

    template<typename Type>
    struct radix_key_category_helper<
        Type,
        typename std::enable_if<is_binary_key<Type>::value>::type>
    {
        static D_CONSTEXPR radix_key_category value =
            radix_key_category::binary;
    };

#endif  // D_RADIX_HAVE_TEXT_KEYS

NS_END  // internal

// radix_key_category_of
//   trait: maps a key type onto its descent alphabet.
template<typename Type>
struct radix_key_category_of
{
    static D_CONSTEXPR radix_key_category value =
        internal::radix_key_category_helper<Type>::value;
};


// ===========================================================================
// III. NODE SHAPE TRAITS
// ===========================================================================

NS_INTERNAL

    template<typename Type, typename = void>
    struct has_radix_value_helper
    {
        static D_CONSTEXPR bool value = false;
    };

    template<typename Type>
    struct has_radix_value_helper<
        Type, void_t<typename clean_t<Type>::value_type>>
    {
        static D_CONSTEXPR bool value = true;
    };

    template<typename Type, typename = void>
    struct has_radix_children_helper
    {
        static D_CONSTEXPR bool value = false;
    };

    template<typename Type>
    struct has_radix_children_helper<
        Type, void_t<decltype(std::declval<Type>().children)>>
    {
        static D_CONSTEXPR bool value = true;
    };

NS_END  // internal

template<typename Type>
struct has_radix_value
    : re_std::bool_constant<internal::has_radix_value_helper<Type>::value>
{};

// has_radix_children
//   trait: the node exposes the CHILD SLOT ARRAY -- which is the separation
// structure itself (one slot per label, so two children cannot collide).
template<typename Type>
struct has_radix_children
    : re_std::bool_constant<internal::has_radix_children_helper<Type>::value>
{};

// is_radix_node
//   trait: value_type + children.
template<typename Type>
struct is_radix_node
    : re_std::bool_constant<
          ( has_radix_value<Type>::value &&
            has_radix_children<Type>::value )>
{};


// ===========================================================================
// IV.  CONTAINER INTERFACE TRAITS
// ===========================================================================

NS_INTERNAL

    template<typename Type, typename = void>
    struct has_options_type_helper
    {
        static D_CONSTEXPR bool value = false;
    };

    template<typename Type>
    struct has_options_type_helper<
        Type, void_t<typename clean_t<Type>::options_type>>
    {
        static D_CONSTEXPR bool value = true;
    };

    template<typename Type, typename = void>
    struct has_option_flags_helper
    {
        static D_CONSTEXPR bool value = false;
    };

    template<typename Type>
    struct has_option_flags_helper<
        Type, void_t<decltype(clean_t<Type>::option_flags)>>
    {
        static D_CONSTEXPR bool value = true;
    };

NS_END  // internal

template<typename Type>
struct has_options_type
    : re_std::bool_constant<internal::has_options_type_helper<Type>::value>
{};

template<typename Type>
struct has_option_flags
    : re_std::bool_constant<internal::has_option_flags_helper<Type>::value>
{};

template<typename Type>
struct is_radix_tree_base
    : re_std::bool_constant<
          ( has_options_type<Type>::value &&
            has_option_flags<Type>::value &&
            has_radix_value<Type>::value )>
{};


// ---------------------------------------------------------------------------
//  _v shorthands
// ---------------------------------------------------------------------------
//   Inline variables are C++17.  The framework gates them; so do we, and the
// ::value form below C++17 is always available.  (The old header wrote 27 of
// these ungated, which is one of the reasons it could not build at C++14.)

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

#if D_RADIX_HAVE_TEXT_KEYS
    template<typename Type>
    inline constexpr bool is_text_key_v = is_text_key<Type>::value;
#endif

    template<typename Type>
    inline constexpr bool is_binary_key_v = is_binary_key<Type>::value;

    template<typename Type>
    inline constexpr std::size_t key_bit_width_v =
        key_bit_width<Type>::value;

    template<typename Type>
    inline constexpr bool has_radix_value_v = has_radix_value<Type>::value;

    template<typename Type>
    inline constexpr bool has_radix_children_v =
        has_radix_children<Type>::value;

    template<typename Type>
    inline constexpr bool is_radix_node_v = is_radix_node<Type>::value;

    template<typename Type>
    inline constexpr radix_key_category radix_key_category_v =
        radix_key_category_of<Type>::value;

    template<typename Type>
    inline constexpr bool has_options_type_v =
        has_options_type<Type>::value;

    template<typename Type>
    inline constexpr bool has_option_flags_v =
        has_option_flags<Type>::value;

    template<typename Type>
    inline constexpr bool is_radix_tree_base_v =
        is_radix_tree_base<Type>::value;

#endif  // inline variables (C++17); use the ::value form below C++17


// ===========================================================================
// V.   OPTIONS
// ===========================================================================
//   THE FRAMEWORK'S AXES ARE THE SOURCE OF TRUTH.  container_options.hpp holds
// nine of them, type-level.  DContainerOption held three of the same nine as
// bits, and called itself the option system "for every djinterp container".
// Two systems cannot both be that.
//
//   It is kept below, working, because the two trees name it 51 times and
// should be ported rather than broken.  But read what it cannot say:
//
//       MULTIPLICITY.  Whether a key may map to more than one value.
//
// A radix tree is addressable exactly when it may not -- that is mu_1, that is
// separation, that is why find(key) returns AT MOST ONE thing.  The bitmask has
// no bit for it and no room to grow one without renumbering every axis.  The
// framework's axis surface has said it all along.

// radix_axis_structure
//   a radix tree is a tree. Not configurable.
D_STATIC_CONSTEXPR container_structure radix_axis_structure =
    container_structure::hierarchical;

// radix_axis_multiplicity
//   THE axis. A key names at most one value; that is what makes the key an
// ADDRESS rather than a query, and it is the thing DContainerOption could not
// express. Not configurable either -- a radix tree that allowed duplicate keys
// would not resolve, and would not be a radix tree.
D_STATIC_CONSTEXPR container_multiplicity radix_axis_multiplicity =
    container_multiplicity::unique;


// ---------------------------------------------------------------------------
//  DContainerOption  (bridge -- retained, deprecated)
// ---------------------------------------------------------------------------

// container_option
//   enum: the legacy bitmask. Each axis maps ONTO the framework's, exactly:
//
//       writable / immutable / compile_time
//           -> container_lifetime{mutable_storage, immutable,
//                                 constexpr_storage}
//       ordered / unordered
//           -> container_ordering{ordered, unordered}
//       fixed_size / dynamic_size
//           -> container_storage_kind{static_storage, dynamic_storage}
//
//   Prefer the framework axes. The conversions below are the migration path.
enum class container_option : unsigned
{
    none         = 0x00u,

    // mutability axis (bits 0-2)   -> container_lifetime
    writable     = 0x01u,
    immutable    = 0x02u,
    compile_time = 0x04u,

    // ordering axis (bits 3-4)     -> container_ordering
    ordered      = 0x08u,
    unordered    = 0x10u,

    // storage axis (bits 5-6)      -> container_storage_kind
    fixed_size   = 0x20u,
    dynamic_size = 0x40u
};

// DContainerOption
//   alias: the old PascalCase spelling. Retained so the trees compile.
using DContainerOption = container_option;


// --- bitwise operators ---

D_CONSTEXPR_INLINE container_option
operator|(container_option _lhs, container_option _rhs) D_NOEXCEPT
{
    return static_cast<container_option>(
        static_cast<unsigned>(_lhs) | static_cast<unsigned>(_rhs));
}

D_CONSTEXPR_INLINE container_option
operator&(container_option _lhs, container_option _rhs) D_NOEXCEPT
{
    return static_cast<container_option>(
        static_cast<unsigned>(_lhs) & static_cast<unsigned>(_rhs));
}

D_CONSTEXPR_INLINE container_option
operator^(container_option _lhs, container_option _rhs) D_NOEXCEPT
{
    return static_cast<container_option>(
        static_cast<unsigned>(_lhs) ^ static_cast<unsigned>(_rhs));
}

D_CONSTEXPR_INLINE container_option
operator~(container_option _v) D_NOEXCEPT
{
    return static_cast<container_option>(~static_cast<unsigned>(_v));
}

inline container_option&
operator|=(container_option& _lhs, container_option _rhs) D_NOEXCEPT
{
    _lhs = (_lhs | _rhs);

    return _lhs;
}

inline container_option&
operator&=(container_option& _lhs, container_option _rhs) D_NOEXCEPT
{
    _lhs = (_lhs & _rhs);

    return _lhs;
}


// --- axis masks ---
//   These were object-like MACROS (D_CONTAINER_OPTION_*_MASK), which do not
// respect scope and cannot be qualified. They are constants now.

D_STATIC_CONSTEXPR container_option container_option_mutability_mask =
    static_cast<container_option>(
        static_cast<unsigned>(container_option::writable)  |
        static_cast<unsigned>(container_option::immutable) |
        static_cast<unsigned>(container_option::compile_time));

D_STATIC_CONSTEXPR container_option container_option_ordering_mask =
    static_cast<container_option>(
        static_cast<unsigned>(container_option::ordered) |
        static_cast<unsigned>(container_option::unordered));

D_STATIC_CONSTEXPR container_option container_option_storage_mask =
    static_cast<container_option>(
        static_cast<unsigned>(container_option::fixed_size) |
        static_cast<unsigned>(container_option::dynamic_size));

// the old macro spellings, retained.
#define D_CONTAINER_OPTION_MUTABILITY_MASK \
    (::djinterp::container_option_mutability_mask)
#define D_CONTAINER_OPTION_ORDERING_MASK \
    (::djinterp::container_option_ordering_mask)
#define D_CONTAINER_OPTION_STORAGE_MASK \
    (::djinterp::container_option_storage_mask)


// --- axis extraction ---

D_CONSTEXPR_INLINE container_option
container_option_mutability(container_option _flags) D_NOEXCEPT
{
    return (_flags & container_option_mutability_mask);
}

D_CONSTEXPR_INLINE container_option
container_option_ordering(container_option _flags) D_NOEXCEPT
{
    return (_flags & container_option_ordering_mask);
}

D_CONSTEXPR_INLINE container_option
container_option_storage(container_option _flags) D_NOEXCEPT
{
    return (_flags & container_option_storage_mask);
}

D_CONSTEXPR_INLINE bool
container_option_has(
    container_option _flags,
    container_option _bit
) D_NOEXCEPT
{
    return ((_flags & _bit) == _bit);
}


// --- axis validation ---

NS_INTERNAL

    // popcount_constexpr
    //   Kernighan. One iteration per SET bit.
    D_CONSTEXPR_CPP14 D_INLINE std::size_t
    popcount_constexpr(unsigned _v) D_NOEXCEPT
    {
        std::size_t count = 0u;

        while (_v != 0u)
        {
            _v     = (_v & (_v - 1u));
            count += 1u;
        }

        return count;
    }

NS_END  // internal

// container_option_axis_valid
//   true when at most one bit is set within each axis. This is the bitmask's
// hand-rolled restatement of what the framework's type-level axes make
// structurally impossible: you cannot pass two lifetimes to
// options_container_base, so there is nothing to validate.
D_CONSTEXPR_CPP14 D_INLINE bool
container_option_axis_valid(container_option _flags) D_NOEXCEPT
{
    return (
        ( internal::popcount_constexpr(static_cast<unsigned>(
              container_option_mutability(_flags))) <= 1u ) &&
        ( internal::popcount_constexpr(static_cast<unsigned>(
              container_option_ordering(_flags))) <= 1u ) &&
        ( internal::popcount_constexpr(static_cast<unsigned>(
              container_option_storage(_flags))) <= 1u ) );
}

// radix_tree_option_resolve
//   fills unset axes with radix-tree defaults.
D_CONSTEXPR_INLINE container_option
radix_tree_option_resolve(
    container_option _flags,
    std::size_t      _capacity
) D_NOEXCEPT
{
    return (
        ( ( container_option_mutability(_flags) == container_option::none )
              ? container_option::writable
              : container_option_mutability(_flags) )
        |
        ( ( container_option_ordering(_flags) == container_option::none )
              ? container_option::unordered
              : container_option_ordering(_flags) )
        |
        ( ( container_option_storage(_flags) == container_option::none )
              ? ( (_capacity > 0u) ? container_option::fixed_size
                                   : container_option::dynamic_size )
              : container_option_storage(_flags) ) );
}


// --- the bridge to the framework's axes ---

// radix_lifetime_of / radix_ordering_of / radix_storage_kind_of
//   map a legacy bitmask onto the framework axis it always meant. Port through
// these, then drop the bitmask.
D_CONSTEXPR_INLINE container_lifetime
radix_lifetime_of(container_option _flags) D_NOEXCEPT
{
    return ( container_option_has(_flags, container_option::compile_time)
                 ? container_lifetime::constexpr_storage
           : container_option_has(_flags, container_option::immutable)
                 ? container_lifetime::immutable
                 : container_lifetime::mutable_storage );
}

D_CONSTEXPR_INLINE container_ordering
radix_ordering_of(container_option _flags) D_NOEXCEPT
{
    return ( container_option_has(_flags, container_option::ordered)
                 ? container_ordering::ordered
                 : container_ordering::unordered );
}

D_CONSTEXPR_INLINE container_storage_kind
radix_storage_kind_of(container_option _flags) D_NOEXCEPT
{
    return ( container_option_has(_flags, container_option::fixed_size)
                 ? container_storage_kind::static_storage
                 : container_storage_kind::dynamic_storage );
}


// ===========================================================================
// VI.  TEXT PREFIX UTILITIES  --  THE MEET
// ===========================================================================

#if D_RADIX_HAVE_TEXT_KEYS

// text_prefix_match_result
//   struct: WHERE THE MEET FALLS. The longest common prefix of a key and an
// edge label is their greatest lower bound in the prefix order, and it can sit
// in exactly four places relative to the two words -- which is why a descent
// has exactly four cases:
//
//     exact key == edge the meet is BOTH words: found it
//     key_consumed key < edge the key runs out inside the edge:
//                                         the key is a proper prefix; on
// insert
//                                         the edge SPLITS HERE
//     edge_consumed edge < key the edge runs out inside the key:
//                                         descend, carry the remainder
//     neither they diverge the meet is a proper prefix of both:
//                                         on insert the edge splits at the
// meet
//                                         and BOTH get a child
//
//   The split point is the meet. Always. That is the whole of edge splitting.
struct text_prefix_match_result
{
    // |meet| -- how many leading bytes the two words share
    std::size_t common_len;

    // the meet is the whole key  (key is a prefix of, or equal to, edge)
    bool key_consumed;

    // the meet is the whole edge (edge is a prefix of, or equal to, key)
    bool edge_consumed;

    // the meet is both  (key == edge)
    bool exact;
};

// common_prefix_length
//   THE MEET, as a length.  |lcp(a, b)| -- the greatest lower bound of the two
// keys in the prefix order.  Everything else in a radix tree is bookkeeping
// around this number: it is where you descend to, where you split, and what
// the lca of two stored keys is.
D_CONSTEXPR_CPP14 D_INLINE std::size_t
common_prefix_length(
    re_std::string_view _a,
    re_std::string_view _b
) D_NOEXCEPT
{
    std::size_t i   = 0u;
    std::size_t len = ( (_a.size() < _b.size()) ? _a.size() : _b.size() );

    while ( (i < len) && (_a[i] == _b[i]) )
    {
        ++i;
    }

    return i;
}

// text_prefix_compare
//   computes the meet of _key and _edge and reports which of the four cases
// the descent is in.
D_CONSTEXPR_CPP14 D_INLINE text_prefix_match_result
text_prefix_compare(
    re_std::string_view _key,
    re_std::string_view _edge
) D_NOEXCEPT
{
    text_prefix_match_result r = {};

    r.common_len    = common_prefix_length(_key, _edge);
    r.key_consumed  = (r.common_len == _key.size());
    r.edge_consumed = (r.common_len == _edge.size());
    r.exact         = (r.key_consumed && r.edge_consumed);

    return r;
}

#endif  // D_RADIX_HAVE_TEXT_KEYS


// ===========================================================================
// VII. BINARY PREFIX UTILITIES  --  THE MEET, BITWISE
// ===========================================================================

// bit_at
//   the label at position _pos of _key's address. Position 0 is the MOST
// significant bit, so the word reads root-first -- the order a descent
// consumes it in.
//
//   POINTER KEYS WORK NOW. The old body shifted _key directly, so a pointer --
// which is_binary_key has always admitted -- was a hard compile error at the
// first instantiation ("invalid static_cast from 'long unsigned int' to
// 'int*'"). The shift happens on key_bits(), so a pointer is shifted as its
// uintptr_t image. One name, both key kinds; the trees' call sites are
// unchanged.
template<typename Key,
         typename = enable_if_binary_key<Key>>
D_CONSTEXPR_INLINE bool
bit_at(
    Key         _key,
    std::size_t _pos
) D_NOEXCEPT
{
    return static_cast<bool>(
        ( key_bits(_key) >> ((sizeof(Key) * 8u) - 1u - _pos) ) &
        static_cast<key_bits_t<Key>>(1));
}

// binary_prefix_length
// THE MEET, in bits. |lcp(a, b)| -- how many leading labels the two addresses
// share, scanning at most _max_bits from the MSB. Same number, same meaning,
// same job as common_prefix_length: it is where you descend to, and where you
// split.
template<typename Key,
         typename = enable_if_binary_key<Key>>
D_CONSTEXPR_INLINE std::size_t
binary_prefix_length(
    Key         _a,
    Key         _b,
    std::size_t _max_bits = (sizeof(Key) * 8u)
) D_NOEXCEPT
{
    std::size_t i = 0u;

    while ( (i < _max_bits) && (bit_at(_a, i) == bit_at(_b, i)) )
    {
        ++i;
    }

    return i;
}


// ===========================================================================
// VIII. NODE TYPES
// ===========================================================================

// radix_terminal
//   struct: the value stored where a key ENDS. A plain flag rather than
// std::optional, so the struct stays trivially default-constructible and
// constexpr-safe.
//
//   present == true is what makes a node a STORED KEY rather than a waypoint
// on the way to one. A radix tree's internal nodes are addresses that exist
// because something below them exists -- which is the ancestor-closure again,
// arrived at from the other side.
template<typename Value>
struct radix_terminal
{
    // true when a stored key terminates here
    bool present;

    // the mapped value; indeterminate when present == false
    Value value;

    D_CONSTEXPR radix_terminal() D_NOEXCEPT
        : present(false),
          value()
    {}

    D_CONSTEXPR explicit
    radix_terminal(const Value& _v)
        noexcept(std::is_nothrow_copy_constructible<Value>::value)
        : present(true),
          value(_v)
    {}

    D_CONSTEXPR explicit
    radix_terminal(Value&& _v)
        noexcept(std::is_nothrow_move_constructible<Value>::value)
        : present(true),
          value(static_cast<Value&&>(_v))
    {}
};

// radix_node_base
//   struct: CRTP base carrying what both node kinds share. Derived is the
// concrete node, which gives a typed parent pointer without a vtable.
//
//   The parent chain is how you recover a node's ADDRESS: walk to the root
// concatenating edge labels. THE ROOT CONTRIBUTES NO LABEL -- it is where the
// key begins, not a byte of it -- so the address of the root is the EMPTY key,
// and find("") is the root. That is the base of the induction, not a special
// case.
template<typename Derived>
struct radix_node_base
{
    // nullptr at the root
    Derived* parent;

    // true when a complete key terminates at this node
    bool is_terminal;

    D_CONSTEXPR radix_node_base() D_NOEXCEPT
        : parent(nullptr),
          is_terminal(false)
    {}
};


// ===========================================================================
// IX.  TRAVERSAL FRAME
// ===========================================================================

// radix_traversal_frame
//   struct: one frame of an explicit descent stack. Iterative, so a deep trie
// cannot overflow the machine stack -- and a trie over long keys IS deep.
template<typename NodePtr>
struct radix_traversal_frame
{
    NodePtr     node;
    std::size_t child_index;

    D_CONSTEXPR explicit
    radix_traversal_frame(NodePtr _n) D_NOEXCEPT
        : node(_n),
          child_index(0u)
    {}
};


// ===========================================================================
// X.   CONSTANTS
// ===========================================================================
//   These were object-like macros.  A macro does not respect namespaces, cannot
// be qualified, and will happily rewrite an unrelated identifier in a header
// included after it.  They are constants; the macro spellings are kept so the
// two trees still compile.

// radix_alpha_size
//   the number of child slots in a text node: one per byte value. THIS ARRAY
// IS THE SEPARATION STRUCTURE. One slot per label means two children of one
// node cannot share a first byte -- mu_1 is not enforced, it is
// unrepresentable to violate.
D_STATIC_CONSTEXPR std::size_t radix_alpha_size = 256u;

// radix_binary_branches
//   the number of child slots in a binary node: one per bit value. Same
// structure, alphabet of two.
D_STATIC_CONSTEXPR std::size_t radix_binary_branches = 2u;

#define D_RADIX_ALPHA_SIZE        (::djinterp::radix_alpha_size)
#define D_RADIX_BINARY_BRANCHES   (::djinterp::radix_binary_branches)


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_RADIX_RADIX_TREE_COMMON_HPP
