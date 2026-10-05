/*******************************************************************************
* djinterp [core]                                                     writer.hpp
*
*   Generic, type-agnostic IO foundation: output sinks plus the `printer`
* and `writer` functors. Nothing here knows about documents, XML, HTML,
* or any concrete value type -- the functors are parameterised on a
* transform and a destination, so the same machinery serves text, binary,
* or structured output. The document- and file-specific layers
* (document_printer / node_printer, file_writer, document_writer + cursors)
* build ON this header; this header depends on nothing but the standard
* library.
*
*   THE TWO FUNCTORS:
*
*   printer<Fn> -- a PURE transformation `In -> Out`. It takes its input
*     by const reference and returns a freshly produced output; it never
*     mutates the input. Printers compose: `p.then(g)` yields a printer
*     that applies `p` and then `g`. This is the reusable, configurable
*     "render this value to that representation" object -- the generalised
*     form of text_template's `interpolate`.
*
*   writer<Sink, Printer> -- an EFFECTFUL builder. It owns a destination
*     (a sink) and a printer, and `write(x)` renders `x` through the printer
*     and appends the result to the sink, leaving `x` unchanged. A writer is
*     therefore "a printer plus somewhere to put the result"; the two ideas
*     are one mechanism, not two.
*
*   SINKS:
*   A sink is any callable `(const char*, std::size_t)`. `string_sink`
*   appends into a std::string; `stream_sink` writes into a std::ostream;
*   a user lambda or a functional-style consumer works directly. File,
*   console, string, and buffer destinations are all just different sinks.
*
*   BINARY:
*   "Type-agnostic" includes binary: a sink moves raw bytes, and a
*   std::string is used purely as a byte buffer (it may hold embedded NULs
*   and arbitrary bytes). A binary printer returns a byte-filled string (or
*   any contiguous `.data()`/`.size()` range) and the writer appends it
*   verbatim.
*
*   Requires C++14 (return-type deduction); self-suppresses below it.
*
*
* path:      /inc/djinterp/core/text/writer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.18
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    OUTPUT SINKS
      ------------
      a. string_sink
      b. stream_sink

II.   PRINTER FUNCTOR
      ---------------
      a. identity_fn / compose_fn (internal)
      b. printer<Fn>
      c.    make_printer, id_printer
            d. print

III.  WRITER FUNCTOR
      --------------
      a. writer<Sink, Printer>
      b. make_writer
      c.    string_writer
            d. stream_writer
*/

#ifndef DJINTERP_TEXT_WRITER_HPP
#define DJINTERP_TEXT_WRITER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"


// Return-type deduction (`auto` returns) underlies the factories and
// composition; below C++14 this module contributes nothing.
#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                  I.   OUTPUT SINKS                                      ///
///////////////////////////////////////////////////////////////////////////////

// string_sink
//   struct: a sink that appends written bytes to a std::string. Holds the
// target by pointer; the target must outlive the sink. Satisfies the sink
// contract -- `operator()(const char*, std::size_t)`.
struct string_sink
{
    std::string* out;

    explicit string_sink(
        std::string& _out
    )
        : out(&_out)
    {}

    void
    operator()(
        const char* _data,
        std::size_t _size
    )
    {
        out->append(_data, _size);

        return;
    }
};


// stream_sink
//   struct: a sink that writes bytes to a std::ostream (std::cout, an
// ofstream, an ostringstream). Holds the stream by pointer; the stream
// must outlive the sink.
struct stream_sink
{
    std::ostream* out;

    explicit stream_sink(
        std::ostream& _out
    )
        : out(&_out)
    {}

