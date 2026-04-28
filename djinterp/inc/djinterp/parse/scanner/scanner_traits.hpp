/******************************************************************************
* djinterp [scan]                                          scanner_traits.hpp
*
* Scanner SFINAE detection traits:
*   This header provides a suite of compile-time structural traits for
* detecting whether a type conforms to the scanner interface defined by
* the djinterp scanning framework.  Detection is purely structural —
* no tagging, no base-class checks, no RTTI.
*
* Traits provided:
*   - has_input_type<T>             does T expose `input_type`?
*   - has_item_type<T>              does T expose `item_type`?
*   - has_result_type<T>            does T expose `result_type`?
*   - has_do_scan_file_method<T>    does T have a callable `do_scan_file`?
*   - has_do_reset_method<T>        does T have a callable `do_reset`?
*   - has_scan_file_method<T>       does T expose `scan_file`?
*   - has_scan_directory_method<T>  does T expose `scan_directory`?
*   - has_results_method<T>         does T expose `results`?
*   - is_scanner<T>                 full structural scanner check
*   - is_file_scanner<T>            structural check for path-based scanners
*   - scanners_share_input<A,B>     do two scanners share input_type?
*   - scanners_share_items<A,B>     do two scanners share item_type?
*   - scanner_input_type<T>         extracts input_type (SFINAE-safe)
*   - scanner_item_type<T>          extracts item_type (SFINAE-safe)
*   - scanner_result_type<T>        extracts result_type (SFINAE-safe)
*
*
* path:      /inc/cpp/scan/scanner_traits.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.17
******************************************************************************/

#ifndef DJINTERP_SCANNER_TRAITS_
#define DJINTERP_SCANNER_TRAITS_ 1

#include <cstddef>
#include <string>
#include <type_traits>
#include "../core/djinterp.hpp"


// D_KEYWORD_SCAN
//   keyword: resolves to `scan`.
// Used to specify that a unit of code pertains to the scanning
// subsystem.  Guarded so either scanner.hpp or scanner_traits.hpp
// may be included first.
#ifndef D_KEYWORD_SCAN
    #define D_KEYWORD_SCAN              scan
#endif

// NS_SCAN
//   namespace: the scan subsystem namespace.
#ifndef NS_SCAN
    #define NS_SCAN                     D_NAMESPACE(D_KEYWORD_SCAN)
#endif


NS_DJINTERP
NS_SCAN
NS_TRAITS


// ================================================================
//  has_input_type
// ================================================================

NS_INTERNAL

    // has_input_type_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_input_type_helper : std::false_type
    {};

    // has_input_type_helper (success case)
    //   trait: succeeds when _Type::input_type is well-formed.
    template<typename _Type>
    struct has_input_type_helper<_Type,
        void_t<typename _Type::input_type>
    > : std::true_type
    {};

NS_END  // internal

// has_input_type
//   trait: detects whether _Type exposes a nested `input_type`
// typedef.
template<typename _Type>
struct has_input_type : internal::has_input_type_helper<_Type>
{};

// has_input_type_v
//   value: convenience alias for has_input_type<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_input_type_v = has_input_type<_Type>::value;
#endif


// ================================================================
//  has_item_type
// ================================================================

NS_INTERNAL

    // has_item_type_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_item_type_helper : std::false_type
    {};

    // has_item_type_helper (success case)
    //   trait: succeeds when _Type::item_type is well-formed.
    template<typename _Type>
    struct has_item_type_helper<_Type,
        void_t<typename _Type::item_type>
    > : std::true_type
    {};

NS_END  // internal

// has_item_type
//   trait: detects whether _Type exposes a nested `item_type`
// typedef.
template<typename _Type>
struct has_item_type : internal::has_item_type_helper<_Type>
{};

// has_item_type_v
//   value: convenience alias for has_item_type<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_item_type_v = has_item_type<_Type>::value;
#endif


// ================================================================
//  has_result_type
// ================================================================

NS_INTERNAL

    // has_result_type_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_result_type_helper : std::false_type
    {};

    // has_result_type_helper (success case)
    //   trait: succeeds when _Type::result_type is well-formed.
    template<typename _Type>
    struct has_result_type_helper<_Type,
        void_t<typename _Type::result_type>
    > : std::true_type
    {};

