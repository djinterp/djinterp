/******************************************************************************
* djinterp [text]                                                   writer.hpp
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
*   printer<_Fn> -- a PURE transformation `In -> Out`. It takes its input
*     by const reference and returns a freshly produced output; it never
*     mutates the input. Printers compose: `p.then(g)` yields a printer
*     that applies `p` and then `g`. This is the reusable, configurable
*     "render this value to that representation" object -- the generalised
*     form of text_template's `interpolate`.
*
*   writer<_Sink, _Printer> -- an EFFECTFUL builder. It owns a destination
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
* path:      /inc/djinterp/core/text/writer.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.18
******************************************************************************/

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
      b. printer<_Fn>
      c. make_printer, id_printer
      d. print

III.  WRITER FUNCTOR
      --------------
      a. writer<_Sink, _Printer>
      b. make_writer
      c. string_writer
      d. stream_writer
*/

#ifndef DJINTERP_TEXT_WRITER_
#define DJINTERP_TEXT_WRITER_ 1

// std
#include <cstddef>
#include <string>
#include <ostream>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"


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
        template<typename _In>
        const _In&
        operator()(
            const _In& _in
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
        template<typename _In>
        std::string
        operator()(
            const _In& _in
        ) const
        {
            return std::string(_in);
        }
    };


    // compose_fn
    //   function object: the composition g . f -- applies `f` then `g`.
    // Produced by `printer::then`; kept as a named type (not a lambda) so a
    // printer's function member is a stable, nameable type.
    template<typename _F,
             typename _G>
    struct compose_fn
    {
        _F f;
        _G g;

        template<typename _In>
        auto
        operator()(
            const _In& _in
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
// by const reference). `_Fn` is the underlying transform.
template<typename _Fn>
class printer
{
public:
    using function_type = _Fn;

    // printer ()
    //   constructor: default -- usable when `_Fn` is default-constructible
    // (the identity / to-string transforms used as writer defaults). For a
    // lambda-based `_Fn` this is simply not available, which is harmless.
    printer() = default;

    // printer (fn)
    //   constructor: bind the underlying transform.
    explicit printer(
        _Fn _fn
    )
        : m_fn(static_cast<_Fn&&>(_fn))
    {}

    // operator()
    //   function: apply the transform, producing output from input without
    // modifying the input.
    template<typename _In>
    D_NODISCARD auto
    operator()(
        const _In& _in
    ) const
        -> decltype(std::declval<const _Fn&>()(_in))
    {
        return m_fn(_in);
    }

    // then
    //   function: compose -- returns a printer that applies THIS transform
    // and then `_next` (any callable, including another printer).
    template<typename _Next>
    D_NODISCARD auto
    then(
        _Next _next
    ) const
    {
        return printer<internal::compose_fn<_Fn, _Next>>(
            internal::compose_fn<_Fn, _Next>{
                m_fn, static_cast<_Next&&>(_next) });
    }

    // function
    //   function: the underlying transform.
    D_NODISCARD const _Fn&
    function() const
    {
        return m_fn;
    }

private:
    _Fn m_fn;
};


// make_printer
//   function: build a printer from any callable, deducing and decaying the
// stored transform type.
template<typename _Fn>
D_NODISCARD printer<typename std::decay<_Fn>::type>
make_printer(
    _Fn&& _fn
)
{
    return printer<typename std::decay<_Fn>::type>(
        static_cast<_Fn&&>(_fn));
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
template<typename _In,
         typename _Printer>
D_NODISCARD auto
print(
    const _In&      _value,
    const _Printer& _printer
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
// unchanged. `_Sink` is any callable `(const char*, std::size_t)`;
// `_Printer` renders an input into a contiguous `.data()`/`.size()` range
// (a std::string by default, via the to-string printer -- so literals,
// `const char*`, and std::string all write without a custom printer).
// Returns `*this` so writes chain.
template<typename _Sink,
         typename _Printer = printer<internal::to_string_fn>>
class writer
{
public:
    using sink_type    = _Sink;
    using printer_type = _Printer;

    // writer (sink)
    //   constructor: bind a sink; default to-string printer (text inputs).
    explicit writer(
        _Sink _sink
    )
        : m_sink(static_cast<_Sink&&>(_sink))
        , m_printer()
    {}

    // writer (sink, printer)
    //   constructor: bind a sink and a value-rendering printer.
    writer(
        _Sink    _sink,
        _Printer _printer
    )
        : m_sink(static_cast<_Sink&&>(_sink))
        , m_printer(static_cast<_Printer&&>(_printer))
    {}

    // write
    //   function: render `_value` and append it to the sink; `_value` is
    // not modified.
    template<typename _In>
    writer&
    write(
        const _In& _value
    )
    {
        const auto& _fragment = m_printer(_value);
        m_sink(_fragment.data(), _fragment.size());

        return *this;
    }

    // operator()
    //   function: the call face of a writer; forwards to write.
    template<typename _In>
    writer&
    operator()(
        const _In& _value
    )
    {
        return write(_value);
    }

    // sink / printer accessors
    D_NODISCARD const _Sink&
    sink() const
    {
        return m_sink;
    }

    D_NODISCARD const _Printer&
    printer_of() const
    {
        return m_printer;
    }

private:
    _Sink    m_sink;
    _Printer m_printer;
};


// make_writer
//   function: build a writer from a sink (and optional printer), decaying
// the stored sink type.
template<typename _Sink>
D_NODISCARD writer<typename std::decay<_Sink>::type>
make_writer(
    _Sink&& _sink
)
{
    return writer<typename std::decay<_Sink>::type>(
        static_cast<_Sink&&>(_sink));
}

template<typename _Sink,
         typename _Printer>
D_NODISCARD writer<typename std::decay<_Sink>::type, _Printer>
make_writer(
    _Sink&&  _sink,
    _Printer _printer
)
{
    return writer<typename std::decay<_Sink>::type, _Printer>(
        static_cast<_Sink&&>(_sink), static_cast<_Printer&&>(_printer));
}


// string_writer
//   class: a writer that OWNS its output buffer (a std::string) and exposes
// it. Convenient when the destination is an in-memory string rather than an
// external sink. Renders inputs through `_Printer` (to-string by default).
template<typename _Printer = printer<internal::to_string_fn>>
class string_writer
{
public:
    using printer_type = _Printer;

    // string_writer ()
    //   constructor: empty buffer; default to-string printer.
    string_writer()
        : m_buffer()
        , m_printer()
    {}

    // string_writer (printer)
    //   constructor: empty buffer; explicit value-rendering printer.
    explicit string_writer(
        _Printer _printer
    )
        : m_buffer()
        , m_printer(static_cast<_Printer&&>(_printer))
    {}

    // write
    //   function: render `_value` and append it to the owned buffer.
    template<typename _In>
    string_writer&
    write(
        const _In& _value
    )
    {
        const auto& _fragment = m_printer(_value);
        m_buffer.append(_fragment.data(), _fragment.size());

        return *this;
    }

    template<typename _In>
    string_writer&
    operator()(
        const _In& _value
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
    _Printer    m_printer;
};


// stream_writer
//   function: a writer bound to a std::ostream destination. The returned
// writer holds a `stream_sink`; the stream must outlive it.
template<typename _Printer = printer<internal::to_string_fn>>
D_NODISCARD writer<stream_sink, _Printer>
stream_writer(
    std::ostream& _stream
)
{
    return writer<stream_sink, _Printer>(stream_sink(_stream));
}

template<typename _Printer>
D_NODISCARD writer<stream_sink, _Printer>
stream_writer(
    std::ostream& _stream,
    _Printer      _printer
)
{
    return writer<stream_sink, _Printer>(
        stream_sink(_stream), static_cast<_Printer&&>(_printer));
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


#endif  // DJINTERP_TEXT_WRITER_