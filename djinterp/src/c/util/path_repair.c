#include "..\..\..\..\inc\djinterp\c\util\path_repair.h"


#ifndef D_PATH_REPAIR_PATH_MAX
    #define D_PATH_REPAIR_PATH_MAX 4096
#endif

#ifndef D_PATH_REPAIR_LIST_INITIAL_CAPACITY
    #define D_PATH_REPAIR_LIST_INITIAL_CAPACITY 32
#endif

#ifndef D_PATH_REPAIR_BUFFER_INITIAL_CAPACITY
    #define D_PATH_REPAIR_BUFFER_INITIAL_CAPACITY 4096
#endif


/*******************************************************************************
* I.    Internal types
*******************************************************************************/

// d_path_repair_string_list
//   struct: growable array of owned C strings used during
// directory traversal.
struct d_path_repair_string_list
{
    char**  items;
    size_t  count;
    size_t  capacity;
};

// d_path_repair_collect_context
//   struct: visitor context for collecting target files from
// the file tree.
struct d_path_repair_collect_context
{
    const struct d_path_repair_config* config;
    struct d_path_repair_string_list*  paths;
    const struct d_file_tree*          tree;
};

// d_path_repair_basename_build_context
//   struct: visitor context for building the basename lookup
// table from the file tree.
struct d_path_repair_basename_build_context
{
    struct d_path_repair_index* index;
    const struct d_file_tree*   tree;
};


/*******************************************************************************
* II.   Internal utility forward declarations
*******************************************************************************/

static char*       d_path_repair_strdup(const char* _text);
static bool        d_path_repair_is_directory(const char* _path);
static bool        d_path_repair_is_regular_file(const char* _path);
static bool        d_path_repair_is_hidden_name(const char* _name);
static void        d_path_repair_normalize_slashes(char* _path);
static void        d_path_repair_join_paths(char*       _out,
                                            size_t      _out_size,
                                            const char* _left,
                                            const char* _right);
static const char* d_path_repair_basename_of(const char* _path);
static bool        d_path_repair_has_extension(const char* _path,
                                               const char* _extension);
static bool        d_path_repair_make_relative_path(const char* _from_dir,
                                                    const char* _to_path,
                                                    char*       _out,
                                                    size_t      _out_size);
static bool        d_path_repair_read_file(const char* _path,
                                           char**      _contents);
static bool        d_path_repair_write_file(const char* _path,
                                            const char* _contents);
static bool        d_path_repair_append_bytes(char**      _buffer,
                                              size_t*     _size,
                                              size_t*     _capacity,
                                              const char* _bytes,
                                              size_t      _count);
static void        d_path_repair_log(
                       const struct d_path_repair_config* _config,
                       enum d_path_repair_log_mode        _minimum,
                       const char*                        _message);


/*******************************************************************************
* III.  Internal list helper forward declarations
*******************************************************************************/

static bool d_path_repair_string_list_push(
                struct d_path_repair_string_list* _list,
                const char*                       _text);
static void d_path_repair_string_list_free(
                struct d_path_repair_string_list* _list);
static bool d_path_repair_candidate_list_push(
                struct d_path_repair_candidate_list* _candidates,
                size_t                               _start_offset,
                size_t                               _end_offset,
                size_t                               _line,
                size_t                               _column,
                const char*                          _matched_text,
                const char*                          _extracted_path);
static bool d_path_repair_resolution_list_push(
                struct d_path_repair_resolution_list* _resolutions,
                bool                                  _is_valid,
                bool                                  _was_fixed,
                bool                                  _was_ambiguous,
                const char*                           _original_path,
                const char*                           _normalized_path,
                const char*                           _repaired_path);
static bool d_path_repair_index_push_basename(
                struct d_path_repair_index* _index,
                const char*                 _basename,
                const char*                 _relative_path);


/*******************************************************************************
* IV.   Internal extraction and resolution forward declarations
*******************************************************************************/

static bool   d_path_repair_extract_prefix_suffix_candidates(
                  const struct d_path_repair_config*  _config,
                  const char*                         _file_contents,
                  struct d_path_repair_candidate_list* _candidates);
static bool   d_path_repair_resolve_one_path(
                  const struct d_path_repair_config* _config,
                  const struct d_path_repair_index*  _index,
                  const char*                        _current_file_path,
                  const char*                        _original_path,
                  bool*                              _is_valid,
                  bool*                              _was_fixed,
                  bool*                              _was_ambiguous,
                  char**                             _normalized_path,
                  char**                             _repaired_path);
static size_t d_path_repair_count_duplicate_basenames(
                  const struct d_path_repair_index* _index);
static int    d_path_repair_compare_basename_entry(const void* _a,
                                                   const void* _b);

/*******************************************************************************
* V.    Internal visitor callbacks
*******************************************************************************/

static void d_path_repair_collect_visitor(
                struct d_file_tree_node* _node,
                size_t                   _depth,
                void*                    _context);
static void d_path_repair_basename_build_visitor(
                struct d_file_tree_node* _node,
                size_t                   _depth,
                void*                    _context);


/*******************************************************************************
* VI.   Internal utility implementations
*******************************************************************************/

/*
d_path_repair_strdup
  Portable string duplication.

Parameter(s):
  _text: the string to duplicate; must not be NULL.
Return:
  A newly allocated copy, or NULL on failure.
*/
static char*
d_path_repair_strdup
(
    const char* _text
)
{
    size_t len;
    char*  copy;

    if (!_text)
    {
        return NULL;
    }

    len  = strlen(_text);
    copy = (char*)malloc(len + 1);

    if (!copy)
    {
        return NULL;
    }

    memcpy(copy, _text, len + 1);

    return copy;
}


/*
d_path_repair_is_directory
  Returns true if the path refers to a directory.

Parameter(s):
  _path: filesystem path to check.
Return:
  true if the path is a directory; otherwise false.
*/
static bool
d_path_repair_is_directory
(
    const char* _path
)
{
    if (!_path)
    {
        return false;
    }

    return (d_is_dir(_path) != 0);
}


/*
d_path_repair_is_regular_file
  Returns true if the path refers to a regular file.

Parameter(s):
  _path: filesystem path to check.
Return:
  true if the path is a regular file; otherwise false.
*/
static bool
d_path_repair_is_regular_file
(
    const char* _path
)
{
    if (!_path)
    {
        return false;
    }

    return (d_is_file(_path) != 0);
}


/*
d_path_repair_is_hidden_name
  Returns true if a filename is hidden (starts with '.').

Parameter(s):
  _name: the filename component to check.
Return:
  true if the name starts with '.'; otherwise false.
*/
static bool
d_path_repair_is_hidden_name
(
    const char* _name
)
{
    if ( (!_name) ||
         (_name[0] == '\0') )
    {
        return false;
    }

    return (_name[0] == '.');
}


