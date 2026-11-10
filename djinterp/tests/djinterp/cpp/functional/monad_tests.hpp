/*******************************************************************************
* djinterp [testing]                                             monad_tests.hpp
*
*   DTest declarations and shared helper types for the monad.hpp unit
* test suite.  Each like-group semantic section of monad.hpp maps to a
* single `bool()` section function declared here and
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
* small, self-contained `DTestMaybe<T>` monad (a maybe / optional) and
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
*
* path:      /tests/djinterp/cpp/functional/monad_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.31
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTING_FUNCTIONAL_MONAD_
#define DJINTERP_TESTING_FUNCTIONAL_MONAD_ 1

// std
#include <cstdio>
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include <djinterp/cpp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include <djinterp/cpp/meta/type_traits.hpp>
#include <djinterp/cpp/functional/monad.hpp>
#include <djinterp/cpp/modules/test/test_traits.hpp>  // D_TEST_STATIC
#endif
#ifdef DTEST_SPEC_MODE
#include <djinterp/cpp/modules/test/test_defaults.hpp>  // module_spec, run_module
#endif



// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

///////////////////////////////////////////////////////////////////////////////
///                I.   THE TEST MONAD                                       ///
///////////////////////////////////////////////////////////////////////////////
//   DTestMaybe and its some() factory live directly in djinterp (NOT in
// djinterp::testing) on purpose: a real monad type lives in the same
// namespace as monad.hpp's operator|, so that argument-dependent lookup
// finds djinterp::operator| when a value is piped.  Defining the test
// monad here reproduces that arrangement faithfully; were it nested in
// djinterp::testing, ADL would not reach djinterp::operator| and the
// pipeline tests would not represent real usage.

// DTestMaybe
//   class: a minimal maybe / optional monad used to exercise the
// generic operations in monad.hpp.  Either holds a value (some) or is
// empty (none).  Equality is provided so assertions can compare
// results directly.
template<typename _Type>
class DTestMaybe
{
public:
    using value_type = _Type;

    // none-constructing default.
    DTestMaybe()
        : m_has(false),
          m_value()
    {}

    // some-constructing.
    explicit DTestMaybe(
        _Type _value
    )
        : m_has(true),
          m_value(std::move(_value))
    {}

    // none
    //   factory: the empty value of this monad.
    static
    DTestMaybe none()
    {
        return DTestMaybe();
    }

    // is_some
    //   returns: whether a value is held.
    bool is_some() const
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
    //   compares two DTestMaybe values: both none, or both some with
    // equal values.
    bool operator==(
        const DTestMaybe& _other
    ) const
    {
        return (m_has == _other.m_has) &&
               ( !m_has || (m_value == _other.m_value) );
    }

    bool operator!=(
        const DTestMaybe& _other
    ) const
    {
        return !(*this == _other);
    }

private:
    bool  m_has;
    _Type m_value;
};

// some
//   factory: constructs a some-valued DTestMaybe<T> (deduces T).
template<typename _Type>
DTestMaybe<typename std::decay<_Type>::type> some(
    _Type&& _value
)
{
    return DTestMaybe<typename std::decay<_Type>::type>(
        std::forward<_Type>(_value));
}


NS_TESTING

// record_assertion
//   function: one check of a converted body: on failure it prints what was
// expected and clears the body's verdict.  (The bodies used DTest's retired
// handler API, record_assertion(handler, condition, what).)
inline
void record_assertion(
    bool&       _ok,
    bool        _condition,
    const char* _what
)
{
    if (!_condition)
    {
        std::printf("    ! %s\n", _what);
        _ok = false;
    }

    return;
}

// record_assertion (with detail)
//   function: the five-argument form the bodies also used: the detail and
// the extra text are printed with a failure.
inline
void record_assertion(
    bool&       _ok,
    bool        _condition,
    const char* _what,
    const char* _detail,
    const char* _extra = nullptr
)
{
    if (!_condition)
    {
        std::printf("    ! %s (%s%s%s)\n",
                    _what,
                    _detail ? _detail : "",
                    _extra ? "; " : "",
                    _extra ? _extra : "");
        _ok = false;
    }

    return;
}


// DUnitTestTally
//   struct: kept for the converted bodies' run_unit_test calls; a body's
// verdict is its bool now, so the tally holds nothing.
struct DUnitTestTally
{
};

// run_unit_test
//   function: runs one named unit inside a converted body, and prints its
// name when it was the unit that first failed.
template<typename Fn>
void run_unit_test(
    bool&           _ok,
    DUnitTestTally& _tally,
    const char*     _name,
    Fn              _unit
)
{
    const bool before = _ok;

    (void)_tally;
    _unit();

    if ( (before) &&
         (!_ok) )
    {
        std::printf("    [FAIL] %s\n", _name);
    }

    return;
}

// D_MONAD_CHECK
//   macro: one check of a converted body, its text the message.
#define D_MONAD_CHECK(...) \
    record_assertion(verdict, (__VA_ARGS__), #__VA_ARGS__)



///////////////////////////////////////////////////////////////////////////////
///                II.  SHARED ARROWS & FUNCTIONS                            ///
///////////////////////////////////////////////////////////////////////////////
//   Named functor types (not generic lambdas) so the suite compiles on
// the C++11 baseline.  "Arrows" are Kleisli arrows T -> M<U>; "plain"
// functions are ordinary T -> U used with monad_map.

// arrow_inc
//   struct: Kleisli arrow int -> DTestMaybe<int> adding one.
struct arrow_inc
{
    DTestMaybe<int> operator()(
        const int& _x
    ) const
    {
        return DTestMaybe<int>(_x + 1);
    }
};

// DArrowTimesTen
//   struct: Kleisli arrow int -> DTestMaybe<int> multiplying by ten.
struct DArrowTimesTen
{
    DTestMaybe<int> operator()(
        const int& _x
    ) const
    {
        return DTestMaybe<int>(_x * 10);
    }
};

// DArrowToNone
//   struct: Kleisli arrow int -> DTestMaybe<int> that always fails
// (returns none).  Drives short-circuit paths.
struct DArrowToNone
{
    DTestMaybe<int> operator()(
        const int&
    ) const
    {
        return DTestMaybe<int>::none();
    }
};

// DArrowToString
//   struct: Kleisli arrow int -> DTestMaybe<std::string> (type-changing).
struct DArrowToString
{
    DTestMaybe<std::string> operator()(
        const int& _x
    ) const
    {
        return DTestMaybe<std::string>(std::to_string(_x));
    }
};

// plain_double
//   struct: ordinary transform int -> int doubling (for monad_map).
struct plain_double
{
    int operator()(
        const int& _x
    ) const
    {
        return _x * 2;
    }
};

// DPlainToString
//   struct: ordinary transform int -> std::string (type-changing map).
struct DPlainToString
{
    std::string operator()(
        const int& _x
    ) const
    {
        return std::to_string(_x);
    }
};

// DBinaryAdd
//   struct: binary function (int, int) -> int for lift_m2.
struct DBinaryAdd
{
    int operator()(
        const int& _a,
        const int& _b
    ) const
    {
        return _a + _b;
    }
};

// DBinaryConcatSum
//   struct: binary function (int, int) -> std::string (type-changing
// lift_m2) producing the decimal string of the sum.
struct DBinaryConcatSum
{
    std::string operator()(
        const int& _a,
        const int& _b
    ) const
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
///                III. monad_traits SPECIALIZATION FOR DTestMaybe           ///
///////////////////////////////////////////////////////////////////////////////
//   Must live in djinterp:: (where monad_traits is declared), NOT in
// the generic operations look up
// monad_traits<M> by exact specialization.  DTestMaybe lives directly
// in djinterp (see Section I), so the specialization names it there.
//   (We are already inside djinterp here, having just closed the inner
// testing namespace above; the specialization is added directly.)

// monad_traits<DTestMaybe<T>>
//   trait: the monad protocol implementation for the test monad.
// Provides value_type, rebind, unit, and a short-circuiting bind --
// the three primitives the generic operations build on.
template<typename _Type>
struct monad_traits< ::djinterp::DTestMaybe<_Type> >
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
    using rebind = ::djinterp::DTestMaybe<_To>;

    // unit
    //   function: lifts a plain value into a some-valued monad.
    static
    ::djinterp::DTestMaybe<_Type> unit(
        _Type _value
    )
    {
        return ::djinterp::DTestMaybe<_Type>(std::move(_value));
    }

    // bind
    //   function: threads the held value through _function (a Kleisli
    // arrow T -> M<U>); a none input short-circuits to none of the
    // result type.  The return type is the function's own result type.
    template<typename _Function>
    static
    auto bind(
        const ::djinterp::DTestMaybe<_Type>& _monad,
        _Function                            _function
    ) -> typename std::decay< decltype(_function(std::declval<_Type>()))>::type
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


#endif  // !DTEST_SPEC_MODE  (fixtures)


NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                IV.  SECTION FUNCTION DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////
//   One section function per like-group semantic section of monad.hpp, each
// returning its verdict.

bool monad_tests_traits();
bool monad_tests_protocol();
bool monad_tests_operations();
bool monad_tests_composition();
bool monad_tests_pipeline();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// monad_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
monad_spec()
{
    return dt::module_spec{
        "monad",
        "monad.hpp",
        {
            dt::block_spec{
                "traits",
                "Traits",
                {
                    { "monad_tests_traits",
                      "Exercises monad.hpp Section 0 (predicate SFINAE traits "
                      "+ concepts)",
                      &monad_tests_traits },
                }
            },
            dt::block_spec{
                "protocol",
                "Protocol",
                {
                    { "monad_tests_protocol",
                      "Exercises monad.hpp Section I (is_monad, monad_traits)",
                      &monad_tests_protocol },
                }
            },
            dt::block_spec{
                "operations",
                "Operations",
                {
                    { "monad_tests_operations",
                      "Exercises monad.hpp Section II core operations",
                      &monad_tests_operations },
                }
            },
            dt::block_spec{
                "composition",
                "Composition",
                {
                    { "monad_tests_composition",
                      "Exercises monad.hpp Section II sequencing/composition",
                      &monad_tests_composition },
                }
            },
            dt::block_spec{
                "pipeline",
                "Pipeline",
                {
                    { "monad_tests_pipeline",
                      "Exercises monad.hpp Sections III + IV (combinators, "
                      "operator|)",
                      &monad_tests_pipeline },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_FUNCTIONAL_MONAD_
