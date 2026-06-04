# `env_db2.h` — IBM Db2

Deep detection for IBM Db2. Includes `env_db2_config.h`, then `env_db.h`, then the
Db2 CLI/ODBC client header (path from `D_CFG_ENV_DB2_C_PATH`, default `<sqlcli1.h>`).
Prefix: **`D_ENV_DB2_`**.

Db2 is really three products with divergent version lines and feature sets, so the
target platform family is a required deployment property.

## Configuration (`env_db2_config.h`)

| Macro | Meaning |
| --- | --- |
| `D_CFG_ENV_USING_DB2` | `1` to include the CLI header and detect the client |
| `D_CFG_ENV_DB2_C_PATH` | CLI header path (default `<sqlcli1.h>`) |
| `D_CFG_ENV_DB2_CUSTOM` | `1` to skip detection; supply `D_ENV_DB2_DETECTED_CLIENT_VERSION` |
| `D_CFG_ENV_DB2_SERVER_VERSION` | Encoded target server version for gating (**no default**) |
| `D_CFG_ENV_DB2_PLATFORM` | Target family: `_LUW` (0, default) / `_ZOS` (1) / `_ISERIES` (2) |

Platform auto-selects from `D_ENV_DB2_DETECTED_ZOS` / `_ISERIES`, else LUW.

## Key output symbols

| Symbol | Meaning |
| --- | --- |
| `D_ENV_DB2_VERSION_ID` / `_MAJOR` / `_MINOR` / `_STRING` | Resolved Db2 version |
| `D_ENV_DB2_CLIENT_VERSION_ID` | Detected CLI client version |
| `D_ENV_DB2_SERVER_VERSION_ID` | Target server version (from config) |
| `D_ENV_DB2_SERVER_AT_LEAST(...)` | Version-comparison gate |
| `D_ENV_DB2_IS_LUW` / `_IS_ZOS` / `_IS_ISERIES` | Platform-family flags |
| `D_ENV_DB2_HAS_*` | Per-feature flags (e.g. `_HAS_BOOLEAN_TYPE`, `_HAS_BINARY_TYPE`, `_HAS_ANCHORED_TYPES`, `_HAS_ANALYTICS`, `_HAS_ADVANCED_SECURITY`, `_HAS_AUDIT`) |
| `D_ENV_DB2_*` (compat layer) | Additional names exported into the shared namespace |

## Sections

Config; version encoding; client (CLI) detection; target server detection; platform
family; version comparison; client (CLI) features; SQL language features (CTEs,
recursive CTEs, …); data types (LOBs, XML/pureXML, DECFLOAT, BOOLEAN, BINARY);
transactions & concurrency (savepoints, two-phase commit, …); storage & table
organization (row vs column / BLU); HA & replication (HADR, pureScale,
Q-replication); federation & external data; security; programmability;
convenience/composite; deprecation/removal; and a consumer compatibility layer of
extra `D_ENV_DB2_*` names.