/*
d_path_repair_normalize_slashes
  Converts all backslashes to forward slashes in-place.

Parameter(s):
  _path: the path string to normalize; may be NULL.
Return:
  none.
*/
static void
d_path_repair_normalize_slashes
(
    char* _path
)
{
    if (!_path)
    {
        return;
    }

    while (*_path)
    {
        if (*_path == '\\')
        {
            *_path = '/';
        }

        _path++;
    }

    return;
}


/*
d_path_repair_join_paths
  Joins two path components with the platform separator.

Parameter(s):
  _out:      output buffer.
  _out_size: size of the output buffer.
  _left:     left path component.
  _right:    right path component.
Return:
  none.
*/
static void
d_path_repair_join_paths
(
    char*       _out,
    size_t      _out_size,
    const char* _left,
    const char* _right
)
{
    size_t left_len;

    if ( (!_out)    ||
         (!_left)   ||
         (!_right)  ||
         (_out_size == 0) )
    {
        return;
    }

    left_len = strlen(_left);

    // check if left already ends with a separator
    if ( (left_len > 0) &&
         (_left[left_len - 1] != '/') &&
         (_left[left_len - 1] != '\\') )
    {
        snprintf(_out, _out_size, "%s/%s", _left, _right);
    }
    else
    {
        snprintf(_out, _out_size, "%s%s", _left, _right);
    }

    return;
}


/*
d_path_repair_basename_of
  Returns a pointer to the filename component of a path.

Parameter(s):
  _path: the full path.
Return:
  A pointer into _path at the basename, or _path itself if no separator
is found.
*/
static const char*
d_path_repair_basename_of
(
    const char* _path
)
{
    const char* last_sep;
    const char* p;

    if (!_path)
    {
        return NULL;
    }

    last_sep = NULL;

    for (p = _path; *p; p++)
    {
        if ( (*p == '/') ||
             (*p == '\\') )
        {
            last_sep = p;
        }
    }

    if (last_sep)
    {
        return last_sep + 1;
    }

    return _path;
}


/*
d_path_repair_has_extension
  Returns true if the path ends with the given file extension.

Parameter(s):
  _path:      the file path to check.
  _extension: the extension to match (e.g. ".h"). If NULL, every file
              matches.
Return:
  true if the path has the given extension; otherwise false.
*/
static bool
d_path_repair_has_extension
(
    const char* _path,
    const char* _extension
)
{
    size_t path_len;
    size_t ext_len;

    if (!_path)
    {
        return false;
    }

    // NULL extension means "match all"
    if (!_extension)
    {
        return true;
    }

    path_len = strlen(_path);
    ext_len  = strlen(_extension);

    if (ext_len > path_len)
    {
        return false;
    }

    return (strcmp(_path + path_len - ext_len, _extension) == 0);
}


/*
d_path_repair_make_relative_path
  Computes a relative path from _from_dir to _to_path by consuming common
leading components, then emitting "../" for each remaining component in
_from_dir, followed by the remainder of _to_path.

Parameter(s):
  _from_dir: the directory to compute relative to.
  _to_path:  the absolute target path.
  _out:      output buffer for the relative path.
  _out_size: size of the output buffer.
Return:
  true on success; otherwise false.
*/
static bool
d_path_repair_make_relative_path
(
    const char* _from_dir,
    const char* _to_path,
    char*       _out,
    size_t      _out_size
)
{
    char   from_copy[D_PATH_REPAIR_PATH_MAX];
    char   to_copy[D_PATH_REPAIR_PATH_MAX];
    char*  from_parts[256];
    char*  to_parts[256];
    size_t from_count;
    size_t to_count;
    size_t common;
    size_t i;
    size_t offset;
    char*  token;
    char*  saveptr;

    if ( (!_from_dir) ||
         (!_to_path)  ||
         (!_out)      ||
         (_out_size == 0) )
    {
        return false;
    }

    snprintf(from_copy, sizeof(from_copy), "%s", _from_dir);
    snprintf(to_copy, sizeof(to_copy), "%s", _to_path);

    d_path_repair_normalize_slashes(from_copy);
    d_path_repair_normalize_slashes(to_copy);

    // tokenize both paths
    from_count = 0;
    saveptr    = NULL;
    token      = d_strtok_r(from_copy, "/", &saveptr);

    while (token && (from_count < 256))
    {
        from_parts[from_count] = token;
        from_count++;
        token = d_strtok_r(NULL, "/", &saveptr);
    }

    to_count = 0;
    saveptr  = NULL;
    token    = d_strtok_r(to_copy, "/", &saveptr);

    while (token && (to_count < 256))
    {
        to_parts[to_count] = token;
        to_count++;
        token = d_strtok_r(NULL, "/", &saveptr);
    }

    // find common prefix length
    common = 0;

    while ( (common < from_count) &&
            (common < to_count)   &&
            (strcmp(from_parts[common], to_parts[common]) == 0) )
    {
        common++;
    }

    // build relative path
    _out[0] = '\0';
    offset  = 0;

    // emit ../ for each remaining from component
    for (i = common; i < from_count; i++)
    {
        if (offset + 3 >= _out_size)
        {
            return false;
        }

        memcpy(_out + offset, "../", 3);
        offset += 3;
    }

    // emit remaining to components
    for (i = common; i < to_count; i++)
    {
        size_t part_len;

        part_len = strlen(to_parts[i]);

        if (i > common)
        {
            if (offset + 1 >= _out_size)
            {
                return false;
            }

            _out[offset] = '/';
            offset++;
        }

        if (offset + part_len >= _out_size)
        {
            return false;
        }

        memcpy(_out + offset, to_parts[i], part_len);
        offset += part_len;
    }

    _out[offset] = '\0';

    // edge case: empty result means same directory
    if (offset == 0)
    {
        snprintf(_out, _out_size, ".");
    }

    return true;
}


/*
d_path_repair_read_file
  Reads an entire file into memory as a null-terminated string.

Parameter(s):
  _path:     path of the file to read.
  _contents: output pointer receiving the allocated file contents.
Return:
  true on success; otherwise false.
*/
static bool
d_path_repair_read_file
(
    const char* _path,
    char**      _contents
)
{
    FILE*  file;
    long   file_size;
    char*  buffer;

    if ( (!_path) ||
         (!_contents) )
    {
        return false;
    }

    file = fopen(_path, "rb");

    if (!file)
    {
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);

        return false;
    }

    file_size = ftell(file);

    if (file_size < 0)
    {
        fclose(file);

        return false;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);

        return false;
    }

    buffer = (char*)malloc((size_t)file_size + 1);

    if (!buffer)
    {
        fclose(file);

        return false;
    }

    if (fread(buffer, 1, (size_t)file_size, file) != (size_t)file_size)
    {
        free(buffer);
        fclose(file);

        return false;
    }

    fclose(file);

    buffer[file_size] = '\0';
    *_contents        = buffer;

    return true;
}


