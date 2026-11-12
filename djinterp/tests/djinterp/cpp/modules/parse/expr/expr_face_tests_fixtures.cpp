/*******************************************************************************
* djinterp [cpp/parse]                              expr_face_tests_fixtures.cpp
*
* The face suite's fixtures: the calculator built by the C API and by the
* face, the texts the parity block reads, the scanner's classes, and the
* parse by the C API alone that the face's is held to.
*
*
* path:      /tests/djinterp/cpp/modules/parse/expr/expr_face_tests_fixtures.cpp
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#include "./expr_face_tests.hpp"  // corresponding header


#if (D_TESTS_FACE_FULL == 1)


//==============================================================================
// THE LANGUAGE
//==============================================================================


// d_tests_face_texts
//   constant: well formed first, then each of the parser's errors at least
// once, then texts that hold more than one.
const char* const d_tests_face_texts[] =
{
    "1 + 2 * 3",
    "-a ^ b ^ c!",
    "f(x, y + 1)[2]",
    "a < b <= c and not d",
    "[1, 2; 3, 4]",
    "|x - y| / 2",
    "2 x y",
    "((a)) * (b + c)",
    "a or b and c",
    "g()",
    "-(-x)!",
    "x[1, 2]",
    "1 +",
    "(a",
    "a ) b",
    "1 2",
    "a # b",
    "",
    "[1, 2; 3",
    "x[1, 2, 3]",
    "a + * b - ",
    "f(a, ]",
    "(1 + ) * (2 - ) / (3 *)",
    "a < < b"
};

// d_tests_face_text_count
//   constant: how many texts there are.
const std::size_t d_tests_face_text_count =
    sizeof(d_tests_face_texts) / sizeof(d_tests_face_texts[0]);


/*
d_tests_face_two_numbers
  The calculator's juxtaposition rule: never between two numbers.
*/
static bool
d_tests_face_two_numbers(
    void*                             _ctx,
    const struct dj_parse_expr_term*  _term,
    re_std::uint32_t                  _left,
    const struct dj_parse_expr_token* _token
)
{
    (void)_ctx;

    return (!( (_term->nodes[_left].kind == D_PARSE_EXPR_NODE_LEAF) &&
               (_term->nodes[_left].op == D_TESTS_FACE_C_NUM)       &&
               (_token->cls == D_TESTS_FACE_C_NUM) ));
}

/*
d_tests_face_bracket
  A C mixfix of two parts, zeroed but for them.
*/
static ::dj_parse_expr_mixfix
d_tests_face_bracket(
    re_std::uint8_t  _lead,
    re_std::uint32_t _open,
    const char*      _open_spelling,
    re_std::uint32_t _close,
    const char*      _close_spelling
)
{
    ::dj_parse_expr_mixfix m;

    std::memset(&m, 0, sizeof(m));
    m.lead             = _lead;
    m.parts            = 2u;
    m.part[0]          = _open;
    m.part[1]          = _close;
    m.part_spelling[0] = _open_spelling;
    m.part_spelling[1] = _close_spelling;

    return m;
}

