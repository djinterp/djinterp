/*******************************************************************************
* djinterp [parse]                                         expression_static.hpp
*
*   The compile-time face of an expression: the term encoded in the *type*,
* for the zero-overhead path.  Where expression.hpp's term is a heap-backed
* mu whose shape is decided at runtime, a static expression's shape is fixed
* at compile time -- each node is a distinct type, operands are stored by
* value, and a fold is template recursion the optimiser flattens.  It is the
* representation a math kernel wants: `constexpr e = add(lit(2.0), sq(x));`
* evaluates with no allocation and no dispatch.
*
*   THE MIRROR.  Two node kinds parallel the dynamic leaf / apply:
*
*     static_leaf<Atom>                 -- a leaf holding an atom by value
*     static_apply<OpTag, Operands...>  -- an operator applied to operand
*                                          nodes held by value in a tuple
*
* An operator is a *tag type* exposing its id: `OpTag::op_id_type` and
* `static constexpr OpTag::value`.  The tag names the operator in the type
* system; its value is the runtime id the dynamic term uses, so the two
* faces meet at reify.
*
*   PARITY, AND ITS LIMIT.  Construction, compile-time shape (arity / size /
* depth are static constexpr), a fold (static_evaluate, the compile-time
* catamorphism), and reify to the dynamic term all reach parity with the
* dynamic ops.  Rewriting does not: a rewritten static term generally has a
* different type, which the type system cannot produce from a value, so
* transforms stay dynamic -- reify first, rewrite there.  static_evaluate's
* application handler receives its operands as a std::array<R, N> rather than
* the dynamic vector<R> (an array is a literal type, so the fold stays
* constexpr); the leaf handler is identical.
*
*   PORTABILITY.  Compile-time shape and leaf construction are C++11;
* application construction and static_evaluate use the relaxed constexpr and
* index sequences of C++14 (D_CONSTEXPR_CPP14, std::tuple), running at compile
* time wherever the supplied algebra and atoms are literal.  reify is a
* runtime bridge (it builds the heap-backed dynamic term).
*
*
* path:      /inc/djinterp/parse/expression/expression_static.hpp
* link(s):   ch-recursion.tex, ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CRTP BASE                             (static_expr<Derived>)
      ------------------------------------------------------------

II.   NODES                                 (static_leaf / static_apply)
      ------------------------------------------------------------------

III.  FACTORIES                             (make_static_leaf / make_static_apply)
      ----------------------------------------------------------------------------

IV.   COMPILE-TIME SHAPE                     (static_arity / _size / _depth)
      ----------------------------------------------------------------------

V.    COMPILE-TIME FOLD                      (static_evaluate)
      --------------------------------------------------------

VI.   REIFY                                  (static term -> dynamic expression)
      --------------------------------------------------------------------------

VII.  DETECTION                              (is_static_expr / _leaf / _apply)
      ------------------------------------------------------------------------
*/

#ifndef DJINTERP_PARSE_EXPRESSION_EXPRESSION_STATIC_HPP
#define DJINTERP_PARSE_EXPRESSION_EXPRESSION_STATIC_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "./expression.hpp"
// re_std
#include "../../../re_std/utility/make_integer_sequence.hpp"  // re_std::index_sequence,
                                                              // make_index_sequence


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    CRTP BASE  (static_expr<Derived>)                    ///
///////////////////////////////////////////////////////////////////////////////

// static_expr
//   class: the CRTP base every static expression node derives from.  It
// tags a type as a static expression (so is_static_expr can recognize it by
// inheritance) and exposes the derived node through self().  It carries no
// state and imposes no vtable; the node's shape lives entirely in its type.
template<typename Derived>
class static_expr
{
public:
    // self
    //   the most-derived node, for generic code operating through the base.
    D_NODISCARD
    D_CONSTEXPR
    const Derived& self() const
    {
        return static_cast<const Derived&>(*this);
    }

protected:
    // only derived nodes construct the base
    static_expr() {}
};


