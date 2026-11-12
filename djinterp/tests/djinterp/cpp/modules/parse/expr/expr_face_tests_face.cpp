/*******************************************************************************
* djinterp [cpp/parse]                                  expr_face_tests_face.cpp
*
* The face suite's "face" block: the face's own rules, each where the C API
* has none of its own to hold it to: storage, copies and moves, the
* builder's refusals, C++ sources, the result's copies, C++ writers, and
* folds and walks that run C++ callbacks, which may throw.
*
*
* path:      /tests/djinterp/cpp/modules/parse/expr/expr_face_tests_face.cpp
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#include "./expr_face_tests.hpp"  // corresponding header


#if (D_TESTS_FACE_FULL == 1)

using djinterp::parse::expr::DTerm;
using djinterp::parse::expr::DNodeView;
using djinterp::parse::expr::DSignature;
using djinterp::parse::expr::DResult;
using djinterp::parse::expr::DOptions;
using djinterp::parse::expr::DScanner;
using djinterp::parse::expr::DRenderOptions;


//==============================================================================
// HELPERS
//==============================================================================


/*
DTestsFaceTokens
  A C++98 source over a fixed run of tokens: `a + b`, then the end.
*/
class DTestsFaceTokens
{
public:
    DTestsFaceTokens()
        : m_at(0u),
          m_throw(false)
    {}

    explicit DTestsFaceTokens(
        bool _throw
    )
        : m_at(0u),
          m_throw(_throw)
    {}

    int
    next(
        re_std::uint8_t        _position,
        re_std::uint32_t       _mode,
        ::dj_parse_expr_token& _out
    )
    {
        static const re_std::uint32_t CLASSES[3] =
        {
            D_TESTS_FACE_C_NAME, D_TESTS_FACE_C_PLUS, D_TESTS_FACE_C_NAME
        };

        (void)_position;
        (void)_mode;
        std::memset(&_out, 0, sizeof(_out));

        // a source that throws has failed
        if ( (m_throw) && (m_at == 1u) )
        {
            throw std::runtime_error("the source broke");
        }

        if (m_at >= 3u)
        {
            _out.cls    = D_PARSE_EXPR_CLASS_END;
            _out.offset = 5u;

            return 0;
        }

        _out.cls    = CLASSES[m_at];
        _out.offset = m_at * 2u;
        _out.length = 1u;
        ++m_at;

        return 1;
    }

private:
    re_std::uint32_t m_at;
    bool             m_throw;
};

/*
DTestsFaceNext
  The same tokens as a callable, for make_source().
*/
class DTestsFaceNext
{
public:
    DTestsFaceNext()
        : m_tokens()
    {}

    int
    operator()(
        re_std::uint8_t        _position,
        re_std::uint32_t       _mode,
        ::dj_parse_expr_token& _out
    )
    {
        return m_tokens.next(_position, _mode, _out);
    }

private:
    DTestsFaceTokens m_tokens;
};

/*
DTestsFaceAtoms
  An atom writer: each leaf as its text between brackets, or a refusal, or a
throw, at the leaf the test names.
*/
class DTestsFaceAtoms
{
public:
    DTestsFaceAtoms(
        const char*      _text,
        re_std::uint32_t _fail_at,
        bool             _throw
    )
        : m_text(_text),
          m_fail_at(_fail_at),
          m_throw(_throw)
    {}

    bool
    operator()(
        DNodeView    _leaf,
        std::string& _out
    )
    {
        if (_leaf.index() == m_fail_at)
        {
            if (m_throw)
            {
                throw std::runtime_error("the atom writer broke");
            }

            return false;
        }

        _out += '[';
        _out.append(m_text + _leaf.offset(), _leaf.length());
        _out += ']';

        return true;
    }

private:
    const char*      m_text;
    re_std::uint32_t m_fail_at;
    bool             m_throw;
};

/*
DTestsFaceWriter
  A writer that keeps what it is given, refusing or throwing past a limit.
*/
class DTestsFaceWriter
{
public:
    DTestsFaceWriter(
        std::size_t _limit,
        bool        _throw
    )
        : m_text(),
          m_limit(_limit),
          m_throw(_throw)
    {}

