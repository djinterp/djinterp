/******************************************************************************
* djinterp [test]                               test_context_tests_context.cpp
*
*   Section I of the test_context suite: the FOCUS STRUCT itself.  test_context
* is a plain aggregate - four nullable report-node pointers (run / module_ /
* unit / check) and three 1-based display indices (module_index / unit_index /
* check_index) - so the units here pin the properties an aggregate of that
* shape must have: its default member initializers (all-null, all-zero),
* aggregate initialization (named fields filled, the remainder falling to their
* defaults), independent per-field assignment with no cross-talk, and value
* (copy) semantics - the last mattering because the focus is passed and copied
* by value as a section re-focuses during iteration.
*
*   Every body is C++17-gated (test_context does not exist below C++17); under
* an older standard each unit passes vacuously.
*
* path:      /tests/djinterp/test/test_context/test_context_tests_context.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "test_context_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_context_default_members
//   a default-constructed focus has all four parts null and all three indices
// zero - the "outside iteration, nothing observed yet" state the document
// stack's lenient projections fall back on.
bool
tests_context_default_members()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    dt::test_context c;

    ok = D_CTX_CHECK(c.run == nullptr)     && ok;
    ok = D_CTX_CHECK(c.module_ == nullptr) && ok;
    ok = D_CTX_CHECK(c.unit == nullptr)    && ok;
    ok = D_CTX_CHECK(c.check == nullptr)   && ok;

    ok = D_CTX_CHECK(c.module_index == std::size_t(0)) && ok;
    ok = D_CTX_CHECK(c.unit_index == std::size_t(0))   && ok;
    ok = D_CTX_CHECK(c.check_index == std::size_t(0))  && ok;
#endif

    return ok;
}

// tests_context_aggregate_init
//   test_context is an aggregate, so brace initialization fills the leading
// fields and every field left unmentioned falls to its default member
// initializer - empty braces yield the all-default state, a single leading
// pointer leaves the rest null/zero, and a full list lands each of the seven
// values in its own field.
bool
tests_context_aggregate_init()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    report_fixture f;

    // empty braces -> every field at its default member initializer
    dt::test_context e{};
    ok = D_CTX_CHECK(e.run == nullptr && e.module_ == nullptr &&
                     e.unit == nullptr && e.check == nullptr)               && ok;
    ok = D_CTX_CHECK(e.module_index == std::size_t(0) &&
                     e.unit_index == std::size_t(0) &&
                     e.check_index == std::size_t(0))                       && ok;

    // one leading field named -> the remainder stays at defaults
    dt::test_context p{&f.run};
    ok = D_CTX_CHECK(p.run == &f.run)                                       && ok;
    ok = D_CTX_CHECK(p.module_ == nullptr && p.unit == nullptr &&
                     p.check == nullptr)                                    && ok;
    ok = D_CTX_CHECK(p.module_index == std::size_t(0) &&
                     p.unit_index == std::size_t(0) &&
                     p.check_index == std::size_t(0))                       && ok;

    // all seven fields, distinct values -> each lands in its own slot
    dt::test_context full{&f.run, &f.module_, &f.unit, &f.check,
                          std::size_t(3), std::size_t(5), std::size_t(7)};
    ok = D_CTX_CHECK(full.run == &f.run)                    && ok;
    ok = D_CTX_CHECK(full.module_ == &f.module_)            && ok;
    ok = D_CTX_CHECK(full.unit == &f.unit)                 && ok;
    ok = D_CTX_CHECK(full.check == &f.check)               && ok;
    ok = D_CTX_CHECK(full.module_index == std::size_t(3))  && ok;
    ok = D_CTX_CHECK(full.unit_index == std::size_t(5))    && ok;
    ok = D_CTX_CHECK(full.check_index == std::size_t(7))   && ok;
#endif

    return ok;
}

// tests_context_field_assignment
//   every field is a public data member, independently assignable.  Writing
// all seven distinct values reads each back faithfully; writing exactly one
// field on a fresh focus leaves the other six untouched (no aliasing in the
// layout).
bool
tests_context_field_assignment()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    report_fixture f;

    // assign all seven, read all seven
    dt::test_context c;
    c.run          = &f.run;
    c.module_      = &f.module_;
    c.unit         = &f.unit;
    c.check        = &f.check;
    c.module_index = std::size_t(11);
    c.unit_index   = std::size_t(22);
    c.check_index  = std::size_t(33);

    ok = D_CTX_CHECK(c.run == &f.run)                     && ok;
    ok = D_CTX_CHECK(c.module_ == &f.module_)             && ok;
    ok = D_CTX_CHECK(c.unit == &f.unit)                  && ok;
    ok = D_CTX_CHECK(c.check == &f.check)                && ok;
    ok = D_CTX_CHECK(c.module_index == std::size_t(11))  && ok;
    ok = D_CTX_CHECK(c.unit_index == std::size_t(22))    && ok;
    ok = D_CTX_CHECK(c.check_index == std::size_t(33))   && ok;

    // isolation, pointer field: set only `unit`
    dt::test_context iso;
    iso.unit = &f.unit;
    ok = D_CTX_CHECK(iso.unit == &f.unit)                                   && ok;
    ok = D_CTX_CHECK(iso.run == nullptr && iso.module_ == nullptr &&
                     iso.check == nullptr)                                  && ok;
    ok = D_CTX_CHECK(iso.module_index == std::size_t(0) &&
                     iso.unit_index == std::size_t(0) &&
                     iso.check_index == std::size_t(0))                     && ok;

    // isolation, index field: set only `check_index`
    dt::test_context iso2;
    iso2.check_index = std::size_t(99);
    ok = D_CTX_CHECK(iso2.check_index == std::size_t(99))                   && ok;
    ok = D_CTX_CHECK(iso2.module_index == std::size_t(0) &&
                     iso2.unit_index == std::size_t(0))                     && ok;
    ok = D_CTX_CHECK(iso2.run == nullptr && iso2.module_ == nullptr &&
                     iso2.unit == nullptr && iso2.check == nullptr)         && ok;
#endif

    return ok;
}

// tests_context_copy_semantics
//   the focus is a value: a copy equals its source field-by-field, and
// mutating the copy leaves the source untouched - the property a section
// relies on when it hands a re-focused copy to a nested skeleton without
// disturbing the focus it holds.
bool
tests_context_copy_semantics()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    report_fixture f;

    dt::test_context src{&f.run, &f.module_, &f.unit, &f.check,
                         std::size_t(2), std::size_t(4), std::size_t(6)};

    // copy-construct: equal field-by-field
    dt::test_context cpy = src;
    ok = D_CTX_CHECK(cpy.run == src.run && cpy.module_ == src.module_ &&
                     cpy.unit == src.unit && cpy.check == src.check)        && ok;
    ok = D_CTX_CHECK(cpy.module_index == src.module_index &&
                     cpy.unit_index == src.unit_index &&
                     cpy.check_index == src.check_index)                    && ok;

    // mutate the copy: source unaffected
    cpy.run          = nullptr;
    cpy.module_index = std::size_t(100);
    ok = D_CTX_CHECK(cpy.run == nullptr && cpy.module_index == std::size_t(100)) && ok;
    ok = D_CTX_CHECK(src.run == &f.run)                    && ok;   // source pointer intact
    ok = D_CTX_CHECK(src.module_index == std::size_t(2))   && ok;   // source index intact

    // copy-assign onto a default target
    dt::test_context tgt;
    tgt = src;
    ok = D_CTX_CHECK(tgt.unit == &f.unit && tgt.check_index == std::size_t(6)) && ok;
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
