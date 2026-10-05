/*******************************************************************************
* djinterp [core]                                                   document.hpp
*
*   A fluent facade over the build side (`document_writer`) and the print
* side (`document_printer`): one object you build a tree into and then
* serialise, with the format carried as a policy. The façade adds ergonomics
* only -- it runs once, before the serialisation loop -- so it costs nothing
* per element.
*   ONE FAÇADE, BOTH EXPRESSIBILITIES:
*   `document<Policy>` is generic over the policy, so the SAME façade serves
* both selection modes:
*     compile-time   using xml_document  = document<xml_print_policy>;
*                    using html_document = document<html_print_policy>;
*       -- the policy is a type, fully inlined, zero cost.
*     runtime        document<boxed_print_policy> doc( html_policy() );
*       -- the policy is a value chosen at runtime (XML vs HTML from a config
*          string, say), dispatched through `boxed_print_policy`.
* Building the tree is identical either way; the format lives entirely in the
* policy at serialisation time.
*
*   Requires C++17 (the writer and printer layers it composes are C++17);
* self-suppresses below it.
*
*
* path:      /inc/djinterp/core/util/document/document.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.18
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    DOCUMENT FACADE
      ---------------
      a. document<Policy>
      b. xml_document
*/

#ifndef DJINTERP_UTIL_DOCUMENT_DOCUMENT_HPP
#define DJINTERP_UTIL_DOCUMENT_DOCUMENT_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <fstream>
#include <ostream>
#include <string>
// djinterp
#include "../../../djinterp.hpp"
#include "../../text/printer.hpp"           // document_printer, render, policies
#include "./document_writer.hpp"   // document_writer, cursor


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                  I.   DOCUMENT FAÇADE                                   ///
///////////////////////////////////////////////////////////////////////////////

// document
//   class: build-and-serialise in one handle. Build the tree through
// `root(...)` and cursors exactly as with `document_writer`; configure
// rendering fluently (`pretty`, `indent`, `set_options`); then emit with
// `to_string` / `to_stream` / `to_file`. The policy is held as a member, so
// `document<some_concrete_policy>` is a compile-time choice and
// `document<boxed_print_policy>` (constructed with a runtime policy value) is
// a runtime choice.
template<typename Policy = xml_print_policy>
class document
{
public:
    // document ()
    //   constructor: default -- available when `Policy` is default-
    // constructible (the compile-time / concrete-policy case).
    document() = default;

    // document (policy)
    //   constructor: supply the policy value. Required for a boxed/runtime
    // policy, which has no default.
    explicit document(
        Policy _policy
    )
        : m_policy(static_cast<Policy&&>(_policy))
    {}

    // -- build ---------------------------------------------------------------

    // root -- create the root element and return a cursor at it (once).
    cursor
    root(
        xml_string_t _name
    )
    {
        return m_writer.root(static_cast<xml_string_t&&>(_name));
    }

    // at -- a cursor at a previously saved stable index.
    D_NODISCARD cursor
    at(
        d_index _index
    )
    {
        return m_writer.at(_index);
    }

    // -- configure (fluent) --------------------------------------------------

    document&
    pretty(
        bool _pretty
    )
    {
        m_options.pretty = _pretty;
        return *this;
    }

    document&
    indent(
        xml_string_t _indent
    )
    {
        m_options.indent = static_cast<xml_string_t&&>(_indent);
        return *this;
    }

    document&
    set_options(
        print_options _options
    )
    {
        m_options = static_cast<print_options&&>(_options);
        return *this;
    }

    // -- serialise -----------------------------------------------------------

    // to_string -- render the document to a fresh string.
    D_NODISCARD xml_string_t
    to_string() const
    {
        return render(m_writer.document(), m_policy, m_options);
    }

    // to_stream -- render the document into an output stream.
    void
    to_stream(
        std::ostream& _stream
    ) const
    {
        document_printer<stream_sink, Policy> _printer(
            stream_sink(_stream), m_options, m_policy);
        _printer.print(m_writer.document());

        return;
    }

    // to_file -- render the document into a file (binary mode, verbatim).
    void
    to_file(
        const std::string& _path
    ) const
    {
        std::ofstream _file(_path, std::ios::out | std::ios::binary);
        document_printer<stream_sink, Policy> _printer(
            stream_sink(_file), m_options, m_policy);
        _printer.print(m_writer.document());

        return;
    }

    // -- access --------------------------------------------------------------

    D_NODISCARD const document_writer&
    writer() const
    {
        return m_writer;
    }

    D_NODISCARD const print_options&
    options() const
    {
        return m_options;
    }

private:
    document_writer m_writer;
    print_options   m_options;
    Policy          m_policy;
};


// xml_document
//   type: the fluent façade fixed to XML serialisation (compile-time policy).
using xml_document = document<xml_print_policy>;


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_DOCUMENT_HPP