///////////////////////////////////////////////////////////////////////////////
///             II.   NODES  (static_leaf / static_apply)                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // static_sum_sizes
    //   trait: the sum of node_size over a pack of operand nodes -- the
    // child contribution to an application's node_size.
    template<typename... Operands>
    struct static_sum_sizes;

    template<>
    struct static_sum_sizes<>
    {
        static D_CONSTEXPR std::size_t value = 0;
    };

    template<typename Head,
             typename... Tail>
    struct static_sum_sizes<Head, Tail...>
    {
        static D_CONSTEXPR std::size_t value =
            Head::node_size + static_sum_sizes<Tail...>::value;
    };

    // static_max_depths
    //   trait: the maximum node_depth over a pack of operand nodes -- the
    // child contribution to an application's node_depth.
    template<typename... Operands>
    struct static_max_depths;

    template<>
    struct static_max_depths<>
    {
        static D_CONSTEXPR std::size_t value = 0;
    };

    template<typename Head,
             typename... Tail>
    struct static_max_depths<Head, Tail...>
    {
        static D_CONSTEXPR std::size_t value =
            (Head::node_depth > static_max_depths<Tail...>::value)
                ? Head::node_depth
                : static_max_depths<Tail...>::value;
    };

NS_END  // internal


// static_leaf
//   class: a leaf of a static expression -- a node carrying one atom by
// value.  Its shape constants are the base case: no children, size and
// depth one.
template<typename Atom>
class static_leaf : public static_expr<static_leaf<Atom> >
{
public:
    using atom_type = Atom;

    static D_CONSTEXPR std::size_t arity      = 0;
    static D_CONSTEXPR std::size_t node_size  = 1;
    static D_CONSTEXPR std::size_t node_depth = 1;

    D_CONSTEXPR
    explicit
    static_leaf(
        const Atom& _atom
    )
        : m_atom(_atom)
    {}

    // atom
    //   the value carried at this leaf.
    D_NODISCARD
    D_CONSTEXPR
    const Atom& atom() const { return m_atom; }

private:
    Atom m_atom;
};


// static_apply
//   class: an application of an operator to operand nodes.  The operator is
// the tag OpTag (exposing op_id_type and a static constexpr value); the
// operands are the sub-nodes, held by value in a tuple, so the whole term
// is one composite value whose type is its shape.  Arity is the operand
// count; size and depth fold over the operands at compile time.
template<typename OpTag,
         typename... Operands>
class static_apply
    : public static_expr<static_apply<OpTag, Operands...> >
{
public:
    using op_tag_type   = OpTag;
    using op_id_type    = typename OpTag::op_id_type;
    using operands_type = std::tuple<Operands...>;

    static D_CONSTEXPR std::size_t arity      = sizeof...(Operands);
    static D_CONSTEXPR std::size_t node_size  =
        1 + internal::static_sum_sizes<Operands...>::value;
    static D_CONSTEXPR std::size_t node_depth =
        1 + internal::static_max_depths<Operands...>::value;

    D_CONSTEXPR_CPP14
    explicit
    static_apply(
        const Operands&... _operands
    )
        : m_operands(_operands...)
    {}

    // op_id
    //   the operator's runtime id (the tag's value) -- the bridge to the
    // dynamic term.
    D_NODISCARD
    D_CONSTEXPR
    op_id_type op_id() const { return OpTag::value; }

    // operands
    //   the tuple of operand nodes.
    D_NODISCARD
    D_CONSTEXPR
    const operands_type& operands() const { return m_operands; }

private:
    operands_type m_operands;
};


///////////////////////////////////////////////////////////////////////////////
///             III.  FACTORIES                                            ///
///////////////////////////////////////////////////////////////////////////////

// make_static_leaf
//   function: a static leaf carrying an atom (its type deduced).
template<typename Atom>
D_NODISCARD
D_CONSTEXPR
static_leaf<typename std::decay<Atom>::type>
make_static_leaf
(
    const Atom& _atom
)
{
    return static_leaf<typename std::decay<Atom>::type>(_atom);
}

// make_static_apply
//   function: a static application of the operator tag OpTag (named
// explicitly) to operand nodes (their types deduced).
template<typename OpTag,
         typename... Operands>
