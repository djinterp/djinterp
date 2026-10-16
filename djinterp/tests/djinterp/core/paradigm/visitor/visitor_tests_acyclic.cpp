/*******************************************************************************
* djinterp [core]                                      visitor_tests_acyclic.cpp
*
*
* path:      /tests/djinterp/core/paradigm/visitor/visitor_tests_acyclic.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : visitor_tests_acyclic.cpp
//   Section III: the acyclic visitor.  Same double dispatch, opposite contract.
//
//   The classic form couples the two hierarchies: a visitor must name every
// element type it can be handed, so adding an element forces every visitor to
// change.  The acyclic form breaks that cycle by type-erasing the visitor root
// down to `acyclic_visitor` -- which names NO element type at all -- and having
// the element recover the interface it needs by dynamic_cast at the point of
// dispatch.  The element set is therefore OPEN: a visitor handles what it cares
// about, and an element it does not handle is not an error.
//
//   That last clause is the whole distinction, and it is what
// tests_d_acyclic_visitable_ignores_unhandled exists to pin.  It runs the SAME
// scenario as the classic section's tests_d_visitable_unhandled_element_throws --
// a visitor that covers only part of the element set, handed an element outside
// it -- and requires the OPPOSITE outcome: silently skipped, default returned.
// The two tests are the contracts, stated from both sides.  A header that swapped
// them, or that made both throw, or both absorb, would still pass every "does
// dispatch work" test in either section, which is exactly why neither section's
// dispatch tests are sufficient on their own.
//
//   acyclic_visitable<> is also where bug C lived: the class carried a second
// template parameter, `_ReturnType _DefaultReturn = _ReturnType()`, which is a
// NON-TYPE parameter of type _ReturnType.  void is not a permitted type for one,
// so the default spelling -- and the one the header's own usage example gives --
// was ill-formed.  Every fixture in this section uses acyclic_visitable<>, so the
// section as a whole would not compile against the shipped header.

// std
#include <string>
#include <type_traits>
// djinterp
#include "visitor_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    // late_element
    //   fixture: an element type introduced AFTER round_visitor was written -- it
    // appears nowhere in that visitor's declaration, and the visitor was compiled
    // without knowing it exists.  Under the classic form this could not be
    // written at all without editing the visitor's element list; that it can be
    // written here at all is the acyclicity.
    class late_element : public acyclic_visitable<>
    {
    public:
        int seen;

        late_element()
            : seen(0)
        {}

        D_ACYCLIC_VISITABLE(late_element, void)
    };

    // late_visitor
    //   fixture: a visitor for the late element, likewise introduced without
    // touching anything that already existed.
    class late_visitor : public acyclic_visitor,
                         public visitor_of<late_element>
    {
    public:
        int lates;

        late_visitor()
            : lates(0)
        {}

        void visit(late_element& _e)
        {
            ++lates;
            ++_e.seen;
        }
    };
}


/*
tests_acyclic_visitor_is_polymorphic
  Verifies the type-erased visitor root has a vtable.  Everything in this section
  rests on it: D_ACYCLIC_VISITABLE dispatches by dynamic_cast from an
  acyclic_visitor&, and dynamic_cast requires a polymorphic source.  A root
  without a virtual destructor would make the strategy impossible AND leak on
  every delete-through-base.
  Tests the following:
  - acyclic_visitor is polymorphic and has a virtual destructor
  - it is NOT abstract: it names no element type and declares no visit(), so a
    visitor that handles nothing is a legitimate, constructible thing
*/
bool
tests_acyclic_visitor_is_polymorphic()
{
    static_assert(std::is_polymorphic<acyclic_visitor>::value,
                  "dynamic_cast requires a polymorphic source");
    static_assert(std::has_virtual_destructor<acyclic_visitor>::value,
                  "and destruction through the root must be well-defined");

    static_assert(!std::is_abstract<acyclic_visitor>::value,
                  "the root names no element type, so it is constructible");
    static_assert(!std::is_abstract<blind_visitor>::value,
                  "and a visitor handling nothing is legitimate");

    return true;
}

/*
tests_visitor_of_is_abstract
  Verifies the per-type acyclic interface's visit() is pure virtual, so a visitor
  claiming to handle an element must actually implement it.
  Tests the following:
  - visitor_of<E> and visitor_of<E, R> are abstract
  - they are polymorphic, which the dynamic_cast's target must be
  - visitor_of<E> is NOT related to acyclic_visitor: the interfaces are mixed in
    independently, which is what lets a visitor pick an arbitrary subset
*/
bool
tests_visitor_of_is_abstract()
{
    static_assert(std::is_abstract<visitor_of<acyclic_circle> >::value,
                  "visit() is pure virtual");
    static_assert(std::is_abstract<visitor_of<scored_node, int> >::value,
                  "for a non-void return type too");
    static_assert(std::is_polymorphic<visitor_of<acyclic_circle> >::value,
                  "and the dynamic_cast target must be polymorphic");

    static_assert(!std::is_base_of<acyclic_visitor,
                                   visitor_of<acyclic_circle> >::value,
                  "the per-type interface is INDEPENDENT of the root -- a visitor "
                  "mixes in whichever subset it wants");

    return true;
}

