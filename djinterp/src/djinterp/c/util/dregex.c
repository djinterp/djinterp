/*******************************************************************************
* djinterp [c][utility]                                                 dregex.c
*
*   Definitions for the non-inline declarations in dregex.h.
*     A pattern is parsed to an index-addressed tree, lowered by Thompson
* construction to a flat instruction program, and run by parallel NFA
* simulation.  Every live thread carries the offset at which it started, and
* threads are deduplicated per instruction per input position, which yields
* leftmost-longest semantics directly: the smallest surviving start wins, and
* among threads sharing that start the latest reported end wins.
*     Capture groups use a second execution path rather than a change to the
* simulation.  The simulation fixes the overall extent; a bounded backtracking
* pass then enumerates the parses of that one window, anchored at both ends,
* and keeps the POSIX-preferred assignment.  The fast path is untouched and
* pays nothing for the feature, which is how gawk separates its own matcher
* from its submatch engine.
*
*
* path:      /src/djinterp/c/util/dregex.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "../../../../inc/djinterp/c/util/dregex.h"  // corresponding header
// std
#include <ctype.h>   // isalpha, isdigit, isspace and the other class tests
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memset, memcpy, strncmp
// djinterp
#include "../../../../inc/djinterp/c/dckdint.h"  // d_ckd_add, d_ckd_mul


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Instruction program
//------------------------------------------------------------------------------
// 1.1.1
// dj_internal_op
//   enum: opcode of one compiled instruction.  CHAR, ANY and CLASS consume a
//   byte; BOL and EOL assert a position; SPLIT and JMP are control flow; SAVE
//   marks a group boundary; MARK records the offset at which one iteration of
//   a repetition began, LOOP resets that repetition's iteration count, and
//   PROGRESS either takes the back-edge, diverts to the loop exit in `x`, or
//   fails, according to whether the iteration consumed and whether any earlier
//   one did; MATCH reports the end of a candidate match.
enum dj_internal_op
{
    D_INTERNAL_OP_CHAR = 0,
    D_INTERNAL_OP_ANY,
    D_INTERNAL_OP_CLASS,
    D_INTERNAL_OP_BOL,
    D_INTERNAL_OP_EOL,
    D_INTERNAL_OP_SPLIT,
    D_INTERNAL_OP_JMP,
    D_INTERNAL_OP_SAVE,
    D_INTERNAL_OP_LOOP,
    D_INTERNAL_OP_MARK,
    D_INTERNAL_OP_PROGRESS,
    D_INTERNAL_OP_MATCH
};

// 1.1.2
// dj_internal_inst
//   struct: one compiled instruction.  `x` and `y` are absolute program
//   addresses, used by SPLIT and JMP; `value` holds the literal byte for CHAR,
//   the class index for CLASS, the slot number for SAVE, where group `n`
//   occupies slots `2n` and `2n + 1`, and the loop number for LOOP, MARK and
//   PROGRESS, where loop `k` occupies marks `2k` and `2k + 1` past the slots.
struct dj_internal_inst
{
    enum dj_internal_op op;
    int                value;
    int                x;
    int                y;
};

// 1.2    Pattern tree
//------------------------------------------------------------------------------
// 1.2.1
// dj_internal_node_kind
//   enum: kind of one parse-tree node.  Repetition is uniform: `*`, `+`, `?`
//   and `{n,m}` all become REP with a bound pair, where a negative maximum
//   denotes an unbounded repetition.
enum dj_internal_node_kind
{
    D_INTERNAL_NODE_EMPTY = 0,
    D_INTERNAL_NODE_CHAR,
    D_INTERNAL_NODE_ANY,
    D_INTERNAL_NODE_CLASS,
    D_INTERNAL_NODE_BOL,
    D_INTERNAL_NODE_EOL,
    D_INTERNAL_NODE_CAT,
    D_INTERNAL_NODE_ALT,
    D_INTERNAL_NODE_GROUP,
    D_INTERNAL_NODE_REP
};

// 1.2.2
// d_internal_node
//   struct: one parse-tree node.  Children are indices into the node pool
//   rather than pointers, so the whole tree is freed in one call and survives
//   reallocation of the pool.
struct dj_internal_node
{
    enum dj_internal_node_kind kind;
    int                       value;  // byte, class index, or group number
    int                       left;
    int                       right;
    int                       min;
    int                       max;    // negative denotes unbounded
};

// 1.3    Execution state
//------------------------------------------------------------------------------
// 1.3.1
// dj_internal_list
//   struct: one thread list.  Parallel arrays hold the program counter and the
//   subject offset at which each thread began matching.
struct dj_internal_list
{
    int*   pc;
    size_t count;
    size_t* starts;
};

// 1.3.2
// dj_regex
//   struct: a compiled pattern together with the scratch buffers its
//   simulation needs.  The buffers are sized at compile time and reused, so
//   matching performs no allocation.
struct dj_regex
{
    struct dj_internal_inst* program;
    size_t                  program_count;
    unsigned char*          classes;      // class_count blocks of 32 bytes
    size_t                  class_count;
    size_t                  group_count;
    size_t                  loop_count;
    struct dj_internal_list  list_a;
    struct dj_internal_list  list_b;
    unsigned*               seen;
    unsigned                generation;
    int*                    stack;
};

// 1.4    Parser state
//------------------------------------------------------------------------------
// 1.4.1
// dj_internal_parser
//   struct: cursor and output pools used while parsing a pattern.  `status`
//   latches the first error, after which every production returns immediately.
struct dj_internal_parser
{
    const char*             pattern;
    size_t                  position;
    size_t                  length;
    struct dj_internal_node* nodes;
    size_t                  node_count;
    size_t                  node_capacity;
    unsigned char*          classes;
    size_t                  class_count;
    size_t                  class_capacity;
    int                     group_count;
    enum dj_regex_status     status;
};

// 1.4.2
// dj_internal_emitter
//   struct: growable instruction buffer used while lowering the parse tree.
struct dj_internal_emitter
{
    struct dj_internal_inst* program;
    size_t                  count;
    size_t                  capacity;
    int                     loop_count;
    enum dj_regex_status     status;
};


//==============================================================================
// 2.  CHARACTER CLASSES
//==============================================================================


/*
d_internal_class_set
  Sets the bit for one byte in a 32-byte class bitmap.

Parameter(s):
  _bitmap: the bitmap to modify.
  _byte:   the byte whose bit to set.
Return:
  none.
*/
static
void d_internal_class_set(
    unsigned char* _bitmap,
    unsigned char  _byte
)
{
    _bitmap[_byte >> 3] |= (unsigned char)(1u << (_byte & 7u));

    return;
}


/*
d_internal_class_test
  Reports whether a byte is a member of a class bitmap.

Parameter(s):
  _bitmap: the bitmap to query.
  _byte:   the byte to test.
Return:
  A boolean value corresponding to either:
  - true, if the byte is a member, or
  - false, otherwise.
*/
static
bool d_internal_class_test(
    const unsigned char* _bitmap,
    unsigned char        _byte
)
{
    return ((_bitmap[_byte >> 3] & (unsigned char)(1u << (_byte & 7u))) != 0);
}


/*
d_internal_class_add_named
  Adds every member of a POSIX named class to a bitmap.
NOTE:
  The ctype predicates are passed an unsigned value, since passing a plain
  `char` is undefined for negative values.

Parameter(s):
  _bitmap: the bitmap to modify.
  _name:   the class name, without the enclosing `[:` and `:]`.
  _length: length of the name in bytes.
Return:
  A boolean value corresponding to either:
  - true, if the name is a recognized class, or
  - false, otherwise.
*/
static
bool d_internal_class_add_named(
    unsigned char* _bitmap,
    const char*    _name,
    size_t         _length
)
{
    // parameter validation first
    if ( (!_bitmap) ||
         (!_name)   ||
         (_length == 0) )
    {
        return false;
    }

    // resolve the name once, then apply the matching predicate to all bytes
    int kind = -1;

    static const char* const names[] =
    {
        "alpha", "digit", "alnum", "upper", "lower", "space",
        "blank", "punct", "print", "graph", "cntrl", "xdigit"
    };

    // find the index of the requested class
    for (size_t index = 0; index < (sizeof(names) / sizeof(names[0])); ++index)
    {
        if ( (strlen(names[index]) == _length) &&
             (strncmp(names[index], _name, _length) == 0) )
        {
            kind = (int)index;
            break;
        }
    }

    // reject an unknown class name
    if (kind < 0)
    {
        return false;
    }

    // test every byte value against the selected predicate
    for (int byte = 0; byte < 256; ++byte)
    {
        bool member = false;

        switch (kind)
        {
            case 0:  member = (isalpha(byte)  != 0); break;
            case 1:  member = (isdigit(byte)  != 0); break;
            case 2:  member = (isalnum(byte)  != 0); break;
            case 3:  member = (isupper(byte)  != 0); break;
            case 4:  member = (islower(byte)  != 0); break;
            case 5:  member = (isspace(byte)  != 0); break;
            case 6:  member = ((byte == ' ') || (byte == '\t')); break;
            case 7:  member = (ispunct(byte)  != 0); break;
            case 8:  member = (isprint(byte)  != 0); break;
            case 9:  member = (isgraph(byte)  != 0); break;
            case 10: member = (iscntrl(byte)  != 0); break;
            default: member = (isxdigit(byte) != 0); break;
        }

        // record the byte when the predicate accepts it
        if (member)
        {
            d_internal_class_set(_bitmap, (unsigned char)byte);
        }
    }

    return true;
}


//==============================================================================
// 3.  PARSER
//==============================================================================
// Recursive descent over POSIX ERE.  Precedence runs alternation, then
// concatenation, then repetition, then atoms.  An error latches in the parser
// status and every production unwinds without further consumption.


static
int d_internal_parse_alternation(
    struct dj_internal_parser* _parser
);


/*
d_internal_node_new
  Appends a node to the parse-tree pool.

Parameter(s):
  _parser: the parser whose pool to extend.
  _kind:   the kind of node to append.
Return:
  The index of the new node, or -1 on failure.
*/
static int
d_internal_node_new(
    struct dj_internal_parser*  _parser,
    enum dj_internal_node_kind  _kind
)
{
    // grow the pool when it is full
    if (_parser->node_count == _parser->node_capacity)
    {
        size_t capacity = 16;
        size_t bytes    = 0;

        // the pool doubles; abandon parsing where a size would wrap
        if ( ( (_parser->node_capacity != 0)                       &&
               (d_ckd_mul(&capacity,
                          _parser->node_capacity,
                          2u)) )                                   ||
             (d_ckd_mul(&bytes,
                        capacity,
                        sizeof(struct dj_internal_node))) )
        {
            _parser->status = D_REGEX_ERR_MEMORY;

            return -1;
        }

        struct dj_internal_node* grown = realloc(_parser->nodes, bytes);

        // abandon parsing when the pool cannot grow
        if (!grown)
        {
            _parser->status = D_REGEX_ERR_MEMORY;
            return -1;
        }

        _parser->nodes         = grown;
        _parser->node_capacity = capacity;
    }

    struct dj_internal_node* node = &_parser->nodes[_parser->node_count];

    node->kind  = _kind;
    node->value = 0;
    node->left  = -1;
    node->right = -1;
    node->min   = 0;
    node->max   = 0;

    return (int)(_parser->node_count++);
}


