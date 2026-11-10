/*******************************************************************************
* djinterp [test]                                            recursion_tests.hpp
*
*   Unit-test declarations for core/functional/recursion.hpp.  One declaration
* group per section of the module under test:
*
*     recursion_tests_mu.cpp        -- I.        mu<F>, the least fixed point
*     recursion_tests_traits.cpp    -- II-IV.    recursive_traits /
*                                                corecursive_traits, detection,
*                                                the mu<F> instances
*     recursion_tests_cata.cpp      -- V.        cata[phi], the universal fold
*     recursion_tests_ana.cpp       -- VI.       ana[psi], the universal unfold
*     recursion_tests_hylo.cpp      -- VII.      hylo, the deforested refold
*
*   FIXTURES.  recursion.hpp folds any Functor, so the suite supplies three
* base functors as single-argument template-template citizens, each with a
* functor_traits instance (specialised at djinterp:: scope):
*
*     nat_f<X>   = 1 + X                Zero | Succ X          (one hole)
*     list_f<X>  = 1 + Int * X          Nil  | Cons Int X      (hole + payload)
*     tree_f<X>  = Int | X * X          Leaf Int | Branch X X  (two holes)
*
* nat_f exercises the single-hole path, list_f the payload-vs-hole distinction
* (Cons threads its Int but recurses on its tail), and tree_f the multi-child
* fold (Branch maps BOTH holes).  std::vector<int> is additionally registered
* as a native recursive carrier over list_f -- the "a native carrier
* participates without being rewritten as a fixed point" story from the header
* -- so cata / ana run over a plain vector, not only over mu.
*
*   Builders (make_*) assemble fixed points directly through In, and verifiers
* (nat_to_unsigned, list_to_vector, ...) read them back by walking out(),
* neither using a recursion scheme -- so cata is checked against hand-built
* values, and ana / hylo against hand-written walks, without circularity.
*
*   All tests are flat in djinterp::testing.
*
*
* path:      /tests/djinterp/cpp/functional/recursion_tests.hpp
* link(s):   TBA
* author(s): teer                                            created: 2026.07.11
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    FIXTURES  (base functors, native carrier, builders, verifiers, forms)
      ---------------------------------------------------------------------

I.    MU
      --

      --

      --

II.   TRAITS + DETECTION
      ------------------

III.  CATA
      ----

IV.   ANA
      ---

V.    HYLO
      ----
*/


#ifndef DJINTERP_TEST_RECURSION_TESTS_
#define DJINTERP_TEST_RECURSION_TESTS_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
// djinterp (module under test; pulls functor.hpp + the meta closure)
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/recursion.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


///////////////////////////////////////////////////////////////////////////////
///             0.    FIXTURES                                              ///
///////////////////////////////////////////////////////////////////////////////

// DNotRecursive
//   type: a plain struct with neither recursive_traits nor corecursive_traits,
// the negative case for the detection traits.
struct DNotRecursive
{
};


// -- base functor: nat_f<X> = 1 + X ---------------------------------------

// nat_f
//   fixture functor: the Peano base functor.  is_zero marks the Zero arm;
// otherwise succ is the single recursive hole.
template<typename _X>
struct nat_f
{
    using value_type = _X;

    bool is_zero;
    _X   succ;

    nat_f()
        : is_zero(true),
          succ()
    {}
};

template<typename _X>
inline
nat_f<_X> nat_zero()
{
    nat_f<_X> _n;
    _n.is_zero = true;
    return _n;
}

template<typename _X>
inline
nat_f<_X> nat_succ(
    const _X& _x
)
{
    nat_f<_X> _n;
    _n.is_zero = false;
    _n.succ    = _x;
    return _n;
}


// -- base functor: list_f<X> = 1 + Int * X --------------------------------

// list_f
//   fixture functor: the list base functor.  head is an Int PAYLOAD (not a
// hole, so a fold threads it unchanged); tail is the single recursive hole.
template<typename _X>
struct list_f
{
    using value_type = _X;

