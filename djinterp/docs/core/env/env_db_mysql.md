# `env_mysql.h`, `env_mariadb.h`, `env_mysql_common.h` — MySQL family

The MySQL family is detected by three cooperating headers. Both vendor headers pull
in the shared **`env_mysql_common.h`** rather than `env_db.h` directly; the common
header does vendor disambiguation and provides version-agnostic, family-wide flags.

| Header | Vendor | Prefix |
| --- | --- | --- |
| `env_mysql_common.h` | shared MySQL-family infrastructure | `D_ENV_MYSQL_COMMON_` |
| `env_mysql.h` | Oracle MySQL | `D_ENV_MYSQL_` |
| `env_mariadb.h` | MariaDB | `D_ENV_MARIADB_` |

`env_mysql.h` includes `env_mysql_common.h`; `env_mariadb.h` includes it via
`../mysql/env_mysql_common.h`. The common header pulls in both the MariaDB and MySQL
config headers so it can disambiguate which family macros are present (MariaDB
defines MySQL's macros too, so order/disambiguation matters).

## `env_mysql_common.h`

Vendor disambiguation, version encode/decode helpers, client-library detection, and
version-agnostic capability flags shared across the family.

| Symbol | Meaning |
| --- | --- |
| `D_ENV_MYSQL_COMMON_FAMILY_DETECTED` | A MySQL-family client/server was identified |
| `D_ENV_MYSQL_COMMON_ENCODE_VERSION(...)` / `_DECODE_MAJOR/_MINOR/_PATCH(...)` | Version codec helpers |
| `D_ENV_MYSQL_COMMON_HAS_*` | Shared flags (e.g. `_HAS_CLIENT_LIB`, `_HAS_ANY_SSL`, `_HAS_BLOB_TYPES`, `_HAS_COMMIT_ROLLBACK`, storage engines like `_HAS_ARCHIVE_ENGINE` / `_HAS_BLACKHOLE_ENGINE` / `_HAS_CSV_ENGINE`) |

Sections: vendor disambiguation; version encoding; client-library detection; C API
features (version-agnostic); core storage-engine detection; SSL/TLS library
detection; common data types; character-set basics; platform connection methods.

## `env_mysql.h` (Oracle MySQL)

Naming `D_ENV_MYSQL_[CATEGORY]_[FEATURE]` (1/0) and `D_ENV_MYSQL_VERSION_[COMPONENT]`.
Config switch `D_CFG_ENV_MYSQL_CUSTOM`; custom mode uses `D_ENV_MYSQL_DETECTED_*`.

| Symbol | Meaning |
| --- | --- |
| `D_ENV_MYSQL_VERSION_ID` / `_MAJOR` / `_MINOR` / `_PATCH` / `_STRING` | Resolved version |
| `D_ENV_MYSQL_VERSION_AT_LEAST(...)` | Version-comparison gate |
| `D_ENV_MYSQL_DEFAULT_AUTH_IS_CACHING_SHA2` / `_DEFAULT_CHARSET_IS_UTF8MB4` | Default-config indicators |
| `D_ENV_MYSQL_HAS_*` | Per-feature flags (e.g. `_HAS_ATOMIC_DDL`, `_HAS_ASYNC_API`, `_HAS_AUTH_CACHING_SHA2`, `_HAS_AUTH_FIDO`, `_HAS_AUDIT_LOG`) |

Sections: config; vendor guard; version detection; version comparison; client
library; C API (version-gated); SSL/TLS; authentication; data types; storage engines;
InnoDB features; replication & HA; X Protocol & X DevAPI; character set; optimizer &
performance; security & administration; platform; composite; deprecation/removal.

## `env_mariadb.h` (MariaDB)

Naming `D_ENV_MARIADB_[CATEGORY]_[FEATURE]` (1/0) and `D_ENV_MARIADB_VERSION_[COMPONENT]`.
Config switch `D_CFG_ENV_MARIADB_CUSTOM`; custom mode uses `D_ENV_MARIADB_DETECTED_*`.

| Symbol | Meaning |
| --- | --- |
| `D_ENV_MARIADB_VERSION_ID` / `_MAJOR` / `_MINOR` / `_PATCH` / `_STRING` | Resolved version |
| `D_ENV_MARIADB_VERSION_AT_LEAST(...)` | Version-comparison gate |
| `D_ENV_MARIADB_DEFAULT_AUTH_IS_ED25519` / `_DEFAULT_CHARSET_IS_UTF8MB4` | Default-config indicators |
| `D_ENV_MARIADB_HAS_*` | Per-feature flags (e.g. `_HAS_ARIA`, `_HAS_APPLICATION_TIME_PERIODS`, `_HAS_ACCOUNT_LOCKING`, `_HAS_ASYNC_API`) |

Sections: config; version detection; version comparison; client library; C API
(version-gated); SSL/TLS; authentication; storage engines (MariaDB-specific); SQL
extensions; data types; InnoDB features; replication & HA; character set; optimizer &
performance; security & administration; platform; composite; deprecation/removal.
