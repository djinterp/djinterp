/******************************************************************************
* djinterp [container]                              iterator_category_traits.hpp
*
*   The CATEGORY refinement of the iterability axis: given an iterable container,
* what traversal strength does its iterator grant?  Where iterable_container_traits
* answers whether a begin()/end() traversal exists (and in which mode), this reads
* the category of that traversal - forward, bidirectional, or random-access - and
* whether the storage is contiguous (a data() accessor).
*
*   The category is that of the container's begin() iterator, taken through
* std::iterator_traits so a pointer iterator (as std::array yields) resolves to
* random-access.  The probe is gated on iterability, so the category is never
* extracted from a non-iterator - std::iterator_traits<void> is ill-formed before
* C++17, and the gate keeps it from ever being named.
*
*   These are the signals the sequential-layout classification consumes (see
* ordered_container.hpp): contiguous storage, a random-access iterator, and so on
* distinguish an array from a deque from a list.  The category strengthens down a
* chain - random-access implies bidirectional implies forward - which the
* iterator_category_kind summary reports as the strongest satisfied.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language as the rest do.
*
*
* path:      /inc/djinterp/core/container/traits/iterator_category_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_ITERATOR_CATEGORY_TRAITS_
#define DJINTERP_ITERATOR_CATEGORY_TRAITS_ 1

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"                       // clean_t, NS_*, feature macros
#include "../../meta/trait_detect.hpp"              // D_TYPE_TRAIT_* detection macros
#include "./iterable_container_traits.hpp"          // is_iterable_container (the gate)


NS_DJINTERP


// ===========================================================================
// I.   Category detection (gated on iterability)
// ===========================================================================

NS_INTERNAL

    // begin_iterator_helper
    //   helper: the iterator type a container's begin() yields.  Named only where
    // the container is iterable, so begin() is known to exist.
    template<typename _Container>
    struct begin_iterator_helper
    {
        using type =
            decltype(std::begin(std::declval<clean_t<_Container>&>()));
    };

    // iter_category_helper
    //   helper: the iterator_category of an iterator, via std::iterator_traits
    // (so a raw pointer resolves to random-access).
    template<typename _Iter>
    struct iter_category_helper
    {
        using type =
            typename std::iterator_traits<_Iter>::iterator_category;
    };

    // category_derives_helper
    //   helper: for an ITERABLE container, whether its iterator's category derives
    // from _Tag.  The default (non-iterable) is false and instantiates neither of
    // the above, keeping std::iterator_traits away from a void iterator.
    template<typename _Container,
             typename _Tag,
             bool     _Iterable =
                 is_iterable_container<clean_t<_Container>>::value>
    struct category_derives_helper : std::false_type
    {};

    template<typename _Container,
             typename _Tag>
    struct category_derives_helper<_Container, _Tag, true>
        : std::is_base_of<_Tag,
              typename iter_category_helper<
                  typename begin_iterator_helper<_Container>::type>::type>
    {};

NS_END  // internal

// is_forward_iterable
//   trait: true iff the container's iterator is (at least) a forward iterator.
template<typename _Type>
struct is_forward_iterable
    : internal::category_derives_helper<_Type, std::forward_iterator_tag>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_forward_iterable)

// is_bidirectional_iterable
//   trait: true iff the container's iterator is (at least) bidirectional.
template<typename _Type>
struct is_bidirectional_iterable
    : internal::category_derives_helper<_Type, std::bidirectional_iterator_tag>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_bidirectional_iterable)

// is_random_access_iterable
//   trait: true iff the container's iterator is random-access.
template<typename _Type>
struct is_random_access_iterable
    : internal::category_derives_helper<_Type, std::random_access_iterator_tag>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_random_access_iterable)


// ===========================================================================
// II.  Storage-shape signals
// ===========================================================================

// has_data_accessor
//   trait: detects a data() accessor - the mark of contiguous storage.
D_TYPE_TRAIT_TRUE(has_data_accessor,
    decltype(std::declval<const clean_t<_Type>&>().data()))

// is_contiguous_iterable
//   trait: true iff the container is random-access AND exposes contiguous storage
// through data() - the strongest traversal an in-memory sequence can offer.
template<typename _Type>
struct is_contiguous_iterable
    : std::integral_constant<bool,
            has_data_accessor<clean_t<_Type>>::value
         && is_random_access_iterable<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_contiguous_iterable)


// ===========================================================================
// III. Category summary
// ===========================================================================

// iterator_category_kind
//   enum: the strongest category a container's iterator satisfies.
enum class iterator_category_kind
{
    none,           // not iterable
    input,          // iterable, but weaker than forward
    forward,        // forward
    bidirectional,  // bidirectional
    random_access   // random-access
};

// iterator_category_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
iterator_category_name(iterator_category_kind _k) noexcept
{
    return ( _k == iterator_category_kind::none          ? "none"
           : _k == iterator_category_kind::input         ? "input"
           : _k == iterator_category_kind::forward       ? "forward"
           : _k == iterator_category_kind::bidirectional ? "bidirectional"
           :                                               "random_access" );
}

// iterator_category_of
//   trait: classifies a container by the strongest category its iterator meets.
template<typename _Type>
struct iterator_category_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr iterator_category_kind value =
        ( !is_iterable_container<clean_type>::value )
              ? iterator_category_kind::none
      : (  is_random_access_iterable<clean_type>::value )
              ? iterator_category_kind::random_access
      : (  is_bidirectional_iterable<clean_type>::value )
              ? iterator_category_kind::bidirectional
      : (  is_forward_iterable<clean_type>::value )
              ? iterator_category_kind::forward
      :         iterator_category_kind::input;

    using type = std::integral_constant<iterator_category_kind, value>;
};

template<typename _Type>
using iterator_category_of_t = typename iterator_category_of<_Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr iterator_category_kind iterator_category_of_v =
        iterator_category_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr iterator_category_kind iterator_category_of_v =
        iterator_category_of<_Type>::value;
#endif


NS_END  // djinterp


#endif  // DJINTERP_ITERATOR_CATEGORY_TRAITS_
