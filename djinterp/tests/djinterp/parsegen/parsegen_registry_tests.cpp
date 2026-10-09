/*******************************************************************************
* djinterp [parsegen]                                parsegen_registry_tests.cpp
*
* The stage-registry suite's sections: the capability vocabulary, the registry
* that selects a stage by it, and the growth policy the parse containers
* share.
*   Selection never invokes a stage, so every stage here is a declaration and
* nothing more: a name, a kind, the capabilities it provides, needs and
* rejects, and a table pointer the registry stores without looking inside.
*
*
* path:      /tests/djinterp/parsegen/parsegen_registry_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./parsegen_registry_tests.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <cstring>  // std::strcmp, std::strstr
// djinterp
#include "../../../inc/djinterp/parse/diagnostic.hpp"   // fixed_diagnostics
#include "../../../inc/djinterp/parse/machine.hpp"      // op_set, machine
#include "../../../inc/djinterp/parsegen/registry.hpp"  // the registry's face
// re_std
#include "../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::uint32_t


namespace
{

using djinterp::parse::fixed_diagnostics;
using namespace djinterp::parsegen;

// toy_table
//   struct: the vtable a stage of this kind would carry. The registry stores
// it as an opaque pointer and never looks inside.
struct toy_table
{
    int marker;
};

const toy_table PEG_TABLE = { 1 };
const toy_table LR_TABLE  = { 2 };
const toy_table GLR_TABLE = { 3 };

// PEG_NOTATION
//   constant: what a grammar written in PEG's notation uses.
const feature_set PEG_NOTATION = feature::ordered_choice
                               | feature::character_class
                               | feature::syntactic_predicate
                               | feature::capture;

// BNF_NOTATION
//   constant: what a grammar written in BNF uses.
const feature_set BNF_NOTATION = feature::unordered_choice
                               | feature::empty_production
                               | feature::literal_string;

// ORDERED
//   constant: a grammar the PEG family can run.
const feature_set ORDERED = feature::ordered_choice
                          | feature::character_class;

// RECURSIVE
//   constant: a grammar it cannot.
const feature_set RECURSIVE = feature::unordered_choice
                            | feature::left_recursion;

// make_stage
//   function: a stage of a kind, by name, providing, needing and rejecting
// nothing until its caller says otherwise.
stage
make_stage(
    const char* _name,
    stage_kind  _kind
)
{
    stage built = {};

    built.name    = _name;
    built.summary = "";
    built.kind    = static_cast<re_std::uint32_t>(_kind);

    return built;
}

// peg_family
//   function: a PEG family -- it handles ordered choice and predicates and
// cannot do left recursion, which is invariant 4 stated as a declaration.
stage
peg_family()
{
    stage built = make_stage("peg", stage_kind::family);

    built.provides = PEG_NOTATION;
    built.rejects  = feature::left_recursion | feature::unordered_choice;
    built.table    = &PEG_TABLE;

    return built;
}

// lr_family
//   function: an LR family -- it handles what PEG rejects, and rejects the
// predicates PEG handles.
stage
lr_family()
{
    stage built = make_stage("lr", stage_kind::family);

    built.provides = RECURSIVE | feature::precedence;
    built.rejects  = feature_set(feature::syntactic_predicate);
    built.table    = &LR_TABLE;

    return built;
}

// glr_family
//   function: an experimental family that accepts everything LR does and
// rejects nothing.
stage
glr_family()
{
    stage built = make_stage("glr", stage_kind::family);

    built.provides = RECURSIVE | feature::ambiguity;
    built.table    = &GLR_TABLE;
    built.flags    = D_PARSEGEN_STAGE_EXPERIMENTAL;

    return built;
}

// frontend
//   function: a frontend providing a notation.
stage
frontend(
    const char*        _name,
    const feature_set& _notation
)
{
    stage built = make_stage(_name, stage_kind::frontend);

    built.provides = _notation;

    return built;
}

// populate
//   function: the registry most sections start from -- the PEG family and two
// frontends, so that filtering by kind has something to filter.
bool
populate(
    registry& _registry
)
{
    return ( (_registry.add(peg_family()))                              &&
             (_registry.add(frontend("peg-notation", PEG_NOTATION)))    &&
             (_registry.add(frontend("bnf", BNF_NOTATION)))             );
}

// chose
//   function: whether a selection settled on the stage of a given name.
bool
chose(
    const stage* _chosen,
    const char*  _name
)
{
    return ( (_chosen != nullptr)                       &&
             (std::strcmp(_chosen->name, _name) == 0)   );
}

/*
d_tests_parsegen_features_algebra
  The capability vocabulary as a set.
  Tests the following:
  - ordered and unordered choice are distinct bits with no default between them
  - containment, and the missing-bits difference
*/
bool
d_tests_parsegen_features_algebra(void)
{
    const feature_set wanted = feature::ordered_choice | feature::capture;
    const feature_set absent = PEG_NOTATION.missing(
        feature::ordered_choice | feature::left_recursion);

    // the distinction the whole vocabulary exists to protect: PEG's bar and
    // BNF's bar are not the same operator, and neither is the default
    if ( (PEG_NOTATION.any(feature::unordered_choice))  ||
         (BNF_NOTATION.any(feature::ordered_choice))    ||
         (!PEG_NOTATION.has(feature::ordered_choice))   )
    {
        return false;
    }

    return ( (PEG_NOTATION.has(wanted))                 &&
             (absent.has(feature::left_recursion))      &&
             (!absent.any(feature::ordered_choice))     );
}

