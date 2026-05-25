/******************************************************************************
* djinterp [options]                                         option_layer.hpp
*
* Cascading option layers with sparse override storage.
*   Provides option_layer, an adapter that holds a sparse local option
* set of overrides and a reference to a parent layer.  Lookups check
* the local set first; on miss, they fall through to the parent.  The
* chain terminates at a root layer (no parent).
*
*   SPACE EFFICIENCY:
*   Each descendant stores only its overrides - not a full copy of the
* inherited configuration.  A great-grandchild with 2 overrides out of
* 500 inherited options uses space for exactly 2 entries plus one
* pointer.
*
*   OVERRIDE POLICY:
*   Mutations (insert, insert_or_assign, operator[]) are gated by an
* override policy.  The policy is any type satisfying the structural
* contract `bool allows(key, depth) const`.  Built-in policies
* include override_allow_all (default), override_deny_all (supremacy),
* override_to_depth<N>, and override_if<Pred> for functional logic.
* See option_override_policy.hpp.
*
*   COMPILE-TIME SUPPORT:
*   All read operations are D_CONSTEXPR.  When both the local set and
* parent chain are constexpr-capable, the entire layer is usable in
* constexpr contexts.  Write operations are D_CONSTEXPR when the
* local set's backing container supports constexpr mutation (C++20
* std::vector, or custom constexpr containers).
*
*   LAYER DEPTH:
*   Each layer knows its depth at compile time.  The root is depth 0.
* Depth is computed from the parent chain and passed to the override
* policy.
*
*   TYPE ALIASES:
*   option_layer satisfies the option_set structural contract
* (key_type, mapped_type, value_type, find, contains, size, begin,
* end) so that it is detected by option_set_traits.hpp.  Note that
* size() returns the local override count, not the total inherited
* count.  Use effective_size() for the total.
*
* TABLE OF CONTENTS
* =================
* I.    Layer Depth Computation
* II.   no_parent Tag
* III.  option_layer (root specialization)
* IV.   option_layer (chained - with parent)
* V.    Factory Functions
*
*
* path:      /inc/djinterp/options/option_layer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.12
******************************************************************************/

#ifndef DJINTERP_OPTION_LAYER_
#define DJINTERP_OPTION_LAYER_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./option_pair.hpp"
#include "./option_set.hpp"
#include "./option_override_policy.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Layer Depth Computation
// ===========================================================================

// no_parent
//   tag: sentinel type indicating a root layer with no parent.
struct no_parent
{};

NS_INTERNAL

    // layer_depth
    //   trait: computes the depth of a layer at compile time.
    // Root layers (parent = no_parent) have depth 0.
    template<typename _Layer>
    struct layer_depth
    {
        static constexpr std::size_t value = 0;
    };

NS_END  // internal


// ===========================================================================
// II.  option_layer (primary template - chained, with parent)
// ===========================================================================

// option_layer
//   class: a cascading layer that holds sparse local overrides
// and falls through to a parent layer for keys not present
// locally.
//
// _LocalSet    - the local option set type (e.g.
//                option_set<std::vector<option_pair<K,V>>>)
// _ParentLayer - the parent layer type, or no_parent for root
// _Policy      - override policy type (default: allow all)
//
// Example:
//   // root with all defaults
//   using root_set = option_set<
//       std::vector<option_pair<std::string, int>>>;
//   option_layer<root_set> root;
//   root.local().insert("width", 800);
//   root.local().insert("height", 600);
//   root.local().insert("depth", 32);
//
//   // child overrides just one option
//   using child_set = option_set<
//       std::vector<option_pair<std::string, int>>>;
//   auto child = make_option_layer(child_set{}, root);
//   child.local().insert("width", 1920);
//
//   child.resolve("width");  // 1920 (local)
//   child.resolve("height"); // 600  (from root)
//   child.resolve("depth");  // 32   (from root)
template<typename _LocalSet,
         typename _ParentLayer = no_parent,
         typename _Policy      = override_allow_all>
class option_layer
{
public:
    // --- deduced types ---
    using local_set_type  = _LocalSet;
    using parent_type     = _ParentLayer;
    using policy_type     = _Policy;
    using key_type        = typename _LocalSet::key_type;
    using mapped_type     = typename _LocalSet::mapped_type;
    using value_type      = typename _LocalSet::value_type;
    using size_type       = std::size_t;

    // --- depth ---
    static constexpr std::size_t depth =
        internal::layer_depth<_ParentLayer>::value + 1;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // from local set + parent reference
    D_CONSTEXPR
    option_layer(const _LocalSet&    _local,
                 const _ParentLayer& _parent)
        : m_local(_local),
          m_parent(&_parent),
          m_policy()
    {}