    void
    operator()(
        const char* _data,
        std::size_t _size
    )
    {
        out->write(_data, static_cast<std::streamsize>(_size));

        return;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                  II.   PRINTER FUNCTOR                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // identity_fn
    //   function object: returns its input unchanged. Use as a printer's
    // transform when inputs are already in the destination's byte form
    // (a std::string, string_view, or other `.data()`/`.size()` range);
    // it is zero-copy but does NOT accept bare string literals.
    struct identity_fn
    {
        template<typename In>
        const In&
        operator()(
            const In& _in
        ) const
        {
            return _in;
        }
    };


    // to_string_fn
    //   function object: coerces an input to an owned std::string -- the
    // writers' default transform, so string literals, `const char*`,
    // std::string, and (C++17) std::string_view all write without the
    // caller supplying a printer. Non-string inputs do not compile here,
    // which correctly forces an explicit printer for them.
    struct to_string_fn
    {
        template<typename In>
        std::string
        operator()(
            const In& _in
        ) const
        {
            return std::string(_in);
        }
    };


    // compose_fn
    //   function object: the composition g . f -- applies `f` then `g`.
    // Produced by `printer::then`; kept as a named type (not a lambda) so a
    // printer's function member is a stable, nameable type.
    template<typename F,
             typename G>
    struct compose_fn
    {
        F f;
        G g;

        template<typename In>
        auto
        operator()(
            const In& _in
        ) const
            -> decltype(g(f(_in)))
        {
            return g(f(_in));
        }
    };

NS_END  // internal


// printer
//   class: a pure transformation `In -> Out`, wrapped as a composable
// functor. Applying a printer never mutates its input (the input is taken
// by const reference). `Fn` is the underlying transform.
template<typename Fn>
class printer
{
public:
    using function_type = Fn;

    // printer ()
    //   constructor: default -- usable when `Fn` is default-constructible
    // (the identity / to-string transforms used as writer defaults). For a
    // lambda-based `Fn` this is simply not available, which is harmless.
    printer() = default;

    // printer (fn)
    //   constructor: bind the underlying transform.
    explicit printer(
        Fn _fn
    )
        : m_fn(static_cast<Fn&&>(_fn))
    {}

    // operator()
    //   function: apply the transform, producing output from input without
    // modifying the input.
    template<typename In>
    D_NODISCARD auto
    operator()(
        const In& _in
    ) const
        -> decltype(std::declval<const Fn&>()(_in))
    {
        return m_fn(_in);
    }

    // then
    //   function: compose -- returns a printer that applies THIS transform
    // and then `_next` (any callable, including another printer).
    template<typename Next>
    D_NODISCARD auto
    then(
        Next _next
    ) const
    {
        return printer<internal::compose_fn<Fn, Next>>(
            internal::compose_fn<Fn, Next>{
                m_fn, static_cast<Next&&>(_next) });
    }

    // function
    //   function: the underlying transform.
    D_NODISCARD const Fn&
    function() const
    {
        return m_fn;
    }

private:
    Fn m_fn;
};


// make_printer
//   function: build a printer from any callable, deducing and decaying the
// stored transform type.
template<typename Fn>
D_NODISCARD printer<typename std::decay<Fn>::type>
make_printer(
    Fn&& _fn
)
{
    return printer<typename std::decay<Fn>::type>(
        static_cast<Fn&&>(_fn));
}


// id_printer
//   function: the identity printer -- returns its input unchanged.
D_NODISCARD inline printer<internal::identity_fn>
id_printer()
{
    return printer<internal::identity_fn>(internal::identity_fn{});
}


// print
//   function: apply `_printer` to `_value`, returning the produced output.
// `_value` is not modified. A convenience for the one-shot case; the
// fluent `print(value).to(...)` builder is a later layer.
template<typename In,
         typename Printer>
D_NODISCARD auto
print(
    const In&      _value,
    const Printer& _printer
)
    -> decltype(_printer(_value))
{
    return _printer(_value);
}


///////////////////////////////////////////////////////////////////////////////
///                  III.   WRITER FUNCTOR                                  ///
///////////////////////////////////////////////////////////////////////////////

// writer
//   class: an effectful builder over a sink. `write(x)` renders `x` through
// the bound printer and appends the result to the sink, leaving `x`
// unchanged. `Sink` is any callable `(const char*, std::size_t)`;
// `Printer` renders an input into a contiguous `.data()`/`.size()` range
// (a std::string by default, via the to-string printer -- so literals,
// `const char*`, and std::string all write without a custom printer).
// Returns `*this` so writes chain.
template<typename Sink,
         typename Printer = printer<internal::to_string_fn>>
class writer
{
public:
    using sink_type    = Sink;
    using printer_type = Printer;

    // writer (sink)
    //   constructor: bind a sink; default to-string printer (text inputs).
    explicit writer(
        Sink _sink
    )
        : m_sink(static_cast<Sink&&>(_sink))
        , m_printer()
    {}

    // writer (sink, printer)
    //   constructor: bind a sink and a value-rendering printer.
    writer(
        Sink     _sink,
        Printer _printer
    )
        : m_sink(static_cast<Sink&&>(_sink))
        , m_printer(static_cast<Printer&&>(_printer))
    {}

    // write
    //   function: render `_value` and append it to the sink; `_value` is
    // not modified.
    template<typename In>
    writer&
    write(
        const In& _value
    )
    {
        const auto& _fragment = m_printer(_value);
        m_sink(_fragment.data(), _fragment.size());

        return *this;
    }

    // operator()
    //   function: the call face of a writer; forwards to write.
    template<typename In>
    writer&
    operator()(
        const In& _value
    )
    {
        return write(_value);
    }

    // sink / printer accessors
    D_NODISCARD const Sink&
    sink() const
    {
        return m_sink;
    }

    D_NODISCARD const Printer&
    printer_of() const
    {
        return m_printer;
    }

private:
    Sink     m_sink;
    Printer m_printer;
};


// make_writer
//   function: build a writer from a sink (and optional printer), decaying
// the stored sink type.
template<typename Sink>
D_NODISCARD writer<typename std::decay<Sink>::type>
make_writer(
    Sink&& _sink
)
{
    return writer<typename std::decay<Sink>::type>(
        static_cast<Sink&&>(_sink));
}

template<typename Sink,
         typename Printer>
D_NODISCARD writer<typename std::decay<Sink>::type, Printer>
make_writer(
    Sink&&  _sink,
    Printer _printer
)
{
    return writer<typename std::decay<Sink>::type, Printer>(
        static_cast<Sink&&>(_sink), static_cast<Printer&&>(_printer));
}


// string_writer
//   class: a writer that OWNS its output buffer (a std::string) and exposes
// it. Convenient when the destination is an in-memory string rather than an
// external sink. Renders inputs through `Printer` (to-string by default).
template<typename Printer = printer<internal::to_string_fn>>
class string_writer
{
public:
    using printer_type = Printer;

    // string_writer ()
    //   constructor: empty buffer; default to-string printer.
    string_writer()
        : m_buffer()
        , m_printer()
    {}

    // string_writer (printer)
    //   constructor: empty buffer; explicit value-rendering printer.
    explicit string_writer(
        Printer _printer
    )
        : m_buffer()
        , m_printer(static_cast<Printer&&>(_printer))
    {}

    // write
    //   function: render `_value` and append it to the owned buffer.
    template<typename In>
    string_writer&
    write(
        const In& _value
    )
    {
        const auto& _fragment = m_printer(_value);
        m_buffer.append(_fragment.data(), _fragment.size());

        return *this;
    }

    template<typename In>
    string_writer&
    operator()(
        const In& _value
    )
    {
        return write(_value);
    }

    // str -- the accumulated buffer
    D_NODISCARD const std::string&
    str() const
    {
        return m_buffer;
    }

    // take -- move the accumulated buffer out
    D_NODISCARD std::string
    take()
    {
        return static_cast<std::string&&>(m_buffer);
    }

private:
    std::string m_buffer;
    Printer     m_printer;
};


// stream_writer
//   function: a writer bound to a std::ostream destination. The returned
// writer holds a `stream_sink`; the stream must outlive it.
template<typename Printer = printer<internal::to_string_fn>>
D_NODISCARD writer<stream_sink, Printer>
stream_writer(
    std::ostream& _stream
)
{
    return writer<stream_sink, Printer>(stream_sink(_stream));
}

template<typename Printer>
D_NODISCARD writer<stream_sink, Printer>
stream_writer(
    std::ostream& _stream,
    Printer       _printer
)
{
    return writer<stream_sink, Printer>(
        stream_sink(_stream), static_cast<Printer&&>(_printer));
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_TEXT_WRITER_HPP
