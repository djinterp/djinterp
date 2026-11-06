/*******************************************************************************
* djinterp [cpp]                                                 cpp_named20.hpp
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
*     djinterp::re_std::concepts::default_constructible<T>
*     djinterp::re_std::concepts::move_constructible<T>
*     djinterp::re_std::concepts::value_type<T>
*   (etc.)
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_CONCEPTS             - concept availability
*   D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES    - move trait availability
*
*
* path:      /inc/djinterp/cpp/meta/stl/cpp_named20.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.27
*                                                            revised: 2026.11.04
*******************************************************************************/

#ifndef DJINTERP_META_STL_CPP_NAMED20_HPP
#define DJINTERP_META_STL_CPP_NAMED20_HPP 1

// djinterp
#include "../../djinterp.hpp"  // framework root


// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named20.hpp can only be used in C++ compilation mode"
#endif

// include C++17 layer
#include "cpp_named17.hpp"

// C++20 additions are active for C++20 and all later standards
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// std
#include <concepts>
#include <iterator>
#include <random>
#include <type_traits>
// the feature-tested headers, at file scope: a standard header inside a
// namespace declares the library in it
#if D_ENV_CPP_FEATURE_STL_FORMAT
    #include <format>
#endif  // D_ENV_CPP_FEATURE_STL_FORMAT
#if D_ENV_CPP_FEATURE_STL_RANGES
    #include <ranges>
#endif  // D_ENV_CPP_FEATURE_STL_RANGES


NS_DJINTERP
D_NAMESPACE(stl)

