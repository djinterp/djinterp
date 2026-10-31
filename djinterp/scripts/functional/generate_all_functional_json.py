#!/usr/bin/env python3
"""
Generates per-header JSON files for the remaining djinterp::functional
headers.

This is a one-shot writer encoding the classification audit done in
the previous chat turn. Re-run after edits to the SYMBOLS data to
regenerate the files consistently. Existing extractor.json and
comparator.json are NOT touched (already hand-tuned).
"""

import json
import os

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   "data_djinterp_functional")
os.makedirs(OUT, exist_ok=True)

# Constexpr-eligibility classes used below
# C11_C11 : compiles + constexpr from C++11 (pure metafunction / value-stored helper)
# C11_C14 : compiles from C++11 but reliable constexpr only from C++14
#           (decltype-on-declval chains, generic-deduced returns)
# C11_C20 : compiles from C++11 but constexpr depends on std::vector or
#           construct_at (C++20)
# C11_NONE: compiles from C++11 but never constexpr (std::function chain,
#           std::map storage, try/catch, IO)
# C14_C14 : module requires C++14 itself (generic lambdas, etc.)
# C14_C20 : module requires C++14, constexpr awaits C++20 storage gates

EMPTY_DEFAULTS = {
    "intrinsic_required": False, "intrinsic_names": "", "detection_macro": "",
    "t_alias_in": None, "v_var_in": None, "deprecated_in": None,
    "depends_on": [], "failure_reason": None,
    "secondary_min": None, "constexpr_in_secondary": None,
}

# ============================================================================
# Support-axis blockers
# ============================================================================
# The project floor is C++98 (matches the version axis in _config.json).
# Every symbol with primary_in > "C++98" needs a failure_reason explaining
# WHY it doesn't reach the floor. These blockers cover the common cases;
# specific symbols can override via sym(..., cpp98_blocker="...").

PROJECT_FLOOR = "C++98"

GENERIC_C11_BLOCKER = (
    "Implementation uses C++11 language features: variadic templates for the "
    "parameter packs in operator() and constructors, rvalue references with "
    "std::forward (perfect forwarding) for moving callables into helper "
    "members, decltype-based trailing return types, `using` type aliases, "
    "and brace-init in factories. The djinterp functional framework targets "
    "C++11 as its baseline; no C++98 back-port is planned."
)

GENERIC_C14_BLOCKER = (
    "Requires C++14: the helper protocol uses generic lambdas (auto "
    "parameters) and deduced return types to abstract over downstream value "
    "types without making each helper doubly generic. The rest of djinterp "
    "functional compiles on C++11; if C++14 is unavailable, expressing the "
    "same dataflow in C++11 means using view.hpp + accumulator.hpp + "
    "producer.hpp directly."
)

# Specific blocker for variable-template aliases (_v forms). The
# corresponding `::value` / metafunction form IS available on C++11 (and
# would even be available on C++98 if the underlying trait were).
VAR_TEMPLATE_BLOCKER = (
    "Variable templates are a C++14 language feature. The corresponding "
    "::value form (e.g. is_inspectable<F>::value, function_traits<F>::arity) "
    "is available on C++11. A C++98 path would require porting the "
    "underlying trait off variadic templates, which is not planned."
)

DEFAULT_BLOCKERS = {
    "C++11": GENERIC_C11_BLOCKER,
    "C++14": GENERIC_C14_BLOCKER,
}


def sym(name, group, klass, notes="", depends_on=None,
        cx_note=None, cpp98_blocker=None):
    """Build a symbol record.

    Arguments:
      name, group     : as in the JSON schema.
      klass           : tag mapping to (primary_in, constexpr_in_primary).
      notes           : the JSON `notes` field; constexpr-ceiling commentary
                        may be appended below via cx_note.
      depends_on      : list of external/unmet dependencies.
      cx_note         : optional constexpr-axis commentary; folded into
                        notes with a "constexpr-ceiling:" prefix so that
                        the constexpr matrix's grey cells are explained
                        without polluting the support-axis failure_reason.
      cpp98_blocker   : optional override of the support-axis blocker.
                        Defaults to a generic per-version explanation.
                        Set this for symbols with unusual support-axis
                        blockers (e.g. variable templates, std::function).
    """
    base = dict(EMPTY_DEFAULTS)
    primary, cx = {
        "C11_C11":  ("C++11", "C++11"),
        "C11_C14":  ("C++11", "C++14"),
        "C11_C17":  ("C++11", "C++17"),
        "C11_C20":  ("C++11", "C++20"),
        "C11_NONE": ("C++11", None),
        "C14_C14":  ("C++14", "C++14"),
        "C14_C20":  ("C++14", "C++20"),
        "C14_NONE": ("C++14", None),
    }[klass]
    base["name"] = name
    base["group"] = group
    base["primary_in"] = primary
    base["constexpr_in_primary"] = cx

    # Notes: append constexpr-ceiling commentary if supplied.
    merged_notes = notes
    if cx_note:
        if merged_notes and not merged_notes.rstrip().endswith("."):
            merged_notes = merged_notes.rstrip() + "."
        merged_notes = (merged_notes.rstrip() +
                        " constexpr-ceiling: " + cx_note).strip()
    base["notes"] = merged_notes

    # Support-axis failure_reason: set when primary_in > project floor.
    if primary == PROJECT_FLOOR:
        base["failure_reason"] = None
    else:
        base["failure_reason"] = (cpp98_blocker or
                                  DEFAULT_BLOCKERS.get(primary, ""))

    if depends_on:
        base["depends_on"] = list(depends_on)

    return base


# ============================================================================
# Per-header symbol rosters
# ============================================================================

FILES = {}

