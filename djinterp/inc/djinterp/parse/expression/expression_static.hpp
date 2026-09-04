/******************************************************************************
* djinterp [expression]                                     expression_static.hpp
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
* index sequences of C++14 (D_CONSTEXPR14, std::tuple), running at compile
* time wherever the supplied algebra and atoms are literal.  reify is a
* runtime bridge (it builds the heap-backed dynamic term).
*
*
* TABLE OF CONTENTS
* =================
* I.    CRTP BASE                             (static_expr<Derived>)
* II.   NODES                                 (static_leaf / static_apply)
* III.  FACTORIES                             (make_static_leaf / make_static_apply)
* IV.   COMPILE-TIME SHAPE                     (static_arity / _size / _depth)
* V.    COMPILE-TIME FOLD                      (static_evaluate)
* VI.   REIFY                                  (static term -> dynamic expression)
* VII.  DETECTION                              (is_static_expr / _leaf / _apply)
*
*
* path:      /inc/djinterp/parse/expression/expression_static.hpp
* link(s):   ch-recursion.tex, ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#ifndef DJINTERP_EXPRESSION_EXPRESSION_STATIC_
#define DJINTERP_EXPRESSION_EXPRESSION_STATIC_ 1

// std
#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "./expression.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    CRTP BASE  (static_expr<Derived>)                    ///
///////////////////////////////////////////////////////////////////////////////

// static_expr
//   class: the CRTP base every static expression node derives from.  It
// tags a type as a static expression (so is_static_expr can recognize it by
// inheritance) and exposes the derived node through self().  It carries no
// state and imposes no vtable; the node's shape lives entirely in its type.
template<typename _Derived>
class static_expr
{
public:
    // self
    //   the most-derived node, for generic code operating through the base.
    D_NODISCARD
    D_CONSTEXPR
    const _Derived& self() const
    {
        return static_cast<const _Derived&>(*this);
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
    template<typename... _Operands>
    struct static_sum_sizes;

    template<>
    struct static_sum_sizes<>
    {
        static D_CONSTEXPR std::size_t value = 0;
    };

    template<typename _Head,
             typename... _Tail>
    struct static_sum_sizes<_Head, _Tail...>
    {
        static D_CONSTEXPR std::size_t value =
            _Head::node_size + static_sum_sizes<_Tail...>::value;
    };

    // static_max_depths
    //   trait: the maximum node_depth over a pack of operand nodes -- the
    // child contribution to an application's node_depth.
    template<typename... _Operands>
    struct static_max_depths;

    template<>
    struct static_max_depths<>
    {
        static D_CONSTEXPR std::size_t value = 0;
    };

    template<typename _Head,
             typename... _Tail>
    struct static_max_depths<_Head, _Tail...>
    {
        static D_CONSTEXPR std::size_t value =
            (_Head::node_depth > static_max_depths<_Tail...>::value)
                ? _Head::node_depth
                : static_max_depths<_Tail...>::value;
    };

NS_END  // internal


// static_leaf
//   class: a leaf of a static expression -- a node carrying one atom by
// value.  Its shape constants are the base case: no children, size and
// depth one.
template<typename _Atom>
class static_leaf : public static_expr<static_leaf<_Atom> >
{
public:
    using atom_type = _Atom;

    static D_CONSTEXPR std::size_t arity      = 0;
    static D_CONSTEXPR std::size_t node_size  = 1;
    static D_CONSTEXPR std::size_t node_depth = 1;

    D_CONSTEXPR
    explicit
    static_leaf(
        const _Atom& _atom
    )
        : m_atom(_atom)
    {}

    // atom
    //   the value carried at this leaf.
    D_NODISCARD
    D_CONSTEXPR
    const _Atom& atom() const { return m_atom; }

private:
    _Atom m_atom;
};


// static_apply
//   class: an application of an operator to operand nodes.  The operator is
// the tag _OpTag (exposing op_id_type and a static constexpr value); the
// operands are the sub-nodes, held by value in a tuple, so the whole term
// is one composite value whose type is its shape.  Arity is the operand
// count; size and depth fold over the operands at compile time.
template<typename _OpTag,
         typename... _Operands>
class static_apply
    : public static_expr<static_apply<_OpTag, _Operands...> >
{
public:
    using op_tag_type   = _OpTag;
    using op_id_type    = typename _OpTag::op_id_type;
    using operands_type = std::tuple<_Operands...>;

    static D_CONSTEXPR std::size_t arity      = sizeof...(_Operands);
    static D_CONSTEXPR std::size_t node_size  =
        1 + internal::static_sum_sizes<_Operands...>::value;
    static D_CONSTEXPR std::size_t node_depth =
        1 + internal::static_max_depths<_Operands...>::value;

    D_CONSTEXPR14
    explicit
    static_apply(
        const _Operands&... _operands
    )
        : m_operands(_operands...)
    {}

    // op_id
    //   the operator's runtime id (the tag's value) -- the bridge to the
    // dynamic term.
    D_NODISCARD
    D_CONSTEXPR
    op_id_type op_id() const { return _OpTag::value; }

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
template<typename _Atom>
D_NODISCARD
D_CONSTEXPR
static_leaf<typename std::decay<_Atom>::type>
make_static_leaf
(
    const _Atom& _atom
)
{
    return static_leaf<typename std::decay<_Atom>::type>(_atom);
}

// make_static_apply
//   function: a static application of the operator tag _OpTag (named
// explicitly) to operand nodes (their types deduced).
template<typename _OpTag,
         typename... _Operands>
D_NODISCARD
D_CONSTEXPR14
static_apply<_OpTag, typename std::decay<_Operands>::type...>
make_static_apply
(
    const _Operands&... _operands
)
{
    return static_apply<_OpTag, typename std::decay<_Operands>::type...>(
        _operands...);
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   COMPILE-TIME SHAPE                                   ///
///////////////////////////////////////////////////////////////////////////////
//   The shape of a static term is known from its type; these read the node
// constants as ordinary (constexpr) calls.

// static_arity
//   function: the number of immediate operands of a node (0 for a leaf).
template<typename _Node>
D_NODISCARD
D_CONSTEXPR
std::size_t static_arity(const _Node&) { return _Node::arity; }

// static_size
//   function: the total node count of a term.
template<typename _Node>
D_NODISCARD
D_CONSTEXPR
std::size_t static_size(const _Node&) { return _Node::node_size; }

// static_depth
//   function: the height of a term.
template<typename _Node>
D_NODISCARD
D_CONSTEXPR
std::size_t static_depth(const _Node&) { return _Node::node_depth; }


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

template<typename _Result,
         typename _Atom,
         typename _OnLeaf,
         typename _OnApply>
D_CONSTEXPR14
_Result
static_evaluate(const static_leaf<_Atom>& _leaf,
                _OnLeaf                    _on_leaf,
                _OnApply                   _on_apply);

template<typename _Result,
         typename _OpTag,
         typename... _Operands,
         typename _OnLeaf,
         typename _OnApply>
D_CONSTEXPR14
_Result
static_evaluate(const static_apply<_OpTag, _Operands...>& _apply,
                _OnLeaf                                    _on_leaf,
                _OnApply                                   _on_apply);


NS_INTERNAL

    // static_evaluate_apply
    //   helper: folds an application -- evaluates each operand to _Result,
    // collects the results into a std::array (a literal type, so the fold
    // stays constexpr), then hands the operator id and that array to the
    // application handler.
    template<typename _Result,
             typename _OpTag,
             typename... _Operands,
             typename _OnLeaf,
             typename _OnApply,
             std::size_t... _Indices>
    D_CONSTEXPR14
    _Result
    static_evaluate_apply(
        const static_apply<_OpTag, _Operands...>& _apply,
        _OnLeaf                                   _on_leaf,
        _OnApply                                  _on_apply,
        std::index_sequence<_Indices...>
    )
    {
        std::array<_Result, sizeof...(_Operands)> _results = {{
            ::djinterp::static_evaluate<_Result>(
                std::get<_Indices>(_apply.operands()),
                _on_leaf,
                _on_apply)...
        }};

        return _on_apply(_apply.op_id(), _results);
    }

NS_END  // internal


// -- definitions ------------------------------------------------------------

// static_evaluate (leaf)
template<typename _Result,
         typename _Atom,
         typename _OnLeaf,
         typename _OnApply>
D_CONSTEXPR14
_Result
static_evaluate
(
    const static_leaf<_Atom>& _leaf,
    _OnLeaf                    _on_leaf,
    _OnApply                   /*_on_apply*/
)
{
    return _on_leaf(_leaf.atom());
}

