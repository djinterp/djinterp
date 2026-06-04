# `env_dynamodb.h` — Amazon DynamoDB

Deep detection for Amazon DynamoDB. Includes `env_dynamodb_config.h`, then
`env_db.h`, then `<aws/core/VersionConfig.h>` (the AWS SDK for C++ is a C++-only
API; the DynamoDB client header path is `D_CFG_ENV_DYNAMODB_CPP_PATH`). Public
prefix **`D_ENV_DYNAMODB_`**, with an abbreviated `D_ENV_DDB_` form for the
feature/version gates.

DynamoDB is a fully managed service with **no installable server and no server
version**. Detection therefore centers on the AWS SDK (the client), the deployment
target (cloud vs Local), and a manually declared capability profile.

## Configuration (`env_dynamodb_config.h`)

| Macro | Meaning |
| --- | --- |
| `D_CFG_ENV_USING_DYNAMODB` | `1` to include the AWS SDK and detect it |
| `D_CFG_ENV_DYNAMODB_CPP_PATH` | SDK client header path (default `<aws/dynamodb/DynamoDBClient.h>`) |
| `D_CFG_ENV_DYNAMODB_CUSTOM` | `1` to skip detection; supply `D_ENV_DYNAMODB_DETECTED_SDK_VERSION` |
| `D_CFG_ENV_DYNAMODB_TARGET` | Deployment target: `_CLOUD` (0, default) / `_LOCAL` (1) |

Target auto-selects to `_LOCAL` if `D_ENV_DYNAMODB_DETECTED_LOCAL` is defined, else
`_CLOUD`. Service capabilities that depend on region/account/table settings are
declared via `D_ENV_DYNAMODB_DETECTED_*` / `D_ENV_DYNAMODB_NO_*` opt-in macros
rather than a version number.

## Key output symbols

| Symbol | Meaning |
| --- | --- |
| `D_ENV_DYNAMODB_SDK_VERSION_ID` / `_MAJOR` / `_MINOR` / `_PATCH` / `_STRING` | Detected AWS SDK version |
| `D_ENV_DDB_SDK_VERSION_ID` | Abbreviated alias used by the gates |
| `D_ENV_DDB_IS_CLOUD` / `_IS_LOCAL` | Deployment-target flags |
| `D_ENV_DDB_HAS_*` | Per-feature flags (e.g. `_HAS_BATCH_OPS`, `_HAS_CONDITIONAL_WRITES`, `_HAS_AUTO_SCALING`, `_HAS_ATOMIC_COUNTERS`, `_HAS_DAX`, `_HAS_CONTRIBUTOR_INSIGHTS`, `_HAS_ASYNC_API`) |
| `D_ENV_DB_HAS_DYNAMODB_CLIENT_CPP` | Client-present flag in the shared `env_db.h` namespace |

## Sections

Config; version encoding; AWS SDK version detection; SDK version comparison;
deployment target; capability-profile helpers; SDK client features; data model
(partition/sort keys, item attributes, document model); indexing; capacity &
scaling; request & query features (batch get/write, conditional writes, …); streams
& change data; replication, backup & lifecycle (global tables, PITR); caching (DAX);
security (encryption at rest, KMS CMKs); convenience/composite; and a consumer
compatibility layer of `D_ENV_DYNAMODB_*` names.
