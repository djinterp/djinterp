/*******************************************************************************
* djinterp [cpp][parse]                               expr_face_tests_bridge.cpp
*
* The face suite's "bridge" block: bridge.hpp, from C++11, v1's floor, on a
* stand-in instantiation of v1's term, expression<re_std::uint32_t,
* std::string>: an application's operator its v2 id, a leaf's atom its text.
* Each term is also folded by the face into an s-expression, and v1's term,
* printed the same way, must read the same. Below C++11 each test is
* skipped, and says why.
*
*
* path:      /tests/djinterp/cpp/modules/parse/expr/expr_face_tests_bridge.cpp
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.23
*******************************************************************************/

#include "./expr_face_tests.hpp"  // corresponding header


// D_TESTS_FACE_BRIDGE
//   macro: 1 where the bridge's tests run: from C++11, with the parts the
// suite stands on built.
#if ( (D_ENV_LANG_IS_CPP11_OR_HIGHER) && (D_TESTS_FACE_FULL == 1) )
    #define D_TESTS_FACE_BRIDGE 1
#else
    #define D_TESTS_FACE_BRIDGE 0
#endif

// D_TESTS_FACE_BELOW
//   constant: the reason a bridge test gives where it does not run.
#define D_TESTS_FACE_BELOW \
    "the bridge is from C++11, v1's floor, and needs the full core"


#if (D_TESTS_FACE_BRIDGE == 1)

using djinterp::parse::expr::DTerm;
using djinterp::parse::expr::DNodeView;
using djinterp::parse::expr::DSignature;
using djinterp::parse::expr::DResult;

// DTestsFaceV1
//   type: the stand-in instantiation of v1's term.
using DTestsFaceV1 = djinterp::expression<re_std::uint32_t, std::string>;


//==============================================================================
// HELPERS
//==============================================================================


/*
d_tests_face_v1_text
  v1's term as an s-expression, the operator as two digits first, as the
face's DTestsFaceSexpr writes v2's. A test's own recursion, over a small
term.
*/
static std::string
d_tests_face_v1_text(
    const DTestsFaceV1& _expression
)
{
    if (djinterp::is_leaf<re_std::uint32_t, std::string>(_expression))
    {
        return djinterp::atom_of<re_std::uint32_t, std::string>(_expression);
    }

    const re_std::uint32_t op =
        djinterp::operator_of<re_std::uint32_t, std::string>(_expression);
    std::string            out("(");

    out += (char)('0' + (op / 10u));
    out += (char)('0' + (op % 10u));

    for (const DTestsFaceV1& child :
         djinterp::children_of<re_std::uint32_t, std::string>(_expression))
    {
        out += ' ';
        out += d_tests_face_v1_text(child);
    }

    return out + ")";
}

/*
DTestsFaceSexpr
  The face's algebra for the same s-expression; a hole is "?".
*/
class DTestsFaceSexpr
{
public:
    explicit DTestsFaceSexpr(
        const char* _text
    )
        : m_text(_text)
    {}

    std::string
    operator()(
        DNodeView          _node,
        const std::string* _children,
        std::size_t        _count
    )
    {
        if (_node.is_hole())
        {
            return std::string("?");
        }

        if (!_node.is_apply())
        {
            return std::string(m_text + _node.offset(), _node.length());
        }

        std::string out("(");

        out += (char)('0' + (_node.op() / 10u));
        out += (char)('0' + (_node.op() % 10u));

        for (std::size_t i = 0u; i < _count; ++i)
        {
            out += ' ';
            out += _children[i];
        }

        return out + ")";
    }

private:
    const char* m_text;
};

/*
d_tests_face_v1_of
  v1's term for a parse of a text, through the bridge; the bridge's answer.
*/
static int
d_tests_face_v1_of(
    const DResult&   _result,
    const char*      _text,
    DTestsFaceV1&    _out,
    re_std::uint32_t _flags = 0u
)
{
    return djinterp::parse::expr::to_expression<re_std::uint32_t,
                                                 std::string>(
        _result.term(),
        _result.root(),
        [](DNodeView _node)
        {
            return _node.op();
        },
        [_text](DNodeView _node)
        {
            return (_node.is_hole())
                   ? std::string("?")
                   : std::string(_text + _node.offset(), _node.length());
        },
        _out,
        _flags);
}

