/******************************************************************************
* djinterp                                                  iterator_traits.hpp
*
* Container-level iterator classification traits.
*   Provides SFINAE-based compile-time detection of iterator properties
* at both the iterator level (what kind of iterator is it?) and the
* container level (what kind of iterators does it provide?).
*
*   Iterator-level traits supplement the standard iterator_traits with
* container-specific detection for begin()/end(), cbegin()/cend(),
* rbegin()/rend(), and constexpr iteration.
*
*   Container-level iterability traits combine iterator detection with
* begin/end availability to answer "can I iterate this container with
* at least X-category iterators?"
*
*   PORTABILITY:
*   C++11 baseline.  `_v` variable templates are gated behind
* D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES (C++14+).  The contiguous-
* iterator detection probes std::contiguous_iterator_tag only on
* C++20+.
*
* TABLE OF CONTENTS
* =================
* I.    Iterator-Level Traits
* II.   Iterator Category Extraction
* III.  Container-Level Iterability
* IV.   Const / Reverse Iteration Detection
* V.    Iterator Compatibility
* VI.   iterator_level Enum
* VII.  Combined Classification
*
*
* path:      /inc/djinterp/container/iterator/iterator_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.05.20
******************************************************************************/

#ifndef DJINTERP_ITERATOR_TRAITS_
#define DJINTERP_ITERATOR_TRAITS_ 1

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../core/djinterp.hpp"
#include "../../../core/meta/type_traits.hpp"


NS_DJINTERP

// ===========================================================================
// I.   Iterator-Level Traits
// ===========================================================================
// SFINAE detection of iterator category by structural
// probing.  Each trait yields std::true_type or
// std::false_type and is paired with a `_v` variable
// template alias when variable templates are available.

// is_input_iterator
//   trait: true if _Type satisfies the structural
// requirements of an InputIterator: has the five nested
// types, supports dereference, pre/post-increment, and
// equality/inequality comparison.
template<typename _Type,
         typename = void>
struct is_input_iterator : std::false_type
{};

template<typename _Type>
struct is_input_iterator<_Type, void_t<
    typename _Type::value_type,
    typename _Type::difference_type,
    typename _Type::pointer,
    typename _Type::reference,
    typename _Type::iterator_category,
    decltype(++std::declval<_Type&>()),
    decltype(std::declval<_Type&>()++),
    decltype(*std::declval<_Type&>()),
    decltype(std::declval<const _Type&>() ==
             std::declval<const _Type&>()),
    decltype(std::declval<const _Type&>() !=
             std::declval<const _Type&>())
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_input_iterator_v
    //   variable template: value of is_input_iterator<_Type>.
    template<typename _Type>
    constexpr bool is_input_iterator_v =
        is_input_iterator<_Type>::value;
#endif

// is_output_iterator
//   trait: true if _Type supports assignment through
// dereference and pre/post-increment, and its category
// derives from output_iterator_tag.
template<typename _Type,
         typename = void>
struct is_output_iterator : std::false_type
{};

template<typename _Type>
struct is_output_iterator<_Type, void_t<
    decltype(*std::declval<_Type&>() =
        std::declval<typename
            std::iterator_traits<_Type>::value_type>()),
    decltype(++std::declval<_Type&>()),
    decltype(std::declval<_Type&>()++)
>> : std::is_base_of<
         std::output_iterator_tag,
         typename std::iterator_traits<
             _Type>::iterator_category>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_output_iterator_v
    //   variable template: value of is_output_iterator<_Type>.
    template<typename _Type>
    constexpr bool is_output_iterator_v =
        is_output_iterator<_Type>::value;
#endif

// is_forward_iterator
//   trait: true if _Type is default-constructible, has the
// standard nested types, and its category derives from
// forward_iterator_tag.
template<typename _Type,
         typename = void>
struct is_forward_iterator : std::false_type
{};

template<typename _Type>
struct is_forward_iterator<_Type, void_t<
    typename std::iterator_traits<_Type>::value_type,
    typename std::iterator_traits<_Type>::difference_type,
    typename std::iterator_traits<_Type>::reference,
    typename std::iterator_traits<_Type>::pointer,
    decltype(_Type())