    // from local set + parent reference + policy
    D_CONSTEXPR
    option_layer(const _LocalSet&    _local,
                 const _ParentLayer& _parent,
                 const _Policy&      _policy)
        : m_local(_local),
          m_parent(&_parent),
          m_policy(_policy)
    {}

    // move local set + parent reference
    D_CONSTEXPR
    option_layer(_LocalSet&&         _local,
                 const _ParentLayer& _parent)
        : m_local(static_cast<_LocalSet&&>(_local)),
          m_parent(&_parent),
          m_policy()
    {}

    // move local set + parent reference + policy
    D_CONSTEXPR
    option_layer(_LocalSet&&         _local,
                 const _ParentLayer& _parent,
                 const _Policy&      _policy)
        : m_local(static_cast<_LocalSet&&>(_local)),
          m_parent(&_parent),
          m_policy(_policy)
    {}

    // -----------------------------------------------------------------
    //  access: local set and parent
    // -----------------------------------------------------------------

    // local
    //   returns a reference to the local override set.
    D_CONSTEXPR const _LocalSet&
    local() const noexcept
    {
        return m_local;
    }

    D_CONSTEXPR _LocalSet&
    local() noexcept
    {
        return m_local;
    }

    // parent
    //   returns a reference to the parent layer.
    D_CONSTEXPR const _ParentLayer&
    parent() const noexcept
    {
        return *m_parent;
    }

    // policy
    //   returns a reference to the override policy.
    D_CONSTEXPR const _Policy&
    policy() const noexcept
    {
        return m_policy;
    }

    // -----------------------------------------------------------------
    //  resolve: cascading lookup
    // -----------------------------------------------------------------

    // resolve
    //   returns the effective value for a key by checking local
    // first, then falling through to parent.  Undefined behaviour
    // if the key does not exist in any layer.
    D_CONSTEXPR const mapped_type&
    resolve(const key_type& _k) const
    {
        auto it = m_local.find(_k);

        if (it != m_local.end())
        {
            return it->value;
        }

        return m_parent->resolve(_k);
    }

    // resolve_or
    //   returns the effective value for a key, or _fallback if
    // the key is absent from all layers.
    D_CONSTEXPR mapped_type
    resolve_or(const key_type&    _k,
               const mapped_type& _fallback) const
    {
        auto it = m_local.find(_k);

        if (it != m_local.end())
        {
            return it->value;
        }

        return m_parent->resolve_or(_k, _fallback);
    }

    // contains
    //   returns true if the key exists in any layer of the
    // chain.
    D_CONSTEXPR bool
    contains(const key_type& _k) const
    {
        if (m_local.contains(_k))
        {
            return true;
        }

        return m_parent->contains(_k);
    }

    // contains_locally
    //   returns true if the key exists in the local override
    // set only (not inherited).
    D_CONSTEXPR bool
    contains_locally(const key_type& _k) const
    {
        return m_local.contains(_k);
    }

    // is_overridden
    //   returns true if the key exists locally AND in the
    // parent chain (i.e. the local value shadows an inherited
    // one).
    D_CONSTEXPR bool
    is_overridden(const key_type& _k) const
    {
        return ( m_local.contains(_k) &&
                 m_parent->contains(_k) );
    }

    // is_inherited
    //   returns true if the key's effective value comes from
    // a parent layer, not from local.
    D_CONSTEXPR bool
    is_inherited(const key_type& _k) const
    {
        return ( !m_local.contains(_k) &&
                 m_parent->contains(_k) );
    }

    // origin_depth
    //   returns the depth of the layer that owns the effective
    // value for _k.  Returns depth if local, otherwise
    // delegates to parent.
    D_CONSTEXPR std::size_t
    origin_depth(const key_type& _k) const
    {
        if (m_local.contains(_k))
        {
            return depth;
        }

        return m_parent->origin_depth(_k);
    }

    // -----------------------------------------------------------------
    //  option_set structural contract (for trait detection)
    // -----------------------------------------------------------------
    // These methods operate on the LOCAL set so that
    // option_set_traits.hpp recognizes option_layer as
    // option_set_like.  Use resolve() for cascading lookup.

    // find (local only)
    D_CONSTEXPR auto
    find(const key_type& _k) const
        -> decltype(m_local.find(_k))
    {
        return m_local.find(_k);
    }

    D_CONSTEXPR auto
    find(const key_type& _k)
        -> decltype(m_local.find(_k))
    {
        return m_local.find(_k);
    }

    // size (local overrides only)
    D_CONSTEXPR size_type
    size() const noexcept
    {
        return m_local.size();
    }

