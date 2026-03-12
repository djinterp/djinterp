/******************************************************************************
* djinterp [core]                                              cpp_named20.h
*
* C++20 named requirement concepts:
*   Extends the C++17 named requirement traits with first-class C++20
* concept definitions. Concepts provide superior constraint checking with
* cleaner syntax, better error messages, and subsumption support for
* overload resolution.
*
* ADDITIONS OVER C++17:
*   - Concept versions of all six core named requirements
*   - Concept versions of nothrow variants
*   - Concept versions of trivially-qualified variants
*   - Concept versions of compound requirement traits
*   - scalar_type concept
*   - trivially_copyable concept
*   - trivial_type concept        (deprecated in C++26)
*   - standard_layout_type concept
*   - pod_type concept            (deprecated in C++20)
*   - Range traits and concepts   (gated on __cpp_lib_ranges)
*   - RangeAdaptorObject traits
*   - RangeAdaptorClosureObject traits
*   - std::ranges:: concept wrappers (sized, borrowed, viewable, etc.)
*   - BasicFormatter / Formatter traits (gated on __cpp_lib_format)
*   - Concurrency concepts (BasicLockable through SharedTimedMutex)
*
* CONCEPT NAMING CONVENTION:
*   Concepts use the same snake_case names as the struct traits without the
*   `is_` prefix, in a dedicated `concepts` sub-namespace:
*     djinterp::traits::concepts::default_constructible<_T>
*     djinterp::traits::concepts::move_constructible<_T>
*     djinterp::traits::concepts::value_type<_T>
*   (etc.)
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_CONCEPTS             - concept availability
*   D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES    - move trait availability
*
* path:      \inc\cpp_named20.h
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.27
******************************************************************************/

#ifndef DJINTERP_CPP_NAMED20_
#define DJINTERP_CPP_NAMED20_ 1

// require env.h to be included first
#ifndef DJINTERP_ENVIRONMENT_
    #error "cpp_named20.h requires env.h to be included first"
#endif

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named20.h can only be used in C++ compilation mode"
#endif

// include C++17 layer
#include "cpp_named17.h"

// C++20 additions are active for C++20 and all later standards
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

#include <type_traits>
#include <concepts>
#include <iterator>
#include <random>


NS_DJINTERP
NS_TRAITS

