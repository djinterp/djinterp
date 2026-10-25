/*******************************************************************************
* djinterp [tools]                                                       dmsvs.h
*
* Visual Studio's files, read without loss.
*   Two kinds of file. A solution -- .slnx, which is XML, or .sln, Visual
* Studio's older text -- lists projects, the folders that hold them, what
* each must build after, and the configurations the solution builds. An
* MSBuild file -- .vcxproj, .vcxproj.filters, .props, .targets -- is XML
* whose elements are property groups, item definition groups and item
* groups, imports and targets, each of them perhaps under a Condition.
*   An XML file is read into its elements, in document order, each with its
* place in the text: where its start tag begins, where its content begins
* and ends, and where its end tag ends. The bytes between are the file's
* own, so a change is a splice that leaves the rest of the file as it was,
* its byte-order mark and its line endings included. A solution is read
* into projects, too, the same from either form.
*   A condition is understood when it tests the configuration, the platform,
* or both, the way Visual Studio writes it: `'$(Configuration)|$(Platform)'
* =='Debug|x64'`. Any other condition -- exists(...), a property's value --
* is reported as such, for a caller to treat as it chooses.
*   Every record of a solution says where it stands in the text, so a
* change to one is a splice too: a project's lines or element, a folder's,
* and, in a .sln, each global section's.
*   Three things a writer of these files needs as well: a GUID made from a
* name, the same every time; a path written relative to a directory, the
* way Visual Studio writes one; and text escaped for XML.
*   Not done: evaluation. A property's value is its text, $(...) and %(...)
* unexpanded, and imports are not followed.
*   A stand-in, local to dawk: XML belongs to the parse tier, and when that
* tier reads it, this unit gives way to it.
*
* path:      /inc/djinterp/tools/dawk/ext/msvs/dmsvs.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Constants
         1.  D_MSVS_NONE
    2.  Kinds
         1.  d_msvs_kind
         2.  d_msvs_condition
    3.  Records
         1.  d_msvs_element
         2.  d_msvs_project
         3.  d_msvs_folder
         4.  d_msvs_section
         5.  d_msvs
2.  OPERATIONS
    ----------
    1.  Reading
    2.  Elements
    3.  Conditions
    4.  Writing
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_MSVS_DMSVS_H
#define DJINTERP_TOOLS_DAWK_EXT_MSVS_DMSVS_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_MSVS_NONE
//   constant: the index of a record that is not there -- the root's parent,
//   a leaf's first child, a last sibling's next.
#define D_MSVS_NONE ((size_t)-1)

// 1.2    Kinds
//------------------------------------------------------------------------------
// 1.2.1
// d_msvs_kind
//   enum: what a file is, by its name.
enum d_msvs_kind
{
    D_MSVS_OTHER = 0,
    D_MSVS_SOLUTION,  // .slnx, or .sln
    D_MSVS_PROJECT,   // .vcxproj, .vcxitems, .csproj, .vbproj, .fsproj
    D_MSVS_FILTERS,   // .vcxproj.filters
    D_MSVS_USER,      // .vcxproj.user
    D_MSVS_PROPS      // .props, .targets
};

// 1.2.2
// d_msvs_condition
//   enum: what a Condition tests.
enum d_msvs_condition
{
    D_MSVS_ALWAYS = 0,     // there is none, or it is empty
    D_MSVS_CONFIGURATION,  // the configuration, the platform, or both
    D_MSVS_UNKNOWN         // anything else
};

// 1.3    Records
//------------------------------------------------------------------------------
// 1.3.1
// d_msvs_element
//   struct: one XML element.  `start` is its `<` and `end` one past the `>`
//   ending it; `content_start` and `content_end` bound what stands between
//   its tags, and are equal for an empty element.  Its name, as written, is
//   `name_length` bytes from `name`, and its attributes, raw, are
//   `attributes_length` bytes from `attributes`.  `line` and `column` are
//   where it starts, from 1, and `end_line` the line of its last byte.  The
//   links are indices into the elements, in document order.
struct d_msvs_element
{
    size_t start;
    size_t end;
    size_t content_start;
    size_t content_end;
    size_t name;
    size_t name_length;
    size_t attributes;
    size_t attributes_length;
    size_t line;
    size_t column;
    size_t end_line;
    size_t depth;
    size_t parent;
    size_t first_child;
    size_t next_sibling;
    size_t child_count;
    bool   empty;
};

// 1.3.2
// d_msvs_project
//   struct: one project a solution lists.  `path` is as the solution writes
//   it; `name` is its display name, or the path's file name without its
//   extension; `type` and `id` are GUIDs in lower case without braces, ""
//   when not given; `folder` is the solution folder holding it, as .slnx
//   writes one, "/tools/dawk/", or "" at the top.  Its dependencies are
//   `dependency_count` entries of the dependencies from `first_dependency`,
//   each a project's path.  `start` and `end` bound its record: in a .slnx
//   its element, which is `element`; in a .sln its lines, Project( to
//   EndProject and the line ending after, with `depends` the start of its
//   ProjectSection(ProjectDependencies) line and `depends_close` of that
//   section's EndProjectSection line, D_MSVS_NONE when it has none.
struct d_msvs_project
{
    char*  name;
    char*  path;
    char*  type;
    char*  id;
    char*  folder;
    size_t first_dependency;
    size_t dependency_count;
    size_t line;
    size_t start;
    size_t end;
    size_t element;
    size_t depends;
    size_t depends_close;
};

// 1.3.3
// d_msvs_folder
//   struct: one solution folder, by its path, "/tools/dawk/".  A .slnx
//   writes each as a <Folder> of that name, which is `element`; a .sln as a
//   project of the folder type, nested in its parent, whose id is `id`.
//   `start` and `end` bound its record as a project's do.
struct d_msvs_folder
{
    char*  path;
    char*  id;
    size_t line;
    size_t start;
    size_t end;
    size_t element;
};

// 1.3.4
// d_msvs_section
//   struct: one GlobalSection of a .sln: its name, as in
//   GlobalSection(NestedProjects), and its lines -- `start` its first,
//   `content` the one after, `close` its EndGlobalSection line, and `end`
//   past that line's ending.
struct d_msvs_section
{
    char*  name;
    size_t line;
    size_t start;
    size_t content;
    size_t close;
    size_t end;
};

// 1.3.5
// d_msvs
//   struct: one file read.  The text is the caller's, borrowed while the
//   records are used.  An XML file has `elements`, the root first; a
//   solution, of either form, has `projects`, `folders`, `dependencies` and
//   `configurations` -- each a "Configuration|Platform" the solution builds,
//   which a .slnx naming platforms alone builds as Debug and Release.
//   A .sln has `sections` too, and `global_start`, `global_close` and
//   `global_end` bound its Global ... EndGlobal lines as a section's are
//   bound, D_MSVS_NONE when it has none.  `bom` and `crlf` say how the text
//   begins and ends its lines.  When reading fails, `error_line` and
//   `error_column` say where.
struct d_msvs
{
    const char*             text;
    size_t                  length;
    enum d_msvs_kind        kind;
    bool                    xml;
    bool                    bom;
    bool                    crlf;
    struct d_msvs_element*  elements;
    size_t                  element_count;
    struct d_msvs_project*  projects;
    size_t                  project_count;
    struct d_msvs_folder*   folders;
    size_t                  folder_count;
    struct d_msvs_section*  sections;
    size_t                  section_count;
    size_t                  global_start;
    size_t                  global_close;
    size_t                  global_end;
    char**                  dependencies;
    size_t                  dependency_count;
    char**                  configurations;
    size_t                  configuration_count;
    size_t                  error_line;
    size_t                  error_column;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Reading
//------------------------------------------------------------------------------
// d_msvs_kind_of names a file's kind by its name, in any case.
enum d_msvs_kind d_msvs_kind_of(const char* _path);
/**
 * @brief Reads a Visual Studio file's text into records.
 *
 * @note A text that begins, after any byte-order mark and space, with `<`
 *       is read as XML: it must be well formed, with one root; a solution's
 *       root must be <Solution>, and an MSBuild file's <Project>.  Any
 *       other text must be a .sln, and begin "Microsoft Visual Studio
 *       Solution File".
 *
 * @param[out] _out       receives the records; release it with
 *                        d_msvs_release, whether or not reading succeeded.
 * @param[in]  _text      the text, borrowed for as long as `_out` is used.
 * @param[in]  _length    its length in bytes.
 * @param[in]  _kind      the file's kind, from d_msvs_kind_of; D_MSVS_OTHER
 *                        takes it from the text.
 * @param[out] _problem   receives why reading failed, as
 *                        "line L, column C: ...".
 * @param[in]  _capacity  the size of `_problem`.
 * @return `true` if the text was read whole; `false` otherwise.
 */
