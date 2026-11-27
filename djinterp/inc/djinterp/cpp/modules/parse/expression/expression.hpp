/*******************************************************************************
* djinterp [cpp][parse]                                           expression.hpp
*
*   The common core of every expression language in the framework - math,
* parser, CLI predicate, and whatever comes next.  An expression is a
* recursive term over a signature: at each node either a leaf carrying an
* atom, or an operator applied to child expressions.  This header supplies
* exactly the part common to all of them, and nothing a derived language
* would want to own.
*
*   AN EXPRESSION IS mu OF ONE FUNCTOR.  The whole term is the least fixed
* point of a single leaf-inclusive signature functor:
*
*     expr_layer<OpId, Atom, X>  =  leaf(Atom)  |  apply(OpId, [X])
*     expression<OpId, Atom>     =  mu< expr_layer<OpId, Atom, _> >
*
* Registering functor_traits for expr_layer is all it takes: the universal
* fold cata (recursion.hpp) then interprets the term, ana builds it, hylo
* refolds it, and cofree<expr_layer, Ann> gives the annotated variant - one
* functor serving the bare term and its decorated form alike.  Evaluate,
* render, type-check, simplify, and cost are each just an algebra
* expr_layer<OpId, Atom, R> -> R handed to cata; the structural recursion is
* written once, in recursion.hpp, never here.
*
*   WHY mu AND NOT free.  free<F, A> would model the same object (its Pure A
* is this functor's leaf variant), and its free_bind is a ready substitution
* - but mu is what recursion.hpp is built for ("an expression IR" is its
* named client), it hands back ana and hylo that free does not, and its one
* leaf-inclusive functor is reused verbatim by cofree for annotation instead
* of needing a second, leaf-free functor.  Substitution and rewriting are
* provided as cata algebras (expression_ops.hpp) rather than lost.
*
*   WHAT LIVES WHERE.  This header is the noun: the operator model, the
* signature functor and its Functor instance, the term and its constructors,
* the annotated term - a class derived from the cofree, so that a function
* template deduces it - and detection.  The verbs - fold / evaluate / query /
* rewrite / substitute - live in expression_ops.hpp; a compile-time typed
* mirror in expression_static.hpp; precedence-aware printing in
* expression_render.hpp; the precedence-climbing parser in
* expression_parse.hpp.  cata (recursion.hpp) already folds the term with no
* further wiring.
*
*   REQUIREMENTS.  Atom and OpId must be default-constructible, copyable,
* and (for signature lookup) OpId equality-comparable - the same shape mu
* and cofree already ask of a layer.  The dynamic term is heap-backed
* (shared_ptr, via mu) and not constexpr; the compile-time face is
* expression_static.hpp.
*
*
* path:      /inc/djinterp/cpp/modules/parse/expression/expression.hpp
* link(s):   ch-recursion.tex, ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    OPERATOR MODEL
      --------------
      1.    associativity / fixity
      2.    operator_descriptor
      3.    operator_signature<OpId, Value, Container>

II.   SIGNATURE FUNCTOR                    (expr_layer<OpId, Atom, Child>)
      --------------------------------------------------------------------

III.  FUNCTOR REGISTRATION                 (functor_traits<expr_layer>)
      -----------------------------------------------------------------

IV.   THE EXPRESSION TERM
      -------------------
      1.    expression<OpId, Atom>           (= mu<expr_layer>)
      2.    constructors                      (expr_leaf / expr_apply)
      3.    observers                         (is_leaf / atom_of / ...)
      4.    annotated_expression<OpId, Atom, Ann>   (a cofree<expr_layer, Ann>)
            cofree_map / cofree_extend / annotated_cata, and its traits

V.    STRUCTURAL DETECTION                  (is_expr_layer / is_expression)
      ---------------------------------------------------------------------
*/

#ifndef DJINTERP_PARSE_EXPRESSION_HPP
#define DJINTERP_PARSE_EXPRESSION_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <memory>       // std::shared_ptr, std::make_shared
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../../../meta/type_utility.hpp"  // void_t
#include "../../../meta/kv_pair.hpp"
#include "../../../util/lookup/lookup_sentinels.hpp"
#include "../../../functional/functor.hpp"
#include "../../../functional/comonad.hpp"
#include "../../../functional/recursion.hpp"
#include "../../../functional/cofree.hpp"
// re_std
#include "../../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::int32_t