    bool
    operator()(
        const char* _bytes,
        std::size_t _length
    )
    {
        if (m_text.size() + _length > m_limit)
        {
            if (m_throw)
            {
                throw std::runtime_error("the writer broke");
            }

            return false;
        }

        m_text.append(_bytes, _length);

        return true;
    }

    const std::string&
    text() const
    {
        return m_text;
    }

private:
    std::string m_text;
    std::size_t m_limit;
    bool        m_throw;
};

/*
DTestsFaceEval
  An algebra: a single-digit number's value, a sum, a product, a negation;
it throws at a division, and counts the nodes it is given.
*/
class DTestsFaceEval
{
public:
    explicit DTestsFaceEval(
        const char* _text
    )
        : m_text(_text),
          m_calls(0u)
    {}

    double
    operator()(
        DNodeView     _node,
        const double* _children,
        std::size_t   _count
    )
    {
        ++m_calls;

        if (_node.is_leaf())
        {
            return (double)(m_text[_node.offset()] - '0');
        }

        if (_node.is_hole())
        {
            return -1.0;
        }

        switch (_node.op())
        {
            case D_TESTS_FACE_O_ADD:
                return _children[0] + _children[1];
            case D_TESTS_FACE_O_MUL:
                return _children[0] * _children[1];
            case D_TESTS_FACE_O_NEG:
                return -_children[0];
            case D_TESTS_FACE_O_DIV:
                throw std::runtime_error("no division here");
            default:
                break;
        }

        return (_count > 0u) ? _children[0] : 0.0;
    }

    re_std::uint32_t
    calls() const
    {
        return m_calls;
    }

private:
    const char*      m_text;
    re_std::uint32_t m_calls;
};

/*
DTestsFaceShow
  An algebra: a subtree as an s-expression, the operator's number first; and
how many times it was given each node.
*/
class DTestsFaceShow
{
public:
    DTestsFaceShow(
        const char*      _text,
        re_std::uint32_t _nodes
    )
        : m_text(_text),
          m_seen(_nodes, 0u)
    {}

    std::string
    operator()(
        DNodeView          _node,
        const std::string* _children,
        std::size_t        _count
    )
    {
        std::string out;

        ++m_seen[_node.index()];

        if (!_node.is_apply())
        {
            return std::string(m_text + _node.offset(), _node.length());
        }

        out += '(';
        out += (char)('0' + (_node.op() / 10u));
        out += (char)('0' + (_node.op() % 10u));

        for (std::size_t i = 0u; i < _count; ++i)
        {
            out += ' ';
            out += _children[i];
        }

        return out + ")";
    }

    const std::vector<re_std::uint32_t>&
    seen() const
    {
        return m_seen;
    }

private:
    const char*                   m_text;
    std::vector<re_std::uint32_t> m_seen;
};

/*
DTestsFaceVisits
  A visitor: each visit as (`e` or `l`, node, parent); it skips an operator's
children, stops at the n-th entry, or throws there, as asked.
*/
class DTestsFaceVisits
{
public:
    DTestsFaceVisits(
        re_std::uint32_t _skip_op,
        re_std::uint32_t _stop_at,
        bool             _throw
    )
        : m_visits(),
          m_skip_op(_skip_op),
          m_stop_at(_stop_at),
          m_entered(0u),
          m_throw(_throw)
    {}

    int
    enter(
        DNodeView        _node,
        re_std::uint32_t _parent
    )
    {
        ++m_entered;
        record('e', _node.index(), _parent);

        if (m_entered == m_stop_at)
        {
            if (m_throw)
            {
                throw std::runtime_error("the visitor broke");
            }

            return D_PARSE_EXPR_WALK_STOP;
        }

        return ( (_node.is_apply()) && (_node.op() == m_skip_op) )
               ? D_PARSE_EXPR_WALK_SKIP
               : D_PARSE_EXPR_WALK_CONTINUE;
    }

    int
    leave(
        DNodeView        _node,
        re_std::uint32_t _parent
    )
    {
        record('l', _node.index(), _parent);

        return D_PARSE_EXPR_WALK_CONTINUE;
    }

