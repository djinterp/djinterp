/*******************************************************************************
* djinterp [core]                                    memento_tests_undo_redo.cpp
*
*
* path:      /tests/djinterp/core/paradigm/momento/memento_tests_undo_redo.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : memento_tests_undo_redo.cpp
//   The undo_redo_stack (section VI): a dual-stack undo/redo history over a
// snapshot policy. Covers checkpoint + undo, the undo/redo round-trip, the
// empty-stack guards, redo invalidation on a fresh checkpoint, the availability
// flags and depth counters, clear / clear_redo, and the optional peeks.

// std
#include <cstddef>
#include <string>
// djinterp
#include "memento_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_undo_redo_checkpoint_undo
  Verifies undo restores the most recent checkpoint.
  Tests the following:
  - undo returns true when a checkpoint exists
  - the state is restored to the last checkpointed value
*/
bool
tests_undo_redo_checkpoint_undo()
{
    undo_redo_stack<int> h;

    int s = 10;
    h.checkpoint(s);   // remember 10
    s = 20;

    bool did = h.undo(s);

    bool ok = true;

    ok = ok && (did);
    ok = ok && (s == 10);

    return ok;
}

/*
tests_undo_redo_roundtrip
  Verifies a full undo-then-redo sequence over two checkpoints.
  Tests the following:
  - successive undos walk back through the checkpoints
  - redo re-applies the most recently undone state
*/
bool
tests_undo_redo_roundtrip()
{
    undo_redo_stack<int> h;

    int s = 10;
    h.checkpoint(s);
    s = 20;
    h.checkpoint(s);
    s = 30;

    bool ok = true;

    ok = ok && (h.undo(s) && s == 20);   // back to last checkpoint
    ok = ok && (h.undo(s) && s == 10);   // back to first checkpoint
    ok = ok && (h.redo(s) && s == 20);   // forward again

    return ok;
}

/*
tests_undo_redo_undo_empty
  Verifies undo on an empty undo stack.
  Tests the following:
  - undo returns false and leaves the state unchanged
*/
bool
tests_undo_redo_undo_empty()
{
    undo_redo_stack<int> h;

    int s = 5;
    bool did = h.undo(s);

    return (!did && s == 5);
}

/*
tests_undo_redo_redo_empty
  Verifies redo with nothing to redo.
  Tests the following:
  - redo returns false and leaves the state unchanged
*/
bool
tests_undo_redo_redo_empty()
{
    undo_redo_stack<int> h;

    int s = 5;
    h.checkpoint(s);

    bool did = h.redo(s);   // never undone, so nothing to redo

    return (!did && s == 5);
}

/*
tests_undo_redo_checkpoint_clears_redo
  Verifies a fresh checkpoint invalidates the redo history.
  Tests the following:
  - after an undo populates the redo stack, a new checkpoint clears it
  - redo then reports nothing available
*/
bool
tests_undo_redo_checkpoint_clears_redo()
{
    undo_redo_stack<int> h;

    int s = 1;
    h.checkpoint(s);
    s = 2;
    h.undo(s);              // s == 1, redo now holds a state

    bool had_redo = h.can_redo();

    s = 7;
    h.checkpoint(s);        // branching: redo must be cleared

    bool ok = true;

    ok = ok && (had_redo);
    ok = ok && (!h.can_redo());

    return ok;
}

/*
tests_undo_redo_can_flags
  Verifies can_undo / can_redo availability flags.
  Tests the following:
  - a fresh stack can neither undo nor redo
  - a checkpoint enables undo
  - an undo enables redo
*/
bool
tests_undo_redo_can_flags()
{
    undo_redo_stack<int> h;

    bool ok = (!h.can_undo() && !h.can_redo());

    int s = 3;
    h.checkpoint(s);
    ok = ok && (h.can_undo() && !h.can_redo());

    s = 4;
    h.undo(s);
    ok = ok && (h.can_redo());

    return ok;
}

/*
tests_undo_redo_depths
  Verifies the undo/redo depth counters.
  Tests the following:
  - undo_depth counts available undo steps
  - redo_depth counts available redo steps
*/
bool
tests_undo_redo_depths()
{
    undo_redo_stack<int> h;

    int s = 0;
    h.checkpoint(s); s = 1;
    h.checkpoint(s); s = 2;

    bool ok = (h.undo_depth() == 2 && h.redo_depth() == 0);

    h.undo(s);
    ok = ok && (h.undo_depth() == 1 && h.redo_depth() == 1);

    return ok;
}

/*
tests_undo_redo_clear
  Verifies clear discards both stacks.
  Tests the following:
  - after clear neither undo nor redo is available
*/
bool
tests_undo_redo_clear()
{
    undo_redo_stack<int> h;

    int s = 0;
    h.checkpoint(s); s = 1;
    h.checkpoint(s); s = 2;
    h.undo(s);

    h.clear();

    return (!h.can_undo() && !h.can_redo());
}

/*
tests_undo_redo_clear_redo
  Verifies clear_redo discards only the forward history.
  Tests the following:
  - clear_redo empties the redo stack
  - the undo stack is preserved
*/
bool
tests_undo_redo_clear_redo()
{
    undo_redo_stack<int> h;

    int s = 0;
    h.checkpoint(s); s = 1;
    h.checkpoint(s); s = 2;
    h.undo(s);              // redo now holds a state

    h.clear_redo();

    bool ok = true;

    ok = ok && (!h.can_redo());
    ok = ok && (h.can_undo());

    return ok;
}

/*
tests_undo_redo_tagged
  Verifies the tagged checkpoint overload.
  Tests the following:
  - a tagged checkpoint participates in undo like an untagged one (C++17+)
*/
bool
tests_undo_redo_tagged()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    undo_redo_stack<int> h;

    int s = 10;
    h.checkpoint(s, std::string_view("v1"));
    s = 20;

    bool did = h.undo(s);

    return (did && s == 10);
#else
    return true;
#endif
}

/*
tests_undo_redo_peek
  Verifies the optional peek_undo / peek_redo accessors.
  Tests the following:
  - peek_undo is nullopt on an empty stack, and yields the top after a checkpoint
  - peek_redo yields the top after an undo (C++17+)
*/
bool
tests_undo_redo_peek()
{
#if D_MEMENTO_HAS_OPTIONAL
    undo_redo_stack<int> h;

    bool ok = (!h.peek_undo().has_value());

    int s = 42;
    h.checkpoint(s);
    ok = ok && (h.peek_undo().has_value());
    ok = ok && (h.peek_undo()->get().state == 42);

    s = 7;
    h.undo(s);
    ok = ok && (h.peek_redo().has_value());

    return ok;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
