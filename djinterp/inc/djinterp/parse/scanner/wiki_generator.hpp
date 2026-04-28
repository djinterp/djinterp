// wiki_generator.hpp
//
//   Module 3: autonomous wiki generator. Reads the catalogue
// database and produces a complete, interlinked wiki with
// per-module pages, per-symbol pages, an index, and Mermaid
// dependency graphs. Supports Markdown, HTML, and MediaWiki
// output formats.

#ifndef D_CATALOGUE_WIKI_WIKI_GENERATOR_HPP
#define D_CATALOGUE_WIKI_WIKI_GENERATOR_HPP

#include "../common/types.hpp"
#include "../database/catalogue_db.hpp"

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace d_catalogue {

// ============================================================
// page content
// ============================================================

// d_wiki_page
//   struct: represents a single generated wiki page with its
//   filesystem path, title, and rendered body content.
struct d_wiki_page
{
    std::string path;
    std::string title;
    std::string body;
    std::string category;
};

// ============================================================
// wiki generation callbacks
// ============================================================

// d_wiki_callbacks
//   struct: optional hooks for progress during generation.
struct d_wiki_callbacks
{
    std::function<void(const std::string& _page_path)>
        on_page_written;

    std::function<void(const std::string& _message)>
        on_progress;

    std::function<bool(const d_symbol_info& _sym)>
        on_symbol_filter;

    std::function<bool(const d_module_info& _mod)>
        on_module_filter;
};

// ============================================================
// wiki generator
// ============================================================

// d_wiki_generator
//   class: reads from a d_catalogue_db and autonomously
//   produces a complete, cross-linked wiki filesystem.
class d_wiki_generator
{
private:
    using page_map = std::map<std::string, d_wiki_page>;

public:
    d_wiki_generator();
    d_wiki_generator(const d_wiki_config&  _config,
                     const d_catalogue_db& _db);
    ~d_wiki_generator();

    // -- configuration --

    void set_config(const d_wiki_config& _config);
    void set_database(const d_catalogue_db& _db);
    void set_callbacks(const d_wiki_callbacks& _callbacks);

    const d_wiki_config& config() const;

    // -- generation --

    // generate
    //   function: runs the full wiki generation pipeline:
    //   index, module pages, symbol pages, and graphs.
    size_t generate();

    // generate_index
    //   function: generates only the main index page.
    d_wiki_page generate_index_page() const;

    // generate_module_page
    //   function: generates a wiki page for one module.
    d_wiki_page generate_module_page(const d_module_info& _mod) const;

    // generate_symbol_page
    //   function: generates a wiki page for one symbol.
    d_wiki_page generate_symbol_page(const d_symbol_info& _sym) const;

    // generate_dependency_graph
    //   function: generates a Mermaid dependency graph for
    //   either a module or the entire framework.
    std::string generate_dependency_graph(const std::string& _module) const;
    std::string generate_full_dependency_graph() const;

    // -- access to generated pages --

    const page_map& pages() const;

private:
    // -- page rendering --

    std::string m_render_header(const std::string& _title, int _level) const;
    std::string m_render_link(const std::string& _text, const std::string& _target) const;
    std::string m_render_code_block(const std::string& _code, const std::string& _lang) const;
    std::string m_render_table_header(const std::vector<std::string>& _cols) const;
    std::string m_render_table_row(const std::vector<std::string>& _cols) const;
    std::string m_render_table_end() const;
    std::string m_render_badge(const std::string& _label, const std::string& _color) const;
    std::string m_render_horizontal_rule() const;
    std::string m_render_bold(const std::string& _text) const;
    std::string m_render_italic(const std::string& _text) const;
    std::string m_render_list_item(const std::string& _text, int _indent) const;

    // -- section builders --

    std::string m_build_symbol_signature(const d_symbol_info& _sym) const;
    std::string m_build_parameter_table(const d_symbol_info& _sym) const;
    std::string m_build_qualifier_badges(const d_symbol_info& _sym) const;
    std::string m_build_dependency_section(const d_symbol_info& _sym) const;
    std::string m_build_members_table(const std::string& _parent_usr) const;
    std::string m_build_enum_table(const d_symbol_info& _sym) const;
    std::string m_build_module_summary_table() const;
    std::string m_build_category_stats() const;

    // -- path helpers --

    std::string m_page_filename(const std::string& _name) const;
    std::string m_symbol_page_path(const d_symbol_info& _sym) const;
    std::string m_module_page_path(const d_module_info& _mod) const;
    std::string m_file_extension() const;
    std::string m_sanitize_filename(const std::string& _name) const;

    // -- file I/O --

    bool m_write_page(const d_wiki_page& _page) const;
    bool m_ensure_directory(const std::string& _dir) const;

    // -- members --

    d_wiki_config         m_config;
    const d_catalogue_db* m_db;
    d_wiki_callbacks      m_callbacks;
    page_map              m_pages;
};

} // namespace d_catalogue

#endif // D_CATALOGUE_WIKI_WIKI_GENERATOR_HPP
