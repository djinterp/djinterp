/******************************************************************************
* djinterp [test]                                     maybe_tests_lifetime.cpp
*
* Lifetime, value access, and mutation tests for maybe<T> (section I of
* maybe.hpp: the primitive itself).
*
*   test_construction  -- empty / nothing-tag / value copy / value move /
*                         copy / move constructors, including moved-from state.
*   test_assignment    -- copy, move, nothing, and value assignment, plus
*                         self-assignment and empty <-> non-empty transitions.
*   test_observers     -- has_value, is_nothing, operator bool, value() in its
*                         const / mutable / rvalue overloads, value_or, expect.
*   test_modifiers     -- reset and emplace.
*   test_lifetime      -- balanced construction / destruction of the contained
*                         value via an instance-counting payload type.
******************************************************************************/

#include <string>
#include <stdexcept>
#include <utility>
#include "./maybe_tests.hpp"


NS_DJINTERP
NS_TESTING


// definitions of the tracked payload's static counters (declared in header).
int tracked::live        = 0;
int tracked::constructed = 0;
int tracked::destroyed   = 0;


void test_construction(test::test_handler& _h)
{
    // default constructor -> empty
    maybe<int> e;
    test::record_assertion(_h, !e.has_value(),
        "construct: default is empty");

    // nothing-tag constructor -> empty
    maybe<int> n(nothing_v);
    test::record_assertion(_h, !n.has_value(),
        "construct: nothing_v tag is empty");

    // value copy constructor
    int        src = 7;
    maybe<int> cv(src);
    test::record_assertion(_h, cv.has_value() && cv.value() == 7,
        "construct: from const lvalue holds value");

    // value move constructor
    maybe<std::string> mv(std::string("hello"));
    test::record_assertion(_h, mv.has_value() && mv.value() == "hello",
        "construct: from rvalue holds value");

    // copy constructor (value present)
    maybe<int> a(42);
    maybe<int> b(a);
    test::record_assertion(_h,
        b.has_value() && b.value() == 42 && a.has_value() && a.value() == 42,
        "construct: copy of a value is an independent equal value");

    // copy constructor (empty source)
    maybe<int> empty_src;
    maybe<int> copy_of_empty(empty_src);
    test::record_assertion(_h, !copy_of_empty.has_value(),
        "construct: copy of empty is empty");

    // move constructor (value present). NOTE: maybe's move constructor copies
    // the discriminator and move-constructs the value, but does NOT clear the
    // source flag, so the moved-from maybe still reports has_value() == true.
    maybe<std::string> msrc(std::string("world"));
    maybe<std::string> mdst(std::move(msrc));
    test::record_assertion(_h, mdst.has_value() && mdst.value() == "world",
        "construct: move target holds the value");
    test::record_assertion(_h, msrc.has_value(),
        "construct: moved-from maybe retains its has_value flag");

    // move constructor (empty source)
    maybe<int> mempty_src;
    maybe<int> mempty_dst(std::move(mempty_src));
    test::record_assertion(_h, !mempty_dst.has_value(),
        "construct: move of empty is empty");

    return;
}


void test_assignment(test::test_handler& _h)
{
    // copy-assign value onto empty (constructs)
    maybe<int> a;
    maybe<int> src(5);
    a = src;
    test::record_assertion(_h, a.has_value() && a.value() == 5,
        "assign: copy onto empty constructs value");

    // copy-assign value onto value (assigns in place)
    maybe<int> b(1);
    b = src;
    test::record_assertion(_h, b.value() == 5,
        "assign: copy onto value updates in place");

    // copy-assign empty onto value (resets)
    maybe<int> c(9);
    maybe<int> empty;
    c = empty;
    test::record_assertion(_h, !c.has_value(),
        "assign: copy of empty resets target");

    // self copy-assignment is a no-op
    maybe<int> s(11);
    maybe<int>& sref = s;
    s = sref;
    test::record_assertion(_h, s.has_value() && s.value() == 11,
        "assign: self copy-assignment preserves value");

    // move-assign value onto empty (constructs)
    maybe<std::string> ma;
    ma = std::string("x");
    test::record_assertion(_h, ma.has_value() && ma.value() == "x",
        "assign: move value onto empty constructs");

    // move-assign value onto value (assigns)
    ma = std::string("y");
    test::record_assertion(_h, ma.value() == "y",
        "assign: move value onto value updates");

    // move-assign maybe onto value
    maybe<std::string> md(std::string("a"));
    maybe<std::string> me(std::string("b"));
    md = std::move(me);
    test::record_assertion(_h, md.value() == "b",
        "assign: move-assign maybe updates target");

    // move-assign empty maybe onto value (resets)
    maybe<int> mv(3);
    maybe<int> mempty;
    mv = std::move(mempty);
    test::record_assertion(_h, !mv.has_value(),
        "assign: move of empty resets target");

    // self move-assignment is a no-op
    maybe<int> sm(13);
    maybe<int>& smref = sm;
    sm = std::move(smref);
    test::record_assertion(_h, sm.has_value() && sm.value() == 13,
        "assign: self move-assignment preserves value");

    // nothing-tag assignment resets
    maybe<int> nt(8);
    nt = nothing_v;
    test::record_assertion(_h, !nt.has_value(),
        "assign: nothing_v tag resets to empty");

    // value lvalue assignment onto empty
    maybe<int> lv;
    int        lvsrc = 21;
    lv = lvsrc;
    test::record_assertion(_h, lv.has_value() && lv.value() == 21,
        "assign: const-lvalue value onto empty constructs");

    return;
}


