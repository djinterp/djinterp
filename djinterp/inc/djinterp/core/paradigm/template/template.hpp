/******************************************************************************
* djinterp [paradigm]                                             template.hpp
*
* Type-agnostic templating foundation (C++).
*   This header defines the minimal structural vocabulary for the djinterp
* templating subframework:  source --> template + f(t) --> result.  A
* TEMPLATE is a sequence of FRAGMENTS; a SOURCE resolves slots to values; a
* SINK absorbs emitted values; functional OPERATORS (map, default, gate)
* transform what flows between them.  Everything here is domain-neutral -
* no characters, no bytes - so text_template.hpp, binary_template.hpp, and
* friends can each instantiate the same machinery over their own value
* domains.
*   The design is TAGLESS: no fragment, source, or sink is required to
* inherit from anything.  Role membership is decided purely structurally
* (SFINAE), by what an entity can DO:
*
*     sink      anything invocable with an emitted value:   sink(v)
*     source    anything const-invocable with a key:        source(k) -> r
*               (if r is optional-like - bool-testable and dereferenceable -
*               it is treated as "maybe a value"; otherwise as the value)
*     fragment  one of three structural kinds, dispatched in priority order:
*                 1. RENDERABLE  - exposes  frag.render(source, sink)
*                                  (slots, sequences, operator wrappers,
*                                  nested templates, and any user type)
*                 2. LITERAL     - the sink can consume it directly: sink(frag)
*                 3. RESOLVABLE  - the source can resolve it:  source(frag)
*
*   Literals outrank raw resolution deliberately: pass-through is the
* common, unsurprising case, and resolution is requested explicitly via
* templates::slot(key) (or by using a dedicated key type the sink cannot
* consume).  A generic-lambda source therefore cannot hijack literal
* fragments.
*
*   basic_template<Fragments...> is itself just a renderable fragment over a
* heterogeneous std::tuple, so templates nest and concatenate freely, and
* the entire structure is fixed during translation.  Every primitive is
* D_CONSTEXPR-constructible and render() is D_CONSTEXPR14, so a template
* rendered through constexpr sources and sinks folds at compile time; the
* same template drives a runtime sink unchanged (dual domain, cf. the
* functional module).
*
* USAGE:
*   std::map<std::string, std::string> values =
*       { { "user", "teer" }, { "n", "3" } };
*
*   auto greeting = templates::seq(
*       "Hello, ",                              // literal
*       templates::slot(std::string("user")),   // resolved against source
*       "! You have ",
*       templates::slot(std::string("n"))
*           | templates::defaulted(std::string("no")),
*       " new messages."
*   );
*
*   std::vector<std::string> out;
*   render(greeting, templates::from_lookup(values), templates::into(out));
*
*   // layered sources: per-render values over global defaults
*   auto src = templates::overlay(templates::from_lookup(values),
*                                 templates::constant_source(
*                                     std::string("<missing>")));
*
*   // functional operators at any granularity
*   auto loud = greeting | templates::mapped(to_upper);
*
*
* path:      /inc/djinterp/core/paradigm/template/template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    STRUCTURAL PROTOCOL TRAITS
      1.  is_optional_like<T>                   (bool-testable + dereferenceable)
      2.  is_sink_for<Sink, Value>              (sink(value) is well-formed)
      3.  is_source_for<Source, Key>            (source(key) is well-formed)
      4.  source_yield_t<Source, Key>           (what source(key) yields)
      5.  is_renderable<Frag, Source, Sink>     (frag.render(source, sink))
      6.  is_fragment<Frag, Source, Sink>       (renderable || literal || resolvable)
II.   FRAGMENT DISPATCH                         (internal)
      1.  emit_resolved                         (optional-like unwrap -> sink)
      2.  render_fragment                       (3-way structural dispatch)
      3.  render_tuple_helper                   (fold over fragment tuple)
      4.  mapped_sink_helper                    (sink decorator: f(v) -> sink)
      5.  counting_sink_helper                  (sink decorator: count emissions)
III.  FRAGMENT HELPERS                           (internal)
      1.  literal_helper                        (forced pass-through)
      2.  slot_helper                           (explicit resolution point)
      3.  mapped_helper                         (transform emitted values)
      4.  defaulted_helper                      (emit fallback when silent)
      5.  when_helper                           (predicate-gated rendering)
      6.  mapped_adapter / defaulted_adapter /
          when_adapter                          (pipeline RHS forms)
IV.   SOURCE / SINK HELPERS                      (internal)
      1.  constant_source_helper                (every key -> same value)
      2.  null_source_helper                    (every key -> nothing)
      3.  overlay_source_helper                 (primary, else fallback)
      4.  lookup_source_helper                  (map.find -> pointer)
      5.  into_helper                           (push_back sink)
V.    BASIC TEMPLATE
      1.  basic_template<Fragments...>          (tuple of fragments; renderable)
VI.   FACTORIES (namespace templates)
      1.  literal(v)
      2.  slot(key) / slot(key, fn)
      3.  seq(fragments...)
      4.  mapped(frag, fn)                      (also single-arg adapter form)
      5.  defaulted(frag, v)                    (also single-arg adapter form)
      6.  when(pred, frag)                      (also single-arg adapter form)
      7.  constant_source(v) / null_source<V>()
      8.  overlay(s1, s2, ...)
      9.  from_lookup(map)
      10. into(container)
VII.  PIPELINE OPERATORS
      1.  operator|(fragment, mapped_adapter)
      2.  operator|(fragment, defaulted_adapter)
      3.  operator|(fragment, when_adapter)
VIII. RENDER DRIVERS
      1.  render(frag, source, sink)
      2.  render_to<Container>(frag, source)
      3.  join(t1, t2)
IX.   MODULE TRAITS & CONCEPTS
      1.  is_basic_template<T>
      2.  *_v aliases (C++14)
      3.  OptionalLike / SinkFor / SourceFor /
          Renderable / Fragment (C++20)
*/


#ifndef DJINTERP_PARADIGM_TEMPLATE_
#define DJINTERP_PARADIGM_TEMPLATE_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"


// D_CONSTEXPR14
//   macro: resolves to D_CONSTEXPR where relaxed (C++14) constexpr is
// available, and to nothing otherwise.  Mirrors consumer.hpp and
// transducer.hpp so the spelling stays consistent across modules; render
// bodies mutate local state (loop indices, emission counters) and are
// therefore constexpr only under the relaxed rules.
#ifndef D_CONSTEXPR14
#  if D_ENV_LANG_IS_CPP14_OR_HIGHER
#    define D_CONSTEXPR14 D_CONSTEXPR
#  else
#    define D_CONSTEXPR14
#  endif
#endif


