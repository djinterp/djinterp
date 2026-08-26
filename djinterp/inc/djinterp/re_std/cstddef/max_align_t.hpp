/******************************************************************************
* djinterp [re_std]                                             max_align_t.hpp
*
* max_align_t typedef:
*   A POD type whose alignment requirement is at least as strict as every
* scalar type's -- the alignment ::operator new is required to hand back.
* It is an implementation-supplied type, so the primary path re-exports
* std::max_align_t and preserves identity.
*
*   THE FALLBACK IS A DEGRADATION, AND IS LABELLED AS ONE:
*   std::max_align_t landed in <cstddef> late on some toolchains (GCC's
* libstdc++ before 4.9 shipped only the global ::max_align_t, and some
* freestanding libraries ship neither). Rather than hard-error there,
* re_std synthesises a union of the widest scalars. That union is
* correct on every ABI the authors are aware of, but it is a
* CONSTRUCTION rather than the implementation's own answer: on a target
* with an over-aligned vector scalar it could be less strict than the
* real max_align_t. Code that must be certain should static_assert on
* alignof(max_align_t) for its own platform.
*
*   Because the fallback is not the same type as std::max_align_t,
* identity is preserved only on the primary path -- which is why the
* coverage entry records this symbol as a re-export with a documented
* degraded tier rather than as a clean one.
*
*   C++11 FLOOR: the type does not exist in C++98.
*
*
* path:      /inc/djinterp/re_std/cstddef/max_align_t.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CSTDDEF_MAX_ALIGN_T_
#define DJINTERP_RE_STD_CSTDDEF_MAX_ALIGN_T_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
//   permitted: fundamental types only.
#include <cstddef>


// D_RE_STD_HAS_STD_MAX_ALIGN_T
//   constant: 1 if std::max_align_t is declared by <cstddef>.
#ifndef D_RE_STD_HAS_STD_MAX_ALIGN_T
    #if ( defined(D_ENV_COMPILER_GCC) &&                                      \
          !D_ENV_COMPILER_VERSION_AT_LEAST(4, 9, 0) )
        #define D_RE_STD_HAS_STD_MAX_ALIGN_T  0
    #else
        #define D_RE_STD_HAS_STD_MAX_ALIGN_T  1
    #endif
#endif


NS_RESTD

#if D_RE_STD_HAS_STD_MAX_ALIGN_T

    // max_align_t
    //   typedef: identity-preserving re-export of std::max_align_t.
    using ::std::max_align_t;

#else

    NS_INTERNAL

        // max_align_union
        //   union: stand-in for a missing std::max_align_t. Every member is
        // a widest-in-its-family scalar, so the union's alignment is the
        // strictest of the four. Members are never read -- only the
        // union's alignment and size matter.
        union max_align_union
        {
            long double     m_ld;
            long long       m_ll;
            void*           m_p;
            void (*m_pf)();
        };

    NS_END  // internal

    // max_align_t
    //   typedef: degraded stand-in used where <cstddef> declares no
    // std::max_align_t. See the header comment -- this is not the
    // implementation's own type and identity is not preserved.
    typedef internal::max_align_union max_align_t;

#endif  // D_RE_STD_HAS_STD_MAX_ALIGN_T

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CSTDDEF_MAX_ALIGN_T_
