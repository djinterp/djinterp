/******************************************************************************
* djinterp [parsegen]                                              registry.c
*
*   Definitions for the non-inline declarations in registry.h.
*
*
* path:      /src/djinterp/parsegen/registry.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../inc/djinterp/parsegen/registry.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <string.h>  // memset, strcmp
// djinterp
#include "../../../inc/djinterp/parse/storage.h"  // d_parse_grow, the shared
                                                  // growth policy
#if (D_INTERNAL_PARSEGEN_REGISTRY_HEAP == 1)
#include <stdlib.h>  // malloc, free
#endif  // D_INTERNAL_PARSEGEN_REGISTRY_HEAP


// D_INTERNAL_REGISTRY_REASON
//   macro: the size of the buffer a selection failure explains itself in.
#define D_INTERNAL_REGISTRY_REASON  256u

// D_INTERNAL_REGISTRY_CAUSE
//   macro: the size of the buffer one candidate's ruling-out capability is
// rendered in. Smaller than the note it is folded into, so composing the two
// is provably within bounds rather than merely usually so.
#define D_INTERNAL_REGISTRY_CAUSE   160u


/*
d_parsegen_registry_init
  Initialises a registry over a caller-supplied table.
NOTE:
  A static array is the usual shape: a build links in the stages it wants and
registers them once, so nothing here need allocate at all.

Parameter(s):
  _registry: the registry to initialise; ignored if NULL.
  _stages:   storage for the stage table; may be NULL for an empty registry.
  _capacity: how many stages _stages holds.
Return:
  none.
*/
void
d_parsegen_registry_init(
    struct d_parsegen_registry* _registry,
    struct d_parsegen_stage*    _stages,
    uint32_t                    _capacity
)
{
    if (!_registry)
    {
        return;
    }

    memset(_registry, 0, sizeof(*_registry));

    _registry->stages   = _stages;
    _registry->capacity = (_stages != NULL) ? _capacity : 0u;

    // clear the table, so an unregistered slot cannot be mistaken for a stage
    if ( (_stages != NULL) &&
         (_capacity > 0u) )
    {
        memset(_stages, 0, (size_t)_capacity * sizeof(*_stages));
    }

    return;
}


#if (D_INTERNAL_PARSEGEN_REGISTRY_HEAP == 1)

/*
d_parsegen_registry_init_heap
  Initialises a registry over a table it allocates and owns, which then grows
on demand.

Parameter(s):
  _registry: the registry to initialise; ignored if NULL.
  _capacity: stages to reserve room for; 0 reserves nothing and defers the
             first allocation to the first registration.
Return:
  0 on success; -1 if _registry is NULL or the allocation was refused.
*/
int
d_parsegen_registry_init_heap(
    struct d_parsegen_registry* _registry,
    uint32_t                    _capacity
)
{
    if (!_registry)
    {
        return -1;
    }

    d_parsegen_registry_init(_registry, NULL, 0u);

    _registry->flags = (uint8_t)D_PARSEGEN_REGISTRY_OWNS_STAGES;

    // an empty reservation is valid; the first registration will allocate
    if (_capacity == 0u)
    {
        return 0;
    }

    struct d_parsegen_stage* table =
        (struct d_parsegen_stage*)malloc((size_t)_capacity * sizeof(*table));

    // check if memory allocation was successful
    if (!table)
    {
        return -1;
    }

    memset(table, 0, (size_t)_capacity * sizeof(*table));

    _registry->stages   = table;
    _registry->capacity = _capacity;

    return 0;
}

#endif  // D_INTERNAL_PARSEGEN_REGISTRY_HEAP


