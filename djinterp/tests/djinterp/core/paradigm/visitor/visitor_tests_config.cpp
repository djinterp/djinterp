/*******************************************************************************
* djinterp [core]                                       visitor_tests_config.cpp
*
*
* path:      /tests/djinterp/core/paradigm/visitor/visitor_tests_config.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : visitor_tests_config.cpp
//   Section I: the feature gates and the default return type.
//
//   The gates are the module's contract with the dialect: each one claims a
// language feature is available, and the sections below them are compiled or not
// on that basis.  A gate that lies does not fail loudly -- it silently compiles
// out a whole dispatch strategy, or admits one the compiler cannot support.  So
// each gate is checked against the dialect DIRECTLY, from __cplusplus, rather than
// against the D_ENV_* macros it is defined in terms of, which would only prove the
// header agrees with itself.
//
//   D_VISITOR_DEFAULT_RETURN_TYPE is checked the same way: not merely that the
// macro is void, but that every default-parameterized root in the header actually
// PICKS IT UP.  void is the return type most visitors use and the one most likely
// to break a return-type-generic implementation -- acyclic_visitable's ill-formed
// non-type parameter (bug C) was precisely a void-only failure, and it lived in a
// default argument, where nobody would look.

// std
#include <string>
#include <type_traits>
// djinterp
#include "visitor_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    // dialect_is
    //   fixture: the dialect, read straight from __cplusplus rather than from the
    // D_ENV_* macros the gates are defined in terms of.  Checking a gate against
    // the macro it is defined as would be a tautology.
    D_CONSTEXPR bool dialect_is_cpp11_or_higher()
    {
        return (__cplusplus >= 201103L);
    }

    D_CONSTEXPR bool dialect_is_cpp17_or_higher()
    {
        return (__cplusplus >= 201703L);
    }

    D_CONSTEXPR bool dialect_is_cpp20_or_higher()
    {
        return (__cplusplus >= 202002L);
    }
}


/*
tests_feature_gate_variadic_templates
  Verifies D_VISITOR_HAS_VARIADIC_TEMPLATES tracks C++11.  This gate is the one
  that matters most: it guards concrete_visitor, the whole acyclic section and the
  whole static section.  If it were wrong low, three of the four dispatch
  strategies would simply vanish.
  Tests the following:
  - the gate is 1 exactly when the dialect is C++11 or higher
  - the gate is defined at all (an undefined macro would expand to 0 in #if and
    silently disable everything)
*/
bool
tests_feature_gate_variadic_templates()
{
#ifndef D_VISITOR_HAS_VARIADIC_TEMPLATES
    return false;   // not merely off -- absent
#else
    D_CONSTEXPR bool gate = (D_VISITOR_HAS_VARIADIC_TEMPLATES != 0);

    return (gate == dialect_is_cpp11_or_higher());
#endif
}

/*
tests_feature_gate_variant
  Verifies D_VISITOR_HAS_VARIANT tracks C++17.  This gate guards the entire
  variant section AND the <variant>/<tuple> includes, so it has to agree with the
  dialect in both directions.
  Tests the following:
  - the gate is 1 exactly when the dialect is C++17 or higher
*/
bool
tests_feature_gate_variant()
{
#ifndef D_VISITOR_HAS_VARIANT
    return false;
#else
    D_CONSTEXPR bool gate = (D_VISITOR_HAS_VARIANT != 0);

    return (gate == dialect_is_cpp17_or_higher());
#endif
}

/*
tests_feature_gate_concepts
  Verifies D_VISITOR_HAS_CONCEPTS tracks C++20.
  Tests the following:
  - the gate is 1 exactly when the dialect is C++20 or higher
*/
bool
tests_feature_gate_concepts()
{
#ifndef D_VISITOR_HAS_CONCEPTS
    return false;
#else
    D_CONSTEXPR bool gate = (D_VISITOR_HAS_CONCEPTS != 0);

    return (gate == dialect_is_cpp20_or_higher());
#endif
}

/*
tests_feature_gate_fold_expressions
  Verifies D_VISITOR_HAS_FOLD_EXPRESSIONS tracks C++17.  overloaded's
  `using _Fns::operator()...;` is a pack-expanded using-declaration rather than a
  fold, and visitor_for's `(... && ...)` is a fold but sits behind the CONCEPTS
  gate -- so this gate currently guards nothing.  It is still part of the module's
  advertised interface, and a consumer may branch on it, so it must not lie.
  Tests the following:
  - the gate is 1 exactly when the dialect is C++17 or higher
*/
bool
tests_feature_gate_fold_expressions()
{
#ifndef D_VISITOR_HAS_FOLD_EXPRESSIONS
    return false;
#else
    D_CONSTEXPR bool gate = (D_VISITOR_HAS_FOLD_EXPRESSIONS != 0);

    return (gate == dialect_is_cpp17_or_higher());
#endif
}

