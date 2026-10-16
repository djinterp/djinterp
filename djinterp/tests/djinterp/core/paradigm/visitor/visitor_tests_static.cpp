/*******************************************************************************
* djinterp [core]                                       visitor_tests_static.cpp
*
*
* path:      /tests/djinterp/core/paradigm/visitor/visitor_tests_static.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : visitor_tests_static.cpp
//   Section IV: the static visitor.  CRTP, and no vtable anywhere.
//
//   The two runtime strategies pay for their flexibility with an indirect call and
// (in both cases, once the classic form's cross-cast is spelled correctly) an RTTI
// lookup per dispatch.  The static visitor pays neither: the derived visitor's
// type is a template parameter, so `static_cast<_Derived*>(this)->visit(_element)`
// resolves at compile time and the whole hierarchy can be devirtualized away.
// tests_static_visitor_has_no_vtable pins that directly -- if a static visitor were
// ever polymorphic, the strategy would have lost the only thing it is chosen for.
//
//   The consequence is that "the visitor does not handle this element" is no longer
// a runtime outcome at all.  There is no default to return and no exception to
// throw: the call simply does not compile.  That makes visit_result the load-
// bearing piece of the section -- it is the SFINAE-friendly detector that decides
// whether apply() even exists for a given visitor/element pair, and
// tests_visit_result_is_sfinae_friendly checks it reports absence as a missing
// ::type rather than a hard error.  A detector that hard-errored would take the
// whole overload set down with it.
//
//   The other thing worth pinning is apply()'s three overloads.  They differ only
// in constness -- of the element, and of the visitor -- and the fixtures return
// four DIFFERENT strings precisely so a mis-selection is visible.  A visitor whose
// overloads all returned the same thing would pass a dispatch test while apply()
// picked the wrong one every single time.

// std
#include <string>
#include <type_traits>
// djinterp
#include "visitor_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    // has_visit_result
    //   trait: does internal::visit_result<V, E> have a ::type?  Detected rather
    // than asserted, so that "no, it does not" is an ANSWER and not a compile
    // error -- which is the very property under test.
    template<typename _Visitor,
             typename _Element,
             typename = void>
    struct has_visit_result
        : std::false_type
    {};

    template<typename _Visitor,
             typename _Element>
    struct has_visit_result<
        _Visitor,
        _Element,
        void_t<typename internal::visit_result<_Visitor, _Element>::type> >
        : std::true_type
    {};

    // has_apply
    //   trait: is v.apply(e) a valid call?  The static form's answer to "this
    // visitor cannot handle that element" is that apply() does not exist, so the
    // question has to be asked in a detectable way.
    template<typename _Visitor,
             typename _Element,
             typename = void>
    struct has_apply
        : std::false_type
    {};

    template<typename _Visitor,
             typename _Element>
    struct has_apply<
        _Visitor,
        _Element,
        void_t<decltype(std::declval<_Visitor&>().apply(
            std::declval<_Element&>()))> >
        : std::true_type
    {};
}


/*
tests_visit_result_deduces_the_return_type
  Verifies visit_result<V, E>::type is exactly decltype(v.visit(e)).
  Tests the following:
  - a string-returning visitor deduces std::string
  - an int-returning visitor over the SAME element deduces int, so the deduction
    follows the visitor and not the element
  - the same visitor over two different elements agrees, since both its overloads
    return the same type
*/
bool
tests_visit_result_deduces_the_return_type()
{
    static_assert(std::is_same<
                      internal::visit_result<tag_visitor, static_circle>::type,
                      std::string>::value,
                  "deduced from the visitor's visit()");
    static_assert(std::is_same<
                      internal::visit_result<id_visitor, static_circle>::type,
                      int>::value,
                  "a different visitor over the SAME element deduces its own type");
    static_assert(std::is_same<
                      internal::visit_result<id_visitor, static_rect>::type,
                      int>::value,
                  "and the same visitor agrees across its elements");

    return true;
}

/*
tests_visit_result_is_sfinae_friendly
  Verifies an unhandled element yields NO ::type rather than a hard error.  This is
  load-bearing: visit_result appears in the return type of every apply() and
  accept() overload, so if it hard-errored for an unhandled pair it would take the
  entire overload set down with it rather than simply removing the one candidate.
  Under this strategy "the visitor cannot handle that" IS a substitution failure --
  there is no runtime path for it at all.
  Tests the following:
  - visit_result<V, E> has a ::type for a handled element
  - it has NO ::type for an unhandled one, and asking is not an error
  - consequently apply() does not exist for the unhandled pair, and asking THAT is
    not an error either
*/
bool
tests_visit_result_is_sfinae_friendly()
{
    static_assert(has_visit_result<id_visitor, static_circle>::value,
                  "a handled element has a deduced result");
    static_assert(has_visit_result<id_visitor, static_rect>::value,
                  "for each handled element");

    static_assert(!has_visit_result<id_visitor, static_tri>::value,
                  "an UNHANDLED element has no ::type -- and asking is not an error");

    // and the consequence: apply() simply is not there for that pair
    static_assert(has_apply<id_visitor, static_circle>::value,
                  "apply() exists for a handled element");
    static_assert(!has_apply<id_visitor, static_tri>::value,
                  "and does not for an unhandled one");

    return ( has_visit_result<id_visitor, static_circle>::value &&
            !has_visit_result<id_visitor, static_tri>::value );
}

