# Expression core v2: the design

Lane F, for agreement with lanes E, G and H and the math session by Friday
2026-11-06. The design is the contract: anything that changes it after Friday
goes back to the lanes it touches.

**Status, Thursday 11-05.** The C core is built and tested in this delivery:
term, signature, parser, diagnostics, folds, walks and the renderer, with
every case of sections 6.3 and 6.4 in its suite (section 21 lists what is
there, what is measured, and what building it changed in this text). Lanes
E, G and H answered section 20 on 11-05; their answers are folded in, and
decisions F15 to F19 came of them. Week 2 (Monday 11-09 to Friday 11-13)
adds the scanner, the C++98 face, the bridge, the parity suite and the
sub-parse, and moves the suites onto DTest 2.0.

## 0. What this settles

**The parser holds postfix operators with operands, and calls.** A call
`f(a, b)` and an index `A[i, j]` are roles of their opening bracket after an
operand, at a precedence the language's table gives (math: 100), and their
arguments are parsed in the same pass, with diagnostics at the character. The
transpose `A'` and the factorial `n!` are postfix roles at their own
precedences. `(a + b)[1]`, `A'[1]`, `f(x)[1]` and `A[1][2]` index any operand,
as S252 asks, with no table beside the standard one and nothing put back after
the parse. So for S190 the math session can choose core v2, and S209 and S252
then follow it: the parser layer's pass that reads calls and bars as atoms
before the climb (S209) and the postfix table whose indices are put back
after it (S252) are no longer needed once math moves onto core v2.

What else core v2 holds, each of which math's parser layer does by hand today:

