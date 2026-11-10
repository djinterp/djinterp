/*******************************************************************************
* djinterp [test]                                       maybe_tests_lifetime.cpp
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
*
*
* path:      /tests/djinterp/cpp/functional/maybe_tests_lifetime.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.01
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <stdexcept>
#include <string>
#include <utility>
// djinterp
#include "./maybe_tests.hpp"


NS_DJINTERP
NS_TESTING


// definitions of the tracked payload's static counters (declared in header).
int tracked::live        = 0;
int tracked::constructed = 0;
int tracked::destroyed   = 0;


bool test_construction()
{
    bool verdict = true;

    // default constructor -> empty
    maybe<int> e;
    record_assertion(verdict, !e.has_value(),
        "construct: default is empty");

    // nothing-tag constructor -> empty
    maybe<int> n(nothing_v);
    record_assertion(verdict, !n.has_value(),
        "construct: nothing_v tag is empty");

    // value copy constructor
    int        src = 7;
    maybe<int> cv(src);
    record_assertion(verdict, cv.has_value() && cv.value() == 7,
        "construct: from const lvalue holds value");

    // value move constructor
    maybe<std::string> mv(std::string("hello"));
    record_assertion(verdict, mv.has_value() && mv.value() == "hello",
        "construct: from rvalue holds value");

    // copy constructor (value present)
    maybe<int> a(42);
    maybe<int> b(a);
    record_assertion(verdict,
        b.has_value() && b.value() == 42 && a.has_value() && a.value() == 42,
        "construct: copy of a value is an independent equal value");

    // copy constructor (empty source)
    maybe<int> empty_src;
    maybe<int> copy_of_empty(empty_src);
    record_assertion(verdict, !copy_of_empty.has_value(),
        "construct: copy of empty is empty");

    // move constructor (value present). NOTE: maybe's move constructor copies
    // the discriminator and move-constructs the value, but does NOT clear the
    // source flag, so the moved-from maybe still reports has_value() == true.
    maybe<std::string> msrc(std::string("world"));
    maybe<std::string> mdst(std::move(msrc));
    record_assertion(verdict, mdst.has_value() && mdst.value() == "world",
        "construct: move target holds the value");
    record_assertion(verdict, msrc.has_value(),
        "construct: moved-from maybe retains its has_value flag");

    // move constructor (empty source)
    maybe<int> mempty_src;
    maybe<int> mempty_dst(std::move(mempty_src));
    record_assertion(verdict, !mempty_dst.has_value(),
        "construct: move of empty is empty");

    return verdict;
}


bool test_assignment()
{
    bool verdict = true;

    // copy-assign value onto empty (constructs)
    maybe<int> a;
    maybe<int> src(5);
    a = src;
    record_assertion(verdict, a.has_value() && a.value() == 5,
        "assign: copy onto empty constructs value");

    // copy-assign value onto value (assigns in place)
    maybe<int> b(1);
    b = src;
    record_assertion(verdict, b.value() == 5,
        "assign: copy onto value updates in place");

    // copy-assign empty onto value (resets)
    maybe<int> c(9);
    maybe<int> empty;
    c = empty;
    record_assertion(verdict, !c.has_value(),
        "assign: copy of empty resets target");

    // self copy-assignment is a no-op
    maybe<int> s(11);
    maybe<int>& sref = s;
    s = sref;
    record_assertion(verdict, s.has_value() && s.value() == 11,
        "assign: self copy-assignment preserves value");

    // move-assign value onto empty (constructs)
    maybe<std::string> ma;
    ma = std::string("x");
    record_assertion(verdict, ma.has_value() && ma.value() == "x",
        "assign: move value onto empty constructs");

    // move-assign value onto value (assigns)
    ma = std::string("y");
    record_assertion(verdict, ma.value() == "y",
        "assign: move value onto value updates");

    // move-assign maybe onto value
    maybe<std::string> md(std::string("a"));
    maybe<std::string> me(std::string("b"));
    md = std::move(me);
    record_assertion(verdict, md.value() == "b",
        "assign: move-assign maybe updates target");

    // move-assign empty maybe onto value (resets)
    maybe<int> mv(3);
    maybe<int> mempty;
    mv = std::move(mempty);
    record_assertion(verdict, !mv.has_value(),
        "assign: move of empty resets target");

    // self move-assignment is a no-op
    maybe<int> sm(13);
    maybe<int>& smref = sm;
    sm = std::move(smref);
    record_assertion(verdict, sm.has_value() && sm.value() == 13,
        "assign: self move-assignment preserves value");

    // nothing-tag assignment resets
    maybe<int> nt(8);
    nt = nothing_v;
    record_assertion(verdict, !nt.has_value(),
        "assign: nothing_v tag resets to empty");

    // value lvalue assignment onto empty
    maybe<int> lv;
    int        lvsrc = 21;
    lv = lvsrc;
    record_assertion(verdict, lv.has_value() && lv.value() == 21,
        "assign: const-lvalue value onto empty constructs");

    return verdict;
}


bool test_observers()
{
    bool verdict = true;

    maybe<int> v(4);
    maybe<int> e;

    // has_value / is_nothing are complementary
    record_assertion(verdict, v.has_value() && !v.is_nothing(),
        "observe: value -> has_value, not is_nothing");
    record_assertion(verdict, !e.has_value() && e.is_nothing(),
        "observe: empty -> is_nothing, not has_value");

    // explicit operator bool
    record_assertion(verdict, static_cast<bool>(v) && !static_cast<bool>(e),
        "observe: operator bool tracks presence");

    // value() const
    const maybe<int>& cv = v;
    record_assertion(verdict, cv.value() == 4,
        "observe: const value() reads contained value");

    // value() mutable (write through)
    v.value() = 6;
    record_assertion(verdict, v.value() == 6,
        "observe: mutable value() permits write-through");

    // value() rvalue overload
    record_assertion(verdict, maybe<int>(10).value() == 10,
        "observe: rvalue value() yields the moved value");

    // value_or present / absent
    record_assertion(verdict, v.value_or(-1) == 6,
        "observe: value_or returns value when present");
    record_assertion(verdict, e.value_or(-1) == -1,
        "observe: value_or returns default when empty");

    // expect present
    record_assertion(verdict, v.expect("should not throw") == 6,
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
    record_assertion(verdict, threw && msg == "missing!",
        "observe: expect throws runtime_error with message when empty");

    return verdict;
}


bool test_modifiers()
{
    bool verdict = true;

    // reset clears a value
    maybe<int> v(3);
    v.reset();
    record_assertion(verdict, !v.has_value(),
        "modify: reset clears the value");

    // reset on empty is a harmless no-op
    v.reset();
    record_assertion(verdict, !v.has_value(),
        "modify: reset on empty is a no-op");

    // emplace into empty
    maybe<std::string> s;
    std::string&       ref = s.emplace(3u, 'a');   // "aaa"
    record_assertion(verdict, s.has_value() && s.value() == "aaa",
        "modify: emplace constructs in place");
    record_assertion(verdict, &ref == &s.value(),
        "modify: emplace returns a reference to the new value");

    // emplace over an existing value replaces it
    s.emplace(2u, 'b');                            // "bb"
    record_assertion(verdict, s.value() == "bb",
        "modify: emplace over a value replaces it");

    return verdict;
}


bool test_lifetime()
{
    bool verdict = true;

    tracked::reset_counters();

    {
        maybe<tracked> m(tracked(1));
        record_assertion(verdict, tracked::live >= 1 && m.has_value(),
            "lifetime: value construction yields a live instance");

        // reset destroys the contained instance
        m.reset();
        record_assertion(verdict, !m.has_value(),
            "lifetime: reset leaves the maybe empty");

        // emplace constructs again
        m.emplace(2);
        record_assertion(verdict, m.has_value() && m.value().id == 2,
            "lifetime: emplace re-creates the contained value");

        // copy then independent destruction
        maybe<tracked> copy(m);
        record_assertion(verdict, copy.has_value() && copy.value().id == 2,
            "lifetime: copy duplicates the payload");
    }   // both maybes destroyed here

    // every constructed instance must have been destroyed, and none leak.
    record_assertion(verdict, tracked::live == 0,
        "lifetime: no live instances after scope exit");
    record_assertion(verdict,
        tracked::constructed == tracked::destroyed && tracked::constructed > 0,
        "lifetime: construction and destruction counts balance");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
