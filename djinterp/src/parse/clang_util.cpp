#include "../parse/clang_util.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  internal  —  hashing
// ================================================================

NS_INTERNAL

    /*
    fnv1a_64
      Computes FNV-1a 64-bit hash of a byte sequence.

    Parameter(s):
      _data:   pointer to the byte sequence.
      _length: number of bytes to hash.
    Return:
      The 64-bit FNV-1a hash value.
    */
    std::uint64_t
    fnv1a_64
    (
        const char*     _data,
        std::size_t     _length
    )
    {
        std::uint64_t hash = 14695981039346656037ULL;

        for (std::size_t i = 0; i < _length; ++i)
        {
            hash ^= static_cast<std::uint64_t>(
                static_cast<unsigned char>(_data[i])
            );

            hash *= 1099511628211ULL;
        }

        return hash;
    }

    /*
    fnv1a_64_string
      Convenience wrapper for std::string.

    Parameter(s):
      _str: the string to hash.
    Return:
      The 64-bit FNV-1a hash value.
    */
    std::uint64_t
    fnv1a_64_string
    (
        const std::string& _str
    )
    {
        return fnv1a_64(_str.data(), _str.size());
    }

    /*
    fnv1a_64_combine
      Combines two hashes.

    Parameter(s):
      _a: the first hash.
      _b: the second hash.
    Return:
      The combined hash.
    */
    std::uint64_t
    fnv1a_64_combine
    (
        std::uint64_t _a,
        std::uint64_t _b
    )
    {
        _a ^= _b;
        _a *= 1099511628211ULL;

        return _a;
    }

NS_END  // internal


// ================================================================
//  cx_to_string
// ================================================================

/*
cx_to_string
  Converts a CXString to a std::string and disposes the
CXString.

Parameter(s):
  _cx: the CXString to convert.
Return:
  The converted std::string, or "" if the CXString is null.
*/
std::string
cx_to_string
(
    CXString _cx
)
{
    const char* cstr = clang_getCString(_cx);
    std::string result = cstr ? cstr : "";

    clang_disposeString(_cx);

    return result;
}


// ================================================================
//  cx_source_location
// ================================================================

/*
cx_source_location
  Extracts an arena::source_location from a libclang cursor,
interning the filename into the string table.

Parameter(s):
  _cursor:  the libclang cursor.
  _strings: the string table for interning filenames.
Return:
  An arena::source_location describing the cursor's position.
*/
arena::source_location
cx_source_location
(
    CXCursor                _cursor,
    arena::string_table&    _strings
)
{
    CXSourceLocation loc = clang_getCursorLocation(_cursor);

    CXFile      file;
    unsigned    line;
    unsigned    column;
    unsigned    offset;

    clang_getSpellingLocation(loc,
                              &file,
                              &line,
                              &column,
                              &offset);

    // extent length
    CXSourceRange range  = clang_getCursorExtent(_cursor);
    CXSourceLocation end = clang_getRangeEnd(range);

    unsigned end_offset;

    clang_getSpellingLocation(end,
                              nullptr,
                              nullptr,
                              nullptr,
                              &end_offset);

    std::uint32_t length = (end_offset > offset)
                               ? (end_offset - offset)
                               : 0;

    // intern filename
    std::string filename = cx_to_string(clang_getFileName(file));

    arena::string_id file_id = _strings.intern(filename);

    return arena::source_location(
        file_id,
        static_cast<std::uint32_t>(line),
        static_cast<std::uint16_t>(column),
        static_cast<std::uint32_t>(offset),
        length
    );
}


// ================================================================
//  cx_stable_id
// ================================================================