/*
d_tests_parsegen_features_render
  What a diagnostic shows a reader for a capability set.
  Tests the following:
  - a set renders as its named bits
  - the empty set has a name, so a diagnostic never prints nothing
  - a bit above the reserved base renders without colliding with a named one
*/
bool
d_tests_parsegen_features_render(void)
{
    const feature_set wanted = feature::ordered_choice | feature::capture;
    const feature_set nothing;
    const feature_set custom(D_PARSEGEN_FEATURE_USER);
    char              text[256];

    (void)wanted.render(text, sizeof(text));

    if ( (!std::strstr(text, "ORDERED_CHOICE"))  ||
         (!std::strstr(text, "CAPTURE"))         )
    {
        return false;
    }

    (void)nothing.render(text, sizeof(text));

    if (std::strcmp(text, "none") != 0)
    {
        return false;
    }

    (void)custom.render(text, sizeof(text));

    return (std::strstr(text, "USER+0") != nullptr);
}

/*
d_tests_parsegen_registry_kinds
  Stages of several kinds in one table.
  Tests the following:
  - a family and two frontends live in one registry
  - they are counted by kind, and a kind with none counts zero
*/
bool
d_tests_parsegen_registry_kinds(void)
{
    fixed_registry<8u> reg;

    return ( (populate(reg))                                 &&
             (reg.size() == 3u)                              &&
             (reg.count_of(stage_kind::family) == 1u)        &&
             (reg.count_of(stage_kind::frontend) == 2u)      &&
             (reg.count_of(stage_kind::backend) == 0u)       );
}

/*
d_tests_parsegen_registry_select
  Selection as set arithmetic: nothing rejected, everything needed.
  Tests the following:
  - a grammar the family can run selects it, and its table comes back with it
  - a grammar it cannot is refused, with one error and a continuation note
    under it for the candidate that was ruled out
  - the text names the capability asked for and the clause that ruled, which
    is what the reader needs; the identity is the domain and code
*/
bool
d_tests_parsegen_registry_select(void)
{
    fixed_diagnostics<32u, 2048u> diag;
    fixed_registry<8u>            reg;

    if (!populate(reg))
    {
        return false;
    }

    const stage* const chosen = reg.select(stage_kind::family, ORDERED, &diag);

    if ( (!chose(chosen, "peg"))           ||
         (chosen->table != &PEG_TABLE)     ||
         (diag.failed())                   )
    {
        return false;
    }

    // the failure names the capability, not merely the fact of failure
    if ( (reg.select(stage_kind::family, RECURSIVE, &diag))  ||
         (!diag.failed())                                    ||
         (diag.size() < 2u)                                  )
    {
        return false;
    }

    return ( (diag[0].is(D_PARSEGEN_DIAG_DOMAIN_REGISTRY,
                         D_PARSEGEN_DIAG_REGISTRY_NONE_ACCEPTS))     &&
             (diag[1].is(D_PARSEGEN_DIAG_DOMAIN_REGISTRY,
                         D_PARSEGEN_DIAG_REGISTRY_REJECTED))         &&
             (diag[1].continuation())                                &&
             (std::strstr(diag[0].text(), "LEFT_RECURSION"))         &&
             (std::strstr(diag[1].text(), "peg"))                    &&
             (std::strstr(diag[1].text(), "rejects"))                );
}