    bool is_nil;
    int  head;
    _X   tail;

    list_f()
        : is_nil(true),
          head(0),
          tail()
    {}
};

template<typename _X>
inline
list_f<_X> list_nil()
{
    list_f<_X> _l;
    _l.is_nil = true;
    return _l;
}

template<typename _X>
inline
list_f<_X> list_cons(
    int       _head,
    const _X& _tail
)
{
    list_f<_X> _l;
    _l.is_nil = false;
    _l.head   = _head;
    _l.tail   = _tail;
    return _l;
}


// -- base functor: tree_f<X> = Int | X * X --------------------------------

// tree_f
//   fixture functor: a binary-tree base functor.  A Leaf carries an Int
// value; a Branch has TWO recursive holes (left, right), so a fold must map
// both.
template<typename _X>
struct tree_f
{
    using value_type = _X;

    bool is_leaf;
    int  value;
    _X   left;
    _X   right;

    tree_f()
        : is_leaf(true),
          value(0),
          left(),
          right()
    {}
};

template<typename _X>
inline
tree_f<_X> tree_leaf(
    int _value
)
{
    tree_f<_X> _t;
    _t.is_leaf = true;
    _t.value   = _value;
    return _t;
}

template<typename _X>
inline
tree_f<_X> tree_branch(
    const _X& _left,
    const _X& _right
)
{
    tree_f<_X> _t;
    _t.is_leaf = false;
    _t.left    = _left;
    _t.right   = _right;
    return _t;
}


// -- named algebra / coalgebra forms (for the callable-shape tests) -------
//   cata / ana / hylo accept any callable; these let the tests confirm a free
// function and a function object work, not only a lambda.

// nat_depth_fn / DNatDepthAlg: the Peano depth algebra phi : nat_f<int> -> int
inline
int nat_depth_fn(
    const nat_f<int>& _layer
)
{
    return _layer.is_zero ? 0 : (1 + _layer.succ);
}

struct DNatDepthAlg
{
    int operator()(
        const nat_f<int>& _layer
    ) const
    {
        return _layer.is_zero ? 0 : (1 + _layer.succ);
    }
};

// nat_succ_fn / DNatSuccCo: the coalgebra psi : int -> nat_f<int> for Succ^n
inline
nat_f<int> nat_succ_fn(
    int _n
)
{
    return (_n <= 0) ? nat_zero<int>() : nat_succ<int>(_n - 1);
}

struct DNatSuccCo
{
    nat_f<int> operator()(
        int _n
    ) const
    {
        return (_n <= 0) ? nat_zero<int>() : nat_succ<int>(_n - 1);
    }
};


// -- manual builders (direct In-nesting; no recursion scheme) -------------

// make_nat: Succ^n Zero.
inline
mu<nat_f> make_nat(
    unsigned _n
)
{
    mu<nat_f> _acc = mu<nat_f>::In(nat_zero<mu<nat_f> >());
    for (unsigned _i = 0; _i < _n; ++_i)
    {
        _acc = mu<nat_f>::In(nat_succ<mu<nat_f> >(_acc));
    }
    return _acc;
}

// make_list: Cons over the elements, left to right (front = outermost Cons).
inline
mu<list_f> make_list(
    const std::vector<int>& _xs
)
{
    mu<list_f> _acc = mu<list_f>::In(list_nil<mu<list_f> >());
    for (std::size_t _i = _xs.size(); _i > 0; --_i)
    {
        _acc = mu<list_f>::In(list_cons<mu<list_f> >(_xs[_i - 1], _acc));
    }
    return _acc;
}

// make_leaf / make_branch: tree constructors.
inline
mu<tree_f> make_leaf(
    int _value
)
{
    return mu<tree_f>::In(tree_leaf<mu<tree_f> >(_value));
}

