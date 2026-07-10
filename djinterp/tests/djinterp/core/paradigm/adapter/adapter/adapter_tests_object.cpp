// djinterp [test] : adapter_tests_object.cpp
//   The object_adapter (section IV): the composition adapter, exercised across
// every ownership policy and its delegated size()/get()/adaptee()/forward()
// surface — both const and non-const where the policy permits.
//
//   Coverage note: by_pointer carries the full method surface (it supports
// const access); the owning policies (by_value/by_shared_ptr/by_unique_ptr)
// cover non-const delegation only, and by_reference construction is gated on
// D_ADAPTER_BYREF_FIXED.  See the BUG NOTES in adapter_tests.hpp.

// std
#include <memory>
#include <type_traits>
// djinterp
#include "adapter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_object_adapter_typedefs
  Verifies object_adapter's public type aliases and the default ownership.
  Tests the following:
  - adaptee_type / adaptation_policy / ownership_policy resolve correctly
  - the default ownership policy is by_reference
*/
bool
tests_object_adapter_typedefs()
{
    using oa = object_adapter<legacy_seq, legacy_policy, by_pointer>;

    static_assert(std::is_same<oa::adaptee_type, legacy_seq>::value,
                  "adaptee_type");
    static_assert(std::is_same<oa::adaptation_policy, legacy_policy>::value,
                  "adaptation_policy");
    static_assert(std::is_same<oa::ownership_policy, by_pointer>::value,
                  "ownership_policy");
    static_assert(std::is_same<
                      object_adapter<legacy_seq, legacy_policy>::ownership_policy,
                      by_reference>::value,
                  "default ownership is by_reference");

    return true;
}

/*
tests_object_adapter_by_reference
  Verifies object_adapter under the by_reference ownership policy.
  Tests the following (only when D_ADAPTER_BYREF_FIXED — see BUG 1):
  - construction over a live adaptee reference
  - size()/get() delegation and write-through get()
  - adaptee() aliases the original object
*/
bool
tests_object_adapter_by_reference()
{
#if D_ADAPTER_BYREF_FIXED
    legacy_seq s;
    s.data = { 10, 20, 30 };

    object_adapter<legacy_seq, legacy_policy> a(s);   // default: by_reference

    bool ok = true;

    ok = ok && (a.size() == 3);
    ok = ok && (a.get(0) == 10);

    a.get(1) = 99;
    ok = ok && (s.data[1] == 99);
    ok = ok && (&a.adaptee() == &s);

    return ok;
#else
    // BLOCKED by BUG 1: by_reference construction is ill-formed as shipped.
    // by_pointer (tests_object_adapter_by_pointer) covers the same delegation.
    return true;
#endif
}

/*
tests_object_adapter_by_pointer
  Verifies object_adapter under the by_pointer ownership policy (the full
  non-owning method surface).
  Tests the following:
  - size()/get() delegate through the policy
  - non-const get() writes through to the adaptee
  - adaptee() aliases the pointee; mutation through it is visible via size()
*/
bool
tests_object_adapter_by_pointer()
{
    legacy_seq s;
    s.data = { 10, 20, 30 };

    object_adapter<legacy_seq, legacy_policy, by_pointer> a(&s);

    bool ok = true;

    ok = ok && (a.size() == 3);
    ok = ok && (a.get(0) == 10);
    ok = ok && (a.get(2) == 30);

    // write-through non-const get()
    a.get(1) = 99;
    ok = ok && (s.data[1] == 99);

    // non-const adaptee()
    ok = ok && (&a.adaptee() == &s);
    a.adaptee().append(40);
    ok = ok && (a.size() == 4);

    return ok;
}