/*
d_parsegen_registry_release
  Releases any table the registry owns and leaves it empty.
CAUTION:
  A registry holds stages by value but their names, summaries, and function
tables by pointer. Those belong to whoever registered them and must outlive the
registry; releasing it frees nothing of theirs.

Parameter(s):
  _registry: the registry to release; ignored if NULL.
Return:
  none.
*/
void
d_parsegen_registry_release(
    struct d_parsegen_registry* _registry
)
{
    if (!_registry)
    {
        return;
    }

#if (D_INTERNAL_PARSEGEN_REGISTRY_HEAP == 1)
    // free only what this registry allocated; caller storage is never touched
    if ( (_registry->stages != NULL) &&
         ((_registry->flags & D_PARSEGEN_REGISTRY_OWNS_STAGES) != 0u) )
    {
        free(_registry->stages);
    }
#endif  // D_INTERNAL_PARSEGEN_REGISTRY_HEAP

    memset(_registry, 0, sizeof(*_registry));

    return;
}


/*
d_parsegen_registry_add
  Registers a stage.
NOTE:
  A stage is copied by value, so the caller may register from a temporary; the
pointers inside it are not, so the name, summary, and function table must
outlive the registry. Registering the same kind and name twice replaces the
earlier entry, which is what lets a build override a framework stage with its
own without unregistering anything.

Parameter(s):
  _registry: the registry to register into; may be NULL.
  _stage:    the stage to register; may be NULL. Must carry a name and a kind.
Return:
  0 on success; -1 on an invalid argument or when the table cannot grow.
*/
int
d_parsegen_registry_add(
    struct d_parsegen_registry*    _registry,
    const struct d_parsegen_stage* _stage
)
{
    // reject a missing registry, a missing stage, and a stage with no name --
    // a nameless stage could never be found, named, or explained
    if ( (!_registry)     ||
         (!_stage)        ||
         (!_stage->name)  ||
         (_stage->name[0] == '\0') )
    {
        return -1;
    }

    // replace an existing registration of the same kind and name, so a build
    // may override a framework stage simply by registering after it
    for (uint32_t index = 0u; index < _registry->count; index++)
    {
        if ( (_registry->stages[index].kind == _stage->kind) &&
             (strcmp(_registry->stages[index].name, _stage->name) == 0) )
        {
            _registry->stages[index] = *_stage;

            return 0;
        }
    }

    // make room, or refuse
    if (_registry->count >= _registry->capacity)
    {
#if (D_INTERNAL_PARSEGEN_REGISTRY_HEAP == 1)
        void* const grown =
            d_parse_grow(_registry->stages,
                         &_registry->capacity,
                         _registry->count + 1u,
                         (uint32_t)sizeof(*_registry->stages),
                         (_registry->flags &
                          D_PARSEGEN_REGISTRY_OWNS_STAGES) != 0u);

        // check if memory allocation was successful
        if (!grown)
        {
            return -1;
        }

        _registry->stages = (struct d_parsegen_stage*)grown;
#else
        return -1;
#endif  // D_INTERNAL_PARSEGEN_REGISTRY_HEAP
    }

    _registry->stages[_registry->count] = *_stage;
    _registry->count++;

    return 0;
}


/*
d_parsegen_registry_count_of
  How many stages of one kind are registered.

Parameter(s):
  _registry: the registry to read; may be NULL.
  _kind:     the stage kind to count.
Return:
  The count, which is 0 for a missing registry.
*/
uint32_t
d_parsegen_registry_count_of(
    const struct d_parsegen_registry* _registry,
    uint32_t                          _kind
)
{
    if (!_registry)
    {
        return 0u;
    }

    uint32_t total = 0u;

    for (uint32_t index = 0u; index < _registry->count; index++)
    {
        if (_registry->stages[index].kind == _kind)
        {
            total++;
        }
    }

    return total;
}


