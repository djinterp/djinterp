/*******************************************************************************
* djinterp [tools]                                                   dgit_text.h
*
* Git's text files, read without loss.
*   Three kinds of file, each read as git reads it: the ignore files --
* .gitignore, info/exclude, core.excludesFile; the attribute files --
* .gitattributes, info/attributes, core.attributesFile; and the files written
* in git's configuration syntax -- a repository's config, a user's
* .gitconfig, .gitmodules. A file is read into records, each with its place
* in the text, so a change to one is a splice that leaves every other byte
* as it was.
*   The records: every line, with what it is; each pattern of an ignore or
* attribute file, as written and as git matches it, with an attribute file's
* states -- set, unset, unspecified, a value -- and its macros; and each
* section and entry of a configuration file, as written and as git reads it:
* the canonical key, the value with its quotes and escapes undone.
*   Reading never fails on what a file holds. What git refuses -- an
* attribute of an invalid name, a negative pattern among attributes, a
* configuration file past a line git cannot read -- is kept as a problem,
* with its line, and the line kept as text.
*   Matching is git's: wildmatch, the pattern rules of gitignore(5) and
* gitattributes(5) -- a pattern without a slash matches a name at any depth,
* one with a slash is anchored to its file's directory, a trailing slash
* matches only a directory -- the precedence of the files that apply to a
* path, the exclusion of everything below a directory that is ignored, and
* the macros of attribute files, `binary` among them.
*   Independent of the repository: what a path's ignore and attribute files
* are, and where, is dgit_repo's to find; this unit takes them as scopes.
*
* path:      /inc/djinterp/tools/dawk/ext/git/dgit_text.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Constants
         1.  D_GIT_TEXT_NONE
         2.  D_GIT_TEXT_PATHNAME
         3.  D_GIT_TEXT_CASEFOLD
    2.  Kinds
         1.  d_git_text_kind
         2.  d_git_text_line_kind
         3.  d_git_text_state
    3.  Records
         1.  d_git_text_line
         2.  d_git_text_pattern
         3.  d_git_text_attribute
         4.  d_git_text_section
         5.  d_git_text_entry
         6.  d_git_text_problem
         7.  d_git_text
    4.  Lookups
         1.  d_git_text_scope
         2.  d_git_text_query
         3.  d_git_text_match
         4.  d_git_text_value
         5.  d_git_text_values
    5.  Text
         1.  d_git_buffer
2.  OPERATIONS
    ----------
    1.  Kinds
    2.  Reading
    3.  Records
    4.  Matching
    5.  Ignoring
    6.  Attributes
    7.  Configuration
    8.  Quoting
    9.  Text
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_GIT_DGIT_TEXT_H
#define DJINTERP_TOOLS_DAWK_EXT_GIT_DGIT_TEXT_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// re_std
#include "../../../../../re_std/cstdint/dstdint.h"  // int64_t


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_GIT_TEXT_NONE
//   constant: the index of a record that is not there, and the place of
//   text that is not written -- an entry with no `=`.
#define D_GIT_TEXT_NONE ((size_t)-1)

// 1.1.2
// D_GIT_TEXT_PATHNAME
//   constant: wildmatch's flag that a `/` is matched only by a `/` -- not
//   by `?`, `*` or a class -- and that `**` between slashes matches any
//   number of directories.
#define D_GIT_TEXT_PATHNAME 1u

// 1.1.3
// D_GIT_TEXT_CASEFOLD
//   constant: wildmatch's flag that letters match in either case, as git
//   matches with core.ignoreCase.
#define D_GIT_TEXT_CASEFOLD 2u

// 1.2    Kinds
//------------------------------------------------------------------------------
// 1.2.1
// d_git_text_kind
//   enum: what a file is.
enum d_git_text_kind
{
    D_GIT_TEXT_OTHER = 0,
    D_GIT_TEXT_IGNORE,      // .gitignore, info/exclude, git/ignore
    D_GIT_TEXT_ATTRIBUTES,  // .gitattributes, info/attributes
    D_GIT_TEXT_CONFIG       // config, .gitconfig, .gitmodules
};

// 1.2.2
// d_git_text_line_kind
//   enum: what a line is.
enum d_git_text_line_kind
{
    D_GIT_TEXT_BLANK = 0,  // nothing git reads: empty, or white space
    D_GIT_TEXT_COMMENT,    // `#` -- or, in a configuration, `;`
    D_GIT_TEXT_PATTERN,    // a pattern, an attribute file's with states
    D_GIT_TEXT_MACRO,      // an attribute file's [attr]name and states
    D_GIT_TEXT_SECTION,    // a configuration's [section], perhaps with an
                           // entry after it on the line
    D_GIT_TEXT_ENTRY,      // a configuration's entry, or a line one
                           // continues onto
    D_GIT_TEXT_REFUSED     // a line git reads no rule from: see problems
};

// 1.2.3
// d_git_text_state
//   enum: what an attribute says of a path.
enum d_git_text_state
{
    D_GIT_TEXT_SET = 0,      // text
    D_GIT_TEXT_UNSET,        // -text
    D_GIT_TEXT_UNSPECIFIED,  // !text, or nothing said
    D_GIT_TEXT_VALUE         // eol=lf
};

// 1.3    Records
//------------------------------------------------------------------------------
// 1.3.1
// d_git_text_line
//   struct: one line: `start` to `end`, its ending -- `\n`, or `\r\n` --
//   not among them, and `next` past its ending, where the next begins.
//   `pattern` is the pattern or macro a line is; `section` the section a
//   configuration's line heads or is within; `entry` the entry a line
//   holds or continues; each D_GIT_TEXT_NONE when there is none.
struct d_git_text_line
{
    size_t                    start;
    size_t                    end;
    size_t                    next;
    enum d_git_text_line_kind kind;
    size_t                    pattern;
    size_t                    section;
    size_t                    entry;
};

// 1.3.2
// d_git_text_pattern
//   struct: one pattern of an ignore or attribute file, or one macro of an
//   attribute file.  `start` to `end` is the pattern as written: an ignore
//   file's `!` among it, a quoted pattern's quotes, the trailing spaces git
//   trims not; a macro's is its "[attr]name".  `text` is the pattern git
//   matches, `length` bytes at an offset into the strings: the `!` and a
//   trailing `/` off, a quoted pattern unquoted; a macro's name.
//   `negated` is an ignore pattern's `!`; `directory` a trailing `/`,
//   which matches only a directory; `anywhere` that the pattern holds no
//   other `/`, and so matches a name at any depth below its file.  An
//   attribute file's states are `attribute_count` from `first_attribute`.
struct d_git_text_pattern
{
    size_t line;
    size_t start;
    size_t end;
    size_t text;
    size_t length;
    bool   negated;
    bool   directory;
    bool   anywhere;
    bool   quoted;
    bool   macro;
    size_t first_attribute;
    size_t attribute_count;
};

// 1.3.3
// d_git_text_attribute
//   struct: one state of an attribute line, as written from `start` to
//   `end` -- "text", "-text", "!text", "eol=lf".  Its name is
//   `name_start` to `name_end`; a value's is `value_start` to `value_end`,
//   each D_GIT_TEXT_NONE for a state that has none.
struct d_git_text_attribute
{
    size_t                start;
    size_t                end;
    size_t                name_start;
    size_t                name_end;
    enum d_git_text_state state;
    size_t                value_start;
    size_t                value_end;
};

// 1.3.4
// d_git_text_section
//   struct: one section header of a configuration, `start` at its `[` and
//   `end` past its `]`, on line `line`.  Its name as written is
//   `name_start` to `name_end`; a subsection's text between its quotes,
//   escapes and all, `sub_start` to `sub_end`, D_GIT_TEXT_NONE without.
//   `prefix` is what the keys of its entries begin with, at an offset into
//   the strings: the name in lower case, then a dot and the subsection --
//   unescaped, as written, or, in the old form [section.sub] that `legacy`
//   says, in lower case.  Its entries are `entry_count` from
//   `first_entry`.
struct d_git_text_section
{
    size_t line;
    size_t start;
    size_t end;
    size_t name_start;
    size_t name_end;
    size_t sub_start;
    size_t sub_end;
    size_t prefix;
    bool   legacy;
    size_t first_entry;
    size_t entry_count;
};

// 1.3.5
// d_git_text_entry
//   struct: one entry of a configuration, from line `line` to
//   `last_line`, which differ when a value continues past a line's end.
//   `start` is its name's first byte and `end` past its value's last, a
//   comment after it not among them.  Its name as written is `name_start`
//   to `name_end`; `equals` the place of its `=`, and its value as written
//   `value_start` to `value_end`, quotes and escapes and all, each
//   D_GIT_TEXT_NONE for a name alone, which git reads as true.  `key` is
//   its canonical key at an offset into the strings, the section's prefix,
//   a dot, and the name in lower case; `value` its value as git reads it,
//   D_GIT_TEXT_NONE for a name alone.  `section` is its section, or
//   D_GIT_TEXT_NONE for one before any.
struct d_git_text_entry
{
    size_t line;
    size_t last_line;
    size_t start;
    size_t end;
    size_t section;
    size_t name_start;
    size_t name_end;
    size_t equals;
    size_t value_start;
    size_t value_end;
    size_t key;
    size_t value;
};

// 1.3.6
// d_git_text_problem
//   struct: something git refuses, on line `line`, said at an offset into
//   the strings.
struct d_git_text_problem
{
    size_t line;
    size_t what;
};

// 1.3.7
// d_git_text
//   struct: a file's records.  `text` is borrowed.  `bom` is whether the
//   text begins with a byte-order mark, which git reads past; `crlf`
//   whether its lines end in `\r\n`, as a change writes them.  `broken` is
//   the line a configuration's reading stopped at, D_GIT_TEXT_NONE when
//   git reads it all: git refuses a file it cannot read to its end.
struct d_git_text
{
    const char*                  text;
    size_t                       length;
    enum d_git_text_kind         kind;
    bool                         bom;
    bool                         crlf;
    size_t                       broken;
    struct d_git_text_line*      lines;
    size_t                       line_count;
    size_t                       line_capacity;
    struct d_git_text_pattern*   patterns;
    size_t                       pattern_count;
    size_t                       pattern_capacity;
    struct d_git_text_attribute* attributes;
    size_t                       attribute_count;
    size_t                       attribute_capacity;
    struct d_git_text_section*   sections;
    size_t                       section_count;
    size_t                       section_capacity;
    struct d_git_text_entry*     entries;
    size_t                       entry_count;
    size_t                       entry_capacity;
    struct d_git_text_problem*   problems;
    size_t                       problem_count;
    size_t                       problem_capacity;
    char*                        strings;
    size_t                       strings_length;
    size_t                       strings_capacity;
};

// 1.4    Lookups
//------------------------------------------------------------------------------
// 1.4.1
// d_git_text_scope
//   struct: one file that applies to a path, as a lookup is given it.
//   `base` is the directory, from the top of the work tree and without a
//   trailing slash, its patterns are relative to: "" for the top's, and
//   for a file outside the tree -- info/exclude, core.excludesFile; "a/b"
//   for a/b/.gitignore.  A file applies only to paths below its base.
//   `macros` is whether an attribute file's macros count, which git
//   allows only in the top's .gitattributes and in the files outside the
//   tree.
struct d_git_text_scope
{
    const struct d_git_text* text;
    const char*              base;
    bool                     macros;
};

// 1.4.2
// d_git_text_query
//   struct: a path asked about: from the top of the work tree, apart by
//   `/`, `length` bytes; `directory` whether it names a directory; and
//   `icase` whether names compare in either case, as core.ignoreCase says.
struct d_git_text_query
{
    const char* path;
    size_t      length;
    bool        directory;
    bool        icase;
};

// 1.4.3
// d_git_text_match
//   struct: the pattern that decided whether a path is ignored: pattern
//   `pattern` of scope `scope`; `ignored` is false for a negated one.
//   `through` is the length of the path's leading directory the pattern
//   matched, when a directory above the path is ignored -- git looks no
//   further below one -- and the path's length otherwise.
struct d_git_text_match
{
    size_t scope;
    size_t pattern;
    bool   ignored;
    size_t through;
};

// 1.4.4
// d_git_text_value
//   struct: what one attribute is for a path: its name, `name_length`
//   bytes, and its state, a value `value_length` bytes for
//   D_GIT_TEXT_VALUE; borrowed from the scopes' texts.  `scope` and
//   `pattern` are the line that said it -- a pattern, or the macro that
//   expanded to it -- `scope` D_GIT_TEXT_NONE for git's own `binary`.
struct d_git_text_value
{
    const char*           name;
    size_t                name_length;
    enum d_git_text_state state;
    const char*           value;
    size_t                value_length;
    size_t                scope;
    size_t                pattern;
};

// 1.4.5
// d_git_text_values
//   struct: the attributes said of a path, `count` of them, ordered by
//   name; none of them is unspecified.
struct d_git_text_values
{
    struct d_git_text_value* items;
    size_t                   count;
    size_t                   capacity;
};

// 1.5    Text
//------------------------------------------------------------------------------
// 1.5.1
// d_git_buffer
//   struct: text being made: `length` bytes, terminated, in `capacity`;
//   `failed` once memory ran out, after which it takes no more.  Zeroed,
//   it is empty.
struct d_git_buffer
{
    char*  text;
    size_t length;
    size_t capacity;
    bool   failed;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Kinds
//------------------------------------------------------------------------------
// d_git_text_kind_of names a file's kind by its name: .gitignore, exclude
// and ignore are ignore files; .gitattributes and attributes attribute
// files; config, config.worktree, .gitconfig, gitconfig, .gitmodules and
// .lfsconfig configurations; anything else D_GIT_TEXT_OTHER.
// d_git_text_kind_named reads a name a program gives -- "ignore",
// "attributes", "config", or a file's name -- and d_git_text_kind_name
// gives the kind's own name, "ignore", "attributes", "config", or "other".
enum d_git_text_kind d_git_text_kind_of(const char* _path);
enum d_git_text_kind d_git_text_kind_named(const char* _name);
const char*          d_git_text_kind_name(enum d_git_text_kind _kind);

// 2.2    Reading
//------------------------------------------------------------------------------
/**
 * @brief Reads a file's text into records.
 *
 * @note Any text is a file; what git refuses in one is kept among its
 *       problems.  A byte-order mark is kept, and read past.
 *
 * @param[out] _out       receives the records; release it with
 *                        d_git_text_release, whether or not reading
 *                        succeeded.
 * @param[in]  _text      the text, borrowed for as long as `_out` is used.
 * @param[in]  _length    its length in bytes.
 * @param[in]  _kind      what it is: not D_GIT_TEXT_OTHER.
 * @param[out] _problem   receives why reading failed.
 * @param[in]  _capacity  the size of `_problem`.
 * @return `true` if the text was read; `false` for D_GIT_TEXT_OTHER, or
 *         when memory ran out.
 */