    const std::vector<re_std::uint32_t>&
    visits() const
    {
        return m_visits;
    }

private:
    void
    record(
        char             _kind,
        re_std::uint32_t _node,
        re_std::uint32_t _parent
    )
    {
        m_visits.push_back((re_std::uint32_t)_kind);
        m_visits.push_back(_node);
        m_visits.push_back(_parent);
    }

    std::vector<re_std::uint32_t> m_visits;
    re_std::uint32_t              m_skip_op;
    re_std::uint32_t              m_stop_at;
    re_std::uint32_t              m_entered;
    bool                          m_throw;
};

/*
d_tests_face_reference_walk
  The visits a plain walk makes, by recursion over the views: the reference
the face's iterative walk is held to.
*/
static void
d_tests_face_reference_walk(
    DNodeView                      _node,
    re_std::uint32_t               _parent,
    std::vector<re_std::uint32_t>& _visits
)
{
    _visits.push_back((re_std::uint32_t)'e');
    _visits.push_back(_node.index());
    _visits.push_back(_parent);

    for (re_std::uint32_t i = 0u; i < _node.size(); ++i)
    {
        d_tests_face_reference_walk(_node.child(i), _node.index(), _visits);
    }

    _visits.push_back((re_std::uint32_t)'l');
    _visits.push_back(_node.index());
    _visits.push_back(_parent);

    return;
}

#endif  // D_TESTS_FACE_FULL


//==============================================================================
// TESTS
//==============================================================================


/*
d_tests_face_term_storage
  A term on the heap grows as it is built; one in the caller's storage holds
what fits and refuses the rest; a hole marks its term; a cleared term is
empty and keeps its storage.
*/
bool
d_tests_face_term_storage(void)
{
#if (D_TESTS_FACE_FULL == 1)
    DTerm                 heap;
    ::dj_parse_expr_node  nodes[3];
    re_std::uint32_t      args[2];
    DTerm                 mine(nodes, 3u, args, 2u);
    DTerm                 holed;
    re_std::uint32_t      pair[2];

    // on the heap
    pair[0] = heap.leaf(D_TESTS_FACE_C_NUM, 7u, 0u, 1u);
    pair[1] = heap.leaf(D_TESTS_FACE_C_NAME, 0u, 4u, 1u);
    heap.set_root(heap.apply(D_TESTS_FACE_O_ADD, pair, 2u, 0u, 5u));

    D_TEST_REQUIRE( (heap.good()) && (heap.size() == 3u) &&
                    (heap.root() == 2u) && (heap.verify()) );
    D_TEST_REQUIRE( (heap.root_node().is_apply()) &&
                    (heap.root_node().op() == D_TESTS_FACE_O_ADD) &&
                    (heap.root_node().size() == 2u) &&
                    (heap.root_node().length() == 5u) );
    D_TEST_REQUIRE( (heap.root_node().child(0u).is_leaf()) &&
                    (heap.root_node().child(0u).value() == 7u) &&
                    (heap.root_node().child(1u).offset() == 4u) &&
                    (!heap.root_node().child(2u).valid()) );
    D_TEST_REQUIRE( (!heap.has_holes()) && (!DNodeView().valid()) );

    // in the caller's storage, until it is full
    pair[0] = mine.leaf(D_TESTS_FACE_C_NAME, 0u);
    pair[1] = mine.leaf(D_TESTS_FACE_C_NAME, 1u);
    mine.set_root(mine.apply(D_TESTS_FACE_O_MUL, pair, 2u));

    D_TEST_REQUIRE( (mine.size() == 3u) && (mine.get()->nodes == nodes) &&
                    (mine.verify()) );
    D_TEST_REQUIRE(mine.leaf(D_TESTS_FACE_C_NAME, 2u) ==
                   D_PARSE_EXPR_NO_NODE);
    D_TEST_REQUIRE(mine.size() == 3u);

    // a hole
    (void)holed.hole(3u);
    D_TEST_REQUIRE( (holed.has_holes()) && (holed.node(0u).is_hole()) &&
                    (holed.node(0u).offset() == 3u) );

    // cleared
    heap.clear();
    D_TEST_REQUIRE( (heap.empty()) &&
                    (heap.root() == D_PARSE_EXPR_NO_NODE) &&
                    (heap.leaf(D_TESTS_FACE_C_NUM, 1u) == 0u) );

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}

