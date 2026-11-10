/*******************************************************************************
* djinterp [cpp]                                                      writer.hpp
*
*   Generic, type-agnostic IO foundation: output sinks plus the `printer`
* and `writer` functors. Nothing here knows about documents, XML, HTML,
* or any concrete value type -- the functors are parameterised on a
* transform and a destination, so the same machinery serves text, binary,
* or structured output. The document- and file-specific layers
* (DDocumentPrinter / node_printer, file_writer, DDocumentWriter + cursors)
* build ON this header; this header depends on nothing but the standard
* library.
*
*   THE TWO FUNCTORS:
*
*   DPrinter<Fn> -- a PURE transformation `In -> Out`. It takes its input
*     by const reference and returns a freshly produced output; it never
*     mutates the input. Printers compose: `p.then(g)` yields a printer
*     that applies `p` and then `g`. This is the reusable, configurable
*     "render this value to that representation" object -- the generalised
*     form of DTextTemplate's `interpolate`.
*
*   DWriter<Sink, Printer> -- an EFFECTFUL builder. It owns a destination
*     (a sink) and a printer, and `write(x)` renders `x` through the printer
*     and appends the result to the sink, leaving `x` unchanged. A writer is
*     therefore "a printer plus somewhere to put the result"; the two ideas
*     are one mechanism, not two.
*
*   SINKS:
*   A sink is any callable `(const char*, std::size_t)`. `DStringSink`
*   appends into a std::string; `DStreamSink` writes into a std::ostream;
*   `output_stream_sink` writes into any output stream (cpp/util/output): the
*   console, a file, a dj_string, a caller's array, several at once; a user
*   lambda or a functional-style consumer works directly. File, console,
*   string, and buffer destinations are all just different sinks.
*
*   A SINK RETURNS NOTHING, SO ITS STREAM KEEPS THE FAILURE. The three sinks
*   here write through an output stream, whose status is sticky: a failed
*   write -- std::bad_alloc from a string, a std::ostream gone bad, a full
*   disk -- ends the stream for good instead of throwing through the writer
*   or vanishing. Ask the sink, or the writer, ok() once after writing. A
*   sink of the caller's own that has an ok() is asked the same way.
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
* path:      /inc/djinterp/cpp/text/writer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.18
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OUTPUT SINKS
    ------------
    1.  Sinks over a stream
         1.  DStringSink
         2.  DStreamSink
         3.  output_stream_sink
2.  PRINTER FUNCTOR
    ---------------
    1.  Transforms (internal)
         1.  DIdentityFn
         2.  DToStringFn
         3.  DComposeFn
    2.  The printer
         1.  DPrinter<Fn>
         2.  make_printer
         3.  id_printer
         4.  print
3.  WRITER FUNCTOR
    --------------
    1.  The writer
         1.  sink_has_ok (internal)
         2.  DWriter<Sink, Printer>
         3.  make_writer
    2.  Writers with a destination
         1.  DStringWriter
         2.  stream_writer
         3.  output_writer
*/

#ifndef DJINTERP_TEXT_WRITER_HPP
#define DJINTERP_TEXT_WRITER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>      // std::size_t
#include <ostream>      // std::ostream
#include <string>       // std::string
#include <type_traits>  // std::decay, std::true_type, std::false_type
#include <utility>      // std::declval
// djinterp
#include "../djinterp.hpp"                       // framework root
#include "../util/output/output_stream.hpp"      // DOutputStream,
                                                 // DOutputStreamSink
#include "../util/output/output_stream_std.hpp"  // attach_std_string,
                                                 // attach_std_ostream


// Return-type deduction (`auto` returns) underlies the factories and
// composition; below C++14 this module contributes nothing.
#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_DJINTERP


//==============================================================================
// 1.  OUTPUT SINKS
//==============================================================================
// Each owns or borrows an output stream and writes through it, so a failed
// write is the stream's sticky status, read with ok().