>> : std::is_base_of<
         std::forward_iterator_tag,
         typename std::iterator_traits<
             _Type>::iterator_category>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_forward_iterator_v
    //   variable template: value of is_forward_iterator<_Type>.
    template<typename _Type>
    constexpr bool is_forward_iterator_v =
        is_forward_iterator<_Type>::value;
#endif

// is_bidirectional_iterator
//   trait: true if _Type supports pre/post-decrement and
// its category derives from bidirectional_iterator_tag.
template<typename _Type,
         typename = void>
struct is_bidirectional_iterator : std::false_type
{};

template<typename _Type>
struct is_bidirectional_iterator<_Type, void_t<
    decltype(--std::declval<_Type&>()),
    decltype(std::declval<_Type&>()--)
>> : std::is_base_of<
         std::bidirectional_iterator_tag,
         typename std::iterator_traits<
             _Type>::iterator_category>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_bidirectional_iterator_v
    //   variable template: value of
    // is_bidirectional_iterator<_Type>.
    template<typename _Type>
    constexpr bool is_bidirectional_iterator_v =
        is_bidirectional_iterator<_Type>::value;
#endif

// is_random_access_iterator
//   trait: true if _Type supports +=, -=, +, -, [], and
// relational comparisons, and its category derives from
// random_access_iterator_tag.
template<typename _Type,
         typename = void>
struct is_random_access_iterator : std::false_type
{};

template<typename _Type>
struct is_random_access_iterator<_Type, void_t<
    decltype(std::declval<_Type&>() +=
        std::declval<typename std::iterator_traits<
            _Type>::difference_type>()),
    decltype(std::declval<_Type&>() -=
        std::declval<typename std::iterator_traits<
            _Type>::difference_type>()),
    decltype(std::declval<const _Type&>() +
        std::declval<typename std::iterator_traits<
            _Type>::difference_type>()),
    decltype(std::declval<const _Type&>() -
        std::declval<typename std::iterator_traits<
            _Type>::difference_type>()),
    decltype(std::declval<const _Type&>() -
        std::declval<const _Type&>()),
    decltype(std::declval<const _Type&>()[
        std::declval<typename std::iterator_traits<
            _Type>::difference_type>()]),
    decltype(std::declval<const _Type&>() <
        std::declval<const _Type&>()),
    decltype(std::declval<const _Type&>() >=
        std::declval<const _Type&>())
>> : std::is_base_of<
         std::random_access_iterator_tag,
         typename std::iterator_traits<
             _Type>::iterator_category>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_random_access_iterator_v
    //   variable template: value of
    // is_random_access_iterator<_Type>.
    template<typename _Type>
    constexpr bool is_random_access_iterator_v =
        is_random_access_iterator<_Type>::value;
#endif

// is_contiguous_iterator
//   trait: true if _Type is a random-access iterator over
// contiguous memory.  Raw pointers always qualify; class
// iterators must additionally carry contiguous_iterator_tag
// (C++20).
NS_INTERNAL

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // has_contiguous_tag
    //   trait: detects whether the iterator's category is
    // (or derives from) std::contiguous_iterator_tag.
    template<typename _Iter,
             typename = void>
    struct has_contiguous_tag : std::false_type
    {};

    template<typename _Iter>
    struct has_contiguous_tag<_Iter,
        typename std::enable_if<std::is_base_of<
            std::contiguous_iterator_tag,
            typename std::iterator_traits<
                _Iter>::iterator_category>::value
        >::type> : std::true_type
    {};
#else
    // pre-C++20 fallback: contiguous_iterator_tag does not
    // exist, so only raw pointers qualify.
    template<typename _Iter>
    struct has_contiguous_tag : std::false_type
    {};
#endif

NS_END  // internal

template<typename _Type>
struct is_contiguous_iterator
{
    static constexpr bool value =
        ( is_random_access_iterator<_Type>::value          &&
          ( std::is_pointer<_Type>::value                  ||
            internal::has_contiguous_tag<_Type>::value ) );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_contiguous_iterator_v
    //   variable template: value of
    // is_contiguous_iterator<_Type>.
    template<typename _Type>
    constexpr bool is_contiguous_iterator_v =
        is_contiguous_iterator<_Type>::value;
#endif


// ===========================================================================
// II.  Iterator Category Extraction
// ===========================================================================

NS_INTERNAL

