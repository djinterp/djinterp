/*******************************************************************************
* djinterp [core]                                      visitor_tests_classic.cpp
*
*
* path:      /tests/djinterp/core/paradigm/visitor/visitor_tests_classic.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : visitor_tests_classic.cpp
//   Section II: the classic (Gamma-style) visitor.  Double dispatch over a CLOSED
// element set: the visitor is required to handle every element, and
// concrete_visitor enforces that by making each visit() pure virtual.
//
//   The hard part of this strategy is a structural one, and it is worth stating
// plainly because it is what both of this section's bugs came from.  The element's
// accept() receives the visitor TYPE-ERASED, as visitor_base<R>& -- and
// visitor_base<R> declares no visit() at all.  So accept() has to recover the
// concrete per-type interface, visitor_of_impl<R, ThisType>, from a reference to
// a base that knows nothing about it.  Those two are SIBLING bases within the
// concrete visitor, not base and derived, so the recovery is a CROSS-CAST -- and
// it only works if two things hold at once:
//
//     1.  visitor_of_impl<R, X> is genuinely a base of the visitor for EVERY
//         element type X in the set (not just the last one), and
//     2.  the cast can actually express a cross-cast.
//
//   The shipped header had neither.  Its overload-set chain declared its own
// visit() overloads and never inherited visitor_of_impl at all, so there was no
// subobject to find (and the chain did not compile regardless, since it named a
// visit() on an empty terminal that had none).  And D_VISITABLE used static_cast,
// which cannot express a cross-cast between unrelated types and is simply
// ill-formed.  tests_concrete_visitor_inherits_every_per_type_interface pins (1)
// -- deliberately including a MID-PACK element, since a chain that happened to
// inherit only the last one would still pass a one-element test.
//
//   The other thing this section pins is the CONTRACT, in
// tests_d_visitable_unhandled_element_throws.  A visitor that cannot accept an
// element is a program error here, not a runtime condition -- the closed set says
// so -- and it must be reported.  The acyclic section runs the identical scenario
// and requires the OPPOSITE outcome, and a header that got the two backwards would
// still pass every "does dispatch work" test in both.

// std
#include <string>
#include <type_traits>
#include <typeinfo>
// djinterp
#include "visitor_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_visitor_base_return_type_typedef
  Verifies the visitor root advertises its return type.
  Tests the following:
  - visitor_base<R>::return_type is R, for void and for a non-trivial R
*/
bool
tests_visitor_base_return_type_typedef()
{
    static_assert(std::is_same<visitor_base<std::string>::return_type,
                               std::string>::value,
                  "return_type is R");
    static_assert(std::is_same<visitor_base<void>::return_type, void>::value,
                  "including void");
    static_assert(std::is_same<visitor_base<int>::return_type, int>::value,
                  "and a value type");

    return true;
}

/*
tests_visitor_base_is_polymorphic
  Verifies the visitor root has a vtable.  This is not decoration: D_VISITABLE
  recovers the per-type interface from a visitor_base<R>& by cross-cast, and a
  cross-cast requires the source to be polymorphic.  A visitor_base without a
  virtual destructor would make the entire classic strategy impossible AND leak on
  every delete-through-base.
  Tests the following:
  - visitor_base<R> is polymorphic
  - it has a virtual destructor, so deleting through it is well-defined
*/
bool
tests_visitor_base_is_polymorphic()
{
    static_assert(std::is_polymorphic<visitor_base<void> >::value,
                  "the visitor root must be polymorphic for the cross-cast");
    static_assert(std::is_polymorphic<visitor_base<std::string> >::value,
                  "for every return type");
    static_assert(std::has_virtual_destructor<visitor_base<void> >::value,
                  "and destruction through it must be well-defined");

    return true;
}

/*
tests_visitable_base_return_type_typedef
  Verifies the element root advertises the return type accept() produces.
  Tests the following:
  - visitable_base<R>::return_type is R
  - it agrees with the visitor's return_type for the same R, which is what lets
    accept() forward visit()'s result unchanged
*/
bool
tests_visitable_base_return_type_typedef()
{
    static_assert(std::is_same<visitable_base<std::string>::return_type,
                               std::string>::value,
                  "return_type is R");
    static_assert(std::is_same<visitable_base<void>::return_type, void>::value,
                  "including void");
    static_assert(std::is_same<visitable_base<std::string>::return_type,
                               visitor_base<std::string>::return_type>::value,
                  "element and visitor agree on R");

    return true;
}

