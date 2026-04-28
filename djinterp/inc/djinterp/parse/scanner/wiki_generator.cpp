#include "wiki_generator.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace d_catalogue {

// ============================================================
// construction / destruction
// ============================================================

/*
d_wiki_generator::d_wiki_generator
  Default-constructs the generator with no database attached.

Parameter(s):
  (none)
Return:
  (constructor)
*/
d_wiki_generator::d_wiki_generator()
    : m_config()
    , m_db(nullptr)
    , m_callbacks()
    , m_pages()
{
}

/*
d_wiki_generator::d_wiki_generator
  Constructs the generator with a config and database reference.

Parameter(s):
  _config: wiki generation configuration.
  _db:     the catalogue database to read from.
Return:
  (constructor)
*/
d_wiki_generator::d_wiki_generator
(
    const d_wiki_config&  _config,
    const d_catalogue_db& _db
)
    : m_config(_config)
    , m_db(&_db)
    , m_callbacks()
    , m_pages()
{
}

/*
d_wiki_generator::~d_wiki_generator
  Destructor.

Parameter(s):
  (none)
Return:
  (destructor)
*/
d_wiki_generator::~d_wiki_generator()
{
}

// ============================================================
// configuration
// ============================================================

/*
d_wiki_generator::set_config
  Replaces the generation configuration.

Parameter(s):
  _config: the new configuration.
Return:
  none.
*/
void
d_wiki_generator::set_config
(
    const d_wiki_config& _config
)
{
    m_config = _config;

    return;
}

/*
d_wiki_generator::set_database
  Attaches a catalogue database.

Parameter(s):
  _db: the database to read from.
Return:
  none.
*/
void
d_wiki_generator::set_database
(
    const d_catalogue_db& _db
)
{
    m_db = &_db;

    return;
}

/*
d_wiki_generator::set_callbacks
  Installs progress and filter callbacks.

Parameter(s):
  _callbacks: the callbacks to install.
Return:
  none.
*/
void
d_wiki_generator::set_callbacks
(
    const d_wiki_callbacks& _callbacks
)
{
    m_callbacks = _callbacks;

    return;
}

/*
d_wiki_generator::config
  Returns the current configuration.

Parameter(s):
  (none)
Return:
  the d_wiki_config.
*/
const d_wiki_config&
d_wiki_generator::config() const
{
    return m_config;
}

// ============================================================
// generation pipeline
// ============================================================

/*
d_wiki_generator::generate
  Runs the full wiki generation pipeline. Creates the output
  directory, generates the index page, all module pages, all
  symbol pages, and dependency graphs. Writes everything to
  the filesystem.

Parameter(s):
  (none)
Return:
  the total number of pages generated.
*/
size_t
d_wiki_generator::generate()
{
    // validate state
    if (!m_db)
    {
        return 0;
    }

    m_pages.clear();

    // create the output directory structure
    m_ensure_directory(m_config.output_directory);
    m_ensure_directory(m_config.output_directory + "/modules");
    m_ensure_directory(m_config.output_directory + "/symbols");

    if (m_callbacks.on_progress)
    {
        m_callbacks.on_progress("starting wiki generation...");
    }

    // generate the main index
    if (m_config.generate_index)
    {
        d_wiki_page index = generate_index_page();
        m_pages[index.path] = index;
        m_write_page(index);

        if (m_callbacks.on_page_written)
        {
            m_callbacks.on_page_written(index.path);
        }
    }

    // generate module pages
    if (m_config.generate_module_pages)
    {
        std::vector<d_module_info> modules = m_db->all_modules();

        for (const auto& mod : modules)
        {
            // apply module filter
            if (m_callbacks.on_module_filter)
            {
                if (!m_callbacks.on_module_filter(mod))
                {
                    continue;
                }
            }

            d_wiki_page page = generate_module_page(mod);
            m_pages[page.path] = page;
            m_write_page(page);

            if (m_callbacks.on_page_written)
            {
                m_callbacks.on_page_written(page.path);
            }
        }
    }

    // generate symbol pages
    if (m_config.generate_symbol_pages)
    {
        m_db->for_each_symbol(
            [this](const d_symbol_info& _sym)
            {
                // apply symbol filter
                if (m_callbacks.on_symbol_filter)
                {
                    if (!m_callbacks.on_symbol_filter(_sym))
                    {
                        return;
                    }
                }

                d_wiki_page page =
                    generate_symbol_page(_sym);
                m_pages[page.path] = page;
                m_write_page(page);

                if (m_callbacks.on_page_written)
                {
                    m_callbacks.on_page_written(page.path);
                }
            });
    }

    if (m_callbacks.on_progress)
    {
        m_callbacks.on_progress(
            "wiki generation complete: "
            + std::to_string(m_pages.size())
            + " pages");
    }

    return m_pages.size();
}