// DTestsFaceAnnotated
//   type: an annotated term of the stand-in's operators and atoms, each node
// carrying an int.
using DTestsFaceAnnotated =
    djinterp::annotated_expression<re_std::uint32_t, std::string, int>;

/*
d_tests_face_annotated_leaf, d_tests_face_annotated_apply
  A leaf of an atom, and an application of children, each annotated.
*/
DTestsFaceAnnotated
d_tests_face_annotated_leaf(
    const std::string& _atom,
    int                _annotation
)
{
    return DTestsFaceAnnotated::make(
        _annotation, DTestsFaceAnnotated::layer_type::leaf(_atom));
}

DTestsFaceAnnotated
d_tests_face_annotated_apply(
    re_std::uint32_t                          _op,
    const std::vector<DTestsFaceAnnotated>&   _children,
    int                                       _annotation
)
{
    std::vector<std::shared_ptr<DTestsFaceAnnotated> > boxed;

    for (std::size_t i = 0u; i < _children.size(); ++i)
    {
        boxed.push_back(std::make_shared<DTestsFaceAnnotated>(_children[i]));
    }

    return DTestsFaceAnnotated::make(
        _annotation, DTestsFaceAnnotated::layer_type::apply(_op, boxed));
}

/*
d_tests_face_annotation_sum
  A generic function over any annotated term, called with no template
arguments: the sum of its annotations, read through its children as the
node type.
*/
template<typename OpId,
         typename Atom,
         typename Annotation>
Annotation
d_tests_face_annotation_sum(
    const djinterp::annotated_expression<OpId, Atom, Annotation>& _node
)
{
    Annotation sum = _node.head();

    if (_node.unwrap().is_leaf())
    {
        return sum;
    }

    const std::vector<std::shared_ptr<
        djinterp::annotated_expression<OpId, Atom, Annotation> > >& children =
        _node.unwrap().children();

    for (std::size_t i = 0u; i < children.size(); ++i)
    {
        sum += d_tests_face_annotation_sum(*children[i]);
    }

    return sum;
}

/*
d_tests_face_annotation_size
  An extend's function: the nodes of a whole subterm.
*/
int
d_tests_face_annotation_size(
    const DTestsFaceAnnotated& _node
)
{
    int size = 1;

    if (!_node.unwrap().is_leaf())
    {
        for (std::size_t i = 0u; i < _node.unwrap().children().size(); ++i)
        {
            size += d_tests_face_annotation_size(
                        *_node.unwrap().children()[i]);
        }
    }

    return size;
}

#endif  // D_TESTS_FACE_BRIDGE


//==============================================================================
// TESTS
//==============================================================================


/*
d_tests_face_bridge_to
  Each well-formed text, parsed and bridged: v1's term reads as the face's
fold of the v2 term does, operator for operator and leaf for leaf.
*/
bool
d_tests_face_bridge_to(void)
{
#if (D_TESTS_FACE_BRIDGE == 1)
    DSignature  s;
    std::size_t bridged = 0u;

    D_TEST_REQUIRE(d_tests_face_language(s));

    for (std::size_t i = 0u; i < d_tests_face_text_count; ++i)
    {
        const char* const text = d_tests_face_texts[i];
        const DResult     r    = d_tests_face_scan(s, text);

        if (!r.ok())
        {
            continue;
        }

        DTestsFaceV1    v1;
        DTestsFaceSexpr sexpr(text);
        std::string     v2;

        D_TEST_REQUIRE( (d_tests_face_v1_of(r, text, v1) == 0) &&
                        (djinterp::parse::expr::fold(r.term(), r.root(),
                                                     sexpr, v2) == 0) );

        if (d_tests_face_v1_text(v1) != v2)
        {
            (void)d_test_suite_note_current("v1's term differs on \"%s\"",
                                            text);
        }

        D_TEST_REQUIRE(d_tests_face_v1_text(v1) == v2);
        ++bridged;
    }

    D_TEST_REQUIRE(bridged >= 12u);

    // one by its shape: 1 + 2 * 3
    const DResult sum = d_tests_face_scan(s, "1 + 2 * 3");
    DTestsFaceV1  v1;

    D_TEST_REQUIRE(d_tests_face_v1_of(sum, "1 + 2 * 3", v1) == 0);
    D_TEST_REQUIRE(d_tests_face_v1_text(v1) == "(02 1 (04 2 3))");

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_BELOW);
#endif  // D_TESTS_FACE_BRIDGE
}