NS_DJINTERP


//   Nothing here is a new carrier: the term is mu (recursion.hpp) over the
// one functor defined below, folding is cata, annotation is cofree, and the
// operator model is plain data.  The module is the expression-shaped naming
// of machinery that already exists.


///////////////////////////////////////////////////////////////////////////////
///             I.    OPERATOR MODEL                                        ///
///////////////////////////////////////////////////////////////////////////////

// =================================================================
//  1. associativity / fixity
// =================================================================

// associativity
//   typedef: how repeated infix operators of equal precedence group.
// Drives both parenthesization (rendering) and precedence climbing
// (parsing).
typedef re_std::int32_t associativity;

// DAssoc*
//   constants: the associativity classes.
constexpr associativity DAssocNone  = 0;   // non-associative (a op b op c ill-formed)
constexpr associativity DAssocLeft  = 1;   // (a op b) op c
constexpr associativity DAssocRight = 2;   // a op (b op c)

// fixity
//   typedef: where an operator sits relative to its operands.
typedef re_std::int32_t fixity;

// DFix*
//   constants: the fixity classes.
constexpr fixity DFixNullary  = 0;   // no operands (a constant symbol: pi, true)
constexpr fixity DFixPrefix   = 1;   // op x        (-x, not x)
constexpr fixity DFixInfix    = 2;   // x op y       (x + y, x and y)
constexpr fixity DFixPostfix  = 3;   // x op         (x!, x++)
constexpr fixity DFixMatchfix = 4;   // open x close (|x|, (x), [x])


// =================================================================
//  2. operator_descriptor
// =================================================================

// operator_descriptor
//   struct: the metadata common to every operator in every expression
// language - arity and the precedence / associativity / fixity a renderer
// and a parser both need, plus algebraic flags a simplifier can consult.  It
// carries no identity of its own: it is the *value* of a signature entry,
// keyed by the operator id.  A literal type, so the compile-time face may
// hold a constexpr table of these and the dynamic face a runtime one.
struct operator_descriptor
{
    unsigned      arity;
    int           precedence;
    associativity assoc;
    fixity        fix;
    bool          commutative;
    bool          associative;
    bool          idempotent;
    const char*   spelling;

    // operator_descriptor (default)
    D_CONSTEXPR_CPP14
    operator_descriptor()
        : arity       (0)
        , precedence  (0)
        , assoc       (DAssocNone)
        , fix         (DFixNullary)
        , commutative (false)
        , associative (false)
        , idempotent  (false)
        , spelling    ("")
    {}

    // operator_descriptor (full)
    //   the common fields carry defaults so a language names only what it
    // must; flags default off.
    D_CONSTEXPR_CPP14
    operator_descriptor(
        unsigned      _arity,
        int           _precedence  = 0,
        associativity _assoc       = DAssocLeft,
        fixity        _fix         = DFixInfix,
        const char*   _spelling    = "",
        bool          _commutative = false,
        bool          _associative = false,
        bool          _idempotent  = false
    )
        : arity       (_arity)
        , precedence  (_precedence)
        , assoc       (_assoc)
        , fix         (_fix)
        , commutative (_commutative)
        , associative (_associative)
        , idempotent  (_idempotent)
        , spelling    (_spelling)
    {}
};


// =================================================================
//  3. operator_signature<OpId, Value, Container>
// =================================================================

// operator_signature
//   class: the described operator set of one expression language - simply
// a sequence of key-value pairs, each an operator id mapped to its metadata.
// Container-agnostic (any sequence of DKvPair rows: vector, deque, array,
// list) and value-agnostic (the value defaults to operator_descriptor but
// may be any per-operator payload a language wants).  A renderer consults it
// for spelling and parenthesization, a parser for precedence and fixity.
// Lookup is a first-match-wins walk - the intuition of the lookup family -
// and a miss yields a null value or the lookup_npos index.
template<typename OpId,
         typename Value      = operator_descriptor,
         typename Container = std::vector<DKvPair<OpId, Value> > >