NS_DJINTERP


//   DUAL DOMAIN.  A template is a pure description: a tuple of fragments whose
// shape is fixed during translation.  Rendering is a fold of that description
// against a source and into a sink, and the fold itself carries D_CONSTEXPR14 -
// so the SAME template renders into a constexpr accumulator at compile time or
// streams into a std::vector at run time, unchanged.  Only the source and sink
// decide the domain.  This mirrors the step/driver split in the functional
// module: fragments are the steps, render_fragment is the driver.

///////////////////////////////////////////////////////////////////////////////
///             I.    STRUCTURAL PROTOCOL TRAITS                            ///
///////////////////////////////////////////////////////////////////////////////


//  is_optional_like


NS_INTERNAL

    // is_optional_like_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct is_optional_like_helper : std::false_type
    {};

    // is_optional_like_helper (success case)
    //   trait: succeeds when a const _Type is both contextually
    // convertible to bool (is there a value?) and dereferenceable
    // (give me the value).  maybe<T>, T*, and std::optional<T> all
    // satisfy it; plain values do not, so resolution results that
    // are not optional-like are emitted directly.
    template<typename _Type>
    struct is_optional_like_helper<
        _Type,
        void_t<
            decltype(static_cast<bool>(std::declval<const _Type&>())),
            decltype(*std::declval<const _Type&>())
        >
    > : std::true_type
    {};

NS_END  // internal

// is_optional_like
//   trait: detects whether _Type models the optional protocol
// (bool-testable and dereferenceable).  Resolution results of this
// shape are unwrapped before emission; "nothing" emits nothing.
template<typename _Type>
struct is_optional_like
    : internal::is_optional_like_helper<clean_t<_Type>>
{};



//  is_sink_for


NS_INTERNAL

    // is_sink_for_helper
    //   trait: primary template (failure case).
    template<typename _Sink,
             typename _Value,
             typename = void>
    struct is_sink_for_helper : std::false_type
    {};

    // is_sink_for_helper (success case)
    //   trait: succeeds when _Sink is invocable with a const
    // _Value&.  This is the exact call shape the renderer uses to
    // emit, so the trait doubles as the structural definition of
    // the sink role for that value type.
    template<typename _Sink,
             typename _Value>
    struct is_sink_for_helper<
        _Sink,
        _Value,
        void_t<decltype(
            std::declval<_Sink&>()(std::declval<const _Value&>())
        )>
    > : std::true_type
    {};

NS_END  // internal

// is_sink_for
//   trait: detects whether _Sink can absorb values of type _Value,
// i.e. `sink(value)` is well-formed.  Any callable qualifies -
// lambdas, consumer.hpp helpers, into(container) - no base class
// is required.
template<typename _Sink,
         typename _Value>
struct is_sink_for
    : internal::is_sink_for_helper<clean_t<_Sink>, clean_t<_Value>>
{};



//  is_source_for  /  source_yield_t


NS_INTERNAL

    // is_source_for_helper
    //   trait: primary template (failure case).
    template<typename _Source,
             typename _Key,
             typename = void>
    struct is_source_for_helper : std::false_type
    {};

    // is_source_for_helper (success case)
    //   trait: succeeds when a const _Source is invocable with a
    // const _Key&.  Sources are read-only during a render, so
    // const-invocability is part of the contract.
    template<typename _Source,
             typename _Key>
    struct is_source_for_helper<
        _Source,
        _Key,
        void_t<decltype(
            std::declval<const _Source&>()(std::declval<const _Key&>())
        )>
    > : std::true_type
    {};

    // source_yield_helper
    //   trait: primary template (failure case; no `type`).
    template<typename _Source,
             typename _Key,
             typename = void>
    struct source_yield_helper
    {};

    // source_yield_helper (success case)
    //   trait: exposes the type yielded by `source(key)`.
    template<typename _Source,
             typename _Key>
    struct source_yield_helper<
        _Source,
        _Key,
        void_t<decltype(
            std::declval<const _Source&>()(std::declval<const _Key&>())
        )>
    >
    {
        using type = decltype(
            std::declval<const _Source&>()(std::declval<const _Key&>())
        );
    };

NS_END  // internal

// is_source_for
//   trait: detects whether _Source can resolve keys of type _Key,
// i.e. `source(key)` is well-formed on a const source.  The yield
// may be a plain value (emitted as-is) or optional-like (unwrapped;
// "nothing" emits nothing).
template<typename _Source,
         typename _Key>
struct is_source_for
    : internal::is_source_for_helper<clean_t<_Source>, clean_t<_Key>>
{};

// source_yield_t
//   type: the (possibly reference, possibly optional-like) type a
// source yields for a key.  SFINAE-friendly: ill-formed when
// is_source_for is false.
template<typename _Source,
         typename _Key>
using source_yield_t =
    typename internal::source_yield_helper<clean_t<_Source>, clean_t<_Key>>::type;



//  is_renderable


NS_INTERNAL
    // is_renderable_helper
    //   trait: primary template (failure case).
    template<typename _Fragment,
             typename _Source,
             typename _Sink,
             typename = void>
    struct is_renderable_helper : std::false_type
    {};

    // is_renderable_helper (success case)
    //   trait: succeeds when a const _Fragment exposes
    // `render(const _Source&, _Sink&)`.  Self-rendering fragments
    // own their behavior completely: slots, operator wrappers,
    // nested templates, and arbitrary user types all enter the
    // dispatch through this single structural door.
    template<typename _Fragment,
             typename _Source,
             typename _Sink>
    struct is_renderable_helper<
        _Fragment,
        _Source,
        _Sink,
        void_t<decltype(
            std::declval<const _Fragment&>().render(
                std::declval<const _Source&>(),
                std::declval<_Sink&>()
            )
        )>
    > : std::true_type
    {};

NS_END  // internal

// is_renderable
//   trait: detects whether _Fragment exposes a
// `render(const _Source&, _Sink&)` member - the self-rendering
// fragment shape, and the highest-priority kind in dispatch.
template<typename _Fragment,
         typename _Source,
         typename _Sink>
struct is_renderable
    : internal::is_renderable_helper<clean_t<_Fragment>,
                                     clean_t<_Source>,
                                     clean_t<_Sink>>
{};

