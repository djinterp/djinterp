#include "../parse/cpp_def_parser.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  internal  —  body visitor
// ================================================================

NS_INTERNAL

    // cpp_def_visitor_context
    //   struct: callback context for the definition body walker.
    struct cpp_def_visitor_context
    {
        parse_context*      ctx;
        arena::node_id      def_node_id;
    };

    /*
    cpp_def_body_visitor
      Walks the body of a function definition and extracts call
    expressions, local variables, return statements, member
    references, and declaration references.

    Parameter(s):
      _cursor: the current cursor.
      _parent: the parent cursor (libclang-internal).
      _data:   the cpp_def_visitor_context.
    Return:
      CXChildVisit_Recurse always.
    */
    CXChildVisitResult
    cpp_def_body_visitor
    (
        CXCursor        _cursor,
        CXCursor        _parent,
        CXClientData    _data
    )
    {
        auto* vctx = static_cast<cpp_def_visitor_context*>(
            _data
        );

        CXCursorKind kind = clang_getCursorKind(_cursor);

        // skip system headers
        CXSourceLocation loc = clang_getCursorLocation(_cursor);

        if (clang_Location_isInSystemHeader(loc))
        {
            return CXChildVisit_Continue;
        }

        arena::symbol_data child_data;
        bool should_add = false;

        switch (kind)
        {
            case CXCursor_CallExpr:
            {
                child_data.kind = def_symbol_kind::call_expr;

                CXCursor callee = clang_getCursorReferenced(
                    _cursor
                );

                child_data.name_id = cx_cursor_spelling(
                    callee,
                    vctx->ctx->strings
                );

                child_data.type_id = cx_type_spelling(
                    _cursor,
                    vctx->ctx->strings
                );

                child_data.location = cx_source_location(
                    _cursor,
                    vctx->ctx->strings
                );

                CXCursorKind callee_kind = clang_getCursorKind(
                    callee
                );

                if ( (callee_kind == CXCursor_FunctionDecl)   ||
                     (callee_kind == CXCursor_CXXMethod)      ||
                     (callee_kind == CXCursor_Constructor)     ||
                     (callee_kind == CXCursor_FunctionTemplate) )
                {
                    child_data.signature_id =
                        cx_function_signature(
                            callee,
                            vctx->ctx->strings
                        );
                }

                should_add = true;

                break;
            }

            case CXCursor_VarDecl:
            {
                child_data.kind = def_symbol_kind::local_var;

                child_data.name_id = cx_cursor_spelling(
                    _cursor,
                    vctx->ctx->strings
                );

                child_data.type_id = cx_type_spelling(
                    _cursor,
                    vctx->ctx->strings
                );

                child_data.location = cx_source_location(
                    _cursor,
                    vctx->ctx->strings
                );

                child_data.storage = cx_storage_class(_cursor);

                should_add = true;

                break;
            }

            case CXCursor_ReturnStmt:
            {
                child_data.kind = def_symbol_kind::return_stmt;

                child_data.location = cx_source_location(
                    _cursor,
                    vctx->ctx->strings
                );

                should_add = true;

                break;
            }

            case CXCursor_MemberRefExpr:
            {
                child_data.kind = def_symbol_kind::member_ref;

                child_data.name_id = cx_cursor_spelling(
                    _cursor,
                    vctx->ctx->strings
                );

                child_data.type_id = cx_type_spelling(
                    _cursor,
                    vctx->ctx->strings
                );

                child_data.location = cx_source_location(
                    _cursor,
                    vctx->ctx->strings
                );

                should_add = true;

                break;
            }

            case CXCursor_DeclRefExpr:
            {
                child_data.kind = def_symbol_kind::decl_ref;

                CXCursor referenced = clang_getCursorReferenced(
                    _cursor
                );

                child_data.name_id = cx_cursor_spelling(
                    referenced,
                    vctx->ctx->strings
                );

                child_data.type_id = cx_type_spelling(
                    referenced,
                    vctx->ctx->strings
                );

                child_data.location = cx_source_location(
                    _cursor,
                    vctx->ctx->strings
                );

                should_add = true;

                break;
            }

            default:
                break;
        }

        if (should_add)
        {
            std::uint64_t stable_id = cx_stable_id(_cursor);

            arena::node_id child_id = vctx->ctx->tree.allocate(
                stable_id,
                child_data
            );

            vctx->ctx->tree.append_child(
                vctx->def_node_id,
                child_id
            );

            vctx->ctx->xref.bind(
                stable_id,
                arena::domain::symbol,
                child_id
            );
        }

        return CXChildVisit_Recurse;
    }


    // --------------------------------------------------------
    //  top-level definition visitor
    // --------------------------------------------------------

    // cpp_def_top_visitor_context
    //   struct: context for the top-level walk that finds
    // function definitions.
    struct cpp_def_top_visitor_context
    {
        parse_context*      ctx;
        arena::node_id      root_id;
    };

    /*
    is_function_definition
      Returns true if the cursor is a function-like definition
    (has a body).

    Parameter(s):
      _cursor: the libclang cursor.
    Return:
      true if the cursor is a definition, false otherwise.
    */
    bool
    is_function_definition
    (
        CXCursor _cursor
    )
    {
        CXCursorKind kind = clang_getCursorKind(_cursor);

        bool is_function_like =
            ( (kind == CXCursor_FunctionDecl)      ||
              (kind == CXCursor_CXXMethod)         ||
              (kind == CXCursor_Constructor)        ||
              (kind == CXCursor_Destructor)         ||
              (kind == CXCursor_ConversionFunction) ||
              (kind == CXCursor_FunctionTemplate) );

        if (!is_function_like)
        {
            return false;
        }

        return clang_isCursorDefinition(_cursor);
    }

    /*
    cpp_def_top_visitor
      Walks the top level of the AST looking for function
    definitions.  For each one found, creates a definition node,
    populates it, then walks the body for internal details.

    Parameter(s):
      _cursor: the current cursor.
      _parent: the parent cursor (libclang-internal).
      _data:   the cpp_def_top_visitor_context.
    Return:
      CXChildVisit_Continue for definitions (body walked
      manually), CXChildVisit_Recurse otherwise.
    */
    CXChildVisitResult
    cpp_def_top_visitor
    (
        CXCursor        _cursor,
        CXCursor        _parent,
        CXClientData    _data
    )
    {
        auto* vctx = static_cast<cpp_def_top_visitor_context*>(
            _data
        );

        // skip system headers
        CXSourceLocation loc = clang_getCursorLocation(_cursor);

        if (clang_Location_isInSystemHeader(loc))
        {
            return CXChildVisit_Continue;
        }

        if (is_function_definition(_cursor))
        {
            CXCursorKind kind = clang_getCursorKind(_cursor);

            // build the definition node
            arena::symbol_data def_data;

            def_data.kind = def_symbol_kind::function_def;

            def_data.location = cx_source_location(
                _cursor,
                vctx->ctx->strings
            );

            def_data.name_id = cx_cursor_spelling(
                _cursor,
                vctx->ctx->strings
            );

            def_data.type_id = cx_type_spelling(
                _cursor,
                vctx->ctx->strings
            );

            def_data.signature_id = cx_function_signature(
                _cursor,
                vctx->ctx->strings
            );

            def_data.comment_id = cx_raw_comment(
                _cursor,
                vctx->ctx->strings
            );

            def_data.access = cx_access_specifier(_cursor);

            // storage + method qualifiers
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

            def_data.storage = storage;

            // allocate and link
            std::uint64_t stable_id = cx_stable_id(_cursor);

            arena::node_id def_id = vctx->ctx->tree.allocate(
                stable_id,
                def_data
            );

            vctx->ctx->tree.append_child(
                vctx->root_id,
                def_id
            );

            vctx->ctx->xref.bind(
                stable_id,
                arena::domain::symbol,
                def_id
            );

            // add parameters as children
            int num_args = clang_Cursor_getNumArguments(
                _cursor
            );

            for (int i = 0; i < num_args; ++i)
            {
                CXCursor arg = clang_Cursor_getArgument(
                    _cursor,
                    i
                );

                arena::symbol_data param_data;

                param_data.kind =
                    arena::symbol_kind::parameter_decl;

                param_data.name_id = cx_cursor_spelling(
                    arg,
                    vctx->ctx->strings
                );

                param_data.type_id = cx_type_spelling(
                    arg,
                    vctx->ctx->strings
                );

                param_data.location = cx_source_location(
                    arg,
                    vctx->ctx->strings
                );

                std::uint64_t param_sid = cx_stable_id(arg);

                arena::node_id param_id =
                    vctx->ctx->tree.allocate(
                        param_sid,
                        param_data
                    );

                vctx->ctx->tree.append_child(def_id,
                                              param_id);

                vctx->ctx->xref.bind(
                    param_sid,
                    arena::domain::symbol,
                    param_id
                );
            }

            // walk the function body
            cpp_def_visitor_context body_ctx;

            body_ctx.ctx         = vctx->ctx;
            body_ctx.def_node_id = def_id;

            clang_visitChildren(
                _cursor,
                cpp_def_body_visitor,
                &body_ctx
            );

            return CXChildVisit_Continue;
        }

        // not a definition — recurse to find definitions
        // inside namespaces, classes, etc.
        return CXChildVisit_Recurse;
    }

