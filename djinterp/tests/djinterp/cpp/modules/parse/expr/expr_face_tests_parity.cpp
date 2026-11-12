/*******************************************************************************
* djinterp [cpp/parse]                                expr_face_tests_parity.cpp
*
* The face suite's "parity" block: the face held to the C API it stands on.
* The calculator is built both ways, and each text parsed both ways: the C
* side with the C scanner, term, sink and parse alone, the face's with
* DScanner, parse() and the copies it makes. Every term must be the same
* arena node for node, every result and diagnostic the same field for
* field, every render the same text, every fold the same value from the
* same nodes, and every walk the same visits.
*   Where the two differ, the test notes the text it was reading.
*
*
* path:      /tests/djinterp/cpp/modules/parse/expr/expr_face_tests_parity.cpp
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
using djinterp::parse::expr::DRenderOptions;


//==============================================================================
// HELPERS
//==============================================================================


/*
D_TESTS_FACE_FNV
  The FNV-1a step the two folds' algebras share.
*/
#define D_TESTS_FACE_FNV(_hash, _word)                                        \
    ( ((_hash) ^ (re_std::uint32_t)(_word)) * 16777619u )

/*
d_tests_face_note
  Notes the text a parity test was reading when it found a difference.
*/
static void
d_tests_face_note(
    const char* _what,
    const char* _text
)
{
    (void)d_test_suite_note_current("%s differ on \"%s\"", _what, _text);

    return;
}

/*
d_tests_face_same_token
  Whether two tokens are the same, field for field.
*/
static bool
d_tests_face_same_token(
    const ::dj_parse_expr_token& _a,
    const ::dj_parse_expr_token& _b
)
{
    return ( (_a.cls == _b.cls)       && (_a.flags == _b.flags)   &&
             (_a.offset == _b.offset) && (_a.length == _b.length) &&
             (_a.value == _b.value) );
}

/*
d_tests_face_same_text
  Whether two spellings are the same text, or both none.
*/
static bool
d_tests_face_same_text(
    const char* _a,
    const char* _b
)
{
    return ( ( (!_a) && (!_b) ) ||
             ( (_a) && (_b) && (std::strcmp(_a, _b) == 0) ) );
}

/*
d_tests_face_same_mixfix
  Whether two mixfix descriptors are the same, spellings by their text.
*/
static bool
d_tests_face_same_mixfix(
    const ::dj_parse_expr_mixfix& _a,
    const ::dj_parse_expr_mixfix& _b
)
{
    bool same = ( (_a.lead == _b.lead)                   &&
                  (_a.parts == _b.parts)                 &&
                  (_a.list_hole == _b.list_hole)         &&
                  (_a.tail == _b.tail)                   &&
                  (_a.separator == _b.separator)         &&
                  (_a.row_separator == _b.row_separator) &&
                  (_a.row_op == _b.row_op)               &&
                  (_a.rows_op == _b.rows_op)             &&
                  (_a.min_items == _b.min_items)         &&
                  (_a.max_items == _b.max_items)         &&
                  (_a.hole_mode == _b.hole_mode)         &&
                  (d_tests_face_same_text(_a.separator_spelling,
                                          _b.separator_spelling)) &&
                  (d_tests_face_same_text(_a.row_separator_spelling,
                                          _b.row_separator_spelling)) );

    for (re_std::uint32_t i = 0u; ( (same) && (i < _a.parts) ); ++i)
    {
        same = ( (_a.part[i] == _b.part[i]) &&
                 (d_tests_face_same_text(_a.part_spelling[i],
                                         _b.part_spelling[i])) );
    }

    return same;
}

/*
DTestsFaceSides
  The calculator built both ways, released at the end of the test.
*/
class DTestsFaceSides
{
public:
    DTestsFaceSides()
        : m_c_built(false),
          m_face_built(false)
    {
        std::memset(&m_c, 0, sizeof(m_c));
        m_c_built    = d_tests_face_c_language(&m_c);
        m_face_built = d_tests_face_language(m_face);
    }

