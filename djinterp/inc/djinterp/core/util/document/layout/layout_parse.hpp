/*******************************************************************************
* djinterp [core]                                               layout_parse.hpp
*
*   The TEXTUAL construction surface -- a block-form DSL parsed into a layout_doc.
* A construct is a keyword, an optional quoted argument, optional key="value"
* attributes, and an optional { ... } child block:
*
*       section "Introduction" style="lead" {
*           content "intro.body"
*           section "Motivation" { content "intro.motivation" }
*       }
*
* The top-level sequence of blocks is wrapped in the dialect's root construct.
* The result is the SAME annotated term the other surfaces produce.
*
*   DIALECT-DRIVEN, NOTHING HARDCODED.  The grammar is configured by a
* layout_grammar: which keywords are block constructs (keyword -> op), which are
* content leaves (keyword -> atom kind), the root op, and where a positional
* argument lands in the bag.  The parser names no construct; a dialect supplies
* the table, exactly as the interpreter is driven by the signature.
*
*   ON THE PARSE SUBFRAMEWORK.  This is a recursive descent over
* parse::parse_state<char>, returning parse::parse_result<...> -- the
* subframework's carrier and result.  Each sub-parser has the shape
* (parse_state& -> parse_result), which IS the signature parser<R, E> erases, so
* lifting these onto the CRTP combinators (parser/combinators.hpp) is mechanical.
* A recursive descent is used here (rather than a self-referential combinator
* tree) because the recursive block rule needs the erased-handle knot, and this
* form is what was validated end to end.
*
*   NOTE: include paths are relative to /inc/djinterp/core/util/document/; adjust to
* your build's -I roots if they differ (the parse subframework sits at
* /inc/djinterp/parse, beside core rather than under it).
*
*
* path:      /inc/djinterp/core/util/document/layout/layout_parse.hpp
* link(s):   ch-parsing.tex, ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    LAYOUT GRAMMAR                      (the dialect's parse configuration)
      -----------------------------------------------------------------------

II.   SCANNING                            (whitespace / identifier / string)
      ----------------------------------------------------------------------

III.  BLOCK GRAMMAR                       (block / attrs / sequence)
      --------------------------------------------------------------

IV.   ENTRY                               (parse_document)
      ----------------------------------------------------
*/

#ifndef DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_PARSE_HPP
#define DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_PARSE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../../../djinterp.hpp"          // NS_*, gates, D_NODISCARD
#include "./layout.hpp"                // layout_doc, layout_atom, apply_node,
                                       // make_leaf_node
#include "../../../../parse/parse.hpp"    // parse_state, parse_result, parse_status, DParseStatus*


#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    LAYOUT GRAMMAR                                        ///
///////////////////////////////////////////////////////////////////////////////

// layout_grammar
//   struct: the dialect's parse configuration -- everything the generic parser
// needs to turn keywords into constructs, and nothing construct-specific baked
// in.  block_ops maps a keyword to its operator; leaf_words maps a keyword to
// the atom kind of the content leaf it builds; root_op wraps the top level; and
// arg_key names the bag entry a block's positional argument fills (a section's
// title, typically).  Specialised to the default layout_atom, since parsing
// produces content references.
template<typename OpId>
struct layout_grammar
{
    std::vector<std::pair<std::string, OpId> >                  block_ops;
    std::vector<std::pair<std::string, layout_atom::kind_t> >    leaf_words;
    OpId                                                         root_op;
    std::string                                                  arg_key;

    // layout_grammar (default)
    layout_grammar()
        : block_ops (),
          leaf_words(),
          root_op   (),
          arg_key   ("title")
    {}

    // op_for
    //   the operator for a block keyword, or null when it is not one.
    D_NODISCARD const OpId*
    op_for(
        const std::string& _keyword
    ) const
    {
        for (std::size_t _i = 0; _i < block_ops.size(); ++_i)
        {
            if (block_ops[_i].first == _keyword)
            {
                return &block_ops[_i].second;
            }
        }

        return nullptr;
    }

    // leaf_kind_for
    //   the atom kind for a leaf keyword via _out, or false when it is not one.
    D_NODISCARD bool
    leaf_kind_for(
        const std::string&    _keyword,
        layout_atom::kind_t&  _out
    ) const
    {
        for (std::size_t _i = 0; _i < leaf_words.size(); ++_i)
        {
            if (leaf_words[_i].first == _keyword)
            {
                _out = leaf_words[_i].second;
                return true;
            }
        }

        return false;
    }
};


