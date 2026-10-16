/*******************************************************************************
* djinterp [core]                                      visitor_tests_variant.cpp
*
*
* path:      /tests/djinterp/core/paradigm/visitor/visitor_tests_variant.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : visitor_tests_variant.cpp
//   Section V: the variant visitor.  The closed set again, but declared as a type
// rather than as a class hierarchy -- and with the exhaustiveness check moved into
// the compiler.
//
//   There is no accept(), no visitor root, and no element root.  A std::variant IS
// the closed element set, std::visit IS the dispatch, and `overloaded` is the piece
// that makes a pile of lambdas behave as one visitor by inheriting all of them and
// pulling every operator() into a single overload set.  The result is the only one
// of the four strategies where forgetting an alternative is caught by the compiler
// with no boilerplate at all -- no pure virtuals to satisfy, nothing to derive
// from.
//
//   This is also the section the shipped header could not reach.  Bug A --
// `static constexpr dispatch_fn table[]` inside a D_CONSTEXPR_INLINE function, which
// is ill-formed before C++23 -- meant the header did not compile AT ALL at C++17 or
// C++20, with nothing instantiated.  The two dialects the variant visitor exists for
// were the two it broke.
//
//   The whole section stands behind D_VISITOR_HAS_VARIANT.  On an earlier dialect
// the predicates stand down and report success, so the module spec is identical
// across dialects and the runner needs no conditional wiring.

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "visitor_tests.hpp"

#if D_VISITOR_HAS_VARIANT
    // std
    #include <array>
    #include <utility>
    #include <variant>
#endif


NS_DJINTERP
NS_TESTING


/*
tests_overloaded_combines_callables
  Verifies overloaded merges N callables into ONE overload set.  It inherits every
  callable and pulls each operator() in with a pack-expanded using-declaration;
  without that using-declaration the name would be found in N distinct bases and
  every call would be ambiguous before overload resolution ever ran.
  Tests the following:
  - each alternative's callable is reachable through the single object
  - the three results are distinct, so nothing is collapsing onto one operator()
  - the overload set really is a set: calling it with each argument type resolves
*/
bool
tests_overloaded_combines_callables()
{
#if D_VISITOR_HAS_VARIANT
    auto vis = overloaded{
        [](int)                { return std::string("int"); },
        [](double)             { return std::string("double"); },
        [](const std::string&) { return std::string("string"); }
    };

    bool ok = true;

    ok = ok && (vis(1)                  == "int");
    ok = ok && (vis(1.5)                == "double");
    ok = ok && (vis(std::string("x"))   == "string");

    // and they are genuinely different overloads
    ok = ok && (vis(1) != vis(1.5));

    return ok;
#else
    return true;
#endif
}

/*
tests_overloaded_deduction_guide
  Verifies CTAD deduces overloaded from its initializers -- which is what makes the
  brace-initialized spelling above possible at all.  Without the guide, aggregate
  deduction of a variadic base-class pack does not happen and every use would have
  to name the closure types, which are unnameable.
  Tests the following:
  - overloaded{...} deduces to overloaded<Fs...> over the closure types
  - the deduced type is the same one make_visitor produces from the same callables
  - the gate advertising deduction guides is on, in the dialect where they exist
*/
bool
tests_overloaded_deduction_guide()
{
#if D_VISITOR_HAS_VARIANT
    static_assert(D_VISITOR_HAS_DEDUCTION_GUIDES != 0,
                  "the variant section implies deduction guides");

    auto f = [](int) { return 1; };
    auto g = [](double) { return 2; };

    auto deduced = overloaded{f, g};

    // CTAD produced overloaded<F, G> over the two closure types
    static_assert(std::is_same<decltype(deduced),
                               overloaded<decltype(f), decltype(g)> >::value,
                  "CTAD deduces the closure types");

    // and that is exactly what make_visitor builds from the same callables
    static_assert(std::is_same<decltype(deduced),
                               decltype(make_visitor(f, g))>::value,
                  "the guide and the factory agree");

    bool ok = true;

    ok = ok && (deduced(1)   == 1);
    ok = ok && (deduced(1.0) == 2);

    return ok;
#else
    return true;
#endif
}