/*
d_tests_face_term_copies
  A copy is deep and equal; an assignment replaces; a self-assignment keeps;
a copy that cannot fit says so through good(); a subtree is appended by
copy(); from C++11, a move leaves its source empty.
*/
bool
d_tests_face_term_copies(void)
{
#if (D_TESTS_FACE_FULL == 1)
    DTerm                a;
    re_std::uint32_t     pair[2];
    re_std::uint32_t     sum;
    ::dj_parse_expr_node small_nodes[2];
    re_std::uint32_t     small_args[1];
    DTerm                small(small_nodes, 2u, small_args, 1u);

    // (x + y) * 2
    pair[0] = a.leaf(D_TESTS_FACE_C_NAME, 0u, 1u, 1u);
    pair[1] = a.leaf(D_TESTS_FACE_C_NAME, 1u, 5u, 1u);
    sum     = a.apply(D_TESTS_FACE_O_ADD, pair, 2u, 0u, 7u);
    pair[0] = sum;
    pair[1] = a.leaf(D_TESTS_FACE_C_NUM, 2u, 10u, 1u);
    a.set_root(a.apply(D_TESTS_FACE_O_MUL, pair, 2u, 0u, 11u));

    DTerm b(a);

    D_TEST_REQUIRE( (b.good()) && (b == a) && (b.size() == a.size()) &&
                    (b.get()->nodes != a.get()->nodes) &&
                    (b.digest() == a.digest()) &&
                    (d_tests_face_same_nodes(a.get(), b.get())) );

    DTerm c;

    (void)c.leaf(D_TESTS_FACE_C_NUM, 9u);
    c.set_root(0u);
    D_TEST_REQUIRE(c != a);
    c = a;
    D_TEST_REQUIRE( (c.good()) && (c == a) &&
                    (d_tests_face_same_nodes(a.get(), c.get())) );

    DTerm& same = c;

    c = same;
    D_TEST_REQUIRE( (c.good()) && (c == a) );

    // too small for the copy
    small = a;
    D_TEST_REQUIRE(!small.good());

    // a subtree appended
    DTerm            d;
    re_std::uint32_t copied;

    (void)d.leaf(D_TESTS_FACE_C_NUM, 1u);
    copied = d.copy(a, sum);
    D_TEST_REQUIRE( (copied == 3u) && (d.size() == 4u) &&
                    (d.equal(copied, a, sum, true)) &&
                    (d.digest(copied) == a.digest(sum)) );

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    DTerm moved(std::move(b));

    D_TEST_REQUIRE( (moved == a) && (b.empty()) &&
                    (b.root() == D_PARSE_EXPR_NO_NODE) );

    DTerm again;

    again = std::move(moved);
    D_TEST_REQUIRE( (again == a) && (moved.empty()) );
#endif  // C++11

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}