/*
tests_visitor_of_return_type_typedef
  Verifies the per-type interface advertises its return type.
  Tests the following:
  - visitor_of<E, R>::return_type is R
  - a different R gives a DIFFERENT interface type, so return type is part of the
    interface's identity -- which is why D_ACYCLIC_VISITABLE must name it when it
    casts
*/
bool
tests_visitor_of_return_type_typedef()
{
    static_assert(std::is_same<visitor_of<scored_node, int>::return_type,
                               int>::value,
                  "return_type is R");
    static_assert(std::is_same<visitor_of<acyclic_circle, void>::return_type,
                               void>::value,
                  "including void");

    static_assert(!std::is_same<visitor_of<scored_node, int>,
                                visitor_of<scored_node, void> >::value,
                  "R is part of the interface's identity");

    return true;
}

/*
tests_visitor_of_default_return_type_is_void
  Verifies visitor_of's return type defaults to void, and that the defaulted
  spelling names the same type the element's macro will look for.  If these
  diverged, the dynamic_cast would search for an interface the visitor does not
  have, and every dispatch would silently fall through to the default.
  Tests the following:
  - visitor_of<E> is visitor_of<E, void>
  - a void-returning element's visitor derives from exactly that type
*/
bool
tests_visitor_of_default_return_type_is_void()
{
    static_assert(std::is_same<visitor_of<acyclic_circle>,
                               visitor_of<acyclic_circle, void> >::value,
                  "the default R is void");

    static_assert(std::is_base_of<visitor_of<acyclic_circle, void>,
                                  round_visitor>::value,
                  "and that is the interface the element's macro searches for");

    return true;
}

/*
tests_acyclic_visitable_default_is_void
  Verifies acyclic_visitable is USABLE in its default form.  This is the shape of
  bug C: the class carried a second template parameter of the form
  `_ReturnType _DefaultReturn = _ReturnType()`, which is a NON-TYPE parameter whose
  type is _ReturnType -- and void is not a permitted type for one.  So
  acyclic_visitable<void>, both the default and the spelling the header's own usage
  comment gives, was ill-formed.  The parameter was never referenced by anything.
  Tests the following:
  - acyclic_visitable<> instantiates, and elements derive from it
  - acyclic_visitable<> and acyclic_visitable<void> are the SAME type, so the
    default argument is present and correct
  - the class takes exactly one template argument now, so the explicit spelling a
    caller would reach for is the one that works
*/
bool
tests_acyclic_visitable_default_is_void()
{
    static_assert(std::is_same<acyclic_visitable<>,
                               acyclic_visitable<void> >::value,
                  "the defaulted and explicit void spellings are one type");

    static_assert(std::is_base_of<acyclic_visitable<void>,
                                  acyclic_circle>::value,
                  "and elements really derive from it");
    static_assert(std::is_base_of<acyclic_visitable<>, acyclic_rect>::value,
                  "via either spelling");

    // and it is usable: an element of it constructs, accepts and dispatches
    acyclic_circle c;
    round_visitor  v;

    c.accept(v);

    return (c.seen == 1);
}

/*
tests_acyclic_visitable_return_type_typedef
  Verifies the element root advertises the return type accept() produces.
  Tests the following:
  - acyclic_visitable<R>::return_type is R, for void and for a value type
  - it agrees with the visitor_of<E, R> the element's macro will dispatch through
*/
bool
tests_acyclic_visitable_return_type_typedef()
{
    static_assert(std::is_same<acyclic_visitable<>::return_type, void>::value,
                  "the default return type is void");
    static_assert(std::is_same<acyclic_visitable<int>::return_type, int>::value,
                  "and a value type is carried through");

    static_assert(std::is_same<acyclic_visitable<int>::return_type,
                               visitor_of<scored_node, int>::return_type>::value,
                  "element and per-type interface agree on R");

    return true;
}

/*
tests_acyclic_visitable_is_abstract
  Verifies accept() is pure virtual, so an element cannot forget to implement it,
  and that the macro really satisfies it.
  Tests the following:
  - acyclic_visitable<R> is abstract
  - a concrete element expanding D_ACYCLIC_VISITABLE is not
*/
bool
tests_acyclic_visitable_is_abstract()
{
    static_assert(std::is_abstract<acyclic_visitable<> >::value,
                  "accept() is pure virtual");
    static_assert(std::is_abstract<acyclic_visitable<int> >::value,
                  "for every return type");

    static_assert(!std::is_abstract<acyclic_circle>::value,
                  "D_ACYCLIC_VISITABLE satisfies it (the macro's `override` would "
                  "have caught it otherwise)");
    static_assert(!std::is_abstract<scored_node>::value,
                  "including the non-void case");

    return true;
}

