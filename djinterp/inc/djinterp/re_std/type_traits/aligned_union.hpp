/******************************************************************************
* djinterp [restd]                                              aligned_union.hpp
*
* aligned_union trait:
*   Yields `type` as a POD type suitable for use as uninitialized storage
* for an object of any of _Types..., with size at least _Len bytes (or
* the largest sizeof(_Types...), whichever is greater) and alignment at
* least the maximum of alignof(_Types...).
*
*   Also exposes `alignment_value` as a static constexpr std::size_t,
* equal to that maximum alignment.
*
*   STANDARD STATUS:
*   Introduced in C++11. Deprecated in C++23 (P1413R3), same rationale
* as aligned_storage. restd retains the trait on all C++11+ tiers per
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
*   <cstddef> for std::size_t. No restd traits required (the pack_max
* helper is self-contained).
*
*
* path:      /inc/djinterp/re_std/type_traits/aligned_union.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_ALIGNED_UNION_
#define DJINTERP_RESTD_TYPE_TRAITS_ALIGNED_UNION_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>  // std::size_t


NS_RESTD


    NS_INTERNAL

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

        // pack_max<_N>
        //   trait: 1-element base case.
        template<std::size_t _N>
        struct pack_max<_N>
        {
            static const std::size_t value = _N;
        };

        // pack_max<_A, _B, _Rest...>
        //   trait: 2+-element step. Picks the larger of _A and _B,
        //          recurses on (max, _Rest...).
        template<std::size_t _A,
                 std::size_t _B,
                 std::size_t... _Rest>
        struct pack_max<_A, _B, _Rest...>
            : pack_max<( _A > _B ? _A : _B ), _Rest...>
        {};

    NS_END  // internal


    // aligned_union
    //   trait: yields `type` as a POD struct suitable for storage of
    //          any of _Types..., with size >= max(_Len, sizeof(_Types)...)
    //          and alignment >= max(alignof(_Types)...). Also exposes
    //          `alignment_value` as the maximum alignment.
    template<std::size_t _Len,
             typename... _Types>
    struct aligned_union
    {
        // alignment_value
        //   constant: maximum alignment among _Types. Declared
        //             `static const` (not `static constexpr`) for
        //             reliability across the C++11+ matrix -- the
        //             integral-type-with-constant-initializer rule has
        //             worked since C++98, so no compiler in our gate
        //             will reject this even if its constexpr support
        //             is incomplete. Matches the std spec's wording
        //             for the C++11 form of the trait.
        static const std::size_t alignment_value
            = internal::pack_max<alignof(_Types)...>::value;

        // type
        //   struct: the storage type. Sized to the largest of _Len and
        //           any sizeof(_Types...), aligned to alignment_value.
        struct type
        {
            alignas( internal::pack_max<alignof(_Types)...>::value )
            unsigned char m_data[
                internal::pack_max<_Len, sizeof(_Types)...>::value ];
        };
    };


    // aligned_union_t
    //   alias: convenience alias yielding aligned_union<...>::type
    //          directly. Available wherever alias templates are.
    #if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
        template<std::size_t _Len,
                 typename... _Types>
        using aligned_union_t = typename aligned_union<_Len, _Types...>::type;
    #endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RESTD_TYPE_TRAITS_ALIGNED_UNION_