// ============================================================
// individual page generators
// ============================================================

/*
d_wiki_generator::generate_index_page
  Generates the main wiki index page with a module listing,
  category statistics, and the full dependency graph.

Parameter(s):
  (none)
Return:
  the rendered index page.
*/
d_wiki_page
d_wiki_generator::generate_index_page() const
{
    d_wiki_page page;
    std::string body;

    page.title    = m_config.wiki_title;
    page.path     = m_config.output_directory
                  + "/index" + m_file_extension();
    page.category = "index";

    // title
    body += m_render_header(m_config.wiki_title, 1);
    body += "\n";

    // statistics overview
    body += m_render_header("Overview", 2);
    body += "\n";
    body += m_build_category_stats();
    body += "\n";

    // module listing
    body += m_render_header("Modules", 2);
    body += "\n";
    body += m_build_module_summary_table();
    body += "\n";

    // full dependency graph
    if (m_config.generate_dependency_graphs)
    {
        body += m_render_header("Dependency Graph", 2);
        body += "\n";
        body += m_render_code_block(
                    generate_full_dependency_graph(),
                    "mermaid");
        body += "\n";
    }

    page.body = body;

    return page;
}

/*
d_wiki_generator::generate_module_page
  Generates a wiki page for a single module, listing all its
  symbols grouped by category, its includes, and its dependency
  graph.

Parameter(s):
  _mod: the module to generate a page for.
Return:
  the rendered module page.
*/
d_wiki_page
d_wiki_generator::generate_module_page
(
    const d_module_info& _mod
)
const
{
    d_wiki_page page;
    std::string body;

    page.title    = "Module: " + _mod.name;
    page.path     = m_module_page_path(_mod);
    page.category = "module";

    // title and description
    body += m_render_header(page.title, 1);
    body += "\n";

    if (!_mod.description.empty())
    {
        body += _mod.description + "\n\n";
    }

    // source files
    body += m_render_header("Files", 2);
    body += "\n";

    if (!_mod.header_files.empty())
    {
        body += m_render_bold("Headers:") + "\n";

        for (const auto& h : _mod.header_files)
        {
            body += m_render_list_item(
                        m_render_code_block(h, ""), 0);
        }

        body += "\n";
    }

    if (!_mod.source_files.empty())
    {
        body += m_render_bold("Sources:") + "\n";

        for (const auto& s : _mod.source_files)
        {
            body += m_render_list_item(
                        m_render_code_block(s, ""), 0);
        }

        body += "\n";
    }

    // dependencies
    if (!_mod.depends_on_modules.empty())
    {
        body += m_render_header("Dependencies", 2);
        body += "\n";

        for (const auto& dep : _mod.depends_on_modules)
        {
            body += m_render_list_item(
                        m_render_link(
                            dep,
                            "../modules/"
                                + m_sanitize_filename(dep)
                                + m_file_extension()),
                        0);
        }

        body += "\n";
    }

    // symbols grouped by category
    body += m_render_header("Symbols", 2);
    body += "\n";

    std::vector<d_symbol_info> symbols =
        m_db->symbols_in_module(_mod.name);

    // group by category
    std::map<DSymbolCategory,
             std::vector<const d_symbol_info*>> grouped;

    for (const auto& sym : symbols)
    {
        grouped[sym.category].push_back(&sym);
    }

    // render each category group
    for (const auto& [cat, syms] : grouped)
    {
        body += m_render_header(
                    std::string(
                        d_symbol_category_to_string(cat))
                        + "s",
                    3);
        body += "\n";

        // table header
        body += m_render_table_header(
                    {"Name", "Type", "Description"});

        for (const auto* sym : syms)
        {
            std::string link = m_render_link(
                sym->name,
                "../symbols/"
                    + m_sanitize_filename(sym->qualified_name)
                    + m_file_extension());

            body += m_render_table_row(
                        {link,
                         sym->type_spelling,
                         sym->comment.description});
        }

        body += m_render_table_end();
        body += "\n";
    }

    // dependency graph for this module
    if (m_config.generate_dependency_graphs)
    {
        body += m_render_header("Dependency Graph", 2);
        body += "\n";
        body += m_render_code_block(
                    generate_dependency_graph(_mod.name),
                    "mermaid");
        body += "\n";
    }

    page.body = body;

    return page;
}