    // safe_iterator_category
    //   helper: extracts iterator_category, or void on
    // mismatch.
    template<typename _Iter,
             typename = void>
    struct safe_iterator_category
    {
        using type = void;
    };

    template<typename _Iter>
    struct safe_iterator_category<_Iter, void_t<
        typename std::iterator_traits<
            _Iter>::iterator_category>>
    {
        using type = typename std::iterator_traits<
            _Iter>::iterator_category;
    };

    template<typename _Iter>
    using safe_iterator_category_t =
        typename safe_iterator_category<_Iter>::type;

    // safe_begin_iterator
    //   helper: extracts the iterator type from begin(),
    // or void.
    template<typename _C,
             typename = void>
    struct safe_begin_iterator
    {
        using type = void;
    };

    template<typename _C>
    struct safe_begin_iterator<_C, void_t<
        decltype(std::begin(std::declval<_C&>()))
    >>
    {
        using type =
            decltype(std::begin(std::declval<_C&>()));
    };

    template<typename _C>
    using safe_begin_iterator_t =
        typename safe_begin_iterator<_C>::type;

NS_END  // internal

// iterator_category_of
//   trait: extracts the iterator category of a container's
// begin() iterator, yielding void if unavailable.
template<typename _Container>
struct iterator_category_of
{
private:
    using cleaned    = clean_t<_Container>;
    using _IterType =
        internal::safe_begin_iterator_t<cleaned>;

public:
    using type =
        internal::safe_iterator_category_t<_IterType>;
};

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
    // iterator_category_of_t
    //   alias: convenience for
    // iterator_category_of<_Container>::type.
    template<typename _Container>
    using iterator_category_of_t =
        typename iterator_category_of<_Container>::type;
#endif


// ===========================================================================
// III. Container-Level Iterability
// ===========================================================================

// is_iterable
//   trait: true if begin(c) and end(c) are well-formed.
template<typename _Type,
         typename = void>
struct is_iterable : std::false_type
{};

template<typename _Type>
struct is_iterable<_Type, void_t<
    decltype(std::begin(std::declval<_Type&>())),
    decltype(std::end(std::declval<_Type&>()))
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_iterable_v
    //   variable template: value of is_iterable<_Type>.
    template<typename _Type>
    constexpr bool is_iterable_v = is_iterable<_Type>::value;
#endif

// is_input_iterable
//   trait: true if container provides at least input
// iterators.
template<typename _Type,
         typename = void>
struct is_input_iterable : std::false_type
{};

template<typename _Type>
struct is_input_iterable<_Type,
    typename std::enable_if<
        is_iterable<_Type>::value                                          &&
        is_input_iterator<
            internal::safe_begin_iterator_t<_Type>>::value
    >::type> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_input_iterable_v
    //   variable template: value of is_input_iterable<_Type>.
    template<typename _Type>
    constexpr bool is_input_iterable_v =
        is_input_iterable<_Type>::value;
#endif

// is_output_iterable
//   trait: true if container provides output iterators.
template<typename _Type,
         typename = void>
struct is_output_iterable : std::false_type
{};

template<typename _Type>
struct is_output_iterable<_Type,
    typename std::enable_if<
        is_iterable<_Type>::value                                          &&
        is_output_iterator<
            internal::safe_begin_iterator_t<_Type>>::value
    >::type> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_output_iterable_v
    //   variable template: value of is_output_iterable<_Type>.
    template<typename _Type>
    constexpr bool is_output_iterable_v =
        is_output_iterable<_Type>::value;
#endif

// is_forward_iterable
//   trait: true if container provides at least forward
// iterators.
template<typename _Type,
         typename = void>
struct is_forward_iterable : std::false_type
{};

template<typename _Type>
struct is_forward_iterable<_Type,
    typename std::enable_if<
        is_iterable<_Type>::value                                          &&
        is_forward_iterator<
            internal::safe_begin_iterator_t<_Type>>::value
    >::type> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_forward_iterable_v
    //   variable template: value of is_forward_iterable<_Type>.
    template<typename _Type>
    constexpr bool is_forward_iterable_v =
        is_forward_iterable<_Type>::value;
#endif

// is_bidirectional_iterable
//   trait: true if container provides bidirectional
// iterators.
template<typename _Type,
         typename = void>
struct is_bidirectional_iterable : std::false_type
{};

template<typename _Type>
struct is_bidirectional_iterable<_Type,
    typename std::enable_if<
        is_iterable<_Type>::value                                          &&
        is_bidirectional_iterator<
            internal::safe_begin_iterator_t<_Type>>::value
    >::type> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_bidirectional_iterable_v
    //   variable template: value of
    // is_bidirectional_iterable<_Type>.
    template<typename _Type>
    constexpr bool is_bidirectional_iterable_v =
        is_bidirectional_iterable<_Type>::value;
#endif

// is_random_access_iterable
//   trait: true if container provides random-access
// iterators.
template<typename _Type,
         typename = void>
struct is_random_access_iterable : std::false_type
{};

template<typename _Type>
struct is_random_access_iterable<_Type,
    typename std::enable_if<
        is_iterable<_Type>::value                                          &&
        is_random_access_iterator<
            internal::safe_begin_iterator_t<_Type>>::value
    >::type> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_random_access_iterable_v
    //   variable template: value of
    // is_random_access_iterable<_Type>.
    template<typename _Type>
    constexpr bool is_random_access_iterable_v =
        is_random_access_iterable<_Type>::value;
#endif

// is_contiguous_iterable
//   trait: true if container provides contiguous iterators
// (raw pointer or contiguous_iterator_tag).
template<typename _Type,
         typename = void>
struct is_contiguous_iterable : std::false_type
{};

template<typename _Type>
struct is_contiguous_iterable<_Type,
    typename std::enable_if<
        is_iterable<_Type>::value                                          &&
        is_contiguous_iterator<
            internal::safe_begin_iterator_t<_Type>>::value
    >::type> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_contiguous_iterable_v
    //   variable template: value of is_contiguous_iterable<_Type>.
    template<typename _Type>
    constexpr bool is_contiguous_iterable_v =
        is_contiguous_iterable<_Type>::value;
#endif


// ===========================================================================
// IV.  Const / Reverse Iteration Detection
// ===========================================================================

// --- const iteration ---

D_TYPE_TRAIT_TRUE(has_cbegin,
    decltype(std::declval<const _Type&>().cbegin()))

D_TYPE_TRAIT_TRUE(has_cend,
    decltype(std::declval<const _Type&>().cend()))

// has_const_iteration
//   trait: true if container supports const iteration via
// cbegin()/cend().
template<typename _Type>
struct has_const_iteration
{
private:
    using cleaned = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_cbegin<cleaned>::value  &&
          has_cend<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_const_iteration_v
    //   variable template: value of has_const_iteration<_Type>.
    template<typename _Type>
    constexpr bool has_const_iteration_v =
        has_const_iteration<_Type>::value;
#endif

// --- reverse iteration ---

D_TYPE_TRAIT_TRUE(has_rbegin,
    decltype(std::declval<_Type&>().rbegin()))

D_TYPE_TRAIT_TRUE(has_rend,
    decltype(std::declval<_Type&>().rend()))

D_TYPE_TRAIT_TRUE(has_crbegin,
    decltype(std::declval<const _Type&>().crbegin()))

D_TYPE_TRAIT_TRUE(has_crend,
    decltype(std::declval<const _Type&>().crend()))

// has_reverse_iteration
//   trait: true if container supports reverse iteration
// via rbegin()/rend().
template<typename _Type>
struct has_reverse_iteration
{
private:
    using cleaned = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_rbegin<cleaned>::value &&
          has_rend<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_reverse_iteration_v
    //   variable template: value of has_reverse_iteration<_Type>.
    //
    //   Previously this alias was supplied by a duplicate
    // definition over in container_traits.hpp; that duplicate
    // has been removed in favour of this canonical home, which
    // sits next to the struct it aliases.
    template<typename _Type>
    constexpr bool has_reverse_iteration_v =
        has_reverse_iteration<_Type>::value;
#endif

// has_const_reverse_iteration
//   trait: true if container supports const reverse
// iteration via crbegin()/crend().
template<typename _Type>
struct has_const_reverse_iteration
{
private:
    using cleaned = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_crbegin<cleaned>::value &&
          has_crend<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_const_reverse_iteration_v
    //   variable template: value of
    // has_const_reverse_iteration<_Type>.
    template<typename _Type>
    inline constexpr bool has_const_reverse_iteration_v = has_const_reverse_iteration<_Type>::value;
#endif


// ===========================================================================
// V.   Iterator Compatibility
// ===========================================================================

// iterators_compatible
//   trait: true if two containers provide iterators over
// the same value_type.
template<typename _A,
         typename _B,
         typename = void>
struct iterators_compatible : std::false_type
{};

template<typename _A,
         typename _B>
struct iterators_compatible<_A, _B, void_t<
    typename _A::value_type,
    typename _B::value_type
>> : std::is_same<
         typename _A::value_type,
         typename _B::value_type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // iterators_compatible_v
    //   variable template: value of
    // iterators_compatible<_A, _B>.
    template<typename _A,
             typename _B>
    constexpr bool iterators_compatible_v =
        iterators_compatible<_A, _B>::value;
#endif


// ===========================================================================
// VI.  iterator_level Enum
// ===========================================================================

// iterator_level
//   enum: classifies the strongest iterator category a
// container provides.
enum class iterator_level
{
    none,
    input,
    output,
    forward,
    bidirectional,
    random_access,
    contiguous
};

NS_INTERNAL

