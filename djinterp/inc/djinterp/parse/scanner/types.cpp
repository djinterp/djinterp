#include "types.hpp"

namespace d_catalogue {

// ============================================================
// enum-to-string conversions
// ============================================================

/*
d_symbol_category_to_string
  Returns a human-readable string for the given symbol category.

Parameter(s):
  _category: the symbol category to convert.
Return:
  a null-terminated string literal naming the category.
*/
const char*
d_symbol_category_to_string
(
    DSymbolCategory _category
)
{
    // select the string corresponding to the category
    switch (_category)
    {
        case DSymbolCategoryFunction:         return "function";
        case DSymbolCategoryMethod:           return "method";
        case DSymbolCategoryConstructor:      return "constructor";
        case DSymbolCategoryDestructor:       return "destructor";
        case DSymbolCategoryStruct:           return "struct";
        case DSymbolCategoryClass:            return "class";
        case DSymbolCategoryEnum:             return "enum";
        case DSymbolCategoryEnumConstant:     return "enum_constant";
        case DSymbolCategoryTypedef:          return "typedef";
        case DSymbolCategoryTypeAlias:        return "type_alias";
        case DSymbolCategoryVariable:         return "variable";
        case DSymbolCategoryField:            return "field";
        case DSymbolCategoryMacroDefinition:  return "macro_definition";
        case DSymbolCategoryMacroExpansion:   return "macro_expansion";
        case DSymbolCategoryNamespace:        return "namespace";
        case DSymbolCategoryTemplateParam:    return "template_param";
        case DSymbolCategoryConcept:          return "concept";
        case DSymbolCategoryUnion:            return "union";
        case DSymbolCategoryUsingDecl:        return "using_decl";
        case DSymbolCategoryFunctionTemplate: return "function_template";
        case DSymbolCategoryClassTemplate:    return "class_template";
        case DSymbolCategoryUnknown:          return "unknown";
    }

    return "unknown";
}

/*
d_symbol_category_from_string
  Converts a category name string back to its enum value.

Parameter(s):
  _str: the string to parse (e.g. "function", "class").
Return:
  the corresponding DSymbolCategory, or DSymbolCategoryUnknown
  if the string is not recognized.
*/
DSymbolCategory
d_symbol_category_from_string
(
    const std::string& _str
)
{
    if (_str == "function")          { return DSymbolCategoryFunction;         }
    if (_str == "method")            { return DSymbolCategoryMethod;           }
    if (_str == "constructor")       { return DSymbolCategoryConstructor;      }
    if (_str == "destructor")        { return DSymbolCategoryDestructor;       }
    if (_str == "struct")            { return DSymbolCategoryStruct;           }
    if (_str == "class")             { return DSymbolCategoryClass;            }
    if (_str == "enum")              { return DSymbolCategoryEnum;             }
    if (_str == "enum_constant")     { return DSymbolCategoryEnumConstant;     }
    if (_str == "typedef")           { return DSymbolCategoryTypedef;          }
    if (_str == "type_alias")        { return DSymbolCategoryTypeAlias;        }
    if (_str == "variable")          { return DSymbolCategoryVariable;         }
    if (_str == "field")             { return DSymbolCategoryField;            }
    if (_str == "macro_definition")  { return DSymbolCategoryMacroDefinition;  }
    if (_str == "macro_expansion")   { return DSymbolCategoryMacroExpansion;   }
    if (_str == "namespace")         { return DSymbolCategoryNamespace;        }
    if (_str == "template_param")    { return DSymbolCategoryTemplateParam;    }
    if (_str == "concept")           { return DSymbolCategoryConcept;          }
    if (_str == "union")             { return DSymbolCategoryUnion;            }
    if (_str == "using_decl")        { return DSymbolCategoryUsingDecl;        }
    if (_str == "function_template") { return DSymbolCategoryFunctionTemplate; }
    if (_str == "class_template")    { return DSymbolCategoryClassTemplate;    }

    return DSymbolCategoryUnknown;
}

/*
d_access_specifier_to_string
  Returns a human-readable string for the given access specifier.

Parameter(s):
  _access: the access specifier to convert.
Return:
  a null-terminated string literal naming the access level.
*/
const char*
d_access_specifier_to_string
(
    DAccessSpecifier _access
)
{
    switch (_access)
    {
        case DAccessSpecifierPublic:    return "public";
        case DAccessSpecifierProtected: return "protected";
        case DAccessSpecifierPrivate:   return "private";
        case DAccessSpecifierNone:      return "none";
    }

    return "none";
}

/*
d_linkage_kind_to_string
  Returns a human-readable string for the given linkage kind.

Parameter(s):
  _linkage: the linkage kind to convert.
Return:
  a null-terminated string literal naming the linkage.
*/
const char*
d_linkage_kind_to_string
(
    DLinkageKind _linkage
)
{
    switch (_linkage)
    {
        case DLinkageKindInternal: return "internal";
        case DLinkageKindExternal: return "external";
        case DLinkageKindNone:     return "none";
    }

    return "none";
}

// ============================================================
// struct constructors
// ============================================================

/*
d_symbol_info::d_symbol_info
  Default-constructs a symbol_info with all fields zeroed or
  set to sensible defaults.

Parameter(s):
  (none)
Return:
  (constructor)
*/
d_symbol_info::d_symbol_info()
    : id(0)
    , name()
    , qualified_name()
    , usr()
    , category(DSymbolCategoryUnknown)
    , access(DAccessSpecifierNone)
    , linkage(DLinkageKindNone)
    , definition_loc()
    , declaration_loc()
    , type_spelling()
    , return_type()
    , underlying_type()
    , is_const(false)
    , is_static(false)
    , is_virtual(false)
    , is_pure_virtual(false)
    , is_inline(false)
    , is_constexpr(false)
    , is_noexcept(false)
    , is_template(false)
    , is_variadic(false)
    , is_definition(false)
    , is_deprecated(false)
    , parameters()
    , template_parameters()
    , comment()
    , module_name()
    , parent_usr()
    , parent_name()
    , referenced_usrs()
    , base_classes()
    , enum_constants()
{
}

/*
d_module_info::d_module_info
  Default-constructs a module_info.

Parameter(s):
  (none)
Return:
  (constructor)
*/
d_module_info::d_module_info()
    : id(0)
    , name()
    , path()
    , description()
    , header_files()
    , source_files()
    , includes()
    , symbol_usrs()
    , depends_on_modules()
    , category_counts()
{
}

/*
d_scanner_config::d_scanner_config
  Default-constructs scanner configuration with common C/C++
  file extensions and standard flags.

Parameter(s):
  (none)
Return:
  (constructor)
*/
d_scanner_config::d_scanner_config()
    : include_paths()
    , compile_flags()
    , file_extensions({".h", ".hpp", ".c", ".cpp", ".cc", ".cxx"})
    , framework_root()
    , follow_symlinks(false)
    , scan_system_headers(false)
    , extract_macros(true)
    , verbose(false)
{
}

/*
d_database_config::d_database_config
  Default-constructs database configuration.

Parameter(s):
  (none)
Return:
  (constructor)
*/
d_database_config::d_database_config()
    : db_path("catalogue.db")
    , recreate_tables(false)
    , use_transactions(true)
    , busy_timeout_ms(5000)
{
}

/*
d_wiki_config::d_wiki_config
  Default-constructs wiki generation configuration.

Parameter(s):
  (none)
Return:
  (constructor)
*/
d_wiki_config::d_wiki_config()
    : output_directory("wiki_output")
    , wiki_title("Framework Reference")
    , base_url("")
    , format(DWikiFormatMarkdown)
    , generate_index(true)
    , generate_dependency_graphs(true)
    , generate_module_pages(true)
    , generate_symbol_pages(true)
    , include_source_links(true)
{
}

} // namespace d_catalogue
