/*******************************************************************************
* djinterp [re_std]                                            aligned_union.hpp
*
* aligned_union trait:
*   Yields `type` as a POD type suitable for use as uninitialized storage
* for an object of any of Types..., with size at least Len bytes (or
* the largest sizeof(Types...), whichever is greater) and alignment at
* least the maximum of alignof(Types...).
*
*   Also exposes `alignment_value` as a static constexpr std::size_t,
* equal to that maximum alignment.
*
*   STANDARD STATUS:
*   Introduced in C++11. Deprecated in C++23 (P1413R3), same rationale
* as aligned_storage. re_std retains the trait on all C++11+ tiers per
* project policy. No [[deprecated]] attribute by default.
*
*   _TYPES... MUST BE NON-EMPTY:
*   The standard requires at least one type in the pack. Calling
* aligned_union<Len> (no types) is ill-formed; this implementation
* triggers a hard error at the alignment computation (the internal
* pack_max helper has no specialization for an empty pack). A
* static_assert with a clearer message could be added, but the natural
* error reaches the user before they get far.
*
*   PORTABILITY:
*   Available on C++11 and later (requires alignas, alignof, variadic
* templates). C++98/03 omits the trait.
*
*   DEPENDENCIES:
*   <cstddef> for std::size_t. No re_std traits required (the pack_max
* helper is self-contained).
*
*
* path:      /inc/re_std/type_traits/aligned_union.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_ALIGNED_UNION_HPP
#define RE_STD_TYPE_TRAITS_ALIGNED_UNION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t


namespace re_std
{


    namespace internal
    {

        // pack_max
        //   trait: compile-time maximum of a non-empty pack of
        //          std::size_t values. Recursive structure: a 1-element
        //          base case and a 2+-element step that picks the
        //          larger of the first two and recurses on the rest.
        //          The primary template is intentionally undefined --
        //          calling pack_max<> (empty pack) is a hard error
        //          flagging an ill-formed aligned_union with no types.
        template<std::size_t...>
        struct pack_max;

        // pack_max<N>
        //   trait: 1-element base case.
        template<std::size_t N>
        struct pack_max<N>
        {
            static const std::size_t value = N;
        };

        // pack_max<A, B, Rest...>
        //   trait: 2+-element step. Picks the larger of A and B,
        //          recurses on (max, Rest...).
        template<std::size_t A,
                 std::size_t B,
                 std::size_t... Rest>
        struct pack_max<A, B, Rest...>
            : pack_max<( A > B ? A : B ), Rest...>
        {};

    }  // internal


    // aligned_union
    //   trait: yields `type` as a POD struct suitable for storage of
    //          any of Types..., with size >= max(Len, sizeof(Types)...)
    //          and alignment >= max(alignof(Types)...). Also exposes
    //          `alignment_value` as the maximum alignment.
    template<std::size_t Len,
             typename... Types>
    struct aligned_union
    {
        // alignment_value
        //   constant: maximum alignment among Types. Declared
        //             `static const` (not `static constexpr`) for
        //             reliability across the C++11+ matrix -- the
        //             integral-type-with-constant-initializer rule has
        //             worked since C++98, so no compiler in our gate
        //             will reject this even if its constexpr support
        //             is incomplete. Matches the std spec's wording
        //             for the C++11 form of the trait.
        static const std::size_t alignment_value
            = internal::pack_max<alignof(Types)...>::value;

        // type
        //   struct: the storage type. Sized to the largest of Len and
        //           any sizeof(Types...), aligned to alignment_value.
        struct type
        {
            alignas( internal::pack_max<alignof(Types)...>::value )
            unsigned char m_data[
                internal::pack_max<Len, sizeof(Types)...>::value ];
        };
    };


    // aligned_union_t
    //   alias: convenience alias yielding aligned_union<...>::type
    //          directly. Available wherever alias templates are.
    #if RE_STD_LANG_HAS_ALIAS_TEMPLATES
        template<std::size_t Len,
                 typename... Types>
        using aligned_union_t = typename aligned_union<Len, Types...>::type;
    #endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_TYPE_TRAITS_ALIGNED_UNION_HPP
