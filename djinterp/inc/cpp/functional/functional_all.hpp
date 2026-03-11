/******************************************************************************
* djinterp [functional]                                   functional_all.hpp
*
* Root header for the C++ functional programming module.
*   Includes all functional sub-modules providing a complete template-based
* functional programming library with SFINAE constraints.
*
* SUB-MODULES:
*   stl_functional.hpp            - backported STL utilities
*   functional.hpp                - type aliases, composition, combinators
*   functional_traits.hpp         - SFINAE type traits for callables
*   functional_algorithms.hpp     - higher-order algorithms (map, filter, etc.)
*   predicate_combinators.hpp     - predicate AND/OR/XOR/NOT combinators
*   compose.hpp                   - composition, partial application, memoize
*   pipeline.hpp                  - typed chainable pipeline
*   fn_builder.hpp                - fluent function chain builder
*   filter.hpp                    - collection filtering framework
*
*
* path:      \inc\functional\functional_all.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.19
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_ALL_HPP_
#define DJINTERP_FUNCTIONAL_ALL_HPP_ 1


// i.    STL backports and core utilities
#include "./stl_functional.hpp"

// ii.   type aliases, composition, logical combinators
#include "./functional.hpp"

// iii.  SFINAE type traits
#include "./functional_traits.hpp"

// iv.   higher-order algorithms
#include "./functional_algorithms.hpp"

// v.    predicate combinators
#include "./predicate_combinators.hpp"

// vi.   composition, partial application, memoization
#include "./compose.hpp"

// vii.  typed pipeline
#include "./pipeline.hpp"

// viii. fluent function chain builder
#include "./fn_builder.hpp"

// ix.   collection filtering framework
#include "./filter.hpp"


#endif  // DJINTERP_FUNCTIONAL_ALL_HPP_