// =========================================================================
// I.   CORE NAMED REQUIREMENT CONCEPTS (C++20)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // default_constructible
    //   concept: constrains _T to types that are default-constructible.
    template<typename _T>
    concept default_constructible =
        std::is_default_constructible_v<_T>;

    // copy_constructible
    //   concept: constrains _T to types that are copy-constructible
    // from const _T&.
    template<typename _T>
    concept copy_constructible =
        std::is_copy_constructible_v<_T>;

    // copy_assignable
    //   concept: constrains _T to types that are copy-assignable
    // from const _T&.
    template<typename _T>
    concept copy_assignable =
        std::is_copy_assignable_v<_T>;

    // destructible
    //   concept: constrains _T to types with an accessible, non-deleted
    // destructor.
    template<typename _T>
    concept destructible =
        std::is_destructible_v<_T>;

    // move_constructible
    //   concept: constrains _T to types that are move-constructible
    // from _T&&.
    template<typename _T>
    concept move_constructible =
        std::is_move_constructible_v<_T>;

    // move_assignable
    //   concept: constrains _T to types that are move-assignable
    // from _T&&.
    template<typename _T>
    concept move_assignable =
        std::is_move_assignable_v<_T>;


    // =====================================================================
    // II.  NOTHROW CONCEPTS (C++20)
    // =====================================================================

    // nothrow_default_constructible
    //   concept: constrains _T to types that are default-constructible
    // without throwing exceptions.
    template<typename _T>
    concept nothrow_default_constructible =
        std::is_nothrow_default_constructible_v<_T>;

    // nothrow_copy_constructible
    //   concept: constrains _T to types that are copy-constructible
    // without throwing exceptions.
    template<typename _T>
    concept nothrow_copy_constructible =
        std::is_nothrow_copy_constructible_v<_T>;

    // nothrow_copy_assignable
    //   concept: constrains _T to types that are copy-assignable
    // without throwing exceptions.
    template<typename _T>
    concept nothrow_copy_assignable =
        std::is_nothrow_copy_assignable_v<_T>;

    // nothrow_destructible
    //   concept: constrains _T to types that are destructible without
    // throwing exceptions.
    template<typename _T>
    concept nothrow_destructible =
        std::is_nothrow_destructible_v<_T>;

    // nothrow_move_constructible
    //   concept: constrains _T to types that are move-constructible
    // without throwing exceptions.
    template<typename _T>
    concept nothrow_move_constructible =
        std::is_nothrow_move_constructible_v<_T>;

    // nothrow_move_assignable
    //   concept: constrains _T to types that are move-assignable
    // without throwing exceptions.
    template<typename _T>
    concept nothrow_move_assignable =
        std::is_nothrow_move_assignable_v<_T>;


    // =====================================================================
    // III. TRIVIALLY-QUALIFIED CONCEPTS (C++20)
    // =====================================================================

    // trivially_default_constructible
    //   concept: constrains _T to types that are trivially
    // default-constructible.
    template<typename _T>
    concept trivially_default_constructible =
        std::is_trivially_default_constructible_v<_T>;

    // trivially_copy_constructible
    //   concept: constrains _T to types that are trivially
    // copy-constructible.
    template<typename _T>
    concept trivially_copy_constructible =
        std::is_trivially_copy_constructible_v<_T>;

    // trivially_copy_assignable
    //   concept: constrains _T to types that are trivially
    // copy-assignable.
    template<typename _T>
    concept trivially_copy_assignable =
        std::is_trivially_copy_assignable_v<_T>;

    // trivially_destructible
    //   concept: constrains _T to types that are trivially destructible.
    template<typename _T>
    concept trivially_destructible =
        std::is_trivially_destructible_v<_T>;

    // trivially_move_constructible
    //   concept: constrains _T to types that are trivially
    // move-constructible.
    template<typename _T>
    concept trivially_move_constructible =
        std::is_trivially_move_constructible_v<_T>;

    // trivially_move_assignable
    //   concept: constrains _T to types that are trivially
    // move-assignable.
    template<typename _T>
    concept trivially_move_assignable =
        std::is_trivially_move_assignable_v<_T>;


    // =====================================================================
    // IV.  COMPOUND REQUIREMENT CONCEPTS (C++20)
    // =====================================================================

    // fully_copyable
    //   concept: constrains _T to types satisfying both
    // CopyConstructible and CopyAssignable.
    template<typename _T>
    concept fully_copyable =
        ( copy_constructible<_T> &&
          copy_assignable<_T> );

    // fully_movable
    //   concept: constrains _T to types satisfying both
    // MoveConstructible and MoveAssignable.
    template<typename _T>
    concept fully_movable =
        ( move_constructible<_T> &&
          move_assignable<_T> );

    // fully_constructible
    //   concept: constrains _T to types satisfying
    // DefaultConstructible, CopyConstructible, and MoveConstructible.
    template<typename _T>
    concept fully_constructible =
        ( default_constructible<_T> &&
          copy_constructible<_T>    &&
          move_constructible<_T> );

    // fully_assignable
    //   concept: constrains _T to types satisfying both
    // CopyAssignable and MoveAssignable.
    template<typename _T>
    concept fully_assignable =
        ( copy_assignable<_T> &&
          move_assignable<_T> );

    // value_type
    //   concept: constrains _T to types satisfying all six core named
    // requirements: DefaultConstructible, CopyConstructible,
    // MoveConstructible, CopyAssignable, MoveAssignable, and
    // Destructible.
    template<typename _T>
    concept value_type =
        ( default_constructible<_T> &&
          copy_constructible<_T>    &&
          move_constructible<_T>    &&
          copy_assignable<_T>       &&
          move_assignable<_T>       &&
          destructible<_T> );


    // =====================================================================
    // V.   TYPE CLASSIFICATION CONCEPTS (C++20)
    // =====================================================================

    // scalar_type
    //   concept: constrains _T to scalar types (arithmetic, enumeration,
    // pointer, pointer-to-member, or std::nullptr_t).
    template<typename _T>
    concept scalar_type =
        std::is_scalar_v<_T>;

    // trivially_copyable
    //   concept: constrains _T to types that are trivially copyable
    // (safe to copy via memcpy/memmove).
    template<typename _T>
    concept trivially_copyable =
        std::is_trivially_copyable_v<_T>;

    // trivial_type
    //   concept: constrains _T to trivial types (trivially
    // default-constructible and trivially copyable).
    // note: TrivialType named requirement is deprecated in C++26.
    template<typename _T>
    concept trivial_type =
        std::is_trivial_v<_T>;

    // standard_layout_type
    //   concept: constrains _T to types satisfying the
    // StandardLayoutType named requirement.
    template<typename _T>
    concept standard_layout_type =
        std::is_standard_layout_v<_T>;

    // pod_type
    //   concept: constrains _T to POD (Plain Old Data) types.
    // note: PODType named requirement is deprecated in C++20. Prefer
    // trivially_copyable with standard_layout_type for new code.
    template<typename _T>
    concept pod_type =
        ( trivial_type<_T> &&
          standard_layout_type<_T> );


    // =====================================================================
    // VI.  COMPARISON AND BOOLEAN CONCEPTS (C++20)
    // =====================================================================

    // boolean_testable
    //   concept: constrains _T to types that are contextually
    // convertible to bool.
    template<typename _T>
    concept boolean_testable =
        std::convertible_to<_T, bool>;

    // equality_comparable
    //   concept: constrains _T to types satisfying
    // EqualityComparable, using std::equality_comparable.
    template<typename _T>
    concept equality_comparable =
        std::equality_comparable<_T>;

    // less_than_comparable
    //   concept: constrains _T to types supporting operator< with a
    // bool-convertible result.
    template<typename _T>
    concept less_than_comparable = requires(const _T _a,
                                            const _T _b)
    {
        { _a < _b } -> boolean_testable;
    };

    // swappable
    //   concept: constrains _T to types that are swappable, using
    // std::swappable.
    template<typename _T>
    concept swappable =
        std::swappable<_T>;

    // value_swappable
    //   concept: constrains _T to iterator-like types whose
    // dereferenced values are swappable.
    template<typename _T>
    concept value_swappable = requires(_T _a, _T _b)
    {
        *_a;
        requires std::swappable<decltype(*_a)>;
    };

    // nullable_pointer
    //   concept: constrains _T to types satisfying the NullablePointer
    // named requirement.
    template<typename _T>
    concept nullable_pointer =
        ( default_constructible<_T>                      &&
          copy_constructible<_T>                         &&
          copy_assignable<_T>                            &&
          destructible<_T>                               &&
          equality_comparable<_T>                        &&
          std::constructible_from<_T, std::nullptr_t>    &&
          boolean_testable<_T> );

    // hashable
    //   concept: constrains _T to types for which std::hash<_T> is a
    // valid specialization.
    template<typename _T>
    concept hashable =
        is_hashable_v<_T>;

    // allocator
    //   concept: constrains _A to types satisfying the basic
    // structural Allocator requirements.
    template<typename _A>
    concept allocator =
        is_allocator_v<_A>;


    // =====================================================================
    // VII. CALLABLE AND PREDICATE CONCEPTS (C++20)
    // =====================================================================

    // callable
    //   concept: constrains _F to types invocable with _Args... using
    // std::invocable.
    template<typename _F,
             typename... _Args>
    concept callable =
        std::invocable<_F, _Args...>;

    // predicate
    //   concept: constrains _F to types satisfying Predicate for _Arg,
    // using std::predicate.
    template<typename _F,
             typename _Arg>
    concept predicate =
        std::predicate<_F, _Arg>;

    // binary_predicate
    //   concept: constrains _F to types satisfying BinaryPredicate
    // for _Arg1 and _Arg2.
    template<typename _F,
             typename _Arg1,
             typename _Arg2>
    concept binary_predicate =
        std::predicate<_F, _Arg1, _Arg2>;

    // compare
    //   concept: constrains _F to types satisfying Compare for _T
    // (BinaryPredicate on const _T&, const _T&, implying strict weak
    // ordering).
    // note: std::strict_weak_order provides the semantic contract;
    // this concept checks the structural requirement only.
    template<typename _F,
             typename _T>
    concept compare =
        binary_predicate<_F, const _T&, const _T&>;


    // =====================================================================
    // VIII. CONTAINER CONCEPTS (C++20)
    // =====================================================================

    // container
    //   concept: constrains _C to types satisfying the structural
    // Container named requirement.
    template<typename _C>
    concept container =
        is_container_v<_C>;

    // reversible_container
    //   concept: constrains _C to types satisfying
    // ReversibleContainer.
    template<typename _C>
    concept reversible_container =
        is_reversible_container_v<_C>;

    // allocator_aware_container
    //   concept: constrains _C to types satisfying
    // AllocatorAwareContainer.
    template<typename _C>
    concept allocator_aware_container =
        is_allocator_aware_container_v<_C>;

    // sequence_container
    //   concept: constrains _C to types satisfying the structural
    // SequenceContainer named requirement.
    template<typename _C>
    concept sequence_container =
        is_sequence_container_v<_C>;

    // contiguous_container
    //   concept: constrains _C to types satisfying
    // ContiguousContainer: a Container with contiguous data() storage.
    template<typename _C>
    concept contiguous_container =
        is_contiguous_container_v<_C>;

    // associative_container
    //   concept: constrains _C to types satisfying
    // AssociativeContainer.
    template<typename _C>
    concept associative_container =
        is_associative_container_v<_C>;

    // unordered_associative_container
    //   concept: constrains _C to types satisfying
    // UnorderedAssociativeContainer.
    template<typename _C>
    concept unordered_associative_container =
        is_unordered_associative_container_v<_C>;


    // =====================================================================
    // IX.  CONTAINER ELEMENT CONCEPTS (C++20)
    // =====================================================================

    // default_insertable
    //   concept: constrains _T to types that are DefaultInsertable
    // into a container using allocator _A.
    template<typename _T,
             typename _A = std::allocator<_T>>
    concept default_insertable =
        is_default_insertable_v<_T, _A>;

    // copy_insertable
    //   concept: constrains _T to types that are CopyInsertable into
    // a container using allocator _A.
    template<typename _T,
             typename _A = std::allocator<_T>>
    concept copy_insertable =
        is_copy_insertable_v<_T, _A>;

    // move_insertable
    //   concept: constrains _T to types that are MoveInsertable into
    // a container using allocator _A.
    template<typename _T,
             typename _A = std::allocator<_T>>
    concept move_insertable =
        is_move_insertable_v<_T, _A>;

    // emplace_constructible
    //   concept: constrains _T to types that are
    // EmplaceConstructible into a container using allocator _A from
    // _Args... .
    template<typename _T,
             typename _A,
             typename... _Args>
    concept emplace_constructible =
        is_emplace_constructible_v<_T, _A, _Args...>;

    // erasable
    //   concept: constrains _T to types that are Erasable from a
    // container using allocator _A.
    template<typename _T,
             typename _A = std::allocator<_T>>
    concept erasable =
        is_erasable_v<_T, _A>;

    // fully_insertable
    //   concept: constrains _T to types satisfying
    // DefaultInsertable, CopyInsertable, MoveInsertable, and
    // Erasable for allocator _A.
    template<typename _T,
             typename _A = std::allocator<_T>>
    concept fully_insertable =
        ( default_insertable<_T, _A> &&
          copy_insertable<_T, _A>    &&
          move_insertable<_T, _A>    &&
          erasable<_T, _A> );


    // =====================================================================
    // X.   ITERATOR CONCEPTS (C++20)
    // =====================================================================

    // legacy_iterator
    //   concept: constrains _I to types satisfying the
    // LegacyIterator named requirement.
    template<typename _I>
    concept legacy_iterator =
        is_legacy_iterator_v<_I>;

    // legacy_input_iterator
    //   concept: constrains _I to types satisfying the
    // LegacyInputIterator named requirement. Uses
    // std::input_iterator where possible.
    template<typename _I>
    concept legacy_input_iterator =
        std::input_iterator<_I>;

    // legacy_output_iterator
    //   concept: constrains _I to types satisfying the
    // LegacyOutputIterator named requirement. Uses
    // std::output_iterator with value_type.
    template<typename _I>
    concept legacy_output_iterator =
        is_legacy_output_iterator_v<_I>;

    // legacy_forward_iterator
    //   concept: constrains _I to types satisfying the
    // LegacyForwardIterator named requirement. Uses
    // std::forward_iterator.
    template<typename _I>
    concept legacy_forward_iterator =
        std::forward_iterator<_I>;

    // legacy_bidirectional_iterator
    //   concept: constrains _I to types satisfying the
    // LegacyBidirectionalIterator named requirement. Uses
    // std::bidirectional_iterator.
    template<typename _I>
    concept legacy_bidirectional_iterator =
        std::bidirectional_iterator<_I>;

    // legacy_random_access_iterator
    //   concept: constrains _I to types satisfying the
    // LegacyRandomAccessIterator named requirement. Uses
    // std::random_access_iterator.
    template<typename _I>
    concept legacy_random_access_iterator =
        std::random_access_iterator<_I>;

    // legacy_contiguous_iterator
    //   concept: constrains _I to types satisfying the
    // LegacyContiguousIterator named requirement. Uses
    // std::contiguous_iterator.
    template<typename _I>
    concept legacy_contiguous_iterator =
        std::contiguous_iterator<_I>;


    // =====================================================================
    // XI.  STREAM I/O CONCEPTS (C++20)
    // =====================================================================

    // unformatted_input_stream
    //   concept: constrains _S to types supporting unformatted input
    // stream operations (get, read, gcount).
    template<typename _S>
    concept unformatted_input_stream =
        is_unformatted_input_stream_v<_S>;

    // formatted_input_stream
    //   concept: constrains _S to types supporting formatted input
    // via operator>>.
    template<typename _S>
    concept formatted_input_stream =
        is_formatted_input_stream_v<_S>;

    // unformatted_output_stream
    //   concept: constrains _S to types supporting unformatted output
    // stream operations (put, write).
    template<typename _S>
    concept unformatted_output_stream =
        is_unformatted_output_stream_v<_S>;

    // formatted_output_stream
    //   concept: constrains _S to types supporting formatted output
    // via operator<<.
    template<typename _S>
    concept formatted_output_stream =
        is_formatted_output_stream_v<_S>;

    // input_stream
    //   concept: constrains _S to types supporting both formatted
    // and unformatted input.
    template<typename _S>
    concept input_stream =
        ( unformatted_input_stream<_S> &&
          formatted_input_stream<_S> );

    // output_stream
    //   concept: constrains _S to types supporting both formatted
    // and unformatted output.
    template<typename _S>
    concept output_stream =
        ( unformatted_output_stream<_S> &&
          formatted_output_stream<_S> );


    // =====================================================================
    // XII. RANDOM NUMBER GENERATION CONCEPTS (C++20)
    // =====================================================================

    // seed_sequence
    //   concept: constrains _S to types satisfying the SeedSequence
    // named requirement.
    template<typename _S>
    concept seed_sequence =
        is_seed_sequence_v<_S>;

    // uniform_random_bit_generator
    //   concept: constrains _G to types satisfying the
    // UniformRandomBitGenerator named requirement. Uses
    // std::uniform_random_bit_generator.
    template<typename _G>
    concept uniform_random_bit_generator =
        std::uniform_random_bit_generator<_G>;

    // random_number_engine
    //   concept: constrains _E to types satisfying the
    // RandomNumberEngine named requirement.
    template<typename _E>
    concept random_number_engine =
        is_random_number_engine_v<_E>;

    // random_number_engine_adaptor
    //   concept: constrains _E to types satisfying the
    // RandomNumberEngineAdaptor named requirement.
    template<typename _E>
    concept random_number_engine_adaptor =
        is_random_number_engine_adaptor_v<_E>;

    // random_number_distribution
    //   concept: constrains _D to types satisfying the
    // RandomNumberDistribution named requirement.
    template<typename _D>
    concept random_number_distribution =
        is_random_number_distribution_v<_D>;

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XIII. FORMATTER TRAITS (C++20)
// =========================================================================

