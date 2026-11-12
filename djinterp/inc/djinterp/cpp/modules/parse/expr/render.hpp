/*******************************************************************************
* djinterp [cpp/parse]                                                render.hpp
*
* Expression core v2's C++ face (design section 13): rendering.
*   render() writes a subtree as text with d_parse_expr_render: into a
* std::string, or through a writer, any object with
*     bool operator()(const char* bytes, std::size_t length)
* A leaf is written by the options' C atom writer, or by a C++ one, any
* object with
*     bool operator()(DNodeView leaf, std::string& out)
* which appends the leaf's text. Nothing a writer throws crosses the C
* renderer: the face catches it, and the render has failed.
*   Compiled where the renderer is (D_CFG_PARSE_EXPR_RENDER).
*
*
* TABLE OF CONTENTS
* =================
* 1.  OPTIONS
*     -------
*     1.  DRenderOptions
* 2.  THUNKS
*     ------
*     1.  DRenderThunks, DStringWriter, DNoAtomWriter
* 3.  RENDERING
*     ---------
*     1.  Through a writer
*     2.  Into a string
*
*
* path:      /inc/djinterp/cpp/modules/parse/expr/render.hpp
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_PARSE_EXPR_RENDER_HPP
#define DJINTERP_PARSE_EXPR_RENDER_HPP 1

// djinterp
#include "./term.hpp"                                 // DTerm, DNodeView
#include "./signature.hpp"                            // DSignature
#include "../../../../c/modules/parse/expr/render.h"  // the C renderer
// std
#include <cstddef>  // std::size_t
#include <cstring>  // std::memset
#include <string>   // std::string


#if (D_INTERNAL_PARSE_EXPR_RENDER == 1)


NS_DJINTERP
NS_PARSE
NS_EXPR


//==============================================================================
// 1.  OPTIONS
//==============================================================================


// 1.1    DRenderOptions
//------------------------------------------------------------------------------
// DRenderOptions
//   class: the render's options (render.h 1.2.2), zeroed: leaves as their
// spans' text where `text` is given, holes as "?", the author's groups not
// kept.
class DRenderOptions : public ::dj_parse_expr_render_options
{
public:
    DRenderOptions()
    {
        std::memset(static_cast< ::dj_parse_expr_render_options* >(this), 0,
                    sizeof(::dj_parse_expr_render_options));
    }
};

// a DRenderOptions is the C options, and is passed as them
D_STATIC_ASSERT(sizeof(DRenderOptions) ==
                    sizeof(::dj_parse_expr_render_options),
                "DRenderOptions must add nothing to the C options");


//==============================================================================
// 2.  THUNKS
//==============================================================================


// 2.1    DRenderThunks
//------------------------------------------------------------------------------
// DRenderThunks
//   class: the C writer and atom writer for C++ ones, each given as its
// context; what they throw stops here, a failure.
template<typename Writer,
         typename AtomWriter>
class DRenderThunks
{
public:
    // writes through a C++ writer
    static std::size_t
    write(
        void*       _ctx,
        const char* _bytes,
        std::size_t _length
    )
    {
        try
        {
            return ((*static_cast<Writer*>(_ctx))(_bytes, _length))
                   ? _length
                   : 0u;
        }
        catch (...)
        {
            return 0u;
        }
    }

    // a leaf through a C++ atom writer, then the C write function
    static bool
    atom(
        void*                       _ctx,
        const ::dj_parse_expr_term* _term,
        re_std::uint32_t            _node,
        ::dj_parse_expr_write_fn    _write,
        void*                       _write_ctx
    )
    {
        try
        {
            std::string text;

            if (!(*static_cast<AtomWriter*>(_ctx))(DNodeView(_term, _node),
                                                   text))
            {
                return false;
            }

            return ( (text.empty()) ||
                     (_write(_write_ctx, text.data(), text.size()) ==
                      text.size()) );
        }
        catch (...)
        {
            return false;
        }
    }
};

// DStringWriter
//   class: a writer that appends to a string.
class DStringWriter
{
public:
    explicit DStringWriter(
        std::string& _out
    )
        : m_out(&_out)
    {}

    bool
    operator()(
        const char* _bytes,
        std::size_t _length
    )
    {
        m_out->append(_bytes, _length);

        return true;
    }

private:
    std::string* m_out;
};

// DNoAtomWriter
//   class: the atom writer of a render that has none of C++'s.
class DNoAtomWriter
{
public:
    bool
    operator()(
        DNodeView,
        std::string&
    )
    {
        return false;
    }
};


//==============================================================================
// 3.  RENDERING
//==============================================================================


// 3.1    Through a writer
//------------------------------------------------------------------------------
// render_to
//   function: writes a subtree through a writer, leaves as the options say.
// Returns true when the whole text was written.
template<typename Writer>
bool
render_to(
    const DSignature&                     _signature,
    const DTerm&                          _term,
    re_std::uint32_t                      _root,
    Writer&                               _writer,
    const ::dj_parse_expr_render_options* _options = NULL
)
{
    return d_parse_expr_render(_signature.get(), _term.get(), _root,
                               _options,
                               &DRenderThunks<Writer, DNoAtomWriter>::write,
                               &_writer);
}

// render_to
//   function: writes a subtree through a writer, each leaf by a C++ atom
// writer, the options' other settings kept.
template<typename Writer,
         typename AtomWriter>
bool
render_to(
    const DSignature&                     _signature,
    const DTerm&                          _term,
    re_std::uint32_t                      _root,
    Writer&                               _writer,
    AtomWriter&                           _atom,
    const ::dj_parse_expr_render_options* _options = NULL
)
{
    DRenderOptions options;

    if (_options)
    {
        static_cast< ::dj_parse_expr_render_options& >(options) = *_options;
    }

    options.atom     = &DRenderThunks<Writer, AtomWriter>::atom;
    options.atom_ctx = &_atom;

    return d_parse_expr_render(_signature.get(), _term.get(), _root,
                               &options,
                               &DRenderThunks<Writer, AtomWriter>::write,
                               &_writer);
}


// 3.2    Into a string
//------------------------------------------------------------------------------
// render
//   function: a subtree's text, appended to `_out`. Returns true when the
// whole text was written.
inline bool
render(
    const DSignature&                     _signature,
    const DTerm&                          _term,
    re_std::uint32_t                      _root,
    std::string&                          _out,
    const ::dj_parse_expr_render_options* _options = NULL
)
{
    DStringWriter writer(_out);

    return render_to(_signature, _term, _root, writer, _options);
}

// render
//   function: as above, each leaf by a C++ atom writer.
template<typename AtomWriter>
bool
render(
    const DSignature&                     _signature,
    const DTerm&                          _term,
    re_std::uint32_t                      _root,
    AtomWriter&                           _atom,
    std::string&                          _out,
    const ::dj_parse_expr_render_options* _options = NULL
)
{
    DStringWriter writer(_out);

    return render_to(_signature, _term, _root, writer, _atom, _options);
}

// render
//   function: the term's root's text; empty where it could not be rendered.
inline std::string
render(
    const DSignature&                     _signature,
    const DTerm&                          _term,
    const ::dj_parse_expr_render_options* _options = NULL
)
{
    std::string text;

    if (!render(_signature, _term, _term.root(), text, _options))
    {
        text.clear();
    }

    return text;
}


NS_END  // expr
NS_END  // parse
NS_END  // djinterp


#endif  // D_INTERNAL_PARSE_EXPR_RENDER


#endif  // DJINTERP_PARSE_EXPR_RENDER_HPP