bool        d_git_text_read(struct d_git_text*   _out,
                            const char*          _text,
                            size_t               _length,
                            enum d_git_text_kind _kind,
                            char*                _problem,
                            size_t               _capacity);
void        d_git_text_release(struct d_git_text* _text);

// 2.3    Records
//------------------------------------------------------------------------------
// d_git_text_string gives the string at an offset into a file's strings,
// terminated, or NULL for D_GIT_TEXT_NONE; d_git_text_line_of the index of
// the line an offset is on.
const char* d_git_text_string(const struct d_git_text* _text,
                              size_t                   _offset);
size_t      d_git_text_line_of(const struct d_git_text* _text,
                               size_t                   _offset);

// 2.4    Matching
//------------------------------------------------------------------------------
// d_git_text_wildmatch is git's wildmatch: whether a pattern, `?`, `*`,
// `**`, [classes] and `\` escapes, matches the whole of a text, under
// D_GIT_TEXT_PATHNAME and D_GIT_TEXT_CASEFOLD.  A pattern ends at its
// length, or at a NUL before it.
// d_git_text_matches is whether one pattern of an ignore or attribute file
// matches a path, as git matches it: a pattern without a slash against
// the path's last name, one with a slash against the path below the
// scope's base; a directory pattern only a directory.  A macro matches
// nothing.
bool        d_git_text_wildmatch(const char* _pattern,
                                 size_t      _pattern_length,
                                 const char* _text,
                                 size_t      _text_length,
                                 unsigned    _flags);
