/*******************************************************************************
* bitwisedom [bitwisedom]                                               cenv.hpp
*
* The define environment: macros, as the preprocessor expands them, and
* integer constant expressions, as C and C++ evaluate them.
*   A macro row's value is whatever its name comes to in C. Macros are
* written in terms of other macros -- D_CODE_ARRAY_INDEX_HI is
* D_CODE_BLOCK_ARRAY | D_BYTE_BIT_R(4, 1), and D_BYTE_BIT_R is built on
* D_UINT_CAST -- so the value comes from an environment of definitions:
* the standard headers' (stdint.h, limits.h, stdbool.h), the sheet's own
* environment text, every macro row of the sheet, and the sheet's other
* symbols, which the environment asks for by name.
*   EXPANSION is the standard's, by Prosser's algorithm: hide sets, so a
* macro is never replaced within its own replacement; arguments expanded
* before substitution, except beside # and ##; placemarkers for empty
* arguments; __VA_ARGS__, __VA_OPT__ (C23 and C++20), and the `, ##
* __VA_ARGS__` every compiler accepts. Every token remembers the invocation
* it came out of, so a problem deep in an expansion is reported as where it
* happened: "D_BYTE_BIT_R(3, 0): shift count -4 is negative".
*   EVALUATION is typed. Literals take the types C gives them, casts and the
* fixed-width typedefs convert, and every operator applies the integer
* promotions and the usual arithmetic conversions of the sheet's language,
* level and data model. Undefined behaviour -- a negative or too-wide shift,
* signed overflow, division by zero -- is found and reported, and leaves no
* value; implementation-defined behaviour is noted, with the value every
* mainstream compiler gives. Unevaluated operands (the branch of ?: not
* taken, the right of a decided && or ||, sizeof's) are typed but not
* evaluated, so their problems are not problems.
*   NOTHING HERE THROWS.
*
* path:      /inc/djinterp/tools/bitwisedom/cenv.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.09
*                                                            revised: 2026.10.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VALUES
    ------
    1.  c_type
    2.  c_value
    3.  Diagnostics and results
2.  MACROS
    ------
    1.  c_macro
    2.  c_settings and c_scope
3.  THE ENVIRONMENT
    ---------------
    1.  c_env
    2.  Directives
4.  NAMES
    -----
    1.  Suggestions
*/

#ifndef BITWISEDOM_CENV_HPP
#define BITWISEDOM_CENV_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>      // std::size_t
#include <functional>   // std::function
#include <map>          // std::map
#include <memory>       // std::shared_ptr
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <utility>      // std::pair
#include <vector>       // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./bits.hpp"   // u64, i64
#include "./clex.hpp"   // c_token
#include "./sheet.hpp"  // standard, data_model


namespace bitwisedom
{


//==============================================================================
// 1.  VALUES
//==============================================================================


// 1.1    c_type
//------------------------------------------------------------------------------
// 1.1.1
// c_type
//   struct: an integer type. `rank` orders the standard types -- 0 bool, 1
// char, 2 short, 3 int, 4 long, 5 long long -- and with the signedness and
// the width (the data model's) is all arithmetic needs. `name` is how the
// type was spelled, kept for messages: a uint64_t says uint64_t, not
// unsigned long long.
struct c_type
{
    int         rank      = 3;
    bool        is_signed = true;
    int         bits      = 32;
    std::string name      = "int";

    D_NODISCARD bool same(const c_type& _other) const
    {
        return ( (rank == _other.rank) &&
                 (is_signed == _other.is_signed) );
    }
};

// 1.2    c_value
//------------------------------------------------------------------------------
// 1.2.1
// c_value
//   struct: a typed integer. `bits` holds the value in two's complement,
// sign-extended to 64 bits when the type is signed and zero-extended when it
// is not, so a signed value reads back with signed() and an unsigned one
// directly.
struct c_value
{
    c_type type;
    u64    bits = 0;

    D_NODISCARD i64 as_signed() const
    {
        return static_cast<i64>(bits);
    }

    D_NODISCARD bool negative() const
    {
        return ( (type.is_signed) &&
                 (static_cast<i64>(bits) < 0) );
    }
};

// 1.3    Diagnostics and results
//------------------------------------------------------------------------------
// 1.3.1
// c_level
//   enum: how much a diagnostic matters. A note is information -- an
// implementation-defined conversion, say; a warning is a problem the C
// itself has, undefined behaviour above all; an error stops evaluation.
enum class c_level : int
{
    note = 0,
    warning,
    error
};

// 1.3.2
// c_problem
//   enum: what kind of problem a diagnostic reports, for a caller that
// weighs some kinds differently.
enum class c_problem : int
{
    none = 0,
    syntax,          // the text is not an expression
    undefined_name,  // a name nothing defines
    arguments,       // a macro invoked with the wrong number of arguments
    expansion,       // ## made no token; a recursion; too much expansion
    not_integer,     // a string, a floating value, a pointer
    undefined,       // undefined behaviour
    implementation,  // implementation-defined behaviour
    style            // legal, but likely not what was meant
};

// 1.3.3
// c_diagnostic
//   struct: one thing evaluation found. `chain` is the invocations it was
// found inside, innermost first, each as it was written --
// "D_BYTE_BIT_R(3, 0)"; `column` is where it stands in the evaluated text,
// when it stands there at all (0 otherwise).
struct c_diagnostic
{
    c_level                  level   = c_level::error;
    c_problem                problem = c_problem::none;
    std::string              message;
    std::vector<std::string> chain;
    int                      line    = 0;
    int                      column  = 0;

