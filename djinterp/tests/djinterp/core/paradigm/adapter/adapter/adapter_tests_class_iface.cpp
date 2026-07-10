// djinterp [test] : adapter_tests_class_iface.cpp
//   The inheritance-based adapters and the forwarding policies: class_adapter
// (section V, public-Target / private-Adaptee MI with a protected
// adaptee_ref()), interface_adapter (section VI, CRTP with a held adaptee),
// and the method-forwarding policies forward_as_is / forward_with_transform
// (section VII).
//
//   interface_adapter's const adaptee() is reached through by_pointer; its
// default by_value store provides no const access (see BUG 2).

// std
#include <stack>
#include <type_traits>
#include <utility>
// djinterp
#include "adapter_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // ================= class_adapter fixtures =================

    // shape_iface
    //   type: the target interface class_adapter presents publicly.
    struct shape_iface
    {
        virtual ~shape_iface() = default;

        virtual int area() const = 0;
    };

    // legacy_rect
    //   type: the adaptee implementation, with an idiosyncratic member.
    struct legacy_rect
    {
        int w;
        int h;

        legacy_rect()
            : w(0),
              h(0)
        {}

        legacy_rect(
                int _w,
                int _h
            )
            : w(_w),
              h(_h)
        {}

        int
        compute_area() const
        {
            return w * h;
        }
    };

    // rect_adapter
    //   type: the concrete adapter — satisfies shape_iface by delegating to the
    // privately-inherited legacy_rect through adaptee_ref().  Because the
    // adaptee is a private base, its injected-class-name is inaccessible inside
    // the derived class, so the base type is named through a namespace-scope
    // alias, the adaptee-taking constructors are inherited, and the adaptee
    // type is referred to via the public adaptee_type alias.
    class rect_adapter;

    using rect_adapter_base = class_adapter<shape_iface, legacy_rect, rect_adapter>;

    class rect_adapter : public rect_adapter_base
    {
    public:
        using base = rect_adapter_base;
        using base::base;

        rect_adapter() = default;

        int
        area() const override
        {
            return this->adaptee_ref().compute_area();   // const adaptee_ref()
        }

        // raw : expose adaptee_ref() (both overloads) for direct testing.
        adaptee_type&
        raw()
        {
            return this->adaptee_ref();
        }

        const adaptee_type&
        raw() const
        {
            return this->adaptee_ref();
        }
    };

    // ================= interface_adapter fixtures =================

    // stack_deque
    //   type: a deque-ish face over std::stack<int>, held by_value (the default
    // ownership).  All accessors are non-const, matching what by_value permits.
    class stack_deque : public interface_adapter<stack_deque, std::stack<int>>
    {
    public:
        using base = interface_adapter<stack_deque, std::stack<int>>;
        using base::base;

        void
        push_back(
            int _v
        )
        {
            this->adaptee().push(_v);

            return;
        }

        void
        pop_back()
        {
            this->adaptee().pop();

            return;
        }

        int&
        back()
        {
            return this->adaptee().top();
        }

        std::size_t
        count()
        {
            return this->adaptee().size();
        }
    };

    // counter_box
    //   type: a trivial adaptee for the by_pointer interface_adapter.
    struct counter_box
    {
        int n;

        counter_box()
            : n(0)
        {}
    };

    // boxed_counter
    //   type: CRTP adapter over counter_box held by_pointer, so const adaptee()
    // is well-formed and can back a const accessor.
    class boxed_counter : public interface_adapter<boxed_counter, counter_box, by_pointer>
    {
    public:
        using base = interface_adapter<boxed_counter, counter_box, by_pointer>;
        using base::base;

        void
        bump()
        {
            this->adaptee().n++;                 // non-const adaptee()

            return;
        }

        int
        value() const
        {
            return this->adaptee().n;            // const adaptee()
        }
    };

    // ================= forwarding-policy fixtures =================

    // accumulator
    //   type: a stateful adaptee whose add() the forwarding policies target.
    struct accumulator
    {
        int total;

        accumulator()
            : total(0)
        {}

        int
        add(
            int _n
        )
        {
            total += _n;

            return total;
        }
    };

    // doubler
    //   type: an argument transform for forward_with_transform.
    struct doubler
    {
        int
        operator()(
            int _x
        ) const
        {
            return _x * 2;
        }
    };
}


/*
tests_class_adapter_basic
  Verifies class_adapter delegation through the protected adaptee_ref().
  Tests the following:
  - the target method resolves to the adaptee implementation
  - mutation through the non-const adaptee_ref() is reflected
*/
bool
tests_class_adapter_basic()
{
    legacy_rect  r(3, 4);
    rect_adapter a(r);

    bool ok = true;

    ok = ok && (a.area() == 12);

    // mutate through non-const adaptee_ref()
    a.raw().w = 5;
    ok = ok && (a.area() == 20);

    return ok;
}

/*
tests_class_adapter_polymorphism
  Verifies class_adapter satisfies the target interface polymorphically.
  Tests the following:
  - a base-class reference dispatches to the adapter's override
  - a base-class pointer does the same
*/
bool
tests_class_adapter_polymorphism()
{
    legacy_rect  r(6, 7);
    rect_adapter a(r);

    bool ok = true;

    shape_iface& iface = a;
    ok = ok && (iface.area() == 42);

    shape_iface* p = &a;
    ok = ok && (p->area() == 42);

    return ok;
}

