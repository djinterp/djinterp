/*******************************************************************************
* djinterp [test]                                              visitor_tests.hpp
*
*   Unit-test module for the djinterp visitor pattern header.  Declarations and
* shared fixtures only; each predicate is defined in the section translation unit
* that owns it, and every test is flat in djinterp::testing.
*
*   The header under test offers FOUR dispatch strategies over the same idea --
* "let an operation be added to a hierarchy without touching the hierarchy" --
* and they trade against each other along two axes: WHEN the element type is
* recovered, and WHAT happens when the visitor does not handle it.
*
*     classic   (II)   runtime, via a cross-cast to the per-type interface.
*                      CLOSED set: the visitor must handle every element, and
*                      concrete_visitor makes each visit() pure virtual to force
*                      it.  An unhandled element is a program error -> throws.
*
*     acyclic   (III)  runtime, via dynamic_cast to visitor_of<T>.
*                      OPEN set: a visitor handles what it cares about and
*                      ignores the rest.  An unhandled element returns a default.
*
*     static    (IV)   COMPILE TIME, via CRTP.  No vtable at all.  An unhandled
*                      element is not a runtime outcome -- the call does not
*                      compile.
*
*     variant   (V)    compile time, via std::visit over a closed alternative
*                      set.  Exhaustiveness is checked by the compiler.
*
*   The two runtime forms are the interesting pair, because they are so easily
* conflated: both use RTTI, both are "the visitor pattern".  The difference is
* the CONTRACT, and the suite pins it from both sides --
* tests_d_visitable_unhandled_element_throws and
* tests_d_acyclic_visitable_ignores_unhandled are the same scenario with opposite
* required outcomes, and a header that got either one backwards would still pass
* a naive "does dispatch work" test.
*
*   BUILD PREREQUISITE.  The shipped header does not compile, and this suite does
* not build against it.  Five defects, all fixed in the accompanying patch:
*
*     A.  variant_visit_with_index_impl declares `static constexpr dispatch_fn
*         table[]` inside a D_CONSTEXPR_INLINE function.  A variable of static
*         storage duration is not permitted in a constexpr function before C++23,
*         so the ENTIRE HEADER fails to compile at C++17 and C++20 -- the two
*         dialects the variant visitor exists for -- with nothing instantiated.
*
*     B.  internal::visitor_of_base's recursive case writes
*         `using visitor_of_base<R, Tail...>::visit;`, but the terminal
*         visitor_of_base<R> declares no visit() at all, so the using-declaration
*         names nothing.  concrete_visitor is a hard error for ANY element count.
*         The chain also declared its own visit() overloads and never inherited
*         visitor_of_impl, leaving D_VISITABLE's cast with no subobject to find.
*
*     C.  acyclic_visitable's second template parameter,
*         `_ReturnType _DefaultReturn = _ReturnType()`, is a NON-TYPE parameter of
*         type _ReturnType.  void is not a permitted type for one, so
*         acyclic_visitable<void> -- both the default AND the spelling the
*         header's own usage example gives -- is ill-formed.  The parameter was
*         never referenced.
*
*     D.  D_VISITABLE static_casts visitor_base<R>& to visitor_of_impl<R,This>&.
*         Those are SIBLING bases within the concrete visitor, not base and
*         derived, so the cast is between unrelated types and is ill-formed.
*         A cross-cast requires dynamic_cast.
*
*     E.  std::derived_from is used with no <concepts> include, and (after D)
*         std::bad_cast with no <typeinfo>.  Both happen to resolve through
*         transitive includes on libstdc++; neither is guaranteed to.
*
*
* path:      /tests/djinterp/core/paradigm/visitor/visitor_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.11
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_VISITOR_TESTS_
#define DJINTERP_VISITOR_TESTS_ 1

// std
#include <string>
// djinterp
#include "../../../../../inc/djinterp/core/paradigm/visitor/visitor.hpp"

#if D_VISITOR_HAS_VARIANT
    // std
    #include <variant>
#endif

#ifdef DTEST_SPEC_MODE
    #include "djinterp/test/test_defaults.hpp"
#endif


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                        SHARED FIXTURES                                   ///
///////////////////////////////////////////////////////////////////////////////

//   Fixtures used by more than one section live here.  Section-local ones stay in
// anonymous namespaces in the section that owns them, so a fixture's blast radius
// is visible from where it is declared.

// ---------------------------------------------------------------------------
// II. classic visitor -- a CLOSED shape hierarchy over a std::string sink
// ---------------------------------------------------------------------------

class classic_circle;
class classic_rect;
class classic_tri;

// classic_shape
//   fixture: the element root.  accept() is pure virtual and takes the visitor
// type-erased as visitor_base<std::string>& -- which is the whole difficulty the
// classic form has to solve, since that type declares no visit() at all.
class classic_shape : public visitable_base<std::string>
{
public:
    virtual ~classic_shape()
    {}
};

// classic_circle / classic_rect / classic_tri
//   fixture: the closed element set.  D_VISITABLE injects accept(); each element
// dispatches to the visitor's visit() overload for its own concrete type.
class classic_circle : public classic_shape
{
public:
    D_VISITABLE(std::string)

    std::string tag() const
    {
        return "circle";
    }
};

class classic_rect : public classic_shape
{
public:
    D_VISITABLE(std::string)

    std::string tag() const
    {
        return "rect";
    }
};

class classic_tri : public classic_shape
{
public:
    D_VISITABLE(std::string)

    std::string tag() const
    {
        return "tri";
    }
};

// shape_namer
//   fixture: a TOTAL visitor -- it handles every element in the closed set, which
// is what concrete_visitor's pure-virtual visit()s require of it.
class shape_namer : public concrete_visitor<std::string,
                                            classic_circle,
                                            classic_rect,
                                            classic_tri>
{
public:
    std::string visit(classic_circle& _e)
    {
        return "saw " + _e.tag();
    }

    std::string visit(classic_rect& _e)
    {
        return "saw " + _e.tag();
    }

    std::string visit(classic_tri& _e)
    {
        return "saw " + _e.tag();
    }
};

// shape_counter
//   fixture: a stateful total visitor.  Visitors accumulate across a traversal;
// that is most of the point of the pattern, so it is worth a fixture that can
// actually show it.
class shape_counter : public concrete_visitor<std::string,
                                              classic_circle,
                                              classic_rect,
                                              classic_tri>
{
public:
    int circles;
    int rects;
    int tris;

    shape_counter()
        : circles(0)
        , rects(0)
        , tris(0)
    {}

    std::string visit(classic_circle&)
    {
        ++circles;
        return "c";
    }

    std::string visit(classic_rect&)
    {
        ++rects;
        return "r";
    }

    std::string visit(classic_tri&)
    {
        ++tris;
        return "t";
    }
};

// circle_only_visitor
//   fixture: a PARTIAL visitor -- a well-formed visitor over a SMALLER closed set
// {classic_circle}.  Handing it an element outside that set violates the classic
// form's closed-set contract, and the suite requires that to be reported rather
// than silently absorbed.
class circle_only_visitor : public concrete_visitor<std::string, classic_circle>
{
public:
    std::string visit(classic_circle&)
    {
        return "only circle";
    }
};

// ---------------------------------------------------------------------------
// II. classic visitor -- the void-return case
// ---------------------------------------------------------------------------

class dot;

// dot_base
//   fixture: element root with the DEFAULT return type, i.e. void.  void is the
// return type most visitors actually use, and it is the one most likely to break
// a return-type-generic implementation, so it gets its own hierarchy.
class dot_base : public visitable_base<>
{
public:
    virtual ~dot_base()
    {}
};

class dot : public dot_base
{
public:
    int hits;

    dot()
        : hits(0)
    {}

    D_VISITABLE(void)
};

// dot_counter
//   fixture: a void-returning visitor that mutates both itself and the element.
class dot_counter : public concrete_visitor<void, dot>
{
public:
    int count;

    dot_counter()
        : count(0)
    {}

    void visit(dot& _d)
    {
        ++count;
        ++_d.hits;
    }
};

// ---------------------------------------------------------------------------
// III. acyclic visitor -- an OPEN element set
// ---------------------------------------------------------------------------

class acyclic_circle;
class acyclic_rect;
class acyclic_tri;

// acyclic_circle / acyclic_rect / acyclic_tri
//   fixture: three independent elements.  They share no element root beyond
// acyclic_visitable<> and, crucially, the VISITOR base knows nothing about them --
// that is the acyclicity.  acyclic_tri exists to be the element the partial
// visitor below does not handle.
class acyclic_circle : public acyclic_visitable<>
{
public:
    int seen;

    acyclic_circle()
        : seen(0)
    {}

    D_ACYCLIC_VISITABLE(acyclic_circle, void)
};

class acyclic_rect : public acyclic_visitable<>
{
public:
    int seen;

    acyclic_rect()
        : seen(0)
    {}

    D_ACYCLIC_VISITABLE(acyclic_rect, void)
};

class acyclic_tri : public acyclic_visitable<>
{
public:
    int seen;

    acyclic_tri()
        : seen(0)
    {}

    D_ACYCLIC_VISITABLE(acyclic_tri, void)
};

// round_visitor
//   fixture: handles circle and rect, and knows NOTHING of triangle.  Under the
// acyclic form this is legitimate, not an error -- which is exactly the contract
// the suite has to pin, since under the classic form the same shape of visitor is
// a bug.
class round_visitor : public acyclic_visitor,
                      public visitor_of<acyclic_circle>,
                      public visitor_of<acyclic_rect>
{
public:
    int circles;
    int rects;

    round_visitor()
        : circles(0)
        , rects(0)
    {}

    void visit(acyclic_circle& _e)
    {
        ++circles;
        ++_e.seen;
    }

    void visit(acyclic_rect& _e)
    {
        ++rects;
        ++_e.seen;
    }
};

// blind_visitor
//   fixture: an acyclic visitor that handles NOTHING.  Every element falls
// through to the default.
class blind_visitor : public acyclic_visitor
{};

// scored_node
//   fixture: a non-void acyclic element.  The no-match path has to produce a
// VALUE here, not just do nothing, so it exercises a different line of the macro.
class scored_node;

class scored_node : public acyclic_visitable<int>
{
public:
    D_ACYCLIC_VISITABLE(scored_node, int)
};

// scorer
//   fixture: handles scored_node and returns a distinctive value, so a default
// return is impossible to confuse with a real one.
class scorer : public acyclic_visitor,
               public visitor_of<scored_node, int>
{
public:
    int visit(scored_node&)
    {
        return 42;
    }
};

// ---------------------------------------------------------------------------
// IV. static visitor -- CRTP, no vtable
// ---------------------------------------------------------------------------

// static_circle / static_rect / static_tri
//   fixture: elements that accept static visitors.  static_tri is handled by no
// visitor here -- under this strategy that is a COMPILE-time fact, so the suite
// observes it through visit_result rather than by calling anything.
class static_circle : public static_visitable<static_circle>
{
public:
    int id;

    static_circle()
        : id(1)
    {}
};

class static_rect : public static_visitable<static_rect>
{
public:
    int id;

    static_rect()
        : id(2)
    {}
};

class static_tri : public static_visitable<static_tri>
{
public:
    int id;

    static_tri()
        : id(3)
    {}
};

// tag_visitor
//   fixture: distinguishes every path through static_visitor::apply.  The four
// overloads return four DIFFERENT strings on purpose: a visitor whose overloads
// returned the same thing would pass a dispatch test even if apply() selected the
// wrong one every time.
class tag_visitor : public static_visitor<tag_visitor>
{
public:
    std::string visit(static_circle&)
    {
        return "circle/mut";
    }

    std::string visit(static_rect&)
    {
        return "rect/mut";
    }

    std::string visit(const static_circle&)
    {
        return "circle/const-elem";
    }

    std::string visit(const static_rect&)
    {
        return "rect/const-elem";
    }
};

// const_tag_visitor
//   fixture: only const-callable.  Selects static_visitor::apply's third overload
// -- the one taking `this` by const -- and nothing else can.
class const_tag_visitor : public static_visitor<const_tag_visitor>
{
public:
    std::string visit(const static_circle&) const
    {
        return "circle/const-visitor";
    }

    std::string visit(const static_rect&) const
    {
        return "rect/const-visitor";
    }
};

// id_visitor
//   fixture: a static visitor with a non-string return type, to show the return
// type is deduced from the visitor rather than fixed by the base.
class id_visitor : public static_visitor<id_visitor>
{
public:
    int visit(static_circle& _e)
    {
        return _e.id * 10;
    }

    int visit(static_rect& _e)
    {
        return _e.id * 10;
    }
};

// ---------------------------------------------------------------------------
// V. variant visitor (C++17+)
// ---------------------------------------------------------------------------

#if D_VISITOR_HAS_VARIANT

// cell
//   fixture: a three-alternative variant.  The alternatives have distinct types
// so the active one is always identifiable from the result alone.
using cell = std::variant<int, double, std::string>;

// number
//   fixture: a two-alternative variant, used for multi-variant dispatch where the
// suite needs a small, fully enumerable cross product.
using number = std::variant<int, double>;

#endif  // D_VISITOR_HAS_VARIANT


///////////////////////////////////////////////////////////////////////////////
///                        TEST DECLARATIONS                                 ///
///////////////////////////////////////////////////////////////////////////////

// -- I. configuration & feature gates ---------------------------------------
bool tests_feature_gate_variadic_templates();
bool tests_feature_gate_variant();
bool tests_feature_gate_concepts();
bool tests_feature_gate_fold_expressions();
bool tests_feature_gate_deduction_guides();
bool tests_feature_gates_agree_with_each_other();
bool tests_default_return_type_is_void();
bool tests_default_return_type_propagates_to_every_root();

// -- II. classic visitor ----------------------------------------------------
bool tests_visitor_base_return_type_typedef();
bool tests_visitor_base_is_polymorphic();
bool tests_visitable_base_return_type_typedef();
bool tests_visitable_base_is_abstract();
bool tests_visitor_of_impl_is_abstract();
bool tests_concrete_visitor_single_element();
bool tests_concrete_visitor_multiple_elements();
bool tests_concrete_visitor_is_a_visitor_base();
bool tests_concrete_visitor_inherits_every_per_type_interface();
bool tests_concrete_visitor_is_abstract_until_all_visits_implemented();
bool tests_concrete_visitor_overload_set_is_unambiguous();
bool tests_concrete_visitor_return_type_typedef();
bool tests_d_visitable_dispatches_to_the_matching_overload();
bool tests_d_visitable_double_dispatch_through_element_base();
bool tests_d_visitable_dispatch_through_visitor_base();
bool tests_d_visitable_returns_the_visitors_value();
bool tests_d_visitable_void_return();
bool tests_d_visitable_unhandled_element_throws();
bool tests_classic_visitor_accumulates_across_a_traversal();

// -- III. acyclic visitor ---------------------------------------------------
bool tests_acyclic_visitor_is_polymorphic();
bool tests_visitor_of_is_abstract();
bool tests_visitor_of_return_type_typedef();
bool tests_visitor_of_default_return_type_is_void();
bool tests_acyclic_visitable_default_is_void();
bool tests_acyclic_visitable_return_type_typedef();
bool tests_acyclic_visitable_is_abstract();
bool tests_d_acyclic_visitable_dispatches_to_the_matching_interface();
bool tests_d_acyclic_visitable_ignores_unhandled();
bool tests_d_acyclic_visitable_ignores_everything_for_a_blind_visitor();
bool tests_d_acyclic_visitable_returns_default_when_unhandled();
bool tests_d_acyclic_visitable_non_void_return();
bool tests_acyclic_visitor_is_decoupled_from_the_element_set();
bool tests_acyclic_visitor_accumulates_across_a_traversal();

// -- IV. static visitor -----------------------------------------------------
bool tests_visit_result_deduces_the_return_type();
bool tests_visit_result_is_sfinae_friendly();
bool tests_visit_result_t_alias_agrees();
bool tests_visit_result_distinguishes_const_elements();
bool tests_static_visitor_apply_dispatches_by_type();
bool tests_static_visitor_apply_selects_const_element_overload();
bool tests_static_visitor_apply_selects_const_visitor_overload();
bool tests_static_visitor_return_type_follows_the_visitor();
bool tests_static_visitor_has_no_vtable();
bool tests_static_visitable_accept_forwards_to_apply();
bool tests_static_visitable_accept_const();
bool tests_static_visitable_accept_return_type();
bool tests_static_visitor_dispatch_is_compile_time();

// -- V. variant visitor (C++17+) --------------------------------------------
bool tests_overloaded_combines_callables();
bool tests_overloaded_deduction_guide();
bool tests_overloaded_generic_fallback();
bool tests_make_visitor_builds_an_overload_set();
bool tests_make_visitor_decays_its_arguments();
bool tests_variant_visit_dispatches_on_the_active_alternative();
bool tests_variant_visit_returns_the_visitors_value();
bool tests_variant_visit_multi_variant();
bool tests_variant_visit_is_constexpr();
bool tests_variant_visit_with_index_supplies_the_index();
bool tests_variant_visit_with_index_is_a_compile_time_constant();
bool tests_variant_visit_with_index_covers_every_alternative();

// -- VI. concepts (C++20+) --------------------------------------------------
bool tests_concept_visitable_type();
bool tests_concept_visitable_type_rejects_non_visitable();
bool tests_concept_visitor_for();
bool tests_concept_visitor_for_rejects_partial_visitor();
bool tests_concept_acyclic_visitor_for();
bool tests_concept_acyclic_visitor_for_rejects_plain_visitor();
bool tests_constrained_accept_dispatches();
bool tests_constrained_accept_is_gated_by_the_concept();


///////////////////////////////////////////////////////////////////////////////
///                        MODULE SPECIFICATION                              ///
///////////////////////////////////////////////////////////////////////////////

#ifdef DTEST_SPEC_MODE

// visitor_spec
//   function: the module specification handed to run_module.  One block per
// section of the header under test, in the header's own order.
//
//   The variant and concept blocks are NOT conditionally omitted.  Their
// predicates stand down and report success on a dialect that lacks the feature,
// so the spec -- and therefore the test count -- is identical across every
// dialect, and the runner needs no conditional wiring.
inline ::djinterp::test::module_spec
visitor_spec()
{
    namespace dt = ::djinterp::test;

    dt::module_spec spec;

    spec.name       = "visitor";
    spec.descriptor = "visitor pattern: classic, acyclic, static and variant dispatch";

    // -- I. configuration & feature gates -----------------------------------
    {
        dt::block_spec block;

        block.name       = "configuration";
        block.descriptor = "feature gates and the default return type";

        block.tests.push_back({"feature_gate_variadic_templates",
            "D_VISITOR_HAS_VARIADIC_TEMPLATES tracks C++11",
            &tests_feature_gate_variadic_templates});
        block.tests.push_back({"feature_gate_variant",
            "D_VISITOR_HAS_VARIANT tracks C++17",
            &tests_feature_gate_variant});
        block.tests.push_back({"feature_gate_concepts",
            "D_VISITOR_HAS_CONCEPTS tracks C++20",
            &tests_feature_gate_concepts});
        block.tests.push_back({"feature_gate_fold_expressions",
            "D_VISITOR_HAS_FOLD_EXPRESSIONS tracks C++17",
            &tests_feature_gate_fold_expressions});
        block.tests.push_back({"feature_gate_deduction_guides",
            "D_VISITOR_HAS_DEDUCTION_GUIDES tracks C++17",
            &tests_feature_gate_deduction_guides});
        block.tests.push_back({"feature_gates_agree_with_each_other",
            "the three C++17 gates cannot disagree",
            &tests_feature_gates_agree_with_each_other});
        block.tests.push_back({"default_return_type_is_void",
            "D_VISITOR_DEFAULT_RETURN_TYPE defaults to void",
            &tests_default_return_type_is_void});
        block.tests.push_back({"default_return_type_propagates_to_every_root",
            "every defaulted root agrees on the return type",
            &tests_default_return_type_propagates_to_every_root});

        spec.blocks.push_back(block);
    }

    // -- II. classic visitor ------------------------------------------------
    {
        dt::block_spec block;

        block.name       = "classic";
        block.descriptor = "Gamma-style double dispatch over a closed element set";

        block.tests.push_back({"visitor_base_return_type_typedef",
            "visitor_base<R>::return_type is R",
            &tests_visitor_base_return_type_typedef});
        block.tests.push_back({"visitor_base_is_polymorphic",
            "the visitor root has a vtable",
            &tests_visitor_base_is_polymorphic});
        block.tests.push_back({"visitable_base_return_type_typedef",
            "visitable_base<R>::return_type is R",
            &tests_visitable_base_return_type_typedef});
        block.tests.push_back({"visitable_base_is_abstract",
            "accept() is pure virtual",
            &tests_visitable_base_is_abstract});
        block.tests.push_back({"visitor_of_impl_is_abstract",
            "the per-type interface's visit() is pure virtual",
            &tests_visitor_of_impl_is_abstract});
        block.tests.push_back({"concrete_visitor_single_element",
            "concrete_visitor<R, A> instantiates and dispatches",
            &tests_concrete_visitor_single_element});
        block.tests.push_back({"concrete_visitor_multiple_elements",
            "concrete_visitor<R, A, B, C> instantiates and dispatches",
            &tests_concrete_visitor_multiple_elements});
        block.tests.push_back({"concrete_visitor_is_a_visitor_base",
            "and is usable as visitor_base<R>&",
            &tests_concrete_visitor_is_a_visitor_base});
        block.tests.push_back({"concrete_visitor_inherits_every_per_type_interface",
            "visitor_of_impl<R,X> is a base for EVERY X, not just the last",
            &tests_concrete_visitor_inherits_every_per_type_interface});
        block.tests.push_back({"concrete_visitor_is_abstract_until_all_visits_implemented",
            "the closed set is enforced at compile time",
            &tests_concrete_visitor_is_abstract_until_all_visits_implemented});
        block.tests.push_back({"concrete_visitor_overload_set_is_unambiguous",
            "every visit() overload is visible in one scope",
            &tests_concrete_visitor_overload_set_is_unambiguous});
        block.tests.push_back({"concrete_visitor_return_type_typedef",
            "concrete_visitor<R,...>::return_type is R",
            &tests_concrete_visitor_return_type_typedef});
        block.tests.push_back({"d_visitable_dispatches_to_the_matching_overload",
            "accept() reaches the overload for the element's own type",
            &tests_d_visitable_dispatches_to_the_matching_overload});
        block.tests.push_back({"d_visitable_double_dispatch_through_element_base",
            "dispatch is on the element's DYNAMIC type",
            &tests_d_visitable_double_dispatch_through_element_base});
        block.tests.push_back({"d_visitable_dispatch_through_visitor_base",
            "and works with the visitor type-erased to visitor_base<R>&",
            &tests_d_visitable_dispatch_through_visitor_base});
        block.tests.push_back({"d_visitable_returns_the_visitors_value",
            "accept() returns what visit() produced",
            &tests_d_visitable_returns_the_visitors_value});
        block.tests.push_back({"d_visitable_void_return",
            "and the void case works",
            &tests_d_visitable_void_return});
        block.tests.push_back({"d_visitable_unhandled_element_throws",
            "a closed-set violation is reported, not absorbed",
            &tests_d_visitable_unhandled_element_throws});
        block.tests.push_back({"classic_visitor_accumulates_across_a_traversal",
            "a stateful visitor accumulates over a heterogeneous sequence",
            &tests_classic_visitor_accumulates_across_a_traversal});

        spec.blocks.push_back(block);
    }

    // -- III. acyclic visitor -----------------------------------------------
    {
        dt::block_spec block;

        block.name       = "acyclic";
        block.descriptor = "dynamic_cast dispatch over an OPEN element set";

        block.tests.push_back({"acyclic_visitor_is_polymorphic",
            "the type-erased root has a vtable, so dynamic_cast is possible",
            &tests_acyclic_visitor_is_polymorphic});
        block.tests.push_back({"visitor_of_is_abstract",
            "visitor_of<E>::visit is pure virtual",
            &tests_visitor_of_is_abstract});
        block.tests.push_back({"visitor_of_return_type_typedef",
            "visitor_of<E,R>::return_type is R",
            &tests_visitor_of_return_type_typedef});
        block.tests.push_back({"visitor_of_default_return_type_is_void",
            "visitor_of<E> defaults R to void",
            &tests_visitor_of_default_return_type_is_void});
        block.tests.push_back({"acyclic_visitable_default_is_void",
            "acyclic_visitable<> and <void> are usable",
            &tests_acyclic_visitable_default_is_void});
        block.tests.push_back({"acyclic_visitable_return_type_typedef",
            "acyclic_visitable<R>::return_type is R",
            &tests_acyclic_visitable_return_type_typedef});
        block.tests.push_back({"acyclic_visitable_is_abstract",
            "accept() is pure virtual",
            &tests_acyclic_visitable_is_abstract});
        block.tests.push_back({"d_acyclic_visitable_dispatches_to_the_matching_interface",
            "accept() finds visitor_of<ThisType> on the visitor",
            &tests_d_acyclic_visitable_dispatches_to_the_matching_interface});
        block.tests.push_back({"d_acyclic_visitable_ignores_unhandled",
            "an element the visitor does not handle is SILENTLY SKIPPED",
            &tests_d_acyclic_visitable_ignores_unhandled});
        block.tests.push_back({"d_acyclic_visitable_ignores_everything_for_a_blind_visitor",
            "a visitor handling nothing is legal and inert",
            &tests_d_acyclic_visitable_ignores_everything_for_a_blind_visitor});
        block.tests.push_back({"d_acyclic_visitable_returns_default_when_unhandled",
            "the no-match path yields a value-initialized return",
            &tests_d_acyclic_visitable_returns_default_when_unhandled});
        block.tests.push_back({"d_acyclic_visitable_non_void_return",
            "and a handled element yields the visitor's value",
            &tests_d_acyclic_visitable_non_void_return});
        block.tests.push_back({"acyclic_visitor_is_decoupled_from_the_element_set",
            "the visitor root knows nothing of any element type",
            &tests_acyclic_visitor_is_decoupled_from_the_element_set});
        block.tests.push_back({"acyclic_visitor_accumulates_across_a_traversal",
            "a partial visitor accumulates only what it handles",
            &tests_acyclic_visitor_accumulates_across_a_traversal});

        spec.blocks.push_back(block);
    }

    // -- IV. static visitor -------------------------------------------------
    {
        dt::block_spec block;

        block.name       = "static";
        block.descriptor = "CRTP compile-time dispatch -- no vtable, no RTTI";

        block.tests.push_back({"visit_result_deduces_the_return_type",
            "visit_result<V,E>::type is decltype(v.visit(e))",
            &tests_visit_result_deduces_the_return_type});
        block.tests.push_back({"visit_result_is_sfinae_friendly",
            "an unhandled element yields NO ::type rather than a hard error",
            &tests_visit_result_is_sfinae_friendly});
        block.tests.push_back({"visit_result_t_alias_agrees",
            "visit_result_t is visit_result<...>::type",
            &tests_visit_result_t_alias_agrees});
        block.tests.push_back({"visit_result_distinguishes_const_elements",
            "V/E and V/const E are deduced independently",
            &tests_visit_result_distinguishes_const_elements});
        block.tests.push_back({"static_visitor_apply_dispatches_by_type",
            "apply() reaches the overload for the element's static type",
            &tests_static_visitor_apply_dispatches_by_type});
        block.tests.push_back({"static_visitor_apply_selects_const_element_overload",
            "a const element selects visit(const E&)",
            &tests_static_visitor_apply_selects_const_element_overload});
        block.tests.push_back({"static_visitor_apply_selects_const_visitor_overload",
            "a const visitor selects visit(const E&) const",
            &tests_static_visitor_apply_selects_const_visitor_overload});
        block.tests.push_back({"static_visitor_return_type_follows_the_visitor",
            "the return type is the visitor's, not the base's",
            &tests_static_visitor_return_type_follows_the_visitor});
        block.tests.push_back({"static_visitor_has_no_vtable",
            "the CRTP payoff: a static visitor is NOT polymorphic",
            &tests_static_visitor_has_no_vtable});
        block.tests.push_back({"static_visitable_accept_forwards_to_apply",
            "element.accept(v) == v.apply(element)",
            &tests_static_visitable_accept_forwards_to_apply});
        block.tests.push_back({"static_visitable_accept_const",
            "a const element forwards to the const-element overload",
            &tests_static_visitable_accept_const});
        block.tests.push_back({"static_visitable_accept_return_type",
            "accept() reports the visitor's return type",
            &tests_static_visitable_accept_return_type});
        block.tests.push_back({"static_visitor_dispatch_is_compile_time",
            "an unhandled element is a COMPILE-time fact, not a runtime one",
            &tests_static_visitor_dispatch_is_compile_time});

        spec.blocks.push_back(block);
    }

    // -- V. variant visitor -------------------------------------------------
    {
        dt::block_spec block;

        block.name       = "variant";
        block.descriptor = "std::visit over a closed alternative set (C++17+)";

        block.tests.push_back({"overloaded_combines_callables",
            "overloaded merges N callables into one overload set",
            &tests_overloaded_combines_callables});
        block.tests.push_back({"overloaded_deduction_guide",
            "CTAD deduces overloaded from its initializers",
            &tests_overloaded_deduction_guide});
        block.tests.push_back({"overloaded_generic_fallback",
            "a generic callable catches the alternatives the others miss",
            &tests_overloaded_generic_fallback});
        block.tests.push_back({"make_visitor_builds_an_overload_set",
            "make_visitor is the CTAD-free spelling of overloaded",
            &tests_make_visitor_builds_an_overload_set});
        block.tests.push_back({"make_visitor_decays_its_arguments",
            "lvalues and rvalues produce the same overload-set type",
            &tests_make_visitor_decays_its_arguments});
        block.tests.push_back({"variant_visit_dispatches_on_the_active_alternative",
            "the alternative held, not the one declared first",
            &tests_variant_visit_dispatches_on_the_active_alternative});
        block.tests.push_back({"variant_visit_returns_the_visitors_value",
            "and the visitor's value is returned unchanged",
            &tests_variant_visit_returns_the_visitors_value});
        block.tests.push_back({"variant_visit_multi_variant",
            "multi-dispatch over the cross product of two variants",
            &tests_variant_visit_multi_variant});
        block.tests.push_back({"variant_visit_is_constexpr",
            "and it folds in a constant expression",
            &tests_variant_visit_is_constexpr});
        block.tests.push_back({"variant_visit_with_index_supplies_the_index",
            "the visitor receives the active alternative's index",
            &tests_variant_visit_with_index_supplies_the_index});
        block.tests.push_back({"variant_visit_with_index_is_a_compile_time_constant",
            "as an integral_constant, usable in a constant expression",
            &tests_variant_visit_with_index_is_a_compile_time_constant});
        block.tests.push_back({"variant_visit_with_index_covers_every_alternative",
            "every alternative reports its own index",
            &tests_variant_visit_with_index_covers_every_alternative});

        spec.blocks.push_back(block);
    }

    // -- VI. concepts -------------------------------------------------------
    {
        dt::block_spec block;

        block.name       = "concepts";
        block.descriptor = "concept-constrained visitation (C++20+)";

        block.tests.push_back({"concept_visitable_type",
            "visitable_type admits an element with accept()",
            &tests_concept_visitable_type});
        block.tests.push_back({"concept_visitable_type_rejects_non_visitable",
            "and rejects one without",
            &tests_concept_visitable_type_rejects_non_visitable});
        block.tests.push_back({"concept_visitor_for",
            "visitor_for admits a visitor covering every element",
            &tests_concept_visitor_for});
        block.tests.push_back({"concept_visitor_for_rejects_partial_visitor",
            "and rejects one missing an overload -- the fold is over ALL",
            &tests_concept_visitor_for_rejects_partial_visitor});
        block.tests.push_back({"concept_acyclic_visitor_for",
            "acyclic_visitor_for admits a proper acyclic visitor",
            &tests_concept_acyclic_visitor_for});
        block.tests.push_back({"concept_acyclic_visitor_for_rejects_plain_visitor",
            "and requires BOTH the root and every visitor_of<E>",
            &tests_concept_acyclic_visitor_for_rejects_plain_visitor});
        block.tests.push_back({"constrained_accept_dispatches",
            "constrained_accept calls visit() and returns its value",
            &tests_constrained_accept_dispatches});
        block.tests.push_back({"constrained_accept_is_gated_by_the_concept",
            "and is not callable at all for a visitor that cannot handle it",
            &tests_constrained_accept_is_gated_by_the_concept});

        spec.blocks.push_back(block);
    }

    return spec;
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_VISITOR_TESTS_