# -------- function_traits.hpp ---------------------------------------------
FILES["function_traits.hpp"] = {
    "subtitle": "Compile-time introspection of callable types. Extracts arity, "
                "return type, argument types, and class type for any callable: "
                "free functions, function pointers, member-function pointers, "
                "std::function, lambdas, ordinary functor types.",
    "blurb": "Compile-time callable introspection (arity, return type, arg types, class type) for every common callable shape.",
    "symbols": [
        sym("function_traits", "Trait (primary template)", "C11_C11",
            "Primary template; inspects T::operator() (the lambda case). "
            "Inherits from the member-function-pointer specialization."),
        sym("function_traits<R(Args...)>", "Specialization", "C11_C11",
            "Canonical function-type specialization; all other forms forward here."),
        sym("function_traits<R(*)(Args...)>", "Specialization", "C11_C11",
            "Function-pointer specialization."),
        sym("function_traits<R(&)(Args...)>", "Specialization", "C11_C11",
            "Function-reference specialization."),
        sym("function_traits<R(C::*)(Args...)>", "Specialization", "C11_C11",
            "Non-const member-function-pointer specialization; exposes class_type."),
        sym("function_traits<R(C::*)(Args...) const>", "Specialization", "C11_C11",
            "Const member-function-pointer specialization; the lambda case."),
        sym("function_traits<R(C::*)(Args...) volatile>", "Specialization", "C11_C11",
            "Volatile member-function-pointer specialization."),
        sym("function_traits<R(C::*)(Args...) const volatile>", "Specialization", "C11_C11",
            "Const-volatile member-function-pointer specialization."),
        sym("function_traits<std::function<R(Args...)>>", "Specialization", "C11_C11",
            "std::function specialization; forwards to bare function-type form."),
        sym("return_type_t", "Convenience alias", "C11_C11",
            "function_traits<F>::return_type shorthand."),
        sym("arg_t<F, N>", "Convenience alias", "C11_C11",
            "Nth argument type shorthand."),
        sym("args_tuple_t<F>", "Convenience alias", "C11_C11",
            "Parameter list as std::tuple."),
        sym("arity_v<F>", "Convenience alias", "C14_C14",
            "Variable-template shorthand. Gated on D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES.",
            cpp98_blocker=VAR_TEMPLATE_BLOCKER),
        sym("is_inspectable<F>", "Predicate trait", "C11_C11",
            "Detects whether function_traits<F> yields usable type information. "
            "False for generic lambdas / overloaded callables."),
        sym("is_inspectable_v<F>", "Predicate trait", "C14_C14",
            "Variable-template shorthand for is_inspectable.",
            cpp98_blocker=VAR_TEMPLATE_BLOCKER),
    ],
}

# -------- filterable_traits.hpp -------------------------------------------
FILES["filterable_traits.hpp"] = {
    "subtitle": "SFINAE-based traits for filterable container requirements. "
                "Detects whether a container supports iteration, value_type, "
                "push_back or insert, an optional native filter method, etc.",
    "blurb": "Container-shape detection traits (has_begin, has_end, has_value_type, has_push_back, has_insert, has_size, has_empty, has_filter_method) and the aggregate is_filterable.",
    "symbols": [
        sym("has_begin",          "Member-detection trait", "C11_C11",
            "Detects std::begin(_Type&) well-formedness via the void_t SFINAE idiom."),
        sym("has_end",            "Member-detection trait", "C11_C11",
            "Detects std::end(_Type&) well-formedness."),
        sym("has_value_type",     "Member-detection trait", "C11_C11",
            "Detects nested ::value_type alias."),
        sym("has_push_back",      "Member-detection trait", "C11_C11",
            "Detects push_back(value_type) member."),
        sym("has_insert",         "Member-detection trait", "C11_C11",
            "Detects insert(iterator, value_type) member."),
        sym("has_size",           "Member-detection trait", "C11_C11"),
        sym("has_empty",          "Member-detection trait", "C11_C11"),
        sym("has_iterator",       "Iterator-property trait", "C11_C11",
            "Detects nested ::iterator alias."),
        sym("has_const_iterator", "Iterator-property trait", "C11_C11"),
        sym("has_filter_method",  "Native-filter detection", "C11_C11",
            "Detects a .filter(predicate) member function."),
        sym("is_iterable",        "Composite trait", "C11_C11",
            "True when both has_begin and has_end."),
        sym("is_output_capable",  "Composite trait", "C11_C11",
            "True when has_push_back or has_insert."),
        sym("is_filterable",      "Composite trait", "C11_C11",
            "Aggregate: iterable, has value_type, and is output-capable."),
        sym("is_filterable_v",    "Convenience alias", "C14_C14",
            "Variable-template shorthand.",
            cpp98_blocker=VAR_TEMPLATE_BLOCKER),
        sym("is_iterable_v",      "Convenience alias", "C14_C14",
            "Variable-template shorthand.",
            cpp98_blocker=VAR_TEMPLATE_BLOCKER),
        sym("has_filter_method_v","Convenience alias", "C14_C14",
            "Variable-template shorthand.",
            cpp98_blocker=VAR_TEMPLATE_BLOCKER),
        sym("filterable_value_t", "Convenience alias", "C11_C11",
            "Extracts value_type or nonesuch."),
        sym("filter (generic)",   "SFINAE-gated function", "C11_C20",
            "Returns a new container with only elements passing the predicate. "
            "Construction uses push_back/insert; constexpr requires C++20 constexpr vector.",
            cx_note="Constructs the result container via push_back; constexpr only with C++20 constexpr-allocator-aware containers."),
        sym("filter (native dispatch)", "SFINAE-gated function", "C11_C11",
            "Delegates to the container's own .filter() method when one exists. "
            "Constexpr-ness inherited from the container's method."),
    ],
}

# -------- predicate.hpp ---------------------------------------------------
FILES["predicate.hpp"] = {
    "subtitle": "Template predicate combinators with AND, OR, XOR, NOT. "
                "Type-safe, SFINAE-constrained, value-stored, constexpr-clean.",
    "blurb": "Predicate combinators: predicate_and, predicate_or, predicate_xor, predicate_not, all stored by value with short-circuiting.",
    "symbols": [
        sym("predicate_and_combinator", "Internal helper", "C11_C11",
            "Short-circuiting binary AND combinator stored by value."),
        sym("predicate_or_combinator",  "Internal helper", "C11_C11",
            "Short-circuiting binary OR combinator."),
        sym("predicate_xor_combinator", "Internal helper", "C11_C11",
            "Binary XOR combinator (no short-circuit by nature of XOR)."),
        sym("predicate_not_combinator", "Internal helper", "C11_C11",
            "Unary NOT combinator."),
        sym("predicate_and", "Factory", "C11_C11",
            "Variadic-friendly AND factory."),
        sym("predicate_or",  "Factory", "C11_C11"),
        sym("predicate_xor", "Factory", "C11_C11"),
        sym("predicate_not", "Factory", "C11_C11"),
    ],
}

# -------- compose.hpp -----------------------------------------------------
FILES["compose.hpp"] = {
    "subtitle": "Template function composition and partial application. "
                "Composed transformers, partial application, variadic compose/pipe, "
                "tap (side-effect injection), memoize (runtime), and a Y combinator (fix).",
    "blurb": "Composition utilities: compose_transformer, compose_all / pipe_all, partial_back, tap, memoize (runtime), and fix (Y combinator).",
    "symbols": [
        sym("composed_transformer", "Internal helper", "C11_C11",
            "Two-stage composition: compose(f,g)(x) = g(f(x)). Stored by value, fully constexpr."),
        sym("partial_consumer",     "Internal helper", "C11_C11",
            "Partial application binding a trailing argument."),
        sym("tap_helper",           "Internal helper", "C11_C11",
            "Pass-through with side effect; returns value unchanged. "
            "constexpr-eligible when the side-effect callable is itself constexpr."),
        sym("fix_helper",           "Internal helper", "C11_C11",
            "Y combinator over arbitrary arity; recursive lambdas via self-injection."),
        sym("memoize_helper",       "Internal helper", "C11_NONE",
            "Caches results in a std::map. Inherently mutable + allocating.",
            cx_note="std::map storage with mutable cache; never constexpr-eligible (std::map has no constexpr operations as of C++23). Also defeats the purpose of memoization in constant-expression contexts where the compiler already constant-folds."),
        sym("compose_transformer (factory)", "Factory", "C11_C11"),
        sym("compose_all", "Variadic factory", "C11_C14",
            "N-ary right-to-left composition. Recursive template; relaxed constexpr (C++14) needed for the multi-statement body in some cases.",
            cx_note="Body contains multi-statement constexpr that needs C++14 relaxed-constexpr rules."),
        sym("pipe_all",    "Variadic factory", "C11_C14",
            "N-ary left-to-right composition.",
            cx_note="Same as compose_all."),
        sym("partial_back",  "Factory", "C11_C11",
            "Partially applies the last argument."),
        sym("tap",           "Factory", "C11_C11",
            "Pass-through factory for side-effect injection."),
        sym("memoize",       "Factory", "C11_NONE",
            "Wraps a function with a cache.",
            cx_note="See memoize_helper."),
        sym("fix",           "Factory", "C11_C11",
            "Y combinator factory for recursive lambdas."),
    ],
}