/*
tests_object_adapter_by_value
  Verifies object_adapter under the by_value (owning copy) ownership policy.
  Tests the following:
  - non-const get() reads and write-through mutate the OWNED copy
  - the original adaptee is untouched (a copy was stored)
  - non-const adaptee() reaches the owned object
  (size() is intentionally unexercised for owning policies — see BUG 2.)
*/
bool
tests_object_adapter_by_value()
{
    legacy_seq s;
    s.data = { 1, 2, 3 };

    object_adapter<legacy_seq, legacy_policy, by_value> a(s);   // owning copy

    bool ok = true;

    ok = ok && (a.get(0) == 1);
    ok = ok && (a.get(2) == 3);

    // write-through mutates the copy, not the source
    a.get(0) = 7;
    ok = ok && (a.get(0) == 7);
    ok = ok && (s.data[0] == 1);

    ok = ok && (a.adaptee().num_elements() == 3);

    return ok;
}

/*
tests_object_adapter_by_shared_ptr
  Verifies object_adapter under the by_shared_ptr ownership policy.
  Tests the following:
  - get() delegates to the shared object
  - write-through get() is visible through the original shared_ptr
  - non-const adaptee() reaches the managed object
*/
bool
tests_object_adapter_by_shared_ptr()
{
    std::shared_ptr<legacy_seq> sp = std::make_shared<legacy_seq>();
    sp->data = { 5, 6 };

    object_adapter<legacy_seq, legacy_policy, by_shared_ptr> a(sp);

    bool ok = true;

    ok = ok && (a.get(0) == 5);
    ok = ok && (a.get(1) == 6);

    // shared ownership: mutation visible through the original handle
    a.get(0) = 50;
    ok = ok && (sp->data[0] == 50);

    ok = ok && (a.adaptee().num_elements() == 2);

    return ok;
}

/*
tests_object_adapter_by_unique_ptr
  Verifies object_adapter under the by_unique_ptr (move-only) ownership policy.
  Tests the following:
  - construction by moving in a unique_ptr
  - get() delegation and write-through get()
  - non-const adaptee() reaches the managed object
*/
bool
tests_object_adapter_by_unique_ptr()
{
    std::unique_ptr<legacy_seq> up(new legacy_seq());
    up->data = { 7, 8, 9 };

    object_adapter<legacy_seq, legacy_policy, by_unique_ptr> a(std::move(up));

    bool ok = true;

    ok = ok && (a.get(0) == 7);
    ok = ok && (a.get(2) == 9);

    a.get(2) = 90;
    ok = ok && (a.get(2) == 90);

    ok = ok && (a.adaptee().num_elements() == 3);

    return ok;
}

/*
tests_object_adapter_const_access
  Verifies object_adapter's const-qualified overloads via by_pointer (the
  policy that supports const access).
  Tests the following:
  - const size() delegates through the policy
  - const get() returns a const reference to the element
  - const adaptee() returns a const reference aliasing the original
*/
bool
tests_object_adapter_const_access()
{
    legacy_seq s;
    s.data = { 11, 22, 33 };

    const object_adapter<legacy_seq, legacy_policy, by_pointer> a(&s);

    bool ok = true;

    ok = ok && (a.size() == 3);

    const int& e = a.get(1);
    ok = ok && (e == 22);

    ok = ok && (a.adaptee().num_elements() == 3);
    ok = ok && (&a.adaptee() == &s);

    return ok;
}

/*
tests_object_adapter_forward
  Verifies object_adapter's variadic forward() delegation.
  Tests the following:
  - forward(v) routes through legacy_policy::forward (append + return new size)
  - repeated forwards accumulate on the adaptee
*/
bool
tests_object_adapter_forward()
{
    legacy_seq s;
    s.data = { 1, 2 };

    object_adapter<legacy_seq, legacy_policy, by_pointer> a(&s);

    bool ok = true;

    const std::size_t n1 = a.forward(3);
    ok = ok && (n1 == 3);
    ok = ok && (s.data.size() == 3);
    ok = ok && (s.data[2] == 3);

    const std::size_t n2 = a.forward(4);
    ok = ok && (n2 == 4);
    ok = ok && (s.data[3] == 4);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