/*
tests_feature_gate_deduction_guides
  Verifies D_VISITOR_HAS_DEDUCTION_GUIDES tracks C++17.  This one DOES guard
  something observable -- the overloaded deduction guide -- and the variant
  section's CTAD spelling depends on it.
  Tests the following:
  - the gate is 1 exactly when the dialect is C++17 or higher
*/
bool
tests_feature_gate_deduction_guides()
{
#ifndef D_VISITOR_HAS_DEDUCTION_GUIDES
    return false;
#else
    D_CONSTEXPR bool gate = (D_VISITOR_HAS_DEDUCTION_GUIDES != 0);

    return (gate == dialect_is_cpp17_or_higher());
#endif
}

/*
tests_feature_gates_agree_with_each_other
  Verifies the three C++17 gates cannot disagree.  They are defined from the same
  predicate, so a divergence would mean one of them had been edited independently
  -- which is exactly how a gate drifts from the feature it names.
  Tests the following:
  - VARIANT, FOLD_EXPRESSIONS and DEDUCTION_GUIDES are all equal
  - the gates are MONOTONE: concepts implies variant implies variadic templates.
    A dialect cannot have std::variant without variadic templates, so a gate set
    that is not monotone is incoherent regardless of which entry is wrong.
*/
bool
tests_feature_gates_agree_with_each_other()
{
    D_CONSTEXPR bool variadic = (D_VISITOR_HAS_VARIADIC_TEMPLATES != 0);
    D_CONSTEXPR bool variant  = (D_VISITOR_HAS_VARIANT != 0);
    D_CONSTEXPR bool concepts = (D_VISITOR_HAS_CONCEPTS != 0);
    D_CONSTEXPR bool folds    = (D_VISITOR_HAS_FOLD_EXPRESSIONS != 0);
    D_CONSTEXPR bool guides   = (D_VISITOR_HAS_DEDUCTION_GUIDES != 0);

    bool ok = true;

    // the three C++17 gates are the same predicate
    ok = ok && (variant == folds);
    ok = ok && (variant == guides);

    // monotone: each feature implies every earlier one
    ok = ok && (!concepts || variant);
    ok = ok && (!variant  || variadic);

    return ok;
}

/*
tests_default_return_type_is_void
  Verifies the module's default return type is void.
  Tests the following:
  - D_VISITOR_DEFAULT_RETURN_TYPE is void
  - it is defined (an undefined macro here would be a hard error at every default
    argument, but a WRONGLY defined one would silently change every root's
    signature, so it is worth naming explicitly)
*/
bool
tests_default_return_type_is_void()
{
#ifndef D_VISITOR_DEFAULT_RETURN_TYPE
    return false;
#else
    static_assert(std::is_same<D_VISITOR_DEFAULT_RETURN_TYPE, void>::value,
                  "the module's default return type is void");

    return std::is_same<D_VISITOR_DEFAULT_RETURN_TYPE, void>::value;
#endif
}

/*
tests_default_return_type_propagates_to_every_root
  Verifies every default-parameterized root actually PICKS UP the default -- which
  is a different claim from the macro being void, and the one that matters.  Each
  root spells the default independently, so one of them can drift (or, as with
  acyclic_visitable's non-type parameter, be ill-formed for exactly this value)
  without any of the others noticing.
  Tests the following:
  - visitor_base<>, visitable_base<>, visitor_of<E> and acyclic_visitable<> all
    report void as their return_type
  - each defaulted spelling names the SAME type as the explicit <void> spelling,
    so the default argument is not merely present but correct
*/
bool
tests_default_return_type_propagates_to_every_root()
{
    struct element {};

    // each root's own typedef reports void...
    static_assert(std::is_same<visitor_base<>::return_type, void>::value,
                  "visitor_base<> defaults to void");
    static_assert(std::is_same<visitable_base<>::return_type, void>::value,
                  "visitable_base<> defaults to void");
    static_assert(std::is_same<visitor_of<element>::return_type, void>::value,
                  "visitor_of<E> defaults to void");
    static_assert(std::is_same<acyclic_visitable<>::return_type, void>::value,
                  "acyclic_visitable<> defaults to void");

    // ...and the defaulted spelling names the same TYPE as the explicit one.
    // acyclic_visitable is the one that matters here: its default argument used to
    // be ill-formed for precisely this return type.
    static_assert(std::is_same<visitor_base<>, visitor_base<void> >::value,
                  "visitor_base<> IS visitor_base<void>");
    static_assert(std::is_same<visitable_base<>, visitable_base<void> >::value,
                  "visitable_base<> IS visitable_base<void>");
    static_assert(std::is_same<visitor_of<element>,
                               visitor_of<element, void> >::value,
                  "visitor_of<E> IS visitor_of<E, void>");
    static_assert(std::is_same<acyclic_visitable<>,
                               acyclic_visitable<void> >::value,
                  "acyclic_visitable<> IS acyclic_visitable<void>");

    return true;
}


NS_END  // testing
NS_END  // djinterp
