/*******************************************************************************
* djinterp [core]                                                 optionator.hpp
*
*   The bridge between the options subframework and the functional
* dataflow subframework: an option-configured, operator|-pluggable
* pipeline.  An `optionator<...>` carries an option_set as a COMPILE-TIME
* environment; stages piped into it draw their configuration from that
* environment and contribute a source (producer<>) or a transform
* (transducer); a terminal drives the assembled pipeline into a sink.
*
*   This is the Reader idiom expressed in djinterp's own vocabulary: the
* option_set is the environment, each stage is a Reader-action over it,
* and `into()` runs the Reader against the concrete options to obtain a
* producer -> consumer driver.
*
*   THE FUNCTOR MODULE STAYS UNTOUCHED.
*   optionator imposes no meaning on option args and knows nothing about
* any specific functor.  A stage becomes option-configurable purely by a
* `configures<Tag>` adapter (Pattern C) - which may live in a SEPARATE
* adapter header (see optionator_adapters.hpp).  The functor module the
* stage wraps therefore never imports the options machinery and is never
* edited: all option->parameter binding lives in the adapter.
*
*   DEPENDENCY DISCIPLINE.
*   The core depends only on option_set.hpp (find/contains, normalization
* via option_compose.hpp) and on the functional dataflow trio's detection
* surface (producer.hpp, transducer.hpp).  It does NOT depend on the tag
* vocabulary (option_tags.hpp) or the extractors (option_set_compare.hpp):
* reading values out of the environment is the adapter's job, so the bridge
* is free of any opinion about how an option carries its value.
*
*   SHAPE OF A PIPELINE.
*       optionator<opts...>                      // unsourced (environment)
*           | source_stage(...)                  // -> option_chain (sourced)
*           | transform_stage(...)               // extend (compose)
*           | transform_stage(...)               // extend (compose)
*           | into(sink);                         // drive -> void
*   ...or close with a CONFIGURED terminal that reads the same environment:
*           | terminal_stage(...);                // configures<>-built sink
*   The first stage MUST be a source (its configures<>::apply returns a
* producer<>); every later stage MUST be a transform (a transducer); a
* configured terminal's configures<>::apply returns a consumer (void(const A&)).
*
*
* path:      /inc/djinterp/core/option/optionator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.18
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    configures                  (Pattern-C adapter trait + detection)
      -----------------------------------------------------------------

II.   pending_stage + stage       (operator|-pluggable stage descriptor)
      ------------------------------------------------------------------

III.  terminals                   (into / collect / configured terminal)
      ------------------------------------------------------------------

IV.   pipeline internals          (identity_xform, driver, normalize_options)
      -----------------------------------------------------------------------

V.    optionator + option_chain   (unsourced head + sourced pipeline)
      ---------------------------------------------------------------

VI.   operator| wiring            (stage application + terminal application)
      ----------------------------------------------------------------------

VII.  traits & concepts           (is_optionator / is_option_chain / ...)
      -------------------------------------------------------------------
*/

#ifndef DJINTERP_OPTION_OPTIONATOR_HPP
#define DJINTERP_OPTION_OPTIONATOR_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "../meta/type_utility.hpp"  // clean_t
#include "./option_set.hpp"             // option_set<> + queries
#include "./option_compose.hpp"         // compose_options_t (inline-options front door)
#include "../functional/producer.hpp"   // producer<>, producer_step, is_producer
#include "../functional/transducer.hpp" // transducer_base, is_transducer,
                                        // reducing_state, transducer operator|


//   optionator leans on the transducer layer (C++14+) and the option<>
// pack form (auto NTTP, C++17), so the bridge as a whole requires C++17.
// Below that it is suppressed; the options and functional subframeworks
// remain independently usable.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP

// ===========================================================================
// I.   configures
// ===========================================================================

// configures
//   trait: the option-configuration adapter for a stage tag.  Pattern-C
// opt-in - a stage becomes option-configurable by specializing this for
// its tag.  The primary is left UNDEFINED so an unconfigured tag is a
// hard, named error (see is_configurable_stage and the operator| asserts).
//
//   A specialization must provide a single member template:
//
//       template<typename Options, typename... Args>
//       static D_CONSTEXPR auto apply(Args&&... _args);
//
// reading whatever it needs from the carried option_set Options at COMPILE
// TIME (e.g. via option_set_find_t + the extractors in
// option_set_compare.hpp) and returning either a producer<> (a SOURCE
// stage) or a transducer (a TRANSFORM stage).  _args are the runtime
// values captured at the pipe call site.
//
//   The specialization may live in a dedicated adapter header so the
// functor module the stage wraps is never touched.
template<typename Tag>
struct configures;