bool        d_git_text_matches(const struct d_git_text_scope* _scope,
                               size_t                         _pattern,
                               const struct d_git_text_query* _query);

// 2.5    Ignoring
//------------------------------------------------------------------------------
/**
 * @brief Finds the pattern that decides whether git ignores a path.
 *
 * @note The scopes are given lowest first -- core.excludesFile,
 *       info/exclude, the top's .gitignore, then each directory's down to
 *       the path's, then the patterns a command gives -- and the last
 *       pattern that matches, in the last scope that has one, decides.
 *       Each directory above the path is asked first, as a directory, of
 *       the scopes above it: one that is ignored decides for everything
 *       below it, as git looks no further.
 *
 * @param[in]  _scopes  the files that apply, lowest first.
 * @param[in]  _count   how many.
 * @param[in]  _query   the path.
 * @param[out] _out     receives the pattern that decided.
 * @return `true` if a pattern decided, ignoring the path or, negated, not;
 *         `false` when none matched, and git does not ignore it.
 */
bool        d_git_text_ignored(const struct d_git_text_scope* _scopes,
                               size_t                         _count,
                               const struct d_git_text_query* _query,
                               struct d_git_text_match*       _out);

// 2.6    Attributes
//------------------------------------------------------------------------------
/**
 * @brief Gives every attribute the attribute files say of a path.
 *
 * @note The scopes are given lowest first -- the system's file,
 *       core.attributesFile, the top's .gitattributes, each directory's
 *       down to the path's, then info/attributes -- and git's own macro,
 *       binary = -diff -merge -text, is below them all.  For each
 *       attribute the last line that says it, in the highest scope that
 *       has one, decides; an attribute set that is a macro sets what the
 *       macro says of the attributes no line has decided.  Of a macro
 *       defined twice the last, highest, counts.
 *
 * @param[in]  _scopes  the files that apply, lowest first.
 * @param[in]  _count   how many.
 * @param[in]  _query   the path; a directory's attributes are those its
 *                      patterns ending in `/` give.
 * @param[out] _out     receives the attributes, ordered by name, none
 *                      unspecified; release it with
 *                      d_git_text_values_release, whatever is returned.
 * @return `true`; `false` when memory ran out.
 */
