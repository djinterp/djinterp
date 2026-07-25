/******************************************************************************
* djinterp [test]                              test_context_tests_builders.cpp
*
*   Section II of the test_context suite: the FOCUS BUILDERS.  at_run,
* at_module, at_unit and at_check each name a common focus shape by populating
* a prefix of the four parts (and the indices that ride alongside them).  Each
* builder is pinned on both call forms - with its trailing index arguments
* given and with them defaulted to zero - confirming it fills exactly the
* fields it names and leaves the rest at their defaults.
*
*   Four cross-cutting properties close the section: NULL TOLERANCE (the
* builders store a pointer, never dereference it, so a null part is stored
* faithfully), POINTER DISTINCTNESS (four distinct nodes land in their own
* fields with no cross-wiring), CONST POINTEES (the parts are pointer-to-const,
* so const nodes are accepted), and INDEX VERBATIM (an index is stored exactly
* as passed - the 1-based display convention is the caller's to apply, so no
* builder adds a phantom offset).  A final sweep states the FOCUS-DEPTH
* invariant uniformly: each builder nulls every part deeper than its level and
* zeroes every index it does not set.
*
*   Every body is C++17-gated (the builders do not exist below C++17); under an
* older standard each unit passes vacuously.
*
* path:      /tests/djinterp/test/test_context/test_context_tests_builders.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "test_context_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_builder_at_run
//   at_run carries only the run pointer; the three deeper parts are null and
// all three indices are zero (the outermost focus, from which per_module
// iteration refocuses).
bool
tests_builder_at_run()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    report_fixture f;

    dt::test_context c = dt::at_run(&f.run);
    ok = D_CTX_CHECK(c.run == &f.run)                                      && ok;
    ok = D_CTX_CHECK(c.module_ == nullptr && c.unit == nullptr &&
                     c.check == nullptr)                                   && ok;
    ok = D_CTX_CHECK(c.module_index == std::size_t(0) &&
                     c.unit_index == std::size_t(0) &&
                     c.check_index == std::size_t(0))                      && ok;
#endif

    return ok;
}

// tests_builder_at_module
//   at_module carries run + module + the module's index.  The index argument
// defaults to zero when omitted and is otherwise stored as given; unit and
// check stay null and their indices stay zero.
bool
tests_builder_at_module()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    report_fixture f;

    // default index (omitted)
    dt::test_context d = dt::at_module(&f.run, &f.module_);
    ok = D_CTX_CHECK(d.run == &f.run && d.module_ == &f.module_)           && ok;
    ok = D_CTX_CHECK(d.module_index == std::size_t(0))                     && ok;  // default
    ok = D_CTX_CHECK(d.unit == nullptr && d.check == nullptr)              && ok;
    ok = D_CTX_CHECK(d.unit_index == std::size_t(0) &&
                     d.check_index == std::size_t(0))                      && ok;

    // explicit index
    dt::test_context x = dt::at_module(&f.run, &f.module_, std::size_t(4));
    ok = D_CTX_CHECK(x.module_index == std::size_t(4))                     && ok;
    ok = D_CTX_CHECK(x.run == &f.run && x.module_ == &f.module_)           && ok;
    ok = D_CTX_CHECK(x.unit == nullptr && x.check == nullptr)              && ok;
    ok = D_CTX_CHECK(x.unit_index == std::size_t(0) &&
                     x.check_index == std::size_t(0))                      && ok;
#endif

    return ok;
}

// tests_builder_at_unit
//   at_unit carries run + module + unit + the module and unit indices (both
// defaulting to zero).  check stays null and check_index stays zero.
bool
tests_builder_at_unit()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    report_fixture f;

    // default indices (omitted)
    dt::test_context d = dt::at_unit(&f.run, &f.module_, &f.unit);
    ok = D_CTX_CHECK(d.run == &f.run && d.module_ == &f.module_ &&
                     d.unit == &f.unit)                                    && ok;
    ok = D_CTX_CHECK(d.module_index == std::size_t(0) &&
                     d.unit_index == std::size_t(0))                       && ok;  // defaults
    ok = D_CTX_CHECK(d.check == nullptr)                                   && ok;
    ok = D_CTX_CHECK(d.check_index == std::size_t(0))                      && ok;

    // explicit indices
    dt::test_context x = dt::at_unit(&f.run, &f.module_, &f.unit,
                                     std::size_t(6), std::size_t(9));
    ok = D_CTX_CHECK(x.module_index == std::size_t(6) &&
                     x.unit_index == std::size_t(9))                       && ok;
    ok = D_CTX_CHECK(x.run == &f.run && x.module_ == &f.module_ &&
                     x.unit == &f.unit)                                    && ok;
    ok = D_CTX_CHECK(x.check == nullptr &&
                     x.check_index == std::size_t(0))                      && ok;
#endif

    return ok;
}

// tests_builder_at_check
//   at_check carries every part and every index (each index defaulting to
// zero).  This is the fully-populated focus, for addressing one assertion.
bool
tests_builder_at_check()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    report_fixture f;

    // default indices (omitted)
    dt::test_context d = dt::at_check(&f.run, &f.module_, &f.unit, &f.check);
    ok = D_CTX_CHECK(d.run == &f.run && d.module_ == &f.module_ &&
                     d.unit == &f.unit && d.check == &f.check)             && ok;
    ok = D_CTX_CHECK(d.module_index == std::size_t(0) &&
                     d.unit_index == std::size_t(0) &&
                     d.check_index == std::size_t(0))                      && ok;

    // explicit indices
    dt::test_context x = dt::at_check(&f.run, &f.module_, &f.unit, &f.check,
                                      std::size_t(1), std::size_t(2), std::size_t(3));
    ok = D_CTX_CHECK(x.run == &f.run && x.module_ == &f.module_ &&
                     x.unit == &f.unit && x.check == &f.check)             && ok;
    ok = D_CTX_CHECK(x.module_index == std::size_t(1) &&
                     x.unit_index == std::size_t(2) &&
                     x.check_index == std::size_t(3))                      && ok;
#endif

    return ok;
}

// tests_builder_nullptr_accepted
//   the builders store a pointer without dereferencing it, so a null part is
// carried faithfully (the lenient nullable design) - and the indices are still
// recorded alongside the null pointers.
bool
tests_builder_nullptr_accepted()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    dt::test_context r = dt::at_run(nullptr);
    ok = D_CTX_CHECK(r.run == nullptr)                                     && ok;

    dt::test_context m = dt::at_module(nullptr, nullptr, std::size_t(5));
    ok = D_CTX_CHECK(m.run == nullptr && m.module_ == nullptr)             && ok;
    ok = D_CTX_CHECK(m.module_index == std::size_t(5))                     && ok;   // index still recorded

    dt::test_context u = dt::at_unit(nullptr, nullptr, nullptr,
                                     std::size_t(7), std::size_t(8));
    ok = D_CTX_CHECK(u.run == nullptr && u.module_ == nullptr &&
                     u.unit == nullptr)                                    && ok;
    ok = D_CTX_CHECK(u.module_index == std::size_t(7) &&
                     u.unit_index == std::size_t(8))                       && ok;

    dt::test_context c = dt::at_check(nullptr, nullptr, nullptr, nullptr,
                                      std::size_t(1), std::size_t(2), std::size_t(3));
    ok = D_CTX_CHECK(c.run == nullptr && c.module_ == nullptr &&
                     c.unit == nullptr && c.check == nullptr)              && ok;
    ok = D_CTX_CHECK(c.module_index == std::size_t(1) &&
                     c.unit_index == std::size_t(2) &&
                     c.check_index == std::size_t(3))                      && ok;
#endif

    return ok;
}

// tests_builder_pointer_distinctness
//   handed four distinct nodes, at_check routes each into its own field (a
// swap would surface as a field holding the wrong address), and the four
// stored addresses are mutually distinct.
bool
tests_builder_pointer_distinctness()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    report_fixture f;

    dt::test_context c = dt::at_check(&f.run, &f.module_, &f.unit, &f.check);

    // each field holds its own node (no cross-wiring)
    ok = D_CTX_CHECK(c.run == &f.run)          && ok;
    ok = D_CTX_CHECK(c.module_ == &f.module_)  && ok;
    ok = D_CTX_CHECK(c.unit == &f.unit)        && ok;
    ok = D_CTX_CHECK(c.check == &f.check)      && ok;

    // the four stored addresses are mutually distinct
    const void* pr = c.run;
    const void* pm = c.module_;
    const void* pu = c.unit;
    const void* pc = c.check;
    ok = D_CTX_CHECK(pr != pm && pr != pu && pr != pc)                     && ok;
    ok = D_CTX_CHECK(pm != pu && pm != pc)                                 && ok;
    ok = D_CTX_CHECK(pu != pc)                                             && ok;
#endif

    return ok;
}

// tests_builder_const_pointees
//   the four parts are pointer-to-const, so the builders accept const report
// nodes - the ordinary case, since a focus only ever reads its pointees.
bool
tests_builder_const_pointees()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    const dt::test_report   cr;
    const dt::report_module cm;
    const dt::report_unit   cu;
    const dt::report_check  cc;

    dt::test_context c = dt::at_check(&cr, &cm, &cu, &cc,
                                      std::size_t(1), std::size_t(1), std::size_t(1));
    ok = D_CTX_CHECK(c.run == &cr && c.module_ == &cm &&
                     c.unit == &cu && c.check == &cc)                      && ok;

    // the narrower builders accept const pointees too
    dt::test_context r = dt::at_run(&cr);
    ok = D_CTX_CHECK(r.run == &cr)                                         && ok;
    dt::test_context m = dt::at_module(&cr, &cm);
    ok = D_CTX_CHECK(m.run == &cr && m.module_ == &cm)                     && ok;
    dt::test_context u = dt::at_unit(&cr, &cm, &cu);
    ok = D_CTX_CHECK(u.run == &cr && u.module_ == &cm && u.unit == &cu)    && ok;
#endif

    return ok;
}

// tests_builder_index_verbatim
//   an index is stored exactly as passed: zero stays zero (no phantom +1 - the
// 1-based convention is applied by the caller, e.g. a refocus closure
// installing i + 1), an arbitrary value round-trips, and the maximum size_t
// survives intact (an added offset would wrap it to zero).
bool
tests_builder_index_verbatim()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    report_fixture f;

    // zero passes through as zero (no hidden offset)
    dt::test_context z = dt::at_module(&f.run, &f.module_, std::size_t(0));
    ok = D_CTX_CHECK(z.module_index == std::size_t(0))                     && ok;

    // an arbitrary triple round-trips unchanged
    dt::test_context a = dt::at_check(&f.run, &f.module_, &f.unit, &f.check,
                                      std::size_t(41), std::size_t(42), std::size_t(43));
    ok = D_CTX_CHECK(a.module_index == std::size_t(41))                    && ok;
    ok = D_CTX_CHECK(a.unit_index == std::size_t(42))                      && ok;
    ok = D_CTX_CHECK(a.check_index == std::size_t(43))                     && ok;

    // the maximum size_t survives (a phantom +1 would wrap it to zero)
    const std::size_t big = static_cast<std::size_t>(-1);
    dt::test_context m = dt::at_unit(&f.run, &f.module_, &f.unit, big, big);
    ok = D_CTX_CHECK(m.module_index == big && m.unit_index == big)         && ok;
#endif

    return ok;
}

// tests_builder_focus_depth_invariant
//   stated uniformly across the builders: each nulls every part deeper than
// its level and zeroes every index it does not set - checked with NONZERO
// indices where the builder accepts them, so a zero deeper index is provably
// the builder's doing and not an echo of the input.
bool
tests_builder_focus_depth_invariant()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    report_fixture f;

    // at_run: only run; everything deeper null, every index zero
    {
        dt::test_context c = dt::at_run(&f.run);
        ok = D_CTX_CHECK(c.module_ == nullptr && c.unit == nullptr &&
                         c.check == nullptr)                               && ok;
        ok = D_CTX_CHECK(c.module_index == std::size_t(0) &&
                         c.unit_index == std::size_t(0) &&
                         c.check_index == std::size_t(0))                  && ok;
    }
    // at_module (nonzero module_index): unit/check null, their indices zero
    {
        dt::test_context c = dt::at_module(&f.run, &f.module_, std::size_t(5));
        ok = D_CTX_CHECK(c.unit == nullptr && c.check == nullptr)          && ok;
        ok = D_CTX_CHECK(c.unit_index == std::size_t(0) &&
                         c.check_index == std::size_t(0))                  && ok;
    }
    // at_unit (nonzero module/unit indices): check null, check_index zero
    {
        dt::test_context c = dt::at_unit(&f.run, &f.module_, &f.unit,
                                         std::size_t(5), std::size_t(6));
        ok = D_CTX_CHECK(c.check == nullptr)                               && ok;
        ok = D_CTX_CHECK(c.check_index == std::size_t(0))                  && ok;
    }
    // at_check: the invariant bottoms out - nothing deeper remains null
    {
        dt::test_context c = dt::at_check(&f.run, &f.module_, &f.unit, &f.check,
                                          std::size_t(5), std::size_t(6), std::size_t(7));
        ok = D_CTX_CHECK(c.run != nullptr && c.module_ != nullptr &&
                         c.unit != nullptr && c.check != nullptr)          && ok;
    }
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