// =========================================================================
// I.   CORE NAMED REQUIREMENT CONCEPTS (C++20)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // default_constructible
    //   concept: constrains T to types that are default-constructible.
    template<typename T>
    concept default_constructible =
        std::is_default_constructible_v<T>;

    // copy_constructible
    //   concept: constrains T to types that are copy-constructible
    // from const T&.
    template<typename T>
    concept copy_constructible =
        std::is_copy_constructible_v<T>;

    // copy_assignable
    //   concept: constrains T to types that are copy-assignable
    // from const T&.
    template<typename T>
    concept copy_assignable =
        std::is_copy_assignable_v<T>;

    // destructible
    //   concept: constrains T to types with an accessible, non-deleted
    // destructor.
    template<typename T>
    concept destructible =
        std::is_destructible_v<T>;

    // move_constructible
    //   concept: constrains T to types that are move-constructible
    // from T&&.
    template<typename T>
    concept move_constructible =
        std::is_move_constructible_v<T>;

    // move_assignable
    //   concept: constrains T to types that are move-assignable
    // from T&&.
    template<typename T>
    concept move_assignable =
        std::is_move_assignable_v<T>;


    // =====================================================================
    // II.  NOTHROW CONCEPTS (C++20)
    // =====================================================================

    // nothrow_default_constructible
    //   concept: constrains T to types that are default-constructible
    // without throwing exceptions.
    template<typename T>
    concept nothrow_default_constructible =
        std::is_nothrow_default_constructible_v<T>;

    // nothrow_copy_constructible
    //   concept: constrains T to types that are copy-constructible
    // without throwing exceptions.
    template<typename T>
    concept nothrow_copy_constructible =
        std::is_nothrow_copy_constructible_v<T>;

    // nothrow_copy_assignable
    //   concept: constrains T to types that are copy-assignable
    // without throwing exceptions.
    template<typename T>
    concept nothrow_copy_assignable =
        std::is_nothrow_copy_assignable_v<T>;

    // nothrow_destructible
    //   concept: constrains T to types that are destructible without
    // throwing exceptions.
    template<typename T>
    concept nothrow_destructible =
        std::is_nothrow_destructible_v<T>;

    // nothrow_move_constructible
    //   concept: constrains T to types that are move-constructible
    // without throwing exceptions.
    template<typename T>
    concept nothrow_move_constructible =
        std::is_nothrow_move_constructible_v<T>;

    // nothrow_move_assignable
    //   concept: constrains T to types that are move-assignable
    // without throwing exceptions.
    template<typename T>
    concept nothrow_move_assignable =
        std::is_nothrow_move_assignable_v<T>;


    // =====================================================================
    // III. TRIVIALLY-QUALIFIED CONCEPTS (C++20)
    // =====================================================================

    // trivially_default_constructible
    //   concept: constrains T to types that are trivially
    // default-constructible.
    template<typename T>
    concept trivially_default_constructible =
        std::is_trivially_default_constructible_v<T>;

    // trivially_copy_constructible
    //   concept: constrains T to types that are trivially
    // copy-constructible.
    template<typename T>
    concept trivially_copy_constructible =
        std::is_trivially_copy_constructible_v<T>;

    // trivially_copy_assignable
    //   concept: constrains T to types that are trivially
    // copy-assignable.
    template<typename T>
    concept trivially_copy_assignable =
        std::is_trivially_copy_assignable_v<T>;

    // trivially_destructible
    //   concept: constrains T to types that are trivially destructible.
    template<typename T>
    concept trivially_destructible =
        std::is_trivially_destructible_v<T>;

    // trivially_move_constructible
    //   concept: constrains T to types that are trivially
    // move-constructible.
    template<typename T>
    concept trivially_move_constructible =
        std::is_trivially_move_constructible_v<T>;

    // trivially_move_assignable
    //   concept: constrains T to types that are trivially
    // move-assignable.
    template<typename T>
    concept trivially_move_assignable =
        std::is_trivially_move_assignable_v<T>;


    // =====================================================================
    // IV.  COMPOUND REQUIREMENT CONCEPTS (C++20)
    // =====================================================================

    // fully_copyable
    //   concept: constrains T to types satisfying both
    // CopyConstructible and CopyAssignable.
    template<typename T>
    concept fully_copyable =
        ( copy_constructible<T> &&
          copy_assignable<T> );

    // fully_movable
    //   concept: constrains T to types satisfying both
    // MoveConstructible and MoveAssignable.
    template<typename T>
    concept fully_movable =
        ( move_constructible<T> &&
          move_assignable<T> );

    // fully_constructible
    //   concept: constrains T to types satisfying
    // DefaultConstructible, CopyConstructible, and MoveConstructible.
    template<typename T>
    concept fully_constructible =
        ( default_constructible<T> &&
          copy_constructible<T>    &&
          move_constructible<T> );

    // fully_assignable
    //   concept: constrains T to types satisfying both
    // CopyAssignable and MoveAssignable.
    template<typename T>
    concept fully_assignable =
        ( copy_assignable<T> &&
          move_assignable<T> );

    // value_type
    //   concept: constrains T to types satisfying all six core named
    // requirements: DefaultConstructible, CopyConstructible,
    // MoveConstructible, CopyAssignable, MoveAssignable, and
    // Destructible.
    template<typename T>
    concept value_type =
        ( default_constructible<T> &&
          copy_constructible<T>    &&
          move_constructible<T>    &&
          copy_assignable<T>       &&
          move_assignable<T>       &&
          destructible<T> );


    // =====================================================================
    // V.   TYPE CLASSIFICATION CONCEPTS (C++20)
    // =====================================================================

    // scalar_type
    //   concept: constrains T to scalar types (arithmetic, enumeration,
    // pointer, pointer-to-member, or std::nullptr_t).
    template<typename T>
    concept scalar_type =
        std::is_scalar_v<T>;

    // trivially_copyable
    //   concept: constrains T to types that are trivially copyable
    // (safe to copy via memcpy/memmove).
    template<typename T>
    concept trivially_copyable =
        std::is_trivially_copyable_v<T>;

    // trivial_type
    //   concept: constrains T to trivial types (trivially
    // default-constructible and trivially copyable).
    // note: TrivialType named requirement is deprecated in C++26.
    template<typename T>
    concept trivial_type =
        std::is_trivial_v<T>;

    // standard_layout_type
    //   concept: constrains T to types satisfying the
    // StandardLayoutType named requirement.
    template<typename T>
    concept standard_layout_type =
        std::is_standard_layout_v<T>;

    // pod_type
    //   concept: constrains T to POD (Plain Old Data) types.
    // note: PODType named requirement is deprecated in C++20. Prefer
    // trivially_copyable with standard_layout_type for new code.
    template<typename T>
    concept pod_type =
        ( trivial_type<T> &&
          standard_layout_type<T> );


    // =====================================================================
    // VI.  COMPARISON AND BOOLEAN CONCEPTS (C++20)
    // =====================================================================

    // boolean_testable
    //   concept: constrains T to types that are contextually
    // convertible to bool.
    template<typename T>
    concept boolean_testable =
        std::convertible_to<T, bool>;

    // equality_comparable
    //   concept: constrains T to types satisfying
    // EqualityComparable, using std::equality_comparable.
    template<typename T>
    concept equality_comparable =
        std::equality_comparable<T>;

    // less_than_comparable
    //   concept: constrains T to types supporting operator< with a
    // bool-convertible result.
    template<typename T>
    concept less_than_comparable = requires(const T _a,
                                            const T _b)
    {
        { _a < _b } -> boolean_testable;
    };

    // swappable
    //   concept: constrains T to types that are swappable, using
    // std::swappable.
    template<typename T>
    concept swappable =
        std::swappable<T>;

    // value_swappable
    //   concept: constrains T to iterator-like types whose
    // dereferenced values are swappable.
    template<typename T>
    concept value_swappable = requires(T _a, T _b)
    {
        *_a;
        requires std::swappable<decltype(*_a)>;
    };

    // nullable_pointer
    //   concept: constrains T to types satisfying the NullablePointer
    // named requirement.
    template<typename T>
    concept nullable_pointer =
        ( default_constructible<T>                      &&
          copy_constructible<T>                         &&
          copy_assignable<T>                            &&
          destructible<T>                               &&
          equality_comparable<T>                        &&
          std::constructible_from<T, std::nullptr_t>    &&
          boolean_testable<T> );

    // hashable
    //   concept: constrains T to types for which std::hash<T> is a
    // valid specialization.
    template<typename T>
    concept hashable =
        is_hashable_v<T>;

    // allocator
    //   concept: constrains A to types satisfying the basic
    // structural Allocator requirements.
    template<typename A>
    concept allocator =
        is_allocator_v<A>;


    // =====================================================================
    // VII. CALLABLE AND PREDICATE CONCEPTS (C++20)
    // =====================================================================

    // callable
    //   concept: constrains F to types invocable with Args... using
    // std::invocable.
    template<typename F,
             typename... Args>
    concept callable =
        std::invocable<F, Args...>;

    // predicate
    //   concept: constrains F to types satisfying Predicate for Arg,
    // using std::predicate.
    template<typename F,
             typename Arg>
    concept predicate =
        std::predicate<F, Arg>;

    // binary_predicate
    //   concept: constrains F to types satisfying BinaryPredicate
    // for Arg1 and Arg2.
    template<typename F,
             typename Arg1,
             typename Arg2>
    concept binary_predicate =
        std::predicate<F, Arg1, Arg2>;

    // compare
    //   concept: constrains F to types satisfying Compare for T
    // (BinaryPredicate on const T&, const T&, implying strict weak
    // ordering).
    // note: std::strict_weak_order provides the semantic contract;
    // this concept checks the structural requirement only.
    template<typename F,
             typename T>
    concept compare =
        binary_predicate<F, const T&, const T&>;


    // =====================================================================
    // VIII. CONTAINER CONCEPTS (C++20)
    // =====================================================================

    // container
    //   concept: constrains C to types satisfying the structural
    // Container named requirement.
    template<typename C>
    concept container =
        is_container_v<C>;

    // reversible_container
    //   concept: constrains C to types satisfying
    // ReversibleContainer.
    template<typename C>
    concept reversible_container =
        is_reversible_container_v<C>;

    // allocator_aware_container
    //   concept: constrains C to types satisfying
    // AllocatorAwareContainer.
    template<typename C>
    concept allocator_aware_container =
        is_allocator_aware_container_v<C>;

    // sequence_container
    //   concept: constrains C to types satisfying the structural
    // SequenceContainer named requirement.
    template<typename C>
    concept sequence_container =
        is_sequence_container_v<C>;

    // contiguous_container
    //   concept: constrains C to types satisfying
    // ContiguousContainer: a Container with contiguous data() storage.
    template<typename C>
    concept contiguous_container =
        is_contiguous_container_v<C>;

    // associative_container
    //   concept: constrains C to types satisfying
    // AssociativeContainer.
    template<typename C>
    concept associative_container =
        is_associative_container_v<C>;

    // unordered_associative_container
    //   concept: constrains C to types satisfying
    // UnorderedAssociativeContainer.
    template<typename C>
    concept unordered_associative_container =
        is_unordered_associative_container_v<C>;


    // =====================================================================
    // IX.  CONTAINER ELEMENT CONCEPTS (C++20)
    // =====================================================================

    // default_insertable
    //   concept: constrains T to types that are DefaultInsertable
    // into a container using allocator A.
    template<typename T,
             typename A = std::allocator<T>>
    concept default_insertable =
        is_default_insertable_v<T, A>;

    // copy_insertable
    //   concept: constrains T to types that are CopyInsertable into
    // a container using allocator A.
    template<typename T,
             typename A = std::allocator<T>>
    concept copy_insertable =
        is_copy_insertable_v<T, A>;

    // move_insertable
    //   concept: constrains T to types that are MoveInsertable into
    // a container using allocator A.
    template<typename T,
             typename A = std::allocator<T>>
    concept move_insertable =
        is_move_insertable_v<T, A>;

    // emplace_constructible
    //   concept: constrains T to types that are
    // EmplaceConstructible into a container using allocator A from
    // Args... .
    template<typename T,
             typename A,
             typename... Args>
    concept emplace_constructible =
        is_emplace_constructible_v<T, A, Args...>;

    // erasable
    //   concept: constrains T to types that are Erasable from a
    // container using allocator A.
    template<typename T,
             typename A = std::allocator<T>>
    concept erasable =
        is_erasable_v<T, A>;

    // fully_insertable
    //   concept: constrains T to types satisfying
    // DefaultInsertable, CopyInsertable, MoveInsertable, and
    // Erasable for allocator A.
    template<typename T,
             typename A = std::allocator<T>>
    concept fully_insertable =
        ( default_insertable<T, A> &&
          copy_insertable<T, A>    &&
          move_insertable<T, A>    &&
          erasable<T, A> );


    // =====================================================================
    // X.   ITERATOR CONCEPTS (C++20)
    // =====================================================================

    // legacy_iterator
    //   concept: constrains I to types satisfying the
    // LegacyIterator named requirement.
    template<typename I>
    concept legacy_iterator =
        is_legacy_iterator_v<I>;

    // legacy_input_iterator
    //   concept: constrains I to types satisfying the
    // LegacyInputIterator named requirement. Uses
    // std::input_iterator where possible.
    template<typename I>
    concept legacy_input_iterator =
        std::input_iterator<I>;

    // legacy_output_iterator
    //   concept: constrains I to types satisfying the
    // LegacyOutputIterator named requirement. Uses
    // std::output_iterator with value_type.
    template<typename I>
    concept legacy_output_iterator =
        is_legacy_output_iterator_v<I>;

    // legacy_forward_iterator
    //   concept: constrains I to types satisfying the
    // LegacyForwardIterator named requirement. Uses
    // std::forward_iterator.
    template<typename I>
    concept legacy_forward_iterator =
        std::forward_iterator<I>;

    // legacy_bidirectional_iterator
    //   concept: constrains I to types satisfying the
    // LegacyBidirectionalIterator named requirement. Uses
    // std::bidirectional_iterator.
    template<typename I>
    concept legacy_bidirectional_iterator =
        std::bidirectional_iterator<I>;

    // legacy_random_access_iterator
    //   concept: constrains I to types satisfying the
    // LegacyRandomAccessIterator named requirement. Uses
    // std::random_access_iterator.
    template<typename I>
    concept legacy_random_access_iterator =
        std::random_access_iterator<I>;

    // legacy_contiguous_iterator
    //   concept: constrains I to types satisfying the
    // LegacyContiguousIterator named requirement. Uses
    // std::contiguous_iterator.
    template<typename I>
    concept legacy_contiguous_iterator =
        std::contiguous_iterator<I>;


    // =====================================================================
    // XI.  STREAM I/O CONCEPTS (C++20)
    // =====================================================================

    // unformatted_input_stream
    //   concept: constrains S to types supporting unformatted input
    // stream operations (get, read, gcount).
    template<typename S>
    concept unformatted_input_stream =
        is_unformatted_input_stream_v<S>;

    // formatted_input_stream
    //   concept: constrains S to types supporting formatted input
    // via operator>>.
    template<typename S>
    concept formatted_input_stream =
        is_formatted_input_stream_v<S>;

    // unformatted_output_stream
    //   concept: constrains S to types supporting unformatted output
    // stream operations (put, write).
    template<typename S>
    concept unformatted_output_stream =
        is_unformatted_output_stream_v<S>;

    // formatted_output_stream
    //   concept: constrains S to types supporting formatted output
    // via operator<<.
    template<typename S>
    concept formatted_output_stream =
        is_formatted_output_stream_v<S>;

    // input_stream
    //   concept: constrains S to types supporting both formatted
    // and unformatted input.
    template<typename S>
    concept input_stream =
        ( unformatted_input_stream<S> &&
          formatted_input_stream<S> );

    // output_stream
    //   concept: constrains S to types supporting both formatted
    // and unformatted output.
    template<typename S>
    concept output_stream =
        ( unformatted_output_stream<S> &&
          formatted_output_stream<S> );


    // =====================================================================
    // XII. RANDOM NUMBER GENERATION CONCEPTS (C++20)
    // =====================================================================

    // seed_sequence
    //   concept: constrains S to types satisfying the SeedSequence
    // named requirement.
    template<typename S>
    concept seed_sequence =
        is_seed_sequence_v<S>;

    // uniform_random_bit_generator
    //   concept: constrains G to types satisfying the
    // UniformRandomBitGenerator named requirement. Uses
    // std::uniform_random_bit_generator.
    template<typename G>
    concept uniform_random_bit_generator =
        std::uniform_random_bit_generator<G>;

    // random_number_engine
    //   concept: constrains E to types satisfying the
    // RandomNumberEngine named requirement.
    template<typename E>
    concept random_number_engine =
        is_random_number_engine_v<E>;

    // random_number_engine_adaptor
    //   concept: constrains E to types satisfying the
    // RandomNumberEngineAdaptor named requirement.
    template<typename E>
    concept random_number_engine_adaptor =
        is_random_number_engine_adaptor_v<E>;

    // random_number_distribution
    //   concept: constrains D to types satisfying the
    // RandomNumberDistribution named requirement.
    template<typename D>
    concept random_number_distribution =
        is_random_number_distribution_v<D>;

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XIII. FORMATTER TRAITS (C++20)
// =========================================================================