    ~DTestsFaceSides()
    {
        d_parse_expr_signature_release(&m_c);
    }

    bool
    built() const
    {
        return ( (m_c_built) && (m_face_built) );
    }

    const ::dj_parse_expr_signature*
    c() const
    {
        return &m_c;
    }

    const DSignature&
    face() const
    {
        return m_face;
    }

private:
    DTestsFaceSides(const DTestsFaceSides&);
    DTestsFaceSides& operator=(const DTestsFaceSides&);

    ::dj_parse_expr_signature m_c;
    DSignature                m_face;
    bool                      m_c_built;
    bool                      m_face_built;
};

/*
d_tests_face_parity_text
  Parses one text both ways, with the same options, and holds the two to
each other: result, term and diagnostics. Counts the parses that succeeded.
*/
static bool
d_tests_face_parity_text(
    const DTestsFaceSides&         _sides,
    const char*                    _text,
    const ::dj_parse_expr_options* _options,
    std::size_t&                   _ok
)
{
    DTestsFaceCParse c;
    const bool       bound  = d_tests_face_c_parse(_sides.c(), _text,
                                                   _options, c);
    const DResult    f      = d_tests_face_scan(_sides.face(), _text,
                                                _options);
    const ::dj_parse_expr_result& cr = c.m_result;
    const bool       result =
        ( (bound)                                   &&
          (f.status() == cr.status)                 &&
          (f.root() == cr.root)                     &&
          (f.errors() == cr.errors)                 &&
          (f.domain() == cr.domain)                 &&
          (f.code() == cr.code)                     &&
          (f.diag_index() == cr.diag_index)         &&
          (d_tests_face_same_span(f.span(), cr.span)) &&
          (d_tests_face_same_token(f.next(), cr.next)) );
    const bool       term   = ( (bound) &&
                                (d_tests_face_same_nodes(&c.m_term,
                                                         f.term().get())) );
    bool             diags  = ( (bound) &&
                                (f.diagnostics().size() == c.m_sink.count) );

    for (re_std::uint32_t i = 0u; ( (diags) && (i < c.m_sink.count) ); ++i)
    {
        const ::dj_parse_diagnostic&              cd = c.m_sink.items[i];
        const djinterp::parse::expr::DDiagnostic& fd = f.diagnostics()[i];

        diags = ( (fd.severity() == cd.severity) &&
                  (fd.continuation() ==
                   ((cd.flags & D_PARSE_DIAG_FLAG_CONTINUATION) != 0u)) &&
                  (fd.domain() == cd.domain) &&
                  (fd.code() == cd.code) &&
                  (d_tests_face_same_span(fd.span(), cd.span)) &&
                  (fd.message() == d_parse_diag_message(&c.m_sink, &cd)) );
    }

    if (f.ok())
    {
        ++_ok;
    }

    d_tests_face_c_release(c);

    if (!result)
    {
        d_tests_face_note("the results", _text);
    }

    if (!term)
    {
        d_tests_face_note("the terms", _text);
    }

    if (!diags)
    {
        d_tests_face_note("the diagnostics", _text);
    }

    D_TEST_REQUIRE(result);
    D_TEST_REQUIRE(term);
    D_TEST_REQUIRE(diags);

    return true;
}


// the folds' algebras
//------------------------------------------------------------------------------

/*
DTestsFaceCHash
  The C algebra's context: the text its leaves are read from, and how many
nodes it was given.
*/
struct DTestsFaceCHash
{
    const char*      m_text;
    re_std::uint32_t m_calls;
};

