/*******************************************************************************
* djinterp [test]                                           optionator_tests.hpp
*
*   Shared surface for the optionator.hpp unit suite.  optionator is the bridge
* from the options subframework to the functional dataflow trio: an
* option_set carried as a COMPILE-TIME environment, with operator|-pluggable
* stages that read that environment and contribute a source (producer<>), a
* transform (transducer), or a configured sink (consumer).  Testing it end to
* end therefore needs concrete stages, so this header supplies a small set of
* stand-ins built strictly against the real functional contracts:
*
*     - test_producer     a finite source emitting 1..n; models the producer
*                         protocol (nested value_type + const-nullary call
*                         returning producer_step), so is_producer accepts it.
*     - test_scale        a map transducer (value * factor); derives the real
*                         transducer_base, so is_transducer accepts it and the
*                         real transducer operator| composes it.
*     - test_over         a filter transducer (keep value > threshold); same.
*     - configures<...>   Pattern-C adapters binding each stage tag to the
*                         above, READING their parameter out of the carried
*                         option_set at compile time (env_int over field<>),
*                         which is exactly how a real adapter draws config from
*                         the environment.  src/scale/over read the environment;
*                         count reads a runtime arg (exercising stage()'s
*                         argument capture); sink reads a runtime pointer arg
*                         (a configured terminal draining into it).
*
*   The environment carries its compile-time values in the option_set's own
* field<> slot vocabulary (field<std::integral_constant<int,N>>), so the set is
* the ordinary kind option_set already forms; optionator only ever uses it as a
* TYPE (find/contains), never instantiating it.
*
*   optionator.hpp requires C++17 (the transducer layer + the option<> auto-NTTP
* pack form) and self-suppresses below it, so the stand-ins, the adapters, and
* every section body are gated to C++17 and emit empty below it; the block
* declarations and same/same_v stay standard-agnostic.
*
*
* path:      /tests/djinterp/core/option/optionator_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_OPTIONATOR_TESTS_
#define DJINTERP_OPTIONATOR_TESTS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
// djinterp [test] - framework runner
// djinterp [functional] - producer / transducer contracts the stand-ins model
#include "../../../../inc/djinterp/core/functional/producer.hpp"
#include "../../../../inc/djinterp/core/functional/transducer.hpp"
// djinterp [option] - option_set (field<> slots + queries) and the header under test
#include "../../../../inc/djinterp/core/option/option_set.hpp"
#ifdef DTEST_SPEC_MODE
#include <djinterp/test/test_defaults.hpp>
#endif  // DTEST_SPEC_MODE
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/option/optionator.hpp"
#endif  // !DTEST_SPEC_MODE


NS_DJINTERP
NS_TESTING

#ifndef DTEST_SPEC_MODE   // fixtures: normal face only


// same
//   trait: ORDER-SENSITIVE type identity, for asserting exact descriptor,
// option_chain, and option_set types.  Standard-agnostic.
template<typename _Lhs,
         typename _Rhs>
struct same
    : std::false_type
{};

template<typename _T>
struct same<_T, _T>
    : std::true_type
{};

template<typename _Lhs,
         typename _Rhs>
D_CONSTEXPR_INLINE bool same_v = same<_Lhs, _Rhs>::value;


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// ---- environment vocabulary ---------------------------------------------

// cfg
//   enum: the configuration keys the environment option_set carries.
enum class cfg
{
    count,
    factor,
    threshold,
    absent      // never placed in a set; for negative contains<> checks
};

// env_val
//   type: a field slot whose element type encodes a compile-time int, so a
// value rides in the option TYPE (which is all optionator reads).
template<int _N>
using env_val = ::djinterp::field<std::integral_constant<int, _N>>;

// env_int
//   trait: pull the compile-time int back out of an option<key, env_val<N>>.
// The stand-in adapters use this to read their parameter from the environment
// (option_set_find_t locates the option; env_int decodes its value).
template<typename _Option>
struct env_int;

template<auto        _Key,
         typename    _IntConst,
         typename... _Rest>
struct env_int< ::djinterp::option<_Key, ::djinterp::field<_IntConst>, _Rest...> >
{
    static constexpr int value = _IntConst::value;
};

// env
//   type: the canonical environment - count=5, factor=10, threshold=3.
using env = ::djinterp::option_set<
    ::djinterp::option<cfg::count,     env_val<5>>,
    ::djinterp::option<cfg::factor,    env_val<10>>,
    ::djinterp::option<cfg::threshold, env_val<3>> >;


// ---- stand-in pipeline stages (built against the real contracts) ---------

// test_producer
//   a finite source emitting 1, 2, ..., n.  Nested value_type + const-nullary
// operator() returning producer_step<int> satisfy the producer protocol; the
// cursor is mutable so the const call can advance, and the whole producer is
// copied by the driver (leaving the chain's source intact / re-runnable).
struct test_producer
{
    using value_type = int;

    int         m_n;
    mutable int m_cur;

    explicit test_producer(int _n)
        : m_n(_n)
        , m_cur(0)
    {}

    ::djinterp::producer_step<int>
    operator()() const
    {
        if (m_cur < m_n)
        {
            ++m_cur;
            return ::djinterp::make_step(m_cur);
        }
        return ::djinterp::no_step<int>();
    }
};

// test_scale
//   a map transducer: forwards each value multiplied by m_factor.  Derives the
// real transducer_base so is_transducer accepts it and operator| composes it;
// the returned reducer has the (state&, const value&) shape the driver folds.
struct test_scale
    : ::djinterp::transducer_base<test_scale>
{
    int m_factor;

    explicit test_scale(int _factor)
        : m_factor(_factor)
    {}

    template<typename _Downstream>
    auto
    operator()(
        _Downstream _downstream
    ) const
    {
        int factor = m_factor;
        return [factor, _downstream](auto& _state, const auto& _value) mutable
        {
            _downstream(_state, _value * factor);
        };
    }
};

// test_over
//   a filter transducer: forwards a value only when it exceeds m_threshold.
struct test_over
    : ::djinterp::transducer_base<test_over>
{
    int m_threshold;

    explicit test_over(int _threshold)
        : m_threshold(_threshold)
    {}

    template<typename _Downstream>
    auto
    operator()(
        _Downstream _downstream
    ) const
    {
        int threshold = m_threshold;
        return [threshold, _downstream](auto& _state, const auto& _value) mutable
        {
            if (_value > threshold)
            {
                _downstream(_state, _value);
            }
        };
    }
};


// test_take
//   a bounded transducer: forwards at most m_n values, then marks the reducing
// state done - the short-circuit that drives drive_into_consumer's is_done()
// exit.  Mirrors the real take transducer's mutable-counter closure.
struct test_take
    : ::djinterp::transducer_base<test_take>
{
    std::size_t m_n;

    explicit test_take(std::size_t _n)
        : m_n(_n)
    {}

    template<typename _Downstream>
    auto
    operator()(
        _Downstream _downstream
    ) const
    {
        std::size_t remaining = m_n;
        return [remaining, _downstream](auto& _state, const auto& _value) mutable
        {
            if (remaining == 0)
            {
                _state.mark_done();
                return;
            }
            _downstream(_state, _value);
            --remaining;
            if (remaining == 0)
            {
                _state.mark_done();
            }
        };
    }
};


// ---- stage tags ----------------------------------------------------------
//   Each names a configures<> adapter below, except unconfigured_tag, which
// deliberately has none (for is_configurable_stage's negative case).
struct src_tag          {};   // source, reads count from the environment
struct count_tag        {};   // source, reads a RUNTIME count argument
struct scale_tag        {};   // transform, reads factor from the environment
struct over_tag         {};   // transform, reads threshold from the environment
struct take_tag         {};   // transform, bounds the stream (RUNTIME count) -> mark_done
struct sink_tag         {};   // configured terminal, drains into a pointed-at vector
struct unconfigured_tag {};   // no adapter


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


// ===========================================================================
// section block-providers (one per translation unit)
#endif  // !DTEST_SPEC_MODE

// ===========================================================================
//
//   Each returns the section's block_spec; the runner assembles them into a
// single module_spec.  Names mirror the header's own section grouping.

// I.        configures / is_configurable_stage   (optionator_tests_configures.cpp)
::djinterp::test::block_spec optionator_configures_block();

// II + III. descriptors + factories              (optionator_tests_descriptors.cpp)
::djinterp::test::block_spec optionator_descriptors_block();

// IV.       pipeline internals                   (optionator_tests_internals.cpp)
::djinterp::test::block_spec optionator_internals_block();

// V + VI.   assembly + operator| execution       (optionator_tests_pipeline.cpp)
::djinterp::test::block_spec optionator_pipeline_block();

// VII.      traits & concepts                    (optionator_tests_traits.cpp)
::djinterp::test::block_spec optionator_traits_block();


// ===========================================================================
// SPEC PROVIDER   (spec face - the runner hands this to run_module)
// ===========================================================================
#ifdef DTEST_SPEC_MODE
inline ::djinterp::test::module_spec
optionator_spec()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::module_spec mod;
    mod.name       = "optionator.hpp";
    mod.descriptor =
        "option_set as a compile-time environment for an operator|-pluggable "
        "producer / transducer / consumer pipeline";

    mod.blocks.push_back(tt::optionator_configures_block());    // I
    mod.blocks.push_back(tt::optionator_descriptors_block());   // II + III
    mod.blocks.push_back(tt::optionator_internals_block());     // IV
    mod.blocks.push_back(tt::optionator_pipeline_block());      // V + VI
    mod.blocks.push_back(tt::optionator_traits_block());        // VII

    return mod;
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


// ---- configures<> adapters (specialized in djinterp, over the testing tags) --
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

NS_DJINTERP

    // source reading the environment's count
    template<>
    struct configures<testing::src_tag>
    {
        template<typename    _Options,
                 typename... _Args>
        static D_CONSTEXPR auto
        apply(_Args&&...)
        {
            constexpr int n = testing::env_int<
                option_set_find_t<_Options, testing::cfg::count>>::value;
            return testing::test_producer(n);
        }
    };

    // source reading a runtime count argument (exercises stage() arg capture)
    template<>
    struct configures<testing::count_tag>
    {
        template<typename _Options>
        static D_CONSTEXPR auto
        apply(int _n)
        {
            return testing::test_producer(_n);
        }
    };

    // transform reading the environment's factor
    template<>
    struct configures<testing::scale_tag>
    {
        template<typename    _Options,
                 typename... _Args>
        static D_CONSTEXPR auto
        apply(_Args&&...)
        {
            constexpr int f = testing::env_int<
                option_set_find_t<_Options, testing::cfg::factor>>::value;
            return testing::test_scale(f);
        }
    };

    // transform reading the environment's threshold
    template<>
    struct configures<testing::over_tag>
    {
        template<typename    _Options,
                 typename... _Args>
        static D_CONSTEXPR auto
        apply(_Args&&...)
        {
            constexpr int t = testing::env_int<
                option_set_find_t<_Options, testing::cfg::threshold>>::value;
            return testing::test_over(t);
        }
    };

    // bounding transform reading a runtime count (marks the stream done)
    template<>
    struct configures<testing::take_tag>
    {
        template<typename _Options>
        static D_CONSTEXPR auto
        apply(int _n)
        {
            return testing::test_take(static_cast<std::size_t>(_n));
        }
    };

    // configured terminal: build a consumer that drains into the pointed-at
    // vector (the sink derives from the pipe call site, via a runtime arg)
    template<>
    struct configures<testing::sink_tag>
    {
        template<typename _Options>
        static auto
        apply(std::vector<int>* _out)
        {
            return [_out](const int& _value)
            {
                _out->push_back(_value);
            };
        }
    };

NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_OPTIONATOR_TESTS_