NS_END  // internal


// ================================================================
//  cpp_def_parser
// ================================================================

/*
cpp_def_parser (constructor)
  Constructs a definition parser writing into the given context.

Parameter(s):
  _ctx:        the parse context (arena + xref + strings).
  _filename:   the virtual filename for the source buffer.
  _standard:   the C++ standard flag.
  _extra_args: additional compiler flags.
Return:
  (constructor)
*/
cpp_def_parser::cpp_def_parser
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
  Parses the source buffer and extracts all function definitions
with their internal structure.

Parameter(s):
  _state: the parse state describing the source buffer.
Return:
  A parse_result containing the root node_id on success, or a
  parse_error on failure.
*/
parse_result<cpp_def_parser::result_type>
cpp_def_parser::do_parse
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
            "fatal diagnostic during C++ parse"
        );
    }

    // allocate root node
    arena::symbol_data root_data;

    root_data.kind    = arena::symbol_kind::unknown;
    root_data.name_id = m_ctx.strings.intern(m_filename);

    std::uint64_t root_stable_id =
        internal::fnv1a_64_string(
            m_filename ? m_filename : "<unknown>"
        );

    arena::node_id root_id = m_ctx.tree.allocate(
        root_stable_id,
        root_data
    );

    m_ctx.xref.bind(
        root_stable_id,
        arena::domain::symbol,
        root_id
    );

    // walk the AST for definitions
    internal::cpp_def_top_visitor_context vctx;

    vctx.ctx     = &m_ctx;
    vctx.root_id = root_id;

    CXCursor root_cursor =
        clang_getTranslationUnitCursor(tu);

    clang_visitChildren(
        root_cursor,
        internal::cpp_def_top_visitor,
        &vctx
    );

    _state.advance(length);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return parse_result<result_type>(root_id);
}


NS_END  // parse
NS_END  // djinterp