    // empty (local overrides only)
    D_CONSTEXPR bool
    empty() const noexcept
    {
        return m_local.empty();
    }

    // begin / end (local iteration)
    D_CONSTEXPR auto begin() const noexcept
        -> decltype(m_local.begin())
    {
        return m_local.begin();
    }

    D_CONSTEXPR auto end() const noexcept
        -> decltype(m_local.end())
    {
        return m_local.end();
    }

    D_CONSTEXPR auto begin() noexcept
        -> decltype(m_local.begin())
    {
        return m_local.begin();
    }

    D_CONSTEXPR auto end() noexcept
        -> decltype(m_local.end())
    {
        return m_local.end();
    }

    // -----------------------------------------------------------------
    //  policy-gated modifiers
    // -----------------------------------------------------------------

    // insert
    //   inserts a local override.  The override policy is
    // consulted; if it denies, the insert is silently ignored
    // and returns false.
    template<typename _S = _LocalSet>
    D_CONSTEXPR auto
    insert(const key_type&    _k,
           const mapped_type& _v)
        -> decltype(std::declval<_S&>().insert(_k, _v))
    {
        if (!m_policy.allows(_k, depth))
        {
            return false;
        }

        return m_local.insert(_k, _v);
    }

    // insert_or_assign
    //   inserts or overwrites a local override, gated by
    // the override policy.
    template<typename _S = _LocalSet>
    D_CONSTEXPR auto
    insert_or_assign(const key_type&    _k,
                     const mapped_type& _v)
        -> decltype(std::declval<_S&>().insert_or_assign(
                        _k, _v))
    {
        if (!m_policy.allows(_k, depth))
        {
            return false;
        }

        return m_local.insert_or_assign(_k, _v);
    }

    // erase
    //   removes a local override.  Does not affect parent
    // layers.  After erasure, the key reverts to its
    // inherited value.
    template<typename _S = _LocalSet>
    D_CONSTEXPR auto
    erase(const key_type& _k)
        -> decltype(std::declval<_S&>().erase(_k))
    {
        return m_local.erase(_k);
    }

private:
    _LocalSet          m_local;
    const _ParentLayer* m_parent;
    _Policy            m_policy;
};


// ===========================================================================
// III. option_layer (root specialization - no parent)
// ===========================================================================
// When _ParentLayer is no_parent, the layer is a root: lookups
// only check the local set, and there is no fallthrough.

template<typename _LocalSet,
         typename _Policy>
class option_layer<_LocalSet, no_parent, _Policy>
{
public:
    // --- deduced types ---
    using local_set_type  = _LocalSet;
    using parent_type     = no_parent;
    using policy_type     = _Policy;
    using key_type        = typename _LocalSet::key_type;
    using mapped_type     = typename _LocalSet::mapped_type;
    using value_type      = typename _LocalSet::value_type;
    using size_type       = std::size_t;

    // --- depth ---
    static constexpr std::size_t depth = 0;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    D_CONSTEXPR option_layer() = default;

    D_CONSTEXPR explicit
    option_layer(const _LocalSet& _local)
        : m_local(_local),
          m_policy()
    {}

    D_CONSTEXPR
    option_layer(const _LocalSet& _local,
                 const _Policy&   _policy)
        : m_local(_local),
          m_policy(_policy)
    {}

    D_CONSTEXPR explicit
    option_layer(_LocalSet&& _local)
        : m_local(static_cast<_LocalSet&&>(_local)),
          m_policy()
    {}

    D_CONSTEXPR
    option_layer(_LocalSet&& _local,
                 const _Policy& _policy)
        : m_local(static_cast<_LocalSet&&>(_local)),
          m_policy(_policy)
    {}

    // -----------------------------------------------------------------
    //  access
    // -----------------------------------------------------------------

    D_CONSTEXPR const _LocalSet&
    local() const noexcept
    {
        return m_local;
    }

    D_CONSTEXPR _LocalSet&
    local() noexcept
    {
        return m_local;
    }

    D_CONSTEXPR const _Policy&
    policy() const noexcept
    {
        return m_policy;
    }

    // -----------------------------------------------------------------
    //  resolve (root: local only)
    // -----------------------------------------------------------------

    D_CONSTEXPR const mapped_type&
    resolve(const key_type& _k) const
    {
        return m_local.at(_k);
    }

    D_CONSTEXPR mapped_type
    resolve_or(const key_type&    _k,
               const mapped_type& _fallback) const
    {
        return m_local.value_or(_k, _fallback);
    }

    D_CONSTEXPR bool
    contains(const key_type& _k) const
    {
        return m_local.contains(_k);
    }

    D_CONSTEXPR bool
    contains_locally(const key_type& _k) const
    {
        return m_local.contains(_k);
    }