/*
d_tests_face_c_ops
  The calculator's prefix, infix, postfix and chain operators, by the C API.
*/
static bool
d_tests_face_c_ops(
    ::dj_parse_expr_signature* _s
)
{
    return ( (d_parse_expr_define_atom(_s, D_TESTS_FACE_C_NUM, 0u)) &&
             (d_parse_expr_define_atom(_s, D_TESTS_FACE_C_NAME,
                                       D_PARSE_EXPR_ROLE_CALLABLE)) &&
             (d_parse_expr_define_op(_s, D_TESTS_FACE_O_NEG,
                                     D_PARSE_EXPR_FORM_PREFIX, 70,
                                     D_PARSE_EXPR_ASSOC_RIGHT, 0u, "-")) &&
             (d_parse_expr_define_op(_s, D_TESTS_FACE_O_NOT,
                                     D_PARSE_EXPR_FORM_PREFIX, 30,
                                     D_PARSE_EXPR_ASSOC_RIGHT,
                                     D_PARSE_EXPR_OP_WORD, "not")) &&
             (d_parse_expr_define_op(_s, D_TESTS_FACE_O_ADD,
                                     D_PARSE_EXPR_FORM_INFIX, 50,
                                     D_PARSE_EXPR_ASSOC_LEFT,
                                     D_PARSE_EXPR_OP_COMMUTATIVE, "+")) &&
             (d_parse_expr_define_op(_s, D_TESTS_FACE_O_SUB,
                                     D_PARSE_EXPR_FORM_INFIX, 50,
                                     D_PARSE_EXPR_ASSOC_LEFT, 0u, "-")) &&
             (d_parse_expr_define_op(_s, D_TESTS_FACE_O_MUL,
                                     D_PARSE_EXPR_FORM_INFIX, 60,
                                     D_PARSE_EXPR_ASSOC_LEFT, 0u, "*")) &&
             (d_parse_expr_define_op(_s, D_TESTS_FACE_O_DIV,
                                     D_PARSE_EXPR_FORM_INFIX, 60,
                                     D_PARSE_EXPR_ASSOC_LEFT, 0u, "/")) &&
             (d_parse_expr_define_op(_s, D_TESTS_FACE_O_POW,
                                     D_PARSE_EXPR_FORM_INFIX, 80,
                                     D_PARSE_EXPR_ASSOC_RIGHT, 0u, "^")) &&
             (d_parse_expr_define_op(_s, D_TESTS_FACE_O_AND,
                                     D_PARSE_EXPR_FORM_INFIX, 20,
                                     D_PARSE_EXPR_ASSOC_LEFT,
                                     D_PARSE_EXPR_OP_WORD, "and")) &&
             (d_parse_expr_define_op(_s, D_TESTS_FACE_O_OR,
                                     D_PARSE_EXPR_FORM_INFIX, 10,
                                     D_PARSE_EXPR_ASSOC_LEFT,
                                     D_PARSE_EXPR_OP_WORD, "or")) &&
             (d_parse_expr_define_op(_s, D_TESTS_FACE_O_FACT,
                                     D_PARSE_EXPR_FORM_POSTFIX, 90,
                                     D_PARSE_EXPR_ASSOC_LEFT, 0u, "!")) &&
             (d_parse_expr_define_chain(_s, D_TESTS_FACE_O_LT, 40,
                                        D_TESTS_FACE_O_AND, "<")) &&
             (d_parse_expr_define_chain(_s, D_TESTS_FACE_O_LE, 40,
                                        D_TESTS_FACE_O_AND, "<=")) &&
             (d_parse_expr_define_op(_s, D_TESTS_FACE_O_ROW,
                                     D_PARSE_EXPR_FORM_PREFIX, 0,
                                     D_PARSE_EXPR_ASSOC_RIGHT, 0u, "row")) &&
             (d_parse_expr_define_op(_s, D_TESTS_FACE_O_MATRIX,
                                     D_PARSE_EXPR_FORM_PREFIX, 0,
                                     D_PARSE_EXPR_ASSOC_RIGHT, 0u,
                                     "matrix")) );
}

