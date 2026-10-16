/*******************************************************************************
* djinterp [core]                                     visitor_tests_concepts.cpp
*
*
* path:      /tests/djinterp/core/paradigm/visitor/visitor_tests_concepts.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : visitor_tests_concepts.cpp
//   Section VI: the C++20 concept parallels.
//
//   None of the four strategies below them needs a concept to work.  What the
// concepts buy is the DIAGNOSTIC: without them, handing a visitor an element it
// cannot handle fails somewhere inside the dispatch machinery -- inside a
// static_cast in a CRTP base, or as a substitution failure with no ::type -- and
// the error names the header's internals rather than the caller's mistake.  With
// them, the caller is turned away AT THE CONSTRAINT.
//
//   That only holds if the concepts are SOUND: if a concept admits a type, the code
// it gates must actually work for it.  A constraint that accepts what the
// constrained code then rejects is worse than no constraint at all, because it
// moves the error later while claiming to have moved it earlier.  So each concept
// is checked in both directions -- what it admits, and what it turns away -- and
// tests_constrained_accept_is_gated_by_the_concept goes further, verifying that the
// rejection is a DETECTABLE constraint failure rather than a hard error inside the
// function body.
//
//   The whole section stands behind D_VISITOR_HAS_CONCEPTS.  Below C++20 the
// predicates stand down and report success, so the module spec is identical across
// dialects and the runner needs no conditional wiring.  The traits the concepts
// parallel are covered in the sections that own them.

// std
#include <string>
#include <type_traits>
// djinterp
#include "visitor_tests.hpp"

#if D_VISITOR_HAS_CONCEPTS
    // std
    #include <concepts>
    #include <utility>
#endif


NS_DJINTERP
NS_TESTING


#if D_VISITOR_HAS_CONCEPTS

namespace
{
    // total_visitor / partial_visitor
    //   fixture: two plain (non-derived) visitors over the same element pair.  The
    // concepts under test are structural -- they ask what a type CAN DO, not what
    // it derives from -- so the fixtures deliberately inherit nothing.
    struct total_visitor
    {
        std::string visit(static_circle&)
        {
            return "circle";
        }

        std::string visit(static_rect&)
        {
            return "rect";
        }
    };

    struct partial_visitor
    {
        std::string visit(static_circle&)
        {
            return "circle";
        }
        // ...and nothing for static_rect
    };

    // not_a_visitor
    //   fixture: no visit() at all.
    struct not_a_visitor
    {};

    // acceptor
    //   fixture: an element exposing accept(), which is all visitable_type asks
    // for.  It derives from no visitor root -- the concept is structural.
    struct acceptor
    {
        template<typename _Visitor>
        void accept(_Visitor&)
        {}
    };

    // no_accept
    //   fixture: an element with no accept().
    struct no_accept
    {};

    // plain_acyclic
    //   fixture: a well-formed acyclic visitor -- derives from the root AND from a
    // visitor_of<E> for each element it handles.
    struct plain_acyclic : acyclic_visitor,
                           visitor_of<acyclic_circle>,
                           visitor_of<acyclic_rect>
    {
        void visit(acyclic_circle&) override
        {}

        void visit(acyclic_rect&) override
        {}
    };

    // rootless_acyclic
    //   fixture: has the per-type interfaces but NOT the acyclic_visitor root, so it
    // can never be passed to accept() -- the dynamic_cast has nothing to start from.
    // It is the case the concept's first conjunct exists to catch.
    struct rootless_acyclic : visitor_of<acyclic_circle>
    {
        void visit(acyclic_circle&) override
        {}
    };

    // try_constrained_accept
    //   trait: is constrained_accept(e, v) a valid call?  Detected rather than
    // asserted, so a rejection is an ANSWER instead of a compile error -- which is
    // the property under test.
    template<typename _Element,
             typename _Visitor,
             typename = void>
    struct try_constrained_accept
        : std::false_type
    {};

    template<typename _Element,
             typename _Visitor>
    struct try_constrained_accept<
        _Element,
        _Visitor,
        std::void_t<decltype(constrained_accept(std::declval<_Element&>(),
                                                std::declval<_Visitor&>()))> >
        : std::true_type
    {};
}

#endif  // D_VISITOR_HAS_CONCEPTS


/*
tests_concept_visitable_type
  Verifies visitable_type admits an element that exposes accept().
  Tests the following:
  - a structural acceptor satisfies it, deriving from nothing
  - so do the real element roots from the classic and acyclic sections, each with
    the visitor type it actually accepts -- which is what makes the concept useful
    rather than merely true of a toy
*/
bool
tests_concept_visitable_type()
{
#if D_VISITOR_HAS_CONCEPTS
    static_assert(visitable_type<acceptor, total_visitor>,
                  "a type with accept() satisfies the concept");

    // the real elements, with the visitor each of them actually takes
    static_assert(visitable_type<classic_circle, visitor_base<std::string> >,
                  "a classic element accepts a visitor_base");
    static_assert(visitable_type<acyclic_circle, acyclic_visitor>,
                  "an acyclic element accepts an acyclic_visitor");
    static_assert(visitable_type<static_circle, tag_visitor>,
                  "and a static element accepts its static visitor");

    return true;
#else
    return true;
#endif
}

