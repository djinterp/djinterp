/*******************************************************************************
* djinterp [tools]                                                  cli_edit.hpp
*
* The command-line kit's C++ face: commands that change a file.
*   `cli::edit` is the dj_cli_edit record a command keeps, with the C calls of
* cli_edit.h as its members: the operand -- bound, if the command asks, to
* the std::string the command reads the file's name from -- and the options
* for where the result goes, added each where the command's other
* definitions put it; then, after the parse, where the result goes, whether
* `-n` was given, and the result written there, whole or not at all. The
* rule is the program's: by default (A) the file is written back where it
* was read, and a program made with program_flags::edits_to_output (B)
* writes standard output unless given `-i`.
*
*
* path:      /inc/djinterp/tools/cli/cli_edit.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.25
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Where the result goes
         1.  edit_place
2.  THE EDIT
    --------
    1.  The class
         1.  edit
3.  DEFINITIONS
    -----------
    1.  Making
    2.  Defining
    3.  Writing
*/

#ifndef DJINTERP_TOOLS_CLI_EDIT_HPP
#define DJINTERP_TOOLS_CLI_EDIT_HPP 1

// std
#include <string>  // std::string
// djinterp
#include "../../cpp/djinterp.hpp"                         // framework root
#include "../../cpp/modules/parse/cli/arg_parser/arg_parser.hpp"  // command, option,
                                                      // DCString, text,
                                                      // status
#include "./c/cli_edit.h"                             // dj_cli_edit, the C
                                                      // calls
#include "./cli_program.hpp"                          // program


namespace djinterp
{
namespace cli
{

//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Where the result goes
//------------------------------------------------------------------------------
// 1.1.1
// edit_place
//   enum: the dj_cli_edit_place values: back over the file read; to the file
// `-o` names; or to standard output.
struct edit_place
{
    enum value
    {
        back   = D_CLI_EDIT_BACK,
        out    = D_CLI_EDIT_OUT,
        output = D_CLI_EDIT_OUTPUT
    };
};


//==============================================================================
// 2.  THE EDIT
//==============================================================================


// 2.1    The class
//------------------------------------------------------------------------------
// 2.1.1
// edit
//   class: what a command that changes a file was given, and where its result
// goes. Every member that names a C call does what the call's contract says
// (cli_edit.h); record() is the record those calls take. A copy refers to
// the same options.
class edit
{
public:
    // making: nothing added (D_CLI_EDIT_INIT)
    edit() D_NOEXCEPT;

    /**
     * @brief Adds the command's operand, the file it changes, required;
     *        d_cli_edit_add_operand.
     *
     * @param[in,out] _program  the program, whose flags choose the rule.
     * @param[in]     _command  the command.
     * @param[in]     _name     the operand's name: `SHEET`.
     * @param[in]     _what     what the command changes: "the sheet".
     * @return the operand; a NULL handle when it could not be added, the
     *         program's run then saying why.
     */
    cli::option add_operand(
        program&     _program,
        cli::command _command,
        DCString     _name,
        DCString     _what
    ) D_NOEXCEPT;
    /**
     * @brief Adds the command's operand, as above, bound to the string the
     *        file's name is read into, as CLI11's add_option(name,
     *        variable) binds one.
     *
     * @param[in,out] _program  the program.
     * @param[in]     _command  the command.
     * @param[in]     _name     the operand's name.
     * @param[in]     _what     what the command changes.
     * @param[out]    _file     the string, which must outlive the parse.
     * @return the operand, or a NULL handle, as above.
     */
    cli::option add_operand(
        program&     _program,
        cli::command _command,
        DCString     _name,
        DCString     _what,
        std::string& _file
    );

    // the options for where the result goes -- `-o FILE` and `-n`, and for
    // B `-i` -- added where the command's other options are:
    // d_cli_edit_add_options
    cli::status add_options(
        program&     _program,
        cli::command _command,
        DCString     _what
    ) D_NOEXCEPT;

    // after a parse: where the result goes, `-n` aside; whether `-n` was
    // given -- d_cli_edit_target, d_cli_edit_dry_run
    edit_place::value target(
        const program& _program
    ) const D_NOEXCEPT;
    bool dry_run() const D_NOEXCEPT;

    /**
     * @brief Writes the result where it goes: nothing after `-n`, a file
     *        whole or not at all, or standard output through the program's
     *        own output; d_cli_edit_write.
     *
     * @param[in] _program  the program, which says a failure.
     * @param[in] _data     the bytes; a std::string converts.
     * @return exit_status::ok, or exit_status::trouble, said, the file as it
     *         was.
     */
    int write(
        const program& _program,
        const text&    _data
    ) const D_NOEXCEPT;

    // the record, for the C calls
    ::dj_cli_edit& record() D_NOEXCEPT;
    const ::dj_cli_edit& record() const D_NOEXCEPT;

private:
    ::dj_cli_edit m_edit;
};


//==============================================================================
// 3.  DEFINITIONS
//==============================================================================


// 3.1    Making
//------------------------------------------------------------------------------
// edit::edit
//   D_CLI_EDIT_INIT, field by field, as C++98 initializes no member with a
// braced list.
inline
edit::edit() D_NOEXCEPT
{
    m_edit.operand  = D_NULLPTR;
    m_edit.out      = D_NULLPTR;
    m_edit.dry_run  = D_NULLPTR;
    m_edit.in_place = D_NULLPTR;
}

// 3.2    Defining
//------------------------------------------------------------------------------
// edit::add_operand / add_options
//   The C calls on the program's record; the bound operand binds once made.
inline
cli::option edit::add_operand(
    program&     _program,
    cli::command _command,
    DCString     _name,
    DCString     _what
) D_NOEXCEPT
{
    return cli::option(d_cli_edit_add_operand(&_program.kit(),
                                              _command.get(),
                                              &m_edit,
                                              _name.get(),
                                              _what.get()));
}

inline
cli::option edit::add_operand(
    program&     _program,
    cli::command _command,
    DCString     _name,
    DCString     _what,
    std::string& _file
)
{
    cli::option operand = add_operand(_program,
                                      _command,
                                      _name,
                                      _what);

    // the string the name is read into
    if (operand.valid())
    {
        operand.bind(_file);
    }

    return operand;
}

inline
cli::status edit::add_options(
    program&     _program,
    cli::command _command,
    DCString     _what
) D_NOEXCEPT
{
    return cli::status(d_cli_edit_add_options(&_program.kit(),
                                              _command.get(),
                                              &m_edit,
                                              _what.get()));
}

// 3.3    Writing
//------------------------------------------------------------------------------
// edit::target / dry_run / write / record
//   The C calls on the record.
inline
edit_place::value edit::target(
    const program& _program
) const D_NOEXCEPT
{
    return static_cast<edit_place::value>(
               d_cli_edit_target(&_program.kit(),
                                 &m_edit));
}

inline
bool edit::dry_run() const D_NOEXCEPT
{
    return d_cli_edit_dry_run(&m_edit);
}

inline
int edit::write(
    const program& _program,
    const text&    _data
) const D_NOEXCEPT
{
    return d_cli_edit_write(&_program.kit(),
                            &m_edit,
                            _data.data(),
                            _data.size());
}

inline
::dj_cli_edit& edit::record() D_NOEXCEPT
{
    return m_edit;
}

inline
const ::dj_cli_edit& edit::record() const D_NOEXCEPT
{
    return m_edit;
}

}  // namespace cli
}  // namespace djinterp


#endif  // DJINTERP_TOOLS_CLI_EDIT_HPP
