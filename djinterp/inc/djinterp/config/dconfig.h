/*******************************************************************************
* djinterp [config]                                                    dconfig.h
*
*   Configuration umbrella. Including this file resolves the ENTIRE config
* graph up front. Use it when you want cross-cutting deductions applied
* deterministically, or to precompile all configuration into a PCH.
*
*   You do NOT need this for normal use: each module pulls its own
* cfg_<module>.h, which pulls cfg_common.h -- so you only pay for the modules
* you include (demand-loading). This umbrella is the opt-in "resolve
* everything" path.
*
*   targets:  no module; a translation unit or a PCH that wants the whole
*             graph resolved includes it once, early
*   requires: cfg_common.h, then every cfg_*.h under config/, each after the
*             configs it depends on
*
*
* path:      /inc/djinterp/config/dconfig.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_CONFIG_DCONFIG_H
#define DJINTERP_CONFIG_DCONFIG_H 1

// djinterp
// Root first: user overrides + testing preset + shared helpers.
#include "cfg_common.h"
#include "djinterp/config/env/cfg_env.h"           // env: shared knobs
#include "djinterp/config/env/cfg_env_lang.h"      // env: language
#include "djinterp/config/env/cfg_env_posix.h"     // env: POSIX / XSI
#include "djinterp/config/env/cfg_env_arch.h"      // env: architecture
#include "djinterp/config/env/cfg_env_os.h"        // env: OS
#include "djinterp/config/env/cfg_env_compiler.h"  // env: compiler
#include "djinterp/config/env/cfg_env_build.h"     // env: build type
#include "djinterp/config/env/os/cfg_env_apple.h"  // env: Apple
#include "djinterp/config/env/os/cfg_env_linux.h"  // env: Linux
#include "djinterp/config/env/ui/cfg_env_qt.h"     // env: Qt
#include "djinterp/config/env/net/tls/cfg_env_tls.h"    // env: TLS
#include "djinterp/config/env/net/curl/cfg_env_curl.h"  // env: curl
#include "djinterp/config/env/net/ssh/cfg_env_ssh.h"    // env: SSH
#include "djinterp/config/env/net/imap/cfg_env_imap.h"  // env: IMAP
#include "djinterp/config/env/cfg_env_printer.h"  // env: the printer
// env/util: the compression codecs, their link pragmas, the archive formats
#include "djinterp/config/env/util/compress/cfg_env_compress.h"
#include "djinterp/config/env/util/compress/cfg_env_compress_link.h"
#include "djinterp/config/env/util/archive/cfg_env_archive.h"
// env/db: the database family's switches, then each database's
#include "djinterp/config/env/db/cfg_env_db.h"
#include "djinterp/config/env/db/arangodb/cfg_env_arangodb.h"
#include "djinterp/config/env/db/cassandra/cfg_env_cassandra.h"
#include "djinterp/config/env/db/db2/cfg_env_db2.h"
#include "djinterp/config/env/db/dynamodb/cfg_env_dynamodb.h"
#include "djinterp/config/env/db/mariadb/cfg_env_mariadb.h"
#include "djinterp/config/env/db/mongodb/cfg_env_mongodb.h"
#include "djinterp/config/env/db/mysql/cfg_env_mysql.h"
#include "djinterp/config/env/db/oracle/cfg_env_oracle.h"
#include "djinterp/config/env/db/postgres/cfg_env_postgres.h"
#include "djinterp/config/env/db/redis/cfg_env_redis.h"
#include "djinterp/config/env/db/sqlite/cfg_env_sqlite.h"
#include "cfg_qualifiers.h"      // storage / linkage qualifiers
// c/util: the variadic macro limits (compression and archives come later)
#include "djinterp/config/c/util/cfg_dmacro.h"
// c/memory: the shared knobs, the memory source, then each allocator
#include "djinterp/config/c/memory/cfg_mem_common.h"
#include "djinterp/config/c/memory/cfg_mem_source.h"
#include "djinterp/config/c/memory/cfg_arena.h"
#include "djinterp/config/c/memory/cfg_pool.h"
// containers: the C family and its table, the C++ array, the table DSL
#include "djinterp/config/c/container/cfg_container.h"
#include "djinterp/config/c/container/table/cfg_table.h"
#include "djinterp/config/cpp/container/array/cfg_array.h"
#include "djinterp/config/cpp/container/table/cfg_table.h"
// configs that define no knobs yet: the C++ iterators, registries,
// functional and text modules, and the command-line options
#include "djinterp/config/cpp/container/iterator/cfg_container_iterator.h"
#include "djinterp/config/cpp/container/registry/cfg_registry.h"
#include "djinterp/config/cpp/functional/cfg_functional.h"
#include "djinterp/config/cpp/text/cfg_text.h"
#include "djinterp/config/cpp/util/cli/cfg_cli_option.h"
// c/event: the shared knobs, then each part's
#include "djinterp/config/c/event/cfg_event_common.h"
#include "djinterp/config/c/event/cfg_event.h"
#include "djinterp/config/c/event/cfg_event_dispatcher.h"
#include "djinterp/config/c/event/cfg_event_handler.h"
#include "djinterp/config/c/event/cfg_event_registry.h"
#include "djinterp/config/c/event/cfg_event_table.h"
// c/filesys: the shared knobs, then each operation's
#include "djinterp/config/c/filesys/cfg_file_common.h"
#include "djinterp/config/c/filesys/cfg_file_desc.h"
#include "djinterp/config/c/filesys/cfg_file_dir.h"
#include "djinterp/config/c/filesys/cfg_file_io.h"
#include "djinterp/config/c/filesys/cfg_file_link.h"
#include "djinterp/config/c/filesys/cfg_file_lock.h"
#include "djinterp/config/c/filesys/cfg_file_open.h"
#include "djinterp/config/c/filesys/cfg_file_ops.h"
#include "djinterp/config/c/filesys/cfg_file_path.h"
#include "djinterp/config/c/filesys/cfg_file_pipe.h"
#include "djinterp/config/c/filesys/cfg_file_seek.h"
#include "djinterp/config/c/filesys/cfg_file_space.h"
#include "djinterp/config/c/filesys/cfg_file_stat.h"
#include "djinterp/config/c/filesys/cfg_file_sync.h"
#include "djinterp/config/c/filesys/cfg_file_temp.h"
// C++ filesys: the path, and the file tree
#include "djinterp/config/cpp/filesys/cfg_file_path.h"
#include "djinterp/config/cpp/container/tree/file/cfg_filesys.h"
// c/util/ostream: the output stream, after filesys (its file backend),
// then text's template formatter over it
#include "djinterp/config/c/util/ostream/cfg_ostream.h"
#include "djinterp/config/c/text/cfg_ostream_template.h"
#include "djinterp/config/c/text/cfg_ostream_format.h"
// options: the C environment source, then the C++ options
#include "djinterp/config/c/option/cfg_option_env.h"
#include "djinterp/config/cpp/option/cfg_option.h"
// output packaging: the library's, then DTest's over it
#include "djinterp/config/cpp/util/output/cfg_output_packaging.h"
#include "djinterp/config/cpp/modules/test/cfg_test_options.h"
// c/modules/test: the object and its parts (the printer comes later)
#include "djinterp/config/c/modules/test/cfg_test_object.h"
#include "djinterp/config/c/modules/test/cfg_test_metadata.h"
#include "djinterp/config/c/modules/test/cfg_test_event.h"
#include "djinterp/config/c/modules/test/cfg_test_counter.h"
#include "djinterp/config/c/modules/test/cfg_test_parity.h"
#include "djinterp/config/c/modules/test/cfg_test.h"
// parse: the substrate, the argument parser, the command-line kit, then
// parser generation, after parse
#include "djinterp/config/c/modules/parse/cfg_parse.h"
#include "djinterp/config/c/modules/parse/cli/arg_parser/cfg_arg_parser.h"
#include "djinterp/config/tools/cli/cfg_cli.h"
#include "djinterp/config/c/modules/parsegen/cfg_parsegen.h"
// tools: bitwisedom
#include "djinterp/config/tools/bitwisedom/cfg_bitwisedom.h"
// c/modules/math: the generic value's boxes, then the wide integer's
#include "djinterp/config/c/modules/math/cfg_value.h"       // boxed holders
#include "djinterp/config/c/modules/math/cfg_value_wide.h"  // wide integer
// net: the foundation, then the bindings, kernels and transports
#include "djinterp/config/c/modules/net/cfg_net.h"            // foundation
#include "djinterp/config/c/modules/net/curl/cfg_curl.h"      // libcurl
#include "djinterp/config/c/modules/net/pop/cfg_pop.h"        // POP3
#include "djinterp/config/c/modules/net/ssh/cfg_ssh.h"        // SSH
#include "djinterp/config/c/modules/net/ssl/cfg_ssl.h"        // SSL/TLS
#include "djinterp/config/c/modules/net/starttls/cfg_starttls.h"  // STARTTLS
#include "djinterp/config/c/modules/net/tcp/cfg_tcp.h"        // TCP transport
#include "djinterp/config/c/modules/net/http/cfg_http.h"      // HTTP
#include "djinterp/config/c/modules/net/ftp/cfg_ftp.h"        // FTP
#include "djinterp/config/c/modules/net/sftp/cfg_sftp.h"      // SFTP
// compression, archives and the test printer include the platform's headers,
// so they come after every config whose answer turns on what a unit included
// first (cfg_file_common.h's path limits, cfg_mem_source.h's aligned API):
// those resolve here as they do on their own
#include "djinterp/config/c/util/compress/cfg_compress.h"
#include "djinterp/config/c/util/archive/cfg_archive.h"
#include "djinterp/config/c/modules/test/cfg_test_printer.h"
// the SQLite layer, after env's SQLite config: resolving everything leaves
// D_CFG_ENV_USING_SQLITE as env's config decided it, off unless set
#include "djinterp/config/c/db/sqlite/cfg_sqlite_common.h"  // SQLite layer
#include "djinterp/config/c/db/sqlite/cfg_sqlite_connection.h"  // its opens
// the MariaDB/MySQL layer, likewise after env's MariaDB config, which leaves
// D_CFG_ENV_USING_MARIADB off unless set
#include "djinterp/config/c/db/mariadb/cfg_mariadb_common.h"  // MariaDB layer
#include "djinterp/config/c/db/mariadb/cfg_mariadb_connection.h"  // connects
// the PostgreSQL layer, likewise after env's PostgreSQL config, which leaves
// D_CFG_ENV_USING_POSTGRESQL off unless set
#include "djinterp/config/c/db/postgres/cfg_postgres_common.h"  // PostgreSQL
#include "djinterp/config/c/db/postgres/cfg_postgres_connection.h"  // connects
// the MongoDB layer, likewise after env's MongoDB config, which leaves
// D_CFG_ENV_USING_MONGODB off unless set
#include "djinterp/config/c/db/mongodb/cfg_mongodb_common.h"  // MongoDB layer
#include "djinterp/config/c/db/mongodb/cfg_mongodb_connection.h"  // connects
// the layers' C++ faces, which read nothing above
#include "djinterp/config/cpp/db/cfg_db_face.h"  // the faces' format check
// #include "<path>/cfg_<sub>.h"  // <- add future subframeworks here


// ===========================================================================
// II.  CROSS-CUTTING DEDUCTIONS
// ===========================================================================
//   Long-range propagation that demand-loading cannot order correctly belongs
// here (module A influencing an otherwise-unrelated module B). Every rule is
// #ifndef-guarded so explicit user overrides still win. Example:
//
//     #if D_CFG_IS_ON(D_CFG_FOO_ADVANCED)
//     #  ifndef D_CFG_BAR_BACKEND
//     #    define D_CFG_BAR_BACKEND 1
//     #  endif
//     #endif
//
//   (none defined yet)


#endif  // DJINTERP_CONFIG_DCONFIG_H
