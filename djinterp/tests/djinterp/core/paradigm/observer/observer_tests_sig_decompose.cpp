/*******************************************************************************
* djinterp [core]                               observer_tests_sig_decompose.cpp
*
*
* path:      /tests/djinterp/core/paradigm/observer/observer_tests_sig_decompose.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : observer_tests_sig_decompose.cpp
//   Signature decomposition (section 1): sig_decompose<R(Args...)> exposes the
// return type, the corresponding function-pointer type, and the arity. Verified
// across void and non-void returns and zero-, one-, and multi-argument shapes.

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "observer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_sig_decompose_return_type
  Verifies the return_type alias.
  Tests the following:
  - return_type is the signature's return type (void and non-void)
*/
bool
tests_sig_decompose_return_type()
{
    static_assert(std::is_same<sig_decompose<void(int)>::return_type, void>::value,
                  "void return");
    static_assert(std::is_same<sig_decompose<int(double)>::return_type, int>::value,
                  "int return");

    return ( std::is_same<sig_decompose<void(int)>::return_type, void>::value &&
             std::is_same<sig_decompose<int(double)>::return_type, int>::value );
}

/*
tests_sig_decompose_arity
  Verifies the arity constant.
  Tests the following:
  - arity is zero for a no-argument signature
  - arity counts each argument
*/
bool
tests_sig_decompose_arity()
{
    static_assert(sig_decompose<void()>::arity == 0,          "nullary");
    static_assert(sig_decompose<void(int)>::arity == 1,       "unary");
    static_assert(sig_decompose<void(int, int, int)>::arity == 3, "ternary");

    return ( sig_decompose<void()>::arity == 0 &&
             sig_decompose<void(int)>::arity == 1 &&
             sig_decompose<void(int, int, int)>::arity == 3 );
}

/*
tests_sig_decompose_fn_ptr_type
  Verifies the fn_ptr_type alias.
  Tests the following:
  - fn_ptr_type is the pointer-to-function type for the signature
*/
bool
tests_sig_decompose_fn_ptr_type()
{
    static_assert(std::is_same<
                      sig_decompose<int(char, double)>::fn_ptr_type,
                      int(*)(char, double)>::value,
                  "fn ptr type");

    return std::is_same<
        sig_decompose<int(char, double)>::fn_ptr_type,
        int(*)(char, double)>::value;
}

/*
tests_sig_decompose_void_and_multi
  Verifies decomposition of a void multi-argument signature end to end.
  Tests the following:
  - return_type, arity, and fn_ptr_type are all consistent for void(int, double)
*/
bool
tests_sig_decompose_void_and_multi()
{
    using S = sig_decompose<void(int, double)>;

    bool ok = true;

    ok = ok && (std::is_same<S::return_type, void>::value);
    ok = ok && (S::arity == 2);
    ok = ok && (std::is_same<S::fn_ptr_type, void(*)(int, double)>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
