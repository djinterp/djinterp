/*******************************************************************************
* djinterp [core]                                          memento_tests_any.cpp
*
*
* path:      /tests/djinterp/core/paradigm/momento/memento_tests_any.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : memento_tests_any.cpp
//   The type-erased memento (section VII): any_memento (built on re_std::any)
// and any_memento_caretaker. Covers construction + holds<T>, the scalar (SBO)
// and class (heap) get<T> paths, the mutable get, type identity, reset, and
// metadata carriage; then the caretaker's push/pop/peek, its ability to hold
// heterogeneous snapshot types, for_each, clear, and the optional try_pop.

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "memento_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_any_memento_construct_holds
  Verifies construction stores a snapshot and holds<T> reports its type.
  Tests the following:
  - has_value is true after construction
  - holds<T> is true for the stored type and false for another
*/
bool
tests_any_memento_construct_holds()
{
    any_memento m(int(42));

    bool ok = true;

    ok = ok && (m.has_value());
    ok = ok && (m.holds<int>());
    ok = ok && (!m.holds<double>());

    return ok;
}

/*
tests_any_memento_get_sbo
  Verifies the by-value get for scalar (SBO) snapshot types.
  Tests the following:
  - a signed integer snapshot reads back by value
  - a bool snapshot reads back by value
  - a floating-point snapshot reads back by value
*/
bool
tests_any_memento_get_sbo()
{
    any_memento mi(int(-7));
    any_memento mb(true);
    any_memento md(3.5);

    bool ok = true;

    ok = ok && (mi.get<int>() == -7);
    ok = ok && (mb.get<bool>() == true);
    ok = ok && (md.get<double>() == 3.5);

    return ok;
}

/*
tests_any_memento_get_heap
  Verifies the const-reference get for class (heap) snapshot types.
  Tests the following:
  - a std::string snapshot reads back by const reference with the right value
*/
bool
tests_any_memento_get_heap()
{
    any_memento m(std::string("hello"));

    const std::string& s = m.get<std::string>();

    return (s == "hello");
}

/*
tests_any_memento_get_mutable
  Verifies the mutable-reference get for class snapshot types.
  Tests the following:
  - the stored snapshot can be mutated in place through the mutable get
*/
bool
tests_any_memento_get_mutable()
{
    any_memento m(std::string("ab"));

    m.get<std::string>() += "c";

    return (m.get<std::string>() == "abc");
}

/*
tests_any_memento_type
  Verifies the type-id accessor.
  Tests the following:
  - two mementos holding the same type share a type id
  - two mementos holding different types have different type ids
*/
bool
tests_any_memento_type()
{
    any_memento a(int(1));
    any_memento b(int(2));
    any_memento c(std::string("x"));

    bool ok = true;

    ok = ok && (a.type() == b.type());
    ok = ok && (a.type() != c.type());

    return ok;
}

/*
tests_any_memento_reset
  Verifies reset clears the stored snapshot.
  Tests the following:
  - has_value is false after reset
*/
bool
tests_any_memento_reset()
{
    any_memento m(int(5));
    m.reset();

    return (!m.has_value());
}

/*
tests_any_memento_metadata
  Verifies metadata travels with the type-erased memento.
  Tests the following:
  - a memento constructed with metadata carries the sequence through
*/
bool
tests_any_memento_metadata()
{
    any_memento m(int(9), memento_metadata(6));

    return (m.metadata().sequence == 6);
}

/*
tests_any_caretaker_push_pop
  Verifies the type-erased caretaker's push and pop.
  Tests the following:
  - push grows the history
  - pop returns and removes the newest memento
*/
bool
tests_any_caretaker_push_pop()
{
    any_memento_caretaker<> ct;

    ct.push(any_memento(int(1)));
    ct.push(any_memento(int(2)));

    bool ok = (ct.size() == 2);

    any_memento m = ct.pop();
    ok = ok && (m.holds<int>() && m.get<int>() == 2);
    ok = ok && (ct.size() == 1);

    return ok;
}

/*
tests_any_caretaker_peek
  Verifies the type-erased caretaker's peek.
  Tests the following:
  - peek returns the newest memento without removing it
*/
bool
tests_any_caretaker_peek()
{
    any_memento_caretaker<> ct;
    ct.push(any_memento(int(1)));
    ct.push(any_memento(std::string("top")));

    bool ok = true;

    ok = ok && (ct.peek().holds<std::string>());
    ok = ok && (ct.peek().get<std::string>() == "top");
    ok = ok && (ct.size() == 2);

    return ok;
}

/*
tests_any_caretaker_heterogeneous
  Verifies the caretaker holds heterogeneous snapshot types.
  Tests the following:
  - mementos of different stored types coexist and read back correctly
*/
bool
tests_any_caretaker_heterogeneous()
{
    any_memento_caretaker<> ct;
    ct.push(any_memento(int(1)));
    ct.push(any_memento(std::string("two")));
    ct.push(any_memento(double(3.0)));

    // any_memento_caretaker has no indexed at(); walk oldest-to-newest.
    int  idx = 0;
    bool ok  = true;
    ct.for_each([&](const any_memento& _m)
    {
        if (idx == 0)
        {
            ok = ok && (_m.holds<int>() && _m.get<int>() == 1);
        }
        else if (idx == 1)
        {
            ok = ok && (_m.holds<std::string>() && _m.get<std::string>() == "two");
        }
        else if (idx == 2)
        {
            ok = ok && (_m.holds<double>() && _m.get<double>() == 3.0);
        }
        ++idx;
    });
    ok = ok && (idx == 3);

    return ok;
}

/*
tests_any_caretaker_for_each
  Verifies the caretaker's for_each traversal.
  Tests the following:
  - the callback visits every stored memento
*/
bool
tests_any_caretaker_for_each()
{
    any_memento_caretaker<> ct;
    ct.push(any_memento(int(1)));
    ct.push(any_memento(int(2)));
    ct.push(any_memento(int(3)));

    int count = 0;
    ct.for_each([&](const any_memento&) { ++count; });

    return (count == 3);
}

/*
tests_any_caretaker_empty_clear
  Verifies the caretaker's empty and clear.
  Tests the following:
  - a fresh caretaker is empty
  - clear empties a populated caretaker
*/
bool
tests_any_caretaker_empty_clear()
{
    any_memento_caretaker<> ct;

    bool ok = (ct.empty());

    ct.push(any_memento(int(1)));
    ct.push(any_memento(int(2)));
    ok = ok && (!ct.empty());

    ct.clear();
    ok = ok && (ct.empty() && ct.size() == 0);

    return ok;
}

/*
tests_any_caretaker_try_pop
  Verifies the caretaker's optional try_pop.
  Tests the following:
  - try_pop yields the newest memento when non-empty
  - try_pop yields nullopt when empty (C++17+)
*/
bool
tests_any_caretaker_try_pop()
{
#if D_MEMENTO_HAS_OPTIONAL
    any_memento_caretaker<> ct;
    ct.push(any_memento(int(8)));

    std::optional<any_memento> got = ct.try_pop();

    bool ok = (got.has_value() && got->holds<int>() && got->get<int>() == 8);

    std::optional<any_memento> none = ct.try_pop();
    ok = ok && (!none.has_value());

    return ok;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