/*
d_wiki_generator::generate_symbol_page
  Generates a wiki page for a single symbol, including its
  signature, documentation, parameters, qualifiers, and
  dependency info.

Parameter(s):
  _sym: the symbol to generate a page for.
Return:
  the rendered symbol page.
*/
d_wiki_page
d_wiki_generator::generate_symbol_page
(
    const d_symbol_info& _sym
)
const
{
    d_wiki_page page;
    std::string body;

    page.title = _sym.qualified_name.empty()
               ? _sym.name
               : _sym.qualified_name;
    page.path     = m_symbol_page_path(_sym);
    page.category = d_symbol_category_to_string(_sym.category);

    // title
    body += m_render_header(page.title, 1);
    body += "\n";

    // qualifier badges
    std::string badges = m_build_qualifier_badges(_sym);

    if (!badges.empty())
    {
        body += badges + "\n\n";
    }

    // category and module breadcrumb
    body += m_render_bold("Category:") + " "
          + d_symbol_category_to_string(_sym.category) + "\n";
    body += m_render_bold("Module:") + " "
          + m_render_link(
                _sym.module_name,
                "../modules/"
                    + m_sanitize_filename(_sym.module_name)
                    + m_file_extension())
          + "\n";

    if (!_sym.parent_name.empty())
    {
        body += m_render_bold("Parent:") + " "
              + _sym.parent_name + "\n";
    }

    body += "\n";

    // description from comment
    if ( (!_sym.comment.description.empty()) ||
         (!_sym.comment.raw_comment.empty()) )
    {
        body += m_render_header("Description", 2);
        body += "\n";

        if (!_sym.comment.description.empty())
        {
            body += _sym.comment.description + "\n\n";
        }
        else
        {
            body += _sym.comment.raw_comment + "\n\n";
        }
    }

    // signature
    std::string sig = m_build_symbol_signature(_sym);

    if (!sig.empty())
    {
        body += m_render_header("Signature", 2);
        body += "\n";
        body += m_render_code_block(sig, "cpp");
        body += "\n";
    }

    // parameters
    if (!_sym.parameters.empty())
    {
        body += m_render_header("Parameters", 2);
        body += "\n";
        body += m_build_parameter_table(_sym);
        body += "\n";
    }

    // return documentation
    if (!_sym.comment.return_doc.empty())
    {
        body += m_render_header("Return Value", 2);
        body += "\n";
        body += _sym.comment.return_doc + "\n\n";
    }

    // template parameters
    if (!_sym.template_parameters.empty())
    {
        body += m_render_header("Template Parameters", 2);
        body += "\n";

        for (const auto& tp : _sym.template_parameters)
        {
            body += m_render_list_item(
                        m_render_code_block(tp, ""), 0);
        }

        body += "\n";
    }

    // base classes
    if (!_sym.base_classes.empty())
    {
        body += m_render_header("Base Classes", 2);
        body += "\n";

        for (const auto& bc : _sym.base_classes)
        {
            body += m_render_list_item(bc, 0);
        }

        body += "\n";
    }

    // enum constants
    if (!_sym.enum_constants.empty())
    {
        body += m_render_header("Values", 2);
        body += "\n";
        body += m_build_enum_table(_sym);
        body += "\n";
    }

    // members (for classes/structs)
    if ( (_sym.category == DSymbolCategoryClass)  ||
         (_sym.category == DSymbolCategoryStruct) ||
         (_sym.category == DSymbolCategoryClassTemplate) )
    {
        std::string members =
            m_build_members_table(_sym.usr);

        if (!members.empty())
        {
            body += m_render_header("Members", 2);
            body += "\n";
            body += members;
            body += "\n";
        }
    }

    // dependencies
    std::string deps = m_build_dependency_section(_sym);

    if (!deps.empty())
    {
        body += m_render_header("Dependencies", 2);
        body += "\n";
        body += deps;
        body += "\n";
    }

    // source location
    if ( (m_config.include_source_links) &&
         (!_sym.definition_loc.file.empty()) )
    {
        body += m_render_header("Source", 2);
        body += "\n";
        body += m_render_code_block(
                    _sym.definition_loc.file + ":"
                    + std::to_string(
                          _sym.definition_loc.line),
                    "");
        body += "\n";
    }

    page.body = body;

    return page;
}

// ============================================================
// dependency graphs
// ============================================================

/*
d_wiki_generator::generate_dependency_graph
  Generates a Mermaid graph showing dependencies for a single
  module.

Parameter(s):
  _module: the module name.
Return:
  a Mermaid graph definition string.
*/
std::string
d_wiki_generator::generate_dependency_graph
(
    const std::string& _module
)
const
{
    std::string            graph;
    std::set<std::string>  deps;

    if (!m_db)
    {
        return "";
    }

    deps  = m_db->module_dependencies(_module);
    graph = "graph LR\n";
    graph += "    " + m_sanitize_filename(_module)
           + "[\"" + _module + "\"]\n";

    // add edges to each dependency
    for (const auto& dep : deps)
    {
        std::string safe_dep = m_sanitize_filename(dep);

        graph += "    " + safe_dep
               + "[\"" + dep + "\"]\n";
        graph += "    " + m_sanitize_filename(_module)
               + " --> " + safe_dep + "\n";
    }

    return graph;
}