/*
tests_class_adapter_constructors
  Verifies class_adapter's three constructors and its type aliases.
  Tests the following:
  - default construction yields a default adaptee
  - copy-from-adaptee construction
  - move-from-adaptee construction
  - target_type / adaptee_type aliases
*/
bool
tests_class_adapter_constructors()
{
    bool ok = true;

    // default ctor
    rect_adapter a1;
    a1.raw().w = 2;
    a1.raw().h = 3;
    ok = ok && (a1.area() == 6);

    // copy-adaptee ctor
    legacy_rect  r(4, 5);
    rect_adapter a2(r);
    ok = ok && (a2.area() == 20);

    // move-adaptee ctor
    rect_adapter a3(legacy_rect(2, 8));
    ok = ok && (a3.area() == 16);

    static_assert(std::is_same<rect_adapter::target_type, shape_iface>::value,
                  "target_type");
    static_assert(std::is_same<rect_adapter::adaptee_type, legacy_rect>::value,
                  "adaptee_type");

    return ok;
}

/*
tests_class_adapter_const_adaptee_ref
  Verifies the const overload of adaptee_ref().
  Tests the following:
  - a const adapter reaches the adaptee through const adaptee_ref()
  - the const target method reads through it correctly
*/
bool
tests_class_adapter_const_adaptee_ref()
{
    legacy_rect        r(3, 9);
    const rect_adapter a(r);

    bool ok = true;

    ok = ok && (a.raw().compute_area() == 27);   // const adaptee_ref() via raw()
    ok = ok && (a.area() == 27);                 // const adaptee_ref() via area()

    return ok;
}

/*
tests_interface_adapter_by_value
  Verifies interface_adapter under the default by_value ownership.
  Tests the following:
  - default construction yields an empty adaptee
  - value construction moves a prebuilt adaptee in
  - non-const delegation (push/pop/top/size) round-trips
  - adaptee_type / ownership_policy aliases
*/
bool
tests_interface_adapter_by_value()
{
    bool ok = true;

    // default ctor
    stack_deque sd;
    sd.push_back(10);
    sd.push_back(20);
    ok = ok && (sd.count() == 2);
    ok = ok && (sd.back() == 20);

    sd.pop_back();
    ok = ok && (sd.count() == 1);
    ok = ok && (sd.back() == 10);

    // value ctor (move a prebuilt adaptee in)
    std::stack<int> st;
    st.push(7);
    st.push(8);
    stack_deque sd2(std::move(st));
    ok = ok && (sd2.count() == 2);
    ok = ok && (sd2.back() == 8);

    static_assert(std::is_same<stack_deque::adaptee_type,
                               std::stack<int>>::value,
                  "adaptee_type");
    static_assert(std::is_same<stack_deque::ownership_policy, by_value>::value,
                  "default ownership is by_value");

    return ok;
}

/*
tests_interface_adapter_by_pointer
  Verifies interface_adapter under by_pointer ownership.
  Tests the following:
  - value construction over a pointer to a live adaptee
  - non-const adaptee() mutates the pointee
  - const adaptee() reads it back
*/
bool
tests_interface_adapter_by_pointer()
{
    counter_box   box;
    boxed_counter c(&box);

    bool ok = true;

    c.bump();
    c.bump();
    ok = ok && (box.n == 2);
    ok = ok && (c.value() == 2);

    return ok;
}

/*
tests_interface_adapter_const_adaptee
  Verifies the const overload of interface_adapter::adaptee() on a const
  adapter (via by_pointer, which supports const access).
  Tests the following:
  - a const adapter reads its adaptee through const adaptee()
*/
bool
tests_interface_adapter_const_adaptee()
{
    counter_box box;
    box.n = 5;

    const boxed_counter c(&box);

    bool ok = true;

    ok = ok && (c.value() == 5);

    return ok;
}

/*
tests_forward_as_is
  Verifies the forward_as_is policy (identity method forwarding).
  Tests the following:
  - invoke() calls the named member with the given arguments
  - state changes on the adaptee persist across calls
*/
bool
tests_forward_as_is()
{
    accumulator acc;

    bool ok = true;

    const int r1 = forward_as_is::invoke(acc, &accumulator::add, 5);
    ok = ok && (r1 == 5);
    ok = ok && (acc.total == 5);

    const int r2 = forward_as_is::invoke(acc, &accumulator::add, 3);
    ok = ok && (r2 == 8);

    return ok;
}

/*
tests_forward_with_transform
  Verifies the forward_with_transform policy (argument-transforming forward).
  Tests the following:
  - each argument is passed through the transform before the member call
*/
bool
tests_forward_with_transform()
{
    accumulator acc;
    doubler     d;

    bool ok = true;

    // add(double(5)) == add(10)
    const int r = forward_with_transform<doubler>::invoke(acc,
                                                          &accumulator::add,
                                                          d,
                                                          5);
    ok = ok && (r == 10);
    ok = ok && (acc.total == 10);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
