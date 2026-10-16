/*******************************************************************************
* djinterp [core]                                 kv_pair_tests_construction.cpp
*
*
* path:      /tests/djinterp/core/meta/kv_pair_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "kv_pair_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_construction_member_typedefs
  kv_pair exposes key_type and value_type.
  Tests the following:
  - key_type is the first template argument
  - value_type is the second
  - the two may differ
  - the members m_key and m_value have those types
*/
bool
tests_construction_member_typedefs()
{
    D_KV_CHECK((std::is_same<kv_pair<int, long>::key_type, int>::value));
    D_KV_CHECK((std::is_same<kv_pair<int, long>::value_type, long>::value));

    // distinct key/value types
    D_KV_CHECK(!(std::is_same<kv_pair<int, long>::key_type,
                              kv_pair<int, long>::value_type>::value));

    // members have the declared types
    D_KV_CHECK((std::is_same<decltype(std::declval<kv_pair<int, long> >().m_key),
                             int>::value));
    D_KV_CHECK((std::is_same<decltype(std::declval<kv_pair<int, long> >().m_value),
                             long>::value));

    return true;
}

/*
tests_construction_value_constructor_sets_members
  The const-ref constructor stores its arguments.
  Tests the following:
  - m_key holds the key argument
  - m_value holds the value argument
  - a second pair with different arguments holds those
  - the key and value are independent (setting one does not disturb the other)
*/
bool
tests_construction_value_constructor_sets_members()
{
    ii p(3, 4);
    D_KV_CHECK(p.m_key == 3);
    D_KV_CHECK(p.m_value == 4);

    ii q(10, 20);
    D_KV_CHECK(q.m_key == 10);
    D_KV_CHECK(q.m_value == 20);

    // independence
    D_KV_CHECK(p.m_key == 3 && p.m_value == 4);

    return true;
}

/*
tests_construction_default_constructor
  The default constructor yields a usable pair.
  Tests the following:
  - a default-constructed pair can be created
  - its members are assignable afterward
  - the assigned values read back
  - the type is default constructible by the trait
*/
bool
tests_construction_default_constructor()
{
    D_KV_CHECK(std::is_default_constructible<ii>::value);

    ii p;
    p.m_key   = 5;
    p.m_value = 6;
    D_KV_CHECK(p.m_key == 5);
    D_KV_CHECK(p.m_value == 6);

    return true;
}

/*
tests_construction_rvalue_constructor_moves
  The rvalue constructor moves its operands rather than copying.
  Tests the following:
  - constructing from two rvalue `tracked` operands increments their move counts
  - the moved-into members carry a nonzero move count
  - a lvalue (copy) construction does NOT increment the move count
  - the key's identity (tag) survives the move
*/
bool
tests_construction_rvalue_constructor_moves()
{
    // rvalue operands -> the rvalue ctor runs -> members are move-constructed
    kv_pair<tracked, tracked> moved(tracked(7), tracked(8));
    D_KV_CHECK(moved.m_key.tag == 7);
    D_KV_CHECK(moved.m_value.tag == 8);
    D_KV_CHECK(moved.m_key.moves >= 1);     // was moved at least once
    D_KV_CHECK(moved.m_value.moves >= 1);

    // lvalue operands -> the const-ref ctor runs -> no move
    tracked k(1), v(2);
    kv_pair<tracked, tracked> copied(k, v);
    D_KV_CHECK(copied.m_key.moves == 0);
    D_KV_CHECK(copied.m_value.moves == 0);
    D_KV_CHECK(copied.m_key.tag == 1);

    return true;
}

/*
tests_construction_make_kv_deduces_decayed_types
  make_kv deduces decayed key and value types.
  Tests the following:
  - a string literal argument decays to std::string via std::decay in the test's
    own call (passing a std::string) -- key_type is std::string
  - a plain int value gives value_type int
  - the built pair holds the given key and value
  - passing a const lvalue still decays (no const/ref in the deduced types)
*/
bool
tests_construction_make_kv_deduces_decayed_types()
{
    auto p = make_kv(std::string("alpha"), 42);
    D_KV_CHECK((std::is_same<decltype(p)::key_type, std::string>::value));
    D_KV_CHECK((std::is_same<decltype(p)::value_type, int>::value));
    D_KV_CHECK(p.m_key == "alpha");
    D_KV_CHECK(p.m_value == 42);

    // const lvalue operands: decay strips const and ref
    const int ck = 5;
    const long cv = 6;
    auto q = make_kv(ck, cv);
    D_KV_CHECK((std::is_same<decltype(q)::key_type, int>::value));
    D_KV_CHECK((std::is_same<decltype(q)::value_type, long>::value));
    D_KV_CHECK(q.m_key == 5);
    D_KV_CHECK(q.m_value == 6);

    return true;
}

/*
tests_construction_is_constexpr_constructible
  A kv_pair<int,int> can live in a constant expression.
  Tests the following:
  - a constexpr pair can be constructed
  - its key reads back in a constant expression
  - its value reads back in a constant expression
  - make_kv's result is usable to initialize (runtime check on the values)
*/
bool
tests_construction_is_constexpr_constructible()
{
    constexpr ii p(9, 10);
    static_assert(p.m_key == 9, "constexpr key");
    static_assert(p.m_value == 10, "constexpr value");

    D_KV_CHECK(p.m_key == 9);
    D_KV_CHECK(p.m_value == 10);

    // a constexpr pair used in a further constant expression
    constexpr int sum = p.m_key + p.m_value;
    D_KV_CHECK(sum == 19);

    return true;
}

NS_END  // testing
NS_END  // djinterp