/*
d_wiki_generator::generate_full_dependency_graph
  Generates a Mermaid graph of all inter-module dependencies.

Parameter(s):
  (none)
Return:
  a Mermaid graph definition string.
*/
std::string
d_wiki_generator::generate_full_dependency_graph() const
{
    std::string graph;

    if (!m_db)
    {
        return "";
    }

    graph = "graph LR\n";

    std::vector<d_module_info> modules = m_db->all_modules();

    // declare all module nodes
    for (const auto& mod : modules)
    {
        graph += "    " + m_sanitize_filename(mod.name)
               + "[\"" + mod.name + "\"]\n";
    }

    // add dependency edges
    for (const auto& mod : modules)
    {
        std::set<std::string> deps =
            m_db->module_dependencies(mod.name);

        for (const auto& dep : deps)
        {
            graph += "    " + m_sanitize_filename(mod.name)
                   + " --> "
                   + m_sanitize_filename(dep) + "\n";
        }
    }

    return graph;
}

/*
d_wiki_generator::pages
  Returns all generated pages.

Parameter(s):
  (none)
Return:
  the page map.
*/
const d_wiki_generator::page_map&
d_wiki_generator::pages() const
{
    return m_pages;
}

// ============================================================
// format-aware rendering primitives
// ============================================================

/*
d_wiki_generator::m_render_header
  Renders a section header in the configured output format.

Parameter(s):
  _title: the header text.
  _level: heading level (1-6).
Return:
  the rendered header string.
*/
std::string
d_wiki_generator::m_render_header
(
    const std::string& _title,
    int                _level
)
const
{
    switch (m_config.format)
    {
        case DWikiFormatMarkdown:
        {
            return std::string(_level, '#') + " " + _title
                 + "\n";
        }

        case DWikiFormatHTML:
        {
            std::string tag = "h" + std::to_string(_level);

            return "<" + tag + ">" + _title
                 + "</" + tag + ">\n";
        }

        case DWikiFormatMediaWiki:
        {
            std::string eq(_level, '=');

            return eq + " " + _title + " " + eq + "\n";
        }
    }

    return _title + "\n";
}

/*
d_wiki_generator::m_render_link
  Renders a hyperlink in the configured output format.

Parameter(s):
  _text:   the link display text.
  _target: the link URL or path.
Return:
  the rendered link.
*/
std::string
d_wiki_generator::m_render_link
(
    const std::string& _text,
    const std::string& _target
)
const
{
    switch (m_config.format)
    {
        case DWikiFormatMarkdown:
            return "[" + _text + "](" + _target + ")";

        case DWikiFormatHTML:
            return "<a href=\"" + _target + "\">"
                 + _text + "</a>";

        case DWikiFormatMediaWiki:
            return "[[" + _target + "|" + _text + "]]";
    }

    return _text;
}

/*
d_wiki_generator::m_render_code_block
  Renders a fenced/tagged code block.

Parameter(s):
  _code: the code content.
  _lang: the language hint (may be empty).
Return:
  the rendered code block.
*/
std::string
d_wiki_generator::m_render_code_block
(
    const std::string& _code,
    const std::string& _lang
)
const
{
    // for inline code (no language, short)
    if ( (_lang.empty()) &&
         (_code.find('\n') == std::string::npos) )
    {
        switch (m_config.format)
        {
            case DWikiFormatMarkdown:
                return "`" + _code + "`";

            case DWikiFormatHTML:
                return "<code>" + _code + "</code>";

            case DWikiFormatMediaWiki:
                return "<code>" + _code + "</code>";
        }
    }

    // fenced block
    switch (m_config.format)
    {
        case DWikiFormatMarkdown:
            return "```" + _lang + "\n" + _code + "\n```\n";

        case DWikiFormatHTML:
            return "<pre><code class=\"language-"
                 + _lang + "\">"
                 + _code + "</code></pre>\n";

        case DWikiFormatMediaWiki:
            return "<syntaxhighlight lang=\""
                 + _lang + "\">\n"
                 + _code
                 + "\n</syntaxhighlight>\n";
    }

    return _code + "\n";
}

/*
d_wiki_generator::m_render_table_header
  Renders a table header row.

Parameter(s):
  _cols: the column header labels.
Return:
  the rendered header.
*/
std::string
d_wiki_generator::m_render_table_header
(
    const std::vector<std::string>& _cols
)
const
{
    std::string result;

    switch (m_config.format)
    {
        case DWikiFormatMarkdown:
        {
            result = "|";

            for (const auto& col : _cols)
            {
                result += " " + col + " |";
            }

            result += "\n|";

            for (size_t i = 0; i < _cols.size(); i++)
            {
                result += " --- |";
            }

            result += "\n";
            break;
        }

        case DWikiFormatHTML:
        {
            result = "<table>\n<thead><tr>\n";

            for (const auto& col : _cols)
            {
                result += "  <th>" + col + "</th>\n";
            }

            result += "</tr></thead>\n<tbody>\n";
            break;
        }

        case DWikiFormatMediaWiki:
        {
            result = "{| class=\"wikitable\"\n! ";

            for (size_t i = 0; i < _cols.size(); i++)
            {
                if (i > 0)
                {
                    result += " !! ";
                }

                result += _cols[i];
            }

            result += "\n";
            break;
        }
    }

    return result;
}

