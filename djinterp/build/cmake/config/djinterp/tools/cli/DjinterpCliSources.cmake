#*******************************************************************************
# djinterp [tools]                                      DjinterpCliSources.cmake
#
# The sources of djinterp's command-line kit and of the argument parser it
# is built on, for whatever compiles them:
#
#     DJINTERP_ARG_PARSER_SOURCES   the parser's C sources, the sinks it
#                                   writes through, and re_std's one C source
#     DJINTERP_CLI_SOURCES          the kit's C sources, and filesys, which
#                                   reads and writes its files
#
# DjinterpCli.cmake makes the libraries a program links from them; the
# kit's tests make their own, in testing mode. DJINTERP_ROOT, the repository
# root, defaults to the tree this file is in.
#
# path:      /build/cmake/config/djinterp/tools/cli/DjinterpCliSources.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
#                                                            revised: 2026.10.25
#*******************************************************************************
include_guard(GLOBAL)

if(NOT DJINTERP_ROOT)
    get_filename_component(DJINTERP_ROOT
                           "${CMAKE_CURRENT_LIST_DIR}/../../../../../.."
                           ABSOLUTE)
endif()

set(_djinterp_cli_src "${DJINTERP_ROOT}/src/djinterp")
set(_djinterp_cli_parser "${_djinterp_cli_src}/c/modules/parse/cli/arg_parser")

# DJINTERP_ARG_PARSER_SOURCES / DJINTERP_CLI_SOURCES
#   the argument parser's sources, with what it writes through; the kit's,
# with filesys.
set(DJINTERP_ARG_PARSER_SOURCES
    "${_djinterp_cli_parser}/arg_argv.c"
    "${_djinterp_cli_parser}/arg_argv_source.c"
    "${_djinterp_cli_parser}/arg_command.c"
    "${_djinterp_cli_parser}/arg_command_option.c"
    "${_djinterp_cli_parser}/arg_command_query.c"
    "${_djinterp_cli_parser}/arg_command_set.c"
    "${_djinterp_cli_parser}/arg_command_tree.c"
    "${_djinterp_cli_parser}/arg_command_util.c"
    "${_djinterp_cli_parser}/arg_common.c"
    "${_djinterp_cli_parser}/arg_completion.c"
    "${_djinterp_cli_parser}/arg_completion_fish.c"
    "${_djinterp_cli_parser}/arg_completion_pwsh.c"
    "${_djinterp_cli_parser}/arg_completion_sh.c"
    "${_djinterp_cli_parser}/arg_config.c"
    "${_djinterp_cli_parser}/arg_config_read.c"
    "${_djinterp_cli_parser}/arg_config_write.c"
    "${_djinterp_cli_parser}/arg_escape.c"
    "${_djinterp_cli_parser}/arg_formatter.c"
    "${_djinterp_cli_parser}/arg_formatter_command.c"
    "${_djinterp_cli_parser}/arg_formatter_option.c"
    "${_djinterp_cli_parser}/arg_internal.c"
    "${_djinterp_cli_parser}/arg_list.c"
    "${_djinterp_cli_parser}/arg_option.c"
    "${_djinterp_cli_parser}/arg_option_link.c"
    "${_djinterp_cli_parser}/arg_option_result.c"
    "${_djinterp_cli_parser}/arg_option_set.c"
    "${_djinterp_cli_parser}/arg_option_value.c"
    "${_djinterp_cli_parser}/arg_parse.c"
    "${_djinterp_cli_parser}/arg_parse_option.c"
    "${_djinterp_cli_parser}/arg_parse_positional.c"
    "${_djinterp_cli_parser}/arg_parse_text.c"
    "${_djinterp_cli_parser}/arg_parser.c"
    "${_djinterp_cli_parser}/arg_process.c"
    "${_djinterp_cli_parser}/arg_process_config.c"
    "${_djinterp_cli_parser}/arg_suggest.c"
    "${_djinterp_cli_parser}/arg_system.c"
    "${_djinterp_cli_parser}/arg_type.c"
    "${_djinterp_cli_parser}/arg_validator.c"
    "${_djinterp_cli_parser}/arg_validator_extra.c"
    "${_djinterp_cli_parser}/arg_validator_member.c"
    "${_djinterp_cli_parser}/arg_validator_path.c"
    "${_djinterp_cli_parser}/arg_validator_unit.c"
    "${_djinterp_cli_src}/c/util/sink_common.c"
    "${DJINTERP_ROOT}/src/re_std/c/cstdint/dstdint.c")

set(DJINTERP_CLI_SOURCES
    "${_djinterp_cli_src}/tools/cli/c/cli_edit.c"
    "${_djinterp_cli_src}/tools/cli/c/cli_format.c"
    "${_djinterp_cli_src}/tools/cli/c/cli_io.c"
    "${_djinterp_cli_src}/tools/cli/c/cli_operand.c"
    "${_djinterp_cli_src}/tools/cli/c/cli_plugin.c"
    "${_djinterp_cli_src}/tools/cli/c/cli_program.c"
    "${_djinterp_cli_src}/tools/cli/c/cli_system.c"
    "${_djinterp_cli_src}/c/filesys/file_common.c"
    "${_djinterp_cli_src}/c/filesys/file_desc.c"
    "${_djinterp_cli_src}/c/filesys/file_dir.c"
    "${_djinterp_cli_src}/c/filesys/file_io.c"
    "${_djinterp_cli_src}/c/filesys/file_link.c"
    "${_djinterp_cli_src}/c/filesys/file_lock.c"
    "${_djinterp_cli_src}/c/filesys/file_open.c"
    "${_djinterp_cli_src}/c/filesys/file_ops.c"
    "${_djinterp_cli_src}/c/filesys/file_path.c"
    "${_djinterp_cli_src}/c/filesys/file_pipe.c"
    "${_djinterp_cli_src}/c/filesys/file_seek.c"
    "${_djinterp_cli_src}/c/filesys/file_space.c"
    "${_djinterp_cli_src}/c/filesys/file_stat.c"
    "${_djinterp_cli_src}/c/filesys/file_sync.c"
    "${_djinterp_cli_src}/c/filesys/file_temp.c")
