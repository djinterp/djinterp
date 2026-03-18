// scanner.hpp
//
//   Module 1: libclang-based framework scanner. Walks entire directory
// trees, parses every translation unit, and extracts symbols with
// their categories, comments, modules, and dependency edges.

#ifndef D_CATALOGUE_SCANNER_SCANNER_HPP
#define D_CATALOGUE_SCANNER_SCANNER_HPP

#include "../common/types.hpp"

#include <clang-c/Index.h>

#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace cataloguer
{

// ============================================================
// scanner callbacks
// ============================================================

// scanner_callbacks
//   struct: optional hooks for progress reporting and filtering
//   during a scan.
struct scanner_callbacks
{
    // called before each file; return false to skip
    std::function<bool(const std::string& _file)> on_file_begin;

    // called after each file completes
    std::function<void(const std::string& _file,
                       size_t             _symbols_found)> on_file_complete;

    // called per symbol; return false to exclude
    std::function<bool(const symbol_info& _symbol)> on_symbol_discovered;

    // called on diagnostics / errors
    std::function<void(const std::string& _file,
                       const std::string& _message,
                       int                _severity)> on_diagnostic;
};

// ============================================================
// framework scanner
// ============================================================

// framework_scanner
//   class: walks an entire C/C++ framework using libclang,
//   cataloguing every symbol by module, category, dependencies,
//   and associated documentation comments.
class framework_scanner
{
private:
    using symbol_map = std::map<std::string, symbol_info>;
    using module_map = std::map<std::string, module_info>;

public:
    using symbol_iterator       = symbol_map::iterator;
    using const_symbol_iterator = symbol_map::const_iterator;
    using module_iterator       = module_map::iterator;
    using const_module_iterator = module_map::const_iterator;

    framework_scanner();
    explicit framework_scanner(const scanner_config& _config);
    ~framework_scanner();

    // -- configuration --

    void add_include_path(const std::string& _path);
    void add_compile_flag(const std::string& _flag);
    void set_config(const scanner_config& _config);
    void set_callbacks(const scanner_callbacks& _callbacks);

    const scanner_config& config() const;

    // -- scanning --

    size_t scan_directory(const std::string& _dir);
    size_t scan_file(const std::string& _file);
    void   reset();

    // -- read-only access --

    const symbol_map& symbols() const;
    const module_map& modules() const;

    std::vector<const symbol_info*> symbols_in_module(const std::string& _module) const;
    std::vector<const symbol_info*> symbols_by_category(DSymbolCategory _category) const;

    const symbol_info* find_by_usr(const std::string& _usr) const;
    const symbol_info* find_by_name(const std::string& _name) const;

    // -- dependency edges --

    std::vector<std::pair<std::string, std::string>> dependency_edges() const;
    std::set<std::string> transitive_deps(const std::string& _usr) const;

    // -- statistics --

    size_t                            total_symbols() const;
    size_t                            total_modules() const;
    size_t                            total_files_scanned() const;
    std::map<DSymbolCategory, size_t> category_histogram() const;

private:
    // -- clang index lifecycle --
    void m_init_index();
    void m_destroy_index();

    // -- file discovery --
    static std::vector<std::string> m_collect_files(const std::string&              _dir,
                                                    const std::vector<std::string>& _exts,
                                                    bool                            _follow);

    // -- parsing --
    size_t m_parse_translation_unit(const std::string& _file);
    void   m_report_diagnostics(CXTranslationUnit  _tu,
                                const std::string& _file);

    // -- AST visitor (static callback + context) --

    struct m_visit_context
    {
        framework_scanner* scanner;
        std::string          file;
    };

    static CXChildVisitResult m_visit_cursor_callback(CXCursor  _cursor,
                                                      CXCursor  _parent,
                                                      CXClientData _ctx);

    void m_process_cursor(CXCursor           _cursor,
                          CXCursor           _parent,
                          const std::string& _file);

    // -- symbol extraction --
    symbol_info    m_extract_symbol(CXCursor _cursor, 
                                      CXCursor _parent) const;
    DSymbolCategory  m_classify_cursor(CXCursor _cursor) const;
    DAccessSpecifier m_extract_access(CXCursor _cursor) const;
    DLinkageKind     m_extract_linkage(CXCursor _cursor) const;
    std::string      m_extract_qualified_name(CXCursor _cursor) const;
    std::string      m_extract_usr(CXCursor _cursor) const;

    // -- comment parsing --
    d_comment_info   m_extract_comment(CXCursor _cursor) const;
    d_comment_info   m_parse_brief_comment(const std::string& _raw) const;

    // -- parameter and template extraction --
    std::vector<parameter_info> m_extract_parameters(CXCursor _cursor) const;
    std::vector<std::string>    m_extract_template_params(CXCursor _cursor) const;
    std::vector<std::string>    m_extract_base_classes(CXCursor _cursor) const;

    // -- dependency extraction --
    void m_extract_references(CXCursor     _cursor, 
                              symbol_info& _symbol) const;

    // -- module resolution --
    std::string m_resolve_module(const std::string& _file) const;
    void        m_register_module(const std::string& _file);
    void        m_extract_includes(const std::string& _file, 
                                   module_info& _mod) const;

    // -- utilities --
    static std::string m_cx_to_std(CXString _cx);
    static bool        m_is_in_system_header(CXCursor _cursor);

    // -- members --
    CXIndex               m_index;
    scanner_config      m_config;
    scanner_callbacks     m_callbacks;
    symbol_map            m_symbols;
    module_map            m_modules;
    std::set<std::string> m_scanned_files;
};

NS_END  // cataloguer


#endif // D_CATALOGUE_SCANNER_SCANNER_HPP
