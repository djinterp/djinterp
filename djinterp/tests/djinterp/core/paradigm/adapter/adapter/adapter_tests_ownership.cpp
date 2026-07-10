// djinterp [test] : adapter_tests_ownership.cpp
//   Feature gates (section I) and the five ownership storage policies
// (section II): by_reference, by_pointer, by_value, by_shared_ptr,
// by_unique_ptr — their storage typedefs and access() behavior in isolation.

// std
#include <memory>
#include <type_traits>
// djinterp
#include "adapter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_feature_gates
  Verifies the configuration macros track the detected language level.
  Tests the following:
  - D_ADAPTER_HAS_IF_CONSTEXPR mirrors the C++17 gate
  - D_ADAPTER_HAS_CONCEPTS mirrors the C++20 gate
  - D_ADAPTER_HAS_DEDUCTION_GUIDES mirrors the C++17 gate
*/
bool
tests_feature_gates()
{
    bool ok = true;

    // if-constexpr availability follows C++17
    ok = ok && (D_ADAPTER_HAS_IF_CONSTEXPR ==
                (D_ENV_LANG_IS_CPP17_OR_HIGHER ? 1 : 0));

    // concept availability follows C++20
    ok = ok && (D_ADAPTER_HAS_CONCEPTS ==
                (D_ENV_LANG_IS_CPP20_OR_HIGHER ? 1 : 0));

    // deduction-guide availability follows C++17
    ok = ok && (D_ADAPTER_HAS_DEDUCTION_GUIDES ==
                (D_ENV_LANG_IS_CPP17_OR_HIGHER ? 1 : 0));

    return ok;
}

/*
tests_by_reference
  Verifies the by_reference ownership policy.
  Tests the following:
  - stored_type and reference_type are both _Adaptee&
  - access() returns a reference bound to the very same object
  - writes through the returned reference reach the adaptee
*/
bool
tests_by_reference()
{
    using storage = by_reference::storage<int>;

    static_assert(std::is_same<storage::stored_type, int&>::value,
                  "by_reference::stored_type must be _Adaptee&");
    static_assert(std::is_same<storage::reference_type, int&>::value,
                  "by_reference::reference_type must be _Adaptee&");

    bool ok = true;

    int  x = 5;
    int& r = storage::access(x);

    ok = ok && (&r == &x);
    ok = ok && (r == 5);

    // write-through
    r = 9;
    ok = ok && (x == 9);

    return ok;
}

/*
tests_by_pointer
  Verifies the by_pointer ownership policy.
  Tests the following:
  - stored_type is _Adaptee*, reference_type is _Adaptee&
  - access() dereferences the pointer to the same object
  - writes through the returned reference reach the pointee
*/
bool
tests_by_pointer()
{
    using storage = by_pointer::storage<int>;

    static_assert(std::is_same<storage::stored_type, int*>::value,
                  "by_pointer::stored_type must be _Adaptee*");
    static_assert(std::is_same<storage::reference_type, int&>::value,
                  "by_pointer::reference_type must be _Adaptee&");

    bool ok = true;

    int  x = 7;
    int& r = storage::access(&x);

    ok = ok && (&r == &x);
    ok = ok && (r == 7);

    // write-through
    r = 3;
    ok = ok && (x == 3);

    return ok;
}

/*
tests_by_value
  Verifies the by_value ownership policy.
  Tests the following:
  - stored_type is _Adaptee (owning), reference_type is _Adaptee&
  - access() returns a reference to the stored object
  - writes through the returned reference reach the stored object
*/
bool
tests_by_value()
{
    using storage = by_value::storage<int>;

    static_assert(std::is_same<storage::stored_type, int>::value,
                  "by_value::stored_type must be _Adaptee");
    static_assert(std::is_same<storage::reference_type, int&>::value,
                  "by_value::reference_type must be _Adaptee&");

    bool ok = true;

    int  v = 4;
    int& r = storage::access(v);

    ok = ok && (&r == &v);
    ok = ok && (r == 4);

    // write-through
    r = 8;
    ok = ok && (v == 8);

    return ok;
}

/*
tests_by_shared_ptr
  Verifies the by_shared_ptr ownership policy.
  Tests the following:
  - stored_type is std::shared_ptr<_Adaptee>, reference_type is _Adaptee&
  - access() dereferences the shared_ptr to the managed object
  - writes through the returned reference reach the managed object
*/
bool
tests_by_shared_ptr()
{
    using storage = by_shared_ptr::storage<int>;

    static_assert(std::is_same<storage::stored_type,
                               std::shared_ptr<int>>::value,
                  "by_shared_ptr::stored_type must be shared_ptr<_Adaptee>");
    static_assert(std::is_same<storage::reference_type, int&>::value,
                  "by_shared_ptr::reference_type must be _Adaptee&");

    bool ok = true;

    std::shared_ptr<int> sp = std::make_shared<int>(11);
    int&                 r  = storage::access(sp);

    ok = ok && (&r == sp.get());
    ok = ok && (r == 11);

    // write-through
    r = 22;
    ok = ok && (*sp == 22);

    return ok;
}

/*
tests_by_unique_ptr
  Verifies the by_unique_ptr ownership policy.
  Tests the following:
  - stored_type is std::unique_ptr<_Adaptee>, reference_type is _Adaptee&
  - access() dereferences the unique_ptr to the managed object
  - writes through the returned reference reach the managed object
*/
bool
tests_by_unique_ptr()
{
    using storage = by_unique_ptr::storage<int>;

    static_assert(std::is_same<storage::stored_type,
                               std::unique_ptr<int>>::value,
                  "by_unique_ptr::stored_type must be unique_ptr<_Adaptee>");
    static_assert(std::is_same<storage::reference_type, int&>::value,
                  "by_unique_ptr::reference_type must be _Adaptee&");

    bool ok = true;

    std::unique_ptr<int> up(new int(13));
    int&                 r = storage::access(up);

    ok = ok && (&r == up.get());
    ok = ok && (r == 13);

    // write-through
    r = 26;
    ok = ok && (*up == 26);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
