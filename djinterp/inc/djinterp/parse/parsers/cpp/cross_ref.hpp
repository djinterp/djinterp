/*******************************************************************************
* djinterp [parse]                                                 cross_ref.hpp
*
* Cross-reference index:
*   This header defines a type-erased cross-reference index that maps
* stable identity keys to (domain, node_id) pairs across multiple
* arenas.  It is the bridge between independently-owned arenas — the
* parser's symbol tree, the extension's diff tree, the wiki's document
* tree — enabling O(1) lookup from a logical entity to all of its
* physical representations.
*
* Contents:
*   - domain_id         domain classifier
*   - cross_ref_entry   single (domain, node_id) binding
*   - cross_ref         the index itself
*
* Design:
*   The index is intentionally decoupled from the arena template.  It
* stores only integer identifiers and does not hold references or
* pointers into any arena.  This means arenas can reallocate freely
* without invalidating the index.  The cost is one hash-map lookup per
* cross-reference query — acceptable for the wiki/sync paths where it
* is used, and never on the hot parse loop.
*
*
* path:      /inc/djinterp/parse/parsers/cpp/cross_ref.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_PARSE_PARSERS_CPP_CROSS_REF_HPP
#define DJINTERP_PARSE_PARSERS_CPP_CROSS_REF_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but core/container/arena/arena.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <unordered_map>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../../../core/container/arena/arena.hpp"  // node_id
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint16_t, uint64_t


NS_DJINTERP


// ================================================================
//  domain_id
// ================================================================

// domain_id
//   typedef: identifies which subsystem (arena) a node belongs
// to.
typedef re_std::uint16_t domain_id;


// ================================================================
//  well-known domains
// ================================================================

namespace domain
{
    constexpr domain_id file_system = 0x0001;
    constexpr domain_id symbol      = 0x0002;
    constexpr domain_id wiki        = 0x0003;
    constexpr domain_id diff        = 0x0004;

    // user_base
    //   constant: starting domain_id for user-defined domains.
    constexpr domain_id user_base   = 0x0100;
};


// ================================================================
//  cross_ref_entry
// ================================================================

// cross_ref_entry
//   struct: a single binding from a logical entity to a physical
// node within a specific domain's arena.
struct cross_ref_entry
{
    domain_id   domain;
    node_id     id;

    cross_ref_entry();

    cross_ref_entry
    (
        domain_id _domain,
        node_id   _id
    );

    bool operator==(const cross_ref_entry& _other) const;
    bool operator!=(const cross_ref_entry& _other) const;
};


// ================================================================
//  cross_ref
// ================================================================

// cross_ref
//   class: maps stable_id -> all (domain, node_id) pairs
// representing that logical entity across all arenas.
class cross_ref
{
public:

    // registration
    void        bind(re_std::uint64_t _stable_id, domain_id _domain, node_id _id);
    void        unbind(domain_id _domain, node_id _id);

    // forward lookup: stable_id -> entries
    const std::vector<cross_ref_entry>* find(re_std::uint64_t _stable_id) const;
    node_id     find_in_domain(re_std::uint64_t _stable_id, domain_id _domain) const;

    // reverse lookup: (domain, node_id) -> stable_id
    re_std::uint64_t stable_id_of(domain_id _domain, node_id _id) const;

    // bulk operations
    void        unbind_domain(domain_id _domain);
    void        clear();
    std::size_t size() const;

    // arena convenience — template, stays in header
    template<typename Payload>
    void
    bind_arena
    (
        domain_id                _domain,
        const arena<Payload>&   _arena
    )
    {
        for (std::size_t i = 0; i < _arena.size(); ++i)
        {
            node_id id = static_cast<node_id>(i);

            bind(_arena[id].stable_id,
                 _domain,
                 id);
        }

        return;
    }

private:

    D_STATIC re_std::uint64_t make_reverse_key(domain_id _domain, node_id _id);

    std::unordered_map<
        re_std::uint64_t,
        std::vector<cross_ref_entry>
    > m_forward;

    std::unordered_map<
        re_std::uint64_t,
        re_std::uint64_t
    > m_reverse;
};


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSE_PARSERS_CPP_CROSS_REF_HPP
