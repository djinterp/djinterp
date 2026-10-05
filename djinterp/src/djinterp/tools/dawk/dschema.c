/*******************************************************************************
* djinterp [tools]                                                     dschema.c
*
* Definitions for the non-inline declarations in dschema.h.
*   A binding is an array indexed by symbol, sized to the table when bound:
* dispatch is one bounds check and one load.  A symbol interned after the
* binding has no entry, which is right -- no registered property has it.
*
*
* path:      /src/djinterp/tools/dawk/dschema.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dschema.h"  // corresponding header
// std
#include <stdlib.h>  // calloc, free
#include <string.h>  // strcmp, strlen
// djinterp
#include "../../../../inc/djinterp/tools/dawk/dsymbol.h"  // d_symbol_intern


// d_schema_binding
//   struct: entry pointers indexed by symbol.
struct d_schema_binding
{
    const struct d_schema_property**  by_symbol;
    size_t                            limit;
};


const struct d_schema_property*
d_schema_find(
    const struct d_schema* _schema,
    const char*            _name
)
{
    for (size_t at = 0u; (_schema) && (_name) && (at < _schema->count); ++at)
    {
        if (strcmp(_schema->properties[at].name, _name) == 0)
        {
            return &_schema->properties[at];
        }
    }

    return NULL;
}


/*
d_schema_validate
  Walks rule declarations only.  At-rule blocks -- @table rows, @set -- are
host syntax whose "properties" are keys, not properties.
*/
bool
d_schema_validate(
    const struct d_schema*    _schema,
    const struct d_dss_sheet* _sheet,
    uint32_t*                 _out_line,
    const char**              _out_name,
    const char**              _out_why
)
{
    for (size_t at = 0u; (_schema) && (_sheet) &&
                         (at < d_dss_rule_count(_sheet)); ++at)
    {
        const struct d_dss_rule* const rule = d_dss_rule_at(_sheet, at);

        for (uint32_t which = 0u; which < rule->declaration_count; ++which)
        {
            const struct d_dss_declaration* const declaration =
                d_dss_declaration_at(_sheet, rule->first_declaration + which);
            const char* const name = d_dss_text(_sheet,
                                                declaration->property);
            const struct d_schema_property* const entry =
                d_schema_find(_schema, name);
            const char* why = NULL;

            // unknown, or known with a value its check refuses
            if ( (!entry) ||
                 ( (entry->check) &&
                   (!entry->check(_sheet, declaration, &why)) ) )
            {
                if (_out_line)
                {
                    *_out_line = declaration->line;
                }

                if (_out_name)
                {
                    *_out_name = name;
                }

                if (_out_why)
                {
                    *_out_why = entry ? why : "no host registered it";
                }

                return false;
            }
        }
    }

    return true;
}


/*
d_schema_bind
  Interns every registered name, so the array can be sized to cover them.
*/
struct d_schema_binding*
d_schema_bind(
    const struct d_schema* _schema,
    struct d_symbol_table* _symbols
)
{
    if ( (!_schema) || (!_symbols) )
    {
        return NULL;
    }

    for (size_t at = 0u; at < _schema->count; ++at)
    {
        const char* const name = _schema->properties[at].name;

        if (d_symbol_intern(_symbols, name, strlen(name)) == D_SYMBOL_NONE)
        {
            return NULL;
        }
    }

    struct d_schema_binding* const binding = calloc(1u, sizeof(*binding));

    if (!binding)
    {
        return NULL;
    }

    binding->limit     = d_symbol_count(_symbols);
    binding->by_symbol = calloc(binding->limit + 1u,
                                sizeof(const struct d_schema_property*));

    if (!binding->by_symbol)
    {
        free(binding);

        return NULL;
    }

    for (size_t at = 0u; at < _schema->count; ++at)
    {
        const char* const name   = _schema->properties[at].name;
        const uint32_t    symbol = d_symbol_find(_symbols, name, strlen(name));

        binding->by_symbol[symbol] = &_schema->properties[at];
    }

    return binding;
}


void
d_schema_binding_free(
    struct d_schema_binding* _binding
)
{
    if (_binding)
    {
        free((void*)_binding->by_symbol);
        free(_binding);
    }

    return;
}


const struct d_schema_property*
d_schema_lookup(
    const struct d_schema_binding* _binding,
    uint32_t                       _symbol
)
{
    if ( (!_binding) || (_symbol >= _binding->limit) )
    {
        return NULL;
    }

    return _binding->by_symbol[_symbol];
}


/*
d_schema_evaluate
  A property with no evaluate callback is the host's to read: nothing is
evaluated here, exactly as for a modifier before the registry existed.
*/
bool
d_schema_evaluate(
    const struct d_schema_binding*  _binding,
    const struct d_dss_sheet*       _sheet,
    struct d_node_tree*             _tree,
    uint32_t                        _node,
    const struct d_dss_declaration* _declaration,
    bool*                           _out_evaluated
)
{
    const struct d_schema_property* const entry =
        _declaration ? d_schema_lookup(_binding, _declaration->symbol) : NULL;

    if ( (!entry) || (!entry->evaluate) )
    {
        if (_out_evaluated)
        {
            *_out_evaluated = false;
        }

        return true;
    }

    return entry->evaluate(_sheet, _tree, _node, _declaration,
                           _out_evaluated);
}
