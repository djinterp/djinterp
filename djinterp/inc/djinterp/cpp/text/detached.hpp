/*******************************************************************************
* djinterp [cpp]                                                    detached.hpp
*
*   An owning handle for a node made outside a tree. The markup modules'
* factories (make_div, make_paragraph, make_style_rule and the rest) return
* one, and their adopting functions take one, by rvalue, and nothing else.
* So a node that is never adopted is freed with its handle, and a node in a
* tree, which has no handle, can't be given a second owner: that doesn't
* compile.
*   The handle holds its node as std::unique_ptr would. It moves, and doesn't
* copy. get() gives the module's facade over the node, so a subtree can be
* built under a node before the node is placed, as a DOM's createElement and
* appendChild allow. A handle converts to another facade's handle over the
* same node type when its facade converts to that facade, as
* unique_ptr<Derived> does to unique_ptr<Base>: a Less rule's handle is a CSS
* rule's.
*   An adoption is refused when the parent is empty, when the handle is, or
* when the parent is the handle's node or lies under it, since the node would
* then own itself. A refused handle keeps its node.
*
* HOW TO USE
* ----------
*   Call a factory, build under its handle if you like, then give the handle
* to a parent, by move:
*
*     auto div = html::make_div<B>("main");
*     div->adopt(html::make_span<B>("hi"));
*     doc.body_element().adopt(std::move(div));
*
*   get() and -> reach the node through its module's facade. A handle never
* adopted frees its node when it goes; reset() frees it at once.
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   No D_CFG_* knob reaches this header. A facade it holds names its
* node_type, and constructs empty and from a node_type*; a node adopted into
* keeps its children as std::unique_ptrs in `children`, and its parent in
* `parent`.
*
*
* path:      /inc/djinterp/cpp/text/detached.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
*                                                            revised: 2026.11.18
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    TRAITS
      ------
      a. is_detached_handle
      b. is_detached_convertible

II.   DDetached<Facade>
      -----------------
      a. construction, moves and make
      b. get, the arrow and the node

III.  ADOPTION
      --------
      a. internal::detached_access
*/

#ifndef  DJINTERP_TEXT_DETACHED_HPP
#define  DJINTERP_TEXT_DETACHED_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <memory>       // std::unique_ptr
#include <type_traits>  // std::enable_if, std::integral_constant, std::is_*
#include <utility>      // std::forward, std::move
// djinterp
#include "../djinterp.hpp"  // framework root


NS_DJINTERP

template<typename Facade>
class DDetached;

NS_INTERNAL

    struct detached_access;

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                I.   TRAITS                                              ///
///////////////////////////////////////////////////////////////////////////////

// is_detached_handle
//   trait: true for a DDetached<Facade>, false for anything else, a facade
// included.
template<typename Type>
struct is_detached_handle
:   public std::false_type
{};

template<typename Facade>
struct is_detached_handle<DDetached<Facade>>
:   public std::true_type
{};

// is_detached_convertible
//   trait: true when a handle of `From` may become a handle of `To`: both
// facades are over one node type, and `From` converts to `To`. A facade
// converts to itself.
template<typename From,
         typename To>
struct is_detached_convertible
:   public std::integral_constant<bool,
        ( (std::is_same<typename From::node_type,
                        typename To::node_type>::value) &&
          (std::is_convertible<From, To>::value) )>
{};


///////////////////////////////////////////////////////////////////////////////
///                II.   DDetached<Facade>                                  ///
///////////////////////////////////////////////////////////////////////////////

// DDetached
//   class: owns one node made outside a tree, until a parent adopts it. The
// facade must name its node type as `node_type`, be default-constructible as
// an empty facade, and be explicitly constructible from a `node_type*`, as
// every markup facade is.
template<typename Facade>
class DDetached
{
public:
    // facade_type
    //   type: the facade get() gives.
    using facade_type = Facade;

    // node_type
    //   type: the backend node the handle owns.
    using node_type   = typename Facade::node_type;


    /// a.  construction, moves and make

    // DDetached (default)
    //   ctor: an empty handle, owning nothing.
    DDetached() noexcept
    :   m_node()
    {}

    // DDetached (move)
    //   ctor: takes `_other`'s node, leaving `_other` empty.
    DDetached(
        DDetached&& _other
    ) noexcept
    :   m_node(std::move(_other.m_node))
    {}