/*
tests_concept_visitable_type_rejects_non_visitable
  Verifies the concept turns away a type with no accept().
  Tests the following:
  - a type with no accept() at all is rejected
  - an element is rejected for a VISITOR it cannot accept, even though it does have
    an accept() -- the concept is about the pair, not the element alone
*/
bool
tests_concept_visitable_type_rejects_non_visitable()
{
#if D_VISITOR_HAS_CONCEPTS
    static_assert(!visitable_type<no_accept, total_visitor>,
                  "no accept() -- rejected");
    static_assert(!visitable_type<int, total_visitor>,
                  "nor does a scalar qualify");

    // it is the PAIR that is constrained: a classic element has an accept(), but not
    // one that takes an acyclic_visitor
    static_assert(!visitable_type<classic_circle, acyclic_visitor>,
                  "a classic element does not accept an acyclic visitor");
    static_assert(visitable_type<classic_circle, visitor_base<std::string> >,
                  "while it does accept the visitor it was built for");

    return true;
#else
    return true;
#endif
}

/*
tests_concept_visitor_for
  Verifies visitor_for admits a visitor that can visit every listed element.
  Tests the following:
  - a total visitor satisfies it for the whole element list
  - it satisfies it for each element individually, so the fold is not vacuously true
  - the concept is structural: a visitor deriving from nothing qualifies
*/
bool
tests_concept_visitor_for()
{
#if D_VISITOR_HAS_CONCEPTS
    static_assert(visitor_for<total_visitor, static_circle, static_rect>,
                  "a visitor covering both elements satisfies the concept");

    static_assert(visitor_for<total_visitor, static_circle>,
                  "and each element on its own");
    static_assert(visitor_for<total_visitor, static_rect>,
                  "for both");

    // the real composed visitor from the classic section also qualifies -- the
    // concept asks what the type can DO, not what it derives from
    static_assert(visitor_for<shape_namer,
                              classic_circle, classic_rect, classic_tri>,
                  "including a concrete_visitor over its own element set");

    return true;
#else
    return true;
#endif
}