/*
d_tests_face_signature_building
  The calculator by the face: sealed and found (find() answers once
sealed); a row the C API refuses leaves good() false; a sealed signature
takes no row; from C++11, a move keeps the rows.
*/
bool
d_tests_face_signature_building(void)
{
#if (D_TESTS_FACE_FULL == 1)
    DSignature s;

    D_TEST_REQUIRE( (!s.sealed()) && (s.good()) );
    D_TEST_REQUIRE(d_tests_face_language(s));
    D_TEST_REQUIRE( (s.good()) && (s.sealed()) );

    const ::dj_parse_expr_op* add = s.find(D_TESTS_FACE_O_ADD);
    const ::dj_parse_expr_op* le  = s.find(D_TESTS_FACE_O_LE);
    const ::dj_parse_expr_op* idx = s.find(D_TESTS_FACE_O_INDEX);

    D_TEST_REQUIRE( (add) && (add->form == D_PARSE_EXPR_FORM_INFIX) &&
                    (add->precedence == 50) &&
                    ((add->flags & D_PARSE_EXPR_OP_COMMUTATIVE) != 0u) &&
                    (std::strcmp(add->spelling, "+") == 0) );
    D_TEST_REQUIRE( (le) && (le->assoc == D_PARSE_EXPR_ASSOC_CHAIN) &&
                    (le->join == D_TESTS_FACE_O_AND) );
    D_TEST_REQUIRE( (idx) && (idx->mixfix.lead == 1u) &&
                    (idx->mixfix.min_items == 1u) &&
                    (idx->mixfix.max_items == 2u) );
    D_TEST_REQUIRE(!s.find(999u));

    // sealed: no row more
    s.atom(99u);
    D_TEST_REQUIRE(!s.good());

    // a row the C API refuses: an operator without an id
    DSignature bad;

    bad.atom(D_TESTS_FACE_C_NAME)
       .infix(D_PARSE_EXPR_OP_NONE, 50, D_PARSE_EXPR_ASSOC_LEFT, "+");
    D_TEST_REQUIRE(!bad.good());
    D_TEST_REQUIRE(!bad.seal());

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    DSignature t;

    D_TEST_REQUIRE(d_tests_face_language(t));

    DSignature moved(std::move(t));

    D_TEST_REQUIRE( (moved.sealed()) && (moved.find(D_TESTS_FACE_O_POW)) );
#endif  // C++11

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}

/*
d_tests_face_sources
  `a + b` from a C++98 source class, a callable through make_source(), an
array and a vector, and from C++11 a lambda: the same term each time. A
source that throws has failed: the parse is fatal, and nothing escapes it.
*/
bool
d_tests_face_sources(void)
{
#if (D_TESTS_FACE_FULL == 1)
    DSignature s;

    D_TEST_REQUIRE(d_tests_face_language(s));

    DTestsFaceTokens tokens;
    DResult          first = djinterp::parse::expr::parse(s, tokens);

    D_TEST_REQUIRE( (first.ok()) && (first.errors() == 0u) &&
                    (first.term().root_node().op() == D_TESTS_FACE_O_ADD) &&
                    (first.next().cls == D_PARSE_EXPR_CLASS_END) );

    // a callable
    djinterp::parse::expr::DFunctionSource<DTestsFaceNext> callable =
        djinterp::parse::expr::make_source(DTestsFaceNext());
    DResult second = djinterp::parse::expr::parse(s, callable);

    D_TEST_REQUIRE( (second.ok()) && (second.term() == first.term()) );

    // an array and a vector
    ::dj_parse_expr_token run[3];
    DTestsFaceTokens      again;

    for (re_std::uint32_t i = 0u; i < 3u; ++i)
    {
        (void)again.next(D_PARSE_EXPR_OPERAND, 0u, run[i]);
    }

    DResult                              third  =
        djinterp::parse::expr::parse(s, run, 3u, 5u);
    std::vector< ::dj_parse_expr_token > vec(run, run + 3);
    DResult                              fourth =
        djinterp::parse::expr::parse(s, vec, 5u);

    D_TEST_REQUIRE( (third.ok()) && (third.term() == first.term()) );
    D_TEST_REQUIRE( (fourth.ok()) && (fourth.term() == first.term()) );

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    DTestsFaceTokens inner;
    auto             lambda = djinterp::parse::expr::make_source(
        [&inner](re_std::uint8_t        _position,
                 re_std::uint32_t       _mode,
                 ::dj_parse_expr_token& _out)
        {
            return inner.next(_position, _mode, _out);
        });
    DResult fifth = djinterp::parse::expr::parse(s, lambda);

    D_TEST_REQUIRE( (fifth.ok()) && (fifth.term() == first.term()) );
#endif  // C++11

    // a throw is a failure, caught at the face
    DTestsFaceTokens breaks(true);
    bool             escaped = false;
    DResult          broken;

    try
    {
        broken = djinterp::parse::expr::parse(s, breaks);
    }
    catch (...)
    {
        escaped = true;
    }

    D_TEST_REQUIRE( (!escaped) && (broken.status() == D_PARSE_EXPR_FATAL) );

    // the default scanner
    DResult scanned = d_tests_face_scan(s, "a+b");

    D_TEST_REQUIRE( (scanned.ok()) && (scanned.term() == first.term()) );

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}

