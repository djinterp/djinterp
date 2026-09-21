/******************************************************************************
* djinterp [dawk]                                                       dawk.h
*
*   Embedding and extension interface for the dawk interpreter.
*     dawk is a POSIX awk implementation exposed as a library.  Section 4
* defines the host hooks through which a tree-structured layer supplies its
* own record source, field splitter, patterns, statements and builtins
* without modifying the conforming core.
*
*
* path:      /inc/djinterp/tools/dawk/dawk.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.18
*                                                          revised: 2026.09.18
******************************************************************************/
/*
TABLE OF CONTENTS
=================
1.  TYPES AND STATUS
    ----------------
    1.  Opaque types
         1.  d_awk
         2.  d_awk_value
    2.  Status and conformance
         1.  d_awk_status
         2.  d_awk_conformance
2.  LIFECYCLE
    ---------
    1.  Creation and teardown
    2.  Compilation
    3.  Execution
    4.  Diagnostics
3.  VALUES
    ------
    1.  Value tags
         1.  d_awk_value_tag
    2.  Construction
    3.  Inspection
4.  HOST EXTENSION INTERFACE
    ------------------------
    1.  Hook types
         1.  d_awk_fn_read_record
         2.  d_awk_fn_split_record
         3.  d_awk_fn_to_string
         4.  d_awk_fn_match_pattern
         5.  d_awk_fn_parse_pattern
         6.  d_awk_fn_exec_statement
         7.  d_awk_fn_builtin
    2.  Host descriptor
         1.  d_awk_host
    3.  Registration
*/

#ifndef DJINTERP_TOOLS_DAWK_DAWK_H
#define DJINTERP_TOOLS_DAWK_DAWK_H 1

// std
#include <stdbool.h>    // bool
#include <stddef.h>     // size_t
#include <stdint.h>     // int64_t
// djinterp
#include "../djinterp.h"  // framework root


//==============================================================================
// 1.  TYPES AND STATUS
//==============================================================================
// Opaque interpreter and value handles, and the status and conformance
// enumerations shared by every entry point below.


// 1.1    Opaque types
//------------------------------------------------------------------------------
// 1.1.1
// d_awk
//   struct: an interpreter instance; owns the compiled program, the global
//   namespace, the open streams, and the registered host descriptor.
struct d_awk;

// 1.1.2
// d_awk_value
//   struct: one awk scalar or array cell.  Layout is private; use the
//   accessors in section 3.
struct d_awk_value;

// 1.2    Status and conformance
//------------------------------------------------------------------------------
// 1.2.1
// d_awk_status
//   enum: result of any operation that can fail.
enum d_awk_status
{
    D_AWK_OK = 0,
    D_AWK_ERR_SYNTAX,
    D_AWK_ERR_RUNTIME,
    D_AWK_ERR_IO,
    D_AWK_ERR_MEMORY,
    D_AWK_ERR_UNSUPPORTED
};

// 1.2.2
// d_awk_conformance
//   enum: selects how far the interpreter may depart from POSIX.  Under
//   D_AWK_STRICT_POSIX every hook in section 4 is refused at registration,
//   so the core remains independently verifiable against the conformance
//   suites no matter what a host layer later adds.
enum d_awk_conformance
{
    D_AWK_STRICT_POSIX = 0,
    D_AWK_HOST_EXTENSIONS
};


//==============================================================================
// 2.  LIFECYCLE
//==============================================================================
// Creation, compilation, execution and error reporting.  An interpreter may
// be compiled once and run many times; globals persist across runs unless
// the caller resets them.


// 2.1    Creation and teardown
//------------------------------------------------------------------------------
struct d_awk*     d_awk_new(void);
void              d_awk_free(struct d_awk* _awk);
enum d_awk_status d_awk_set_conformance(struct d_awk*            _awk,
                                        enum d_awk_conformance   _level);

// 2.2    Compilation
//------------------------------------------------------------------------------
enum d_awk_status d_awk_compile(struct d_awk* _awk,
                                const char*   _program,
                                const char*   _origin);
enum d_awk_status d_awk_compile_file(struct d_awk* _awk,
                                     const char*   _path);

// 2.3    Execution
//------------------------------------------------------------------------------
enum d_awk_status d_awk_run(struct d_awk*     _awk,
                            int               _argc,
                            char* const*      _argv);
enum d_awk_status d_awk_set_var(struct d_awk* _awk,
                                const char*   _name,
                                const char*   _value);
int               d_awk_exit_status(const struct d_awk* _awk);

// 2.4    Diagnostics
//------------------------------------------------------------------------------
const char*       d_awk_error_message(const struct d_awk* _awk);
const char*       d_awk_error_origin(const struct d_awk* _awk);
size_t            d_awk_error_line(const struct d_awk* _awk);
size_t            d_awk_error_column(const struct d_awk* _awk);


//==============================================================================
// 3.  VALUES
//==============================================================================
// awk's scalar model, plus the one tag a host layer adds.  Every builtin
// reaches a value through d_awk_value_to_string, so an extern value behaves
// as its own text everywhere the language already works.


// 3.1    Value tags
//------------------------------------------------------------------------------
// 3.1.1
// d_awk_value_tag
//   enum: discriminates the scalar model.  D_AWK_VAL_STRNUM is awk's
//   string-that-looks-numeric, which compares numerically; a string constant
//   of identical text does not.  D_AWK_VAL_EXTERN is the host seam: an
//   opaque pointer whose text is materialized lazily by the host.
enum d_awk_value_tag
{
    D_AWK_VAL_UNINIT = 0,
    D_AWK_VAL_STRING,
    D_AWK_VAL_NUMBER,
    D_AWK_VAL_STRNUM,
    D_AWK_VAL_ARRAY,
    D_AWK_VAL_EXTERN
};

