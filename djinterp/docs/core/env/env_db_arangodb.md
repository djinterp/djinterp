# `env_arangodb.h` — ArangoDB

Deep detection for ArangoDB. Includes `env_arangodb_config.h`, then `env_db.h`, then
the C++ driver header (path from `D_CFG_ENV_ARANGODB_CPP_PATH`). Prefix:
**`D_ENV_ARANGO_`**. (ArangoDB's native driver, fuerte, plus VelocyPack, are a
C++ API.)

## Configuration (`env_arangodb_config.h`)

| Macro | Meaning |
| --- | --- |
| `D_CFG_ENV_USING_ARANGODB` | `1` to include the driver and detect it |
| `D_CFG_ENV_ARANGODB_CPP_PATH` | C++ driver header path |
| `D_CFG_ENV_ARANGODB_CUSTOM` | `1` to skip detection; supply version/edition manually |

## Key output symbols

| Symbol | Meaning |
| --- | --- |
| `D_ENV_ARANGO_VERSION_ID` / `_MAJOR` / `_MINOR` / `_PATCH` / `_STRING` | Resolved ArangoDB version |
| `D_ENV_ARANGO_VERSION_AT_LEAST(...)` | Version-comparison gate |
| `D_ENV_ARANGO_IS_COMMUNITY` / `_IS_ENTERPRISE` | Edition flags |
| `D_ENV_ARANGO_HAS_*` | Per-feature flags (e.g. `_HAS_AQL`, `_HAS_ANALYZERS`, `_HAS_ANALYZER_GEO`, `_HAS_ANALYZER_PIPELINE`, `_HAS_ACTIVE_FAILOVER`) |

## Sections

Config; version encoding; version detection; version comparison; edition (Community
vs Enterprise); client driver & protocol (fuerte, VelocyPack); storage engine
(RocksDB; MMFiles removal); index types (persistent, TTL, fulltext, geo, inverted);
ArangoSearch & Views (IResearch analyzers); AQL features; graph features;
transactions; replication & clustering; security & authentication; Foxx
microservices; backup & restore; collection & schema features; optimizer &
diagnostics; convenience/composite; and deprecation/removal.