# -------- curry.hpp -------------------------------------------------------
FILES["curry.hpp"] = {
    "subtitle": "Function currying, uncurrying, flip, identity, always, never. "
                "Auto-arity curry via function_traits, plus explicit-arity curry_n.",
    "blurb": "Currying (curry / curry_n), uncurry, flip, identity, always (constant function), never (always-false predicate).",
    "symbols": [
        sym("index_seq",       "Internal helper", "C11_C11",
            "C++11 stand-in for std::index_sequence (C++14)."),
        sym("make_index_seq",  "Internal helper", "C11_C11",
            "Recursive builder for index_seq<0, 1, ..., N-1>."),
        sym("apply_tuple",     "Internal helper", "C11_C14",
            "Invokes a callable with arguments from a tuple. Predates std::apply (C++17). "
            "Reliable constexpr from C++14.",
            cx_note="Index-sequence-based apply; constexpr robust on C++14+."),
        sym("curry_helper",    "Internal helper", "C11_C14",
            "Variadic accumulator that calls f when enough args have been supplied. "
            "Uses function_traits for auto-arity detection.",
            cx_note="Args stored in a std::tuple; std::make_tuple is constexpr from C++14."),
        sym("curry_n_helper",  "Internal helper", "C11_C14",
            "Fixed-arity curry accumulator; arity supplied explicitly.",
            cx_note="std::tuple-based accumulation; constexpr from C++14."),
        sym("flip_helper",     "Internal helper", "C11_C11",
            "Swaps first two arguments of a binary-or-greater callable."),
        sym("always_helper",   "Internal helper", "C11_C11",
            "Returns a stored value regardless of arguments."),
        sym("uncurry_helper",  "Internal helper", "C11_C14",
            "Converts curried form back to n-ary; uses std::tuple internally.",
            cx_note="std::tuple/std::get; constexpr from C++14."),
        sym("curry",   "Factory", "C11_C14",
            "Auto-arity curry. Uses function_traits to detect arity.",
            cx_note="Inherits curry_helper's C++14 constexpr ceiling."),
        sym("curry_n", "Factory", "C11_C14",
            "Explicit-arity curry.",
            cx_note="Inherits curry_n_helper."),
        sym("uncurry", "Factory", "C11_C14"),
        sym("flip",    "Factory", "C11_C11",
            "Swap first two args."),
        sym("identity","Factory", "C11_C11",
            "Returns its argument unchanged."),
        sym("always",  "Factory", "C11_C11",
            "Constant function. Also exported as `constant`."),
        sym("never",   "Factory", "C11_C11",
            "Always-false predicate."),
    ],
}

# -------- monad.hpp -------------------------------------------------------
FILES["monad.hpp"] = {
    "subtitle": "Monad protocol and generic monadic operations. monad_traits<M> "
                "specialisation for each concrete monad; monad_unit, monad_bind, "
                "monad_map, monad_join, monad_then, kleisli_compose, lift_m2.",
    "blurb": "Monad protocol (monad_traits<M>) plus generic operations: unit, bind, map, join, then, kleisli compose, lift_m2.",
    "symbols": [
        sym("monad_traits<M>", "Protocol primary", "C11_C11",
            "Primary template, undefined by default. Each concrete monad provides its own specialization."),
        sym("is_monad<T>", "Detection trait", "C11_C11",
            "Detects whether monad_traits<T> has been specialized."),
        sym("monad_unit<M, T>", "Operation", "C11_C11",
            "Lifts a value into M<T>. Constexpr propagation depends on the specialisation."),
        sym("monad_bind", "Operation", "C11_C11",
            "M<T> + (T -> M<U>) -> M<U>. Constexpr depends on the specialisation; maybe/result currently cap at C++20 due to storage."),
        sym("monad_map",  "Operation", "C11_C11",
            "Functorial map; implemented as bind + unit."),
        sym("monad_join", "Operation", "C11_C11",
            "Flattens M<M<T>> to M<T>."),
        sym("monad_then", "Operation", "C11_C11",
            "Sequence; discards first result."),
        sym("kleisli_compose", "Operation", "C11_C11",
            "(A -> M<B>) and (B -> M<C>) -> (A -> M<C>)."),
        sym("lift_m2", "Operation", "C11_C11",
            "Binary applicative-style lift."),
        sym("bind_with(f)", "Pipeline combinator", "C11_C11",
            "RHS of operator| for m | bind_with(f)."),
        sym("map_with(f)",  "Pipeline combinator", "C11_C11"),
        sym("then_with(other)", "Pipeline combinator", "C11_C11"),
        sym("filter_with(p)", "Pipeline combinator", "C11_C11",
            "mzero-style; uses the monad's zero() if available."),
        sym("operator| (monad, combinator)", "Pipeline operator", "C11_C11"),
    ],
}

