/******************************************************************************
* djinterp [util]                                        container_metadata.hpp
*
* djinterp container-metadata foundational module:
*   Container-agnostic, payload-agnostic association between a container
* and its metadata. Builds on the three primitives from
* `util/metadata/metadata.hpp` (single value, key-value entry,
* homogeneous collection) and ties a payload of any of these shapes to
* an arbitrary container.
*
*   PURPOSE
*   =======
*   Many container types want to carry side information that isn't part
* of their data: descriptive labels (table titles, list names), formatting
* hints, versioning, tags, comments, provenance, and so on. This module
* provides the foundational wrapper - `container_metadata` - that pairs
* a container with a metadata payload without imposing requirements on
* either. The container is held by reference (non-owning); the payload
* is owned.
*
*   DESIGN
*   ======
*   `container_metadata<_Container, _Payload>` is a thin pairing:
*     - `_Container`  : any user-provided container type. The class
*                       holds a pointer to an instance. There is NO
*                       requirement on the container - no expected
*                       `value_type`, `size()`, iterators, or anything
*                       else. If the user wants to associate metadata
*                       with a non-container too, this works.
*     - `_Payload`    : the metadata payload. Typically one of
*                       `metadata_value<>`, `metadata_entry<>`, or
*                       `metadata_collection<>`, but any type is
*                       accepted. The `is_metadata` trait can be used
*                       at the call site to enforce constraints when
*                       desired.
*
*   The pairing is intentionally non-owning of the container - a
* container_metadata "decorates" a container that lives elsewhere.
* This makes it cheap to attach and detach metadata, and lets multiple
* metadata views coexist for the same container.
*
*   COMPOSITION
*   ===========
*   For containers carrying multiple kinds of metadata (e.g. a single
* title PLUS a key-value formatting dictionary PLUS a list of tags),
* compose container_metadata instances or build a payload that is
* itself a collection of mixed metadata primitives.
*
*   PORTABILITY
*   ===========
*     version: C++11 or higher; `_v` companions C++14+.
*     dependencies:
*       - djinterp.hpp           : NS_DJINTERP, NS_INTERNAL, clean_t
*       - core/meta/type_traits.hpp : void_t
*       - util/metadata/metadata.hpp : metadata primitives, is_metadata
*
*
* path:      /inc/djinterp/util/metadata/container_metadata.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.18
******************************************************************************/

#ifndef DJINTERP_CONTAINER_METADATA_
#define DJINTERP_CONTAINER_METADATA_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../core/meta/type_traits.hpp"
#include "./metadata.hpp"