// 1.1    Sinks over a stream
//------------------------------------------------------------------------------
// 1.1.1
// DStringSink
//   class: a sink that appends written bytes to a std::string, through an
// output stream it owns, so a failed append (std::bad_alloc) ends the stream
// rather than throwing through the writer. The target must outlive the sink.
// Movable, not copyable, as the stream is.
class DStringSink
{
public:
    explicit DStringSink(
        std::string& _out
    ) D_NOEXCEPT
        : m_stream()
    {
        (void)attach_std_string(m_stream, _out);
    }

    DStringSink(DStringSink&&)            = default;
    DStringSink& operator=(DStringSink&&) = default;

    // the sink face: append the bytes
    void operator()(
        const char* _data,
        std::size_t _size
    ) D_NOEXCEPT
    {
        (void)m_stream.write(_data, _size);

        return;
    }

    // ok: whether every append so far succeeded
    D_NODISCARD bool ok() const D_NOEXCEPT
    {
        return m_stream.is_valid();
    }

    // stream: the stream the sink writes through, for its status
    D_NODISCARD const DOutputStream& stream() const D_NOEXCEPT
    {
        return m_stream;
    }

private:
    DOutputStream m_stream;
};

// 1.1.2
// DStreamSink
//   class: a sink that writes bytes to a std::ostream (std::cout, an ofstream,
// an ostringstream) through an output stream it owns, so a std::ostream gone
// bad, or one that throws, ends the stream and shows in ok(). The std::ostream
// must outlive the sink. Movable, not copyable.
class DStreamSink
{
public:
    explicit DStreamSink(
        std::ostream& _out
    ) D_NOEXCEPT
        : m_stream()
    {
        (void)attach_std_ostream(m_stream, _out);
    }

    DStreamSink(DStreamSink&&)            = default;
    DStreamSink& operator=(DStreamSink&&) = default;

    // the sink face: write the bytes
    void operator()(
        const char* _data,
        std::size_t _size
    ) D_NOEXCEPT
    {
        (void)m_stream.write(_data, _size);

        return;
    }

    // ok: whether every write so far reached the std::ostream
    D_NODISCARD bool ok() const D_NOEXCEPT
    {
        return m_stream.is_valid();
    }

    // stream: the stream the sink writes through, for its status
    D_NODISCARD const DOutputStream& stream() const D_NOEXCEPT
    {
        return m_stream;
    }

private:
    DOutputStream m_stream;
};

// 1.1.3
// output_stream_sink
//   type: a sink over the caller's own output stream, which must outlive it:
// any destination a DOutputStream reaches. Copyable; it borrows the stream,
// which keeps the failure (DOutputStreamSink, cpp/util/output).
typedef DOutputStreamSink output_stream_sink;


//==============================================================================
// 2.  PRINTER FUNCTOR
//==============================================================================


// 2.1    Transforms (internal)
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.1.1
    // DIdentityFn
    //   function object: returns its input unchanged. Use as a printer's
    // transform when inputs are already in the destination's byte form
    // (a std::string, string_view, or other `.data()`/`.size()` range);
    // it is zero-copy but does NOT accept bare string literals.
    struct DIdentityFn
    {
        template<typename In>
        const In& operator()(
            const In& _in
        ) const
        {
            return _in;
        }
    };


    // 2.1.2
    // DToStringFn
    //   function object: coerces an input to an owned std::string -- the
    // writers' default transform, so string literals, `const char*`,
    // std::string, and (C++17) std::string_view all write without the
    // caller supplying a printer. Non-string inputs do not compile here,
    // which correctly forces an explicit printer for them.
    struct DToStringFn
    {
        template<typename In>
        std::string operator()(
            const In& _in
        ) const
        {
            return std::string(_in);
        }
    };


    // 2.1.3
    // DComposeFn
    //   function object: the composition g . f -- applies `f` then `g`.
    // Produced by `printer::then`; kept as a named type (not a lambda) so a
    // printer's function member is a stable, nameable type.
    template<typename F,
             typename G>
    struct DComposeFn
    {
        F f;
        G g;

        template<typename In>
        auto operator()(
            const In& _in
        ) const -> decltype(g(f(_in)))
        {
            return g(f(_in));
        }
    };

NS_END  // internal