NS_END  // internal

// has_result_type
//   trait: detects whether _Type exposes a nested `result_type`
// typedef.
template<typename _Type>
struct has_result_type : internal::has_result_type_helper<_Type>
{};

// has_result_type_v
//   value: convenience alias for has_result_type<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_result_type_v = has_result_type<_Type>::value;
#endif


// ================================================================
//  has_do_scan_file_method
// ================================================================

NS_INTERNAL

    // has_do_scan_file_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_do_scan_file_method_helper : std::false_type
    {};

    // has_do_scan_file_method_helper (success case)
    //   trait: succeeds when _Type has a callable `do_scan_file`
    // that accepts a `const _Type::input_type&`.
    template<typename _Type>
    struct has_do_scan_file_method_helper<
        _Type,
        void_t<decltype(
            std::declval<_Type>().do_scan_file(
                std::declval<
                    const typename _Type::input_type&
                >()
            )
        )>
    > : std::true_type
    {};

NS_END  // internal

// has_do_scan_file_method
//   trait: detects whether _Type has a `do_scan_file` member
// function accepting `const input_type&`.
template<typename _Type>
struct has_do_scan_file_method
    : internal::has_do_scan_file_method_helper<_Type>
{};

// has_do_scan_file_method_v
//   value: convenience alias for has_do_scan_file_method<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_do_scan_file_method_v =
        has_do_scan_file_method<_Type>::value;
#endif


// ================================================================
//  has_do_reset_method
// ================================================================

NS_INTERNAL

    // has_do_reset_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_do_reset_method_helper : std::false_type
    {};

    // has_do_reset_method_helper (success case)
    //   trait: succeeds when _Type has a no-argument callable
    // `do_reset`.
    template<typename _Type>
    struct has_do_reset_method_helper<
        _Type,
        void_t<decltype(std::declval<_Type>().do_reset())>
    > : std::true_type
    {};

NS_END  // internal

// has_do_reset_method
//   trait: detects whether _Type has a `do_reset` member
// function taking no arguments.
template<typename _Type>
struct has_do_reset_method
    : internal::has_do_reset_method_helper<_Type>
{};

// has_do_reset_method_v
//   value: convenience alias for has_do_reset_method<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_do_reset_method_v =
        has_do_reset_method<_Type>::value;
#endif


// ================================================================
//  has_scan_file_method
// ================================================================

NS_INTERNAL

    // has_scan_file_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_scan_file_method_helper : std::false_type
    {};

    // has_scan_file_method_helper (success case)
    //   trait: succeeds when _Type has a callable `scan_file`
    // accepting `const _Type::input_type&`.
    template<typename _Type>
    struct has_scan_file_method_helper<
        _Type,
        void_t<decltype(
            std::declval<_Type>().scan_file(
                std::declval<
                    const typename _Type::input_type&
                >()
            )
        )>
    > : std::true_type
    {};

NS_END  // internal

// has_scan_file_method
//   trait: detects whether _Type has a public `scan_file`
// member function (typically inherited from scanner_base).
template<typename _Type>
struct has_scan_file_method
    : internal::has_scan_file_method_helper<_Type>
{};

// has_scan_file_method_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_scan_file_method_v =
        has_scan_file_method<_Type>::value;
#endif


// ================================================================
//  has_scan_directory_method
// ================================================================

NS_INTERNAL

    // has_scan_directory_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_scan_directory_method_helper : std::false_type
    {};

    // has_scan_directory_method_helper (success case)
    //   trait: succeeds when _Type has a callable `scan_directory`
    // accepting a `const std::string&`.
    template<typename _Type>
    struct has_scan_directory_method_helper<
        _Type,
        void_t<decltype(
            std::declval<_Type>().scan_directory(
                std::declval<const std::string&>()
            )
        )>
    > : std::true_type
    {};

NS_END  // internal

// has_scan_directory_method
//   trait: detects whether _Type has a public `scan_directory`
// member function.
template<typename _Type>
struct has_scan_directory_method
    : internal::has_scan_directory_method_helper<_Type>
{};

// has_scan_directory_method_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_scan_directory_method_v =
        has_scan_directory_method<_Type>::value;
#endif


// ================================================================
//  has_results_method
// ================================================================