class operator_signature
{
public:
    using op_id_type     = OpId;
    using value_type     = Value;
    using entry_type     = DKvPair<OpId, Value>;
    using container_type = Container;

    operator_signature()
        : m_entries()
    {}

    // operator_signature (from a prebuilt sequence)
    //   lets a language supply any container of key-value pairs - a
    // constexpr std::array of them, say - instead of building one entry at
    // a time.
    explicit
    operator_signature(
        const container_type& _entries
    )
        : m_entries(_entries)
    {}

    // define
    //   method: appends the entry (op -> value).  Returns *this for
    // chaining.  Requires a back-insertable container; the prebuilt-sequence
    // constructor covers fixed ones.
    operator_signature&
    define
    (
        const OpId&  _op,
        const Value& _value
    )
    {
        m_entries.push_back(entry_type(_op, _value));

        return *this;
    }

    // describe
    //   method: the value bound to an operator id, or null on a miss.
    D_NODISCARD
    const Value*
    describe
    (
        const OpId& _op
    ) const
    {
        for (typename container_type::const_iterator _it = m_entries.begin();
             _it != m_entries.end();
             ++_it)
        {
            if (_it->key == _op)
            {
                return &_it->value;
            }
        }

        return nullptr;
    }

    // index_of
    //   method: the position of an operator's entry, or lookup_npos on a
    // miss.
    D_NODISCARD
    std::size_t
    index_of
    (
        const OpId& _op
    ) const
    {
        std::size_t _i = 0;

        for (typename container_type::const_iterator _it = m_entries.begin();
             _it != m_entries.end();
             ++_it, ++_i)
        {
            if (_it->key == _op)
            {
                return _i;
            }
        }

        return lookup_npos;
    }

    // has
    //   method: whether an operator id is described here.
    D_NODISCARD
    bool
    has
    (
        const OpId& _op
    ) const
    {
        return (describe(_op) != nullptr);
    }

    // entries
    //   accessor: the underlying sequence of key-value pairs.
    D_NODISCARD
    const container_type& entries() const { return m_entries; }

    // introspection
    D_NODISCARD std::size_t size()  const { return m_entries.size();  }
    D_NODISCARD bool        empty() const { return m_entries.empty(); }

private:
    container_type m_entries;
};


///////////////////////////////////////////////////////////////////////////////
///             II.   SIGNATURE FUNCTOR  (expr_layer<OpId, Atom, Child>)    ///
///////////////////////////////////////////////////////////////////////////////

// expr_layer
//   class: one unrolled layer of an expression - the signature functor
// whose fixed point is the term.  A layer is either a leaf carrying an atom,
// or an application of an operator to a sequence of children of type Child
// (the recursive positions).  As a functor it is single-argument in Child
// (OpId and Atom are fixed), and mapping a layer maps only the children,
// leaving the operator and any atom in place - the shape recursion.hpp and
// cofree.hpp fold and annotate over.
template<typename OpId,
         typename Atom,
         typename Child>
class expr_layer
{
public:
    using op_id_type    = OpId;
    using atom_type     = Atom;
    using child_type    = Child;
    using children_type = std::vector<Child>;

    // expr_layer (default)
    //   a leaf carrying a default atom.  Required because mu and cofree box
    // a default-constructible layer.
    expr_layer()
        : m_is_leaf  (true)
        , m_atom     ()
        , m_op       ()
        , m_children ()
    {}

    // leaf
    //   factory: a leaf node carrying an atom.
    D_NODISCARD
    static
    expr_layer leaf(
        const Atom& _atom
    )
    {
        expr_layer _layer;
        _layer.m_is_leaf = true;
        _layer.m_atom    = _atom;

        return _layer;
    }

    // apply
    //   factory: an application of an operator to its children (which may be
    // empty, for a nullary operator).
    D_NODISCARD
    static
    expr_layer apply(
        const OpId&          _op,
        const children_type& _children
    )
    {
        expr_layer _layer;
        _layer.m_is_leaf  = false;
        _layer.m_op       = _op;
        _layer.m_children = _children;

        return _layer;
    }

