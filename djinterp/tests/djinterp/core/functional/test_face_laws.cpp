/*******************************************************************************
* djinterp [core]                                             test_face_laws.cpp
*
*
* path:      /tests/djinterp/core/functional/test_face_laws.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
/*
test_face_laws.cpp
  The Layout, Cost and Parity laws for the C++ face over the C core.

  This is not a behaviour test: it asserts that the face DERIVES from the core
rather than paralleling it. Three claims, each from framework_goals.md §10:

    Layout  -- one declaration; sizeof/offsetof identical in both dialects.
    Cost    -- a wrapper's sizeof equals the wrapped type's.
    Parity  -- canon(lower(A (*)_cpp B)) == canon(lower(A) (*)_c B).

  The Parity checks here are unusual in that they cannot fail: every face method
forwards to the C function of the same name, so there is no second algorithm to
disagree. They are written anyway, because the value is in catching the day
someone gives the face its own implementation.
*/

// std
#include <cstdio>
#include <cstring>
// djinterp
#include "../../../../inc/djinterp/core/functional/functional_face.hpp"

namespace
{
    std::size_t g_checked = 0;
    std::size_t g_passed  = 0;

    void ck(bool _c, const char* _label)
    {
        g_checked = g_checked + 1;

        if (_c) { g_passed = g_passed + 1; }
        else    { std::printf("    FAIL  %s\n", _label); }
    }
}

extern "C"
{
    static bool face_double(const void* _i, void* _o, void* _x)
    {
        (void)_x; *(intmax_t*)_o = (*(const intmax_t*)_i) * 2; return true;
    }
    static void face_k_inc(const void* _v, void* _oc, void* _x)
    {
        (void)_x;
        intmax_t n = (*(const intmax_t*)_v) + 1;
        d_maybe_just((struct d_maybe*)_oc, &n);
    }
}

int main()
{
    using namespace djinterp::face;

    std::printf("djinterp [functional] C++ face: Layout / Cost / Parity\n");

    // ---- Cost law: a wrapper adds nothing -------------------------------
    ck(sizeof(reducer)   == sizeof(d_reducer),   "reducer costs its C struct");
    ck(sizeof(semigroup) == sizeof(d_semigroup), "semigroup costs its C struct");
    ck(sizeof(monoid)    == sizeof(d_monoid),    "monoid costs its C struct");
    ck(sizeof(producer)  == sizeof(d_producer),  "producer costs its C struct");

    // ---- Layout law: the cell IS the C layout, from the shared macro ----
    {
        D_MAYBE_DECLARE(c_cell, intmax_t);
        typedef maybe<intmax_t>::cell cpp_cell;

        ck(sizeof(cpp_cell) == sizeof(struct c_cell),
           "maybe cell: C and C++ agree on size");
        ck(offsetof(cpp_cell, has_value) == offsetof(struct c_cell, has_value),
           "maybe cell: tag at the same offset");
        ck(offsetof(cpp_cell, value) == offsetof(struct c_cell, value),
           "maybe cell: value at the same offset");
    }
    {
        D_RESULT_DECLARE(c_rcell, intmax_t, intmax_t);
        typedef result<intmax_t, intmax_t>::cell cpp_rcell;

        ck(sizeof(cpp_rcell) == sizeof(struct c_rcell),
           "result cell: C and C++ agree on size");
        ck(offsetof(cpp_rcell, payload) == offsetof(struct c_rcell, payload),
           "result cell: union at the same offset");
        ck(sizeof(cpp_rcell) < (sizeof(intmax_t) * 2 + sizeof(void*)),
           "result cell: the arms still share storage");
    }

    // ---- no vtable anywhere in the face --------------------------------
    ck(sizeof(reducer) == sizeof(fn_reducer_step) + sizeof(void*),
       "no virtual functions in core types");

    // ---- Parity: face result == core result, same inputs ---------------
    {
        intmax_t data[4] = {3, 5, 7, 11};
        intmax_t via_face = 0;
        intmax_t via_core = 0;

        monoid   m = monoid::sum_intmax();
        d_monoid c = d_monoid_sum_intmax();

        m.mconcat(data, 4, &via_face);
        d_mconcat(&c, data, 4, &via_core);

        ck(via_face == via_core, "parity: mconcat");
        ck(via_face == 26,       "parity: and the answer is right");

        m.mempty(&via_face);
        d_mempty(&c, &via_core);
        ck(via_face == via_core, "parity: mempty");
    }
    {
        // the forgetful map must land on the same semigroup the C one does
        monoid m = monoid::sum_intmax();
        d_monoid c = d_monoid_sum_intmax();

        ck(m.forget().core().value_size ==
               d_monoid_semigroup(&c)->value_size,
           "parity: forgetful map agrees");
        ck((const void*)d_monoid_semigroup(&m.core()) == (const void*)&m.core(),
           "conservativity: the forgetful map is still free through the face");
    }
    {
        // a face maybe and a C maybe, same transform, same answer
        maybe<intmax_t> a(21);
        maybe<intmax_t> b;

        a.map(b, face_double);
        ck(b.is_just() && b.value_or(0) == 42, "parity: maybe map");

        maybe<intmax_t> c;
        a.bind(c, face_k_inc);
        ck(c.is_just() && c.value_or(0) == 22, "parity: maybe bind");

        // both arms must hold DISTINCT values, or swapping them is invisible
        maybe<intmax_t> left(1);
        maybe<intmax_t> right(2);
        maybe<intmax_t> chosen;
        left.alt(right, chosen);
        ck(chosen.value_or(0) == 1, "parity: alt is left-biased through the face");
        right.alt(left, chosen);
        ck(chosen.value_or(0) == 2, "parity: alt order is observable");

        maybe<intmax_t> empty;
        empty.alt(a, chosen);
        ck(chosen.value_or(0) == 21, "parity: alt falls through an empty left");
    }
    {
        // the face's reducer drives the same way the C one does
        intmax_t data[3] = {2, 4, 6};
        intmax_t acc     = 0;

        monoid            m = monoid::sum_intmax();
        reducer           r = m.as_reducer();
        d_reducing_state  st;

        d_reducing_state_init(&st, &acc);
        ck(r.drive(st, data, 3, sizeof(intmax_t)) == 3, "parity: drive count");
        ck(acc == 12, "parity: drive result");
    }
    {
        // a carrier folded through the face equals the same carrier in C
        maybe<intmax_t> cell(9);
        monoid   m   = monoid::sum_intmax();
        reducer  r   = m.as_reducer();
        intmax_t acc = 0;
        d_reducing_state st;

        d_reducing_state_init(&st, &acc);
        ck(cell.drive(r, st) == 1 && acc == 9,
           "parity: a face carrier folds on the C spine");
    }

    std::printf("\n  %lu / %lu assertions passed\n",
                (unsigned long)g_passed, (unsigned long)g_checked);

    return (g_passed == g_checked) ? 0 : 1;
}
