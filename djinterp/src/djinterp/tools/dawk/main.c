/*******************************************************************************
* djinterp [tools]                                                        main.c
*
*   Command-line driver for the dawk interpreter.
*     The synopsis is awk's: -F for the field separator, -v for a pre-BEGIN
* assignment, -f for a program file, otherwise the first operand is the
* program. A file operand of the form name=value is an assignment applied when
* the operand is reached, not a file.
*     -f may be given more than once, and the program is then the files in the
* order given, as POSIX has it; a diagnostic names the file and the line the
* text came from, not a line of their concatenation. -f - reads the program
* from standard input.
*     -x names an extension to install, which adds host functions a program
* calls by name: story, for e-learning stories; cmake, for CMake files;
* ninja, for Ninja's files and builds, loaded as ninja loads them; msvs, for
* Visual Studio's, or either of its parts alone, sln for solutions
* and vcxproj for MSBuild's files; wiki, for a wiki's pages; and git, for
* git's own files and a repository, read as git reads them. A part named
* beside msvs is msvs's. Without one, dawk is POSIX awk and nothing more.
*
*
* path:      /src/djinterp/tools/dawk/main.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dinterp.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // fprintf, fopen, fread, snprintf, stdin
#include <stdlib.h>   // malloc, realloc, free, getenv, strtoul
#include <string.h>   // strcmp, strlen, strncmp, memcpy
// djinterp
#include "../../../../inc/djinterp/tools/dawk/dparse.h"                      // d_awk_parse
#include "../../../../inc/djinterp/tools/dawk/dtree.h"                       // d_awk_source_tree_init
#include "../../../../inc/djinterp/tools/dawk/dsettings.h"                   // d_settings
#include "../../../../inc/djinterp/tools/dawk/ext/cmake/dhost_cmake.h"       // d_host_cmake
#include "../../../../inc/djinterp/tools/dawk/ext/git/dhost_git.h"           // d_host_git
#include "../../../../inc/djinterp/tools/dawk/ext/msvs/dhost_msvs.h"         // d_host_msvs
#include "../../../../inc/djinterp/tools/dawk/ext/msvs/dhost_sln.h"          // d_host_sln
#include "../../../../inc/djinterp/tools/dawk/ext/msvs/dhost_vcxproj.h"      // d_host_vcxproj
#include "../../../../inc/djinterp/tools/dawk/ext/ninja/dhost_ninja.h"       // d_host_ninja
#include "../../../../inc/djinterp/tools/dawk/ext/story/dhost_articulate.h"  // d_articulate_driver
#include "../../../../inc/djinterp/tools/dawk/ext/story/dhost_story.h"       // d_host_story
#include "../../../../inc/djinterp/tools/dawk/ext/wiki/dhost_wiki.h"         // d_host_wiki


// the environment, which ENVIRON is built from; POSIX declares it, and no
// header of ISO C does
extern char** environ;

// d_internal_options
//   struct: what the command line asked for.  Program files and -v
// assignments are kept in the order given, which is the order POSIX applies
// them in; every pointer borrows from argv, and `operands` is the tail of it
// after the program.
struct d_internal_options
{
    const char*  separator;
    const char*  tree_root;
    const char** prog_files;
    size_t       prog_file_count;
    const char** assigns;
    size_t       assign_count;
    size_t       call_depth;
    size_t       stack_budget;
    char**       operands;
    int          operand_count;
    const char** extensions;
    size_t       extension_count;
};

// d_internal_extension
//   struct: an extension -x can name: how its state is made, how it is
// installed into an interpreter, and how it is released after the run; and
// the extension it is a part of, which installs it too, or NULL.
struct d_internal_extension
{
    const char* name;
    void*       (*make)(void);
    bool        (*install)(void*, struct d_awk_interp*);
    void        (*release)(void*);
    const char* part_of;
};

// d_internal_source
//   struct: the program text and, for each -f file, the line of the text its
// first line became, so a diagnostic can be traced back to its file.
struct d_internal_source
{
    char*   text;
    size_t  length;
    size_t* first_lines;
    size_t  file_count;
};


/*
d_internal_slurp
  Reads a whole file into memory; "-" is standard input, as POSIX has it for
-f.

Parameter(s):
  _path:       the file to read.
  _out_length: receives the length in bytes.
Return:
  The contents, owned by the caller, or NULL on failure.
*/
static char*
d_internal_slurp(
    const char* _path,
    size_t*     _out_length
)
{
    const bool  is_stdin = (strcmp(_path, "-") == 0);
    FILE* const stream   = is_stdin ? stdin : fopen(_path, "rb");

    // report the failure rather than returning an empty program
    if (!stream)
    {
        return NULL;
    }

    size_t capacity = 4096u;
    size_t used     = 0;
    char*  buffer   = malloc(capacity);

    while (buffer)
    {
        const size_t got = fread(&buffer[used], 1u, capacity - used - 1u,
                                 stream);

        used += got;

        // a short read means the file is exhausted
        if ((used + 1u) < capacity)
        {
            break;
        }

        capacity *= 2u;

        char* grown = realloc(buffer, capacity);

        // abandon the read when the buffer could not grow
        if (!grown)
        {
            free(buffer);
            buffer = NULL;
            break;
        }

        buffer = grown;
    }

    // standard input stays open for the data the program may read
    if (!is_stdin)
    {
        (void)fclose(stream);
    }

    // report the length only when the read succeeded
    if (buffer)
    {
        buffer[used] = '\0';
        *_out_length = used;
    }

    return buffer;
}