/*
tests_d_acyclic_visitable_dispatches_to_the_matching_interface
  Verifies accept() locates visitor_of<ThisType> on the visitor and calls it.
  Tests the following:
  - each handled element reaches its own visit(), and only its own
  - the visitor received the element BY REFERENCE, so its mutation is visible on
    the original object
  - dispatch works through the element root, where the static type says nothing
*/
bool
tests_d_acyclic_visitable_dispatches_to_the_matching_interface()
{
    acyclic_circle c;
    acyclic_rect   r;

    round_visitor v;

    c.accept(v);
    r.accept(v);

    bool ok = true;

    ok = ok && (v.circles == 1);
    ok = ok && (v.rects   == 1);

    // the element itself was mutated -- so visit() got *this, not a copy
    ok = ok && (c.seen == 1);
    ok = ok && (r.seen == 1);

    // and through the element root, whose static type carries no element identity
    acyclic_visitable<>* elem = &c;

    elem->accept(v);

    ok = ok && (v.circles == 2);
    ok = ok && (c.seen    == 2);
    ok = ok && (v.rects   == 1);    // still: the rect overload did not fire

    return ok;
}

/*
tests_d_acyclic_visitable_ignores_unhandled
  Verifies the OPEN-SET CONTRACT -- the property that distinguishes this strategy
  from the classic one.  round_visitor handles circles and rects and knows nothing
  of triangles.  Handing it a triangle is NOT an error: the element is silently
  skipped and accept() returns.

  This is the same scenario as the classic section's
  tests_d_visitable_unhandled_element_throws, with the opposite required outcome.
  Together the two tests are the two contracts.  Either one alone would be
  satisfied by a header that had them backwards.
  Tests the following:
  - accept() on an unhandled element does NOT throw
  - it leaves the element untouched -- nothing ran
  - it leaves the VISITOR untouched, so no other overload fired by mistake (a
    dynamic_cast that matched the wrong interface would show up here and nowhere
    else)
  - and the visitor still dispatches correctly afterwards for what it does handle
*/
bool
tests_d_acyclic_visitable_ignores_unhandled()
{
    acyclic_circle c;
    acyclic_tri    t;      // round_visitor has never heard of this type

    round_visitor v;

    // the unhandled element: no throw, no effect
    t.accept(v);

    bool ok = true;

    ok = ok && (t.seen    == 0);   // nothing ran on the element
    ok = ok && (v.circles == 0);   // and no OTHER overload fired by mistake
    ok = ok && (v.rects   == 0);

    // the visitor is undamaged and still works for what it handles
    c.accept(v);

    ok = ok && (v.circles == 1);
    ok = ok && (c.seen    == 1);

    // and skipping is repeatable, not a one-off
    t.accept(v);
    t.accept(v);

    ok = ok && (t.seen    == 0);
    ok = ok && (v.circles == 1);   // still untouched by the triangles

    return ok;
}

/*
tests_d_acyclic_visitable_ignores_everything_for_a_blind_visitor
  Verifies the degenerate case: a visitor that mixes in NO visitor_of<E> at all is
  legal, constructible, and inert.  This is the far end of the open set, and it has
  to work, because "the visitor handles a subset" includes the empty subset.
  Tests the following:
  - an acyclic_visitor with no interfaces at all constructs
  - every element accepts it without throwing
  - no element is touched
*/
bool
tests_d_acyclic_visitable_ignores_everything_for_a_blind_visitor()
{
    blind_visitor v;

    acyclic_circle c;
    acyclic_rect   r;
    acyclic_tri    t;

    c.accept(v);
    r.accept(v);
    t.accept(v);

    bool ok = true;

    ok = ok && (c.seen == 0);
    ok = ok && (r.seen == 0);
    ok = ok && (t.seen == 0);

    return ok;
}

/*
tests_d_acyclic_visitable_returns_default_when_unhandled
  Verifies the no-match path produces a VALUE-INITIALIZED return, not garbage.  For
  a void element "returns a default" is vacuous, so it is only observable on a
  non-void one -- and this is a different line of the macro from the dispatching
  path, reached only when the dynamic_cast yields null.
  Tests the following:
  - a non-void element accepted by a visitor that does not handle it returns
    _ReturnType(), i.e. 0 for int
  - the same element accepted by a visitor that DOES handle it returns a
    distinctive value, so the default is unmistakably a default and not a
    coincidence
*/
bool
tests_d_acyclic_visitable_returns_default_when_unhandled()
{
    scored_node n;

    blind_visitor blind;   // handles nothing at all

    bool ok = true;

    ok = ok && (n.accept(blind) == 0);      // value-initialized int

    // and the handled case returns something that could not be mistaken for it
    scorer s;

    ok = ok && (n.accept(s) == 42);
    ok = ok && (n.accept(s) != n.accept(blind));

    return ok;
}

