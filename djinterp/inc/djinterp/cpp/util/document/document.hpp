/*******************************************************************************
* djinterp [cpp]                                                    document.hpp
*
*   A fluent facade over the build side (`DDocumentWriter`) and the print
* side (`DDocumentPrinter`): one object you build a tree into and then
* serialise, with the format carried as a policy. The façade adds ergonomics
* only -- it runs once, before the serialisation loop -- so it costs nothing
* per element.
*   ONE FAÇADE, BOTH EXPRESSIBILITIES:
*   `DDocument<Policy>` is generic over the policy, so the SAME façade serves
* both selection modes:
*     compile-time   using DXmlDocument  = DDocument<DXmlPrintPolicy>;
*                    using DHtmlDocument = DDocument<DHtmlPrintPolicy>;
*       -- the policy is a type, fully inlined, zero cost.
*     runtime        DDocument<DBoxedPrintPolicy> doc( html_policy() );
*       -- the policy is a value chosen at runtime (XML vs HTML from a config
*          string, say), dispatched through `DBoxedPrintPolicy`.
* Building the tree is identical either way; the format lives entirely in the
* policy at serialisation time.
*
*   EVERY DESTINATION SAYS WHETHER THE DOCUMENT ARRIVED. to_stream and
* to_file write through an output stream (cpp/util/output), which keeps the
* first failure -- a file that would not open, a std::ostream gone bad, a
* full disk -- and each returns true only when every byte reached its
* destination; to_file also counts the final flush and close, as fclose
* does. to_stream takes a std::ostream or any DOutputStream.
*
*   Requires C++17 (the writer and printer layers it composes are C++17);
* self-suppresses below it.
*
*
* path:      /inc/djinterp/cpp/util/document/document.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.18
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DOCUMENT FACADE
    ---------------
    1.  The facade
         1.  DDocument<Policy>
         2.  DXmlDocument
*/

#ifndef DJINTERP_UTIL_DOCUMENT_DOCUMENT_HPP
#define DJINTERP_UTIL_DOCUMENT_DOCUMENT_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <ostream>  // std::ostream
#include <string>   // std::string
// djinterp
#include "../../djinterp.hpp"               // framework root
#include "../../text/printer.hpp"           // DDocumentPrinter, render,
                                            // policies, the sinks
#include "../output/output_stream.hpp"      // DOutputStream
#include "./document_writer.hpp"            // DDocumentWriter, cursor


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


//==============================================================================
// 1.  DOCUMENT FACADE
//==============================================================================


// 1.1    The facade
//------------------------------------------------------------------------------
// 1.1.1
// DDocument
//   class: build-and-serialise in one handle. Build the tree through
// `root(...)` and cursors exactly as with `DDocumentWriter`; configure
// rendering fluently (`pretty`, `indent`, `set_options`); then emit with
// `to_string` / `to_stream` / `to_file`. The policy is held as a member, so
// `DDocument<some_concrete_policy>` is a compile-time choice and
// `DDocument<DBoxedPrintPolicy>` (constructed with a runtime policy value) is
// a runtime choice.
template<typename Policy = DXmlPrintPolicy>
class DDocument
{
public:
    // document ()
    //   constructor: default -- available when `Policy` is default-
    // constructible (the compile-time / concrete-policy case).
    DDocument() = default;

    // document (policy)
    //   constructor: supply the policy value. Required for a boxed/runtime
    // policy, which has no default.
    explicit DDocument(
        Policy _policy
    )
        : m_policy(static_cast<Policy&&>(_policy))
    {}

    // -- build ---------------------------------------------------------------

    // root -- create the root element and return a cursor at it (once).
    DCursor
    root(
        DXmlString _name
    )
    {
        return m_writer.root(static_cast<DXmlString&&>(_name));
    }

    // at -- a cursor at a previously saved stable index.
    D_NODISCARD DCursor
    at(
        dj_index _index
    )
    {
        return m_writer.at(_index);
    }

    // -- configure (fluent) --------------------------------------------------

    DDocument&
    pretty(
        bool _pretty
    )
    {
        m_options.pretty = _pretty;

        return *this;
    }

    DDocument&
    indent(
        DXmlString _indent
    )
    {
        m_options.indent = static_cast<DXmlString&&>(_indent);

        return *this;
    }

    DDocument&
    set_options(
        DPrintOptions _options
    )
    {
        m_options = static_cast<DPrintOptions&&>(_options);

        return *this;
    }

    // -- serialise -----------------------------------------------------------

    // to_string -- render the document to a fresh string.
    D_NODISCARD DXmlString
    to_string() const
    {
        return render(m_writer.document(), m_policy, m_options);
    }

    // to_stream -- render the document into a std::ostream; true when every
    // byte reached it (a stream gone bad, or one that throws, is false).
    bool
    to_stream(
        std::ostream& _stream
    ) const
    {
        DDocumentPrinter<DStreamSink, Policy> _printer(
            DStreamSink(_stream), m_options, m_policy);
        _printer.print(m_writer.document());

        return _printer.sink().ok();
    }

    // to_stream -- render the document into any output stream: the console,
    // a file, a dj_string, a caller's array, several at once; true while the
    // stream is healthy, which keeps the reason when it is not.
    bool
    to_stream(
        DOutputStream& _stream
    ) const
    {
        DDocumentPrinter<output_stream_sink, Policy> _printer(
            output_stream_sink(_stream), m_options, m_policy);
        _printer.print(m_writer.document());

        return _stream.is_valid();
    }

    // to_file -- render the document into a file (binary mode, verbatim);
    // true only when the file opened, every byte was written, and it closed.
    bool
    to_file(
        const std::string& _path
    ) const
    {
        DOutputStream _file;

        (void)_file.open_file(_path.c_str());

        const bool written = to_stream(_file);

        return _file.close() && written;
    }

    // -- access --------------------------------------------------------------

    D_NODISCARD const DDocumentWriter&
    writer() const
    {
        return m_writer;
    }

    D_NODISCARD const DPrintOptions&
    options() const
    {
        return m_options;
    }

private:
    DDocumentWriter m_writer;
    DPrintOptions   m_options;
    Policy          m_policy;
};


// 1.1.2
// DXmlDocument
//   type: the fluent façade fixed to XML serialisation (compile-time policy).
using DXmlDocument = DDocument<DXmlPrintPolicy>;


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_DOCUMENT_HPP