/*
d_parsegen_registry_render
  Writes the names of every stage of one kind, separated by ", ".
NOTE:
  This is the second half of a useful "no such stage" diagnostic: telling a
caller that `abnf` is not registered is only half an answer, and the other half
is which frontends are.

Parameter(s):
  _registry: the registry to read; may be NULL.
  _kind:     the stage kind to list.
  _out:      the buffer to write into; may be NULL when _size is 0.
  _size:     the size of _out in bytes, including the terminator.
Return:
  The number of characters the full listing would occupy, excluding the
terminator.
*/
size_t
d_parsegen_registry_render(
    const struct d_parsegen_registry* _registry,
    uint32_t                          _kind,
    char*                             _out,
    size_t                            _size
)
{
    size_t needed    = 0u;
    char*  cursor    = _out;
    size_t remaining = _size;
    int    first     = 1;

    if (!_registry)
    {
        const int written = snprintf(_out, _size, "none");

        return (written < 0) ? 0u : (size_t)written;
    }

    for (uint32_t index = 0u; index < _registry->count; index++)
    {
        const struct d_parsegen_stage* const stage = &_registry->stages[index];

        if (stage->kind != _kind)
        {
            continue;
        }

        const int written = snprintf(cursor,
                                     remaining,
                                     "%s%s",
                                     first ? "" : ", ",
                                     stage->name);
        const size_t grew = (written < 0) ? 0u : (size_t)written;

        first   = 0;
        needed += grew;

        // keep counting after the buffer fills, so the return stays correct
        if (grew < remaining)
        {
            cursor    += grew;
            remaining -= grew;
        }
        else
        {
            cursor    = NULL;
            remaining = 0u;
        }
    }

    // a kind with nothing registered has a name of its own
    if (first)
    {
        const int written = snprintf(_out, _size, "none");

        return (written < 0) ? 0u : (size_t)written;
    }

    return needed;
}


/*
d_parsegen_registry_find
  The stage of one kind registered under a name.
NOTE:
  This is the caller saying which stage it wants. When there is no such stage
the diagnostic lists the ones there are, because the next thing the caller
needs is the spelling it should have used.

Parameter(s):
  _registry: the registry to search; may be NULL.
  _kind:     the stage kind to search.
  _name:     the name to match exactly; may be NULL.
  _diag:     the sink to explain a failure through; may be NULL.
Return:
  The stage, or NULL when none matches.
*/
const struct d_parsegen_stage*
d_parsegen_registry_find(
    const struct d_parsegen_registry* _registry,
    uint32_t                          _kind,
    const char*                       _name,
    struct d_parse_diag_sink*         _diag
)
{
    if ( (!_registry) ||
         (!_name)     )
    {
        return NULL;
    }

    for (uint32_t index = 0u; index < _registry->count; index++)
    {
        const struct d_parsegen_stage* const stage = &_registry->stages[index];

        if ( (stage->kind == _kind) &&
             (strcmp(stage->name, _name) == 0) )
        {
            return stage;
        }
    }

    char available[D_INTERNAL_REGISTRY_CAUSE];

    (void)d_parsegen_registry_render(_registry,
                                     _kind,
                                     available,
                                     sizeof(available));

    char message[D_INTERNAL_REGISTRY_REASON];

    (void)snprintf(message,
                   sizeof(message),
                   "no stage named '%s' is registered; available: %s",
                   _name,
                   available);

    (void)d_parse_diag_emit(_diag,
                            (int)D_PARSE_SEVERITY_ERROR,
                            (uint16_t)D_PARSEGEN_DIAG_DOMAIN_FRONTEND,
                            (uint16_t)0,
                            d_parse_span_unknown(),
                            message);

    return NULL;
}