/*
d_internal_class_new
  Appends a zeroed 32-byte class bitmap to the parser's class pool.

Parameter(s):
  _parser: the parser whose class pool to extend.
Return:
  The index of the new bitmap, or -1 on failure.
*/
static
int d_internal_class_new(
    struct dj_internal_parser* _parser
)
{
    // grow the pool when it is full
    if (_parser->class_count == _parser->class_capacity)
    {
        size_t capacity = 4;
        size_t bytes    = 0;

        // the pool doubles; abandon parsing where a size would wrap
        if ( ( (_parser->class_capacity != 0)                      &&
               (d_ckd_mul(&capacity,
                          _parser->class_capacity,
                          2u)) )                                   ||
             (d_ckd_mul(&bytes,
                        capacity,
                        32u)) )
        {
            _parser->status = D_REGEX_ERR_MEMORY;

            return -1;
        }

        unsigned char* grown = realloc(_parser->classes, bytes);

        // abandon parsing when the pool cannot grow
        if (!grown)
        {
            _parser->status = D_REGEX_ERR_MEMORY;
            return -1;
        }

        _parser->classes        = grown;
        _parser->class_capacity = capacity;
    }

    memset(&_parser->classes[_parser->class_count * 32u], 0, 32u);

    return (int)(_parser->class_count++);
}


/*
d_internal_peek
  Returns the byte at the cursor without consuming it.

Parameter(s):
  _parser: the parser to inspect.
Return:
  The byte at the cursor, or -1 at end of pattern.
*/
static
int d_internal_peek(
    const struct dj_internal_parser* _parser
)
{
    // report end of input rather than reading past the pattern
    if (_parser->position >= _parser->length)
    {
        return -1;
    }

    return (int)(unsigned char)_parser->pattern[_parser->position];
}


/*
d_internal_parse_escape_byte
  Consumes the byte following a backslash and resolves it to a literal value.
NOTE:
  awk resolves the C escapes in regular-expression context, and a backslash
  before any other byte yields that byte.  `\b` is a backspace, not a word
  boundary, which POSIX ERE does not define.

Parameter(s):
  _parser: the parser positioned just after the backslash.
Return:
  The resolved byte value, or -1 when the pattern ends after the backslash.
*/
static
int d_internal_parse_escape_byte(
    struct dj_internal_parser* _parser
)
{
    const int next = d_internal_peek(_parser);

    // a trailing backslash is a syntax error
    if (next < 0)
    {
        _parser->status = D_REGEX_ERR_SYNTAX;
        return -1;
    }

    _parser->position++;

    // resolve the C escapes; any other byte stands for itself
    switch (next)
    {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case 'f': return '\f';
        case 'v': return '\v';
        case 'a': return '\a';
        case 'b': return '\b';
        default:  break;
    }

    // an octal escape takes up to three digits
    if ((next >= '0') && (next <= '7'))
    {
        int value = next - '0';

        for (int taken = 0; taken < 2; ++taken)
        {
            const int digit = d_internal_peek(_parser);

            // stop at the first byte that is not an octal digit
            if ((digit < '0') || (digit > '7'))
            {
                break;
            }

            value = (value * 8) + (digit - '0');
            _parser->position++;
        }

        return (value & 0xFF);
    }

    return next;
}


/*
d_internal_parse_bracket_sub
  Parses a bracketed sub-expression of a bracket expression - a named class
[:name:], a collating element [.c.] or an equivalence class [=c=] - when one
starts at the parser's position, and adds its members to the set.

Parameter(s):
  _parser: the parser positioned at a '[' inside a bracket expression.
  _bitmap: the set being built.
Return:
  1 when a sub-expression was read and added; 0 when none starts here, the
position unchanged; -1 on failure, with the parser's status set.
*/
static
int d_internal_parse_bracket_sub(
    struct dj_internal_parser* _parser,
    unsigned char*             _bitmap
)
{
    const int kind = (_parser->position + 1 < _parser->length)
                   ? (int)_parser->pattern[_parser->position + 1]
                   : -1;

    // only ':', '.' and '=' introduce a bracketed sub-expression
    if ((kind != ':') && (kind != '.') && (kind != '='))
    {
        return 0;
    }

    const char        terminator[3] = { (char)kind, ']', '\0' };
    const char* const found         =
        strstr(&_parser->pattern[_parser->position + 2], terminator);

    // an unterminated sub-expression is a literal bracket
    if (!found)
    {
        return 0;
    }

    const char* const body        = &_parser->pattern[_parser->position + 2];
    const size_t      body_length = (size_t)(found - body);

    // a named class expands; the other forms add one byte
    if (kind == ':')
    {
        // reject a class name the implementation lacks
        if (!d_internal_class_add_named(_bitmap, body, body_length))
        {
            _parser->status = D_REGEX_ERR_SYNTAX;

            return -1;
        }
    }
    else if (body_length >= 1)
    {
        d_internal_class_set(_bitmap, (unsigned char)body[0]);
    }

    _parser->position = (size_t)(found - _parser->pattern) + 2u;

    return 1;
}

/*
d_internal_parse_bracket_member
  Parses one member of a bracket expression - a byte, escaped or not, or a
range of them - and adds it to the set.

Parameter(s):
  _parser: the parser positioned at the member.
  _bitmap: the set being built.
Return:
  0 when the member was added; -1 on failure, with the parser's status set
where the member itself is malformed.
*/
static
int d_internal_parse_bracket_member(
    struct dj_internal_parser* _parser,
    unsigned char*             _bitmap
)
{
    int current = d_internal_peek(_parser);

    // resolve the member, honouring a backslash escape
    if (current == '\\')
    {
        _parser->position++;
        current = d_internal_parse_escape_byte(_parser);

        // propagate a malformed escape
        if (current < 0)
        {
            return -1;
        }
    }
    else
    {
        _parser->position++;
    }

    const int follow = d_internal_peek(_parser);

    // a hyphen that is not last introduces a range
    if ( (follow == '-')                                 &&
         (_parser->position + 1 < _parser->length)        &&
         (_parser->pattern[_parser->position + 1] != ']') )
    {
        _parser->position++;  // consume '-'

        int upper = d_internal_peek(_parser);

        // resolve the upper bound, honouring a backslash escape
        if (upper == '\\')
        {
            _parser->position++;
            upper = d_internal_parse_escape_byte(_parser);
        }
        else
        {
            _parser->position++;
        }

        // reject an inverted or malformed range
        if ((upper < 0) || (upper < current))
        {
            _parser->status = D_REGEX_ERR_SYNTAX;

            return -1;
        }

        // add every byte in the inclusive range
        for (int byte = current; byte <= upper; ++byte)
        {
            d_internal_class_set(_bitmap, (unsigned char)byte);
        }

        return 0;
    }

    d_internal_class_set(_bitmap, (unsigned char)current);

    return 0;
}

/*
d_internal_parse_bracket
  Parses a bracket expression and produces a CLASS node.

Parameter(s):
  _parser: the parser positioned at the opening bracket.
Return:
  The index of the new node, or -1 on failure.
*/
static
int d_internal_parse_bracket(
    struct dj_internal_parser* _parser
)
{
    _parser->position++;  // consume '['

    const int class_index = d_internal_class_new(_parser);

    // abandon parsing when the class pool could not grow
    if (class_index < 0)
    {
        return -1;
    }

    unsigned char* bitmap  = &_parser->classes[(size_t)class_index * 32u];
    bool           negated = false;

    // a leading caret negates the whole set
    if (d_internal_peek(_parser) == '^')
    {
        negated = true;
        _parser->position++;
    }

    bool first = true;

    // members are read until the closing bracket, which is literal if first
    while (true)
    {
        const int current = d_internal_peek(_parser);

        // an unterminated bracket expression is a syntax error
        if (current < 0)
        {
            _parser->status = D_REGEX_ERR_SYNTAX;

            return -1;
        }

        // the closing bracket ends the expression unless it is the first byte
        if ((current == ']') && (!first))
        {
            _parser->position++;
            break;
        }

        first = false;

        // a named class, collating element or equivalence class
        if (current == '[')
        {
            const int sub = d_internal_parse_bracket_sub(_parser, bitmap);

            // a sub-expression read, or a failure in one
            if (sub != 0)
            {
                if (sub < 0)
                {
                    return -1;
                }

                continue;
            }
        }

        // any other member: a byte or a range
        if (d_internal_parse_bracket_member(_parser, bitmap) < 0)
        {
            return -1;
        }
    }

    // invert the membership of a negated set
    if (negated)
    {
        for (size_t index = 0; index < 32u; ++index)
        {
            bitmap[index] = (unsigned char)(~bitmap[index]);
        }
    }

    const int node = d_internal_node_new(_parser, D_INTERNAL_NODE_CLASS);

    // propagate a pool-growth failure
    if (node < 0)
    {
        return -1;
    }

    _parser->nodes[node].value = class_index;

    return node;
}


/*
d_internal_parse_interval
  Attempts to read a `{n,m}` interval at the cursor.
NOTE:
  A brace that does not introduce a well-formed interval is not an error: the
  caller falls back to treating it as a literal byte, which is what awk
  implementations do in practice.

Parameter(s):
  _parser:   the parser positioned at the opening brace.
  _out_min:  receives the lower bound.
  _out_max:  receives the upper bound, or -1 when unbounded.
Return:
  A boolean value corresponding to either:
  - true, if a well-formed interval was read and consumed, or
  - false, otherwise, with the cursor unchanged.
*/
static
bool d_internal_parse_interval(
    struct dj_internal_parser* _parser,
    int*                       _out_min,
    int*                       _out_max
)
{
    const size_t saved = _parser->position;

    _parser->position++;  // consume '{'

    int  minimum = 0;
    int  maximum = -1;
    bool has_min = false;

    // read the lower bound
    while (true)
    {
        const int digit = d_internal_peek(_parser);

        // stop at the first byte that is not a digit
        if ((digit < '0') || (digit > '9'))
        {
            break;
        }

        minimum = (minimum * 10) + (digit - '0');
        has_min = true;
        _parser->position++;

        // reject a bound beyond the expansion limit
        if (minimum > D_REGEX_DUP_MAX)
        {
            _parser->position = saved;
            _parser->status   = D_REGEX_ERR_RANGE;
            return false;
        }
    }

    // an interval must state a lower bound
    if (!has_min)
    {
        _parser->position = saved;
        return false;
    }

    // a comma introduces an optional upper bound
    if (d_internal_peek(_parser) == ',')
    {
        _parser->position++;

        bool has_max = false;
        int  bound   = 0;

        // read the upper bound when one is present
        while (true)
        {
            const int digit = d_internal_peek(_parser);

            // stop at the first byte that is not a digit
            if ((digit < '0') || (digit > '9'))
            {
                break;
            }

            bound   = (bound * 10) + (digit - '0');
            has_max = true;
            _parser->position++;

            // reject a bound beyond the expansion limit
            if (bound > D_REGEX_DUP_MAX)
            {
                _parser->position = saved;
                _parser->status   = D_REGEX_ERR_RANGE;
                return false;
            }
        }

        maximum = has_max ? bound : -1;
    }
    else
    {
        maximum = minimum;
    }

    // the interval must close with a brace
    if (d_internal_peek(_parser) != '}')
    {
        _parser->position = saved;
        return false;
    }

    _parser->position++;

    // reject an inverted range
    if ((maximum >= 0) && (maximum < minimum))
    {
        _parser->status = D_REGEX_ERR_RANGE;
        return false;
    }

    *_out_min = minimum;
    *_out_max = maximum;

    return true;
}