/*
d_tests_face_diagnostics
  Without a sink, the diagnostics are copied into the result with their
messages, and a copied result copies them; with the caller's sink they stay
there, and the result holds none.
*/
bool
d_tests_face_diagnostics(void)
{
#if (D_TESTS_FACE_FULL == 1)
    DSignature s;

    D_TEST_REQUIRE(d_tests_face_language(s));

    DResult r = d_tests_face_scan(s, "1 +");

    D_TEST_REQUIRE( (!r.ok()) && (r.status() == D_PARSE_EXPR_ERROR) &&
                    (r.errors() == 1u) );
    D_TEST_REQUIRE( (r.code() == D_PARSE_EXPR_DIAG_MISSING_OPERAND) &&
                    (r.domain() != 0u) );
    D_TEST_REQUIRE( (!r.diagnostics().empty()) &&
                    (r.diag_index() < r.diagnostics().size()) );

    const djinterp::parse::expr::DDiagnostic& d =
        r.diagnostics()[r.diag_index()];

    D_TEST_REQUIRE( (d.severity() == D_PARSE_SEVERITY_ERROR) &&
                    (!d.continuation()) && (d.domain() == r.domain()) &&
                    (d.code() == r.code()) && (!d.message().empty()) &&
                    (d_tests_face_same_span(d.span(), r.span())) );

    DResult copy = r;

    D_TEST_REQUIRE( (copy.diagnostics().size() == r.diagnostics().size()) &&
                    (copy.diagnostics()[0].message() ==
                     r.diagnostics()[0].message()) &&
                    (copy.term() == r.term()) );

    // the caller's sink
    ::dj_parse_diag_sink              sink;
    ::dj_parse_diagnostic             items[8];
    char                              text[512];
    const ::dj_parse_expr_scan_config config = d_tests_face_config();
    DScanner                          scanner;

    d_parse_diag_sink_init(&sink, items, 8u, text, 512u);
    D_TEST_REQUIRE(scanner.bind(s, config, "1 +"));

    DResult kept = djinterp::parse::expr::parse(s, scanner.source(), NULL,
                                                &sink);

    D_TEST_REQUIRE( (!kept.ok()) && (kept.diagnostics().empty()) &&
                    (sink.count == r.diagnostics().size()) &&
                    (kept.diag_index() == r.diag_index()) );
    D_TEST_REQUIRE(std::strcmp(d_parse_diag_message(&sink,
                                                    &sink.items[0]),
                               r.diagnostics()[0].message().c_str()) == 0);

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}