NS_INTERNAL

    // is_complete_helper
    //   helper: detects whether Type is a complete type.  Used only to
    // turn "no configures<> adapter for this tag" into a friendly
    // diagnostic; not a load-bearing trait.
    template<typename Type,
             typename = void>
    struct is_complete_helper : std::false_type
    {};

    template<typename Type>
    struct is_complete_helper<Type, decltype(void(sizeof(Type)))>
        : std::true_type
    {};


    // always_false
    //   helper: type-dependent false, so a static_assert inside a
    // discarded `if constexpr` branch fires only when that branch is
    // actually selected.
    template<typename...>
    struct always_false : std::false_type
    {};

NS_END  // internal


// is_configurable_stage
//   trait: true iff a configures<Tag> adapter has been defined.
template<typename Tag>
struct is_configurable_stage
    : internal::is_complete_helper<configures<Tag>>
{};


// ===========================================================================
// II.  pending_stage + stage
// ===========================================================================

NS_INTERNAL

    // pending_stage
    //   helper: an UNCONFIGURED stage descriptor.  Carries the stage tag
    // and the call-site arguments by value; it acquires its configuration
    // only when piped into an optionator, at which point operator| reads
    // the optionator's option_set and calls configures<Tag>::apply.
    template<typename    Tag,
             typename... Args>
    class pending_stage
    {
    public:
        using tag_type = Tag;

        explicit D_CONSTEXPR pending_stage(
            Args... _args
        )
            : m_args(std::move(_args)...)
        {}

        // build
        //   configure this stage against the option_set Options and
        // materialize it (a producer<> or a transducer).
        template<typename Options>
        D_CONSTEXPR auto
        build() const
        {
            return build_impl<Options>(std::index_sequence_for<Args...>{});
        }

    private:
        template<typename       Options,
                 std::size_t... I>
        D_CONSTEXPR auto
        build_impl(
            std::index_sequence<I...>
        ) const
        {
            return configures<Tag>::template apply<Options>(
                std::get<I>(m_args)...);
        }

        std::tuple<Args...> m_args;
    };

NS_END  // internal


// stage
//   function: build an unconfigured stage descriptor for Tag, capturing
// _args by value.  The generic, zero-boilerplate surface; a per-functor
// surface function (e.g. `render::source(...)`) is just a thin wrapper
// over this living in the adapter header.
template<typename    Tag,
         typename... Args>
D_NODISCARD
D_CONSTEXPR
internal::pending_stage<Tag, typename std::decay<Args>::type...>
stage(
    Args&&... _args
)
{
    return internal::pending_stage<Tag, typename std::decay<Args>::type...>(
        std::forward<Args>(_args)...);
}


// ===========================================================================
// III. terminals
// ===========================================================================

NS_INTERNAL

    // into_terminal
    //   helper: terminal descriptor carrying a sink, produced by into().
    // Applied by operator|(option_chain, into_terminal).
    template<typename Sink>
    class into_terminal
    {
    public:
        explicit into_terminal(
            Sink _sink
        )
            : m_sink(std::move(_sink))
        {}

        const Sink& sink() const D_NOEXCEPT { return m_sink; }
        Sink&       sink()       D_NOEXCEPT { return m_sink; }

    private:
        Sink m_sink;
    };


    // collect_terminal
    //   helper: terminal descriptor requesting a drain to
    // std::vector<Out>, produced by collect<Out>().
    template<typename Out>
    struct collect_terminal
    {
        using value_type = Out;
    };


    // pending_terminal
    //   helper: an UNCONFIGURED, OPTION-CONFIGURED terminal descriptor - the
    // sink counterpart of pending_stage.  Carries a terminal tag and the
    // call-site arguments by value; it acquires its consumer only when piped
    // onto an option_chain, at which point operator| reads the carried option_set
    // and calls configures<Tag>::apply.  For a terminal that adapter returns
    // a CONSUMER (a callable of shape void(const A&)) rather than a producer or
    // a transducer, and the assembled pipeline is then driven into it.  This is
    // what lets a sink derive itself from the environment - the report's
    // destinations, document type, and per-node routing all read from the same
    // option_set the rest of the pipeline draws on.
    template<typename    Tag,
             typename... Args>
    class pending_terminal
    {
    public:
        using tag_type = Tag;

        explicit D_CONSTEXPR pending_terminal(
            Args... _args
        )
            : m_args(std::move(_args)...)
        {}

        // build
        //   configure this terminal against the option_set Options and
        // materialize its consumer.
        template<typename Options>
        D_CONSTEXPR auto
        build() const
        {
            return build_impl<Options>(std::index_sequence_for<Args...>{});
        }

    private:
        template<typename       Options,
                 std::size_t... I>
        D_CONSTEXPR auto
        build_impl(
            std::index_sequence<I...>
        ) const
        {
            return configures<Tag>::template apply<Options>(
                std::get<I>(m_args)...);
        }

        std::tuple<Args...> m_args;
    };