/*
tests_visit_result_t_alias_agrees
  Verifies the alias is the trait, with no drift between the two spellings.
  Tests the following:
  - visit_result_t<V, E> is visit_result<V, E>::type, for several pairs
*/
bool
tests_visit_result_t_alias_agrees()
{
    static_assert(std::is_same<
                      internal::visit_result_t<tag_visitor, static_circle>,
                      internal::visit_result<tag_visitor, static_circle>::type
                  >::value,
                  "_t is the trait's ::type");
    static_assert(std::is_same<
                      internal::visit_result_t<id_visitor, static_rect>,
                      internal::visit_result<id_visitor, static_rect>::type
                  >::value,
                  "for every pair");

    return true;
}

/*
tests_visit_result_distinguishes_const_elements
  Verifies V/E and V/const E are deduced INDEPENDENTLY.  apply()'s three overloads
  each name a different one of these, so if the trait collapsed them the overloads
  would all report the same return type and the const-ness of the selected visit()
  would become unobservable.
  Tests the following:
  - visit_result<V, E> and visit_result<V, const E> both resolve for tag_visitor,
    which has a distinct overload for each
  - const_tag_visitor, whose visit() is const-qualified, resolves under a CONST
    visitor -- declval<const V>() cannot call a non-const member, so this is
    exactly what separates apply()'s third overload from its second
  - and a non-const-callable visitor does NOT resolve under a const visitor
*/
bool
tests_visit_result_distinguishes_const_elements()
{
    // tag_visitor declares visit(E&) and visit(const E&) separately
    static_assert(has_visit_result<tag_visitor, static_circle>::value,
                  "the mutable element resolves");
    static_assert(has_visit_result<tag_visitor, const static_circle>::value,
                  "and so does the const element, via its own overload");

    // const_tag_visitor's visit() is const-qualified: a CONST visitor can call it
    static_assert(has_visit_result<const const_tag_visitor,
                                   const static_circle>::value,
                  "a const visitor can reach a const-qualified visit()");

    // ...but tag_visitor's visit() is not const-qualified, so a const visitor
    // cannot.  This is precisely the distinction apply()'s third overload rests on.
    static_assert(!has_visit_result<const tag_visitor,
                                    const static_circle>::value,
                  "and cannot reach a non-const one");

    return true;
}

/*
tests_static_visitor_apply_dispatches_by_type
  Verifies apply() reaches the overload for the element's static type.  The
  fixtures return DIFFERENT tags per element on purpose: a visitor whose overloads
  returned the same value would pass this test while apply() dispatched to the
  wrong one every time.
  Tests the following:
  - apply(circle) and apply(rect) reach their own overloads
  - the two results differ
  - a second visitor over the same elements dispatches independently
*/
bool
tests_static_visitor_apply_dispatches_by_type()
{
    static_circle c;
    static_rect   r;

    tag_visitor v;

    bool ok = true;

    ok = ok && (v.apply(c) == "circle/mut");
    ok = ok && (v.apply(r) == "rect/mut");
    ok = ok && (v.apply(c) != v.apply(r));

    // a different visitor over the same elements
    id_visitor iv;

    ok = ok && (iv.apply(c) == 10);   // c.id * 10
    ok = ok && (iv.apply(r) == 20);   // r.id * 10
    ok = ok && (iv.apply(c) != iv.apply(r));

    return ok;
}

/*
tests_static_visitor_apply_selects_const_element_overload
  Verifies a CONST element selects visit(const E&).  apply()'s first two overloads
  differ only in the element's constness; partial ordering picks the second for a
  const argument, and the tags make the choice observable.
  Tests the following:
  - a const element reaches visit(const E&), not visit(E&)
  - a non-const element still reaches visit(E&), so the const overload did not
    swallow both
  - and this holds for each element type independently
*/
bool
tests_static_visitor_apply_selects_const_element_overload()
{
    const static_circle cc;
    const static_rect   cr;

    static_circle mc;

    tag_visitor v;

    bool ok = true;

    ok = ok && (v.apply(cc) == "circle/const-elem");
    ok = ok && (v.apply(cr) == "rect/const-elem");

    // the mutable element still goes to the mutable overload
    ok = ok && (v.apply(mc) == "circle/mut");
    ok = ok && (v.apply(mc) != v.apply(cc));

    return ok;
}