/*
d_tests_parsegen_registry_extend
  Adding a family is a registration, not an edit to the chooser.
  Tests the following:
  - a query no family accepted succeeds once a family that accepts it is
    registered, with nothing that chose between the others touched
  - the grammar the first family ran still selects the first family
*/
bool
d_tests_parsegen_registry_extend(void)
{
    fixed_diagnostics<32u, 2048u> diag;
    fixed_registry<8u>            reg;

    if ( (!populate(reg))                                      ||
         (reg.select(stage_kind::family, RECURSIVE, nullptr))  ||
         (!reg.add(lr_family()))                               )
    {
        return false;
    }

    const stage* const now = reg.select(stage_kind::family, RECURSIVE, &diag);

    if ( (!chose(now, "lr"))           ||
         (now->table != &LR_TABLE)     ||
         (diag.failed())               )
    {
        return false;
    }

    // lr rejects predicates and peg was registered first
    return chose(reg.select(stage_kind::family, ORDERED, &diag), "peg");
}

/*
d_tests_parsegen_registry_flags
  What a stage's flags do to selection.
  Tests the following:
  - an experimental stage is never selected automatically
  - it can still be asked for by name
  - registering the same kind and name again overrides rather than duplicates
  - a stage marked default wins over one that merely accepts first
*/
bool
d_tests_parsegen_registry_flags(void)
{
    fixed_diagnostics<32u, 2048u> diag;
    fixed_registry<8u>            reg;
    stage                         earlier   =
        make_stage("earley", stage_kind::family);
    stage                         preferred = lr_family();

    earlier.provides = RECURSIVE;
    preferred.flags  = D_PARSEGEN_STAGE_DEFAULT;

    if ( (!populate(reg))            ||
         (!reg.add(glr_family()))    ||
         (!reg.add(earlier))         ||
         (!reg.add(lr_family()))     )
    {
        return false;
    }

    const stage* const named = reg.find(stage_kind::family, "glr", &diag);

    // glr was registered first and accepts, yet it is passed over; of the two
    // that remain, the one registered first is chosen
    if ( (!chose(reg.select(stage_kind::family, RECURSIVE, &diag),
                 "earley"))                                         ||
         (!named)                                                   ||
         (named->table != &GLR_TABLE)                               ||
         (diag.failed())                                            )
    {
        return false;
    }

    // the same kind and name again: four families still, not five -- and the
    // one now marked default wins over the one that merely accepts first
    return ( (reg.add(preferred))                                         &&
             (reg.count_of(stage_kind::family) == 4u)                     &&
             (chose(reg.select(stage_kind::family, RECURSIVE, &diag),
                    "lr"))                                                );
}

/*
d_tests_parsegen_registry_needs
  The needs-clause, the other half of the selection rule.
  Tests the following:
  - a backend that needs a token stream is passed over for a byte grammar,
    and the note says which clause ruled and names the capability
  - it is chosen once the grammar is over tokens
*/
bool
d_tests_parsegen_registry_needs(void)
{
    fixed_diagnostics<32u, 2048u> diag;
    fixed_registry<8u>            reg;
    stage                         backend =
        make_stage("token-backend", stage_kind::backend);

    backend.needs = feature_set(feature::token_stream);

    if ( (!reg.add(backend))                                 ||
         (reg.select(stage_kind::backend, ORDERED, &diag))   ||
         (!diag[1].is(D_PARSEGEN_DIAG_DOMAIN_REGISTRY,
                      D_PARSEGEN_DIAG_REGISTRY_REJECTED))    ||
         (!std::strstr(diag[1].text(), "needs"))             ||
         (!std::strstr(diag[1].text(), "TOKEN_STREAM"))      )
    {
        return false;
    }

    diag.clear();

    return chose(reg.select(stage_kind::backend,
                            ORDERED | feature::token_stream,
                            &diag),
                 "token-backend");
}

/*
d_tests_parsegen_registry_find
  A lookup by name that finds nothing.
  Tests the following:
  - the failure is identified by the registry's domain and its own code
  - it names what was asked for and lists what is registered, since the next
    thing the caller needs is the spelling it should have used
*/
bool
d_tests_parsegen_registry_find(void)
{
    fixed_diagnostics<32u, 2048u> diag;
    fixed_registry<8u>            reg;

    if (!populate(reg))
    {
        return false;
    }

    return ( (!reg.find(stage_kind::frontend, "abnf", &diag))          &&
             (diag[0].is(D_PARSEGEN_DIAG_DOMAIN_REGISTRY,
                         D_PARSEGEN_DIAG_REGISTRY_NO_SUCH_STAGE))      &&
             (std::strstr(diag[0].text(), "abnf"))                     &&
             (std::strstr(diag[0].text(), "bnf"))                      );
}