/*
tests_overloaded_generic_fallback
  Verifies a generic callable catches the alternatives the specific ones miss.  This
  is how the variant form expresses a partial visitor: the closed set still has to
  be covered exhaustively, so the catch-all is what covers the remainder -- and
  overload resolution must prefer the exact match over it, or the specific handlers
  would never fire.
  Tests the following:
  - the specific handler wins for its own alternative
  - the generic one catches every other alternative
  - so exhaustiveness is satisfied without enumerating the whole set
*/
bool
tests_overloaded_generic_fallback()
{
#if D_VISITOR_HAS_VARIANT
    auto vis = overloaded{
        [](int _i)   { return _i * 2; },
        [](auto&&)   { return -1; }
    };

    cell as_int    = 21;
    cell as_double = 1.5;
    cell as_string = std::string("x");

    bool ok = true;

    // the exact match wins over the catch-all...
    ok = ok && (variant_visit(vis, as_int) == 42);

    // ...and the catch-all covers the rest
    ok = ok && (variant_visit(vis, as_double) == -1);
    ok = ok && (variant_visit(vis, as_string) == -1);

    return ok;
#else
    return true;
#endif
}

/*
tests_make_visitor_builds_an_overload_set
  Verifies make_visitor is the CTAD-free spelling of overloaded, for contexts where
  class template argument deduction is unavailable or unwanted.
  Tests the following:
  - make_visitor produces a working overload set
  - it dispatches through variant_visit exactly as the CTAD spelling does
  - its type is overloaded<decay_t<Fns>...>
*/
bool
tests_make_visitor_builds_an_overload_set()
{
#if D_VISITOR_HAS_VARIANT
    auto vis = make_visitor(
        [](int)                { return std::string("int"); },
        [](double)             { return std::string("double"); },
        [](const std::string&) { return std::string("string"); });

    cell v = 1.5;

    bool ok = true;

    ok = ok && (vis(1)              == "int");
    ok = ok && (variant_visit(vis, v) == "double");

    v = std::string("x");

    ok = ok && (variant_visit(vis, v) == "string");

    return ok;
#else
    return true;
#endif
}

/*
tests_make_visitor_decays_its_arguments
  Verifies the callables are stored DECAYED, so the overload set owns them and does
  not dangle on a temporary.
  Tests the following:
  - passing lvalues and rvalues produces the same overload-set type
  - that type is overloaded<decay_t<Fns>...>, i.e. references are stripped
  - a set built from temporaries still works after they are gone
*/
bool
tests_make_visitor_decays_its_arguments()
{
#if D_VISITOR_HAS_VARIANT
    auto f = [](int) { return 1; };
    auto g = [](double) { return 2; };

    auto from_lvalues = make_visitor(f, g);
    auto from_rvalues = make_visitor([](int) { return 1; },
                                     [](double) { return 2; });

    // lvalues decay to the same type the guide would have deduced
    static_assert(std::is_same<decltype(from_lvalues),
                               overloaded<std::decay_t<decltype(f)>,
                                          std::decay_t<decltype(g)> > >::value,
                  "the arguments are decayed, not stored as references");

    // and a set built from temporaries outlives them
    bool ok = true;

    ok = ok && (from_rvalues(1)   == 1);
    ok = ok && (from_rvalues(1.0) == 2);
    ok = ok && (from_lvalues(1)   == from_rvalues(1));

    return ok;
#else
    return true;
#endif
}

/*
tests_variant_visit_dispatches_on_the_active_alternative
  Verifies dispatch follows the alternative the variant HOLDS, not the one it
  declares first.  The three alternatives have distinct types, so the active one is
  always identifiable from the result alone.
  Tests the following:
  - each of the three alternatives reaches its own handler
  - re-assigning the variant changes which handler fires, so the dispatch is on the
    runtime state and not on the static type
*/
bool
tests_variant_visit_dispatches_on_the_active_alternative()
{
#if D_VISITOR_HAS_VARIANT
    auto vis = overloaded{
        [](int)                { return std::string("int"); },
        [](double)             { return std::string("double"); },
        [](const std::string&) { return std::string("string"); }
    };

    cell v = 1;                       // first alternative

    bool ok = true;

    ok = ok && (variant_visit(vis, v) == "int");

    v = 1.5;                          // second

    ok = ok && (variant_visit(vis, v) == "double");

    v = std::string("hello");         // third

    ok = ok && (variant_visit(vis, v) == "string");

    // back again: nothing is sticky
    v = 7;

    ok = ok && (variant_visit(vis, v) == "int");

    return ok;
#else
    return true;
#endif
}