///////////////////////////////////////////////////////////////////////////////
///             II.   SCANNING                                              ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // peek_char
    //   helper: the current character, or '\0' at end of input.
    D_NODISCARD inline char
    peek_char(
        parse::parse_state<char>& _state
    )
    {
        return _state.at_end() ? '\0' : (*_state.current());
    }

    // is_space_char / is_ident_char
    //   helper: the character classes the DSL cares about.
    D_NODISCARD inline bool
    is_space_char(char _c)
    {
        return ( (_c == ' ')  || (_c == '\t') ||
                 (_c == '\n') || (_c == '\r') );
    }

    D_NODISCARD inline bool
    is_ident_char(char _c)
    {
        return ( ( (_c >= 'a') && (_c <= 'z') ) ||
                 ( (_c >= 'A') && (_c <= 'Z') ) ||
                 (_c == '_') );
    }

    // skip_spaces
    //   helper: consume any run of whitespace.
    inline void
    skip_spaces(
        parse::parse_state<char>& _state
    )
    {
        while ( (!_state.at_end()) &&
                is_space_char(*_state.current()) )
        {
            _state.advance(1);
        }

        return;
    }

    // scan_identifier
    //   helper: a run of identifier characters after leading whitespace (empty
    // when none is present).
    D_NODISCARD inline std::string
    scan_identifier(
        parse::parse_state<char>& _state
    )
    {
        skip_spaces(_state);

        std::string _out;

        while ( (!_state.at_end()) &&
                is_ident_char(*_state.current()) )
        {
            _out.push_back(*_state.current());
            _state.advance(1);
        }

        return _out;
    }

    // scan_string_literal
    //   helper: a "double-quoted" literal after leading whitespace, its body
    // returned via _out.  Returns false (without consuming) when no quote opens,
    // and on an unterminated literal.
    D_NODISCARD inline bool
    scan_string_literal(
        parse::parse_state<char>& _state,
        std::string&              _out
    )
    {
        skip_spaces(_state);

        if ( _state.at_end() ||
             (*_state.current() != '"') )
        {
            return false;
        }

        _state.advance(1);   // opening quote
        _out.clear();

        while ( (!_state.at_end()) &&
                (*_state.current() != '"') )
        {
            _out.push_back(*_state.current());
            _state.advance(1);
        }

        if (_state.at_end())
        {
            return false;    // unterminated
        }

        _state.advance(1);   // closing quote

        return true;
    }

    // make_leaf_atom
    //   helper: a content-leaf atom of the requested kind carrying _arg.
    D_NODISCARD inline layout_atom
    make_leaf_atom(
        layout_atom::kind_t _kind,
        const std::string&  _arg
    )
    {
        if (_kind == layout_atom::kind_literal)
        {
            return layout_atom::literal(_arg);
        }

        if (_kind == layout_atom::kind_meta_ref)
        {
            return layout_atom::meta_ref(_arg);
        }

        return layout_atom::body_ref(_arg);
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             III.  BLOCK GRAMMAR                                         ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // (mutual recursion: a block may contain a brace-delimited sequence of
    //  blocks)
    template<typename OpId>
    parse::parse_result<layout_doc<OpId> >
    parse_block(
        parse::parse_state<char>&    _state,
        const layout_grammar<OpId>& _grammar
    );

    // scan_attributes
    //   helper: consume zero or more key="value" pairs into _bag.  Stops at the
    // first token that is not an attribute, restoring the offset so the caller
    // sees it.  Fails only on a key with '=' but no valid quoted value.
    template<typename OpId>
    D_NODISCARD bool
    scan_attributes(
        parse::parse_state<char>&    _state,
        doc_attributes&              _bag,
        std::string&                 _error,
        std::size_t&                 _error_at
    )
    {
        for (;;)
        {
            std::size_t _save = _state.offset;

            std::string _key = scan_identifier(_state);
            if (_key.empty())
            {
                _state.offset = _save;
                return true;
            }

            skip_spaces(_state);

            if ( _state.at_end() ||
                 (*_state.current() != '=') )
            {
                _state.offset = _save;   // an identifier, but not an attribute
                return true;
            }

            _state.advance(1);           // '='

            std::string _value;
            if (!scan_string_literal(_state, _value))
            {
                _error    = "expected a quoted attribute value";
                _error_at = _state.offset;
                return false;
            }

            _bag.set(_key, _value);
        }
    }

    // parse_block_sequence
    //   helper: a run of blocks.  When _braced, it is bracketed by { } and ends
    // at the matching }; otherwise it runs to end of input (the top level).
    template<typename OpId>
    D_NODISCARD parse::parse_result<std::vector<layout_doc<OpId> > >
    parse_block_sequence(
        parse::parse_state<char>&    _state,
        const layout_grammar<OpId>& _grammar,
        bool                         _braced
    )
    {
        using vec_type = std::vector<layout_doc<OpId> >;
        using out_type = parse::parse_result<vec_type>;

        vec_type _out;

        if (_braced)
        {
            skip_spaces(_state);

            if ( _state.at_end() ||
                 (*_state.current() != '{') )
            {
                return out_type::make_error(
                    parse::DParseStatusFailure, _state.offset,
                    "expected '{'");
            }

            _state.advance(1);
        }

        for (;;)
        {
            skip_spaces(_state);

            if (_braced)
            {
                if ( (!_state.at_end()) &&
                     (*_state.current() == '}') )
                {
                    _state.advance(1);
                    break;
                }

                if (_state.at_end())
                {
                    return out_type::make_error(
                        parse::DParseStatusEndOfInput, _state.offset,
                        "unterminated '{' block");
                }
            }
            else
            {
                if (_state.at_end())
                {
                    break;
                }
            }

            parse::parse_result<layout_doc<OpId> > _block =
                parse_block<OpId>(_state, _grammar);

            if (!_block.ok())
            {
                return out_type(_block.error());
            }

            _out.push_back(_block.value());
        }

        return out_type(_out);
    }

    // parse_block (definition)
    //   helper: one construct -- keyword, optional quoted argument, optional
    // attributes, and (for a block construct) an optional child block.  Resolves
    // the keyword against the grammar: a leaf keyword builds a content leaf; a
    // block keyword builds an application, its argument landing in bag[arg_key].
    template<typename OpId>
    parse::parse_result<layout_doc<OpId> >
    parse_block(
        parse::parse_state<char>&    _state,
        const layout_grammar<OpId>& _grammar
    )
    {
        using out_type = parse::parse_result<layout_doc<OpId> >;

        std::size_t _start = _state.offset;

        std::string _keyword = scan_identifier(_state);
        if (_keyword.empty())
        {
            return out_type::make_error(
                parse::DParseStatusFailure, _start,
                "expected a construct keyword");
        }

        // optional positional argument
        std::string _arg;
        bool        _has_arg = scan_string_literal(_state, _arg);

        // optional attributes
        doc_attributes _bag;
        std::string    _attr_error;
        std::size_t    _attr_error_at = _state.offset;

        if (!scan_attributes<OpId>(_state, _bag, _attr_error, _attr_error_at))
        {
            return out_type::make_error(
                parse::DParseStatusFailure, _attr_error_at, _attr_error);
        }

        // a content leaf?
        layout_atom::kind_t _kind;
        if (_grammar.leaf_kind_for(_keyword, _kind))
        {
            if (!_has_arg)
            {
                return out_type::make_error(
                    parse::DParseStatusFailure, _start,
                    "content construct '" + _keyword + "' needs a name");
            }

            return out_type(
                make_leaf_node<OpId, layout_atom>(
                    _bag, make_leaf_atom(_kind, _arg)));
        }

        // a block construct?
        const OpId* _op = _grammar.op_for(_keyword);
        if (_op == nullptr)
        {
            return out_type::make_error(
                parse::DParseStatusFailure, _start,
                "unknown construct '" + _keyword + "'");
        }

        if (_has_arg)
        {
            _bag.set(_grammar.arg_key, _arg);
        }

        // optional child block
        std::vector<layout_doc<OpId> > _children;

        skip_spaces(_state);

        if ( (!_state.at_end()) &&
             (*_state.current() == '{') )
        {
            parse::parse_result<std::vector<layout_doc<OpId> > > _seq =
                parse_block_sequence<OpId>(_state, _grammar, true);

            if (!_seq.ok())
            {
                return out_type(_seq.error());
            }

            _children = _seq.value();
        }

        return out_type(
            apply_node<OpId, layout_atom>(
                _bag,
                *_op,
                static_cast<std::vector<layout_doc<OpId> >&&>(_children)));
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             IV.   ENTRY                                                 ///
///////////////////////////////////////////////////////////////////////////////

// parse_document
//   function: parse a whole document from _state -- a top-level sequence of
// blocks wrapped in the grammar's root construct.  Fails on any malformed block
// and on trailing input the grammar did not consume.  The core entry; it takes
// the subframework's state so a caller may position or reuse it.
template<typename OpId>
D_NODISCARD
parse::parse_result<layout_doc<OpId> >
parse_document(
    parse::parse_state<char>&    _state,
    const layout_grammar<OpId>& _grammar
)
{
    using out_type = parse::parse_result<layout_doc<OpId> >;

    parse::parse_result<std::vector<layout_doc<OpId> > > _seq =
        internal::parse_block_sequence<OpId>(_state, _grammar, false);

    if (!_seq.ok())
    {
        return out_type(_seq.error());
    }

    internal::skip_spaces(_state);

    if (!_state.at_end())
    {
        return out_type::make_error(
            parse::DParseStatusFailure, _state.offset,
            "trailing input after document");
    }

    return out_type(
        apply_node<OpId, layout_atom>(
            doc_attributes(),
            _grammar.root_op,
            _seq.value()));
}

// parse_document (string convenience)
//   function: parse from a source string.  Builds a parse_state over _src's
// characters (parse_state<char> takes a pointer and a length) and delegates;
// the core overload above takes the state directly.
template<typename OpId>
D_NODISCARD
parse::parse_result<layout_doc<OpId> >
parse_document(
    const std::string&           _src,
    const layout_grammar<OpId>& _grammar
)
{
    parse::parse_state<char> _state(_src.data(),
                                   _src.size());

    return parse_document<OpId>(_state, _grammar);
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_PARSE_HPP