/*
d_parsegen_registry_select
  The stage of one kind best suited to a capability profile.
NOTE:
  Registration order decides between stages that all accept, unless one is
marked default -- so a build's choice is a declaration rather than an accident
of link order. An experimental stage is never selected; it must be named.
  When nothing accepts, every candidate is reported with the reason it was
ruled out. That listing is the actual product of this function: "no family
accepts this grammar" is not an answer, and "peg rejects LEFT_RECURSION; lr
needs TOKEN_STREAM" is.

Parameter(s):
  _registry: the registry to search; may be NULL.
  _kind:     the stage kind to select from.
  _features: what the grammar uses.
  _diag:     the sink to explain a failure through; may be NULL.
Return:
  The selected stage, or NULL when none accepts.
*/
const struct d_parsegen_stage*
d_parsegen_registry_select(
    const struct d_parsegen_registry* _registry,
    uint32_t                          _kind,
    d_parsegen_features               _features,
    struct d_parse_diag_sink*         _diag
)
{
    if (!_registry)
    {
        return NULL;
    }

    const struct d_parsegen_stage* chosen = NULL;

    for (uint32_t index = 0u; index < _registry->count; index++)
    {
        const struct d_parsegen_stage* const stage = &_registry->stages[index];

        if ( (stage->kind != _kind) ||
             ((stage->flags & D_PARSEGEN_STAGE_EXPERIMENTAL) != 0u) )
        {
            continue;
        }

        if (!d_parsegen_stage_accepts(stage, _features))
        {
            continue;
        }

        // a stage marked default wins outright; otherwise the first to accept
        // holds the place
        if ((stage->flags & D_PARSEGEN_STAGE_DEFAULT) != 0u)
        {
            return stage;
        }

        if (!chosen)
        {
            chosen = stage;
        }
    }

    if (chosen)
    {
        return chosen;
    }

    // nothing accepted: say what was tried and what stopped each candidate
    char wanted[D_INTERNAL_REGISTRY_CAUSE];

    (void)d_parsegen_features_render(_features, wanted, sizeof(wanted));

    char message[D_INTERNAL_REGISTRY_REASON];

    (void)snprintf(message,
                   sizeof(message),
                   "no stage accepts a grammar using %s",
                   wanted);

    (void)d_parse_diag_emit(_diag,
                            (int)D_PARSE_SEVERITY_ERROR,
                            (uint16_t)D_PARSEGEN_DIAG_DOMAIN_FAMILY,
                            (uint16_t)0,
                            d_parse_span_unknown(),
                            message);

    for (uint32_t index = 0u; index < _registry->count; index++)
    {
        const struct d_parsegen_stage* const stage = &_registry->stages[index];

        if (stage->kind != _kind)
        {
            continue;
        }

        char reason[D_INTERNAL_REGISTRY_CAUSE];

        const d_parsegen_features blocked = _features & stage->rejects;
        const d_parsegen_features absent  =
            d_parsegen_features_missing(_features, stage->needs);

        // name the specific capability that ruled this candidate out, since
        // that is what the caller has to act on
        if (blocked != D_PARSEGEN_FEATURE_NONE)
        {
            (void)d_parsegen_features_render(blocked,
                                             reason,
                                             sizeof(reason));
        }
        else if (absent != D_PARSEGEN_FEATURE_NONE)
        {
            (void)d_parsegen_features_render(absent, reason, sizeof(reason));
        }
        else
        {
            (void)snprintf(reason, sizeof(reason), "experimental");
        }

        char note[D_INTERNAL_REGISTRY_REASON];

        (void)snprintf(note,
                       sizeof(note),
                       "%s: %s %s",
                       stage->name,
                       (blocked != D_PARSEGEN_FEATURE_NONE)
                       ? "rejects"
                       : ((absent != D_PARSEGEN_FEATURE_NONE)
                          ? "needs"
                          : "is"),
                       reason);

        // a continuation, so a renderer shows it under the error it explains
        // rather than as a problem of its own
        (void)d_parse_diag_emit_flagged(
            _diag,
            (int)D_PARSE_SEVERITY_NOTE,
            (uint16_t)D_PARSEGEN_DIAG_DOMAIN_FAMILY,
            (uint16_t)0,
            (uint8_t)D_PARSE_DIAG_FLAG_CONTINUATION,
            d_parse_span_unknown(),
            note);
    }

    return NULL;
}
