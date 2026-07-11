/******************************************************************************
* djinterp [test]                                    comonad_tests_extend.cpp
*
*   Section II.2 of the comonad.hpp suite: extend, the co-bind, which recomputes
* the focus from the WHOLE context (f : W<A> -> B), keeping the environment, to
* produce W<B>.  Covers focus recomputation with the environment preserved, the
* proof that f is handed the whole context (a function that reads the
* environment), a focus-type change, the three comonad laws (extend(extract) ==
* id, extract . extend(f) == f, and co-Kleisli associativity), forwarding, and
* the result types -- over std::pair, kv_pair (field-level, key-only equality),
* and the custom Identity comonad.
*
* path:      /tests/djinterp/core/functional/comonad_tests_extend.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "comonad_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                extend OVER THE Env INSTANCES + custom                    ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_extend_pair_recompute
  Tests the following:
  - extend recomputes the focus and keeps the environment: extend(("ctx",10),
    focus*2) == ("ctx", 20).
*/
static bool
tests_extend_pair_recompute()
{
    const std::pair<std::string, int> r =
        ::djinterp::extend(std::pair<std::string, int>("ctx", 10), co_focus_x2());

    return (r == std::make_pair(std::string("ctx"), 20));
}

/*
tests_extend_pair_preserves_env
  Tests the following:
  - the environment (first component) is preserved unchanged by extend.
*/
static bool
tests_extend_pair_preserves_env()
{
    const std::pair<std::string, int> r =
        ::djinterp::extend(std::pair<std::string, int>("ctx", 10), co_focus_x2());

    return (r.first == "ctx");
}

/*
tests_extend_pair_sees_env
  Tests the following:
  - extend hands the WHOLE context to f: a function of both focus and
    environment (focus + env length) recomputes accordingly.  ("ab",10) ->
    ("ab", 12), since 10 + len("ab") == 12.
*/
static bool
tests_extend_pair_sees_env()
{
    const std::pair<std::string, int> r =
        ::djinterp::extend(std::pair<std::string, int>("ab", 10), pair_env_focus());

    return (r == std::make_pair(std::string("ab"), 12));
}

/*
tests_extend_pair_type_change
  Tests the following:
  - extend may change the focus type while keeping the environment: focus
    int->string yields ("ctx", "10").
*/
static bool
tests_extend_pair_type_change()
{
    const std::pair<std::string, std::string> r =
        ::djinterp::extend(std::pair<std::string, int>("ctx", 10), co_focus_show());

    return (r == std::make_pair(std::string("ctx"), std::string("10")));
}

/*
tests_extend_kv_recompute
  Tests the following:
  - extend over kv keeps the key (environment) and recomputes the value
    (focus): kv(3,10) with focus*2 has m_key 3, m_value 20 (field-level, since
    kv equality is key-only).
*/
static bool
tests_extend_kv_recompute()
{
    const ::djinterp::kv_pair<int, int> r =
        ::djinterp::extend(::djinterp::kv_pair<int, int>(3, 10), co_focus_x2());

    return ( (r.m_key == 3) &&
             (r.m_value == 20) );
}

/*
tests_extend_kv_sees_key
  Tests the following:
  - extend over kv hands the whole pair to f: value + key recomputes the value.
    kv(3,10) -> m_value 13, m_key 3.
*/
static bool
tests_extend_kv_sees_key()
{
    const ::djinterp::kv_pair<int, int> r =
        ::djinterp::extend(::djinterp::kv_pair<int, int>(3, 10), kv_key_focus());

    return ( (r.m_key == 3) &&
             (r.m_value == 13) );
}

/*
tests_extend_kv_type_change
  Tests the following:
  - extend over kv may change the value type: kv(3,10) with focus int->string
    has m_key 3, m_value "10".
*/
static bool
tests_extend_kv_type_change()
{
    const ::djinterp::kv_pair<int, std::string> r =
        ::djinterp::extend(::djinterp::kv_pair<int, int>(3, 10), co_focus_show());

    return ( (r.m_key == 3) &&
             (r.m_value == "10") );
}

/*
tests_extend_ident
  Tests the following:
  - extend over the Identity comonad recomputes its focus: ident(5) with
    focus*2 == ident(10).
*/
static bool
tests_extend_ident()
{
    const ident<int> r = ::djinterp::extend(ident<int>(5), co_focus_x2());

    return (r == ident<int>(10));
}


///////////////////////////////////////////////////////////////////////////////
///                COMONAD LAWS                                              ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_extend_left_identity
  Tests the following:
  - the comonad LEFT IDENTITY: extend(w, extract) == w, over pair, ident, and kv
    (kv checked at the field level under its key-only equality).
*/
static bool
tests_extend_left_identity()
{
    const std::pair<std::string, int> wp("ctx", 10);
    const ident<int>                  wi(7);
    const ::djinterp::kv_pair<int, int> wk(3, 10);

    const bool pair_ok  = (::djinterp::extend(wp, co_extract()) == wp);
    const bool ident_ok = (::djinterp::extend(wi, co_extract()) == wi);

    const ::djinterp::kv_pair<int, int> rk = ::djinterp::extend(wk, co_extract());
    const bool kv_ok = ( (rk.m_key == 3) &&
                         (rk.m_value == 10) );

    return (pair_ok && ident_ok && kv_ok);
}

