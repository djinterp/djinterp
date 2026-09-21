/*******************************************************************************
* djinterp [parsegen]                                              registry.hpp
*
*   The C++ face of the capability vocabulary and the stage registry.
*   `feature_set` wraps the C mask in a strong type so a capability profile
* cannot be confused with any other integer and composes with `|`; it is one
* word, constexpr throughout, and converts to the C type implicitly, so a
* profile can be built at compile time and handed to a C query for nothing.
*
*   `registry` derives from d_parsegen_registry, adds no data member, and is
* asserted layout-identical.  `stage` is an ALIAS rather than a derived type,
* for the same reason `instr` is: a registry is an array of them.
*
* path:      /inc/djinterp/parsegen/registry.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

#ifndef DJINTERP_PARSEGEN_REGISTRY_HPP_
#define DJINTERP_PARSEGEN_REGISTRY_HPP_ 1

// std
#include <cstddef>              // std::size_t
#include <cstdint>              // std::uint32_t, std::uint64_t
#include <type_traits>          // std::is_standard_layout
// djinterp
#include "../parse/diagnostic.hpp"  // parse::diagnostics
#include "./parsegen.hpp"           // framework root, NS_PARSEGEN
#include "./registry.h"             // the C registry this layer faces


NS_DJINTERP
NS_PARSEGEN


// feature
//   enum: one capability a grammar may use or a stage may handle.  A scoped
// enum over the C bits, so a capability cannot be passed where an opcode or a
// family identifier was meant.
enum class feature : std::uint64_t
{
    none                = D_PARSEGEN_FEATURE_NONE,
    ordered_choice      = D_PARSEGEN_ORDERED_CHOICE,
    unordered_choice    = D_PARSEGEN_UNORDERED_CHOICE,
    bounded_repeat      = D_PARSEGEN_BOUNDED_REPEAT,
    empty_production    = D_PARSEGEN_EMPTY_PRODUCTION,
    left_recursion      = D_PARSEGEN_LEFT_RECURSION,
    syntactic_predicate = D_PARSEGEN_SYNTACTIC_PREDICATE,
    precedence          = D_PARSEGEN_PRECEDENCE,
    rule_parameters     = D_PARSEGEN_RULE_PARAMETERS,
    character_class     = D_PARSEGEN_CHARACTER_CLASS,
    literal_string      = D_PARSEGEN_LITERAL_STRING,
    case_insensitive    = D_PARSEGEN_CASE_INSENSITIVE,
    unicode             = D_PARSEGEN_UNICODE,
    capture             = D_PARSEGEN_CAPTURE,
    host_action         = D_PARSEGEN_HOST_ACTION,
    error_recovery      = D_PARSEGEN_ERROR_RECOVERY,
    token_stream        = D_PARSEGEN_TOKEN_STREAM,
    indentation         = D_PARSEGEN_INDENTATION,
    lookahead_k         = D_PARSEGEN_LOOKAHEAD_K,
    ambiguity           = D_PARSEGEN_AMBIGUITY,
    memoization         = D_PARSEGEN_MEMOIZATION,
    incremental         = D_PARSEGEN_INCREMENTAL
};


// feature_set
//   class: a set of capabilities.  One word, constexpr throughout, and
// implicitly convertible to the C mask, so a profile declared at compile time
// costs exactly what the integer costs.
class feature_set
{
public:
    // feature_set
    //   constructor: the empty set.
    constexpr feature_set() noexcept
        : m_bits(0u)
    {}

    // feature_set
    //   constructor: the set of one capability.
    constexpr feature_set(
        feature _feature
    ) noexcept
        : m_bits(static_cast<std::uint64_t>(_feature))
    {}

    // feature_set
    //   constructor: adopts a raw mask, for a bit outside the named range.
    explicit constexpr feature_set(
        d_parsegen_features _bits
    ) noexcept
        : m_bits(_bits)
    {}

    // operator d_parsegen_features
    //   function: the underlying mask, so every C entry point takes this type
    // directly.
    constexpr operator d_parsegen_features() const noexcept
    {
        return m_bits;
    }

    // operator|
    //   function: the union of two sets.
    constexpr feature_set
    operator|(
        const feature_set& _other
    ) const noexcept
    {
        return feature_set(m_bits | _other.m_bits);
    }

    // operator|=
    //   function: adds another set to this one.
    constexpr feature_set&
    operator|=(
        const feature_set& _other
    ) noexcept
    {
        m_bits |= _other.m_bits;

        return *this;
    }

    // operator&
    //   function: the intersection of two sets.
    constexpr feature_set
    operator&(
        const feature_set& _other
    ) const noexcept
    {
        return feature_set(m_bits & _other.m_bits);
    }

    // has
    //   accessor: whether every capability of another set is present.
    constexpr bool
    has(
        const feature_set& _required
    ) const noexcept
    {
        return (m_bits & _required.m_bits) == _required.m_bits;
    }

    // any
    //   accessor: whether any capability of another set is present.  The
    // rejection test.
    constexpr bool
    any(
        const feature_set& _probe
    ) const noexcept
    {
        return (m_bits & _probe.m_bits) != 0u;
    }

    // missing
    //   accessor: the capabilities another set requires that this lacks --
    // what a diagnostic prints rather than merely that something did not fit.
    constexpr feature_set
    missing(
        const feature_set& _required
    ) const noexcept
    {
        return feature_set(_required.m_bits & ~m_bits);
    }

    // empty
    //   accessor: whether the set holds any capability.
    constexpr bool
    empty() const noexcept
    {
        return (m_bits == 0u);
    }

    // render
    //   function: writes the named bits into a caller buffer, `|`-separated.
    std::size_t
    render(
        char*       _out,
        std::size_t _size
    ) const noexcept
    {
        return d_parsegen_features_render(m_bits, _out, _size);
    }

private:
    d_parsegen_features m_bits;
};


// operator|
//   function: builds a set from two capabilities, so a profile reads as
// `feature::ordered_choice | feature::capture`.
constexpr feature_set
operator|(
    feature _left,
    feature _right
) noexcept
{
    return feature_set(_left) | feature_set(_right);
}


// stage_kind
//   enum: what part of the pipeline a stage occupies.
enum class stage_kind : std::uint32_t
{
    frontend = D_PARSEGEN_STAGE_FRONTEND,
    pass     = D_PARSEGEN_STAGE_PASS,
    family   = D_PARSEGEN_STAGE_FAMILY,
    backend  = D_PARSEGEN_STAGE_BACKEND
};


// stage
//   type: one registrable stage, unchanged.  An alias rather than a derived
// type, because a registry is an array of them.
using stage = ::d_parsegen_stage;


// registry
//   class: the stages a pipeline can be built from, with lifetime.  Move-only,
// as every owning container here is.
class registry : public d_parsegen_registry
{
public:
    using const_iterator = const stage*;

    // registry
    //   constructor: an empty registry owning nothing.
    registry() noexcept
    {
        d_parsegen_registry_init(this, nullptr, 0u);
    }

    // registry
    //   constructor: a registry over a caller-supplied table.
    registry(
        stage*        _stages,
        std::uint32_t _capacity
    ) noexcept
    {
        d_parsegen_registry_init(this, _stages, _capacity);
    }

    registry(const registry&)            = delete;
    registry& operator=(const registry&) = delete;

    // registry
    //   constructor: takes over another registry's table and ownership.
    registry(
        registry&& _other
    ) noexcept
        : d_parsegen_registry(_other)
    {
        d_parsegen_registry_init(&_other, nullptr, 0u);
    }

    // operator=
    //   function: releases this registry, then takes over another's.
    registry&
    operator=(
        registry&& _other
    ) noexcept
    {
        // guard against self-move, which would release the table being taken
        if (this != &_other)
        {
            d_parsegen_registry_release(this);

            static_cast<d_parsegen_registry&>(*this) = _other;

            d_parsegen_registry_init(&_other, nullptr, 0u);
        }

        return *this;
    }

    // ~registry
    //   destructor: releases any table this registry owns.
    ~registry() noexcept
    {
        d_parsegen_registry_release(this);
    }

#if (D_INTERNAL_PARSEGEN_REGISTRY_HEAP == 1)
    // reserve
    //   function: replaces this registry's table with one it allocates and
    // owns, which then grows on demand.
    D_NODISCARD bool
    reserve(
        std::uint32_t _capacity = 0u
    ) noexcept
    {
        d_parsegen_registry_release(this);

        return (d_parsegen_registry_init_heap(this, _capacity) == 0);
    }
#endif  // D_INTERNAL_PARSEGEN_REGISTRY_HEAP

    // add
    //   function: registers a stage, replacing any earlier one of the same
    // kind and name.
    D_NODISCARD bool
    add(
        const stage& _stage
    ) noexcept
    {
        return (d_parsegen_registry_add(this, &_stage) == 0);
    }

    // find
    //   accessor: the stage of one kind registered under a name, or null.  A
    // failure is explained through the sink, listing what is registered.
    const stage*
    find(
        stage_kind         _kind,
        const char*        _name,
        d_parse_diag_sink* _diag = nullptr
    ) const noexcept
    {
        return d_parsegen_registry_find(this,
                                        static_cast<std::uint32_t>(_kind),
                                        _name,
                                        _diag);
    }

    // select
    //   accessor: the stage of one kind best suited to a capability profile,
    // or null.  A failure is explained candidate by candidate, each with the
    // capability that ruled it out.
    const stage*
    select(
        stage_kind         _kind,
        const feature_set& _features,
        d_parse_diag_sink* _diag = nullptr
    ) const noexcept
    {
        return d_parsegen_registry_select(this,
                                          static_cast<std::uint32_t>(_kind),
                                          _features,
                                          _diag);
    }

    // count_of
    //   accessor: how many stages of one kind are registered.
    std::uint32_t
    count_of(
        stage_kind _kind
    ) const noexcept
    {
        return d_parsegen_registry_count_of(
                   this,
                   static_cast<std::uint32_t>(_kind));
    }

    // size
    //   accessor: how many stages are registered, of every kind.
    constexpr std::uint32_t
    size() const noexcept
    {
        return count;
    }

    // begin
    //   accessor: a pointer to the first registered stage.
    constexpr const_iterator
    begin() const noexcept
    {
        return stages;
    }

    // end
    //   accessor: a pointer one past the last registered stage.
    constexpr const_iterator
    end() const noexcept
    {
        return (stages != nullptr) ? (stages + count) : nullptr;
    }
};


// fixed_registry
//   class: a registry carrying its own table.  The usual shape: a build links
// in the stages it wants and registers them once, allocating nothing.
template<std::uint32_t _Capacity>
class fixed_registry : public registry
{
public:
    // fixed_registry
    //   constructor: binds the embedded table as this registry's storage.
    fixed_registry() noexcept
    {
        d_parsegen_registry_init(this, m_stages, _Capacity);
    }

private:
    stage m_stages[_Capacity];
};


//   The claim these types cost nothing over the C ones, checked.
static_assert(sizeof(feature_set) == sizeof(d_parsegen_features),
              "parsegen::feature_set must be exactly the mask it wraps");
static_assert(std::is_trivially_copyable<feature_set>::value,
              "parsegen::feature_set must remain trivially copyable");
static_assert(sizeof(registry) == sizeof(d_parsegen_registry),
              "parsegen::registry must add no data member");
static_assert(alignof(registry) == alignof(d_parsegen_registry),
              "parsegen::registry must add no data member");
static_assert(std::is_standard_layout<registry>::value,
              "parsegen::registry must remain standard-layout");


NS_END  // parsegen
NS_END  // djinterp


#endif  // DJINTERP_PARSEGEN_REGISTRY_HPP_