inline
mu<tree_f> make_branch(
    const mu<tree_f>& _left,
    const mu<tree_f>& _right
)
{
    return mu<tree_f>::In(tree_branch<mu<tree_f> >(_left, _right));
}

// make_balanced_tree: a full binary tree of the given depth, every leaf = 1
// (depth 0 is a single leaf; 2^depth leaves).
inline
mu<tree_f> make_balanced_tree(
    unsigned _depth
)
{
    if (_depth == 0)
    {
        return make_leaf(1);
    }
    mu<tree_f> _sub = make_balanced_tree(_depth - 1);
    return make_branch(_sub, _sub);
}


// -- manual verifiers (walk out() directly; no recursion scheme) ----------

// nat_to_unsigned: the depth of a Peano value.
inline
unsigned nat_to_unsigned(
    const mu<nat_f>& _m
)
{
    unsigned         _d   = 0;
    const mu<nat_f>* _cur = &_m;
    while (!_cur->out().is_zero)
    {
        ++_d;
        _cur = &_cur->out().succ;
    }
    return _d;
}

// list_to_vector: the elements of a list value, in order.
inline
std::vector<int> list_to_vector(
    const mu<list_f>& _m
)
{
    std::vector<int>  _r;
    const mu<list_f>* _cur = &_m;
    while (!_cur->out().is_nil)
    {
        _r.push_back(_cur->out().head);
        _cur = &_cur->out().tail;
    }
    return _r;
}

// tree_leaf_sum_manual: sum of the leaf values.
inline
int tree_leaf_sum_manual(
    const mu<tree_f>& _m
)
{
    const tree_f<mu<tree_f> >& _layer = _m.out();
    if (_layer.is_leaf)
    {
        return _layer.value;
    }
    return tree_leaf_sum_manual(_layer.left)
         + tree_leaf_sum_manual(_layer.right);
}

// tree_height_manual: edges on the longest root-to-leaf path (leaf = 0).
inline
int tree_height_manual(
    const mu<tree_f>& _m
)
{
    const tree_f<mu<tree_f> >& _layer = _m.out();
    if (_layer.is_leaf)
    {
        return 0;
    }
    const int _lh = tree_height_manual(_layer.left);
    const int _rh = tree_height_manual(_layer.right);
    return 1 + ((_lh > _rh) ? _lh : _rh);
}
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///             I.    MU                                                    ///
///////////////////////////////////////////////////////////////////////////////

bool tests_mu_default_empty();
bool tests_mu_in_nonempty();
bool tests_mu_out_returns_layer();
bool tests_mu_layer_type();
bool tests_mu_sharing();
bool tests_mu_nested_navigation();


///////////////////////////////////////////////////////////////////////////////
///             II.   TRAITS + DETECTION                                    ///
///////////////////////////////////////////////////////////////////////////////

bool tests_recursive_traits_mu();
bool tests_recursive_traits_project();
bool tests_corecursive_traits_mu();
bool tests_corecursive_traits_embed();
bool tests_traits_roundtrip();
bool tests_is_recursive();
bool tests_is_corecursive();
bool tests_custom_carrier_registration();


///////////////////////////////////////////////////////////////////////////////
///             III.  CATA                                                  ///
///////////////////////////////////////////////////////////////////////////////

bool tests_cata_nat_depth();
bool tests_cata_list_sum();
bool tests_cata_list_length();
bool tests_cata_list_to_string();
bool tests_cata_tree_leaf_sum();
bool tests_cata_tree_height();
bool tests_cata_native_vector();
bool tests_cata_base_case();
bool tests_cata_reflection_law();
bool tests_cata_algebra_forms();


///////////////////////////////////////////////////////////////////////////////
///             IV.   ANA                                                   ///
///////////////////////////////////////////////////////////////////////////////

bool tests_ana_nat();
bool tests_ana_list_range();
bool tests_ana_tree_balanced();
bool tests_ana_native_vector();
bool tests_ana_base_case();
bool tests_ana_produces_structure();
bool tests_ana_coalgebra_forms();