/*
d_wiki_generator::m_render_table_row
  Renders a single table data row.

Parameter(s):
  _cols: the cell values.
Return:
  the rendered row.
*/
std::string
d_wiki_generator::m_render_table_row
(
    const std::vector<std::string>& _cols
)
const
{
    std::string result;

    switch (m_config.format)
    {
        case DWikiFormatMarkdown:
        {
            result = "|";

            for (const auto& col : _cols)
            {
                result += " " + col + " |";
            }

            result += "\n";
            break;
        }

        case DWikiFormatHTML:
        {
            result = "<tr>\n";

            for (const auto& col : _cols)
            {
                result += "  <td>" + col + "</td>\n";
            }

            result += "</tr>\n";
            break;
        }

        case DWikiFormatMediaWiki:
        {
            result = "|-\n| ";

            for (size_t i = 0; i < _cols.size(); i++)
            {
                if (i > 0)
                {
                    result += " || ";
                }

                result += _cols[i];
            }

            result += "\n";
            break;
        }
    }

    return result;
}

/*
d_wiki_generator::m_render_table_end
  Renders the table closing markup.

Parameter(s):
  (none)
Return:
  the closing markup.
*/
std::string
d_wiki_generator::m_render_table_end() const
{
    switch (m_config.format)
    {
        case DWikiFormatMarkdown:  return "\n";
        case DWikiFormatHTML:      return "</tbody></table>\n";
        case DWikiFormatMediaWiki: return "|}\n";
    }

    return "\n";
}

/*
d_wiki_generator::m_render_badge
  Renders an inline badge/label.

Parameter(s):
  _label: the badge text.
  _color: a CSS color hint (Markdown falls back to bold).
Return:
  the rendered badge.
*/
std::string
d_wiki_generator::m_render_badge
(
    const std::string& _label,
    const std::string& _color
)
const
{
    switch (m_config.format)
    {
        case DWikiFormatMarkdown:
            return "**`" + _label + "`** ";

        case DWikiFormatHTML:
            return "<span style=\"background:" + _color
                 + ";color:white;padding:2px 6px;"
                   "border-radius:3px;font-size:0.85em;\">"
                 + _label + "</span> ";

        case DWikiFormatMediaWiki:
            return "{{badge|" + _label + "|" + _color + "}} ";
    }

    return _label + " ";
}

/*
d_wiki_generator::m_render_horizontal_rule
  Renders a horizontal rule.

Parameter(s):
  (none)
Return:
  the rendered rule.
*/
std::string
d_wiki_generator::m_render_horizontal_rule() const
{
    switch (m_config.format)
    {
        case DWikiFormatMarkdown:  return "\n---\n\n";
        case DWikiFormatHTML:      return "<hr/>\n";
        case DWikiFormatMediaWiki: return "\n----\n\n";
    }

    return "\n";
}

/*
d_wiki_generator::m_render_bold
  Renders bold text.

Parameter(s):
  _text: the text to embolden.
Return:
  the rendered bold text.
*/
std::string
d_wiki_generator::m_render_bold
(
    const std::string& _text
)
const
{
    switch (m_config.format)
    {
        case DWikiFormatMarkdown:  return "**" + _text + "**";
        case DWikiFormatHTML:      return "<b>" + _text + "</b>";
        case DWikiFormatMediaWiki: return "'''" + _text + "'''";
    }

    return _text;
}

/*
d_wiki_generator::m_render_italic
  Renders italic text.

Parameter(s):
  _text: the text to italicize.
Return:
  the rendered text.
*/
std::string
d_wiki_generator::m_render_italic
(
    const std::string& _text
)
const
{
    switch (m_config.format)
    {
        case DWikiFormatMarkdown:  return "*" + _text + "*";
        case DWikiFormatHTML:      return "<i>" + _text + "</i>";
        case DWikiFormatMediaWiki: return "''" + _text + "''";
    }

    return _text;
}

/*
d_wiki_generator::m_render_list_item
  Renders a list item.

Parameter(s):
  _text:   the item text.
  _indent: the nesting level (0-based).
Return:
  the rendered list item.
*/
std::string
d_wiki_generator::m_render_list_item
(
    const std::string& _text,
    int                _indent
)
const
{
    std::string prefix(_indent * 2, ' ');

    switch (m_config.format)
    {
        case DWikiFormatMarkdown:
            return prefix + "- " + _text + "\n";

        case DWikiFormatHTML:
            return prefix + "<li>" + _text + "</li>\n";

        case DWikiFormatMediaWiki:
            return std::string(_indent + 1, '*')
                 + " " + _text + "\n";
    }

    return _text + "\n";
}