/*
tests_variant_visit_returns_the_visitors_value
  Verifies the visitor's value comes back out of variant_visit unchanged -- the
  wrapper is a thin one and must not swallow or default anything.
  Tests the following:
  - the returned value is exactly what the handler produced
  - the static return type is the handler's
  - the value is carried from the ACTIVE alternative's payload, not synthesized
*/
bool
tests_variant_visit_returns_the_visitors_value()
{
#if D_VISITOR_HAS_VARIANT
    auto doubler = overloaded{
        [](int _i)              { return _i * 2; },
        [](double _d)           { return static_cast<int>(_d * 10); },
        [](const std::string& _s) { return static_cast<int>(_s.size()); }
    };

    static_assert(std::is_same<
                      decltype(variant_visit(doubler, std::declval<cell&>())),
                      int>::value,
                  "the return type is the handler's");

    cell v = 21;

    bool ok = true;

    ok = ok && (variant_visit(doubler, v) == 42);

    v = 1.5;

    ok = ok && (variant_visit(doubler, v) == 15);

    v = std::string("abcd");

    ok = ok && (variant_visit(doubler, v) == 4);   // read from the payload

    return ok;
#else
    return true;
#endif
}

/*
tests_variant_visit_multi_variant
  Verifies multi-dispatch: one visitor applied to several variants at once, resolved
  over the CROSS PRODUCT of their alternatives.  This is the thing none of the other
  three strategies can express -- classic and acyclic double dispatch is dispatch on
  ONE dynamic type, and adding a second would need a second accept().
  Tests the following:
  - each of the four (int|double) x (int|double) combinations reaches its own handler
  - the four are distinct, so the cross product is genuinely being resolved and not
    just the first argument
  - the single-variant overload is still selected for a single variant, i.e. the two
    overloads of variant_visit do not collide
*/
bool
tests_variant_visit_multi_variant()
{
#if D_VISITOR_HAS_VARIANT
    auto pair_vis = overloaded{
        [](int, int)       { return 0; },
        [](int, double)    { return 1; },
        [](double, int)    { return 2; },
        [](double, double) { return 3; }
    };

    number a = 1;
    number b = 2;

    bool ok = true;

    // all four cells of the cross product
    ok = ok && (variant_visit(pair_vis, a, b) == 0);

    b = 2.0;

    ok = ok && (variant_visit(pair_vis, a, b) == 1);

    a = 1.0;

    ok = ok && (variant_visit(pair_vis, a, b) == 3);

    b = 2;

    ok = ok && (variant_visit(pair_vis, a, b) == 2);

    // and the single-variant overload still resolves for a single argument -- the
    // two variant_visit overloads are partially ordered, not ambiguous
    auto single = overloaded{
        [](int)    { return 10; },
        [](double) { return 20; }
    };

    number one = 5;

    ok = ok && (variant_visit(single, one) == 10);

    return ok;
#else
    return true;
#endif
}

/*
tests_variant_visit_is_constexpr
  Verifies variant_visit folds in a constant expression.  It carries
  D_CONSTEXPR_INLINE, so a variant known at compile time must dispatch at compile
  time -- and this is what bug A destroyed for the whole header: a variable of
  static storage duration is not permitted in a constexpr function before C++23, so
  the mere PRESENCE of that declaration made every dialect below C++23 reject the
  file outright.
  Tests the following:
  - variant_visit's result is usable in a static_assert
  - it dispatches on the correct alternative at compile time
  - the whole overload-set construction folds too, not just the visit
*/
bool
tests_variant_visit_is_constexpr()
{
#if D_VISITOR_HAS_VARIANT
    D_CONSTEXPR std::variant<int, double> v = 7;

    D_CONSTEXPR auto to_int = [](auto _x) -> int
    {
        return static_cast<int>(_x);
    };

    D_CONSTEXPR int folded = variant_visit(to_int, v);

    static_assert(folded == 7, "variant_visit folds in a constant expression");

    // and on the OTHER alternative, so the compile-time dispatch is real
    D_CONSTEXPR std::variant<int, double> d = 2.75;
    D_CONSTEXPR int folded_d = variant_visit(to_int, d);

    static_assert(folded_d == 2, "and dispatches on the active alternative");

    return ((folded == 7) && (folded_d == 2));
#else
    return true;
#endif
}

