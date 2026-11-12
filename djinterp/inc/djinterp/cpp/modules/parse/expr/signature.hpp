/*******************************************************************************
* djinterp [cpp/parse]                                             signature.hpp
*
* Expression core v2's C++ face (design section 13): the signature.
*   DSignature holds a dj_parse_expr_signature and builds it with the C
* functions, each builder returning *this so a language's table reads as one
* statement. A builder that fails leaves good() false, and seal() then
* answers false; seal() checks and indexes the whole, as
* d_parse_expr_signature_seal does. A mixfix operator's descriptor is made
* with DMixfix, which is the C descriptor with builders of its own.
*   A signature owns its operators' and roles' arrays where it grows them, so
* it is not copied; it is moved from C++11.
*
*
* TABLE OF CONTENTS
* =================
* 1.  MIXFIX DESCRIPTORS
*     ------------------
*     1.  DMixfix
* 2.  SIGNATURES
*     ----------
*     1.  DSignature
*
*
* path:      /inc/djinterp/cpp/modules/parse/expr/signature.hpp
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_PARSE_EXPR_SIGNATURE_HPP
#define DJINTERP_PARSE_EXPR_SIGNATURE_HPP 1

// djinterp
#include "./term.hpp"                                    // NS_EXPR
#include "../../../../c/modules/parse/expr/signature.h"  // the C signature
// std
#include <cstring>  // std::memset


NS_DJINTERP
NS_PARSE
NS_EXPR


//==============================================================================
// 1.  MIXFIX DESCRIPTORS
//==============================================================================


// 1.1    DMixfix
//------------------------------------------------------------------------------
// DMixfix
//   class: a mixfix operator's descriptor (signature.h 2.1.1), zeroed, with
// builders: bracket() for the common two parts, list() to make its hole a
// list, rows() for a list in rows, with_tail() for an operand after the last
// part, in() for the mode its holes are read in (F18).
class DMixfix : public ::dj_parse_expr_mixfix
{
public:
    DMixfix()
    {
        std::memset(static_cast< ::dj_parse_expr_mixfix* >(this), 0,
                    sizeof(::dj_parse_expr_mixfix));
    }

    // two parts, an opening and a closing; `_lead` 1 after an operand
    DMixfix&
    bracket(
        re_std::uint32_t _open,
        const char*      _open_spelling,
        re_std::uint32_t _close,
        const char*      _close_spelling,
        re_std::uint8_t  _lead = 0u
    )
    {
        this->lead             = _lead;
        this->parts            = 2u;
        this->part[0]          = _open;
        this->part[1]          = _close;
        this->part_spelling[0] = _open_spelling;
        this->part_spelling[1] = _close_spelling;

        return *this;
    }

    // the hole a list: items between `_min` and `_max`
    // (D_PARSE_EXPR_ITEMS_UNBOUNDED), parted by a separator
    DMixfix&
    list(
        re_std::uint32_t _separator,
        const char*      _separator_spelling,
        re_std::uint16_t _min = 0u,
        re_std::uint16_t _max = D_PARSE_EXPR_ITEMS_UNBOUNDED
    )
    {
        this->list_hole          = 1u;
        this->separator          = _separator;
        this->separator_spelling = _separator_spelling;
        this->min_items          = _min;
        this->max_items          = _max;

        return *this;
    }

    // rows parted by another separator, each a `_row_op` node and the whole
    // a `_rows_op` one
    DMixfix&
    rows(
        re_std::uint32_t _row_separator,
        const char*      _row_separator_spelling,
        re_std::uint32_t _row_op,
        re_std::uint32_t _rows_op
    )
    {
        this->row_separator          = _row_separator;
        this->row_separator_spelling = _row_separator_spelling;
        this->row_op                 = _row_op;
        this->rows_op                = _rows_op;

        return *this;
    }

    // an operand after the last part
    DMixfix&
    with_tail()
    {
        this->tail = 1u;

        return *this;
    }

    // the mode the holes are read in, one bit
    DMixfix&
    in(
        re_std::uint32_t _mode
    )
    {
        this->hole_mode = _mode;

        return *this;
    }
};

// a DMixfix is the C descriptor, and is passed as one
D_STATIC_ASSERT(sizeof(DMixfix) == sizeof(::dj_parse_expr_mixfix),
                "DMixfix must add nothing to dj_parse_expr_mixfix");


//==============================================================================
// 2.  SIGNATURES
//==============================================================================


// 2.1    DSignature
//------------------------------------------------------------------------------
// DSignature
//   class: a language, the C signature held by value. Made empty, it grows on
// the heap where the heap is built in; made over the caller's arrays, it
// never grows.
class DSignature
{
public:
    DSignature()
        : m_good(true)
    {
#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
        d_parse_expr_signature_init_heap(&m_signature);
#else
        d_parse_expr_signature_init(&m_signature, NULL, 0u, NULL, 0u);
#endif  // D_INTERNAL_PARSE_EXPR_HEAP
    }

    DSignature(
        ::dj_parse_expr_op*   _ops,
        re_std::uint32_t      _op_count,
        ::dj_parse_expr_role* _roles,
        re_std::uint32_t      _role_count
    )
        : m_good(true)
    {
        d_parse_expr_signature_init(&m_signature, _ops, _op_count, _roles,
                                    _role_count);
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    DSignature(
        DSignature&& _other
    ) D_NOEXCEPT
        : m_signature(_other.m_signature),
          m_good(_other.m_good)
    {
        d_parse_expr_signature_init(&_other.m_signature, NULL, 0u, NULL, 0u);
    }

    DSignature&
    operator=(
        DSignature&& _other
    ) D_NOEXCEPT
    {
        if (this != &_other)
        {
            d_parse_expr_signature_release(&m_signature);
            m_signature = _other.m_signature;
            m_good      = _other.m_good;
            d_parse_expr_signature_init(&_other.m_signature, NULL, 0u, NULL,
                                        0u);
        }

        return *this;
    }

    DSignature(const DSignature&)            = delete;
    DSignature& operator=(const DSignature&) = delete;
#endif  // C++11

    ~DSignature()
    {
        d_parse_expr_signature_release(&m_signature);
    }

    // operators ------------------------------------------------------------

    // a prefix operator, right-associative
    DSignature&
    prefix(
        re_std::uint32_t _id,
        re_std::int32_t  _precedence,
        const char*      _spelling,
        re_std::uint16_t _flags = 0u
    )
    {
        return note(d_parse_expr_define_op(&m_signature, _id,
                                           D_PARSE_EXPR_FORM_PREFIX,
                                           _precedence,
                                           D_PARSE_EXPR_ASSOC_RIGHT, _flags,
                                           _spelling));
    }

    // an infix operator: D_PARSE_EXPR_ASSOC_LEFT, _RIGHT or _NONE
    DSignature&
    infix(
        re_std::uint32_t _id,
        re_std::int32_t  _precedence,
        re_std::uint8_t  _assoc,
        const char*      _spelling,
        re_std::uint16_t _flags = 0u
    )
    {
        return note(d_parse_expr_define_op(&m_signature, _id,
                                           D_PARSE_EXPR_FORM_INFIX,
                                           _precedence, _assoc, _flags,
                                           _spelling));
    }

    // a postfix operator, left-associative
    DSignature&
    postfix(
        re_std::uint32_t _id,
        re_std::int32_t  _precedence,
        const char*      _spelling,
        re_std::uint16_t _flags = 0u
    )
    {
        return note(d_parse_expr_define_op(&m_signature, _id,
                                           D_PARSE_EXPR_FORM_POSTFIX,
                                           _precedence,
                                           D_PARSE_EXPR_ASSOC_LEFT, _flags,
                                           _spelling));
    }

    // a chain, its links joined by the infix operator `_join`
    DSignature&
    chain(
        re_std::uint32_t _id,
        re_std::int32_t  _precedence,
        re_std::uint32_t _join,
        const char*      _spelling
    )
    {
        return note(d_parse_expr_define_chain(&m_signature, _id, _precedence,
                                              _join, _spelling));
    }

    DSignature&
    mixfix(
        re_std::uint32_t                _id,
        re_std::int32_t                 _precedence,
        const ::dj_parse_expr_mixfix&   _mixfix,
        re_std::uint8_t                 _assoc = 0u,
        re_std::uint16_t                _flags = 0u
    )
    {
        return note(d_parse_expr_define_mixfix(&m_signature, _id, _precedence,
                                               _assoc, _flags, &_mixfix));
    }

    // a whole operator record (signature.h 2.1.2)
    DSignature&
    op(
        const ::dj_parse_expr_op& _op
    )
    {
        return note(d_parse_expr_define(&m_signature, &_op));
    }

    // the juxtaposition operator, and the language's say on where it applies
    DSignature&
    juxtapose(
        re_std::uint32_t              _id,
        ::dj_parse_expr_juxtapose_fn  _hook = NULL,
        void*                         _ctx  = NULL
    )
    {
        return note(d_parse_expr_juxtapose(&m_signature, _id, _hook, _ctx));
    }

    // roles ----------------------------------------------------------------

    // a token class read as an atom in operand position
    DSignature&
    atom(
        re_std::uint32_t _class,
        re_std::uint16_t _flags = 0u
    )
    {
        return note(d_parse_expr_define_atom(&m_signature, _class, _flags));
    }

    // a token class beginning an operator in a position
    // (D_PARSE_EXPR_OPERAND or _OPERATOR)
    DSignature&
    token(
        re_std::uint32_t _class,
        re_std::uint8_t  _position,
        re_std::uint32_t _op,
        re_std::uint16_t _flags = 0u
    )
    {
        return note(d_parse_expr_define_token(&m_signature, _class,
                                              _position, _op, _flags));
    }

    // a custom role: a hook the parser calls for the class in the position
    DSignature&
    custom(
        re_std::uint32_t          _class,
        re_std::uint8_t           _position,
        ::dj_parse_expr_custom_fn _hook,
        void*                     _ctx   = NULL,
        re_std::uint16_t          _flags = 0u,
        re_std::uint32_t          _op    = D_PARSE_EXPR_OP_NONE,
        re_std::uint32_t          _modes = 0u
    )
    {
        ::dj_parse_expr_role role;

        std::memset(&role, 0, sizeof(role));
        role.cls        = _class;
        role.position   = _position;
        role.kind       = D_PARSE_EXPR_ROLE_CUSTOM;
        role.flags      = _flags;
        role.op         = _op;
        role.modes      = _modes;
        role.custom     = _hook;
        role.custom_ctx = _ctx;

        return note(d_parse_expr_define_role(&m_signature, &role));
    }

    // a whole role record (signature.h 2.2.2)
    DSignature&
    role(
        const ::dj_parse_expr_role& _role
    )
    {
        return note(d_parse_expr_define_role(&m_signature, &_role));
    }

    // sealing and reading --------------------------------------------------

    // checks and indexes the signature, reporting into a sink where given
    bool
    seal(
        ::dj_parse_diag_sink* _sink = NULL
    )
    {
        return note(d_parse_expr_signature_seal(&m_signature, _sink)).m_good;
    }

    // false once a builder or the seal has failed
    bool
    good() const
    {
        return m_good;
    }

    bool
    sealed() const
    {
        return ( (m_signature.flags & D_PARSE_EXPR_SIGNATURE_SEALED) != 0u );
    }

    // an operator, by id, once sealed; NULL otherwise
    const ::dj_parse_expr_op*
    find(
        re_std::uint32_t _id
    ) const
    {
        return d_parse_expr_find_op(&m_signature, _id);
    }

    ::dj_parse_expr_signature*
    get()
    {
        return &m_signature;
    }

    const ::dj_parse_expr_signature*
    get() const
    {
        return &m_signature;
    }

private:
#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    DSignature(const DSignature&);
    DSignature& operator=(const DSignature&);
#endif  // below C++11

    // keeps a builder's failure, so that good() answers for the whole chain
    DSignature&
    note(
        bool _done
    )
    {
        m_good = ( (m_good) && (_done) );

        return *this;
    }

    ::dj_parse_expr_signature m_signature;
    bool                      m_good;
};


NS_END  // expr
NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_EXPR_SIGNATURE_HPP
