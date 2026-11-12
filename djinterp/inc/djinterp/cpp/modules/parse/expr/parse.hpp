/*******************************************************************************
* djinterp [cpp/parse]                                                 parse.hpp
*
* Expression core v2's C++ face (design section 13): parsing.
*   parse() reads one expression with d_parse_expr_parse and returns a
* DResult: the term, the status, the error's domain, code, span and index,
* the token the parse ended at, and the diagnostics, copied out of the sink
* with their messages, so a result is a plain value at every level. A parse
* may instead report into the caller's sink, and the result then holds no
* copies.
*   A source is a C source (dj_parse_expr_source, as the default scanner
* binds), an array of tokens, or any object with a member
*     int next(re_std::uint8_t position, re_std::uint32_t mode,
*              dj_parse_expr_token& out)
* answering 1 for a token, 0 at the end and -1 on a failure, as the C shape
* does: a C++98 functor class, or from C++11 a lambda through
* make_source(). Nothing a source throws crosses the C parser: the face
* catches it, and the source has failed.
*   DDiagnostics is the C++98 view this face carries of the sink's records
* (the design's section 13 decision: the foundation's diagnostic.hpp is
* C++11-floored, and a copy is what a value result needs anyway).
*
*
* TABLE OF CONTENTS
* =================
* 1.  OPTIONS AND DIAGNOSTICS
*     -----------------------
*     1.  DOptions
*     2.  DDiagnostic
* 2.  RESULTS
*     -------
*     1.  DResult
* 3.  SOURCES
*     -------
*     1.  The thunk
*     2.  DFunctionSource, make_source
* 4.  PARSING
*     -------
*     1.  The one parse
*     2.  Every source
*
*
* path:      /inc/djinterp/cpp/modules/parse/expr/parse.hpp
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_PARSE_EXPR_PARSE_HPP
#define DJINTERP_PARSE_EXPR_PARSE_HPP 1

// djinterp
#include "./term.hpp"                                    // DTerm, NS_EXPR
#include "./signature.hpp"                               // DSignature
#include "../../../../c/modules/parse/diagnostic.h"      // the sink
#include "../../../../c/modules/parse/expr/parse.h"      // the C parser
// std
#include <cstring>  // std::memset
#include <string>   // std::string
#include <vector>   // std::vector
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <utility>  // std::move, std::forward
#endif


NS_DJINTERP
NS_PARSE
NS_EXPR


//==============================================================================
// 1.  OPTIONS AND DIAGNOSTICS
//==============================================================================


// 1.1    DOptions
//------------------------------------------------------------------------------
// DOptions
//   class: the parse's options (parse.h 2.2.2), zeroed: every setting 0, the
// one in force. Its members are the C struct's, set as they are.
class DOptions : public ::dj_parse_expr_options
{
public:
    DOptions()
    {
        std::memset(static_cast< ::dj_parse_expr_options* >(this), 0,
                    sizeof(::dj_parse_expr_options));
    }
};

// a DOptions is the C options, and is passed as them
D_STATIC_ASSERT(sizeof(DOptions) == sizeof(::dj_parse_expr_options),
                "DOptions must add nothing to dj_parse_expr_options");


// 1.2    DDiagnostic
//------------------------------------------------------------------------------
// DDiagnostic
//   class: one record of the sink, copied with its message.
class DDiagnostic
{
public:
    DDiagnostic()
        : m_severity(0u),
          m_flags(0u),
          m_domain(0u),
          m_code(0u)
    {
        std::memset(&m_span, 0, sizeof(m_span));
    }

    DDiagnostic(
        const ::dj_parse_diag_sink*  _sink,
        const ::dj_parse_diagnostic& _record
    )
        : m_severity(_record.severity),
          m_flags(_record.flags),
          m_domain(_record.domain),
          m_code(_record.code),
          m_span(_record.span),
          m_message(d_parse_diag_message(_sink, &_record))
    {}

    // D_PARSE_SEVERITY_NOTE, _WARNING, _ERROR or _FATAL
    re_std::uint8_t
    severity() const
    {
        return m_severity;
    }

    // whether it continues the record before it: an error's note
    bool
    continuation() const
    {
        return ((m_flags & D_PARSE_DIAG_FLAG_CONTINUATION) != 0u);
    }

    re_std::uint16_t
    domain() const
    {
        return m_domain;
    }

    re_std::uint16_t
    code() const
    {
        return m_code;
    }

    const ::dj_parse_span&
    span() const
    {
        return m_span;
    }

    const std::string&
    message() const
    {
        return m_message;
    }

private:
    re_std::uint8_t  m_severity;
    re_std::uint8_t  m_flags;
    re_std::uint16_t m_domain;
    re_std::uint16_t m_code;
    ::dj_parse_span  m_span;
    std::string      m_message;
};

// DDiagnostics
//   type: a result's diagnostics, in the order they were reported.
typedef std::vector<DDiagnostic> DDiagnostics;


//==============================================================================
// 2.  RESULTS
//==============================================================================


// 2.1    DResult
//------------------------------------------------------------------------------
// DResult
//   class: what a parse found (parse.h 2.2.3), with the term it built and the
// diagnostics it reported.
class DResult
{
public:
    DResult()
    {
        std::memset(&m_result, 0, sizeof(m_result));
        m_result.status     = D_PARSE_EXPR_FATAL;
        m_result.root       = D_PARSE_EXPR_NO_NODE;
        m_result.diag_index = 0xFFFFFFFFu;
    }

    // D_PARSE_EXPR_OK, _ERROR or _FATAL
    int
    status() const
    {
        return m_result.status;
    }

    bool
    ok() const
    {
        return (m_result.status == D_PARSE_EXPR_OK);
    }

    re_std::uint32_t
    root() const
    {
        return m_result.root;
    }

    re_std::uint32_t
    errors() const
    {
        return m_result.errors;
    }

    // the error's domain and code; 0 where there is none
    re_std::uint16_t
    domain() const
    {
        return m_result.domain;
    }

    re_std::uint16_t
    code() const
    {
        return m_result.code;
    }

    const ::dj_parse_span&
    span() const
    {
        return m_result.span;
    }

    // the error's index in the sink, 0xFFFFFFFF where it was not stored
    re_std::uint32_t
    diag_index() const
    {
        return m_result.diag_index;
    }

    // the token the parse ended at, unread
    const ::dj_parse_expr_token&
    next() const
    {
        return m_result.next;
    }

    const DTerm&
    term() const
    {
        return m_term;
    }

    DTerm&
    term()
    {
        return m_term;
    }

    // the copies, or none where the caller's sink was given
    const DDiagnostics&
    diagnostics() const
    {
        return m_diagnostics;
    }

    const ::dj_parse_expr_result&
    record() const
    {
        return m_result;
    }

    // the parts a parse fills
    ::dj_parse_expr_result&
    record()
    {
        return m_result;
    }

    DDiagnostics&
    diagnostics()
    {
        return m_diagnostics;
    }

private:
    ::dj_parse_expr_result m_result;
    DTerm                  m_term;
    DDiagnostics           m_diagnostics;
};


//==============================================================================
// 3.  SOURCES
//==============================================================================


// 3.1    The thunk
//------------------------------------------------------------------------------
// DSourceThunk
//   class: the C source's `next` for a C++ source, which it is given as its
// context; what the source throws stops at it, a failure.
template<typename Source>
class DSourceThunk
{
public:
    static int
    next(
        void*                  _ctx,
        re_std::uint8_t        _position,
        re_std::uint32_t       _mode,
        ::dj_parse_expr_token* _out
    )
    {
        try
        {
            return static_cast<Source*>(_ctx)->next(_position, _mode, *_out);
        }
        catch (...)
        {
            return -1;
        }
    }

    // the C source over a C++ one, which must outlive the parse
    static ::dj_parse_expr_source
    bind(
        Source& _source
    )
    {
        ::dj_parse_expr_source source;

        source.next = &DSourceThunk<Source>::next;
        source.ctx  = &_source;

        return source;
    }
};


// 3.2    DFunctionSource, make_source
//------------------------------------------------------------------------------
// DFunctionSource
//   class: a source over a callable `int (re_std::uint8_t, re_std::uint32_t,
// dj_parse_expr_token&)`: a C++98 functor, or a lambda from C++11.
template<typename Function>
class DFunctionSource
{
public:
    explicit DFunctionSource(
        const Function& _function
    )
        : m_function(_function)
    {}

    int
    next(
        re_std::uint8_t        _position,
        re_std::uint32_t       _mode,
        ::dj_parse_expr_token& _out
    )
    {
        return m_function(_position, _mode, _out);
    }

private:
    Function m_function;
};

// make_source
//   function: a source over a callable.
template<typename Function>
DFunctionSource<Function>
make_source(
    const Function& _function
)
{
    return DFunctionSource<Function>(_function);
}


//==============================================================================
// 4.  PARSING
//==============================================================================


// 4.1    The one parse
//------------------------------------------------------------------------------
// parse
//   function: parses with a C source into a result. Without a sink, the
// diagnostics go to one of the parse's own, D_PARSE_DIAG_DEFAULT_ITEMS
// records and D_PARSE_DIAG_DEFAULT_TEXT bytes, and are copied into the
// result; with one, they go there and the result holds no copies.
inline DResult
parse(
    const DSignature&             _signature,
    ::dj_parse_expr_source&       _source,
    const ::dj_parse_expr_options* _options = NULL,
    ::dj_parse_diag_sink*          _sink    = NULL
)
{
    DResult                              result;
    ::dj_parse_diag_sink                 own;
    std::vector< ::dj_parse_diagnostic > items(
        (std::size_t)D_PARSE_DIAG_DEFAULT_ITEMS);
    std::vector<char>                    text(
        (std::size_t)D_PARSE_DIAG_DEFAULT_TEXT);

    d_parse_diag_sink_init(&own, &items[0], (re_std::uint32_t)items.size(),
                           &text[0], (re_std::uint32_t)text.size());

    ::dj_parse_diag_sink* const sink = (_sink) ? _sink : &own;

    (void)d_parse_expr_parse(_signature.get(), &_source, _options,
                             result.term().get(), sink, &result.record());

    for (re_std::uint32_t i = 0u; ( (!_sink) && (i < own.count) ); ++i)
    {
        result.diagnostics().push_back(DDiagnostic(&own, own.items[i]));
    }

    return result;
}


// 4.2    Every source
//------------------------------------------------------------------------------
// parse
//   function: parses tokens already in an array, the END token's offset
// `_end`.
inline DResult
parse(
    const DSignature&              _signature,
    const ::dj_parse_expr_token*   _tokens,
    re_std::uint32_t               _count,
    re_std::uint32_t               _end,
    const ::dj_parse_expr_options* _options = NULL,
    ::dj_parse_diag_sink*          _sink    = NULL
)
{
    ::dj_parse_expr_array_source state;
    ::dj_parse_expr_source       source;

    d_parse_expr_source_array(&source, &state, _tokens, _count, _end);

    return parse(_signature, source, _options, _sink);
}

// parse
//   function: parses tokens in a vector.
inline DResult
parse(
    const DSignature&                          _signature,
    const std::vector< ::dj_parse_expr_token >& _tokens,
    re_std::uint32_t                           _end,
    const ::dj_parse_expr_options*             _options = NULL,
    ::dj_parse_diag_sink*                      _sink    = NULL
)
{
    return parse(_signature,
                 (_tokens.empty()) ? NULL : &_tokens[0],
                 (re_std::uint32_t)_tokens.size(), _end, _options, _sink);
}

// parse
//   function: parses with a C++ source, any object with the member `next`
// (section 3).
template<typename Source>
DResult
parse(
    const DSignature&              _signature,
    Source&                        _source,
    const ::dj_parse_expr_options* _options = NULL,
    ::dj_parse_diag_sink*          _sink    = NULL
)
{
    ::dj_parse_expr_source source = DSourceThunk<Source>::bind(_source);

    return parse(_signature, source, _options, _sink);
}


NS_END  // expr
NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_EXPR_PARSE_HPP
