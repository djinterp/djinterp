/******************************************************************************
* djinterp [test]                                        profunctor_tests.hpp
*
*   Unit-test declarations for core/functional/profunctor.hpp.  One declaration
* group per section of the module under test:
*
*     profunctor_tests_arrow.cpp       -- I.   profn<F> + make_profn
*     profunctor_tests_protocol.cpp    -- II.  profunctor_traits, is_profunctor,
*                                               the internal helpers, the concept
*     profunctor_tests_operations.cpp  -- III. dimap / lmap / rmap and the laws
*     profunctor_tests_instance.cpp    -- IV.  the profn<F> instance
*
*   FIXTURES.  The generic operations dispatch through profunctor_traits, so to
* prove they are not hard-wired to profn the suite also defines a SECOND,
* distinct profunctor instance, pf_arrow<F> (its own carrier, make function, and
* profunctor_traits::dimap).  A handful of constexpr function objects (doubler,
* add_one, add_ten, even_p, b2i) drive the conditional-constexpr paths under
* static_assert -- including input/output type changes at compile time -- while
* runtime tests use lambdas.  Two negatives pin the detector: not_profunctor
* (no specialisation) and pf_no_marker (a profunctor_traits WITHOUT the
* is_specialized marker, which must read as not-a-profunctor).
*
*   Profunctor equality is behavioural: two arrows are "the same" when they
* agree on sample inputs (a profn carries a callable and has no operator==), so
* the law tests evaluate both sides at several points.
*
*   All tests are flat in djinterp::testing.
*
* path:      /inc/djinterp/test/functional/profunctor_tests.hpp
* link(s):   TBA
* author(s): teer                                          created: 2026.07.11
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    FIXTURES  (constexpr functors, negatives, the second instance, traits)
I.    ARROW WRAPPER
II.   PROTOCOL + DETECTION
III.  OPERATIONS (dimap / lmap / rmap)
IV.   INSTANCE (profn<F>)
*/


#ifndef DJINTERP_TEST_PROFUNCTOR_TESTS_
#define DJINTERP_TEST_PROFUNCTOR_TESTS_ 1

// std
#include <string>
#include <type_traits>
#include <utility>
// djinterp (module under test)
#include "../../core/functional/profunctor.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///             0.    FIXTURES                                              ///
///////////////////////////////////////////////////////////////////////////////

// -- constexpr function objects (drive the constexpr paths) ---------------

// doubler / add_one / add_ten: int -> int, constexpr.
struct doubler
{
    constexpr int operator()(int _x) const { return _x * 2; }
};

struct add_one
{
    constexpr int operator()(int _x) const { return _x + 1; }
};

struct add_ten
{
    constexpr int operator()(int _x) const { return _x + 10; }
};

// even_p: int -> bool, constexpr -- a covariant (output) type change.
struct even_p
{
    constexpr bool operator()(int _x) const { return (_x % 2) == 0; }
};

// b2i: bool -> int, constexpr -- a contravariant (input) type change.
struct b2i
{
    constexpr int operator()(bool _b) const { return _b ? 10 : 0; }
};


// -- negatives ------------------------------------------------------------

// not_profunctor: no profunctor_traits specialisation at all.
struct not_profunctor
{
};

// pf_no_marker: a profunctor_traits specialisation WITHOUT is_specialized
// (defined below at djinterp scope) -- must read as not-a-profunctor.
struct pf_no_marker
{
};


// -- a second, distinct profunctor instance (generic-dispatch witness) ----

// pf_arrow
//   fixture profunctor: a parallel arrow wrapper, unrelated to profn, so that
// dimap / lmap / rmap can be shown to dispatch through the trait rather than
// being wired to profn.
template<typename _Fn>
struct pf_arrow
{
    _Fn f;

    template<typename _Arg>
    D_CONSTEXPR
    auto operator()(
        _Arg&& _arg
    ) const
    -> decltype(f(std::forward<_Arg>(_arg)))
    {
        return f(std::forward<_Arg>(_arg));
    }
};

template<typename _Fn>
D_CONSTEXPR
pf_arrow<typename std::decay<_Fn>::type>
make_pf_arrow(
    _Fn&& _f
)
{
    return pf_arrow<typename std::decay<_Fn>::type>{ std::forward<_Fn>(_f) };
}