bool             d_msvs_read(struct d_msvs*   _out,
                             const char*      _text,
                             size_t           _length,
                             enum d_msvs_kind _kind,
                             char*            _problem,
                             size_t           _capacity);
void             d_msvs_release(struct d_msvs* _msvs);

// 2.2    Elements
//------------------------------------------------------------------------------
// d_msvs_is compares an element's name, which XML compares exactly.
// d_msvs_attribute finds an attribute by its name and gives its value raw,
// between the quotes; d_msvs_value gives an attribute's value decoded, and
// d_msvs_text an element's text decoded -- its content, when it holds no
// element -- each into a buffer the caller sizes as the raw text and one
// more, returning the length written.  Decoding is XML's: line endings are
// LF, and a value's tabs and line endings spaces, before references are
// read, so a reference to either is kept.
bool             d_msvs_is(const struct d_msvs*         _msvs,
                           const struct d_msvs_element* _element,
                           const char*                  _name);
bool             d_msvs_attribute(const struct d_msvs*         _msvs,
                                  const struct d_msvs_element* _element,
                                  const char*                  _name,
                                  size_t*                      _out_start,
                                  size_t*                      _out_length);
size_t           d_msvs_value(const struct d_msvs* _msvs,
                              size_t               _start,
                              size_t               _length,
                              char*                _out);