/*
tests_visitable_base_is_abstract
  Verifies accept() is pure virtual, so an element cannot forget to implement it.
  Tests the following:
  - visitable_base<R> is abstract
  - a concrete element that expands D_VISITABLE is NOT abstract, so the macro
    really does satisfy the pure virtual rather than merely adding an overload
    alongside it
*/
bool
tests_visitable_base_is_abstract()
{
    static_assert(std::is_abstract<visitable_base<std::string> >::value,
                  "accept() is pure virtual");
    static_assert(std::is_abstract<classic_shape>::value,
                  "and an element root that does not implement it stays abstract");

    static_assert(!std::is_abstract<classic_circle>::value,
                  "D_VISITABLE satisfies the pure virtual, it does not sit beside it");
    static_assert(std::is_base_of<visitable_base<std::string>,
                                  classic_circle>::value,
                  "and the element really is a visitable_base");

    return true;
}

/*
tests_visitor_of_impl_is_abstract
  Verifies the per-type interface's visit() is pure virtual.  This is what forces
  a concrete_visitor to implement every element's overload -- the closed-set
  guarantee is nothing more than this pure virtual, repeated once per element.
  Tests the following:
  - visitor_of_impl<R, E> is abstract
  - it is polymorphic, which the cross-cast's target must be
*/
bool
tests_visitor_of_impl_is_abstract()
{
    static_assert(std::is_abstract<
                      visitor_of_impl<std::string, classic_circle> >::value,
                  "visit() is pure virtual");
    static_assert(std::is_polymorphic<
                      visitor_of_impl<std::string, classic_circle> >::value,
                  "and the cross-cast's target must be polymorphic");
    static_assert(std::is_abstract<visitor_of_impl<void, dot> >::value,
                  "for the void return type too");

    return true;
}

/*
tests_concrete_visitor_single_element
  Verifies concrete_visitor instantiates and dispatches for a ONE-element set.
  The shipped overload-set chain failed here: its recursive case imported visit()
  from the terminal, and the terminal -- the empty pack -- declares none, so a
  single element was already a hard error.
  Tests the following:
  - concrete_visitor<R, A> instantiates
  - a visitor deriving from it dispatches to visit(A&)
*/
bool
tests_concrete_visitor_single_element()
{
    circle_only_visitor v;

    classic_circle c;

    bool ok = true;

    ok = ok && (v.visit(c) == "only circle");

    // and it is a visitor_base, so it can be handed to accept()
    ok = ok && (c.accept(v) == "only circle");

    return ok;
}

/*
tests_concrete_visitor_multiple_elements
  Verifies concrete_visitor instantiates and dispatches for a MULTI-element set,
  where the chain actually recurses.
  Tests the following:
  - concrete_visitor<R, A, B, C> instantiates
  - each element reaches its own overload, and the three results are DISTINCT --
    a chain that collapsed the overloads onto one another would still return a
    string, just the wrong one
*/
bool
tests_concrete_visitor_multiple_elements()
{
    shape_namer v;

    classic_circle c;
    classic_rect   r;
    classic_tri    t;

    bool ok = true;

    ok = ok && (v.visit(c) == "saw circle");
    ok = ok && (v.visit(r) == "saw rect");
    ok = ok && (v.visit(t) == "saw tri");

    // the three really are different, so nothing is collapsing
    ok = ok && (v.visit(c) != v.visit(r));
    ok = ok && (v.visit(r) != v.visit(t));

    return ok;
}

/*
tests_concrete_visitor_is_a_visitor_base
  Verifies the composed visitor is usable where the element's accept() expects
  one, which is the only reason the composition exists.
  Tests the following:
  - concrete_visitor<R, ...> derives from visitor_base<R>
  - a reference to the base binds to a concrete visitor
  - the base is UNAMBIGUOUS -- a layout that produced two visitor_base subobjects
    would make this binding ill-formed, which is precisely what a per-type
    interface deriving from visitor_base would have caused
*/
bool
tests_concrete_visitor_is_a_visitor_base()
{
    static_assert(std::is_base_of<visitor_base<std::string>,
                                  shape_namer>::value,
                  "a concrete visitor IS a visitor_base");

    shape_namer v;

    // the conversion is unambiguous: exactly one visitor_base<R> subobject
    static_assert(std::is_convertible<shape_namer*,
                                      visitor_base<std::string>*>::value,
                  "and the base subobject is unique");

    visitor_base<std::string>& base = v;

    classic_circle c;

    return (c.accept(base) == "saw circle");
}

