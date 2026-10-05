/*******************************************************************************
* djinterp [core]                                           container_decode.hpp
*
*   The CONTAINER decoder: dec_tau lifted from a leaf to a whole container,
* the
* exact inverse of container_encode.hpp and the reconstruction half of the
* model's
* round trip (containers.tex, Serialization). Derived from decode.hpp - a leaf
* is
* read by that header's dec_tau, a node by the decoder of its own type,
* recursively - so a container is rebuilt by reading its SIZE, then that many
* COMPONENTS, appending each into a fresh container.
*
*   THE RECURSION ON STRUCTURE (T = tau + F[T]):
*   `decode_container` mirrors `encode_container_into` step for step:
*     1. read |c| - the leading LENGTH PREFIX, which bounds the component
*   count;
*     2. read |c| components in turn - an ITERABLE element recurses through
*   this
*        same container decoder, any other element is a leaf read by
*      decode.hpp's
*        dec_tau - and append each into the container being built.
* The length prefix is what makes step 2 finite and unambiguous: the decoder
* knows exactly how many components to read and where this field ends, even
* when
* nested inside another container.
*
*   A COPY THROUGH THE MEDIUM:
*   dec(enc(c)) is a container EQUAL to c at the serialised level, but NOT
* identical - its identity, allocator, and traversal machinery are made anew
* (containers.tex, Serialization). The rebuilt container is a fresh object,
* its
* cells freshly appended.
*
*   DISCIPLINE RE-ESTABLISHED, NOT READ:
*   Sortedness, uniqueness, keying and the like are not carried in the stream;
* they are invariants the target container reasserts AS COMPONENTS ARE
* APPENDED
* (containers.tex, Serialization: an invariant "the decoder may re-establish
* or
* verify rather than store").  Appending decoded elements into a set drops
* duplicates and re-sorts; into a sorted container, re-orders. So an
* order-blind
* or sorted discipline round-trips at its own native level without the encoder
* having stored the discipline.
*
*   RECONSTRUCTION REQUIREMENTS:
*   The target container must be DEFAULT-CONSTRUCTIBLE (a fresh empty one is
* built) and APPENDABLE - it must expose `push_back` (a sequence) or, failing
* that, `insert` (an associative container). `reserve` is used when present,
* and
* the reservation is capped by the reader's remaining bytes so a corrupt
* oversized
* length cannot force a runaway allocation.  Elements must be default-
* constructible too, per decode.hpp's `decode_result`.
*
*   PARTIALITY:
*   The first failure - a short read, or an element that fails to decode -
* propagates outward as a failed `decode_result`; no partially-built container
* escapes.  A caller checks `ok`.
*
*   OUT OF SCOPE - CYCLES:
*   As with the encoder, only the finite trees of positional nesting are
* rebuilt; a shared or cyclic node graph (containers.tex, Serialization,
* "Cyclic
* containers") needs label/back-pointer resolution not provided here.
*
*   PORTABILITY:
*   C++11 baseline.  Runtime function templates, matching decode.hpp.
*
*
* path:      /inc/djinterp/core/container/container_decode.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONTAINER_DECODE_HPP
#define DJINTERP_CONTAINER_CONTAINER_DECODE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"                       // clean_t, NS_*, feature macros
#include "../meta/trait_detect.hpp"              // D_VOID_T, D_TYPE_TRAIT_VALUE_BOOL
#include "../binary/decode.hpp"                             // byte_reader, decode_result, dec_tau (decode)
#include "./traits/iterable_container_traits.hpp"   // is_iterable_container
#include "./traits/element_relation_traits.hpp"     // element_type_of_t


NS_DJINTERP


// ===========================================================================
// I.   Recursive decodability
// ===========================================================================
//   Symmetric with is_encodable: a type is decodable when it is a leaf dec_tau
// (decode.hpp) or an iterable container whose element is itself decodable,
// bounded by the same depth budget.

NS_INTERNAL

    // decodable_probe
    //   helper: the recursive decodability predicate, `Depth`-bounded. The
    // primary is the STOP case - a leaf, a non-container, or an exhausted
    // budget
    // - and reports leaf-decodability directly; the `Recurse` specialization
    // is taken only for a non-leaf iterable container with budget left, and
    // defers to its element type. Gating the recursion on the specialization
    // is what keeps the deeper level from being instantiated (and a leaf/void
    // from referring to itself) when no descent is called for.
    template<typename Type,
             std::size_t Depth,
             bool Recurse =
                 ( ( Depth > 0 )
                && is_iterable_container<Type>::value
                && !is_leaf_decodable<Type>::value )>
    struct decodable_probe
        : std::integral_constant<bool, is_leaf_decodable<Type>::value>
    {};

    template<typename Type,
             std::size_t Depth>
    struct decodable_probe<Type, Depth, true>
        : std::integral_constant<bool,
              decodable_probe<
                  clean_t<element_type_of_t<Type>>,
                  ( Depth - 1 )
              >::value>
    {};

NS_END  // internal

// is_decodable
//   trait: true iff `Type` can be decoded - a leaf, or an iterable container
// of decodable elements (to a default recursion budget of 32).
template<typename Type>
struct is_decodable
    : std::integral_constant<bool,
          internal::decodable_probe<clean_t<Type>, 32>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_decodable)

// is_decodable_container
//   trait: true iff `Type` is an iterable container that is decodable - the
// gate on the container decoder proper.
template<typename Type>
struct is_decodable_container
    : std::integral_constant<bool,
          ( is_iterable_container<clean_t<Type>>::value
         && is_decodable<clean_t<Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_decodable_container)


// ===========================================================================
// II.  Reconstruction primitives (append + reserve)
// ===========================================================================

NS_INTERNAL

    // decode_has_push_back
    //   helper: detects `push_back(value_type)` - the sequence-append surface.
    template<typename Container,
             typename = void>
    struct decode_has_push_back : std::false_type
    {};

    // decode_has_push_back specialization
    //   helper: the detected case -- selected when the type exposes the member
    // function `push_back()`.
    template<typename Container>
    struct decode_has_push_back<Container,
        D_VOID_T<decltype(std::declval<Container&>().push_back(
            std::declval<typename clean_t<Container>::value_type>()))>>
        : std::true_type
    {};

    // decode_has_reserve
    //   helper: detects `reserve(size_t)` - a growable, out-of-line store
    // worth pre-sizing (a vector, an unordered container); harmlessly absent
    // elsewhere.
    template<typename Container,
             typename = void>
    struct decode_has_reserve : std::false_type
    {};

    // decode_has_reserve specialization
    //   helper: the detected case -- selected when the type exposes the member
    // function `reserve()`.
    template<typename Container>
    struct decode_has_reserve<Container,
        D_VOID_T<decltype(std::declval<Container&>().reserve(
            std::declval<std::size_t>()))>>
        : std::true_type
    {};

    // container_appender
    //   helper: append one decoded component. The primary uses push_back (a
    // sequence); the specialization falls back to insert (an associative
    // container). A container offering neither is not reconstructible - a hard
    // error here, correctly.
    template<typename Container,
             bool HasPushBack = decode_has_push_back<Container>::value>
    struct container_appender
    {
        template<typename Value>
        static void append(Container& _container, Value&& _value)
        {
            _container.push_back(static_cast<Value&&>(_value));

            return;
        }
    };

    template<typename Container>
    struct container_appender<Container, false>
    {
        template<typename Value>
        static void append(Container& _container, Value&& _value)
        {
            _container.insert(static_cast<Value&&>(_value));

            return;
        }
    };

    // maybe_reserve
    //   helper: pre-size a container that supports reserve; a no-op otherwise.
    template<typename Container>
    void maybe_reserve(Container& _container, std::size_t _hint, std::true_type)
    {
        _container.reserve(_hint);

        return;
    }

    template<typename Container>
    void maybe_reserve(Container&, std::size_t, std::false_type)
    {
        return;
    }

    template<typename Container>
    void maybe_reserve(Container& _container, std::size_t _hint)
    {
        maybe_reserve(_container, _hint,
            decode_has_reserve<Container>{});

        return;
    }

NS_END  // internal


// ===========================================================================
// III. The container decoder (forward declaration)
// ===========================================================================
//   Declared before the element dispatcher, which calls it for a nested node;
// defined below, using that dispatcher for its own elements.

// decode_container
//   function: read one container of type `Container` from the reader.
template<typename Container>
decode_result<clean_t<Container>>
decode_container(byte_reader& _reader);


// ===========================================================================
// IV.  Per-element dispatch (the sum T = tau + F[T])
// ===========================================================================

NS_INTERNAL

    // element_decoder
    //   helper: read one component. The primary is the LEAF summand (tau) - it
    // defers to decode.hpp's dec_tau; the specialization is the NODE summand
    // (F[T]) - an iterable element read by the container decoder, recursively.
    template<typename Elem,
             bool Nested = is_iterable_container<Elem>::value>
    struct element_decoder
    {
        static decode_result<Elem> from(byte_reader& _reader)
        {
            // leaf: read through the element decoder of decode.hpp
            return decode<Elem>(_reader);
        }
    };

    // element_decoder<Elem, true>
    //   helper: the case where `is_iterable_container<Elem>::value` is true.
    template<typename Elem>
    struct element_decoder<Elem, true>
    {
        static decode_result<Elem> from(byte_reader& _reader)
        {
            // node: recurse into the sub-container
            return decode_container<Elem>(_reader);
        }
    };

NS_END  // internal


// ===========================================================================
// V.   The container decoder (definition)
// ===========================================================================

template<typename Container>
decode_result<clean_t<Container>>
decode_container(byte_reader& _reader)
{
    using container_type = clean_t<Container>;
    using value_type     = clean_t<element_type_of_t<container_type>>;

    // 1. the length prefix |c| - fail the whole decode if it cannot be read
    decode_result<decode_length_type> _length =
        decode<decode_length_type>(_reader);

    if (!_length.ok)
    {
        return decode_failure<container_type>();
    }

    // a fresh, empty container - the rebuilt value is a new object
    container_type    _container;
    const std::size_t _count = static_cast<std::size_t>(_length.value);

    // pre-size where possible, but never trust a corrupt oversized count
    // beyond the bytes actually left to read (each component costs at least
    // one byte)
    internal::maybe_reserve(_container,
        ( _count < _reader.remaining() ) ? _count : _reader.remaining());

    // 2. read exactly |c| components, appending each; the first failure aborts
    for (std::size_t _i = 0; _i < _count; ++_i)
    {
        decode_result<value_type> _element =
            internal::element_decoder<value_type>::from(_reader);

        // a short or invalid component fails the whole container
        if (!_element.ok)
        {
            return decode_failure<container_type>();
        }

        internal::container_appender<container_type>::append(
            _container, static_cast<value_type&&>(_element.value));
    }

    return decode_success(static_cast<container_type&&>(_container));
}


// ===========================================================================
// VI.  Convenience over a whole byte_string
// ===========================================================================

// decode_container
//   function: convenience over a whole `byte_string` - wraps it in a reader
// and reads one container of type `Container` from the front.
template<typename Container>
decode_result<clean_t<Container>>
decode_container(const byte_string& _bytes)
{
    byte_reader _reader(_bytes);

    return decode_container<Container>(_reader);
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONTAINER_DECODE_HPP
