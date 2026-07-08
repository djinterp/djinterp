/******************************************************************************
* djinterp [testing]                                           monad_tests.hpp
*
*   DTest declarations and shared helper types for the monad.hpp unit
* test suite.  Each like-group semantic section of monad.hpp maps to a
* single `void(test_handler&)` section function declared here and
* defined in its own .cpp translation unit:
*
*     section in monad.hpp              test .cpp file
*     -------------------------------   ------------------------------
*     0.  predicate traits / concepts   monad_tests_traits.cpp
*     I.  protocol (is_monad, traits)   monad_tests_protocol.cpp
*     II. core ops (unit/bind/map/join) monad_tests_operations.cpp
*     II. then / kleisli / lift_m2      monad_tests_composition.cpp
*     III+IV. combinators + operator|   monad_tests_pipeline.cpp
*
*   THE TEST MONAD:
*   monad.hpp is a protocol header -- monad_traits<M> is an undefined
* primary, so the generic operations cannot be exercised without a
* concrete monad that specializes it.  This header therefore defines a
* small, self-contained `test_maybe<T>` monad (a maybe / optional) and
* its monad_traits specialization, shared by every section.  It is the
* minimal faithful monad: unit, a bind that short-circuits on the empty
* state, value_type, and rebind.  Helper Kleisli arrows and plain
* functions used across sections are declared here too.
*
*   The compile-time trait/concept checks (section 0) live in
* static_asserts within monad_tests_traits.cpp; the section function
* there records one runtime roll-up assertion so the report shows a row
* for the compile-time suite.
*
*   PORTABILITY:
*   C++11 minimum.  The test arrows are written as named functor types
* (not generic lambdas) so the suite compiles unchanged on C++11.
*
* path:      /tests/djinterp/core/functional/monad_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.31
******************************************************************************/

#ifndef DJINTERP_TESTING_FUNCTIONAL_MONAD_
#define DJINTERP_TESTING_FUNCTIONAL_MONAD_ 1

// std
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include <djinterp/core/djinterp.hpp>
#include <djinterp/core/meta/type_traits.hpp>
#include <djinterp/core/functional/monad.hpp>
#include <djinterp/test/test_common.hpp>
#include <djinterp/test/test_trait.hpp>
#include <djinterp/test/test_handler.hpp>
#include <djinterp/test/test_defaults.hpp>
#include <djinterp/test/test_runner.hpp>


NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///                I.   THE TEST MONAD                                       ///
///////////////////////////////////////////////////////////////////////////////
//   test_maybe and its some() factory live directly in djinterp (NOT in
// djinterp::testing) on purpose: a real monad type lives in the same
// namespace as monad.hpp's operator|, so that argument-dependent lookup
// finds djinterp::operator| when a value is piped.  Defining the test
// monad here reproduces that arrangement faithfully; were it nested in
// djinterp::testing, ADL would not reach djinterp::operator| and the
// pipeline tests would not represent real usage.

// test_maybe
//   class: a minimal maybe / optional monad used to exercise the
// generic operations in monad.hpp.  Either holds a value (some) or is
// empty (none).  Equality is provided so assertions can compare
// results directly.
template<typename _Type>
class test_maybe
{
public:
    using value_type = _Type;

    // none-constructing default.
    test_maybe()
        : m_has(false),
          m_value()
    {}

    // some-constructing.
    explicit test_maybe(
        _Type _value
    )
        : m_has(true),
          m_value(std::move(_value))
    {}

    // none
    //   factory: the empty value of this monad.
    static test_maybe none()
    {
        return test_maybe();
    }

    // is_some
    //   returns: whether a value is held.
    bool 
    is_some() const
    {
        return m_has;
    }

    // value
    //   returns: the held value (undefined if none; tests guard with
    // is_some()).
    const _Type& value() const
    {
        return m_value;
    }

    // operator==
    //   compares two test_maybe values: both none, or both some with
    // equal values.
    bool operator==(
        const test_maybe& _other
    ) const
    {
        return (m_has == _other.m_has) &&
               ( !m_has || (m_value == _other.m_value) );
    }

    bool operator!=(
        const test_maybe& _other
    ) const
    {
        return !(*this == _other);
    }

private:
    bool  m_has;
    _Type m_value;
};

// some
//   factory: constructs a some-valued test_maybe<T> (deduces T).
template<typename _Type>
test_maybe<typename std::decay<_Type>::type>
some(
    _Type&& _value
)
{
    return test_maybe<typename std::decay<_Type>::type>(
        std::forward<_Type>(_value));
}


NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                II.  SHARED ARROWS & FUNCTIONS                            ///
///////////////////////////////////////////////////////////////////////////////
//   Named functor types (not generic lambdas) so the suite compiles on
// the C++11 baseline.  "Arrows" are Kleisli arrows T -> M<U>; "plain"
// functions are ordinary T -> U used with monad_map.

// arrow_inc
//   struct: Kleisli arrow int -> test_maybe<int> adding one.
struct arrow_inc
{
    test_maybe<int> operator()(const int& _x) const
    {
        return test_maybe<int>(_x + 1);
    }
};