    D_CONSTEXPR bool
    is_overridden(const key_type& /*_k*/) const
    {
        return false;
    }

    D_CONSTEXPR bool
    is_inherited(const key_type& /*_k*/) const
    {
        return false;
    }

    D_CONSTEXPR std::size_t
    origin_depth(const key_type& /*_k*/) const
    {
        return 0;
    }

    // -----------------------------------------------------------------
    //  option_set structural contract
    // -----------------------------------------------------------------

    D_CONSTEXPR auto
    find(const key_type& _k) const
        -> decltype(m_local.find(_k))
    {
        return m_local.find(_k);
    }

    D_CONSTEXPR auto
    find(const key_type& _k)
        -> decltype(m_local.find(_k))
    {
        return m_local.find(_k);
    }

    D_CONSTEXPR size_type
    size() const noexcept
    {
        return m_local.size();
    }

    D_CONSTEXPR bool
    empty() const noexcept
    {
        return m_local.empty();
    }

    D_CONSTEXPR auto begin() const noexcept
        -> decltype(m_local.begin())
    {
        return m_local.begin();
    }

    D_CONSTEXPR auto end() const noexcept
        -> decltype(m_local.end())
    {
        return m_local.end();
    }

    D_CONSTEXPR auto begin() noexcept
        -> decltype(m_local.begin())
    {
        return m_local.begin();
    }

    D_CONSTEXPR auto end() noexcept
        -> decltype(m_local.end())
    {
        return m_local.end();
    }

    // -----------------------------------------------------------------
    //  modifiers
    // -----------------------------------------------------------------

    template<typename _S = _LocalSet>
    D_CONSTEXPR auto
    insert(const key_type&    _k,
           const mapped_type& _v)
        -> decltype(std::declval<_S&>().insert(_k, _v))
    {
        return m_local.insert(_k, _v);
    }

    template<typename _S = _LocalSet>
    D_CONSTEXPR auto
    insert_or_assign(const key_type&    _k,
                     const mapped_type& _v)
        -> decltype(std::declval<_S&>().insert_or_assign(
                        _k, _v))
    {
        return m_local.insert_or_assign(_k, _v);
    }

    template<typename _S = _LocalSet>
    D_CONSTEXPR auto
    erase(const key_type& _k)
        -> decltype(std::declval<_S&>().erase(_k))
    {
        return m_local.erase(_k);
    }

private:
    _LocalSet m_local;
    _Policy   m_policy;
};


// ===========================================================================
// IV.  Layer Depth - recursive specialization
// ===========================================================================

NS_INTERNAL

    template<typename _LocalSet,
             typename _ParentLayer,
             typename _Policy>
    struct layer_depth<option_layer<_LocalSet,
                                    _ParentLayer,
                                    _Policy>>
    {
        static constexpr std::size_t value =
            option_layer<_LocalSet,
                         _ParentLayer,
                         _Policy>::depth;
    };

NS_END  // internal


// ===========================================================================
// V.   Factory Functions
// ===========================================================================

// make_option_layer (root)
//   function: creates a root layer from a local set.
template<typename _LocalSet>
D_CONSTEXPR
option_layer<typename std::decay<_LocalSet>::type>
make_option_layer(_LocalSet&& _local)
{
    return option_layer<
        typename std::decay<_LocalSet>::type>(
            static_cast<_LocalSet&&>(_local));
}

// make_option_layer (chained)
//   function: creates a child layer with a parent reference.
template<typename _LocalSet,
         typename _ParentLayer>
D_CONSTEXPR
option_layer<typename std::decay<_LocalSet>::type,
             _ParentLayer>
make_option_layer(_LocalSet&&         _local,
                  const _ParentLayer& _parent)
{
    return option_layer<
        typename std::decay<_LocalSet>::type,
        _ParentLayer>(
            static_cast<_LocalSet&&>(_local),
            _parent);
}

// make_option_layer (chained + policy)
//   function: creates a child layer with parent and policy.
template<typename _LocalSet,
         typename _ParentLayer,
         typename _Policy>
D_CONSTEXPR
option_layer<typename std::decay<_LocalSet>::type,
             _ParentLayer,
             typename std::decay<_Policy>::type>
make_option_layer(_LocalSet&&         _local,
                  const _ParentLayer& _parent,
                  _Policy&&           _policy)
{
    return option_layer<
        typename std::decay<_LocalSet>::type,
        _ParentLayer,
        typename std::decay<_Policy>::type>(
            static_cast<_LocalSet&&>(_local),
            _parent,
            static_cast<_Policy&&>(_policy));
}


NS_END  // djinterp


#endif  // DJINTERP_OPTION_LAYER_