/*
cx_stable_id
  Computes a stable identity hash for a libclang cursor.  Uses
the USR (Unified Symbol Resolution) when available, falling
back to a combination of spelling and file location.

Parameter(s):
  _cursor: the libclang cursor.
Return:
  A 64-bit stable identity hash.
*/
std::uint64_t
cx_stable_id
(
    CXCursor _cursor
)
{
    // prefer USR — stable across TUs
    CXString usr = clang_getCursorUSR(_cursor);
    std::string usr_str = cx_to_string(usr);

    if (!usr_str.empty())
    {
        return internal::fnv1a_64_string(usr_str);
    }

    // fallback: hash spelling + location
    std::string spelling = cx_to_string(
        clang_getCursorSpelling(_cursor)
    );

    CXSourceLocation loc = clang_getCursorLocation(_cursor);

    CXFile      file;
    unsigned    line;
    unsigned    column;
    unsigned    offset;

    clang_getSpellingLocation(loc,
                              &file,
                              &line,
                              &column,
                              &offset);

    std::string filename = cx_to_string(clang_getFileName(file));

    std::uint64_t hash = internal::fnv1a_64_string(filename);

    hash = internal::fnv1a_64_combine(
        hash,
        internal::fnv1a_64_string(spelling)
    );

    hash = internal::fnv1a_64_combine(
        hash,
        static_cast<std::uint64_t>(line)
    );

    hash = internal::fnv1a_64_combine(
        hash,
        static_cast<std::uint64_t>(column)
    );

    return hash;
}


// ================================================================
//  cx_raw_comment
// ================================================================

/*
cx_raw_comment
  Extracts the raw comment text associated with a cursor and
interns it.

Parameter(s):
  _cursor:  the libclang cursor.
  _strings: the string table for interning.
Return:
  The string_id for the comment, or null_string if none.
*/
arena::string_id
cx_raw_comment
(
    CXCursor                _cursor,
    arena::string_table&    _strings
)
{
    CXString raw = clang_Cursor_getRawCommentText(_cursor);
    std::string comment = cx_to_string(raw);

    if (comment.empty())
    {
        return arena::null_string;
    }

    return _strings.intern(comment);
}


// ================================================================
//  cx_type_spelling
// ================================================================

/*
cx_type_spelling
  Extracts the type spelling for a cursor and interns it.

Parameter(s):
  _cursor:  the libclang cursor.
  _strings: the string table for interning.
Return:
  The string_id for the type spelling, or null_string.
*/
arena::string_id
cx_type_spelling
(
    CXCursor                _cursor,
    arena::string_table&    _strings
)
{
    CXType cx_type = clang_getCursorType(_cursor);
    std::string spelling = cx_to_string(
        clang_getTypeSpelling(cx_type)
    );

    if (spelling.empty())
    {
        return arena::null_string;
    }

    return _strings.intern(spelling);
}


// ================================================================
//  cx_cursor_spelling
// ================================================================

/*
cx_cursor_spelling
  Extracts the cursor spelling (name) and interns it.

Parameter(s):
  _cursor:  the libclang cursor.
  _strings: the string table for interning.
Return:
  The string_id for the name, or null_string.
*/
arena::string_id
cx_cursor_spelling
(
    CXCursor                _cursor,
    arena::string_table&    _strings
)
{
    std::string spelling = cx_to_string(
        clang_getCursorSpelling(_cursor)
    );

    if (spelling.empty())
    {
        return arena::null_string;
    }

    return _strings.intern(spelling);
}


// ================================================================
//  cx_function_signature
// ================================================================

/*
cx_function_signature
  Builds a human-readable function signature string from a
function or method cursor and interns it.

Parameter(s):
  _cursor:  the function/method cursor.
  _strings: the string table for interning.
Return:
  The string_id for the full signature.
*/
arena::string_id
cx_function_signature
(
    CXCursor                _cursor,
    arena::string_table&    _strings
)
{
    CXType func_type = clang_getCursorType(_cursor);
    CXType ret_type  = clang_getResultType(func_type);

    std::string sig = cx_to_string(
        clang_getTypeSpelling(ret_type)
    );

    sig += " ";
    sig += cx_to_string(clang_getCursorSpelling(_cursor));
    sig += "(";

    int num_args = clang_Cursor_getNumArguments(_cursor);

    for (int i = 0; i < num_args; ++i)
    {
        if (i > 0)
        {
            sig += ", ";
        }

        CXCursor arg = clang_Cursor_getArgument(_cursor, i);
        CXType arg_type = clang_getCursorType(arg);

        sig += cx_to_string(clang_getTypeSpelling(arg_type));

        std::string arg_name = cx_to_string(
            clang_getCursorSpelling(arg)
        );

        if (!arg_name.empty())
        {
            sig += " ";
            sig += arg_name;
        }
    }

    sig += ")";

    return _strings.intern(sig);
}


