/******************************************************************************
* djinterp [container]                                    content_equality.hpp
*
*   The value-level companion to the comparison profile: the content equalities
* of the model, computed over two container VALUES.  Where the profile compares
* TYPES, this asks whether two containers hold the same content, and at which
* rung of the content hierarchy they agree.
*
*   The hierarchy, finest to coarsest, each coarser than the last:
*
*     =str  structural - same nesting, same positions, same values.
*     =seq  sequential - same FRONTIER: the same leaves, in order, nesting
*                        flattened away.
*     =bag  multiset   - the same leaves with the same multiplicities, in any
*                        order.
*     =set  support    - the same DISTINCT leaves, multiplicities and order
*                        forgotten.
*
*   =str implies =seq implies =bag implies =set.  Each forgets something the
* finer one keeps: shape, then order, then count.  All are taken relative to an
* element relation on the leaf type - here, the leaf's == .
*
*   The FRONTIER descends nesting to the leaves: a container of containers
* expands, a plain container yields its elements, and a text buffer (a string,
* carrying c_str()) is treated as a leaf atom rather than a sequence of
* characters.  A type's NATIVE rung is fixed by its discipline; content_equal
* compares two containers at the COARSER of their natives, the finest rung both
* can be held to.
*
*   COST.  The frontier is materialised and the bag/set tests run in the
* straightforward quadratic form - correctness and clarity over an ordering or
* hashing the leaf type may not offer.  Every equality requires only leaf == .
*
*   PORTABILITY:
*   C++11 baseline.  These are runtime algorithms (they allocate the frontier);
* they are ordinary function templates, not constexpr.
*
*
* path:      /inc/djinterp/core/container/content_equality.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_CONTENT_EQUALITY_
#define DJINTERP_CONTENT_EQUALITY_ 1

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"                          // clean_t, NS_*, feature macros
#include "../meta/trait_detect.hpp"                 // D_VOID_T
#include "./traits/container_comparison_traits.hpp" // content_level, native level
#include "./traits/element_relation_traits.hpp"     // element_type_of_t
#include "./traits/iterable_container_traits.hpp"    // is_iterable_container


NS_DJINTERP


// ===========================================================================
// I.   Frontier (leaves in order)
// ===========================================================================

NS_INTERNAL

    // content_has_c_str_helper
    //   helper: detects a c_str() accessor - marks a text buffer, which the
    // frontier treats as a leaf rather than descending into its characters.
    template<typename _Elem, typename = void>
    struct content_has_c_str_helper : std::false_type {};
    template<typename _Elem>
    struct content_has_c_str_helper<_Elem,
        D_VOID_T<decltype(std::declval<const _Elem&>().c_str())>>
        : std::true_type {};

    // recurse_into_element
    //   helper: whether the frontier descends into an element - true for a
    // nested container, false for a leaf or a text buffer.
    template<typename _Elem>
    struct recurse_into_element
        : std::integral_constant<bool,
                is_iterable_container<clean_t<_Elem>>::value
             && !content_has_c_str_helper<clean_t<_Elem>>::value>
    {};

    // leaf_type_helper
    //   helper: the ultimate leaf type, descending nesting until a non-container
    // (or text buffer) element is reached.
    template<typename _Container,
             bool _Recurse =
                 recurse_into_element<element_type_of_t<_Container>>::value>
    struct leaf_type_helper
    {
        using type = element_type_of_t<_Container>;
    };

    template<typename _Container>
    struct leaf_type_helper<_Container, true>
    {
        using type = typename leaf_type_helper<
            clean_t<element_type_of_t<_Container>>>::type;
    };

NS_END  // internal

// leaf_type_of_t
//   type: the leaf (atom) type at the bottom of a container's nesting.
template<typename _Container>
using leaf_type_of_t =
    typename internal::leaf_type_helper<clean_t<_Container>>::type;

NS_INTERNAL

    // leaf_collector
    //   helper: appends a container's leaves, in order, to an output sequence -
    // pushing each element at the leaf level, recursing at a nested level.
    template<typename _Container,
             bool _Recurse =
                 recurse_into_element<element_type_of_t<_Container>>::value>
    struct leaf_collector
    {
        template<typename _Out>
        static void collect(const _Container& _container, _Out& _out)
        {
            for (const auto& _element : _container)
            {
                _out.push_back(_element);
            }
        }
    };

    template<typename _Container>
    struct leaf_collector<_Container, true>
    {
        template<typename _Out>
        static void collect(const _Container& _container, _Out& _out)
        {
            for (const auto& _element : _container)
            {
                leaf_collector<clean_t<element_type_of_t<_Container>>>
                    ::collect(_element, _out);
            }
        }
    };

NS_END  // internal

// frontier_of
//   function: the frontier of a container - its leaves in position order, with
// all nesting flattened away.
template<typename _Container>
std::vector<leaf_type_of_t<_Container>>
frontier_of(const _Container& _container)
{
    std::vector<leaf_type_of_t<_Container>> _leaves;
    internal::leaf_collector<clean_t<_Container>>::collect(_container, _leaves);
    return _leaves;
}


// ===========================================================================
// II.  The content equalities
// ===========================================================================

NS_INTERNAL

    // count_of / contains_of: multiplicity and membership by leaf == .
    template<typename _Sequence, typename _Value>
    std::size_t count_of(const _Sequence& _seq, const _Value& _value)
    {
        std::size_t _n = 0;
        for (const auto& _element : _seq)
        {
            if (_element == _value)
            {
                ++_n;
            }
        }

        return _n;
    }

    template<typename _Sequence, typename _Value>
    bool contains_of(const _Sequence& _seq, const _Value& _value)
    {
        for (const auto& _element : _seq)
        {
            if (_element == _value)
            {
                return true;
            }
        }

        return false;
    }

NS_END  // internal

// sequential_content_equal
//   function: =seq - the two frontiers are equal leaf-for-leaf, in order.
template<typename _Left,
         typename _Right>
bool
sequential_content_equal(const _Left& _left, const _Right& _right)
{
    const std::vector<leaf_type_of_t<_Left>>  _fl = frontier_of(_left);
    const std::vector<leaf_type_of_t<_Right>> _fr = frontier_of(_right);

    if (_fl.size() != _fr.size())
    {
        return false;
    }

    for (std::size_t _i = 0; _i < _fl.size(); ++_i)
    {
        if (!(_fl[_i] == _fr[_i]))
        {
            return false;
        }
    }

    return true;
}

// multiset_content_equal
//   function: =bag - the frontiers hold the same leaves with the same
// multiplicities, order forgotten.
template<typename _Left,
         typename _Right>
bool
multiset_content_equal(const _Left& _left, const _Right& _right)
{
    const std::vector<leaf_type_of_t<_Left>>  _fl = frontier_of(_left);
    const std::vector<leaf_type_of_t<_Right>> _fr = frontier_of(_right);

    if (_fl.size() != _fr.size())
    {
        return false;
    }

    // equal sizes and matching counts across _fl's values leave no room for an
    // unmatched value in _fr, so one direction suffices.
    for (const auto& _value : _fl)
    {
        if (internal::count_of(_fl, _value) != internal::count_of(_fr, _value))
        {
            return false;
        }
    }

    return true;
}

// set_content_equal
//   function: =set - the frontiers have the same distinct leaves, multiplicity
// and order forgotten.
template<typename _Left,
         typename _Right>
bool
set_content_equal(const _Left& _left, const _Right& _right)
{
    const std::vector<leaf_type_of_t<_Left>>  _fl = frontier_of(_left);
    const std::vector<leaf_type_of_t<_Right>> _fr = frontier_of(_right);

    for (const auto& _value : _fl)
    {
        if (!internal::contains_of(_fr, _value))
        {
            return false;
        }
    }

    for (const auto& _value : _fr)
    {
        if (!internal::contains_of(_fl, _value))
        {
            return false;
        }
    }

    return true;
}

NS_INTERNAL

    // structural_equal_helper
    //   helper: =str - a co-recursion that preserves shape.  Both sides at the
    // leaf level compare elements directly; both nested recurse pair-by-pair; a
    // nesting mismatch is unequal shape and needs no element comparison.
    template<typename _Left,
             typename _Right,
             bool _RecurseLeft =
                 recurse_into_element<element_type_of_t<_Left>>::value,
             bool _RecurseRight =
                 recurse_into_element<element_type_of_t<_Right>>::value>
    struct structural_equal_helper;

    // both leaves
    template<typename _Left, typename _Right>
    struct structural_equal_helper<_Left, _Right, false, false>
    {
        static bool equal(const _Left& _left, const _Right& _right)
        {
            auto _il = std::begin(_left);
            auto _ir = std::begin(_right);
            const auto _el = std::end(_left);
            const auto _er = std::end(_right);

            for (; _il != _el && _ir != _er; ++_il, ++_ir)
            {
                if (!(*_il == *_ir))
                {
                    return false;
                }
            }

            return ( _il == _el && _ir == _er );
        }
    };

    // both nested
    template<typename _Left, typename _Right>
    struct structural_equal_helper<_Left, _Right, true, true>
    {
        static bool equal(const _Left& _left, const _Right& _right)
        {
            auto _il = std::begin(_left);
            auto _ir = std::begin(_right);
            const auto _el = std::end(_left);
            const auto _er = std::end(_right);

            for (; _il != _el && _ir != _er; ++_il, ++_ir)
            {
                if (!structural_equal_helper<
                        clean_t<element_type_of_t<_Left>>,
                        clean_t<element_type_of_t<_Right>>>
                            ::equal(*_il, *_ir))
                {
                    return false;
                }
            }

            return ( _il == _el && _ir == _er );
        }
    };

    // nesting mismatch -> different shape
    template<typename _Left, typename _Right>
    struct structural_equal_helper<_Left, _Right, true, false>
    {
        static bool equal(const _Left&, const _Right&) { return false; }
    };

    template<typename _Left, typename _Right>
    struct structural_equal_helper<_Left, _Right, false, true>
    {
        static bool equal(const _Left&, const _Right&) { return false; }
    };

NS_END  // internal

// structural_content_equal
//   function: =str - same nesting, same positions, same values.
template<typename _Left,
         typename _Right>
bool
structural_content_equal(const _Left& _left, const _Right& _right)
{
    return internal::structural_equal_helper<
        clean_t<_Left>, clean_t<_Right>>::equal(_left, _right);
}


// ===========================================================================
// III. Comparison at the native level
// ===========================================================================

NS_INTERNAL

    // content_equal_helper: dispatch to the equality of a given content level.
    template<typename _Left, typename _Right>
    bool content_equal_helper(const _Left& _left, const _Right& _right,
        std::integral_constant<content_level, content_level::str>)
    {
        return structural_content_equal(_left, _right);
    }

    template<typename _Left, typename _Right>
    bool content_equal_helper(const _Left& _left, const _Right& _right,
        std::integral_constant<content_level, content_level::seq>)
    {
        return sequential_content_equal(_left, _right);
    }

    template<typename _Left, typename _Right>
    bool content_equal_helper(const _Left& _left, const _Right& _right,
        std::integral_constant<content_level, content_level::bag>)
    {
        return multiset_content_equal(_left, _right);
    }

    template<typename _Left, typename _Right>
    bool content_equal_helper(const _Left& _left, const _Right& _right,
        std::integral_constant<content_level, content_level::set>)
    {
        return set_content_equal(_left, _right);
    }

    // content_level::none - no shared content rung; not content-equal.
    template<typename _Left, typename _Right>
    bool content_equal_helper(const _Left&, const _Right&,
        std::integral_constant<content_level, content_level::none>)
    {
        return false;
    }

NS_END  // internal

// content_equal
//   function: whether two containers hold the same content at the COARSER of
// their native rungs - the finest level both can be held to.  A set and a
// sequence with the same support compare equal (at =set); two sequences by
// their frontier (at =seq); two nested containers by shape (at =str).
template<typename _Left,
         typename _Right>
bool
content_equal(const _Left& _left, const _Right& _right)
{
    return internal::content_equal_helper(_left, _right,
        std::integral_constant<content_level,
            content_level_coarser(
                native_content_level_of<clean_t<_Left>>::value,
                native_content_level_of<clean_t<_Right>>::value)>{});
}


NS_END  // djinterp


#endif  // DJINTERP_CONTENT_EQUALITY_