bool        d_git_text_attributes(const struct d_git_text_scope* _scopes,
                                  size_t                         _count,
                                  const struct d_git_text_query* _query,
                                  struct d_git_text_values*      _out);
void        d_git_text_values_release(struct d_git_text_values* _values);
// d_git_text_value_of is what an attribute is among a path's, by name, or
// NULL for one unspecified.
const struct d_git_text_value*
            d_git_text_value_of(const struct d_git_text_values* _values,
                                const char*                     _name);

// 2.7    Configuration
//------------------------------------------------------------------------------
// d_git_text_key writes a key as git compares keys -- the section and the
// name in lower case, a subsection between them as given -- and is false
// for a key git refuses: one with no section or name, a name not
// beginning with a letter, a byte other than a letter, digit or `-` in
// either, a newline in a subsection.  d_git_text_key_parts gives the
// lengths of its section and, D_GIT_TEXT_NONE without one, subsection;
// the name is what is left past the last dot.
bool        d_git_text_key(struct d_git_buffer* _out,
                           const char*          _key);
void        d_git_text_key_parts(const char* _key,
                                 size_t*     _out_section,
                                 size_t*     _out_subsection);
// d_git_text_find_entry is the first entry after `_after` -- or from the
// first, for D_GIT_TEXT_NONE -- whose key is `_key`, canonical; and
// d_git_text_find_section the first section after `_after` whose prefix
// is `_prefix`, "section" or "section.subsection", canonical; each
// D_GIT_TEXT_NONE when there is none.  Neither finds a record past a line
// git cannot read.
size_t      d_git_text_find_entry(const struct d_git_text* _text,
                                  const char*              _key,
                                  size_t                   _after);