/*
tests_static_visitor_apply_selects_const_visitor_overload
  Verifies a CONST VISITOR selects the third overload -- apply(const E&) const --
  which is the only one whose `this` is const and therefore the only one that can
  reach a const-qualified visit().  This is a different axis from the element's
  constness, and the two are easy to conflate.
  Tests the following:
  - a const visitor over a const element dispatches, and reaches the const-
    qualified visit()
  - a NON-const visitor of the same type reaches the same const-qualified visit()
    (its only one), so the third overload is not the sole path to it
  - a visitor whose visit() is NOT const-qualified cannot be used const at all,
    which is what makes the third overload distinct rather than redundant
*/
bool
tests_static_visitor_apply_selects_const_visitor_overload()
{
    const static_circle cc;
    const static_rect   cr;

    const const_tag_visitor cv;

    bool ok = true;

    ok = ok && (cv.apply(cc) == "circle/const-visitor");
    ok = ok && (cv.apply(cr) == "rect/const-visitor");

    // a non-const object of the same visitor type reaches the same visit()
    const_tag_visitor mv;

    ok = ok && (mv.apply(cc) == "circle/const-visitor");

    // and the distinctness of the third overload: a visitor with only non-const
    // visit()s cannot be applied const.  If overload three did not exist, or if it
    // did not qualify _Derived as const, this would be indistinguishable.
    static_assert(!has_apply<const tag_visitor, const static_circle>::value,
                  "a non-const-callable visitor cannot be applied const");
    static_assert(has_apply<const const_tag_visitor, const static_circle>::value,
                  "while a const-callable one can");

    return ok;
}

/*
tests_static_visitor_return_type_follows_the_visitor
  Verifies the return type is the VISITOR's, not something fixed by the base.  The
  base is parameterized only on _Derived, and every return type is deduced through
  visit_result, so two visitors over identical elements may produce entirely
  different types.
  Tests the following:
  - apply()'s static return type is what the visitor's visit() returns
  - two visitors over the same element produce different return types
  - the values agree with calling visit() directly
*/
bool
tests_static_visitor_return_type_follows_the_visitor()
{
    static_circle c;

    tag_visitor tv;
    id_visitor  iv;

    static_assert(std::is_same<decltype(tv.apply(c)), std::string>::value,
                  "the string visitor's apply() returns std::string");
    static_assert(std::is_same<decltype(iv.apply(c)), int>::value,
                  "the int visitor's apply() returns int");
    static_assert(!std::is_same<decltype(tv.apply(c)),
                                decltype(iv.apply(c))>::value,
                  "so the base fixes nothing");

    bool ok = true;

    ok = ok && (tv.apply(c) == tv.visit(c));
    ok = ok && (iv.apply(c) == iv.visit(c));

    return ok;
}

/*
tests_static_visitor_has_no_vtable
  Verifies the CRTP payoff: a static visitor is NOT polymorphic.  This is the only
  reason to choose this strategy over the other three -- dispatch resolves at
  compile time, there is no vtable pointer in the object and no indirect call at
  the site.  A static_visitor that had somehow acquired a virtual function would
  still pass every dispatch test in this file while having silently given up the
  entire point.
  Tests the following:
  - static_visitor<D> and its derived visitors are NOT polymorphic
  - static_visitable<D> and its derived elements are NOT polymorphic
  - by contrast, the two runtime roots ARE -- which is what makes the comparison
    mean something rather than being a vacuous check on an empty class
*/
bool
tests_static_visitor_has_no_vtable()
{
    static_assert(!std::is_polymorphic<static_visitor<tag_visitor> >::value,
                  "the CRTP visitor base has no vtable");
    static_assert(!std::is_polymorphic<tag_visitor>::value,
                  "nor does a derived static visitor");
    static_assert(!std::is_polymorphic<id_visitor>::value,
                  "nor any of them");

    static_assert(!std::is_polymorphic<static_visitable<static_circle> >::value,
                  "the CRTP element base has no vtable either");
    static_assert(!std::is_polymorphic<static_circle>::value,
                  "nor does a derived static element");

    // the comparison that gives the above its meaning: the RUNTIME roots do have
    // vtables, because they must
    static_assert(std::is_polymorphic<visitor_base<void> >::value,
                  "the classic root is polymorphic");
    static_assert(std::is_polymorphic<acyclic_visitor>::value,
                  "and so is the acyclic root");

    return true;
}