/*
d_tests_face_bridge_shares
  `a < b <= c` is `(a < b) and (b <= c)` with one `b`: v1's term holds one
node for it, under both relations.
*/
bool
d_tests_face_bridge_shares(void)
{
#if (D_TESTS_FACE_BRIDGE == 1)
    static const char TEXT[] = "a < b <= c";
    DSignature        s;

    D_TEST_REQUIRE(d_tests_face_language(s));

    const DResult r = d_tests_face_scan(s, TEXT);
    DTestsFaceV1  v1;

    D_TEST_REQUIRE( (r.ok()) && (d_tests_face_v1_of(r, TEXT, v1) == 0) );
    D_TEST_REQUIRE(d_tests_face_v1_text(v1) == "(16 (14 a b) (15 b c))");

    const std::vector<DTestsFaceV1>& both =
        djinterp::children_of<re_std::uint32_t, std::string>(v1);
    const std::vector<DTestsFaceV1>& lt   =
        djinterp::children_of<re_std::uint32_t, std::string>(both[0]);
    const std::vector<DTestsFaceV1>& le   =
        djinterp::children_of<re_std::uint32_t, std::string>(both[1]);

    D_TEST_REQUIRE(&lt[1].out() == &le[0].out());
    D_TEST_REQUIRE(&lt[0].out() != &le[1].out());

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_BELOW);
#endif  // D_TESTS_FACE_BRIDGE
}

/*
d_tests_face_bridge_refusals
  A term with a hole is refused, `_out` untouched, unless holes are asked
for, when the atom function is given the hole; a root that is no node is
refused.
*/
bool
d_tests_face_bridge_refusals(void)
{
#if (D_TESTS_FACE_BRIDGE == 1)
    DResult          holed;
    re_std::uint32_t pair[2];
    const char       text[] = "x + ?";
    DTestsFaceV1     v1     =
        djinterp::expr_leaf<re_std::uint32_t, std::string>("kept");

    pair[0] = holed.term().leaf(D_TESTS_FACE_C_NAME, 0u, 0u, 1u);
    pair[1] = holed.term().hole(4u);
    holed.term().set_root(holed.term().apply(D_TESTS_FACE_O_ADD, pair, 2u,
                                             0u, 5u));
    holed.record().root = holed.term().root();

    D_TEST_REQUIRE( (d_tests_face_v1_of(holed, text, v1) ==
                     D_PARSE_EXPR_FOLD_HAS_HOLES) &&
                    (djinterp::is_leaf<re_std::uint32_t, std::string>(v1)) &&
                    (djinterp::atom_of<re_std::uint32_t, std::string>(v1) ==
                     "kept") );
    D_TEST_REQUIRE( (d_tests_face_v1_of(holed, text, v1,
                                        D_PARSE_EXPR_FOLD_HOLES) == 0) &&
                    (d_tests_face_v1_text(v1) == "(02 x ?)") );

    // a root that is no node
    holed.record().root = 99u;
    D_TEST_REQUIRE( (d_tests_face_v1_of(holed, text, v1,
                                        D_PARSE_EXPR_FOLD_HOLES) ==
                     D_PARSE_EXPR_FOLD_BAD_ARGUMENT) &&
                    (d_tests_face_v1_text(v1) == "(02 x ?)") );

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_BELOW);
#endif  // D_TESTS_FACE_BRIDGE
}