/*
d_tests_face_c_tokens
  The classes that begin the calculator's operators, by the C API.
*/
static bool
d_tests_face_c_tokens(
    ::dj_parse_expr_signature* _s
)
{
    return ( (d_parse_expr_define_token(_s, D_TESTS_FACE_C_MINUS,
                                        D_PARSE_EXPR_OPERAND,
                                        D_TESTS_FACE_O_NEG, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_NOT,
                                        D_PARSE_EXPR_OPERAND,
                                        D_TESTS_FACE_O_NOT, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_PLUS,
                                        D_PARSE_EXPR_OPERATOR,
                                        D_TESTS_FACE_O_ADD, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_MINUS,
                                        D_PARSE_EXPR_OPERATOR,
                                        D_TESTS_FACE_O_SUB, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_STAR,
                                        D_PARSE_EXPR_OPERATOR,
                                        D_TESTS_FACE_O_MUL, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_SLASH,
                                        D_PARSE_EXPR_OPERATOR,
                                        D_TESTS_FACE_O_DIV, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_CARET,
                                        D_PARSE_EXPR_OPERATOR,
                                        D_TESTS_FACE_O_POW, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_AND,
                                        D_PARSE_EXPR_OPERATOR,
                                        D_TESTS_FACE_O_AND, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_OR,
                                        D_PARSE_EXPR_OPERATOR,
                                        D_TESTS_FACE_O_OR, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_BANG,
                                        D_PARSE_EXPR_OPERATOR,
                                        D_TESTS_FACE_O_FACT, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_LT,
                                        D_PARSE_EXPR_OPERATOR,
                                        D_TESTS_FACE_O_LT, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_LE,
                                        D_PARSE_EXPR_OPERATOR,
                                        D_TESTS_FACE_O_LE, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_LP,
                                        D_PARSE_EXPR_OPERAND,
                                        D_TESTS_FACE_O_GROUP, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_LP,
                                        D_PARSE_EXPR_OPERATOR,
                                        D_TESTS_FACE_O_CALL,
                                        D_PARSE_EXPR_ROLE_CALLEE)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_LB,
                                        D_PARSE_EXPR_OPERAND,
                                        D_TESTS_FACE_O_VECTOR, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_LB,
                                        D_PARSE_EXPR_OPERATOR,
                                        D_TESTS_FACE_O_INDEX, 0u)) &&
             (d_parse_expr_define_token(_s, D_TESTS_FACE_C_BAR,
                                        D_PARSE_EXPR_OPERAND,
                                        D_TESTS_FACE_O_ABS, 0u)) );
}

/*
d_tests_face_c_language
  The calculator by the C API alone, on the heap, sealed.
*/
bool
d_tests_face_c_language(
    ::dj_parse_expr_signature* _signature
)
{
    ::dj_parse_expr_mixfix group  =
        d_tests_face_bracket(0u, D_TESTS_FACE_C_LP, "(",
                             D_TESTS_FACE_C_RP, ")");
    ::dj_parse_expr_mixfix call   =
        d_tests_face_bracket(1u, D_TESTS_FACE_C_LP, "(",
                             D_TESTS_FACE_C_RP, ")");
    ::dj_parse_expr_mixfix index  =
        d_tests_face_bracket(1u, D_TESTS_FACE_C_LB, "[",
                             D_TESTS_FACE_C_RB, "]");
    ::dj_parse_expr_mixfix vector =
        d_tests_face_bracket(0u, D_TESTS_FACE_C_LB, "[",
                             D_TESTS_FACE_C_RB, "]");
    ::dj_parse_expr_mixfix bars   =
        d_tests_face_bracket(0u, D_TESTS_FACE_C_BAR, "|",
                             D_TESTS_FACE_C_BAR, "|");

    call.list_hole                = 1u;
    call.separator                = D_TESTS_FACE_C_COMMA;
    call.separator_spelling       = ",";
    call.max_items                = D_PARSE_EXPR_ITEMS_UNBOUNDED;
    index.list_hole               = 1u;
    index.separator               = D_TESTS_FACE_C_COMMA;
    index.separator_spelling      = ",";
    index.min_items               = 1u;
    index.max_items               = 2u;
    vector.list_hole              = 1u;
    vector.separator              = D_TESTS_FACE_C_COMMA;
    vector.separator_spelling     = ",";
    vector.max_items              = D_PARSE_EXPR_ITEMS_UNBOUNDED;
    vector.row_separator          = D_TESTS_FACE_C_SEMI;
    vector.row_separator_spelling = ";";
    vector.row_op                 = D_TESTS_FACE_O_ROW;
    vector.rows_op                = D_TESTS_FACE_O_MATRIX;

    d_parse_expr_signature_init_heap(_signature);

    return ( (d_tests_face_c_ops(_signature)) &&
             (d_parse_expr_define_mixfix(_signature, D_TESTS_FACE_O_GROUP,
                                         0, 0u, D_PARSE_EXPR_OP_TRANSPARENT,
                                         &group)) &&
             (d_parse_expr_define_mixfix(_signature, D_TESTS_FACE_O_CALL,
                                         100, D_PARSE_EXPR_ASSOC_LEFT, 0u,
                                         &call)) &&
             (d_parse_expr_define_mixfix(_signature, D_TESTS_FACE_O_INDEX,
                                         100, D_PARSE_EXPR_ASSOC_LEFT, 0u,
                                         &index)) &&
             (d_parse_expr_define_mixfix(_signature, D_TESTS_FACE_O_VECTOR,
                                         0, 0u, 0u, &vector)) &&
             (d_parse_expr_define_mixfix(_signature, D_TESTS_FACE_O_ABS,
                                         0, 0u, 0u, &bars)) &&
             (d_tests_face_c_tokens(_signature)) &&
             (d_parse_expr_juxtapose(_signature, D_TESTS_FACE_O_MUL,
                                     d_tests_face_two_numbers, NULL)) &&
             (d_parse_expr_signature_seal(_signature, NULL)) );
}