#if D_ENV_CPP_FEATURE_STL_FORMAT

NS_INTERNAL

    // has_basic_formatter_ops
    //   trait: detects BasicFormatter members on
    // std::formatter<T, CharT>: parse(ctx) and format(val, ctx).
    template<typename T,
             typename CharT = char,
             typename = void>
    struct has_basic_formatter_ops
    {
        static constexpr bool value = false;
    };

    template<typename T,
             typename CharT>
    struct has_basic_formatter_ops<T, CharT,
        decltype(static_cast<void>(
            std::declval<std::formatter<T, CharT>>().parse(
                std::declval<
                    std::basic_format_parse_context<CharT>&>()),
            std::declval<std::formatter<T, CharT>>().format(
                std::declval<const T&>(),
                std::declval<
                    std::basic_format_context<
                        typename std::basic_string<CharT>::iterator,
                        CharT>&>())
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_basic_formatter
//   trait: evaluates to true if std::formatter<T, CharT> is a
// valid specialization with parse() and format() members (satisfies
// BasicFormatter).
template<typename T,
         typename CharT = char>
struct is_basic_formatter
{
    static constexpr bool value =
        internal::has_basic_formatter_ops<T, CharT>::value;
};

// is_formatter
//   trait: evaluates to true if std::formatter<T, CharT> satisfies
// the Formatter named requirement: is a BasicFormatter that is also
// DefaultConstructible, CopyConstructible, CopyAssignable, and
// Destructible.
template<typename T,
         typename CharT = char>
struct is_formatter
{
    static constexpr bool value =
        ( is_basic_formatter<T, CharT>::value                     &&
          std::is_default_constructible_v<
              std::formatter<T, CharT>>                           &&
          std::is_copy_constructible_v<
              std::formatter<T, CharT>>                           &&
          std::is_copy_assignable_v<
              std::formatter<T, CharT>>                           &&
          std::is_destructible_v<std::formatter<T, CharT>> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// is_basic_formatter_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_basic_formatter<T, CharT>::value.
template<typename T,
         typename CharT = char>
inline constexpr bool is_basic_formatter_v =
    is_basic_formatter<T, CharT>::value;

// is_formatter_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_formatter<T, CharT>::value.
template<typename T,
         typename CharT = char>
inline constexpr bool is_formatter_v =
    is_formatter<T, CharT>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // basic_formatter
    //   concept: constrains T to types for which
    // std::formatter<T, CharT> provides parse() and format().
    template<typename T,
             typename CharT = char>
    concept basic_formatter =
        is_basic_formatter_v<T, CharT>;

    // formatter
    //   concept: constrains T to types for which
    // std::formatter<T, CharT> satisfies the full Formatter
    // named requirement.
    template<typename T,
             typename CharT = char>
    concept formatter =
        is_formatter_v<T, CharT>;

    // formattable
    //   concept: constrains T to types that can participate in
    // std::format calls (Formatter with char and/or wchar_t).
    template<typename T>
    concept formattable =
        ( formatter<T, char> || formatter<T, wchar_t> );

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // D_ENV_CPP_FEATURE_STL_FORMAT


// =========================================================================
// XIV. CONCURRENCY CONCEPTS (C++20)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // basic_lockable
    //   concept: constrains M to types satisfying the
    // BasicLockable named requirement.
    template<typename M>
    concept basic_lockable =
        is_basic_lockable_v<M>;

    // lockable
    //   concept: constrains M to types satisfying the Lockable
    // named requirement.
    template<typename M>
    concept lockable =
        is_lockable_v<M>;

    // timed_lockable
    //   concept: constrains M to types satisfying the
    // TimedLockable named requirement.
    template<typename M>
    concept timed_lockable =
        is_timed_lockable_v<M>;

    // shared_lockable
    //   concept: constrains M to types satisfying the
    // SharedLockable named requirement.
    template<typename M>
    concept shared_lockable =
        is_shared_lockable_v<M>;

    // shared_timed_lockable
    //   concept: constrains M to types satisfying the
    // SharedTimedLockable named requirement.
    template<typename M>
    concept shared_timed_lockable =
        is_shared_timed_lockable_v<M>;

    // mutex_type
    //   concept: constrains M to types satisfying the Mutex named
    // requirement.
    template<typename M>
    concept mutex_type =
        is_mutex_v<M>;

    // timed_mutex_type
    //   concept: constrains M to types satisfying the TimedMutex
    // named requirement.
    template<typename M>
    concept timed_mutex_type =
        is_timed_mutex_v<M>;

    // shared_mutex_type
    //   concept: constrains M to types satisfying the SharedMutex
    // named requirement.
    template<typename M>
    concept shared_mutex_type =
        is_shared_mutex_v<M>;

    // shared_timed_mutex_type
    //   concept: constrains M to types satisfying the
    // SharedTimedMutex named requirement.
    template<typename M>
    concept shared_timed_mutex_type =
        is_shared_timed_mutex_v<M>;


    // =====================================================================
    // XV.  TYPE TRAIT META-TRAIT CONCEPTS (C++20)
    // =====================================================================

    // unary_type_trait
    //   concept: constrains T to types satisfying the
    // UnaryTypeTrait named requirement.
    template<typename T>
    concept unary_type_trait =
        is_unary_type_trait_v<T>;

    // binary_type_trait
    //   concept: constrains T to types satisfying the
    // BinaryTypeTrait named requirement.
    template<typename T>
    concept binary_type_trait =
        is_binary_type_trait_v<T>;

    // transformation_trait
    //   concept: constrains T to types satisfying the
    // TransformationTrait named requirement.
    template<typename T>
    concept transformation_trait =
        is_transformation_trait_v<T>;


    // =====================================================================
    // XVI. CLOCK, CHARTRAITS, MISC CONCEPTS (C++20)
    // =====================================================================

    // clock_type
    //   concept: constrains C to types satisfying the Clock named
    // requirement.
    // note: named clock_type to avoid collision with std::chrono
    // identifiers.
    template<typename C>
    concept clock_type =
        is_clock_v<C>;

    // trivial_clock_type
    //   concept: constrains C to types satisfying the TrivialClock
    // named requirement.
    template<typename C>
    concept trivial_clock_type =
        is_trivial_clock_v<C>;

    // char_traits_type
    //   concept: constrains Tr to types satisfying the CharTraits
    // named requirement.
    template<typename Tr>
    concept char_traits_type =
        is_char_traits_v<Tr>;

    // bitmask_type
    //   concept: constrains B to types satisfying the BitmaskType
    // named requirement.
    template<typename B>
    concept bitmask_type =
        is_bitmask_type_v<B>;

    // numeric_type
    //   concept: constrains T to types satisfying the NumericType
    // named requirement (arithmetic types).
    template<typename T>
    concept numeric_type =
        is_numeric_type_v<T>;

    // regex_traits_type
    //   concept: constrains Tr to types satisfying the RegexTraits
    // named requirement.
    template<typename Tr>
    concept regex_traits_type =
        is_regex_traits_v<Tr>;

    // literal_type
    //   concept: constrains T to literal types.
    // note: LiteralType is deprecated in C++17 and removed in C++20.
    // This concept uses the conservative structural approximation
    // from is_literal_type_.
    template<typename T>
    concept literal_type =
        is_literal_type_v_<T>;

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XVII. RANGE TRAITS (C++20)
// =========================================================================

#if D_ENV_CPP_FEATURE_STL_RANGES

NS_INTERNAL

    // has_range_ops
    //   trait: detects if R satisfies std::ranges::range (has
    // ranges::begin and ranges::end).
    template<typename R,
             typename = void>
    struct has_range_ops
    {
        static constexpr bool value = false;
    };

    template<typename R>
    struct has_range_ops<R,
        decltype(static_cast<void>(
            std::ranges::begin(std::declval<R&>()),
            std::ranges::end(std::declval<R&>())
        ))>
    {
        static constexpr bool value = true;
    };

    // is_range_adaptor_object_helper
    //   trait: detects if A is a callable that produces a range when
    // invoked with a range argument. Tests A(iota_view) as a
    // representative range input.
    template<typename A,
             typename = void>
    struct is_range_adaptor_object_helper
    {
        static constexpr bool value = false;
    };

    template<typename A>
    struct is_range_adaptor_object_helper<A,
        decltype(static_cast<void>(
            std::ranges::begin(
                std::declval<A>()(
                    std::declval<std::ranges::iota_view<int, int>>()))
        ))>
    {
        static constexpr bool value = true;
    };

    // is_range_adaptor_closure_helper
    //   trait: detects if A supports the pipe operator (range | A)
    // producing a range result.
    template<typename A,
             typename = void>
    struct is_range_adaptor_closure_helper
    {
        static constexpr bool value = false;
    };

    template<typename A>
    struct is_range_adaptor_closure_helper<A,
        decltype(static_cast<void>(
            std::ranges::begin(
                std::declval<std::ranges::iota_view<int, int>>() |
                std::declval<A>())
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_range
//   trait: evaluates to true if R satisfies std::ranges::range
// (has ranges::begin and ranges::end).
template<typename R>
struct is_range
{
    static constexpr bool value =
        internal::has_range_ops<R>::value;
};

// is_range_adaptor_object
//   trait: evaluates to true if A is a RangeAdaptorObject: a
// callable that accepts a range and produces a range.
// note: uses iota_view<int,int> as representative input. May produce
// false negatives for adaptors requiring specific range properties.
template<typename A>
struct is_range_adaptor_object
{
    static constexpr bool value =
        internal::is_range_adaptor_object_helper<A>::value;
};

// is_range_adaptor_closure_object
//   trait: evaluates to true if A is a RangeAdaptorClosureObject:
// supports the pipe syntax (range | A) producing a range.
template<typename A>
struct is_range_adaptor_closure_object
{
    static constexpr bool value =
        internal::is_range_adaptor_closure_helper<A>::value;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// is_range_v (inline constexpr)
//   value: ODR-safe shorthand for is_range<R>::value.
template<typename R>
inline constexpr bool is_range_v =
    is_range<R>::value;

// is_range_adaptor_object_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_range_adaptor_object<A>::value.
template<typename A>
inline constexpr bool is_range_adaptor_object_v =
    is_range_adaptor_object<A>::value;

// is_range_adaptor_closure_object_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_range_adaptor_closure_object<A>::value.
template<typename A>
inline constexpr bool is_range_adaptor_closure_object_v =
    is_range_adaptor_closure_object<A>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // range
    //   concept: constrains R to types satisfying the range named
    // requirement. Wraps std::ranges::range.
    template<typename R>
    concept range =
        std::ranges::range<R>;

    // sized_range
    //   concept: constrains R to ranges with O(1) size().
    template<typename R>
    concept sized_range =
        std::ranges::sized_range<R>;

    // borrowed_range
    //   concept: constrains R to ranges whose iterators remain
    // valid after the range object is destroyed.
    template<typename R>
    concept borrowed_range =
        std::ranges::borrowed_range<R>;

    // viewable_range
    //   concept: constrains R to ranges that can be converted to
    // a view.
    template<typename R>
    concept viewable_range =
        std::ranges::viewable_range<R>;

    // input_range
    //   concept: constrains R to input ranges.
    template<typename R>
    concept input_range =
        std::ranges::input_range<R>;

    // output_range
    //   concept: constrains R to output ranges writable with T.
    template<typename R,
             typename T>
    concept output_range =
        std::ranges::output_range<R, T>;

    // forward_range
    //   concept: constrains R to forward ranges.
    template<typename R>
    concept forward_range =
        std::ranges::forward_range<R>;

    // bidirectional_range
    //   concept: constrains R to bidirectional ranges.
    template<typename R>
    concept bidirectional_range =
        std::ranges::bidirectional_range<R>;

    // random_access_range
    //   concept: constrains R to random-access ranges.
    template<typename R>
    concept random_access_range =
        std::ranges::random_access_range<R>;

    // contiguous_range
    //   concept: constrains R to contiguous ranges.
    template<typename R>
    concept contiguous_range =
        std::ranges::contiguous_range<R>;

    // common_range
    //   concept: constrains R to ranges where begin and end have
    // the same type.
    template<typename R>
    concept common_range =
        std::ranges::common_range<R>;

    // range_adaptor_object
    //   concept: constrains A to types that are
    // RangeAdaptorObjects.
    template<typename A>
    concept range_adaptor_object =
        is_range_adaptor_object_v<A>;

    // range_adaptor_closure_object
    //   concept: constrains A to types that are
    // RangeAdaptorClosureObjects (support pipe syntax).
    template<typename A>
    concept range_adaptor_closure_object =
        is_range_adaptor_closure_object_v<A>;

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // D_ENV_CPP_FEATURE_STL_RANGES


NS_END  // stl
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


#endif  // DJINTERP_META_STL_CPP_NAMED20_HPP