/*
d_tests_face_render
  Into a string, with the spans' text; through a writer; with a C++ atom
writer; and failures: a writer that refuses or throws, an atom writer that
refuses or throws, and a term whose operator the signature lacks.
*/
bool
d_tests_face_render(void)
{
#if (D_TESTS_FACE_FULL == 1)
    static const char TEXT[] = "(a+b)*c";
    DSignature        s;
    DRenderOptions    options;

    D_TEST_REQUIRE(d_tests_face_language(s));

    DResult r = d_tests_face_scan(s, TEXT);

    D_TEST_REQUIRE(r.ok());

    options.text        = TEXT;
    options.text_length = sizeof(TEXT) - 1u;

    std::string plain;

    D_TEST_REQUIRE( (djinterp::parse::expr::render(s, r.term(), r.root(),
                                                   plain, &options)) &&
                    (plain == "(a + b) * c") );
    D_TEST_REQUIRE(djinterp::parse::expr::render(s, r.term(), &options) ==
                   plain);

    // through a writer, and with an atom writer
    DTestsFaceWriter writer(1024u, false);
    DTestsFaceAtoms  atoms(TEXT, D_PARSE_EXPR_NO_NODE, false);
    std::string      bracketed;

    D_TEST_REQUIRE( (djinterp::parse::expr::render_to(s, r.term(), r.root(),
                                                      writer, &options)) &&
                    (writer.text() == plain) );
    D_TEST_REQUIRE( (djinterp::parse::expr::render(s, r.term(), r.root(),
                                                   atoms, bracketed)) &&
                    (bracketed == "([a] + [b]) * [c]") );

    // failures, none escaping
    DTestsFaceWriter short_writer(3u, false);
    DTestsFaceWriter throwing(3u, true);
    DTestsFaceAtoms  refusing(TEXT, 0u, false);
    DTestsFaceAtoms  breaking(TEXT, 0u, true);
    std::string      lost;
    bool             escaped = false;

    try
    {
        D_TEST_REQUIRE(!djinterp::parse::expr::render_to(s, r.term(),
                                                         r.root(),
                                                         short_writer,
                                                         &options));
        D_TEST_REQUIRE(!djinterp::parse::expr::render_to(s, r.term(),
                                                         r.root(), throwing,
                                                         &options));
        D_TEST_REQUIRE(!djinterp::parse::expr::render(s, r.term(), r.root(),
                                                      refusing, lost));
        D_TEST_REQUIRE(!djinterp::parse::expr::render(s, r.term(), r.root(),
                                                      breaking, lost));
    }
    catch (...)
    {
        escaped = true;
    }

    D_TEST_REQUIRE(!escaped);

    // an operator the signature lacks: no text
    DTerm            stray;
    re_std::uint32_t one = stray.leaf(D_TESTS_FACE_C_NUM, 1u);

    stray.set_root(stray.apply(77u, &one, 1u));
    D_TEST_REQUIRE(djinterp::parse::expr::render(s, stray).empty());

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}

/*
d_tests_face_fold
  Values of two types, a double and a std::string; a node two parents share
folded once, and the nodes the root does not reach not at all; holes
refused unless asked for; a root that is no node; and an algebra's throw
unwinding through the fold to its caller, `_out` untouched.
*/
bool
d_tests_face_fold(void)
{
#if (D_TESTS_FACE_FULL == 1)
    DSignature s;

    D_TEST_REQUIRE(d_tests_face_language(s));

    // a number
    static const char SUM[] = "1 + 2 * -3";
    DResult           sum   = d_tests_face_scan(s, SUM);
    DTestsFaceEval    eval(SUM);
    double            value = 0.0;

    D_TEST_REQUIRE( (sum.ok()) &&
                    (djinterp::parse::expr::fold(sum.term(), sum.root(),
                                                 eval, value) == 0) &&
                    (value == -5.0) && (eval.calls() == sum.term().size()) );

    // a chain's shared operand, once
    static const char CHAIN[] = "a < b <= c";
    DResult           chain   = d_tests_face_scan(s, CHAIN);
    DTestsFaceShow    show(CHAIN, chain.term().size());
    std::string       text;

    D_TEST_REQUIRE( (chain.ok()) &&
                    (djinterp::parse::expr::fold(chain.term(), chain.root(),
                                                 show, text) == 0) );
    D_TEST_REQUIRE(text == "(16 (14 a b) (15 b c))");

    for (std::size_t i = 0u; i < show.seen().size(); ++i)
    {
        D_TEST_REQUIRE(show.seen()[i] == 1u);
    }

    // a subtree: the nodes above it are not folded
    DTestsFaceShow    part(CHAIN, chain.term().size());
    const DNodeView   lt = chain.term().root_node().child(0u);
    std::string       left;

    D_TEST_REQUIRE( (djinterp::parse::expr::fold(chain.term(), lt.index(),
                                                 part, left) == 0) &&
                    (left == "(14 a b)") &&
                    (part.seen()[chain.root()] == 0u) );

    // holes, and a root that is no node
    DTerm            holed;
    re_std::uint32_t pair[2];
    double           kept = 42.0;

    pair[0] = holed.leaf(D_TESTS_FACE_C_NUM, 0u, 0u, 1u);
    pair[1] = holed.hole(2u);
    holed.set_root(holed.apply(D_TESTS_FACE_O_ADD, pair, 2u));

    DTestsFaceEval holes("4 ?");

    D_TEST_REQUIRE( (djinterp::parse::expr::fold(holed, holed.root(), holes,
                                                 kept) ==
                     D_PARSE_EXPR_FOLD_HAS_HOLES) &&
                    (kept == 42.0) && (holes.calls() == 0u) );
    D_TEST_REQUIRE( (djinterp::parse::expr::fold(holed, holed.root(), holes,
                                                 kept,
                                                 D_PARSE_EXPR_FOLD_HOLES) ==
                     0) &&
                    (kept == 3.0) );
    D_TEST_REQUIRE( (djinterp::parse::expr::fold(holed, 99u, holes, kept,
                                                 D_PARSE_EXPR_FOLD_HOLES) ==
                     D_PARSE_EXPR_FOLD_BAD_ARGUMENT) &&
                    (kept == 3.0) );

    // a throw unwinds to the caller
    static const char QUOTIENT[] = "1 + 4 / 2";
    DResult           quotient   = d_tests_face_scan(s, QUOTIENT);
    DTestsFaceEval    breaks(QUOTIENT);
    double            untouched  = 7.0;
    bool              threw      = false;

    D_TEST_REQUIRE(quotient.ok());

    try
    {
        (void)djinterp::parse::expr::fold(quotient.term(), quotient.root(),
                                          breaks, untouched);
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }

    D_TEST_REQUIRE( (threw) && (untouched == 7.0) );

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}