#if defined(__cpp_lib_format)

#include <format>

NS_INTERNAL

    // has_basic_formatter_ops
    //   trait: detects BasicFormatter members on
    // std::formatter<_T, _CharT>: parse(ctx) and format(val, ctx).
    template<typename _T,
             typename _CharT = char,
             typename = void>
    struct has_basic_formatter_ops
    {
        static constexpr bool value = false;
    };

    template<typename _T,
             typename _CharT>
    struct has_basic_formatter_ops<_T, _CharT,
        decltype(static_cast<void>(
            std::declval<std::formatter<_T, _CharT>>().parse(
                std::declval<
                    std::basic_format_parse_context<_CharT>&>()),
            std::declval<std::formatter<_T, _CharT>>().format(
                std::declval<const _T&>(),
                std::declval<
                    std::basic_format_context<
                        typename std::basic_string<_CharT>::iterator,
                        _CharT>&>())
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_basic_formatter
//   trait: evaluates to true if std::formatter<_T, _CharT> is a
// valid specialization with parse() and format() members (satisfies
// BasicFormatter).
template<typename _T,
         typename _CharT = char>
struct is_basic_formatter
{
    static constexpr bool value =
        internal::has_basic_formatter_ops<_T, _CharT>::value;
};

// is_formatter
//   trait: evaluates to true if std::formatter<_T, _CharT> satisfies
// the Formatter named requirement: is a BasicFormatter that is also
// DefaultConstructible, CopyConstructible, CopyAssignable, and
// Destructible.
template<typename _T,
         typename _CharT = char>
struct is_formatter
{
    static constexpr bool value =
        ( is_basic_formatter<_T, _CharT>::value                     &&
          std::is_default_constructible_v<
              std::formatter<_T, _CharT>>                           &&
          std::is_copy_constructible_v<
              std::formatter<_T, _CharT>>                           &&
          std::is_copy_assignable_v<
              std::formatter<_T, _CharT>>                           &&
          std::is_destructible_v<std::formatter<_T, _CharT>> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// is_basic_formatter_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_basic_formatter<_T, _CharT>::value.
template<typename _T,
         typename _CharT = char>
inline constexpr bool is_basic_formatter_v =
    is_basic_formatter<_T, _CharT>::value;

// is_formatter_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_formatter<_T, _CharT>::value.
template<typename _T,
         typename _CharT = char>
inline constexpr bool is_formatter_v =
    is_formatter<_T, _CharT>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // basic_formatter
    //   concept: constrains _T to types for which
    // std::formatter<_T, _CharT> provides parse() and format().
    template<typename _T,
             typename _CharT = char>
    concept basic_formatter =
        is_basic_formatter_v<_T, _CharT>;

    // formatter
    //   concept: constrains _T to types for which
    // std::formatter<_T, _CharT> satisfies the full Formatter
    // named requirement.
    template<typename _T,
             typename _CharT = char>
    concept formatter =
        is_formatter_v<_T, _CharT>;

    // formattable
    //   concept: constrains _T to types that can participate in
    // std::format calls (Formatter with char and/or wchar_t).
    template<typename _T>
    concept formattable =
        ( formatter<_T, char> || formatter<_T, wchar_t> );

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // __cpp_lib_format


// =========================================================================
// XIV. CONCURRENCY CONCEPTS (C++20)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // basic_lockable
    //   concept: constrains _M to types satisfying the
    // BasicLockable named requirement.
    template<typename _M>
    concept basic_lockable =
        is_basic_lockable_v<_M>;

    // lockable
    //   concept: constrains _M to types satisfying the Lockable
    // named requirement.
    template<typename _M>
    concept lockable =
        is_lockable_v<_M>;

    // timed_lockable
    //   concept: constrains _M to types satisfying the
    // TimedLockable named requirement.
    template<typename _M>
    concept timed_lockable =
        is_timed_lockable_v<_M>;

    // shared_lockable
    //   concept: constrains _M to types satisfying the
    // SharedLockable named requirement.
    template<typename _M>
    concept shared_lockable =
        is_shared_lockable_v<_M>;

    // shared_timed_lockable
    //   concept: constrains _M to types satisfying the
    // SharedTimedLockable named requirement.
    template<typename _M>
    concept shared_timed_lockable =
        is_shared_timed_lockable_v<_M>;

    // mutex_type
    //   concept: constrains _M to types satisfying the Mutex named
    // requirement.
    template<typename _M>
    concept mutex_type =
        is_mutex_v<_M>;

    // timed_mutex_type
    //   concept: constrains _M to types satisfying the TimedMutex
    // named requirement.
    template<typename _M>
    concept timed_mutex_type =
        is_timed_mutex_v<_M>;

    // shared_mutex_type
    //   concept: constrains _M to types satisfying the SharedMutex
    // named requirement.
    template<typename _M>
    concept shared_mutex_type =
        is_shared_mutex_v<_M>;

    // shared_timed_mutex_type
    //   concept: constrains _M to types satisfying the
    // SharedTimedMutex named requirement.
    template<typename _M>
    concept shared_timed_mutex_type =
        is_shared_timed_mutex_v<_M>;


    // =====================================================================
    // XV.  TYPE TRAIT META-TRAIT CONCEPTS (C++20)
    // =====================================================================

    // unary_type_trait
    //   concept: constrains _T to types satisfying the
    // UnaryTypeTrait named requirement.
    template<typename _T>
    concept unary_type_trait =
        is_unary_type_trait_v<_T>;

    // binary_type_trait
    //   concept: constrains _T to types satisfying the
    // BinaryTypeTrait named requirement.
    template<typename _T>
    concept binary_type_trait =
        is_binary_type_trait_v<_T>;

    // transformation_trait
    //   concept: constrains _T to types satisfying the
    // TransformationTrait named requirement.
    template<typename _T>
    concept transformation_trait =
        is_transformation_trait_v<_T>;


    // =====================================================================
    // XVI. CLOCK, CHARTRAITS, MISC CONCEPTS (C++20)
    // =====================================================================

    // clock_type
    //   concept: constrains _C to types satisfying the Clock named
    // requirement.
    // note: named clock_type to avoid collision with std::chrono
    // identifiers.
    template<typename _C>
    concept clock_type =
        is_clock_v<_C>;

    // trivial_clock_type
    //   concept: constrains _C to types satisfying the TrivialClock
    // named requirement.
    template<typename _C>
    concept trivial_clock_type =
        is_trivial_clock_v<_C>;

    // char_traits_type
    //   concept: constrains _Tr to types satisfying the CharTraits
    // named requirement.
    template<typename _Tr>
    concept char_traits_type =
        is_char_traits_v<_Tr>;

    // bitmask_type
    //   concept: constrains _B to types satisfying the BitmaskType
    // named requirement.
    template<typename _B>
    concept bitmask_type =
        is_bitmask_type_v<_B>;

    // numeric_type
    //   concept: constrains _T to types satisfying the NumericType
    // named requirement (arithmetic types).
    template<typename _T>
    concept numeric_type =
        is_numeric_type_v<_T>;

    // regex_traits_type
    //   concept: constrains _Tr to types satisfying the RegexTraits
    // named requirement.
    template<typename _Tr>
    concept regex_traits_type =
        is_regex_traits_v<_Tr>;

    // literal_type
    //   concept: constrains _T to literal types.
    // note: LiteralType is deprecated in C++17 and removed in C++20.
    // This concept uses the conservative structural approximation
    // from is_literal_type_.
    template<typename _T>
    concept literal_type =
        is_literal_type_v_<_T>;

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XVII. RANGE TRAITS (C++20)
// =========================================================================

#if defined(__cpp_lib_ranges)

#include <ranges>

NS_INTERNAL

    // has_range_ops
    //   trait: detects if _R satisfies std::ranges::range (has
    // ranges::begin and ranges::end).
    template<typename _R,
             typename = void>
    struct has_range_ops
    {
        static constexpr bool value = false;
    };

    template<typename _R>
    struct has_range_ops<_R,
        decltype(static_cast<void>(
            std::ranges::begin(std::declval<_R&>()),
            std::ranges::end(std::declval<_R&>())
        ))>
    {
        static constexpr bool value = true;
    };

    // is_range_adaptor_object_helper
    //   trait: detects if _A is a callable that produces a range when
    // invoked with a range argument. Tests _A(iota_view) as a
    // representative range input.
    template<typename _A,
             typename = void>
    struct is_range_adaptor_object_helper
    {
        static constexpr bool value = false;
    };

    template<typename _A>
    struct is_range_adaptor_object_helper<_A,
        decltype(static_cast<void>(
            std::ranges::begin(
                std::declval<_A>()(
                    std::declval<std::ranges::iota_view<int, int>>()))
        ))>
    {
        static constexpr bool value = true;
    };

    // is_range_adaptor_closure_helper
    //   trait: detects if _A supports the pipe operator (range | _A)
    // producing a range result.
    template<typename _A,
             typename = void>
    struct is_range_adaptor_closure_helper
    {
        static constexpr bool value = false;
    };

    template<typename _A>
    struct is_range_adaptor_closure_helper<_A,
        decltype(static_cast<void>(
            std::ranges::begin(
                std::declval<std::ranges::iota_view<int, int>>() |
                std::declval<_A>())
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_range
//   trait: evaluates to true if _R satisfies std::ranges::range
// (has ranges::begin and ranges::end).
template<typename _R>
struct is_range
{
    static constexpr bool value =
        internal::has_range_ops<_R>::value;
};

// is_range_adaptor_object
//   trait: evaluates to true if _A is a RangeAdaptorObject: a
// callable that accepts a range and produces a range.
// note: uses iota_view<int,int> as representative input. May produce
// false negatives for adaptors requiring specific range properties.
template<typename _A>
struct is_range_adaptor_object
{
    static constexpr bool value =
        internal::is_range_adaptor_object_helper<_A>::value;
};

// is_range_adaptor_closure_object
//   trait: evaluates to true if _A is a RangeAdaptorClosureObject:
// supports the pipe syntax (range | _A) producing a range.
template<typename _A>
struct is_range_adaptor_closure_object
{
    static constexpr bool value =
        internal::is_range_adaptor_closure_helper<_A>::value;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// is_range_v (inline constexpr)
//   value: ODR-safe shorthand for is_range<_R>::value.
template<typename _R>
inline constexpr bool is_range_v =
    is_range<_R>::value;

// is_range_adaptor_object_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_range_adaptor_object<_A>::value.
template<typename _A>
inline constexpr bool is_range_adaptor_object_v =
    is_range_adaptor_object<_A>::value;

// is_range_adaptor_closure_object_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_range_adaptor_closure_object<_A>::value.
template<typename _A>
inline constexpr bool is_range_adaptor_closure_object_v =
    is_range_adaptor_closure_object<_A>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // range
    //   concept: constrains _R to types satisfying the range named
    // requirement. Wraps std::ranges::range.
    template<typename _R>
    concept range =
        std::ranges::range<_R>;

    // sized_range
    //   concept: constrains _R to ranges with O(1) size().
    template<typename _R>
    concept sized_range =
        std::ranges::sized_range<_R>;

    // borrowed_range
    //   concept: constrains _R to ranges whose iterators remain
    // valid after the range object is destroyed.
    template<typename _R>
    concept borrowed_range =
        std::ranges::borrowed_range<_R>;

    // viewable_range
    //   concept: constrains _R to ranges that can be converted to
    // a view.
    template<typename _R>
    concept viewable_range =
        std::ranges::viewable_range<_R>;

    // input_range
    //   concept: constrains _R to input ranges.
    template<typename _R>
    concept input_range =
        std::ranges::input_range<_R>;

    // output_range
    //   concept: constrains _R to output ranges writable with _T.
    template<typename _R,
             typename _T>
    concept output_range =
        std::ranges::output_range<_R, _T>;

    // forward_range
    //   concept: constrains _R to forward ranges.
    template<typename _R>
    concept forward_range =
        std::ranges::forward_range<_R>;

    // bidirectional_range
    //   concept: constrains _R to bidirectional ranges.
    template<typename _R>
    concept bidirectional_range =
        std::ranges::bidirectional_range<_R>;

    // random_access_range
    //   concept: constrains _R to random-access ranges.
    template<typename _R>
    concept random_access_range =
        std::ranges::random_access_range<_R>;

    // contiguous_range
    //   concept: constrains _R to contiguous ranges.
    template<typename _R>
    concept contiguous_range =
        std::ranges::contiguous_range<_R>;

    // common_range
    //   concept: constrains _R to ranges where begin and end have
    // the same type.
    template<typename _R>
    concept common_range =
        std::ranges::common_range<_R>;

    // range_adaptor_object
    //   concept: constrains _A to types that are
    // RangeAdaptorObjects.
    template<typename _A>
    concept range_adaptor_object =
        is_range_adaptor_object_v<_A>;

    // range_adaptor_closure_object
    //   concept: constrains _A to types that are
    // RangeAdaptorClosureObjects (support pipe syntax).
    template<typename _A>
    concept range_adaptor_closure_object =
        is_range_adaptor_closure_object_v<_A>;

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // __cpp_lib_ranges


NS_END  // traits
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


#endif  // DJINTERP_CPP_NAMED20_