/*
d_tests_face_bridge_from
  A v1 table moved into a v2 signature: its prefix, infix and postfix rows
with their precedence, associativity, spelling and flags, a word flagged a
word, the nullary and matchfix rows left and counted. With the classes
added and sealed, the signature parses by the table: precedence, a right
power, `mod` as a word; and the parse bridges back.
*/
bool
d_tests_face_bridge_from(void)
{
#if (D_TESTS_FACE_BRIDGE == 1)
    using djinterp::operator_descriptor;

    djinterp::operator_signature<int> table;

    table.define(1, operator_descriptor(2, 50, djinterp::DAssocLeft,
                                        djinterp::DFixInfix, "+", true, true))
         .define(2, operator_descriptor(2, 50, djinterp::DAssocLeft,
                                        djinterp::DFixInfix, "-"))
         .define(3, operator_descriptor(2, 60, djinterp::DAssocLeft,
                                        djinterp::DFixInfix, "*", true,
                                        true))
         .define(4, operator_descriptor(2, 60, djinterp::DAssocLeft,
                                        djinterp::DFixInfix, "mod"))
         .define(5, operator_descriptor(2, 80, djinterp::DAssocRight,
                                        djinterp::DFixInfix, "^"))
         .define(6, operator_descriptor(1, 70, djinterp::DAssocRight,
                                        djinterp::DFixPrefix, "-"))
         .define(7, operator_descriptor(1, 90, djinterp::DAssocLeft,
                                        djinterp::DFixPostfix, "!"))
         .define(8, operator_descriptor(0, 0, djinterp::DAssocNone,
                                        djinterp::DFixNullary, "pi"))
         .define(9, operator_descriptor(1, 0, djinterp::DAssocNone,
                                        djinterp::DFixMatchfix, "|"));

    DSignature        s;
    const std::size_t left = djinterp::parse::expr::from_operator_signature(
        table,
        [](int _id)
        {
            return (re_std::uint32_t)(_id + 30);
        },
        s);

    D_TEST_REQUIRE( (left == 2u) && (s.good()) && (!s.sealed()) );

    // the classes, which are v2's alone (`mod` a class of its own), and the
    // seal
    const re_std::uint32_t MOD_CLASS = 22u;

    s.atom(D_TESTS_FACE_C_NUM)
     .atom(D_TESTS_FACE_C_NAME)
     .token(D_TESTS_FACE_C_PLUS, D_PARSE_EXPR_OPERATOR, 31u)
     .token(D_TESTS_FACE_C_MINUS, D_PARSE_EXPR_OPERATOR, 32u)
     .token(D_TESTS_FACE_C_STAR, D_PARSE_EXPR_OPERATOR, 33u)
     .token(MOD_CLASS, D_PARSE_EXPR_OPERATOR, 34u)
     .token(D_TESTS_FACE_C_CARET, D_PARSE_EXPR_OPERATOR, 35u)
     .token(D_TESTS_FACE_C_MINUS, D_PARSE_EXPR_OPERAND, 36u)
     .token(D_TESTS_FACE_C_BANG, D_PARSE_EXPR_OPERATOR, 37u);
    D_TEST_REQUIRE(s.seal());

    // the rows, found once sealed; the two left out are not there
    const ::dj_parse_expr_op* add  = s.find(31u);
    const ::dj_parse_expr_op* mod  = s.find(34u);
    const ::dj_parse_expr_op* pow  = s.find(35u);
    const ::dj_parse_expr_op* neg  = s.find(36u);
    const ::dj_parse_expr_op* bang = s.find(37u);

    D_TEST_REQUIRE( (add) && (add->form == D_PARSE_EXPR_FORM_INFIX) &&
                    (add->precedence == 50) &&
                    (add->assoc == D_PARSE_EXPR_ASSOC_LEFT) &&
                    (add->flags == (D_PARSE_EXPR_OP_COMMUTATIVE |
                                    D_PARSE_EXPR_OP_ASSOCIATIVE)) &&
                    (std::strcmp(add->spelling, "+") == 0) );
    D_TEST_REQUIRE( (mod) && (mod->flags == D_PARSE_EXPR_OP_WORD) );
    D_TEST_REQUIRE( (pow) && (pow->assoc == D_PARSE_EXPR_ASSOC_RIGHT) );
    D_TEST_REQUIRE( (neg) && (neg->form == D_PARSE_EXPR_FORM_PREFIX) &&
                    (neg->precedence == 70) );
    D_TEST_REQUIRE( (bang) && (bang->form == D_PARSE_EXPR_FORM_POSTFIX) );
    D_TEST_REQUIRE( (!s.find(38u)) && (!s.find(39u)) );

    static const char TEXT[] = "-1 + 2 * 3 ^ 2 ^ 1 mod a!";
    const DResult     r      = d_tests_face_scan(s, TEXT);
    DTestsFaceV1      v1;

    D_TEST_REQUIRE( (r.ok()) && (d_tests_face_v1_of(r, TEXT, v1) == 0) );
    D_TEST_REQUIRE(d_tests_face_v1_text(v1) ==
                   "(31 (36 1) (34 (33 2 (35 3 (35 2 1))) (37 a)))");

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_BELOW);
#endif  // D_TESTS_FACE_BRIDGE
}


