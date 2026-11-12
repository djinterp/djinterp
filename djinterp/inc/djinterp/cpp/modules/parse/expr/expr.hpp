/*******************************************************************************
* djinterp [cpp/parse]                                                  expr.hpp
*
* Expression core v2's C++ face (design section 13): the umbrella. One
* include for the whole face, `djinterp::parse::expr`: the term and its
* nodes, the signature and mixfix descriptors, parsing and its results,
* folds and walks, rendering and the default scanner. The bridge to v1's
* term is bridge.hpp, from C++11, included on its own.
*   The face is C++98, strict, up to C++26; it links against the C core
* (c/modules/parse/expr/, with diagnostic.c and storage.c).
*
*
* path:      /inc/djinterp/cpp/modules/parse/expr/expr.hpp
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_PARSE_EXPR_HPP
#define DJINTERP_PARSE_EXPR_HPP 1

// djinterp
#include "./term.hpp"       // DTerm, DNodeView
#include "./signature.hpp"  // DSignature, DMixfix
#include "./parse.hpp"      // parse, DResult, DOptions, DDiagnostic
#include "./fold.hpp"       // fold, walk
#include "./render.hpp"     // render, render_to, DRenderOptions
#include "./scan.hpp"       // DScanner, DScanConfig


#endif  // DJINTERP_PARSE_EXPR_HPP
