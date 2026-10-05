/*******************************************************************************
* djinterp [re_std]                                                    range.hpp
*
* range concept-trait header:
*   Provides the C++20 range concept as a SFINAE-detection trait.
* range<T>::value is true iff re_std::begin and re_std::end are both
* well-formed when applied to an lvalue of type T.
*
*   PORTABILITY:
*   - C++11+: real trait via void_t partial-spec SFINAE on the
*     already-shipped iterator_t / sentinel_t aliases.
*   - C++14+: variable-template alias range_v<T> is also defined.
*   - C++98/03: the underlying iterator_t/sentinel_t require alias
*     templates + decltype, so range itself is unavailable; the
*     header is empty on those tiers.
*
*   NAMING:
*   Matches the C++20 concept name std::ranges::range. On a future
* C++20-enabled re_std build the trait struct will be replaced by an
* actual `concept range = ...` declaration in the same name slot;
* until then user code should prefer the portable variable spelling
* range_v<T> (C++14+) over the trait form range<T>::value.
*
*
* path:      /inc/re_std/ranges/range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_RANGE_HPP
#define RE_STD_RANGES_RANGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES && \
      RE_STD_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"


namespace re_std
{


// ===========================================================================
// 0.   INTERNAL DETECTION
// ===========================================================================

namespace internal
{

// range_impl
//   trait: SFINAE-friendly inner. Specialised when the iterator_t /
// sentinel_t aliases are both well-formed for Type.
template<typename Type,
         typename = void>
struct range_impl
    : false_type
{};

template<typename Type>
struct range_impl<Type,
                  void_t<iterator_t<Type>,
                         sentinel_t<Type> > >
    : true_type
{};

}  // internal


// ===========================================================================
// I.   RANGE (concept-trait)
// ===========================================================================

// range
//   trait: true when re_std::begin and re_std::end are well-formed
// on lvalues of Type. Matches the C++20 ranges::range concept.
template<typename Type>
struct range
    : internal::range_impl<Type>
{};


// ===========================================================================
// II.  RANGE_V (variable template, C++14+)
// ===========================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

// range_v
//   variable: portable spelling of range<Type>::value.
template<typename Type>
RE_STD_CONSTEXPR bool range_v = range<Type>::value;

#endif  // variable templates


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_RANGE_HPP