/*
d_internal_push_text
  Appends a borrowed string to a growing list.  The lists here hold a handful
of command-line words, so each push grows by one.

Parameter(s):
  _list:  the list; NULL while it is empty.
  _count: its length, advanced by one.
  _text:  the string to append.
Return:
  A boolean value corresponding to either:
  - true, if the string was appended, or
  - false, if the list could not grow.
*/
static bool
d_internal_push_text(
    const char*** _list,
    size_t*       _count,
    const char*   _text
)
{
    const char** const grown = realloc(*_list,
                                       (*_count + 1u) * sizeof(**_list));

    // report the failure rather than dropping the word
    if (!grown)
    {
        return false;
    }

    grown[(*_count)++] = _text;
    *_list             = grown;

    return true;
}


/*
d_internal_story_make, d_internal_story_install, d_internal_story_release
  The storyline extension, with every driver this build has.
*/
static void*
d_internal_story_make(void)
{
    struct d_host_story* const host = d_host_story_new();

    // an extension that cannot take its driver is not made
    if ( (host) &&
         (!d_host_story_add_driver(host, d_articulate_driver())) )
    {
        d_host_story_free(host);

        return NULL;
    }

    return host;
}


static bool
d_internal_story_install(
    void*                _state,
    struct d_awk_interp* _interp
)
{
    return d_host_story_install(_state, _interp);
}


static void
d_internal_story_release(
    void* _state
)
{
    d_host_story_free(_state);

    return;
}


/*
d_internal_cmake_make, d_internal_cmake_install, d_internal_cmake_release
  The CMake extension.
*/
static void*
d_internal_cmake_make(void)
{
    return d_host_cmake_new();
}


static bool
d_internal_cmake_install(
    void*                _state,
    struct d_awk_interp* _interp
)
{
    return d_host_cmake_install(_state, _interp);
}


static void
d_internal_cmake_release(
    void* _state
)
{
    d_host_cmake_free(_state);

    return;
}


/*
d_internal_ninja_make, d_internal_ninja_install, d_internal_ninja_release
  The Ninja extension: Ninja's files, and builds loaded as ninja loads them.
*/
static void*
d_internal_ninja_make(void)
{
    return d_host_ninja_new();
}


static bool
d_internal_ninja_install(
    void*                _state,
    struct d_awk_interp* _interp
)
{
    return d_host_ninja_install(_state, _interp);
}


static void
d_internal_ninja_release(
    void* _state
)
{
    d_host_ninja_free(_state);

    return;
}


/*
d_internal_msvs_make, d_internal_msvs_install, d_internal_msvs_release
  The Visual Studio extension, both its parts.
*/
static void*
d_internal_msvs_make(void)
{
    return d_host_msvs_new();
}


