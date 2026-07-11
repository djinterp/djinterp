/******************************************************************************
* djinterp [test]                                 comonad_tests_duplicate.cpp
*
*   Section II.3 of the comonad.hpp suite: duplicate, which nests a comonad one
* level (W<A> -> W<W<A>>) and is defined as extend with the identity.  Covers
* the nested structure over the std::pair and kv_pair Env instances (the
* environment is carried on both levels) and the custom Identity comonad, the
* comonad law extract . duplicate == id, the equivalence duplicate(w) ==
* extend(w, id), and the result types.
*
* path:      /tests/djinterp/core/functional/comonad_tests_duplicate.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "comonad_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_duplicate_pair
  Tests the following:
  - duplicate nests the pair with the environment on both levels:
    duplicate(("ctx",10)) == ("ctx", ("ctx", 10)).
*/
static bool
tests_duplicate_pair()
{
    const std::pair<std::string, std::pair<std::string, int> > r =
        ::djinterp::duplicate(std::pair<std::string, int>("ctx", 10));

    return (r == std::make_pair(
                     std::string("ctx"),
                     std::make_pair(std::string("ctx"), 10)));
}

/*
tests_duplicate_pair_extract_law
  Tests the following:
  - the comonad law extract . duplicate == id: the focus of the duplicated
    context is the original context.
*/
static bool
tests_duplicate_pair_extract_law()
{
    const std::pair<std::string, int> w("ctx", 10);

    const std::pair<std::string, int> recovered =
        ::djinterp::extract(::djinterp::duplicate(w));

    return (recovered == w);
}

/*
tests_duplicate_kv
  Tests the following:
  - duplicate nests a kv_pair with the key on both levels: duplicate(kv(3,10))
    has m_key 3 and m_value kv(3,10) (field-level).
*/
static bool
tests_duplicate_kv()
{
    const ::djinterp::kv_pair<int, ::djinterp::kv_pair<int, int> > r =
        ::djinterp::duplicate(::djinterp::kv_pair<int, int>(3, 10));

    return ( (r.m_key == 3)             &&
             (r.m_value.m_key == 3)     &&
             (r.m_value.m_value == 10) );
}

/*
tests_duplicate_ident
  Tests the following:
  - duplicate nests the Identity comonad: duplicate(ident(5)) == ident(ident(5)).
*/
static bool
tests_duplicate_ident()
{
    const ident< ident<int> > r = ::djinterp::duplicate(ident<int>(5));

    return (r == ident< ident<int> >(ident<int>(5)));
}

/*
tests_duplicate_extract_law_kv_ident
  Tests the following:
  - extract . duplicate == id holds for kv (field-level) and the Identity
    comonad too.
*/
static bool
tests_duplicate_extract_law_kv_ident()
{
    const ::djinterp::kv_pair<int, int> wk(3, 10);
    const ::djinterp::kv_pair<int, int> rk =
        ::djinterp::extract(::djinterp::duplicate(wk));
    const bool kv_ok = ( (rk.m_key == 3) &&
                         (rk.m_value == 10) );

    const ident<int> wi(7);
    const bool ident_ok =
        (::djinterp::extract(::djinterp::duplicate(wi)) == wi);

    return (kv_ok && ident_ok);
}

/*
tests_duplicate_equals_extend_id
  Tests the following:
  - duplicate is extend with the identity: duplicate(w) == extend(w, id), over
    pair and the Identity comonad.
*/
static bool
tests_duplicate_equals_extend_id()
{
    const std::pair<std::string, int> wp("ctx", 10);
    const bool pair_ok =
        (::djinterp::duplicate(wp) == ::djinterp::extend(wp, co_id()));

    const ident<int> wi(5);
    const bool ident_ok =
        (::djinterp::duplicate(wi) == ::djinterp::extend(wi, co_id()));

    return (pair_ok && ident_ok);
}

/*
tests_duplicate_result_type_pair
  Tests the following:
  - the result type of duplicate over pair is pair<Env, pair<Env, A>>.
*/
static bool
tests_duplicate_result_type_pair()
{
    return std::is_same<
        decltype(::djinterp::duplicate(
            std::declval< const std::pair<std::string, int>& >())),
        std::pair<std::string, std::pair<std::string, int> > >::value;
}

/*
tests_duplicate_result_type_kv
  Tests the following:
  - the result type of duplicate over kv is kv_pair<Key, kv_pair<Key, V>>.
*/
static bool
tests_duplicate_result_type_kv()
{
    return std::is_same<
        decltype(::djinterp::duplicate(
            std::declval< const ::djinterp::kv_pair<int, int>& >())),
        ::djinterp::kv_pair<int, ::djinterp::kv_pair<int, int> > >::value;
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
comonad_duplicate_block()
{
    dt::block_spec block;

    block.name       = "II.3 duplicate";
    block.descriptor =
        "nest one level (= extend with identity): structure, law, result types";

    block.tests.push_back(dt::test_spec{
        "pair: nests",
        "(\"ctx\",10) -> (\"ctx\", (\"ctx\",10))",
        &tests_duplicate_pair });

    block.tests.push_back(dt::test_spec{
        "pair: extract . duplicate == id",
        "duplicate's focus is the original context",
        &tests_duplicate_pair_extract_law });

    block.tests.push_back(dt::test_spec{
        "kv: nests",
        "key carried on both levels (field-level)",
        &tests_duplicate_kv });

    block.tests.push_back(dt::test_spec{
        "ident: nests",
        "duplicate(ident(5)) == ident(ident(5))",
        &tests_duplicate_ident });

    block.tests.push_back(dt::test_spec{
        "extract . duplicate == id (kv, ident)",
        "the law over the other instances",
        &tests_duplicate_extract_law_kv_ident });

    block.tests.push_back(dt::test_spec{
        "duplicate == extend(id)",
        "definition equivalence",
        &tests_duplicate_equals_extend_id });

    block.tests.push_back(dt::test_spec{
        "result type: pair",
        "pair<Env, pair<Env, A>>",
        &tests_duplicate_result_type_pair });

    block.tests.push_back(dt::test_spec{
        "result type: kv",
        "kv_pair<Key, kv_pair<Key, V>>",
        &tests_duplicate_result_type_kv });

    return block;
}


NS_END  // testing
NS_END  // djinterp
