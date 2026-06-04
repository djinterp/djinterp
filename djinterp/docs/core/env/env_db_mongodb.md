# `env_mongodb.h` — MongoDB

Deep detection for MongoDB. Includes `env_mongodb_config.h`, then `env_db.h`, then
the libmongoc client header (path from `D_CFG_ENV_MONGODB_C_PATH`). Prefix:
**`D_ENV_MONGO_`**.

The C driver (libmongoc) and BSON library (libbson) are auto-detected; the **target
server** version is supplied manually for feature gating.

## Configuration (`env_mongodb_config.h`)

| Macro | Meaning |
| --- | --- |
| `D_CFG_ENV_USING_MONGODB` | `1` to include libmongoc and detect the driver |
| `D_CFG_ENV_MONGODB_C_PATH` | libmongoc header path |
| `D_CFG_ENV_MONGODB_CUSTOM` | `1` to skip detection; supply driver version manually |
| `D_CFG_ENV_MONGODB_SERVER_VERSION` | Encoded target server version for gating |

## Key output symbols

| Symbol | Meaning |
| --- | --- |
| `D_ENV_MONGO_DRIVER_VERSION_ID` / `_STRING` | Detected libmongoc version |
| `D_ENV_MONGO_BSON_VERSION_ID` | Detected libbson version |
| `D_ENV_MONGO_SERVER_VERSION_ID` | Target server version (from config) |
| `D_ENV_MONGO_SERVER_AT_LEAST(...)` | Version-comparison gate |
| `D_ENV_MONGO_IS_COMMUNITY` / `_IS_ENTERPRISE` | Edition flags |
| `D_ENV_MONGO_HAS_*` | Per-feature flags (e.g. `_HAS_AGGREGATE_PIPELINE`, `_HAS_AGG_BUCKET`, `_HAS_AGG_DENSIFY`) |
| `D_ENV_DB_HAS_MONGODB_CLIENT_C` / `_CPP` | Client-present flags in the shared `env_db.h` namespace |

## Sections

Config; version encoding; driver detection (libmongoc/libbson); target server
detection; version comparison; edition (Community vs Enterprise); client library
features; BSON type system; index types (hashed, text, 2dsphere, 2d, wildcard —
server-gated); aggregation pipeline features ($lookup, $graphLookup, …);
transactions; change streams; replica set & sharding; query & command features;
time-series collections; security & encryption; Atlas-specific features;
convenience/composite; and deprecation/removal.
