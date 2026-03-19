#include "../parse/c_parser.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  internal  —  C cursor filter and enrich
// ================================================================

NS_INTERNAL

    /*
    c_cursor_filter
      Returns true for cursor kinds meaningful in a C TU.

    Parameter(s):
      _kind: the CXCursorKind to test.
    Return:
      true if the kind should be visited, false otherwise.
    */
    bool
    c_cursor_filter
    (
        CXCursorKind _kind
    )
    {
        switch (_kind)
        {
            case CXCursor_StructDecl:
            case CXCursor_UnionDecl:
            case CXCursor_EnumDecl:
            case CXCursor_EnumConstantDecl:
            case CXCursor_FunctionDecl:
            case CXCursor_FieldDecl:
            case CXCursor_VarDecl:
            case CXCursor_ParmDecl:
            case CXCursor_TypedefDecl:
            case CXCursor_MacroDefinition:
            case CXCursor_InclusionDirective:
                return true;

            default:
                return false;
        }
    }

    /*
    c_cursor_enrich
      Enriches a symbol_data with C-specific details.  For C
    this is primarily the symbol_kind mapping.

    Parameter(s):
      _cursor:  the libclang cursor.
      _data:    the symbol_data to enrich.
      _strings: the string table for interning.
    Return:
      none.
    */
    void
    c_cursor_enrich
    (
        CXCursor                _cursor,
        arena::symbol_data&     _data,
        arena::string_table&    _strings
    )
    {
        CXCursorKind kind = clang_getCursorKind(_cursor);

        _data.kind   = cx_kind_to_symbol_kind_c(kind);
        _data.access = arena::access_specifier::unspecified;

        // handle union as struct_decl
        if (kind == CXCursor_UnionDecl)
        {
            _data.kind = arena::symbol_kind::struct_decl;
        }

        return;
    }

NS_END  // internal


// ================================================================
//  c_parser
// ================================================================

/*
c_parser (constructor)
  Constructs a C parser writing into the given context.

Parameter(s):
  _ctx:        the parse context (arena + xref + strings).
  _filename:   the virtual filename for the source buffer.
  _extra_args: additional compiler flags beyond the defaults.
Return:
  (constructor)
*/
c_parser::c_parser
(
    parse_context&                  _ctx,
    const char*                     _filename,
    const std::vector<const char*>& _extra_args
)
    : m_ctx      (_ctx),
      m_filename (_filename)
{
    m_args.push_back("-std=c11");
    m_args.push_back("-x");
    m_args.push_back("c");

    for (const char* arg : _extra_args)
    {
        m_args.push_back(arg);
    }
}


/*
do_parse
  Parses the source buffer described by _state through libclang
and populates the arena.

Parameter(s):
  _state: the parse state describing the source buffer.
Return:
  A parse_result containing the root node_id on success, or a
  parse_error on failure.
*/
parse_result<c_parser::result_type>
c_parser::do_parse
(
    parse_state<input_type>& _state
)
{
    if ( (_state.at_end()) ||
         (!_state.current()) )
    {
        return parse_result<result_type>::make_error(
            -1,
            _state.offset,
            "empty source buffer"
        );
    }

    const char* data   = _state.current();
    std::size_t length = _state.remaining();

    // parse options
    unsigned options =
        CXTranslationUnit_DetailedPreprocessingRecord |
        CXTranslationUnit_KeepGoing;

    CXIndex             index = nullptr;
    CXTranslationUnit   tu    = nullptr;

    bool ok = clang_parse_tu(
        data,
        length,
        m_filename,
        m_args.data(),
        static_cast<int>(m_args.size()),
        options,
        index,
        tu
    );

    if (!ok)
    {
        return parse_result<result_type>::make_error(
            -2,
            _state.offset,
            "libclang failed to parse translation unit"
        );
    }

    // check for fatal diagnostics
    unsigned num_diags = clang_getNumDiagnostics(tu);
    bool     has_fatal = false;

    for (unsigned i = 0; i < num_diags; ++i)
    {
        CXDiagnostic diag = clang_getDiagnostic(tu, i);

        CXDiagnosticSeverity severity =
            clang_getDiagnosticSeverity(diag);

        if (severity == CXDiagnostic_Fatal)
        {
            has_fatal = true;
        }

        clang_disposeDiagnostic(diag);
    }

    if (has_fatal)
    {
        clang_disposeTranslationUnit(tu);
        clang_disposeIndex(index);

        return parse_result<result_type>::make_error(
            -3,
            _state.offset,
            "fatal diagnostic during C parse"
        );
    }

    // walk the AST
    arena::node_id root = clang_walk_tu(
        tu,
        m_ctx,
        m_filename,
        internal::c_cursor_filter,
        internal::c_cursor_enrich
    );

    // advance parse_state past entire buffer
    _state.advance(length);

    // clean up libclang resources
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return parse_result<result_type>(root);
}


NS_END  // parse
NS_END  // djinterp
