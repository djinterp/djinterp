/*******************************************************************************
* djinterp [djinterp]                                                     main.c
*
*   Command-line driver for the dawk interpreter.
*     The synopsis is awk's: -F for the field separator, -v for a pre-BEGIN
* assignment, -f for a program file, otherwise the first operand is the
* program. A file operand of the form name=value is an assignment applied when
* the operand is reached, not a file.
*
*
* path:      /src/djinterp/tools/dawk/main.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.09.19
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dinterp.h"  // corresponding header
// std
#include <stdio.h>   // fprintf, fopen, fread
#include <stdlib.h>  // malloc, free
#include <string.h>  // strcmp, strlen
// djinterp
#include "../../../../inc/djinterp/tools/dawk/dparse.h"  // d_awk_parse
#include "../../../../inc/djinterp/tools/dawk/dtree.h"   // d_awk_source_tree_init
#include "../../../../inc/djinterp/tools/dawk/dsettings.h"  // d_settings


/*
d_internal_slurp
  Reads a whole file into memory.

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
    FILE* const stream = fopen(_path, "rb");

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

    (void)fclose(stream);

    // report the length only when the read succeeded
    if (buffer)
    {
        buffer[used] = '\0';
        *_out_length = used;
    }

    return buffer;
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
extern char** environ;

int
main(
    int    argc,
    char** argv
)
{
    const char* separator = NULL;
    const char* prog_file = NULL;
    const char* tree_root = NULL;
    const char* assigns[64];
    size_t      assign_count = 0;
    int         at           = 1;

    // options precede the program and the file operands
    // The interpreter's settings.  The registry lives only while options are
    // read: the values are taken out and it is freed before any path below
    // can return, so no exit leaves it allocated.
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

    while ((at < argc) && (argv[at][0] == '-') && (argv[at][1] != '\0'))
    {
        // a lone `--` ends the options
        if (strcmp(argv[at], "--") == 0)
        {
            at++;
            break;
        }

        if (strcmp(argv[at], "--settings") == 0)
        {
            d_settings_describe(settings, stdout);
            d_settings_free(settings);
            return 0;
        }

        const int taken = d_settings_take_option(settings, argc, argv, &at,
                                                 problem, sizeof(problem));

        if (taken != 0)
        {
            if (taken < 0)
            {
                (void)fprintf(stderr, "dawk: %s\n", problem);
                d_settings_free(settings);
                return 2;
            }

            ++at;
            continue;
        }

        const char option = argv[at][1];
        const char* value = &argv[at][2];

        // an option's value may be attached or separate
        if (*value == '\0')
        {
            at++;

            // an option that needs a value and has none is a usage error
            if (at >= argc)
            {
                (void)fprintf(stderr, "dawk: option -%c needs a value\n",
                              option);
                return 2;
            }

            value = argv[at];
        }

        if (option == 'F')
        {
            separator = value;
        }
        else if (option == 'T')
        {
            tree_root = value;
        }
        else if (option == 'f')
        {
            prog_file = value;
        }
        else if (option == 'v')
        {
            // the assignment list is fixed and generous
            if (assign_count < 64u)
            {
                assigns[assign_count++] = value;
            }
        }
        else
        {
            (void)fprintf(stderr, "dawk: unknown option -%c\n", option);
            return 2;
        }

        at++;
    }

    const size_t call_depth   = (size_t)d_settings_int(settings,
                                                       "interp.call-depth");
    const size_t stack_budget = (size_t)d_settings_int(settings,
                                                       "interp.stack-budget");

    d_settings_free(settings);
    settings = NULL;

    char*  source        = NULL;
    size_t source_length = 0;
    bool   owned         = false;

    // the program comes from a file or from the first remaining operand
    if (prog_file)
    {
        source = d_internal_slurp(prog_file, &source_length);
        owned  = true;

        // a program file that will not open is a usage error
        if (!source)
        {
            (void)fprintf(stderr, "dawk: cannot read %s\n", prog_file);
            return 2;
        }
    }
    else
    {
        // a run with no program at all is a usage error
        if (at >= argc)
        {
            (void)fprintf(stderr,
                          "usage: dawk [-F fs] [-v var=value] "
                          "'program' [file ...]\n");
            return 2;
        }

        source        = argv[at++];
        source_length = strlen(source);
    }

    struct d_awk_program* const program = d_awk_parse(source,
                                                      source_length,
                                                      prog_file ? prog_file
                                                                : "-");

    // the source is no longer needed once the tree exists
    if (owned)
    {
        free(source);
    }

    // a program that will not parse is reported and nothing runs
    if (!program)
    {
        // the diagnostic already leads with file:line:col, so it is printed
        // unadorned and an editor can jump straight to it
        (void)fprintf(stderr, "%s\n", d_awk_parse_error());
        return 2;
    }

    struct d_awk_interp* const interp = d_awk_interp_new(program);


    // a NULL interpreter is handled below; setting limits on it is a no-op

    (void)d_awk_interp_set_limits(interp, call_depth, stack_budget);

    // abandon the run when the interpreter could not be held
    if (!interp)
    {
        d_awk_program_free(program);
        (void)fprintf(stderr, "dawk: out of memory\n");
        return 2;
    }

    (void)d_awk_interp_set_environ(interp, environ);

    // -F sets FS before BEGIN, as an ordinary assignment would
    if (separator)
    {
        char buffer[256];

        (void)snprintf(buffer, sizeof(buffer), "FS=%s", separator);
        (void)d_awk_interp_assign(interp, buffer);
    }

    for (size_t which = 0; which < assign_count; ++which)
    {
        (void)d_awk_interp_assign(interp, assigns[which]);
    }

    for (; at < argc; ++at)
    {
        (void)d_awk_interp_add_input(interp, argv[at]);
    }

    struct d_awk_source tree_source;

    // -T <root> walks a tree instead of reading the operands as files
    if (tree_root)
    {
        if (!d_awk_source_tree_init(&tree_source, tree_root))
        {
            (void)fprintf(stderr, "dawk: cannot walk %s\n", tree_root);
            d_awk_interp_free(interp);
            d_awk_program_free(program);
            return 2;
        }

        (void)d_awk_interp_set_source(interp, &tree_source);
    }

    const int status  = d_awk_interp_run(interp);
    const char* const message = d_awk_interp_error(interp);

    // a runtime diagnostic is reported before the status is returned
    if (message)
    {
        (void)fprintf(stderr, "%s\n", message);
    }

    d_awk_interp_free(interp);
    d_awk_program_free(program);

    return status;
}