NS_END  // internal


// into
//   function: terminal that drives the configured pipeline into a consumer
// sink - any callable of shape void(const A&), including the factories in
// namespace consumers.
template<typename Sink>
D_NODISCARD internal::into_terminal<typename std::decay<Sink>::type>
into(
    Sink&& _sink
)
{
    return internal::into_terminal<typename std::decay<Sink>::type>(
        std::forward<Sink>(_sink));
}


// collect
//   function: terminal that drains the configured pipeline into a
// std::vector<Out>, where Out is the element type the final stage emits.
template<typename Out>
D_NODISCARD D_CONSTEXPR internal::collect_terminal<Out>
collect()
{
    return internal::collect_terminal<Out>{};
}


// terminal
//   function: build an unconfigured, option-configured terminal descriptor for
// Tag, capturing _args by value.  The sink counterpart of stage(): where
// into()/collect() name a concrete sink, terminal() names a tag whose
// configures<> adapter READS the optionator's option_set to construct the
// consumer.  A per-functor surface (e.g. a report emitter) is a thin wrapper
// over this living in the adapter header, exactly as a source surface wraps
// stage().
template<typename    Tag,
         typename... Args>
D_NODISCARD D_CONSTEXPR internal::pending_terminal<Tag, typename std::decay<Args>::type...>
terminal(
    Args&&... _args
)
{
    return internal::pending_terminal<Tag, typename std::decay<Args>::type...>(
        std::forward<Args>(_args)...);
}


// ===========================================================================
// IV.  pipeline internals
// ===========================================================================

NS_INTERNAL

    // identity_xform
    //   helper: the seed transducer of an optionator chain - returns the
    // downstream reducer unchanged.  Lets the first transform stage be
    // handled uniformly; it is special-cased away in option_chain::through so
    // no trivial pass survives into the composed chain.
    struct identity_xform : transducer_base<identity_xform>
    {
        template<typename Reducer>
        D_CONSTEXPR Reducer
        operator()(
            Reducer _downstream
        ) const
        {
            return _downstream;
        }
    };


    // drive_into_consumer
    //   helper: pull _producer to exhaustion through _xform, forwarding
    // each survivor to _consumer.  Mirrors transduce_producer_to_consumer
    // (transducer.hpp) but types the downstream reducer with a GENERIC
    // value parameter, so a type-changing chain (e.g. a map stage) drives
    // correctly into a sink typed to the FINAL element type rather than to
    // the producer's value_type.  _producer is taken by value, so the
    // chain's stored source is left intact (the pipeline is re-runnable).
    template<typename Producer,
             typename Xform,
             typename Consumer>
    void
    drive_into_consumer(
        Producer      _producer,
        const Xform& _xform,
        Consumer&    _consumer
    )
    {
        auto downstream = [&_consumer](auto&       _state,
                                       const auto& _value)
        {
            _consumer(_value);
            (void)_state;

            return;
        };

        reducing_state<int> state(0);
        auto                wrapped = _xform(downstream);

        // pull the source until it signals exhaustion or a bounded stage
        // (take / take_while) marks the fold done
        while (true)
        {
            if (state.is_done())
            {
                break;
            }

            auto step = _producer();

            if (!step.has_value)
            {
                break;
            }

            wrapped(state, step.value);
        }

        return;
    }


    // normalize_options
    //   helper: canonicalize an optionator's option arguments.  A single
    // option_set passes through unchanged; any other pack of surfaces
    // (option<>/defopt<>, sub-sets, or none) is folded into one set via
    // the compose layer, so optionator<my_set> and
    // optionator<defopt<...>, ...> share one canonical environment type.
    template<typename... Options>
    struct normalize_options
    {
        using type = compose_options_t<Options...>;
    };

    template<typename... Os>
    struct normalize_options<option_set<Os...>>
    {
        using type = option_set<Os...>;
    };

NS_END  // internal


// ===========================================================================
// V.   optionator + option_chain
// ===========================================================================

