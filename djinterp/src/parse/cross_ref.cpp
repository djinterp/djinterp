#include "../arena/cross_ref.hpp"


NS_DJINTERP
NS_ARENA


// ================================================================
//  cross_ref_entry
// ================================================================

cross_ref_entry::cross_ref_entry()
    : domain (0),
      id     (null_node)
{
}

cross_ref_entry::cross_ref_entry
(
    domain_id _domain,
    node_id   _id
)
    : domain (_domain),
      id     (_id)
{
}

bool
cross_ref_entry::operator==
(
    const cross_ref_entry& _other
) const
{
    return ( (domain == _other.domain) &&
             (id     == _other.id) );
}

bool
cross_ref_entry::operator!=
(
    const cross_ref_entry& _other
) const
{
    return !(*this == _other);
}


// ================================================================
//  cross_ref  —  private
// ================================================================

/*
make_reverse_key
  Packs (domain, node_id) into a single 64-bit key for the
reverse index.  domain occupies the upper 32 bits, node_id
occupies the lower 32 bits.

Parameter(s):
  _domain: the domain identifier.
  _id:     the node identifier.
Return:
  A 64-bit key encoding both values.
*/
std::uint64_t
cross_ref::make_reverse_key
(
    domain_id _domain,
    node_id   _id
)
{
    return ( (static_cast<std::uint64_t>(_domain) << 32) |
              static_cast<std::uint64_t>(_id) );
}


// ================================================================
//  cross_ref  —  registration
// ================================================================

/*
bind
  Associates a (domain, node_id) pair with a stable_id.  If the
pair is already bound to this stable_id, this is a no-op.

Parameter(s):
  _stable_id: the stable identity key.
  _domain:    the domain owning the node.
  _id:        the node index within that domain's arena.
Return:
  none.
*/
void
cross_ref::bind
(
    std::uint64_t _stable_id,
    domain_id     _domain,
    node_id       _id
)
{
    cross_ref_entry entry(_domain, _id);

    // forward: stable_id -> entries
    auto& entries = m_forward[_stable_id];

    // check for duplicate
    for (const auto& e : entries)
    {
        if (e == entry)
        {
            return;
        }
    }

    entries.push_back(entry);

    // reverse: (domain, node_id) -> stable_id
    std::uint64_t reverse_key = make_reverse_key(_domain, _id);

    m_reverse[reverse_key] = _stable_id;

    return;
}


/*
unbind
  Removes the association between a (domain, node_id) pair and
its stable_id.

Parameter(s):
  _domain: the domain owning the node.
  _id:     the node index.
Return:
  none.
*/
void
cross_ref::unbind
(
    domain_id _domain,
    node_id   _id
)
{
    std::uint64_t reverse_key = make_reverse_key(_domain, _id);

    auto rev_it = m_reverse.find(reverse_key);

    if (rev_it == m_reverse.end())
    {
        return;
    }

    std::uint64_t stable_id = rev_it->second;

    // remove from reverse
    m_reverse.erase(rev_it);

    // remove from forward
    auto fwd_it = m_forward.find(stable_id);

    if (fwd_it != m_forward.end())
    {
        cross_ref_entry target(_domain, _id);
        auto& entries = fwd_it->second;

        for (auto it = entries.begin();
             it != entries.end();
             ++it)
        {
            if (*it == target)
            {
                entries.erase(it);

                break;
            }
        }

        // remove the stable_id entry if no bindings remain
        if (entries.empty())
        {
            m_forward.erase(fwd_it);
        }
    }

    return;
}


// ================================================================
//  cross_ref  —  forward lookup
// ================================================================

/*
find
  Returns a pointer to the entry vector for _stable_id, or
nullptr if not found.

Parameter(s):
  _stable_id: the stable identity key to look up.
Return:
  Pointer to the entry vector, or nullptr.
*/
const std::vector<cross_ref_entry>*
cross_ref::find
(
    std::uint64_t _stable_id
) const
{
    auto it = m_forward.find(_stable_id);

    if (it == m_forward.end())
    {
        return nullptr;
    }

    return &it->second;
}


/*
find_in_domain
  Returns the node_id for _stable_id within _domain, or
null_node if not found.

Parameter(s):
  _stable_id: the stable identity key.
  _domain:    the domain to search within.
Return:
  The node_id, or null_node.
*/
node_id
cross_ref::find_in_domain
(
    std::uint64_t _stable_id,
    domain_id     _domain
) const
{
    auto it = m_forward.find(_stable_id);

    if (it == m_forward.end())
    {
        return null_node;
    }

    for (const auto& entry : it->second)
    {
        if (entry.domain == _domain)
        {
            return entry.id;
        }
    }

    return null_node;
}


// ================================================================
//  cross_ref  —  reverse lookup
// ================================================================

/*
stable_id_of
  Returns the stable_id associated with (domain, node_id), or
0 if not found.

Parameter(s):
  _domain: the domain owning the node.
  _id:     the node index.
Return:
  The stable_id, or 0.
*/
std::uint64_t
cross_ref::stable_id_of
(
    domain_id _domain,
    node_id   _id
) const
{
    std::uint64_t reverse_key = make_reverse_key(_domain, _id);

    auto it = m_reverse.find(reverse_key);

    if (it == m_reverse.end())
    {
        return 0;
    }

    return it->second;
}


// ================================================================
//  cross_ref  —  bulk operations
// ================================================================

/*
unbind_domain
  Removes all bindings for a given domain.  Useful when an
arena is being rebuilt (e.g., full reparse).

Parameter(s):
  _domain: the domain to unbind entirely.
Return:
  none.
*/
void
cross_ref::unbind_domain
(
    domain_id _domain
)
{
    std::vector<std::uint64_t> keys_to_remove;

    for (const auto& pair : m_reverse)
    {
        domain_id d = static_cast<domain_id>(
            pair.first >> 32
        );

        if (d == _domain)
        {
            keys_to_remove.push_back(pair.first);
        }
    }

    for (std::uint64_t key : keys_to_remove)
    {
        domain_id d = static_cast<domain_id>(key >> 32);
        node_id   n = static_cast<node_id>(
            key & 0xFFFFFFFF
        );

        unbind(d, n);
    }

    return;
}


/*
clear
  Removes all bindings.

Parameter(s):
  (none)
Return:
  none.
*/
void
cross_ref::clear()
{
    m_forward.clear();
    m_reverse.clear();

    return;
}


/*
size
  Returns the number of distinct stable_ids tracked.

Parameter(s):
  (none)
Return:
  The count of unique stable_ids.
*/
std::size_t
cross_ref::size() const
{
    return m_forward.size();
}


NS_END  // arena
NS_END  // djinterp