    // observers
    D_NODISCARD bool         is_leaf()  const { return m_is_leaf;  }
    D_NODISCARD bool         is_apply() const { return !m_is_leaf; }
    D_NODISCARD const Atom& atom()     const { return m_atom;     }
    D_NODISCARD const OpId& op()       const { return m_op;       }

    D_NODISCARD
    const children_type& children() const { return m_children; }

    D_NODISCARD
    std::size_t arity() const { return m_children.size(); }

private:
    bool          m_is_leaf;
    Atom          m_atom;
    OpId          m_op;
    children_type m_children;
};


///////////////////////////////////////////////////////////////////////////////
///             III.  FUNCTOR REGISTRATION  (functor_traits<expr_layer>)    ///
///////////////////////////////////////////////////////////////////////////////
//   Registering expr_layer as a Functor is the single wiring step that lets
// the whole recursion-scheme layer act on the term: functor_map rebuilds a
// layer at a new child type, which is exactly what cata / ana / hylo
// (recursion.hpp) and cofree (cofree.hpp) call to fold, build, and annotate.
// map touches only the children; the operator and atom are structure, not
// contents.

// functor_traits<expr_layer<OpId, Atom, Child>>
template<typename OpId,
         typename Atom,
         typename Child>
struct functor_traits<expr_layer<OpId, Atom, Child>, void>
{
    using is_specialized = std::true_type;
    using value_type     = Child;

    template<typename To>
    using rebind = expr_layer<OpId, Atom, To>;

    // map
    //   applies _function to each child, yielding a layer over the mapped
    // child type; a leaf passes through unchanged (bar the child-type
    // rebind), an application keeps its operator and maps its children.
    template<typename Layer,
             typename Function>
    static
    expr_layer<OpId, Atom,
        typename std::decay<decltype(std::declval<Function&>()(
            std::declval<const Child&>()))>::type>
    map
    (
        Layer&&  _layer,
        Function _function
    )
    {
        using to_type = typename std::decay<decltype(
            std::declval<Function&>()(std::declval<const Child&>()))>::type;
        using result_layer = expr_layer<OpId, Atom, to_type>;

        if (_layer.is_leaf())
        {
            return result_layer::leaf(_layer.atom());
        }

        std::vector<to_type> _mapped;
        _mapped.reserve(_layer.children().size());

        // map only the recursive positions
        for (std::size_t _i = 0; _i < _layer.children().size(); ++_i)
        {
            _mapped.push_back(_function(_layer.children()[_i]));
        }

        return result_layer::apply(_layer.op(), _mapped);
    }
};


///////////////////////////////////////////////////////////////////////////////
///             IV.   THE EXPRESSION TERM                                   ///
///////////////////////////////////////////////////////////////////////////////

// =================================================================
//  1. expression<OpId, Atom>
// =================================================================

NS_INTERNAL

    // expr_carrier
    //   helper: binds OpId and Atom so expr_layer presents as the
    // single-argument template-template parameter mu (and cofree) require -
    // the same nested-alias device parser_layer uses for free.
    template<typename OpId,
             typename Atom>
    struct expr_carrier
    {
        template<typename Child>
        using layer = expr_layer<OpId, Atom, Child>;
    };

NS_END  // internal


// expression
//   alias: an expression over operator ids OpId and atoms Atom - the
// least fixed point of expr_layer.  cata folds it, ana builds it, hylo
// refolds it (all from recursion.hpp), with no further registration.
template<typename OpId,
         typename Atom>
using expression = mu<internal::expr_carrier<OpId, Atom>::template layer>;


// =================================================================
//  2. constructors
// =================================================================

// expr_leaf
//   function: a leaf expression carrying an atom.
template<typename OpId,
         typename Atom>
D_NODISCARD
expression<OpId, Atom> expr_leaf(
    const Atom& _atom
)
{
    return expression<OpId, Atom>::In(
        expression<OpId, Atom>::layer_type::leaf(_atom));
}

// expr_apply
//   function: an operator applied to a vector of child expressions.
template<typename OpId,
         typename Atom>
