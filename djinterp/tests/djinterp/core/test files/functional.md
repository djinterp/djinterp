# djinterp `functional` subframework — agent reference

A token-dense map of the 35 C++ header modules tagged `djinterp [functional]`.
Part 1 summarizes each module. Part 2 (appendix) gives per-module signature
tables. Foundational `[meta]`/`[core]` headers it depends on
(`type_traits.hpp`, `concepts.hpp`, `dtuple.hpp`, `member_traits.hpp`,
`djinterp.hpp`, and the NTTP/type carriers `carrier.hpp`/`value_list.hpp` that
`reduce.hpp` folds over) are out of scope here.

---

## Conventions (read once, applies everywhere)

**Namespaces.** All public API lives in `namespace djinterp` (`NS_DJINTERP` →
`namespace djinterp {`). Some modules group their factory functions in a
sub-namespace: `djinterp::comparators`, `djinterp::consumers`,
`djinterp::views`, `djinterp::transducers`, `djinterp::extractors`,
`djinterp::monoids`. Producer
factories are **flat** in `djinterp` (the old `producers` sub-namespace was
retired). Implementation classes live in `djinterp::internal` (`NS_INTERNAL`)
and almost always carry a `_helper` suffix; factories return those
`internal::*_helper` types, which model a role rather than exposing a fixed
public class.

**Naming.** Template parameters are `_LeadingUnderscoreCapital`
(`_Type`, `_Fn`, `_Predicate`, …). Trait families follow a uniform triple:
a `struct is_X<...>` (or `X_type`/`X_result_t`), a `constexpr bool is_X_v`
variable-template shorthand (C++14+), and a C++20 `concept` parallel named
`X`, `X_for`, `X_like`, or `X_c`. Tables below list the trait and note its
`_v`/concept parallels in the description instead of repeating rows.

**Version gating.** `D_CONSTEXPR` = `constexpr` where the standard allows it;
`D_CONSTEXPR20`/`D_CONSTEXPR14` widen it on newer standards. `D_NODISCARD` =
`[[nodiscard]]`. `_v` shorthands are gated on
`D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES` (C++14+); concepts on
`D_ENV_CPP_FEATURE_LANG_CONCEPTS` (C++20). `comparator`, `compose`, and
`predicate` are **dual-standard**: a C++11+ primary path plus a `#else` C++98
fallback exposing identical symbols (const-ref params, no `constexpr`, fixed
`result_type` typedefs, explicit adapter RHS types; variadic factories and
trait blocks are C++11+ only). `transducer` requires **C++14+** and
self-suppresses on C++11; `reduce` and `interpolate` require **C++17+** (the
value/type carriers and `std::string_view` respectively) and self-suppress
below it; `functional_concepts` is **C++20-only** (empty otherwise).

**Shared callable detectors.** `is_callable<Fn, Args...>`,
`callable_result_t<Fn, Args...>`, and `is_predicate<Pred, Arg>` are flat
`djinterp` traits defined by the `functional_traits.hpp` aggregator (with
C++20 concept faces `Callable`/`Predicate` in `functional_concepts.hpp`) and
referenced throughout. Several modules also define their own role-specific
predicate traits (noted per module).

**`operator|` discipline.** Every module that overloads `operator|` constrains
its right-hand operand to that module's own concrete adapter types, so the
view / monad / comparator / extractor / transducer pipelines never collide
when headers are combined.

---

# Part 1 — Module summaries

**`function_traits.hpp`** — Compile-time *introspection* of a callable's
declared shape: arity, return type, argument types, and (for member
functions) class type. Primary template inspects `T::operator()` (the lambda
case); specializations cover free functions, function/member pointers (incl.
cv-qualified), and `std::function`. Generic/templated `operator()` are not
inspectable. Complements `is_callable` ("can I call this?") with "what is the
declared signature?", and adds expression-probing `call_result_t` /
`is_invocable_with` that *do* work on generic lambdas.

**`functional_traits.hpp`** — The *shared callable vocabulary* every combinator
constrains on — the "merged functional-traits aggregator" the conventions refer
to. Three expression-probing traits: `is_callable<Fn, Args...>` (can a
const-lvalue `Fn` be called on `Args`? — succeeds on generic/templated
`operator()`), `callable_result_t<Fn, Args...>` (the type that call yields, or
`internal::call_nonesuch`), and `is_predicate<Pred, Arg>` (callable on one `Arg`
with a bool-convertible result). These are thin, functional-facing renames over
`function_traits.hpp`'s call-detection primitives (`is_invocable_with` /
`call_result_t` / `is_invocable_r_with`), and the header **re-exports
`function_traits.hpp`** so one include carries both the declared-shape
introspection and this "can I call it?" layer. `_v` shorthands under C++14+.
This is the aggregator the accumulator / comparator / extractor / transducer
families and the filter / pipeline / fn_builder combinators all include.

**`functional_concepts.hpp`** *(C++20-only)* — The C++20 *concept faces* of that
vocabulary: `Callable<Fn, Args...>` (face of `is_callable`) and
`Predicate<Pred, Arg>` (face of `is_predicate`), for code that prefers concept
syntax over the SFINAE traits. Deliberately only the **generic, cross-cutting**
concepts — the domain faces (`Composable`, `Monad`, `view_type`,
`is_comparator_c`, …) live with the combinators that define them, so this header
does not re-aggregate them. Empty under earlier standards, where callers fall
back to the `functional_traits.hpp` traits. Names follow the
PascalCase-parallel-of-the-trait convention (leading `is_`/`has_` dropped).

**`structural_traits.hpp`** — Small set of purely structural detection traits
used to wire the parse/scanner/pattern layers onto the dataflow primitives:
`has_find_method` (the repeated-search pattern shape), `has_match_result_type`,
`is_nullary_callable`/`is_unary_callable`, and `produces_optional_like` (a
nullary callable returning a bool-testable, dereferenceable value — the
pull-based unfold source step). Degrades to C++11.

**`predicate.hpp`** — Type-safe predicate *combinators*: `predicate_and/or/xor/
not/nand/nor` (binary/unary) plus variadic `all_of/any_of/none_of`
(C++11+ only; the variadic folds route through helper structs so the return
type is computable at any arity). Stores predicates by value (decayed),
forwards perfectly, propagates `noexcept`. Ships structural traits
(`is_predicate_and`, … `is_predicate_combinator`) and the behavioral
`is_predicate<Pred, Args...>` (callable + bool-convertible result).
Dual-standard.

**`compose.hpp`** — Function *composition* and partial application:
`compose` (math order `f(g(x))`), `pipe`/`compose_transformer` (left-to-right
`g(f(x))`), variadic `compose_all`/`pipe_all`, `partial_back`, `tap`
(side-effect pass-through), `memoize` (cache for pure functions), and `fix`
(Y-combinator for recursive lambdas). Section 0 defines the composition
vocabulary traits/concepts: `callable_result`, `is_invocable[_r]`,
`is_unary_transformer`, `is_composable`, `composition_result`,
`is_composed_transformer`, `is_memoized`. Dual-standard (C++98 keeps only the
binary primitives + memoize).

**`curry.hpp`** — Currying and argument combinators: `curry` (auto-arity, by
SFINAE on invocability), `curry_n<N>` (explicit arity), `uncurry`, `flip`
(swap first two args), `identity`, `always`/`constant` (K combinator),
`never` (always-false). Also defines an arity-classified predicate-trait
family: `is_predicate`, `is_nullary/unary/binary_predicate` (+ `_v` + the
`predicate_for`/`nullary_predicate`/… concepts).

**`comparator.hpp`** — First-class composable *comparators* (binary `(a,b)->bool`,
`std::sort` convention) in `namespace comparators`: factories `natural`,
`by_key`, `by_member`, `by_function`, `lifted`; combinators `reversed`
(invert) and `then` (tie-breaker chain), each with a no-/single-arg adapter
form so `cmp | reversed()` / `c1 | then(c2)` work via `operator|`; derived
predicates `equal_under`, `less_than`, `greater_than`. Traits: `is_comparator`,
`is_binary/unary_predicate`, `has_result_type` (+ `_v` + `_c` concepts).
Dual-standard.

**`pipeline.hpp`** — `function_pipeline<T>`: an *eager*, typed, chainable
pipeline holding intermediate results, with an optional-like error channel
(errors short-circuit subsequent ops). Source factories `from`/`of`/`error`
(+ free `pipeline_from`); chainable `map`, `filter`, `filter_not`, `fold`,
`for_each`, `take`/`take_last`/`take_while`, `skip`/`skip_while`, `slice`,
`distinct`, `reversed`, `sorted`, `flat_map`, `partition_pipe`, `group_by`,
`zip_with`; terminals `to_vector`, `reduce`, `any`/`all`/`none`, `count`,
`begin`/`end`. Traits: `is_pipeline`, `pipeline_value_type`,
`is_pipeline_mapper`/`_predicate`.

**`fn_builder.hpp`** — `fn_builder<Input, Current, Chain>`: a fluent builder
that *composes a typed transform chain* and applies it to input data, with the
chain captured by value in the third template parameter (no `std::function`
indirection — fully inlinable). Ops `map`/`and_then`, `filter`/`where`, `take`,
`skip`, `distinct`, `reversed`, `sorted`, `flat_map`; terminals `execute`/
`operator()`, `fold`, `count`, `any`, plus `chain()`. `make_builder<T>()`
seeds one; `boxed_fn_builder<In,Out>` + `box_builder` are the type-erased
escape hatch. Traits: `is_fn_builder`, `is_boxed_fn_builder`,
`fn_builder_input_type`/`_current_type`, `is_fn_builder_mapper`/`_predicate`.

**`filter.hpp`** — A comprehensive *collection-filtering* framework. Core:
`filter_op_fn<T>` (an op = `vector<T> -> vector<size_t>` of passing indices),
`filter_result<T>` (status + indices + elements), `filter_chain<T>`
(sequential ops, `std::function`-erased), set-theoretic
`filter_union`/`intersection`/`difference`, `filter_iterator<T>`,
`filter_builder<T>` (fluent: positional `take_first/last/nth`,
`skip_first/last`, `head/tail/init/rest`, `range`, `slice`; predicate
`where`/`where_not`; index `at`/`at_indices`; `distinct`, `reverse`; terminals
`apply`, `iterator`, `any/all/none_match`, `count_matches`), and a typed
fast-path `typed_filter<T, Chain>` (+ `make_typed_filter`) that avoids
`std::function` and lowers to a chain via `to_chain()`. Plus filterable-container
detection traits (`is_iterable`, `is_output_capable`, `is_filterable`,
`has_*` member detectors, `filterable_value_t`) and module-shape traits
(`is_filter_operation`, `is_filter_applicable`, `is_filter_result`).

**`monad.hpp`** — The monad *protocol* and generic operations. A monad is
recognized by specializing `monad_traits<M>` (exposes `value_type`,
`rebind<U>`, static `unit`, static `bind`). Generic ops: `monad_unit`,
`monad_bind`, `monad_map` (functorial, via bind+unit), `monad_join` (flatten),
`monad_then` (sequence, discard), `kleisli_compose` (`>=>`), `lift_m2`
(applicative binary lift). Pipeline combinators `bind_with`/`map_with`/
`then_with` + `operator|(monad, combinator)`. Introspection: `is_monad`,
`monad_value_type`, `monad_rebind`, `is_monadic_function`, `is_bindable`,
`is_mappable`, `is_monad_combinator` (+ `_v` + concepts).