// ============================================================
// section builders
// ============================================================

/*
d_wiki_generator::m_build_symbol_signature
  Reconstructs a code-like signature for a symbol.

Parameter(s):
  _sym: the symbol.
Return:
  the signature string.
*/
std::string
d_wiki_generator::m_build_symbol_signature
(
    const d_symbol_info& _sym
)
const
{
    std::string sig;

    // template prefix
    if (!_sym.template_parameters.empty())
    {
        sig += "template<";

        for (size_t i = 0;
             i < _sym.template_parameters.size(); i++)
        {
            if (i > 0)
            {
                sig += ", ";
            }

            sig += _sym.template_parameters[i];
        }

        sig += ">\n";
    }

    // qualifiers
    if (_sym.is_static)    { sig += "static ";    }
    if (_sym.is_constexpr) { sig += "constexpr "; }
    if (_sym.is_inline)    { sig += "inline ";    }
    if (_sym.is_virtual)   { sig += "virtual ";   }

    // return type
    if (!_sym.return_type.empty())
    {
        sig += _sym.return_type + "\n";
    }

    // name
    sig += _sym.qualified_name.empty()
         ? _sym.name
         : _sym.qualified_name;

    // parameters
    if ( (_sym.category == DSymbolCategoryFunction)         ||
         (_sym.category == DSymbolCategoryMethod)           ||
         (_sym.category == DSymbolCategoryConstructor)      ||
         (_sym.category == DSymbolCategoryDestructor)       ||
         (_sym.category == DSymbolCategoryFunctionTemplate) )
    {
        sig += "\n(\n";

        for (size_t i = 0; i < _sym.parameters.size(); i++)
        {
            sig += "    " + _sym.parameters[i].type_spelling
                 + " " + _sym.parameters[i].name;

            if (i + 1 < _sym.parameters.size())
            {
                sig += ",";
            }

            sig += "\n";
        }

        sig += ")";

        if (_sym.is_const)     { sig += " const";     }
        if (_sym.is_noexcept)  { sig += " noexcept";  }
        if (_sym.is_pure_virtual) { sig += " = 0"; }
    }

    // base classes
    if (!_sym.base_classes.empty())
    {
        sig += " :";

        for (size_t i = 0; i < _sym.base_classes.size(); i++)
        {
            sig += "\n    public " + _sym.base_classes[i];

            if (i + 1 < _sym.base_classes.size())
            {
                sig += ",";
            }
        }
    }

    return sig;
}

/*
d_wiki_generator::m_build_parameter_table
  Builds a table of parameter name, type, and documentation.

Parameter(s):
  _sym: the symbol with parameters.
Return:
  the rendered parameter table.
*/
std::string
d_wiki_generator::m_build_parameter_table
(
    const d_symbol_info& _sym
)
const
{
    std::string result;

    result += m_render_table_header(
                  {"Name", "Type", "Description"});

    for (const auto& param : _sym.parameters)
    {
        // find matching documentation
        std::string doc;

        for (const auto& [pname, pdesc] :
             _sym.comment.parameter_docs)
        {
            if (pname == param.name)
            {
                doc = pdesc;
                break;
            }
        }

        result += m_render_table_row(
                      {m_render_code_block(param.name, ""),
                       m_render_code_block(
                           param.type_spelling, ""),
                       doc});
    }

    result += m_render_table_end();

    return result;
}

/*
d_wiki_generator::m_build_qualifier_badges
  Generates inline badges for all active qualifiers.

Parameter(s):
  _sym: the symbol.
Return:
  a string of rendered badges.
*/
std::string
d_wiki_generator::m_build_qualifier_badges
(
    const d_symbol_info& _sym
)
const
{
    std::string result;

    result += m_render_badge(
                  d_symbol_category_to_string(_sym.category),
                  "#0366d6");

    if (_sym.is_static)       { result += m_render_badge("static",       "#6f42c1"); }
    if (_sym.is_const)        { result += m_render_badge("const",        "#28a745"); }
    if (_sym.is_constexpr)    { result += m_render_badge("constexpr",    "#28a745"); }
    if (_sym.is_virtual)      { result += m_render_badge("virtual",      "#e36209"); }
    if (_sym.is_pure_virtual) { result += m_render_badge("pure virtual", "#cb2431"); }
    if (_sym.is_inline)       { result += m_render_badge("inline",       "#959da5"); }
    if (_sym.is_noexcept)     { result += m_render_badge("noexcept",     "#28a745"); }
    if (_sym.is_template)     { result += m_render_badge("template",     "#0366d6"); }
    if (_sym.is_deprecated)   { result += m_render_badge("deprecated",   "#cb2431"); }

    if (_sym.access != DAccessSpecifierNone)
    {
        result += m_render_badge(
                      d_access_specifier_to_string(_sym.access),
                      "#586069");
    }

    return result;
}

