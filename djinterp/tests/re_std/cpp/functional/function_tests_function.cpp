/*******************************************************************************
* djinterp [re_std]                                  function_tests_function.cpp
*
* The function suite, block "function": re_std::function's construction,
* copying, moving, assignment, swap, target access and calls, over each kind
* of target.
*
*
* path:      /tests/re_std/cpp/functional/function_tests_function.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./function_tests.hpp"  // corresponding header

// std
#include <string>
#include <typeinfo>


NS_DJINTERP
NS_TESTING


namespace
{
    // fixture_add_one
    //   function: a free function target.
    int
    fixture_add_one(
        int _x
    )
    {
        return _x + 1;
    }

    // fixture_sink
    //   variable: where fixture_store writes.
    int fixture_sink = 0;

    // fixture_store
    //   function: a void-returning target.
    void
    fixture_store(
        int _x
    )
    {
        fixture_sink = _x;

        return;
    }

    // DFixtureWidget
    //   class: an object for a pointer to member function.
    struct DFixtureWidget
    {
        int m_base;

        explicit DFixtureWidget(
            int _base
        )
            : m_base(_base)
        {}

        int
        scaled(
            int _by
        ) const
        {
            return m_base * _by;
        }
    };

    // DFixtureCounter
    //   class: a stateful functor whose call mutates it.
    struct DFixtureCounter
    {
        int m_count;

        DFixtureCounter()
            : m_count(0)
        {}

        int
        operator()()
        {
            return ++m_count;
        }
    };
}  // anonymous namespace


/*
tests_function_empty
  A function with no target.
  Tests the following:
  - it converts to false and equals nullptr, either way round
  - calling it throws bad_function_call
*/
bool
tests_function_empty()
{
    re_std::function<int(int)> f;
    bool                       threw = false;

    D_TEST_REQUIRE(!f);
    D_TEST_REQUIRE(f == nullptr);
    D_TEST_REQUIRE(nullptr == f);

    try
    {
        (void)f(3);
    }
    catch (const re_std::bad_function_call&)
    {
        threw = true;
    }

    D_TEST_REQUIRE(threw);

    return true;
}

/*
tests_function_lambda
  A lambda target.
  Tests the following:
  - the function is not empty and calls the lambda
*/
bool
tests_function_lambda()
{
    re_std::function<int(int)> f = [](int _x) { return _x * 2; };

    D_TEST_REQUIRE(static_cast<bool>(f));
    D_TEST_REQUIRE(f != nullptr);
    D_TEST_REQUIRE(f(21) == 42);

    return true;
}

/*
tests_function_pointer
  A function pointer target.
  Tests the following:
  - from the pointer, and from the function, which decays to it
*/
bool
tests_function_pointer()
{
    re_std::function<int(int)> f(&fixture_add_one);
    re_std::function<int(int)> g(fixture_add_one);

    D_TEST_REQUIRE(f(9) == 10);
    D_TEST_REQUIRE(g(9) == 10);

    return true;
}

/*
tests_function_null_pointer
  A null function pointer.
  Tests the following:
  - it leaves the function empty
*/
bool
tests_function_null_pointer()
{
    int                        (*pointer)(int) = 0;
    re_std::function<int(int)> f(pointer);

    D_TEST_REQUIRE(!f);

    return true;
}

/*
tests_function_copy
  Copying.
  Tests the following:
  - a copy calls the same target
  - giving the source another target leaves the copy as it was
*/
bool
tests_function_copy()
{
    re_std::function<int(int)> a = [](int _x) { return _x + 100; };
    re_std::function<int(int)> b = a;

    D_TEST_REQUIRE(a(1) == 101);
    D_TEST_REQUIRE(b(1) == 101);

    a = [](int _x) { return _x; };
    D_TEST_REQUIRE(a(1) == 1);
    D_TEST_REQUIRE(b(1) == 101);

    return true;
}

/*
tests_function_move
  Moving.
  Tests the following:
  - the destination takes the target; the source is left empty
*/
bool
tests_function_move()
{
    re_std::function<int(int)> a = [](int _x) { return _x + 7; };
    re_std::function<int(int)> b =
        static_cast<re_std::function<int(int)>&&>(a);

    D_TEST_REQUIRE(static_cast<bool>(b));
    D_TEST_REQUIRE(!a);
    D_TEST_REQUIRE(b(0) == 7);

    return true;
}