/*
d_path_repair_write_file
  Writes a null-terminated string to a file, replacing its contents.

Parameter(s):
  _path:     path of the file to write.
  _contents: the null-terminated text to write.
Return:
  true on success; otherwise false.
*/
static bool
d_path_repair_write_file
(
    const char* _path,
    const char* _contents
)
{
    FILE*  file;
    size_t len;
    size_t written;

    if ( (!_path) ||
         (!_contents) )
    {
        return false;
    }

    file = fopen(_path, "wb");

    if (!file)
    {
        return false;
    }

    len     = strlen(_contents);
    written = fwrite(_contents, 1, len, file);

    fclose(file);

    return (written == len);
}


/*
d_path_repair_append_bytes
  Appends a block of bytes to a growable output buffer.

Parameter(s):
  _buffer:   pointer to the destination buffer pointer.
  _size:     current number of bytes in the buffer.
  _capacity: current allocated capacity of the buffer.
  _bytes:    bytes to append.
  _count:    number of bytes to append.
Return:
  true on success; otherwise false.
*/
static bool
d_path_repair_append_bytes
(
    char**      _buffer,
    size_t*     _size,
    size_t*     _capacity,
    const char* _bytes,
    size_t      _count
)
{
    char*  new_buffer;
    size_t new_capacity;

    if ( (!_buffer)   ||
         (!_size)     ||
         (!_capacity) ||
         (!_bytes) )
    {
        return false;
    }

    if ((*_size + _count + 1) > *_capacity)
    {
        new_capacity = (*_capacity == 0)
                           ? D_PATH_REPAIR_BUFFER_INITIAL_CAPACITY
                           : *_capacity;

        while ((*_size + _count + 1) > new_capacity)
        {
            new_capacity *= 2;
        }

        new_buffer = (char*)realloc(*_buffer, new_capacity);

        if (!new_buffer)
        {
            return false;
        }

        *_buffer   = new_buffer;
        *_capacity = new_capacity;
    }

    memcpy(*_buffer + *_size, _bytes, _count);
    *_size += _count;
    (*_buffer)[*_size] = '\0';

    return true;
}


/*
d_path_repair_log
  Writes a diagnostic message to stderr if the current log mode meets
the minimum threshold.

Parameter(s):
  _config:  the repair configuration (for log_mode).
  _minimum: the minimum log mode required to emit this message.
  _message: the message to print.
Return:
  none.
*/
static void
d_path_repair_log
(
    const struct d_path_repair_config* _config,
    enum d_path_repair_log_mode        _minimum,
    const char*                        _message
)
{
    if ( (!_config) ||
         (!_message) )
    {
        return;
    }

    if (_config->log_mode >= _minimum)
    {
        fprintf(stderr, "%s\n", _message);
    }

    return;
}


/*******************************************************************************
* VII.  Internal list helper implementations
*******************************************************************************/

/*
d_path_repair_string_list_push
  Appends a copy of the given string to a growable string list.

Parameter(s):
  _list: the string list to append to.
  _text: the string to copy and append.
Return:
  true on success; otherwise false.
*/
static bool
d_path_repair_string_list_push
(
    struct d_path_repair_string_list* _list,
    const char*                       _text
)
{
    char** new_items;
    size_t new_capacity;
    char*  copy;

    if ( (!_list) ||
         (!_text) )
    {
        return false;
    }

    if (_list->count >= _list->capacity)
    {
        new_capacity = (_list->capacity == 0)
                           ? D_PATH_REPAIR_LIST_INITIAL_CAPACITY
                           : _list->capacity * 2;

        new_items = (char**)realloc(_list->items,
                                    new_capacity * sizeof(char*));

        if (!new_items)
        {
            return false;
        }

        _list->items    = new_items;
        _list->capacity = new_capacity;
    }

    copy = d_path_repair_strdup(_text);

    if (!copy)
    {
        return false;
    }

    _list->items[_list->count] = copy;
    _list->count++;

    return true;
}


/*
d_path_repair_string_list_free
  Frees all strings and the array in a string list.

Parameter(s):
  _list: the string list to free.
Return:
  none.
*/
static void
d_path_repair_string_list_free
(
    struct d_path_repair_string_list* _list
)
{
    size_t i;

    if (!_list)
    {
        return;
    }

    for (i = 0; i < _list->count; i++)
    {
        free(_list->items[i]);
    }

    free(_list->items);

    _list->items    = NULL;
    _list->count    = 0;
    _list->capacity = 0;

    return;
}


/*
d_path_repair_candidate_list_push
  Appends a new candidate entry to the candidate list.

Parameter(s):
  _candidates:    the list to append to.
  _start_offset:  byte offset where the matched text begins.
  _end_offset:    byte offset where the matched text ends.
  _line:          one-based line number.
  _column:        one-based column number.
  _matched_text:  the full text that matched the pattern.
  _extracted_path: the relative path extracted from the match.
Return:
  true on success; otherwise false.
*/
static bool
d_path_repair_candidate_list_push
(
    struct d_path_repair_candidate_list* _candidates,
    size_t                               _start_offset,
    size_t                               _end_offset,
    size_t                               _line,
    size_t                               _column,
    const char*                          _matched_text,
    const char*                          _extracted_path
)
{
    struct d_path_repair_candidate* new_items;
    struct d_path_repair_candidate* entry;
    size_t                          new_capacity;

    if (!_candidates)
    {
        return false;
    }

    if (_candidates->count >= _candidates->capacity)
    {
        new_capacity = (_candidates->capacity == 0)
                           ? D_PATH_REPAIR_LIST_INITIAL_CAPACITY
                           : _candidates->capacity * 2;

        new_items = (struct d_path_repair_candidate*)realloc(
                        _candidates->items,
                        new_capacity *
                            sizeof(struct d_path_repair_candidate));

        if (!new_items)
        {
            return false;
        }

        _candidates->items    = new_items;
        _candidates->capacity = new_capacity;
    }

    entry = &_candidates->items[_candidates->count];

    entry->start_offset  = _start_offset;
    entry->end_offset    = _end_offset;
    entry->line          = _line;
    entry->column        = _column;
    entry->matched_text  = _matched_text
                               ? d_path_repair_strdup(_matched_text)
                               : NULL;
    entry->extracted_path = _extracted_path
                               ? d_path_repair_strdup(_extracted_path)
                               : NULL;

    _candidates->count++;

    return true;
}