    // DDetached (converting move)
    //   ctor: takes the node of another facade's handle, over the same node
    // type, whose facade converts to this one's.
    template<typename Other,
             typename = typename std::enable_if<
                 ( (!std::is_same<Other, Facade>::value) &&
                   (is_detached_convertible<Other, Facade>::value) )>::type>
    DDetached(
        DDetached<Other>&& _other
    ) noexcept
    :   m_node(std::move(_other.m_node))
    {}

    DDetached(const DDetached&)            = delete;
    DDetached& operator=(const DDetached&) = delete;

    // operator= (move)
    //   function: frees this handle's node, if any, then takes `_other`'s.
    DDetached& operator=(
        DDetached&& _other
    ) noexcept
    {
        m_node = std::move(_other.m_node);

        return *this;
    }

    // ~DDetached
    //   dtor: frees the node, unless a parent adopted it.
    ~DDetached() = default;

    // make
    //   function: a handle owning a new node, made with `_args`. This is how
    // factories start; the node has no parent.
    template<typename... Args>
    static
    DDetached make(
        Args&&... _args
    )
    {
        DDetached made;

        made.m_node.reset(new node_type(std::forward<Args>(_args)...));

        return made;
    }

    // reset
    //   function: frees the node now, leaving the handle empty.
    void reset() noexcept
    {
        m_node.reset();

        return;
    }


    /// b.  get, the arrow and the node

    // valid
    //   function: true while the handle owns a node.
    bool valid() const noexcept
    {
        return static_cast<bool>(m_node);
    }

    // operator bool
    //   function: valid().
    explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_node);
    }

    // get
    //   function: the facade over the node, or an empty facade. It stays
    // usable after the node is adopted, for as long as its new parent lives.
    Facade get() const
    {
        return (m_node) ? Facade(m_node.get())
                        : Facade();
    }

    // arrow
    //   class: what operator-> gives: a facade over the node, for the one
    // expression.
    class arrow
    {
    public:
        explicit
        arrow(
            const Facade& _facade
        )
        :   m_facade(_facade)
        {}

        Facade* operator->()
        {
            return &m_facade;
        }

    private:
        Facade m_facade;
    };

    // operator->
    //   function: calls the facade's functions on the node, as get() would.
    // Like a std::unique_ptr's, it isn't for an empty handle.
    arrow operator->() const
    {
        return arrow(this->get());
    }

    // backend_handle
    //   function: the node, still owned by the handle; or NULL.
    node_type* backend_handle() const noexcept
    {
        return m_node.get();
    }


private:
    template<typename>
    friend class DDetached;

    friend struct ::djinterp::internal::detached_access;

    // m_node
    //   field: the node, until it is adopted.
    std::unique_ptr<node_type> m_node;
};


///////////////////////////////////////////////////////////////////////////////
///                III.   ADOPTION                                          ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // detached_access
    //   struct: what the modules' adopting functions share. A node type it
    // adopts into keeps its children as std::unique_ptrs in `children`, and
    // its parent in `parent`, as every default backend's does.
    struct detached_access
    {
        // adopt
        //   function: makes `_child`'s node the last of `_parent`'s
        // children, and returns it. Returns NULL, and `_child` keeps its
        // node, when `_parent` or `_child` is empty, or when `_parent` is
        // `_child`'s node or lies under it.
        template<typename Node,
                 typename Facade>
        static
        Node* adopt(
            Node*               _parent,
            DDetached<Facade>&  _child
        )
        {
            static_assert(std::is_same<Node, typename Facade::node_type>::value,
                          "a node adopts only a node of its own type");

            Node*       node = _child.m_node.get();
            const Node* above;

            if ( (!_parent) ||
                 (!node) )
            {
                return nullptr;
            }

            // the node would own itself, and never be freed
            for (above = _parent; above; above = above->parent)
            {
                if (above == node)
                {
                    return nullptr;
                }
            }

            // a failed push_back leaves the handle as it was
            _parent->children.push_back(std::move(_child.m_node));
            node->parent = _parent;

            return node;
        }
    };

NS_END  // internal

NS_END  // djinterp


#endif  // floor, for now

#endif  // DJINTERP_TEXT_DETACHED_HPP