/*
tests_static_visitable_accept_forwards_to_apply
  Verifies element.accept(v) is v.apply(element) -- accept() exists only to put the
  call the other way round, and must not change the outcome.
  Tests the following:
  - accept() and apply() produce the same value, for each element
  - accept() dispatches by element type, i.e. it forwards the DERIVED type through
    the CRTP static_cast rather than the base
*/
bool
tests_static_visitable_accept_forwards_to_apply()
{
    static_circle c;
    static_rect   r;

    tag_visitor v;

    bool ok = true;

    ok = ok && (c.accept(v) == v.apply(c));
    ok = ok && (r.accept(v) == v.apply(r));

    // and it really did dispatch by element type
    ok = ok && (c.accept(v) == "circle/mut");
    ok = ok && (r.accept(v) == "rect/mut");
    ok = ok && (c.accept(v) != r.accept(v));

    // with a different visitor too
    id_visitor iv;

    ok = ok && (c.accept(iv) == 10);
    ok = ok && (r.accept(iv) == 20);

    return ok;
}

/*
tests_static_visitable_accept_const
  Verifies a const element forwards through accept()'s const overload, which
  static_casts to `const _Derived&` and therefore selects the const-element visit().
  Tests the following:
  - a const element's accept() reaches visit(const E&)
  - it agrees with apply() on the same const element
  - a non-const element still reaches the mutable overload, so the const accept()
    did not swallow both
*/
bool
tests_static_visitable_accept_const()
{
    const static_circle cc;
    const static_rect   cr;

    static_circle mc;

    tag_visitor v;

    bool ok = true;

    ok = ok && (cc.accept(v) == "circle/const-elem");
    ok = ok && (cr.accept(v) == "rect/const-elem");

    // accept() and apply() agree on the const element
    ok = ok && (cc.accept(v) == v.apply(cc));

    // and the mutable element is unaffected
    ok = ok && (mc.accept(v) == "circle/mut");
    ok = ok && (mc.accept(v) != cc.accept(v));

    return ok;
}

/*
tests_static_visitable_accept_return_type
  Verifies accept() reports the visitor's return type -- so an element can be
  visited by visitors of entirely different result types without the element
  knowing anything about them.
  Tests the following:
  - accept()'s static return type follows the visitor
  - the const overload's return type follows the CONST-element deduction, which may
    differ from the mutable one
*/
bool
tests_static_visitable_accept_return_type()
{
    static_circle       mc;
    const static_circle cc;

    tag_visitor tv;
    id_visitor  iv;

    static_assert(std::is_same<decltype(mc.accept(tv)), std::string>::value,
                  "accept() follows the visitor's return type");
    static_assert(std::is_same<decltype(mc.accept(iv)), int>::value,
                  "and a different visitor gives a different type");

    static_assert(std::is_same<decltype(cc.accept(tv)), std::string>::value,
                  "the const overload deduces through const _Derived");

    // the const overload's deduction is the CONST-element one, which is a distinct
    // query even when it happens to land on the same type
    static_assert(std::is_same<
                      decltype(cc.accept(tv)),
                      internal::visit_result_t<tag_visitor,
                                               const static_circle> >::value,
                  "and it is visit_result_t<V, const D>, not <V, D>");

    return true;
}

/*
tests_static_visitor_dispatch_is_compile_time
  Verifies the strategy's defining property: an unhandled element is a COMPILE-TIME
  fact.  There is no default return and no exception -- the two runtime strategies'
  answers to this question -- because there is no runtime dispatch at all.  The call
  simply does not exist.
  Tests the following:
  - apply() and accept() exist for every handled element
  - NEITHER exists for an unhandled one, and asking is a detectable non-answer
    rather than an error
  - the contrast with the runtime forms is real: the acyclic visitor answers the
    same question with a value at RUNTIME, which is only possible because it has a
    runtime dispatch to fail
*/
bool
tests_static_visitor_dispatch_is_compile_time()
{
    // handled: the call exists
    static_assert(has_apply<id_visitor, static_circle>::value,
                  "apply() exists for a handled element");
    static_assert(has_apply<tag_visitor, static_rect>::value,
                  "for each visitor/element pair that is handled");

    // unhandled: the call does NOT exist.  No default, no throw -- no function.
    static_assert(!has_apply<id_visitor, static_tri>::value,
                  "and does not exist at all for an unhandled one");
    static_assert(!has_apply<tag_visitor, static_tri>::value,
                  "for any visitor that does not handle it");

    // the same fact seen through the trait the overloads are built on
    static_assert(!has_visit_result<tag_visitor, static_tri>::value,
                  "because visit_result has no answer for the pair");

    // and the contrast: the acyclic form answers the identical question at RUNTIME,
    // with a value, because it HAS a runtime dispatch to fail.  This one does not.
    {
        scored_node   n;
        blind_visitor blind;

        if (n.accept(blind) != 0)
        {
            return false;
        }
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
