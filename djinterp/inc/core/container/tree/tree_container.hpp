/******************************************************************************
* djinterp [container]                                     tree_container.hpp
*
* Generalized Tree Container:
* Foundational module for all node-based tree containers. Implements
* core hierarchical state and trait-driven topology detection while
* remaining completely abstract over the underlying node linkage strategy.
*
* path:      /inc/container/tree_container.hpp
* link(s):   TBA
* author(s): djinterp AI Agent                                date: 2026.03.31
******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_CONTAINER_
#define DJINTERP_CONTAINER_TREE_CONTAINER_ 1

#include <memory>
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "node/node_traits.hpp"


NS_DJINTERP
NS_CONTAINER

    // tree_container
    //   class: foundational generalized node-based tree container managing hierarchical state.
    template<typename _ValueType,
             typename _NodeType,
             typename _Allocator  = std::allocator<_NodeType>,
             typename _LockPolicy = void>
    class tree_container
    {
    private:
        using allocator_traits = std::allocator_traits<_Allocator>;

    public:
        // Core and Structural Trait Aliases
        using value_type      = _ValueType;
        using node_type       = _NodeType;
        using allocator_type  = _Allocator;
        using lock_policy     = _LockPolicy;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;
        using depth_type      = std::size_t;
        using reference       = value_type&;
        using const_reference = const value_type&;
        using pointer         = typename allocator_traits::pointer;
        using const_pointer   = typename allocator_traits::const_pointer;

        // -----------------------------------------------------------------
        // constructors
        // -----------------------------------------------------------------

        D_CONSTEXPR tree_container() noexcept
                : m_root(nullptr),
                  m_size(0),
                  m_allocator()
            {}

        D_CONSTEXPR explicit tree_container(
                const allocator_type& _alloc
            ) noexcept
                : m_root(nullptr),
                  m_size(0),
                  m_allocator(_alloc)
            {}


        // -----------------------------------------------------------------
        // capacity
        // -----------------------------------------------------------------

        D_CONSTEXPR bool
        empty() const noexcept
        {
            return (m_size == 0);
        }

        D_CONSTEXPR size_type
        size() const noexcept
        {
            return m_size;
        }

        D_CONSTEXPR size_type
        max_size() const noexcept
        {
            return allocator_traits::max_size(m_allocator);
        }


        // -----------------------------------------------------------------
        // tree navigation / structural SFINAE hooks
        // -----------------------------------------------------------------

        D_CONSTEXPR node_type*
        root() noexcept
        {
            return m_root;
        }

        D_CONSTEXPR const node_type*
        root() const noexcept
        {
            return m_root;
        }


        // -----------------------------------------------------------------
        // modifiers
        // -----------------------------------------------------------------

        D_CONSTEXPR void
        set_root(node_type* _node) noexcept
        {
            m_root = _node;

            return;
        }

        D_CONSTEXPR void
        set_size(size_type _new_size) noexcept
        {
            m_size = _new_size;

            return;
        }

        D_CONSTEXPR void
        clear() noexcept
        {
            // Note: Foundational class only clears state. Deep recursive
            // destruction must be handled by concrete derived structures.
            m_root = nullptr;
            m_size = 0;

            return;
        }

        D_CONSTEXPR void
        swap(tree_container& _other) noexcept
        {
            djinterp::constexpr_swap(m_root, _other.m_root);
            djinterp::constexpr_swap(m_size, _other.m_size);
            djinterp::constexpr_swap(m_allocator, _other.m_allocator);

            return;
        }


        // -----------------------------------------------------------------
        // allocators
        // -----------------------------------------------------------------

        D_CONSTEXPR allocator_type
        get_allocator() const noexcept
        {
            return m_allocator;
        }

    private:
        node_type* m_root;
        size_type      m_size;
        allocator_type m_allocator;
    };


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TREE_CONTAINER_