/*
tests_d_acyclic_visitable_non_void_return
  Verifies a handled element's value is forwarded out of accept() unchanged.
  Tests the following:
  - accept() returns exactly what visit() produced
  - accept()'s static return type is the element's R, not something decayed or
    defaulted
*/
bool
tests_d_acyclic_visitable_non_void_return()
{
    scored_node n;
    scorer      s;

    static_assert(std::is_same<decltype(n.accept(s)), int>::value,
                  "accept() returns the element's R");
    static_assert(std::is_same<scored_node::return_type, int>::value,
                  "which is what the element advertises");

    const int via_accept = n.accept(s);
    const int via_visit  = s.visit(n);

    bool ok = true;

    ok = ok && (via_accept == 42);
    ok = ok && (via_accept == via_visit);

    return ok;
}

/*
tests_acyclic_visitor_is_decoupled_from_the_element_set
  Verifies the acyclicity itself -- the property the strategy is NAMED for.  The
  visitor root names no element type, so a new element type (and a visitor for it)
  can be introduced without touching, recompiling the declaration of, or even
  knowing about, anything that already exists.

  late_element and late_visitor stand in for that: they are declared in this
  translation unit, long after round_visitor, and round_visitor's declaration has
  no idea they exist.  Under the classic form this could not be expressed at all --
  concrete_visitor's element list would have had to name them.
  Tests the following:
  - the visitor ROOT is not parameterized on any element type
  - an existing visitor, which never heard of the new element, silently ignores it
    rather than failing to compile
  - a new visitor for the new element works, and ignores the OLD elements in turn
  - so the coupling really does run in neither direction
*/
bool
tests_acyclic_visitor_is_decoupled_from_the_element_set()
{
    // the root names no element type: that is the whole trick
    static_assert(!std::is_base_of<visitor_of<acyclic_circle>,
                                   acyclic_visitor>::value,
                  "the root knows no element type");

    late_element late;
    acyclic_circle old_elem;

    bool ok = true;

    // an OLD visitor, compiled without knowledge of late_element, simply skips it
    round_visitor old_visitor;

    late.accept(old_visitor);

    ok = ok && (late.seen           == 0);
    ok = ok && (old_visitor.circles == 0);
    ok = ok && (old_visitor.rects   == 0);

    // a NEW visitor handles the new element...
    late_visitor new_visitor;

    late.accept(new_visitor);

    ok = ok && (new_visitor.lates == 1);
    ok = ok && (late.seen         == 1);

    // ...and ignores the old ones, without either side having been modified
    old_elem.accept(new_visitor);

    ok = ok && (old_elem.seen     == 0);
    ok = ok && (new_visitor.lates == 1);

    // the old pairing still works, untouched by any of this
    old_elem.accept(old_visitor);

    ok = ok && (old_elem.seen       == 1);
    ok = ok && (old_visitor.circles == 1);

    return ok;
}

/*
tests_acyclic_visitor_accumulates_across_a_traversal
  Verifies a stateful acyclic visitor accumulates over a heterogeneous sequence,
  counting ONLY what it handles.  This is the open set exercised end to end: the
  sequence contains elements the visitor knows and elements it does not, and both
  outcomes have to be right at once.
  Tests the following:
  - the visitor tallies each handled element type independently and correctly
  - the unhandled elements contribute nothing to any tally
  - the handled tallies sum to the number of handled elements, not to the length of
    the traversal -- so the skipped ones were skipped, not miscounted as something
*/
bool
tests_acyclic_visitor_accumulates_across_a_traversal()
{
    acyclic_circle c1;
    acyclic_circle c2;
    acyclic_rect   r1;
    acyclic_tri    t1;     // unhandled
    acyclic_tri    t2;     // unhandled

    acyclic_visitable<>* elems[5];

    elems[0] = &c1;
    elems[1] = &t1;
    elems[2] = &r1;
    elems[3] = &t2;
    elems[4] = &c2;

    round_visitor v;

    for (int i = 0; i < 5; ++i)
    {
        elems[i]->accept(v);
    }

    bool ok = true;

    ok = ok && (v.circles == 2);
    ok = ok && (v.rects   == 1);

    // the unhandled ones contributed nothing at all
    ok = ok && (t1.seen == 0);
    ok = ok && (t2.seen == 0);

    // and the tallies sum to the HANDLED count (3), not the traversal length (5)
    ok = ok && ((v.circles + v.rects) == 3);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