/*
tests_extend_right_identity
  Tests the following:
  - the comonad law extract . extend(f) == f: the focus of an extended context
    is f applied to the whole original context.  focus*2 of ("ctx",10) is 20.
*/
static bool
tests_extend_right_identity()
{
    const std::pair<std::string, int> w("ctx", 10);

    const int focus_after = ::djinterp::extract(::djinterp::extend(w, co_focus_x2()));
    const int f_of_whole  = co_focus_x2()(w);

    return ( (focus_after == f_of_whole) &&
             (focus_after == 20) );
}

/*
tests_extend_associativity
  Tests the following:
  - co-Kleisli associativity: extend(extend(w, g), f) == extend(w, x -> f(extend(
    x, g))).  With g = focus*2 and f = focus+1 over ("c",10), both give
    ("c", 21).
*/
static bool
tests_extend_associativity()
{
    const std::pair<std::string, int> w("c", 10);

    const std::pair<std::string, int> lhs =
        ::djinterp::extend(::djinterp::extend(w, co_focus_x2()), co_focus_plus1());

    const std::pair<std::string, int> rhs =
        ::djinterp::extend(
            w,
            [](const std::pair<std::string, int>& _x) -> int
            {
                return co_focus_plus1()(::djinterp::extend(_x, co_focus_x2()));
            });

    return ( (lhs == rhs) &&
             (lhs == std::make_pair(std::string("c"), 21)) );
}


///////////////////////////////////////////////////////////////////////////////
///                FORWARDING + RESULT TYPES                                 ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_extend_forwarding
  Tests the following:
  - extend accepts a named lvalue comonad and an rvalue (temporary) one.
*/
static bool
tests_extend_forwarding()
{
    std::pair<std::string, int> w("ctx", 10);

    const std::pair<std::string, int> r_lvalue =
        ::djinterp::extend(w, co_focus_x2());
    const std::pair<std::string, int> r_rvalue =
        ::djinterp::extend(std::pair<std::string, int>("ctx", 10), co_focus_x2());

    return ( (r_lvalue == std::make_pair(std::string("ctx"), 20)) &&
             (r_rvalue == std::make_pair(std::string("ctx"), 20)) );
}

/*
tests_extend_result_type_pair
  Tests the following:
  - the result type of extend over pair is pair<Env, mapped_focus>.
*/
static bool
tests_extend_result_type_pair()
{
    return std::is_same<
        decltype(::djinterp::extend(
            std::declval< const std::pair<std::string, int>& >(),
            std::declval< co_focus_show >())),
        std::pair<std::string, std::string> >::value;
}

/*
tests_extend_result_type_kv
  Tests the following:
  - the result type of extend over kv is kv_pair<Key, mapped_focus>.
*/
static bool
tests_extend_result_type_kv()
{
    return std::is_same<
        decltype(::djinterp::extend(
            std::declval< const ::djinterp::kv_pair<int, int>& >(),
            std::declval< co_focus_show >())),
        ::djinterp::kv_pair<int, std::string> >::value;
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
comonad_extend_block()
{
    dt::block_spec block;

    block.name       = "II.2 extend";
    block.descriptor =
        "co-bind: recompute focus, keep env, sees whole context, comonad laws";

    block.tests.push_back(dt::test_spec{
        "pair: recompute focus",
        "focus recomputed, env kept",
        &tests_extend_pair_recompute });

    block.tests.push_back(dt::test_spec{
        "pair: preserves env",
        "first component unchanged",
        &tests_extend_pair_preserves_env });

    block.tests.push_back(dt::test_spec{
        "pair: sees whole context",
        "f may read the environment",
        &tests_extend_pair_sees_env });

    block.tests.push_back(dt::test_spec{
        "pair: focus type change",
        "int->string focus, env kept",
        &tests_extend_pair_type_change });

    block.tests.push_back(dt::test_spec{
        "kv: recompute value",
        "key kept, value recomputed (field-level)",
        &tests_extend_kv_recompute });

    block.tests.push_back(dt::test_spec{
        "kv: sees whole context",
        "f may read the key",
        &tests_extend_kv_sees_key });

    block.tests.push_back(dt::test_spec{
        "kv: value type change",
        "int->string value, key kept",
        &tests_extend_kv_type_change });

    block.tests.push_back(dt::test_spec{
        "ident: recompute focus",
        "Identity comonad focus recomputed",
        &tests_extend_ident });

    block.tests.push_back(dt::test_spec{
        "law: extend(extract) == id",
        "comonad left identity",
        &tests_extend_left_identity });

    block.tests.push_back(dt::test_spec{
        "law: extract . extend(f) == f",
        "focus of extended context is f(whole)",
        &tests_extend_right_identity });

    block.tests.push_back(dt::test_spec{
        "law: associativity",
        "co-Kleisli associativity holds",
        &tests_extend_associativity });

    block.tests.push_back(dt::test_spec{
        "forwarding",
        "lvalue and rvalue comonad operands",
        &tests_extend_forwarding });

    block.tests.push_back(dt::test_spec{
        "result type: pair",
        "pair<Env, mapped focus>",
        &tests_extend_result_type_pair });

    block.tests.push_back(dt::test_spec{
        "result type: kv",
        "kv_pair<Key, mapped focus>",
        &tests_extend_result_type_kv });

    return block;
}


NS_END  // testing
NS_END  // djinterp