size_t           d_msvs_text(const struct d_msvs*         _msvs,
                             const struct d_msvs_element* _element,
                             char*                        _out);
// d_msvs_child is the first child of `_element` named `_name`, or
// D_MSVS_NONE; d_msvs_is_name is whether a text is an XML name.
size_t           d_msvs_child(const struct d_msvs* _msvs,
                              size_t               _element,
                              const char*          _name);
bool             d_msvs_is_name(const char* _text,
                                size_t      _length);

// 2.3    Conditions
//------------------------------------------------------------------------------
/**
 * @brief Reads what a Condition tests.
 *
 * @note Understood: '$(Configuration)|$(Platform)'=='C|P', and
 *       '$(Configuration)'=='C' and '$(Platform)'=='P' alone, with any
 *       spacing, either side of the `==` -- what Visual Studio and CMake
 *       write.  `_out_tested` receives "C|P", or "C|*" or "*|P" for one
 *       tested alone.
 *
 * @param[in]  _condition  the condition, decoded; may be `NULL`.
 * @param[out] _out_tested receives the configuration it tests, terminated,
 *                         for D_MSVS_CONFIGURATION; may be `NULL`.
 * @param[in]  _capacity   the size of `_out_tested`.
 * @return what the condition tests.
 */
enum d_msvs_condition d_msvs_condition_of(const char* _condition,
                                          char*       _out_tested,
                                          size_t      _capacity);
// d_msvs_config_matches is whether a configuration a condition tests --
// "Debug|x64", "Debug|*", "*|x64" -- holds for one a caller names,
// "Debug|x64", comparing without case, as MSBuild does.
bool             d_msvs_config_matches(const char* _tested,
                                       const char* _config);
// d_msvs_compare_paths orders two files' names as Visual Studio and MSBuild
// compare them -- in any case, with `/` and `\` alike -- below 0, 0 or
// above, as strcmp does.
int              d_msvs_compare_paths(const char* _left,
                                      const char* _right);

// 2.4    Writing
//------------------------------------------------------------------------------
/**
 * @brief Makes a GUID from a name: the same name gives the same GUID, every
 *        time and everywhere.
 *
 * @note A name-based GUID, of RFC 9562's version 5: SHA-1 over a namespace
 *       of djinterp's own and the name.  A project's id, a filter's, a
 *       solution's, made from a name that is its own, stays the same each
 *       time a file is made again.
 *
 * @param[in]  _seed    the name.
 * @param[in]  _length  its length in bytes.
 * @param[out] _out     receives the GUID, lower case and without braces, as
 *                      "xxxxxxxx-xxxx-5xxx-yxxx-xxxxxxxxxxxx", terminated:
 *                      37 bytes.
 */
void             d_msvs_guid(const char* _seed,
                             size_t      _length,
                             char*       _out);
/**
 * @brief Writes a path relative to a directory, as Visual Studio writes
 *        one: ..\..\inc\x.h.
 *
 * @note Both are paths from one place -- the root of a tree, or the root of
 *       the file system -- with `/` or `\` between their parts; `.` parts
 *       are dropped and `..` parts taken back.  Parts compare without case,
 *       as Windows compares them.  A path to the directory itself is ".".
 *
 * @param[in] _directory  the directory the path is to be read from.
 * @param[in] _path       the path.
 * @param[in] _separator  `\` for an MSBuild file or a .sln, `/` for a
 *                        .slnx.
 * @post   The caller frees the path.
 * @return the relative path, allocated; `NULL` when memory runs out, or
 *         when the path climbs above where the two begin.
 */
char*            d_msvs_relative(const char* _directory,
                                 const char* _path,
                                 char        _separator);
/**
 * @brief Escapes text for XML: `&`, `<` and `>` everywhere, a carriage
 *        return everywhere, and in an attribute's value the quote around
 *        it and the tab and line feed XML would otherwise make spaces.
 *
 * @param[in]  _text    the text.
 * @param[in]  _length  its length.
 * @param[in]  _quote   `"` or `'` for an attribute's value, quoted so;
 *                      '\0' for an element's text.
 * @param[out] _out     receives the escaped text, terminated; room for six
 *                      bytes for each of the text's, and one more.
 * @return the length written.
 */
size_t           d_msvs_escape(const char* _text,
                               size_t      _length,
                               char        _quote,
                               char*       _out);


#endif  // DJINTERP_TOOLS_DAWK_EXT_MSVS_DMSVS_H