// ================================================================
//  cx_storage_class
// ================================================================

/*
cx_storage_class
  Extracts the storage class of a cursor as a uint8_t.

Parameter(s):
  _cursor: the libclang cursor.
Return:
  1 for static, 2 for extern, 0 otherwise.
*/
std::uint8_t
cx_storage_class
(
    CXCursor _cursor
)
{
    CX_StorageClass sc = clang_Cursor_getStorageClass(_cursor);

    switch (sc)
    {
        case CX_SC_Static:  return 1;
        case CX_SC_Extern:  return 2;

        default:            return 0;
    }
}


// ================================================================
//  cx_kind_to_symbol_kind_c
// ================================================================

/*
cx_kind_to_symbol_kind_c
  Maps a CXCursorKind to the corresponding arena::symbol_kind
value for the subset of kinds shared by C and C++.

Parameter(s):
  _kind: the libclang cursor kind.
Return:
  The corresponding symbol_kind, or symbol_kind::unknown.
*/
std::uint16_t
cx_kind_to_symbol_kind_c
(
    CXCursorKind _kind
)
{
    switch (_kind)
    {
        case CXCursor_StructDecl:
            return arena::symbol_kind::struct_decl;

        case CXCursor_EnumDecl:
            return arena::symbol_kind::enum_decl;

        case CXCursor_EnumConstantDecl:
            return arena::symbol_kind::enum_constant;

        case CXCursor_FunctionDecl:
            return arena::symbol_kind::function_decl;

        case CXCursor_FieldDecl:
            return arena::symbol_kind::field_decl;

        case CXCursor_VarDecl:
            return arena::symbol_kind::variable_decl;

        case CXCursor_ParmDecl:
            return arena::symbol_kind::parameter_decl;

        case CXCursor_TypedefDecl:
            return arena::symbol_kind::typedef_decl;

        case CXCursor_MacroDefinition:
            return arena::symbol_kind::macro_def;

        case CXCursor_InclusionDirective:
            return arena::symbol_kind::include_directive;

        default:
            return arena::symbol_kind::unknown;
    }
}


// ================================================================
//  cx_kind_to_symbol_kind_cpp
// ================================================================

/*
cx_kind_to_symbol_kind_cpp
  Extends the C mapping with C++-specific cursor kinds.  Falls
through to cx_kind_to_symbol_kind_c for shared kinds.

Parameter(s):
  _kind: the libclang cursor kind.
Return:
  The corresponding symbol_kind, or symbol_kind::unknown.
*/
std::uint16_t
cx_kind_to_symbol_kind_cpp
(
    CXCursorKind _kind
)
{
    switch (_kind)
    {
        case CXCursor_Namespace:
            return arena::symbol_kind::namespace_decl;

        case CXCursor_ClassDecl:
            return arena::symbol_kind::class_decl;

        case CXCursor_ClassTemplate:
        case CXCursor_ClassTemplatePartialSpecialization:
            return arena::symbol_kind::class_decl;

        case CXCursor_CXXMethod:
            return arena::symbol_kind::method_decl;

        case CXCursor_Constructor:
            return arena::symbol_kind::constructor_decl;

        case CXCursor_Destructor:
            return arena::symbol_kind::destructor_decl;

        case CXCursor_FunctionTemplate:
            return arena::symbol_kind::function_decl;

        case CXCursor_TypeAliasDecl:
        case CXCursor_TypeAliasTemplateDecl:
            return arena::symbol_kind::type_alias_decl;

        case CXCursor_TemplateTypeParameter:
        case CXCursor_NonTypeTemplateParameter:
        case CXCursor_TemplateTemplateParameter:
            return arena::symbol_kind::template_decl;

        default:
            return cx_kind_to_symbol_kind_c(_kind);
    }
}


