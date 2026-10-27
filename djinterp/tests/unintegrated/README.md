# Tests not yet in DTest

Every test here was written before it could use DTest, djinterp's test
framework (`inc/djinterp/test`), and is kept as it is until an agent ports
it: the database lane's tests, 29 files, about 46,000 lines. Nothing here
uses DTest yet. They run today exactly as before, from here: the CI
scripts, CMake and the Windows kit point at this folder.

They moved here from `tests/djinterp/c/db` and `tests/djinterp/core/db` on
2026.10.26 (git keeps their history as renames). Only the comments naming
their own location, and their `revised:` lines, changed. One change since,
on the owner's word (2026.10.27, audit decision 30): core/db's shared check
of cancel() (`core/db/t_db_core.hpp`) waits until the server runs the
statement before cancelling it, which a cancel sent sooner can miss. The
layout keeps their depth below the root, so their relative includes
(`../../../../../inc/...`) resolve as before.

## What is here

| Path | What | Run by |
|---|---|---|
| `c/db/<vendor>/t_<vendor>.c` | the C layer against a server (MariaDB, PostgreSQL) or SQLite: one program, its groups listed in `d_tests_<vendor>_all` | `ci/check_db_<vendor>.sh`; CMake `<vendor>_c_tests`; `_handoffs/db_core/windows/run_db_windows.ps1` |
| `c/db/<vendor>/t_<vendor>_stubs.c` | the C layer built without its client: every function's answer | the CI scripts; CMake `<vendor>_stubs_tests`; the Windows kit |
| `c/db/<vendor>/t_<vendor>_cpp.cpp` | the C headers from C++, C++98 to C++23 | the CI scripts; the Windows kit |
| `c/db/<vendor>/t_<vendor>_face.cpp` | the C++ face (`face::<vendor>`), over the stubs and the client | the same |
| `c/db/sqlite/t_sqlite_extension.c` | a loadable extension `t_sqlite.c` loads | `ci/check_db_sqlite.sh`; the Windows kit |
| `c/db/t_db_face_cost.cpp` | the faces' cost against the C calls, at -O2 | `tools/check_db_face_cost.py` |
| `core/db/<vendor>/t_<vendor>_core.cpp` | core/db over each vendor; the checks shared by the three in `core/db/t_db_core.hpp` | the CI scripts; CMake `<vendor>_core_tests`; the Windows kit |
| `core/db/t_database.cpp` | core/db's vocabulary: values, types, the shared parts | the CI scripts; the Windows kit |
| `core/db/database_tests*.{hpp,cpp}` | the older core/db suite, by sections | CMake `database_tests` (its runner is in `build/cmake/config/testing/djinterp/core/db/database`) |

## How they behave, for whoever ports them

- Each is a standalone program with its own `main` (`database_tests`'s is
  its CMake runner): it prints a line per group and one per failed check,
  with the check's expression and line -- and, in the C harnesses, for a
  wrong value what came and what was expected -- ends the harnesses with
  `N checks, M failed`, and exits 0 only when nothing failed.
- Their checks are their own macros: `D_TESTS_<VENDOR>_CHECK`, `_OK`,
  `_RESULT` in the C harnesses, `D_TESTS_<VENDOR>_FACE_CHECK` in the faces,
  `D_TESTS_DB_CORE_CHECK` and `_THROWS` in core/db (`t_db_core.hpp`),
  `D_DB_CHECK` in `database_tests`. A group is a function; the C harnesses
  list theirs in a table (`d_tests_<vendor>_all`), in the headers' order --
  a suite apiece, most naturally.
- The servers come from the environment, never from the tests:
  `DJINTERP_TEST_MARIADB_HOST`, `_PORT`, `_USER`, `_PASSWORD`, `_SOCKET`;
  `DJINTERP_TEST_POSTGRES_*` likewise, with `_DATABASE`. Without a server
  the C harnesses say so and fail; the faces and core/db run only the
  checks that need none, and fail too where
  `DJINTERP_TEST_<VENDOR>_REQUIRED=1`. Each makes and drops databases of
  its own, named for the run.
- `t_sqlite_core.cpp` keeps its files in the folder its first argument
  names (`.` without one); the CI scripts and the Windows kit pass `.` to
  every core/db harness.
- MSVC (C4996) and clang-cl (`-Wdeprecated-declarations`) warn in the
  three `t_<vendor>_core.cpp`, where each checks on purpose that the
  deprecated names (`*_type_support`, `*_feature_support`) still mean
  `*_client_capabilities`: the `#pragma` that silences it is under
  `#if defined(__GNUC__)`, which neither defines. Harmless; a port can
  silence it for both (audit 9.9.13).
- What each group tests is in the comment above it ("Tests the following:
  ..."); why, in `_handoffs/db_core/db_audit.md`.

## When one is ported

Delete it here, and repoint what runs it: `ci/check_db_<vendor>.sh`,
`build/cmake/config/testing/djinterp/core/db/<vendor>/CMakeLists.txt` (and
`database/` for `database_tests`), `.github/workflows/db-<vendor>.yml`,
`_handoffs/db_core/windows/run_db_windows.ps1`,
`tools/check_db_face_cost.py`. A port at another depth below the root
changes the relative includes too.