/*
tests_concrete_visitor_inherits_every_per_type_interface
  Verifies the structural property D_VISITABLE's cross-cast depends on:
  visitor_of_impl<R, X> is a base of the visitor for EVERY element type X in the
  set -- not merely for the last one.

  This is the test the shipped header could not have passed even had it compiled.
  Its chain declared visit() overloads directly and inherited visitor_of_impl for
  no element at all, so the cast had nothing to land on.  A chain that inherited
  the interface only at the recursion's tail would still satisfy a one-element
  test, which is why classic_rect -- deliberately the MIDDLE of the pack -- is
  checked here.
  Tests the following:
  - visitor_of_impl<R, X> is a base of the visitor for the first, middle and last
    element of the set
  - each of those base subobjects is unambiguously reachable
*/
bool
tests_concrete_visitor_inherits_every_per_type_interface()
{
    typedef concrete_visitor<std::string,
                             classic_circle,
                             classic_rect,
                             classic_tri> visitor_t;

    static_assert(std::is_base_of<visitor_of_impl<std::string, classic_circle>,
                                  visitor_t>::value,
                  "FIRST element's interface is a base");
    static_assert(std::is_base_of<visitor_of_impl<std::string, classic_rect>,
                                  visitor_t>::value,
                  "MIDDLE element's interface is a base -- the case a tail-only "
                  "chain would silently miss");
    static_assert(std::is_base_of<visitor_of_impl<std::string, classic_tri>,
                                  visitor_t>::value,
                  "LAST element's interface is a base");

    // and each is unambiguously reachable, which is what the cross-cast needs
    static_assert(std::is_convertible<
                      shape_namer*,
                      visitor_of_impl<std::string, classic_rect>*>::value,
                  "the mid-pack subobject is uniquely reachable");

    // an element type OUTSIDE the set is not a base
    static_assert(!std::is_base_of<visitor_of_impl<std::string, dot>,
                                   visitor_t>::value,
                  "and only the declared element types get an interface");

    return true;
}

/*
tests_concrete_visitor_is_abstract_until_all_visits_implemented
  Verifies the closed set is enforced AT COMPILE TIME.  This is the classic form's
  central promise: you cannot construct a visitor that forgets an element, because
  every visit() in the composed base is pure virtual.
  Tests the following:
  - concrete_visitor<R, A, B, C> is abstract
  - a derived visitor implementing every overload is NOT abstract
  - the per-element interfaces it composes are each abstract on their own
*/
bool
tests_concrete_visitor_is_abstract_until_all_visits_implemented()
{
    static_assert(std::is_abstract<concrete_visitor<std::string,
                                                    classic_circle,
                                                    classic_rect,
                                                    classic_tri> >::value,
                  "every visit() is pure -- the set is closed by construction");

    static_assert(!std::is_abstract<shape_namer>::value,
                  "a visitor implementing all three is concrete");
    static_assert(!std::is_abstract<circle_only_visitor>::value,
                  "as is one over a smaller set, implementing all of ITS set");

    static_assert(std::is_abstract<concrete_visitor<void, dot> >::value,
                  "including the single-element, void-returning case");
    static_assert(!std::is_abstract<dot_counter>::value,
                  "which its implementor makes concrete");

    return true;
}

/*
tests_concrete_visitor_overload_set_is_unambiguous
  Verifies every visit() overload is visible in ONE scope.  The composed chain
  brings each element's interface in as a separate base subobject, so without the
  using-declarations that merge them, the name `visit` would be found in two
  distinct bases and every call would be ambiguous BEFORE overload resolution ever
  ran -- an error that would not depend on the argument type at all.
  Tests the following:
  - calling visit() with each of the three element types resolves
  - the results are distinct, so resolution picked the right overload each time
  - the same holds through a reference to the composed base, where no derived
    declarations are in scope to paper over an ambiguity
*/
bool
tests_concrete_visitor_overload_set_is_unambiguous()
{
    shape_namer v;

    classic_circle c;
    classic_rect   r;
    classic_tri    t;

    bool ok = true;

    // resolved in the derived visitor's own scope
    ok = ok && (v.visit(c) == "saw circle");
    ok = ok && (v.visit(r) == "saw rect");
    ok = ok && (v.visit(t) == "saw tri");

    // and through the composed base itself, where only the merged overload set
    // exists.  A missing using-declaration would make each of these ambiguous.
    concrete_visitor<std::string,
                     classic_circle,
                     classic_rect,
                     classic_tri>& base = v;

    ok = ok && (base.visit(c) == "saw circle");
    ok = ok && (base.visit(r) == "saw rect");
    ok = ok && (base.visit(t) == "saw tri");

    return ok;
}