/*
d_internal_parse_group
  Parses a parenthesised group, which contains a full alternation and
captures; groups are numbered by their opening parenthesis.

Parameter(s):
  _parser: the parser positioned at the opening parenthesis.
Return:
  The index of the new GROUP node, or -1 on failure.
*/
static
int d_internal_parse_group(
    struct dj_internal_parser* _parser
)
{
    // groups are numbered by opening parenthesis, so claim the number now
    if (_parser->group_count >= D_REGEX_GROUP_MAX)
    {
        _parser->status = D_REGEX_ERR_RANGE;

        return -1;
    }

    const int number = ++_parser->group_count;

    _parser->position++;

    const int inner = d_internal_parse_alternation(_parser);

    // propagate a failure from the nested expression
    if (inner < 0)
    {
        return -1;
    }

    // the group must close
    if (d_internal_peek(_parser) != ')')
    {
        _parser->status = D_REGEX_ERR_SYNTAX;

        return -1;
    }

    _parser->position++;

    const int node = d_internal_node_new(_parser, D_INTERNAL_NODE_GROUP);

    // propagate a pool-growth failure
    if (node < 0)
    {
        return -1;
    }

    _parser->nodes[node].left  = inner;
    _parser->nodes[node].value = number;

    return node;
}

/*
d_internal_parse_literal
  Parses a literal atom: a byte, or an escape standing for one.

Parameter(s):
  _parser: the parser positioned at the atom.
Return:
  The index of the new CHAR node, or -1 on failure.
*/
static
int d_internal_parse_literal(
    struct dj_internal_parser* _parser
)
{
    int literal = d_internal_peek(_parser);

    // a backslash introduces an escape; anything else stands for itself
    if (literal == '\\')
    {
        _parser->position++;
        literal = d_internal_parse_escape_byte(_parser);

        // propagate a malformed escape
        if (literal < 0)
        {
            return -1;
        }
    }
    else
    {
        _parser->position++;
    }

    const int node = d_internal_node_new(_parser, D_INTERNAL_NODE_CHAR);

    // propagate a pool-growth failure
    if (node < 0)
    {
        return -1;
    }

    _parser->nodes[node].value = literal;

    return node;
}

/*
d_internal_parse_atom
  Parses a single atom: a group, a bracket expression, an anchor, a wildcard,
  an escape, or a literal byte.

Parameter(s):
  _parser: the parser positioned at the atom.
Return:
  The index of the new node, or -1 on failure.
*/
static
int d_internal_parse_atom(
    struct dj_internal_parser* _parser
)
{
    const int current = d_internal_peek(_parser);

    // an atom cannot begin at end of pattern
    if (current < 0)
    {
        _parser->status = D_REGEX_ERR_SYNTAX;

        return -1;
    }

    switch (current)
    {
        // a parenthesised group contains a full alternation and captures
        case '(':
            return d_internal_parse_group(_parser);

        // a bracket expression produces a class
        case '[':
            return d_internal_parse_bracket(_parser);

        // a quantifier with nothing to repeat, or an unbalanced closing
        // parenthesis, is a syntax error
        case '*':
        case '+':
        case '?':
        case ')':
            _parser->status = D_REGEX_ERR_SYNTAX;

            return -1;

        // the remaining single-byte atoms map directly to node kinds
        case '.':
            _parser->position++;

            return d_internal_node_new(_parser, D_INTERNAL_NODE_ANY);

        case '^':
            _parser->position++;

            return d_internal_node_new(_parser, D_INTERNAL_NODE_BOL);

        case '$':
            _parser->position++;

            return d_internal_node_new(_parser, D_INTERNAL_NODE_EOL);

        default:
            return d_internal_parse_literal(_parser);
    }
}


/*
d_internal_parse_repeat
  Parses an atom together with any repetition operators applied to it.

Parameter(s):
  _parser: the parser positioned at the atom.
Return:
  The index of the resulting node, or -1 on failure.
*/
static
int d_internal_parse_repeat(
    struct dj_internal_parser* _parser
)
{
    int result = d_internal_parse_atom(_parser);

    // propagate a failure from the atom
    if (result < 0)
    {
        return -1;
    }

    // repetition operators associate left and may be stacked
    while (true)
    {
        const int current = d_internal_peek(_parser);
        int       minimum = 0;
        int       maximum = -1;

        // decide which repetition, if any, applies here
        if (current == '*')
        {
            _parser->position++;
            minimum = 0;
            maximum = -1;
        }
        else if (current == '+')
        {
            _parser->position++;
            minimum = 1;
            maximum = -1;
        }
        else if (current == '?')
        {
            _parser->position++;
            minimum = 0;
            maximum = 1;
        }
        else if (current == '{')
        {
            // a brace that is not an interval is a literal, handled elsewhere
            if (!d_internal_parse_interval(_parser, &minimum, &maximum))
            {
                break;
            }
        }
        else
        {
            break;
        }

        const int node = d_internal_node_new(_parser, D_INTERNAL_NODE_REP);

        // propagate a pool-growth failure
        if (node < 0)
        {
            return -1;
        }

        _parser->nodes[node].left = result;
        _parser->nodes[node].min  = minimum;
        _parser->nodes[node].max  = maximum;

        result = node;
    }

    return result;
}


/*
d_internal_parse_concat
  Parses a sequence of repeated atoms up to `|`, `)` or end of pattern.

Parameter(s):
  _parser: the parser positioned at the start of the sequence.
Return:
  The index of the resulting node, or -1 on failure.
*/
static
int d_internal_parse_concat(
    struct dj_internal_parser* _parser
)
{
    int result = -1;

    // accumulate pieces until a terminator is reached
    while (true)
    {
        const int current = d_internal_peek(_parser);

        // stop at end of pattern or at either terminator
        if ((current < 0) || (current == '|') || (current == ')'))
        {
            break;
        }

        const int piece = d_internal_parse_repeat(_parser);

        // propagate a failure from the piece
        if (piece < 0)
        {
            return -1;
        }

        // the first piece becomes the result; later pieces concatenate
        if (result < 0)
        {
            result = piece;
        }
        else
        {
            const int node = d_internal_node_new(_parser, D_INTERNAL_NODE_CAT);

            // propagate a pool-growth failure
            if (node < 0)
            {
                return -1;
            }

            _parser->nodes[node].left  = result;
            _parser->nodes[node].right = piece;
            result                     = node;
        }
    }

    // an empty branch matches the empty string
    if (result < 0)
    {
        result = d_internal_node_new(_parser, D_INTERNAL_NODE_EMPTY);
    }

    return result;
}


/*
d_internal_parse_alternation
  Parses one or more concatenations separated by `|`.

Parameter(s):
  _parser: the parser positioned at the start of the expression.
Return:
  The index of the resulting node, or -1 on failure.
*/
static
int d_internal_parse_alternation(
    struct dj_internal_parser* _parser
)
{
    int result = d_internal_parse_concat(_parser);

    // propagate a failure from the first branch
    if (result < 0)
    {
        return -1;
    }

    // fold each further branch into a left-leaning alternation
    while (d_internal_peek(_parser) == '|')
    {
        _parser->position++;

        const int branch = d_internal_parse_concat(_parser);

        // propagate a failure from the branch
        if (branch < 0)
        {
            return -1;
        }

        const int node = d_internal_node_new(_parser, D_INTERNAL_NODE_ALT);

        // propagate a pool-growth failure
        if (node < 0)
        {
            return -1;
        }

        _parser->nodes[node].left  = result;
        _parser->nodes[node].right = branch;
        result                     = node;
    }

    return result;
}


//==============================================================================
// 4.  COMPILER
//==============================================================================
// Thompson construction.  Each node lowers to a contiguous block of
// instructions whose entry point is the first address emitted, so control flow
// needs only forward patching of SPLIT and JMP targets.


/*
d_internal_emit
  Appends one instruction to the program under construction.

Parameter(s):
  _emitter: the emitter to extend.
  _op:      the opcode to append.
Return:
  The address of the new instruction, or -1 on failure.
*/
static int
d_internal_emit(
    struct dj_internal_emitter* _emitter,
    enum dj_internal_op         _op
)
{
    // grow the buffer when it is full
    if (_emitter->count == _emitter->capacity)
    {
        size_t capacity = 32;
        size_t bytes    = 0;

        // the buffer doubles; abandon compilation where a size would wrap
        if ( ( (_emitter->capacity != 0)                           &&
               (d_ckd_mul(&capacity,
                          _emitter->capacity,
                          2u)) )                                   ||
             (d_ckd_mul(&bytes,
                        capacity,
                        sizeof(struct dj_internal_inst))) )
        {
            _emitter->status = D_REGEX_ERR_MEMORY;

            return -1;
        }

        struct dj_internal_inst* grown = realloc(_emitter->program, bytes);

        // abandon compilation when the buffer cannot grow
        if (!grown)
        {
            _emitter->status = D_REGEX_ERR_MEMORY;
            return -1;
        }

        _emitter->program  = grown;
        _emitter->capacity = capacity;
    }

    struct dj_internal_inst* inst = &_emitter->program[_emitter->count];

    inst->op    = _op;
    inst->value = 0;
    inst->x     = 0;
    inst->y     = 0;

    return (int)(_emitter->count++);
}


static
bool d_internal_compile_node(
    struct dj_internal_emitter*    _emitter,
    const struct dj_internal_node* _nodes,
    int                            _index
);