/*
d_tests_parsegen_storage_fixed
  The growth policy, over caller-supplied storage.
  Tests the following:
  - a registry over a fixed table takes what fits
  - it refuses past its capacity rather than reallocating or overrunning
*/
bool
d_tests_parsegen_storage_fixed(void)
{
    fixed_registry<1u> tiny;

    return ( (tiny.add(make_stage("one", stage_kind::pass)))     &&
             (!tiny.add(make_stage("two", stage_kind::pass)))    );
}

#if (D_INTERNAL_PARSEGEN_REGISTRY_HEAP == 1)
/*
d_tests_parsegen_storage_grown
  The growth policy, over storage the container owns.
  Tests the following:
  - a registry reserved for one stage grows to hold forty
  - everything already in it is kept, in order, across every growth
*/
bool
d_tests_parsegen_storage_grown(void)
{
    static char names[40][4];
    registry    grown;

    if (!grown.reserve(1u))
    {
        return false;
    }

    // forty stages named s00 to s39, each added past the last capacity
    for (int index = 0; index < 40; index++)
    {
        names[index][0] = 's';
        names[index][1] = static_cast<char>('0' + (index / 10));
        names[index][2] = static_cast<char>('0' + (index % 10));
        names[index][3] = '\0';

        if (!grown.add(make_stage(names[index], stage_kind::pass)))
        {
            return false;
        }
    }

    return ( (grown.size() == 40u)                                  &&
             (std::strcmp(grown.begin()->name, "s00") == 0)         &&
             (std::strcmp((grown.end() - 1)->name, "s39") == 0)     );
}
#endif  // D_INTERNAL_PARSEGEN_REGISTRY_HEAP

#if (D_INTERNAL_PARSE_OP_SET_HEAP == 1)
// idle_op
//   struct: an operator that does nothing, so a registry has something to
// register.
struct idle_op
{
    void
    operator()(
        djinterp::parse::machine& _machine
    ) const
    {
        (void)_machine;

        return;
    }
};

/*
d_tests_parsegen_storage_sparse
  The growth policy, in the operator registry that shares it.
  Tests the following:
  - an opcode defined far past the initial capacity grows the table to reach
  - growth zeroes what it adds, so every opcode in between reads as a hole
    rather than as garbage
*/
bool
d_tests_parsegen_storage_sparse(void)
{
    djinterp::parse::op_set sparse(11u);

    if ( (!sparse.reserve(1u))                  ||
         (!sparse.def(37, "FAR", idle_op()))    )
    {
        return false;
    }

    // nothing was ever defined below the one opcode that was
    for (int code = 0; code < 37; code++)
    {
        if (sparse.find(code))
        {
            return false;
        }
    }

    return ( (sparse.find(37) != nullptr)  &&
             (sparse.size() == 38u)        );
}
#endif  // D_INTERNAL_PARSE_OP_SET_HEAP

}  // namespace

// d_tests_parsegen_registry
//   constant: the suite -- every section this build has, the vocabulary before
// the registry that selects by it.
const d_tests_section d_tests_parsegen_registry[] =
{
    { "parsegen_features_algebra",  &d_tests_parsegen_features_algebra  },
    { "parsegen_features_render",   &d_tests_parsegen_features_render   },
    { "parsegen_registry_kinds",    &d_tests_parsegen_registry_kinds    },
    { "parsegen_registry_select",   &d_tests_parsegen_registry_select   },
    { "parsegen_registry_extend",   &d_tests_parsegen_registry_extend   },
    { "parsegen_registry_flags",    &d_tests_parsegen_registry_flags    },
    { "parsegen_registry_needs",    &d_tests_parsegen_registry_needs    },
    { "parsegen_registry_find",     &d_tests_parsegen_registry_find     },
#if (D_INTERNAL_PARSEGEN_REGISTRY_HEAP == 1)
    { "parsegen_storage_grown",     &d_tests_parsegen_storage_grown     },
#endif  // D_INTERNAL_PARSEGEN_REGISTRY_HEAP
#if (D_INTERNAL_PARSE_OP_SET_HEAP == 1)
    { "parsegen_storage_sparse",    &d_tests_parsegen_storage_sparse    },
#endif  // D_INTERNAL_PARSE_OP_SET_HEAP
    { "parsegen_storage_fixed",     &d_tests_parsegen_storage_fixed     }
};

// d_tests_parsegen_registry_count
//   constant: how many sections the table above holds in this build.
const std::size_t d_tests_parsegen_registry_count =
    sizeof(d_tests_parsegen_registry) / sizeof(d_tests_parsegen_registry[0]);

#endif  // floor, for now