/*
d_path_repair_resolution_list_push
  Appends a new resolution entry to the resolution list.

Parameter(s):
  _resolutions:    the list to append to.
  _is_valid:       whether the path was already valid.
  _was_fixed:      whether a repair was found.
  _was_ambiguous:  whether duplicate basenames prevented repair.
  _original_path:  the original extracted path text.
  _normalized_path: the slash-normalized version.
  _repaired_path:  the computed repair, or NULL.
Return:
  true on success; otherwise false.
*/
static bool
d_path_repair_resolution_list_push
(
    struct d_path_repair_resolution_list* _resolutions,
    bool                                  _is_valid,
    bool                                  _was_fixed,
    bool                                  _was_ambiguous,
    const char*                           _original_path,
    const char*                           _normalized_path,
    const char*                           _repaired_path
)
{
    struct d_path_repair_resolution* new_items;
    struct d_path_repair_resolution* entry;
    size_t                           new_capacity;

    if (!_resolutions)
    {
        return false;
    }

    if (_resolutions->count >= _resolutions->capacity)
    {
        new_capacity = (_resolutions->capacity == 0)
                           ? D_PATH_REPAIR_LIST_INITIAL_CAPACITY
                           : _resolutions->capacity * 2;

        new_items = (struct d_path_repair_resolution*)realloc(
                        _resolutions->items,
                        new_capacity *
                            sizeof(struct d_path_repair_resolution));

        if (!new_items)
        {
            return false;
        }

        _resolutions->items    = new_items;
        _resolutions->capacity = new_capacity;
    }

    entry = &_resolutions->items[_resolutions->count];

    entry->is_valid        = _is_valid;
    entry->was_fixed       = _was_fixed;
    entry->was_ambiguous   = _was_ambiguous;
    entry->original_path   = _original_path
                                 ? d_path_repair_strdup(_original_path)
                                 : NULL;
    entry->normalized_path = _normalized_path
                                 ? d_path_repair_strdup(_normalized_path)
                                 : NULL;
    entry->repaired_path   = _repaired_path
                                 ? d_path_repair_strdup(_repaired_path)
                                 : NULL;

    _resolutions->count++;

    return true;
}


/*
d_path_repair_index_push_basename
  Appends a basename/relative-path pair to the index's lookup table.

Parameter(s):
  _index:         the repair index to append to.
  _basename:      the filename component.
  _relative_path: the subtree-relative path to the file.
Return:
  true on success; otherwise false.
*/
static bool
d_path_repair_index_push_basename
(
    struct d_path_repair_index* _index,
    const char*                 _basename,
    const char*                 _relative_path
)
{
    struct d_path_repair_basename_entry* new_items;
    struct d_path_repair_basename_entry* entry;
    size_t                               new_capacity;

    if ( (!_index)    ||
         (!_basename) )
    {
        return false;
    }

    if (_index->basename_count >= _index->basename_capacity)
    {
        new_capacity = (_index->basename_capacity == 0)
                           ? D_PATH_REPAIR_LIST_INITIAL_CAPACITY
                           : _index->basename_capacity * 2;

        new_items = (struct d_path_repair_basename_entry*)realloc(
                        _index->basenames,
                        new_capacity *
                            sizeof(struct d_path_repair_basename_entry));

        if (!new_items)
        {
            return false;
        }

        _index->basenames        = new_items;
        _index->basename_capacity = new_capacity;
    }

    entry = &_index->basenames[_index->basename_count];

    entry->basename      = d_path_repair_strdup(_basename);
    entry->relative_path = _relative_path
                               ? d_path_repair_strdup(_relative_path)
                               : NULL;

    if (!entry->basename)
    {
        return false;
    }

    _index->basename_count++;

    return true;
}


/*******************************************************************************
* VIII. Internal visitor callbacks
*******************************************************************************/

/*
d_path_repair_collect_visitor
  File tree visitor that collects absolute paths of files matching the
configured extension filter.

Parameter(s):
  _node:    the current tree node.
  _depth:   depth of the node in the tree.
  _context: a d_path_repair_collect_context pointer.
Return:
  none.
*/
static void
d_path_repair_collect_visitor
(
    struct d_file_tree_node* _node,
    size_t                   _depth,
    void*                    _context
)
{
    struct d_path_repair_collect_context* ctx;
    struct d_string*                      rel_path;
    char                                  abs_path[D_PATH_REPAIR_PATH_MAX];
    const char*                           name_cstr;

    (void)_depth;

    if ( (!_node) ||
         (!_context) )
    {
        return;
    }

    ctx = (struct d_path_repair_collect_context*)_context;

    // skip directories
    if (_node->type == D_FILE_NODE_TYPE_DIR)
    {
        return;
    }

    // skip hidden files unless configured
    if (_node->name)
    {
        name_cstr = d_string_cstr(_node->name);

        if ( (d_path_repair_is_hidden_name(name_cstr)) &&
             (!(ctx->config->flags & D_PATH_REPAIR_FLAG_INCLUDE_HIDDEN)) )
        {
            return;
        }
    }

    // check extension filter
    if (ctx->config->file_extension)
    {
        name_cstr = d_string_cstr(_node->name);

        if (!d_path_repair_has_extension(name_cstr,
                                         ctx->config->file_extension))
        {
            return;
        }
    }

    // build absolute path from tree-relative path
    rel_path = d_file_tree_node_path(ctx->tree, _node);

    if (!rel_path)
    {
        return;
    }

    d_path_repair_join_paths(abs_path,
                             sizeof(abs_path),
                             ctx->config->root_path,
                             d_string_cstr(rel_path));

    d_path_repair_string_list_push(ctx->paths, abs_path);

    d_string_free(rel_path);

    return;
}


/*
d_path_repair_basename_build_visitor
  File tree visitor that populates the basename lookup table in the repair
index. Only visits file nodes (not directories).

Parameter(s):
  _node:    the current tree node.
  _depth:   depth of the node in the tree.
  _context: a d_path_repair_basename_build_context pointer.
Return:
  none.
*/
static void
d_path_repair_basename_build_visitor
(
    struct d_file_tree_node* _node,
    size_t                   _depth,
    void*                    _context
)
{
    struct d_path_repair_basename_build_context* ctx;
    struct d_string*                             rel_path;
    const char*                                  name_cstr;

    (void)_depth;

    if ( (!_node) ||
         (!_context) )
    {
        return;
    }

    ctx = (struct d_path_repair_basename_build_context*)_context;

    // only index file and symlink nodes, not directories
    if (_node->type == D_FILE_NODE_TYPE_DIR)
    {
        return;
    }

    if (!_node->name)
    {
        return;
    }

    name_cstr = d_string_cstr(_node->name);
    rel_path  = d_file_tree_node_path(ctx->tree, _node);

    if (!rel_path)
    {
        return;
    }

    d_path_repair_index_push_basename(ctx->index,
                                      name_cstr,
                                      d_string_cstr(rel_path));

    d_string_free(rel_path);

    return;
}


/*******************************************************************************
* IX.   Internal extraction and resolution implementations
*******************************************************************************/