# -------- maybe.hpp -------------------------------------------------------
FILES["maybe.hpp"] = {
    "subtitle": "maybe<T> -- a monadic optional value type. Equivalent in purpose "
                "to std::optional (C++17) but C++11+, integrated with the djinterp "
                "monad protocol. Storage uses std::aligned_storage + placement new.",
    "blurb": "maybe<T> optional with monad-protocol integration. Storage via aligned_storage + placement new; constexpr requires a future refactor to construct_at (C++20).",
    "symbols": [
        sym("maybe<T>", "Primitive", "C11_NONE",
            "The optional type itself. Storage uses aligned_storage + placement new; "
            "constexpr would require switching to a tagged union + std::construct_at (C++20+).",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Storage uses placement new on aligned_storage, forbidden in constant evaluation. A planned refactor to tagged-union + std::construct_at would enable constexpr from C++20."),
        sym("nothing_t / nothing_v", "Tag", "C11_C11",
            "Empty-maybe tag type and inline value."),
        sym("just(value)", "Factory", "C11_NONE",
            "Builds a maybe holding the supplied value.",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Inherits maybe<T>'s storage ceiling."),
        sym("nothing<T>()", "Factory", "C11_NONE",
            "Builds an empty maybe.",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Inherits maybe<T>'s storage ceiling."),
        sym("from_pointer", "Factory", "C11_NONE",
            "nullptr -> nothing, else just(*ptr).",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Inherits maybe<T>'s storage ceiling."),
        sym("from_predicate", "Factory", "C11_NONE",
            "just(value) when predicate(value), else nothing.",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Inherits maybe<T>'s storage ceiling."),
        sym("or_else_with(default)", "Pipeline combinator", "C11_NONE",
            "RHS for `m | or_else_with(d)`. Falls back to default on nothing.",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Inherits maybe<T>'s storage ceiling."),
        sym("filter_with(predicate)", "Pipeline combinator", "C11_NONE",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Inherits maybe<T>'s storage ceiling."),
        sym("unwrap_or_with(default)", "Pipeline combinator", "C11_NONE",
            "Alias for or_else_with.",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Inherits maybe<T>'s storage ceiling."),
        sym("expect_with(message)", "Pipeline combinator", "C11_NONE",
            "Throws std::runtime_error on nothing.",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Inherits maybe<T>'s storage ceiling and throws on nothing branch."),
        sym("monad_traits<maybe<T>>", "Monad specialization", "C11_NONE",
            "Hooks maybe into the monad protocol.",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Inherits maybe<T>'s storage ceiling."),
        sym("zip_with(m1, m2, f)", "Free helper", "C11_NONE",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Inherits maybe<T>'s storage ceiling."),
        sym("flatten(m_of_m)", "Free helper", "C11_NONE",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Inherits maybe<T>'s storage ceiling."),
        sym("collect(container_of_maybe)", "Free helper", "C11_NONE",
            "Container of maybe -> maybe of container; all-just to just-of-vector.",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Inherits both maybe<T>'s ceiling and constexpr-vector (C++20)."),
    ],
}

# -------- result.hpp ------------------------------------------------------
FILES["result.hpp"] = {
    "subtitle": "result<T, E> -- a monadic success-or-error type. Storage uses "
                "a tagged union via aligned_storage with a discriminator.",
    "blurb": "result<T, E> with monad protocol integration. Same storage approach as maybe<T>; awaits the same constexpr refactor.",
    "symbols": [
        sym("result<T, E>", "Primitive", "C11_NONE",
            "Tagged union with discriminator. Storage uses aligned_storage; "
            "constexpr requires a tagged-union + construct_at refactor (C++20+).",
            depends_on=["result::constexpr_storage"],
            cx_note="Storage uses placement new on aligned_storage. Planned refactor as for maybe."),
        sym("ok<T, E>(value)", "Factory", "C11_NONE",
            "Builds a success result.",
            depends_on=["result::constexpr_storage"],
            cx_note="Inherits result<T,E>'s storage ceiling."),
        sym("err<T, E>(error)", "Factory", "C11_NONE",
            "Builds an error result.",
            depends_on=["result::constexpr_storage"],
            cx_note="Inherits result<T,E>'s storage ceiling."),
        sym("or_value_with(default)", "Pipeline combinator", "C11_NONE",
            depends_on=["result::constexpr_storage"],
            cx_note="Inherits result<T,E>'s storage ceiling."),
        sym("map_err_with(f)", "Pipeline combinator", "C11_NONE",
            "Transforms the error side.",
            depends_on=["result::constexpr_storage"],
            cx_note="Inherits result<T,E>'s storage ceiling."),
        sym("unwrap_with(message)", "Pipeline combinator", "C11_NONE",
            "Throws on err.",
            depends_on=["result::constexpr_storage"],
            cx_note="Inherits result<T,E>'s storage ceiling; throws on err branch."),
        sym("monad_traits<result<T, E>>", "Monad specialization", "C11_NONE",
            depends_on=["result::constexpr_storage"],
            cx_note="Inherits result<T,E>'s storage ceiling."),
        sym("collect(container_of_result)", "Free helper", "C11_NONE",
            "All-ok to ok-of-vector; first err short-circuits.",
            depends_on=["result::constexpr_storage"],
            cx_note="Inherits both result<T,E>'s ceiling and constexpr-vector (C++20)."),
        sym("combine(r1, r2, f)", "Free helper", "C11_NONE",
            "Binary; both must be ok.",
            depends_on=["result::constexpr_storage"],
            cx_note="Inherits result<T,E>'s storage ceiling."),
        sym("to_maybe(result)", "Free helper", "C11_NONE",
            "Lossy conversion: drops error side.",
            depends_on=["result::constexpr_storage", "maybe::constexpr_storage"],
            cx_note="Inherits both result<T,E>'s and maybe<T>'s storage ceilings."),
    ],
}

# -------- producer.hpp ----------------------------------------------------
FILES["producer.hpp"] = {
    "subtitle": "First-class producers (sources) for functional dataflow. "
                "Pull-driven sources with iterate, unfold, range, repeat, "
                "cycle, generate, take_n/drop_n, concat, interleave, "
                "transform, filter combinators.",
    "blurb": "First-class producers: iterate, unfold, range/iota, repeat, cycle, generate, single, empty, take_n/drop_n, concat, interleave, transform, filter.",
    "symbols": [
        sym("producer_step<T>", "Step type", "C11_C11",
            "(has_value, value) pair returned by each producer call."),
        sym("iterate_helper",    "Internal helper", "C11_C11",
            "Seed + step function; produces a recurrence."),
        sym("unfold_helper",     "Internal helper", "C11_C11",
            "State -> maybe<(state, value)>."),
        sym("range_helper",      "Internal helper", "C11_C11",
            "Numeric range with stride."),
        sym("repeat_helper",     "Internal helper", "C11_C11",
            "Infinite single value."),
        sym("repeat_n_helper",   "Internal helper", "C11_C11",
            "N-bounded single value."),
        sym("cycle_helper",      "Internal helper", "C11_C11",
            "Repeats a container forever via iterator pair."),
        sym("generate_helper",   "Internal helper", "C11_C11",
            "Calls a nullary function forever."),
        sym("empty_producer_helper", "Internal helper", "C11_C11"),
        sym("single_helper",     "Internal helper", "C11_C11",
            "One-shot value."),
        sym("take_n_helper",     "Internal helper", "C11_C11",
            "First-n then exhaust."),
        sym("drop_n_helper",     "Internal helper", "C11_C11",
            "Skip first-n."),
        sym("concat_helper",     "Internal helper", "C11_C11",
            "Sequence two producers."),
        sym("interleave_helper", "Internal helper", "C11_C11",
            "Alternate between two producers."),
        sym("transform_helper",  "Internal helper", "C11_C11",
            "Apply f to outputs."),
        sym("filter_helper",     "Internal helper", "C11_C11",
            "Drop non-matching outputs."),
        sym("producers::iterate", "Factory", "C11_C11"),
        sym("producers::unfold",  "Factory", "C11_C11"),
        sym("producers::range",   "Factory", "C11_C11",
            "Bounded numeric range; alias iota provided."),
        sym("producers::iota",    "Factory", "C11_C11",
            "Infinite numeric source."),
        sym("producers::repeat",  "Factory", "C11_C11"),
        sym("producers::repeat_n","Factory", "C11_C11"),
        sym("producers::cycle",   "Factory", "C11_C11"),
        sym("producers::generate","Factory", "C11_C11"),
        sym("producers::empty",   "Factory", "C11_C11"),
        sym("producers::single",  "Factory", "C11_C11"),
        sym("producers::take_n",  "Adapter factory", "C11_C11"),
        sym("producers::drop_n",  "Adapter factory", "C11_C11"),
        sym("producers::concat",  "Adapter factory", "C11_C11"),
        sym("producers::interleave","Adapter factory", "C11_C11"),
        sym("producers::transform","Adapter factory", "C11_C11"),
        sym("producers::filter",  "Adapter factory", "C11_C11"),
        sym("producers::from_container","Adapter factory", "C11_C11",
            "Lifts an iterator pair to a producer."),
        sym("collect", "Terminal", "C11_C20",
            "Drain to std::vector. Constexpr only with C++20 constexpr-vector.",
            cx_note="std::vector allocations are constexpr from C++20."),
        sym("for_each", "Terminal", "C11_C11",
            "Apply consumer until exhaustion. No allocations."),
        sym("fold", "Terminal", "C11_C11",
            "Drain via accumulator-style step."),
    ],
}

# -------- consumer.hpp ----------------------------------------------------
FILES["consumer.hpp"] = {
    "subtitle": "First-class consumers (sinks) for functional dataflow. "
                "void(const A&) callables with combinators (filtered, mapped, "
                "tee, batched, take, drop, conditional, fallback) and an "
                "opt-in std::function-based boxed_consumer<T>.",
    "blurb": "First-class consumers: print_to, write_to, discard, count_into, filtered, mapped, tee (broadcast), batched, take, drop, conditional, fallback, plus opt-in boxed_consumer<T>.",
    "symbols": [
        sym("print_to_helper",       "Internal helper", "C11_NONE",
            "Streams to std::ostream.",
            cx_note="ostream I/O isn't constexpr-eligible."),
        sym("write_to_helper",       "Internal helper", "C11_C20",
            "push_back to a container reference.",
            cx_note="Generic container push_back; constexpr only with C++20 constexpr containers."),
        sym("discard_helper",        "Internal helper", "C11_C11",
            "Null sink."),
        sym("count_into_helper",     "Internal helper", "C11_C11",
            "Increments a size_t reference."),
        sym("ref_consumer_helper",   "Internal helper", "C11_C11",
            "consumer<T>& as callable."),
        sym("filtered_consumer_helper", "Internal helper", "C11_C11",
            "Predicate gate."),
        sym("mapped_consumer_helper",   "Internal helper", "C11_C11",
            "Input transform (contramap)."),
        sym("tee_consumer_helper",   "Internal helper", "C11_C11",
            "Variadic broadcast."),
        sym("batched_consumer_helper","Internal helper", "C11_C14",
            "Every-N invocation; needs internal counter mutation.",
            cx_note="mutable counter member; reliable constexpr requires C++14."),
        sym("take_consumer_helper",  "Internal helper", "C11_C14",
            "First-N then ignore.",
            cx_note="mutable counter; C++14 needed."),
        sym("drop_consumer_helper",  "Internal helper", "C11_C14",
            "Skip first-N.",
            cx_note="mutable counter; C++14 needed."),
        sym("conditional_consumer_helper", "Internal helper", "C11_C11",
            "Predicate branch over two consumers."),
        sym("fallback_consumer_helper", "Internal helper", "C11_C11",
            "Try primary, then secondary on exception.",
            cx_note="catches exceptions; not constexpr pre-C++26."),
        sym("consumers::print_to",   "Factory", "C11_NONE",
            cx_note="I/O sink."),
        sym("consumers::write_to",   "Factory", "C11_C20",
            cx_note="Container push_back; C++20 constexpr container."),
        sym("consumers::discard",    "Factory", "C11_C11"),
        sym("consumers::count_into", "Factory", "C11_C11"),
        sym("consumers::filtered",   "Factory", "C11_C11"),
        sym("consumers::mapped",     "Factory", "C11_C11"),
        sym("consumers::tee",        "Factory", "C11_C11"),
        sym("consumers::batched",    "Factory", "C11_C14"),
        sym("consumers::take",       "Factory", "C11_C14"),
        sym("consumers::drop",       "Factory", "C11_C14"),
        sym("consumers::conditional","Factory", "C11_C11"),
        sym("consumers::fallback",   "Factory", "C11_NONE",
            cx_note="try/catch in operator(); not constexpr pre-C++26."),
        sym("boxed_consumer<T>",     "Type erasure", "C11_NONE",
            "std::function<void(const T&)>; opt-in.",
            cx_note="std::function is not constexpr in any standard yet."),
        sym("box",                   "Type-erasure factory", "C11_NONE",
            cx_note="Wraps into std::function."),
    ],
}

# -------- accumulator.hpp -------------------------------------------------
FILES["accumulator.hpp"] = {
    "subtitle": "First-class accumulators with combinable folds. State+step+finalize "
                "triples that absorb a stream and yield a value. combine(a, b, c, ...) "
                "runs multiple accumulators in lock-step. Current implementation stores "
                "step/finalize as std::function; a refactor to template-parameter "
                "storage is planned to enable constexpr.",
    "blurb": "First-class accumulators (sum, product, count, min/max, mean, variance, joining, first/last/nth, to_vector, to_map_by, histogram, top_k) with contramap, map_output, filtered, take, combine combinators.",
    "symbols": [
        sym("accumulator<S, I, O>", "Primitive", "C11_NONE",
            "Stores step and finalize as std::function -- inherently runtime. "
            "A planned refactor to template-parameter storage would make most accumulators constexpr-eligible from C++14.",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="step_fn_type / final_fn_type are std::function<...> members. Refactor planned: take step/final as template type parameters (combine_helper already does this via tuple)."),
        sym("make_accumulator", "Factory", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator<>'s std::function storage."),
        # Pre-built accumulators
        sym("sum<T>",        "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("product<T>",    "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("count<T>",      "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("count_if<T, P>","Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("min<T>",        "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("max<T>",        "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("min_by<T, K>",  "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("max_by<T, K>",  "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("mean<T>",       "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("variance<T>",   "Pre-built", "C11_NONE",
            "Welford's online algorithm.",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("stddev<T>",     "Pre-built", "C11_NONE",
            "Newton sqrt iteration; avoids <cmath> dependency.",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("first<T>",      "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("last<T>",       "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("nth<T>",        "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("joining<T>",    "Pre-built", "C11_NONE",
            "String concatenation with separator; uses std::ostringstream internally.",
            cx_note="std::ostringstream; not constexpr."),
        sym("to_vector<T>",  "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Both accumulator's std::function ceiling AND std::vector (C++20)."),
        sym("to_map_by<T, K>", "Pre-built", "C11_NONE",
            "Builds std::map.",
            cx_note="std::map storage."),
        sym("group_by<T, K>",  "Pre-built", "C11_NONE",
            "Builds std::map<K, vector<T>>.",
            cx_note="std::map storage."),
        sym("histogram<T>", "Pre-built", "C11_NONE",
            cx_note="std::map storage."),
        sym("top_k<T>", "Pre-built", "C11_NONE",
            "K largest, sorted desc; uses std::vector.",
            cx_note="std::vector + sort; constexpr C++20 if refactored away from std::function."),
        sym("all_match<T, P>", "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("any_match<T, P>", "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("none_match<T, P>", "Pre-built", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        # Combinators
        sym("contramap",  "Combinator", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("map_output", "Combinator", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("filtered",   "Combinator", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("take",       "Combinator", "C11_NONE",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inherits accumulator's std::function ceiling."),
        sym("combine",    "Combinator", "C11_C14",
            "Variadic parallel folds; stores accumulators in a std::tuple. Already template-typed; constexpr-eligible from C++14 modulo inner accumulators.",
            cx_note="Inner accumulators still use std::function; once those are refactored, combine itself becomes constexpr from C++14."),
    ],
}

# -------- fn_builder.hpp --------------------------------------------------
FILES["fn_builder.hpp"] = {
    "subtitle": "Template fluent builder for function chains. Accumulates "
                "transformers and predicates, then executes the chain on input. "
                "Current implementation stores the chain as a std::function; a "
                "refactor to template-parameter chain storage is planned.",
    "blurb": "Fluent function-chain builder (map, and_then, filter, where, take, skip, distinct, reversed, sorted, flat_map, plus fold/count/any terminals).",
    "symbols": [
        sym("fn_builder<I, C>", "Primitive", "C11_NONE",
            "Stores chain as std::function<vector(vector)>; entire builder runtime-only.",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="chain_fn = std::function<vector(vector)> -- the entire builder is type-erased. Refactor to a stored-by-value template-predecessor chain (same pattern as view.hpp adapters) is planned."),
        sym("create",     "Static factory", "C11_NONE",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("map",        "Fluent op", "C11_NONE",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("and_then",   "Fluent op", "C11_NONE",
            "Alias for map.",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("filter",     "Fluent op", "C11_NONE",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("where",      "Fluent op", "C11_NONE",
            "Alias for filter.",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("take",       "Fluent op", "C11_NONE",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("skip",       "Fluent op", "C11_NONE",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("distinct",   "Fluent op", "C11_NONE",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("reversed",   "Fluent op", "C11_NONE",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("sorted",     "Fluent op", "C11_NONE",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("flat_map",   "Fluent op", "C11_NONE",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("execute",    "Terminal", "C11_NONE",
            "Vector / container / raw-array overloads.",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("operator()", "Terminal", "C11_NONE",
            "Shorthand for execute.",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("fold",       "Terminal", "C11_NONE",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("count",      "Terminal", "C11_NONE",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("any",        "Terminal", "C11_NONE",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
        sym("make_builder","Free factory", "C11_NONE",
            depends_on=["fn_builder::dropping_std_function"],
            cx_note="Inherits fn_builder's std::function chain."),
    ],
}

# -------- filter.hpp ------------------------------------------------------
FILES["filter.hpp"] = {
    "subtitle": "Template collection filtering: positional ops (take, skip, range, "
                "slice), predicate selection, transformations (distinct, reverse), "
                "chaining, set-theoretic combinators, lazy iteration, fluent builder. "
                "Filter operations are type-erased via std::function.",
    "blurb": "Collection filtering framework with fluent filter_builder<T> and set-theoretic combinators (union, intersection, difference). Filter operations type-erased via std::function.",
    "symbols": [
        sym("filter_op_fn<T>", "Operation type", "C11_NONE",
            "std::function<vector<size_t>(const vector<T>&)>.",
            depends_on=["filter::dropping_std_function"],
            cx_note="std::function-typed operation."),
        sym("filter_op_take_first<T>",   "Op factory", "C11_NONE",
            cx_note="Type-erased via std::function."),
        sym("filter_op_skip_first<T>",   "Op factory", "C11_NONE",
            cx_note="Type-erased via std::function."),
        sym("filter_op_range<T>",        "Op factory", "C11_NONE",
            cx_note="Type-erased via std::function."),
        sym("filter_op_slice<T>",        "Op factory", "C11_NONE",
            cx_note="Type-erased via std::function."),
        sym("filter_op_where<T, P>",     "Op factory", "C11_NONE",
            cx_note="Type-erased via std::function."),
        sym("filter_op_distinct<T>",     "Op factory", "C11_NONE",
            cx_note="Type-erased via std::function."),
        sym("filter_op_reverse<T>",      "Op factory", "C11_NONE",
            cx_note="Type-erased via std::function."),
        sym("filter_result<T>",          "Result type", "C11_NONE",
            "Holds std::vector<T> + status info.",
            cx_note="std::vector storage + runtime composition."),
        sym("filter_chain<T>",           "Chain type", "C11_NONE",
            "Sequential application of filter ops; stores std::vector<filter_op_fn>.",
            cx_note="Storage of type-erased ops in std::vector."),
        sym("filter_union<T>",           "Combinator", "C11_NONE",
            cx_note="Operates on filter_chain."),
        sym("filter_intersection<T>",    "Combinator", "C11_NONE",
            cx_note="Operates on filter_chain."),
        sym("filter_difference<T>",      "Combinator", "C11_NONE",
            cx_note="Operates on filter_chain."),
        sym("filter_iterator<T>",        "Lazy iterator", "C11_NONE",
            cx_note="Iterates filter_chain results."),
        sym("filter_builder<T>",         "Fluent builder", "C11_NONE",
            "skip_first / where / take_first / distinct / etc.",
            cx_note="Wraps filter_chain."),
    ],
}

# -------- pipeline.hpp ----------------------------------------------------
FILES["pipeline.hpp"] = {
    "subtitle": "Template function_pipeline<T> holding intermediate results. "
                "Supports map, filter, fold, for_each, take, skip, take_while, "
                "skip_while, distinct, reverse, sort, flat_map, zip, partition, "
                "group_by. Each operation materializes results into std::vector.",
    "blurb": "Eager function pipeline holding intermediate results in std::vector. Type-safe alternative to fn_builder; materializes at each step.",
    "symbols": [
        sym("function_pipeline<T>",   "Class", "C11_C20",
            "Stores std::vector<T>; constexpr from C++20 with constexpr vector.",
            cx_note="std::vector storage; constexpr from C++20."),
        sym("from(container)",        "Static factory", "C11_C20",
            cx_note="Builds a std::vector."),
        sym("map(f)",                 "Fluent op", "C11_C20",
            cx_note="Builds a new std::vector."),
        sym("filter(p)",              "Fluent op", "C11_C20",
            cx_note="Builds a new std::vector."),
        sym("fold(init, f)",          "Terminal", "C11_C14",
            "No allocation needed for fold itself; constexpr from C++14."),
        sym("for_each(f)",            "Terminal", "C11_C14"),
        sym("take(n)",                "Fluent op", "C11_C20",
            cx_note="std::vector copy."),
        sym("skip(n)",                "Fluent op", "C11_C20",
            cx_note="std::vector copy."),
        sym("take_while(p)",          "Fluent op", "C11_C20",
            cx_note="std::vector copy."),
        sym("skip_while(p)",          "Fluent op", "C11_C20",
            cx_note="std::vector copy."),
        sym("distinct()",             "Fluent op", "C11_C20",
            cx_note="std::vector copy."),
        sym("reverse()",              "Fluent op", "C11_C20",
            cx_note="std::vector copy."),
        sym("sort(cmp)",              "Fluent op", "C11_C20",
            cx_note="std::sort + std::vector; constexpr std::sort is C++20."),
        sym("flat_map(f)",            "Fluent op", "C11_C20",
            cx_note="std::vector copy/append."),
        sym("zip(other)",             "Fluent op", "C11_C20",
            cx_note="std::vector."),
        sym("partition(p)",           "Fluent op", "C11_C20",
            cx_note="std::pair<std::vector, std::vector>."),
        sym("group_by(key_fn)",       "Fluent op", "C11_NONE",
            "Builds std::map<K, std::vector<T>>.",
            cx_note="std::map storage."),
        sym("to_vector()",            "Terminal", "C11_C20",
            cx_note="std::vector return."),
    ],
}

# -------- view.hpp --------------------------------------------------------
FILES["view.hpp"] = {
    "subtitle": "Lazy, pull-based views. Adapters compose via operator| without "
                "materializing intermediate results. Pipeline only does work when "
                "forced by a terminal (to_vector, count, fold, for_each, etc.). "
                "Adapter views themselves are constexpr-friendly; terminals that "
                "build a vector require C++20 constexpr-vector.",
    "blurb": "Lazy views + operator| pipeline. transform / filter / take / drop / take_while / drop_while / enumerate / zip / concat / reverse / chunk / stride adapters. Source views: iota, repeat, generate, empty, single.",
    "symbols": [
        sym("view_base",         "CRTP marker", "C11_C11"),
        sym("is_view<T>",        "SFINAE trait", "C11_C11"),
        sym("is_pipeable_to_view<T>", "SFINAE trait", "C11_C11"),
        sym("ref_view<C>",       "Fundamental view", "C11_C11",
            "Non-owning wrap of a container."),
        sym("owning_view<C>",    "Fundamental view", "C11_C11",
            "By-value capture of a container; constexpr depends on the container."),
        sym("iterator_pair_view<It>", "Fundamental view", "C11_C11"),
        sym("iota_view<Int>",    "Source view", "C11_C11",
            "Infinite numeric source."),
        sym("repeat_view<T>",    "Source view", "C11_C11"),
        sym("generate_view<F>",  "Source view", "C11_C11"),
        sym("empty_view<T>",     "Source view", "C11_C11"),
        sym("single_view<T>",    "Source view", "C11_C11"),
        sym("transform_view<V, F>", "Adapter view", "C11_C11"),
        sym("filter_view<V, P>",    "Adapter view", "C11_C11"),
        sym("take_view<V>",         "Adapter view", "C11_C11"),
        sym("drop_view<V>",         "Adapter view", "C11_C11"),
        sym("take_while_view<V, P>", "Adapter view", "C11_C11"),
        sym("drop_while_view<V, P>", "Adapter view", "C11_C11"),
        sym("enumerate_view<V>",    "Adapter view", "C11_C11"),
        sym("zip_view<V1, V2>",     "Adapter view", "C11_C11"),
        sym("concat_view<V1, V2>",  "Adapter view", "C11_C11"),
        sym("reverse_view<V>",      "Adapter view", "C11_C11",
            "Requires bidirectional underlying view."),
        sym("chunk_view<V>",        "Adapter view", "C11_C20",
            "Yields std::vector<T> per chunk; constexpr from C++20.",
            cx_note="Builds std::vector for each chunk; constexpr from C++20."),
        sym("stride_view<V>",       "Adapter view", "C11_C11"),
        sym("views::transform",     "Adapter factory", "C11_C11"),
        sym("views::filter",        "Adapter factory", "C11_C11"),
        sym("views::take",          "Adapter factory", "C11_C11"),
        sym("views::drop",          "Adapter factory", "C11_C11"),
        sym("views::take_while",    "Adapter factory", "C11_C11"),
        sym("views::drop_while",    "Adapter factory", "C11_C11"),
        sym("views::enumerate",     "Adapter factory", "C11_C11"),
        sym("views::zip",           "Adapter factory", "C11_C11"),
        sym("views::concat",        "Adapter factory", "C11_C11"),
        sym("views::reverse",       "Adapter factory", "C11_C11"),
        sym("views::chunk",         "Adapter factory", "C11_C20",
            cx_note="See chunk_view."),
        sym("views::stride",        "Adapter factory", "C11_C11"),
        sym("views::iota",          "Source factory", "C11_C11"),
        sym("views::repeat",        "Source factory", "C11_C11"),
        sym("views::repeat_n",      "Source factory", "C11_C11"),
        sym("views::generate",      "Source factory", "C11_C11"),
        sym("views::empty<T>",      "Source factory", "C11_C11"),
        sym("views::single",        "Source factory", "C11_C11"),
        sym("operator| (view, adapter)",      "Pipeline operator", "C11_C11"),
        sym("operator| (container, adapter)", "Pipeline operator", "C11_C11"),
        sym("to_vector",     "Terminal", "C11_C20",
            cx_note="Materializes to std::vector; constexpr from C++20."),
        sym("to<C>",         "Terminal", "C11_C20",
            cx_note="Materializes to a container; constexpr depends on container."),
        sym("count",         "Terminal", "C11_C11"),
        sym("fold(init, f)", "Terminal", "C11_C11"),
        sym("for_each(f)",   "Terminal", "C11_C11"),
        sym("reduce(f)",     "Terminal", "C11_C11"),
        sym("any_of(p)",     "Terminal", "C11_C11"),
        sym("all_of(p)",     "Terminal", "C11_C11"),
        sym("none_of(p)",    "Terminal", "C11_C11"),
        sym("find_if(p)",    "Terminal", "C11_NONE",
            "Returns maybe<T>.",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Returns maybe<T>; pegged to maybe storage ceiling."),
    ],
}

# -------- transducer.hpp --------------------------------------------------
FILES["transducer.hpp"] = {
    "subtitle": "Composable, source-and-sink-agnostic transformations (C++14+). "
                "A transducer is a function from one reducer to another. Same "
                "map(f) | filter(p) | take(n) chain works against containers, "
                "producers, and views without rewriting. Requires C++14 because "
                "the helpers use generic lambdas with deduced return types.",
    "blurb": "Source/sink-agnostic transducers. Same chain works for containers, producers, views. Requires C++14 (generic lambdas).",
    "symbols": [
        sym("reduced<Acc>",          "Wrapper", "C14_C14",
            "Short-circuit signal; carries a bool + value."),
        sym("is_reduced<T>",         "Trait", "C14_C14"),
        sym("unwrap_reduced",        "Helper", "C14_C14"),
        sym("transducers::map(f)",   "Transducer", "C14_C14"),
        sym("transducers::filter(p)","Transducer", "C14_C14"),
        sym("transducers::filter_not(p)", "Transducer", "C14_C14"),
        sym("transducers::take(n)",  "Transducer", "C14_C14",
            "First-n then short-circuit."),
        sym("transducers::drop(n)",  "Transducer", "C14_C14"),
        sym("transducers::take_while(p)", "Transducer", "C14_C14"),
        sym("transducers::drop_while(p)", "Transducer", "C14_C14"),
        sym("transducers::distinct()", "Transducer", "C14_C20",
            "Tracks seen values in a std::vector; constexpr from C++20.",
            cx_note="Uses std::vector for distinctness tracking; constexpr from C++20."),
        sym("transducers::tap(side_effect)", "Transducer", "C14_C14"),
        sym("transducers::flat_map(f)", "Transducer", "C14_C14",
            "One-to-many expansion."),
        sym("transducers::partition_by(key_fn)", "Transducer", "C14_C20",
            "Chunk while key unchanged; emits std::vector chunks.",
            cx_note="Builds std::vector chunks; constexpr from C++20."),
        sym("compose(t1, t2, ...)",  "Composition", "C14_C14",
            "N-ary composition of transducers."),
        sym("operator| (transducer, transducer)", "Pipeline", "C14_C14"),
        sym("transduce(xform, reducer, init, container)", "Driver", "C14_C14"),
        sym("transduce_into_vector<T>", "Driver", "C14_C20",
            "Materializes to std::vector.",
            cx_note="std::vector return; constexpr from C++20."),
        sym("transduce_into_accumulator","Driver", "C14_NONE",
            "Drains into an accumulator; inherits accumulator's std::function ceiling.",
            depends_on=["accumulator::dropping_std_function"],
            cx_note="Inner accumulator uses std::function for step/finalize."),
        sym("transduce_producer_to_consumer","Driver", "C14_NONE",
            "Drains a producer into a consumer (potentially via boxed_consumer).",
            cx_note="Consumer side may be a boxed_consumer (std::function)."),
    ],
}

# -------- functional.hpp (root + eager algorithms) -----------------------
FILES["functional.hpp"] = {
    "subtitle": "Root header: includes every functional sub-module AND provides "
                "fully generic eager algorithms (map, filter, fold, for_each, "
                "quantifiers, searching, ordering, take/skip, flat_map, zip, "
                "group_by, partition, distinct, reverse, slice, range). All work "
                "on STL containers, raw arrays, iterator pairs, initializer lists.",
    "blurb": "Eager algorithms for STL containers / raw arrays / iterator pairs / initializer lists: map, filter, fold (left/right), for_each, quantifiers, count_if, find_if, find_last, index_of, last_index_of, is_sorted, zip_with, take/skip, reduce, scan, flat_map, group_by, partition, distinct, reverse, slice, range.",
    "symbols": [
        sym("map(container, f)",        "Algorithm",     "C11_C20",
            cx_note="Returns std::vector; constexpr from C++20."),
        sym("filter(container, p)",     "Algorithm",     "C11_C20",
            cx_note="Returns std::vector; constexpr from C++20."),
        sym("fold_left(c, init, f)",    "Algorithm",     "C11_C14"),
        sym("fold_right(c, init, f)",   "Algorithm",     "C11_C14"),
        sym("for_each(c, f)",           "Algorithm",     "C11_C14"),
        sym("any_of(c, p)",             "Quantifier",    "C11_C14"),
        sym("all_of(c, p)",             "Quantifier",    "C11_C14"),
        sym("none_of(c, p)",            "Quantifier",    "C11_C14"),
        sym("count_if(c, p)",           "Search/count",  "C11_C14"),
        sym("find_if(c, p)",            "Search/count",  "C11_NONE",
            "Returns maybe<T>.",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Returns maybe<T>."),
        sym("find_last(c, p)",          "Search/count",  "C11_NONE",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Returns maybe<T>."),
        sym("index_of(c, value)",       "Search/count",  "C11_C14"),
        sym("last_index_of(c, value)",  "Search/count",  "C11_C14"),
        sym("is_sorted(c, cmp)",        "Ordering",      "C11_C14"),
        sym("take(c, n)",               "Take/skip",     "C11_C20",
            cx_note="std::vector return."),
        sym("skip(c, n)",               "Take/skip",     "C11_C20",
            cx_note="std::vector return."),
        sym("reduce(c, f)",             "Reduce/scan",   "C11_NONE",
            "Returns maybe<T>; empty input -> nothing.",
            depends_on=["maybe::constexpr_storage"],
            cx_note="Returns maybe<T>."),
        sym("scan(c, init, f)",         "Reduce/scan",   "C11_C20",
            cx_note="std::vector return."),
        sym("flat_map(c, f)",           "Flat-map/zip",  "C11_C20",
            cx_note="std::vector return."),
        sym("zip_with(c1, c2, f)",      "Flat-map/zip",  "C11_C20",
            cx_note="std::vector return."),
        sym("group_by(c, key_fn)",      "Group/partition", "C11_NONE",
            cx_note="std::map storage."),
        sym("partition(c, p)",          "Group/partition", "C11_C20",
            cx_note="std::pair<std::vector, std::vector>."),
        sym("distinct(c)",              "Distinct/reverse", "C11_C20",
            cx_note="std::vector return."),
        sym("reverse(c)",               "Distinct/reverse", "C11_C20",
            cx_note="std::vector return."),
        sym("slice(c, start, end)",     "Slice/range",   "C11_C20",
            cx_note="std::vector return."),
        sym("range(c, start, end)",     "Slice/range",   "C11_C20",
            "Alias for slice.",
            cx_note="std::vector return."),
    ],
}

# ============================================================================
# Write out the JSON files
# ============================================================================

for header, info in FILES.items():
    path = os.path.join(OUT, os.path.splitext(header)[0] + ".json")
    obj = {
        "header":       header,
        "subtitle":     info["subtitle"],
        "legend_blurb": info["blurb"],
        "symbols":      info["symbols"],
    }
    with open(path, "w") as f:
        json.dump(obj, f, indent=2, ensure_ascii=False)
    print(f"Wrote {path} ({len(info['symbols'])} symbols)")

print(f"\nGenerated {len(FILES)} JSON files in {OUT}/")