**`maybe.hpp`** — `maybe<T>`: a monadic optional (C++11+ `std::optional`-like,
integrated with the monad protocol). Union storage so `T` need not be
default-constructible; constexpr-friendly under C++20. Inspection
`has_value`/`is_nothing`/explicit `operator bool`; access `value`/`value_or`/
`expect`/`reset`/`emplace`; functional `map`, `and_then`, `or_else`, `filter`,
`match`. Tag `nothing_t`/`nothing_v`. Factories `just`, `nothing<T>`,
`from_pointer`, `from_predicate`; pipeline combinators `or_else_with`,
`unwrap_or_with`, `filter_with`, `expect_with` + `operator|`. Specializes
`monad_traits`. Free helpers `zip_with`, `flatten`, `collect`. Traits
`is_maybe`, `is_maybe_predicate` (+ `_v` + `maybe_type`/`maybe_predicate_for`).

**`result.hpp`** — `result<T, E>`: success-or-error (Rust `Result` /
`std::expected`). Explicit error type; default ctor deleted. Built via
`ok<T,E>` / `err<T,E>` (ok/err tags internally). Inspection `is_ok`/`is_err`/
explicit `operator bool`; access `value`/`value_or`/`unwrap`/`error`;
functional `map`, `map_err`, `and_then`, `or_else`, `match`; conversion
`ok()`/`err()` → `maybe`. Participates in the monad protocol over the success
side (errors propagate unchanged). Pipeline combinators `or_value_with`,
`map_err_with`, `unwrap_with` + `operator|`. Free helpers `collect`
(short-circuits on first err), `combine` (binary), `to_maybe` (lossy). Traits
`is_result`, `result_value_type`/`result_error_type`,
`is_result_value_mapper`/`_error_mapper`.

**`functor.hpp`** — The functor *protocol* and the one generic map. A functor
is a type constructor `F<T>` recognized by specializing `functor_traits<F>`
(exposes `value_type`, static `map`, `is_specialized`); `rebind<U>` is
deliberately *not* part of the protocol, since view/producer mapped types
depend on the mapping function, so `functor_map`'s result type is deduced from
the instance's `map`. One generic op: `functor_map(fa, f)` (fmap). A single
blanket specialization keyed on `is_monad` derives `map` from `monad_map`, so
`maybe`, `result`, and every future monad are functors automatically (the
"monad bridge"); `view` and `producer` carry their own one-line specializations
(`views::transform` / the flat `transform`). A transducer is intentionally not
a functor instance — it is a morphism, not an `F<A>` context. Introspection:
`is_functor`, `functor_value_type`, `is_fmappable` (+ `_v` + concepts `Functor`,
`fmappable_with`). Includes `monad.hpp`.

**`applicative.hpp`** — The applicative *protocol* (between functor and monad)
and its generic ops. An applicative adds two operations to a functor: `pure`
(lift a bare value into `F`, `F` explicit) and `ap` (`F<a->b> -> F<a> -> F<b>`,
Haskell `<*>`). From them, `lift_a2(fa, fb, binary_f)` is derived once for every
applicative (the applicative counterpart of `lift_m2`), as `ap(functor_map(fa,
curry2(f)), fb)`. A blanket specialization keyed on `is_monad` derives `pure`
from `monad_unit` and `ap` from `monad_bind`+`monad_map`, so `maybe`, `result`,
and every future monad are applicatives automatically. `pure`/`ap` delegate to
`applicative_traits<F>`; `lift_a2` is generic. Introspection: `is_applicative`,
`applicative_value_type`, `is_applicable` (+ `_v` + concepts `Applicative`,
`applicable_with`). Includes `monad.hpp` and `functor.hpp`.