/*
d_tests_face_language
  The calculator by the face, sealed: the same rows, in the same order.
*/
bool
d_tests_face_language(
    djinterp::parse::expr::DSignature& _signature
)
{
    using djinterp::parse::expr::DMixfix;

    DMixfix group;
    DMixfix call;
    DMixfix index;
    DMixfix vector;
    DMixfix bars;

    group.bracket(D_TESTS_FACE_C_LP, "(", D_TESTS_FACE_C_RP, ")");
    call.bracket(D_TESTS_FACE_C_LP, "(", D_TESTS_FACE_C_RP, ")", 1u)
        .list(D_TESTS_FACE_C_COMMA, ",");
    index.bracket(D_TESTS_FACE_C_LB, "[", D_TESTS_FACE_C_RB, "]", 1u)
         .list(D_TESTS_FACE_C_COMMA, ",", 1u, 2u);
    vector.bracket(D_TESTS_FACE_C_LB, "[", D_TESTS_FACE_C_RB, "]")
          .list(D_TESTS_FACE_C_COMMA, ",")
          .rows(D_TESTS_FACE_C_SEMI, ";", D_TESTS_FACE_O_ROW,
                D_TESTS_FACE_O_MATRIX);
    bars.bracket(D_TESTS_FACE_C_BAR, "|", D_TESTS_FACE_C_BAR, "|");

    _signature
        .atom(D_TESTS_FACE_C_NUM)
        .atom(D_TESTS_FACE_C_NAME, D_PARSE_EXPR_ROLE_CALLABLE)
        .prefix(D_TESTS_FACE_O_NEG, 70, "-")
        .prefix(D_TESTS_FACE_O_NOT, 30, "not", D_PARSE_EXPR_OP_WORD)
        .infix(D_TESTS_FACE_O_ADD, 50, D_PARSE_EXPR_ASSOC_LEFT, "+",
               D_PARSE_EXPR_OP_COMMUTATIVE)
        .infix(D_TESTS_FACE_O_SUB, 50, D_PARSE_EXPR_ASSOC_LEFT, "-")
        .infix(D_TESTS_FACE_O_MUL, 60, D_PARSE_EXPR_ASSOC_LEFT, "*")
        .infix(D_TESTS_FACE_O_DIV, 60, D_PARSE_EXPR_ASSOC_LEFT, "/")
        .infix(D_TESTS_FACE_O_POW, 80, D_PARSE_EXPR_ASSOC_RIGHT, "^")
        .infix(D_TESTS_FACE_O_AND, 20, D_PARSE_EXPR_ASSOC_LEFT, "and",
               D_PARSE_EXPR_OP_WORD)
        .infix(D_TESTS_FACE_O_OR, 10, D_PARSE_EXPR_ASSOC_LEFT, "or",
               D_PARSE_EXPR_OP_WORD)
        .postfix(D_TESTS_FACE_O_FACT, 90, "!")
        .chain(D_TESTS_FACE_O_LT, 40, D_TESTS_FACE_O_AND, "<")
        .chain(D_TESTS_FACE_O_LE, 40, D_TESTS_FACE_O_AND, "<=")
        .prefix(D_TESTS_FACE_O_ROW, 0, "row")
        .prefix(D_TESTS_FACE_O_MATRIX, 0, "matrix")
        .mixfix(D_TESTS_FACE_O_GROUP, 0, group, 0u,
                D_PARSE_EXPR_OP_TRANSPARENT)
        .mixfix(D_TESTS_FACE_O_CALL, 100, call, D_PARSE_EXPR_ASSOC_LEFT)
        .mixfix(D_TESTS_FACE_O_INDEX, 100, index, D_PARSE_EXPR_ASSOC_LEFT)
        .mixfix(D_TESTS_FACE_O_VECTOR, 0, vector)
        .mixfix(D_TESTS_FACE_O_ABS, 0, bars)
        .token(D_TESTS_FACE_C_MINUS, D_PARSE_EXPR_OPERAND, D_TESTS_FACE_O_NEG)
        .token(D_TESTS_FACE_C_NOT, D_PARSE_EXPR_OPERAND, D_TESTS_FACE_O_NOT)
        .token(D_TESTS_FACE_C_PLUS, D_PARSE_EXPR_OPERATOR, D_TESTS_FACE_O_ADD)
        .token(D_TESTS_FACE_C_MINUS, D_PARSE_EXPR_OPERATOR, D_TESTS_FACE_O_SUB)
        .token(D_TESTS_FACE_C_STAR, D_PARSE_EXPR_OPERATOR, D_TESTS_FACE_O_MUL)
        .token(D_TESTS_FACE_C_SLASH, D_PARSE_EXPR_OPERATOR, D_TESTS_FACE_O_DIV)
        .token(D_TESTS_FACE_C_CARET, D_PARSE_EXPR_OPERATOR, D_TESTS_FACE_O_POW)
        .token(D_TESTS_FACE_C_AND, D_PARSE_EXPR_OPERATOR, D_TESTS_FACE_O_AND)
        .token(D_TESTS_FACE_C_OR, D_PARSE_EXPR_OPERATOR, D_TESTS_FACE_O_OR)
        .token(D_TESTS_FACE_C_BANG, D_PARSE_EXPR_OPERATOR, D_TESTS_FACE_O_FACT)
        .token(D_TESTS_FACE_C_LT, D_PARSE_EXPR_OPERATOR, D_TESTS_FACE_O_LT)
        .token(D_TESTS_FACE_C_LE, D_PARSE_EXPR_OPERATOR, D_TESTS_FACE_O_LE)
        .token(D_TESTS_FACE_C_LP, D_PARSE_EXPR_OPERAND, D_TESTS_FACE_O_GROUP)
        .token(D_TESTS_FACE_C_LP, D_PARSE_EXPR_OPERATOR, D_TESTS_FACE_O_CALL,
               D_PARSE_EXPR_ROLE_CALLEE)
        .token(D_TESTS_FACE_C_LB, D_PARSE_EXPR_OPERAND, D_TESTS_FACE_O_VECTOR)
        .token(D_TESTS_FACE_C_LB, D_PARSE_EXPR_OPERATOR, D_TESTS_FACE_O_INDEX)
        .token(D_TESTS_FACE_C_BAR, D_PARSE_EXPR_OPERAND, D_TESTS_FACE_O_ABS)
        .juxtapose(D_TESTS_FACE_O_MUL, &d_tests_face_two_numbers);

    return _signature.seal();
}

