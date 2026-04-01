/******************************************************************************
* djinterp [container]                                       tree_iterator.hpp
*
* Hierarchical Tree Iterators:
* Provides zero-cost imperative cursors and standard-compliant iterators
* for traversing tree topologies. Supports configurable traversal orders
* (pre-order, post-order, level-order, etc.) via policy tags.
*
* path:      /inc/container/tree_iterator.hpp
* link(s):   TBA
* author(s): djinterp AI Agent                                date: 2026.03.31
******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_ITERATOR_
#define DJINTERP_CONTAINER_TREE_ITERATOR_ 1

#include <iterator>
#include <type_traits>
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../meta/node_traits.hpp"


NS_DJINTERP
NS_CONTAINER

    // =========================================================================
    // I.   TRAVERSAL TAGS
    // =========================================================================

    // pre_order
    //   trait: tag type for pre-order tree traversal (Root, Left, Right).
    struct pre_order
    {};

    // post_order
    //   trait: tag type for post-order tree traversal (Left, Right, Root).
    struct post_order
    {};

    // level_order
    //   trait: tag type for breadth-first/level-order traversal.
    struct level_order
    {};

    // leaf_order
    //   trait: tag type for traversing exclusively over leaf nodes.
    struct leaf_order
    {};


    // =========================================================================
    // II.  TREE CURSOR (IMPERATIVE)
    // =========================================================================

    // tree_cursor
    //   class: zero-allocation imperative navigator for tree nodes.
    template<typename _Node>
    class tree_cursor
    {
    public:
        using node_type  = _Node;
        using value_type = typename node_type::value_type;
        using reference  = value_type&;
        using pointer    = value_type*;

        // -----------------------------------------------------------------
        // constructors
        // -----------------------------------------------------------------

        D_CONSTEXPR tree_cursor() noexcept
            : m_node(nullptr)
        {}

        D_CONSTEXPR explicit tree_cursor(
                node_type* _node
            ) noexcept
                : m_node(_node)
            {}


        // -----------------------------------------------------------------
        // observers
        // -----------------------------------------------------------------

        D_CONSTEXPR bool
        is_valid() const noexcept
        {
            return (m_node != nullptr);
        }

        D_CONSTEXPR node_type*
        get() const noexcept
        {
            return m_node;
        }

        D_CONSTEXPR reference
        operator*() const
        {
            return m_node->data();
        }

        D_CONSTEXPR pointer
        operator->() const
        {
            return &(m_node->data());
        }


        // -----------------------------------------------------------------
        // topology observers
        // -----------------------------------------------------------------

        D_CONSTEXPR bool
        has_first_child() const noexcept
        {
            return (m_node && m_node->first_child() != nullptr);
        }

        D_CONSTEXPR bool
        has_next_sibling() const noexcept
        {
            return (m_node && m_node->next_sibling() != nullptr);
        }


        // -----------------------------------------------------------------
        // imperative navigation
        // -----------------------------------------------------------------

        D_CONSTEXPR void
        go_parent() noexcept
        {
            if (m_node)
            {
                m_node = m_node->parent();
            }

            return;
        }

        D_CONSTEXPR void
        go_first_child() noexcept
        {
            if (m_node)
            {
                m_node = m_node->first_child();
            }

            return;
        }

        D_CONSTEXPR void
        go_next_sibling() noexcept
        {
            if (m_node)
            {
                m_node = m_node->next_sibling();
            }

            return;
        }

    private:
        node_type* m_node;
    };


    // =========================================================================
    // III. TREE ITERATOR
    // =========================================================================

    // tree_iterator
    //   class: standard-compliant forward iterator adapting a tree_cursor.
    template<typename _Node,
             typename _Order = pre_order>
    class tree_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = typename _Node::value_type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;
        using cursor_type       = tree_cursor<_Node>;

        // -----------------------------------------------------------------
        // constructors
        // -----------------------------------------------------------------

        D_CONSTEXPR tree_iterator() noexcept
            : m_cursor()
        {}

        D_CONSTEXPR explicit tree_iterator(
                _Node* _node
            ) noexcept
                : m_cursor(_node)
            {}


        // -----------------------------------------------------------------
        // element access
        // -----------------------------------------------------------------

        D_CONSTEXPR reference
        operator*() const
        {
            return *m_cursor;
        }

        D_CONSTEXPR pointer
        operator->() const
        {
            return m_cursor.operator->();
        }


        // -----------------------------------------------------------------
        // traversal
        // -----------------------------------------------------------------

        D_CONSTEXPR tree_iterator&
        operator++() noexcept
        {
            if constexpr (std::is_same_v<_Order, pre_order>)
            {
                _advance_pre_order();
            }
            else if constexpr (std::is_same_v<_Order, post_order>)
            {
                _advance_post_order();
            }
            
            return *this;
        }

        D_CONSTEXPR tree_iterator
        operator++(int) noexcept
        {
            tree_iterator tmp = *this;
            ++(*this);

            return tmp;
        }


        // -----------------------------------------------------------------
        // comparison
        // -----------------------------------------------------------------

        D_CONSTEXPR friend bool
        operator==(
            const tree_iterator& _lhs,
            const tree_iterator& _rhs
        ) noexcept
        {
            return (_lhs.m_cursor.get() == _rhs.m_cursor.get());
        }

        D_CONSTEXPR friend bool
        operator!=(
            const tree_iterator& _lhs,
            const tree_iterator& _rhs
        ) noexcept
        {
            return !(_lhs == _rhs);
        }

    private:

        // -----------------------------------------------------------------
        // internal traversal logic
        // -----------------------------------------------------------------

        D_CONSTEXPR void
        _advance_pre_order() noexcept
        {
            if (!m_cursor.is_valid())
            {
                return;
            }

            if (m_cursor.has_first_child())
            {
                m_cursor.go_first_child();
            }
            else if (m_cursor.has_next_sibling())
            {
                m_cursor.go_next_sibling();
            }
            else
            {
                while ( m_cursor.is_valid() && 
                        !m_cursor.has_next_sibling() )
                {
                    m_cursor.go_parent();
                }

                if (m_cursor.is_valid())
                {
                    m_cursor.go_next_sibling();
                }
            }

            return;
        }

        D_CONSTEXPR void
        _advance_post_order() noexcept
        {
            if (!m_cursor.is_valid())
            {
                return;
            }

            if (m_cursor.has_next_sibling())
            {
                m_cursor.go_next_sibling();

                // Dive to the deepest first child of the sibling
                while (m_cursor.has_first_child())
                {
                    m_cursor.go_first_child();
                }
            }
            else
            {
                m_cursor.go_parent();
            }

            return;
        }

        cursor_type m_cursor;
    };


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TREE_ITERATOR_