// is_fragment
//   trait: detects whether _Fragment participates in rendering at
// all against a given _Source and _Sink - i.e. it is renderable,
// sink-consumable (a literal), or source-resolvable.  This is the
// disjunction the dispatch static_asserts against, so failures
// surface as one readable diagnostic instead of a deep
// instantiation trace.
template<typename _Fragment,
         typename _Source,
         typename _Sink>
struct is_fragment
{
    static constexpr bool value =
        ( is_renderable<_Fragment, _Source, _Sink>::value ||
          is_sink_for<_Sink, _Fragment>::value            ||
          is_source_for<_Source, _Fragment>::value );
};


///////////////////////////////////////////////////////////////////////////////
///             II.   FRAGMENT DISPATCH                                     ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // emit_resolved (optional-like case)
    //   function: unwraps an optional-like resolution result into
    // the sink; "nothing" emits nothing.  Pointers, maybe<T>, and
    // std::optional<T> all route here.
    template<typename _Resolved,
             typename _Sink>
    D_CONSTEXPR14 typename std::enable_if<
        is_optional_like<_Resolved>::value
    >::type
    emit_resolved(
        _Resolved&& _resolved,
        _Sink&      _sink
    )
    {
        // emit only when a value is present
        if (static_cast<bool>(_resolved))
        {
            _sink(*_resolved);
        }

        return;
    }

    // emit_resolved (plain-value case)
    //   function: forwards a non-optional resolution result
    // directly into the sink.
    template<typename _Resolved,
             typename _Sink>
    D_CONSTEXPR14 typename std::enable_if<
        !is_optional_like<_Resolved>::value
    >::type
    emit_resolved(
        _Resolved&& _resolved,
        _Sink&      _sink
    )
    {
        _sink(std::forward<_Resolved>(_resolved));

        return;
    }


    // render_fragment (renderable case)
    //   function: highest-priority dispatch - the fragment renders
    // itself.  Slots, sequences, operator wrappers, nested
    // templates, and user-defined fragments all take this path.
    template<typename _Fragment,
             typename _Source,
             typename _Sink>
    D_CONSTEXPR14 typename std::enable_if<
        is_renderable<_Fragment, _Source, _Sink>::value
    >::type
    render_fragment(
        const _Fragment& _fragment,
        const _Source&   _source,
        _Sink&           _sink
    )
    {
        _fragment.render(_source, _sink);

        return;
    }

    // render_fragment (literal case)
    //   function: the fragment is not renderable but the sink can
    // consume it directly, so it passes through untouched.
    // Literals outrank raw resolution so a generic-lambda source
    // cannot hijack pass-through content; resolution is requested
    // explicitly via templates::slot.
    template<typename _Fragment,
             typename _Source,
             typename _Sink>
    D_CONSTEXPR14 typename std::enable_if<
        ( !is_renderable<_Fragment, _Source, _Sink>::value &&
          is_sink_for<_Sink, _Fragment>::value )
    >::type
    render_fragment(
        const _Fragment& _fragment,
        const _Source&   _source,
        _Sink&           _sink
    )
    {
        (void)_source;
        _sink(_fragment);

        return;
    }

    // render_fragment (resolvable case)
    //   function: the fragment is neither renderable nor
    // sink-consumable, but the source can resolve it - the
    // dedicated-key-type idiom, where a domain key type that the
    // sink cannot absorb resolves without an explicit slot wrapper.
    template<typename _Fragment,
             typename _Source,
             typename _Sink>
    D_CONSTEXPR14 typename std::enable_if<
        ( !is_renderable<_Fragment, _Source, _Sink>::value &&
          !is_sink_for<_Sink, _Fragment>::value            &&
          is_source_for<_Source, _Fragment>::value )
    >::type
    render_fragment(
        const _Fragment& _fragment,
        const _Source&   _source,
        _Sink&           _sink
    )
    {
        emit_resolved(_source(_fragment), _sink);

        return;
    }

    // render_fragment (error case)
    //   function: no structural kind matched; fail with one
    // readable diagnostic at the dispatch site instead of a deep
    // instantiation trace.
    template<typename _Fragment,
             typename _Source,
             typename _Sink>
    D_CONSTEXPR14 typename std::enable_if<
        !is_fragment<_Fragment, _Source, _Sink>::value
    >::type
    render_fragment(
        const _Fragment& _fragment,
        const _Source&   _source,
        _Sink&           _sink
    )
    {
        static_assert(is_fragment<_Fragment, _Source, _Sink>::value,
                      "Fragment is not renderable (no `render(source, sink)` "
                      "member), not consumable by the sink, and not "
                      "resolvable by the source. At least one structural "
                      "kind must apply.");

        (void)_fragment;
        (void)_source;
        (void)_sink;

        return;
    }


    // render_tuple_helper
    //   helper: recursive fold over a fragment tuple (primary /
    // stepping case).  Recursion instead of index_sequence keeps
    // the fold C++11-clean.
    template<std::size_t _Index,
             std::size_t _Count>
    struct render_tuple_helper
    {
        template<typename _Tuple,
                 typename _Source,
                 typename _Sink>
        static D_CONSTEXPR14 void
        apply(
            const _Tuple&  _fragments,
            const _Source& _source,
            _Sink&         _sink
        )
        {
            render_fragment(std::get<_Index>(_fragments), _source, _sink);
            render_tuple_helper<_Index + 1, _Count>::apply(
                _fragments, _source, _sink);

            return;
        }
    };

    // render_tuple_helper<_Count, _Count>
    //   helper: terminal case - all fragments rendered.
    template<std::size_t _Count>
    struct render_tuple_helper<_Count, _Count>
    {
        template<typename _Tuple,
                 typename _Source,
                 typename _Sink>
        static D_CONSTEXPR14 void
        apply(
            const _Tuple&  _fragments,
            const _Source& _source,
            _Sink&         _sink
        )
        {
            (void)_fragments;
            (void)_source;
            (void)_sink;

            return;
        }
    };

    // mapped_sink_helper
    //   helper: sink decorator that applies a transform to every
    // value before forwarding it on.  operator() is constrained on
    // the FULL downstream expression - transform accepts the value
    // AND the inner sink accepts the transformed result - so
    // is_sink_for stays honest through the decoration.  Non-owning:
    // it lives only for the duration of one render call,
    // referencing the transform owned by mapped_helper and the
    // caller's sink.
    template<typename _Fn,
             typename _Sink>
    class mapped_sink_helper
    {
    public:
        D_CONSTEXPR mapped_sink_helper(
            const _Fn& _fn,
            _Sink&     _sink
        )
            : m_fn(&_fn),
              m_sink(&_sink)
        {}

        template<typename _Value,
                 typename = decltype(
                     std::declval<_Sink&>()(
                         std::declval<const _Fn&>()(
                             std::declval<_Value>()))
                 )>
        D_CONSTEXPR14 void
        operator()(
            _Value&& _value
        ) const
        {
            (*m_sink)((*m_fn)(std::forward<_Value>(_value)));

            return;
        }

    private:
        const _Fn* m_fn;
        _Sink*     m_sink;
    };

    // counting_sink_helper
    //   helper: sink decorator that counts emissions while
    // forwarding them unchanged.  Lets defaulted_helper detect
    // "the inner fragment was silent" without knowing anything
    // about the inner fragment's kind.  operator() is constrained
    // on the inner sink accepting the value, keeping is_sink_for
    // honest through the decoration.
    template<typename _Sink>
    class counting_sink_helper
    {
    public:
        D_CONSTEXPR explicit counting_sink_helper(
            _Sink& _sink
        )
            : m_sink(&_sink),
              m_count(0)
        {}

        template<typename _Value,
                 typename = decltype(
                     std::declval<_Sink&>()(std::declval<_Value>())
                 )>
        D_CONSTEXPR14 void
        operator()(
            _Value&& _value
        )
        {
            ++m_count;
            (*m_sink)(std::forward<_Value>(_value));

            return;
        }

        D_NODISCARD D_CONSTEXPR std::size_t
        count() const
        {
            return m_count;
        }

    private:
        _Sink*      m_sink;
        std::size_t m_count;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             III.  FRAGMENT HELPERS                                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // literal_helper
    //   helper: forces pass-through for a stored value, even when
    // the value's type would otherwise be source-resolvable.  The
    // explicit dual of slot_helper.
    template<typename _Value>
    class literal_helper
    {
    public:
        template<typename _VFwd>
        explicit D_CONSTEXPR literal_helper(
            _VFwd&& _value
        )
            : m_value(std::forward<_VFwd>(_value))
        {}

        template<typename _Source,
                 typename _Sink>
        D_CONSTEXPR14 void
        render(
            const _Source& _source,
            _Sink&         _sink
        ) const
        {
            (void)_source;
            _sink(m_value);

            return;
        }

        D_NODISCARD D_CONSTEXPR const _Value&
        value() const
        {
            return m_value;
        }

    private:
        _Value m_value;
    };


    // slot_helper
    //   helper: the explicit resolution point - holds a key,
    // resolves it against the source at render time, and emits the
    // result (optional-like results are unwrapped; "nothing" emits
    // nothing).  render() is SFINAE-constrained on the source
    // actually resolving the key, so an unresolvable slot is
    // reported by the dispatch diagnostic rather than deep inside
    // the call.
    template<typename _Key>
    class slot_helper
    {
    public:
        template<typename _KFwd>
        explicit D_CONSTEXPR slot_helper(
            _KFwd&& _key
        )
            : m_key(std::forward<_KFwd>(_key))
        {}

        template<typename _Source,
                 typename _Sink,
                 typename = decltype(
                     std::declval<const _Source&>()(
                         std::declval<const _Key&>())
                 )>
        D_CONSTEXPR14 void
        render(
            const _Source& _source,
            _Sink&         _sink
        ) const
        {
            emit_resolved(_source(m_key), _sink);

            return;
        }

        D_NODISCARD D_CONSTEXPR const _Key&
        key() const
        {
            return m_key;
        }

    private:
        _Key m_key;
    };


    // mapped_helper
    //   helper: wraps any fragment and applies a transform to
    // every value it emits.  Works uniformly on slots, literals,
    // whole templates, and user fragments, because it decorates
    // the SINK rather than inspecting the fragment.
    template<typename _Fragment,
             typename _Fn>
    class mapped_helper
    {
    public:
        template<typename _FragFwd,
                 typename _FnFwd>
        D_CONSTEXPR mapped_helper(
            _FragFwd&& _fragment,
            _FnFwd&&   _fn
        )
            : m_fragment(std::forward<_FragFwd>(_fragment)),
              m_fn(std::forward<_FnFwd>(_fn))
        {}

        template<typename _Source,
                 typename _Sink>
        D_CONSTEXPR14 void
        render(
            const _Source& _source,
            _Sink&         _sink
        ) const
        {
            mapped_sink_helper<_Fn, _Sink> adapted(m_fn, _sink);

            render_fragment(m_fragment, _source, adapted);

            return;
        }

    private:
        _Fragment m_fragment;
        _Fn       m_fn;
    };


    // defaulted_helper
    //   helper: wraps any fragment and, if rendering it emitted
    // nothing (an unresolved slot, a gated-off section), emits the
    // stored fallback value instead.  Detection is structural -
    // a counting sink - so it composes with every fragment kind.
    template<typename _Fragment,
             typename _Value>
    class defaulted_helper
    {
    public:
        template<typename _FragFwd,
                 typename _VFwd>
        D_CONSTEXPR defaulted_helper(
            _FragFwd&& _fragment,
            _VFwd&&    _value
        )
            : m_fragment(std::forward<_FragFwd>(_fragment)),
              m_value(std::forward<_VFwd>(_value))
        {}

        template<typename _Source,
                 typename _Sink>
        D_CONSTEXPR14 void
        render(
            const _Source& _source,
            _Sink&         _sink
        ) const
        {
            counting_sink_helper<_Sink> counted(_sink);

            render_fragment(m_fragment, _source, counted);

            // inner fragment was silent: emit the fallback
            if (counted.count() == 0)
            {
                _sink(m_value);
            }

            return;
        }

    private:
        _Fragment m_fragment;
        _Value    m_value;
    };


    // when_helper
    //   helper: predicate-gated fragment.  The predicate receives
    // the SOURCE (not a value), so conditional sections can key off
    // anything the source can answer; when it yields false the
    // fragment is skipped entirely.
    template<typename _Predicate,
             typename _Fragment>
    class when_helper
    {
    public:
        template<typename _PFwd,
                 typename _FragFwd>
        D_CONSTEXPR when_helper(
            _PFwd&&    _pred,
            _FragFwd&& _fragment
        )
            : m_pred(std::forward<_PFwd>(_pred)),
              m_fragment(std::forward<_FragFwd>(_fragment))
        {}

        template<typename _Source,
                 typename _Sink>
        D_CONSTEXPR14 void
        render(
            const _Source& _source,
            _Sink&         _sink
        ) const
        {
            // render only when the gate is open
            if (static_cast<bool>(m_pred(_source)))
            {
                render_fragment(m_fragment, _source, _sink);
            }

            return;
        }

    private:
        _Predicate m_pred;
        _Fragment  m_fragment;
    };


    // mapped_adapter
    //   helper: pipeline RHS for `frag | mapped(f)`.
    template<typename _Fn>
    class mapped_adapter
    {
    public:
        template<typename _FnFwd>
        explicit D_CONSTEXPR mapped_adapter(
            _FnFwd&& _fn
        )
            : m_fn(std::forward<_FnFwd>(_fn))
        {}

        template<typename _Fragment>
        D_CONSTEXPR mapped_helper<typename std::decay<_Fragment>::type, _Fn>
        apply(
            _Fragment&& _fragment
        ) const
        {
            return mapped_helper<
                typename std::decay<_Fragment>::type, _Fn>(
                    std::forward<_Fragment>(_fragment), m_fn);
        }

    private:
        _Fn m_fn;
    };

    // defaulted_adapter
    //   helper: pipeline RHS for `frag | defaulted(v)`.
    template<typename _Value>
    class defaulted_adapter
    {
    public:
        template<typename _VFwd>
        explicit D_CONSTEXPR defaulted_adapter(
            _VFwd&& _value
        )
            : m_value(std::forward<_VFwd>(_value))
        {}

        template<typename _Fragment>
        D_CONSTEXPR defaulted_helper<typename std::decay<_Fragment>::type, _Value>
        apply(
            _Fragment&& _fragment
        ) const
        {
            return defaulted_helper<
                typename std::decay<_Fragment>::type, _Value>(
                    std::forward<_Fragment>(_fragment), m_value);
        }

    private:
        _Value m_value;
    };

    // when_adapter
    //   helper: pipeline RHS for `frag | when(pred)`.
    template<typename _Predicate>
    class when_adapter
    {
    public:
        template<typename _PFwd>
        explicit D_CONSTEXPR when_adapter(
            _PFwd&& _pred
        )
            : m_pred(std::forward<_PFwd>(_pred))
        {}

        template<typename _Fragment>
        D_CONSTEXPR when_helper<_Predicate, typename std::decay<_Fragment>::type>
        apply(
            _Fragment&& _fragment
        ) const
        {
            return when_helper<
                _Predicate, typename std::decay<_Fragment>::type>(
                    m_pred, std::forward<_Fragment>(_fragment));
        }

    private:
        _Predicate m_pred;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             IV.   SOURCE / SINK HELPERS                                 ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // constant_source_helper
    //   helper: a source that ignores its key and yields the same
    // stored value for everything.  Useful standalone for testing
    // and as the terminal layer of an overlay chain.
    template<typename _Value>
    class constant_source_helper
    {
    public:
        template<typename _VFwd>
        explicit D_CONSTEXPR constant_source_helper(
            _VFwd&& _value
        )
            : m_value(std::forward<_VFwd>(_value))
        {}

        template<typename _Key>
        D_CONSTEXPR const _Value&
        operator()(
            const _Key& _key
        ) const
        {
            (void)_key;

            return m_value;
        }

    private:
        _Value m_value;
    };

    // null_source_helper
    //   helper: a source that resolves nothing - every key yields
    // a null pointer (optional-like "nothing").  The identity layer
    // of an overlay chain; also makes every bare slot defaultable.
    template<typename _Value>
    class null_source_helper
    {
    public:
        template<typename _Key>
        D_CONSTEXPR const _Value*
        operator()(
            const _Key& _key
        ) const
        {
            (void)_key;

            return static_cast<const _Value*>(nullptr);
        }
    };

    // overlay_source_helper
    //   helper: layered resolution - ask the primary source first;
    // when its (optional-like) yield is "nothing", fall back to the
    // secondary.  The yield is decayed so the helper never returns
    // a reference into a dead temporary; optional-like values are
    // cheap (pointers, maybe<T>) so the copy is immaterial.
    template<typename _Primary,
             typename _Fallback>
    class overlay_source_helper
    {
    public:
        template<typename _PFwd,
                 typename _FFwd>
        D_CONSTEXPR overlay_source_helper(
            _PFwd&& _primary,
            _FFwd&& _fallback
        )
            : m_primary(std::forward<_PFwd>(_primary)),
              m_fallback(std::forward<_FFwd>(_fallback))
        {}

        template<typename _Key>
        D_CONSTEXPR14 auto
        operator()(
            const _Key& _key
        ) const
            -> typename std::decay<decltype(
                   std::declval<const _Primary&>()(_key))>::type
        {
            auto resolved = m_primary(_key);

            // primary answered: use it
            if (static_cast<bool>(resolved))
            {
                return resolved;
            }

            return m_fallback(_key);
        }

    private:
        _Primary  m_primary;
        _Fallback m_fallback;
    };

    // lookup_source_helper
    //   helper: adapts an associative container (anything with
    // `find(key)` / `end()` whose iterator exposes `->second`) into
    // a source.  Yields a pointer to the mapped value - the
    // optional-like protocol - so missing keys emit nothing.
    // NON-OWNING: the container must outlive the helper.
    template<typename _Map>
    class lookup_source_helper
    {
    public:
        explicit D_CONSTEXPR lookup_source_helper(
            const _Map& _map
        )
            : m_map(&_map)
        {}

        template<typename _Key>
        D_CONSTEXPR14 auto
        operator()(
            const _Key& _key
        ) const
            -> decltype(&std::declval<const _Map&>().find(_key)->second)
        {
            auto it = m_map->find(_key);

            // missing key resolves to nothing
            if (it == m_map->end())
            {
                return nullptr;
            }

            return &it->second;
        }

    private:
        const _Map* m_map;
    };

    // into_helper
    //   helper: adapts a container with `push_back` into a sink.
    // operator() is SFINAE-constrained on the push_back actually
    // accepting the value, so is_sink_for answers honestly through
    // this helper (an unconstrained template would claim to absorb
    // everything and mis-steer the literal dispatch).
    // NON-OWNING: the container must outlive the helper (it is
    // intended to be constructed at the render call site).
    template<typename _Container>
    class into_helper
    {
    public:
        explicit D_CONSTEXPR into_helper(
            _Container& _container
        )
            : m_container(&_container)
        {}

        template<typename _Value,
                 typename = decltype(
                     std::declval<_Container&>().push_back(
                         std::declval<_Value>())
                 )>
        D_CONSTEXPR14 void
        operator()(
            _Value&& _value
        ) const
        {
            m_container->push_back(std::forward<_Value>(_value));

            return;
        }

    private:
        _Container* m_container;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             V.    BASIC TEMPLATE                                        ///
///////////////////////////////////////////////////////////////////////////////

// basic_template
//   class: a heterogeneous, immutable sequence of fragments - the
// foundational template type.  It is itself just a renderable
// fragment (render folds the tuple left-to-right through the
// structural dispatch), so templates nest inside templates and
// inside operator wrappers with no special casing.  The fragment
// tuple is fixed during translation; construction is D_CONSTEXPR
// and rendering is D_CONSTEXPR14, so the whole structure
// participates in constant evaluation when its source and sink do.
template<typename... _Fragments>
class basic_template
{
public:
    using fragment_tuple_type = std::tuple<_Fragments...>;

    explicit D_CONSTEXPR basic_template(
        fragment_tuple_type _fragments
    )
        : m_fragments(std::move(_fragments))
    {}

    // render
    //   function: folds every fragment, in order, against the
    // source and into the sink.
    template<typename _Source,
             typename _Sink>
    D_CONSTEXPR14 void
    render(
        const _Source& _source,
        _Sink&         _sink
    ) const
    {
        internal::render_tuple_helper<0, sizeof...(_Fragments)>::apply(
            m_fragments, _source, _sink);

        return;
    }

    // then
    //   function: returns a NEW template with one fragment
    // appended; the original is untouched (templates are values).
    template<typename _Fragment>
    D_NODISCARD D_CONSTEXPR14
    basic_template<_Fragments..., typename std::decay<_Fragment>::type>
    then(
        _Fragment&& _fragment
    ) const
    {
        return basic_template<
            _Fragments..., typename std::decay<_Fragment>::type>(
                std::tuple_cat(
                    m_fragments,
                    std::make_tuple(std::forward<_Fragment>(_fragment))));
    }

    // size
    //   function: the number of fragments in the template.
    D_NODISCARD static D_CONSTEXPR std::size_t
    size()
    {
        return sizeof...(_Fragments);
    }

    // fragments
    //   function: read-only access to the fragment tuple (used by
    // join and by structural transformations in derived modules).
    D_NODISCARD D_CONSTEXPR const fragment_tuple_type&
    fragments() const
    {
        return m_fragments;
    }

private:
    fragment_tuple_type m_fragments;
};


///////////////////////////////////////////////////////////////////////////////
///             VI.   FACTORIES                                             ///
///////////////////////////////////////////////////////////////////////////////

namespace templates
{

    // literal
    //   function: returns a fragment that always passes the given
    // value straight through to the sink - even when the value's
    // type would otherwise be source-resolvable.  Plain values that
    // the sink can consume are literals automatically; this wrapper
    // exists to force the kind when types are ambiguous.
    template<typename _Value>
    D_NODISCARD D_CONSTEXPR internal::literal_helper<typename std::decay<_Value>::type>
    literal(
        _Value&& _value
    )
    {
        return internal::literal_helper<typename std::decay<_Value>::type>(
            std::forward<_Value>(_value));
    }


    // slot
    //   function: returns the explicit resolution point - a
    // fragment that, at render time, asks the source for this key
    // and emits the answer.  Optional-like answers are unwrapped;
    // "nothing" emits nothing (compose with defaulted() for a
    // fallback).
    template<typename _Key>
    D_NODISCARD D_CONSTEXPR internal::slot_helper<typename std::decay<_Key>::type>
    slot(
        _Key&& _key
    )
    {
        return internal::slot_helper<typename std::decay<_Key>::type>(
            std::forward<_Key>(_key));
    }

    // slot (transformed form)
    //   function: convenience for `slot(key) | mapped(fn)` - a slot
    // whose resolved value is passed through a transform before
    // emission.
    template<typename _Key,
             typename _Fn>
    D_NODISCARD D_CONSTEXPR internal::mapped_helper<
        internal::slot_helper<typename std::decay<_Key>::type>,
        typename std::decay<_Fn>::type>
    slot(
        _Key&& _key,
        _Fn&&  _fn
    )
    {
        return internal::mapped_helper<
            internal::slot_helper<typename std::decay<_Key>::type>,
            typename std::decay<_Fn>::type>(
                internal::slot_helper<typename std::decay<_Key>::type>(
                    std::forward<_Key>(_key)),
                std::forward<_Fn>(_fn));
    }


    // seq
    //   function: builds a basic_template from a fragment sequence.
    // Fragments are decayed and stored by value; the result is
    // itself a fragment, so seq calls nest freely.
    template<typename... _Fragments>
    D_NODISCARD D_CONSTEXPR basic_template<typename std::decay<_Fragments>::type...>
    seq(
        _Fragments&&... _fragments
    )
    {
        return basic_template<typename std::decay<_Fragments>::type...>(
            std::tuple<typename std::decay<_Fragments>::type...>(
                std::forward<_Fragments>(_fragments)...));
    }


    // mapped
    //   function: wraps a fragment so every value it emits passes
    // through the transform first.  f(t) at any granularity: a
    // single slot, a sub-template, or a whole template.
    template<typename _Fragment,
             typename _Fn>
    D_NODISCARD D_CONSTEXPR internal::mapped_helper<
        typename std::decay<_Fragment>::type,
        typename std::decay<_Fn>::type>
    mapped(
        _Fragment&& _fragment,
        _Fn&&       _fn
    )
    {
        return internal::mapped_helper<
            typename std::decay<_Fragment>::type,
            typename std::decay<_Fn>::type>(
                std::forward<_Fragment>(_fragment),
                std::forward<_Fn>(_fn));
    }

    // mapped (adapter form)
    //   function: single-argument form for pipeline use:
    // `frag | mapped(fn)`.
    template<typename _Fn>
    D_NODISCARD D_CONSTEXPR internal::mapped_adapter<typename std::decay<_Fn>::type>
    mapped(
        _Fn&& _fn
    )
    {
        return internal::mapped_adapter<typename std::decay<_Fn>::type>(
            std::forward<_Fn>(_fn));
    }


    // defaulted
    //   function: wraps a fragment so that, if it emits nothing
    // (unresolved slot, closed gate), the fallback value is emitted
    // instead.
    template<typename _Fragment,
             typename _Value>
    D_NODISCARD D_CONSTEXPR internal::defaulted_helper<
        typename std::decay<_Fragment>::type,
        typename std::decay<_Value>::type>
    defaulted(
        _Fragment&& _fragment,
        _Value&&    _value
    )
    {
        return internal::defaulted_helper<
            typename std::decay<_Fragment>::type,
            typename std::decay<_Value>::type>(
                std::forward<_Fragment>(_fragment),
                std::forward<_Value>(_value));
    }

    // defaulted (adapter form)
    //   function: single-argument form for pipeline use:
    // `frag | defaulted(v)`.
    template<typename _Value>
    D_NODISCARD D_CONSTEXPR internal::defaulted_adapter<typename std::decay<_Value>::type>
    defaulted(
        _Value&& _value
    )
    {
        return internal::defaulted_adapter<typename std::decay<_Value>::type>(
            std::forward<_Value>(_value));
    }


    // when
    //   function: predicate-gates a fragment.  The predicate
    // receives the source at render time; a false gate skips the
    // fragment entirely.
    template<typename _Predicate,
             typename _Fragment>
    D_NODISCARD D_CONSTEXPR internal::when_helper<
        typename std::decay<_Predicate>::type,
        typename std::decay<_Fragment>::type>
    when(
        _Predicate&& _pred,
        _Fragment&&  _fragment
    )
    {
        return internal::when_helper<
            typename std::decay<_Predicate>::type,
            typename std::decay<_Fragment>::type>(
                std::forward<_Predicate>(_pred),
                std::forward<_Fragment>(_fragment));
    }

    // when (adapter form)
    //   function: single-argument form for pipeline use:
    // `frag | when(pred)`.
    template<typename _Predicate>
    D_NODISCARD D_CONSTEXPR internal::when_adapter<typename std::decay<_Predicate>::type>
    when(
        _Predicate&& _pred
    )
    {
        return internal::when_adapter<typename std::decay<_Predicate>::type>(
            std::forward<_Predicate>(_pred));
    }


    // constant_source
    //   function: returns a source that yields the same value for
    // every key.  Useful for tests and as the terminal layer of an
    // overlay chain.
    template<typename _Value>
    D_NODISCARD D_CONSTEXPR internal::constant_source_helper<typename std::decay<_Value>::type>
    constant_source(
        _Value&& _value
    )
    {
        return internal::constant_source_helper<typename std::decay<_Value>::type>(
            std::forward<_Value>(_value));
    }


    // null_source
    //   function: returns a source that resolves nothing - every
    // key yields an optional-like "nothing" of the given value
    // type.  The identity layer of an overlay chain.
    template<typename _Value>
    D_NODISCARD D_CONSTEXPR internal::null_source_helper<_Value>
    null_source()
    {
        return internal::null_source_helper<_Value>{};
    }


    // overlay
    //   function: layered resolution - ask the primary first; when
    // its (optional-like) answer is "nothing", fall back.  The
    // fallback's yield must be convertible to the primary's decayed
    // yield type.
    template<typename _Primary,
             typename _Fallback>
    D_NODISCARD D_CONSTEXPR internal::overlay_source_helper<
        typename std::decay<_Primary>::type,
        typename std::decay<_Fallback>::type>
    overlay(
        _Primary&&  _primary,
        _Fallback&& _fallback
    )
    {
        return internal::overlay_source_helper<
            typename std::decay<_Primary>::type,
            typename std::decay<_Fallback>::type>(
                std::forward<_Primary>(_primary),
                std::forward<_Fallback>(_fallback));
    }

    // overlay (variadic form)
    //   function: right-folds three or more sources into a layered
    // chain: overlay(a, b, c) == overlay(a, overlay(b, c)).
    template<typename    _Primary,
             typename    _Fallback,
             typename... _Rest>
    D_NODISCARD D_CONSTEXPR auto
    overlay(
        _Primary&&  _primary,
        _Fallback&& _fallback,
        _Rest&&...  _rest
    )
        -> decltype(overlay(std::forward<_Primary>(_primary),
                            overlay(std::forward<_Fallback>(_fallback),
                                    std::forward<_Rest>(_rest)...)))
    {
        return overlay(std::forward<_Primary>(_primary),
                       overlay(std::forward<_Fallback>(_fallback),
                               std::forward<_Rest>(_rest)...));
    }


    // from_lookup
    //   function: adapts an associative container (find/end with
    // `->second` iterators) into a source.  Missing keys resolve to
    // nothing.  NON-OWNING: the container must outlive the source.
    template<typename _Map>
    D_NODISCARD D_CONSTEXPR internal::lookup_source_helper<_Map>
    from_lookup(
        const _Map& _map
    )
    {
        return internal::lookup_source_helper<_Map>(_map);
    }


    // into
    //   function: adapts a push_back container into a sink.
    // NON-OWNING: the container must outlive the sink (construct it
    // at the render call site).
    template<typename _Container>
    D_NODISCARD D_CONSTEXPR internal::into_helper<_Container>
    into(
        _Container& _container
    )
    {
        return internal::into_helper<_Container>(_container);
    }

}  // namespace templates


///////////////////////////////////////////////////////////////////////////////
///             VII.  PIPELINE OPERATORS                                    ///
///////////////////////////////////////////////////////////////////////////////
// The operator| overloads below are tightly constrained on the right-hand
//   side to this module's three named adapter types, so they cannot conflict
//   with the pipeline operators in view.hpp, comparator.hpp, monad.hpp,
//   extractor.hpp, or any other module: each module's adapter types are
//   distinct.

// operator| (fragment | mapped_adapter)
//   function: pipeline composition. Yields a mapped_helper
// equivalent to templates::mapped(fragment, fn).
template<typename _Fragment,
         typename _Fn>
D_NODISCARD D_CONSTEXPR auto
operator|(
    _Fragment&&                   _fragment,
    internal::mapped_adapter<_Fn> _adapter
)
    -> decltype(_adapter.apply(std::forward<_Fragment>(_fragment)))
{
    return _adapter.apply(std::forward<_Fragment>(_fragment));
}


// operator| (fragment | defaulted_adapter)
//   function: pipeline composition. Yields a defaulted_helper
// equivalent to templates::defaulted(fragment, value).
template<typename _Fragment,
         typename _Value>
D_NODISCARD D_CONSTEXPR auto
operator|(
    _Fragment&&                         _fragment,
    internal::defaulted_adapter<_Value> _adapter
)
    -> decltype(_adapter.apply(std::forward<_Fragment>(_fragment)))
{
    return _adapter.apply(std::forward<_Fragment>(_fragment));
}


// operator| (fragment | when_adapter)
//   function: pipeline composition. Yields a when_helper
// equivalent to templates::when(pred, fragment).
template<typename _Fragment,
         typename _Predicate>
D_NODISCARD D_CONSTEXPR auto
operator|(
    _Fragment&&                        _fragment,
    internal::when_adapter<_Predicate> _adapter
)
    -> decltype(_adapter.apply(std::forward<_Fragment>(_fragment)))
{
    return _adapter.apply(std::forward<_Fragment>(_fragment));
}


///////////////////////////////////////////////////////////////////////////////
///             VIII. RENDER DRIVERS                                        ///
///////////////////////////////////////////////////////////////////////////////

// render
//   function: the universal driver - renders ANY fragment (a whole
// template, a single slot, a bare literal) against a source, into a
// sink.  Dispatch is purely structural; see the kind priority in the
// header comment.
template<typename _Fragment,
         typename _Source,
         typename _Sink>
D_CONSTEXPR14 void
render(
    const _Fragment& _fragment,
    const _Source&   _source,
    _Sink&&          _sink
)
{
    internal::render_fragment(_fragment, _source, _sink);

    return;
}


// render_to
//   function: convenience driver - renders into a freshly
// constructed push_back container and returns it.  Runtime-only on
// pre-C++20 standard libraries (allocating containers are not
// constexpr there); under C++20 constexpr containers it folds at
// compile time without source changes.
template<typename _Container,
         typename _Fragment,
         typename _Source>
D_NODISCARD D_CONSTEXPR14 _Container
render_to(
    const _Fragment& _fragment,
    const _Source&   _source
)
{
    _Container result;

    internal::into_helper<_Container> sink(result);

    internal::render_fragment(_fragment, _source, sink);

    return result;
}


// join
//   function: concatenates two basic_templates into one.  The
// fragment tuples are spliced during translation; rendering the
// joined template is identical to rendering the operands in order.
template<typename... _Left,
         typename... _Right>
D_NODISCARD D_CONSTEXPR14 basic_template<_Left..., _Right...>
join(
    const basic_template<_Left...>&  _left,
    const basic_template<_Right...>& _right
)
{
    return basic_template<_Left..., _Right...>(
        std::tuple_cat(_left.fragments(), _right.fragments()));
}


///////////////////////////////////////////////////////////////////////////////
///             IX.   MODULE TRAITS & CONCEPTS                              ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // is_basic_template_helper
    //   trait: primary template (failure case).
    template<typename _Type>
    struct is_basic_template_helper : std::false_type
    {};

    // is_basic_template_helper (success case)
    //   trait: succeeds for any basic_template specialization.
    template<typename... _Fragments>
    struct is_basic_template_helper<basic_template<_Fragments...>>
        : std::true_type
    {};

NS_END  // internal

// is_basic_template
//   trait: detects whether _Type is a basic_template
// specialization.  Note that fragment-hood and template-hood are
// otherwise structural (is_renderable); this trait exists for the
// few places (join, structural rewrites in derived modules) that
// need the concrete tuple-backed type.
template<typename _Type>
struct is_basic_template
    : internal::is_basic_template_helper<clean_t<_Type>>
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_optional_like_v
    //   value: convenience alias for is_optional_like<...>::value.
    template<typename _Type>
    constexpr bool is_optional_like_v = is_optional_like<_Type>::value;

    // is_sink_for_v
    //   value: convenience alias for is_sink_for<...>::value.
    template<typename _Sink,
             typename _Value>
    constexpr bool is_sink_for_v = is_sink_for<_Sink, _Value>::value;

    // is_source_for_v
    //   value: convenience alias for is_source_for<...>::value.
    template<typename _Source,
             typename _Key>
    constexpr bool is_source_for_v = is_source_for<_Source, _Key>::value;

    // is_renderable_v
    //   value: convenience alias for is_renderable<...>::value.
    template<typename _Fragment,
             typename _Source,
             typename _Sink>
    constexpr bool is_renderable_v =
        is_renderable<_Fragment, _Source, _Sink>::value;

    // is_fragment_v
    //   value: convenience alias for is_fragment<...>::value.
    template<typename _Fragment,
             typename _Source,
             typename _Sink>
    constexpr bool is_fragment_v =
        is_fragment<_Fragment, _Source, _Sink>::value;

    // is_basic_template_v
    //   value: convenience alias for is_basic_template<...>::value.
    template<typename _Type>
    constexpr bool is_basic_template_v = is_basic_template<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES



//  protocol concepts  (PascalCase)

//   The structural protocol faces, PascalCase per the project's concept
// naming convention (cf. structural_traits.hpp's Reducer / Transducer).
// Each is defined over the traits above, so the module degrades cleanly
// to C++11: the traits remain, only the concept faces are guarded away.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // OptionalLike
    //   concept: a type modeling the optional protocol -
    // bool-testable and dereferenceable.
    template<typename _Type>
    concept OptionalLike = is_optional_like<_Type>::value;

    // SinkFor
    //   concept: a type that can absorb emitted values of _Value.
    template<typename _Sink,
             typename _Value>
    concept SinkFor = is_sink_for<_Sink, _Value>::value;

    // SourceFor
    //   concept: a type that can resolve keys of _Key (yielding a
    // value or an optional-like "maybe a value").
    template<typename _Source,
             typename _Key>
    concept SourceFor = is_source_for<_Source, _Key>::value;

    // Renderable
    //   concept: a self-rendering fragment - exposes
    // `render(const Source&, Sink&)`.  basic_template and every
    // operator wrapper satisfy this; so does any user type with the
    // member, no inheritance required.
    template<typename _Fragment,
             typename _Source,
             typename _Sink>
    concept Renderable = is_renderable<_Fragment, _Source, _Sink>::value;

    // Fragment
    //   concept: anything renderable, sink-consumable, or
    // source-resolvable - i.e. anything the dispatch can place.
    template<typename _Fragment,
             typename _Source,
             typename _Sink>
    concept Fragment = is_fragment<_Fragment, _Source, _Sink>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_PARADIGM_TEMPLATE_