D_NODISCARD
D_CONSTEXPR_CPP14
static_apply<OpTag, typename std::decay<Operands>::type...>
make_static_apply
(
    const Operands&... _operands
)
{
    return static_apply<OpTag, typename std::decay<Operands>::type...>(
        _operands...);
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   COMPILE-TIME SHAPE                                   ///
///////////////////////////////////////////////////////////////////////////////
//   The shape of a static term is known from its type; these read the node
// constants as ordinary (constexpr) calls.

// static_arity
//   function: the number of immediate operands of a node (0 for a leaf).
template<typename Node>
D_NODISCARD
D_CONSTEXPR
std::size_t static_arity(const Node&) { return Node::arity; }

// static_size
//   function: the total node count of a term.
template<typename Node>
D_NODISCARD
D_CONSTEXPR
std::size_t static_size(const Node&) { return Node::node_size; }

// static_depth
//   function: the height of a term.
template<typename Node>
D_NODISCARD
D_CONSTEXPR
std::size_t static_depth(const Node&) { return Node::node_depth; }


///////////////////////////////////////////////////////////////////////////////
///             V.    COMPILE-TIME FOLD  (static_evaluate)                 ///
///////////////////////////////////////////////////////////////////////////////
//   The compile-time catamorphism.  static_evaluate is the static twin of
// expression_ops.hpp's evaluate: _on_leaf : Atom -> R interprets a leaf and
// _on_apply : (op_id, std::array<R, N>) -> R collapses an application from
// its folded operands.  Because a node kind is a type, the two cases are
// overloads rather than a runtime branch; the leaf / apply overloads and
// the per-operand step are mutually recursive, so they are declared first.

// -- forward declarations ---------------------------------------------------

template<typename Result,
         typename Atom,
         typename OnLeaf,
         typename OnApply>
D_CONSTEXPR_CPP14
Result
static_evaluate(const static_leaf<Atom>& _leaf,
                OnLeaf                     _on_leaf,
                OnApply                    _on_apply);

template<typename Result,
         typename OpTag,
         typename... Operands,
         typename OnLeaf,
         typename OnApply>
D_CONSTEXPR_CPP14
Result
static_evaluate(const static_apply<OpTag, Operands...>& _apply,
                OnLeaf                                     _on_leaf,
                OnApply                                    _on_apply);


NS_INTERNAL

    // static_evaluate_apply
    //   helper: folds an application -- evaluates each operand to Result,
    // collects the results into a std::array (a literal type, so the fold
    // stays constexpr), then hands the operator id and that array to the
    // application handler.
    template<typename Result,
             typename OpTag,
             typename... Operands,
             typename OnLeaf,
             typename OnApply,
             std::size_t... Indices>
    D_CONSTEXPR_CPP14
    Result
    static_evaluate_apply(
        const static_apply<OpTag, Operands...>& _apply,
        OnLeaf                                    _on_leaf,
        OnApply                                   _on_apply,
        re_std::index_sequence<Indices...>
    )
    {
        std::array<Result, sizeof...(Operands)> _results = {{
            ::djinterp::static_evaluate<Result>(
                std::get<Indices>(_apply.operands()),
                _on_leaf,
                _on_apply)...
        }};

        return _on_apply(_apply.op_id(), _results);
    }

NS_END  // internal


// -- definitions ------------------------------------------------------------

// static_evaluate (leaf)
template<typename Result,
         typename Atom,
         typename OnLeaf,
         typename OnApply>
D_CONSTEXPR_CPP14
Result
static_evaluate
(
    const static_leaf<Atom>& _leaf,
    OnLeaf                     _on_leaf,
    OnApply                    /*_on_apply*/
)
{
    return _on_leaf(_leaf.atom());
}

// static_evaluate (apply)
template<typename Result,
         typename OpTag,
         typename... Operands,
         typename OnLeaf,
         typename OnApply>
D_CONSTEXPR_CPP14
Result
static_evaluate
(
    const static_apply<OpTag, Operands...>& _apply,
    OnLeaf                                    _on_leaf,
    OnApply                                   _on_apply
)
{
    return internal::static_evaluate_apply<Result>(
        _apply,
        _on_leaf,
        _on_apply,
        re_std::make_index_sequence<sizeof...(Operands)>{});
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   REIFY  (static term -> dynamic expression)           ///
///////////////////////////////////////////////////////////////////////////////
//   Lowers a static term into the dynamic expression<OpId, Atom>, so a term
// built (and folded) at compile time can enter the runtime machinery --
// rewriting, rendering, substitution.  The target OpId / Atom are named
// explicitly: the caller says which dynamic language the static shape maps
// onto (the tag's value must be usable as OpId, each atom as Atom).  This
// is a runtime bridge -- it materialises the heap-backed term.

// -- forward declarations ---------------------------------------------------

template<typename OpId,
         typename Atom,
         typename LeafAtom>
D_NODISCARD
expression<OpId, Atom>
reify(const static_leaf<LeafAtom>& _leaf);

template<typename OpId,
         typename Atom,
         typename OpTag,
         typename... Operands>
D_NODISCARD
expression<OpId, Atom>
reify(const static_apply<OpTag, Operands...>& _apply);


NS_INTERNAL

    // reify_apply
    //   helper: reifies each operand and assembles the dynamic application.
    template<typename OpId,
             typename Atom,
             typename OpTag,
             typename... Operands,
             std::size_t... Indices>
    D_NODISCARD
    expression<OpId, Atom>
    reify_apply(
        const static_apply<OpTag, Operands...>& _apply,
        re_std::index_sequence<Indices...>
    )
    {
        std::vector<expression<OpId, Atom> > _children;
        _children.reserve(sizeof...(Operands));

        // reify each operand, in order, into the child vector
        const int _expand[] = { 0,
            ( _children.push_back(
                  ::djinterp::reify<OpId, Atom>(
                      std::get<Indices>(_apply.operands()))), 0 )... };
        static_cast<void>(_expand);

        return expr_apply<OpId, Atom>(
            static_cast<OpId>(_apply.op_id()), _children);
    }

NS_END  // internal


// -- definitions ------------------------------------------------------------

// reify (leaf)
template<typename OpId,
         typename Atom,
         typename LeafAtom>
D_NODISCARD
expression<OpId, Atom>
reify
(
    const static_leaf<LeafAtom>& _leaf
)
{
    return expr_leaf<OpId, Atom>(static_cast<Atom>(_leaf.atom()));
}

// reify (apply)
template<typename OpId,
         typename Atom,
         typename OpTag,
         typename... Operands>
D_NODISCARD
expression<OpId, Atom>
reify
(
    const static_apply<OpTag, Operands...>& _apply
)
{
    return internal::reify_apply<OpId, Atom>(
        _apply,
        re_std::make_index_sequence<sizeof...(Operands)>{});
}


///////////////////////////////////////////////////////////////////////////////
///             VII.  DETECTION                                            ///
///////////////////////////////////////////////////////////////////////////////

// is_static_leaf
//   trait: whether a type is a static_leaf<...>.
template<typename Type>
struct is_static_leaf : std::false_type
{};

template<typename Atom>
struct is_static_leaf<static_leaf<Atom> > : std::true_type
{};

// is_static_apply
//   trait: whether a type is a static_apply<...>.
template<typename Type>
struct is_static_apply : std::false_type
{};

template<typename OpTag,
         typename... Operands>
struct is_static_apply<static_apply<OpTag, Operands...> > : std::true_type
{};

// is_static_expr
//   trait: whether a type is a static expression node -- recognized by
// derivation from the CRTP base, so user node types built on static_expr
// are included, after cv-ref stripping.
template<typename Type>
struct is_static_expr
    : std::is_base_of<
          static_expr<typename std::decay<Type>::type>,
          typename std::decay<Type>::type>
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_static_leaf_v
template<typename Type>
static D_CONSTEXPR bool is_static_leaf_v = is_static_leaf<Type>::value;

// is_static_apply_v
template<typename Type>
static D_CONSTEXPR bool is_static_apply_v = is_static_apply<Type>::value;

// is_static_expr_v
template<typename Type>
static D_CONSTEXPR bool is_static_expr_v = is_static_expr<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if ( defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                             \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS == 1) )

// StaticLeaf
template<typename Type>
concept StaticLeaf = is_static_leaf<Type>::value;

// StaticApply
template<typename Type>
concept StaticApply = is_static_apply<Type>::value;

// StaticExpr
template<typename Type>
concept StaticExpr = is_static_expr<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSE_EXPRESSION_EXPRESSION_STATIC_HPP
