/******************************************************************************
* djinterp [test]                                            cofree_tests.hpp
*
*   Declarations and shared fixtures for the cofree.hpp unit suite.  The
* individual tests_* predicates and their block-providers are defined per
* translation unit (one .cpp per like-group semantic section of the header);
* this head carries only what those files and the runner share.
*
*   cofree<F, A> needs an F that is a single-argument, registered Functor whose
* F<shared_ptr<cofree>> is default-constructible.  The fixture opt<T> -- a
* maybe-like single-argument functor (0 or 1 child) -- serves as that F, so
* cofree<opt, A> is a NON-EMPTY descending chain / stream: A :< just(...) :<
* ... :< nothing.  A linear F keeps the built trees easy to read back.
*
*   chain_to_vector walks such a linear chain into a std::vector so map / extend
* / unfold results can be compared by contents; the named functors (dbl / inc /
* show for heads; head_of_node / suffix_sum / suffix_len for whole sub-trees;
* id_head / times_ten / descend for unfold seeds) are named rather than lambdas
* so they may appear in trailing return types on every language floor.
*
*   Every predicate lives flat in djinterp::testing and returns true iff all of
* its checks pass.  Each section .cpp keeps its predicates file-local (internal
* linkage) and exposes exactly one block-provider, declared below.
*
*   SECTIONS (mirroring cofree.hpp's table of contents):
*     I.      the cofree type ............ cofree_tests_type.cpp
*     II.3a   cofree_map / functor_map ... cofree_tests_map.cpp
*     II.3b   cofree_extend / extend ..... cofree_tests_extend.cpp
*     II.3c   unfold_cofree .............. cofree_tests_unfold.cpp
*     III.    registration & protocol .... cofree_tests_protocol.cpp
*
*
* path:      /tests/djinterp/core/functional/cofree_tests.hpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_COFREE_TESTS_
#define DJINTERP_FUNCTIONAL_COFREE_TESTS_ 1

// std
#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp -- the header under test (which pulls in functor.hpp + comonad.hpp),
//   plus the DTest authoring + runner surface.  NOTE: these two include paths
//   are rooted at the djinterp include directory (e.g. -I.../inc); adjust them
//   to match your build tree.
#include "djinterp/core/functional/cofree.hpp"
#include "djinterp/test/test_defaults.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                I.   FUNCTOR FIXTURE:  opt<T>  (the F layer)             ///
///////////////////////////////////////////////////////////////////////////////

// opt
//   fixture: a maybe-like single-argument functor -- engaged (one child) or
// empty (leaf) -- used as cofree's F.  Its empty state is what terminates a
// cofree chain.  Storage is a plain value; every T it is instantiated at here
// (a shared_ptr to a child cofree, or a scalar) is default-constructible.
template<typename _Type>
struct opt
{
    bool   has;
    _Type  val;

    opt()
        : has(false),
          val()
    {
    }

    opt(
        const _Type& _v
    )
        : has(true),
          val(_v)
    {
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  CHAIN WALKER                                         ///
///////////////////////////////////////////////////////////////////////////////

// chain_to_vector
//   helper: walk a linear cofree<opt, A> (head, then the single opt child, and
// so on) into a std::vector<A> of its head values, root first.  Lets map /
// extend / unfold results be compared by contents.
template<typename _A>
std::vector<_A>
chain_to_vector(
    const ::djinterp::cofree< ::djinterp::testing::opt, _A >& _node
)
{
    std::vector<_A> _out;

    _out.push_back(_node.head());

    ::djinterp::testing::opt<
        std::shared_ptr< ::djinterp::cofree< ::djinterp::testing::opt, _A > > >
            _layer = _node.unwrap();

    while (_layer.has)
    {
        std::shared_ptr< ::djinterp::cofree< ::djinterp::testing::opt, _A > >
            _child = _layer.val;

        _out.push_back(_child->head());
        _layer = _child->unwrap();
    }

    return _out;
}


///////////////////////////////////////////////////////////////////////////////
///                III. NAMED FUNCTORS                                       ///
///////////////////////////////////////////////////////////////////////////////

// -- head maps (A -> B) --

// dbl
//   int -> int, double.
struct dbl
{
    int operator()(int _x) const
    {
        return (_x * 2);
    }
};

// inc
//   int -> int, add one.
struct inc
{
    int operator()(int _x) const
    {
        return (_x + 1);
    }
};

// show
//   int -> std::string (proves cofree_map may change the head type).
struct show
{
    std::string operator()(int _x) const
    {
        return std::to_string(_x);
    }
};

// -- whole-node maps (cofree<opt,int> -> B), for extend --

// head_of_node
//   cofree<opt,int> -> int: the node's own head (extend with this is identity
// on the head values).
struct head_of_node
{
    int operator()(
        const ::djinterp::cofree< ::djinterp::testing::opt, int >& _n
    ) const
    {
        return _n.head();
    }
};

// suffix_sum
//   cofree<opt,int> -> int: the sum of every head in this node's whole
// sub-tree (a genuinely context-dependent co-bind).
struct suffix_sum
{
    int operator()(
        const ::djinterp::cofree< ::djinterp::testing::opt, int >& _n
    ) const
    {
        std::vector<int> _v = chain_to_vector(_n);
        int              _s = 0;

        for (std::size_t _i = 0; _i < _v.size(); ++_i)
        {
            _s += _v[_i];
        }

        return _s;
    }
};

// suffix_len
//   cofree<opt,int> -> std::size_t: the number of nodes in this node's
// sub-tree (proves extend may change the head type).
struct suffix_len
{
    std::size_t operator()(
        const ::djinterp::cofree< ::djinterp::testing::opt, int >& _n
    ) const
    {
        return chain_to_vector(_n).size();
    }
};

// -- unfold builders (Seed -> A ; Seed -> F<Seed>) --

// id_head
//   int -> int: each node's head is its seed unchanged.
struct id_head
{
    int operator()(int _s) const
    {
        return _s;
    }
};

// times_ten
//   int -> int: each node's head is ten times its seed (proves the head
// builder's value is what lands in the node).
struct times_ten
{
    int operator()(int _s) const
    {
        return (_s * 10);
    }
};

// descend
//   int -> opt<int>: the child seed is seed-1 while seed > 1, else no child --
// so unfold builds a finite descending chain and terminates.
struct descend
{
    opt<int> operator()(int _s) const
    {
        return (_s > 1) ? opt<int>(_s - 1) : opt<int>();
    }
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  SECTION BLOCK-PROVIDERS  (the runner's surface)      ///
///////////////////////////////////////////////////////////////////////////////

::djinterp::test::block_spec cofree_type_block();
::djinterp::test::block_spec cofree_map_block();
::djinterp::test::block_spec cofree_extend_block();
::djinterp::test::block_spec cofree_unfold_block();
::djinterp::test::block_spec cofree_protocol_block();


NS_END  // testing
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                V.   opt FUNCTOR REGISTRATION                             ///
///////////////////////////////////////////////////////////////////////////////
//   opt<T> is registered as a Functor (in namespace djinterp, where the primary
// lives) so cofree can map over its F-layer.  map applies the function to the
// engaged value or preserves the empty layer.

NS_DJINTERP

template<typename _Type>
struct functor_traits< ::djinterp::testing::opt<_Type>, void >
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    template<typename _Function>
    static
    auto map(
        const ::djinterp::testing::opt<_Type>& _fa,
        _Function                              _function
    )
    -> ::djinterp::testing::opt<
           typename std::decay<decltype(_function(_fa.val))>::type>
    {
        using mapped_t =
            typename std::decay<decltype(_function(_fa.val))>::type;

        return _fa.has
            ? ::djinterp::testing::opt<mapped_t>(_function(_fa.val))
            : ::djinterp::testing::opt<mapped_t>();
    }
};

NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_COFREE_TESTS_
