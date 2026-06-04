# `env_oracle.h` — Oracle Database

Deep detection for Oracle Database. Includes `env_oracle_config.h`, then `env_db.h`,
then the OCI client header (path from `D_CFG_ENV_ORACLE_C_PATH`). Prefix:
**`D_ENV_ORA_`**.

## Configuration (`env_oracle_config.h`)

| Macro | Meaning |
| --- | --- |
| `D_CFG_ENV_USING_ORACLE` | `1` to include OCI and detect the client |
| `D_CFG_ENV_ORACLE_C_PATH` | OCI header path (e.g. `<oci.h>`) |
| `D_CFG_ENV_ORACLE_CUSTOM` | `1` to skip detection; supply version/edition manually |

## Key output symbols

| Symbol | Meaning |
| --- | --- |
| `D_ENV_ORA_VERSION_ID` / `_MAJOR` / `_MINOR` / `_STRING` | Resolved Oracle version |
| `D_ENV_ORA_VERSION_AT_LEAST(...)` | Version-comparison gate |
| `D_ENV_ORA_EDITION_XE` / `_EDITION_SE` / `_EDITION_EE` | Edition (Express / Standard Edition 2 / Enterprise) |
| `D_ENV_ORA_HAS_*` | Per-feature flags (e.g. `_HAS_23AI_FEATURES`, `_HAS_ACTIVE_DATA_GUARD`, `_HAS_ANALYTIC_FUNCTIONS`, `_HAS_APPLICATION_CONTAINERS`) |
| `D_ENV_DB_HAS_ORACLE_CLIENT_C` / `_CPP` | Client-present flags in the shared `env_db.h` namespace |

## Sections

Config; version encoding; version detection; version comparison; edition & option
detection (EE options: Partitioning, RAC, In-Memory, …); OCI client detection;
multitenant architecture (CDB/PDB); SQL features (analytic functions, MODEL clause,
…); JSON support; index types (B-tree, bitmap, function-based, domain, …);
partitioning; replication & HA; flashback; security; in-memory & performance;
PL/SQL & procedural; XML/Text/spatial; advanced queuing; generated & virtual
columns; platform integration; and convenience/composite macros.