// pf_compose: the composed callable post . fn . pre behind pf_arrow's dimap.
template<typename _Pre,
         typename _Fn,
         typename _Post>
struct pf_compose
{
    _Pre  pre;
    _Fn   fn;
    _Post post;

    template<typename _Arg>
    D_CONSTEXPR
    auto operator()(
        _Arg&& _arg
    ) const
    -> decltype(post(fn(pre(std::forward<_Arg>(_arg)))))
    {
        return post(fn(pre(std::forward<_Arg>(_arg))));
    }
};


// -- helper trait: is the result of dimap still a profn? ------------------

// is_profn_type
//   trait: true exactly for profn<F> -- used to confirm dimap returns a fresh
// arrow WITHOUT unwrapping.
template<typename _Type>
struct is_profn_type : std::false_type
{
};

template<typename _Fn>
struct is_profn_type<profn<_Fn> > : std::true_type
{
};

// fwd_probe
//   fixture callable: reports which reference category it was called with, so
// profn's perfect-forwarding of the argument can be observed.
struct fwd_probe
{
    int operator()(int& ) const       { return 1; }   // lvalue
    int operator()(int&&) const       { return 2; }   // rvalue
    int operator()(const int&) const  { return 3; }   // const lvalue
};


///////////////////////////////////////////////////////////////////////////////
///             I.    ARROW WRAPPER                                         ///
///////////////////////////////////////////////////////////////////////////////

bool tests_profn_construct();
bool tests_profn_call();
bool tests_make_profn_decay();
bool tests_profn_forwarding();
bool tests_profn_constexpr();
bool tests_profn_return_type();


///////////////////////////////////////////////////////////////////////////////
///             II.   PROTOCOL + DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

bool tests_is_profunctor_positive();
bool tests_is_profunctor_negative();
bool tests_is_profunctor_cvref();
bool tests_is_profunctor_requires_marker();
bool tests_profunctor_traits_members();
bool tests_profunctor_concept();
bool tests_internal_helpers();


///////////////////////////////////////////////////////////////////////////////
///             III.  OPERATIONS (dimap / lmap / rmap)                      ///
///////////////////////////////////////////////////////////////////////////////

bool tests_dimap_both_ends();
bool tests_dimap_type_change();
bool tests_lmap_input_only();
bool tests_rmap_output_only();
bool tests_law_identity();
bool tests_law_composition();
bool tests_rmap_covariant_composition();
bool tests_lmap_contravariant_composition();
bool tests_dimap_equals_lmap_rmap();
bool tests_operations_constexpr();


///////////////////////////////////////////////////////////////////////////////
///             IV.   INSTANCE (profn<F>)                                   ///
///////////////////////////////////////////////////////////////////////////////

bool tests_instance_composition_order();
bool tests_instance_returns_profn();
bool tests_instance_composable();
bool tests_instance_preserves_original();
bool tests_instance_is_specialized();
bool tests_instance_generic_dispatch();


NS_END  // testing


///////////////////////////////////////////////////////////////////////////////
///             FIXTURE PROFUNCTOR INSTANCES  (djinterp scope)              ///
///////////////////////////////////////////////////////////////////////////////

// profunctor_traits<pf_arrow<_Fn>>  -- the second instance: dimap composes a
// fresh pf_arrow around post . fn . pre, exactly as profn does for itself.
template<typename _Fn>
struct profunctor_traits<testing::pf_arrow<_Fn>, void>
{
    using is_specialized = std::true_type;

    template<typename _Pre,
             typename _Post>
    static
    D_CONSTEXPR
    testing::pf_arrow<testing::pf_compose<_Pre, _Fn, _Post> >
    dimap(
        const testing::pf_arrow<_Fn>& _p,
        _Pre                          _pre,
        _Post                         _post
    )
    {
        return testing::make_pf_arrow(
            testing::pf_compose<_Pre, _Fn, _Post>{ _pre, _p.f, _post });
    }
};

// profunctor_traits<pf_no_marker>  -- present but WITHOUT the is_specialized
// marker, so is_profunctor<pf_no_marker> must be false.
template<>
struct profunctor_traits<testing::pf_no_marker, void>
{
};


NS_END  // djinterp


#endif  // DJINTERP_TEST_PROFUNCTOR_TESTS_
