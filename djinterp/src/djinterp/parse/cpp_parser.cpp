#include "../parse/cpp_parser.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  internal  —  C++ cursor filter and enrich
// ================================================================

NS_INTERNAL

    /*
    cpp_cursor_filter
      Returns true for cursor kinds meaningful in a C++ TU.
    This is a superset of the C filter.

    Parameter(s):
      _kind: the CXCursorKind to test.
    Return:
      true if the kind should be visited, false otherwise.
    */
    bool
    cpp_cursor_filter
    (
        CXCursorKind _kind
    )
    {
        switch (_kind)
        {
            // C-shared
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

            // C++ specific
            case CXCursor_Namespace:
            case CXCursor_ClassDecl:
            case CXCursor_ClassTemplate:
            case CXCursor_ClassTemplatePartialSpecialization:
            case CXCursor_CXXMethod:
            case CXCursor_Constructor:
            case CXCursor_Destructor:
            case CXCursor_ConversionFunction:
            case CXCursor_FunctionTemplate:
            case CXCursor_TypeAliasDecl:
            case CXCursor_TypeAliasTemplateDecl:
            case CXCursor_TemplateTypeParameter:
            case CXCursor_NonTypeTemplateParameter:
            case CXCursor_TemplateTemplateParameter:
            case CXCursor_CXXBaseSpecifier:
            case CXCursor_UsingDeclaration:
            case CXCursor_UsingDirective:
            case CXCursor_CXXAccessSpecifier:
            case CXCursor_FriendDecl:
            case CXCursor_NamespaceAlias:
                return true;

            default:
                return false;
        }
    }

    /*
    cpp_cursor_enrich
      Enriches a symbol_data with C++-specific details:
    symbol_kind mapping, access specifiers, and method
    qualifiers.

    Parameter(s):
      _cursor:  the libclang cursor.
      _data:    the symbol_data to enrich.
      _strings: the string table for interning.
    Return:
      none.
    */
    void
    cpp_cursor_enrich
    (
        CXCursor                _cursor,
        arena::symbol_data&     _data,
        arena::string_table&    _strings
    )
    {
        CXCursorKind kind = clang_getCursorKind(_cursor);

        // map kind
        _data.kind = cx_kind_to_symbol_kind_cpp(kind);

        // union -> struct_decl
        if (kind == CXCursor_UnionDecl)
        {
            _data.kind = arena::symbol_kind::struct_decl;
        }

        // access specifier
        _data.access = cx_access_specifier(_cursor);

        // method qualifiers — pack into storage byte
        std::uint8_t storage = cx_storage_class(_cursor);

        if ( (kind == CXCursor_CXXMethod)          ||
             (kind == CXCursor_Constructor)         ||
             (kind == CXCursor_Destructor)          ||
             (kind == CXCursor_ConversionFunction) )
        {
            if (clang_CXXMethod_isVirtual(_cursor))
            {
                storage |= cpp_storage_flag::virtual_;
            }

            if (clang_CXXMethod_isPureVirtual(_cursor))
            {
                storage |= cpp_storage_flag::pure_virtual;
            }

            if (clang_CXXMethod_isConst(_cursor))
            {
                storage |= cpp_storage_flag::const_;
            }

            if (clang_CXXMethod_isStatic(_cursor))
            {
                storage |= cpp_storage_flag::static_;
            }
        }

        _data.storage = storage;

        // base specifiers — store base class name in type_id
        if (kind == CXCursor_CXXBaseSpecifier)
        {
            CXCursor referenced = clang_getCursorReferenced(
                _cursor
            );

            std::string base_name = cx_to_string(
                clang_getCursorSpelling(referenced)
            );

            if (!base_name.empty())
            {
                _data.type_id = _strings.intern(base_name);
            }

            _data.kind = arena::symbol_kind::class_decl;
        }

        // using declarations
        if ( (kind == CXCursor_UsingDeclaration) ||
             (kind == CXCursor_UsingDirective) )
        {
            _data.kind = arena::symbol_kind::type_alias_decl;
        }

        // namespace aliases
        if (kind == CXCursor_NamespaceAlias)
        {
            _data.kind = arena::symbol_kind::namespace_decl;
        }

        // access specifier nodes — structural, not symbols
        if (kind == CXCursor_CXXAccessSpecifier)
        {
            _data.kind = arena::symbol_kind::unknown;
        }

        // friend declarations
        if (kind == CXCursor_FriendDecl)
        {
            _data.kind = arena::symbol_kind::unknown;
        }

        // conversion functions
        if (kind == CXCursor_ConversionFunction)
        {
            _data.kind = arena::symbol_kind::method_decl;

            _data.signature_id = cx_function_signature(
                _cursor,
                _strings
            );
        }

        // template parameters
        if ( (kind == CXCursor_TemplateTypeParameter)    ||
             (kind == CXCursor_NonTypeTemplateParameter)  ||
             (kind == CXCursor_TemplateTemplateParameter) )
        {
            _data.kind = arena::symbol_kind::template_decl;
        }

        return;
    }

NS_END  // internal


// ================================================================
//  cpp_parser
// ================================================================

/*
cpp_parser (constructor)
  Constructs a C++ parser writing into the given context.

Parameter(s):
  _ctx:        the parse context (arena + xref + strings).
  _filename:   the virtual filename for the source buffer.
  _standard:   the C++ standard flag (e.g., "-std=c++17").
  _extra_args: additional compiler flags.
Return:
  (constructor)
*/
cpp_parser::cpp_parser
(
    parse_context&                  _ctx,
    const char*                     _filename,
    const char*                     _standard,
    const std::vector<const char*>& _extra_args
)
    : m_ctx      (_ctx),
      m_filename (_filename)
{
    m_args.push_back(_standard);
    m_args.push_back("-x");
    m_args.push_back("c++");
    m_args.push_back("-Wno-pragma-once-outside-header");

    for (const char* arg : _extra_args)
    {
        m_args.push_back(arg);
    }
}


/*
do_parse
  Parses the source buffer described by _state through libclang
and populates the arena with the full C++ symbol hierarchy.

Parameter(s):
  _state: the parse state describing the source buffer.
Return:
  A parse_result containing the root node_id on success, or a
  parse_error on failure.
*/
parse_result<cpp_parser::result_type>
cpp_parser::do_parse
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

    // check diagnostics
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
            "fatal diagnostic during C++ parse"
        );
    }

    // walk the AST
    arena::node_id root = clang_walk_tu(
        tu,
        m_ctx,
        m_filename,
        internal::cpp_cursor_filter,
        internal::cpp_cursor_enrich
    );

    _state.advance(length);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return parse_result<result_type>(root);
}


NS_END  // parse
NS_END  // djinterp