/*
d_wiki_generator::m_build_dependency_section
  Builds a list of direct dependencies and dependents for
  a symbol.

Parameter(s):
  _sym: the symbol.
Return:
  the rendered dependency section.
*/
std::string
d_wiki_generator::m_build_dependency_section
(
    const d_symbol_info& _sym
)
const
{
    std::string result;

    if (!m_db)
    {
        return result;
    }

    // direct dependencies (what this symbol uses)
    std::vector<std::string> deps =
        m_db->direct_dependencies(_sym.usr);

    if (!deps.empty())
    {
        result += m_render_bold("Depends on:") + "\n";

        for (const auto& dep_usr : deps)
        {
            d_symbol_info dep_sym =
                m_db->find_symbol_by_usr(dep_usr);

            if (!dep_sym.name.empty())
            {
                result += m_render_list_item(
                    m_render_link(
                        dep_sym.qualified_name.empty()
                            ? dep_sym.name
                            : dep_sym.qualified_name,
                        "../symbols/"
                            + m_sanitize_filename(
                                  dep_sym.qualified_name
                                      .empty()
                                  ? dep_sym.name
                                  : dep_sym.qualified_name)
                            + m_file_extension()),
                    0);
            }
        }

        result += "\n";
    }

    // direct dependents (what uses this symbol)
    std::vector<std::string> dependents =
        m_db->direct_dependents(_sym.usr);

    if (!dependents.empty())
    {
        result += m_render_bold("Used by:") + "\n";

        for (const auto& ref_usr : dependents)
        {
            d_symbol_info ref_sym =
                m_db->find_symbol_by_usr(ref_usr);

            if (!ref_sym.name.empty())
            {
                result += m_render_list_item(
                    m_render_link(
                        ref_sym.qualified_name.empty()
                            ? ref_sym.name
                            : ref_sym.qualified_name,
                        "../symbols/"
                            + m_sanitize_filename(
                                  ref_sym.qualified_name
                                      .empty()
                                  ? ref_sym.name
                                  : ref_sym.qualified_name)
                            + m_file_extension()),
                    0);
            }
        }

        result += "\n";
    }

    return result;
}

/*
d_wiki_generator::m_build_members_table
  Builds a table of all symbols whose parent USR matches.

Parameter(s):
  _parent_usr: the parent symbol's USR.
Return:
  the rendered members table.
*/
std::string
d_wiki_generator::m_build_members_table
(
    const std::string& _parent_usr
)
const
{
    std::string result;

    if (!m_db)
    {
        return result;
    }

    d_symbol_filter filter;
    filter.parent_usr = _parent_usr;

    std::vector<d_symbol_info> members =
        m_db->query_symbols(filter);

    if (members.empty())
    {
        return result;
    }

    result += m_render_table_header(
                  {"Name", "Category", "Access", "Type"});

    for (const auto& mem : members)
    {
        result += m_render_table_row(
            {m_render_link(
                 mem.name,
                 "../symbols/"
                     + m_sanitize_filename(
                           mem.qualified_name.empty()
                           ? mem.name
                           : mem.qualified_name)
                     + m_file_extension()),
             d_symbol_category_to_string(mem.category),
             d_access_specifier_to_string(mem.access),
             mem.type_spelling});
    }

    result += m_render_table_end();

    return result;
}

/*
d_wiki_generator::m_build_enum_table
  Builds a table of enum constant names and values.

Parameter(s):
  _sym: the enum symbol.
Return:
  the rendered table.
*/
std::string
d_wiki_generator::m_build_enum_table
(
    const d_symbol_info& _sym
)
const
{
    std::string result;

    result += m_render_table_header({"Name", "Value"});

    for (const auto& [name, val] : _sym.enum_constants)
    {
        result += m_render_table_row(
                      {m_render_code_block(name, ""),
                       std::to_string(val)});
    }

    result += m_render_table_end();

    return result;
}

/*
d_wiki_generator::m_build_module_summary_table
  Builds a summary table of all modules for the index page.

Parameter(s):
  (none)
Return:
  the rendered summary table.
*/
std::string
d_wiki_generator::m_build_module_summary_table() const
{
    std::string result;

    if (!m_db)
    {
        return result;
    }

    result += m_render_table_header(
                  {"Module", "Symbols", "Dependencies"});

    std::vector<d_module_info> modules = m_db->all_modules();

    for (const auto& mod : modules)
    {
        std::vector<d_symbol_info> syms =
            m_db->symbols_in_module(mod.name);

        std::string link = m_render_link(
            mod.name,
            "modules/" + m_sanitize_filename(mod.name)
                + m_file_extension());

        result += m_render_table_row(
            {link,
             std::to_string(syms.size()),
             std::to_string(mod.depends_on_modules.size())});
    }

    result += m_render_table_end();

    return result;
}

