/*******************************************************************************
* djinterp [djinterp]                                               djinterp.hpp
*
* C++ core header for the djinterp framework.
*   Extends the C core with the namespace macros and the C++ qualifier kit,
* each spelled for the language level in use, from ISO C++98 to C++23. The
* type utilities it once held (void_t, clean, repeat_type, resolve_self, ...)
* are in core/meta/type_utility.hpp.
*
*
* path:      /inc/djinterp/cpp/djinterp.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.11.12
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  C++ KEYWORDS & NAMESPACE MACROS
    -------------------------------
    1.  C++ keywords
         1.  D_KEYWORD_CPP
         2.  D_KEYWORD_PARADIGM
         3.  D_KEYWORD_STL
         4.  D_KEYWORD_TRAITS
         5.  D_KEYWORD_CONCEPTS
         6.  D_KEYWORD_CONTAINER
         7.  D_KEYWORD_EXCEPTION
    2.  Namespace macros
         1.  D_NAMESPACE
         2.  NS_END
         3.  NS_CONCEPTS
         4.  NS_CONTAINER
         5.  NS_DATABASE
         6.  NS_DJINTERP
         7.  NS_ERROR
         8.  NS_EXCEPTION
         9.  NS_INTERNAL
         10. NS_MATH
         11. NS_MESSAGE
         12. NS_PARADIGM
         13. NS_TEST
         14. NS_TESTING
         15. NS_TRAITS
2.  QUALIFIER SUPPORT
    -----------------
    1.  Constexpr family
         1.  D_CONSTEXPR
         2.  D_STATIC_CONSTEXPR
         3.  D_CONSTEXPR_INLINE
         4.  D_STATIC_CONSTEXPR_INLINE
         5.  D_CONSTEXPR_CPP14
         6.  D_CONSTEXPR_CPP17
         7.  D_CONSTEXPR_CPP20
         8.  D_CONSTEXPR_REQ
    2.  Variable qualifiers
         1.  D_INLINE_VAR
         2.  D_CONSTEXPR_INLINE_VAR
         3.  D_CONSTEXPR_VAR
    3.  Template-parameter constraints
         1.  D_CONCEPT_PARAM
    4.  C++11 feature spellings
         1.  D_NOEXCEPT
         2.  D_NOEXCEPT_IF
         3.  D_MOVE_ENABLED
         4.  D_EXPLICIT_BOOL
         5.  D_DELETED_FN
         6.  D_DELETED_FN_MSG
         7.  D_NULLPTR
         8.  D_OVERRIDE
3.  NAMESPACE ALIASES
    -----------------
    1.  Framework aliases
         1.  functional
*/

#ifndef DJINTERP_CPP
#define DJINTERP_CPP 1

// djinterp
#include "../c/djinterp.h"  // framework root
#include "../env/env.h"     // D_ENV_LANG_*, read by the floor and the kit

//   THE LANGUAGE FLOOR is C++98 (decision 3.6, with levels above it): this
// header and everything it includes compile at every level from ISO strict
// C++98 (-DD_CFG_ENV_ISO_STRICT=1) to C++23, and include no standard C++
// header. The kit below is the authority on what each level spells; a
// facility that needs a later level is gated on the env layer's
// D_ENV_LANG_IS_CPPnn_OR_HIGHER and absent below it -- never an error, and
// never a second meaning for the same name. The env layer is read rather than
// __cplusplus, so the level compared is the one env detection settled on,
// MSVC included, whose __cplusplus stays at 199711L unless /Zc:__cplusplus is
// given.
//   What it still refuses is C: the C framework's root is c/djinterp.h.
#if !D_ENV_LANG_USING_CPP
    #error "djinterp.hpp is C++; C code includes djinterp/c/djinterp.h."
#endif

// djinterp
#include "../config/cfg_qualifiers.h"     // qualifier configuration
#include "../env/cpp/env_cpp_features.h"  // D_ENV_CPP_FEATURE_*


// 1.  C++ KEYWORDS & NAMESPACE MACROS and 2.  QUALIFIER SUPPORT live in
// djinterp_cpp.h, which C headers can include too: no C header includes C++.
#include "../c/djinterp_cpp.h"


//==============================================================================
// 3.  NAMESPACE ALIASES
//==============================================================================
// The type utilities that used to follow here (void_t, abs_value, clean,
// constexpr_swap, repeat_type, self, resolve_self) are metaprogramming, and
// live in core/meta/type_utility.hpp (decision 4.4); the root keeps the macro
// kit and the namespace vocabulary.


NS_DJINTERP

// 3.1    Framework aliases
//------------------------------------------------------------------------------
// 3.1.1
// functional
//   alias: `functional` as a second name for the framework namespace,
// because the core/functional and core/event headers refer to their own
// entities as functional::result and so on. Declared inside djinterp, so
// it adds no name to the global namespace.
namespace functional = ::djinterp;


NS_END  // djinterp


//   TRANSITIONAL. Every unit that uses the type utilities reached them
// through this header until they moved, so the root still includes their new
// home, at its END: type_utility.hpp includes this header first and needs the
// kit above, so a unit may include either header first. Once every such unit
// includes core/meta/type_utility.hpp itself, this include goes, and the root
// no longer depends on core/ at all (results_root.md lists the units).
#include "./meta/type_utility.hpp"  // void_t, clean, resolve_self, ...


#endif  // DJINTERP_CPP