**`foldable.hpp`** — The foldable *protocol* and the generic folds. A foldable
is a context `F<T>` holding zero or more `T` that collapses to a single result
by threading a reducer through its elements; where Functor unified the per-type
map, Foldable unifies the per-type fold. A foldable is recognized by
specializing `foldable_traits<F>` with one operation, `fold_left` (the strict
left fold `(acc, x) -> acc` matching `reduce.hpp`'s runtime driver). From that
single obligation, `fold_right`, `fold_map`, `fold_to_vector`, `fold_length`,
`fold_is_empty`, `fold_any`, and `fold_all` are all derived generically. Unlike
Functor / Applicative there is **no monad bridge** — a fold is not expressible
through `unit`/`bind` — so each foldable supplies its own `fold_left`: `maybe`
and `result` over their zero-or-one carried value (an `err` is empty), `view`
by iterating its lazy pipeline, `producer` by pulling a copy to exhaustion
(non-destructive). The generic operations carry a `fold_` prefix to stay
collision-free with the `view` terminals (`to_vector`, `any_of`, `all_of`) and
the `predicate.hpp` combinators. Introspection: `is_foldable`,
`foldable_value_type` (+ `_v` + concept `Foldable`). Stands on the core header
alone. Infinite views / producers must be bounded before folding.

**`semigroup.hpp`** — The semigroup *protocol* and its associative combine. A
semigroup is a type `T` with one operation, an associative binary combine
`T x T -> T` (no identity required). Where Functor unified the per-type map and
Foldable the per-type fold, Semigroup unifies the per-type combine: a `view`'s
concat, a `result`'s combine, string and vector concatenation, numeric addition
— all one associative operation under different names. A semigroup is
recognized by specializing `semigroup_traits<T>` with a static `combine`; the
free function is **`mappend(a, b)`** (Haskell's `<>`). The name is deliberately
not `combine`: `accumulator.hpp` already owns flat `combine(...)` (variadic
parallel folds), so this layer uses the collision-free monoid vocabulary
`mappend` / `mempty` / `mconcat`. Introspection: `is_semigroup` (+ `_v` +
concept `Semigroup`). Concrete instances live in `monoid.hpp`. Core header only.

**`monoid.hpp`** — The monoid *protocol*, its identity-aware operations, and the
standard instances. A monoid is a semigroup with an identity `mempty` such that
`mappend(mempty, x) == mappend(x, mempty) == x`. A monoid is recognized by
specializing `monoid_traits<T>` with a static `empty` (combine comes from its
`semigroup_traits`). Operations: **`mempty<T>()`** (the identity, `T` explicit),
**`mconcat(fa)`** (combine every element of a Foldable of monoid values), and
**`fold_monoid(fa, f)`** (map each element of any Foldable into a monoid, then
`mconcat` — the protocol-driven counterpart of `foldable`'s explicit-monoid
`fold_map`); `mconcat`/`fold_monoid` build on the Foldable protocol. Instances,
each defining a type's combine *and* identity in one place: `std::string` and
`std::vector<T>` (concatenation), and the newtypes in **namespace `monoids`** —
`sum`, `product`, `all`, `any`, `min`, `max` (a scalar is a monoid in more than
one way, so the wrapper names the intended one; the namespace keeps them clear
of the flat `accumulator` factories `sum`/`min`/`max`/`mean`). Each newtype
wraps a public `value`. Introspection: `is_monoid` (+ `_v` + concept `Monoid`).
Includes `semigroup.hpp` and `foldable.hpp`.

**`alternative.hpp`** — The Alternative *protocol*: a monoid on a functor. An
Alternative is a context `F<A>` with a distinguished failure value and an
associative choice (Haskell's `<|>`). Where Monoid unified combining for plain
values, Alternative unifies the per-type "fall back / try the other one" that
already lives, under different names, in `maybe` (`or_else`) and in the
sequence types as concatenation. A type is recognized by specializing
`alternative_traits<F>` with `empty()` and `choice(a, b)`; the free functions
are **`aempty<F>()`** (the failure, `F` explicit, like `mempty` — not named
`empty`, which is heavily used in the tree) and **`alt(a, b)`** (the choice).
**`asum(fa)`** folds a Foldable of alternatives with `alt` from `aempty` — the
Alternative counterpart of `mconcat` ("take the first that succeeds"). Instances:
`maybe` (the uniform, canonical one — `empty` is `nothing`, `choice` keeps the
first that holds a value), `view` and `producer` (`empty` is the empty sequence,
`choice` is concatenation). `result` is intentionally **not** an Alternative —
like Haskell's `Either` it has no canonical empty. Alternative conceptually
refines Applicative, but the concept requires only `alternative_traits` (the
sequence families carry the structure without otherwise being applicative
here). Introspection: `is_alternative`, `alternative_value_type` (+ `_v` +
concept `Alternative`). Includes `foldable.hpp`.

**`traversable.hpp`** — The Traversable *protocol*: `traverse` and `sequence`,
the capstone of the layer. A traversable `T<A>` is walked left-to-right while an
applicative effect runs at each element, the results collected back into the
shape: **`traverse(ta, f)`** takes `f : A -> F<B>` and yields `F<T<B>>`. It needs
a structure that is both a Functor (rebuild the shape) and a Foldable (walk the
elements), and an Applicative `F` (thread and combine the effects) — all
provided by the surrounding headers. **`sequence(ta)`** is the special case
`traverse(ta, identity)`: it turns `T<F<A>>` inside out into `F<T<A>>`.
Traversing a maybe with a result-producing function turns `maybe<A>` into
`result<maybe<B>,E>` (the error hoisted out); sequencing a `vector<maybe<A>>`
yields `maybe<vector<A>>` that is `just` only if every element was a value. The
effect `F` is recovered from the *type* of `f`'s result, so `traverse` is
well-typed even when `f` is never called (empty structure → `pure` of the empty
shape). A type is recognized by specializing `traversable_traits<T>` with a
single `traverse`. Instances: `maybe` and `result` (shape-preserving, over their
zero-or-one element), and the sequences `view`, `producer`, and `std::vector`
(which **materialise** into `F<std::vector<B>>`, since the effects must be
sequenced). For the effect `F`, this layer ships `maybe` and `result` (the
applicatives). Introspection: `is_traversable`, `traversable_value_type` (+ `_v`
+ concept `Traversable`). Includes `functor.hpp`, `applicative.hpp`,
`foldable.hpp`.

**`bifunctor.hpp`** — The Bifunctor *protocol*: map both type parameters of a
two-parameter type at once. `bimap(fab, f, g)` applies `f : A -> C` to the first
and `g : B -> D` to the second, yielding `F<C, D>`; `map_first` / `map_second`
map one side (derived as `bimap` with identity on the other). It unifies the
"map both sides" a two-parameter type already has — most visibly `result`, whose
`bimap` is `map` then `map_err`, so `map_first == map` and `map_second ==
map_err`. Instances: `std::pair`, `kv_pair` (here), and `result` (in
`result.hpp`). Introspection: `is_bifunctor`, `bifunctor_first_type` /
`bifunctor_second_type` (+ `_v`/`_t` + concept `Bifunctor`).

**`comonad.hpp`** — The Comonad *protocol*, the dual of Monad: `extract(w)`
reads a focus value out of a context `W<A>` (the dual of `unit`), `extend(w, f)`
re-decorates the whole structure with `f : W<A> -> B` (the dual of `bind`), and
`duplicate(w)` nests it `W<A> -> W<W<A>>` (the dual of `join`, derived as
`extend` with identity). A comonad is never empty — a focus can always be read.
Instances: the Env (co-reader) comonad over `std::pair<E, A>` and `kv_pair<K, V>`
(focus = the second component; the first is the ambient environment).
Introspection: `is_comonad`, `comonad_value_type` (+ `_v` + concept `Comonad`).

**`profunctor.hpp`** — The Profunctor *protocol*: a two-parameter arrow
`P<A, B>`, contravariant in the input, covariant in the output. `dimap(p, pre,
post)` pre-composes `pre : A' -> A` and post-composes `post : B -> B'`, turning a
`P<A, B>` into a `P<A', B'>`; `lmap` / `rmap` adapt one end. Because a bare
lambda does not expose its domain and codomain, the header ships a light arrow
wrapper, **`profn<F>`** (made with `make_profn`), holding a callable and itself
callable — the canonical instance, where `dimap` is `post . fn . pre`.
Introspection: `is_profunctor` (+ `_v` + concept `Profunctor`).

**`free.hpp`** — The Free monad: turns any functor into a monad. `free<F, A> =
Pure A | Roll (F<free<F, A>>)` — a tree of pure leaves and `F`-shaped layers,
letting you build a program as data and interpret it later. `lift_free` injects
one layer of `F` instructions; `free_bind` grafts a continuation onto every
leaf; **`fold_free(prog, onPure, onImpure)`** runs it against an algebra
`F<R> -> R`. `free<F, A>` registers `monad_traits` (and is a Functor via the
bridge), so every monad combinator applies. C++ has no higher-kinded types, so
`F` is a single-argument template-template parameter and the recursion is
carried by `std::shared_ptr` (heap; not constexpr); `F` must be a registered
Functor. (Note: `djinterp::free` shares its spelling with C's `::free` — qualify
it when `<cstdlib>` is in scope.)

**`cofree.hpp`** — The Cofree comonad, the dual of Free: `cofree<F, A> = A :<
F<cofree<F, A>>` — every node carries a head value *and* a layer of `F`, so it is
never empty (an annotated tree, or a non-empty stream with `F = maybe`).
`extract` reads the head, `extend` re-decorates each node from its whole
sub-tree, and **`unfold_cofree(seed, head_fn, layer_fn)`** coiteratively builds
one (the dual of `fold_free`). `cofree<F, A>` registers `functor_traits` and
`comonad_traits`, so `functor_map` and the generic `extract` / `extend` /
`duplicate` apply. Same `shared_ptr` recursion and registered-Functor
requirement as Free.

**`extractor.hpp`** — First-class *extractors / projections*: a callable
`Target(const Source&)` that reads ("projects") one feature out of a value,
elevated to a first-class value with its own combinators. The smallest unit of
the dataflow vocabulary — a read-only "lens": where producers spin a stream
from nothing and accumulators absorb a stream into a value, an extractor reads a
single value from a single value. Factories in **`namespace extractors`**:
`identity<Source>`, `constant`, `from_function`, `from_member` (data-member
pointer), `from_index<N>` (`std::get`); combinators `then_extract` (compose
`e2(e1(x))`), `fanout` (2- or 3-way → a tuple), `mapped` (post-transform the
output), `filtered` (predicate gate → `maybe`), `guarded` (source-side guard →
`maybe`), `defaulted` (replace `nothing` with a default), `try_extract` (catch →
`maybe`) — each combinator also has a single-arg adapter form so
`e | then_extract(f)`, `| mapped(f)`, `| filtered(p)` chain via `operator|`.
Runtime container drivers: `extract_all` (→ `vector`), `extract_first` (→
`maybe`), `extract_unique`, `extract_into_map`, `group_by_extractor`. Every
primitive is `D_CONSTEXPR`, so an extractor over constexpr callables is itself
constexpr (the drivers are runtime-only pre-C++20). Traits: `is_extractor<Fn,
Source>` (unary, non-void result), `extractor_result_t`, `is_maybe_extractor`
(+ `_v` + concepts `extractor_c` / `maybe_extractor_c`); reuses `maybe.hpp`'s
`is_maybe`. Includes `maybe.hpp` and the callable vocabulary.

**`producer.hpp`** — First-class *producers* (pull-based sources): a nullary
callable returning `producer_step<T>` (`has_value` + `value`; `make_step`/
`no_step` build them). Factories (flat in `djinterp`): `iterate`, `unfold`,
`range`/`iota`, `repeat`/`repeat_n`, `cycle`, `generate`, `empty`, `single`,
`take_n`/`drop_n`, `concat`/`interleave`, `transform`/`filter`,
`from_container`. Many are notionally infinite and become finite only when
bounded. Terminals `collect` (drain to vector), `for_each`, `fold`. Traits
`is_producer_step`, `is_producer`, `producer_value_type` (+ concepts
`producer_step_type`, `producer`).

**`consumer.hpp`** — First-class *consumers* (sinks): any callable
`void(const A&)`. Factories in `namespace consumers`: `print_to`, `write_to`,
`discard`, `count_into`, `filtered` (predicate gate), `mapped` (contramap),
`tee` (variadic broadcast), `batched` (every-N), `take`/`drop`, `conditional`
(predicate branch), `fallback` (try/catch). Type erasure:
`boxed_consumer<T>` = `std::function<void(const T&)>` + `box<T>`. Vocabulary
traits: `consumer_result`, `is_consumer`, `is_predicate`, `is_transformer`,
`is_boxable`, `contramap_input` (+ `_v` + concepts `consumes`,
`predicate_for`, `transformer_for`, `boxable_as`).

**`accumulator.hpp`** — First-class *accumulators*: a `(state, step, finalize)`
triple `accumulator<State, Input, Output, Step, Final>` parameterized on the
functor types (no `std::function`; constexpr-capable). Built via
`make_accumulator`. Pre-built (in `djinterp`): `sum`, `product`, `count`,
`count_if`, `min`/`max`/`min_by`/`max_by`, `mean`/`variance`/`stddev`
(Welford), `first`/`last`/`nth`, `joining`, `to_vector`, `histogram`,
`to_map_by`, `group_by`, `top_k`, `all/any/none_match`. Combinators
`contramap` (pre-map input), `map_output`, `filtered`, `take`, and the
headline `combine(a,b,…)` — runs many accumulators in lock-step over one pass,
yielding a tuple ("applicative folds"). Type erasure `boxed_accumulator<In,Out>`
+ `box_accumulator`. Rich contract traits (`has_*_type`, `has_*_method`,
`is_accumulator`, `is_boxed_accumulator`, `accumulator_state_t`/`input_t`/
`output_t`, concepts `accumulator_like`, …).

**`view.hpp`** — *Lazy, pull-based views* and the `operator|` pipeline — the
connective tissue of the module. `view_base` CRTP marker + `is_view`/
`has_begin_end` detectors. Fundamental views `ref_view` (non-owning),
`owning_view` (by-value), `iterator_pair_view`. Sources `iota_view`,
`repeat_view`, `generate_view`, `empty_view`, `single_view`. Adapters
`transform`, `filter`, `take`, `drop`, `take_while`, `drop_while`,
`enumerate`, `zip`, `concat`, `reverse`, `chunk`, `stride` (each a `*_view`
class + a `namespace views` factory returning an adapter). `operator|` accepts
`view | adapter`, `container | adapter` (auto-lifts to `ref_view`), and
`view/container | terminal`. Terminals: `to_vector`, `to<C>`, `count`, `fold`,
`for_each`, `any_of`/`all_of`/`none_of`. Traits `view_value_type`,
`is_adapter`/`is_terminal` (mutually exclusive), `is_pipeable_to_view`.

**`reduce.hpp`** *(C++17+)* — The *driver* half of the step/driver split: a pure
reducer `(acc, x) -> acc` is written once, domain-agnostic, and one of two
drivers performs the iteration. `reduce_rt(rf, acc, first, last)` /
`reduce_rt(rf, acc, iterable)` is a runtime loop — `constexpr`, so it also folds
at compile time over a constexpr range, and it pulls until the source ends
(lazy / large / infinite sources are fine). `reduce_ct` is a compile-time
recursion covering the two irreducibly-kinded packs:
`reduce_ct(rf, acc, value_list<Values...>)` for the NTTP domain (delegates to
`value_list`'s own fold) and `reduce_ct(rf, acc, type_c<std::tuple<Ts...>>)` for
the type domain (element types need **not** be default-constructible; the
reducer sees each as a `type_c<T>` carrier), with a convenience
`reduce_ct(rf, acc, std::tuple<Ts...>{})` value-passing overload for
default-constructible elements. The same reducer body serves all three domains;
only loop-vs-recursion and the leaf differ. Drivers are deliberately
unconstrained (one body, every domain); a C++20 caller may layer the `Reducer` /
`Transducer` concepts (`structural_traits.hpp`) at the call site. This is the
substrate the `transducer` spine and the stream modules hang off; empty under
C++11. Depends on the `[meta]` carriers (`carrier.hpp`'s `type_c`/`val`,
`value_list.hpp`).

**`transducer.hpp`** *(C++14+)* — Source/sink-agnostic *transducers*: a function
from one reducer (`(reducing_state<Acc>&, const Value&) -> void`) to another,
so one `map|filter|take` chain runs over any source and into any sink.
`transducer_base` CRTP marker; `reduced<Acc>`/`reducing_state<Acc>` carry the
accumulator + early-stop flag (`mark_done`/`is_done`/`unwrap_reduced`). Core
transducers (`namespace transducers`): `map`, `filter`, `filter_not`, `take`,
`drop`, `take_while`, `drop_while`, `distinct`, `tap`, `flat_map`,
`partition_by`. Composition `compose(t1,t2,…)` and `operator|`. Drivers
`transduce`, `transduce_into_vector`, `transduce_into_accumulator`,
`transduce_producer_to_consumer`, `into_reducer`. Traits `is_transducer`,
`is_reducer`, `is_reducing_state`, `reducing_state_acc_t`,
`transduces_reducer`, `transducer_result_t` (+ concepts).

**`interpolate.hpp`** *(C++17+)* — The type-agnostic *interpolation engine* (the
find-and-replace shared by `text_template` and `binary_template`, factored out
so neither owns it). An interpolation is a single left-to-right fold of *scan
events* into a *sink*, with three orthogonal policy axes. **Scanners** own
*where* the placeholders are and drive a pull cursor yielding one `piece` (a
literal run or a key) per step: `brace_scanner` (`{key}`, trimmed `{ key }`,
`{{ }}` escaped), `sigil_scanner` (`$name`, configurable sigil), `replay_scanner`
(replay a pre-scanned cache). **Resolvers** own *what* a key becomes (`key ->
resolution`, a minimal `maybe` with a lazy `or_else`): `empty_resolver`
(identity — the miss-leaves-it-untouched rule that makes *partial* interpolation
well-defined), `map_resolver` (inline bindings), `lookup_resolver` (adapt a
callable), `chain_resolver` (try one else the next via `or_else`, associative —
N frames collapse into one pass), `when_resolver` (gate a resolver on a key
predicate, composing conditions through `predicate.hpp`); factories `bindings` /
`lookup` / `chain` / `when`. **Sinks** own *how* output is assembled
(`literal(run)` + `value(resolved)`): `interp_string_sink` appends into a
buffer, and a byte sink / consumer / accumulator can stand in. The engine itself
is `interpolate_into(sink, scanner, resolver)` (segment-by-segment, no
intermediate structure). `recursive_resolver` (+ `recursive`) re-scans a hit's
value for value-level nested templates. A **lazy functor** `interpolation`
carries the template *and* a resolver chain in its type — consecutive
`.interpolate(...)` extend the chain and do no work until a terminal forces the
single pass — with `.recursive()` and `.prepare()` terminal options;
`prepared_interpolation` (+ `prepare` / `make_prepared` / `prepare_into`)
pre-parses once to render many times over a piece cache. C++20 concepts
`scanner_for` / `resolver_for` / `sink_for` face the three policies.
Self-suppresses below C++17.

**`functional.hpp`** — The **root header**: includes every sub-module above and
adds *eager*, generic, SFINAE-constrained free algorithms (in `djinterp::*`)
that work over STL containers, raw arrays (ptr+count), iterator pairs, and
initializer lists: `map`/`map_in_place`, `filter`/`filter_not`,
`fold_left`/`fold_right`/`reduce`/`scan`, `for_each`/`for_each_const`/
`for_each_indexed`, `any`/`all`/`none`, `count_if`, `find_if`/`find_last`,
`index_of`/`last_index_of`, `is_sorted`, `take`/`take_while`/`skip`/
`skip_while`, `flat_map`, `zip`/`zip_with`, `group_by`, `partition`,
`distinct`, `reverse`, `slice`, `range`. The eager layer coexists with the
lazy/role-based sub-modules; `view.hpp`'s `operator|` complements it.
Re-exports `is_callable`, `is_predicate`, `callable_result_t`.

---

# Part 2 — Appendix: per-module signature tables

Signatures are compacted: `typename`/`std::decay` SFINAE noise is dropped,
template parameters are shown only where they clarify shape, and `→` denotes
the return type. Factory return types named `internal::*_helper` are opaque
role-modeling types. `[98]` = also in the C++98 fallback path; `[11+]` =
C++11+ only within a dual-standard header.

## `function_traits.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `function_traits<T>` | `struct` (primary, inspects `&T::operator()`) | lambda/functor case; decay specialization strips cv/ref |
| `function_traits<R(Args...)>` | canonical specialization | base all others delegate to |
| specializations | `R(*)(Args...)`, `R(&)(Args...)`, `R(C::*)(Args...) [const][volatile][cv]`, `std::function<R(Args...)>` | uniform interface across callable shapes |
| `::return_type` | declared return type | |
| `::arity` | `static constexpr size_t` | named-parameter count |
| `::args_tuple` | `std::tuple<Args...>` | |
| `::arg<N>::type` / `arg_t<F,N>` | N-th parameter type | bounds-checked via `tuple_element` |
| `::is_noexcept` | `static constexpr bool` | false on the bare-function form |
| `::class_type` | owning class | member-fn specializations only |
| `return_type_t<F>` / `args_tuple_t<F>` / `arity_v<F>` | aliases | |
| `is_inspectable<F>` | `struct` → `bool` | false for generic/overloaded callables |
| `call_result_t<F, Args...>` | `alias` → result of `const F(Args...)` or `internal::call_nonesuch` | works on generic lambdas (expression probe) |
| `is_invocable_with<F, Args...>` | `struct` → `bool` (+`_v`) | concrete-arg call check |
| `is_invocable_r_with<R, F, Args...>` | `struct` → `bool` (+`_v`) | call check + result convertible to `R` (void = any) |

## `functional_traits.hpp` *(callable vocabulary; re-exports `function_traits.hpp`)*

| Entity | Signature | Notes |
|---|---|---|
| `is_callable<Fn, Args...>` | `struct` → `bool` (+`_v`) | derives `is_invocable_with`; const-lvalue `Fn` callable on `Args`; works on generic lambdas |
| `callable_result_t<Fn, Args...>` | `alias` (= `call_result_t`) | that call's result, or `internal::call_nonesuch`; gate on `is_callable` first |
| `is_predicate<Pred, Arg>` | `struct` → `bool` (+`_v`) | bool case of `is_invocable_r_with`; callable on one `Arg`, result → `bool` |
| `is_callable_v` / `is_predicate_v` | `constexpr bool` | `[C++14]` variable-template shorthands |

## `functional_concepts.hpp` *(C++20-only; concept faces of `functional_traits.hpp`)*

| Entity | Signature | Notes |
|---|---|---|
| `Callable<Fn, Args...>` | `concept` | face of `is_callable`; generic/cross-cutting only |
| `Predicate<Pred, Arg>` | `concept` | face of `is_predicate` (empty header under < C++20) |

## `structural_traits.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `has_match_result_type<T>` | member-typedef detector (+`_v`) | via `D_DEFINE_HAS_MEMBER_TYPE` |
| `has_find_method<T, In, Result>` | `struct` → `bool` (+`_v`) | detects `bool find(const In&, size_t&, Result&)` |
| `is_nullary_callable<T>` | `struct` → `bool` (+`_v`) | underpins source detection |
| `is_unary_callable<T, Arg>` | `struct` → `bool` (+`_v`) | transform/predicate step shape |
| `produces_optional_like<T>` | `struct` → `bool` (+`_v`) | nullary callable returning bool-testable + dereferenceable (unfold source step) |

## `predicate.hpp` *(dual-standard)*

| Entity | Signature | Notes |
|---|---|---|
| `predicate_and(p1,p2)` | → `internal::predicate_and_combinator<...>` | `p1(x) && p2(x)` |
| `predicate_or(p1,p2)` | → `predicate_or_combinator` | `p1(x) \|\| p2(x)` |
| `predicate_xor(p1,p2)` | → `predicate_xor_combinator` | `p1(x) != p2(x)` |
| `predicate_not(p)` | → `predicate_not_combinator` | `!p(x)` |
| `predicate_nand(p1,p2)` / `predicate_nor(p1,p2)` | → resp. combinators | `!(p1&&p2)` / `!(p1\|\|p2)` |
| `all_of(p1, rest...)` | `[11+]` → folded AND combinator | short-circuit, any arity |
| `any_of(p1, rest...)` | `[11+]` → folded OR combinator | short-circuit |
| `none_of(preds...)` | `[11+]` → NOR combinator | |
| `is_predicate_and/or/xor/not/nand/nor<T>` | `struct` → `bool` (+`_v`) `[11+]` | structural match on each combinator template |
| `is_predicate_combinator<T>` | `struct` → `bool` (+`_v`) `[11+]` | true for any of this header's combinators |
| `is_predicate<Pred, Args...>` | `struct` → `bool` (+`_v`) `[11+]` | behavioral: callable + bool-convertible result |
| concepts | `predicate_combinator<T>`, `predicate<Pred, Args...>` | `[C++20]` |

## `compose.hpp` *(dual-standard)*

Section 0 — vocabulary traits/concepts:

| Entity | Signature | Notes |
|---|---|---|
| `callable_result<F, Args...>` / `callable_result_t` | result of `F(Args...)`; SFINAE-friendly `::type` | |
| `is_invocable<F, Args...>` | `struct` → `bool` (+`_v`) | |
| `is_invocable_r<R, F, Args...>` | `struct` → `bool` (+`_v`) | result convertible to `R`, gated behind invocability |
| `is_unary_transformer<F, In>` | `struct` → `bool` (+`_v`) | callable with one `In`, non-void result |
| `is_composable<Outer, Inner, In>` | `struct` → `bool` (+`_v`) | `Outer(Inner(In))` well-formed |
| `composition_result<Outer,Inner,In>` / `_t` | result type of `Outer(Inner(In))` | |
| `is_composed_transformer<T>` | `struct` → `bool` (+`_v`) | exposes `.first()`/`.second()` |
| `is_memoized<T>` | `struct` → `bool` (+`_v`) | exposes `.clear_cache()`/`.cache_size()` |
| concepts | `invocable_with`, `unary_transformer`, `composable`, `composed_transformer_like`, `memoized_like` | `[C++20]` |

Factories (return opaque `internal::*_helper`):

| Entity | Signature | Notes |
|---|---|---|
| `compose(f, g)` | `[98]` → composed transformer | math order `f(g(x))` |
| `pipe(f, g)` / `compose_transformer(f, g)` | `[98]` → composed transformer | left-to-right `g(f(x))` |
| `compose_all(fns...)` | `[11+]` → folded transformer | right-to-left `f(g(h(x)))` |
| `pipe_all(fns...)` | `[11+]` → folded transformer | left-to-right `h(g(f(x)))` |
| `partial_back(f, z)` | `[11+]` → `partial_consumer_helper` | binds last arg: `(x,y) -> f(x,y,z)` |
| `tap(f)` | `[11+]` → `tap_helper` | runs `f(x)`, returns `x` |
| `memoize(f)` / `memoize<In,Out>(f)` | `[98]` → `memoize_helper` | cache pure fn; `In` must be `<`-comparable (map key) |
| `fix(f)` | `[11+]` → `fix_helper` | Y-combinator: `f(self, args...)` |

## `curry.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `is_predicate<Fn, Args...>` | `struct` → `bool` (+`_v`) | callable + result convertible to bool |
| `is_nullary_predicate<Fn>` | `struct` → `bool` (+`_v`) | arity-0 |
| `is_unary_predicate<Fn, Arg>` | `struct` → `bool` (+`_v`) | arity-1 |
| `is_binary_predicate<Fn, A, B>` | `struct` → `bool` (+`_v`) | arity-2 |
| concepts | `predicate_for`, `nullary_predicate`, `unary_predicate`, `binary_predicate` | `[C++20]` |
| `curry(f)` | → `internal::curry_helper<F>` | auto-arity; invoke when enough args, else return curried |
| `curry_n<N>(f)` | → `curry_n_helper<N, F>` | explicit arity for un-inferrable callables |
| `uncurry(f)` | → `uncurry_helper<F>` | `f(a)(b)(c)` → `f(a,b,c)` |
| `flip(f)` | → `flip_helper<F>` | swaps first two args |
| `identity` | `constexpr internal::identity_fn_helper{}` | returns its argument |
| `always(v)` / `constant(v)` | → `always_helper<V>` | K combinator: ignores args, returns `v` |
| `never` | `constexpr internal::never_helper{}` | always-false predicate |

## `comparator.hpp` *(namespace `comparators`; dual-standard)*

Factories & combinators (return opaque `internal::*_helper`; comparator =
binary `(a,b)->bool`, `std::sort` convention):

| Entity | Signature | Notes |
|---|---|---|
| `natural<T>()` | `[98]` → `natural_helper<T>` | `operator<` ordering (≈ `std::less`) |
| `by_key(key_fn)` | `[98]` → `by_key_helper<KeyFn>` | compare extracted keys via `<` |
| `by_member(&C::m)` | `[98]` → `by_key_helper<member_accessor<C,M>>` | compare by data-member pointer |
| `by_function(fn)` | `[98]` → `by_function_helper<Fn>` | adapt a raw binary-bool lambda into the combinator family |
| `lifted(cmp, key_fn)` | `[98]` → `lifted_helper<Cmp,KeyFn>` | `by_key` but compares keys with `cmp` |
| `reversed(cmp)` | `[98]` → `reversed_helper<Cmp>` | invert ordering |
| `reversed()` | `[98]` → `reversed_adapter` | adapter form: `cmp \| reversed()` |
| `then(c1, c2)` | `[98]` → `then_helper<P,S>` | tie-breaker: fall back to `c2` on `c1` equivalence |
| `then(c2)` | `[98]` → `then_adapter<S>` | adapter form: `c1 \| then(c2)` |
| `equal_under(cmp)` | `[98]` → `equal_under_helper<Cmp>` | binary `==` derived from a comparator |
| `less_than(cmp, x)` | `[98]` → `less_than_helper` | unary `v -> cmp(v, x)` |
| `greater_than(cmp, x)` | `[98]` → `greater_than_helper` | unary `v -> cmp(x, v)` |
| `operator\|(cmp, then_adapter)` / `(cmp, reversed_adapter)` | `[98]` → `then_helper` / `reversed_helper` | tightly typed RHS |

Traits (at `djinterp` scope):

| Entity | Signature | Notes |
|---|---|---|
| `is_comparator<Cmp, T>` | `struct` → `bool` (+`_v`, concept `is_comparator_c`) | callable `(const T&, const T&)` → bool |
| `is_binary_predicate<Fn, A, B>` | `struct` → `bool` (+`_v`, `_c`) | `Fn(const A&, const B&)` → bool |
| `is_unary_predicate<Fn, V>` | `struct` → `bool` (+`_v`, `_c`) | `Fn(const V&)` → bool |
| `has_result_type<T>` | `struct` → `bool` (+`_v`) | nested `result_type` typedef hint |

## `pipeline.hpp`

`function_pipeline<T>` — eager typed pipeline (holds `vector<T>` + error flag):

| Member / free fn | Signature | Notes |
|---|---|---|
| `from(container)` / `from(vector&&)` / `from(init_list)` / `from(ptr, n)` | `static function_pipeline` | source builders |
| `of(args...)` | `static function_pipeline` | variadic source |
| `error(code=-1)` | `static function_pipeline` | error pipeline |
| `pipeline_from(container)` / `pipeline_from(ptr, n)` | free → `function_pipeline<V>` | |
| `map(fn)` | → `function_pipeline<R>` | `R = callable_result_t<Fn,const T&>` |
| `filter(pred)` / `filter_not(pred)` | → `function_pipeline` | |
| `fold(init, fn)` | → `Acc` | |
| `for_each(fn)` | → `const function_pipeline&` | chainable |
| `take(n)` / `take_last(n)` / `take_while(pred)` | → `function_pipeline` | |
| `skip(n)` / `skip_while(pred)` | → `function_pipeline` | |
| `slice(start, end, step=1)` | → `function_pipeline` | |
| `distinct()` / `distinct(eq)` / `reversed()` / `sorted()` / `sorted(cmp)` | → `function_pipeline` | |
| `flat_map(fn)` | → `function_pipeline<R>` | fn returns a container |
| `partition_pipe(pred)` | → `pair<function_pipeline, function_pipeline>` | (pass, fail) |
| `group_by(key_fn)` | → `map<Key, vector<T>>` | |
| `zip_with(other, fn)` | → `function_pipeline<R>` | |
| `to_vector() const&` / `to_vector() &&` | → `vector<T>` | terminal (ref-qualified pair) |
| `reduce(fn)` | → `T` | non-empty required |
| `any(pred)` / `all(pred)` / `none(pred)` | → `bool` | |
| `count(pred)` | → `size_t` | |
| `begin() / end()` | const iterators | range-for support |
| `is_pipeline<T>` (+`_v`, concept `pipeline_type`) | `struct` → `bool` | |
| `pipeline_value_type<P>` / `_t` | element type | SFINAE-friendly |
| `is_pipeline_mapper<Fn,T>` / `is_pipeline_predicate<Pred,T>` (+`_v`, concepts) | `struct` → `bool` | mapper/predicate shape over `const T&` |

## `fn_builder.hpp`

`fn_builder<Input, Current, Chain>` — fluent, value-captured transform chain:

| Member / free fn | Signature | Notes |
|---|---|---|
| `create()` / `make_builder<T>()` | `static` / free → identity-seeded builder | |
| `map(fn)` / `and_then(fn)` | → `fn_builder<In, R, map_chain<...>>` | `and_then` = alias |
| `filter(pred)` / `where(pred)` | → `fn_builder<In, Cur, filter_chain_step<...>>` | `where` = alias |
| `take(n)` / `skip(n)` / `distinct()` / `reversed()` | → `fn_builder<In, Cur, *_chain<...>>` | |
| `sorted(cmp)` | → `fn_builder<In, Cur, sorted_chain<...>>` | |
| `flat_map(fn)` | → `fn_builder<In, R, flat_map_chain<...>>` | |
| `execute(vector/container/ptr,n)` / `operator()(container)` | → `vector<Current>` | run the chain |
| `fold(input, init, fn)` | → `Acc` | terminal |
| `count(input)` / `any(input)` | → `size_t` / `bool` | terminal |
| `chain()` | → `const Chain&` | introspection/boxing |
| `boxed_fn_builder<In, Out>` / `box_builder(b)` | type-erased builder + factory | `std::function` overhead |
| `is_fn_builder<T>` / `is_boxed_fn_builder<T>` (+`_v`, concepts) | `struct` → `bool` | |
| `fn_builder_input_type<B>` / `_current_type<B>` (+`_t`) | element types | SFINAE-friendly |
| `is_fn_builder_mapper<Fn,T>` / `_predicate<Pred,T>` (+`_v`, concepts) | `struct` → `bool` | |

## `filter.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `filter_op_fn<T>` | `using = std::function<vector<size_t>(const vector<T>&)>` | an op returns passing indices |
| `filter_result_status` | `enum class` | op status |
| `filter_result<T>` | `class` (success / error ctors) | status + indices + elements |
| `filter_chain<T>` | `class`: `add(op)`, `apply(input)`, `length()` | sequential ops (erased) |
| `filter_union<T>(chains, input)` | → `filter_result<T>` | OR over chains |
| `filter_intersection<T>(chains, input)` | → `filter_result<T>` | AND over chains |
| `filter_difference<T>(include, exclude, input)` | → `filter_result<T>` | A − B |
| `filter_iterator<T>` | `class` | lazy iteration over filtered results |
| `filter_builder<T>` | `class` (fluent) | see below |
| `filter_builder` ops | `take_first/last/nth(n)`, `skip_first/last(n)`, `head/tail/init/rest()`, `range(s,e)`, `slice(s,e,step)`, `where(pred)`, `where_not(pred)`, `at(i)`, `at_indices(v)`, `distinct()`/`distinct(eq)`, `reverse()` | each → `filter_builder&` |
| `filter_builder` terminals | `apply(input)`→`filter_result<T>`, `build_chain() const&/&&`→`filter_chain<T>`, `iterator(input)`, `any/all/none_match(input)`→`bool`, `count_matches(input)`→`size_t` | |
| `typed_filter<T, Chain>` | `class`: `create()`, `where(pred)`, `take_first(n)`, `skip_first(n)`, `apply(input)`, `to_chain()` | std::function-free fast path |
| `make_typed_filter<T>()` | → `typed_filter<T, typed_identity<T>>` | |
| filterable detectors | `has_begin/end/value_type/push_back/insert/size/empty/iterator/const_iterator/filter_method<T>` | `struct` → `bool` |
| `is_iterable<T>` / `is_output_capable<T>` / `is_filterable<T>` | `struct` → `bool` (+`_v`, concept `filterable_c`) | composite contract |
| `filterable_value_t<T>` | `using` → value_type or `nonesuch` | |
| `is_filter_operation<Fn,Elem>` | `struct` → `bool` (+`_v`, concept `filter_operation_c`) | `filter_op_fn` protocol |
| `is_filter_applicable<T,Elem>` | `struct` → `bool` (+`_v`, concept `filter_applicable_c`) | exposes `.apply(const vector<Elem>&)` |
| `is_filter_result<T>` | `struct` → `bool` (+`_v`, concept `filter_result_c`) | ok/indices/elements surface |

## `monad.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `monad_traits<M>` | primary `struct` (undefined) | specialize to expose `value_type`, `rebind<U>`, static `unit`, static `bind`, `is_specialized` |
| `is_monad<T>` | `struct` → `bool` (+`_v`, concept `monad`) | has a `monad_traits` specialization |
| `monad_value_type<M>` / `_t` | inner `T` | SFINAE-friendly |
| `monad_rebind<M, U>` / `_t` | `M<U>` | |
| `is_monadic_function<F, M>` (+`_v`, concept `monadic_function_for`) | `struct` → `bool` | Kleisli arrow: `F(value) -> monad` |
| `is_bindable<M, F>` (+`_v`, concept `bindable_with`) | `struct` → `bool` | `monad_bind(M, F)` well-formed |
| `is_mappable<M, F>` (+`_v`, concept `mappable_with`) | `struct` → `bool` | `monad_map(M, F)` well-formed |
| `is_monad_combinator<C, M>` (+`_v`, concept `monad_combinator_for`) | `struct` → `bool` | has `apply(M)` |
| `monad_unit<M>(value)` | → `M` | lift (M explicit) |
| `monad_bind(m, f)` | → monad | `f : T -> M<U>`; delegates to `monad_traits::bind` |
| `monad_map(m, f)` | → monad | functorial; via bind+unit |
| `monad_join(mm)` | → inner monad | flatten one layer |
| `monad_then(m1, m2)` | → decayed `M2` | sequence, discard first value |
| `kleisli_compose(f, g)` | → `internal::kleisli_helper<F,G>` | `f >=> g` |
| `lift_m2(ma, mb, f)` | → `rebind<C>` of first monad | applicative binary lift (left-biased) |
| `bind_with(f)` / `map_with(f)` / `then_with(other)` | → `internal::*_combinator` | pipeline RHS |
| `operator\|(monad, combinator)` | → monad | SFINAE: LHS is monad, RHS has `apply(monad)` |

## `maybe.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `nothing_t` / `nothing_v` | tag struct / value | unambiguous empty case |
| `maybe<T>` | `class` (union storage; constexpr-friendly C++20) | "nothing" or "just(x)" |
| ctors | `maybe()`, `maybe(nothing_t)`, `maybe(const T&)`, `maybe(T&&)`, copy, move(noexcept-cond) | |
| assignment | `=(const maybe&)`, `=(maybe&&)`, `=(nothing_t)`, `=(const T&)` | |
| `has_value()` / `is_nothing()` / explicit `operator bool()` | → `bool` | |
| `value() const& / & / &&` | → `T` ref | UB when empty |
| `value_or(default)` | → `T` | default evaluated unconditionally |
| `expect(message)` | → `const T&` | throws `runtime_error` if empty |
| `reset()` / `emplace(args...)` | → `void` / `T&` | |
| `map(fn)` | → `maybe<U>` | functorial |
| `and_then(fn)` | → `maybe<U>` | monadic bind (fn returns maybe) |
| `or_else(fn)` | → `maybe<T>` | fn returns maybe<T> on empty |
| `filter(pred)` | → `maybe<T>` | nothing if pred fails |
| `match(on_just, on_nothing)` | → common type | pattern match |
| `operator==(maybe, maybe)` | → `bool` | |
| `just(v)` | → `maybe<decay<V>>` | |
| `nothing<T>()` | → `maybe<T>` | T explicit |
| `from_pointer(ptr)` | → `maybe<T>` | nothing if null, else just(*ptr) |
| `from_predicate(v, pred)` | → `maybe<T>` | just(v) if pred(v) |
| `or_else_with(default)` / `unwrap_or_with(default)` | → `or_else_combinator` | pipeline RHS (aliases) |
| `filter_with(pred)` | → `filter_combinator` | pipeline RHS |
| `expect_with(message)` | → `expect_combinator` | pipeline RHS, may throw |
| `operator\|(maybe, combinator)` | → result of combinator | SFINAE on `.apply(maybe<T>)` |
| `monad_traits<maybe<T>>` | specialization | `unit` (=just), `bind` |
| `zip_with(ma, mb, f)` | → `maybe<C>` | just(f(a,b)) iff both present |
| `flatten(maybe<maybe<T>>)` | → `maybe<T>` | = monad_join |
| `collect(container_of_maybe)` | → `maybe<vector<T>>` | just(all) or nothing (sequence) |
| `is_maybe<T>` (+`_v`, concept `maybe_type`) | `struct` → `bool` | |
| `is_maybe_predicate<Pred, T>` (+`_v`, concept `maybe_predicate_for`) | `struct` → `bool` | `Pred(const T&)` → bool |

## `result.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `result<T, E>` | `class` (discriminated union; default ctor deleted) | ok(T) or err(E) |
| ctors | `result(ok_tag, T)`, `result(err_tag, E)` (copy/move each), copy, move(noexcept-cond) | tags internal; use `ok`/`err` |
| `is_ok()` / `is_err()` / explicit `operator bool()` | → `bool` | |
| `value() const&/&/&&` | → `T` ref | UB when err |
| `error() const&/&` | → `E` ref | UB when ok |
| `value_or(default)` | → `T` | |
| `unwrap(message)` | → `const T&` | throws `runtime_error` if err |
| `map(fn)` | → `result<U, E>` | err propagates |
| `map_err(fn)` | → `result<T, F>` | ok propagates |
| `and_then(fn)` | → `result<U, E>` | bind on ok side (fn returns result) |
| `or_else(fn)` | → `result` | recover from err |
| `match(on_ok, on_err)` | → common type | |
| `ok()` / `err()` | → `maybe<T>` / `maybe<E>` | lossy conversions |
| `operator==(result, result)` | → `bool` | |
| `ok<T,E>(value)` / `err<T,E>(error)` | → `result<T,E>` | T and E explicit |
| `or_value_with(default)` / `map_err_with(fn)` / `unwrap_with(message)` | → `internal::*_combinator` | pipeline RHS |
| `operator\|(result, combinator)` | → result of combinator | SFINAE on `.apply(result<T,E>)` |
| `monad_traits<result<T,E>>` | specialization over success side | `unit` (=ok), `bind`; err preserved |
| `collect(container_of_result)` | → `result<vector<T>, E>` | ok(all) or first err (short-circuit; SFINAE on element `is_result`) |
| `combine(ra, rb, f)` | → `result<C, E>` | ok(f(a,b)) if both ok else first err |
| `to_maybe(result)` | → `maybe<T>` | lossy (drops error) |
| `is_result<T>` (+`_v`, concept `result_type`) | `struct` → `bool` | |
| `result_value_type<R>` / `result_error_type<R>` (+`_t`) | T / E extractors | SFINAE-friendly |
| `is_result_value_mapper<Fn,T>` / `is_result_error_mapper<Fn,E>` (+`_v`, concepts) | `struct` → `bool` | handler shapes |

## `functor.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `functor_traits<F>` | primary `struct` (undefined) | specialize to expose `value_type`, static `map`, `is_specialized`; `rebind<U>` intentionally *not* in the protocol |
| `functor_traits<F>` *(monad bridge)* | partial spec keyed on `is_monad<F>` | derives `map` from `monad_map`; covers `maybe`/`result`/any monad with no per-type wiring |
| `is_functor<T>` | `struct` → `bool` (+`_v`, concept `Functor`) | has a `functor_traits` specialization |
| `functor_value_type<F>` / `_t` | inner `T` | SFINAE-friendly |
| `functor_map(fa, f)` | → deduced (`F<U>`, or the instance's transformed view/producer) | `D_NODISCARD D_CONSTEXPR`; delegates to `functor_traits<F>::map` |
| `is_fmappable<F, Fn>` (+`_v`, concept `fmappable_with`) | `struct` → `bool` | `functor_map(F, Fn)` well-formed |

Instances: `maybe`/`result` (automatic, via the bridge); `view` and `producer`
via the one-line `functor_traits` specializations shipped as insertion snippets
in their headers. `transducer` is **not** an instance (it is a morphism, not an
`F<A>` context).

## `applicative.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `applicative_traits<F>` | primary `struct` (undefined) | specialize to expose `value_type`, static `pure`, static `ap`, `is_specialized` |
| `applicative_traits<F>` *(monad bridge)* | partial spec keyed on `is_monad<F>` | `pure` = `monad_unit`, `ap` = `monad_bind`+`monad_map`; covers `maybe`/`result`/any monad |
| `is_applicative<T>` | `struct` → `bool` (+`_v`, concept `Applicative`) | has an `applicative_traits` specialization |
| `applicative_value_type<F>` / `_t` | inner `T` | SFINAE-friendly |
| `pure<F>(value)` | → `F` | `D_NODISCARD D_CONSTEXPR`; lift (F explicit), dual of `monad_unit` |
| `ap(ff, fa)` | → deduced | `F<a->b> -> F<a> -> F<b>` (Haskell `<*>`); delegates to `applicative_traits<F>::ap`; short-circuits for `maybe`/`result` |
| `lift_a2(fa, fb, f)` | → deduced | `D_NODISCARD D_CONSTEXPR`; generic via `functor_map(curry2(f))` then `ap`; left-biased; applicative counterpart of `lift_m2` |
| `is_applicable<Ff, Fa>` (+`_v`, concept `applicable_with`) | `struct` → `bool` | `ap(Ff, Fa)` well-formed |

Instances: `maybe`/`result` (automatic, via the bridge). Applicative instances
for `view`/`producer` are out of scope for this change (the roadmap item
unifies *mapping*; `ap` over a view/producer is a separate design choice).

## `foldable.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `foldable_traits<F>` | primary `struct` (undefined; 2nd param a SFINAE hook) | specialize to expose `value_type`, static `fold_left`, `is_specialized`; **no monad bridge** |
| `is_foldable<T>` | `struct` → `bool` (+`_v`, concept `Foldable`) | has a `foldable_traits` specialization |
| `foldable_value_type<F>` / `_t` | inner `T` | SFINAE-friendly |
| `fold_left(fa, init, f)` | → deduced | the one obligation, delegated; `f : (Acc, const T&) -> Acc`; `D_NODISCARD D_CONSTEXPR` |
| `fold_right(fa, init, f)` | → `Acc` | `f : (const T&, Acc) -> Acc`; derived (materialize + reverse); finite only; `D_CONSTEXPR20` |
| `fold_map(fa, f, empty, combine)` | → `M` | map each elem to a monoid value then combine; monoid passed as `(empty, combine)`; `D_CONSTEXPR` |
| `fold_to_vector(fa)` | → `std::vector<T>` | collect in fold order; `D_CONSTEXPR20` |
| `fold_length(fa)` | → `std::size_t` | element count; `D_CONSTEXPR` |
| `fold_is_empty(fa)` | → `bool` | no elements?; `D_CONSTEXPR` |
| `fold_any(fa, p)` / `fold_all(fa, p)` | → `bool` | existential / universal (vacuous-true); no short-circuit; `D_CONSTEXPR` |

Instances: `maybe` / `result` (over the 0-or-1 carried value; `err` is empty),
`view` (iterate the pipeline), `producer` (pull a copy to exhaustion). Each is
keyed on the module's structural trait (`is_maybe` / `is_result` / `is_view` /
`is_producer`), so the instance sets are mutually exclusive and never ambiguous.

## `semigroup.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `semigroup_traits<T>` | primary `struct` (undefined; 2nd param a SFINAE hook) | specialize to expose static `combine`, `is_specialized` |
| `is_semigroup<T>` | `struct` → `bool` (+`_v`, concept `Semigroup`) | has a `semigroup_traits` specialization |
| `mappend(a, b)` | → `T` | the associative combine (`<>`); dispatches to `semigroup_traits<T>::combine`; `D_NODISCARD D_CONSTEXPR`; **not** named `combine` (taken by `accumulator.hpp`) |

## `monoid.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `monoid_traits<T>` | primary `struct` (undefined; 2nd param a SFINAE hook) | specialize to expose static `empty`, `is_specialized`; combine comes from `semigroup_traits<T>` |
| `is_monoid<T>` | `struct` → `bool` (+`_v`, concept `Monoid`) | has a `monoid_traits` specialization (⇒ `is_semigroup` too) |
| `mempty<T>()` | → `T` | the identity element; `T` explicit (dual of `monad_unit`/`pure`); `D_NODISCARD D_CONSTEXPR` |
| `mconcat(fa)` | → `M` | combine every element of a Foldable of monoid `M`; empty → `mempty<M>()`; `D_NODISCARD D_CONSTEXPR` |
| `fold_monoid(fa, f)` | → `M` | map each element of a Foldable into `M` then `mconcat`; identity/combine from the protocol; `D_NODISCARD D_CONSTEXPR` |
| `monoids::sum<T>` / `product<T>` | newtype (`.value`) | `+`/`0`, `*`/`1` |
| `monoids::all` / `any` | newtype (`.value`) | `&&`/`true`, `\|\|`/`false` |
| `monoids::min<T>` / `max<T>` | newtype (`.value`) | keep smaller/larger; identity `numeric_limits<T>::max()`/`lowest()` |

Instances: `std::string`, `std::vector<T>` (concatenation), and the `monoids::`
newtypes — each specializing both `semigroup_traits` and `monoid_traits`.
`foldable.hpp` additionally gains `foldable_traits<std::vector<T>>` so the folds
(and `mconcat`/`fold_monoid`) work on vectors.

## `alternative.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `alternative_traits<F>` | primary `struct` (undefined; 2nd param a SFINAE hook) | specialize to expose `value_type`, static `empty`, static `choice`, `is_specialized` |
| `is_alternative<T>` | `struct` → `bool` (+`_v`, concept `Alternative`) | has an `alternative_traits` specialization |
| `alternative_value_type<F>` / `_t` | inner `A` | SFINAE-friendly |
| `aempty<F>()` | → empty `F` | the failure/identity; `F` explicit (like `mempty`); `D_NODISCARD D_CONSTEXPR`; **not** named `empty` |
| `alt(a, b)` | → deduced | associative choice (`<|>`); `maybe` first-with-value, sequences concatenate; `D_NODISCARD D_CONSTEXPR` |
| `asum(fa)` | → `M` | choose across a Foldable of (uniform) alternatives, `alt` from `aempty`; counterpart of `mconcat`; `D_NODISCARD D_CONSTEXPR` |

Instances: `maybe` (uniform: `nothing` / first-with-value), `view` & `producer`
(empty sequence / concatenation, keyed on `is_view` / `is_producer`). `result`
is excluded (no canonical empty). For the sequence families `choice` yields a
concat context with the same `value_type` but a different static type (as
`functor_map` yields a transform view); `asum` is intended for the uniform case.

## `traversable.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `traversable_traits<T>` | primary `struct` (undefined; 2nd param a SFINAE hook) | specialize to expose `value_type`, static `traverse`, `is_specialized` |
| `is_traversable<T>` | `struct` → `bool` (+`_v`, concept `Traversable`) | has a `traversable_traits` specialization |
| `traversable_value_type<T>` / `_t` | inner `A` | SFINAE-friendly |
| `traverse(ta, f)` | → `F<T'<B>>` | `f : A -> F<B>`; shape-preserving for `maybe`/`result`, materialised (`F<vector<B>>`) for sequences; `D_NODISCARD D_CONSTEXPR` |
| `sequence(ta)` | → `F<T<A>>` | `T<F<A>>` inside out; `= traverse(ta, identity)`; `D_NODISCARD D_CONSTEXPR` |

Instances: `maybe`, `result` (shape-preserving), and `view` / `producer` /
`std::vector` (materialising to `F<std::vector<B>>`). Effects `F`: `maybe`,
`result`. The effect is recovered from `decltype(f(a))`, so the empty-structure
branch is well-typed; sequences must be finite. Short-circuit (`nothing` /
first `err`) is preserved by the applicative.

## `bifunctor.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `bifunctor_traits<F>` | primary (undefined; 2nd param SFINAE hook) | obligation `bimap`; `first_type`, `second_type` |
| `bimap(fab, f, g)` | → `F<C, D>` | map both parameters; `D_CONSTEXPR` |
| `map_first(fab, f)` / `map_second(fab, g)` | → `F<C, B>` / `F<A, D>` | one side; `= bimap` with identity other side |
| `is_bifunctor<T>` | → bool (+`_v`, concept `Bifunctor`) | |
| `bifunctor_first_type<F>` / `second_type<F>` (+`_t`) | the parameter types | |

Instances: `std::pair`, `kv_pair`, `result` (`bimap` = `map` ∘ `map_err`).

## `comonad.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `comonad_traits<W>` | primary (undefined; 2nd param SFINAE hook) | obligations `extract` + `extend` |
| `extract(w)` | `W<A>` → `A` | the counit |
| `extend(w, f)` | `W<A>`, `(W<A>->B)` → `W<B>` | the co-bind |
| `duplicate(w)` | `W<A>` → `W<W<A>>` | `= extend` with identity |
| `is_comonad<T>` (+`_v`, concept `Comonad`); `comonad_value_type<W>` (+`_t`) | | |

Instances: `std::pair<E,A>`, `kv_pair<K,V>` (Env comonad, focus = second).

## `profunctor.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `profn<F>` / `make_profn(f)` | the arrow wrapper | holds + forwards a callable |
| `profunctor_traits<P>` | primary (undefined; 2nd param SFINAE hook) | obligation `dimap` |
| `dimap(p, pre, post)` | `P<A,B>` → `P<A',B'>` | `pre : A'->A`, `post : B->B'` |
| `lmap(p, pre)` / `rmap(p, post)` | one end | `= dimap` with identity other side |
| `is_profunctor<T>` (+`_v`, concept `Profunctor`) | | |

Instance: `profn<F>` (`dimap` = `post . fn . pre`).

## `free.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `free<F, A>` | `Pure A \| Roll (F<free>)` | `pure` / `roll` factories; `F` a template-template param |
| `lift_free(fa)` | `F<A>` → `free<F, A>` | one layer of instructions |
| `free_map(p, f)` / `free_bind(p, k)` | functor / monad maps | |
| `fold_free(p, onPure, onImpure)` | → `R` | the interpreter; `onImpure : F<R> -> R` |
| registration | `monad_traits<free<F,A>>` (+ Functor via bridge) | every monad combinator applies |

Heap recursion (shared_ptr); not constexpr. `F` must be a registered Functor.
Qualify `djinterp::free` (clashes with C `::free`).

## `cofree.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `cofree<F, A>` | `A :< F<cofree>` | `make` factory; `head` / `unwrap`; never empty |
| `unfold_cofree(seed, head_fn, layer_fn)` | builds a cofree | dual of `fold_free` |
| `cofree_map(w, f)` / `cofree_extend(w, f)` | functor / co-bind | |
| registration | `functor_traits` + `comonad_traits<cofree<F,A>>` | `functor_map`, `extract`/`extend`/`duplicate` apply |

Heap recursion (shared_ptr); not constexpr. `F` must be a registered Functor.

## `extractor.hpp` *(namespace `extractors`; primitives `D_CONSTEXPR`)*

Factories & combinators (return opaque `internal::*_helper`; extractor =
`Target(const Source&)`):

| Entity | Signature | Notes |
|---|---|---|
| `identity<Source>()` | → `identity_helper<Source>` | return source unchanged |
| `constant(value)` | → `constant_helper<V>` | ignore source, return stored value |
| `from_function(fn)` | → `function_helper<Fn>` | wrap an arbitrary `Target(const Source&)` callable |
| `from_member(&C::m)` | → `member_helper<C,M>` | pointer-to-data-member projection |
| `from_index<N>()` | → `index_helper<N>` | `std::get<N>` on tuple-likes |
| `then_extract(inner, outer)` / `then_extract(outer)` | → `composed_helper` / `then_extract_adapter` | compose `outer(inner(x))`; adapter form `e \| then_extract(f)` |
| `fanout(e1, e2)` / `fanout(e1, e2, e3)` | → `fanout2_helper` / `fanout3_helper` | tuple of two / three outputs |
| `mapped(e, fn)` / `mapped(fn)` | → `mapped_helper` / `mapped_adapter` | post-transform the output; adapter form |
| `filtered(e, p)` / `filtered(p)` | → `filtered_helper` / `filtered_adapter` | predicate gate → `maybe<Target>`; adapter form |
| `guarded(e, guard)` | → `guarded_helper` | source-side guard → `maybe<Target>` |
| `defaulted(maybe_e, default)` | → `defaulted_helper` | replace `nothing` with a default (unwraps the `maybe`) |
| `try_extract(e)` | → `try_helper` | catch exceptions → `maybe<Target>` |
| `operator\|(extractor, then_extract/mapped/filtered_adapter)` | → resp. helper | tightly typed RHS |

Container drivers (runtime-only; constexpr from C++20):

| Entity | Signature | Notes |
|---|---|---|
| `extract_all(e, container)` | → `vector<Target>` | project every element |
| `extract_first(e, container)` | → `maybe<Target>` | first element, or `nothing` if empty |
| `extract_unique(e, container)` | → `vector<Target>` | de-duplicated projections |
| `extract_into_map(key_e, value_e, c)` | → `map<Key, Value>` | two extractors → a map |
| `group_by_extractor(e, container)` | → `map<Key, vector<Source>>` | bucket sources by extracted key |

Traits (at `djinterp` scope):

| Entity | Signature | Notes |
|---|---|---|
| `is_extractor<Fn, Source>` | `struct` → `bool` (+`_v`, concept `extractor_c`) | callable `(const Source&)`, non-void result |
| `extractor_result_t<Fn, Source>` | decayed result type | `internal::no_result` when not callable |
| `is_maybe<Type>` | reused from `maybe.hpp` | true for a `maybe<T>` specialization |
| `is_maybe_extractor<Fn, Source>` | `struct` → `bool` (+`_v`, concept `maybe_extractor_c`) | extractor whose result is a `maybe<T>` |

## `producer.hpp` *(factories flat in `djinterp`)*

| Entity | Signature | Notes |
|---|---|---|
| `producer_step<T>` | `struct { bool has_value; T value; }` | one pull's result (minimal maybe) |
| `make_step(v)` / `no_step<T>()` | → `producer_step<T>` | value / exhaustion |
| `iterate(seed, step)` | → `iterate_helper` | x, f(x), f(f(x)), … (infinite) |
| `unfold<Value>(state, step)` | → `unfold_helper` | step: `State -> producer_step<pair<Value,State>>`; Value explicit |
| `range(start,end[,step])` / `iota(start,end)` | → `range_helper<Int>` | step may be negative |
| `repeat(v)` / `repeat_n(v, n)` | → `repeat_helper` / `repeat_n_helper` | infinite / n-bounded |
| `cycle(container)` | → `cycle_helper` | repeats container forever |
| `generate(fn)` | → `generate_helper` | calls nullary fn forever |
| `empty<T>()` / `single(v)` | → `empty_producer_helper<T>` / `single_helper` | |
| `take_n(producer, n)` / `drop_n(producer, n)` | → `take_n_helper` (has `.collect()`) / `drop_n_helper` | bound an inner producer |
| `concat(a, b)` / `interleave(a, b)` | → `concat_helper` / `interleave_helper` | sequence / alternate |
| `transform(producer, fn)` / `filter(producer, pred)` | → `transform_helper` / `filter_helper` | |
| `from_container(c)` | → `from_container_producer<C>` | iterable → finite producer (held by value) |
| `collect(producer&)` | → `vector<value_type>` | drain (must be finite/bounded) |
| `for_each(producer&, consumer)` | → `void` | |
| `fold(producer&, init, step)` | → `Acc` | drain through binary step |
| `is_producer_step<T>` (+`_v`, concept `producer_step_type`) | `struct` → `bool` | |
| `is_producer<T>` (+`_v`, concept `producer`) | `struct` → `bool` | nested value_type + const-nullary → producer_step |
| `producer_value_type<T>` | emitted value type | use behind `is_producer` |

## `consumer.hpp` *(factories in namespace `consumers`)*

| Entity | Signature | Notes |
|---|---|---|
| `consumer_result<C, T>` / `_t` | result of `C(const T&)` | SFINAE-friendly |
| `is_consumer<C, T>` (+`_v`, concept `consumes`) | `struct` → `bool` | callable `void(const T&)` |
| `is_predicate<P, T>` (+`_v`, concept `predicate_for`) | `struct` → `bool` | `P(const T&)` → bool |
| `is_transformer<F, T>` (+`_v`, concept `transformer_for`) | `struct` → `bool` | `F(const T&)` → non-void |
| `is_boxable<C, T>` (+`_v`, concept `boxable_as`) | `struct` → `bool` | convertible to `std::function<void(const T&)>` |
| `contramap_input<F, In>` / `_t` | mapped value type B | for computing inner consumer element type |
| `print_to(stream[, sep='\n'])` | → `print_to_helper` | stream sink |
| `write_to(container)` | → `write_to_helper` | push_back sink (container must outlive) |
| `discard()` | → `discard_helper` | null sink |
| `count_into(counter&)` | → `count_into_helper` | increments counter |
| `filtered(consumer, pred)` | → `filtered_consumer_helper` | predicate gate |
| `mapped(consumer, fn)` | → `mapped_consumer_helper` | contramap: `consumer<B>` + `A->B` ⇒ `consumer<A>` |
| `tee(consumers...)` | → `tee_consumer_helper` | broadcast (tee() ill-formed; use discard) |
| `batched(consumer, stride)` | → `batched_consumer_helper` | fire every N |
| `take(consumer, n)` / `drop(consumer, n)` | → `take_consumer_helper` / `drop_consumer_helper` | first-N / skip-first-N |
| `conditional(pred, if_true, if_false)` | → `conditional_consumer_helper` | route by predicate |
| `fallback(primary, secondary)` | → `fallback_consumer_helper` | try primary, on throw run secondary |
| `boxed_consumer<T>` | `using = std::function<void(const T&)>` | type erasure |
| `box<T>(consumer)` | → `boxed_consumer<T>` | T explicit |

## `accumulator.hpp`

| Entity | Signature | Notes |
|---|---|---|
| `accumulator<State, Input, Output, Step, Final>` | `class` | Step: `void(State&, const Input&)`; Final: `Output(const State&)` |
| `::step(v)` / `finalize()` / `run(container)` / `run(first,last)` / `run(ptr,n)` | members | step→`accumulator&` (chainable); finalize/run→`Output` |
| `::state()` / `step_fn()` / `finalize_fn()` | const accessors | for combinators/tests |
| `make_accumulator<Input,Output>(state, step, final)` | → `accumulator<...>` | Input/Output explicit |
| `sum<T>()` / `product<T>()` | → `accumulator<...>` | |
| `count<T>()` / `count_if<T>(pred)` | → `accumulator<size_t,...>` | |
| `min<T>()` / `max<T>()` / `min_by<T>(key)` / `max_by<T>(key)` | → `accumulator<pair<T,bool>,...>` | |
| `mean<T>()` / `variance<T>()` / `stddev<T>()` | → `accumulator<...,double>` | Welford (population) |
| `first<T>()` / `last<T>()` / `nth<T>(n)` | → `accumulator<...>` | |
| `joining<T>(sep)` | → `accumulator<...,string>` | runtime (ostringstream) |
| `to_vector<T>()` / `top_k<T>(k)` | → `accumulator<vector<T>,...>` | to_vector constexpr from C++20 |
| `histogram<T>()` | → `accumulator<map<T,size_t>,...>` | runtime |
| `to_map_by<T>(key)` / `group_by<T>(key)` | → `accumulator<map<K,T or vector<T>>,...>` | runtime |
| `all_match<T>(pred)` / `any_match<T>(pred)` / `none_match<T>(pred)` | → `accumulator<bool,...>` | |
| `contramap<NewInput>(acc, fn)` | → `accumulator<...>` | pre-map each input |
| `map_output(acc, fn)` | → `accumulator<...>` | post-map output |
| `filtered(acc, pred)` / `take(acc, n)` | → `accumulator<...>` | gate / cap inputs |
| `combine(accs...)` | → `internal::combine_helper<...>` | run in lock-step; `.run(c)` → tuple of outputs |
| `boxed_accumulator<In, Out>` / `box_accumulator(acc)` | type-erased + factory | std::function overhead |
| `has_state/input/output/step/final_type<T>` | member-typedef detectors | |
| `has_step/finalize/run/state/step_fn/finalize_fn_method<T>` | member-fn detectors | |
| `has_accumulator_typedefs<T>` / `has_accumulator_interface<T>` | composite detectors | |
| `is_accumulator<T>` / `is_boxed_accumulator<T>` (+`_v`) | `struct` → `bool` | full unboxed contract / erased form |
| `accumulator_state_t/input_t/output_t<T>` | extractors (→ `nonesuch` if absent) | |
| concepts | `accumulator_typedefs`, `accumulator_steppable`, `accumulator_finalizable`, `accumulator_like` | `[C++20]` |

## `view.hpp` *(adapters/sources/terminals factories in namespace `views`)*

| Entity | Signature | Notes |
|---|---|---|
| `view_base<Derived>` | CRTP marker | all views inherit |
| `is_view<T>` (+`_v`, concept `view_type`) | `struct` → `bool` | derives from `view_base` |
| `has_begin_end<T>` (+`_v`) | `struct` → `bool` | container-like |
| `ref_view<C>` / `owning_view<C>` / `iterator_pair_view<It>` | view classes | non-owning / by-value / iterator pair |
| `iota_view<Int>` / `repeat_view<T>` / `generate_view<F>` / `empty_view<T>` / `single_view<T>` | source view classes | iota/repeat infinite-capable |
| `transform_view` / `filter_view` / `take_view` / `drop_view` / `take_while_view` / `drop_while_view` | adapter view classes | lazy |
| `enumerate_view` (yields `pair<size_t,T>`) / `zip_view<V1,V2>` / `concat_view<V1,V2>` / `reverse_view` (bidir req'd) / `chunk_view` (yields `vector<T>`) / `stride_view` | adapter view classes | |
| `views::transform(fn)` / `filter(pred)` / `take(n)` / `drop(n)` / `take_while(pred)` / `drop_while(pred)` | → `internal::*_adapter` | adapter factories |
| `views::enumerate()` / `zip(other)` / `concat(other)` / `reverse()` / `chunk(n)` / `stride(n)` | → `internal::*_adapter` | |
| `views::iota(start[,end])` / `repeat(v)` / `repeat_n(v,n)` / `generate(fn)` / `empty<T>()` / `single(v)` | → source view | source factories |
| `operator\|(view, adapter)` | → new view | SFINAE: view LHS + adapter RHS |
| `operator\|(container, adapter)` | → new view | lifts container to `ref_view` |
| `operator\|(view/container, terminal)` | → terminal result | `is_terminal` = complement of `is_adapter` |
| `to_vector()` | → `to_vector_terminal` | drain to `vector<value_type>` |
| `to<C>()` | → `to_container_terminal<C>` | drain to explicit container (push_back) |
| `count()` | → `count_terminal` | element count (no short-circuit) |
| `fold(init, step)` | → `fold_terminal` | ≈ accumulate |
| `for_each(consumer)` | → `for_each_terminal` | |
| `any_of(pred)` / `all_of(pred)` / `none_of(pred)` | → `*_terminal` | short-circuit (all_of vacuously true) |
| `view_value_type<V>` / `_t` | element type | SFINAE-friendly |
| `is_adapter<T>` / `is_terminal<T>` (concepts `view_adapter` / `view_terminal`) | `struct` → `bool` | mutually exclusive |
| `is_pipeable_to_view<T>` (+`_v`, concept `pipeable_to_view`) | `struct` → `bool` | view OR container |

## `reduce.hpp` *(C++17+; empty under C++11)*

The two drivers of the step/driver split; one reducer body `(acc, x) -> acc`
serves all three domains.

| Entity | Signature | Notes |
|---|---|---|
| `reduce_rt(rf, acc, first, last)` | → `Acc` | runtime fold over an iterator pair; `constexpr` (folds constexpr ranges at compile time) |
| `reduce_rt(rf, acc, iterable)` | → `Acc` | same over an iterable; pulls to exhaustion (lazy / infinite-safe) |
| `reduce_ct(rf, acc, value_list<Values...>)` | → `Acc` | compile-time NTTP fold; delegates to `value_list`'s own fold |
| `reduce_ct(rf, acc, type_c<std::tuple<Ts...>>)` | → `Acc` | compile-time type fold; **preferred** — elements need not be default-constructible; reducer sees `type_c<T>` |
| `reduce_ct(rf, acc, std::tuple<Ts...>{})` | → `Acc` | convenience value-passing form; needs default-constructible elements |
| `internal::reduce_ct_tuple_impl<Rf, Acc, Tuple>` | recursion helper | folds tuple element types head-first |

Reducers are unconstrained; a C++20 caller may layer `structural_traits.hpp`'s
`Reducer` / `Transducer` concepts at the call site.

## `transducer.hpp` *(C++14+; factories in namespace `transducers`)*

| Entity | Signature | Notes |
|---|---|---|
| `transducer_base<Derived>` | CRTP marker | user transducers should inherit |
| `is_transducer<T>` (+`_v`) | `struct` → `bool` | derives from `transducer_base` |
| `reduced<Acc>` | `class` | explicit early-finish wrapper; `is_reduced`, `unwrap_reduced` |
| `reducing_state<Acc>` | `class`: `accumulator()`, `is_done()`, `mark_done()` | accumulator + stop flag passed to step fns |
| `transducers::map(fn)` | → `map_transducer_helper` | transform each value |
| `transducers::filter(pred)` / `filter_not(pred)` | → `filter_transducer_helper` | drop non-/matching |
| `transducers::take(n)` / `drop(n)` | → `take_transducer_helper` / `drop_transducer_helper` | take short-circuits |
| `transducers::take_while(pred)` / `drop_while(pred)` | → `*_transducer_helper` | |
| `transducers::distinct<Value>()` | → `distinct_transducer_helper<Value>` | Value explicit; own seen-set |
| `transducers::tap(side_effect)` | → `tap_transducer_helper` | peek, forward unchanged |
| `transducers::flat_map(fn)` | → `flat_map_transducer_helper` | fn returns iterable; one-to-many |
| `transducers::partition_by(key_fn)` | (TOC) chunk while key unchanged | |
| `compose(t1, t2)` / `compose(t1,t2,t3,rest...)` | → `composed_transducer` | reading order: t1 sees values first |
| `operator\|(transducer, transducer)` | → composed | SFINAE: both derive `transducer_base` |
| `into_reducer(xform, downstream)` | → wrapped reducer | primitive used by drivers |
| `transduce(xform, reducer, init, container)` | → `Acc` | general driver |
| `transduce_into_vector<Out>(xform, container)` | → `vector<Out>` | |
| `transduce_into_accumulator(xform, acc, container)` | → output | needs accumulator.hpp |
| `transduce_producer_to_consumer(xform, producer&, consumer&)` | → `void` | needs producer.hpp + consumer.hpp |
| `is_reducing_state<T>` (+`_v`, concept `reducing_state_c`) | `struct` → `bool` | |
| `reducing_state_acc_t<T>` | carried accumulator type | |
| `is_reducer<Fn, Acc, Value>` (+`_v`, concept `reducer_c`) | `struct` → `bool` | `(reducing_state<Acc>&, const Value&)` |
| `transduces_reducer<X, Reducer>` (+`_v`) | `struct` → `bool` | X applied to a reducer yields a reducer |
| `transducer_result_t<X, Reducer>` | produced reducer type | or `call_nonesuch` |
| concept `transducer_c<X, Reducer>` | marker + behavioral application | `[C++20]` |

## `interpolate.hpp` *(C++17+; self-suppresses below it)*

Scan-event vocabulary, scanners, resolvers, sinks (a `piece` is a literal run or
a key; the fold writes each segment straight to the sink):

| Entity | Signature | Notes |
|---|---|---|
| `piece_kind` / `piece<Type>` | enum / scan token | token = `kind` (literal vs key) + value view |
| `brace_scanner<Type>` | scanner (pull cursor) | `{key}`, trimmed `{ key }`, `{{ }}` escaped |
| `sigil_scanner<Type>` | scanner | `$name` style; configurable sigil |
| `replay_scanner<Type, Cache>` | scanner | replays a pre-scanned `piece` cache |
| `resolution<Value>` | `found()` + `value()` + lazy `or_else` | a minimal `maybe` for one key lookup |
| `empty_resolver` | identity resolver | every key misses → passes through (partial interpolation) |
| `map_resolver<Type>` / `bindings({...})` | inline `{key,value}` bindings | `bindings` takes an initializer-list |
| `lookup_resolver<Type,Fn>` / `lookup(fn)` | adapt a callable | always-hit |
| `chain_resolver<A,B>` / `chain(a, b)` | try `a`, else `b` via `or_else` | associative; N frames → one pass |
| `when_resolver<Pred,R>` / `when(pred, r)` | gate resolver `r` on a key predicate | compose multi-condition gates via `predicate.hpp` first |
| `interp_string_sink` | append into a `basic_string` | a byte sink / consumer / accumulator may stand in |

Engine, recursion, lazy functor, prepared templates:

| Entity | Signature | Notes |
|---|---|---|
| `interpolate_into(sink, scanner, resolver)` | → `void` | the fold; writes segment-by-segment, no intermediate structure |
| `recursive_resolver` / `recursive(inner, max_depth=16)` | re-scan a hit's value | value-level nested templates (depth-capped) |
| `interpolation<...>` | template + resolver chain in the type | `.interpolate(...)` extends the chain; lazy until a terminal |
| `interpolate(template)` / `make_interpolation(...)` | → `interpolation` | seed a lazy interpolation |
| `.recursive()` / `.prepare()` | terminal options | nested-template render / pre-parse |
| `prepared_interpolation<...>` | template + resolver over a piece cache | pre-parse once, render many |
| `prepare(...)` / `make_prepared(...)` / `prepare_into(cache)` | → prepared / fill cache | shared cache vs caller-owned |
| concepts `scanner_for` / `resolver_for` / `sink_for` | `concept` | `[C++20]` faces of the three policies |

## `functional.hpp` *(root; eager free algorithms in `djinterp::*`)*

All accept STL containers, raw arrays (ptr+count), iterator pairs, and
initializer lists; SFINAE-constrained on callable shape via `is_callable` /
`is_predicate` / `callable_result_t`. Internal helpers: `void_t`,
`has_begin_end`, `has_push_back`, `has_reserve`, `container_value_type`,
`maybe_reserve`.

| Algorithm | Forms | Result |
|---|---|---|
| `map(input, fn)` | container / iterator-range / raw-array | `vector<R>` |
| `map_in_place(container, fn)` | container | in-place mutate |
| `filter(input, pred)` / `filter_not(input, pred)` | container / range / array | `vector<T>` |
| `fold_left(input, init, fn)` | container / range | `Acc` |
| `fold_right(input, init, fn)` | container | `Acc` |
| `reduce(input[, init], fn)` | container | `T` / `Acc` |
| `scan(input, init, fn)` | container | `vector<Acc>` (prefix fold) |
| `for_each(container, fn)` / `for_each_const(...)` / `for_each_indexed(...)` | container | side effects |
| `any(c, pred)` / `all(c, pred)` / `none(c, pred)` | container | `bool` |
| `count_if(c, pred)` | container | `size_t` |
| `find_if(c, pred)` / `find_if(mutable c, pred)` / `find_last(c, pred)` | container | iterator/element |
| `index_of(c, value)` / `last_index_of(c, value)` | container | index (or sentinel) |
| `is_sorted(c[, cmp])` | container | `bool` |
| `take(c, n)` / `take_while(c, pred)` / `skip(c, n)` / `skip_while(c, pred)` | container | `vector<T>` |
| `flat_map(c, fn)` | container | flattened `vector<R>` |
| `zip(c1, c2)` / `zip_with(c1, c2, fn)` | two containers | `vector<pair>` / `vector<R>` |
| `group_by(c, key_fn)` | container | `map<Key, vector<T>>` |
| `partition(c, pred)` | container | `pair<vector<T>, vector<T>>` (pass, fail) |
| `distinct(c[, eq])` | container | `vector<T>` |
| `reverse(c)` | container | `vector<T>` |
| `slice(c, start, end, step=1)` / `range(c, start, end)` | container | `vector<T>` |