/*
tests_concrete_visitor_return_type_typedef
  Verifies the composed visitor re-advertises the return type.
  Tests the following:
  - concrete_visitor<R, ...>::return_type is R
  - it agrees with the visitor_base it composes
*/
bool
tests_concrete_visitor_return_type_typedef()
{
    typedef concrete_visitor<std::string, classic_circle> visitor_t;

    static_assert(std::is_same<visitor_t::return_type, std::string>::value,
                  "return_type is R");
    static_assert(std::is_same<visitor_t::return_type,
                               visitor_base<std::string>::return_type>::value,
                  "and agrees with the visitor root");
    static_assert(std::is_same<concrete_visitor<void, dot>::return_type,
                               void>::value,
                  "including void");

    return true;
}

/*
tests_d_visitable_dispatches_to_the_matching_overload
  Verifies accept() reaches the visit() overload for the element's own type.
  Tests the following:
  - each element's accept() produces that element's result
  - the three results are distinct, so accept() is not collapsing onto one overload
*/
bool
tests_d_visitable_dispatches_to_the_matching_overload()
{
    shape_namer v;

    classic_circle c;
    classic_rect   r;
    classic_tri    t;

    bool ok = true;

    ok = ok && (c.accept(v) == "saw circle");
    ok = ok && (r.accept(v) == "saw rect");
    ok = ok && (t.accept(v) == "saw tri");

    ok = ok && (c.accept(v) != r.accept(v));
    ok = ok && (r.accept(v) != t.accept(v));

    return ok;
}

/*
tests_d_visitable_double_dispatch_through_element_base
  Verifies the dispatch is on the element's DYNAMIC type -- which is the whole
  point of "double" dispatch, and the thing a single virtual call cannot do.  The
  elements are held as classic_shape*, so their static type carries no information
  about which overload should run; only the vtable does.
  Tests the following:
  - a heterogeneous sequence held by base pointer dispatches correctly per element
  - the static type of the pointer is the element ROOT, not the concrete type, so
    the correct answer can only come from the dynamic type
*/
bool
tests_d_visitable_double_dispatch_through_element_base()
{
    classic_circle c;
    classic_rect   r;
    classic_tri    t;

    // static type is the root; nothing here says "circle"
    classic_shape* shapes[3];

    shapes[0] = &c;
    shapes[1] = &r;
    shapes[2] = &t;

    // decltype on a subscript yields a reference, so strip it: the point is that
    // the POINTEE is the element root, carrying no concrete type information.
    typedef clean_t<decltype(shapes[0])> held_t;

    static_assert(std::is_same<held_t, classic_shape*>::value,
                  "the pointers carry no concrete type information");

    shape_namer v;

    bool ok = true;

    ok = ok && (shapes[0]->accept(v) == "saw circle");
    ok = ok && (shapes[1]->accept(v) == "saw rect");
    ok = ok && (shapes[2]->accept(v) == "saw tri");

    return ok;
}

/*
tests_d_visitable_dispatch_through_visitor_base
  Verifies dispatch works with the VISITOR type-erased too -- both halves of the
  double dispatch erased at once.  This is the configuration D_VISITABLE actually
  faces: accept()'s parameter is visitor_base<R>&, a type that declares no visit()
  at all, and the concrete interface has to be recovered by cross-cast.
  Tests the following:
  - accept() works when the visitor is passed as visitor_base<R>&
  - the result is identical to passing the concrete visitor directly
  - it works for every element in the set, including through element base pointers
*/
bool
tests_d_visitable_dispatch_through_visitor_base()
{
    shape_namer v;

    visitor_base<std::string>& erased = v;

    classic_circle c;
    classic_rect   r;
    classic_tri    t;

    bool ok = true;

    ok = ok && (c.accept(erased) == "saw circle");
    ok = ok && (r.accept(erased) == "saw rect");
    ok = ok && (t.accept(erased) == "saw tri");

    // erasing the visitor changes nothing
    ok = ok && (c.accept(erased) == c.accept(v));

    // and both halves erased at once
    classic_shape* shape = &r;

    ok = ok && (shape->accept(erased) == "saw rect");

    return ok;
}

/*
tests_d_visitable_returns_the_visitors_value
  Verifies accept() forwards visit()'s result rather than discarding it or
  producing a default.
  Tests the following:
  - accept() returns exactly what the visitor's visit() produced
  - the value is a real one, not a value-initialized R (which is what a swallowed
    dispatch would most plausibly produce)
*/
bool
tests_d_visitable_returns_the_visitors_value()
{
    shape_namer v;

    classic_circle c;

    const std::string via_accept = c.accept(v);
    const std::string via_visit  = v.visit(c);

    bool ok = true;

    ok = ok && (via_accept == via_visit);
    ok = ok && (via_accept == "saw circle");
    ok = ok && (!via_accept.empty());        // not a value-initialized string

    return ok;
}