static bool
d_internal_msvs_install(
    void*                _state,
    struct d_awk_interp* _interp
)
{
    return d_host_msvs_install(_state, _interp);
}


static void
d_internal_msvs_release(
    void* _state
)
{
    d_host_msvs_free(_state);

    return;
}


/*
d_internal_sln_make, d_internal_sln_install, d_internal_sln_release
  The solutions part alone.
*/
static void*
d_internal_sln_make(void)
{
    return d_host_sln_new();
}


static bool
d_internal_sln_install(
    void*                _state,
    struct d_awk_interp* _interp
)
{
    return d_host_sln_install(_state, _interp);
}


static void
d_internal_sln_release(
    void* _state
)
{
    d_host_sln_free(_state);

    return;
}


/*
d_internal_vcxproj_make, d_internal_vcxproj_install,
d_internal_vcxproj_release
  The MSBuild part alone.
*/
static void*
d_internal_vcxproj_make(void)
{
    return d_host_vcxproj_new();
}


static bool
d_internal_vcxproj_install(
    void*                _state,
    struct d_awk_interp* _interp
)
{
    return d_host_vcxproj_install(_state, _interp);
}


static void
d_internal_vcxproj_release(
    void* _state
)
{
    d_host_vcxproj_free(_state);

    return;
}


/*
d_internal_wiki_make, d_internal_wiki_install, d_internal_wiki_release
  The wiki extension: a wiki's pages.
*/
static void*
d_internal_wiki_make(void)
{
    return d_host_wiki_new();
}


static bool
d_internal_wiki_install(
    void*                _state,
    struct d_awk_interp* _interp
)
{
    return d_host_wiki_install(_state, _interp);
}


static void
d_internal_wiki_release(
    void* _state
)
{
    d_host_wiki_free(_state);

    return;
}


/*
d_internal_git_make, d_internal_git_install, d_internal_git_release
  The git extension: git's own files, and a repository.
*/
static void*
d_internal_git_make(void)
{
    return d_host_git_new();
}


static bool
d_internal_git_install(
    void*                _state,
    struct d_awk_interp* _interp
)
{
    return d_host_git_install(_state, _interp);
}


static void
d_internal_git_release(
    void* _state
)
{
    d_host_git_free(_state);

    return;
}


/*
d_internal_extension_named
  The extension -x names, or NULL.
*/
static const struct d_internal_extension*
d_internal_extension_named(
    const char* _name
)
{
    static const struct d_internal_extension extensions[] =
    {
        { "story",   d_internal_story_make,   d_internal_story_install,
          d_internal_story_release,   NULL   },
        { "cmake",   d_internal_cmake_make,   d_internal_cmake_install,
          d_internal_cmake_release,   NULL   },
        { "ninja",   d_internal_ninja_make,   d_internal_ninja_install,
          d_internal_ninja_release,   NULL   },
        { "msvs",   d_internal_msvs_make,    d_internal_msvs_install,
          d_internal_msvs_release,    NULL   },
        { "sln",     d_internal_sln_make,     d_internal_sln_install,
          d_internal_sln_release,     "msvs" },
        { "vcxproj", d_internal_vcxproj_make, d_internal_vcxproj_install,
          d_internal_vcxproj_release, "msvs" },
        { "wiki",    d_internal_wiki_make,    d_internal_wiki_install,
          d_internal_wiki_release,    NULL   },
        { "git",     d_internal_git_make,     d_internal_git_install,
          d_internal_git_release,     NULL   }
    };

    // each extension in turn
    for (size_t at = 0; at < (sizeof(extensions) / sizeof(extensions[0]));
         ++at)
    {
        // the extensions are named exactly
        if (strcmp(extensions[at].name, _name) == 0)
        {
            return &extensions[at];
        }
    }

    return NULL;
}