// ================================================================
//  cx_access_specifier
// ================================================================

/*
cx_access_specifier
  Maps a libclang access specifier to the arena enum.

Parameter(s):
  _cursor: the libclang cursor.
Return:
  The corresponding access_specifier value.
*/
std::uint8_t
cx_access_specifier
(
    CXCursor _cursor
)
{
    CX_CXXAccessSpecifier access = clang_getCXXAccessSpecifier(
        _cursor
    );

    switch (access)
    {
        case CX_CXXPublic:
            return arena::access_specifier::public_;

        case CX_CXXProtected:
            return arena::access_specifier::protected_;

        case CX_CXXPrivate:
            return arena::access_specifier::private_;

        default:
            return arena::access_specifier::unspecified;
    }
}


// ================================================================
//  clang_visit_cursor
// ================================================================

/*
clang_visit_cursor
  The clang_visitChildren callback.  For each cursor that passes
the filter: allocates a node, populates common fields, calls the
enrich callback, links under the current parent, registers in the
cross-ref, and recurses into children.

Parameter(s):
  _cursor: the current cursor.
  _parent: the parent cursor (libclang-internal).
  _data:   the clang_visitor_context.
Return:
  CXChildVisit_Continue always (recursion is handled manually).
*/
CXChildVisitResult
clang_visit_cursor
(
    CXCursor        _cursor,
    CXCursor        _parent,
    CXClientData    _data
)
{
    auto* vctx = static_cast<clang_visitor_context*>(_data);

    // skip system headers
    CXSourceLocation loc = clang_getCursorLocation(_cursor);

    if (clang_Location_isInSystemHeader(loc))
    {
        return CXChildVisit_Continue;
    }

    CXCursorKind kind = clang_getCursorKind(_cursor);

    // apply language-specific filter
    if ( (vctx->filter) &&
         (!vctx->filter(kind)) )
    {
        return CXChildVisit_Recurse;
    }

    // populate common fields
    arena::symbol_data sym;

    sym.location     = cx_source_location(_cursor,
                                          vctx->ctx->strings);

    sym.name_id      = cx_cursor_spelling(_cursor,
                                          vctx->ctx->strings);

    sym.type_id      = cx_type_spelling(_cursor,
                                        vctx->ctx->strings);

    sym.comment_id   = cx_raw_comment(_cursor,
                                      vctx->ctx->strings);

    sym.storage      = cx_storage_class(_cursor);

    // build signature for function-like cursors
    if ( (kind == CXCursor_FunctionDecl)     ||
         (kind == CXCursor_CXXMethod)        ||
         (kind == CXCursor_Constructor)      ||
         (kind == CXCursor_Destructor)       ||
         (kind == CXCursor_FunctionTemplate) )
    {
        sym.signature_id = cx_function_signature(
            _cursor,
            vctx->ctx->strings
        );
    }

    // enrich with language-specific fields
    if (vctx->enrich)
    {
        vctx->enrich(_cursor, sym, vctx->ctx->strings);
    }

    // compute stable identity
    std::uint64_t stable_id = cx_stable_id(_cursor);

    // allocate and link
    arena::node_id node_id = vctx->ctx->tree.allocate(
        stable_id,
        sym
    );

    if (vctx->parent_id != arena::null_node)
    {
        vctx->ctx->tree.append_child(vctx->parent_id,
                                     node_id);
    }

    // register in cross-ref
    vctx->ctx->xref.bind(stable_id,
                          arena::domain::symbol,
                          node_id);

    // recurse with this node as parent
    clang_visitor_context child_ctx;

    child_ctx.ctx       = vctx->ctx;
    child_ctx.parent_id = node_id;
    child_ctx.filter    = vctx->filter;
    child_ctx.enrich    = vctx->enrich;

    clang_visitChildren(_cursor,
                        clang_visit_cursor,
                        &child_ctx);

    return CXChildVisit_Continue;
}