// optionator
//   class: the UNSOURCED head of an option-configured pipeline.  It is the
// compile-time environment a stage reads from and carries no runtime
// state.  Pipe a SOURCE stage into it (operator|) to obtain a sourced
// pipeline (option_chain); pipe further TRANSFORM stages to extend it; finish
// with into() / collect().
//
//   Options may be a single option_set or a pack of surfaces (option<> /
// defopt<>); either way it is normalized to one canonical set, exposed as
// ::options_t.
//
// Usage:
//   optionator<my_options>{}
//       | render::source(text)
//       | render::clean()
//       | into(sink);
template<typename... Options>
struct optionator
{
    // options_t
    //   type: the canonical option_set this optionator carries (its
    // compile-time environment).
    using options_t = typename internal::normalize_options<Options...>::type;
};


NS_INTERNAL

    // option_chain
    //   helper: a SOURCED option-configured pipeline.  Holds the source
    // producer and the accumulated transducer by value (fully inlinable);
    // Set is the carried option_set, threaded unchanged across every
    // extension.  Built by operator|(optionator, source-stage), extended
    // by operator|(option_chain, transform-stage), run by into() / collect().
    template<typename Set,
             typename Producer,
             typename Xform>
    class option_chain
    {
    public:
        using options_t     = Set;
        using producer_t = Producer;
        using xform_t    = Xform;

        option_chain(
            Producer _producer,
            Xform     _xform
        )
            : m_producer(std::move(_producer)),
              m_xform(std::move(_xform))
        {}

        // through
        //   extend the chain with an already-configured transducer _stage,
        // returning the new option_chain type.  The identity_xform seed is
        // special-cased away so the first real transform composes alone.
        template<typename Stage>
        D_NODISCARD D_CONSTEXPR auto
        through(
            Stage _stage
        ) const
        {
            if constexpr (std::is_same<Xform, identity_xform>::value)
            {
                // first transform: replace the identity seed outright
                return option_chain<Set, Producer, Stage>(
                    m_producer,
                    std::move(_stage));
            }
            else
            {
                // subsequent transform: compose after the existing chain
                // via the transducer pipeline operator (reading order:
                // existing stages see values first).  operator| is
                // SFINAE-locked to transducer_base operands, so it never
                // collides with compose() over plain callables, and
                // is_transducer decays cv-ref so the const member binds.
                auto composed = (m_xform | std::move(_stage));

                return option_chain<Set, Producer, decltype(composed)>(
                    m_producer,
                    std::move(composed));
            }
        }

        // into
        //   drive the pipeline into a consumer sink (void(const A&)).
        template<typename Sink>
        void
        into(
            Sink _sink
        ) const
        {
            drive_into_consumer(m_producer, m_xform, _sink);

            return;
        }

        // collect
        //   drain the pipeline into a std::vector<Out>, where Out is the
        // element type the final stage emits.
        template<typename Out>
        D_NODISCARD
        std::vector<Out>
        collect() const
        {
            std::vector<Out> result;

            auto sink = [&result](const Out& _value)
            {
                result.push_back(_value);

                return;
            };

            drive_into_consumer(m_producer, m_xform, sink);

            return result;
        }

        const Producer& source()     const D_NOEXCEPT { return m_producer; }
        const Xform&    transducer() const D_NOEXCEPT { return m_xform; }

    private:
        Producer m_producer;
        Xform     m_xform;
    };

NS_END  // internal


// ===========================================================================
// VI.  operator| wiring
// ===========================================================================
//
//   Both overloads are tightly typed to optionator's own LHS and RHS
// templates, so they never collide with the operator| pipelines in
// view.hpp / monad.hpp / comparator.hpp / transducer.hpp.

// operator| (optionator | source-stage)
//   seed the pipeline: configure the piped stage against the optionator's
// option_set and require it to be a SOURCE (producer<>).  Yields a sourced
// option_chain whose transducer is the identity seed.
template<typename    Tag,
         typename... Args,
         typename... Options>
D_NODISCARD
auto operator|
(
    const optionator<Options...>&,
    const internal::pending_stage<Tag, Args...>& _stage
)
{
    using set_t = typename optionator<Options...>::options_t;

    static_assert(is_configurable_stage<Tag>::value,
        "optionator: no configures<> adapter for this stage tag.  Define "
        "a configures<> specialization (typically in an adapter header) "
        "describing how the stage reads its option_set.");

    auto first = _stage.template build<set_t>();

    static_assert(is_producer<decltype(first)>::value,
        "optionator: the FIRST stage piped into an optionator must be a "
        "SOURCE - its configures<>::apply must return a producer<>.  "
        "Transform stages may only follow a seeded source.");

    return internal::option_chain<set_t,
                               decltype(first),
                               internal::identity_xform>(
        std::move(first),
        internal::identity_xform{});
}


