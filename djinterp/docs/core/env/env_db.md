# `env.db` — Database Environment Detection

The database layer mirrors the core `env.h` design but for database systems. It has
**two tiers**:

1. **`env_db.h`** — a lightweight, self-contained *classifier*. It detects which
   database a translation unit is being built against (by probing vendor client
   macros), assigns it a unified id/name/version, a category bitmask, and a
   capability bitmask. This is all most code needs.
2. **Per-vendor headers** (`env_redis.h`, `env_postgres.h`, …) — *deep* detectors.
   Each pulls in its own config header, then `env_db.h`, then the vendor's client
   header, and exposes a large vendor-specific feature surface (client/driver
   version, target server version, edition/distribution, and dozens to ~150
   capability flags). Each is documented in its own brief: `env_db_<name>.md`.

`env_db.h` does **not** include the per-vendor headers; you include a vendor header
explicitly when you want its detailed surface, and it brings `env_db.h` along.

| Module | Header | Brief |
| --- | --- | --- |
| Unified classifier (this file) | `env_db.h` | `env_db.md` |
| Redis / Valkey | `env_redis.h` | `env_db_redis.md` |
| PostgreSQL | `env_postgres.h` | `env_db_postgres.md` |
| SQLite | `env_sqlite.h` | `env_db_sqlite.md` |
| Oracle Database | `env_oracle.h` | `env_db_oracle.md` |
| MongoDB | `env_mongodb.h` | `env_db_mongodb.md` |
| Oracle MySQL | `env_mysql.h` | `env_db_mysql.md` |
| MariaDB | `env_mariadb.h` | `env_db_mariadb.md` |
| MySQL-family shared | `env_mysql_common.h` | `env_db_mysql.md` |
| IBM Db2 | `env_db2.h` | `env_db_db2.md` |
| Apache Cassandra | `env_cassandra.h` | `env_db_cassandra.md` |
| ArangoDB | `env_arangodb.h` | `env_db_arangodb.md` |
| Amazon DynamoDB | `env_dynamodb.h` | `env_db_dynamodb.md` |

## Configuration

`env_db.h` includes `env_db_config.h`, which defines `D_CFG_ENV_DB_CUSTOM`:

- `0` (default) — automatic detection from vendor-provided preprocessor macros
  (`MARIADB_VERSION_ID`, `MYSQL_VERSION_ID`, `PG_VERSION_NUM`, `SQLITE_VERSION_NUMBER`,
  etc.).
- `1` — skip detection; the host must pre-define a `D_ENV_DB_DETECTED_<VENDOR>`
  variable (e.g. `D_ENV_DB_DETECTED_POSTGRESQL`). Defining one of those also enables
  custom mode.

Each per-vendor header has its own analogous `D_CFG_ENV_<DB>_CUSTOM` and
`D_CFG_ENV_USING_<DB>` switches in its config header.

## II. Database system identification

Each vendor gets a unique single-bit `D_ENV_DB_FLAG_*` id, so a build's database is
identifiable and sets can be tested with bitwise ops.

| Symbol | Value | Symbol | Value |
| --- | --- | --- | --- |
| `D_ENV_DB_FLAG_UNKNOWN` | `0x0000` | `D_ENV_DB_FLAG_MSSQL` | `0x0100` |
| `D_ENV_DB_FLAG_MARIADB` | `0x0001` | `D_ENV_DB_FLAG_DB2` | `0x0200` |
| `D_ENV_DB_FLAG_MYSQL` | `0x0002` | `D_ENV_DB_FLAG_FIREBASE` | `0x0400` |
| `D_ENV_DB_FLAG_POSTGRESQL` | `0x0004` | `D_ENV_DB_FLAG_CASSANDRA` | `0x0800` |
| `D_ENV_DB_FLAG_SQLITE` | `0x0008` | `D_ENV_DB_FLAG_COUCHDB` | `0x1000` |
| `D_ENV_DB_FLAG_MONGODB` | `0x0010` | `D_ENV_DB_FLAG_NEO4J` | `0x2000` |
| `D_ENV_DB_FLAG_REDIS` | `0x0020` | `D_ENV_DB_FLAG_DYNAMODB` | `0x4000` |
| `D_ENV_DB_FLAG_ARANGODB` | `0x0040` | | |
| `D_ENV_DB_FLAG_ORACLE` | `0x0080` | | |

## III. Database categorization

`D_ENV_DB_CAT_*` is a bitmask (a DB may carry several categories). Primary models:
`RELATIONAL`, `DOCUMENT`, `KEY_VALUE`, `GRAPH`, `COLUMN_FAMILY`, `TIME_SERIES`,
`SEARCH_ENGINE`. Secondary traits: `IN_MEMORY`, `EMBEDDED`, `DISTRIBUTED`,
`CLOUD_NATIVE`, `MULTI_MODEL`. SQL support: `SQL`, `NOSQL`.