// ================================================================
//  clang_parse_tu
// ================================================================

/*
clang_parse_tu
  Creates a CXIndex, parses a translation unit from the given
source text with the given compiler arguments, and returns the
(index, tu) pair through output parameters.

Parameter(s):
  _source_data:   pointer to the in-memory source buffer.
  _source_length: length of the source buffer in bytes.
  _filename:      virtual filename assigned to the buffer.
  _args:          array of compiler argument strings.
  _num_args:      number of compiler arguments.
  _options:       CXTranslationUnit_* option flags.
  _out_index:     receives the created CXIndex.
  _out_tu:        receives the created CXTranslationUnit.
Return:
  true on success, false on failure.
*/
bool
clang_parse_tu
(
    const char*             _source_data,
    std::size_t             _source_length,
    const char*             _filename,
    const char* const*      _args,
    int                     _num_args,
    unsigned                _options,
    CXIndex&                _out_index,
    CXTranslationUnit&      _out_tu
)
{
    _out_index = clang_createIndex(
        /* excludeDeclarationsFromPCH */ 0,
        /* displayDiagnostics         */ 0
    );

    if (!_out_index)
    {
        return false;
    }

    // set up unsaved file for in-memory parsing
    CXUnsavedFile unsaved;

    unsaved.Filename = _filename;
    unsaved.Contents = _source_data;
    unsaved.Length   = static_cast<unsigned long>(_source_length);

    _out_tu = clang_parseTranslationUnit(
        _out_index,
        _filename,
        _args,
        _num_args,
        &unsaved,
        1,
        _options
    );

    if (!_out_tu)
    {
        clang_disposeIndex(_out_index);
        _out_index = nullptr;

        return false;
    }

    return true;
}


// ================================================================
//  clang_walk_tu
// ================================================================

/*
clang_walk_tu
  Walks the translation unit's AST from the root cursor,
populating the parse_context's arena and cross-ref using the
provided filter and enrich callbacks.

Parameter(s):
  _tu:       the parsed translation unit.
  _ctx:      the parse context (arena + xref + strings).
  _filename: filename for the root node's name.
  _filter:   cursor kind filter callback.
  _enrich:   language-specific enrichment callback.
Return:
  The node_id allocated for the translation unit root.
*/
arena::node_id
clang_walk_tu
(
    CXTranslationUnit   _tu,
    parse_context&      _ctx,
    const char*         _filename,
    cursor_filter_fn    _filter,
    cursor_enrich_fn    _enrich
)
{
    // allocate a root node for the translation unit
    arena::symbol_data root_data;

    root_data.kind    = arena::symbol_kind::unknown;
    root_data.name_id = _ctx.strings.intern(
        _filename ? _filename : "<unknown>"
    );

    std::uint64_t root_stable_id = internal::fnv1a_64_string(
        _filename ? _filename : "<unknown>"
    );

    arena::node_id root_id = _ctx.tree.allocate(
        root_stable_id,
        root_data
    );

    _ctx.xref.bind(root_stable_id,
                    arena::domain::symbol,
                    root_id);

    // set up the visitor context
    clang_visitor_context vctx;

    vctx.ctx       = &_ctx;
    vctx.parent_id = root_id;
    vctx.filter    = _filter;
    vctx.enrich    = _enrich;

    // walk
    CXCursor root_cursor = clang_getTranslationUnitCursor(_tu);

    clang_visitChildren(root_cursor,
                        clang_visit_cursor,
                        &vctx);

    return root_id;
}


NS_END  // parse
NS_END  // djinterp