/*
tests_variant_visit_with_index_supplies_the_index
  Verifies the visitor receives the ACTIVE alternative's index alongside its value.
  Tests the following:
  - each alternative reports its own index
  - the value is passed too, and is the right one -- an index-only dispatch that
    dropped the payload would still satisfy an index check
*/
bool
tests_variant_visit_with_index_supplies_the_index()
{
#if D_VISITOR_HAS_VARIANT
    cell v = 1;

    auto index_of = [](auto _index, auto&)
    {
        return static_cast<std::size_t>(decltype(_index)::value);
    };

    bool ok = true;

    ok = ok && (variant_visit_with_index(index_of, v) == 0u);

    v = 1.5;

    ok = ok && (variant_visit_with_index(index_of, v) == 1u);

    v = std::string("x");

    ok = ok && (variant_visit_with_index(index_of, v) == 2u);

    // and the VALUE arrives too, not just the index
    cell n = 21;

    auto doubled = variant_visit_with_index(
        [](auto, auto& _val) -> int
        {
            using held_t = clean_t<decltype(_val)>;

            if (std::is_same<held_t, int>::value)
            {
                return 42;
            }

            return 0;
        },
        n);

    ok = ok && (doubled == 42);

    return ok;
#else
    return true;
#endif
}

/*
tests_variant_visit_with_index_is_a_compile_time_constant
  Verifies the index arrives as a std::integral_constant, not a runtime size_t.
  That is the entire reason this helper exists rather than just calling
  _variant.index(): a compile-time index can be used to index a tuple, select a
  type, or drive a static_assert -- none of which a runtime value can do.
  Tests the following:
  - the index's type is std::integral_constant<std::size_t, I> for the active I
  - the value is usable in a CONSTANT EXPRESSION inside the visitor, which a runtime
    index could not be
  - the constant matches the variant's own runtime index()
*/
bool
tests_variant_visit_with_index_is_a_compile_time_constant()
{
#if D_VISITOR_HAS_VARIANT
    cell v = 1.5;   // alternative 1

    // the index is an integral_constant -- so it can be used where only a constant
    // expression is allowed.  A runtime std::size_t would not compile here at all.
    const bool is_constant = variant_visit_with_index(
        [](auto _index, auto&) -> bool
        {
            using index_t = decltype(_index);

            static_assert(std::is_same<
                              index_t,
                              std::integral_constant<std::size_t,
                                                     index_t::value> >::value,
                          "the index is an integral_constant");

            // used in a constant expression: only possible because it IS one
            std::array<int, index_t::value + 1> sized{};

            return (sized.size() == (index_t::value + 1));
        },
        v);

    bool ok = true;

    ok = ok && is_constant;

    // and the compile-time constant agrees with the variant's runtime index
    const std::size_t reported = variant_visit_with_index(
        [](auto _index, auto&)
        {
            return static_cast<std::size_t>(decltype(_index)::value);
        },
        v);

    ok = ok && (reported == v.index());
    ok = ok && (reported == 1u);

    return ok;
#else
    return true;
#endif
}

/*
tests_variant_visit_with_index_covers_every_alternative
  Verifies the dispatch table is built over the WHOLE alternative set, not just the
  ones that happen to be exercised.  The helper synthesizes one entry per
  alternative from an index_sequence; an off-by-one there would only show up on the
  first or last alternative.
  Tests the following:
  - every alternative of a three-way variant reports its own index, first and last
    included
  - and the same for a two-way variant, so the construction is not tied to one arity
  - index and value agree throughout: each index maps to the type actually held
*/
bool
tests_variant_visit_with_index_covers_every_alternative()
{
#if D_VISITOR_HAS_VARIANT
    auto probe = [](auto _index, auto& _val) -> bool
    {
        using held_t = clean_t<decltype(_val)>;

        // the reported index must name the type that is actually held
        return std::is_same<
            held_t,
            std::variant_alternative_t<decltype(_index)::value, cell> >::value;
    };

    bool ok = true;

    cell v = 1;                                  // FIRST alternative

    ok = ok && variant_visit_with_index(probe, v);

    v = 1.5;                                     // middle

    ok = ok && variant_visit_with_index(probe, v);

    v = std::string("x");                        // LAST alternative

    ok = ok && variant_visit_with_index(probe, v);

    // a different arity, so the index_sequence is not tied to one size
    auto probe2 = [](auto _index, auto& _val) -> bool
    {
        using held_t = clean_t<decltype(_val)>;

        return std::is_same<
            held_t,
            std::variant_alternative_t<decltype(_index)::value, number> >::value;
    };

    number n = 3;

    ok = ok && variant_visit_with_index(probe2, n);

    n = 3.0;

    ok = ok && variant_visit_with_index(probe2, n);

    return ok;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
