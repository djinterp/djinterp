/*******************************************************************************
* djinterp [tools]                                                     dinterp.h
*
*   Tree-walking interpreter for the syntax tree in dparse.h.
*     Fields are split lazily: a record is not divided until a field or NF is
* read, and $0 is not rebuilt until a field is written. A program that only
* prints whole records therefore never splits one.
*     Values are owned by their container and copied on assignment, which is
* what awk's scalar semantics already are. Arrays pass by reference, and only
* through function parameters.
*
*
* path:      /inc/djinterp/tools/dawk/dinterp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  d_awk_interp
2.  OPERATIONS
    ----------
    1.  Lifecycle
    2.  Configuration
    3.  Execution
    4.  Diagnostics
*/

#ifndef DJINTERP_TOOLS_DAWK_DINTERP_H
#define DJINTERP_TOOLS_DAWK_DINTERP_H 1

// std
#include <stdbool.h>   // bool
#include <stddef.h>    // size_t
// djinterp
#include "./dparse.h"  // d_awk_program


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Interpreter
//------------------------------------------------------------------------------
// 1.1.1
// d_awk_interp
//   struct: one interpreter over one parsed program.  Layout is private.
struct d_awk_interp;

// D_AWK_CALL_DEPTH_DEFAULT
//   constant: the default cap on nested user-function calls.  A deterministic
// limit: the same program fails at the same depth everywhere.
#define D_AWK_CALL_DEPTH_DEFAULT 512u

// D_AWK_STACK_BUDGET_DEFAULT
//   constant: the default stack budget in bytes.  The call count cannot bound
// the stack on its own -- a call costs from under 1 KB to nearly 4 KB of C
// stack depending on how deeply its body nests expressions -- so the running
// interpreter measures its own stack use against this.  The default leaves
// headroom under the platform's default main-thread stack: 1 MB on Windows,
// 8 MB on the others.  An embedder on a smaller thread must set it lower.
#if defined(_WIN32)
    #define D_AWK_STACK_BUDGET_DEFAULT 786432u
#else
    #define D_AWK_STACK_BUDGET_DEFAULT 6291456u
#endif

struct d_awk_source;
struct d_awk_host_function;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Lifecycle
//------------------------------------------------------------------------------
struct d_awk_interp* d_awk_interp_new(struct d_awk_program* _program);
void                 d_awk_interp_free(struct d_awk_interp* _interp);

// 2.2    Configuration
//------------------------------------------------------------------------------
// An assignment given with -v, or as a file operand of the form name=value,
// is input-derived text and therefore obeys the numeric string rule.
/**
 * @brief Applies a `name=value` assignment, as -v and an operand give one.
 *
 * @note The value's escape sequences are decoded as a string literal's are,
 *       as POSIX has it, so the operand `x='a\nz'` assigns a newline.
 *
 * @param[in,out] _interp  the interpreter.
 * @param[in]     _text    a name -- a letter or an underscore, then letters,
 *                         digits and underscores -- then `=` and the value.
 * @return `true` if the assignment was applied; `false` if `_text` is not an
 *         assignment, which makes an operand a file name, or if memory ran
 *         out.
 */
bool                 d_awk_interp_assign(struct d_awk_interp* _interp,
                                         const char*          _text);
bool                 d_awk_interp_add_input(struct d_awk_interp* _interp,
                                            const char*          _path);
bool                 d_awk_interp_set_environ(struct d_awk_interp* _interp,
                                              char* const*         _environ);

bool                 d_awk_interp_set_source(struct d_awk_interp* _interp,
                                             struct d_awk_source* _source);
bool                 d_awk_interp_set_limits(struct d_awk_interp* _interp,
                                             size_t               _call_depth,
                                             size_t               _stack_budget);
/**
 * @brief Adds an extension's host functions, which a program then calls by
 *        name as it calls its own.
 *
 * @note A function the program defines itself wins over a host function of
 *       the same name. Under D_AWK_STRICT_POSIX every call is refused.
 *
 * @param[in,out] _interp     the interpreter.
 * @param[in]     _functions  the descriptors, borrowed: they must outlive
 *                            the interpreter, as a static table does.
 * @param[in]     _count      how many descriptors there are.
 * @param[in]     _user       handed to every one of them when it is called.
 * @return `true` if every function was added; `false` if a name was taken
 *         already, a descriptor is incomplete, or memory ran out, with none
 *         of them added and the error recorded.
 */
bool                 d_awk_interp_add_functions(
                         struct d_awk_interp*              _interp,
                         const struct d_awk_host_function* _functions,
                         size_t                            _count,
                         void*                             _user);

// 2.3    Execution
//------------------------------------------------------------------------------
int                  d_awk_interp_run(struct d_awk_interp* _interp);

// 2.4    Diagnostics
//------------------------------------------------------------------------------
const char*          d_awk_interp_error(const struct d_awk_interp* _interp);


#endif  // DJINTERP_TOOLS_DAWK_DINTERP_H
