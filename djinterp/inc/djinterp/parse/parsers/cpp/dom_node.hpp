/******************************************************************************
* djinterp [dom]                                              dom_node.hpp
*
* Base DOM Node Payload:
*   This header defines the foundational payload struct for a DOM-style
* representation of parsed C or C++ source code.  The dom_node is the
* arena_tree payload — it carries the identity, kind, qualifiers,
* source location, and string references for one declaration.
*
* Design:
*   - Fixed-size, value-semantic.  Safe for arena storage.
*   - String references are uint32_t indices into a dom_string_table.
*     The table is owned by the tree (dom_tree_base), not the node.
*   - All fields common to both C and C++ are defined here.
*     Language-specific payloads (cpp_dom_node) inherit and extend.
*   - Qualifiers use the uint64_t bitfield from lang/common.hpp so
*     that the bitwise predicates (is_const, is_static, etc.) work
*     directly on the node's qualifier field.
*
* Contents:
*   I.    dom_string_id / null_string
*   II.   dom_string_table
*   III.  dom_node
*
* Dependencies:
*   lang/common.hpp — symbol_kind, qualifier, access_specifier
*
*
* path:      /inc/cpp/dom/dom_node.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.16
******************************************************************************/

#ifndef DJINTERP_DOM_NODE_
#define DJINTERP_DOM_NODE_ 1

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "../core/djinterp.hpp"
#include "../lang/common.hpp"


NS_DJINTERP


// =============================================================================
// I.   dom_string_id / null_string
// =============================================================================

// dom_string_id
//   type: index into a dom_string_table.  Zero is the null
// sentinel (empty / unset).
using dom_string_id = std::uint32_t;

// D_DOM_NULL_STRING
//   constant: sentinel string ID meaning "no string".
constexpr dom_string_id D_DOM_NULL_STRING = 0;


// =============================================================================
// II.  dom_string_table
// =============================================================================

// dom_string_table
//   class: simple string interning table.  Maps strings to
// compact integer IDs for storage in dom_node payloads.
// Lifetime-coupled to the owning dom_tree_base — all string
// IDs are valid as long as the table exists.
//
//   Slot 0 is permanently reserved as the empty/null string.
class dom_string_table
{
public:
    dom_string_table()
        : m_strings(1, "")
        , m_index()
    {
    }

    // intern
    //   method: returns the ID for _str, inserting it if this
    // is the first time it has been seen.
    dom_string_id
    intern(const std::string& _str)
    {
        if (_str.empty())
        {
            return D_DOM_NULL_STRING;
        }

        auto it = m_index.find(_str);

        if (it != m_index.end())
        {
            return it->second;
        }

        dom_string_id id =
            static_cast<dom_string_id>(m_strings.size());
        m_strings.push_back(_str);
        m_index.emplace(_str, id);

        return id;
    }

    // resolve
    //   method: returns the string for the given ID.
    // ID 0 returns the empty string.
    const std::string&
    resolve(dom_string_id _id) const
    {
        if (_id >= m_strings.size())
        {
            return m_strings[0];
        }

        return m_strings[_id];
    }

    // size
    //   method: returns the total number of interned strings
    // (including the null slot).
    std::size_t
    size() const
    {
        return m_strings.size();
    }

    // clear
    //   method: resets the table to its initial state (null
    // slot only).
    void
    clear()
    {
        m_strings.clear();
        m_strings.push_back("");
        m_index.clear();

        return;
    }

private:
    std::vector<std::string>                         m_strings;
    std::unordered_map<std::string, dom_string_id>   m_index;
};


// =============================================================================
// III. dom_node
// =============================================================================

// dom_node
//   struct: base DOM node payload for parsed C/C++ source code.
// Contains all fields common to both languages.  Fixed-size and
// value-semantic for arena storage.
//
//   String fields are dom_string_id indices into the owning
// tree's dom_string_table.  The node does not own strings.
struct dom_node
{
    // ---- identity ----

    // kind
    //   field: symbol_kind value classifying this declaration.
    std::uint16_t   kind;

    // access
    //   field: access_specifier value (public/protected/private).
    std::uint8_t    access;

    // storage
    //   field: packed storage-class flags (see cpp_storage_flag).
    std::uint8_t    storage;

    // qualifiers
    //   field: bitfield combining cv-qualifiers, storage class,
    // and language-specific modifiers.  Use the predicates from
    // common.hpp / cpp.hpp (is_const, is_virtual, etc.) directly
    // on this field.
    std::uint64_t   qualifiers;

    // stable_id
    //   field: persistent identity hash for diffing across
    // parses.  Matches arena_node::stable_id semantics.
    std::uint64_t   stable_id;


    // ---- string references ----

    // name
    //   field: the unqualified name of the declaration.
    // D_DOM_NULL_STRING for anonymous entities.
    dom_string_id   name;

    // type_spelling
    //   field: textual representation of the declared type
    // (e.g. "const int*", "std::vector<int>").
    dom_string_id   type_spelling;

    // comment
    //   field: raw documentation comment text.
    dom_string_id   comment;


    // ---- source location ----

    // file
    //   field: string ID of the source file path.
    dom_string_id   file;

    // line
    //   field: 1-based source line.  0 = unknown.
    std::uint32_t   line;

    // column
    //   field: 1-based source column.  0 = unknown.
    std::uint32_t   column;


    // ---- flags ----

    // is_definition
    //   field: true if this node is a definition (as opposed
    // to a forward declaration).
    bool            is_definition;


    // ============================================================
    //  construction
    // ============================================================

    dom_node()
        : kind(lang::symbol_kind::unknown)
        , access(lang::access_specifier::unspecified)
        , storage(0)
        , qualifiers(lang::qualifier::none)
        , stable_id(0)
        , name(D_DOM_NULL_STRING)
        , type_spelling(D_DOM_NULL_STRING)
        , comment(D_DOM_NULL_STRING)
        , file(D_DOM_NULL_STRING)
        , line(0)
        , column(0)
        , is_definition(false)
    {
    }

    dom_node(std::uint16_t  _kind,
             dom_string_id  _name)
        : kind(_kind)
        , access(lang::access_specifier::unspecified)
        , storage(0)
        , qualifiers(lang::qualifier::none)
        , stable_id(0)
        , name(_name)
        , type_spelling(D_DOM_NULL_STRING)
        , comment(D_DOM_NULL_STRING)
        , file(D_DOM_NULL_STRING)
        , line(0)
        , column(0)
        , is_definition(false)
    {
    }
};


NS_END  // djinterp


#endif  // DJINTERP_DOM_NODE_