    // text
    //   the message, prefixed with the chain: "A(1) in B: message".
    D_NODISCARD std::string text() const;
};

// 1.3.4
// c_result
//   struct: what evaluating a text came to. `ok` means a value was found;
// it may still carry notes and warnings. `undefined_behaviour` means the
// evaluation met undefined behaviour, and then there is no value.
// `expansion` is the text after macro expansion; `unknown` every name left
// that nothing defines.
struct c_result
{
    bool                      ok                  = false;
    c_value                   value;
    bool                      undefined_behaviour = false;
    std::vector<c_diagnostic> diagnostics;
    std::string               expansion;
    std::vector<std::string>  unknown;

    // first
    //   the first diagnostic of a level or above, or nullptr.
    D_NODISCARD const c_diagnostic* first(c_level _level) const;
};


//==============================================================================
// 2.  MACROS
//==============================================================================


// 2.1    c_macro
//------------------------------------------------------------------------------
// 2.1.1
// c_macro
//   struct: one definition. `params` excludes the ellipsis; a variadic
// macro has `variadic` set and reaches its extra arguments as __VA_ARGS__
// (or, written `name...`, by that name). `body` is the replacement as
// written. `origin` says where the definition came from, for messages.
struct c_macro
{
    std::string              name;
    bool                     function_like = false;
    std::vector<std::string> params;
    bool                     variadic      = false;
    std::string              variadic_name = "__VA_ARGS__";
    std::string              body;
    std::string              origin;

    // the body's tokens, read once at definition
    std::vector<c_token>     tokens;
};

// parameters -- a parameter list's text ("_bits, _value", "fmt, ...") read
// into a macro; false and why when it is not one
D_NODISCARD bool  read_params(const std::string& _text,
                              c_macro&           _macro,
                              std::string&       _error);
D_NODISCARD std::string  params_text(const c_macro& _macro);

// 2.2    c_settings and c_scope
//------------------------------------------------------------------------------
// 2.2.1
// c_settings
//   struct: the language, its level and the target's data model.
struct c_settings
{
    standard   level = standard::c99;
    data_model model = data_model::llp64;
};

// 2.2.2
// c_lookup
//   type: a name the environment has no macro for, offered to its owner. On
// success it sets the value and returns true; on failure it returns false,
// with an error when the name is known but has no value (a cycle, a row in
// error) and none when the name is not known at all.
using c_lookup = std::function<bool(const std::string& _name,
                                    c_value&           _value,
                                    std::string&       _error)>;

// 2.2.3
// c_scope
//   struct: what an evaluation may look up beyond the macros: the lookup,
// and the names it knows, for suggestions when a name is misspelled.
struct c_scope
{
    c_lookup                 lookup;
    std::vector<std::string> names;
};


//==============================================================================
// 3.  THE ENVIRONMENT
//==============================================================================


// 3.1    c_env
//------------------------------------------------------------------------------
// 3.1.1
// c_env
//   class: a table of macros at a language, level and data model.
class c_env
{
public:
    /**
     * @brief Makes an environment.
     *
     * @param[in] _settings  the language, level and data model.
     * @param[in] _standard  whether to begin with what the standard headers
     *                       and the compiler define: <stdint.h>, <limits.h>,
     *                       <stdbool.h> (before C23), <stddef.h>'s NULL,
     *                       and __STDC_VERSION__ or __cplusplus.
     */
    explicit c_env(const c_settings& _settings,
                   bool              _standard = true);

    D_NODISCARD const c_settings& settings() const;

    /**
     * @brief Defines a macro, replacing any definition of the same name.
     *
     * @param[in]  _macro  the definition; its body is scanned here.
     * @param[out] _error  why the definition is ill-formed: `#` before a
     *                     non-parameter, `##` at either end, a parameter
     *                     named twice.
     * @return `true` if it was defined; an ill-formed definition is not.
     */
    bool define(c_macro      _macro,
                std::string& _error);
    bool undefine(const std::string& _name);

    D_NODISCARD const c_macro*  find(const std::string& _name) const;
    D_NODISCARD bool            is_standard(const std::string& _name) const;
    D_NODISCARD std::vector<std::string>  names() const;

    /**
     * @brief Evaluates a text as an integer constant expression.
     *
     * @param[in] _text   the expression, macros and all.
     * @param[in] _scope  the names beyond the macros.
     * @return the value and its type, or why there is none.
     */
    D_NODISCARD c_result evaluate(std::string_view _text,
                                  const c_scope&   _scope) const;

