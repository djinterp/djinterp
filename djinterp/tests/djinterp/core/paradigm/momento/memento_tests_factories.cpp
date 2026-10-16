/*******************************************************************************
* djinterp [core]                                    memento_tests_factories.cpp
*
*
* path:      /tests/djinterp/core/paradigm/momento/memento_tests_factories.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : memento_tests_factories.cpp
//   The convenience factories (section VIII, C++14+): make_memento,
// restore_memento, make_caretaker, and make_undo_redo. Every predicate body is
// guarded on the C++14 gate and passes vacuously on older dialects, so the
// declarations still link at C++11.

// std
#include <cstddef>
// djinterp
#include "memento_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_make_memento
  Verifies make_memento captures state into a memento (default deep-copy).
  Tests the following:
  - the returned memento holds a copy of the supplied state
*/
bool
tests_make_memento()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    auto m = make_memento(42);

    return (m.state == 42);
#else
    return true;
#endif
}

/*
tests_make_memento_with_policy
  Verifies make_memento honours an explicit snapshot policy.
  Tests the following:
  - a clone-policy capture yields a memento whose snapshot is the clone type
*/
bool
tests_make_memento_with_policy()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    clonable_state c(9);

    auto m = make_memento<clone_snapshot>(c);

    return (m.state.value == 9);
#else
    return true;
#endif
}

/*
tests_restore_memento
  Verifies restore_memento restores state from a memento.
  Tests the following:
  - state captured via make_memento is restored after mutation
*/
bool
tests_restore_memento()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    int s = 7;
    auto snap = make_memento(s);
    s = 0;
    restore_memento(s, snap);

    return (s == 7);
#else
    return true;
#endif
}

/*
tests_make_caretaker
  Verifies make_caretaker builds a typed caretaker.
  Tests the following:
  - the returned caretaker accepts pushes and reports size
*/
bool
tests_make_caretaker()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    auto ct = make_caretaker<int>();
    ct.push(memento<int>(1));
    ct.push(memento<int>(2));

    return (ct.size() == 2 && ct.peek().state == 2);
#else
    return true;
#endif
}

/*
tests_make_undo_redo
  Verifies make_undo_redo builds a working undo/redo stack.
  Tests the following:
  - a checkpoint followed by an undo restores the checkpointed state
*/
bool
tests_make_undo_redo()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    auto ur = make_undo_redo<int>();

    int s = 3;
    ur.checkpoint(s);
    s = 9;
    bool did = ur.undo(s);

    return (did && s == 3);
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
