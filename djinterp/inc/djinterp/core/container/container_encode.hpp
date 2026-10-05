/*******************************************************************************
* djinterp [core]                                           container_encode.hpp
*
*   The CONTAINER encoder: enc_tau lifted from a leaf to a whole container,
* the
* recursion the model prescribes (containers.tex, Serialization, "Building a
* faithful encoder").  Derived from encode.hpp - a leaf is written by that
* header's enc_tau, a node by the encoder of its own type, recursively - so
* the
* bits of a container are the bits of its SIZE followed by the concatenated
* bits
* of its COMPONENTS, in container order.
*
*   THE RECURSION ON STRUCTURE (T = tau + F[T]):
*   A container factors as a leaf frontier plus optional sub-containers
* (containers.tex, Structure).  `encode_container_into` mirrors that frontier:
*     1. write |c| - the element count, as a fixed-width leading LENGTH
*   PREFIX;
*     2. write each component in turn - an ITERABLE element recurses through
*   this
*        same container encoder, any other element is a leaf written by
*        encode.hpp's enc_tau.
* The per-element choice is the sum T = tau + F[T] made operational: the node
* summand recurses, the leaf summand bottoms out.
*
*   UNIQUELY DECODABLE:
*   The leading count is the model's LENGTH-PREFIX strategy - "the size,
* encoded
* first, bounds the element count" - so a decoder knows exactly how many
* components follow and where this container's field ends, even nested inside
* another.  The count is written through encode.hpp's fixed-width leaf writer
* (8 bytes, big-endian: `encode_length_type`), itself uniquely decodable.
*
*   WHAT THE STREAM CARRIES (and what it need not):
*   Per the model, faithfulness at the native level fixes what the stream
* preserves.  This encoder carries the SIZE (the count prefix), the ELEMENTS
* (every leaf, and every nested shape by its own recursive count), and ORDER
* AND
* SHAPE (elements are emitted in traversal order; each nesting level re-emits
* its
* own count). It does NOT separately carry discipline "beyond this -
* sortedness,
* capacity, keying" - those are invariants a decoder re-establishes on insert,
* exactly as an overlay is checked rather than stored.  For an ORDER-BLIND
* discipline (a set, a multiset) the traversal order is one enumeration of the
* bag; the container reasserts its own order as elements are re-inserted, so
* the
* round trip is faithful at that discipline's own (coarser) native level.
*
*   TEXT BUFFERS:
*   A string is iterable, so it is encoded generically as a length-prefixed
* run
* of its characters (each a leaf) - a faithful, uniquely-decodable encoding.
* This differs from the leaf-atom treatment content_equality.hpp gives a text
* buffer; here the generic container path already serialises it correctly, so
* no
* special case is made. A caller wanting an opaque string leaf gives the
* string
* type a member `encode_into` (encode.hpp), which then takes precedence.
*
*   OUT OF SCOPE - CYCLES:
*   The recursion terminates only on the finite trees of positional nesting. A
* node container that SHARES or CYCLES (containers.tex, Serialization, "Cyclic
* containers") would not halt under a purely recursive encoder; serialising
* the
* graph by labelling nodes and encoding back-pointers is a separate facility,
* not
* provided here.
*
*   PORTABILITY:
*   C++11 baseline. Runtime function templates (they allocate/append),
* matching
* content_equality.hpp and byte_size.hpp.
*
*
* path:      /inc/djinterp/core/container/container_encode.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONTAINER_ENCODE_HPP
#define DJINTERP_CONTAINER_CONTAINER_ENCODE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"                         // clean_t, NS_*, feature macros
#include "../meta/trait_detect.hpp"                // D_VOID_T, D_TYPE_TRAIT_VALUE_BOOL
#include "../binary/encode.hpp"                    // byte, byte_string, enc_tau (encode_into)
#include "./traits/iterable_container_traits.hpp"  // is_iterable_container
#include "./traits/element_relation_traits.hpp"    // element_type_of_t


NS_DJINTERP


// ===========================================================================
// I.   Recursive encodability
// ===========================================================================
//   A type is encodable when it is a leaf enc_tau (encode.hpp) or an iterable
// container whose element is itself encodable.  The mutual recursion peels one
// container layer per step and bottoms out at a leaf (or a non-container, whose
// element type is void and encodes as neither) - bounded by a depth budget
// against a pathologically self-referential type, as container_depth is.

NS_INTERNAL

    // encodable_probe
    //   helper: the recursive encodability predicate, `Depth`-bounded. The
    // primary is the STOP case - a leaf, a non-container, or an exhausted
    // budget
    // - and reports leaf-encodability directly; the `Recurse` specialization
    // is taken only for a non-leaf iterable container with budget left, and
    // defers to its element type. Gating the recursion on the specialization
    // is what keeps the deeper level from being instantiated (and a leaf/void
    // from referring to itself) when no descent is called for.
    template<typename Type,
             std::size_t Depth,
             bool Recurse =
                 ( ( Depth > 0 )
                && is_iterable_container<Type>::value
                && !is_leaf_encodable<Type>::value )>
    struct encodable_probe
        : std::integral_constant<bool, is_leaf_encodable<Type>::value>
    {};

    template<typename Type,
             std::size_t Depth>
    struct encodable_probe<Type, Depth, true>
        : std::integral_constant<bool,
              encodable_probe<
                  clean_t<element_type_of_t<Type>>,
                  ( Depth - 1 )
              >::value>
    {};

NS_END  // internal

// is_encodable
//   trait: true iff `Type` can be encoded - a leaf, or an iterable container
// of encodable elements (to a default recursion budget of 32).
template<typename Type>
struct is_encodable
    : std::integral_constant<bool,
          internal::encodable_probe<clean_t<Type>, 32>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_encodable)

// is_encodable_container
//   trait: true iff `Type` is an iterable container that is encodable - the
// gate on the container encoder proper.
template<typename Type>
struct is_encodable_container
    : std::integral_constant<bool,
          ( is_iterable_container<clean_t<Type>>::value
         && is_encodable<clean_t<Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_encodable_container)


// ===========================================================================
// II.  Element count (the size |c|)
// ===========================================================================

NS_INTERNAL

    // encode_has_size_method
    //   helper: detects a const size() accessor - present on most containers,
    // absent on std::forward_list (counted by iterator distance instead).
    template<typename Type,
             typename = void>
    struct encode_has_size_method : std::false_type
    {};

    // encode_has_size_method specialization
    //   helper: the detected case -- selected when the type exposes the member
    // function `size()`.
    template<typename Type>
    struct encode_has_size_method<Type,
        D_VOID_T<decltype(std::declval<const clean_t<Type>&>().size())>>
        : std::true_type
    {};

    // encode_element_count
    //   helper: the number of components of `_container` - size() when present,
    // else the iterator distance. This is the |c| written as the length prefix.
    template<typename Container>
    std::size_t
    encode_element_count(const Container& _container, std::true_type)
    {
        return static_cast<std::size_t>(_container.size());
    }

    template<typename Container>
    std::size_t
    encode_element_count(const Container& _container, std::false_type)
    {
        return static_cast<std::size_t>(
            std::distance(std::begin(_container), std::end(_container)));
    }

    template<typename Container>
    std::size_t
    encode_element_count(const Container& _container)
    {
        return encode_element_count(_container,
            encode_has_size_method<Container>{});
    }

NS_END  // internal


// ===========================================================================
// III. The container encoder (forward declaration)
// ===========================================================================
//   Declared before the element dispatcher, which calls it for a nested node;
// defined below, using that dispatcher for its own elements.

// encode_container_into
//   function: write `_container` into `_sink` - its size, then its components.
template<typename Sink,
         typename Container>
void
encode_container_into(Sink& _sink, const Container& _container);


// ===========================================================================
// IV.  Per-element dispatch (the sum T = tau + F[T])
// ===========================================================================

NS_INTERNAL

    // element_encoder
    //   helper: encode one component. The primary is the LEAF summand (tau) -
    // it defers to encode.hpp's enc_tau; the specialization is the NODE
    // summand (F[T]) - an iterable element that recurses through the container
    // encoder.
    template<typename Elem,
             bool Nested = is_iterable_container<Elem>::value>
    struct element_encoder
    {
        template<typename Sink>
        static void into(Sink& _sink, const Elem& _element)
        {
            // leaf: write through the element encoder of encode.hpp
            encode_into(_sink, _element);

            return;
        }
    };

    template<typename Elem>
    struct element_encoder<Elem, true>
    {
        template<typename Sink>
        static void into(Sink& _sink, const Elem& _element)
        {
            // node: recurse into the sub-container
            encode_container_into(_sink, _element);

            return;
        }
    };

NS_END  // internal


// ===========================================================================
// V.   The container encoder (definition)
// ===========================================================================

template<typename Sink,
         typename Container>
void
encode_container_into(Sink& _sink, const Container& _container)
{
    // 1. the length prefix |c| - a fixed-width leaf, so the component count and
    //    this container's field boundary are recoverable (uniquely decodable)
    encode_into(_sink,
        static_cast<encode_length_type>(
            internal::encode_element_count(_container)));

    // 2. the components, in traversal order - each dispatched leaf-or-node
    for (const auto& _element : _container)
    {
        internal::element_encoder<clean_t<element_type_of_t<Container>>>
            ::into(_sink, _element);
    }

    return;
}


// ===========================================================================
// VI.  Unified entry + convenience
// ===========================================================================

// encode_into (container overload)
//   function: fold the leaf `encode_into` and this container encoder into one
// name, so `encode_into(sink, x)` writes any encodable `x`. Selected only for
// an iterable container that is NOT itself a leaf (a member `encode_into`,
// being leaf-encodable, therefore takes precedence).
template<typename Sink,
         typename Container,
         typename std::enable_if<
             ( is_iterable_container<clean_t<Container>>::value &&
               !is_leaf_encodable<clean_t<Container>>::value ),
             int>::type = 0>
void
encode_into(Sink& _sink, const Container& _container)
{
    encode_container_into(_sink, _container);

    return;
}

// encode_container
//   function: the container encoder as a total map to B* - allocates a fresh
// `byte_string`, writes `_container` into it, and returns it.
template<typename Container>
byte_string
encode_container(const Container& _container)
{
    byte_string _out;
    encode_container_into(_out, _container);

    return _out;
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONTAINER_ENCODE_HPP