| Today, in math's parser layer | In core v2 |
|---|---|
| a pass gives each token its role before the climb, so `-` is unary or binary by where it stands (S190, chunk 12) | every token class has a role in operand position and one in operator position; the parser asks the one that fits where it stands |
| errors mapped from token indices back to characters (chunk 27) | every token, node and diagnostic carries a byte span; line and column on request |
| calls and bars read as atoms before the climb, put back after (S209) | calls and bars are mixfix operators of the table |
| indexing as a postfix operator of a second table, indices put back (S252) | indexing is a mixfix operator after an operand |
| brackets read as one operand before the climb, so a bracketed relation is no link (S215) | a group is one node with a `grouped` flag, so a chain never takes it as a link |
| chains grouped to the right, then joined by `and`, the shared operand copied (S214) | chain associativity: the links are joined by the operator the table names, the shared operand one node in both links |
| implied multiplication inserted as a `*` token (S196) | a juxtaposition operator of the table, with the language's rule for where it applies |
| nested groups cost a stack frame each, depth bound by the stack (S250) | no recursion anywhere: the parser, the folds and the renderer use explicit stacks, and a depth guard is a knob and an option |
| a non-associative operator repeated groups to the right silently (v1) | a non-associative operator repeated at one precedence is an error at the second, with a note at the first |
| folds meet terms made by default (S190's review) | no node is made by default: a missing operand is a `hole` node with its span, and a term with holes is refused by the folds unless asked |

The rest of this document is how.

## 1. Who it is for

| Client | Lane | What it needs from the core | Where it comes from |
|---|---|---|---|
| math's expression language | math session | the table above: roles by position, calls, indexing, transpose and factorial, bars, matrix literals with rows, chains, implied multiplication, errors at characters, a depth guard | `cpp/modules/math/expr/parse.hpp`; S190, S196, S201, S207, S209, S213 to S215, S243, S250, S252 |
| the command language and its DSL | G | non-associative comparison (`a == b == c` refused), a command as a primary that takes the arguments after its verb, `|` and `;` as n-ary pipelines and sequences, spans for `d_cli_report_at`; in December, the grammar's own predicates (explicit `and`, `or`, `not` and comparisons over command primaries) and blocks `{ ... }` by the sub-parse, with no juxtaposition | `cpp/modules/parse/grammar/djinterp.peg`; `functional_cli.hpp`; cli-taken 80; lane G's `docs/djinterp/interp/DESIGN.md` |
| bitwisedom's integer expressions | G | C's operators at C's precedence, calls `BIT(n)`, `GENMASK(h, l)`, positions for errors | `tools/bitwisedom/expr.hpp` |
| dawk's awk expressions | H | concatenation by juxtaposition, which must ask whether the next token can begin an operand; non-associative relations; `>` not a comparison inside `print`'s list; `/re/` or division by where it stands; `cmd | getline`; `?:`; `++` both prefix and postfix | `tools/dawk/dparse.c` |
| options, the registry, desc and logging | E | diagnostics in the message-code registry; knobs and options in E's pattern; nothing printed by the core | E's brief; the owner's standing rule on knobs and options |
| vparse's notation readers | F | PEG's `/`, sequence by juxtaposition, prefix `&` `!`, postfix `*` `+` `?`; EBNF's `|`, `,`, `[ ]`, `{ }` | `cpp/modules/parsegen/vparse/notation.hpp`, `ebnf.hpp` |
| text's template language | D | member access, filters, calls, subscripts | lane D's templates, when they want it |

The core is generic: it knows no language. Each client supplies a table of
operators and roles, a token source, and, where it needs one, a hook; the
core parses, reports, folds and renders.

## 2. What v1 cannot do

v1 is `cpp/modules/parse/expression/`: a term (`expression<OpId, Atom>`, the
fixed point of one functor, over `shared_ptr`), its folds (`cata`), a
renderer, a compile-time mirror, and a precedence climber
(`expression_parse.hpp`). The climber's limits are the ones S190's review
lists:

- **One role per token.** `op_of(token)` returns one operator, and the
  climber decides prefix or infix from the descriptor, not from where the
  token stands; so a language with `-` both unary and binary must assign
  roles before the climb (chunk 12's probe).
- **Token offsets.** `parse_error` carries the index of the token, not a
  character (chunk 27).
- **One kind of bracket, and no calls.** `is_open` and `is_close` match any
  open with any close; a call, an index, a pair of bars or a literal has no
  form, so each is read before the climb and put back after (S209, S252).
- **No non-associative operators.** `DAssocNone` climbs as right association
  (`_right_min` adds 1 only for `DAssocLeft`), so `a == b == c` parses
  silently as `a == (b == c)`, and the CLI grammar's rule that it is an error
  is not kept.
- **Recursion.** `parse_nud` and `parse_climb` recurse for every bracket and
  every prefix operator; S250 bought depth by moving work out of the
  recursing frame, and the depth is still the stack's.
- **One error.** The first failure returns; nothing after it is seen.
- **C++11.** Below it the header is empty; there is no C core and no C++98
  face, so the parity law has nothing to test.

v1's term and folds are good and stay (section 15). What v2 replaces is the
climber, and it adds the C core.

## 3. The shape

```
  C99 core       c/modules/parse/expr/          terms, signatures, parse,
                                                diagnostics, folds, walks,
                                                rendering, a default scanner
     |
  C++98 face     cpp/modules/parse/expr/        classes over the C core,
                                                nothing computed twice;
                                                the levels' extras above 98
     |
  C++11 bridge   cpp/modules/parse/expr/        a v2 term to v1's
                 bridge.hpp                     expression<OpId, Atom> and
                                                back, so v1's folds keep
                                                working
```

The C core reuses the foundation (`FOUNDATION.md`): its diagnostic sink and
span (`diagnostic.h`), its growth policy (`storage.h`), and lane A's checked
arithmetic (`c/dckdint.h`). It holds no global state.

## 4. The term

### 4.1 Nodes

A term is a flat arena of nodes in an array, linked by index, with no
pointers, so it copies, hashes and serialises whole, as the foundation's
programs and grammars do.

```c
struct dj_parse_expr_node
{
    uint8_t  kind;     // D_PARSE_EXPR_NODE_LEAF, _APPLY or _HOLE
    uint8_t  flags;    // D_PARSE_EXPR_NODE_GROUPED, _CHAIN, _JUXTAPOSED, ...
    uint16_t depth;    // its longest path to a leaf, 1 for a leaf,
                       // saturating at 65535 (section 6.6)
    uint32_t op;       // apply: its operator's id; leaf: its token class
    uint32_t first;    // apply: index of its first child in the args array;
                       // leaf: its token's value
    uint32_t count;    // apply: its number of children; leaf: 0
    uint32_t offset;   // its span's first byte: a group's open bracket
    uint32_t length;   // its span's length in bytes, its brackets included
};  // 24 bytes, asserted in both languages
```

```c
struct dj_parse_expr_term
{
    struct dj_parse_expr_node* nodes;
    uint32_t                   node_count;
    uint32_t                   node_capacity;
    uint32_t*                  args;       // child indices, by node
    uint32_t                   arg_count;
    uint32_t                   arg_capacity;
    uint32_t                   root;
    uint32_t                   depth;      // the longest root-to-leaf path
    uint32_t                   flags;      // _OWNS_NODES, _OWNS_ARGS, _HAS_HOLES
};
```

Every value is 32 bits wide: a 64-bit integer does not exist at ISO strict
C++98 on 32-bit targets, 64-bit Windows or macOS (re_std's rule), and the C++
face must compile there. A leaf's value is the language's: an index into its
own table of atoms, an interned lexeme's offset, or a small integer.

### 4.2 Invariants

1. **Post-order.** A node's children have lower indices than the node. The
   parser builds bottom-up, so this holds by construction, and
   `d_parse_expr_term_verify` checks it with the rest. A fold is therefore one
   pass over the array, in order, with no recursion and no stack.
2. **A DAG where a chain shares.** `a < b <= c` is
   `and(lt(a, b), le(b, c))` with `b` one node, the child of both links:
   S214's review's "the operand shared in the representation later". A fold
   in arena order computes `b` once. Nothing else shares.
3. **No node made by default.** A missing operand is a `hole` node at the
   place it should have stood; the term carries `_HAS_HOLES`, and a fold
   refuses such a term unless its caller passes
   `D_PARSE_EXPR_FOLD_HOLES`.
4. **Spans cover their text.** A node's span covers every token it was made
   from: a group's includes its brackets, a call's its callee and its closing
   bracket, a chain's join its first operand to its last.
5. **Groups leave no node.** `(a + b)` is the node `a + b` with
   `_GROUPED` set and its span widened over the brackets, as S215 keeps it;
   folds and rewrites never step through a bracket node. A grouped leaf,
   `(x)`, keeps its token's span, since a leaf's span is its text (the
   renderer's default atom writes it); the brackets are inside the span of
   whatever holds it, and a diagnostic about the group itself points at them.

### 4.3 Building and rewriting

The builder is public: `d_parse_expr_term_leaf`, `d_parse_expr_term_apply`
(children by index), `d_parse_expr_term_hole`, and `d_parse_expr_term_copy`
(a subtree of one term into another, its nodes renumbered). A language's own
pass after the parse (math's `piecewise`, a simplifier) writes a new term
with these, as v1's rewrites write a new `expression`. Equality
(`d_parse_expr_term_equal`, spans and `_GROUPED` ignored unless asked) and a
digest (`d_parse_expr_term_digest`, 32-bit FNV-1a over the reachable nodes in
order, week 2) serve the parity tests and caches.

## 5. The signature

A signature is the language: its operators and, for each token class, its
roles. It is built once, sealed (verified and indexed), and then read-only,
so threads may share it.

### 5.1 Operators

```c
struct dj_parse_expr_op
{
    uint32_t    id;          // the language's, not 0
    uint8_t     form;        // D_PARSE_EXPR_FORM_PREFIX, _INFIX, _POSTFIX,
                             // _MIXFIX
    uint8_t     assoc;       // _LEFT, _RIGHT, _NONE, _CHAIN
    uint16_t    flags;       // _FLATTEN, _WORD, _TRANSPARENT, _COMMUTATIVE,
                             // _ASSOCIATIVE, _IDEMPOTENT
    int32_t     precedence;  // larger binds tighter
    uint32_t    join;        // a chain's: the operator that joins its links
    const char* spelling;    // what the renderer writes
    struct dj_parse_expr_mixfix mixfix;  // a mixfix operator's parts
};
```

- **Prefix**, **infix** and **postfix** are the classic forms.
- **Associativity** is `left`, `right`, `none` or `chain`. `none` refuses a
  second operator of the same precedence beside the first (`a == b == c` and
  `a < b == c` in the CLI grammar). `chain` collects `a < b <= c` and joins
  its links with `join` (math: `and`).
- **`_FLATTEN`** makes a repeated left-associative operator one n-ary node:
  `a | b | c` is `pipe(a, b, c)`, as `functional_cli`'s pipeline is a
  sequence.
- **`_WORD`** marks an operator spelled as a word (`and`, `mod`): the
  renderer spaces it, and it never starts a juxtaposed operand.
- The algebraic flags (`_COMMUTATIVE`, `_ASSOCIATIVE`, `_IDEMPOTENT`) are
  v1's, carried for simplifiers.

### 5.2 Mixfix operators

A mixfix operator has more than one token: a call, an index, a group, a
list, a pair of bars, `?:`, `if ... then ... else`. One descriptor covers
them all:

```c
struct dj_parse_expr_mixfix
{
    uint8_t     lead;           // 1: it follows an operand (a call, an index,
                                // ?:); 0: it begins one (a group, a list)
    uint8_t     parts;          // its keyword tokens: 2 to 4
    uint8_t     list_hole;      // the hole between parts 0 and 1 is a list
    uint8_t     tail;           // an operand follows the last part (?:'s
                                // third, else's branch), at the precedence
    uint32_t    part[4];        // each keyword's token class
    const char* part_spelling[4];
    uint32_t    separator;      // the list's item separator class (`,`)
    uint32_t    row_separator;  // its row separator class (`;`), or 0
    const char* separator_spelling;      // what the renderer writes
    const char* row_separator_spelling;
    uint32_t    row_op;         // the operator of a row, where rows are seen
    uint32_t    rows_op;        // the operator of the whole, where rows are
    uint16_t    min_items;      // a list's bounds; max 0xFFFF for none
    uint16_t    max_items;
};
```

The holes between keywords are closed: each is parsed from precedence 0 up
to the next keyword, so `f(a + b, c)` needs no brackets inside. A tail is
open: it is parsed at the operator's precedence, so `a ? b : c ? d : e` is
right-associative as C has it. The term of a mixfix operator is one apply
node: its lead operand first, then its holes' items, then its tail. Math's
call is `call(f, a, b)`; its index `index(A, i, j)`; its literal `[1, 2; 3,
4]` is `matrix(row(1, 2), row(3, 4))` and `[1, 2, 3]` is `vector(1, 2, 3)`
(`rows_op` and `row_op`, where a row separator is seen; the operator's own id
where none is); `?:` is `cond(a, b, c)`. A `_TRANSPARENT` mixfix of one item
is a group: `(a)` is `a`, flagged `_GROUPED`. Given a list hole (and no rows),
it is a group for one item and its own node for any other count: awk's
`(i, j) in arr` is `in(group(i, j), arr)`, and a tuple is the same shape. The
language's check refuses the node where it does not take one (F16).

Every form math's parser layer reads by hand is one row:

| text | operator | lead | parts | hole | tail |
|---|---|---|---|---|---|
| `f(a, b)` | call | yes, a callee | `(` `)` | list, `,`, 0 or more | no |
| `A[i, j]` | index | yes | `[` `]` | list, `,`, 1 or more | no |
| `(a)` | group, transparent | no | `(` `)` | one | no |
| `[1, 2; 3, 4]` | vector, rows: matrix of rows | no | `[` `]` | list, `,` and `;`, 0 or more | no |
| `|x|` | abs | no | `|` `|` | one | no |
| `c ? a : b` | cond | yes | `?` `:` | one | yes |
| `if c then a else b` | if | no | `if` `then` `else` | one, one | yes |

### 5.3 Roles

A role says what a token class does in one position: in **operand position**
(nothing before it ends an operand: the start, after an operator, after an
open bracket or a separator) or in **operator position** (an operand ends just
before it).

```c
struct dj_parse_expr_role
{
    uint32_t cls;       // the token class
    uint8_t  position;  // D_PARSE_EXPR_OPERAND or D_PARSE_EXPR_OPERATOR
    uint8_t  kind;      // _ATOM, _OP (the op's form says which), _CUSTOM
    uint16_t flags;     // _CALLABLE, _CALLEE, _ADJACENT, _NO_JUXTAPOSE
    uint32_t op;        // the operator this token begins; a custom role's,
                        // the operator it reads as (below)
    uint32_t modes;     // the modes it is a role in; 0 for all
    uint32_t order;     // set when defined: roles are tried in this order
    dj_parse_expr_custom_fn custom;  // a _CUSTOM role's hook
    void*    custom_ctx;
};
```

- An **atom** role makes a leaf; `_CALLABLE` marks a class that may stand
  before a call's bracket (math's names, not its numbers).
- An **op** role begins its operator: a prefix operator or a mixfix one
  without a lead in operand position; an infix or postfix operator, or a
  mixfix one with a lead, in operator position.
- **`_CALLEE`** on a lead mixfix role: only where the operand before it is an
  ungrouped leaf of a `_CALLABLE` class. Math: `f(x)` is a call, `2(y)` and
  `(x)(y)` are not (S201), and with implied multiplication on they are
  products.
- **`_ADJACENT`**: only where no space precedes the token (awk's call of a
  function of its own, `f(x)` but not `f (x)`).
- **Modes**: a bit set. A parse runs in a mode, and a custom hook may run a
  sub-parse in another; a role whose `modes` miss it is not there. awk's
  `print a > "f"` reads its list in a mode without the relational `>`.
- A class may have several roles in one position, tried in order; the first
  whose conditions hold is the one. A custom role that declines is passed
  over, and where no role takes a token in operator position, juxtaposition
  is tried next: so a `++` that declines after something that cannot be
  assigned begins the next operand, and awk's `1 ++i` is `concat(1, ++i)`.
- A **custom role in operator position may name an operator** (`op`). It is
  then called after the frames that bind tighter than that operator are
  reduced, so its `_left` is the operand that operator would take: `$i++`
  hands `++` the `$i`, and `"a" "b" | getline` hands `| getline` the whole
  concatenation (F15). With no operator named, it is given what is on top.
- The **closing tokens** and **separators** need no role: a mixfix operator's
  parts declare them, and the parser knows which open operator expects which.

### 5.4 Juxtaposition

A signature may name one juxtaposition operator, an infix operator with no
token. Where a token in operator position has no role there whose conditions
hold, but has one in operand position, the parser applies the juxtaposition
operator and reads the token as an operand. The language's hook,
`may_juxtapose(ctx, term, left, token)`, refuses where it must: math refuses
between two numbers (S196); awk's concatenation takes it as it comes. A
`_NO_JUXTAPOSE` role never begins a juxtaposed operand. Math's implied
multiplication is the operator `mul` at 60, present only when G5's option is
on (math builds two signatures, or one with the operator in a mode). The node
is flagged `_JUXTAPOSED`, and its span covers both operands as any infix
node's does (invariant 4); where a diagnostic needs the implied operator's
place, it is the first byte of the operand it stands before, as math's
implied `*` has the offset of the operand after it today.

### 5.5 Sealing

`d_parse_expr_signature_seal` checks the signature and indexes it. It
refuses, with a diagnostic each: an operator id 0 or used twice; a form or an
association that is not one of the four; a role whose operator is not
defined, or whose form does not fit its position, an atom role in operator
position, a custom role without a hook, a role for the end of the input; a
chain whose `join` is not an infix operator; a mixfix with fewer than two parts
or more than four, a part that is the end of the input, a list hole with
`min_items` above `max_items`, rows whose operators are not defined, a
`_TRANSPARENT` operator that is not a group, or has rows; a custom role that
names an operator not defined; a juxtaposition operator that is not infix.
An operator that no role begins is not refused: it is one only a hook builds
(the command language's `cmd`), sealed and rendered by its form like any
other. A closing
part that is also an infix role is not refused: the order of section 6.2
reads it as the close wherever its operator is the innermost open, so an
infix `|` inside a pair of bars needs brackets, as it does in every notation
with both. Roles are sorted by class and position, and found by a binary
search.

## 6. The parser

### 6.1 Tokens

```c
struct dj_parse_expr_token
{
    uint32_t cls;     // the language's class; 0 is the end of the input
    uint32_t flags;   // D_PARSE_EXPR_TOKEN_SPACE_BEFORE, _LINE_BEFORE
    uint32_t offset;  // its first byte in the text
    uint32_t length;  // its bytes
    uint32_t value;   // the language's: what a leaf of it holds
};
```

Tokens come from a **source**, pulled one at a time:

```c
struct dj_parse_expr_source
{
    int   (*next)(void*                       _ctx,
                  uint32_t                    _position,  // operand or operator
                  uint32_t                    _mode,
                  struct dj_parse_expr_token* _out);
    void* ctx;
};
```

The parser tells the source where it stands. That is what lets awk read `/`
as a regular expression in operand position and as division in operator
position, and math tell an opening bar from a closing one without a pass of
its own. A language with a context-free lexer ignores the position; one with
its tokens already in an array uses `d_parse_expr_source_array`. A source
that fails (a character no token begins with) emits its own diagnostic, in its
own domain, and returns the failure; the parse stops there.

### 6.2 The algorithm

Operator precedence over two explicit stacks, an operand stack of node
indices and an operator stack of frames, in one pass over the tokens: the
shunting-yard machine with Pratt's two positions. Each token is pushed and
popped at most once, so the parse is linear in the tokens. Nothing recurses,
so the depth is bounded by memory and by the guard, never by the C stack.

In **operand position** a token is:
- an atom: a leaf is pushed, and the parser moves to operator position;
- a prefix operator, or a mixfix one with no lead: a frame is pushed, and the
  parser stays in operand position;
- a custom role: its hook is called (section 6.5);
- the next part of the innermost open mixfix, a separator of its list, or the
  end: a hole is pushed and `missing operand` reported (`(1 + )`, `[1, ]`,
  `x[]`), unless the open list is empty and takes zero items (`f()`, `[]`);
  with `D_PARSE_EXPR_OPTION_STOP`, so is a token with no role in either
  position, which then ends the expression (below);
- anything else: `unexpected token`.

In **operator position** a token is, in this order:
1. the next part of the innermost open mixfix, or a separator of its list:
   the frames above it are reduced, and its hole or item is done;
2. an infix or postfix operator, or a lead mixfix whose conditions hold:
   frames that bind tighter are reduced first (below), then an infix or lead
   mixfix frame is pushed and the parser moves to operand position, or a
   postfix node is built at once and the parser stays; a custom role is
   called, after the same reductions where it names its operator, and one
   that declines is passed over;
3. a token with a role in operand position, where the signature has a
   juxtaposition operator and the hook allows: the juxtaposition operator is
   taken as an infix, and the token read in operand position;
4. a closing part of an outer open operator: `mismatched` at the token, with
   a note at the innermost open part; the frames inside the outer one are
   closed, and the token read again as its part (a separator of an outer
   operator is step 8's: `f((a, b))`'s comma stands in a group, not in the
   call);
5. the end: every frame is reduced; an open mixfix is `unclosed`, at its first
   part, the innermost one reported, as math has it; and, with
   `D_PARSE_EXPR_OPTION_STOP`, any token the expression cannot go on with,
   which is left unread in the result's `next` (F17): the `;`, the newline,
   the `)` of a statement's own `if (`, the next statement's word. Inside an
   open bracket it ends the parse the same way, and the bracket is
   `unclosed`; a closing part of another kind there is step 6's;
6. a closing part that no open operator awaits, where one is open:
   `mismatched`, with the same note, and the token taken as the innermost
   one's close: `(1 + 2]` is one slip and one error, not an `unopened` and an
   `unclosed`;
7. a closing part where nothing is open: `unopened`;
8. anything else: `unexpected token`.

**Reduction.** On an infix or postfix operator of precedence `p`, the frames
on top are reduced while they bind tighter: a frame of precedence `q > p`; or
`q == p` and the incoming operator is left-associative; for `none`, `q == p`
is `non-associative`, at the incoming operator, with a note at the frame's;
for `chain`, `q == p` with a chain frame extends the chain. A prefix frame of
precedence `q` is reduced while `q > p`, so a prefix operator takes into its
operand every operator at least as tight as itself, as v1's climber does
(`-x ^ 2` is `-(x ^ 2)` with `-` at 70 and `^` at 80; `-x * y` is `(-x) *
y`). A chain frame and a flattening frame collect their operands and build
one node when reduced. Brackets are not reduced past: a mixfix frame stops
the reduction, as a group does in every operator-precedence parser.

### 6.3 Math's cases, as core v2 parses them

With math's table (section 17.1): prefix `-` `+` at 70, `^` at 80 right, `'`
at 75, `!` at 90, call and index at 100, relations 40 as a chain joined by
`and`, `not` at 30, `and` 20, `or` 10, `*` 60, and implied multiplication on:

| text | term | the rule |
|---|---|---|
| `-v[1]` | `neg(index(v, 1))` | the index (100) binds before the prefix (70) is reduced |
| `2 ^ v[1]` | `pow(2, index(v, 1))` | |
| `A[1][2]` | `index(index(A, 1), 2)` | a lead mixfix takes any operand |
| `x'[1]` | `index(transpose(x), 1)` | |
| `f(x)[1]`, `(a + b)[1]` | `index(call(f, x), 1)`, `index(add(a, b), 1)` | the group is the node `add`, `_GROUPED` |
| `A' ^ 2`, `A ^ 2'` | `pow(transpose(A), 2)`, `transpose(pow(A, 2))` | `'` (75) reduces `^` (80) first, as MATLAB reads it (S252) |
| `-A'`, `x!'` | `neg(transpose(A))`, `transpose(factorial(x))` | |
| `2 ^ 3!`, `-3!`, `n!!` | `pow(2, factorial(3))`, `neg(factorial(3))`, `factorial(factorial(n))` | S206 |
| `sin(x) ^ 2` | `pow(call(sin, x), 2)` | |
| `2 sin(x)`, `2(y)`, `(x)(y)`, `x(y)` | `mul(2, call(sin, x))`, `mul(2, y)`, `mul(x, y)`, `call(x, y)` | `_CALLEE` holds only after an ungrouped name (S201) |
| `|x| |y|`, `||x| - 1|` | `mul(abs(x), abs(y))`, `abs(sub(abs(x), 1))` | a bar in operator position closes the innermost open pair of bars, and in operand position opens one (S207) |
| `a < b <= c` | `and(lt(a, b), le(b, c))`, `b` one node | chain, joined by `and` (S214) |
| `(a < b) = c` | `eq(lt(a, b), c)` | a grouped relation is one operand, no link (S215) |
| `x > 0 and x < 1`, `not x = y` | `and(gt(x, 0), lt(x, 1))`, `not(eq(x, y))` | S213 |
| `[1, 2; 3, 4]`, `[a, b]`, `[]` | `matrix(row(1, 2), row(3, 4))`, `vector(a, b)`, `vector()` | rows where a `;` is seen (S260, S261) |
| `1/2x` | `mul(div(1, 2), x)` | juxtaposition as tight as `*`, left (S196) |

What stays math's: which name is a constant, the shapes of a literal's rows
(`bad_shape`), `piecewise`'s branches (S216), and the folds. Each is a check
over the finished term, or a check hook on each node as it is built
(section 6.5), with the node's span for the error.

### 6.4 The command language's and awk's cases

The command language:

- `a == b == c`, `a < b == c`: `non-associative` at the second operator,
  with a note at the first, as the grammar's section III requires.
- `ls -l | grep x ; echo done`: the command is a custom role on the verb's
  class (section 6.5) that takes its arguments while they are options or
  operands; `|` (20) and `;` (10) are flattening infix operators:
  `seq(pipe(cmd(ls, -l), cmd(grep, x)), cmd(echo, done))`.
- `not a and b`: `and(not(a), b)` with `not` at 50 above `and` at 40.

awk, as dawk's `dparse.c` has it (lane H's review of 11-05). Its ladder,
lowest first, with the precedences the suite gives it:

| level | operators | precedence | association |
|---|---|---|---|
| assignment | `=` and the compound forms | 10 | right |
| conditional | `?:`, a lead mixfix with a tail | 20 | right |
| or, and | `\|\|`, `&&` | 30, 40 | left |
| membership | `in`, a word | 50 | left |
| matching | `~`, `!~` | 60 | left |
| relations | `<` `<=` `!=` `==` `>` `>=` | 70 | none |
| input pipe | `\| getline`, a custom role on `\|` that names it | 80 | left |
| concatenation | juxtaposition, flattening | 90 | left |
| additive, multiplicative | `+` `-`, `*` `/` `%` | 100, 110 | left |
| unary | `!` `+` `-` | 120 | prefix |
| power | `^` | 130 | right |
| steps | `++` `--`: prefix, and postfix as a custom role | 140 | |
| field | `$` | 150 | prefix |
| call, index, group | `f(`, `a[`, `(`, mixfix | 160 | |

`| getline` stands between concatenation and the relations, as a reference
awk reads it: `cmd | getline line > 0` is `gt(pipeget(cmd, line), 0)`, and in
`"a" "b" | getline` the pipe's left is the whole concatenation. The cases,
each in the suite with its term (section 21):

1. **A step after what cannot be assigned begins the next operand.** The
   postfix `++` and `--` are a custom role that names its operator and
   declines unless its operand is an ungrouped name, a field or an element;
   the declined token is then juxtaposed and read as a prefix: `1 ++i` is
   `concat(1, preinc(i))`, `a ++b` is `concat(postinc(a), b)`, `(a)++ b` is
   `concat(a, preinc(b))`. Because the role names its operator, `$` is
   reduced first: `$i++` is `postinc(field(i))`, and `$++i` is
   `field(preinc(i))`.
2. **A bracketed list only `in` takes.** `(` is a transparent operator with a
   list hole (5.2): `(a)` is `a`, `(i, j)` is `group(i, j)`, and the check
   refuses the list everywhere but on `in`'s left, at the list. (`print (a, b)
   > "f"` is the statement parser's.) That `in`'s right is an array's name is
   a check too.
3. **The ladder** above: `a = b ? c : d ? x : y`, `a || b && c`,
   `a && b in arr`, `a ~ b < c` (`match(a, lt(b, c))`), `a < b c`
   (`lt(a, concat(b, c))`), `a + b * c % d`.
4. **`getline` in operand position**, a custom role with `_NO_JUXTAPOSE`:
   `getline` and `getline var` now; `getline < file` and
   `getline var < file` read the file at concatenation's floor, by the
   sub-parse (6.5, week 2). The pipe form takes the same optional target.
5. **`print`'s mode is scoped.** The statement parser reads `print`'s list
   with `D_PARSE_EXPR_OPTION_STOP` in a mode without the relational `>`, so
   the list ends at `,`, `>`, `>>` or a `|` not followed by `getline` (whose
   role declines), and the statement parser reads the redirection. Inside
   brackets `>` compares again (`print (a > b)`): each mixfix names the mode
   its holes are read in, `hole_mode`, 0 for the mode outside (F18, built in
   week 2 with the sub-parse).
6. **`^` above a sign, its right operand free to begin with one**: `-2^2` is
   `neg(pow(2, 2))`, `2^-3` reads, `2^-3^2` is `pow(2, neg(pow(3, 2)))`.
7. **A call only where `(` touches its name.** dawk's lexer makes the
   adjacent name a class of its own, so its table gives `(` no call role
   after an ordinary operand and `x (y)` is a concatenation (F4); the core's
   `_ADJACENT` flag does the same from a token's spacing, as the suite has
   it: `f(x)` is `call(f, x)`, `f (x)` is `concat(f, x)`. A builtin that
   takes a space before `(`, and `length` without one, are a custom role on
   the builtin class, each one's arity a check.
8. **What begins a concatenated operand.** A name, number, string, `/re/`,
   `$`, `!`, `++`, `--` or `(` may (`a !b`, `a $1`, `a (b)`); `-` and `+` may
   not, as their infix roles come first (`a b -1` is `concat(a, sub(b, 1))`);
   `getline` may not (`_NO_JUXTAPOSE`); `in` has no operand role. `/=` is
   divide-assign in operator position and begins a regular expression in
   operand position: the source's (F4). A newline ends an expression except
   after `&&`, `||`, `,` or a backslash: the source skips the ones that do
   not end it, and `D_PARSE_EXPR_OPTION_STOP` ends the expression at the
   ones that do, without an error.

### 6.5 Hooks

Hooks are where a language does what no table can say. Each is a C function
pointer with a context, and each is optional.

- **A custom role** (`_CUSTOM`) is called with the parser, the token, and the
  operand before it (in operator position). It may peek at and take tokens
  (`d_parse_expr_peek`, `d_parse_expr_take`), build nodes into the parse's
  term (`d_parse_expr_parser_term`), report (`d_parse_expr_report`), run a
  sub-parse, or decline. `peek` and `take` read in a mode of the hook's
  (0 for the parse's; a mode is one bit, and a parse's own is 1 unless its
  options give another, so a hook's own is another bit, 2 or 4), so a hook
  can have its source read a block raw, as
  awk's is between its braces (F19); since the parser cannot read a token
  again, a hook takes what it read in another mode before it returns, and a
  hook that leaves one stops the parse. The nodes it builds may be of an
  operator no role begins, defined for its form and spelling so that the
  renderer writes it: the command, `cmd`, a flattening infix spelled `""`. It returns 1 with the node it made, 0 to decline
  (the token is then read as if the role were not there; what it peeked
  stays peeked), or -1 for a failure it has reported. A node it returns must
  be of the term and within the guard.
- **The sub-parse** (`d_parse_expr_sub`, week 2) reads one expression inside
  a hook: `(parser, floor, mode, &node)`, the operators taken those that bind
  tighter than `floor`, in `mode`, ending without an error at the first token
  it cannot go on with, which the hook then peeks. `getline < file` reads the
  file with the floor at concatenation. It counts against the depth guard
  like any nesting, and its brackets scope its mode like any others (F18).
- **`may_juxtapose`** (section 5.4).
- **`check`** is called on each apply node as it is built, with its span; it
  may refuse it with a diagnostic of the language's domain. Math's `when` and
  `otherwise` outside `piecewise`, an index of three indices, a literal's
  rows of different lengths.

### 6.6 The depth guard

`depth` is the term's longest path from the root to a leaf, kept as nodes are
built (each node's in its `depth`). Past `max_depth` the parse reports `too
deep` and stops: at the token that would open an operator past it (open
operators nest at least as deep as the term they make), or at the operator
whose node is built past it (a long left-nested chain opens nothing). A node's depth is 16 bits,
so the guard is 1 to 65535, or `D_PARSE_EXPR_DEPTH_UNBOUNDED`; a value
between is refused, knob and option both, as no node could record it. The core needs no guard, since nothing in it recurses; the
guard is for what consumes the term and does recurse: v1's `cata` through the
bridge, math's simplifier. S250's review asked for it, with the bound the
owner's to set: the knob's default is 1000 (a 1 MB stack, Windows' default,
holds that many of v1's `cata` frames with room to spare), the option
overrides it, and `D_PARSE_EXPR_DEPTH_UNBOUNDED` leaves only memory.

## 7. Diagnostics

### 7.1 The channel

Every diagnostic goes into the foundation's sink (`dj_parse_diag_sink`), in a
new domain, `D_PARSE_DIAG_DOMAIN_EXPR` (4), with a byte span. So a parse, the
lexer before it, and the language's checks after it report into one ordered
report, as the foundation intends; a caller reads it as it reads any other
stage's.

| code | name (`parse.expr.`) | where | when |
|---|---|---|---|
| 0 | `unexpected` | the token | no role fits where it stands; a token after a whole expression |
| 1 | `missing_operand` | where it should stand: the token there, or the end | the end, a closing part or a separator where an operand must stand |
| 2 | `unclosed` | the open part | a mixfix never closed; with a note where the expression ended |
| 3 | `unopened` | the token | a closing part with nothing open |
| 4 | `mismatched` | the token | a closing part of another open operator than the innermost; with a note at the innermost open part |
| 5 | `non_associative` | the second operator | two `none` operators of one precedence side by side (`'==' cannot follow '<' without brackets`); with a note at the first |
| 6 | `arity` | the operator's first part | a list of fewer or more items than its bounds |
| 7 | `too_deep` | the token, or the operator past the guard | past the depth guard |
| 8 | `refused` | the node | the language's check refused it; its own message |
| 9 | `bad_signature` | none | sealing found the table inconsistent |

The names are lane E's registry's (agreed 11-05): the enum's tails,
lower-cased, under `parse.expr`, one spelling for the code; parse.expr's
registry table is written beside the enum in week 3.

Codes are append-only, as the foundation's are. Out of memory and a full
arena are the core domain's `D_PARSE_DIAG_OUT_OF_MEMORY` and
`D_PARSE_DIAG_CAPACITY`: a term or stack that could have grown and could not
is out of memory; one on the caller's storage, or in a build without the
heap, that is full is at capacity. A note is a record flagged
`D_PARSE_DIAG_FLAG_CONTINUATION`, following its error under its error's code,
so a reader keyed on domain and code finds the note's own text. A message
names the token where the text is given
(`unexpected '*'`, `a '(' is never closed`); without the text it names the
class's spelling, or says `token`.

Math maps them onto its statuses: 0 to `unexpected_token`, 1 to
`missing_operand`, 2, 3 and 4 to `unbalanced`, 6 to `wrong_arity`, 7 to a new
`too_deep`, and its own checks keep their statuses.

### 7.2 Line and column

A span is bytes. `d_parse_expr_locate(text, length, offset, &line, &column)`
gives the 1-based line, counting `\n`, `\r\n` and a lone `\r` each as one
break, and the 1-based column in characters (Unicode code points, as the
style guide counts a line's length). The lane G kit's `d_cli_report_at` takes
them: `g.vp:2:12: error: ...` (cli-taken 80).

### 7.3 Which error first

The parse goes on after an error, so that it can find one further left that
it only learns of later (a bracket never closed is known at the end, and
placed at its opening). Recovery is plain: a missing operand becomes a hole;
an unexpected token is skipped; a mismatched close closes the frames inside
the one it matches, or, where none awaits it, closes the innermost; a
non-associative pair is taken as left-associative; the
end closes what is open. It stops after `max_errors` errors.

The result names one diagnostic as **the** error: by default the leftmost,
which is math's rule ("of two errors the one further left is given", S209) and
every compiler's. One difference from math's parser layer: today a bracket's
error comes before a grammar error wherever it is, because brackets are
matched in a pass of their own. `1 + * 2)` is today `unbalanced` at the `)`,
and with core v2 `unexpected '*'`, which is further left. The option
`error_order` keeps math's order where its tests need it: `structure_first`
names the leftmost bracket error where there is one.

## 8. Folds and walks

A fold is an algebra applied bottom-up: one pass over the arena in order, each
node's value computed from its children's, held in an array of values the
caller sizes (or the core allocates, under the heap knob).

```c
typedef int (*dj_parse_expr_algebra)(void*                              _ctx,
                                     const struct dj_parse_expr_fold_view* _view,
                                     void*                              _out);
int d_parse_expr_fold(const struct dj_parse_expr_term* _term,
                      uint32_t                         _root,
                      size_t                           _value_size,
                      void*                            _values,   // a value per node
                      unsigned char*                   _marks,    // a byte per node
                      dj_parse_expr_algebra            _algebra,
                      void*                            _ctx,
                      uint32_t                         _flags,
                      void*                            _out);
```

The view gives the node and its children's values, by index, with no copy. A
fold marks the nodes the root reaches, then visits them, each once, in arena
order, so a chain's shared
operand is folded once. An algebra that fails stops the fold and its failure
is returned. Strict evaluation suits math's folds as they are (v1's `cata` is
strict too); for the lazy case, `and`'s second operand or `piecewise`'s
branches, there is the walk: depth first with an explicit stack, an `enter`
that may skip a node's children, and a `leave`.

## 9. Rendering

`d_parse_expr_render` writes a term back as text, through a write function
(`size_t (*)(void*, const char*, size_t)`, which lane D's stream takes), or
into a buffer as `snprintf` writes (`d_parse_expr_render_buffer`), with an atom
writer for the leaves (by default, the leaf's span's text where the source
is given and holds it, nothing for an empty span, and otherwise its value in
decimal). It is iterative too, and the
bracket rule below is the parser's own decision (one function, shared), so
the two cannot drift.

**Brackets** are written where reading the text back would group otherwise:

- the operand on an operator's left needs them where it is an infix or prefix
  node that binds looser than the operator, or as tightly on the wrong side
  (a right- or non-associative or chain operator);
- the operand on its right needs them where it is an infix or postfix node
  that binds looser, or as tightly on the wrong side (a left- or
  non-associative or chain operator): `A ^ (2')`, `a - (b - c)`;
- a postfix node on the left and a prefix node on the right never need them,
  since nothing can reach into them: `x'[1]`, `2 ^ -x`; S252's rule for the
  index ("bare where the operand is a leaf, a call, a literal, an index or a
  postfix operator's") is this rule;
- a mixfix node's holes never need them;
- the rule looks at an operand's whole open edge, not only its top operator:
  in `(a + not b) <= n` the left operand's top operator, `+`, binds tighter
  than `<=`, but its right edge ends in the prefix `not`, looser than `<=`,
  which would take `b <= n` if the brackets went, so they stay.

A chain's join (`_CHAIN`) is written as the chain, `a < b <= c`; a flattened
node with its operator between each operand; a juxtaposed node with the
operator's spelling, so math's implied product is written `2 * x` as the term
holds a product, or, for an operator spelled `""` (awk's concatenation), as a
space, with brackets around a right operand that begins with a token that has
a role in operator position (`a (-1)`). Options: keep the author's groups
(`_KEEP_GROUPS`), and the spacing (`_COMPACT`: spaces only around words).

The renderer's promise, tested both ways: reading a rendered term gives the
same term, spans and `_GROUPED` aside; and rendering a read text gives a text
that reads as the same term.

## 10. The default scanner

Most clients need no lexer of their own. `d_parse_expr_scan` (week 2) is a
source for the common case: blanks skipped (newlines as a class of their own,
where asked);
names (`[A-Za-z_][A-Za-z0-9_]*`, and the bytes 0x80 and above, so UTF-8 names
pass whole); numbers (digits, a point, an exponent); strings in `"` and `'`
with backslash escapes; and every spelling the signature gives its operators
and parts, longest first, so `<=` before `<` and `!=` before `!`. A word that
is an operator's spelling (`and`) is the operator only where the next byte
cannot continue a name. Math keeps its own lexer (its numbers are
`decimal.h`'s grammar, S189); the CLI grammar has its own option shapes; the
scanner is for bitwisedom, the templates, vparse's notations and tests.

## 11. Control: knobs and options

Each setting that takes a default and an override is a knob that sets the
default and an option that overrides it, the owner's rule, in lane E's
knob-and-option pattern (agreed 11-05, its API frozen 11-13). Three are rows
of a program-wide `parse` group: `expr_max_depth` (1 to 65535, or the named
value `unbounded`), `expr_max_errors` (1 to 65535) and `expr_error_order`
(`leftmost` or `structure_first`), each defaulting to its knob. So
`DJINTERP_PARSE_EXPR_MAX_DEPTH=unbounded` and
`--parse.expr_error_order=structure_first` work. An option of 0 means the
setting in force, read through its cache when a parse starts; the group sits
in a unit of parse's own, and a knob (`D_CFG_PARSE_SETTINGS`, default 1)
compiles it out, 0 then meaning the knob again. Until week 2 builds it, 0
means the knob. The group is written before a program starts its threads,
and its header says so (promise 8).

| knob (`cfg_parse.h`) | default | option (`dj_parse_expr_options`) | what it is |
|---|---|---|---|
| `D_CFG_PARSE_EXPR_HEAP` | `D_CFG_PARSE_ALL`, else 1 | none | the heap forms; off, every term, signature and scratch is the caller's storage |
| `D_CFG_PARSE_EXPR_MAX_DEPTH` | 1000 | `max_depth` | the depth guard (section 6.6) |
| `D_CFG_PARSE_EXPR_MAX_ERRORS` | 16 | `max_errors` | errors before the parse stops, 1 to 65535; 1 stops at the first met |
| `D_CFG_PARSE_EXPR_ERROR_ORDER` | leftmost | `error_order` | which error is the error (section 7.3) |
| `D_CFG_PARSE_EXPR_RENDER` | 1 | none | the renderer compiled in |
| `D_CFG_PARSE_EXPR_SCAN` | 1 | none | the default scanner compiled in (week 2) |
| none | | `flags` | `D_PARSE_EXPR_OPTION_STOP`: end at a token the expression cannot go on with (F17) |

A build without the heap is a full one: every structure has a fixed-storage
initializer. The parser's stacks (operands, open operators, a chain's links)
start in its own frame, 128, 64 and 32 deep, which holds what a person
writes; with the heap they grow past that, and without it a parse that needs
more stops at `capacity`. Week 2 adds the caller's scratch for those builds:
a buffer passed in the options, sized by `d_parse_expr_scratch_size(tokens)`.
`d_parse_expr_term_copy` needs scratch too, and without the heap copies
nothing until then.

No setting is thread-related: the core has no shared state, a sealed
signature is read-only, and a parse writes only its own term, scratch and
sink. That is promise 8's "says why not".

## 12. Memory and sizes

Every allocation goes through `d_parse_grow` or one call of the caller's
choosing, and every size is computed with `d_ckd_mul` and `d_ckd_add` before
the call (promise 3). Counts are 32 bits; a text of 4 GiB or more, or a term
of more than 2^32 - 1 nodes, is `capacity`, not a wrap. `sizeof`, `offsetof`
and `alignof` of the node, the token and the term are asserted in C and C++
(promise 3's layout gate); the node's size and offsets are asserted now, the
rest with the face in week 2.

## 13. The C++98 face

`djinterp::parse::expr`, one class per C structure, each holding its C
structure and calling the C functions, so nothing is computed twice and the
parity tests compare like with like.

- `signature`: builder methods (`atom`, `prefix`, `infix`, `postfix`,
  `mixfix`, `juxtapose`, `custom`), each returning `*this`; `seal()`.
- `term`: nodes as `node_view` (kind, op, children, span, flags), equality,
  digest; copyable at every level, movable from C++11.
- `parse(signature, source, options)` and `parse(signature, tokens,
  options)`, returning a `result`: the term, the diagnostics, the error's
  index, and whether it succeeded. A source is any class with a `next`
  member of the C shape's arguments; a C++98 functor, a C++11 lambda through
  an adapter.
- `fold<Value>(term, algebra)`: the algebra a functor `Value
  operator()(node_view, const Value* children, std::size_t count)`; the
  values held in a `std::vector<Value>` in arena order, no recursion.
- `walk(term, visitor)`; `render(signature, term, options, atom_writer)`
  returning a `std::string` or writing through a functor.
- `diagnostics`: a C++98 view over the sink: count, each record, its
  message, the error. The foundation's `diagnostic.hpp` is C++11-floored;
  either its floor comes down to C++98 in week 2's polish (the better
  answer: one view at every level), or `expr` carries the C++98 view until it
  does. Week 2 decides by what the port costs.

Above C++98, and only as additions: move constructors and `noexcept` from
C++11, lambdas and `std::initializer_list` tables for signatures from C++11,
`std::string_view` texts from C++17, concepts for the algebra and the source
from C++20. Nothing that changes a result depends on the level.

The **bridge** (`bridge.hpp`, C++11 and up, v1's floor):
`to_expression<OpId, Atom>(term, op_of, atom_of)` builds v1's
`expression<OpId, Atom>` from a v2 term, iteratively, so math's folds, its
simplifier and lane D's layout keep their v1 term; and
`from_operator_signature(operator_signature<OpId>)` gives a v2 signature
from a v1 table, for a language moving its table.

## 14. Tests

| suite | language | what it holds |
|---|---|---|
| `t_parse_expr_c` | C99 to C23 | signatures sealed and refused, each form, each role condition, each diagnostic with its span and note, recovery, the depth guard, folds, walks, rendering, the scanner, each fixed-storage form |
| `t_parse_expr_cpp` | C++98 strict to C++26 | the same through the face |
| `t_parse_expr_parity` | C++98 to C++26 | each text of the corpus through the C API and the face: the same arena node for node, the same diagnostics (domain, code, span), the same render, the same fold |
| `t_parse_expr_roundtrip` | C | 100,000 random terms over a test signature with every form, rendered, read back, compared; and random texts read and rendered, read again |
| `t_parse_expr_math` | C++, with the math session | every text of math's face test and fuzz, through math's v2 table and through its parser layer: the same term, the same status at the same character, but where 7.3's difference is the reason |
| the command grammar's | C++, lane G's, in `tests/djinterp/cpp/modules/interp/` | the command grammar's examples, on the interpreter's table |
| fuzz harness | C, clang | the parser over random bytes through the scanner: no crash, no leak, no undefined behaviour, time linear in the input |

All of it in DTest 2.0 (frozen Friday 11-06), on the ladders with GCC and
Clang, plain and testing, under ASan and UBSan, and under the knob matrix
(heap off, renderer off, scanner off). The parse is held to linear time by a
test that doubles the input and expects at most twice the steps, counted, not
timed.

## 15. Files, names, and what happens to v1

**C core**, `inc/djinterp/c/modules/parse/expr/` and
`src/djinterp/c/modules/parse/expr/`: `term.h`, `signature.h`, `parse.h`,
`fold.h`, `render.h`, `scan.h` (week 2), and `expr.h`, the umbrella; the
sources share one header of their own, `expr_internal.h`, which holds the
decision the parser and the renderer both make. **C++ face**,
`inc/djinterp/cpp/modules/parse/expr/`: the same stems in `.hpp`, and
`bridge.hpp`. **Config**: the knobs join `cfg_parse.h`, one file for the
subframework as the config readme asks. **Tests**:
`tests/djinterp/c/modules/parse/expr/` and
`tests/djinterp/cpp/modules/parse/expr/`.

**Names**: functions `d_parse_expr_*`, types `dj_parse_expr_*`, macros
`D_PARSE_EXPR_*`, the namespace `djinterp::parse::expr` (`NS_EXPR` inside
`NS_PARSE`). Guards and banner tags follow their siblings in the tree:
`DJINTERP_C_PARSE_EXPR_TERM_H`, `DJINTERP_PARSE_EXPR_TERM_HPP`, `[c/parse]`.
The style guide derives a guard from every segment (`..._C_MODULES_PARSE_...`)
and a tag from the top directory (`[c]`); all 171 of the tree's headers under
a `modules/` directory leave it out of their guards, and its parse banners say
`[c/parse]`. When lane A settles which
is right, these follow in the same sweep.

**v1**:
- `expression.hpp`, `expression_ops.hpp`, `expression_static.hpp`,
  `expression_render.hpp`: kept, the C++11 algebraic view over a term (math,
  lane D's layout); preview in 0.8; reached from v2 by the bridge.
- `expression_parse.hpp`, the climber: deprecated in 0.8 (its banner says
  so, and points at v2); retired when math has moved, in 0.9 at the latest.
- `cpp/modules/parse/expression_ops.hpp`, `expression_parse.hpp`,
  `expression_render.hpp` and `expression_static.hpp`, one directory up:
  stale copies of 09.21 that include `../math/expression.hpp`, which does not
  exist, and fail at every level; nothing includes them. They retire in this
  delivery: 4 of the 14 C++ units that fail at every level.

## 16. The two `/parse/` foundations (style #80)

The parse session and the vparse session each built a substrate under
`/parse/`. What overlaps:

| concept | the parse session's | the vparse session's (FOUNDATION.md) |
|---|---|---|
| diagnostics | `parse_error` (C++: a status, a token offset, a string), and the C BNF parsers' booleans | `dj_parse_diag_sink` (C, with spans, domains and codes; the lexer reports into it too) |
| a grammar | `dj_parse_grammar` of `parsers/bnf`, `abnf`, `ebnf` (C) | `dj_parsegen_grammar`, the neutral grammar (C) |
| a token | `parse/token/token.hpp`, the BNF parsers' `dj_parse_token`, `lex/lex_token.h` | the machine's family-private cursor |
| a cursor and a result | `parse_state<T>`, `parse_result<R>` | the machine and its program |
| an older vparse | `cpp/modules/parse/vparse/` and its sources | `cpp/modules/parsegen/vparse/`, the consolidated one |

**Options.**

- **A. One architecture**, joined from the bottom: one diagnostic channel
  (the sink), one span, one growth policy, under everything; core v2 the
  first module built on it, used by the combinators' expression parsing and
  by the CLI; `parse_error` gains a span and writes itself into a sink, so the
  combinator substrate stays and reports where everything else does; the C
  BNF, ABNF and EBNF parsers become parsegen frontends producing the neutral
  grammar, and `dj_parse_grammar` retires; the older vparse retires.
  For: one report from any pipeline, which is what the sink was built for;
  one grammar model for parsegen's analysis to run on; the parity law
  satisfied once, in the C layer. Against: the most work, spread over two
  releases; the BNF parsers' callers change in 0.9.
- **B. Both kept, the boundary documented**: combinators for hand-written
  parsers, the machine for generated ones; a converter between the two
  diagnostic shapes. For: nothing moves. Against: three channels stay three,
  two grammar models drift apart, and every new module chooses between them.
- **C. One retired**: the combinator substrate (C++ only, no C core) or the
  machine. Against: the first holds the CLI and most of parse's C++ users,
  the second holds parsegen; neither can go.

**Recommendation: A**, in this order. 0.8 (November): the sink and the span
for everything 0.8 makes stable, which is core v2 and the lexer (already),
with `parse_error` given a span and `emit_into(sink)`; the older vparse and
the stale expression copies retired; the BNF parsers keep their API and gain
the sink as an optional second channel. 0.9 (December): the BNF family as
parsegen frontends, `dj_parse_grammar` retired, and parse's outer layers
fixed as lane B's suites find faults. Each step is a delivery of its own, so
nothing waits on the whole.

## 17. Adoption, by client

### 17.1 Math (the math session)

**S190**: decided for core v2 (its parser holds postfix operators with
operands and calls); **S209** and **S252** follow (their passes retire when
math moves).

Math's table, in v2's terms:

| class | position | role | operator | precedence | association |
|---|---|---|---|---|---|
| number | operand | atom | | | |
| name | operand | atom, `_CALLABLE` | | | |
| `-` `+` | operand | prefix | `neg`, `pos` | 70 | |
| `-` `+` | operator | infix | `sub`, `add` | 50 | left |
| `*` `/` `mod` | operator | infix (`mod` a word) | `mul`, `div`, `mod` | 60 | left |
| none | operator | juxtaposition, with G5's option | `mul` | 60 | left |
| `^` | operator | infix | `pow` | 80 | right |
| `'` | operator | postfix | `transpose` | 75 | |
| `!` | operator | postfix | `factorial` | 90 | |
| `=` `!=` `<` `<=` `>` `>=` | operator | infix | `eq` to `ge` | 40 | chain, joined by `and` |
| `not` | operand | prefix, a word | `logical_not` | 30 | |
| `and`, `or` | operator | infix, words | `logical_and`, `logical_or` | 20, 10 | left |
| `(` | operand | mixfix, transparent | group | | |
| `(` | operator | mixfix, lead, `_CALLEE` | `call` | 100 | |
| `[` | operand | mixfix, list with rows | `vector`; rows: `matrix` of `row`s | | |
| `[` | operator | mixfix, lead, list of 1 or more | `index` | 100 | |
| `|` | operand | mixfix, one item, with G5's option | `abs` | | |

The plan, step by step, each step checked by every text of math's face test
and fuzz reading as before:

1. Math's lexer becomes a v2 source (its tokens already carry their offset).
2. Its table is built as above; `piecewise`, the constants and the literal's
   shapes as a check hook and a pass over the finished term.
3. `expr_parse` runs core v2 and converts through the bridge to `expr_term`;
   the folds, the renderer and the simplifier are untouched.
4. The parser layer's passes (roles, brackets and bars, groups put back,
   chains joined, the index table) are deleted.
5. Later, at math's choice: its folds onto the arena (one pass, no
   recursion, the shared operand folded once), its renderer onto v2's (S208's
   adapter gone).

What math must accept or set: the error order (section 7.3), for which the
option exists; `too_deep` as a new status; a call's term has its callee as
its first child (`call(f, x)`), which the bridge turns into math's `expr_op`
with its name.

### 17.2 Lane G: the command language, the DSL, bitwisedom

Lane G agreed to core v2 for the command language on 11-05. Week 2: G
builds its source (the reader, its P1 to P7) and its table on the C core as
delivered. Week 3 (16 to 22): the two are joined, with lane F: the table of
`djinterp.peg`'s section III (`;` 10 and `|` 20 flattening, `or` 30 and
`and` 40, which G flattens too, `not` 50 prefix, the comparisons 60
non-associative), the command as a custom role on `WORD` in operand position
building `cmd(verb, arg1, ..., argN)`, each word a leaf whose span is the
word as written, groups. `functional_cli.hpp` then moves to lane G's
interpreter module with its spine on v2's term. December, by Friday 12-04:
the grammar's own predicates, explicit `and`, `or`, `not` and comparisons
over command primaries, and blocks, `{ Sequence }` (G's P8), as an apply
node among a command's arguments, read by the sub-parse with `}` among its
ends; the language has no juxtaposition and no `if ... then ... else`.
G drafts the edits to `djinterp.peg` and its ABNF and EBNF companions for
P1 to P8 as a patch in Monday's delivery; lane F takes them in week 3.
bitwisedom's integer expressions stay on their own parser in 0.8 and 0.9.

Answered 11-05: `d_cli_report_at` takes a 1-based line and column, which
the interpreter gets from `d_parse_expr_locate`; a command's arguments are
leaves of its node. The command language's table and its suite live in the
interpreter's module (`tests/djinterp/cpp/modules/interp/`), the table being
the interpreter's; this suite keeps its own CLI-like cases.

### 17.3 Lane H: dawk

Section 6.4 shows awk's expressions on v2, with the eight constructs lane H's
review of 11-05 found 6.4 missing; each is in the suite with its term, but
`getline < file` and `print`'s scoped mode, which come with week 2's
sub-parse and `hole_mode`. Lane H moves dawk's expressions onto core v2 in
December, when awk becomes a dialect of the DSL, and until then dawk's
suite is the reference core v2's awk table is checked against.

### 17.4 Lane E

Answered 11-05: domain 4 and its ten codes are agreed, named as in 7.1; a
note carries its error's code (done); section 11's settings are rows of a
`parse` group in E's pattern (week 2, with option_env's named values); a
describer for a signature (identity, state, each operator's spellings, role,
precedence, association and arity, its bytes) in December, once desc's
describer interface is frozen. The core logs nothing and prints nothing: the
log reaches parse's diagnostics as a subscriber, through the sink's hook.

## 18. Schedule

| date | what |
|---|---|
| Wed 11-04 | the C core built and tested (section 21); the 22 allocation sites checked |
| Fri 11-06 | this design agreed; S190 and style #80 decided |
| Mon 11-09 | the scanner; the sub-parse and `hole_mode` (F18); the caller's scratch; the C suite on DTest 2.0 |
| Wed 11-11 | the C++98 face; the bridge; the parity suite; the round trip at 100,000 terms and the fuzz harness |
| Fri 11-13 | interfaces agreed; the `parse` settings group on lane E's option_env; core v2's code to math, G and H; parse polish begins (the 14 C++ units and 1 C unit failing at every level; the 4 to gate below C++11; parse's long functions) |
| week 3 | parsegen's and vparse's notation readers on v2; the command grammar with lane G; vparse's long functions split |
| week 4 | suites for parse's stable core, docs; the bison, flex and LLVM adapters' design note |
| December | the DSL's hooks for lane G by 12-04; v1's climber retired once math has moved; a result flag saying every error could be mended by more input (a missing operand at the end, an unclosed bracket), for the interpreter's continuation prompt (lane G) |

## 19. Decisions

Each with its alternatives; the recommendation is the one taken.

**F1. The term is a flat arena in post-order, linked by index.**
- *For:* copies, hashes and serialises whole, as the foundation's programs and
  grammars do; a fold is one pass, with no recursion; a chain's shared operand
  is one node, folded once; C and C++ hold the same bytes, so parity is exact.
- *Against:* a subtree is not a value of its own; a rewrite writes a new
  term, as v1's do anyway.
- *Alternative, a tree of pointers in C:* each node allocated; no
  serialisation, and the depth bound by recursion again.
- *Alternative, v1's term in C++ over a C parser:* no C term, so no C folds
  and no parity at the term.

**F2. The parser is operator precedence over explicit stacks, with a role per
token class and position.**
- *For:* linear, no recursion, any depth; roles by position are Pratt's,
  which every language here needs; mixfix and juxtaposition fit as frames;
  recovery is a local decision at each token.
- *Against:* harder to read than a recursive Pratt parser.
- *Alternative, a recursive Pratt parser:* simpler code, and the depth bound
  by the stack, which S250 spent a chunk working around.
- *Alternative, LR tables from parsegen:* parsegen is preview, and a table
  needs a generator step before every change of a language's table.

**F3. Diagnostics go into the foundation's sink, in a domain of their own,
the leftmost error first, with recovery.** *For:* one report per pipeline;
the S209 rule made general. *Against:* math's bracket-first order changes,
which the option keeps where needed. *Alternative, the first error only:*
cheaper, and an unclosed bracket further left is missed.

**F4. Tokens are pulled, and the source told the position.** *For:* awk's
`/`, math's bars and any lexer that must know whether an operand ends here.
*Against:* a callback per token. *Alternative, an array only:* those lexers
need a pass of their own again. The array form is given too.

**F5. Names: `d_parse_expr_`, `dj_parse_expr_`, `D_PARSE_EXPR_`,
`djinterp::parse::expr`, directories `expr/`.** *For:* the siblings'
pattern (`d_parse_diag_`, `d_parse_machine_`); `D_EXPR_` is already the math
lane's macro prefix (`D_EXPR_RELATION_OP` in `math/function/function.hpp`);
`expr/` keeps v2 apart from v1's `expression/`. *Against:* long names.
*Alternative, `d_expr_` and `expression/`:* shorter, and two prefixes and two
generations in one directory.

**F6. 32-bit values throughout.** *For:* the face compiles at ISO strict
C++98 on every target. *Against:* a leaf holds an index, not a pointer or a
64-bit number. *Alternative, a 64-bit payload:* the face then has a 64-bit
floor, as `pool.h` has.

**F7. A depth guard as a knob of 1000 and an option, over a core that needs
none.** *For:* S250's review; recursing consumers protected on a 1 MB stack.
*Against:* a sum of more than 1000 terms is refused by default, and the
option is the way past it. *Alternative, unbounded by default:* the core is
safe, and v1's `cata` overflows a small stack on a deep enough term.

**F8. v1's term and folds stay, as the C++11 algebraic view, with a bridge;
its climber is deprecated in 0.8.** *For:* math's folds and lane D's layout
unchanged; math moves at its pace. *Against:* two term types until math's
folds move. *Alternative, retire v1 in 0.8:* math and lane D rewrite in
November.

**F9. Style #80: one architecture, joined from the bottom (16, A).**

**F10. The depth guard is 1 to 65535, or unbounded.** A node keeps its depth
in the 16 bits its 24-byte layout has free. *For:* the node stays 24 bytes;
no consumer that recurses can use a bound near 65535 frames on an ordinary
stack. *Against:* no bound between 65536 and none can be asked for.
*Alternative, a 32-bit depth:* the node grows to 28 bytes, a sixth more per
node, for a range nobody uses. *Alternative, a guard above 65535 clamped or
read as none:* silent, and it either refuses terms the caller allowed or
drops the protection asked for.

**F11. A closing part that nothing open awaits, inside an open operator, is
one `mismatched` error, and closes the innermost.** *For:* one slip, one
error, at the character (`(1 + 2]`), and the parse goes on in step with the
text. *Against:* where the writer left out an opening bracket, the close
lands on the wrong one; the note at the innermost open part shows which.
*Alternative, `unopened` at the token and `unclosed` at the end:* two errors
for one slip, the second far from it.

Five more came of the lanes' review on 11-05 (H, E, G).

**F15. A custom role in operator position may name its operator, and is then
called after the reductions that operator would make.** *For:* the hook is
given the operand the operator would take (`$i++`'s `$i`, the whole
concatenation before `| getline`) with no parser internals in its hands; a
role naming no operator behaves as before. *Against:* a hook that declines
leaves those reductions made; they are the ones any looser operator would
make next, so only a later role of the same class at a tighter precedence
could tell, and roles are tried in their order. *Alternative, a predicate on
operator roles:* the same question of what the predicate sees, and a second
way to write what a custom role already is. *Alternative, a reduce call in
the hook interface:* every hook then reasons about frames.

**F16. A transparent operator with a list hole is a group for one item and
its own node for any other count.** *For:* `(a)` stays the bracket-free
node of invariant 5, and awk's `(i, j)` (a tuple, elsewhere) is a node a
check can place. *Against:* one operator id for two shapes; a fold tells them
apart by the node's kind, a leaf or an application of the group's id.
*Alternative, two operators on `(`:* the parser would have to choose before
it has seen the comma. *Alternative, a node for every group:* every client
then steps through bracket nodes, which invariant 5 exists to spare it.

**F17. `D_PARSE_EXPR_OPTION_STOP` ends an expression at a token it cannot go
on with, and the result gives the token back.** *For:* a statement's parser
(awk's, the DSL's blocks, C's `if (`) hands the core an expression and takes
back the `;`, the newline or the `)`, and the table, which already says what
continues an expression, says what ends it. *Against:* a typo that would
have been `unexpected` ends the expression instead, and the statement parser
reports it. *Alternative, the source ending the input there:* the source
cannot know what the table continues with, and the statement parser loses
the token. *Alternative, a set of ending classes per parse:* each language
lists its terminators and misses one at a time.

**F18. A mixfix's holes are read in the mode it names (`hole_mode`, 0 for
the mode outside).** *For:* `print (a > b)` compares while `print a > b`
redirects, by the table, and a sub-parse's mode reaches no further than its
brackets. *Against:* one more field in the descriptor. *Alternative, every
bracket restoring the parse's first mode:* wrong where the statement parser
starts the parse in `print`'s mode, as dawk's would. (Built in week 2, with
the sub-parse.)

**F19. `peek` and `take` read in a mode of the hook's, and a hook may not
leave behind a token read in another mode.** *For:* a source reads awk's
block raw when a hook asks for it, unaware of hooks, as lane G recommends;
the rule follows from a pull source, which cannot read a token twice.
*Against:* a hook must take what it read raw. *Alternative, the sub-parse's
mode alone:* a raw block is not an expression. *Alternative, letting a hook
change its source's state between takes:* the source then knows of hooks,
and the parser must promise how far it reads ahead.

## 20. What each lane is asked, by Friday 11-06

- **The math session:** S190 for core v2; whether the error order of 7.3
  should be math's default (the option exists either way); the depth guard's
  default (1000, or the bound chunk 33's probe measured), within F10's range;
  whether F11's reading of `(1 + 2]` suits (it maps to `unbalanced` either
  way, now at the `]` alone).
- **Lane G:** the two questions of 17.2; whether week 3 suits the command
  grammar. *Answered 11-05 (17.2); F19 came of it.*
- **Lane H:** whether awk's grammar has a construct 6.4 misses. *Answered
  11-05: eight, now in 6.4 (F15 to F18 came of them).*
- **Lane E:** the domain and codes of 17.4. *Answered 11-05 (17.4, 11).*

## 21. The build in this delivery

### 21.1 What is there

| file | what |
|---|---|
| `inc/.../parse/expr/term.h`, `src/.../term.c` | the arena: fixed or heap storage, leaves, applications, holes, copy, verify, equality |
| `inc/.../parse/expr/signature.h`, `src/.../signature.c` | operators of the four forms, chains, mixfix descriptors, juxtaposition, roles, sealing with its diagnostics, lookup |
| `inc/.../parse/expr/parse.h`, `src/.../parse.c` | tokens and sources (the array source), the parser of section 6, the hook interface, `d_parse_expr_locate` |
| `inc/.../parse/expr/fold.h`, `src/.../fold.c` | the fold (marks, then arena order) and the walk (`enter`, `leave`, skip and stop) |
| `inc/.../parse/expr/render.h`, `src/.../render.c` | the renderer, through a write function or into a buffer |
| `inc/.../parse/expr/expr.h` | the umbrella |
| `src/.../parse/expr/expr_internal.h` | the frame kinds and the one bracket decision the parser and the renderer share |
| `inc/djinterp/config/.../cfg_parse.h` | the five knobs of section 11 that exist now, validated |
| `inc/.../parse/diagnostic.h` | `D_PARSE_DIAG_DOMAIN_EXPR` (4) |
| `tests/.../parse/expr/t_parse_expr.c` | the C suite: thirteen sections, every case of 6.3 and 6.4 (dawk's ladder and lane H's eight among them), every code of 7.1, recovery, both error orders, the guard, a full arena on caller storage, the stop option, a hook's own mode, the command language rendered back, 3,000 random round trips, the renderings named in 9, folds and walks, `locate` |

### 21.2 Measured

- The suite passes at C99, C11, C17 and C23 with GCC under ASan and UBSan,
  and with Clang under valgrind (no errors, no leaks).
- 840,000 random terms (every form of math's table, depth 1 to 6) rendered
  and read back as themselves: none failed.
- Line coverage under the suite: 87.4% of the five sources (term 85%,
  signature 91%, parse 91%, render 81%, fold 83%), against the core's floor
  of 80%.
- Each rule added on 11-05 fails its cases when taken out: without F15,
  `$i++` reads as `field(postinc(i))` and `a b | getline` as
  `concat(a, pipeget(b))`; without F16, `(i, j) in arr` does not parse.
- Every header and source compiles at every C level and every C++ level, GCC
  and Clang, plain and testing, ISO strict C++98 included; and under the knob
  matrix (heap off, renderer off, both, `D_CFG_PARSE_ALL` off, the other
  knobs at their edges).
- A build without the heap reads a 10,001-token left chain and 40 nested
  groups in its own stack rooms, and stops a 5,001-deep right chain and 3,000
  nested groups at `capacity`, with nothing overrun.

### 21.3 Not yet

The scanner (`scan.h`); `d_parse_expr_sub` and `hole_mode` (F18); the
`parse` settings group (lane E's option_env); the caller's scratch for builds
without the heap, and `d_parse_expr_scratch_size`; `d_parse_expr_term_digest`;
the C++98 face, the bridge and the parity suite; the suites on DTest 2.0; the
round trip at 100,000 terms and the fuzz harness as suites of their own; the
token's and the term's layout asserts. All of it is week 2's (section 18).

### 21.4 What building it changed in this text

Each is folded into its section above; listed here so a reader of the draft
of Monday 11-02 sees what moved.

- 4.1: the node's spare 16 bits hold its depth.
- 4.2, invariant 5: a grouped leaf keeps its token's span.
- 5.2, 5.3: the separators' spellings, for the renderer; a role's `order`.
- 5.4: a juxtaposed node's span covers both operands.
- 5.5: sealing's refusals, as built.
- 6.2: a closing part nothing awaits, inside an open operator, is one
  `mismatched` (F11); an outer operator's part re-reads its token.
- 6.5: the hook interface, as built.
- 6.6: the guard is checked at open operators and at built nodes, and is 1
  to 65535 or unbounded (F10).
- 7.1: `capacity` against `out of memory`; notes as continuations.
- 8: the fold takes a mark per node.
- 9: the buffer form; the bracket rule over an operand's whole open edge,
  found by the round trip.
- 11: the stack rooms of a build without the heap.

And from the lanes' review of 11-05:

- 1, 17.2: lane G's December language corrected: explicit predicates and
  blocks, no juxtaposition, no `if ... then ... else`.
- 5.2: a transparent operator with a list (F16).
- 5.3, 6.2: a declined role falls through to juxtaposition; a custom role
  may name its operator (F15); the stop option (F17).
- 5.5, 6.5: an operator only a hook builds is allowed; `peek` and `take` in
  a mode (F19); the sub-parse's shape, and `hole_mode` (F18).
- 6.4: dawk's whole ladder and lane H's eight constructs.
- 6.6, 7.1: `too_deep` at the operator past the guard; the registry's names;
  a note under its error's code; `non_associative` names both operators.
- 11: the settings as lane E's `parse` group rows; the error allowance 1 to
  65535; the `flags` option.

And from lane G's check of the merged deliveries, 11-06:

- 6.5: a mode is one bit, and a parse's own is 1, so a hook's own is
  another.
- 9: a leaf with an empty span renders as nothing, not as its value.
