/******************************************************************************
* djinterp [container]                                     tree_container.hpp
*
* Generalized Tree Container:
*   Inherits from node_container and adds tree-specific semantics:
* root() / has_root() as aliases for entry_point() / has_entry(),
* plus set_root() / adopt_root() / release_root() for tree-domain
* naming.
*
*   All ownership, size tracking, allocator management, and Rule of
* Five behavior are handled by node_container.  tree_container is
* a thin semantic layer that says "the entry point is a root node."
*
*   Topology detection (binary, n-ary, parented) is deferred to
* tree_container_traits.hpp, which inspects the node_type.
*
*
* path:      /inc/container/tree_container.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.11
******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_CONTAINER_
#define DJINTERP_CONTAINER_TREE_CONTAINER_ 1

#include <memory>
#include <type_traits>
#include "../../djinterp.hpp"
#include "node/node_container.hpp"
#include "node/node_traits.hpp"


NS_DJINTERP
NS_CONTAINER

    // tree_container
    //   class: tree-specific node container.  Inherits all
    // ownership, size, and allocator management from
    // node_container.  Adds root() / has_root() as the
    // tree-domain entry point interface.
    template<typename _ValueType,
             typename _NodeType,
             typename _Allocator       = std::allocator<_NodeType>,
             typename _LockPolicy      = void,
             typename _OwnershipPolicy = non_owning_policy>
    class tree_container
        : public node_container<_ValueType,
                                _NodeType,
                                _Allocator,
                                _LockPolicy,
                                _OwnershipPolicy>
    {
    private:
        using base = node_container<_ValueType,
                                    _NodeType,
                                    _Allocator,
                                    _LockPolicy,
                                    _OwnershipPolicy>;

    public:
        // Re-export base type aliases
        using typename base::value_type;
        using typename base::node_type;
        using typename base::allocator_type;
        using typename base::lock_policy;
        using typename base::ownership_policy;
        using typename base::size_type;
        using typename base::difference_type;
        using typename base::depth_type;
        using typename base::reference;
        using typename base::const_reference;
        using typename base::pointer;
        using typename base::const_pointer;
        using typename base::entry_storage;

        // -----------------------------------------------------------------
        // constructors — forward to base
        // -----------------------------------------------------------------

        using base::base;

        // -----------------------------------------------------------------
        // tree-specific entry point interface
        // -----------------------------------------------------------------

        // root
        //   returns a mutable pointer to the root node.
        D_CONSTEXPR node_type*
        root() noexcept
        {
            return base::entry_point();
        }

        // root (const)
        D_CONSTEXPR const node_type*
        root() const noexcept
        {
            return base::entry_point();
        }

        // has_root
        //   returns true if the tree has a root node.
        D_CONSTEXPR bool
        has_root() const noexcept
        {
            return base::has_entry();
        }

        // set_root (raw pointer)
        //   sets the root node.  Ownership depends on policy.
        D_CONSTEXPR void
        set_root(
            node_type* _node
        ) noexcept(!_OwnershipPolicy::owns)
        {
            base::set_entry(_node);

            return;
        }

        // adopt_root (unique_ptr)
        //   takes exclusive ownership of a root node.
        //   Only available with unique_owning_policy.
        template<typename _Dummy = _OwnershipPolicy,
                 std::enable_if_t<
                     std::is_same<_Dummy,
                                  unique_owning_policy>::value,
                     int> = 0>
        void
        adopt_root(
            std::unique_ptr<node_type> _node
        )
        {
            base::adopt_entry(std::move(_node));

            return;
        }

        // share_root (shared_ptr)
        //   shares ownership of a root node.
        //   Only available with shared_owning_policy.
        template<typename _Dummy = _OwnershipPolicy,
                 std::enable_if_t<
                     std::is_same<_Dummy,
                                  shared_owning_policy>::value,
                     int> = 0>
        void
        share_root(
            std::shared_ptr<node_type> _node
        )
        {
            base::share_entry(std::move(_node));

            return;
        }

        // release_root
        //   releases ownership of the root and returns it.
        node_type*
        release_root()
        {
            return base::release_entry();
        }
    };


    // =========================================================================
    //  CONVENIENCE ALIASES
    // =========================================================================

    // owning_tree_container
    //   alias: tree_container with unique_owning_policy.
    template<typename _ValueType,
             typename _NodeType,
             typename _Allocator  = std::allocator<_NodeType>,
             typename _LockPolicy = void>
    using owning_tree_container =
        tree_container<_ValueType,
                       _NodeType,
                       _Allocator,
                       _LockPolicy,
                       unique_owning_policy>;

    // shared_tree_container
    //   alias: tree_container with shared_owning_policy.
    template<typename _ValueType,
             typename _NodeType,
             typename _Allocator  = std::allocator<_NodeType>,
             typename _LockPolicy = void>
    using shared_tree_container =
        tree_container<_ValueType,
                       _NodeType,
                       _Allocator,
                       _LockPolicy,
                       shared_owning_policy>;


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TREE_CONTAINER_