///////////////////////////////////////////////////////////////////////////////
///             V.    HYLO                                                  ///
///////////////////////////////////////////////////////////////////////////////

bool tests_hylo_nat_depth();
bool tests_hylo_list_sum();
bool tests_hylo_factorial();
bool tests_hylo_tree();
bool tests_hylo_equals_cata_ana();
bool tests_hylo_base_case();
bool tests_hylo_forms();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// recursion_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
recursion_spec()
{
    return dt::module_spec{
        "recursion",
        "In / out / empty / layer_type / shared-layer semantics",
        {
            dt::block_spec{
                "mu",
                "Mu",
                {
                    { "tests_mu_default_empty",
                      "A default-constructed mu holds no layer",
                      &tests_mu_default_empty },
                    { "tests_mu_in_nonempty",
                      "In boxes a layer, yielding a non-empty value",
                      &tests_mu_in_nonempty },
                    { "tests_mu_out_returns_layer",
                      "out() returns the boxed F-layer",
                      &tests_mu_out_returns_layer },
                    { "tests_mu_layer_type",
                      "The layer_type alias is exactly F applied to the fixed "
                      "point",
                      &tests_mu_layer_type },
                    { "tests_mu_sharing",
                      "The layer sits behind a shared_ptr: copies share it, "
                      "and In boxes a COPY of its argument (later mutation of "
                      "the source layer does not leak in)",
                      &tests_mu_sharing },
                    { "tests_mu_nested_navigation",
                      "A multi-layer value can be walked one layer at a time "
                      "through out(), and the same works for a functor with "
                      "two holes",
                      &tests_mu_nested_navigation },
                }
            },
            dt::block_spec{
                "traits",
                "Traits",
                {
                    { "tests_recursive_traits_mu",
                      "The mu<F> fold-side instance is well-formed",
                      &tests_recursive_traits_mu },
                    { "tests_recursive_traits_project",
                      "project peels exactly one layer, equal to out()",
                      &tests_recursive_traits_project },
                    { "tests_corecursive_traits_mu",
                      "The mu<F> build-side instance is well-formed",
                      &tests_corecursive_traits_mu },
                    { "tests_corecursive_traits_embed",
                      "embed builds exactly one layer, invertible by out()",
                      &tests_corecursive_traits_embed },
                    { "tests_traits_roundtrip",
                      "project and embed are mutual inverses on one layer",
                      &tests_traits_roundtrip },
                    { "tests_is_recursive",
                      "is_recursive recognises fold-side carriers and rejects "
                      "the rest",
                      &tests_is_recursive },
                    { "tests_is_corecursive",
                      "is_corecursive recognises build-side carriers and "
                      "rejects the rest",
                      &tests_is_corecursive },
                    { "tests_custom_carrier_registration",
                      "A native carrier (std::vector<int>) is made foldable "
                      "and buildable purely by specialising the traits -- no "
                      "rewrite as a fixed point",
                      &tests_custom_carrier_registration },
                }
            },
            dt::block_spec{
                "cata",
                "Cata",
                {
                    { "tests_cata_nat_depth",
                      "Folds a Peano value to its depth",
                      &tests_cata_nat_depth },
                    { "tests_cata_list_sum",
                      "Folds a list to the sum of its elements",
                      &tests_cata_list_sum },
                    { "tests_cata_list_length",
                      "Folds a list to its length -- the algebra ignores the "
                      "head payload and counts only structure, exercising the "
                      "payload-vs-hole distinction",
                      &tests_cata_list_length },
                    { "tests_cata_list_to_string",
                      "Folds a list to a std::string, so the result carrier "
                      "differs from both the fixed point and the element type "
                      "-- exercising the explicit _Result pin",
                      &tests_cata_list_to_string },
                    { "tests_cata_tree_leaf_sum",
                      "Folds a binary tree to the sum of its leaves -- a "
                      "Branch algebra combines the results of BOTH recursive "
                      "holes",
                      &tests_cata_tree_leaf_sum },
                    { "tests_cata_tree_height",
                      "Folds a binary tree to its height -- a Branch algebra "
                      "takes the max over both holes, another two-child fold",
                      &tests_cata_tree_height },
                    { "tests_cata_native_vector",
                      "Folds a plain std::vector<int> as a list, via its "
                      "recursive_traits registration -- cata runs over a "
                      "native carrier, not a mu",
                      &tests_cata_native_vector },
                    { "tests_cata_base_case",
                      "A one-layer value with no recursive holes folds by "
                      "applying the algebra to that single base layer",
                      &tests_cata_base_case },
                    { "tests_cata_reflection_law",
                      "cata with the embed algebra is the identity (cata[In] "
                      "== id): folding a value by re-boxing each layer "
                      "reproduces it",
                      &tests_cata_reflection_law },
                    { "tests_cata_algebra_forms",
                      "The algebra may be any callable",
                      &tests_cata_algebra_forms },
                }
            },
            dt::block_spec{
                "ana",
                "Ana",
                {
                    { "tests_ana_nat",
                      "Unfolds an int into a Peano value",
                      &tests_ana_nat },
                    { "tests_ana_list_range",
                      "Unfolds a seed into a descending list",
                      &tests_ana_list_range },
                    { "tests_ana_tree_balanced",
                      "Unfolds a depth seed into a full binary tree -- a "
                      "coalgebra with TWO holes (Branch seeds both children)",
                      &tests_ana_tree_balanced },
                    { "tests_ana_native_vector",
                      "Unfolds a seed directly into a std::vector<int>, via "
                      "its corecursive_traits registration -- ana builds a "
                      "native carrier, not a mu",
                      &tests_ana_native_vector },
                    { "tests_ana_base_case",
                      "When the coalgebra yields a hole-less layer "
                      "immediately, ana builds a single base layer",
                      &tests_ana_base_case },
                    { "tests_ana_produces_structure",
                      "ana lays down the expected layers, checked by walking "
                      "out() directly (not by a fold), so the unfold is "
                      "validated independently of cata",
                      &tests_ana_produces_structure },
                    { "tests_ana_coalgebra_forms",
                      "The coalgebra may be any callable",
                      &tests_ana_coalgebra_forms },
                }
            },
            dt::block_spec{
                "hylo",
                "Hylo",
                {
                    { "tests_hylo_nat_depth",
                      "Refolds an int seed to a depth without building a "
                      "Peano value",
                      &tests_hylo_nat_depth },
                    { "tests_hylo_list_sum",
                      "Refolds a seed into a sum through the list functor, no "
                      "list materialised",
                      &tests_hylo_list_sum },
                    { "tests_hylo_factorial",
                      "The textbook hylomorphism: unfold [n, n-1, ..., 1] and "
                      "fold by product, fused",
                      &tests_hylo_factorial },
                    { "tests_hylo_tree",
                      "Refolds a depth seed through the two-hole tree functor",
                      &tests_hylo_tree },
                    { "tests_hylo_equals_cata_ana",
                      "hylo[phi, psi] agrees with cata[phi] ",
                      &tests_hylo_equals_cata_ana },
                    { "tests_hylo_base_case",
                      "A seed the coalgebra maps straight to a hole-less "
                      "layer refolds by one algebra application",
                      &tests_hylo_base_case },
                    { "tests_hylo_forms",
                      "Both operands may be any callable",
                      &tests_hylo_forms },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing


///////////////////////////////////////////////////////////////////////////////
///             FIXTURE FUNCTOR + CARRIER INSTANCES  (djinterp scope)       ///
///////////////////////////////////////////////////////////////////////////////

// functor_traits<nat_f<_X>>
//   the identity-plus-unit functor: map recurses on the Succ hole, rebuilds
// Zero unchanged.
#ifndef DTEST_SPEC_MODE  // fixture instances - normal mode only
template<typename _X>
struct functor_traits<testing::nat_f<_X>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _X;

    template<typename _U>
    using rebind = testing::nat_f<_U>;

    template<typename _Function>
    static
    auto map(
        const testing::nat_f<_X>& _fa,
        _Function                 _f
    ) -> testing::nat_f<typename std::decay<decltype(_f(_fa.succ))>::type>
    {
        using out_t = typename std::decay<decltype(_f(_fa.succ))>::type;

        testing::nat_f<out_t> _r;
        if (_fa.is_zero)
        {
            _r.is_zero = true;
        }
        else
        {
            _r.is_zero = false;
            _r.succ    = _f(_fa.succ);
        }
        return _r;
    }
};

// functor_traits<list_f<_X>>
//   map threads the Int head unchanged and recurses on the tail hole.
template<typename _X>
struct functor_traits<testing::list_f<_X>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _X;

    template<typename _U>
    using rebind = testing::list_f<_U>;

    template<typename _Function>
    static
    auto map(
        const testing::list_f<_X>& _fa,
        _Function                  _f
    ) -> testing::list_f<typename std::decay<decltype(_f(_fa.tail))>::type>
    {
        using out_t = typename std::decay<decltype(_f(_fa.tail))>::type;

        testing::list_f<out_t> _r;
        if (_fa.is_nil)
        {
            _r.is_nil = true;
        }
        else
        {
            _r.is_nil = false;
            _r.head   = _fa.head;
            _r.tail   = _f(_fa.tail);
        }
        return _r;
    }
};

// functor_traits<tree_f<_X>>
//   map keeps a Leaf's value and recurses on BOTH Branch holes.
template<typename _X>
struct functor_traits<testing::tree_f<_X>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _X;

    template<typename _U>
    using rebind = testing::tree_f<_U>;

    template<typename _Function>
    static
    auto map(
        const testing::tree_f<_X>& _fa,
        _Function                  _f
    ) -> testing::tree_f<typename std::decay<decltype(_f(_fa.left))>::type>
    {
        using out_t = typename std::decay<decltype(_f(_fa.left))>::type;

        testing::tree_f<out_t> _r;
        if (_fa.is_leaf)
        {
            _r.is_leaf = true;
            _r.value   = _fa.value;
        }
        else
        {
            _r.is_leaf = false;
            _r.left    = _f(_fa.left);
            _r.right   = _f(_fa.right);
        }
        return _r;
    }
};


// recursive_traits<std::vector<int>>
//   a native carrier folded as a list: project peels the front element as the
// Cons payload and the remaining vector as the tail hole.
template<>
struct recursive_traits<std::vector<int>, void>
{
    using is_specialized = std::true_type;

    template<typename _A>
    using base = testing::list_f<_A>;

    static
    testing::list_f<std::vector<int> > project(
        const std::vector<int>& _v
    )
    {
        if (_v.empty())
        {
            return testing::list_nil<std::vector<int> >();
        }
        return testing::list_cons<std::vector<int> >(
            _v.front(),
            std::vector<int>(_v.begin() + 1, _v.end()));
    }
};

// corecursive_traits<std::vector<int>>
//   embed prepends the Cons payload onto the tail vector.
template<>
struct corecursive_traits<std::vector<int>, void>
{
    using is_specialized = std::true_type;

    template<typename _A>
    using base = testing::list_f<_A>;

    static
    std::vector<int> embed(
        const testing::list_f<std::vector<int> >& _layer
    )
    {
        if (_layer.is_nil)
        {
            return std::vector<int>();
        }
        std::vector<int> _r;
        _r.push_back(_layer.head);
        _r.insert(_r.end(), _layer.tail.begin(), _layer.tail.end());
        return _r;
    }
};
#endif  // !DTEST_SPEC_MODE  (fixture instances)


NS_END  // djinterp


#endif  // DJINTERP_TEST_RECURSION_TESTS_