D_NODISCARD
expression<OpId, Atom> expr_apply(
    const OpId&                                 _op,
    const std::vector<expression<OpId, Atom> >& _children
)
{
    return expression<OpId, Atom>::In(
        expression<OpId, Atom>::layer_type::apply(_op, _children));
}

// expr_apply (nullary)
//   function: an operator with no operands - a constant symbol.
template<typename OpId,
         typename Atom>
D_NODISCARD
expression<OpId, Atom> expr_apply(
    const OpId& _op
)
{
    return expr_apply<OpId, Atom>(
        _op, std::vector<expression<OpId, Atom> >());
}

// expr_apply (unary)
//   function: an operator applied to one child.
template<typename OpId,
         typename Atom>
D_NODISCARD
expression<OpId, Atom> expr_apply(
    const OpId&                   _op,
    const expression<OpId, Atom>& _child
)
{
    std::vector<expression<OpId, Atom> > _children;
    _children.push_back(_child);

    return expr_apply<OpId, Atom>(_op, _children);
}

// expr_apply (binary)
//   function: an operator applied to two children.
template<typename OpId,
         typename Atom>
D_NODISCARD
expression<OpId, Atom> expr_apply(
    const OpId&                   _op,
    const expression<OpId, Atom>& _left,
    const expression<OpId, Atom>& _right
)
{
    std::vector<expression<OpId, Atom> > _children;
    _children.reserve(2);
    _children.push_back(_left);
    _children.push_back(_right);

    return expr_apply<OpId, Atom>(_op, _children);
}


// =================================================================
//  3. observers
// =================================================================

// is_leaf
//   function: whether the expression's root node is a leaf.
template<typename OpId,
         typename Atom>
D_NODISCARD
bool is_leaf(
    const expression<OpId, Atom>& _expression
)
{
    return _expression.out().is_leaf();
}

// atom_of
//   function: the atom at a leaf root.  Precondition: is_leaf.
template<typename OpId,
         typename Atom>
D_NODISCARD
const Atom& atom_of(
    const expression<OpId, Atom>& _expression
)
{
    return _expression.out().atom();
}

// operator_of
//   function: the operator id at an application root.  Precondition:
// !is_leaf.
template<typename OpId,
         typename Atom>
D_NODISCARD
const OpId& operator_of(
    const expression<OpId, Atom>& _expression
)
{
    return _expression.out().op();
}

// children_of
//   function: the child expressions at an application root.
template<typename OpId,
         typename Atom>
D_NODISCARD
const std::vector<expression<OpId, Atom> >& children_of(
    const expression<OpId, Atom>& _expression
)
{
    return _expression.out().children();
}


// =================================================================
//  4. annotated_expression<OpId, Atom, Ann>
// =================================================================

// annotated_expression (forward)
//   class: declared ahead of its carrier, which names it as the type of a
// node's children.
template<typename OpId,
         typename Atom,
         typename Annotation>
class annotated_expression;


NS_INTERNAL

    // expr_child
    //   helper: the type a layer of an annotated expression holds where its
    // carrier is given Child. A cofree hands its functor
    // shared_ptr<cofree<F, Ann>> as the recursive hole; for the annotated
    // expression's own carrier that one hole is retyped to the node,
    // shared_ptr<annotated_expression<OpId, Atom, Ann>>, so a node's
    // children read as the node and a const reference binds to them. Every
    // other Child - the folded F<Result> that cata rebuilds, the values
    // project reads - is left as it is.
    template<typename OpId,
             typename Atom,
             typename Child>
    struct expr_child
    {
        using type = Child;
    };

    // annotated_carrier
    //   helper: expr_layer as the single-argument functor the annotated
    // expression's cofree is over, its children's type through expr_child.
    // The plain term keeps expr_carrier.
    template<typename OpId,
             typename Atom>
    struct annotated_carrier
    {
        template<typename Child>
        using layer =
            expr_layer<OpId, Atom,
                       typename expr_child<OpId, Atom, Child>::type>;
    };

    template<typename OpId,
             typename Atom,
             typename Annotation>
    struct expr_child<
        OpId,
        Atom,
        std::shared_ptr<DCofree<annotated_carrier<OpId, Atom>::template layer,
                                Annotation> > >
    {
        using type = std::shared_ptr<
            ::djinterp::annotated_expression<OpId, Atom, Annotation> >;
    };