/*
d_path_repair_extract_prefix_suffix_candidates
  Scans file contents for candidate relative paths delimited by a prefix
and/or suffix. Tracks line and column numbers for each match.

Parameter(s):
  _config:        the repair configuration.
  _file_contents: the source text to scan.
  _candidates:    output list of candidates.
Return:
  true on success; otherwise false.
*/
static bool
d_path_repair_extract_prefix_suffix_candidates
(
    const struct d_path_repair_config*  _config,
    const char*                         _file_contents,
    struct d_path_repair_candidate_list* _candidates
)
{
    const char* cursor;
    const char* prefix;
    const char* suffix;
    size_t      prefix_len;
    size_t      suffix_len;
    size_t      line;
    size_t      column;
    size_t      line_start_offset;
    const char* match_start;
    const char* path_start;
    const char* path_end;
    size_t      path_len;
    char        path_buf[D_PATH_REPAIR_PATH_MAX];
    char        match_buf[D_PATH_REPAIR_PATH_MAX];

    if ( (!_config)        ||
         (!_file_contents) ||
         (!_candidates) )
    {
        return false;
    }

    prefix = _config->prefix;
    suffix = _config->suffix;

    prefix_len = prefix ? strlen(prefix) : 0;
    suffix_len = suffix ? strlen(suffix) : 0;

    // prefix mode requires a prefix
    if ( (_config->match_mode & D_PATH_REPAIR_MATCH_PREFIX) &&
         (prefix_len == 0) )
    {
        return true;
    }

    cursor            = _file_contents;
    line              = 1;
    column            = 1;
    line_start_offset = 0;

    while (*cursor)
    {
        // track line numbers
        if (*cursor == '\n')
        {
            line++;
            column            = 1;
            line_start_offset = (size_t)(cursor - _file_contents) + 1;
            cursor++;
            continue;
        }

        // look for prefix match
        if ( (prefix_len > 0) &&
             (strncmp(cursor, prefix, prefix_len) == 0) )
        {
            match_start = cursor;
            path_start  = cursor + prefix_len;

            // find the end of the path: either at suffix or at a
            // whitespace/control character
            if (suffix_len > 0)
            {
                path_end = strstr(path_start, suffix);

                if (!path_end)
                {
                    // no closing suffix found; skip this prefix
                    column++;
                    cursor++;
                    continue;
                }
            }
            else
            {
                // no suffix: consume until whitespace or end
                path_end = path_start;

                while ( (*path_end) &&
                        (!isspace((unsigned char)*path_end)) )
                {
                    path_end++;
                }
            }

            path_len = (size_t)(path_end - path_start);

            if ( (path_len > 0) &&
                 (path_len < D_PATH_REPAIR_PATH_MAX) )
            {
                size_t match_len;
                size_t match_end_offset;

                memcpy(path_buf, path_start, path_len);
                path_buf[path_len] = '\0';

                match_len = (size_t)(path_end + suffix_len - match_start);

                if (match_len < D_PATH_REPAIR_PATH_MAX)
                {
                    memcpy(match_buf, match_start, match_len);
                    match_buf[match_len] = '\0';
                }
                else
                {
                    match_buf[0] = '\0';
                }

                match_end_offset =
                    (size_t)(path_end + suffix_len - _file_contents);

                d_path_repair_candidate_list_push(
                    _candidates,
                    (size_t)(path_start - _file_contents),
                    (size_t)(path_end - _file_contents),
                    line,
                    (size_t)(path_start - _file_contents)
                        - line_start_offset + 1,
                    match_buf,
                    path_buf);
            }

            // advance past this match
            cursor = path_end + suffix_len;
            column = (size_t)(cursor - _file_contents)
                         - line_start_offset + 1;
            continue;
        }

        column++;
        cursor++;
    }

    return true;
}


/*
d_path_repair_resolve_one_path
  Validates or repairs one extracted relative path. First checks whether
the normalized path already resolves to a file relative to the current
file's directory. If not, searches the basename lookup table for a unique
match and computes the corrected relative path.

Parameter(s):
  _config:            repair configuration.
  _index:             the repair index (tree + basename table).
  _current_file_path: absolute path of the file being processed.
  _original_path:     extracted relative path text.
  _is_valid:          output: set to true if the path is already valid.
  _was_fixed:         output: set to true if a repair was computed.
  _was_ambiguous:     output: set to true if duplicates blocked repair.
  _normalized_path:   output: allocated normalized version of the path.
  _repaired_path:     output: allocated repaired path, or NULL.
Return:
  true if resolution completed (even if no fix was found); false on
parameter or allocation errors.
*/
static bool
d_path_repair_resolve_one_path
(
    const struct d_path_repair_config* _config,
    const struct d_path_repair_index*  _index,
    const char*                        _current_file_path,
    const char*                        _original_path,
    bool*                              _is_valid,
    bool*                              _was_fixed,
    bool*                              _was_ambiguous,
    char**                             _normalized_path,
    char**                             _repaired_path
)
{
    char        normalized[D_PATH_REPAIR_PATH_MAX];
    char        current_dir[D_PATH_REPAIR_PATH_MAX];
    char        candidate_absolute[D_PATH_REPAIR_PATH_MAX];
    const char* basename;
    size_t      i;
    size_t      match_count;
    ssize_t     matched_index;

    if ( (!_config)            ||
         (!_index)             ||
         (!_current_file_path) ||
         (!_original_path)     ||
         (!_is_valid)          ||
         (!_was_fixed)         ||
         (!_was_ambiguous)     ||
         (!_normalized_path)   ||
         (!_repaired_path) )
    {
        return false;
    }

    *_is_valid        = false;
    *_was_fixed       = false;
    *_was_ambiguous   = false;
    *_normalized_path = NULL;
    *_repaired_path   = NULL;

    snprintf(normalized,
             sizeof(normalized),
             "%s",
             _original_path);

    d_path_repair_normalize_slashes(normalized);

    *_normalized_path = d_path_repair_strdup(normalized);

    if (!*_normalized_path)
    {
        return false;
    }

    // get the containing directory of the current file
    snprintf(current_dir,
             sizeof(current_dir),
             "%s",
             _current_file_path);

    {
        char* last_slash;
        char* last_backslash;
        char* last_separator;

        last_slash     = strrchr(current_dir, '/');
        last_backslash = strrchr(current_dir, '\\');
        last_separator = last_slash;

        if ( (last_backslash) &&
             ( (!last_separator) ||
               (last_backslash > last_separator) ) )
        {
            last_separator = last_backslash;
        }

        if (last_separator)
        {
            *last_separator = '\0';
        }
        else
        {
            current_dir[0] = '.';
            current_dir[1] = '\0';
        }
    }

    // check if the normalized path is already valid
    d_path_repair_join_paths(candidate_absolute,
                             sizeof(candidate_absolute),
                             current_dir,
                             normalized);

    if (d_path_repair_is_regular_file(candidate_absolute))
    {
        *_is_valid = true;

        return true;
    }

    // basename-based repair against the sorted lookup table
    basename      = d_path_repair_basename_of(normalized);
    match_count   = 0;
    matched_index = -1;

    for (i = 0; i < _index->basename_count; i++)
    {
        if (strcmp(_index->basenames[i].basename, basename) == 0)
        {
            match_count++;
            matched_index = (ssize_t)i;
        }
    }

    if (match_count == 0)
    {
        // no file with this basename in the subtree
        return true;
    }

    if (match_count > 1)
    {
        *_was_ambiguous = true;

        return true;
    }

    // exactly one match: compute relative path from current dir to target
    {
        char target_absolute[D_PATH_REPAIR_PATH_MAX];
        char relative_result[D_PATH_REPAIR_PATH_MAX];

        d_path_repair_join_paths(
            target_absolute,
            sizeof(target_absolute),
            _config->root_path,
            _index->basenames[matched_index].relative_path);

        if (!d_path_repair_make_relative_path(current_dir,
                                              target_absolute,
                                              relative_result,
                                              sizeof(relative_result)))
        {
            return true;
        }

        if (_config->flags & D_PATH_REPAIR_FLAG_NORMALIZE_SLASHES)
        {
            d_path_repair_normalize_slashes(relative_result);
        }

        *_repaired_path = d_path_repair_strdup(relative_result);

        if (!*_repaired_path)
        {
            return false;
        }

        *_was_fixed = true;
    }

    return true;
}


