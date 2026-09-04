/******************************************************************************
* djinterp [expression]                                            expression.hpp
*
*   The common core of every expression language in the framework -- math,
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
* refolds it, and cofree<expr_layer, Ann> gives the annotated variant -- one
* functor serving the bare term and its decorated form alike.  Evaluate,
* render, type-check, simplify, and cost are each just an algebra
* expr_layer<OpId, Atom, R> -> R handed to cata; the structural recursion is
* written once, in recursion.hpp, never here.
*
*   WHY mu AND NOT free.  free<F, A> would model the same object (its Pure A
* is this functor's leaf variant), and its free_bind is a ready substitution
* -- but mu is what recursion.hpp is built for ("an expression IR" is its
* named client), it hands back ana and hylo that free does not, and its one
* leaf-inclusive functor is reused verbatim by cofree for annotation instead
* of needing a second, leaf-free functor.  Substitution and rewriting are
* provided as cata algebras (expression_ops.hpp) rather than lost.
*
*   WHAT LIVES WHERE.  This header is the noun: the operator model, the
* signature functor and its Functor instance, the term and its constructors,
* the annotated alias, and detection.  The verbs -- fold / evaluate / query /
* rewrite / substitute -- live in expression_ops.hpp; a compile-time typed
* mirror in expression_static.hpp; precedence-aware printing in
* expression_render.hpp; the precedence-climbing parser in
* expression_parse.hpp.  cata (recursion.hpp) already folds the term with no
* further wiring.
*
*   REQUIREMENTS.  _Atom and _OpId must be default-constructible, copyable,
* and (for signature lookup) _OpId equality-comparable -- the same shape mu
* and cofree already ask of a layer.  The dynamic term is heap-backed
* (shared_ptr, via mu) and not constexpr; the compile-time face is
* expression_static.hpp.
*
*
* TABLE OF CONTENTS
* =================
* I.    OPERATOR MODEL
*       1.  associativity / fixity
*       2.  operator_descriptor
*       3.  operator_signature<OpId, Value, Container>
* II.   SIGNATURE FUNCTOR                    (expr_layer<OpId, Atom, Child>)
* III.  FUNCTOR REGISTRATION                 (functor_traits<expr_layer>)
* IV.   THE EXPRESSION TERM
*       1.  expression<OpId, Atom>           (= mu<expr_layer>)
*       2.  constructors                      (expr_leaf / expr_apply)
*       3.  observers                         (is_leaf / atom_of / ...)
*       4.  annotated_expression<OpId, Atom, Ann>   (= cofree<expr_layer, Ann>)
* V.    STRUCTURAL DETECTION                  (is_expr_layer / is_expression)
*
*
* path:      /inc/djinterp/parse/expression.hpp
* link(s):   ch-recursion.tex, ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#ifndef DJINTERP_EXPRESSION_EXPRESSION_
#define DJINTERP_EXPRESSION_EXPRESSION_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "../meta/kv_pair.hpp"
#include "../util/lookup/lookup_sentinels.hpp"
#include "../functional/functor.hpp"
#include "../functional/recursion.hpp"
#include "../functional/cofree.hpp"


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
typedef std::int32_t associativity;

// DAssoc*
//   constants: the associativity classes.
constexpr associativity DAssocNone  = 0;   // non-associative (a op b op c ill-formed)
constexpr associativity DAssocLeft  = 1;   // (a op b) op c
constexpr associativity DAssocRight = 2;   // a op (b op c)

// fixity
//   typedef: where an operator sits relative to its operands.
typedef std::int32_t fixity;

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
// language -- arity and the precedence / associativity / fixity a renderer
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
    D_CONSTEXPR
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
    D_CONSTEXPR
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
//   class: the described operator set of one expression language -- simply
// a sequence of key-value pairs, each an operator id mapped to its metadata.
// Container-agnostic (any sequence of kv_pair rows: vector, deque, array,
// list) and value-agnostic (the value defaults to operator_descriptor but
// may be any per-operator payload a language wants).  A renderer consults it
// for spelling and parenthesization, a parser for precedence and fixity.
// Lookup is a first-match-wins walk -- the intuition of the lookup family --
// and a miss yields a null value or the lookup_npos index.
template<typename _OpId,
         typename _Value     = operator_descriptor,
         typename _Container = std::vector<kv_pair<_OpId, _Value> > >
class operator_signature
{
public:
    using op_id_type     = _OpId;
    using value_type     = _Value;
    using entry_type     = kv_pair<_OpId, _Value>;
    using container_type = _Container;

    operator_signature()
        : m_entries()
    {}

    // operator_signature (from a prebuilt sequence)
    //   lets a language supply any container of key-value pairs -- a
    // constexpr std::array of them, say -- instead of building one entry at
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
        const _OpId&  _op,
        const _Value& _value
    )
    {
        m_entries.push_back(entry_type(_op, _value));

        return *this;
    }

    // describe
    //   method: the value bound to an operator id, or null on a miss.
    D_NODISCARD
    const _Value*
    describe
    (
        const _OpId& _op
    ) const
    {
        for (typename container_type::const_iterator _it = m_entries.begin();
             _it != m_entries.end();
             ++_it)
        {
            if (_it->m_key == _op)
            {
                return &_it->m_value;
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
        const _OpId& _op
    ) const
    {
        std::size_t _i = 0;

        for (typename container_type::const_iterator _it = m_entries.begin();
             _it != m_entries.end();
             ++_it, ++_i)
        {
            if (_it->m_key == _op)
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
        const _OpId& _op
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
//   class: one unrolled layer of an expression -- the signature functor
// whose fixed point is the term.  A layer is either a leaf carrying an atom,
// or an application of an operator to a sequence of children of type _Child
// (the recursive positions).  As a functor it is single-argument in _Child
// (_OpId and _Atom are fixed), and mapping a layer maps only the children,
// leaving the operator and any atom in place -- the shape recursion.hpp and
// cofree.hpp fold and annotate over.
template<typename _OpId,
         typename _Atom,
         typename _Child>
class expr_layer
{
public:
    using op_id_type    = _OpId;
    using atom_type     = _Atom;
    using child_type    = _Child;
    using children_type = std::vector<_Child>;

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
    static expr_layer
    leaf
    (
        const _Atom& _atom
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
    static expr_layer
    apply
    (
        const _OpId&         _op,
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
    D_NODISCARD const _Atom& atom()     const { return m_atom;     }
    D_NODISCARD const _OpId& op()       const { return m_op;       }

    D_NODISCARD
    const children_type& children() const { return m_children; }

    D_NODISCARD
    std::size_t arity() const { return m_children.size(); }

private:
    bool          m_is_leaf;
    _Atom         m_atom;
    _OpId         m_op;
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

// functor_traits<expr_layer<_OpId, _Atom, _Child>>
template<typename _OpId,
         typename _Atom,
         typename _Child>
struct functor_traits<expr_layer<_OpId, _Atom, _Child>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _Child;

    template<typename _To>
    using rebind = expr_layer<_OpId, _Atom, _To>;

    // map
    //   applies _function to each child, yielding a layer over the mapped
    // child type; a leaf passes through unchanged (bar the child-type
    // rebind), an application keeps its operator and maps its children.
    template<typename _Layer,
             typename _Function>
    static
    expr_layer<_OpId, _Atom,
        typename std::decay<decltype(std::declval<_Function&>()(
            std::declval<const _Child&>()))>::type>
    map
    (
        _Layer&&  _layer,
        _Function _function
    )
    {
        using to_type = typename std::decay<decltype(
            std::declval<_Function&>()(std::declval<const _Child&>()))>::type;
        using result_layer = expr_layer<_OpId, _Atom, to_type>;

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
    //   helper: binds _OpId and _Atom so expr_layer presents as the
    // single-argument template-template parameter mu (and cofree) require --
    // the same nested-alias device parser_layer uses for free.
    template<typename _OpId,
             typename _Atom>
    struct expr_carrier
    {
        template<typename _Child>
        using layer = expr_layer<_OpId, _Atom, _Child>;
    };

NS_END  // internal


// expression
//   alias: an expression over operator ids _OpId and atoms _Atom -- the
// least fixed point of expr_layer.  cata folds it, ana builds it, hylo
// refolds it (all from recursion.hpp), with no further registration.
template<typename _OpId,
         typename _Atom>
using expression = mu<internal::expr_carrier<_OpId, _Atom>::template layer>;


// =================================================================
//  2. constructors
// =================================================================

// expr_leaf
//   function: a leaf expression carrying an atom.
template<typename _OpId,
         typename _Atom>
D_NODISCARD
expression<_OpId, _Atom>
expr_leaf
(
    const _Atom& _atom
)
{
    return expression<_OpId, _Atom>::In(
        expression<_OpId, _Atom>::layer_type::leaf(_atom));
}

// expr_apply
//   function: an operator applied to a vector of child expressions.
template<typename _OpId,
         typename _Atom>
D_NODISCARD
expression<_OpId, _Atom>
expr_apply
(
    const _OpId&                                  _op,
    const std::vector<expression<_OpId, _Atom> >& _children
)
{
    return expression<_OpId, _Atom>::In(
        expression<_OpId, _Atom>::layer_type::apply(_op, _children));
}

// expr_apply (nullary)
//   function: an operator with no operands -- a constant symbol.
template<typename _OpId,
         typename _Atom>
D_NODISCARD
expression<_OpId, _Atom>
expr_apply
(
    const _OpId& _op
)
{
    return expr_apply<_OpId, _Atom>(
        _op, std::vector<expression<_OpId, _Atom> >());
}

// expr_apply (unary)
//   function: an operator applied to one child.
template<typename _OpId,
         typename _Atom>
D_NODISCARD
expression<_OpId, _Atom>
expr_apply
(
    const _OpId&                     _op,
    const expression<_OpId, _Atom>&  _child
)
{
    std::vector<expression<_OpId, _Atom> > _children;
    _children.push_back(_child);

    return expr_apply<_OpId, _Atom>(_op, _children);
}

// expr_apply (binary)
//   function: an operator applied to two children.
template<typename _OpId,
         typename _Atom>
D_NODISCARD
expression<_OpId, _Atom>
expr_apply
(
    const _OpId&                     _op,
    const expression<_OpId, _Atom>&  _left,
    const expression<_OpId, _Atom>&  _right
)
{
    std::vector<expression<_OpId, _Atom> > _children;
    _children.reserve(2);
    _children.push_back(_left);
    _children.push_back(_right);

    return expr_apply<_OpId, _Atom>(_op, _children);
}


// =================================================================
//  3. observers
// =================================================================

// is_leaf
//   function: whether the expression's root node is a leaf.
template<typename _OpId,
         typename _Atom>
D_NODISCARD
bool
is_leaf
(
    const expression<_OpId, _Atom>& _expression
)
{
    return _expression.out().is_leaf();
}

// atom_of
//   function: the atom at a leaf root.  Precondition: is_leaf.
template<typename _OpId,
         typename _Atom>
D_NODISCARD
const _Atom&
atom_of
(
    const expression<_OpId, _Atom>& _expression
)
{
    return _expression.out().atom();
}

// operator_of
//   function: the operator id at an application root.  Precondition:
// !is_leaf.
template<typename _OpId,
         typename _Atom>
D_NODISCARD
const _OpId&
operator_of
(
    const expression<_OpId, _Atom>& _expression
)
{
    return _expression.out().op();
}

// children_of
//   function: the child expressions at an application root.
template<typename _OpId,
         typename _Atom>
D_NODISCARD
const std::vector<expression<_OpId, _Atom> >&
children_of
(
    const expression<_OpId, _Atom>& _expression
)
{
    return _expression.out().children();
}


// =================================================================
//  4. annotated_expression<OpId, Atom, Ann>
// =================================================================

// annotated_expression
//   alias: an expression whose every node also carries an annotation of
// type _Annotation -- the cofree comonad over the very same signature
// functor.  Attribute grammars, inferred-type decoration, source spans, and
// cached values are all annotations; extract reads a node's, extend
// re-decorates from whole sub-trees (comonad.hpp), and unfold_cofree builds
// one (cofree.hpp).
template<typename _OpId,
         typename _Atom,
         typename _Annotation>
using annotated_expression =
    cofree<internal::expr_carrier<_OpId, _Atom>::template layer, _Annotation>;


///////////////////////////////////////////////////////////////////////////////
///             V.    STRUCTURAL DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

// is_expr_layer
//   trait: whether a type is a signature layer expr_layer<...>.
template<typename _Type>
struct is_expr_layer : std::false_type
{};

template<typename _OpId,
         typename _Atom,
         typename _Child>
struct is_expr_layer<expr_layer<_OpId, _Atom, _Child> > : std::true_type
{};


NS_INTERNAL

    // is_expression_helper
    //   helper: an expression is a mu whose unrolled layer is an expr_layer.
    // mu<F> exposes layer_type = F<mu<F>>, so testing that alias against
    // is_expr_layer recognizes the term without naming _OpId / _Atom.
    template<typename _Type,
             typename = void>
    struct is_expression_helper : std::false_type
    {};

    template<typename _Type>
    struct is_expression_helper<_Type, void_t<typename _Type::layer_type> >
        : is_expr_layer<typename _Type::layer_type>
    {};

NS_END  // internal


// is_expression
//   trait: whether a type is an expression term (a mu over a signature
// layer), after cv-ref stripping.
template<typename _Type>
struct is_expression
    : internal::is_expression_helper<typename std::decay<_Type>::type>
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_expr_layer_v
//   constant: shorthand for is_expr_layer<_Type>::value.
template<typename _Type>
static D_CONSTEXPR bool is_expr_layer_v = is_expr_layer<_Type>::value;

// is_expression_v
//   constant: shorthand for is_expression<_Type>::value.
template<typename _Type>
static D_CONSTEXPR bool is_expression_v = is_expression<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if ( defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                             \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS == 1) )

// ExprLayer
//   concept: satisfied by a signature layer.
template<typename _Type>
concept ExprLayer = is_expr_layer<_Type>::value;

// Expression
//   concept: satisfied by an expression term.
template<typename _Type>
concept Expression = is_expression<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_EXPRESSION_EXPRESSION_