    // iterator_level_helper
    //   trait: priority cascade resolving the strongest
    // iteration category supported by a container.
    template<typename _Type>
    struct iterator_level_helper
    {
    private:
        using cleaned = clean_t<_Type>;

    public:
        static constexpr iterator_level value =
            is_contiguous_iterable<cleaned>::value
                ? iterator_level::contiguous

            : is_random_access_iterable<cleaned>::value
                ? iterator_level::random_access

            : is_bidirectional_iterable<cleaned>::value
                ? iterator_level::bidirectional

            : is_forward_iterable<cleaned>::value
                ? iterator_level::forward

            : is_output_iterable<cleaned>::value
                ? iterator_level::output

            : is_input_iterable<cleaned>::value
                ? iterator_level::input

            : iterator_level::none;
    };

NS_END  // internal

// container_iterator_level
//   trait: determines the strongest iterator category the
// container provides.
template<typename _Type>
struct container_iterator_level
{
    static constexpr iterator_level value =
        internal::iterator_level_helper<_Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // container_iterator_level_v
    //   variable template: value of
    // container_iterator_level<_Type>.
    template<typename _Type>
    constexpr iterator_level container_iterator_level_v =
        container_iterator_level<_Type>::value;
#endif


// ===========================================================================
// VII. Combined Classification
// ===========================================================================

// container_iterator_class
//   struct: complete iterator classification of a container
// type.
template<typename _Type>
struct container_iterator_class
{
    // iterability by category
    static constexpr bool is_iter =
        is_iterable<_Type>::value;
    static constexpr bool input_iter =
        is_input_iterable<_Type>::value;
    static constexpr bool output_iter =
        is_output_iterable<_Type>::value;
    static constexpr bool forward_iter =
        is_forward_iterable<_Type>::value;
    static constexpr bool bidir_iter =
        is_bidirectional_iterable<_Type>::value;
    static constexpr bool random_access_iter =
        is_random_access_iterable<_Type>::value;
    static constexpr bool contiguous_iter =
        is_contiguous_iterable<_Type>::value;

    // iteration variants
    static constexpr bool has_const_iter =
        has_const_iteration<_Type>::value;
    static constexpr bool has_reverse_iter =
        has_reverse_iteration<_Type>::value;
    static constexpr bool has_const_reverse_iter =
        has_const_reverse_iteration<_Type>::value;

    // strongest category
    static constexpr iterator_level level =
        container_iterator_level<_Type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_ITERATOR_TRAITS_