/*
d_tests_face_config
  The scanner's classes for the calculator: names, numbers and strays; its
operators' spellings come from the signature.
*/
::dj_parse_expr_scan_config
d_tests_face_config(void)
{
    ::dj_parse_expr_scan_config config;

    std::memset(&config, 0, sizeof(config));
    config.name_class   = D_TESTS_FACE_C_NAME;
    config.number_class = D_TESTS_FACE_C_NUM;
    config.stray_class  = D_TESTS_FACE_C_STRAY;

    return config;
}


//==============================================================================
// THE C PARSE
//==============================================================================


/*
d_tests_face_c_parse
  Parses a text with the C API alone: the default scanner, a term on the
heap, and a sink of the face's default size, so that its records are the
ones the face copies.
  Returns false where the scan could not be bound; the term is then empty
but still to be released.
*/
bool
d_tests_face_c_parse(
    const ::dj_parse_expr_signature* _signature,
    const char*                      _text,
    const ::dj_parse_expr_options*   _options,
    DTestsFaceCParse&                _out
)
{
    const ::dj_parse_expr_scan_config config = d_tests_face_config();
    ::dj_parse_expr_scanner           scanner;
    ::dj_parse_expr_source            source;

    std::memset(&_out.m_result, 0, sizeof(_out.m_result));
    d_parse_expr_term_init_heap(&_out.m_term);
    d_parse_diag_sink_init(&_out.m_sink, _out.m_items,
                           (re_std::uint32_t)D_PARSE_DIAG_DEFAULT_ITEMS,
                           _out.m_text,
                           (re_std::uint32_t)D_PARSE_DIAG_DEFAULT_TEXT);

    if (!d_parse_expr_scan(&source, &scanner, _signature, &config, _text,
                           std::strlen(_text)))
    {
        return false;
    }

    (void)d_parse_expr_parse(_signature, &source, _options, &_out.m_term,
                             &_out.m_sink, &_out.m_result);

    return true;
}