/*
tests_concept_visitor_for_rejects_partial_visitor
  Verifies the fold is over ALL the elements -- a visitor missing even one overload
  is rejected.  This is the one property that makes the concept worth having: it is
  a conjunction, and a conjunction that quietly behaved as a disjunction would still
  admit every total visitor and pass every positive test above.
  Tests the following:
  - a visitor missing one overload is rejected for the FULL list
  - it is still accepted for the sub-list it does cover, so the rejection is about
    the missing element specifically and not about the visitor being malformed
  - a visitor with no visit() at all is rejected
  - the empty element list is vacuously satisfied, which is the fold's identity
*/
bool
tests_concept_visitor_for_rejects_partial_visitor()
{
#if D_VISITOR_HAS_CONCEPTS
    static_assert(!visitor_for<partial_visitor, static_circle, static_rect>,
                  "missing ONE overload fails the whole conjunction");

    // ...but it is fine for the element it does handle: the rejection is specific
    static_assert(visitor_for<partial_visitor, static_circle>,
                  "the visitor is not malformed -- it simply lacks static_rect");
    static_assert(!visitor_for<partial_visitor, static_rect>,
                  "which is exactly the element it is rejected for");

    static_assert(!visitor_for<not_a_visitor, static_circle>,
                  "and a type with no visit() at all is rejected");

    // the identity of the fold: no elements to check, so nothing to fail
    static_assert(visitor_for<not_a_visitor>,
                  "an empty element list is vacuously satisfied");

    return true;
#else
    return true;
#endif
}

/*
tests_concept_acyclic_visitor_for
  Verifies acyclic_visitor_for admits a proper acyclic visitor -- one that derives
  from the type-erased root AND from visitor_of<E> for every element listed.  Unlike
  the two concepts above, this one is about INHERITANCE, because the acyclic dispatch
  is a dynamic_cast and only a real base subobject can be found by one.
  Tests the following:
  - a visitor with the root and both interfaces satisfies it
  - it satisfies it for each element individually
  - the round_visitor fixture from the acyclic section qualifies for the elements it
    handles
*/
bool
tests_concept_acyclic_visitor_for()
{
#if D_VISITOR_HAS_CONCEPTS
    static_assert(acyclic_visitor_for<plain_acyclic,
                                      acyclic_circle, acyclic_rect>,
                  "root + every visitor_of<E> -- admitted");

    static_assert(acyclic_visitor_for<plain_acyclic, acyclic_circle>,
                  "and for each element on its own");

    // the real fixture the acyclic section dispatches through
    static_assert(acyclic_visitor_for<round_visitor,
                                      acyclic_circle, acyclic_rect>,
                  "including round_visitor, over the elements it handles");

    return true;
#else
    return true;
#endif
}

/*
tests_concept_acyclic_visitor_for_rejects_plain_visitor
  Verifies BOTH conjuncts are load-bearing: the root AND every per-type interface.
  Each half fails for a different reason at runtime, and a concept that checked only
  one would let the other through.
  Tests the following:
  - a visitor with the interfaces but NOT the root is rejected (the dynamic_cast has
    nothing to start from -- it could never be passed to accept() at all)
  - a visitor with the root but MISSING an interface is rejected for the element it
    lacks, while still being admitted for the one it has
  - a structurally-correct but unrelated visitor -- one that has visit() overloads
    yet derives from nothing -- is rejected, which is what distinguishes this concept
    from visitor_for
*/
bool
tests_concept_acyclic_visitor_for_rejects_plain_visitor()
{
#if D_VISITOR_HAS_CONCEPTS
    // has visitor_of<acyclic_circle>, but not the root
    static_assert(!acyclic_visitor_for<rootless_acyclic, acyclic_circle>,
                  "no acyclic_visitor root -- rejected, however many interfaces");

    // has the root, but is missing the interface for acyclic_tri
    static_assert(!acyclic_visitor_for<round_visitor, acyclic_tri>,
                  "missing the per-type interface -- rejected for that element");
    static_assert(acyclic_visitor_for<round_visitor, acyclic_circle>,
                  "while still admitted for the elements it does implement");
    static_assert(!acyclic_visitor_for<round_visitor,
                                       acyclic_circle, acyclic_tri>,
                  "and one missing element fails the whole conjunction");

    // the distinction from visitor_for: total_visitor CAN visit its elements
    // structurally, but derives from nothing -- so it is not an acyclic visitor
    static_assert(visitor_for<total_visitor, static_circle>,
                  "structurally it can visit...");
    static_assert(!acyclic_visitor_for<total_visitor, static_circle>,
                  "...but it is not an ACYCLIC visitor, which is an inheritance claim");

    // a blind visitor has the root and no interfaces: fine for nothing, and it says so
    static_assert(!acyclic_visitor_for<blind_visitor, acyclic_circle>,
                  "the root alone is not enough");
    static_assert(acyclic_visitor_for<blind_visitor>,
                  "though with no elements to cover, the root alone suffices");

    return true;
#else
    return true;
#endif
}