void test_observers(test::test_handler& _h)
{
    maybe<int> v(4);
    maybe<int> e;

    // has_value / is_nothing are complementary
    test::record_assertion(_h, v.has_value() && !v.is_nothing(),
        "observe: value -> has_value, not is_nothing");
    test::record_assertion(_h, !e.has_value() && e.is_nothing(),
        "observe: empty -> is_nothing, not has_value");

    // explicit operator bool
    test::record_assertion(_h, static_cast<bool>(v) && !static_cast<bool>(e),
        "observe: operator bool tracks presence");

    // value() const
    const maybe<int>& cv = v;
    test::record_assertion(_h, cv.value() == 4,
        "observe: const value() reads contained value");

    // value() mutable (write through)
    v.value() = 6;
    test::record_assertion(_h, v.value() == 6,
        "observe: mutable value() permits write-through");

    // value() rvalue overload
    test::record_assertion(_h, maybe<int>(10).value() == 10,
        "observe: rvalue value() yields the moved value");

    // value_or present / absent
    test::record_assertion(_h, v.value_or(-1) == 6,
        "observe: value_or returns value when present");
    test::record_assertion(_h, e.value_or(-1) == -1,
        "observe: value_or returns default when empty");

    // expect present
    test::record_assertion(_h, v.expect("should not throw") == 6,
        "observe: expect returns value when present");

    // expect absent -> throws std::runtime_error with the message
    bool        threw = false;
    std::string msg;
    try
    {
        (void)e.expect("missing!");
    }
    catch (const std::runtime_error& _ex)
    {
        threw = true;
        msg   = _ex.what();
    }
    test::record_assertion(_h, threw && msg == "missing!",
        "observe: expect throws runtime_error with message when empty");

    return;
}


void test_modifiers(test::test_handler& _h)
{
    // reset clears a value
    maybe<int> v(3);
    v.reset();
    test::record_assertion(_h, !v.has_value(),
        "modify: reset clears the value");

    // reset on empty is a harmless no-op
    v.reset();
    test::record_assertion(_h, !v.has_value(),
        "modify: reset on empty is a no-op");

    // emplace into empty
    maybe<std::string> s;
    std::string&       ref = s.emplace(3u, 'a');   // "aaa"
    test::record_assertion(_h, s.has_value() && s.value() == "aaa",
        "modify: emplace constructs in place");
    test::record_assertion(_h, &ref == &s.value(),
        "modify: emplace returns a reference to the new value");

    // emplace over an existing value replaces it
    s.emplace(2u, 'b');                            // "bb"
    test::record_assertion(_h, s.value() == "bb",
        "modify: emplace over a value replaces it");

    return;
}


void test_lifetime(test::test_handler& _h)
{
    tracked::reset_counters();

    {
        maybe<tracked> m(tracked(1));
        test::record_assertion(_h, tracked::live >= 1 && m.has_value(),
            "lifetime: value construction yields a live instance");

        // reset destroys the contained instance
        m.reset();
        test::record_assertion(_h, !m.has_value(),
            "lifetime: reset leaves the maybe empty");

        // emplace constructs again
        m.emplace(2);
        test::record_assertion(_h, m.has_value() && m.value().id == 2,
            "lifetime: emplace re-creates the contained value");

        // copy then independent destruction
        maybe<tracked> copy(m);
        test::record_assertion(_h, copy.has_value() && copy.value().id == 2,
            "lifetime: copy duplicates the payload");
    }   // both maybes destroyed here

    // every constructed instance must have been destroyed, and none leak.
    test::record_assertion(_h, tracked::live == 0,
        "lifetime: no live instances after scope exit");
    test::record_assertion(_h,
        tracked::constructed == tracked::destroyed && tracked::constructed > 0,
        "lifetime: construction and destruction counts balance");

    return;
}


NS_END  // testing
NS_END  // djinterp