/*
d_tests_face_c_hash
  The C algebra: a node's hash from its kind, operator, leaf text and its
children's hashes.
*/
static int
d_tests_face_c_hash(
    void*                                 _ctx,
    const struct dj_parse_expr_fold_view* _view,
    void*                                 _out
)
{
    DTestsFaceCHash* const            ctx  = static_cast<DTestsFaceCHash*>(
                                                 _ctx);
    const struct dj_parse_expr_node&  node = _view->term->nodes[_view->node];
    re_std::uint32_t                  hash = 2166136261u;

    hash = D_TESTS_FACE_FNV(hash, node.kind);
    hash = D_TESTS_FACE_FNV(hash, node.op);

    if (node.kind == D_PARSE_EXPR_NODE_APPLY)
    {
        for (re_std::uint32_t i = 0u; i < node.count; ++i)
        {
            hash = D_TESTS_FACE_FNV(hash,
                       *static_cast<const re_std::uint32_t*>(
                           d_parse_expr_fold_child(_view, i)));
        }
    }
    else
    {
        for (re_std::uint32_t i = 0u; i < node.length; ++i)
        {
            hash = D_TESTS_FACE_FNV(hash,
                       (unsigned char)ctx->m_text[node.offset + i]);
        }
    }

    ++ctx->m_calls;
    *static_cast<re_std::uint32_t*>(_out) = hash;

    return 0;
}

/*
DTestsFaceHash
  The face's algebra, the same hash.
*/
class DTestsFaceHash
{
public:
    explicit DTestsFaceHash(
        const char* _text
    )
        : m_text(_text),
          m_calls(0u)
    {}