size_t      d_git_text_find_section(const struct d_git_text* _text,
                                    const char*              _prefix,
                                    size_t                   _after);
// d_git_text_bool reads a value as git reads a boolean: NULL -- a name
// alone -- true, "" false, true, yes, on, false, no, off in any case, and
// a number, true when it is not 0; false for anything else.
// d_git_text_int reads a value as git reads a number: decimal, 0x hex or
// 0 octal, signed, then k, m or g in either case for 1024, 1024^2 or
// 1024^3; false for anything else, or one past 64 bits.
bool        d_git_text_bool(const char* _value,
                            bool*       _out);
bool        d_git_text_int(const char* _value,
                           int64_t*    _out);

// 2.8    Quoting
//------------------------------------------------------------------------------
// d_git_text_quote_value writes a value as git config writes one, in
// double quotes when it begins or ends with white space or holds `;` or
// `#`, its newlines, tabs, backspaces, quotes and backslashes escaped;
// d_git_text_quote_subsection a subsection's text, between its quotes.
// d_git_text_quote_path writes a path as git prints one: as it is, or in
// double quotes with C's escapes when it holds a control byte, a quote,
// a backslash, or a byte past ASCII, as core.quotePath has it by default.
// d_git_text_unquote reads text in C's quotes, as git reads a quoted
// pattern: `_used` receives how many bytes the quoted text took, its
// quotes among them; false for text that is not quoted, or is badly so.
bool        d_git_text_quote_value(struct d_git_buffer* _out,
                                   const char*          _value);
bool        d_git_text_quote_subsection(struct d_git_buffer* _out,
                                        const char*          _subsection);
bool        d_git_text_quote_path(struct d_git_buffer* _out,
                                  const char*          _path,
                                  size_t               _length);
bool        d_git_text_unquote(struct d_git_buffer* _out,
                               const char*          _text,
                               size_t               _length,
                               size_t*              _used);

// 2.9    Text
//------------------------------------------------------------------------------
// The buffer's writers add text, keeping it terminated, and are false once
// memory has run out; d_git_buffer_take gives its text, which the caller
// frees, and empties it -- NULL if it failed -- and d_git_buffer_free
// frees it.
bool        d_git_buffer_add(struct d_git_buffer* _buffer,
                             const char*          _text,
                             size_t               _length);
bool        d_git_buffer_add_text(struct d_git_buffer* _buffer,
                                  const char*          _text);
bool        d_git_buffer_add_char(struct d_git_buffer* _buffer,
                                  char                 _byte);
char*       d_git_buffer_take(struct d_git_buffer* _buffer);
void        d_git_buffer_free(struct d_git_buffer* _buffer);


#endif  // DJINTERP_TOOLS_DAWK_EXT_GIT_DGIT_TEXT_H