/*
d_tests_face_walk
  The walk's visits against a recursive reference; a skip enters no child
but still leaves; a stop ends the walk at once; a root that is no node; and
a visitor's throw unwinding to the caller.
*/
bool
d_tests_face_walk(void)
{
#if (D_TESTS_FACE_FULL == 1)
    DSignature s;

    D_TEST_REQUIRE(d_tests_face_language(s));

    DResult r = d_tests_face_scan(s, "f(x, y + 1)[2] * -z");

    D_TEST_REQUIRE(r.ok());

    // plain, against the reference
    DTestsFaceVisits              plain(0u, 0u, false);
    std::vector<re_std::uint32_t> reference;

    d_tests_face_reference_walk(r.term().root_node(), D_PARSE_EXPR_NO_NODE,
                                reference);
    D_TEST_REQUIRE( (djinterp::parse::expr::walk(r.term(), r.root(),
                                                 plain)) &&
                    (plain.visits() == reference) );
    D_TEST_REQUIRE( (reference[1] == r.root()) &&
                    (reference[2] == D_PARSE_EXPR_NO_NODE) );

    // a skip at the sum: entered and left, its children not
    DTestsFaceVisits skipped(D_TESTS_FACE_O_ADD, 0u, false);
    std::size_t      adds = 0u;

    D_TEST_REQUIRE(djinterp::parse::expr::walk(r.term(), r.root(), skipped));
    D_TEST_REQUIRE(skipped.visits().size() == reference.size() - 2u * 6u);

    for (std::size_t i = 0u; i + 2u < skipped.visits().size(); i += 3u)
    {
        const DNodeView node = r.term().node(skipped.visits()[i + 1u]);

        if ( (node.is_apply()) && (node.op() == D_TESTS_FACE_O_ADD) )
        {
            ++adds;
        }
    }

    D_TEST_REQUIRE(adds == 2u);

    // a stop at the third entry
    DTestsFaceVisits stopped(0u, 3u, false);

    D_TEST_REQUIRE( (djinterp::parse::expr::walk(r.term(), r.root(),
                                                 stopped)) &&
                    (stopped.visits().size() == 9u) &&
                    (std::equal(stopped.visits().begin(),
                                stopped.visits().end(),
                                reference.begin())) );

    // a root that is no node
    DTestsFaceVisits none(0u, 0u, false);

    D_TEST_REQUIRE( (!djinterp::parse::expr::walk(r.term(), 999u, none)) &&
                    (none.visits().empty()) );

    // a throw unwinds to the caller
    DTestsFaceVisits breaks(0u, 2u, true);
    bool             threw = false;

    try
    {
        (void)djinterp::parse::expr::walk(r.term(), r.root(), breaks);
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }

    D_TEST_REQUIRE( (threw) && (breaks.visits().size() == 6u) );

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}