/*
d_wiki_generator::m_build_category_stats
  Builds an overview of symbol counts per category.

Parameter(s):
  (none)
Return:
  the rendered statistics section.
*/
std::string
d_wiki_generator::m_build_category_stats() const
{
    std::string result;

    if (!m_db)
    {
        return result;
    }

    d_db_stats stats = m_db->statistics();

    result += m_render_table_header({"Metric", "Count"});
    result += m_render_table_row(
                  {"Total Symbols",
                   std::to_string(stats.total_symbols)});
    result += m_render_table_row(
                  {"Total Modules",
                   std::to_string(stats.total_modules)});
    result += m_render_table_row(
                  {"Total Files",
                   std::to_string(stats.total_files)});
    result += m_render_table_row(
                  {"Total Dependencies",
                   std::to_string(stats.total_dependencies)});
    result += m_render_table_end();

    result += "\n";
    result += m_render_table_header({"Category", "Count"});

    for (const auto& [cat, cnt] : stats.category_counts)
    {
        result += m_render_table_row(
                      {d_symbol_category_to_string(cat),
                       std::to_string(cnt)});
    }

    result += m_render_table_end();

    return result;
}

// ============================================================
// path helpers
// ============================================================

/*
d_wiki_generator::m_symbol_page_path
  Computes the filesystem path for a symbol's wiki page.

Parameter(s):
  _sym: the symbol.
Return:
  the output file path.
*/
std::string
d_wiki_generator::m_symbol_page_path
(
    const d_symbol_info& _sym
)
const
{
    std::string name = _sym.qualified_name.empty()
                     ? _sym.name
                     : _sym.qualified_name;

    return m_config.output_directory + "/symbols/"
         + m_sanitize_filename(name) + m_file_extension();
}

/*
d_wiki_generator::m_module_page_path
  Computes the filesystem path for a module's wiki page.

Parameter(s):
  _mod: the module.
Return:
  the output file path.
*/
std::string
d_wiki_generator::m_module_page_path
(
    const d_module_info& _mod
)
const
{
    return m_config.output_directory + "/modules/"
         + m_sanitize_filename(_mod.name)
         + m_file_extension();
}

/*
d_wiki_generator::m_file_extension
  Returns the file extension for the configured wiki format.

Parameter(s):
  (none)
Return:
  the extension string (e.g. ".md").
*/
std::string
d_wiki_generator::m_file_extension() const
{
    switch (m_config.format)
    {
        case DWikiFormatMarkdown:  return ".md";
        case DWikiFormatHTML:      return ".html";
        case DWikiFormatMediaWiki: return ".wiki";
    }

    return ".md";
}

/*
d_wiki_generator::m_sanitize_filename
  Replaces characters unsafe for filenames with underscores.

Parameter(s):
  _name: the raw name.
Return:
  the sanitized filename (no extension).
*/
std::string
d_wiki_generator::m_sanitize_filename
(
    const std::string& _name
)
const
{
    std::string result = _name;

    // replace :: with double-underscore
    {
        size_t pos = result.find("::");

        while (pos != std::string::npos)
        {
            result.replace(pos, 2, "__");
            pos = result.find("::", pos + 2);
        }
    }

    // replace remaining unsafe chars
    for (char& c : result)
    {
        if ( (c == '/') || (c == '\\') || (c == ':')  ||
             (c == '*') || (c == '?')  || (c == '"')  ||
             (c == '<') || (c == '>')  || (c == '|')  ||
             (c == ' ') )
        {
            c = '_';
        }
    }

    return result;
}

// ============================================================
// file I/O
// ============================================================

/*
d_wiki_generator::m_write_page
  Writes a wiki page to the filesystem.

Parameter(s):
  _page: the page to write.
Return:
  true on success.
*/
bool
d_wiki_generator::m_write_page
(
    const d_wiki_page& _page
)
const
{
    std::ofstream out(_page.path);

    // check if file opened
    if (!out.is_open())
    {
        return false;
    }

    out << _page.body;
    out.close();

    return true;
}

/*
d_wiki_generator::m_ensure_directory
  Creates a directory and all parents if they don't exist.

Parameter(s):
  _dir: the directory path.
Return:
  true on success.
*/
bool
d_wiki_generator::m_ensure_directory
(
    const std::string& _dir
)
const
{
    std::error_code ec;
    fs::create_directories(_dir, ec);

    return (!ec);
}

} // namespace d_catalogue