NS_DJINTERP


    // =========================================================================
    // I.   CONTAINER METADATA
    // =========================================================================
    //
    // `container_metadata<_Container, _Payload>`
    //   Pairs a non-owning reference to a `_Container` with an owned
    // `_Payload` of metadata. The class is deliberately minimal - it
    // exposes the held container, the held payload, and a small set
    // of binding controls. Anything richer (typed access, dictionary
    // lookup, named-key indexing) belongs in a higher-level layer
    // that targets a specific payload shape.
    //
    //   No constraints are imposed on either template parameter:
    //     - `_Container` is not required to be iterable, sized,
    //       allocator-aware, or anything else. The class holds a
    //       pointer; the user's code is responsible for whatever
    //       operations it performs through that pointer.
    //     - `_Payload` is not required to satisfy `is_metadata`.
    //       Callers wanting that constraint can `static_assert` at
    //       their site of use.

    // container_metadata
    //   class: container-agnostic, payload-agnostic
    // metadata pairing.
    template<typename _Container,
             typename _Payload>
    class container_metadata
    {
    public:
        using container_type = _Container;
        using payload_type   = _Payload;
        using self_type      = container_metadata<_Container, _Payload>;

        // container_metadata()
        //   constructor: default - unbound (no container), empty
        // payload.
        container_metadata()
            : m_container(nullptr),
              m_payload()
        {}

        // container_metadata(payload)
        //   constructor: bound only to a payload (no container).
        // Useful when the metadata exists before the container or
        // independently of any specific instance.
        explicit container_metadata(
            const _Payload& _payload
        )
            : m_container(nullptr),
              m_payload(_payload)
        {}

        // container_metadata(payload&&)
        //   constructor: bound only to a payload (move).
        explicit container_metadata(
            _Payload&& _payload
        )
        noexcept(std::is_nothrow_move_constructible<_Payload>::value)
            : m_container(nullptr),
              m_payload(std::move(_payload))
        {}

        // container_metadata(container, payload)
        //   constructor: bound to both a container and a payload.
        container_metadata(
            _Container&     _container,
            const _Payload& _payload
        )
            : m_container(&_container),
              m_payload(_payload)
        {}

        // container_metadata(container, payload&&)
        //   constructor: bound to both, payload moved.
        container_metadata(
            _Container& _container,
            _Payload&&  _payload
        )
        noexcept(std::is_nothrow_move_constructible<_Payload>::value)
            : m_container(&_container),
              m_payload(std::move(_payload))
        {}

        // container_metadata(container)
        //   constructor: bound only to a container (empty payload).
        explicit container_metadata(
            _Container& _container
        )
            : m_container(&_container),
              m_payload()
        {}

        // -----------------------------------------------------------------
        //  container binding
        // -----------------------------------------------------------------

        // bind
        //   function: associates this metadata with `_container`.
        void bind(_Container& _container) noexcept
        {
            m_container = &_container;

            return;
        }

        // unbind
        //   function: disassociates this metadata from any container.
        // The payload is preserved; only the container pointer is
        // cleared.
        void unbind() noexcept
        {
            m_container = nullptr;

            return;
        }

        // is_bound
        //   function: true iff a container is currently associated.
        bool is_bound() const noexcept
        {
            return (m_container != nullptr);
        }

        // get_container
        //   function: pointer to the associated container (may be
        // null).
        _Container*       get_container()       noexcept
        {
            return m_container;
        }

        // get_container
        //   function: const pointer to the associated container.
        const _Container* get_container() const noexcept
        {
            return m_container;
        }

        // -----------------------------------------------------------------
        //  payload access
        // -----------------------------------------------------------------

        // payload
        //   function: const accessor for the metadata payload.
        const _Payload& payload() const noexcept
        {
            return m_payload;
        }

        // payload
        //   function: mutable accessor for the metadata payload.
        _Payload& payload() noexcept
        {
            return m_payload;
        }

        // set_payload
        //   function: replaces the metadata payload (copy).
        void set_payload(const _Payload& _payload)
        {
            m_payload = _payload;

            return;
        }

        // set_payload
        //   function: replaces the metadata payload (move).
        void set_payload(_Payload&& _payload)
        noexcept(std::is_nothrow_move_assignable<_Payload>::value)
        {
            m_payload = std::move(_payload);

            return;
        }

        // -----------------------------------------------------------------
        //  comparison
        // -----------------------------------------------------------------

        // operator==
        //   function: compares the payload only - container identity
        // is ignored, since the container is non-owning.
        bool operator==(const self_type& _other) const
        {
            return (m_payload == _other.m_payload);
        }

        // operator!=
        //   function: inequality on payload.
        bool operator!=(const self_type& _other) const
        {
            return !(*this == _other);
        }

    private:
        _Container* m_container;
        _Payload    m_payload;
    };


    // =========================================================================
    // II.  CONTAINER-METADATA DETECTION
    // =========================================================================

    // is_container_metadata
    //   trait: detects `container_metadata<>` specializations and
    // structurally compatible types (exposing `container_type` and
    // `payload_type` aliases plus `get_container()` and `payload()`
    // accessors).
    template<typename _Type,
             typename = void>
    struct is_container_metadata : std::false_type
    {};

    // is_container_metadata (specialization)
    //   trait: SFINAE success case.
    template<typename _Type>
    struct is_container_metadata<_Type,
        void_t<typename _Type::container_type,
               typename _Type::payload_type,
               decltype(std::declval<const _Type&>().get_container()),
               decltype(std::declval<const _Type&>().payload())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        // is_container_metadata_v
        //   value: variable-template companion to
        // `is_container_metadata`.
        template<typename _Type>
        inline constexpr bool is_container_metadata_v =
            is_container_metadata<_Type>::value;
    #endif


    // =========================================================================
    // III. CONVENIENCE FACTORY
    // =========================================================================

    // make_container_metadata
    //   function: factory deducing `_Container` and `_Payload` from
    // the arguments.
    template<typename _Container,
             typename _Payload>
    container_metadata<clean_t<_Container>, clean_t<_Payload>>
    make_container_metadata(_Container& _container,
                            _Payload&&  _payload)
    {
        return container_metadata<clean_t<_Container>,
                                  clean_t<_Payload>>(
            _container,
            std::forward<_Payload>(_payload));
    }


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_METADATA_