    re_std::uint32_t
    operator()(
        DNodeView               _node,
        const re_std::uint32_t* _children,
        std::size_t             _count
    )
    {
        re_std::uint32_t hash = 2166136261u;

        hash = D_TESTS_FACE_FNV(hash, _node.kind());
        hash = D_TESTS_FACE_FNV(hash, _node.record().op);

        if (_node.is_apply())
        {
            for (std::size_t i = 0u; i < _count; ++i)
            {
                hash = D_TESTS_FACE_FNV(hash, _children[i]);
            }
        }
        else
        {
            for (re_std::uint32_t i = 0u; i < _node.length(); ++i)
            {
                hash = D_TESTS_FACE_FNV(hash,
                           (unsigned char)m_text[_node.offset() + i]);
            }
        }

        ++m_calls;

        return hash;
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


// the walks' visitors
//------------------------------------------------------------------------------

/*
DTestsFaceCVisits
  The C visitor's context: the visits, an operator whose children are
skipped, and the entry the walk stops at.
*/
struct DTestsFaceCVisits
{
    std::vector<re_std::uint32_t> m_visits;
    re_std::uint32_t              m_skip_op;
    re_std::uint32_t              m_stop_at;
    re_std::uint32_t              m_entered;
};

/*
d_tests_face_c_enter, d_tests_face_c_leave
  The C visitor, as the face's DTestsFaceSameVisits.
*/
static int
d_tests_face_c_enter(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    re_std::uint32_t                 _node,
    re_std::uint32_t                 _parent
)
{
    DTestsFaceCVisits* const ctx = static_cast<DTestsFaceCVisits*>(_ctx);

    ++ctx->m_entered;
    ctx->m_visits.push_back((re_std::uint32_t)'e');
    ctx->m_visits.push_back(_node);
    ctx->m_visits.push_back(_parent);

    if (ctx->m_entered == ctx->m_stop_at)
    {
        return D_PARSE_EXPR_WALK_STOP;
    }

    return ( (_term->nodes[_node].kind == D_PARSE_EXPR_NODE_APPLY) &&
             (_term->nodes[_node].op == ctx->m_skip_op) )
           ? D_PARSE_EXPR_WALK_SKIP
           : D_PARSE_EXPR_WALK_CONTINUE;
}

static int
d_tests_face_c_leave(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    re_std::uint32_t                 _node,
    re_std::uint32_t                 _parent
)
{
    DTestsFaceCVisits* const ctx = static_cast<DTestsFaceCVisits*>(_ctx);

    (void)_term;
    ctx->m_visits.push_back((re_std::uint32_t)'l');
    ctx->m_visits.push_back(_node);
    ctx->m_visits.push_back(_parent);

    return D_PARSE_EXPR_WALK_CONTINUE;
}

/*
DTestsFaceSameVisits
  The face's visitor, the same visits.
*/
class DTestsFaceSameVisits
{
public:
    DTestsFaceSameVisits(
        re_std::uint32_t _skip_op,
        re_std::uint32_t _stop_at
    )
        : m_visits(),
          m_skip_op(_skip_op),
          m_stop_at(_stop_at),
          m_entered(0u)
    {}

    int
    enter(
        DNodeView        _node,
        re_std::uint32_t _parent
    )
    {
        ++m_entered;
        m_visits.push_back((re_std::uint32_t)'e');
        m_visits.push_back(_node.index());
        m_visits.push_back(_parent);

        if (m_entered == m_stop_at)
        {
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
        m_visits.push_back((re_std::uint32_t)'l');
        m_visits.push_back(_node.index());
        m_visits.push_back(_parent);

        return D_PARSE_EXPR_WALK_CONTINUE;
    }

    const std::vector<re_std::uint32_t>&
    visits() const
    {
        return m_visits;
    }

private:
    std::vector<re_std::uint32_t> m_visits;
    re_std::uint32_t              m_skip_op;
    re_std::uint32_t              m_stop_at;
    re_std::uint32_t              m_entered;
};


// the renders' atom writers
//------------------------------------------------------------------------------

/*
d_tests_face_c_atom
  The C atom writer: a leaf's text between braces, the text the context.
*/
static bool
d_tests_face_c_atom(
    void*                       _ctx,
    const ::dj_parse_expr_term* _term,
    re_std::uint32_t            _node,
    ::dj_parse_expr_write_fn    _write,
    void*                       _write_ctx
)
{
    const char* const           text = static_cast<const char*>(_ctx);
    const ::dj_parse_expr_node& node = _term->nodes[_node];

    return ( (_write(_write_ctx, "{", 1u) == 1u) &&
             ( (node.length == 0u) ||
               (_write(_write_ctx, text + node.offset, node.length) ==
                node.length) ) &&
             (_write(_write_ctx, "}", 1u) == 1u) );
}

/*
DTestsFaceBraces
  The face's atom writer, the same text.
*/
class DTestsFaceBraces
{
public:
    explicit DTestsFaceBraces(
        const char* _text
    )
        : m_text(_text)
    {}

    bool
    operator()(
        DNodeView    _leaf,
        std::string& _out
    )
    {
        _out += '{';
        _out.append(m_text + _leaf.offset(), _leaf.length());
        _out += '}';

        return true;
    }

private:
    const char* m_text;
};

/*
d_tests_face_c_render
  A subtree's text by the C renderer, or "<none>" where it could not be
rendered.
*/
static std::string
d_tests_face_c_render(
    const ::dj_parse_expr_signature*      _signature,
    const ::dj_parse_expr_term*           _term,
    re_std::uint32_t                      _root,
    const ::dj_parse_expr_render_options* _options
)
{
    std::vector<char> buffer(1024u, '\0');
    const std::size_t length = d_parse_expr_render_buffer(_signature, _term,
                                                          _root, _options,
                                                          &buffer[0],
                                                          buffer.size());

    if ( (length == (std::size_t)-1) || (length >= buffer.size()) )
    {
        return std::string("<none>");
    }

    return std::string(&buffer[0], length);
}

#endif  // D_TESTS_FACE_FULL


//==============================================================================
// TESTS
//==============================================================================


/*
d_tests_face_parity_signature
  The calculator by the C API and by the face: the same operators and roles,
in the same order once sealed, field for field, the same juxtaposition and
flags.
*/
bool
d_tests_face_parity_signature(void)
{
#if (D_TESTS_FACE_FULL == 1)
    DTestsFaceSides sides;

    D_TEST_REQUIRE(sides.built());

    const ::dj_parse_expr_signature* c = sides.c();
    const ::dj_parse_expr_signature* f = sides.face().get();

    D_TEST_REQUIRE( (c->op_count == f->op_count) &&
                    (c->role_count == f->role_count) &&
                    (c->op_count == 19u) );
    D_TEST_REQUIRE( (c->juxtapose == f->juxtapose) &&
                    (c->may_juxtapose == f->may_juxtapose) &&
                    (c->juxtapose_ctx == f->juxtapose_ctx) &&
                    (c->flags == f->flags) );

    for (re_std::uint32_t i = 0u; i < c->op_count; ++i)
    {
        const ::dj_parse_expr_op& x = c->ops[i];
        const ::dj_parse_expr_op& y = f->ops[i];

        D_TEST_REQUIRE( (x.id == y.id) && (x.form == y.form) &&
                        (x.assoc == y.assoc) && (x.flags == y.flags) &&
                        (x.precedence == y.precedence) &&
                        (x.join == y.join) &&
                        (d_tests_face_same_text(x.spelling, y.spelling)) &&
                        (d_tests_face_same_mixfix(x.mixfix, y.mixfix)) );
    }

    for (re_std::uint32_t i = 0u; i < c->role_count; ++i)
    {
        const ::dj_parse_expr_role& x = c->roles[i];
        const ::dj_parse_expr_role& y = f->roles[i];

        D_TEST_REQUIRE( (x.cls == y.cls) && (x.position == y.position) &&
                        (x.kind == y.kind) && (x.flags == y.flags) &&
                        (x.op == y.op) && (x.modes == y.modes) &&
                        (x.order == y.order) && (x.custom == y.custom) &&
                        (x.custom_ctx == y.custom_ctx) );
    }

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}

/*
d_tests_face_parity_built
  One term built by the C builders and by the face's, a hole and a shared
node in it: the same arena, the same digest at every node; a subtree copied
and a whole term assigned both ways: the same again.
*/
bool
d_tests_face_parity_built(void)
{
#if (D_TESTS_FACE_FULL == 1)
    ::dj_parse_expr_term c;
    DTerm                f;
    re_std::uint32_t     cn[6];
    re_std::uint32_t     fn[6];
    re_std::uint32_t     pair[2];

    d_parse_expr_term_init_heap(&c);

    // x, y, (x + y), hole, ((x + y) * hole), ((x + y) - ((x + y) * hole))
    cn[0]   = d_parse_expr_term_leaf(&c, D_TESTS_FACE_C_NAME, 3u, 0u, 1u);
    cn[1]   = d_parse_expr_term_leaf(&c, D_TESTS_FACE_C_NAME, 4u, 4u, 1u);
    pair[0] = cn[0];
    pair[1] = cn[1];
    cn[2]   = d_parse_expr_term_apply(&c, D_TESTS_FACE_O_ADD, pair, 2u, 0u,
                                      5u);
    cn[3]   = d_parse_expr_term_hole(&c, 9u);
    pair[0] = cn[2];
    pair[1] = cn[3];
    cn[4]   = d_parse_expr_term_apply(&c, D_TESTS_FACE_O_MUL, pair, 2u, 0u,
                                      10u);
    pair[0] = cn[2];
    pair[1] = cn[4];
    cn[5]   = d_parse_expr_term_apply(&c, D_TESTS_FACE_O_SUB, pair, 2u, 0u,
                                      10u);
    c.root  = cn[5];

    fn[0]   = f.leaf(D_TESTS_FACE_C_NAME, 3u, 0u, 1u);
    fn[1]   = f.leaf(D_TESTS_FACE_C_NAME, 4u, 4u, 1u);
    pair[0] = fn[0];
    pair[1] = fn[1];
    fn[2]   = f.apply(D_TESTS_FACE_O_ADD, pair, 2u, 0u, 5u);
    fn[3]   = f.hole(9u);
    pair[0] = fn[2];
    pair[1] = fn[3];
    fn[4]   = f.apply(D_TESTS_FACE_O_MUL, pair, 2u, 0u, 10u);
    pair[0] = fn[2];
    pair[1] = fn[4];
    fn[5]   = f.apply(D_TESTS_FACE_O_SUB, pair, 2u, 0u, 10u);
    f.set_root(fn[5]);

    bool same = ( (d_tests_face_same_nodes(&c, f.get())) &&
                  (d_parse_expr_term_verify(&c) == f.verify()) &&
                  (f.has_holes()) );

    for (re_std::uint32_t i = 0u; ( (same) && (i < 6u) ); ++i)
    {
        same = ( (cn[i] == fn[i]) &&
                 (d_parse_expr_term_digest(&c, cn[i]) == f.digest(fn[i])) );
    }

    // a subtree copied, a whole term assigned
    ::dj_parse_expr_term c_copy;
    ::dj_parse_expr_term c_whole;
    DTerm                f_copy;

    d_parse_expr_term_init_heap(&c_copy);
    d_parse_expr_term_init_heap(&c_whole);

    const re_std::uint32_t c_at = d_parse_expr_term_copy(&c_copy, &c, cn[4]);
    const re_std::uint32_t f_at = f_copy.copy(f, fn[4]);

    c_copy.root = c_at;
    f_copy.set_root(f_at);

    const DTerm f_whole(f);

    same = ( (same) && (c_at == f_at) &&
             (d_tests_face_same_nodes(&c_copy, f_copy.get())) &&
             (d_parse_expr_term_assign(&c_whole, &c)) &&
             (f_whole.good()) &&
             (d_tests_face_same_nodes(&c_whole, f_whole.get())) );

    d_parse_expr_term_release(&c_whole);
    d_parse_expr_term_release(&c_copy);
    d_parse_expr_term_release(&c);

    D_TEST_REQUIRE(same);

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}

/*
d_tests_face_parity_parse
  Every text both ways, with the settings in force: the same results, terms
and diagnostics; and the texts reach both the parser's successes and its
errors.
*/
bool
d_tests_face_parity_parse(void)
{
#if (D_TESTS_FACE_FULL == 1)
    DTestsFaceSides sides;
    std::size_t     ok = 0u;

    D_TEST_REQUIRE(sides.built());

    for (std::size_t i = 0u; i < d_tests_face_text_count; ++i)
    {
        D_TEST_REQUIRE(d_tests_face_parity_text(sides, d_tests_face_texts[i],
                                                NULL, ok));
    }

    D_TEST_REQUIRE( (ok >= 12u) && (ok + 10u <= d_tests_face_text_count) );

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}

/*
d_tests_face_parity_options
  Every text both ways with options set: one error allowed, three in the
structure-first order, and a depth guard of three; the face's DOptions and
the C options, the same fields set.
*/
bool
d_tests_face_parity_options(void)
{
#if (D_TESTS_FACE_FULL == 1)
    DTestsFaceSides       sides;
    ::dj_parse_expr_options c_options[3];
    DOptions              f_options[3];
    std::size_t           ok = 0u;

    D_TEST_REQUIRE(sides.built());

    std::memset(c_options, 0, sizeof(c_options));
    c_options[0].max_errors  = 1u;
    c_options[1].max_errors  = 3u;
    c_options[1].error_order = 2u;
    c_options[2].max_depth   = 3u;

    for (std::size_t k = 0u; k < 3u; ++k)
    {
        f_options[k].max_errors  = c_options[k].max_errors;
        f_options[k].error_order = c_options[k].error_order;
        f_options[k].max_depth   = c_options[k].max_depth;

        D_TEST_REQUIRE(std::memcmp(&c_options[k],
                                   static_cast< ::dj_parse_expr_options* >(
                                       &f_options[k]),
                                   sizeof(::dj_parse_expr_options)) == 0);

        for (std::size_t i = 0u; i < d_tests_face_text_count; ++i)
        {
            D_TEST_REQUIRE(d_tests_face_parity_text(sides,
                                                    d_tests_face_texts[i],
                                                    &f_options[k], ok));
        }
    }

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}

/*
d_tests_face_parity_render
  Every term both ways rendered both ways: with the spans' text, the groups
kept, compact, without the text, and with an atom writer of each side's
own.
*/
bool
d_tests_face_parity_render(void)
{
#if (D_TESTS_FACE_FULL == 1)
    DTestsFaceSides sides;
    std::size_t     rendered = 0u;

    D_TEST_REQUIRE(sides.built());

    for (std::size_t i = 0u; i < d_tests_face_text_count; ++i)
    {
        const char* const text = d_tests_face_texts[i];
        DTestsFaceCParse  c;
        const bool        bound = d_tests_face_c_parse(sides.c(), text, NULL,
                                                       c);
        const DResult     f     = d_tests_face_scan(sides.face(), text);
        bool              same  = bound;

        for (re_std::uint32_t k = 0u; ( (same) && (f.ok()) && (k < 5u) );
             ++k)
        {
            ::dj_parse_expr_render_options c_options;
            DRenderOptions                 f_options;
            std::string                    f_text;

            std::memset(&c_options, 0, sizeof(c_options));
            c_options.text        = (k == 3u) ? NULL : text;
            c_options.text_length = (k == 3u) ? 0u : std::strlen(text);
            c_options.flags       =
                (k == 1u) ? D_PARSE_EXPR_RENDER_KEEP_GROUPS
              : (k == 2u) ? D_PARSE_EXPR_RENDER_COMPACT
                          : 0u;
            static_cast< ::dj_parse_expr_render_options& >(f_options) =
                c_options;

            if (k == 4u)
            {
                DTestsFaceBraces braces(text);

                c_options.atom     = &d_tests_face_c_atom;
                c_options.atom_ctx = const_cast<char*>(text);

                same = ( (djinterp::parse::expr::render(sides.face(),
                                                        f.term(), f.root(),
                                                        braces, f_text,
                                                        &f_options)) &&
                         (f_text ==
                          d_tests_face_c_render(sides.c(), &c.m_term,
                                                c.m_result.root,
                                                &c_options)) );
            }
            else
            {
                same = ( (djinterp::parse::expr::render(sides.face(),
                                                        f.term(), f.root(),
                                                        f_text,
                                                        &f_options)) &&
                         (f_text ==
                          d_tests_face_c_render(sides.c(), &c.m_term,
                                                c.m_result.root,
                                                &c_options)) );
            }

            rendered += (same) ? 1u : 0u;
        }

        d_tests_face_c_release(c);

        if (!same)
        {
            d_tests_face_note("the renders", text);
        }

        D_TEST_REQUIRE(same);
    }

    D_TEST_REQUIRE(rendered >= 5u * 12u);

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}

/*
d_tests_face_parity_fold
  Every term both ways folded from every node: the same answer, the same
value, from as many nodes; with and without holes allowed, and from a root
that is no node.
*/
bool
d_tests_face_parity_fold(void)
{
#if (D_TESTS_FACE_FULL == 1)
    DTestsFaceSides sides;
    std::size_t     folds = 0u;

    D_TEST_REQUIRE(sides.built());

    for (std::size_t i = 0u; i < d_tests_face_text_count; ++i)
    {
        const char* const text = d_tests_face_texts[i];
        DTestsFaceCParse  c;
        const bool        bound = d_tests_face_c_parse(sides.c(), text, NULL,
                                                       c);
        const DResult     f     = d_tests_face_scan(sides.face(), text);
        bool              same  = bound;

        for (re_std::uint32_t flags = 0u; ( (same) && (flags < 2u) );
             ++flags)
        {
            // each node, then one past the last
            for (re_std::uint32_t root = 0u;
                 ( (same) && (root <= c.m_term.node_count) );
                 ++root)
            {
                DTestsFaceCHash  c_ctx;
                re_std::uint32_t c_value = 0u;
                DTestsFaceHash   algebra(text);
                re_std::uint32_t f_value = 0u;

                c_ctx.m_text  = text;
                c_ctx.m_calls = 0u;

                const int c_rc = d_parse_expr_fold(&c.m_term, root,
                                                   sizeof(re_std::uint32_t),
                                                   NULL, NULL,
                                                   &d_tests_face_c_hash,
                                                   &c_ctx, flags, &c_value);
                const int f_rc = djinterp::parse::expr::fold(f.term(), root,
                                                             algebra,
                                                             f_value, flags);

                same = ( (c_rc == f_rc) && (c_value == f_value) &&
                         (c_ctx.m_calls == algebra.calls()) );
                folds += ( (same) && (c_rc == 0) ) ? 1u : 0u;
            }
        }

        d_tests_face_c_release(c);

        if (!same)
        {
            d_tests_face_note("the folds", text);
        }

        D_TEST_REQUIRE(same);
    }

    D_TEST_REQUIRE(folds >= 100u);

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}

/*
d_tests_face_parity_walk
  Every term both ways walked from its root: plain, the sums' and the
calls' children skipped, and stopped at the second and the fourth entry:
the same visits and the same answer; and the same digest at every node.
*/
bool
d_tests_face_parity_walk(void)
{
#if (D_TESTS_FACE_FULL == 1)
    static const re_std::uint32_t SKIP[5] =
    {
        0u, D_TESTS_FACE_O_ADD, D_TESTS_FACE_O_CALL, 0u, 0u
    };
    static const re_std::uint32_t STOP[5] = { 0u, 0u, 0u, 2u, 4u };
    DTestsFaceSides               sides;
    std::size_t                   walks = 0u;

    D_TEST_REQUIRE(sides.built());

    for (std::size_t i = 0u; i < d_tests_face_text_count; ++i)
    {
        const char* const text = d_tests_face_texts[i];
        DTestsFaceCParse  c;
        const bool        bound = d_tests_face_c_parse(sides.c(), text, NULL,
                                                       c);
        const DResult     f     = d_tests_face_scan(sides.face(), text);
        bool              same  = bound;

        for (re_std::uint32_t k = 0u; ( (same) && (k < 5u) ); ++k)
        {
            DTestsFaceCVisits    c_ctx;
            DTestsFaceSameVisits visitor(SKIP[k], STOP[k]);

            c_ctx.m_skip_op = SKIP[k];
            c_ctx.m_stop_at = STOP[k];
            c_ctx.m_entered = 0u;

            const bool c_ran = d_parse_expr_walk(&c.m_term, c.m_result.root,
                                                 &d_tests_face_c_enter,
                                                 &d_tests_face_c_leave,
                                                 &c_ctx);
            const bool f_ran = djinterp::parse::expr::walk(f.term(),
                                                           f.root(),
                                                           visitor);

            same = ( (c_ran == f_ran) &&
                     (c_ctx.m_visits == visitor.visits()) );
            walks += ( (same) && (c_ran) ) ? 1u : 0u;
        }

        for (re_std::uint32_t n = 0u;
             ( (same) && (n <= c.m_term.node_count) );
             ++n)
        {
            same = (d_parse_expr_term_digest(&c.m_term, n) ==
                    f.term().digest(n));
        }

        d_tests_face_c_release(c);

        if (!same)
        {
            d_tests_face_note("the walks or digests", text);
        }

        D_TEST_REQUIRE(same);
    }

    D_TEST_REQUIRE(walks >= 5u * 12u);

    return true;
#else
    D_TEST_SKIP(D_TESTS_FACE_WITHOUT);
#endif  // D_TESTS_FACE_FULL
}