// operator| (option_chain | transform-stage)
//   extend a sourced pipeline: configure the piped stage against the
// carried option_set and compose it.  It must be a TRANSFORM (a
// transducer); a second source, or anything else, is a named hard error.
template<typename    Tag,
         typename... Args,
         typename    Set,
         typename    Producer,
         typename    Xform>
D_NODISCARD
auto operator|
(
    const internal::option_chain<Set, Producer, Xform>& _chain,
    const internal::pending_stage<Tag, Args...>&      _stage
)
{
    static_assert(is_configurable_stage<Tag>::value,
        "optionator: no configures<> adapter for this stage tag.");

    auto produced = _stage.template build<Set>();

    if constexpr (is_transducer<decltype(produced)>::value)
    {
        return _chain.through(std::move(produced));
    }
    else if constexpr (is_producer<decltype(produced)>::value)
    {
        static_assert(internal::always_false<Tag>::value,
            "optionator: a source has already been seeded; only transform "
            "stages (transducers) may follow it.");
    }
    else
    {
        static_assert(internal::always_false<Tag>::value,
            "optionator: a configures<>::apply must return either a "
            "producer<> (a source) or a transducer (a transform).");
    }
}


// operator| (option_chain | into-terminal)
//   run the assembled pipeline, forwarding every survivor to the sink.
template<typename Set,
         typename Producer,
         typename Xform,
         typename Sink>
void operator|
(
    const internal::option_chain<Set, Producer, Xform>& _chain,
    const internal::into_terminal<Sink>&               _terminal
)
{
    _chain.into(_terminal.sink());

    return;
}

// operator| (option_chain | configured terminal)
//   configure the terminal against the carried option_set to obtain a consumer
// (void(const A&)), then drive the assembled pipeline into it.  The
// configured-sink counterpart of operator|(option_chain, into_terminal): there the
// sink is supplied concretely, here it is BUILT from the environment, so the
// whole pipeline - source, transforms, and sink - is driven by the single
// option_set the optionator carries.
template<typename    Tag,
         typename... Args,
         typename    Set,
         typename    Producer,
         typename    Xform>
void operator|
(
    const internal::option_chain<Set, Producer, Xform>&  _chain,
    const internal::pending_terminal<Tag, Args...>&    _terminal
)
{
    static_assert(is_configurable_stage<Tag>::value,
        "optionator: no configures<> adapter for this terminal tag.  Define a "
        "configures<> specialization whose apply() returns a consumer "
        "(void(const A&)) built from the option_set.");

    auto sink = _terminal.template build<Set>();

    _chain.into(sink);

    return;
}


// operator| (option_chain | collect-terminal)
//   run the assembled pipeline, draining survivors into a std::vector.
template<typename Set,
         typename Producer,
         typename Xform,
         typename Out>
D_NODISCARD
std::vector<Out> operator|
(
    const internal::option_chain<Set, Producer, Xform>& _chain,
    const internal::collect_terminal<Out>&
)
{
    return _chain.template collect<Out>();
}


// ===========================================================================
// VII. traits & concepts
// ===========================================================================

// is_optionator
//   trait: true iff Type is an (unsourced) optionator<...> head.
template<typename Type>
struct is_optionator : std::false_type
{};

template<typename... Options>
struct is_optionator<optionator<Options...>> : std::true_type
{};

template<typename Type>
D_CONSTEXPR bool is_optionator_v =
    is_optionator<clean_t<Type>>::value;


// is_option_chain
//   trait: true iff Type is a sourced option_chain pipeline.
template<typename Type>
struct is_option_chain : std::false_type
{};

template<typename Set,
         typename Producer,
         typename Xform>
struct is_option_chain<internal::option_chain<Set, Producer, Xform>>
    : std::true_type
{};

template<typename Type>
D_CONSTEXPR bool is_option_chain_v = is_option_chain<clean_t<Type>>::value;


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    // optionator_c
    //   concept: satisfied by an unsourced optionator<...> head.
    template<typename Type>
    concept Optionator = is_optionator_v<Type>;

    // opt_chain_c
    //   concept: satisfied by a sourced option_chain pipeline.
    template<typename Type>
    concept OptionChain = is_option_chain_v<Type>;

    // configurable_stage_c
    //   concept: satisfied iff a configures<Tag> adapter exists for Tag.
    template<typename Tag>
    concept ConfigurableStage = is_configurable_stage<Tag>::value;

#endif  // C++20 concepts available


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_OPTION_OPTIONATOR_HPP