/*
d_tests_face_scan
  Parses a text with the face and the default scanner, the diagnostics
copied into the result.
*/
djinterp::parse::expr::DResult
d_tests_face_scan(
    const djinterp::parse::expr::DSignature& _signature,
    const char*                              _text,
    const ::dj_parse_expr_options*           _options
)
{
    const ::dj_parse_expr_scan_config config = d_tests_face_config();
    djinterp::parse::expr::DScanner   scanner;

    if (!scanner.bind(_signature, config, _text))
    {
        return djinterp::parse::expr::DResult();
    }

    return djinterp::parse::expr::parse(_signature, scanner.source(),
                                        _options);
}

/*
d_tests_face_c_release
  Gives back a C parse's term.
*/
void
d_tests_face_c_release(
    DTestsFaceCParse& _parse
)
{
    d_parse_expr_term_release(&_parse.m_term);

    return;
}

/*
d_tests_face_same_span
  Whether two spans are the same, field for field.
*/
bool
d_tests_face_same_span(
    const ::dj_parse_span& _a,
    const ::dj_parse_span& _b
)
{
    return ( (_a.source == _b.source) &&
             (_a.offset == _b.offset) &&
             (_a.length == _b.length) &&
             (_a.line == _b.line)     &&
             (_a.column == _b.column) );
}

/*
d_tests_face_same_nodes
  Whether two terms are the same arena: their counts, root, depth and flags,
each node field for field, and each argument.
*/
bool
d_tests_face_same_nodes(
    const ::dj_parse_expr_term* _a,
    const ::dj_parse_expr_term* _b
)
{
    if ( (_a->node_count != _b->node_count) ||
         (_a->arg_count != _b->arg_count)   ||
         (_a->root != _b->root)             ||
         (_a->depth != _b->depth)           ||
         (_a->flags != _b->flags) )
    {
        return false;
    }

    for (re_std::uint32_t i = 0u; i < _a->node_count; ++i)
    {
        const ::dj_parse_expr_node& x = _a->nodes[i];
        const ::dj_parse_expr_node& y = _b->nodes[i];

        if ( (x.kind != y.kind)   || (x.flags != y.flags)   ||
             (x.depth != y.depth) || (x.op != y.op)         ||
             (x.first != y.first) || (x.count != y.count)   ||
             (x.offset != y.offset) || (x.length != y.length) )
        {
            return false;
        }
    }

    for (re_std::uint32_t i = 0u; i < _a->arg_count; ++i)
    {
        if (_a->args[i] != _b->args[i])
        {
            return false;
        }
    }

    return true;
}


#endif  // D_TESTS_FACE_FULL