NS_END  // internal


// annotated_expression
//   class: an expression whose every node also carries an annotation of
// type Annotation - the cofree comonad over the very same signature functor.
// Attribute grammars, inferred-type decoration, source spans, and cached
// values are all annotations; extract reads a node's, extend re-decorates
// from whole sub-trees (comonad.hpp), and annotated_cata folds one.
//   A class derived from the cofree, not an alias of it: an alias puts its
// three parameters in a non-deduced context ([temp.deduct.type]/5), so a
// function template taking one could not deduce them. As a class, a call
// such as emit(doc) deduces OpId, Atom and Annotation. A node's children are
// shared_ptr<annotated_expression>, so they read as the node; make, the
// map, extend and annotated_cata below keep the type, so a result is an
// annotated_expression too. It converts from the cofree it derives from,
// and to it, as a derived class does. Copying and moving are the implicit
// ones.
template<typename OpId,
         typename Atom,
         typename Annotation>
class annotated_expression
    : public DCofree<internal::annotated_carrier<OpId, Atom>::template layer,
                     Annotation>
{
public:
    using base_type  =
        DCofree<internal::annotated_carrier<OpId, Atom>::template layer,
                Annotation>;
    using layer_type = typename base_type::layer_type;
    using value_type = Annotation;

    // annotated_expression (default)
    //   a node with a default annotation and a default layer, as the
    // cofree's.
    annotated_expression()
        : base_type()
    {}

    // annotated_expression (from the cofree)
    //   the node the cofree is: a conversion, so a value the cofree's own
    // functions make is one of these again.
    annotated_expression(
        const base_type& _node
    )
        : base_type(_node)
    {}

    // make
    //   factory: a node with the given annotation and layer of children.
    static annotated_expression make(
        const Annotation& _head,
        const layer_type& _layer
    )
    {
        return annotated_expression(base_type::make(_head, _layer));
    }
};


// cofree_map, cofree_extend (annotated_expression; declared)
//   declared ahead of the helpers below, which call them for each child, so
// that the helpers find them and not the cofree's own.
template<typename OpId,
         typename Atom,
         typename Annotation,
         typename Function>
D_NODISCARD
annotated_expression<OpId, Atom,
                     typename std::decay<decltype(std::declval<Function&>()(
                         std::declval<const Annotation&>()))>::type>
cofree_map(
    const annotated_expression<OpId, Atom, Annotation>& _node,
    Function                                            _function
);

template<typename OpId,
         typename Atom,
         typename Annotation,
         typename Function>
D_NODISCARD
annotated_expression<OpId, Atom,
                     typename std::decay<decltype(std::declval<Function&>()(
                         std::declval<const annotated_expression<
                             OpId, Atom, Annotation>&>()))>::type>
cofree_extend(
    const annotated_expression<OpId, Atom, Annotation>& _node,
    Function                                            _function
);


NS_INTERNAL

    // annotated_rebuild
    //   helper: a node of annotation Result at a node's place: its head
    // given, its children each made by _child, its leaf's atom or its
    // application's operator kept.
    template<typename Result,
             typename OpId,
             typename Atom,
             typename Annotation,
             typename ChildFn>
    annotated_expression<OpId, Atom, Result> annotated_rebuild(
        const annotated_expression<OpId, Atom, Annotation>& _node,
        const Result&                                       _head,
        ChildFn&                                            _child
    )
    {
        using result_type = annotated_expression<OpId, Atom, Result>;
        using layer       = typename result_type::layer_type;

        if (_node.unwrap().is_leaf())
        {
            return result_type::make(_head,
                                     layer::leaf(_node.unwrap().atom()));
        }

        std::vector<std::shared_ptr<result_type> > _children;

        _children.reserve(_node.unwrap().children().size());

        for (std::size_t _i = 0; _i < _node.unwrap().children().size(); ++_i)
        {
            _children.push_back(std::make_shared<result_type>(
                _child(*_node.unwrap().children()[_i])));
        }

        return result_type::make(_head,
                                 layer::apply(_node.unwrap().op(),
                                              _children));
    }

    // annotated_map_step, annotated_extend_step
    //   helpers: a child of cofree_map's and of cofree_extend's, recursing.
    template<typename Function>
    struct annotated_map_step
    {
        Function function;

        template<typename Node>
        auto operator()(
            const Node& _child
        ) -> decltype(::djinterp::cofree_map(_child, function))
        {
            return ::djinterp::cofree_map(_child, function);
        }
    };

    template<typename Function>
    struct annotated_extend_step
    {
        Function function;

        template<typename Node>
        auto operator()(
            const Node& _child
        ) -> decltype(::djinterp::cofree_extend(_child, function))
        {
            return ::djinterp::cofree_extend(_child, function);
        }
    };