// static_evaluate (apply)
template<typename _Result,
         typename _OpTag,
         typename... _Operands,
         typename _OnLeaf,
         typename _OnApply>
D_CONSTEXPR14
_Result
static_evaluate
(
    const static_apply<_OpTag, _Operands...>& _apply,
    _OnLeaf                                   _on_leaf,
    _OnApply                                  _on_apply
)
{
    return internal::static_evaluate_apply<_Result>(
        _apply,
        _on_leaf,
        _on_apply,
        std::make_index_sequence<sizeof...(_Operands)>{});
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   REIFY  (static term -> dynamic expression)           ///
///////////////////////////////////////////////////////////////////////////////
//   Lowers a static term into the dynamic expression<OpId, Atom>, so a term
// built (and folded) at compile time can enter the runtime machinery --
// rewriting, rendering, substitution.  The target _OpId / _Atom are named
// explicitly: the caller says which dynamic language the static shape maps
// onto (the tag's value must be usable as _OpId, each atom as _Atom).  This
// is a runtime bridge -- it materialises the heap-backed term.

// -- forward declarations ---------------------------------------------------

template<typename _OpId,
         typename _Atom,
         typename _LeafAtom>
D_NODISCARD
expression<_OpId, _Atom>
reify(const static_leaf<_LeafAtom>& _leaf);

template<typename _OpId,
         typename _Atom,
         typename _OpTag,
         typename... _Operands>
D_NODISCARD
expression<_OpId, _Atom>
reify(const static_apply<_OpTag, _Operands...>& _apply);


NS_INTERNAL

    // reify_apply
    //   helper: reifies each operand and assembles the dynamic application.
    template<typename _OpId,
             typename _Atom,
             typename _OpTag,
             typename... _Operands,
             std::size_t... _Indices>
    D_NODISCARD
    expression<_OpId, _Atom>
    reify_apply(
        const static_apply<_OpTag, _Operands...>& _apply,
        std::index_sequence<_Indices...>
    )
    {
        std::vector<expression<_OpId, _Atom> > _children;
        _children.reserve(sizeof...(_Operands));

        // reify each operand, in order, into the child vector
        const int _expand[] = { 0,
            ( _children.push_back(
                  ::djinterp::reify<_OpId, _Atom>(
                      std::get<_Indices>(_apply.operands()))), 0 )... };
        static_cast<void>(_expand);

        return expr_apply<_OpId, _Atom>(
            static_cast<_OpId>(_apply.op_id()), _children);
    }

NS_END  // internal


// -- definitions ------------------------------------------------------------

// reify (leaf)
template<typename _OpId,
         typename _Atom,
         typename _LeafAtom>
D_NODISCARD
expression<_OpId, _Atom>
reify
(
    const static_leaf<_LeafAtom>& _leaf
)
{
    return expr_leaf<_OpId, _Atom>(static_cast<_Atom>(_leaf.atom()));
}

// reify (apply)
template<typename _OpId,
         typename _Atom,
         typename _OpTag,
         typename... _Operands>
D_NODISCARD
expression<_OpId, _Atom>
reify
(
    const static_apply<_OpTag, _Operands...>& _apply
)
{
    return internal::reify_apply<_OpId, _Atom>(
        _apply,
        std::make_index_sequence<sizeof...(_Operands)>{});
}


///////////////////////////////////////////////////////////////////////////////
///             VII.  DETECTION                                            ///
///////////////////////////////////////////////////////////////////////////////

// is_static_leaf
//   trait: whether a type is a static_leaf<...>.
template<typename _Type>
struct is_static_leaf : std::false_type
{};

template<typename _Atom>
struct is_static_leaf<static_leaf<_Atom> > : std::true_type
{};

// is_static_apply
//   trait: whether a type is a static_apply<...>.
template<typename _Type>
struct is_static_apply : std::false_type
{};

template<typename _OpTag,
         typename... _Operands>
struct is_static_apply<static_apply<_OpTag, _Operands...> > : std::true_type
{};

// is_static_expr
//   trait: whether a type is a static expression node -- recognized by
// derivation from the CRTP base, so user node types built on static_expr
// are included, after cv-ref stripping.
template<typename _Type>
struct is_static_expr
    : std::is_base_of<
          static_expr<typename std::decay<_Type>::type>,
          typename std::decay<_Type>::type>
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_static_leaf_v
template<typename _Type>
static D_CONSTEXPR bool is_static_leaf_v = is_static_leaf<_Type>::value;

// is_static_apply_v
template<typename _Type>
static D_CONSTEXPR bool is_static_apply_v = is_static_apply<_Type>::value;

// is_static_expr_v
template<typename _Type>
static D_CONSTEXPR bool is_static_expr_v = is_static_expr<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if ( defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                             \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS == 1) )

// StaticLeaf
template<typename _Type>
concept StaticLeaf = is_static_leaf<_Type>::value;

// StaticApply
template<typename _Type>
concept StaticApply = is_static_apply<_Type>::value;

// StaticExpr
template<typename _Type>
concept StaticExpr = is_static_expr<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_EXPRESSION_EXPRESSION_STATIC_