/*
tests_constrained_accept_dispatches
  Verifies constrained_accept actually does the visit, and returns its value.  The
  constraint is the point of the function, but a constraint on a function that did
  nothing would be useless.
  Tests the following:
  - it calls visit() on the element and returns what visit() produced
  - it dispatches by element type
  - its return type is the visitor's
*/
bool
tests_constrained_accept_dispatches()
{
#if D_VISITOR_HAS_CONCEPTS
    static_circle c;
    static_rect   r;

    total_visitor v;

    static_assert(std::is_same<decltype(constrained_accept(c, v)),
                               std::string>::value,
                  "the return type is the visitor's");

    bool ok = true;

    ok = ok && (constrained_accept(c, v) == "circle");
    ok = ok && (constrained_accept(r, v) == "rect");
    ok = ok && (constrained_accept(c, v) != constrained_accept(r, v));

    // and it agrees with calling visit() directly
    ok = ok && (constrained_accept(c, v) == v.visit(c));

    return ok;
#else
    return true;
#endif
}

/*
tests_constrained_accept_is_gated_by_the_concept
  Verifies the rejection happens AT THE CONSTRAINT -- which is the entire reason the
  function exists.  Without it, a visitor missing an overload fails inside the
  dispatch machinery with an error naming the header's internals; with it, the call
  is simply not viable, and the compiler says so at the call site.

  The rejection must also be DETECTABLE -- a substitution failure, not a hard error --
  or the function could not be used in any SFINAE context and would take the enclosing
  overload set down with it.  That is what the trait below measures: a hard error
  inside constrained_accept's body would fail to compile this test rather than report
  false.
  Tests the following:
  - the call is viable for a visitor that handles the element
  - it is NOT viable for one that does not, and asking is not an error
  - the gate tracks visitor_for exactly, positive and negative -- so the constraint
    is the concept, with no drift
*/
bool
tests_constrained_accept_is_gated_by_the_concept()
{
#if D_VISITOR_HAS_CONCEPTS
    // viable where the visitor handles the element
    static_assert(try_constrained_accept<static_circle, total_visitor>::value,
                  "a visitor that handles the element may call it");
    static_assert(try_constrained_accept<static_rect, total_visitor>::value,
                  "for each element it handles");

    // NOT viable where it does not -- and asking is a detectable non-answer
    static_assert(!try_constrained_accept<static_rect, partial_visitor>::value,
                  "a visitor missing the overload is turned away AT THE CONSTRAINT");
    static_assert(!try_constrained_accept<static_circle, not_a_visitor>::value,
                  "as is one with no visit() at all");

    // and the gate is exactly the concept, with no drift between the two
    static_assert(try_constrained_accept<static_rect, total_visitor>::value ==
                  visitor_for<total_visitor, static_rect>,
                  "the constraint tracks visitor_for (positive)");
    static_assert(try_constrained_accept<static_rect, partial_visitor>::value ==
                  visitor_for<partial_visitor, static_rect>,
                  "the constraint tracks visitor_for (negative)");

    return ( try_constrained_accept<static_circle, total_visitor>::value &&
            !try_constrained_accept<static_rect, partial_visitor>::value );
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