// 2.2    The printer
//------------------------------------------------------------------------------
// 2.2.1
// DPrinter
//   class: a pure transformation `In -> Out`, wrapped as a composable
// functor. Applying a printer never mutates its input (the input is taken
// by const reference). `Fn` is the underlying transform.
template<typename Fn>
class DPrinter
{
public:
    using function_type = Fn;

    // printer ()
    //   constructor: default -- usable when `Fn` is default-constructible
    // (the identity / to-string transforms used as writer defaults). For a
    // lambda-based `Fn` this is simply not available, which is harmless.
    DPrinter() = default;

    // printer (fn)
    //   constructor: bind the underlying transform.
    explicit DPrinter(
        Fn _fn
    )
        : m_fn(static_cast<Fn&&>(_fn))
    {}

    // operator()
    //   function: apply the transform, producing output from input without
    // modifying the input.
    template<typename In>
    D_NODISCARD auto operator()(
        const In& _in
    ) const -> decltype(std::declval<const Fn&>()(_in))
    {
        return m_fn(_in);
    }

    // then
    //   function: compose -- returns a printer that applies THIS transform
    // and then `_next` (any callable, including another printer).
    template<typename Next>
    D_NODISCARD auto then(
        Next _next
    ) const
    {
        return DPrinter<internal::DComposeFn<Fn, Next>>(
            internal::DComposeFn<Fn, Next>{
                m_fn, static_cast<Next&&>(_next) });
    }

    // function
    //   function: the underlying transform.
    D_NODISCARD const Fn& function() const
    {
        return m_fn;
    }

private:
    Fn m_fn;
};


// 2.2.2
// make_printer
//   function: build a printer from any callable, deducing and decaying the
// stored transform type.
template<typename Fn>
D_NODISCARD DPrinter<typename std::decay<Fn>::type> make_printer(
    Fn&& _fn
)
{
    return DPrinter<typename std::decay<Fn>::type>(
        static_cast<Fn&&>(_fn));
}


// 2.2.3
// id_printer
//   function: the identity printer -- returns its input unchanged.
D_NODISCARD inline DPrinter<internal::DIdentityFn> id_printer()
{
    return DPrinter<internal::DIdentityFn>(internal::DIdentityFn{});
}


// 2.2.4
// print
//   function: apply `_printer` to `_value`, returning the produced output.
// `_value` is not modified. A convenience for the one-shot case; the
// fluent `print(value).to(...)` builder is a later layer.
template<typename In,
         typename Printer>
D_NODISCARD auto print(
    const In&      _value,
    const Printer& _printer
) -> decltype(_printer(_value))
{
    return _printer(_value);
}


//==============================================================================
// 3.  WRITER FUNCTOR
//==============================================================================


// 3.1    The writer
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.1.1
    // sink_has_ok
    //   trait: whether a sink can say whether its writes succeeded, by an
    // ok() a const sink answers.
    template<typename Sink,
             typename = void>
    struct sink_has_ok : std::false_type
    {};

    template<typename Sink>
    struct sink_has_ok<Sink,
                       decltype(static_cast<void>(
                           std::declval<const Sink&>().ok()))>
        : std::true_type
    {};

    // sink_ok
    //   function: the sink's ok(), or true for a sink that cannot fail
    // visibly.
    template<typename Sink>
    bool sink_ok(
        const Sink& _sink,
        std::true_type
    )
    {
        return static_cast<bool>(_sink.ok());
    }

    template<typename Sink>
    bool sink_ok(
        const Sink& _sink,
        std::false_type
    )
    {
        (void)_sink;

        return true;
    }

NS_END  // internal

// 3.1.2
// DWriter
//   class: an effectful builder over a sink. `write(x)` renders `x` through
// the bound printer and appends the result to the sink, leaving `x`
// unchanged. `Sink` is any callable `(const char*, std::size_t)`;
// `Printer` renders an input into a contiguous `.data()`/`.size()` range
// (a std::string by default, via the to-string printer -- so literals,
// `const char*`, and std::string all write without a custom printer).
// Returns `*this` so writes chain.
template<typename Sink,
         typename Printer = DPrinter<internal::DToStringFn>>
class DWriter
{
public:
    using sink_type    = Sink;
    using printer_type = Printer;