NS_END  // internal


// cofree_map (annotated_expression)
//   function: (A -> B) applied to the annotation of every node, the term
// kept: the result is an annotated_expression<OpId, Atom, B>. Chosen over
// the cofree's own for an annotated expression, whose children are this
// type and not the cofree's.
template<typename OpId,
         typename Atom,
         typename Annotation,
         typename Function>
D_NODISCARD
annotated_expression<OpId, Atom,
                     typename std::decay<decltype(std::declval<Function&>()(
                         std::declval<const Annotation&>()))>::type>
cofree_map(
    const annotated_expression<OpId, Atom, Annotation>& _node,
    Function                                            _function
)
{
    using result_t = typename std::decay<decltype(
        std::declval<Function&>()(std::declval<const Annotation&>()))>::type;

    internal::annotated_map_step<Function> _step = { _function };

    return internal::annotated_rebuild<result_t>(_node, _function(
                                                     _node.head()),
                                                 _step);
}

// cofree_extend (annotated_expression)
//   function: the comonadic extend - each node's annotation becomes
// f(the whole node), the term kept: the result is an
// annotated_expression<OpId, Atom, B>, and f is given each node as an
// annotated_expression<OpId, Atom, Annotation>.
template<typename OpId,
         typename Atom,
         typename Annotation,
         typename Function>
D_NODISCARD
annotated_expression<OpId, Atom,
                     typename std::decay<decltype(std::declval<Function&>()(
                         std::declval<const annotated_expression<
                             OpId, Atom, Annotation>&>()))>::type>
cofree_extend(
    const annotated_expression<OpId, Atom, Annotation>& _node,
    Function                                            _function
)
{
    using result_t = typename std::decay<decltype(
        std::declval<Function&>()(std::declval<const annotated_expression<
            OpId, Atom, Annotation>&>()))>::type;

    internal::annotated_extend_step<Function> _step = { _function };

    return internal::annotated_rebuild<result_t>(_node, _function(_node),
                                                 _step);
}

// annotated_cata (annotated_expression)
//   function: the cofree's annotated fold, by its binary algebra
// phi : (Annotation, expr_layer<OpId, Atom, Result>) -> Result, named for
// the annotated expression so that a call on one deduces it as itself.
template<typename Result,
         typename OpId,
         typename Atom,
         typename Annotation,
         typename BinaryAlgebra>
D_NODISCARD
Result annotated_cata(
    BinaryAlgebra                                       _algebra,
    const annotated_expression<OpId, Atom, Annotation>& _value
)
{
    using base_type =
        typename annotated_expression<OpId, Atom, Annotation>::base_type;

    return ::djinterp::annotated_cata<Result>(
        _algebra, static_cast<const base_type&>(_value));
}


//   The annotated expression is a Functor, a Comonad and a foldable carrier
// as the cofree is, each registered for it, so functor_map, extract, extend,
// duplicate and cata take it directly and give it back.

// functor_traits<annotated_expression<OpId, Atom, Annotation>>
template<typename OpId,
         typename Atom,
         typename Annotation>
struct functor_traits<annotated_expression<OpId, Atom, Annotation>, void>
{
    using is_specialized = std::true_type;
    using value_type     = Annotation;

    template<typename To>
    using rebind = annotated_expression<OpId, Atom, To>;