// arrow_times_ten
//   struct: Kleisli arrow int -> test_maybe<int> multiplying by ten.
struct arrow_times_ten
{
    test_maybe<int> operator()(const int& _x) const
    {
        return test_maybe<int>(_x * 10);
    }
};

// arrow_to_none
//   struct: Kleisli arrow int -> test_maybe<int> that always fails
// (returns none).  Drives short-circuit paths.
struct arrow_to_none
{
    test_maybe<int> operator()(const int&) const
    {
        return test_maybe<int>::none();
    }
};

// arrow_to_string
//   struct: Kleisli arrow int -> test_maybe<std::string> (type-changing).
struct arrow_to_string
{
    test_maybe<std::string> operator()(const int& _x) const
    {
        return test_maybe<std::string>(std::to_string(_x));
    }
};

// plain_double
//   struct: ordinary transform int -> int doubling (for monad_map).
struct plain_double
{
    int operator()(const int& _x) const
    {
        return _x * 2;
    }
};

// plain_to_string
//   struct: ordinary transform int -> std::string (type-changing map).
struct plain_to_string
{
    std::string operator()(const int& _x) const
    {
        return std::to_string(_x);
    }
};

// binary_add
//   struct: binary function (int, int) -> int for lift_m2.
struct binary_add
{
    int operator()(const int& _a, const int& _b) const
    {
        return _a + _b;
    }
};

// binary_concat_sum
//   struct: binary function (int, int) -> std::string (type-changing
// lift_m2) producing the decimal string of the sum.
struct binary_concat_sum
{
    std::string operator()(const int& _a, const int& _b) const
    {
        return std::to_string(_a + _b);
    }
};

// not_a_monad
//   struct: a plain aggregate with no monad_traits specialization.
// Negative case for the monad-detection traits.
struct not_a_monad
{
    int m_value;
};


NS_END  // testing


///////////////////////////////////////////////////////////////////////////////
///                III. monad_traits SPECIALIZATION FOR test_maybe           ///
///////////////////////////////////////////////////////////////////////////////
//   Must live in djinterp:: (where monad_traits is declared), NOT in
// the generic operations look up
// monad_traits<M> by exact specialization.  test_maybe lives directly
// in djinterp (see Section I), so the specialization names it there.
//   (We are already inside djinterp here, having just closed the inner
// testing namespace above; the specialization is added directly.)

// monad_traits<test_maybe<T>>
//   trait: the monad protocol implementation for the test monad.
// Provides value_type, rebind, unit, and a short-circuiting bind --
// the three primitives the generic operations build on.
template<typename _Type>
struct monad_traits< ::djinterp::test_maybe<_Type> >
{
    // value_type
    //   type: the inner value type T.
    using value_type = _Type;

    // is_specialized
    //   marker: present on every monad_traits specialization;
    // is_monad detects it.
    using is_specialized = std::true_type;

    // rebind
    //   alias: the monad re-parameterized over a new inner type U.
    template<typename _To>
    using rebind = ::djinterp::test_maybe<_To>;

    // unit
    //   function: lifts a plain value into a some-valued monad.
    static ::djinterp::test_maybe<_Type>
    unit(
        _Type _value
    )
    {
        return ::djinterp::test_maybe<_Type>(std::move(_value));
    }

    // bind
    //   function: threads the held value through _function (a Kleisli
    // arrow T -> M<U>); a none input short-circuits to none of the
    // result type.  The return type is the function's own result type.
    template<typename _Function>
    static auto bind(
        const ::djinterp::test_maybe<_Type>& _monad,
        _Function                                     _function
    )
        -> typename std::decay<
               decltype(_function(std::declval<_Type>()))>::type
    {
        using result_t = typename std::decay<
            decltype(_function(std::declval<_Type>()))>::type;

        if (_monad.is_some())
        {
            return _function(_monad.value());
        }

        return result_t::none();
    }
};


NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                IV.  SECTION FUNCTION DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////
//   One section function per like-group semantic section of monad.hpp.
// Each matches the framework's section signature
// `void(test::test_handler&)`.

void monad_tests_traits(::djinterp::test::test_handler& _handler);
void monad_tests_protocol(::djinterp::test::test_handler& _handler);
void monad_tests_operations(::djinterp::test::test_handler& _handler);
void monad_tests_composition(::djinterp::test::test_handler& _handler);
void monad_tests_pipeline(::djinterp::test::test_handler& _handler);


///////////////////////////////////////////////////////////////////////////////
///                V.   MODULE WIRING                                        ///
///////////////////////////////////////////////////////////////////////////////

// monad_module_info
//   constant: per-module identity bound at the in-output banner site.
extern const ::djinterp::test::test_module_info monad_module_info;

// monad_module_run_all
//   function: schedules every monad.hpp test section against the
// runner engine in document order.
void monad_module_run_all(::djinterp::test::test_runner_ctx& _ctx);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_FUNCTIONAL_MONAD_