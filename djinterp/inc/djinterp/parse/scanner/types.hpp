// types.hpp
//
//   Shared type definitions for the d_catalogue framework scanner.
// Contains all enumerations, lightweight structs, and type aliases used
// across the scanner, database, and wiki modules.

#ifndef D_CATALOGUE_COMMON_TYPES_HPP
#define D_CATALOGUE_COMMON_TYPES_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace d_catalogue {

// ============================================================
// symbol category
// ============================================================

// DSymbolCategory
//   enum: classification tag for every discovered symbol.
enum DSymbolCategory
{
    DSymbolCategoryUnknown          = 0,
    DSymbolCategoryFunction         = 1,
    DSymbolCategoryMethod           = 2,
    DSymbolCategoryConstructor      = 3,
    DSymbolCategoryDestructor       = 4,
    DSymbolCategoryStruct           = 5,
    DSymbolCategoryClass            = 6,
    DSymbolCategoryEnum             = 7,
    DSymbolCategoryEnumConstant     = 8,
    DSymbolCategoryTypedef          = 9,
    DSymbolCategoryTypeAlias        = 10,
    DSymbolCategoryVariable         = 11,
    DSymbolCategoryField            = 12,
    DSymbolCategoryMacroDefinition  = 13,
    DSymbolCategoryMacroExpansion   = 14,
    DSymbolCategoryNamespace        = 15,
    DSymbolCategoryTemplateParam    = 16,
    DSymbolCategoryConcept          = 17,
    DSymbolCategoryUnion            = 18,
    DSymbolCategoryUsingDecl        = 19,
    DSymbolCategoryFunctionTemplate = 20,
    DSymbolCategoryClassTemplate    = 21
};

const char* d_symbol_category_to_string(DSymbolCategory _category);

DSymbolCategory d_symbol_category_from_string(const std::string& _str);

// ============================================================
// access specifier
// ============================================================

// DAccessSpecifier
//   enum: access level for class/struct members.
enum DAccessSpecifier
{
    DAccessSpecifierNone      = 0,
    DAccessSpecifierPublic    = 1,
    DAccessSpecifierProtected = 2,
    DAccessSpecifierPrivate   = 3
};

const char* d_access_specifier_to_string(DAccessSpecifier _access);

// ============================================================
// linkage kind
// ============================================================

// DLinkageKind
//   enum: linkage classification for symbols.
enum DLinkageKind
{
    DLinkageKindNone     = 0,
    DLinkageKindInternal = 1,
    DLinkageKindExternal = 2
};

const char* d_linkage_kind_to_string(DLinkageKind _linkage);

// ============================================================
// source location
// ============================================================

// d_source_location
//   struct: file, line, and column of a symbol's definition or
//   declaration.
struct d_source_location
{
    std::string file;
    uint32_t    line;
    uint32_t    column;
};

// ============================================================
// parameter info
// ============================================================

// d_parameter_info
//   struct: name and type of a single function/method parameter.
struct d_parameter_info
{
    std::string name;
    std::string type_spelling;
    std::string default_value;
};

// ============================================================
// comment info
// ============================================================

// d_comment_info
//   struct: extracted documentation associated with a symbol,
//   parsed according to the style guide's brief comment format.
struct d_comment_info
{
    std::string raw_comment;
    std::string brief_comment;
    std::string category_tag;
    std::string description;
    std::string return_doc;

    std::vector<std::pair<std::string, std::string>>
        parameter_docs;
};

// ============================================================
// symbol info
// ============================================================

// d_symbol_info
//   struct: complete catalogue entry for one discovered symbol.
// Contains identity, location, type qualifiers, documentation,
// and dependency information.
struct d_symbol_info
{
    // identity
    int64_t          id;
    std::string      name;
    std::string      qualified_name;
    std::string      usr;
    DSymbolCategory  category;
    DAccessSpecifier access;
    DLinkageKind     linkage;

    // location
    d_source_location definition_loc;
    d_source_location declaration_loc;

    // type info
    std::string type_spelling;
    std::string return_type;
    std::string underlying_type;

    // qualifiers
    bool is_const;
    bool is_static;
    bool is_virtual;
    bool is_pure_virtual;
    bool is_inline;
    bool is_constexpr;
    bool is_noexcept;
    bool is_template;
    bool is_variadic;
    bool is_definition;
    bool is_deprecated;

    // parameters (functions/methods)
    std::vector<d_parameter_info> parameters;

    // template parameters
    std::vector<std::string> template_parameters;

    // documentation
    d_comment_info comment;

    // relationships
    std::string              module_name;
    std::string              parent_usr;
    std::string              parent_name;
    std::set<std::string>    referenced_usrs;
    std::vector<std::string> base_classes;

    // enum-specific
    std::vector<std::pair<std::string, int64_t>>
        enum_constants;

    d_symbol_info();
};

// ============================================================
// module info
// ============================================================

// d_module_info
//   struct: aggregated information about one logical module
//   (typically a header/source pair or a directory grouping).
struct d_module_info
{
    int64_t                        id;
    std::string                    name;
    std::string                    path;
    std::string                    description;
    std::vector<std::string>       header_files;
    std::vector<std::string>       source_files;
    std::vector<std::string>       includes;
    std::vector<std::string>       symbol_usrs;
    std::set<std::string>          depends_on_modules;
    std::map<DSymbolCategory, int> category_counts;

    d_module_info();
};

// ============================================================
// scanner configuration
// ============================================================

// d_scanner_config
//   struct: configures how the scanner traverses and parses
//   the framework.
struct d_scanner_config
{
    std::vector<std::string> include_paths;
    std::vector<std::string> compile_flags;
    std::vector<std::string> file_extensions;
    std::string              framework_root;
    bool                     follow_symlinks;
    bool                     scan_system_headers;
    bool                     extract_macros;
    bool                     verbose;

    d_scanner_config();
};

// ============================================================
// database configuration
// ============================================================

// d_database_config
//   struct: configures the database storage backend.
struct d_database_config
{
    std::string db_path;
    bool        recreate_tables;
    bool        use_transactions;
    int         busy_timeout_ms;

    d_database_config();
};

// ============================================================
// wiki configuration
// ============================================================

// DWikiFormat
//   enum: output format for the wiki generator.
enum DWikiFormat
{
    DWikiFormatMarkdown  = 0,
    DWikiFormatHTML      = 1,
    DWikiFormatMediaWiki = 2
};

// d_wiki_config
//   struct: configures how the wiki is generated from the
//   catalogue database.
struct d_wiki_config
{
    std::string output_directory;
    std::string wiki_title;
    std::string base_url;
    DWikiFormat format;
    bool        generate_index;
    bool        generate_dependency_graphs;
    bool        generate_module_pages;
    bool        generate_symbol_pages;
    bool        include_source_links;

    d_wiki_config();
};

} // namespace d_catalogue

#endif // D_CATALOGUE_COMMON_TYPES_HPP