/*
d_internal_take_options
  Reads the options that precede the program, into `_options`.
NOTE:
  The interpreter's settings registry lives only while options are read: the
  values are taken out and it is freed before this returns, so no exit leaves
  it allocated.

Parameter(s):
  _argc:    argument count.
  _argv:    argument vector.
  _at:      the first argument to read; left at the first operand.
  _options: receives the options.
Return:
  -1 to go on and run, or the status to exit with at once: 0 after
  --settings, 2 on a usage error.
*/
static int
d_internal_take_options(
    int                        _argc,
    char**                     _argv,
    int*                       _at,
    struct d_internal_options* _options
)
{
    char call_depth_text[24];
    char stack_budget_text[24];

    (void)snprintf(call_depth_text, sizeof(call_depth_text), "%u",
                   (unsigned)D_AWK_CALL_DEPTH_DEFAULT);
    (void)snprintf(stack_budget_text, sizeof(stack_budget_text), "%lu",
                   (unsigned long)D_AWK_STACK_BUDGET_DEFAULT);

    const struct d_setting_def interp_settings[] =
    {
        { "interp.call-depth", D_SETTING_INT, call_depth_text, NULL, NULL,
          1L, 1000000L, NULL,
          "most nested user-function calls; the same limit on every platform" },
        { "interp.stack-budget", D_SETTING_INT, stack_budget_text, NULL, NULL,
          16384L, 1073741824L, NULL,
          "most C stack a run may consume, in bytes; the real guard against a "
          "crash -- set it lower on a small thread" }
    };

    struct d_settings* settings = d_settings_new();
    char               problem[256];

    // a settings registry that cannot be built, or an environment it
    // refuses, stops the run before anything is read
    if ( (!settings) ||
         (!d_settings_register(settings, interp_settings, 2u)) ||
         (!d_settings_apply(settings, getenv("DAWK_SETTINGS"),
                            D_SETTING_ENVIRONMENT, problem,
                            sizeof(problem))) )
    {
        (void)fprintf(stderr, "dawk: DAWK_SETTINGS: %s\n",
                      settings ? problem : "cannot build settings");
        d_settings_free(settings);

        return 2;
    }

    int status = -1;

    // options precede the program and the file operands
    while ( (status < 0)                         &&
            (*_at < _argc)                       &&
            (_argv[*_at][0] == '-')              &&
            (_argv[*_at][1] != '\0') )
    {
        // a lone `--` ends the options
        if (strcmp(_argv[*_at], "--") == 0)
        {
            (*_at)++;
            break;
        }

        // --settings describes every setting and runs nothing
        if (strcmp(_argv[*_at], "--settings") == 0)
        {
            d_settings_describe(settings, stdout);
            status = 0;
            break;
        }

        const int taken = d_settings_take_option(settings, _argc, _argv, _at,
                                                 problem, sizeof(problem));

        // a setting given on the command line is the registry's to read
        if (taken != 0)
        {
            // a setting the registry refused is a usage error
            if (taken < 0)
            {
                (void)fprintf(stderr, "dawk: %s\n", problem);
                status = 2;
                break;
            }

            (*_at)++;
            continue;
        }

        const char  option = _argv[*_at][1];
        const char* value  = &_argv[*_at][2];

        // an option's value may be attached or separate
        if (*value == '\0')
        {
            (*_at)++;

            // an option that needs a value and has none is a usage error
            if (*_at >= _argc)
            {
                (void)fprintf(stderr, "dawk: option -%c needs a value\n",
                              option);
                status = 2;
                break;
            }

            value = _argv[*_at];
        }

        bool kept = true;

        // each option is recorded; -f and -v may repeat, in order
        if (option == 'F')
        {
            _options->separator = value;
        }
        else if (option == 'T')
        {
            _options->tree_root = value;
        }
        else if (option == 'f')
        {
            kept = d_internal_push_text(&_options->prog_files,
                                        &_options->prog_file_count,
                                        value);
        }
        else if (option == 'v')
        {
            kept = d_internal_push_text(&_options->assigns,
                                        &_options->assign_count,
                                        value);
        }
        else if ( (option == 'x') &&
                  (!d_internal_extension_named(value)) )
        {
            (void)fprintf(stderr, "dawk: unknown extension: %s\n", value);
            status = 2;
            break;
        }
        else if (option == 'x')
        {
            kept = d_internal_push_text(&_options->extensions,
                                        &_options->extension_count,
                                        value);
        }
        else
        {
            (void)fprintf(stderr, "dawk: unknown option -%c\n", option);
            status = 2;
            break;
        }

        // an option that could not be recorded stops the run
        if (!kept)
        {
            (void)fprintf(stderr, "dawk: out of memory\n");
            status = 2;
            break;
        }

        (*_at)++;
    }

    _options->call_depth   = (size_t)d_settings_int(settings,
                                                    "interp.call-depth");
    _options->stack_budget = (size_t)d_settings_int(settings,
                                                    "interp.stack-budget");

    d_settings_free(settings);

    return status;
}


