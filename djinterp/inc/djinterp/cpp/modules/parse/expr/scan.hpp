/*******************************************************************************
* djinterp [cpp/parse]                                                  scan.hpp
*
* Expression core v2's C++ face (design section 13): the default scanner.
*   DScanner binds d_parse_expr_scan to a text and a sealed signature, and
* gives the C source parse() reads. DScanConfig is the configuration (scan.h
* 1.1.2), zeroed: every class 0 until set. The text, the signature and the
* configuration's words are the caller's, and must outlive the parse.
*   Compiled where the scanner is (D_CFG_PARSE_EXPR_SCAN).
*
*
* TABLE OF CONTENTS
* =================
* 1.  THE CONFIGURATION
*     -----------------
*     1.  DScanConfig
* 2.  THE SCANNER
*     -----------
*     1.  DScanner
*
*
* path:      /inc/djinterp/cpp/modules/parse/expr/scan.hpp
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_PARSE_EXPR_SCAN_HPP
#define DJINTERP_PARSE_EXPR_SCAN_HPP 1

// djinterp
#include "./term.hpp"                               // NS_EXPR
#include "./signature.hpp"                          // DSignature
#include "../../../../c/modules/parse/expr/scan.h"  // the C scanner
// std
#include <cstddef>  // std::size_t
#include <cstring>  // std::memset, std::strlen


#if (D_INTERNAL_PARSE_EXPR_SCAN == 1)


NS_DJINTERP
NS_PARSE
NS_EXPR


//==============================================================================
// 1.  THE CONFIGURATION
//==============================================================================


// 1.1    DScanConfig
//------------------------------------------------------------------------------
// DScanConfig
//   class: the classes the scanner gives what it reads, and the caller's
// further spellings (scan.h 1.1.2), zeroed.
class DScanConfig : public ::dj_parse_expr_scan_config
{
public:
    DScanConfig()
    {
        std::memset(static_cast< ::dj_parse_expr_scan_config* >(this), 0,
                    sizeof(::dj_parse_expr_scan_config));
    }
};

// a DScanConfig is the C configuration, and is passed as it
D_STATIC_ASSERT(sizeof(DScanConfig) == sizeof(::dj_parse_expr_scan_config),
                "DScanConfig must add nothing to dj_parse_expr_scan_config");


//==============================================================================
// 2.  THE SCANNER
//==============================================================================


// 2.1    DScanner
//------------------------------------------------------------------------------
// DScanner
//   class: a scan of one text, the C source over it.
class DScanner
{
public:
    DScanner()
        : m_bound(false)
    {
        std::memset(&m_scanner, 0, sizeof(m_scanner));
        std::memset(&m_source, 0, sizeof(m_source));
    }

    // binds the scan; false where an argument is wrong or the text too long
    bool
    bind(
        const DSignature&                    _signature,
        const ::dj_parse_expr_scan_config&   _config,
        const char*                          _text,
        std::size_t                          _length
    )
    {
        m_bound = d_parse_expr_scan(&m_source, &m_scanner, _signature.get(),
                                    &_config, _text, _length);

        return m_bound;
    }

    bool
    bind(
        const DSignature&                    _signature,
        const ::dj_parse_expr_scan_config&   _config,
        const char*                          _text
    )
    {
        return bind(_signature, _config, _text,
                    (_text) ? std::strlen(_text) : 0u);
    }

    bool
    bound() const
    {
        return m_bound;
    }

    // the C source, for parse(); valid while the scanner is
    ::dj_parse_expr_source&
    source()
    {
        return m_source;
    }

private:
    // the C source points into the scanner's state: neither is copied
    DScanner(const DScanner&);
    DScanner& operator=(const DScanner&);

    ::dj_parse_expr_scanner m_scanner;
    ::dj_parse_expr_source  m_source;
    bool                    m_bound;
};


NS_END  // expr
NS_END  // parse
NS_END  // djinterp


#endif  // D_INTERNAL_PARSE_EXPR_SCAN


#endif  // DJINTERP_PARSE_EXPR_SCAN_HPP