/*
d_path_repair_count_duplicate_basenames
  Counts how many basenames appear more than once in the sorted lookup
table.

Parameter(s):
  _index: the repair index.
Return:
  The number of distinct basenames that have duplicates.
*/
static size_t
d_path_repair_count_duplicate_basenames
(
    const struct d_path_repair_index* _index
)
{
    size_t duplicates;
    size_t i;
    bool   in_run;

    if ( (!_index) ||
         (_index->basename_count < 2) )
    {
        return 0;
    }

    duplicates = 0;
    in_run     = false;

    for (i = 1; i < _index->basename_count; i++)
    {
        if (strcmp(_index->basenames[i].basename,
                  _index->basenames[i - 1].basename) == 0)
        {
            if (!in_run)
            {
                duplicates++;
                in_run = true;
            }
        }
        else
        {
            in_run = false;
        }
    }

    return duplicates;
}


/*
d_path_repair_compare_basename_entry
  qsort comparator for basename entries; sorts by basename
lexicographically.

Parameter(s):
  _a: pointer to the first d_path_repair_basename_entry.
  _b: pointer to the second d_path_repair_basename_entry.
Return:
  Negative, zero, or positive as strcmp.
*/
static int
d_path_repair_compare_basename_entry
(
    const void* _a,
    const void* _b
)
{
    const struct d_path_repair_basename_entry* a;
    const struct d_path_repair_basename_entry* b;

    a = (const struct d_path_repair_basename_entry*)_a;
    b = (const struct d_path_repair_basename_entry*)_b;

    return strcmp(a->basename, b->basename);
}


/*******************************************************************************
* X.    Public function implementations
*******************************************************************************/

/*
d_path_repair_config_is_valid
  Returns whether the supplied configuration is minimally valid.

Parameter(s):
  _config: the configuration to validate.
Return:
  true if the configuration is valid; otherwise false.
*/
bool
d_path_repair_config_is_valid
(
    const struct d_path_repair_config* _config
)
{
    if ( (!_config)            ||
         (!_config->root_path) ||
         (_config->root_path[0] == '\0') )
    {
        return false;
    }

    // must have at least one active match mechanism
    if (_config->match_mode == D_PATH_REPAIR_MATCH_NONE)
    {
        return false;
    }

    // prefix mode requires a non-NULL prefix
    if ( (_config->match_mode & D_PATH_REPAIR_MATCH_PREFIX) &&
         (!_config->prefix) )
    {
        if (!(_config->match_mode & D_PATH_REPAIR_MATCH_REGEX))
        {
            return false;
        }
    }

    // regex mode requires a pattern
    if ( (_config->match_mode & D_PATH_REPAIR_MATCH_REGEX) &&
         ( (!_config->regex_pattern) ||
           (_config->regex_pattern[0] == '\0') ) )
    {
        return false;
    }

    return true;
}


/*
d_path_repair_build_index
  Builds a file tree from the configured root path and populates the
sorted basename lookup table from it.

Parameter(s):
  _config: the repair configuration.
  _index:  output index structure.
Return:
  true on success; otherwise false.
*/
bool
d_path_repair_build_index
(
    const struct d_path_repair_config* _config,
    struct d_path_repair_index*        _index
)
{
    struct d_path_repair_basename_build_context ctx;
    bool                                       recursive;

    if ( (!_config) ||
         (!_index) )
    {
        return false;
    }

    _index->tree              = NULL;
    _index->basenames         = NULL;
    _index->basename_count    = 0;
    _index->basename_capacity = 0;

    recursive = (_config->flags & D_PATH_REPAIR_FLAG_RECURSIVE) != 0;

    // build the file tree from the filesystem in a single scan
    _index->tree = d_file_tree_new_from_path(_config->root_path,
                                             recursive);

    if (!_index->tree)
    {
        return false;
    }

    // populate the basename lookup table via tree traversal
    ctx.index = _index;
    ctx.tree  = _index->tree;

    d_file_tree_traverse_preorder(_index->tree,
                                  d_path_repair_basename_build_visitor,
                                  &ctx);

    // sort for efficient duplicate detection and lookup
    if (_index->basename_count > 1)
    {
        qsort(_index->basenames,
              _index->basename_count,
              sizeof(struct d_path_repair_basename_entry),
              d_path_repair_compare_basename_entry);
    }

    return true;
}


/*
d_path_repair_index_free
  Frees the file tree and basename table owned by an index.

Parameter(s):
  _index: the index to free.
Return:
  none.
*/
void
d_path_repair_index_free
(
    struct d_path_repair_index* _index
)
{
    size_t i;

    if (!_index)
    {
        return;
    }

    if (_index->tree)
    {
        d_file_tree_free(_index->tree);
        _index->tree = NULL;
    }

    for (i = 0; i < _index->basename_count; i++)
    {
        free(_index->basenames[i].basename);
        free(_index->basenames[i].relative_path);
    }

    free(_index->basenames);

    _index->basenames         = NULL;
    _index->basename_count    = 0;
    _index->basename_capacity = 0;

    return;
}


/*
d_path_repair_collect_target_files
  Collects the absolute paths of files to be scanned for candidate
relative paths. Uses the already-built file tree instead of re-walking
the filesystem.

Parameter(s):
  _config:     the repair configuration.
  _index:      the pre-built repair index.
  _files:      output array of allocated absolute path strings.
  _file_count: output number of collected files.
Return:
  true on success; otherwise false.
*/
bool
d_path_repair_collect_target_files
(
    const struct d_path_repair_config* _config,
    const struct d_path_repair_index*  _index,
    char***                            _files,
    size_t*                            _file_count
)
{
    struct d_path_repair_collect_context ctx;
    struct d_path_repair_string_list    paths;
    char**                              result;
    size_t                              i;

    if ( (!_config)     ||
         (!_index)      ||
         (!_files)      ||
         (!_file_count) )
    {
        return false;
    }

    paths.items    = NULL;
    paths.count    = 0;
    paths.capacity = 0;

    ctx.config = _config;
    ctx.paths  = &paths;
    ctx.tree   = _index->tree;

    d_file_tree_traverse_preorder(_index->tree,
                                  d_path_repair_collect_visitor,
                                  &ctx);

    // transfer ownership of the string array
    result = (char**)malloc(paths.count * sizeof(char*));

    if ( (!result) &&
         (paths.count > 0) )
    {
        d_path_repair_string_list_free(&paths);

        return false;
    }

    for (i = 0; i < paths.count; i++)
    {
        result[i] = paths.items[i];
    }

    // free only the outer array, not the strings (now owned by result)
    free(paths.items);

    *_files      = result;
    *_file_count = paths.count;

    return true;
}