/*
d_internal_read_files
  Concatenates the -f files into one program, a newline between each pair so
that no file's last line runs into the next file's first, and records where
each file begins.

Parameter(s):
  _options: the options, naming the files.
  _source:  receives the program.
Return:
  A boolean value corresponding to either:
  - true, if every file was read, or
  - false, otherwise, with the reason reported.
*/
static bool
d_internal_read_files(
    const struct d_internal_options* _options,
    struct d_internal_source*        _source
)
{
    const size_t files = _options->prog_file_count;
    size_t       line  = 1;

    _source->first_lines = malloc(files * sizeof(*_source->first_lines));

    // report the failure rather than losing track of the files
    if (!_source->first_lines)
    {
        (void)fprintf(stderr, "dawk: out of memory\n");

        return false;
    }

    for (size_t which = 0; which < files; ++which)
    {
        size_t     length = 0;
        char*      text   = d_internal_slurp(_options->prog_files[which],
                                             &length);
        const bool last   = ((which + 1u) == files);

        // a program file that will not open is a usage error
        if (!text)
        {
            (void)fprintf(stderr, "dawk: cannot read %s\n",
                          _options->prog_files[which]);

            return false;
        }

        char* const grown = realloc(_source->text,
                                    _source->length + length + 2u);

        // abandon the program when it could not grow
        if (!grown)
        {
            free(text);
            (void)fprintf(stderr, "dawk: out of memory\n");

            return false;
        }

        _source->text                     = grown;
        _source->first_lines[which]       = line;
        _source->file_count               = which + 1u;

        memcpy(&_source->text[_source->length], text, length);
        _source->length += length;

        // a newline separates this file from the next
        if (!last)
        {
            _source->text[_source->length++] = '\n';
        }

        _source->text[_source->length] = '\0';

        // the next file begins a line past every newline so far
        for (size_t at = 0; at < length; ++at)
        {
            line += (text[at] == '\n') ? 1u : 0u;
        }

        line += last ? 0u : 1u;
        free(text);
    }

    return true;
}


/*
d_internal_report_parse_error
  Prints a parse diagnostic.  With several -f files, its line is a line of
their concatenation; it is mapped back to the file it came from and the line
within it, so that `file:line:column` is a place an editor can open.

Parameter(s):
  _options: the options, naming the files.
  _source:  the program, recording where each file began.
  _origin:  the origin the program was parsed under.
  _message: the diagnostic, as d_awk_parse_error gives it.
*/
static void
d_internal_report_parse_error(
    const struct d_internal_options* _options,
    const struct d_internal_source*  _source,
    const char*                      _origin,
    const char*                      _message
)
{
    const size_t origin_length = strlen(_origin);

    // one file, or a message not led by the origin, is printed as it is
    if ( (_source->file_count < 2u)                                 ||
         (strncmp(_message, _origin, origin_length) != 0)           ||
         (_message[origin_length] != ':') )
    {
        (void)fprintf(stderr, "%s\n", _message);

        return;
    }

    char*               rest = NULL;
    const unsigned long line = strtoul(&_message[origin_length + 1u],
                                       &rest,
                                       10);
    size_t              file = 0;

    // the file is the last one beginning at or before the line
    while ( ((file + 1u) < _source->file_count) &&
            (_source->first_lines[file + 1u] <= (size_t)line) )
    {
        file++;
    }

    (void)fprintf(stderr, "%s:%lu%s\n",
                  _options->prog_files[file],
                  line - (unsigned long)_source->first_lines[file] + 1ul,
                  rest);

    return;
}