    /**
     * @brief Evaluates a text as an #if condition.
     *
     * @note `defined X` and `defined(X)` are answered from the table, and a
     *       name left after expansion is 0, as the standard says; each such
     *       name is listed in `unknown`, so a caller can tell a condition
     *       that is false from one that only looks it.
     *
     * @param[in] _text  the condition.
     * @return the value, or why there is none.
     */
    D_NODISCARD c_result condition(std::string_view _text) const;

    // expansion alone -- the text with every macro replaced, for display
    D_NODISCARD std::string expand(std::string_view _text) const;

    /**
     * @brief Checks a definition without invoking it.
     *
     * @note Finds what any invocation would meet: a macro called in the
     *       body with the wrong number of arguments; a name in capitals that
     *       nothing defines, with what it might have meant; and, for an
     *       object-like macro, an operator at the top of the body, which
     *       binds to whatever surrounds the macro's use.
     *
     * @param[in] _macro  the definition, which need not be in the table.
     * @param[in] _scope  the names beyond the macros.
     * @return the diagnostics, warnings and notes.
     */
    D_NODISCARD std::vector<c_diagnostic> lint(const c_macro& _macro,
                                               const c_scope& _scope) const;

private:
    c_settings                    m_settings;
    std::map<std::string, c_macro> m_macros;
    std::vector<std::string>      m_standard;
};

// 3.2    Directives
//------------------------------------------------------------------------------
// 3.2.1
// c_branches
//   class: the conditionals a walk over a text is inside, and whether the
// branch it is in is taken. A condition that rests on a name nothing
// defines -- one not reserved to the implementation, which would be a
// compiler's own -- is taken as true, and a note says so: an environment
// is for evaluating, and the other headers a compiler would have read are
// not here.
class c_branches
{
public:
    explicit c_branches(const c_env& _env);

    /**
     * @brief Follows one directive, if it is a conditional.
     *
     * @param[in]  _line    the directive's tokens after the `#`: its name
     *                      first.
     * @param[in]  _number  its line, for notes.
     * @param[out] _notes   what was assumed, and what was ill-formed.
     * @return `true` if it was #if, #ifdef, #ifndef, #elif, #elifdef,
     *         #elifndef, #else or #endif.
     */
    bool feed(const std::vector<c_token>& _line,
              int                         _number,
              std::vector<std::string>&   _notes);

    D_NODISCARD bool         live() const;
    D_NODISCARD std::size_t  depth() const;

private:
    struct frame
    {
        bool outer = true;
        bool taken = false;
        bool done  = false;
    };

    const c_env&       m_env;
    std::vector<frame> m_stack;

    bool decide(const std::vector<c_token>& _line,
                int                         _number,
                std::vector<std::string>&   _notes) const;
};

// directives -- a text's directive lines (the line, and the tokens after
// the `#`), and a #define's read into a macro, with where its body begins
D_NODISCARD std::vector<std::pair<int, std::vector<c_token>>>
        directive_lines(const std::vector<c_token>& _tokens);
D_NODISCARD bool read_define(const std::vector<c_token>& _line,
                             c_macro&                    _macro,
                             std::size_t&                _body,
                             std::string&                _error);

/**
 * @brief Runs a text's directives into an environment.
 *
 * @note Reads #define and #undef, and the conditionals around them, the
 *       way a compiler would, but a condition that depends on a name
 *       nothing defines is taken as true and noted: the environment is
 *       for evaluating, and a compiler's other headers are not here. The
 *       rest of the text -- declarations, code -- is ignored.
 *
 * @param[in,out] _env    the environment.
 * @param[in]     _text   the text.
 * @param[out]    _notes  what was assumed or ignored, and what was wrong,
 *                        each with its line.
 * @return how many definitions were made.
 */
int  run_directives(c_env&                    _env,
                    std::string_view          _text,
                    std::vector<std::string>& _notes);


//==============================================================================
// 4.  NAMES
//==============================================================================


// 4.1    Suggestions
//------------------------------------------------------------------------------
/**
 * @brief The names a misspelt one probably meant.
 *
 * @note Names are compared a word at a time, words being the runs between
 *       underscores: a word may be abbreviated (IDX for INDEX), mistyped by
 *       a letter, or missing or extra; a word replaced by another is a
 *       different name, not a misspelling of it.
 *
 * @param[in] _name        the name nothing defines.
 * @param[in] _candidates  the names that are defined.
 * @return the best one or two, or none.
 */
D_NODISCARD std::vector<std::string> suggest(
    const std::string&              _name,
    const std::vector<std::string>& _candidates);

// type names -- a C type as written, and the standard types' names
D_NODISCARD std::string  type_text(const c_type& _type);

/**
 * @brief The typed value of an integer literal written a given way.
 *
 * @param[in] _value     the bits; for a negative literal, its two's
 *                       complement.
 * @param[in] _negative  whether it is written as a negative number, (-3).
 * @param[in] _decimal   whether it is written in decimal, which C types
 *                       differently from the other bases.
 * @param[in] _suffix    its suffix.
 * @param[in] _settings  the language, level and data model.
 * @return the value, with the type C gives it.
 */
D_NODISCARD c_value  constant_value(u64               _value,
                                    bool              _negative,
                                    bool              _decimal,
                                    int_suffix        _suffix,
                                    const c_settings& _settings);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_CENV_HPP
