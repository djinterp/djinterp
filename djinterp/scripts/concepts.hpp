/******************************************************************************
* djinterp                                                        concepts.hpp
*
* djinterp concepts umbrella module:
*   Single entry point pulling in every C++20 concept module across the
* djinterp framework. Users who want concept-driven constraints across
* multiple domains include this header once; users who want only one
* domain include the focused per-domain header directly.
*
*   PER-DOMAIN HEADERS
*   ==================
*   Core / table:
*     - `core/table_concepts.hpp`             in-memory table concepts
*     - `core/table_layout_concepts.hpp`      table layout concepts
*     - `core/db/database_table_concepts.hpp` database table concepts
*
*   Metadata:
*     - `util/metadata/metadata_concepts.hpp` metadata stack concepts
*
*   Database connection concepts (vendor-specific):
*     - `core/db/mysql/mysql_common_concepts.hpp`  MySQL family
*     - `core/db/mysql/mysql_concepts.hpp`         Oracle MySQL
*     - `core/db/oracle/oracle_concepts.hpp`       Oracle OCI
*     - `core/db/postgres/postgres_concepts.hpp`   PostgreSQL
*     - `core/db/sqlite/sqlite_concepts.hpp`       SQLite
*     - `core/db/mongodb/mongo_concepts.hpp`       MongoDB
*     - `core/db/arangodb/arango_concepts.hpp`     ArangoDB
*
*   USAGE
*   =====
*     #include "djinterp/concepts.hpp"
*
*     template<djinterp::table _T>
*     void process_table(_T& _t) { ... }
*
*     template<djinterp::database_table _T>
*         requires djinterp::named_column_table<_T>
*     void lookup_by_name(_T& _t, std::string_view _col) { ... }
*
*     template<typename _T>
*         requires djinterp::table<_T> && djinterp::table_metadata_concept<
*                      djinterp::table_metadata<_T>>
*     void render_with_metadata(_T& _t) { ... }
*
*   This umbrella header includes ALL vendor connection-concept files,
* which transitively include the vendor headers. For build-time
* economy, prefer the focused per-domain headers when you don't need
* cross-domain constraints.
*
*   PORTABILITY
*   ===========
*     version: requires C++20 (concepts).
*
*
* path:      /scripts/concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_CONCEPTS_
#define DJINTERP_CONCEPTS_ 1


// =============================================================================
// FEATURE GATE
// =============================================================================

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "concepts.hpp requires C++20 concepts support."
#endif


// =============================================================================
// PER-DOMAIN CONCEPTS
// =============================================================================

// core / table
#include "./core/table_concepts.hpp"
#include "./core/table_layout_concepts.hpp"
#include "./core/db/database_table_concepts.hpp"

// metadata
#include "./util/metadata/metadata_concepts.hpp"

// database connection concepts (vendor-specific)
#include "./core/db/mysql/mysql_common_concepts.hpp"
#include "./core/db/mysql/mysql_concepts.hpp"
#include "./core/db/oracle/oracle_concepts.hpp"
#include "./core/db/postgres/postgres_concepts.hpp"
#include "./core/db/sqlite/sqlite_concepts.hpp"
#include "./core/db/mongodb/mongo_concepts.hpp"
#include "./core/db/arangodb/arango_concepts.hpp"


#endif  // DJINTERP_CONCEPTS_