/*
d_internal_apply_assignments
  Applies -F and every -v, in order, before BEGIN runs.  -F sets FS as an
ordinary assignment would.  A -v whose argument is not `name=value` is a usage
error.

Parameter(s):
  _options: the options.
  _interp:  the interpreter.
Return:
  A boolean value corresponding to either:
  - true, if every assignment was applied, or
  - false, otherwise, with the reason reported.
*/
static bool
d_internal_apply_assignments(
    const struct d_internal_options* _options,
    struct d_awk_interp*             _interp
)
{
    // -F is FS=<its value>, spelled out so that it decodes as -v would
    if (_options->separator)
    {
        const size_t length = strlen(_options->separator);
        char* const  text   = malloc(length + 4u);

        // report the failure rather than running with the wrong FS
        if (!text)
        {
            (void)fprintf(stderr, "dawk: out of memory\n");

            return false;
        }

        memcpy(text, "FS=", 3u);
        memcpy(&text[3], _options->separator, length + 1u);

        const bool applied = d_awk_interp_assign(_interp, text);

        free(text);

        // an FS that could not be held stops the run
        if (!applied)
        {
            (void)fprintf(stderr, "dawk: out of memory\n");

            return false;
        }
    }

    for (size_t which = 0; which < _options->assign_count; ++which)
    {
        // a -v that is not an assignment is a usage error
        if (!d_awk_interp_assign(_interp, _options->assigns[which]))
        {
            (void)fprintf(stderr, "dawk: invalid -v option argument: %s\n",
                          _options->assigns[which]);

            return false;
        }
    }

    return true;
}


/*
d_internal_install
  Makes and installs every extension -x named, in order, recording each state
for release after the run; an extension named twice is installed once, and a
part of one also named is installed by it.

Parameter(s):
  _options: the options, naming the extensions.
  _interp:  the interpreter.
  _states:  receives each extension's state, in the order named; eight at
            most, more than there are extensions to name.
Return:
  A boolean value corresponding to either:
  - true, if every extension was installed, or
  - false, otherwise, with the reason reported.
*/
static bool
d_internal_install(
    const struct d_internal_options* _options,
    struct d_awk_interp*             _interp,
    void**                           _states
)
{
    // more extensions than there are to name is a usage error
    if (_options->extension_count > 8u)
    {
        (void)fprintf(stderr, "dawk: too many -x options\n");

        return false;
    }

    for (size_t which = 0; which < _options->extension_count; ++which)
    {
        const struct d_internal_extension* const extension =
            d_internal_extension_named(_options->extensions[which]);
        bool repeated = false;

        // an extension already installed is not installed again
        for (size_t before = 0; before < which; ++before)
        {
            repeated = repeated ||
                       (strcmp(_options->extensions[before],
                               _options->extensions[which]) == 0);
        }

        // nor is a part of one also named, which installs it
        for (size_t other = 0; (extension->part_of) &&
                               (other < _options->extension_count); ++other)
        {
            repeated = repeated ||
                       (strcmp(_options->extensions[other],
                               extension->part_of) == 0);
        }

        if (repeated)
        {
            continue;
        }

        _states[which] = extension->make();

        // an extension that cannot be made or installed stops the run
        if ( (!_states[which]) ||
             (!extension->install(_states[which], _interp)) )
        {
            const char* const why = d_awk_interp_error(_interp);

            (void)fprintf(stderr, "%s\n",
                          (why) ? why : "dawk: out of memory");

            return false;
        }
    }

    return true;
}