// 3.2    Construction
//------------------------------------------------------------------------------
struct d_awk_value* d_awk_value_new_string(struct d_awk* _awk,
                                           const char*   _text,
                                           size_t        _length);
struct d_awk_value* d_awk_value_new_number(struct d_awk* _awk,
                                           double        _number);
struct d_awk_value* d_awk_value_new_extern(struct d_awk* _awk,
                                           void*         _object,
                                           void*         _user);

// 3.3    Inspection
//------------------------------------------------------------------------------
enum d_awk_value_tag d_awk_value_tag_of(const struct d_awk_value* _value);
const char*          d_awk_value_to_string(struct d_awk*       _awk,
                                           struct d_awk_value* _value,
                                           size_t*             _out_length);
double               d_awk_value_to_number(struct d_awk*       _awk,
                                           struct d_awk_value* _value);
void*                d_awk_value_object(const struct d_awk_value* _value);


//==============================================================================
// 4.  HOST EXTENSION INTERFACE
//==============================================================================
// Six hooks, each inert under D_AWK_STRICT_POSIX.  A host layer supplies a
// record source in place of RS splitting and a field splitter in place of FS
// splitting; because a tree traversal is just a record source that yields
// nodes rather than lines, the interpreter's main loop is identical in both
// modes and NR is the traversal ordinal either way.


// 4.1    Hook types
//------------------------------------------------------------------------------
// 4.1.1
// d_awk_fn_read_record
//   type: yields the next record.  Returns D_AWK_OK with `_out` set, or
//   sets `_out` to NULL at end of input.
typedef enum d_awk_status
(*d_awk_fn_read_record)(struct d_awk*        _awk,
                        void*                _user,
                        struct d_awk_value** _out);

// 4.1.2
// d_awk_fn_split_record
//   type: splits a record into fields.  A tree host returns the node's
//   children, so $1..$NF and field access are one operation.
typedef enum d_awk_status
(*d_awk_fn_split_record)(struct d_awk*        _awk,
                         void*                _user,
                         struct d_awk_value*  _record,
                         struct d_awk_value** _out_fields,
                         size_t*              _out_count);

// 4.1.3
// d_awk_fn_to_string
//   type: materializes the text of a D_AWK_VAL_EXTERN value.  The returned
//   buffer must remain valid until the value is released.
typedef const char*
(*d_awk_fn_to_string)(struct d_awk* _awk,
                      void*         _user,
                      void*         _object,
                      size_t*       _out_length);

// 4.1.4
// d_awk_fn_parse_pattern
//   type: consumes a host pattern beginning at `_source`, returning an
//   opaque handle and the number of bytes taken.  Invoked only when the
//   awk grammar rejects the leading token.
typedef enum d_awk_status
(*d_awk_fn_parse_pattern)(struct d_awk* _awk,
                          void*         _user,
                          const char*   _source,
                          size_t        _length,
                          void**        _out_pattern,
                          size_t*       _out_consumed);

// 4.1.5
// d_awk_fn_match_pattern
//   type: tests a host pattern against the current record.
typedef bool
(*d_awk_fn_match_pattern)(struct d_awk*       _awk,
                          void*               _user,
                          void*               _pattern,
                          struct d_awk_value* _record);

// 4.1.6
// d_awk_fn_exec_statement
//   type: executes a host statement parsed into `_statement`.
typedef enum d_awk_status
(*d_awk_fn_exec_statement)(struct d_awk*       _awk,
                           void*               _user,
                           void*               _statement,
                           struct d_awk_value* _record);

// 4.1.7
// d_awk_fn_builtin
//   type: a host-supplied builtin function.
typedef enum d_awk_status
(*d_awk_fn_builtin)(struct d_awk*        _awk,
                    void*                _user,
                    struct d_awk_value** _args,
                    size_t               _count,
                    struct d_awk_value** _out);

// 4.2    Host descriptor
//------------------------------------------------------------------------------
// 4.2.1
// d_awk_host
//   struct: the hooks a layer installs.  Any member may be NULL, in which
//   case the interpreter keeps its POSIX behavior for that seam.
struct d_awk_host
{
    void*                   user;           // opaque, passed to every hook
    d_awk_fn_read_record    read_record;    // replaces RS splitting
    d_awk_fn_split_record   split_record;   // replaces FS splitting
    d_awk_fn_to_string      to_string;      // text of an extern value
    d_awk_fn_parse_pattern  parse_pattern;  // additional pattern syntax
    d_awk_fn_match_pattern  match_pattern;  // evaluation of the above
    d_awk_fn_exec_statement exec_statement; // additional statement syntax
};

// 4.3    Registration
//------------------------------------------------------------------------------
enum d_awk_status d_awk_set_host(struct d_awk*             _awk,
                                 const struct d_awk_host*  _host);
enum d_awk_status d_awk_add_builtin(struct d_awk*     _awk,
                                    const char*       _name,
                                    size_t            _min_args,
                                    size_t            _max_args,
                                    d_awk_fn_builtin  _fn);
enum d_awk_status d_awk_claim_separator(struct d_awk* _awk,
                                        const char*   _name,
                                        bool          _is_record_separator);


#endif  // DJINTERP_TOOLS_DAWK_DAWK_H
