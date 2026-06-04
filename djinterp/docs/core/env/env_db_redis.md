# `env_redis.h` — Redis / Valkey

Deep detection for Redis. Includes `env_redis_config.h`, then `env_db.h`, then the
hiredis client header (path from `D_CFG_ENV_REDIS_C_PATH`, default
`<hiredis/hiredis.h>`). Prefix: **`D_ENV_REDIS_`**.

Because the Redis **server** version is a runtime property, the client (hiredis) is
auto-detected from `HIREDIS_MAJOR/MINOR/PATCH`, while the **target server** version
is supplied manually for feature gating.

## Configuration (`env_redis_config.h`)

| Macro | Meaning |
| --- | --- |
| `D_CFG_ENV_USING_REDIS` | `1` to include hiredis and detect the client (default `0`) |
| `D_CFG_ENV_REDIS_C_PATH` | hiredis header path (default `<hiredis/hiredis.h>`) |
| `D_CFG_ENV_REDIS_CUSTOM` | `1` to skip client detection; supply `D_ENV_REDIS_DETECTED_CLIENT_VERSION` |
| `D_CFG_ENV_REDIS_SERVER_VERSION` | Encoded target server version (`MAJOR*10000+MINOR*100+PATCH`); **no default** so "unset" is distinguishable |

Distribution is declared manually via `D_ENV_REDIS_DETECTED_VALKEY` / `_ENTERPRISE`
/ `_CLOUD` / `_STACK`.

## Key output symbols

| Symbol | Meaning |
| --- | --- |
| `D_ENV_REDIS_VERSION_ID` / `_MAJOR` / `_MINOR` / `_PATCH` / `_STRING` | Resolved Redis version |
| `D_ENV_REDIS_CLIENT_VERSION_ID` | Detected hiredis version |
| `D_ENV_REDIS_SERVER_VERSION_ID` | Target server version (from config) |
| `D_ENV_REDIS_CLIENT_AT_LEAST(...)` / `D_ENV_REDIS_SERVER_AT_LEAST(...)` | Version-comparison gates |
| `D_ENV_REDIS_IS_OSS` / `_IS_VALKEY` / `_IS_ENTERPRISE` / `_IS_CLOUD` / `_IS_STACK` | Distribution flags |
| `D_ENV_REDIS_CLIENT_HAS_RESP3` | hiredis RESP3 support |
| `D_ENV_DB_HAS_REDIS_CLIENT_C` | Client-present flag in the shared `env_db.h` namespace |

## Sections

Config; version encoding; client (hiredis) detection; target server detection;
version comparison; distribution (OSS / Stack / Enterprise / Cloud / Valkey); RESP
protocol (RESP2 vs RESP3); client library features; core data structures;
command-group/execution features; persistence (RDB/AOF/hybrid); replication & HA;
memory & threading; security (`HAS_ACL`, `HAS_AUTH`, …); modules (RediSearch,
RedisJSON, RedisTimeSeries, …); convenience/composite; deprecation/removal; and a
consumer compatibility layer of extra `D_ENV_REDIS_*` names.