/*
d_internal_compile_copies
  Lowers `_count` copies of a repetition's body, one after another.

Parameter(s):
  _emitter: the emitter to extend.
  _nodes:   the parse-tree pool.
  _body:    the index of the body.
  _count:   how many copies; none when not positive.
Return:
  A boolean value corresponding to either:
  - true, if every copy lowered, or
  - false, otherwise.
*/
static
bool d_internal_compile_copies(
    struct dj_internal_emitter*    _emitter,
    const struct dj_internal_node* _nodes,
    int                            _body,
    int                            _count
)
{
    for (int copy = 0; copy < _count; ++copy)
    {
        // abandon compilation when a copy fails to lower
        if (!d_internal_compile_node(_emitter, _nodes, _body))
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_compile_star
  Lowers the loop of an unbounded repetition with a zero lower bound: a loop
around an optional body.

Parameter(s):
  _emitter: the emitter to extend.
  _nodes:   the parse-tree pool.
  _body:    the index of the body.
  _loop:    the loop's counter.
Return:
  A boolean value corresponding to either:
  - true, if the loop lowered, or
  - false, otherwise.
*/
static
bool d_internal_compile_star(
    struct dj_internal_emitter*    _emitter,
    const struct dj_internal_node* _nodes,
    int                            _body,
    int                            _loop
)
{
    const int reset = d_internal_emit(_emitter, D_INTERNAL_OP_LOOP);
    const int split = d_internal_emit(_emitter, D_INTERNAL_OP_SPLIT);
    const int mark  = d_internal_emit(_emitter, D_INTERNAL_OP_MARK);

    // propagate a buffer-growth failure
    if ((reset < 0) || (split < 0) || (mark < 0))
    {
        return false;
    }

    _emitter->program[reset].value = _loop;
    _emitter->program[reset].x     = 0;
    _emitter->program[split].x     = split + 1;
    _emitter->program[mark].value  = _loop;

    // abandon compilation when the body fails to lower
    if (!d_internal_compile_node(_emitter, _nodes, _body))
    {
        return false;
    }

    const int check = d_internal_emit(_emitter, D_INTERNAL_OP_PROGRESS);
    const int jump  = d_internal_emit(_emitter, D_INTERNAL_OP_JMP);

    // propagate a buffer-growth failure
    if ((check < 0) || (jump < 0))
    {
        return false;
    }

    _emitter->program[check].value = _loop;
    _emitter->program[jump].x      = split;
    _emitter->program[split].y     = (int)_emitter->count;
    _emitter->program[check].x     = (int)_emitter->count;

    return true;
}

/*
d_internal_compile_plus
  Lowers the loop of an unbounded repetition with a lower bound of at least
one: the body, then a branch back to it.

Parameter(s):
  _emitter: the emitter to extend.
  _nodes:   the parse-tree pool.
  _body:    the index of the body.
  _loop:    the loop's counter.
Return:
  A boolean value corresponding to either:
  - true, if the loop lowered, or
  - false, otherwise.
*/
static
bool d_internal_compile_plus(
    struct dj_internal_emitter*    _emitter,
    const struct dj_internal_node* _nodes,
    int                            _body,
    int                            _loop
)
{
    const int reset = d_internal_emit(_emitter, D_INTERNAL_OP_LOOP);
    const int entry = (int)_emitter->count;
    const int mark  = d_internal_emit(_emitter, D_INTERNAL_OP_MARK);

    // propagate a buffer-growth failure
    if ((reset < 0) || (mark < 0))
    {
        return false;
    }

    _emitter->program[reset].value = _loop;
    _emitter->program[reset].x     = 0;
    _emitter->program[mark].value  = _loop;

    // abandon compilation when the body fails to lower
    if (!d_internal_compile_node(_emitter, _nodes, _body))
    {
        return false;
    }

    const int check = d_internal_emit(_emitter, D_INTERNAL_OP_PROGRESS);
    const int split = d_internal_emit(_emitter, D_INTERNAL_OP_SPLIT);

    // propagate a buffer-growth failure
    if ((check < 0) || (split < 0))
    {
        return false;
    }

    // PROGRESS precedes the exit branch deliberately.  Deferring the exit
    // first would give it an undo mark taken after this iteration's writes,
    // so an empty final iteration would survive the backtrack and be
    // reported in place of the last one that consumed.
    _emitter->program[check].value = _loop;
    _emitter->program[check].x     = (int)_emitter->count;
    _emitter->program[split].x     = entry;
    _emitter->program[split].y     = (int)_emitter->count;

    return true;
}

/*
d_internal_compile_optional_copy
  Lowers one optional copy of a bounded repetition's body behind its own
split, and reports the split and the progress check, both of which leave to
the shared exit once its address is known.

Parameter(s):
  _emitter:   the emitter to extend.
  _nodes:     the parse-tree pool.
  _body:      the index of the body.
  _bound:     the repetition's counter.
  _out_split: receives the copy's split.
  _out_check: receives the copy's progress check.
Return:
  A boolean value corresponding to either:
  - true, if the copy lowered, or
  - false, otherwise.
*/
static
bool d_internal_compile_optional_copy(
    struct dj_internal_emitter*    _emitter,
    const struct dj_internal_node* _nodes,
    int                            _body,
    int                            _bound,
    int*                           _out_split,
    int*                           _out_check
)
{
    const int split = d_internal_emit(_emitter, D_INTERNAL_OP_SPLIT);

    // propagate a buffer-growth failure
    if (split < 0)
    {
        return false;
    }

    const int mark = d_internal_emit(_emitter, D_INTERNAL_OP_MARK);

    // propagate a buffer-growth failure
    if (mark < 0)
    {
        return false;
    }

    _emitter->program[split].x    = split + 1;
    _emitter->program[mark].value = _bound;

    // abandon compilation when the body fails to lower
    if (!d_internal_compile_node(_emitter, _nodes, _body))
    {
        return false;
    }

    const int check = d_internal_emit(_emitter, D_INTERNAL_OP_PROGRESS);

    // propagate a buffer-growth failure
    if (check < 0)
    {
        return false;
    }

    _emitter->program[check].value = _bound;
    *_out_split                    = split;
    *_out_check                    = check;

    return true;
}

/*
d_internal_compile_optional
  Lowers the optional copies of a bounded repetition, after its mandatory
ones.
NOTE:
  The optional copies follow the same rule as an unbounded repetition: an
  empty copy is kept only when no copy consumed.  The mandatory copies seed
  the count, so /(b{0,0}[^c]?){1,2}/ does not let an empty second copy
  overwrite what the first one captured.

Parameter(s):
  _emitter:  the emitter to extend.
  _nodes:    the parse-tree pool.
  _body:     the index of the body.
  _min:      the mandatory copies already lowered.
  _optional: how many optional copies; at least one.
Return:
  A boolean value corresponding to either:
  - true, if every copy lowered, or
  - false, otherwise.
*/
static
bool d_internal_compile_optional(
    struct dj_internal_emitter*    _emitter,
    const struct dj_internal_node* _nodes,
    int                            _body,
    int                            _min,
    int                            _optional
)
{
    const int bound = _emitter->loop_count++;
    const int reset = d_internal_emit(_emitter, D_INTERNAL_OP_LOOP);
    int*      exits = NULL;
    size_t    bytes = 0;

    // propagate a buffer-growth failure
    if (reset < 0)
    {
        return false;
    }

    _emitter->program[reset].value = bound;
    _emitter->program[reset].x     = _min;

    // two patches a copy; abandon compilation where the size would wrap
    if (d_ckd_mul(&bytes,
                  (size_t)_optional,
                  2u * sizeof(*exits)))
    {
        _emitter->status = D_REGEX_ERR_MEMORY;

        return false;
    }

    exits = malloc(bytes);

    // abandon compilation when the patch list cannot be held
    if (!exits)
    {
        _emitter->status = D_REGEX_ERR_MEMORY;

        return false;
    }

    // emit each optional copy behind its own split
    for (int copy = 0; copy < _optional; ++copy)
    {
        // abandon compilation when a copy fails to lower
        if (!d_internal_compile_optional_copy(_emitter,
                                              _nodes,
                                              _body,
                                              bound,
                                              &exits[copy * 2],
                                              &exits[(copy * 2) + 1]))
        {
            free(exits);

            return false;
        }
    }

    // every optional copy leaves to the instruction after the last one, by its
    // split when the copy is skipped and by its check when the copy was empty
    for (int copy = 0; copy < _optional; ++copy)
    {
        const int split = exits[copy * 2];
        const int check = exits[(copy * 2) + 1];

        _emitter->program[split].y = (int)_emitter->count;
        _emitter->program[check].x = (int)_emitter->count;
    }

    free(exits);

    return true;
}

/*
d_internal_compile_repeat
  Lowers a repetition node by replicating its body.
NOTE:
  An unbounded repetition emits `min - 1` copies of the body followed by a
  loop, and a bounded one emits `min` copies followed by `max - min` optional
  copies whose exits all patch to the same address.

Parameter(s):
  _emitter: the emitter to extend.
  _nodes:   the parse-tree pool.
  _index:   the index of the repetition node.
Return:
  A boolean value corresponding to either:
  - true, if the body lowered successfully, or
  - false, otherwise.
*/
static
bool d_internal_compile_repeat(
    struct dj_internal_emitter*    _emitter,
    const struct dj_internal_node* _nodes,
    int                            _index
)
{
    const struct dj_internal_node* node = &_nodes[_index];
    const int                     body = node->left;
    const int                     min  = node->min;
    const int                     max  = node->max;

    // an unbounded repetition: the copies that precede its loop, then the loop
    if (max < 0)
    {
        // abandon compilation when a copy fails to lower
        if (!d_internal_compile_copies(_emitter, _nodes, body, min - 1))
        {
            return false;
        }

        const int loop = _emitter->loop_count++;

        return (min == 0)
                   ? d_internal_compile_star(_emitter, _nodes, body, loop)
                   : d_internal_compile_plus(_emitter, _nodes, body, loop);
    }

    // a bounded one: the mandatory copies, then the optional ones
    if (!d_internal_compile_copies(_emitter, _nodes, body, min))
    {
        return false;
    }

    return (max > min)
               ? d_internal_compile_optional(_emitter,
                                             _nodes,
                                             body,
                                             min,
                                             max - min)
               : true;
}


/*
d_internal_compile_leaf
  Lowers a leaf - a byte, a class, the wildcard or an anchor - to its one
instruction.

Parameter(s):
  _emitter: the emitter to extend.
  _node:    the leaf.
Return:
  A boolean value corresponding to either:
  - true, if the leaf lowered successfully, or
  - false, otherwise.
*/
static
bool d_internal_compile_leaf(
    struct dj_internal_emitter*    _emitter,
    const struct dj_internal_node* _node
)
{
    enum dj_internal_op op = D_INTERNAL_OP_ANY;

    // map the leaf kind onto its opcode
    if (_node->kind == D_INTERNAL_NODE_CHAR)
    {
        op = D_INTERNAL_OP_CHAR;
    }
    else if (_node->kind == D_INTERNAL_NODE_CLASS)
    {
        op = D_INTERNAL_OP_CLASS;
    }
    else if (_node->kind == D_INTERNAL_NODE_BOL)
    {
        op = D_INTERNAL_OP_BOL;
    }
    else if (_node->kind == D_INTERNAL_NODE_EOL)
    {
        op = D_INTERNAL_OP_EOL;
    }

    const int address = d_internal_emit(_emitter, op);

    // propagate a buffer-growth failure
    if (address < 0)
    {
        return false;
    }

    _emitter->program[address].value = _node->value;

    return true;
}

/*
d_internal_compile_group
  Lowers a capturing group: its body between the two SAVE instructions that
record where the group starts and ends.

Parameter(s):
  _emitter: the emitter to extend.
  _nodes:   the parse-tree pool.
  _node:    the group.
Return:
  A boolean value corresponding to either:
  - true, if the group lowered successfully, or
  - false, otherwise.
*/
static
bool d_internal_compile_group(
    struct dj_internal_emitter*    _emitter,
    const struct dj_internal_node* _nodes,
    const struct dj_internal_node* _node
)
{
    const int open = d_internal_emit(_emitter, D_INTERNAL_OP_SAVE);

    // propagate a buffer-growth failure
    if (open < 0)
    {
        return false;
    }

    _emitter->program[open].value = _node->value * 2;

    // abandon compilation when the body fails to lower
    if (!d_internal_compile_node(_emitter, _nodes, _node->left))
    {
        return false;
    }

    const int close = d_internal_emit(_emitter, D_INTERNAL_OP_SAVE);

    // propagate a buffer-growth failure
    if (close < 0)
    {
        return false;
    }

    _emitter->program[close].value = (_node->value * 2) + 1;

    return true;
}

/*
d_internal_compile_alternation
  Lowers an alternation: a split to either branch, the first jumping past
the second.

Parameter(s):
  _emitter: the emitter to extend.
  _nodes:   the parse-tree pool.
  _node:    the alternation.
Return:
  A boolean value corresponding to either:
  - true, if both branches lowered successfully, or
  - false, otherwise.
*/
static
bool d_internal_compile_alternation(
    struct dj_internal_emitter*    _emitter,
    const struct dj_internal_node* _nodes,
    const struct dj_internal_node* _node
)
{
    const int split = d_internal_emit(_emitter, D_INTERNAL_OP_SPLIT);

    // propagate a buffer-growth failure
    if (split < 0)
    {
        return false;
    }

    _emitter->program[split].x = split + 1;

    // abandon compilation when the first branch fails to lower
    if (!d_internal_compile_node(_emitter, _nodes, _node->left))
    {
        return false;
    }

    const int jump = d_internal_emit(_emitter, D_INTERNAL_OP_JMP);

    // propagate a buffer-growth failure
    if (jump < 0)
    {
        return false;
    }

    _emitter->program[split].y = (int)_emitter->count;

    // abandon compilation when the second branch fails to lower
    if (!d_internal_compile_node(_emitter, _nodes, _node->right))
    {
        return false;
    }

    _emitter->program[jump].x = (int)_emitter->count;

    return true;
}

/*
d_internal_compile_node
  Lowers one parse-tree node and its children into the program.

Parameter(s):
  _emitter: the emitter to extend.
  _nodes:   the parse-tree pool.
  _index:   the index of the node to lower.
Return:
  A boolean value corresponding to either:
  - true, if the node lowered successfully, or
  - false, otherwise.
*/
static
bool d_internal_compile_node(
    struct dj_internal_emitter*    _emitter,
    const struct dj_internal_node* _nodes,
    int                            _index
)
{
    const struct dj_internal_node* node = &_nodes[_index];

    switch (node->kind)
    {
        case D_INTERNAL_NODE_EMPTY:
            return true;

        case D_INTERNAL_NODE_CHAR:
        case D_INTERNAL_NODE_CLASS:
        case D_INTERNAL_NODE_ANY:
        case D_INTERNAL_NODE_BOL:
        case D_INTERNAL_NODE_EOL:
            return d_internal_compile_leaf(_emitter, node);

        case D_INTERNAL_NODE_CAT:
            // abandon compilation when either side fails to lower
            return ( (d_internal_compile_node(_emitter, _nodes, node->left)) &&
                     (d_internal_compile_node(_emitter, _nodes, node->right)) );

        case D_INTERNAL_NODE_GROUP:
            return d_internal_compile_group(_emitter, _nodes, node);

        case D_INTERNAL_NODE_ALT:
            return d_internal_compile_alternation(_emitter, _nodes, node);

        default:
            return d_internal_compile_repeat(_emitter, _nodes, _index);
    }
}


//==============================================================================
// 5.  SIMULATION
//==============================================================================
// A thread is a program counter paired with the offset at which it began.
// Threads are deduplicated per instruction per input position; because carried
// threads are added before the freshly seeded one, the surviving duplicate is
// always the leftmost, which is exactly the POSIX tie-break.


/*
d_internal_add_thread
  Adds a thread and its epsilon closure to a list.
NOTE:
  The closure is walked with an explicit stack rather than by recursion, since
  a pathological pattern could otherwise exhaust the call stack.

Parameter(s):
  _regex:      the pattern being run; supplies the stack and seen buffers.
  _list:       the list to extend.
  _pc:         the program counter at which to enter.
  _start:      the offset at which the thread began matching.
  _position:   the current offset in the subject.
  _length:     the length of the subject.
  _generation: the stamp identifying this list at this position.
Return:
  none.
*/
static
void d_internal_add_thread(
    struct dj_regex*         _regex,
    struct dj_internal_list* _list,
    int                      _pc,
    size_t                   _start,
    size_t                   _position,
    size_t                   _length,
    unsigned                 _generation
)
{
    size_t top = 0;

    _regex->stack[top++] = _pc;

    // walk the closure until no pending entry remains
    while (top > 0)
    {
        const int address = _regex->stack[--top];

        // a program counter already reached here needs no second thread
        if (_regex->seen[address] == _generation)
        {
            continue;
        }

        _regex->seen[address] = _generation;

        const struct dj_internal_inst* inst = &_regex->program[address];

        switch (inst->op)
        {
            case D_INTERNAL_OP_JMP:
            {
                _regex->stack[top++] = inst->x;
                break;
            }

            case D_INTERNAL_OP_SAVE:
            case D_INTERNAL_OP_LOOP:
            case D_INTERNAL_OP_MARK:
            {
                // group and loop bookkeeping belongs to the capture pass
                _regex->stack[top++] = address + 1;
                break;
            }

            case D_INTERNAL_OP_PROGRESS:
            {
                // the simulation has no per-thread offset to test, so both
                // successors are admitted.  That is sound: an empty iteration
                // changes no language, and a thread reaches a program counter
                // at most once per input position, so it still cannot spin.
                _regex->stack[top++] = inst->x;
                _regex->stack[top++] = address + 1;
                break;
            }

            case D_INTERNAL_OP_SPLIT:
            {
                _regex->stack[top++] = inst->y;
                _regex->stack[top++] = inst->x;
                break;
            }

            case D_INTERNAL_OP_BOL:
            {
                // the anchor holds only at the start of the subject
                if (_position == 0)
                {
                    _regex->stack[top++] = address + 1;
                }

                break;
            }

            case D_INTERNAL_OP_EOL:
            {
                // the anchor holds only at the end of the subject
                if (_position == _length)
                {
                    _regex->stack[top++] = address + 1;
                }

                break;
            }

            default:
            {
                _list->pc[_list->count]     = address;
                _list->starts[_list->count] = _start;
                _list->count++;
                break;
            }
        }
    }

    return;
}


/*
d_internal_run_accept
  Records every thread of a list that has reached the accepting instruction,
keeping the leftmost start, and the longest extent within it.

Parameter(s):
  _regex:      the compiled pattern.
  _list:       the threads at this position.
  _position:   the input position the threads have reached.
  _found:      whether a match is held; set when one is recorded.
  _best_start: the held match's start; updated.
  _best_end:   the held match's end; updated.
Return:
  None.
*/
static
void d_internal_run_accept(
    const struct dj_regex*         _regex,
    const struct dj_internal_list* _list,
    size_t                         _position,
    bool*                          _found,
    size_t*                        _best_start,
    size_t*                        _best_end
)
{
    for (size_t index = 0; index < _list->count; ++index)
    {
        const struct dj_internal_inst* inst =
            &_regex->program[_list->pc[index]];

        // only the accepting instruction reports a match
        if (inst->op != D_INTERNAL_OP_MATCH)
        {
            continue;
        }

        const size_t start = _list->starts[index];

        // keep the leftmost start, and the longest extent within it
        if ( (!*_found)                                            ||
             (start < *_best_start)                                ||
             ((start == *_best_start) && (_position > *_best_end)) )
        {
            *_found      = true;
            *_best_start = start;
            *_best_end   = _position;
        }
    }

    return;
}

/*
d_internal_run_step
  Steps every thread of a list that consumes the byte at a position into the
next list, which it empties first.

Parameter(s):
  _regex:    the compiled pattern.
  _current:  the threads at this position.
  _next:     receives the threads at the next.
  _byte:     the byte at this position.
  _position: the position.
  _length:   the length of the subject.
Return:
  None.
*/
static
void d_internal_run_step(
    struct dj_regex*               _regex,
    const struct dj_internal_list* _current,
    struct dj_internal_list*       _next,
    unsigned char                  _byte,
    size_t                         _position,
    size_t                         _length
)
{
    _regex->generation++;
    _next->count = 0;

    for (size_t index = 0; index < _current->count; ++index)
    {
        const int                     address = _current->pc[index];
        const struct dj_internal_inst* inst    = &_regex->program[address];
        bool                          consume = false;

        // decide whether this instruction accepts the byte
        if (inst->op == D_INTERNAL_OP_CHAR)
        {
            consume = (inst->value == (int)_byte);
        }
        else if (inst->op == D_INTERNAL_OP_ANY)
        {
            consume = true;
        }
        else if (inst->op == D_INTERNAL_OP_CLASS)
        {
            const unsigned char* bitmap =
                &_regex->classes[(size_t)inst->value * 32u];

            consume = d_internal_class_test(bitmap, _byte);
        }

        // carry the thread forward when the byte is accepted
        if (consume)
        {
            d_internal_add_thread(_regex,
                                  _next,
                                  address + 1,
                                  _current->starts[index],
                                  _position + 1,
                                  _length,
                                  _regex->generation);
        }
    }

    return;
}

/*
d_internal_run
  Executes the program over a subject and reports the best match.

Parameter(s):
  _regex:      the compiled pattern.
  _text:       the subject bytes.
  _length:     the length of the subject.
  _from:       the offset at which to begin.
  _anchored:   when true, only a match beginning at `_from` is accepted.
  _out_start:  receives the offset of the match.
  _out_length: receives the length of the match.
Return:
  A boolean value corresponding to either:
  - true, if a match was found, or
  - false, otherwise.
*/
static
bool d_internal_run(
    struct dj_regex* _regex,
    const char*      _text,
    size_t           _length,
    size_t           _from,
    bool             _anchored,
    size_t*          _out_start,
    size_t*          _out_length
)
{
    struct dj_internal_list* current = &_regex->list_a;
    struct dj_internal_list* next    = &_regex->list_b;

    current->count = 0;
    next->count    = 0;

    bool   found      = false;
    size_t best_start = 0;
    size_t best_end   = 0;

    _regex->generation++;

    d_internal_add_thread(_regex,
                          current,
                          0,
                          _from,
                          _from,
                          _length,
                          _regex->generation);

    // advance one input position per iteration, including the end position
    for (size_t position = _from; ; ++position)
    {
        // record any thread that has reached the accepting instruction
        d_internal_run_accept(_regex,
                              current,
                              position,
                              &found,
                              &best_start,
                              &best_end);

        // the end position admits no further transition
        if (position == _length)
        {
            break;
        }

        // step every thread that consumes the byte at this position
        d_internal_run_step(_regex,
                            current,
                            next,
                            (unsigned char)_text[position],
                            position,
                            _length);

        struct dj_internal_list* const swap = current;

        current = next;
        next    = swap;

        // seed a later start only while no match has been established
        if ((!found) && (!_anchored))
        {
            d_internal_add_thread(_regex,
                                  current,
                                  0,
                                  position + 1,
                                  position + 1,
                                  _length,
                                  _regex->generation);
        }

        // nothing further can improve on an established match, and an
        // anchored run cannot recover once every thread has died
        if ( (current->count == 0) &&
             ((found) || (_anchored)) )
        {
            break;
        }
    }

    // report the extent only when a match was established
    if (!found)
    {
        return false;
    }

    *_out_start  = best_start;
    *_out_length = best_end - best_start;

    return true;
}


//==============================================================================
// 6.  CAPTURE PASS
//==============================================================================
// The second execution path.  Its input is a window already fixed by the
// simulation, so it never decides where the match is - only how the groups
// divide it.  Anchoring both ends discards most candidate parses before they
// are built, and the step budget bounds what remains.


// 6.1    Backtracking state
//------------------------------------------------------------------------------
// 6.1.1
// dj_internal_undo
//   struct: one entry of the trail that restores a mark on backtracking.  A
//   mark is either a capture slot or a loop back-edge guard; the two share one
//   array so that a single trail unwinds both.
struct dj_internal_undo
{
    size_t index;
    size_t value;
};

// 6.1.2
// dj_internal_choice
//   struct: one deferred alternative, together with the trail mark to unwind
//   to before it is taken.
struct dj_internal_choice
{
    int    pc;
    size_t position;
    size_t undo_mark;
};

// 6.1.3
// dj_internal_capture_state
//   struct: the buffers one capturing pass needs.  `marks` holds the capture
//   slots followed by an entry offset and an iteration count per repetition.
//   All are allocated per call rather than held on the pattern, since
//   capturing is the slow path and the fast path must not carry its cost.
struct dj_internal_capture_state
{
    size_t*                   marks;
    size_t*                   best;
    struct dj_internal_undo*   undo;
    size_t                    undo_count;
    size_t                    undo_capacity;
    struct dj_internal_choice* choice;
    size_t                    choice_count;
    size_t                    choice_capacity;
};


// 6.2    The pass's steps
//------------------------------------------------------------------------------
// dj_internal_step
//   enum: what one instruction of the capture pass did: the parse goes on,
// the branch failed, a parse completed, or the pass ran out of room.
enum dj_internal_step
{
    D_INTERNAL_STEP_ON     = 0,
    D_INTERNAL_STEP_FAIL   = 1,
    D_INTERNAL_STEP_DONE   = 2,
    D_INTERNAL_STEP_BUDGET = 3
};

/*
d_internal_capture_trail
  Writes a mark, first recording the value it displaces on the trail, so a
backtrack can restore it.

Parameter(s):
  _state: the per-call backtracking buffers.
  _index: the mark to write.
  _value: its new value.
Return:
  A boolean value corresponding to either:
  - true, if the mark was written, or
  - false, if the trail is full.
*/
static
bool d_internal_capture_trail(
    struct dj_internal_capture_state* _state,
    size_t                           _index,
    size_t                           _value
)
{
    // the trail must be able to hold the displaced value
    if (_state->undo_count >= _state->undo_capacity)
    {
        return false;
    }

    _state->undo[_state->undo_count].index = _index;
    _state->undo[_state->undo_count].value = _state->marks[_index];
    _state->undo_count++;

    _state->marks[_index] = _value;

    return true;
}

/*
d_internal_capture_consume
  Steps a consuming instruction - a byte, the wildcard or a class - over
the byte at the position, within the window.

Parameter(s):
  _regex:    the compiled pattern.
  _inst:     the instruction.
  _text:     the subject bytes.
  _end:      the offset at which the window ends.
  _pc:       the program counter; advanced on success.
  _position: the input position; advanced on success.
Return:
  D_INTERNAL_STEP_ON when the byte was consumed, D_INTERNAL_STEP_FAIL
otherwise.
*/
static enum dj_internal_step
d_internal_capture_consume(
    const struct dj_regex*         _regex,
    const struct dj_internal_inst* _inst,
    const char*                    _text,
    size_t                         _end,
    int*                           _pc,
    size_t*                        _position
)
{
    // nothing is left in the window to consume
    if (*_position >= _end)
    {
        return D_INTERNAL_STEP_FAIL;
    }

    const unsigned char byte = (unsigned char)_text[*_position];
    bool                fail = false;

    // decide whether this instruction accepts the byte
    if (_inst->op == D_INTERNAL_OP_CHAR)
    {
        fail = ((int)byte != _inst->value);
    }
    else if (_inst->op == D_INTERNAL_OP_CLASS)
    {
        const unsigned char* bitmap =
            &_regex->classes[(size_t)_inst->value * 32u];

        fail = (!d_internal_class_test(bitmap, byte));
    }

    // consume the byte when it was accepted
    if (fail)
    {
        return D_INTERNAL_STEP_FAIL;
    }

    (*_pc)++;
    (*_position)++;

    return D_INTERNAL_STEP_ON;
}

/*
d_internal_capture_progress
  Steps a repetition's PROGRESS check: an iteration that consumed takes the
back-edge and counts; an empty iteration is kept only when it is the only
one.
NOTE:
  GNU regex reports group 1 of /(a{0,1})*b/ against "b" as an empty match at
  offset zero, but reports (a?)* against "aa" as (1,1) - the trailing
  empty iteration is discarded once an earlier one consumed.  Diverting keeps
  the writes; failing unwinds them.

Parameter(s):
  _state:    the per-call backtracking buffers.
  _inst:     the PROGRESS instruction.
  _pairs:    the marks the capture slots take, before the loops' own.
  _pc:       the program counter.
  _position: the input position.
Return:
  D_INTERNAL_STEP_ON, D_INTERNAL_STEP_FAIL, or D_INTERNAL_STEP_BUDGET when
the trail is full.
*/
static enum dj_internal_step
d_internal_capture_progress(
    struct dj_internal_capture_state* _state,
    const struct dj_internal_inst*    _inst,
    size_t                           _pairs,
    int*                             _pc,
    size_t                           _position
)
{
    const size_t base    = _pairs + ((size_t)_inst->value * 2u);
    const size_t entered = _state->marks[base];
    const size_t taken   = _state->marks[base + 1u];

    // an iteration that consumed takes the back-edge, and counts
    if (_position != entered)
    {
        // the trail must be able to hold the displaced value
        if (!d_internal_capture_trail(_state, base + 1u, taken + 1u))
        {
            return D_INTERNAL_STEP_BUDGET;
        }

        (*_pc)++;

        return D_INTERNAL_STEP_ON;
    }

    // an empty iteration is kept only when it is the only one
    if (taken == 0u)
    {
        *_pc = _inst->x;

        return D_INTERNAL_STEP_ON;
    }

    return D_INTERNAL_STEP_FAIL;
}

/*
d_internal_capture_step
  Steps one instruction of the capture pass.

Parameter(s):
  _regex:    the compiled pattern.
  _state:    the per-call backtracking buffers.
  _pairs:    the marks the capture slots take, before the loops' own.
  _text:     the subject bytes.
  _length:   the length of the subject.
  _end:      the offset at which the window ends.
  _pc:       the program counter.
  _position: the input position.
Return:
  What the instruction did (dj_internal_step).
*/
static enum dj_internal_step
d_internal_capture_step(
    struct dj_regex*                  _regex,
    struct dj_internal_capture_state* _state,
    size_t                           _pairs,
    const char*                      _text,
    size_t                           _length,
    size_t                           _end,
    int*                             _pc,
    size_t*                          _position
)
{
    const struct dj_internal_inst* inst = &_regex->program[*_pc];

    switch (inst->op)
    {
        case D_INTERNAL_OP_CHAR:
        case D_INTERNAL_OP_ANY:
        case D_INTERNAL_OP_CLASS:
            return d_internal_capture_consume(_regex,
                                              inst,
                                              _text,
                                              _end,
                                              _pc,
                                              _position);

        // the anchors hold only at the subject's start and end
        case D_INTERNAL_OP_BOL:
        case D_INTERNAL_OP_EOL:
            if (*_position != ((inst->op == D_INTERNAL_OP_BOL) ? 0u : _length))
            {
                return D_INTERNAL_STEP_FAIL;
            }

            (*_pc)++;

            return D_INTERNAL_STEP_ON;

        // a group's edge, and a repetition's bookkeeping: LOOP clears the
        // iteration count, MARK records where this iteration began
        case D_INTERNAL_OP_SAVE:
        case D_INTERNAL_OP_LOOP:
        case D_INTERNAL_OP_MARK:
        {
            const size_t index =
                (inst->op == D_INTERNAL_OP_SAVE)
                    ? (size_t)inst->value
                    : _pairs + ((size_t)inst->value * 2u) +
                      ((inst->op == D_INTERNAL_OP_LOOP) ? 1u : 0u);
            const size_t value = (inst->op == D_INTERNAL_OP_LOOP)
                               ? (size_t)inst->x
                               : *_position;

            // the trail must be able to hold the displaced value
            if (!d_internal_capture_trail(_state, index, value))
            {
                return D_INTERNAL_STEP_BUDGET;
            }

            (*_pc)++;

            return D_INTERNAL_STEP_ON;
        }

        case D_INTERNAL_OP_PROGRESS:
            return d_internal_capture_progress(_state,
                                               inst,
                                               _pairs,
                                               _pc,
                                               *_position);

        case D_INTERNAL_OP_JMP:
            *_pc = inst->x;

            return D_INTERNAL_STEP_ON;

        case D_INTERNAL_OP_SPLIT:
            // the deferred branch is kept for backtracking
            if (_state->choice_count >= _state->choice_capacity)
            {
                return D_INTERNAL_STEP_BUDGET;
            }

            _state->choice[_state->choice_count].pc        = inst->y;
            _state->choice[_state->choice_count].position  = *_position;
            _state->choice[_state->choice_count].undo_mark =
                _state->undo_count;
            _state->choice_count++;
            *_pc = inst->x;

            return D_INTERNAL_STEP_ON;

        default:
            // the first parse that ends exactly where it must is the answer
            return (*_position == _end) ? D_INTERNAL_STEP_DONE
                                        : D_INTERNAL_STEP_FAIL;
    }
}

/*
d_internal_capture_backtrack
  Takes the most recent deferred branch, restoring every mark written since
it was deferred.

Parameter(s):
  _state:    the per-call backtracking buffers.
  _pc:       receives the branch's program counter.
  _position: receives the branch's input position.
Return:
  A boolean value corresponding to either:
  - true, if a branch was taken, or
  - false, if nothing remains to try.
*/
static
bool d_internal_capture_backtrack(
    struct dj_internal_capture_state* _state,
    int*                             _pc,
    size_t*                          _position
)
{
    // the exploration is complete when nothing remains to try
    if (_state->choice_count == 0)
    {
        return false;
    }

    _state->choice_count--;

    const struct dj_internal_choice* entry =
        &_state->choice[_state->choice_count];

    // restore every mark written since the branch was deferred
    while (_state->undo_count > entry->undo_mark)
    {
        _state->undo_count--;
        _state->marks[_state->undo[_state->undo_count].index] =
            _state->undo[_state->undo_count].value;
    }

    *_pc       = entry->pc;
    *_position = entry->position;

    return true;
}

/*
d_internal_capture
  Assigns capture groups within a window whose extent is already known.
NOTE:
  The assignment rule is GNU's, not strict POSIX's, and the difference is
  deliberate.  POSIX says each subexpression matches the longest string
  consistent with the whole match, which would give /(a|ab)(b?)/ on "ab" the
  groups ("ab", ""); GNU regex reports ("a", "b"), preferring the earlier
  alternative.  gawk is built on GNU regex, and gensub() compatibility with
  gawk is the reason capture groups exist here at all, so the first parse
  found under greedy, left-first exploration wins.
CAUTION:
  An iteration of a repetition that consumes nothing must not take the
  back-edge, which is what makes the pattern (a?)* on "aa" report (1,1).  The
  compiler brackets each repetition body with MARK and PROGRESS so the test is
  per iteration rather than per address; nesting therefore works, and the fast
  path treats both as epsilon transitions.

Parameter(s):
  _regex:  the compiled pattern.
  _state:  the per-call backtracking buffers.
  _text:   the subject bytes.
  _length: the length of the subject.
  _start:  the offset at which the match begins.
  _end:    the offset at which the match ends.
Return:
  D_REGEX_OK when a parse of the window was completed, and
  D_REGEX_ERR_BUDGET when the step budget ran out before any was.
*/
static enum dj_regex_status
d_internal_capture(
    struct dj_regex*                  _regex,
    struct dj_internal_capture_state* _state,
    const char*                      _text,
    size_t                           _length,
    size_t                           _start,
    size_t                           _end
)
{
    const size_t pairs = (_regex->group_count + 1u) * 2u;
    const size_t total = pairs + (_regex->loop_count * 2u);

    // slots occupy the low marks and each repetition owns two of the rest, so
    // a single trail unwinds captures and loop bookkeeping together
    for (size_t mark = 0; mark < total; ++mark)
    {
        _state->marks[mark] = D_REGEX_NO_MATCH;
    }

    _state->marks[0]     = _start;
    _state->undo_count   = 0;
    _state->choice_count = 0;

    size_t position = _start;
    int    pc       = 0;

    // explore until a parse completes, or the alternatives or budget run out
    for (size_t steps = 0; steps < (size_t)D_REGEX_BACKTRACK_MAX; ++steps)
    {
        const enum dj_internal_step step =
            d_internal_capture_step(_regex,
                                    _state,
                                    pairs,
                                    _text,
                                    _length,
                                    _end,
                                    &pc,
                                    &position);

        // a completed parse is the answer
        if (step == D_INTERNAL_STEP_DONE)
        {
            _state->marks[1] = position;
            memcpy(_state->best,
                   _state->marks,
                   pairs * sizeof(*_state->best));

            return D_REGEX_OK;
        }

        // the trail or the choices are full
        if (step == D_INTERNAL_STEP_BUDGET)
        {
            return D_REGEX_ERR_BUDGET;
        }

        // take the most recent deferred branch when the current one died
        if ( (step == D_INTERNAL_STEP_FAIL) &&
             (!d_internal_capture_backtrack(_state, &pc, &position)) )
        {
            break;
        }
    }

    return D_REGEX_ERR_BUDGET;
}


//==============================================================================
// 7.  PUBLIC INTERFACE
//==============================================================================


/**
 * @brief Gives a compiled pattern the scratch buffers its simulation runs in:
 *        two thread lists, the seen marks and the closure stack, sized by its
 *        program.
 *
 * @param[in,out] _regex  the pattern, its scratch pointers NULL.
 * @param[in]     _slots  the program's length, in instructions.
 * @return true when every buffer was allocated; false when a size would not
 *         fit in a size_t, with nothing allocated, or when memory ran out,
 *         with what was allocated left for d_regex_free.
 *
 * Every size is checked before the first allocation, so a slot count too
 * large allocates nothing at all.
 */
static bool
d_internal_scratch_new(
    struct dj_regex* _regex,
    size_t           _slots
)
{
    size_t pc_bytes    = 0;
    size_t start_bytes = 0;
    size_t depth       = 0;
    size_t stack_bytes = 0;

    // each size first, so that one too large allocates nothing
    if ( (d_ckd_mul(&pc_bytes,
                    _slots,
                    sizeof(*_regex->list_a.pc)))      ||
         (d_ckd_mul(&start_bytes,
                    _slots,
                    sizeof(*_regex->list_a.starts)))  ||
         (d_ckd_mul(&depth,
                    _slots,
                    2u))                              ||
         (d_ckd_add(&depth,
                    depth,
                    8u))                              ||
         (d_ckd_mul(&stack_bytes,
                    depth,
                    sizeof(*_regex->stack))) )
    {
        return false;
    }

    _regex->list_a.pc     = malloc(pc_bytes);
    _regex->list_a.starts = malloc(start_bytes);
    _regex->list_b.pc     = malloc(pc_bytes);
    _regex->list_b.starts = malloc(start_bytes);
    _regex->seen          = calloc(_slots, sizeof(*_regex->seen));
    _regex->stack         = malloc(stack_bytes);

    return ( (_regex->list_a.pc)     &&
             (_regex->list_a.starts) &&
             (_regex->list_b.pc)     &&
             (_regex->list_b.starts) &&
             (_regex->seen)          &&
             (_regex->stack) );
}


/*
d_internal_report
  Reports an outcome where the caller asked for one.

Parameter(s):
  _out_status: where to report it; may be NULL.
  _status:     the outcome.
Return:
  None.
*/
static
void d_internal_report(
    enum dj_regex_status* _out_status,
    enum dj_regex_status  _status
)
{
    if (_out_status)
    {
        *_out_status = _status;
    }

    return;
}

/*
d_internal_compile_parse
  Parses a whole pattern to its tree. On failure the parser's pools are
released.

Parameter(s):
  _parser:   the parser to fill.
  _pattern:  the pattern text, NUL-terminated.
  _out_root: receives the tree's root.
Return:
  D_REGEX_OK, or the first error the parser latched; a pattern not wholly
consumed is D_REGEX_ERR_SYNTAX.
*/
static enum dj_regex_status
d_internal_compile_parse(
    struct dj_internal_parser* _parser,
    const char*                _pattern,
    int*                       _out_root
)
{
    memset(_parser, 0, sizeof(*_parser));
    _parser->pattern = _pattern;
    _parser->length  = strlen(_pattern);
    _parser->status  = D_REGEX_OK;

    const int root = d_internal_parse_alternation(_parser);

    // the whole pattern must be consumed
    if ((root >= 0) && (_parser->position != _parser->length))
    {
        _parser->status = D_REGEX_ERR_SYNTAX;
    }

    // abandon compilation when parsing failed
    if ((root < 0) || (_parser->status != D_REGEX_OK))
    {
        free(_parser->nodes);
        free(_parser->classes);
        _parser->nodes   = NULL;
        _parser->classes = NULL;

        // the first error the parser latched
        return (_parser->status == D_REGEX_OK) ? D_REGEX_ERR_SYNTAX
                                               : _parser->status;
    }

    *_out_root = root;

    return D_REGEX_OK;
}

/*
d_internal_compile_lower
  Lowers a parsed tree to the program, ending in the accepting instruction.
The tree is released either way; on failure, the program and the parser's
classes are too.

Parameter(s):
  _emitter: the emitter to fill.
  _parser:  the parser holding the tree and its classes.
  _root:    the tree's root.
Return:
  D_REGEX_OK, or the error the emitter latched; a failure it latched none
for is D_REGEX_ERR_MEMORY.
*/
static enum dj_regex_status
d_internal_compile_lower(
    struct dj_internal_emitter* _emitter,
    struct dj_internal_parser*  _parser,
    int                         _root
)
{
    memset(_emitter, 0, sizeof(*_emitter));
    _emitter->status = D_REGEX_OK;

    const bool lowered = d_internal_compile_node(_emitter,
                                                 _parser->nodes,
                                                 _root);
    const int  accept  = lowered
                       ? d_internal_emit(_emitter, D_INTERNAL_OP_MATCH)
                       : -1;

    free(_parser->nodes);
    _parser->nodes = NULL;

    // abandon compilation when lowering failed
    if (accept < 0)
    {
        free(_emitter->program);
        free(_parser->classes);
        _emitter->program = NULL;
        _parser->classes  = NULL;

        // the error the emitter latched
        return (_emitter->status == D_REGEX_OK) ? D_REGEX_ERR_MEMORY
                                                : _emitter->status;
    }

    return D_REGEX_OK;
}

/*
d_regex_compile
  Parses and compiles a POSIX extended regular expression.

Parameter(s):
  _pattern:     the pattern text, NUL-terminated.
  _out_status:  receives the outcome; may be NULL.
Return:
  The compiled pattern, or NULL on failure.
*/
struct dj_regex*
d_regex_compile(
    const char*          _pattern,
    enum dj_regex_status* _out_status
)
{
    struct dj_internal_parser  parser;
    struct dj_internal_emitter emitter;
    int                        root   = -1;
    enum dj_regex_status       status = D_REGEX_ERR_SYNTAX;

    // parameter validation first
    if (!_pattern)
    {
        d_internal_report(_out_status, D_REGEX_ERR_SYNTAX);

        return NULL;
    }

    status = d_internal_compile_parse(&parser, _pattern, &root);

    // abandon compilation when parsing failed
    if (status == D_REGEX_OK)
    {
        status = d_internal_compile_lower(&emitter, &parser, root);
    }

    // abandon compilation when parsing or lowering failed
    if (status != D_REGEX_OK)
    {
        d_internal_report(_out_status, status);

        return NULL;
    }

    struct dj_regex* regex = calloc(1, sizeof(*regex));

    // abandon compilation when the handle cannot be held
    if (!regex)
    {
        d_internal_report(_out_status, D_REGEX_ERR_MEMORY);
        free(emitter.program);
        free(parser.classes);

        return NULL;
    }

    regex->program       = emitter.program;
    regex->program_count = emitter.count;
    regex->classes       = parser.classes;
    regex->class_count   = parser.class_count;
    regex->group_count   = (size_t)parser.group_count;
    regex->loop_count    = (size_t)emitter.loop_count;
    regex->generation    = 0;

    // abandon compilation when any scratch buffer could not be held
    if (!d_internal_scratch_new(regex,
                                emitter.count))
    {
        d_regex_free(regex);
        d_internal_report(_out_status, D_REGEX_ERR_MEMORY);

        return NULL;
    }

    d_internal_report(_out_status, D_REGEX_OK);

    return regex;
}


/*
d_regex_free
  Releases a compiled pattern and its scratch buffers.

Parameter(s):
  _regex: the pattern to release; may be NULL.
Return:
  none.
*/
void d_regex_free(
    struct dj_regex* _regex
)
{
    if (_regex)
    {
        free(_regex->program);
        free(_regex->classes);
        free(_regex->list_a.pc);
        free(_regex->list_a.starts);
        free(_regex->list_b.pc);
        free(_regex->list_b.starts);
        free(_regex->seen);
        free(_regex->stack);
        free(_regex);
    }

    return;
}


/*
d_regex_search
  Locates the leftmost-longest match at or after an offset.

Parameter(s):
  _regex:      the compiled pattern.
  _text:       the subject bytes.
  _length:     the length of the subject.
  _from:       the offset at which to begin.
  _out_match:  receives the extent of the match; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if a match was found, or
  - false, otherwise.
*/
bool d_regex_search(
    struct dj_regex*       _regex,
    const char*            _text,
    size_t                 _length,
    size_t                 _from,
    struct dj_regex_match* _out_match
)
{
    // parameter validation first
    if ( (!_regex)            ||
         ((!_text) && (_length > 0)) ||
         (_from > _length) )
    {
        return false;
    }

    size_t start  = 0;
    size_t extent = 0;

    const bool found = d_internal_run(_regex,
                                      _text,
                                      _length,
                                      _from,
                                      false,
                                      &start,
                                      &extent);

    // report the extent when the caller asked for it
    if ((found) && (_out_match))
    {
        _out_match->start  = start;
        _out_match->length = extent;
    }

    return found;
}


/*
d_regex_match_at
  Reports the longest match beginning exactly at an offset.

Parameter(s):
  _regex:      the compiled pattern.
  _text:       the subject bytes.
  _length:     the length of the subject.
  _at:         the offset at which the match must begin.
  _out_length: receives the length of the match; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if a match begins at the offset, or
  - false, otherwise.
*/
bool d_regex_match_at(
    struct dj_regex* _regex,
    const char*      _text,
    size_t           _length,
    size_t           _at,
    size_t*          _out_length
)
{
    // parameter validation first
    if ( (!_regex)                   ||
         ((!_text) && (_length > 0)) ||
         (_at > _length) )
    {
        return false;
    }

    size_t start  = 0;
    size_t extent = 0;

    const bool found = d_internal_run(_regex,
                                      _text,
                                      _length,
                                      _at,
                                      true,
                                      &start,
                                      &extent);

    // report the extent when the caller asked for it
    if ((found) && (_out_length))
    {
        *_out_length = extent;
    }

    return found;
}


/*
d_regex_test
  Reports whether a pattern matches anywhere in a subject.

Parameter(s):
  _regex:  the compiled pattern.
  _text:   the subject bytes.
  _length: the length of the subject.
Return:
  A boolean value corresponding to either:
  - true, if the pattern matches, or
  - false, otherwise.
*/
bool d_regex_test(
    struct dj_regex* _regex,
    const char*      _text,
    size_t           _length
)
{
    return d_regex_search(_regex, _text, _length, 0, NULL);
}


/*
d_regex_status_text
  Returns a stable description of a status value.

Parameter(s):
  _status: the status to describe.
Return:
  A NUL-terminated description owned by the library.
*/
const char*
d_regex_status_text(
    enum dj_regex_status _status
)
{
    switch (_status)
    {
        case D_REGEX_OK:         return "ok";
        case D_REGEX_ERR_SYNTAX: return "syntax error in regular expression";
        case D_REGEX_ERR_RANGE:  return "repetition or group count "
                                        "out of range";
        case D_REGEX_ERR_MEMORY: return "out of memory";
        default:                 return "capture pass exceeded its budget";
    }
}


/*
d_regex_group_count
  Returns the number of capturing groups in a compiled pattern.
NOTE:
  Group 0 denotes the whole match and is not included in the count.

Parameter(s):
  _regex: the pattern to inspect.
Return:
  The number of capturing groups, or zero when the pattern is NULL.
*/
size_t d_regex_group_count(
    const struct dj_regex* _regex
)
{
    // parameter validation first
    if (!_regex)
    {
        return 0;
    }

    return _regex->group_count;
}


/**
 * @brief Allocates the capturing pass's buffers for one window: the marks,
 *        the best assignment, and the undo and choice stacks.
 *
 * @param[out] _state   the pass's state, zeroed; receives the buffers and the
 *                      stacks' capacities.
 * @param[in]  _regex   the compiled pattern, which has groups.
 * @param[in]  _extent  the window's length, in bytes.
 * @return true when every buffer was allocated; false when a size would not
 *         fit in a size_t or memory ran out, with nothing left allocated.
 *
 * The stacks hold eight entries for every instruction and every byte of the
 * window, and eight more; each product and sum is checked before the first
 * allocation, so a window too large allocates nothing.
 */
static bool
d_internal_capture_state_new(
    struct dj_internal_capture_state* _state,
    const struct dj_regex*            _regex,
    size_t                            _extent
)
{
    size_t pairs        = 0;
    size_t loop_marks   = 0;
    size_t marks        = 0;
    size_t steps        = 0;
    size_t capacity     = 0;
    size_t mark_bytes   = 0;
    size_t best_bytes   = 0;
    size_t undo_bytes   = 0;
    size_t choice_bytes = 0;

    // (groups + 1) * 2 pairs, two marks a loop, (program + extent + 8) * 8
    // entries a stack, and the bytes of each: all checked before any is used
    if ( (d_ckd_add(&pairs,
                    _regex->group_count,
                    1u))                              ||
         (d_ckd_mul(&pairs,
                    pairs,
                    2u))                              ||
         (d_ckd_mul(&loop_marks,
                    _regex->loop_count,
                    2u))                              ||
         (d_ckd_add(&marks,
                    pairs,
                    loop_marks))                      ||
         (d_ckd_add(&steps,
                    _regex->program_count,
                    _extent))                         ||
         (d_ckd_add(&steps,
                    steps,
                    8u))                              ||
         (d_ckd_mul(&capacity,
                    steps,
                    8u))                              ||
         (d_ckd_mul(&mark_bytes,
                    marks,
                    sizeof(*_state->marks)))          ||
         (d_ckd_mul(&best_bytes,
                    pairs,
                    sizeof(*_state->best)))           ||
         (d_ckd_mul(&undo_bytes,
                    capacity,
                    sizeof(*_state->undo)))           ||
         (d_ckd_mul(&choice_bytes,
                    capacity,
                    sizeof(*_state->choice))) )
    {
        return false;
    }

    _state->undo_capacity   = capacity;
    _state->choice_capacity = capacity;
    _state->marks           = malloc(mark_bytes);
    _state->best            = malloc(best_bytes);
    _state->undo            = malloc(undo_bytes);
    _state->choice          = malloc(choice_bytes);

    // all or nothing: a pass with one buffer short does not start
    if ( (!_state->marks)  ||
         (!_state->best)   ||
         (!_state->undo)   ||
         (!_state->choice) )
    {
        free(_state->marks);
        free(_state->best);
        free(_state->undo);
        free(_state->choice);
        memset(_state, 0, sizeof(*_state));

        return false;
    }

    return true;
}


/*
d_regex_search_groups
  Locates a match and reports the extent of each capturing group.
NOTE:
  The simulation fixes the overall extent first, so this pass only chooses
  among the parses of one window.  Group assignment follows GNU regex, which
  gawk is built on, rather than strict POSIX; see d_internal_capture.  Index
  zero is the whole match; a group that did not participate reports a start of
  D_REGEX_NO_MATCH.  A return of
  D_REGEX_OK with a group-zero start of D_REGEX_NO_MATCH means the pattern did
  not match, which is distinct from the error statuses.
CAUTION:
  A pathological pattern may exhaust D_REGEX_BACKTRACK_MAX and report
  D_REGEX_ERR_BUDGET even though the fast path found a match.  The fast path
  is unaffected.

Parameter(s):
  _regex:      the compiled pattern.
  _text:       the subject bytes.
  _length:     the length of the subject.
  _from:       the offset at which to begin.
  _out_groups: receives one extent per group, with index zero the whole match.
  _count:      the number of entries in `_out_groups`.
Return:
  D_REGEX_OK, with a group-zero start of D_REGEX_NO_MATCH where there is no
  match; D_REGEX_ERR_SYNTAX for an argument that cannot be right;
  D_REGEX_ERR_MEMORY when the pass's buffers cannot be held; and
  D_REGEX_ERR_BUDGET when the pass completes no parse within its budget.
*/
enum dj_regex_status
d_regex_search_groups(
    struct dj_regex*       _regex,
    const char*           _text,
    size_t                _length,
    size_t                _from,
    struct dj_regex_match* _out_groups,
    size_t                _count
)
{
    // parameter validation first
    if ( (!_regex)                    ||
         ((!_text) && (_length > 0))  ||
         (!_out_groups)               ||
         (_count == 0)                ||
         (_from > _length) )
    {
        return D_REGEX_ERR_SYNTAX;
    }

    // every entry begins absent, so a short array still reads correctly
    for (size_t entry = 0; entry < _count; ++entry)
    {
        _out_groups[entry].start  = D_REGEX_NO_MATCH;
        _out_groups[entry].length = 0;
    }

    size_t start  = 0;
    size_t extent = 0;

    // the simulation fixes the window before any parse of it is considered
    if (!d_internal_run(_regex, _text, _length, _from, false, &start, &extent))
    {
        return D_REGEX_OK;
    }

    _out_groups[0].start  = start;
    _out_groups[0].length = extent;

    // a pattern without groups needs no second pass
    if (_regex->group_count == 0)
    {
        return D_REGEX_OK;
    }

    struct dj_internal_capture_state state;

    memset(&state, 0, sizeof(state));

    // abandon the pass when any buffer could not be held
    if (!d_internal_capture_state_new(&state,
                                      _regex,
                                      extent))
    {
        return D_REGEX_ERR_MEMORY;
    }

    const enum dj_regex_status status = d_internal_capture(_regex,
                                                          &state,
                                                          _text,
                                                          _length,
                                                          start,
                                                          start + extent);

    // transcribe the winning assignment into the caller's array
    if (status == D_REGEX_OK)
    {
        const size_t reported = (_count < (_regex->group_count + 1u))
                              ? _count
                              : (_regex->group_count + 1u);

        for (size_t group = 0; group < reported; ++group)
        {
            const size_t group_start = state.best[group * 2u];
            const size_t group_end   = state.best[(group * 2u) + 1u];

            // a group that did not participate keeps its absent marker
            if ( (group_start == D_REGEX_NO_MATCH) ||
                 (group_end == D_REGEX_NO_MATCH) )
            {
                continue;
            }

            _out_groups[group].start  = group_start;
            _out_groups[group].length = group_end - group_start;
        }
    }

    free(state.marks);
    free(state.best);
    free(state.undo);
    free(state.choice);

    return status;
}


/*
d_regex_program_size
  Returns the instruction count of a compiled pattern.

Parameter(s):
  _regex: the pattern to measure.
Return:
  The number of instructions, or zero when the pattern is NULL.
*/
size_t d_regex_program_size(
    const struct dj_regex* _regex
)
{
    // parameter validation first
    if (!_regex)
    {
        return 0;
    }

    return _regex->program_count;
}