/*
d_path_repair_free_file_list
  Frees a file list returned by d_path_repair_collect_target_files.

Parameter(s):
  _files:      file list to free.
  _file_count: number of entries.
Return:
  none.
*/
void
d_path_repair_free_file_list
(
    char** _files,
    size_t _file_count
)
{
    size_t i;

    if (!_files)
    {
        return;
    }

    for (i = 0; i < _file_count; i++)
    {
        free(_files[i]);
    }

    free(_files);

    return;
}


/*
d_path_repair_find_candidates
  Finds candidate relative file paths in a file's contents according
to the configured match mode.

Parameter(s):
  _config:        repair configuration.
  _file_contents: source text to scan.
  _candidates:    output list of candidate occurrences.
Return:
  true on success; otherwise false.
*/
bool
d_path_repair_find_candidates
(
    const struct d_path_repair_config*  _config,
    const char*                         _file_contents,
    struct d_path_repair_candidate_list* _candidates
)
{
    if ( (!_config)        ||
         (!_file_contents) ||
         (!_candidates) )
    {
        return false;
    }

    _candidates->items    = NULL;
    _candidates->count    = 0;
    _candidates->capacity = 0;

    if (_config->match_mode & D_PATH_REPAIR_MATCH_REGEX)
    {
        // regex backend reserved for future implementation
    }

    if ( (_config->match_mode & D_PATH_REPAIR_MATCH_PREFIX) ||
         (_config->match_mode & D_PATH_REPAIR_MATCH_SUFFIX) )
    {
        return d_path_repair_extract_prefix_suffix_candidates(
                   _config,
                   _file_contents,
                   _candidates);
    }

    return true;
}


/*
d_path_repair_candidate_list_free
  Frees all storage owned by a candidate list.

Parameter(s):
  _candidates: candidate list to free.
Return:
  none.
*/
void
d_path_repair_candidate_list_free
(
    struct d_path_repair_candidate_list* _candidates
)
{
    size_t i;

    if (!_candidates)
    {
        return;
    }

    for (i = 0; i < _candidates->count; i++)
    {
        free(_candidates->items[i].matched_text);
        free(_candidates->items[i].extracted_path);
    }

    free(_candidates->items);

    _candidates->items    = NULL;
    _candidates->count    = 0;
    _candidates->capacity = 0;

    return;
}


/*
d_path_repair_resolve_candidates
  Validates or resolves all extracted candidate relative paths for one
file.

Parameter(s):
  _config:            repair configuration.
  _index:             the repair index.
  _current_file_path: absolute path of the file being processed.
  _candidates:        extracted path candidates.
  _resolutions:       output list of resolutions.
Return:
  true on success; otherwise false.
*/
bool
d_path_repair_resolve_candidates
(
    const struct d_path_repair_config*         _config,
    const struct d_path_repair_index*           _index,
    const char*                                 _current_file_path,
    const struct d_path_repair_candidate_list*  _candidates,
    struct d_path_repair_resolution_list*        _resolutions
)
{
    size_t i;

    if ( (!_config)            ||
         (!_index)             ||
         (!_current_file_path) ||
         (!_candidates)        ||
         (!_resolutions) )
    {
        return false;
    }

    _resolutions->items    = NULL;
    _resolutions->count    = 0;
    _resolutions->capacity = 0;

    for (i = 0; i < _candidates->count; i++)
    {
        bool   is_valid;
        bool   was_fixed;
        bool   was_ambiguous;
        char*  normalized_path;
        char*  repaired_path;

        is_valid        = false;
        was_fixed       = false;
        was_ambiguous   = false;
        normalized_path = NULL;
        repaired_path   = NULL;

        if (!d_path_repair_resolve_one_path(
                 _config,
                 _index,
                 _current_file_path,
                 _candidates->items[i].extracted_path,
                 &is_valid,
                 &was_fixed,
                 &was_ambiguous,
                 &normalized_path,
                 &repaired_path))
        {
            free(normalized_path);
            free(repaired_path);

            return false;
        }

        if (!d_path_repair_resolution_list_push(
                 _resolutions,
                 is_valid,
                 was_fixed,
                 was_ambiguous,
                 _candidates->items[i].extracted_path,
                 normalized_path,
                 repaired_path))
        {
            free(normalized_path);
            free(repaired_path);

            return false;
        }

        free(normalized_path);
        free(repaired_path);
    }

    return true;
}


/*
d_path_repair_resolution_list_free
  Frees all storage owned by a resolution list.

Parameter(s):
  _resolutions: resolution list to free.
Return:
  none.
*/
void
d_path_repair_resolution_list_free
(
    struct d_path_repair_resolution_list* _resolutions
)
{
    size_t i;

    if (!_resolutions)
    {
        return;
    }

    for (i = 0; i < _resolutions->count; i++)
    {
        free(_resolutions->items[i].original_path);
        free(_resolutions->items[i].normalized_path);
        free(_resolutions->items[i].repaired_path);
    }

    free(_resolutions->items);

    _resolutions->items    = NULL;
    _resolutions->count    = 0;
    _resolutions->capacity = 0;

    return;
}


/*
d_path_repair_rewrite_contents
  Rewrites source text by replacing broken candidate paths with their
repaired paths. Unchanged candidates are copied verbatim.

Parameter(s):
  _file_contents:      original source text.
  _candidates:         candidate occurrences in source order.
  _resolutions:        corresponding resolutions.
  _rewritten_contents: output rewritten text.
  _was_changed:        output flag set if any replacement was made.
Return:
  true on success; otherwise false.
*/
bool
d_path_repair_rewrite_contents
(
    const char*                                 _file_contents,
    const struct d_path_repair_candidate_list*  _candidates,
    const struct d_path_repair_resolution_list* _resolutions,
    char**                                      _rewritten_contents,
    bool*                                       _was_changed
)
{
    char*  output;
    size_t output_size;
    size_t output_capacity;
    size_t cursor;
    size_t i;

    if ( (!_file_contents)      ||
         (!_candidates)         ||
         (!_resolutions)        ||
         (!_rewritten_contents) ||
         (!_was_changed) )
    {
        return false;
    }

    if (_candidates->count != _resolutions->count)
    {
        return false;
    }

    output          = NULL;
    output_size     = 0;
    output_capacity = 0;
    cursor          = 0;
    *_was_changed   = false;

    for (i = 0; i < _candidates->count; i++)
    {
        const struct d_path_repair_candidate*  candidate;
        const struct d_path_repair_resolution* resolution;
        const char*                            replacement;

        candidate   = &_candidates->items[i];
        resolution  = &_resolutions->items[i];
        replacement = candidate->extracted_path;

        if ( (resolution->was_fixed) &&
             (resolution->repaired_path) )
        {
            replacement   = resolution->repaired_path;
            *_was_changed = true;
        }
        else if ( (resolution->is_valid)         &&
                  (resolution->normalized_path)  &&
                  (strcmp(candidate->extracted_path,
                         resolution->normalized_path) != 0) )
        {
            replacement   = resolution->normalized_path;
            *_was_changed = true;
        }

        // copy text between previous cursor and this candidate
        if (!d_path_repair_append_bytes(&output,
                                        &output_size,
                                        &output_capacity,
                                        _file_contents + cursor,
                                        candidate->start_offset - cursor))
        {
            free(output);

            return false;
        }

        // write the replacement
        if (!d_path_repair_append_bytes(&output,
                                        &output_size,
                                        &output_capacity,
                                        replacement,
                                        strlen(replacement)))
        {
            free(output);

            return false;
        }

        cursor = candidate->end_offset;
    }

    // copy trailing text after the last candidate
    if (!d_path_repair_append_bytes(&output,
                                    &output_size,
                                    &output_capacity,
                                    _file_contents + cursor,
                                    strlen(_file_contents + cursor)))
    {
        free(output);

        return false;
    }

    *_rewritten_contents = output;

    return true;
}