/*
tests_d_visitable_void_return
  Verifies the void case works.  void is the return type most visitors use, and
  the one most likely to break a return-type-generic implementation: the macro's
  body is `return <cast>.visit(*this);`, which for void is a return of a void
  prvalue -- legal, but only if nothing along the way assumed R was an object type.
  Tests the following:
  - accept() on a void-returning element compiles and runs
  - the visitor's side effect happens exactly once per accept()
  - the element's side effect happens too, so the visitor received *this and not a
    copy
*/
bool
tests_d_visitable_void_return()
{
    dot         d;
    dot_counter v;

    d.accept(v);
    d.accept(v);
    d.accept(v);

    bool ok = true;

    ok = ok && (v.count == 3);    // once per accept()
    ok = ok && (d.hits  == 3);    // the visitor got the element BY REFERENCE

    // and through the erased visitor base
    visitor_base<void>& erased = v;

    d.accept(erased);

    ok = ok && (v.count == 4);
    ok = ok && (d.hits  == 4);

    return ok;
}

/*
tests_d_visitable_unhandled_element_throws
  Verifies the CLOSED-SET CONTRACT.  A visitor is required to handle every element;
  circle_only_visitor is a perfectly well-formed visitor over the smaller set
  {classic_circle}, and handing it a classic_rect violates that requirement.  The
  classic form must report this rather than absorb it -- the pure virtual visit()s
  exist precisely to make forgetting an element impossible, and silently returning
  a default would defeat the guarantee the strategy is chosen FOR.

  This is the same scenario as tests_d_acyclic_visitable_ignores_unhandled, with
  the opposite required outcome.  The two contracts are what distinguish the
  strategies; a header that swapped them would still pass every dispatch test in
  both sections.
  Tests the following:
  - accept() on an element the visitor cannot handle throws std::bad_cast
  - the SAME visitor still dispatches correctly for the element it does handle, so
    the throw is about this element and not a broken visitor
  - and it throws through the type-erased visitor base as well
*/
bool
tests_d_visitable_unhandled_element_throws()
{
    circle_only_visitor v;   // handles classic_circle, and nothing else

    classic_circle c;
    classic_rect   r;

    bool ok = true;

    // the element it DOES handle still works
    ok = ok && (c.accept(v) == "only circle");

    // the element it does not is a program error, and is reported
    bool threw = false;

    try
    {
        (void)r.accept(v);
    }
    catch (const std::bad_cast&)
    {
        threw = true;
    }

    ok = ok && threw;

    // and likewise with the visitor erased -- the path D_VISITABLE actually takes
    visitor_base<std::string>& erased = v;

    bool threw_erased = false;

    try
    {
        classic_tri t;

        (void)t.accept(erased);
    }
    catch (const std::bad_cast&)
    {
        threw_erased = true;
    }

    ok = ok && threw_erased;

    // and the handled element is STILL fine afterwards
    ok = ok && (c.accept(erased) == "only circle");

    return ok;
}

/*
tests_classic_visitor_accumulates_across_a_traversal
  Verifies a stateful visitor accumulates over a heterogeneous sequence -- which is
  most of why one reaches for the pattern at all.  A visitor that dispatched
  correctly but was copied at each accept() would pass every test above and fail
  this one.
  Tests the following:
  - one visitor, walked over a mixed sequence held by base pointer, tallies each
    element type independently
  - the tallies are correct, so no element was miscounted as another
  - the visitor's state persists across accept() calls, so it was taken by
    reference throughout
*/
bool
tests_classic_visitor_accumulates_across_a_traversal()
{
    classic_circle c1;
    classic_circle c2;
    classic_rect   r1;
    classic_tri    t1;
    classic_tri    t2;
    classic_tri    t3;

    classic_shape* shapes[6];

    shapes[0] = &c1;
    shapes[1] = &t1;
    shapes[2] = &r1;
    shapes[3] = &t2;
    shapes[4] = &c2;
    shapes[5] = &t3;

    shape_counter v;

    for (int i = 0; i < 6; ++i)
    {
        (void)shapes[i]->accept(v);
    }

    bool ok = true;

    ok = ok && (v.circles == 2);
    ok = ok && (v.rects   == 1);
    ok = ok && (v.tris    == 3);

    // the total is the traversal length: nothing was dropped or double-counted
    ok = ok && ((v.circles + v.rects + v.tris) == 6);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