/*
d_tests_face_bridge_annotated
  v1's annotated term, a class derived from the cofree, so a generic function
deduces its parameters.
  Tests the following:
  - a node's children are shared_ptr to the node type, so a const reference
    binds to them, and make gives the node type
  - a generic function over any annotated term, called with no template
    arguments, deduces it, at every level from C++11
  - the map, extend, extract, functor_map, duplicate and annotated_cata keep
    the type, and their results read as before
  - it converts to the cofree it derives from and back, and copies and moves
    as a value
*/
bool
d_tests_face_bridge_annotated(void)
{
#if (D_TESTS_FACE_BRIDGE == 1)
    typedef DTestsFaceAnnotated::base_type DTestsFaceCofree;

    // the children are the node's type
    D_TEST_REQUIRE( (std::is_same<
                        DTestsFaceAnnotated::layer_type,
                        djinterp::expr_layer<
                            re_std::uint32_t, std::string,
                            std::shared_ptr<DTestsFaceAnnotated> > >::value) );
    D_TEST_REQUIRE( (std::is_base_of<DTestsFaceCofree,
                                     DTestsFaceAnnotated>::value) );

    // 1 + (2 * x), annotated 10, 1, 20, 2, 3
    std::vector<DTestsFaceAnnotated> inner;

    inner.push_back(d_tests_face_annotated_leaf("2", 2));
    inner.push_back(d_tests_face_annotated_leaf("x", 3));

    std::vector<DTestsFaceAnnotated> outer;

    outer.push_back(d_tests_face_annotated_leaf("1", 1));
    outer.push_back(d_tests_face_annotated_apply(33u, inner, 20));

    const DTestsFaceAnnotated term =
        d_tests_face_annotated_apply(31u, outer, 10);

    // deduced, with no template arguments
    D_TEST_REQUIRE(d_tests_face_annotation_sum(term) == 36);

    // the map keeps the term and the type
    const djinterp::annotated_expression<re_std::uint32_t, std::string,
                                         std::string> named =
        djinterp::cofree_map(term,
                             [](int _n) -> std::string
                             {
                                 return std::string(1, (char)('a' + _n % 26));
                             });

    D_TEST_REQUIRE( (named.head() == "k") &&
                    (named.unwrap().op() == 31u) &&
                    (named.unwrap().children()[1]->unwrap().children()[1]
                         ->head() == "d") &&
                    (named.unwrap().children()[1]->unwrap().children()[1]
                         ->unwrap().atom() == "x") );

    // extend gives each node its subterm's size, as an annotated term
    const DTestsFaceAnnotated sizes =
        djinterp::extend(term, d_tests_face_annotation_size);

    D_TEST_REQUIRE( (djinterp::extract(sizes) == 5) &&
                    (sizes.unwrap().children()[1]->head() == 3) &&
                    (d_tests_face_annotation_sum(sizes) == 11) );

    // functor_map and duplicate, through the traits
    const DTestsFaceAnnotated doubled =
        djinterp::functor_map(term, [](int _n) { return _n * 2; });
    const djinterp::annotated_expression<re_std::uint32_t, std::string,
                                         DTestsFaceAnnotated> copies =
        djinterp::duplicate(term);

    D_TEST_REQUIRE( (d_tests_face_annotation_sum(doubled) == 72) &&
                    (d_tests_face_annotation_sum(copies.head()) == 36) &&
                    (copies.unwrap().children()[0]->head().head() == 1) );

    // the annotated fold: annotations and leaves counted, children folded
    const int folded = djinterp::annotated_cata<int>(
        [](int _annotation,
           const djinterp::expr_layer<re_std::uint32_t, std::string, int>&
               _layer) -> int
        {
            int sum = _annotation;

            if (!_layer.is_leaf())
            {
                for (std::size_t i = 0u; i < _layer.children().size(); ++i)
                {
                    sum += _layer.children()[i];
                }
            }

            return sum;
        },
        term);

    D_TEST_REQUIRE(folded == 36);

    // to the cofree and back; a copy and a move are values
    const DTestsFaceCofree&   plain = term;
    const DTestsFaceAnnotated again(plain);
    DTestsFaceAnnotated       moved = static_cast<DTestsFaceAnnotated&&>(
                                          DTestsFaceAnnotated(again));

    D_TEST_REQUIRE( (d_tests_face_annotation_sum(again) == 36) &&
                    (d_tests_face_annotation_sum(moved) == 36) &&
                    (moved.unwrap().children()[0]->unwrap().atom() == "1") );

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_BELOW);
#endif  // D_TESTS_FACE_BRIDGE
}