/*
d_path_repair_process_file
  Runs the full candidate-extraction, resolution, and rewrite pipeline
for one file.

Parameter(s):
  _config:    repair configuration.
  _index:     the pre-built repair index.
  _file_path: absolute path of the file to process.
  _stats:     statistics accumulator.
Return:
  true on success; otherwise false.
*/
bool
d_path_repair_process_file
(
    const struct d_path_repair_config* _config,
    const struct d_path_repair_index*  _index,
    const char*                        _file_path,
    struct d_path_repair_stats*        _stats
)
{
    char*                                file_contents;
    char*                                rewritten_contents;
    bool                                 was_changed;
    struct d_path_repair_candidate_list  candidates;
    struct d_path_repair_resolution_list resolutions;
    size_t                               i;

    if ( (!_config)    ||
         (!_index)     ||
         (!_file_path) ||
         (!_stats) )
    {
        return false;
    }

    file_contents      = NULL;
    rewritten_contents = NULL;
    was_changed        = false;

    _stats->files_seen++;

    // read the source file
    if (!d_path_repair_read_file(_file_path, &file_contents))
    {
        return false;
    }

    // extract candidates
    if (!d_path_repair_find_candidates(_config,
                                       file_contents,
                                       &candidates))
    {
        free(file_contents);

        return false;
    }

    _stats->candidates_seen += candidates.count;

    // resolve candidates
    if (!d_path_repair_resolve_candidates(_config,
                                          _index,
                                          _file_path,
                                          &candidates,
                                          &resolutions))
    {
        d_path_repair_candidate_list_free(&candidates);
        free(file_contents);

        return false;
    }

    // accumulate statistics
    for (i = 0; i < resolutions.count; i++)
    {
        if (resolutions.items[i].was_fixed)
        {
            _stats->fixed_paths++;
        }
        else if (resolutions.items[i].is_valid)
        {
            _stats->valid_paths++;
        }
        else if (resolutions.items[i].was_ambiguous)
        {
            _stats->ambiguous_paths++;
        }
        else
        {
            _stats->unresolved_paths++;
        }
    }

    // rewrite
    if (!d_path_repair_rewrite_contents(file_contents,
                                         &candidates,
                                         &resolutions,
                                         &rewritten_contents,
                                         &was_changed))
    {
        d_path_repair_resolution_list_free(&resolutions);
        d_path_repair_candidate_list_free(&candidates);
        free(file_contents);

        return false;
    }

    if (was_changed)
    {
        _stats->files_changed++;

        if (_config->log_mode >= D_PATH_REPAIR_LOG_NORMAL)
        {
            fprintf(stderr, "modified: %s\n", _file_path);
        }

        if (!_config->dry_run)
        {
            if (!d_path_repair_write_file(_file_path,
                                          rewritten_contents))
            {
                free(rewritten_contents);
                d_path_repair_resolution_list_free(&resolutions);
                d_path_repair_candidate_list_free(&candidates);
                free(file_contents);

                return false;
            }
        }
    }

    free(rewritten_contents);
    d_path_repair_resolution_list_free(&resolutions);
    d_path_repair_candidate_list_free(&candidates);
    free(file_contents);

    return true;
}


/*
d_path_repair_run
  Runs the full directory-wide relative path repair pipeline: builds the
index once, collects target files from the tree, then processes each file.

Parameter(s):
  _config: repair configuration.
  _stats:  output statistics structure; may be NULL.
Return:
  true on success; otherwise false.
*/
bool
d_path_repair_run
(
    const struct d_path_repair_config* _config,
    struct d_path_repair_stats*        _stats
)
{
    struct d_path_repair_index index;
    char**                     files;
    size_t                     file_count;
    size_t                     i;
    char                       msg_buf[256];

    if (!d_path_repair_config_is_valid(_config))
    {
        return false;
    }

    if (_stats)
    {
        d_path_repair_stats_clear(_stats);
    }

    // single filesystem scan via file_tree
    if (!d_path_repair_build_index(_config, &index))
    {
        return false;
    }

    if (_stats)
    {
        _stats->duplicate_basenames =
            d_path_repair_count_duplicate_basenames(&index);

        if ( (_stats->duplicate_basenames > 0) &&
             (_config->log_mode >= D_PATH_REPAIR_LOG_NORMAL) )
        {
            snprintf(msg_buf,
                     sizeof(msg_buf),
                     "duplicate basenames ignored: %zu",
                     _stats->duplicate_basenames);

            d_path_repair_log(_config,
                              D_PATH_REPAIR_LOG_NORMAL,
                              msg_buf);
        }
    }

    // collect target files from the already-built tree
    if (!d_path_repair_collect_target_files(_config,
                                            &index,
                                            &files,
                                            &file_count))
    {
        d_path_repair_index_free(&index);

        return false;
    }

    // process each file
    for (i = 0; i < file_count; i++)
    {
        if (!d_path_repair_process_file(_config,
                                        &index,
                                        files[i],
                                        _stats))
        {
            if (_config->log_mode >= D_PATH_REPAIR_LOG_NORMAL)
            {
                fprintf(stderr, "failed: %s\n", files[i]);
            }
        }
    }

    d_path_repair_free_file_list(files, file_count);
    d_path_repair_index_free(&index);

    return true;
}

/*
d_path_repair_stats_clear
  Clears a statistics structure to zero.

Parameter(s):
  _stats: statistics structure to clear.
Return:
  none.
*/
void
d_path_repair_stats_clear
(
    struct d_path_repair_stats* _stats
)
{
    if (_stats)
    {
        memset(_stats, 0, sizeof(*_stats));
    }

    return;
}