    template<typename Node,
             typename Function>
    static
    auto map(
        Node&&   _node,
        Function _function
    ) -> decltype(::djinterp::cofree_map(
                      static_cast<const annotated_expression<
                          OpId, Atom, Annotation>&>(_node), _function))
    {
        return ::djinterp::cofree_map(
            static_cast<const annotated_expression<OpId, Atom, Annotation>&>(
                _node),
            _function);
    }
};

// comonad_traits<annotated_expression<OpId, Atom, Annotation>>
template<typename OpId,
         typename Atom,
         typename Annotation>
struct comonad_traits<annotated_expression<OpId, Atom, Annotation>, void>
{
    using is_specialized = std::true_type;
    using value_type     = Annotation;

    static
    Annotation extract(
        const annotated_expression<OpId, Atom, Annotation>& _node
    )
    {
        return _node.head();
    }

    template<typename Node,
             typename Function>
    static
    auto extend(
        Node&&   _node,
        Function _function
    ) -> decltype(::djinterp::cofree_extend(
                      static_cast<const annotated_expression<
                          OpId, Atom, Annotation>&>(_node), _function))
    {
        return ::djinterp::cofree_extend(
            static_cast<const annotated_expression<OpId, Atom, Annotation>&>(
                _node),
            _function);
    }
};

// recursive_traits<annotated_expression<OpId, Atom, Annotation>>
//   its base functor is the cofree's, DEnvLayer: the annotation paired with
// a layer whose holes are the children, read as annotated expressions.
template<typename OpId,
         typename Atom,
         typename Annotation>
struct recursive_traits<annotated_expression<OpId, Atom, Annotation>, void>
{
    using is_specialized = std::true_type;

    template<typename X>
    using base = DEnvLayer<Annotation,
                           internal::annotated_carrier<OpId, Atom>::template
                               layer,
                           X>;

    static
    base<annotated_expression<OpId, Atom, Annotation> > project(
        const annotated_expression<OpId, Atom, Annotation>& _node
    )
    {
        using node_type = annotated_expression<OpId, Atom, Annotation>;

        return base<node_type>{
            _node.head(),
            ::djinterp::functor_map(
                _node.unwrap(),
                [](const std::shared_ptr<node_type>& _child) -> node_type
                {
                    return (*_child);
                }) };
    }
};


///////////////////////////////////////////////////////////////////////////////
///             V.    STRUCTURAL DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

// is_expr_layer
//   trait: whether a type is a signature layer expr_layer<...>.
template<typename Type>
struct is_expr_layer : std::false_type
{};

template<typename OpId,
         typename Atom,
         typename Child>
struct is_expr_layer<expr_layer<OpId, Atom, Child> > : std::true_type
{};


NS_INTERNAL

    // is_expression_helper
    //   helper: an expression is a mu whose unrolled layer is an expr_layer.
    // mu<F> exposes layer_type = F<mu<F>>, so testing that alias against
    // is_expr_layer recognizes the term without naming OpId / Atom.
    template<typename Type,
             typename = void>
    struct is_expression_helper : std::false_type
    {};

    template<typename Type>
    struct is_expression_helper<Type, void_t<typename Type::layer_type> >
        : is_expr_layer<typename Type::layer_type>
    {};

NS_END  // internal


// is_expression
//   trait: whether a type is an expression term (a mu over a signature
// layer), after cv-ref stripping.
template<typename Type>
struct is_expression
    : internal::is_expression_helper<typename std::decay<Type>::type>
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_expr_layer_v
//   constant: shorthand for is_expr_layer<Type>::value.
template<typename Type>
static D_CONSTEXPR bool is_expr_layer_v = is_expr_layer<Type>::value;

// is_expression_v
//   constant: shorthand for is_expression<Type>::value.
template<typename Type>
static D_CONSTEXPR bool is_expression_v = is_expression<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if ( defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                             \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS == 1) )

// ExprLayer
//   concept: satisfied by a signature layer.
template<typename Type>
concept ExprLayer = is_expr_layer<Type>::value;

// Expression
//   concept: satisfied by an expression term.
template<typename Type>
concept Expression = is_expression<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSE_EXPRESSION_HPP
