# `env_postgres.h` — PostgreSQL

Deep detection for PostgreSQL. Includes `env_postgres_config.h`, then `env_db.h`,
then the libpq client header (path from `D_CFG_ENV_POSTGRESQL_C_PATH`). Prefix:
**`D_ENV_PG_`**.

PostgreSQL exposes a real compile-time version (`PG_VERSION_NUM`), so most features
are gated by version rather than a separate server property.

## Configuration (`env_postgres_config.h`)

| Macro | Meaning |
| --- | --- |
| `D_CFG_ENV_USING_POSTGRESQL` | `1` to include libpq and detect the client |
| `D_CFG_ENV_POSTGRESQL_C_PATH` | libpq header path (e.g. `<libpq-fe.h>`) |
| `D_CFG_ENV_POSTGRESQL_CUSTOM` | `1` to skip detection; supply detected version manually |

## Key output symbols

| Symbol | Meaning |
| --- | --- |
| `D_ENV_PG_VERSION_ID` / `_MAJOR` / `_STRING` | Resolved PostgreSQL version |
| `D_ENV_PG_VERSION_AT_LEAST(...)` | Version-comparison gate |
| `D_ENV_PG_DEFAULT_AUTH_IS_SCRAM` | Default auth method indicator |
| `D_ENV_PG_HAS_*` | Per-feature flags (e.g. `_HAS_ARRAY_TYPES`, `_HAS_ADVISORY_LOCKS`, `_HAS_AUTH_GSS`, `_HAS_AUTH_CERT`) |
| `D_ENV_DB_HAS_POSTGRESQL_CLIENT_C` / `_CPP` | Client-present flags in the shared `env_db.h` namespace |

## Sections

Config; version encoding; version detection; version comparison; client library
(libpq) detection; SSL/TLS and authentication; data types (JSONB, range/multirange,
arrays, domains — version-gated); index types (B-tree, Hash, GiST, SP-GiST, GIN,
BRIN); SQL features (CTEs, window functions, LATERAL, UPSERT — version-gated);
partitioning; replication & HA; parallel query & DDL; extension & procedural-language
framework; contrib extensions (hstore, pg_trgm, citext, ltree, …); third-party
extensions; vacuum & storage; optimizer & performance; security & administration;
platform integration; and convenience/composite macros.
