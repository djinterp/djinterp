/******************************************************************************
* djinterp [parse]                                          scanner/scanner.hpp
*
* Scanner CRTP base + scan status/diagnostic + traits + concepts merged.
*   A scanner is the producer side of the parsing pipeline: it takes
* a unit of work (a file path, an in-memory buffer, a token stream)
* and emits items (lexemes, records, characters, tokens) that the
* parser carrier P A in parser/parser.hpp consumes.  Per ch-parsing
* the scanner sits upstream of Σ*; its output is what populates the
* parse_state the parser threads.
*
*   STRUCTURE.  scanner_base<_Derived, _Input> is a CRTP base
* delegating do_scan_file to the derived implementation.  Each unit
* of work is one input_type value (commonly std::string for paths,
* but any addressable handle works).  Items are accumulated in the
* derived's storage and exposed via results(); diagnostics about
* failed reads land in the base's per-session diagnostic list.
*
*   The content axis (what to do with a successfully-opened unit's
* bytes) and the discovery axis (where the units come from) layer on
* top of this base — text_scanner.hpp gives one content layer
* (read-as-text, hand to do_scan_text); file_scanner.hpp adds
* directory walking; the two are deliberately orthogonal.
*
*   ERROR BRIDGING.  The scan_diagnostic produced by emit_file_failed
* carries a parse_error from parse.hpp rather than reinventing one,
* so a parse failure occurring during scanning flows end-to-end as a
* single error type.  This matches the consolidation note in
* parse.hpp's preamble.
*
*   This header consolidates what were three separate files —
* scanner.hpp (the base), scanner_traits.hpp (the SFINAE surface),
* and scanner_concepts.hpp (the C++20 face) — into one primary
* module.
*
* CONTENTS
*   I.    scan_status                       outcome classifier + codes
*   II.   scan_diagnostic                   per-failure descriptor
*   III.  scanner_stats                     per-session aggregates
*   IV.   scanner_base<_Derived, _Input>    CRTP base
*   V.    has_do_scan_file_method /         method-shape detectors
*         has_do_reset_method   /
*         has_scan_file_method  /
*         has_scan_directory_method /
*         has_results_method
*   VI.   is_scanner / is_file_scanner      identity traits
*   VII.  scanners_share_input /            compatibility traits
*         scanners_share_items
*   VIII. SFINAE-safe extractors
*           scanner_input_type /
*           scanner_item_type  /
*           scanner_result_type
*   IX.   C++20 concepts mirroring the traits
*
* path:      /inc/djinterp/parse/scanner/scanner.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_PARSE_SCANNER_
#define DJINTERP_PARSE_SCANNER_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/meta/member_traits.hpp"
#include "../parse.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  I.   scan_status
// ================================================================

// scan_status
//   typedef: classifies the outcome of a scan operation.  Distinct
// from parse_status — scanners report read- and discovery-side
// outcomes, parsers report grammar-side outcomes — even though both
// share the integral underpinning.
typedef std::int32_t scan_status;

// DScanStatus*
//   constants: standard scan status codes.  Derived scanners may
// define additional codes above DScanStatusUserBase.
D_CONSTEXPR scan_status DScanStatusSuccess        =  0;
D_CONSTEXPR scan_status DScanStatusFailure        =  1;
D_CONSTEXPR scan_status DScanStatusReadFailure    =  2;
D_CONSTEXPR scan_status DScanStatusOpenFailure    =  3;
D_CONSTEXPR scan_status DScanStatusEncodingError  =  4;
D_CONSTEXPR scan_status DScanStatusParseFailure   =  5;
D_CONSTEXPR scan_status DScanStatusUserBase       = 64;


// ================================================================
//  II.  scan_diagnostic
// ================================================================

// scan_diagnostic
//   struct: a per-failure descriptor carrying the input handle that
// triggered the failure, an outcome status, and a parse_error with
// the human-readable detail (offset, message).  The parse_error
// embedding gives end-to-end continuity with downstream parser
// failures, which return the same error type.
//
//   _Input is the scanner's input_type — usually std::string for
// path-driven scanners, but any addressable handle works.
template<typename _Input>
struct scan_diagnostic
{
    using input_type = _Input;

    input_type   input;
    scan_status  status;
    parse_error  error;

    scan_diagnostic()
        : input (),
          status(DScanStatusFailure),
          error ()
    {}

    scan_diagnostic(
        const input_type&   _input,
        scan_status         _status,
        const parse_error&  _error
    )
        : input (_input),
          status(_status),
          error (_error)
    {}
};


// ================================================================
//  III. scanner_stats
// ================================================================

// scanner_stats
//   struct: per-session aggregate counts.  Updated by the base as
// scan_file is invoked.  bytes_read is the canonical input volume
// accumulator and is incremented by content-axis layers like
// text_scanner.
struct scanner_stats
{
    std::size_t units_scanned;
    std::size_t units_succeeded;
    std::size_t units_failed;
    std::size_t bytes_read;
    std::size_t items_emitted;

    scanner_stats()
        : units_scanned  (0),
          units_succeeded(0),
          units_failed   (0),
          bytes_read     (0),
          items_emitted  (0)
    {}
};


// ================================================================
//  IV.  scanner_base
// ================================================================

// scanner_base
//   class: the CRTP base for every scanner.  The contract a derived
// scanner _Derived must satisfy is
//
//     using input_type   = ...           the unit of work (e.g. path)
//     using item_type    = ...           the per-scan element produced
//     using result_type  = ...           the per-session accumulation
//     std::size_t do_scan_file(const input_type&);
//     void        do_reset();
//     const result_type& results() const;
//
// — do_scan_file processes one unit, returning the number of items
// emitted; do_reset clears any per-session state; results() exposes
// the accumulated collection.  The base layers on top of these the
// session-wide statistics, diagnostic emission, and the convenience
// scan_files batch entry.
//
//   The header does NOT add directory traversal — that lives in
// file_scanner.hpp where the discovery axis can be composed with
// the content axis (e.g. text_scanner) without tangling them.
template<typename _Derived,
         typename _Input>
class scanner_base
{
public:
    using input_type      = _Input;
    using diagnostic_type = scan_diagnostic<_Input>;

protected:
    scanner_base()
        : m_stats      (),
          m_diagnostics()
    {}

    ~scanner_base()
    {}

public:
    // scan_file
    //   method: processes one input unit by delegating to the
    // derived's do_scan_file.  Returns the number of items the
    // derived reported, and updates the session statistics.
    std::size_t
    scan_file(
        const input_type& _input
    )
    {
        std::size_t produced;

        m_stats.units_scanned += 1;

        produced = static_cast<_Derived&>(*this)
                       .do_scan_file(_input);

        if (produced > 0)
        {
            m_stats.units_succeeded += 1;
            m_stats.items_emitted   += produced;
        }

        return produced;
    }

    // scan_files
    //   method: convenience batch over an iterable range of input
    // units.  Returns the total number of items emitted across the
    // batch.
    template<typename _Range>
    std::size_t
    scan_files(
        const _Range& _inputs
    )
    {
        std::size_t total = 0;

        for (typename _Range::const_iterator it = _inputs.begin();
             it != _inputs.end();
             ++it)
        {
            total += scan_file(*it);
        }

        return total;
    }

    // reset
    //   method: clears per-session state.  The base resets its
    // stats and diagnostics and forwards to the derived's
    // do_reset for any user-side state.
    void
    reset()
    {
        m_stats = scanner_stats();
        m_diagnostics.clear();

        static_cast<_Derived&>(*this).do_reset();

        return;
    }

    // stats
    //   accessor: the per-session aggregate counts.
    D_NODISCARD
    const scanner_stats&
    stats() const D_NOEXCEPT
    {
        return m_stats;
    }

    // diagnostics
    //   accessor: the per-failure diagnostic list.
    D_NODISCARD
    const std::vector<diagnostic_type>&
    diagnostics() const D_NOEXCEPT
    {
        return m_diagnostics;
    }

protected:
    // emit_file_failed
    //   method: records a per-input failure.  The diagnostic
    // carries a parse_error so a downstream parse failure can be
    // surfaced with full offset/message detail in the same shape.
    void
    emit_file_failed(
        const input_type&  _input,
        scan_status        _status,
        const std::string& _message
    )
    {
        m_diagnostics.push_back(
            diagnostic_type(
                _input,
                _status,
                parse_error(
                    DParseStatusFailure,
                    0,
                    _message)));

        m_stats.units_failed += 1;

        return;
    }

    // emit_file_failed (parse_error overload)
    //   method: convenience overload taking a full parse_error so
    // a downstream parse failure can flow through unchanged.
    void
    emit_file_failed(
        const input_type&  _input,
        scan_status        _status,
        const parse_error& _error
    )
    {
        m_diagnostics.push_back(
            diagnostic_type(_input, _status, _error));

        m_stats.units_failed += 1;

        return;
    }

    scanner_stats                m_stats;
    std::vector<diagnostic_type> m_diagnostics;
};


// ================================================================
//  V.   method-shape detectors
// ================================================================

NS_INTERNAL

    // has_do_scan_file_method_helper
    //   trait: detects a `do_scan_file(const input_type&)` member.
    template<typename _T,
             typename = void>
    struct has_do_scan_file_method_helper : std::false_type
    {};

    template<typename _T>
    struct has_do_scan_file_method_helper<_T,
        void_t<decltype(
            std::declval<_T&>().do_scan_file(
                std::declval<
                    const typename clean_t<_T>::input_type&>()))>
    > : std::true_type
    {};

    // has_do_reset_method_helper
    //   trait: detects a `do_reset()` member.
    template<typename _T,
             typename = void>
    struct has_do_reset_method_helper : std::false_type
    {};

    template<typename _T>
    struct has_do_reset_method_helper<_T,
        void_t<decltype(std::declval<_T&>().do_reset())>
    > : std::true_type
    {};

    // has_scan_file_method_helper
    //   trait: detects a `scan_file(const input_type&)` member.
    template<typename _T,
             typename = void>
    struct has_scan_file_method_helper : std::false_type
    {};

    template<typename _T>
    struct has_scan_file_method_helper<_T,
        void_t<decltype(
            std::declval<_T&>().scan_file(
                std::declval<
                    const typename clean_t<_T>::input_type&>()))>
    > : std::true_type
    {};

    // has_scan_directory_method_helper
    //   trait: detects a `scan_directory(const std::string&)`
    // member (typically added by file_scanner.hpp).
    template<typename _T,
             typename = void>
    struct has_scan_directory_method_helper : std::false_type
    {};

    template<typename _T>
    struct has_scan_directory_method_helper<_T,
        void_t<decltype(
            std::declval<_T&>().scan_directory(
                std::declval<const std::string&>()))>
    > : std::true_type
    {};

    // has_results_method_helper
    //   trait: detects a `results()` member.
    template<typename _T,
             typename = void>
    struct has_results_method_helper : std::false_type
    {};

    template<typename _T>
    struct has_results_method_helper<_T,
        void_t<decltype(std::declval<const _T&>().results())>
    > : std::true_type
    {};

NS_END  // internal

// has_do_scan_file_method
template<typename _T>
struct has_do_scan_file_method
    : internal::has_do_scan_file_method_helper<_T>
{};

// has_do_reset_method
template<typename _T>
struct has_do_reset_method
    : internal::has_do_reset_method_helper<_T>
{};

// has_scan_file_method
template<typename _T>
struct has_scan_file_method
    : internal::has_scan_file_method_helper<_T>
{};

// has_scan_directory_method
template<typename _T>
struct has_scan_directory_method
    : internal::has_scan_directory_method_helper<_T>
{};

// has_results_method
template<typename _T>
struct has_results_method
    : internal::has_results_method_helper<_T>
{};


// ================================================================
//  VI.  is_scanner  /  is_file_scanner
// ================================================================

NS_INTERNAL

    // is_scanner_helper
    //   trait: primary template (failure case).
    template<typename _T,
             typename = void>
    struct is_scanner_helper : std::false_type
    {};

    // is_scanner_helper (success case)
    //   trait: succeeds when _T exposes input_type, item_type,
    // result_type, and a callable do_scan_file taking
    // input_type& and returning a size.
    template<typename _T>
    struct is_scanner_helper<_T,
        void_t<
            typename clean_t<_T>::input_type,
            typename clean_t<_T>::item_type,
            typename clean_t<_T>::result_type,
            decltype(
                std::declval<_T&>().do_scan_file(
                    std::declval<
                        const typename clean_t<_T>::input_type&>()))>
    > : std::true_type
    {};

    // is_file_scanner_helper
    //   trait: primary template (failure case).
    template<typename _T,
             bool     _IsScanner = is_scanner_helper<_T>::value,
             typename             = void>
    struct is_file_scanner_helper : std::false_type
    {};

    // is_file_scanner_helper (success case)
    //   trait: a scanner whose input_type is std::string.
    template<typename _T>
    struct is_file_scanner_helper<_T,
        true,
        typename std::enable_if<
            std::is_same<typename clean_t<_T>::input_type,
                         std::string>::value>::type
    > : std::true_type
    {};

NS_END  // internal

// is_scanner
//   trait: full structural check for scanner conformance.
template<typename _T>
struct is_scanner : internal::is_scanner_helper<_T>
{};

// is_file_scanner
//   trait: a structurally conforming scanner whose input_type is
// std::string — the path-driven scanner case.
template<typename _T>
struct is_file_scanner : internal::is_file_scanner_helper<_T>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    static constexpr bool is_scanner_v = is_scanner<_T>::value;

    template<typename _T>
    static constexpr bool is_file_scanner_v =
        is_file_scanner<_T>::value;
#endif


// ================================================================
//  VII. compatibility traits
// ================================================================

NS_INTERNAL

    // scanners_share_input_helper
    template<typename _A,
             typename _B,
             bool     _BothScanners = ( is_scanner<_A>::value &&
                                        is_scanner<_B>::value ),
             typename = void>
    struct scanners_share_input_helper : std::false_type
    {};

    template<typename _A,
             typename _B>
    struct scanners_share_input_helper<_A, _B,
        true,
        typename std::enable_if<
            std::is_same<
                typename clean_t<_A>::input_type,
                typename clean_t<_B>::input_type>::value>::type
    > : std::true_type
    {};

    // scanners_share_items_helper
    template<typename _A,
             typename _B,
             bool     _BothScanners = ( is_scanner<_A>::value &&
                                        is_scanner<_B>::value ),
             typename = void>
    struct scanners_share_items_helper : std::false_type
    {};

    template<typename _A,
             typename _B>
    struct scanners_share_items_helper<_A, _B,
        true,
        typename std::enable_if<
            std::is_same<
                typename clean_t<_A>::item_type,
                typename clean_t<_B>::item_type>::value>::type
    > : std::true_type
    {};

NS_END  // internal

// scanners_share_input
//   trait: two scanners share the same input_type and are therefore
// batch-compatible (can process the same input list).
template<typename _A,
         typename _B>
struct scanners_share_input
    : internal::scanners_share_input_helper<_A, _B>
{};

// scanners_share_items
//   trait: two scanners share the same item_type and are therefore
// merge-compatible (their output streams can be concatenated).
template<typename _A,
         typename _B>
struct scanners_share_items
    : internal::scanners_share_items_helper<_A, _B>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _A,
             typename _B>
    static constexpr bool scanners_share_input_v =
        scanners_share_input<_A, _B>::value;

    template<typename _A,
             typename _B>
    static constexpr bool scanners_share_items_v =
        scanners_share_items<_A, _B>::value;
#endif


// ================================================================
//  VIII. SFINAE-safe extractors
// ================================================================

// scanner_input_type / scanner_input_type_t
//   trait/type: SFINAE-safe extraction of a scanner's input_type;
// yields `void` when absent.
D_DEFINE_MEMBER_TYPE_OR(scanner_input_type, input_type, void)

// scanner_item_type / scanner_item_type_t
//   trait/type: SFINAE-safe extraction of a scanner's item_type.
D_DEFINE_MEMBER_TYPE_OR(scanner_item_type, item_type, void)

// scanner_result_type / scanner_result_type_t
//   trait/type: SFINAE-safe extraction of a scanner's result_type.
D_DEFINE_MEMBER_TYPE_OR(scanner_result_type, result_type, void)


// ================================================================
//  IX.  C++20 concepts
// ================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // scanner_surface
    //   concept: a type exposing input_type, item_type, and
    // result_type.
    template<typename _T>
    concept scanner_surface =
        ( has_input_type<_T>::value  &&
          has_item_type<_T>::value   &&
          has_result_type<_T>::value );

    // scanner_concept
    //   concept: structurally conforming scanner.
    template<typename _T>
    concept scanner_concept = is_scanner<_T>::value;

    // file_scanner_concept
    //   concept: a scanner whose input_type is std::string.
    template<typename _T>
    concept file_scanner_concept = is_file_scanner<_T>::value;

    // stateful_scanner_concept
    //   concept: a scanner with reset and results access.
    template<typename _T>
    concept stateful_scanner_concept =
        ( scanner_concept<_T>           &&
          has_do_reset_method<_T>::value &&
          has_results_method<_T>::value );

    // directory_scanner_concept
    //   concept: a scanner supporting directory traversal.
    template<typename _T>
    concept directory_scanner_concept =
        ( scanner_concept<_T> &&
          has_scan_directory_method<_T>::value );

    // scanners_batch_compatible
    //   concept: a scanner pair sharing input_type.
    template<typename _A,
             typename _B>
    concept scanners_batch_compatible =
        scanners_share_input<_A, _B>::value;

    // scanners_merge_compatible
    //   concept: a scanner pair sharing item_type.
    template<typename _A,
             typename _B>
    concept scanners_merge_compatible =
        scanners_share_items<_A, _B>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_SCANNER_
