#include "scanner.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

namespace fs = std::filesystem;

namespace catalogue 
{

// ============================================================
// construction / destruction
// ============================================================

/*
framework_scanner::framework_scanner
  Default-constructs the scanner and initializes the clang index.

Parameter(s):
  (none)
Return:
  (constructor)
*/
framework_scanner::framework_scanner()
    : m_index(nullptr)
    , m_config()
    , m_callbacks()
    , m_symbols()
    , m_modules()
    , m_scanned_files()
{
    m_init_index();
}

/*
framework_scanner::framework_scanner
  Constructs the scanner with the given configuration.

Parameter(s):
  _config: scanner configuration to use.
Return:
  (constructor)
*/
framework_scanner::framework_scanner
(
    const d_scanner_config& _config
)
    : m_index(nullptr)
    , m_config(_config)
    , m_callbacks()
    , m_symbols()
    , m_modules()
    , m_scanned_files()
{
    m_init_index();
}

/*
framework_scanner::~framework_scanner
  Destroys the scanner and releases the clang index.

Parameter(s):
  (none)
Return:
  (destructor)
*/
framework_scanner::~framework_scanner()
{
    m_destroy_index();
}

// ============================================================
// configuration
// ============================================================

/*
framework_scanner::add_include_path
  Appends an include path to the scanner configuration.

Parameter(s):
  _path: the include directory path to add.
Return:
  none.
*/
void
framework_scanner::add_include_path
(
    const std::string& _path
)
{
    m_config.include_paths.push_back(_path);

    return;
}

/*
framework_scanner::add_compile_flag
  Appends a compile flag to the scanner configuration.

Parameter(s):
  _flag: the flag to add (e.g. "-std=c++20").
Return:
  none.
*/
void
framework_scanner::add_compile_flag
(
    const std::string& _flag
)
{
    m_config.compile_flags.push_back(_flag);

    return;
}

/*
framework_scanner::set_config
  Replaces the entire scanner configuration.

Parameter(s):
  _config: the new configuration.
Return:
  none.
*/
void
framework_scanner::set_config
(
    const d_scanner_config& _config
)
{
    m_config = _config;

    return;
}

/*
framework_scanner::set_callbacks
  Installs progress/filter callbacks.

Parameter(s):
  _callbacks: the callback set to install.
Return:
  none.
*/
void
framework_scanner::set_callbacks
(
    const scanner_callbacks& _callbacks
)
{
    m_callbacks = _callbacks;

    return;
}

/*
framework_scanner::config
  Returns a const reference to the current configuration.

Parameter(s):
  (none)
Return:
  the current d_scanner_config.
*/
const d_scanner_config&
framework_scanner::config() const
{
    return m_config;
}

// ============================================================
// scanning
// ============================================================

/*
framework_scanner::scan_directory
  Recursively discovers all matching source files in _dir and
  parses each one, extracting symbols into the catalogue.

Parameter(s):
  _dir: root directory to scan.
Return:
  the total number of new symbols discovered across all files.
*/
size_t
framework_scanner::scan_directory
(
    const std::string& _dir
)
{
    std::vector<std::string> files;
    size_t                   total_new;

    // validate the directory
    if (!fs::is_directory(_dir))
    {
        if (m_callbacks.on_diagnostic)
        {
            m_callbacks.on_diagnostic(_dir,
                                     "not a valid directory",
                                     3);
        }

        return 0;
    }

    // set framework root if not already set
    if (m_config.framework_root.empty())
    {
        m_config.framework_root = fs::canonical(_dir).string();
    }

    // collect all source files recursively
    files     = m_collect_files(_dir,
                               m_config.file_extensions,
                               m_config.follow_symlinks);
    total_new = 0;

    // parse each file
    for (const auto& file : files)
    {
        total_new += scan_file(file);
    }

    return total_new;
}

/*
framework_scanner::scan_file
  Parses a single translation unit and extracts all symbols.

Parameter(s):
  _file: path to the source file to parse.
Return:
  the number of new symbols discovered in this file.
*/
size_t
framework_scanner::scan_file
(
    const std::string& _file
)
{
    std::string canonical;
    size_t      count_before;
    size_t      count_after;
    size_t      new_symbols;

    // resolve the canonical path
    if (!fs::exists(_file))
    {
        if (m_callbacks.on_diagnostic)
        {
            m_callbacks.on_diagnostic(_file,
                                     "file does not exist",
                                     3);
        }

        return 0;
    }

    canonical = fs::canonical(_file).string();

    // skip already-scanned files
    if (m_scanned_files.count(canonical) > 0)
    {
        return 0;
    }

    // notify callback
    if (m_callbacks.on_file_begin)
    {
        // allow the callback to skip this file
        if (!m_callbacks.on_file_begin(canonical))
        {
            return 0;
        }
    }

    // register the module for this file
    m_register_module(canonical);

    // parse and extract
    count_before = m_symbols.size();
    m_parse_translation_unit(canonical);
    count_after  = m_symbols.size();
    new_symbols  = count_after - count_before;

    m_scanned_files.insert(canonical);

    // notify completion
    if (m_callbacks.on_file_complete)
    {
        m_callbacks.on_file_complete(canonical, new_symbols);
    }

    if (m_config.verbose)
    {
        std::cout << "[scanner] " << canonical
                  << " => " << new_symbols
                  << " symbols\n";
    }

    return new_symbols;
}

/*
framework_scanner::reset
  Clears all discovered symbols, modules, and scanned files.

Parameter(s):
  (none)
Return:
  none.
*/
void
framework_scanner::reset()
{
    m_symbols.clear();
    m_modules.clear();
    m_scanned_files.clear();

    return;
}

// ============================================================
// read-only access
// ============================================================

/*
framework_scanner::symbols
  Returns a const reference to the full symbol map (keyed by USR).

Parameter(s):
  (none)
Return:
  the symbol map.
*/
const framework_scanner::symbol_map&
framework_scanner::symbols() const
{
    return m_symbols;
}

/*
framework_scanner::modules
  Returns a const reference to the full module map.

Parameter(s):
  (none)
Return:
  the module map.
*/
const framework_scanner::module_map&
framework_scanner::modules() const
{
    return m_modules;
}

/*
framework_scanner::symbols_in_module
  Returns pointers to all symbols belonging to _module.

Parameter(s):
  _module: the module name to filter by.
Return:
  a vector of const pointers to matching symbols.
*/
std::vector<const d_symbol_info*>
framework_scanner::symbols_in_module
(
    const std::string& _module
)
const
{
    std::vector<const d_symbol_info*> result;

    // iterate all symbols and collect matches
    for (const auto& [usr, sym] : m_symbols)
    {
        if (sym.module_name == _module)
        {
            result.push_back(&sym);
        }
    }

    return result;
}

/*
framework_scanner::symbols_by_category
  Returns pointers to all symbols of the given category.

Parameter(s):
  _category: the category to filter by.
Return:
  a vector of const pointers to matching symbols.
*/
std::vector<const d_symbol_info*>
framework_scanner::symbols_by_category
(
    DSymbolCategory _category
)
const
{
    std::vector<const d_symbol_info*> result;

    for (const auto& [usr, sym] : m_symbols)
    {
        if (sym.category == _category)
        {
            result.push_back(&sym);
        }
    }

    return result;
}

/*
framework_scanner::find_by_usr
  Finds a single symbol by its Unified Symbol Resolution string.

Parameter(s):
  _usr: the USR to search for.
Return:
  a const pointer to the symbol, or nullptr if not found.
*/
const d_symbol_info*
framework_scanner::find_by_usr
(
    const std::string& _usr
)
const
{
    auto it = m_symbols.find(_usr);

    if (it == m_symbols.end())
    {
        return nullptr;
    }

    return &(it->second);
}

/*
framework_scanner::find_by_name
  Finds the first symbol with the given unqualified name.

Parameter(s):
  _name: the unqualified name to search for.
Return:
  a const pointer to the first match, or nullptr.
*/
const d_symbol_info*
framework_scanner::find_by_name
(
    const std::string& _name
)
const
{
    for (const auto& [usr, sym] : m_symbols)
    {
        if (sym.name == _name)
        {
            return &sym;
        }
    }

    return nullptr;
}

// ============================================================
// dependency analysis
// ============================================================

/*
framework_scanner::dependency_edges
  Returns all direct dependency edges as (from_usr, to_usr) pairs.

Parameter(s):
  (none)
Return:
  a vector of (source_usr, target_usr) pairs.
*/
std::vector<std::pair<std::string, std::string>>
framework_scanner::dependency_edges() const
{
    std::vector<std::pair<std::string, std::string>> edges;

    for (const auto& [usr, sym] : m_symbols)
    {
        // add an edge for each referenced USR
        for (const auto& ref : sym.referenced_usrs)
        {
            edges.emplace_back(usr, ref);
        }
    }

    return edges;
}

/*
framework_scanner::transitive_deps
  Computes all symbols transitively reachable from _usr via
  dependency edges (breadth-first).

Parameter(s):
  _usr: the starting symbol's USR.
Return:
  the set of all transitively-referenced USRs.
*/
std::set<std::string>
framework_scanner::transitive_deps
(
    const std::string& _usr
)
const
{
    std::set<std::string>    visited;
    std::vector<std::string> queue;

    // seed the BFS with _usr
    queue.push_back(_usr);

    // BFS over dependency edges
    while (!queue.empty())
    {
        std::string current = queue.back();
        queue.pop_back();

        // skip if already visited
        if (visited.count(current) > 0)
        {
            continue;
        }

        visited.insert(current);

        auto it = m_symbols.find(current);

        // enqueue all direct references
        if (it != m_symbols.end())
        {
            for (const auto& ref : it->second.referenced_usrs)
            {
                if (visited.count(ref) == 0)
                {
                    queue.push_back(ref);
                }
            }
        }
    }

    // remove the starting node itself
    visited.erase(_usr);

    return visited;
}

// ============================================================
// statistics
// ============================================================

/*
framework_scanner::total_symbols
  Returns the total number of catalogued symbols.

Parameter(s):
  (none)
Return:
  the count.
*/
size_t
framework_scanner::total_symbols() const
{
    return m_symbols.size();
}

/*
framework_scanner::total_modules
  Returns the number of discovered modules.

Parameter(s):
  (none)
Return:
  the count.
*/
size_t
framework_scanner::total_modules() const
{
    return m_modules.size();
}

/*
framework_scanner::total_files_scanned
  Returns the number of unique files parsed.

Parameter(s):
  (none)
Return:
  the count.
*/
size_t
framework_scanner::total_files_scanned() const
{
    return m_scanned_files.size();
}

/*
framework_scanner::category_histogram
  Returns the count of symbols per category.

Parameter(s):
  (none)
Return:
  a map from DSymbolCategory to count.
*/
std::map<DSymbolCategory, size_t>
framework_scanner::category_histogram() const
{
    std::map<DSymbolCategory, size_t> hist;

    for (const auto& [usr, sym] : m_symbols)
    {
        hist[sym.category]++;
    }

    return hist;
}

// ============================================================
// clang index lifecycle
// ============================================================

/*
framework_scanner::m_init_index
  Creates the CXIndex used for all translation unit parsing.

Parameter(s):
  (none)
Return:
  none.
*/
void
framework_scanner::m_init_index()
{
    // excludeDeclarationsFromPCH=0, displayDiagnostics=0
    m_index = clang_createIndex(0, 0);

    return;
}

/*
framework_scanner::m_destroy_index
  Releases the CXIndex.

Parameter(s):
  (none)
Return:
  none.
*/
void
framework_scanner::m_destroy_index()
{
    if (m_index)
    {
        clang_disposeIndex(m_index);
        m_index = nullptr;
    }

    return;
}

// ============================================================
// file discovery
// ============================================================

/*
framework_scanner::m_collect_files
  Recursively enumerates all files under _dir whose extension
  matches one in _exts.

Parameter(s):
  _dir:    the root directory to search.
  _exts:   list of acceptable file extensions (e.g. ".hpp").
  _follow: if true, follow symbolic links.
Return:
  a sorted vector of canonical file paths.
*/
std::vector<std::string>
framework_scanner::m_collect_files
(
    const std::string&              _dir,
    const std::vector<std::string>& _exts,
    bool                            _follow
)
{
    std::vector<std::string> result;
    fs::directory_options    opts;

    opts = _follow
        ? fs::directory_options::follow_directory_symlink
        : fs::directory_options::none;

    // walk the directory tree
    for (const auto& entry :
         fs::recursive_directory_iterator(_dir, opts))
    {
        // skip non-regular files
        if (!entry.is_regular_file())
        {
            continue;
        }

        std::string ext = entry.path().extension().string();

        // check if this extension is in the accepted list
        auto it = std::find(_exts.begin(), _exts.end(), ext);

        if (it != _exts.end())
        {
            result.push_back(
                fs::canonical(entry.path()).string());
        }
    }

    std::sort(result.begin(), result.end());

    return result;
}

// ============================================================
// parsing
// ============================================================

/*
framework_scanner::m_parse_translation_unit
  Parses a single file into a clang translation unit and walks
  its AST to extract all symbols.

Parameter(s):
  _file: the canonical path of the file to parse.
Return:
  the number of symbols extracted from this TU.
*/
size_t
framework_scanner::m_parse_translation_unit
(
    const std::string& _file
)
{
    CXTranslationUnit       tu;
    std::vector<std::string> all_flags;
    std::vector<const char*> c_flags;
    size_t                   count_before;
    m_visit_context          ctx;
    unsigned                 parse_flags;

    count_before = m_symbols.size();
    tu           = nullptr;

    // build the full flag list: user flags + include paths
    all_flags = m_config.compile_flags;

    // add include paths as -I flags
    for (const auto& inc : m_config.include_paths)
    {
        all_flags.push_back("-I" + inc);
    }

    // ensure we get detailed preprocessing info
    all_flags.push_back("-Wno-everything");

    // convert to C string array for libclang
    for (const auto& f : all_flags)
    {
        c_flags.push_back(f.c_str());
    }

    // configure parse flags
    parse_flags =
        CXTranslationUnit_DetailedPreprocessingRecord |
        CXTranslationUnit_SkipFunctionBodies          |
        CXTranslationUnit_KeepGoing;

    // parse the translation unit
    tu = clang_parseTranslationUnit(m_index,
                                    _file.c_str(),
                                    c_flags.data(),
                                    static_cast<int>(c_flags.size()),
                                    nullptr,
                                    0,
                                    parse_flags);

    // check for parse failure
    if (!tu)
    {
        if (m_callbacks.on_diagnostic)
        {
            m_callbacks.on_diagnostic(
                _file,
                "clang_parseTranslationUnit returned null",
                4);
        }

        return 0;
    }

    // report any diagnostics
    m_report_diagnostics(tu, _file);

    // walk the AST
    ctx.scanner = this;
    ctx.file    = _file;

    clang_visitChildren(clang_getTranslationUnitCursor(tu),
                        m_visit_cursor_callback,
                        &ctx);

    clang_disposeTranslationUnit(tu);

    return m_symbols.size() - count_before;
}

/*
framework_scanner::m_report_diagnostics
  Iterates all diagnostics from a parsed TU and forwards them
  to the on_diagnostic callback.

Parameter(s):
  _tu:   the parsed translation unit.
  _file: the file that was parsed (for reporting).
Return:
  none.
*/
void
framework_scanner::m_report_diagnostics
(
    CXTranslationUnit  _tu,
    const std::string& _file
)
{
    unsigned num_diag;
    unsigned i;

    // skip if no callback installed
    if (!m_callbacks.on_diagnostic)
    {
        return;
    }

    num_diag = clang_getNumDiagnostics(_tu);

    // forward each diagnostic
    for (i = 0; i < num_diag; i++)
    {
        CXDiagnostic diag     = clang_getDiagnostic(_tu, i);
        int          severity = clang_getDiagnosticSeverity(diag);
        CXString     msg      = clang_getDiagnosticSpelling(diag);

        // only report warnings and errors
        if (severity >= CXDiagnostic_Warning)
        {
            m_callbacks.on_diagnostic(
                _file,
                m_cx_to_std(msg),
                severity);
        }
        else
        {
            clang_disposeString(msg);
        }

        clang_disposeDiagnostic(diag);
    }

    return;
}

// ============================================================
// AST visitor
// ============================================================

/*
framework_scanner::m_visit_cursor_callback
  Static callback passed to clang_visitChildren. Unpacks the
  context and delegates to m_process_cursor.

Parameter(s):
  _cursor: the current AST node.
  _parent: the parent AST node.
  _ctx:    opaque pointer to m_visit_context.
Return:
  CXChildVisit_Recurse to continue walking, or
  CXChildVisit_Continue to skip children.
*/
CXChildVisitResult
framework_scanner::m_visit_cursor_callback
(
    CXCursor     _cursor,
    CXCursor     _parent,
    CXClientData _ctx
)
{
    m_visit_context* context =
        static_cast<m_visit_context*>(_ctx);

    context->scanner->m_process_cursor(_cursor,
                                       _parent,
                                       context->file);

    return CXChildVisit_Recurse;
}

/*
framework_scanner::m_process_cursor
  Examines a single AST cursor, extracts a symbol from it if
  it's a catalogueable entity, and stores it.

Parameter(s):
  _cursor: the AST node to examine.
  _parent: its parent node.
  _file:   the file being parsed.
Return:
  none.
*/
void
framework_scanner::m_process_cursor
(
    CXCursor           _cursor,
    CXCursor           _parent,
    const std::string& _file
)
{
    DSymbolCategory category;
    std::string     usr;
    d_symbol_info   sym;

    // skip system headers unless configured
    if ( (!m_config.scan_system_headers) &&
         (m_is_in_system_header(_cursor)) )
    {
        return;
    }

    // classify this cursor
    category = m_classify_cursor(_cursor);

    // skip unknown/uninteresting nodes
    if (category == DSymbolCategoryUnknown)
    {
        return;
    }

    // skip macros if not configured
    if ( (!m_config.extract_macros) &&
         ( (category == DSymbolCategoryMacroDefinition) ||
           (category == DSymbolCategoryMacroExpansion) ) )
    {
        return;
    }

    // extract the USR (unique identifier)
    usr = m_extract_usr(_cursor);

    // skip anonymous/unnamed entities
    if (usr.empty())
    {
        return;
    }

    // skip duplicates
    if (m_symbols.count(usr) > 0)
    {
        // if this is a definition and we only had a
        // declaration, update to the definition
        if ( (clang_isCursorDefinition(_cursor)) &&
             (!m_symbols[usr].is_definition) )
        {
            sym                = m_extract_symbol(_cursor,
                                                  _parent);
            sym.module_name    = m_resolve_module(_file);
            sym.is_definition  = true;
            m_symbols[usr]     = sym;
        }

        return;
    }

    // extract the full symbol info
    sym             = m_extract_symbol(_cursor, _parent);
    sym.module_name = m_resolve_module(_file);

    // allow the callback to filter
    if (m_callbacks.on_symbol_discovered)
    {
        if (!m_callbacks.on_symbol_discovered(sym))
        {
            return;
        }
    }

    // store the symbol
    m_symbols[usr] = sym;

    // register it in the module
    auto mod_it = m_modules.find(sym.module_name);

    if (mod_it != m_modules.end())
    {
        mod_it->second.symbol_usrs.push_back(usr);
        mod_it->second.category_counts[category]++;
    }

    return;
}

// ============================================================
// symbol extraction
// ============================================================

/*
framework_scanner::m_extract_symbol
  Builds a complete d_symbol_info from the given cursor.

Parameter(s):
  _cursor: the AST node to extract from.
  _parent: the parent AST node.
Return:
  a fully populated d_symbol_info.
*/
d_symbol_info
framework_scanner::m_extract_symbol
(
    CXCursor _cursor,
    CXCursor _parent
)
const
{
    d_symbol_info   sym;
    CXType          cursor_type;
    CXType          result_type;
    CXSourceLocation loc;
    CXFile          file;
    unsigned        line;
    unsigned        column;
    unsigned        offset;

    // identity
    sym.name           = m_cx_to_std(
                             clang_getCursorSpelling(_cursor));
    sym.qualified_name = m_extract_qualified_name(_cursor);
    sym.usr            = m_extract_usr(_cursor);
    sym.category       = m_classify_cursor(_cursor);
    sym.access         = m_extract_access(_cursor);
    sym.linkage        = m_extract_linkage(_cursor);

    // definition location
    loc = clang_getCursorLocation(_cursor);
    clang_getSpellingLocation(loc,
                              &file,
                              &line,
                              &column,
                              &offset);

    if (file)
    {
        sym.definition_loc.file   =
            m_cx_to_std(clang_getFileName(file));
        sym.definition_loc.line   = line;
        sym.definition_loc.column = column;
    }

    // type info
    cursor_type = clang_getCursorType(_cursor);
    sym.type_spelling =
        m_cx_to_std(clang_getTypeSpelling(cursor_type));

    result_type = clang_getCursorResultType(_cursor);

    if (result_type.kind != CXType_Invalid)
    {
        sym.return_type =
            m_cx_to_std(clang_getTypeSpelling(result_type));
    }

    // qualifiers
    sym.is_definition  = clang_isCursorDefinition(_cursor);
    sym.is_const       =
        clang_isConstQualifiedType(cursor_type);
    sym.is_static      =
        (clang_Cursor_getStorageClass(_cursor) ==
         CX_SC_Static);
    sym.is_virtual     =
        clang_CXXMethod_isVirtual(_cursor);
    sym.is_pure_virtual =
        clang_CXXMethod_isPureVirtual(_cursor);
    sym.is_inline      =
        clang_Cursor_isFunctionInlined(_cursor);
    sym.is_variadic    =
        clang_Cursor_isVariadic(_cursor);

    // template check
    sym.is_template =
        (clang_getSpecializedCursorTemplate(_cursor).kind !=
         CXCursor_InvalidFile);

    // parent info
    CXCursor semantic_parent =
        clang_getCursorSemanticParent(_cursor);

    if ( (semantic_parent.kind != CXCursor_TranslationUnit) &&
         (semantic_parent.kind != CXCursor_InvalidFile) )
    {
        sym.parent_name =
            m_cx_to_std(
                clang_getCursorSpelling(semantic_parent));
        sym.parent_usr = m_extract_usr(semantic_parent);
    }

    // extract subordinate information
    sym.comment             = m_extract_comment(_cursor);
    sym.parameters          = m_extract_parameters(_cursor);
    sym.template_parameters = m_extract_template_params(_cursor);
    sym.base_classes        = m_extract_base_classes(_cursor);

    // extract dependency references
    m_extract_references(_cursor, sym);

    return sym;
}

/*
framework_scanner::m_classify_cursor
  Maps a CXCursorKind to our DSymbolCategory taxonomy.

Parameter(s):
  _cursor: the cursor to classify.
Return:
  the category, or DSymbolCategoryUnknown for uninteresting nodes.
*/
DSymbolCategory
framework_scanner::m_classify_cursor
(
    CXCursor _cursor
)
const
{
    CXCursorKind kind = clang_getCursorKind(_cursor);

    switch (kind)
    {
        case CXCursor_FunctionDecl:
            return DSymbolCategoryFunction;

        case CXCursor_CXXMethod:
            return DSymbolCategoryMethod;

        case CXCursor_Constructor:
            return DSymbolCategoryConstructor;

        case CXCursor_Destructor:
            return DSymbolCategoryDestructor;

        case CXCursor_StructDecl:
            return DSymbolCategoryStruct;

        case CXCursor_ClassDecl:
            return DSymbolCategoryClass;

        case CXCursor_EnumDecl:
            return DSymbolCategoryEnum;

        case CXCursor_EnumConstantDecl:
            return DSymbolCategoryEnumConstant;

        case CXCursor_TypedefDecl:
            return DSymbolCategoryTypedef;

        case CXCursor_TypeAliasDecl:
            return DSymbolCategoryTypeAlias;

        case CXCursor_VarDecl:
            return DSymbolCategoryVariable;

        case CXCursor_FieldDecl:
            return DSymbolCategoryField;

        case CXCursor_MacroDefinition:
            return DSymbolCategoryMacroDefinition;

        case CXCursor_MacroExpansion:
            return DSymbolCategoryMacroExpansion;

        case CXCursor_Namespace:
            return DSymbolCategoryNamespace;

        case CXCursor_TemplateTypeParameter:
        case CXCursor_NonTypeTemplateParameter:
        case CXCursor_TemplateTemplateParameter:
            return DSymbolCategoryTemplateParam;

        case CXCursor_UnionDecl:
            return DSymbolCategoryUnion;

        case CXCursor_UsingDeclaration:
            return DSymbolCategoryUsingDecl;

        case CXCursor_FunctionTemplate:
            return DSymbolCategoryFunctionTemplate;

        case CXCursor_ClassTemplate:
            return DSymbolCategoryClassTemplate;

        default:
            break;
    }

    return DSymbolCategoryUnknown;
}

/*
framework_scanner::m_extract_access
  Extracts the C++ access specifier for a cursor.

Parameter(s):
  _cursor: the cursor to query.
Return:
  the access specifier.
*/
DAccessSpecifier
framework_scanner::m_extract_access
(
    CXCursor _cursor
)
const
{
    CX_CXXAccessSpecifier access =
        clang_getCXXAccessSpecifier(_cursor);

    switch (access)
    {
        case CX_CXXPublic:    return DAccessSpecifierPublic;
        case CX_CXXProtected: return DAccessSpecifierProtected;
        case CX_CXXPrivate:   return DAccessSpecifierPrivate;
        default:               break;
    }

    return DAccessSpecifierNone;
}

/*
framework_scanner::m_extract_linkage
  Extracts the linkage kind for a cursor.

Parameter(s):
  _cursor: the cursor to query.
Return:
  the linkage kind.
*/
DLinkageKind
framework_scanner::m_extract_linkage
(
    CXCursor _cursor
)
const
{
    CXLinkageKind lk = clang_getCursorLinkage(_cursor);

    switch (lk)
    {
        case CXLinkage_Internal: return DLinkageKindInternal;
        case CXLinkage_External:
        case CXLinkage_UniqueExternal:
            return DLinkageKindExternal;
        default:
            break;
    }

    return DLinkageKindNone;
}

/*
framework_scanner::m_extract_qualified_name
  Builds the fully-qualified name by walking semantic parents.

Parameter(s):
  _cursor: the cursor whose qualified name to build.
Return:
  the qualified name (e.g. "ns::class::method").
*/
std::string
framework_scanner::m_extract_qualified_name
(
    CXCursor _cursor
)
const
{
    std::vector<std::string> parts;
    CXCursor                 current;
    std::string              result;

    current = _cursor;

    // walk up the semantic parent chain
    while ( (current.kind != CXCursor_TranslationUnit) &&
            (current.kind != CXCursor_InvalidFile) )
    {
        std::string name =
            m_cx_to_std(clang_getCursorSpelling(current));

        if (!name.empty())
        {
            parts.push_back(name);
        }

        current = clang_getCursorSemanticParent(current);
    }

    // reverse to get outermost-first ordering
    std::reverse(parts.begin(), parts.end());

    // join with "::"
    for (size_t i = 0; i < parts.size(); i++)
    {
        if (i > 0)
        {
            result += "::";
        }

        result += parts[i];
    }

    return result;
}

/*
framework_scanner::m_extract_usr
  Extracts the Unified Symbol Resolution string for a cursor.

Parameter(s):
  _cursor: the cursor to query.
Return:
  the USR string, or empty if unavailable.
*/
std::string
framework_scanner::m_extract_usr
(
    CXCursor _cursor
)
const
{
    return m_cx_to_std(clang_getCursorUSR(_cursor));
}

// ============================================================
// comment parsing
// ============================================================

/*
framework_scanner::m_extract_comment
  Extracts the raw comment and parses it into structured fields
  following the style guide's brief comment format.

Parameter(s):
  _cursor: the cursor whose comment to extract.
Return:
  a populated d_comment_info.
*/
d_comment_info
framework_scanner::m_extract_comment
(
    CXCursor _cursor
)
const
{
    d_comment_info info;
    CXString       raw;
    CXString       brief;

    raw = clang_Cursor_getRawCommentText(_cursor);
    info.raw_comment = m_cx_to_std(raw);

    brief = clang_Cursor_getBriefCommentText(_cursor);
    info.brief_comment = m_cx_to_std(brief);

    // parse the style-guide structured comment
    if (!info.raw_comment.empty())
    {
        info = m_parse_brief_comment(info.raw_comment);
    }

    return info;
}

/*
framework_scanner::m_parse_brief_comment
  Parses a raw comment string into the style-guide format:
    // <name>
    //   <category>: <description>
  and also the function definition format with Parameter(s)
  and Return sections.

Parameter(s):
  _raw: the raw comment text (may contain // or block markers).
Return:
  a structured d_comment_info.
*/
d_comment_info
framework_scanner::m_parse_brief_comment
(
    const std::string& _raw
)
const
{
    d_comment_info  info;
    std::string     cleaned;
    std::istringstream stream;

    info.raw_comment = _raw;

    // strip comment markers (// and /* */)
    cleaned = _raw;

    // remove block comment markers
    {
        size_t pos;

        pos = cleaned.find("/*");

        while (pos != std::string::npos)
        {
            cleaned.erase(pos, 2);
            pos = cleaned.find("/*");
        }

        pos = cleaned.find("*/");

        while (pos != std::string::npos)
        {
            cleaned.erase(pos, 2);
            pos = cleaned.find("*/");
        }
    }

    // remove line comment markers
    {
        std::string       line;
        std::string       result;
        std::istringstream ls(cleaned);

        while (std::getline(ls, line))
        {
            // strip leading "// " or "//"
            size_t pos = line.find("//");

            if (pos != std::string::npos)
            {
                line = line.substr(pos + 2);

                // strip one leading space
                if ( (!line.empty()) &&
                     (line[0] == ' ') )
                {
                    line = line.substr(1);
                }
            }

            result += line + "\n";
        }

        cleaned = result;
    }

    // attempt to parse the brief format:
    //   <name>
    //     <category>: <description>
    stream = std::istringstream(cleaned);

    {
        std::string first_line;
        std::string second_line;

        // read the first non-empty line as the entity name
        while (std::getline(stream, first_line))
        {
            // trim whitespace
            size_t start = first_line.find_first_not_of(" \t");

            if (start != std::string::npos)
            {
                first_line = first_line.substr(start);
                break;
            }
        }

        // read the second line for category: description
        while (std::getline(stream, second_line))
        {
            size_t start =
                second_line.find_first_not_of(" \t");

            if (start != std::string::npos)
            {
                second_line = second_line.substr(start);
                break;
            }
        }

        // parse "category: description"
        size_t colon_pos = second_line.find(':');

        if (colon_pos != std::string::npos)
        {
            info.category_tag =
                second_line.substr(0, colon_pos);
            info.description  =
                second_line.substr(colon_pos + 1);

            // trim leading space from description
            size_t ds = info.description.find_first_not_of(" ");

            if (ds != std::string::npos)
            {
                info.description = info.description.substr(ds);
            }
        }

        info.brief_comment = first_line;
    }

    // parse Parameter(s): section
    {
        size_t param_pos = cleaned.find("Parameter(s):");

        if (param_pos != std::string::npos)
        {
            std::string       params_section;
            std::istringstream ps;
            std::string       line;

            params_section = cleaned.substr(
                param_pos + std::string("Parameter(s):").size());

            // find the end (Return: or end of string)
            size_t ret_pos = params_section.find("Return:");

            if (ret_pos != std::string::npos)
            {
                // also extract the return doc
                info.return_doc =
                    params_section.substr(
                        ret_pos +
                        std::string("Return:").size());

                // trim
                size_t rs =
                    info.return_doc.find_first_not_of(" \t\n");

                if (rs != std::string::npos)
                {
                    info.return_doc =
                        info.return_doc.substr(rs);
                }

                params_section =
                    params_section.substr(0, ret_pos);
            }

            // parse individual parameters
            ps = std::istringstream(params_section);

            while (std::getline(ps, line))
            {
                // look for "_name:" pattern
                size_t us = line.find('_');
                size_t co = line.find(':');

                if ( (us != std::string::npos) &&
                     (co != std::string::npos) &&
                     (co > us) )
                {
                    std::string pname =
                        line.substr(us, co - us);
                    std::string pdesc =
                        line.substr(co + 1);

                    // trim
                    size_t ds =
                        pdesc.find_first_not_of(" \t");

                    if (ds != std::string::npos)
                    {
                        pdesc = pdesc.substr(ds);
                    }

                    info.parameter_docs.emplace_back(
                        pname, pdesc);
                }
            }
        }
    }

    return info;
}

// ============================================================
// parameter and template extraction
// ============================================================

/*
framework_scanner::m_extract_parameters
  Extracts the parameter list from a function or method cursor.

Parameter(s):
  _cursor: the function/method cursor.
Return:
  a vector of d_parameter_info for each parameter.
*/
std::vector<d_parameter_info>
framework_scanner::m_extract_parameters
(
    CXCursor _cursor
)
const
{
    std::vector<d_parameter_info> params;
    int                           num_args;

    num_args = clang_Cursor_getNumArguments(_cursor);

    // skip non-function cursors
    if (num_args < 0)
    {
        return params;
    }

    // extract each parameter
    for (int i = 0; i < num_args; i++)
    {
        CXCursor         arg = clang_Cursor_getArgument(_cursor, i);
        d_parameter_info pi;

        pi.name          = m_cx_to_std(
                               clang_getCursorSpelling(arg));
        pi.type_spelling = m_cx_to_std(
                               clang_getTypeSpelling(
                                   clang_getCursorType(arg)));

        params.push_back(pi);
    }

    return params;
}

/*
framework_scanner::m_extract_template_params
  Extracts template parameter names from a template cursor.

Parameter(s):
  _cursor: the template cursor.
Return:
  a vector of template parameter name strings.
*/
std::vector<std::string>
framework_scanner::m_extract_template_params
(
    CXCursor _cursor
)
const
{
    std::vector<std::string> params;
    int                      num;

    num = clang_Cursor_getNumTemplateArguments(_cursor);

    if (num <= 0)
    {
        return params;
    }

    // note: for proper template param extraction we must
    // visit children of template cursors
    // (clang_Cursor_getNumTemplateArguments is for
    // specializations). Fall back to child visitor.

    return params;
}

/*
framework_scanner::m_extract_base_classes
  Extracts the base class names from a class/struct cursor.

Parameter(s):
  _cursor: the class/struct cursor.
Return:
  a vector of base class type-spelling strings.
*/
std::vector<std::string>
framework_scanner::m_extract_base_classes
(
    CXCursor _cursor
)
const
{
    std::vector<std::string> bases;

    // visit children looking for CXCursor_CXXBaseSpecifier
    clang_visitChildren(
        _cursor,
        [](CXCursor  _child,
           CXCursor  _parent,
           CXClientData _data) -> CXChildVisitResult
        {
            auto* vec =
                static_cast<std::vector<std::string>*>(_data);

            if (clang_getCursorKind(_child) ==
                CXCursor_CXXBaseSpecifier)
            {
                CXType base_type =
                    clang_getCursorType(_child);
                CXString spelling =
                    clang_getTypeSpelling(base_type);
                std::string name =
                    clang_getCString(spelling);

                clang_disposeString(spelling);
                vec->push_back(name);
            }

            return CXChildVisit_Continue;
        },
        &bases);

    return bases;
}

// ============================================================
// dependency extraction
// ============================================================

/*
framework_scanner::m_extract_references
  Visits children of a cursor to find type references, member
  references, and other dependency edges, storing their USRs
  in the symbol's referenced_usrs set.

Parameter(s):
  _cursor: the cursor to scan for references.
  _sym:    the symbol info to populate with dependencies.
Return:
  none.
*/
void
framework_scanner::m_extract_references
(
    CXCursor       _cursor,
    d_symbol_info& _sym
)
const
{
    // visit children to find referenced cursors
    clang_visitChildren(
        _cursor,
        [](CXCursor    _child,
           CXCursor    _parent,
           CXClientData _data) -> CXChildVisitResult
        {
            auto* refs =
                static_cast<std::set<std::string>*>(_data);

            CXCursorKind kind = clang_getCursorKind(_child);

            // type references, member references, decl refs
            if ( (kind == CXCursor_TypeRef)        ||
                 (kind == CXCursor_MemberRef)      ||
                 (kind == CXCursor_DeclRefExpr)    ||
                 (kind == CXCursor_TemplateRef)    ||
                 (kind == CXCursor_CXXBaseSpecifier) )
            {
                CXCursor referenced =
                    clang_getCursorReferenced(_child);
                CXString usr =
                    clang_getCursorUSR(referenced);
                const char* str = clang_getCString(usr);

                if ( (str) && (str[0] != '\0') )
                {
                    refs->insert(str);
                }

                clang_disposeString(usr);
            }

            return CXChildVisit_Recurse;
        },
        &_sym.referenced_usrs);

    return;
}

// ============================================================
// module resolution
// ============================================================

/*
framework_scanner::m_resolve_module
  Determines the module name for a file based on its path
  relative to the framework root. The module is typically the
  stem of the file or its parent directory name.

Parameter(s):
  _file: the canonical file path.
Return:
  the resolved module name.
*/
std::string
framework_scanner::m_resolve_module
(
    const std::string& _file
)
const
{
    fs::path file_path(_file);
    fs::path root_path(m_config.framework_root);

    // try to make relative to framework root
    if ( (!m_config.framework_root.empty()) &&
         (_file.find(m_config.framework_root) == 0) )
    {
        fs::path rel =
            fs::relative(file_path, root_path);

        // use the first directory component as module name,
        // or the stem if it's a top-level file
        if (rel.has_parent_path())
        {
            return rel.begin()->string();
        }

        return rel.stem().string();
    }

    // fallback: use the file stem
    return file_path.stem().string();
}

/*
framework_scanner::m_register_module
  Creates or updates the module entry for a source file.

Parameter(s):
  _file: the canonical file path.
Return:
  none.
*/
void
framework_scanner::m_register_module
(
    const std::string& _file
)
{
    std::string   mod_name;
    fs::path      file_path(_file);
    std::string   ext;

    mod_name = m_resolve_module(_file);
    ext      = file_path.extension().string();

    // create the module if it doesn't exist
    if (m_modules.count(mod_name) == 0)
    {
        d_module_info mod;
        mod.name = mod_name;
        mod.path = file_path.parent_path().string();
        m_modules[mod_name] = mod;
    }

    // register this file as header or source
    if ( (ext == ".h")   ||
         (ext == ".hpp") ||
         (ext == ".hxx") )
    {
        m_modules[mod_name].header_files.push_back(_file);
    }
    else
    {
        m_modules[mod_name].source_files.push_back(_file);
    }

    // extract #include dependencies
    m_extract_includes(_file, m_modules[mod_name]);

    return;
}

/*
framework_scanner::m_extract_includes
  Reads the file and extracts #include directives to populate
  the module's include list and inter-module dependencies.

Parameter(s):
  _file: the file to scan for includes.
  _mod:  the module info to populate.
Return:
  none.
*/
void
framework_scanner::m_extract_includes
(
    const std::string& _file,
    d_module_info&     _mod
)
const
{
    std::ifstream  in(_file);
    std::string    line;
    std::regex     include_re(
        R"(^\s*#\s*include\s*[<"]([^>"]+)[>"])");
    std::smatch    match;

    // check if file opened
    if (!in.is_open())
    {
        return;
    }

    // scan each line for #include
    while (std::getline(in, line))
    {
        if (std::regex_search(line, match, include_re))
        {
            std::string included = match[1].str();
            _mod.includes.push_back(included);

            // extract the stem as a module dependency
            fs::path inc_path(included);
            std::string dep_mod = inc_path.stem().string();

            if (dep_mod != _mod.name)
            {
                _mod.depends_on_modules.insert(dep_mod);
            }
        }
    }

    return;
}

// ============================================================
// utilities
// ============================================================

/*
framework_scanner::m_cx_to_std
  Converts a CXString to std::string and disposes the CXString.

Parameter(s):
  _cx: the CXString to convert.
Return:
  the resulting std::string.
*/
std::string
framework_scanner::m_cx_to_std
(
    CXString _cx
)
{
    const char* str;
    std::string result;

    str = clang_getCString(_cx);

    if (str)
    {
        result = str;
    }

    clang_disposeString(_cx);

    return result;
}

/*
framework_scanner::m_is_in_system_header
  Returns true if the cursor is located in a system header.

Parameter(s):
  _cursor: the cursor to check.
Return:
  true if the cursor is in a system header, false otherwise.
*/
bool
framework_scanner::m_is_in_system_header
(
    CXCursor _cursor
)
{
    CXSourceLocation loc =
        clang_getCursorLocation(_cursor);

    return clang_Location_isInSystemHeader(loc);
}

NS_END
