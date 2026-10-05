/*******************************************************************************
* djinterp [core]                                        table_model_dialect.hpp
*
*   The one-shot convenience over table_model_render.hpp: render a table model
* in a named format and hand back the bytes.  It is separate because it needs
* document_dialect.hpp, which pulls in every renderer -- and the bridge itself
* needs none of them.  A caller that already holds a renderer includes only
* table_model_render.hpp and pays for nothing more; a caller that just wants
* "this table, as HTML" includes this and gets the whole dialect set.  The same
* split as document_format.hpp / document_format_policy.hpp.
*
*   PORTABILITY:
*   C++11 baseline (matches the bridge and the renderers it selects between).
*
*
* path:      /inc/djinterp/core/util/document/templates/table_model_dialect.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.23
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_UTIL_DOCUMENT_TEMPLATES_TABLE_MODEL_DIALECT_HPP
#define DJINTERP_UTIL_DOCUMENT_TEMPLATES_TABLE_MODEL_DIALECT_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <string>
// djinterp
#include "../../../../djinterp.hpp"        // NS_*, D_NODISCARD
#include "../document_format.hpp"       // document_format
#include "./document_dialect.hpp"       // document_dialect, make_document_dialect
#include "./table_model_render.hpp"     // render_table_model, table_render_*


NS_DJINTERP


// render_table_model_to_string
//   function: render _model as a COMPLETE document in _format, with that
// dialect's span / header defaults already applied.  Yields an empty string for
// a format the dialect factory does not serve (pdf, tex, wiki) -- a PDF caller
// constructs pdf_document_renderer itself and calls render_table_model, the
// same opt-in split the rest of the document layer keeps.
template<typename Model>
D_NODISCARD inline std::string
render_table_model_to_string(
    const Model&         _model,
    document_format       _format,
    const doc_attributes& _frame = doc_attributes()
)
{
    document_dialect _dialect = make_document_dialect(_format);

    // an unserved format renders to nothing, not to a wrong dialect
    if (!_dialect.valid())
    {
        return std::string();
    }

    _dialect.renderer().begin_document(_frame);

    render_table_model(_model,
                       _dialect.renderer(),
                       table_render_defaults(_format));

    _dialect.renderer().end_document();

    return _dialect.str();
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_TEMPLATES_TABLE_MODEL_DIALECT_HPP