/*
tests_function_assign
  Assignment.
  Tests the following:
  - assigning a lambda gives an empty function a target
  - assigning nullptr empties it
*/
bool
tests_function_assign()
{
    re_std::function<int(int)> f;

    f = [](int _x) { return _x - 1; };
    D_TEST_REQUIRE(f(5) == 4);

    f = nullptr;
    D_TEST_REQUIRE(!f);

    return true;
}

/*
tests_function_swap
  Swapping.
  Tests the following:
  - the member swap exchanges targets, and the non-member swap back
*/
bool
tests_function_swap()
{
    re_std::function<int(int)> a = [](int) { return 1; };
    re_std::function<int(int)> b = [](int) { return 2; };

    a.swap(b);
    D_TEST_REQUIRE( (a(0) == 2) && (b(0) == 1) );

    swap(a,
         b);
    D_TEST_REQUIRE( (a(0) == 1) && (b(0) == 2) );

    return true;
}

/*
tests_function_target
  Reaching the target.
  Tests the following:
  - target<T>() is the target when T is its type, const or not, and null
    otherwise
  - target_type() is the target's type, and void's for an empty function
*/
bool
tests_function_target()
{
    typedef int (*fixture_pointer)(int);

    re_std::function<int(int)>        f(&fixture_add_one);
    const re_std::function<int(int)>& view  = f;
    re_std::function<int(int)>        empty;
    fixture_pointer*                  found = f.target<fixture_pointer>();

    D_TEST_REQUIRE(found != 0);
    D_TEST_REQUIRE(*found == &fixture_add_one);
    D_TEST_REQUIRE(f.target<int>() == 0);
    D_TEST_REQUIRE(f.target_type() == typeid(fixture_pointer));
    D_TEST_REQUIRE(view.target<fixture_pointer>() != 0);
    D_TEST_REQUIRE(empty.target_type() == typeid(void));

    return true;
}

/*
tests_function_void_return
  A void signature.
  Tests the following:
  - it calls a void target
  - it calls a target with a result and discards the result
*/
bool
tests_function_void_return()
{
    re_std::function<void(int)> f = &fixture_store;
    re_std::function<void(int)> g = [](int _x) { return _x; };

    fixture_sink = 0;
    f(77);
    D_TEST_REQUIRE(fixture_sink == 77);

    g(5);

    return true;
}

/*
tests_function_reference_wrapper
  A reference_wrapper target.
  Tests the following:
  - the function calls the referred-to functor, whose state it shares
*/
bool
tests_function_reference_wrapper()
{
    DFixtureCounter         counter;
    re_std::function<int()> f = re_std::ref(counter);

    D_TEST_REQUIRE(f() == 1);
    D_TEST_REQUIRE(f() == 2);
    D_TEST_REQUIRE(counter.m_count == 2);

    return true;
}

/*
tests_function_stateful
  A stateful functor, by value.
  Tests the following:
  - the function keeps its own copy, whose state lasts across calls
*/
bool
tests_function_stateful()
{
    re_std::function<int()> f = DFixtureCounter();

    D_TEST_REQUIRE(f() == 1);
    D_TEST_REQUIRE(f() == 2);

    return true;
}

/*
tests_function_member_pointer
  A pointer to member function.
  Tests the following:
  - called with the object first, as std::invoke calls it
*/
bool
tests_function_member_pointer()
{
    re_std::function<int(const DFixtureWidget&, int)> f =
        &DFixtureWidget::scaled;
    DFixtureWidget                                    widget(10);

    D_TEST_REQUIRE(f(widget,
                     3) == 30);

    return true;
}

/*
tests_function_convertible_return
  A result of another type.
  Tests the following:
  - an int result converts to the signature's long
*/
bool
tests_function_convertible_return()
{
    re_std::function<long(int)> f = [](int _x) { return _x + 1; };

    D_TEST_REQUIRE(f(41) == 42L);

    return true;
}

/*
tests_function_reference_return
  A reference result.
  Tests the following:
  - the function returns the target's reference, through which the referred
    object changes
*/
bool
tests_function_reference_return()
{
    static int               cell = 5;
    re_std::function<int&()> f    = []() -> int& { return cell; };

    f() = 99;
    D_TEST_REQUIRE(cell == 99);

    return true;
}

/*
tests_function_by_value_argument
  An argument taken by value.
  Tests the following:
  - a std::string argument reaches the target whole
*/
bool
tests_function_by_value_argument()
{
    re_std::function<int(std::string)> f =
        [](std::string _text) { return static_cast<int>(_text.size()); };
    std::string                        text = "hello";

    D_TEST_REQUIRE(f(text) == 5);

    return true;
}


NS_END  // testing
NS_END  // djinterp