NS_INTERNAL

    // has_results_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_results_method_helper : std::false_type
    {};

    // has_results_method_helper (success case)
    //   trait: succeeds when _Type has a no-argument callable
    // `results` whose return type is convertible to a reference
    // to `result_type`.
    template<typename _Type>
    struct has_results_method_helper<
        _Type,
        void_t<decltype(std::declval<const _Type&>().results())>
    > : std::true_type
    {};

NS_END  // internal

// has_results_method
//   trait: detects whether _Type has a `results()` accessor.
template<typename _Type>
struct has_results_method
    : internal::has_results_method_helper<_Type>
{};

// has_results_method_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_results_method_v =
        has_results_method<_Type>::value;
#endif


// ================================================================
//  is_scanner
// ================================================================

NS_INTERNAL

    // is_scanner_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct is_scanner_helper : std::false_type
    {};

    // is_scanner_helper (success case)
    //   trait: succeeds when _Type structurally satisfies the
    // scanner contract: it exposes input_type, item_type,
    // result_type, and callable do_scan_file / do_reset whose
    // do_scan_file return type is convertible to std::size_t.
    template<typename _Type>
    struct is_scanner_helper<_Type,
        void_t<
            typename _Type::input_type,
            typename _Type::item_type,
            typename _Type::result_type,
            decltype(
                std::declval<_Type>().do_scan_file(
                    std::declval<
                        const typename _Type::input_type&
                    >()
                )
            ),
            decltype(std::declval<_Type>().do_reset())
        >>
    {
    private:
        using scan_return = decltype(
            std::declval<_Type>().do_scan_file(
                std::declval<
                    const typename _Type::input_type&
                >()
            )
        );

    public:
        static constexpr bool value =
            std::is_convertible<scan_return, std::size_t>::value;
    };

NS_END  // internal

// is_scanner
//   trait: full structural check for scanner conformance.
// Returns true when _Type exposes input_type, item_type,
// result_type, a do_scan_file(const input_type&) returning a
// size_t-convertible, and a do_reset().
template<typename _Type>
struct is_scanner
    : std::integral_constant<bool,
                             internal::is_scanner_helper<_Type>::value>
{};

// is_scanner_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_scanner_v = is_scanner<_Type>::value;
#endif


// ================================================================
//  is_file_scanner
// ================================================================

NS_INTERNAL

    // is_file_scanner_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             bool     _IsScanner = is_scanner<_Type>::value,
             typename = void>
    struct is_file_scanner_helper : std::false_type
    {};

    // is_file_scanner_helper (success case)
    //   trait: succeeds when _Type is a scanner whose input_type
    // is `std::string` (a file path).
    template<typename _Type>
    struct is_file_scanner_helper<_Type,
        true,
        typename std::enable_if<
            std::is_same<
                typename _Type::input_type,
                std::string
            >::value
        >::type
    > : std::true_type
    {};

NS_END  // internal

// is_file_scanner
//   trait: detects whether _Type is a structurally conforming
// scanner whose input_type is `std::string`, i.e. a file-path
// scanner.
template<typename _Type>
struct is_file_scanner : internal::is_file_scanner_helper<_Type>
{};

// is_file_scanner_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_file_scanner_v = is_file_scanner<_Type>::value;
#endif


// ================================================================
//  scanners_share_input
// ================================================================

NS_INTERNAL

    // scanners_share_input_helper
    //   trait: primary template (failure case).
    template<typename _A,
             typename _B,
             bool     _BothScanners = ( is_scanner<_A>::value &&
                                        is_scanner<_B>::value ),
             typename                = void>
    struct scanners_share_input_helper : std::false_type
    {};

    // scanners_share_input_helper (success case)
    //   trait: succeeds when both types are scanners and share
    // the same input_type.
    template<typename _A,
             typename _B>
    struct scanners_share_input_helper<_A, _B,
        true,
        typename std::enable_if<
            std::is_same<
                typename _A::input_type,
                typename _B::input_type
            >::value
        >::type
    > : std::true_type
    {};

NS_END  // internal

// scanners_share_input
//   trait: detects whether two scanner types share the same
// input_type and are therefore batch-composable over the same
// inputs.
template<typename _A,
         typename _B>