| Helper | Meaning |
| --- | --- |
| `D_ENV_DB_IS_RDBMS(cat)` | category has `RELATIONAL` |
| `D_ENV_DB_IS_NOSQL(cat)` | category has `NOSQL` |
| `D_ENV_DB_IS_DOCUMENT(cat)` | category has `DOCUMENT` |
| `D_ENV_DB_IS_GRAPH(cat)` | category has `GRAPH` |
| `D_ENV_DB_IS_IN_MEMORY(cat)` | category has `IN_MEMORY` |
| `D_ENV_DB_IS_EMBEDDED(cat)` | category has `EMBEDDED` |

## IV. Feature detection flags

`D_ENV_DB_SUPPORTS_*` is a 32-bit capability bitmask, grouped:

- **Transactions:** `TRANSACTIONS`, `SAVEPOINTS`, `NESTED_TRANSACTIONS`, `TWO_PHASE_COMMIT`
- **ACID:** `ACID`, `ATOMICITY`, `CONSISTENCY`, `ISOLATION`, `DURABILITY`
- **Query:** `JOINS`, `SUBQUERIES`, `VIEWS`, `STORED_PROCEDURES`, `TRIGGERS`, `USER_FUNCTIONS`
- **Indexing:** `INDEXES`, `UNIQUE_CONSTRAINTS`, `FOREIGN_KEYS`, `FULL_TEXT_SEARCH`
- **Replication/clustering:** `REPLICATION`, `CLUSTERING`, `SHARDING`, `AUTO_FAILOVER`
- **Advanced:** `JSON`, `XML`, `SPATIAL_DATA`, `PARTITIONING`
- **Security:** `ENCRYPTION`, `SSL_TLS`, `ROW_LEVEL_SECURITY`, `AUDIT_LOGGING`

## V. Detection output (unified surface)

When a database is detected (auto or custom), `env_db.h` defines:

| Symbol | Meaning |
| --- | --- |
| `D_ENV_DB_ID` | The detected `D_ENV_DB_FLAG_*` value |
| `D_ENV_DB_NAME` | Human-readable name (`"PostgreSQL"`) |
| `D_ENV_DB_VERSION_MAJOR` / `_MINOR` / `_PATCH` | Decomposed version (when the vendor exposes one) |
| `D_ENV_DB_VERSION_ID` | Encoded version (`MAJOR*10000 + MINOR*100 + PATCH`) |
| `D_ENV_DB_CATEGORY` | The DB's `D_ENV_DB_CAT_*` bitmask |
| `D_ENV_DB_FEATURES` | The DB's `D_ENV_DB_SUPPORTS_*` bitmask |

Detection order matters: MariaDB is probed before MySQL (it defines MySQL's macros
too). Vendors without a compile-time version (e.g. SQLite via `SQLITE_VERSION_NUMBER`
notwithstanding, and the NoSQL/cloud engines) may omit the `D_ENV_DB_VERSION_*`
family.

## VI. Convenience macros

| Symbol | Meaning |
| --- | --- |
| `D_ENV_DB_HAS_FEATURE(f)` | `1` if `D_ENV_DB_FEATURES` includes feature `f` |
| `D_ENV_DB_IS_CATEGORY(c)` | `1` if `D_ENV_DB_CATEGORY` includes category `c` |
| `D_ENV_DB_IS_DBMS` | `1` if any database was detected (`D_ENV_DB_ID != UNKNOWN`) |
| `D_ENV_DB_VERSION_AT_LEAST(maj,min,pat)` | `1` if version ≥ given (only when `D_ENV_DB_VERSION_ID` exists) |
| `D_ENV_DB_VERSION_BELOW(maj,min,pat)` | `1` if version < given (same guard) |

## VII. Feature combination shortcuts

| Symbol | Meaning |
| --- | --- |
| `D_ENV_DB_IS_FULLY_ACID` | All four ACID properties supported |
| `D_ENV_DB_SUPPORTS_ADVANCED_TRANSACTIONS` | Transactions **and** (savepoints or nested) |
| `D_ENV_DB_SUPPORTS_RELATIONAL_INTEGRITY` | Unique constraints **and** foreign keys |
| `D_ENV_DB_SUPPORTS_HIGH_AVAILABILITY` | Replication **and** (clustering or auto-failover) |
| `D_ENV_DB_SUPPORTS_SCALE_OUT` | Sharding **or** partitioning |

## Consumer compatibility layer

Each per-vendor header contributes a small set of `D_ENV_DB_HAS_<VENDOR>_CLIENT_C`
/ `_CLIENT_CPP` flags (e.g. `D_ENV_DB_HAS_POSTGRESQL_CLIENT_C`,
`D_ENV_DB_HAS_MONGODB_CLIENT_CPP`) that report whether the vendor's client
library/driver was found. These live in the shared `D_ENV_DB_*` namespace so code
can branch on client availability without depending on the full per-vendor surface.
