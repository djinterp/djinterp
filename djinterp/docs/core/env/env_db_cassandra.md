# `env_cassandra.h` — Apache Cassandra

Deep detection for Apache Cassandra. Includes `env_cassandra_config.h`, then
`env_db.h`, then the DataStax C/C++ driver header (path from
`D_CFG_ENV_CASSANDRA_C_PATH`, default `<cassandra.h>`). Public prefix:
**`D_ENV_CASSANDRA_`** (with an abbreviated `D_ENV_CASS_` form used internally for
the driver/server version and edition gates).

The cpp-driver is auto-detected from `CASS_VERSION_*`; the **target server** version
and edition are manual.

## Configuration (`env_cassandra_config.h`)

| Macro | Meaning |
| --- | --- |
| `D_CFG_ENV_USING_CASSANDRA` | `1` to include the cpp-driver and detect it |
| `D_CFG_ENV_CASSANDRA_C_PATH` | Driver header path (default `<cassandra.h>`) |
| `D_CFG_ENV_CASS_CUSTOM` | `1` to skip detection; supply `D_ENV_CASS_DETECTED_DRIVER_VERSION` |
| `D_CFG_ENV_CASS_SERVER_VERSION` | Encoded target server version for gating (**no default**) |

Edition is declared via `D_ENV_CASS_DETECTED_DSE` / `_ASTRA`.

## Key output symbols

| Symbol | Meaning |
| --- | --- |
| `D_ENV_CASSANDRA_VERSION_ID` / `_MAJOR` / `_MINOR` / `_PATCH` / `_STRING` | Resolved Cassandra version |
| `D_ENV_CASS_DRIVER_VERSION_ID` | Detected cpp-driver version |
| `D_ENV_CASS_SERVER_VERSION_ID` | Target server version (from config) |
| `D_ENV_CASS_SERVER_AT_LEAST(...)` | Version-comparison gate |
| `D_ENV_CASS_IS_DSE` / `_IS_ASTRA` | Edition flags (DataStax Enterprise / Astra) |
| `D_ENV_CASSANDRA_HAS_*` | Per-feature flags (e.g. `_HAS_LWT`, `_HAS_MATERIALIZED_VIEWS`, `_HAS_COUNTERS`, `_HAS_DURATION`, `_HAS_PROTOCOL_V3`, `_HAS_AUTH`, `_HAS_AUDIT_LOGGING`) |

## Sections

Config; version encoding; driver detection (DataStax cpp-driver); target server
detection; version comparison; edition (Apache vs DSE); client driver features; CQL
protocol & statement features (prepared statements, …); data model & type system
(partition/clustering keys, wide rows, static columns); indexing & views;
user-defined functions & aggregates; storage engine & compaction; replication,
consistency & distribution (tunable consistency, replication strategies);
security (auth/authorization/encryption); DSE-specific features; convenience/composite;
deprecation/removal; and a consumer compatibility layer of `D_ENV_CASSANDRA_*` names.
