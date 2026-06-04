# `env_sqlite.h` — SQLite

Deep detection for SQLite. Includes `env_sqlite_config.h`, then `env_db.h`, then the
SQLite header (path from `D_CFG_ENV_SQLITE_C_PATH`). Prefix: **`D_ENV_SQLITE_`**.

SQLite is embedded/serverless, so there is no separate server version — everything
is gated on the library version and on the `SQLITE_ENABLE_*` / `SQLITE_OMIT_*`
compile-time options.

## Configuration (`env_sqlite_config.h`)

| Macro | Meaning |
| --- | --- |
| `D_CFG_ENV_USING_SQLITE` | `1` to include the SQLite header and detect it |
| `D_CFG_ENV_SQLITE_C_PATH` | SQLite header path (e.g. `<sqlite3.h>`) |
| `D_CFG_ENV_SQLITE_CUSTOM` | `1` to skip detection; supply version manually |

## Key output symbols

| Symbol | Meaning |
| --- | --- |
| `D_ENV_SQLITE_VERSION_ID` / `_MAJOR` / `_MINOR` / `_PATCH` / `_STRING` | Resolved SQLite version |
| `D_ENV_SQLITE_VERSION_AT_LEAST(...)` | Version-comparison gate |
| `D_ENV_SQLITE_HAS_*` | Per-feature flags (e.g. `_HAS_ANY_FTS`, `_HAS_ATTACH`, `_HAS_AUTHORIZATION`, `_HAS_AUTOINCREMENT`) |
| `D_ENV_DB_HAS_SQLITE_CLIENT_C` | Client-present flag in the shared `env_db.h` namespace |

## Sections

Config; version encoding; version detection; version comparison; threading model
(single-thread / multi-thread / serialized); full-text search (FTS3/4/5); JSON
support; R*Tree & spatial; virtual-table framework; SQL features (CTEs, window
functions — version-gated); journal & WAL mode; memory management; backup,
serialization & session extensions; security & hardening; extension & loadable-module
support; core API features; VFS detection; omitted features (`SQLITE_OMIT_*`); and
convenience/composite macros.