struct scanners_share_input
    : internal::scanners_share_input_helper<_A, _B>
{};

// scanners_share_input_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _A,
             typename _B>
    constexpr bool scanners_share_input_v =
        scanners_share_input<_A, _B>::value;
#endif


// ================================================================
//  scanners_share_items
// ================================================================

NS_INTERNAL

    // scanners_share_items_helper
    //   trait: primary template (failure case).
    template<typename _A,
             typename _B,
             bool     _BothScanners = ( is_scanner<_A>::value &&
                                        is_scanner<_B>::value ),
             typename                = void>
    struct scanners_share_items_helper : std::false_type
    {};

    // scanners_share_items_helper (success case)
    //   trait: succeeds when both types are scanners and share
    // the same item_type.
    template<typename _A,
             typename _B>
    struct scanners_share_items_helper<_A, _B,
        true,
        typename std::enable_if<
            std::is_same<
                typename _A::item_type,
                typename _B::item_type
            >::value
        >::type
    > : std::true_type
    {};

NS_END  // internal

// scanners_share_items
//   trait: detects whether two scanner types produce the same
// item_type and are therefore merge-compatible on the item
// stream.
template<typename _A,
         typename _B>
struct scanners_share_items
    : internal::scanners_share_items_helper<_A, _B>
{};

// scanners_share_items_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _A,
             typename _B>
    constexpr bool scanners_share_items_v =
        scanners_share_items<_A, _B>::value;
#endif


// ================================================================
//  scanner_input_type  /  scanner_item_type  /  scanner_result_type
// ================================================================
// SFINAE-safe type extractors.  Produce `void` when the queried
// type does not expose the expected member typedef.

NS_INTERNAL

    // scanner_input_type_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct scanner_input_type_helper
    {
        using type = void;
    };

    // scanner_input_type_helper (success case)
    //   trait: extracts _Type::input_type when available.
    template<typename _Type>
    struct scanner_input_type_helper<
        _Type,
        void_t<typename _Type::input_type>
    >
    {
        using type = typename _Type::input_type;
    };

    // scanner_item_type_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct scanner_item_type_helper
    {
        using type = void;
    };

    // scanner_item_type_helper (success case)
    //   trait: extracts _Type::item_type when available.
    template<typename _Type>
    struct scanner_item_type_helper<
        _Type,
        void_t<typename _Type::item_type>
    >
    {
        using type = typename _Type::item_type;
    };

    // scanner_result_type_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct scanner_result_type_helper
    {
        using type = void;
    };

    // scanner_result_type_helper (success case)
    //   trait: extracts _Type::result_type when available.
    template<typename _Type>
    struct scanner_result_type_helper<
        _Type,
        void_t<typename _Type::result_type>
    >
    {
        using type = typename _Type::result_type;
    };

NS_END  // internal

// scanner_input_type
//   trait: SFINAE-safe extraction of a scanner's input_type.
// Produces void if _Type does not expose input_type.
template<typename _Type>
struct scanner_input_type
    : internal::scanner_input_type_helper<_Type>
{};

// scanner_input_type_t
//   type: convenience alias for scanner_input_type<_Type>::type.
template<typename _Type>
using scanner_input_type_t =
    typename scanner_input_type<_Type>::type;

// scanner_item_type
//   trait: SFINAE-safe extraction of a scanner's item_type.
// Produces void if _Type does not expose item_type.
template<typename _Type>
struct scanner_item_type
    : internal::scanner_item_type_helper<_Type>
{};

// scanner_item_type_t
//   type: convenience alias for scanner_item_type<_Type>::type.
template<typename _Type>
using scanner_item_type_t =
    typename scanner_item_type<_Type>::type;

// scanner_result_type
//   trait: SFINAE-safe extraction of a scanner's result_type.
// Produces void if _Type does not expose result_type.
template<typename _Type>
struct scanner_result_type
    : internal::scanner_result_type_helper<_Type>
{};

// scanner_result_type_t
//   type: convenience alias for scanner_result_type<_Type>::type.
template<typename _Type>
using scanner_result_type_t =
    typename scanner_result_type<_Type>::type;


NS_END  // traits
NS_END  // scan
NS_END  // djinterp


#endif  // DJINTERP_SCANNER_TRAITS_
