/*******************************************************************************
* bitwisedom [bitwisedom]                                           cli_main.cpp
*
* bitwisedom's command line, and bitwisedom-gen's.
*   `bitwisedom` is a program of commands, made with djinterp's kit: the
* commands that read a sheet, those that make or change one, and the window,
* which the program loads only when `gui` asks for it, so the program
* carries none of it (GRAMMAR.md, section 7).
*   `bitwisedom-gen` is one command's program: gen's options, the sheet as
* an operand or by --example, and the two jobs it always had beside the
* header -- listing the examples and saving one -- as the options it always
* spelled them with. Its work is gen's, so the two cannot drift apart.
*
* path:      /src/djinterp/tools/bitwisedom/cli/cli_main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/
#include <djinterp/tools/bitwisedom/cli.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <memory>  // std::make_shared
#include <string>  // std::string
#include <vector>  // std::vector
// bitwisedom
#include <djinterp/tools/bitwisedom/examples.hpp>  // example_list
#include <djinterp/tools/bitwisedom/persist.hpp>   // save_sheet
#include <djinterp/tools/bitwisedom/version.hpp>   // BITWISEDOM_VERSION
#include "./cli_internal.hpp"                      // the commands


namespace bitwisedom
{

namespace
{

using namespace ::bitwisedom::cmd;

// DESCRIPTION
//   constant: what bitwisedom is, as its help begins.
const char* const DESCRIPTION =
    "a bitfield manager: sheets of named bits, written out as C or C++ "
    "macros";

// WINDOW
//   constant: the heading help lists the window's command under.
const char* const WINDOW = "The window";

// front_request
//   struct: what bitwisedom-gen was asked: gen's request, and its two other
// jobs.
struct front_request
{
    gen_request              gen;
    bool                     list = false;
    std::vector<std::string> save;
};

/*
front_job
  Which of its jobs bitwisedom-gen was given -- one, or a usage error, said:
the examples listed, 0; one saved, 1; a sheet's header, 2.
*/
int
front_job(
    kit::program&        _program,
    kit::command         _command,
    const front_request& _request,
    int&                 _job
)
{
    const bool sheet   = given(_command, "SHEET");
    const bool example = given(_command, "--example");
    const bool save    = given(_command, "--save-example");
    const int  jobs    = (_request.list ? 1 : 0) + (save ? 1 : 0) +
                         ( (sheet || example) ? 1 : 0 );

    _job = _request.list ? 0 : save ? 1 : 2;

    if (jobs > 1)
    {
        return _program.usage_error(_command,
                                    "--list-examples and --save-example "
                                    "each do their work alone, without a "
                                    "sheet");
    }

    if ( (save) &&
         (_request.save.size() != 2u) )
    {
        return _program.usage_error(_command,
                                    "--save-example takes a NAME and a "
                                    "FILE");
    }

    // one sheet: a file, or an example
    return ( (jobs == 1) && ((!sheet) || (!example)) )
           ? D_CLI_EXIT_OK
           : _program.usage_error(_command,
                                  "name one sheet: a file, or --example "
                                  "NAME");
}

/*
run_front
  bitwisedom-gen's work: the examples listed, one saved, or gen's work on
the sheet or the example named -- exactly one of those, as it always
required.
*/
int
run_front(
    kit::program&        _program,
    kit::command         _command,
    const front_request& _request
)
{
    int       job    = 2;
    const int status = front_job(_program, _command, _request, job);
    source    read;

    if (status != D_CLI_EXIT_OK)
    {
        return status;
    }

    // --list-examples
    if (job == 0)
    {
        std::string text;

        for (const example_info& each : example_list())
        {
            text += each.name + "\n";
        }

        emit(_program, text);

        return D_CLI_EXIT_OK;
    }

    // --save-example NAME FILE
    if (job == 1)
    {
        const int failed = read_source(_program,
                                       "example:" + _request.save[0], read)
                           ? kit::write_file(_request.save[1],
                                             save_sheet(read.doc))
                           : -1;

        return (failed == 0)  ? D_CLI_EXIT_OK
             : (failed == -1) ? D_CLI_EXIT_TROUBLE
                              : _program.file_error("write",
                                                    _request.save[1],
                                                    failed);
    }

    return run_gen(_program, _command, _request.gen);
}

}  // anonymous namespace


/*
run_cli
  The program, its commands, and the window's command, whose module is
found and loaded only when it is run.
*/
int
run_cli(
    int                _argc,
    const char* const* _argv
)
{
    kit::program program(BITWISEDOM_PROGRAM,
                         BITWISEDOM_VERSION,
                         DESCRIPTION,
                         kit::program_flags::commands);

    add_read_commands(program);
    add_edit_commands(program);
    add_row_commands(program);
    program.add_plugin("gui",
                       "open the window on the sheets named, or where it "
                       "was left")
        .group(WINDOW);

    return program.run(_argc, _argv);
}

/*
run_gen_front
  bitwisedom-gen: one command's program, whose root does gen's work.
*/
int
run_gen_front(
    int                _argc,
    const char* const* _argv
)
{
    kit::program program("bitwisedom-gen",
                         BITWISEDOM_VERSION,
                         "write a sheet's header, in C or C++: bitwisedom "
                         "gen, as build scripts have always called it");
    const auto   request = std::make_shared<front_request>();
    kit::command root(program);

    root.add_option("SHEET",
                    request->gen.sheet,
                    "the sheet: a sheet file, a bitfield or an export; - for "
                    "standard input");
    root.add_option("--example",
                    request->gen.example,
                    "a built-in example's sheet instead")
        .type_name("NAME");
    add_gen_options(root, request->gen);
    root.add_flag("--list-examples",
                  request->list,
                  "list the built-in examples, and exit")
        .group("Examples");
    root.add_option("--save-example",
                    request->save,
                    "write the built-in example NAME to FILE as a sheet "
                    "file, and exit")
        .type_name("NAME FILE")
        .expected(2)
        .group("Examples");
    program.set_main(
        [request](kit::program& _p, kit::command _c)
        {
            return run_front(_p, _c, *request);
        });

    return program.run(_argc, _argv);
}

}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