/*
d_internal_run
  Parses the program, builds the interpreter and runs it over the operands.

Parameter(s):
  _options: the options, the operands among them.
  _source:  the program.
  _origin:  the name diagnostics give the program.
Return:
  The program's exit status, or 2 on a usage or runtime error.
*/
static int
d_internal_run(
    const struct d_internal_options* _options,
    const struct d_internal_source*  _source,
    const char*                      _origin
)
{
    struct d_awk_program* const program = d_awk_parse(_source->text,
                                                      _source->length,
                                                      _origin);

    // a program that will not parse is reported and nothing runs; the
    // diagnostic leads with file:line:col, so an editor can jump to it
    if (!program)
    {
        d_internal_report_parse_error(_options,
                                      _source,
                                      _origin,
                                      d_awk_parse_error());

        return 2;
    }

    struct d_awk_interp* const interp = d_awk_interp_new(program);

    // abandon the run when the interpreter could not be held
    if (!interp)
    {
        d_awk_program_free(program);
        (void)fprintf(stderr, "dawk: out of memory\n");

        return 2;
    }

    (void)d_awk_interp_set_limits(interp,
                                  _options->call_depth,
                                  _options->stack_budget);
    (void)d_awk_interp_set_environ(interp, environ);

    void*               states[8]   = { NULL };
    struct d_awk_source tree_source = { 0 };
    int                 status      = 2;

    // the extensions, then the assignments, and -T <root> must name a tree
    if (!d_internal_install(_options, interp, states))
    {
        status = 2;
    }
    else if (!d_internal_apply_assignments(_options, interp))
    {
        status = 2;
    }
    else if ( (_options->tree_root) &&
              (!d_awk_source_tree_init(&tree_source, _options->tree_root)) )
    {
        (void)fprintf(stderr, "dawk: cannot walk %s\n", _options->tree_root);
        status = 2;
    }
    else
    {
        for (int which = 0; which < _options->operand_count; ++which)
        {
            (void)d_awk_interp_add_input(interp, _options->operands[which]);
        }

        // the tree replaces the operands as the record source
        if (_options->tree_root)
        {
            (void)d_awk_interp_set_source(interp, &tree_source);
        }

        status = d_awk_interp_run(interp);

        const char* const message = d_awk_interp_error(interp);

        // a runtime diagnostic is reported before the status is returned
        if (message)
        {
            (void)fprintf(stderr, "%s\n", message);
        }
    }

    d_awk_interp_free(interp);
    d_awk_program_free(program);

    // an extension's state outlives the interpreter that called into it
    for (size_t which = 0; which < _options->extension_count; ++which)
    {
        // a state that was made is released by its own extension
        if ( (which < 8u) &&
             (states[which]) )
        {
            d_internal_extension_named(_options->extensions[which])
                ->release(states[which]);
        }
    }

    return status;
}


/*
main
  Parses the command line, builds the program and runs it.

Parameter(s):
  argc: argument count.
  argv: argument vector.
Return:
  The program's exit status, or 2 on a usage or runtime error.
*/
int
main(
    int    argc,
    char** argv
)
{
    struct d_internal_options options = { NULL, NULL, NULL, 0, NULL, 0,
                                          0,    0,    NULL, 0, NULL, 0 };
    struct d_internal_source  source  = { NULL, 0, NULL, 0 };
    int                       at      = 1;
    int                       status  = d_internal_take_options(argc,
                                                                argv,
                                                                &at,
                                                                &options);
    const char*               origin  = "-";

    // the program comes from the -f files, or else from the first operand
    if ( (status < 0) &&
         (options.prog_file_count > 0) )
    {
        origin = options.prog_files[0];
        status = d_internal_read_files(&options, &source) ? -1 : 2;
    }
    else if ( (status < 0) &&
              (at < argc) )
    {
        source.text   = argv[at];
        source.length = strlen(argv[at]);
        at++;
    }
    else if (status < 0)
    {
        // a run with no program at all is a usage error
        (void)fprintf(stderr,
                      "usage: dawk [-F fs] [-v var=value] [-x extension] "
                      "['program' | -f progfile ...] [file ...]\n");
        status = 2;
    }

    // a program in hand is run over the operands after it
    if (status < 0)
    {
        options.operands      = &argv[at];
        options.operand_count = argc - at;
        status                = d_internal_run(&options, &source, origin);
    }

    // only text read from files belongs to this function
    if (options.prog_file_count > 0)
    {
        free(source.text);
    }

    free(source.first_lines);
    free(options.prog_files);
    free(options.assigns);
    free(options.extensions);

    return status;
}