    // writer (sink)
    //   constructor: bind a sink; default to-string printer (text inputs).
    explicit DWriter(
        Sink _sink
    )
        : m_sink(static_cast<Sink&&>(_sink))
        , m_printer()
    {}

    // writer (sink, printer)
    //   constructor: bind a sink and a value-rendering printer.
    DWriter(
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
    DWriter&
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
    DWriter&
    operator()(
        const In& _value
    )
    {
        return write(_value);
    }

    // ok
    //   function: whether every write so far reached the destination, when
    // the sink can say (the sinks above all can); true for one that cannot.
    D_NODISCARD bool
    ok() const
    {
        return internal::sink_ok(m_sink, internal::sink_has_ok<Sink>());
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


// 3.1.3
// make_writer
//   function: build a writer from a sink (and optional printer), decaying
// the stored sink type.
template<typename Sink>
D_NODISCARD DWriter<typename std::decay<Sink>::type> make_writer(
    Sink&& _sink
)
{
    return DWriter<typename std::decay<Sink>::type>(
        static_cast<Sink&&>(_sink));
}

template<typename Sink,
         typename Printer>
D_NODISCARD DWriter<typename std::decay<Sink>::type, Printer> make_writer(
    Sink&&  _sink,
    Printer _printer
)
{
    return DWriter<typename std::decay<Sink>::type, Printer>(
        static_cast<Sink&&>(_sink), static_cast<Printer&&>(_printer));
}


// 3.2    Writers with a destination
//------------------------------------------------------------------------------
// 3.2.1
// DStringWriter
//   class: a writer that OWNS its output buffer (a std::string) and exposes
// it. Convenient when the destination is an in-memory string rather than an
// external sink. Renders inputs through `Printer` (to-string by default).
template<typename Printer = DPrinter<internal::DToStringFn>>
class DStringWriter
{
public:
    using printer_type = Printer;

    // DStringWriter ()
    //   constructor: empty buffer; default to-string printer.
    DStringWriter()
        : m_buffer()
        , m_printer()
    {}

    // DStringWriter (printer)
    //   constructor: empty buffer; explicit value-rendering printer.
    explicit DStringWriter(
        Printer _printer
    )
        : m_buffer()
        , m_printer(static_cast<Printer&&>(_printer))
    {}

    // write
    //   function: render `_value` and append it to the owned buffer.
    template<typename In>
    DStringWriter&
    write(
        const In& _value
    )
    {
        const auto& _fragment = m_printer(_value);
        m_buffer.append(_fragment.data(), _fragment.size());

        return *this;
    }

    template<typename In>
    DStringWriter&
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


// 3.2.2
// stream_writer
//   function: a writer bound to a std::ostream destination. The returned
// writer holds a `DStreamSink`; the stream must outlive it, and the writer's
// ok() says whether every write reached it.
template<typename Printer = DPrinter<internal::DToStringFn>>
D_NODISCARD DWriter<DStreamSink, Printer>
stream_writer(
    std::ostream& _stream
)
{
    return DWriter<DStreamSink, Printer>(DStreamSink(_stream));
}

template<typename Printer>
D_NODISCARD DWriter<DStreamSink, Printer> stream_writer(
    std::ostream& _stream,
    Printer       _printer
)
{
    return DWriter<DStreamSink, Printer>(
        DStreamSink(_stream), static_cast<Printer&&>(_printer));
}

// 3.2.3
// output_writer
//   function: a writer bound to the caller's output stream -- the console, a
// file, a dj_string, a caller's array, several at once. The returned writer
// holds an `output_stream_sink`; the stream must outlive it, and keeps any
// failure.
template<typename Printer = DPrinter<internal::DToStringFn>>
D_NODISCARD DWriter<output_stream_sink, Printer>
output_writer(
    DOutputStream& _stream
)
{
    return DWriter<output_stream_sink, Printer>(output_stream_sink(_stream));
}

template<typename Printer>
D_NODISCARD DWriter<output_stream_sink, Printer> output_writer(
    DOutputStream& _stream,
    Printer        _printer
)
{
    return DWriter<output_stream_sink, Printer>(
        output_stream_sink(_stream), static_cast<Printer&&>(_printer));
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_TEXT_WRITER_HPP
