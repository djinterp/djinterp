/*******************************************************************************
* djinterp [cpp][interp]                                            evaluate.hpp
*
* The command language run: a script's term folded into what it comes to
* (docs/djinterp/interp/DESIGN.md, sections 3.3 and 5). The interpreter's
* skeleton, its first step: sequences and pipelines.
*   A value is a tagged value (decision 4). Until the tagged value's header
* lands, on 12-04, `value` here is its stand-in, with the same tags:
* nothing, a truth value, an integer, a real, text, bytes, a list and a
* record. A command's status goes beside its value, in an `outcome`: 0 done
* and yes, 1 done and no, 2 trouble (GRAMMAR.md, section 8).
*   What each construct of this step comes to:
*
*     a ; b    a, then b, whatever a came to; b's value and status
*     a | b    b takes a's value as its input; the first stage in trouble
*              ends the pipeline with its outcome, the stages after it not
*              run; a stage that comes to no (1) does not end it
*     cmd      the command its verb names, given its words as `argv`, the
*              verb first, and the input; a verb no command has is trouble
*
* The term is walked with core v2's d_parse_expr_walk, a frame for each
* node entered, as section 3.3 has it: a pipeline's stages after one in
* trouble are skipped by the walk, not run and ignored. A sequence and the
* first stage of a pipeline are given the input the construct was given.
*
* HOW TO USE
* ----------
*   Add each command to a `commands` table, a name and a function; read a
* script (language.hpp); then evaluate it with the table and an input --
* nothing, or the text standard input gave the program.
*
* OPTIONS
* -------
*   None yet: the commands are the caller's table. The registry's djins
* resolve them in a later step (DESIGN.md, section 6).
*
* CONFIGURATION
* -------------
*   C++11, the spine's floor: below it the header is empty.
*
*
* path:      /inc/djinterp/cpp/modules/interp/evaluate.hpp
* link(s):   /docs/djinterp/interp/DESIGN.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.22
*                                                            revised: 2026.11.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VALUES
    ------
    1.  value
2.  OUTCOMES
    --------
    1.  outcome
    2.  status constants
3.  COMMANDS
    --------
    1.  command_fn
    2.  commands
4.  EVALUATING
    ----------
    1.  evaluate
*/

#ifndef DJINTERP_INTERP_EVALUATE_HPP
#define DJINTERP_INTERP_EVALUATE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error, as
// the spine is.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <functional>  // std::function
#include <map>         // std::map
#include <string>      // std::string
#include <utility>     // std::pair
#include <vector>      // std::vector
// djinterp
#include "../../djinterp.hpp"  // framework root
#include "./language.hpp"      // script
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::int64_t


NS_DJINTERP
NS_INTERP


//==============================================================================
// 1.  VALUES
//==============================================================================

// 1.1
// value
//   class: the tagged value's stand-in: a tag, and the payload the tag
// says. A list holds values; a record holds text keys, in the order they
// were put, each with its value.
class value
{
public:
    enum kind_t
    {
        nothing = 0,
        truth   = 1,
        integer = 2,
        real    = 3,
        text    = 4,
        bytes   = 5,
        list    = 6,
        record  = 7
    };

    value();

    static value of_truth(bool _truth);
    static value of_integer(re_std::int64_t _integer);
    static value of_real(double _real);
    static value of_text(const std::string& _text);
    static value of_bytes(const std::string& _bytes);
    static value of_list(const std::vector<value>& _items);
    static value of_record(
        const std::vector<std::pair<std::string, value> >& _fields);

    kind_t                                            kind() const;
    bool                                              as_truth() const;
    re_std::int64_t                                   as_integer() const;
    double                                            as_real() const;
    const std::string&                                as_text() const;
    const std::vector<value>&                         items() const;
    const std::vector<std::pair<std::string, value> >& fields() const;

    // equal by tag and payload, a record's fields in order
    bool operator==(const value& _other) const;
    bool operator!=(const value& _other) const;

    // the value as the suites write it: nothing, true, 3, 2.5, "text",
    // bytes(3), [1, 2], {k: v}
    std::string show() const;

private:
    kind_t                                     m_kind;
    bool                                       m_truth;
    re_std::int64_t                            m_integer;
    double                                     m_real;
    std::string                                m_text;
    std::vector<value>                         m_items;
    std::vector<std::pair<std::string, value> > m_fields;
};


//==============================================================================
// 2.  OUTCOMES
//==============================================================================

// 2.1
// outcome
//   struct: what a construct came to: its value, its status, and, for
// trouble, what went wrong, with the span of the construct it went wrong
// in when the interpreter knows it.
struct outcome
{
    value            result;
    int              status;
    std::string      message;
    re_std::uint32_t offset;
    re_std::uint32_t length;
};

// 2.2
// status constants
//   constant: a status, as GRAMMAR.md's section 8 gives it.
static const int status_yes     = 0;
static const int status_no      = 1;
static const int status_trouble = 2;


//==============================================================================
// 3.  COMMANDS
//==============================================================================

// 3.1
// command_fn
//   type: a command: its words, the verb first, and its input; what it
// came to. One that throws is trouble, its exception's words the message.
typedef std::function<outcome(const std::vector<std::string>& _argv,
                              const value&                    _input)>
    command_fn;

// 3.2
// commands
//   class: the commands a script may run, by the verb's name.
class commands
{
public:
    // adds or replaces the command of a name
    void add(const std::string& _name, command_fn _run);
    // the command of a name, or NULL
    const command_fn* find(const std::string& _name) const;

private:
    std::map<std::string, command_fn> m_table;
};


//==============================================================================
// 4.  EVALUATING
//==============================================================================

// 4.1
// evaluate
//   running: a script folded, from its root, with the commands and the
// input; a script that was not read without errors is trouble, its first
// error the message.
outcome evaluate(const script&   _script,
                 const commands& _commands,
                 const value&    _input = value());


NS_END  // interp
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_INTERP_EVALUATE_HPP
