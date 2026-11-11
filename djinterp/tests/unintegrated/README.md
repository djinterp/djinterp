# Tests not yet in DTest

Every test here was written before it could use DTest, djinterp's test
framework (`c/modules/test` and `cpp/modules/test`), and is kept as it is
until it is ported. Nothing here uses DTest yet. Each runs today as it
did before it moved here: the CI scripts in `scripts/ci`, CMake's test leaves
and the Windows kits point at this folder.

The folder is laid out as `tests/djinterp/` is: a C test under `c/`, a C++
one under `cpp/`, each below the module it tests. A test keeps its depth below
the root, so its relative includes (`../../../../../inc/...`) resolve from
here.

The database tests moved here on 2026.10.26 (git keeps their history
as renames), and its C harnesses left again on 2026.11.10, as DTest suites
in `tests/djinterp/c/db/<vendor>/` (CMake's `c_<vendor>_tests` and
`c_<vendor>_stubs_tests`); its C++ harnesses are still here. The output
stream's, its template engine's, the machine environment's and the
environment printer's joined them from the stream's delivery of 2026.10.29,
landed on the restructured layout in November. The stream's seven went into
DTest on 2026.11.09; where each went is under "Ported" below.

## What is here

| Path | What | Run by |
|---|---|---|
| `cpp/db/<vendor>/t_<vendor>_cpp.cpp` | the C headers from C++, C++98 to C++23 | the CI scripts |
| `cpp/db/<vendor>/t_<vendor>_face.cpp` | the C++ face (`face::<vendor>`), over the stubs and the client | the CI scripts |
| `cpp/db/<vendor>/t_<vendor>_core.cpp` | cpp/db over each vendor; the checks shared by all of them are in `cpp/db/t_db_core.hpp` | the CI scripts; CMake `<vendor>_core_tests` |
| `cpp/db/t_database.cpp` | cpp/db's vocabulary: values, types, the shared parts | the CI scripts |
| `cpp/db/t_db_face_cost.cpp` | the faces' cost against the C calls, at -O2 | `scripts/check_db_face_cost.py` |
| `cpp/db/database_tests*.{hpp,cpp}` | the older cpp/db suite, by sections | CMake `database_tests` (its runner is in `build/cmake/config/testing/djinterp/cpp/db/database`) |
| `c/t_dstring_grow.c` | `d_string_internal_grow` with a request above `SIZE_MAX / 2`, which once looped for ever; run with `ASAN_OPTIONS=allocator_may_return_null=1` under ASan | proposed for `scripts/ci/check_ckd.sh` |

Rows still to add: the machine environment and the
environment printer (`c/option`, `cpp/option`, `c/env`, `cpp/env`), and the
checked-arithmetic and container overflow tests (`c/t_dckdint.c`,
`c/container/t_container_overflow.c`).

## How they behave, for whoever ports them

- Each is a standalone program with its own `main` (`database_tests`'s is
  its CMake runner): it prints a line per group and one per failed check,
  with the check's expression and line, ends with `N checks, M failed`, and
  exits 0 only when nothing failed.
- Their checks are their own macros: `D_TESTS_<VENDOR>_FACE_CHECK` in the
  faces, `D_TESTS_DB_CORE_CHECK` and `_THROWS` in cpp/db (`t_db_core.hpp`),
  `D_DB_CHECK` in `database_tests`, and the stream's own in each of its
  three. A group is a function. The database's C suites, ported from
  harnesses of this shape, show one way: each group opens its test with the
  block of the header it tests, and records its checks with
  `d_test_suite_check`.
- The servers come from the environment, never from the tests:
  `DJINTERP_TEST_MARIADB_HOST`, `_PORT`, `_USER`, `_PASSWORD`, `_SOCKET`;
  `DJINTERP_TEST_POSTGRES_*` likewise, with `_DATABASE`. Without a server
  the faces and cpp/db run only the checks that need none, and fail where
  `DJINTERP_TEST_<VENDOR>_REQUIRED=1`. Each makes and drops databases of its
  own, named for the run.
- `t_sqlite_core.cpp` keeps its files in the folder its first argument
  names (`.` without one); the CI scripts pass `.` to every cpp/db harness.
- What each group tests is in the comment above it ("Tests the following:
  ...").

## Ported

| Was here | Now, in DTest's C face | Its CMake target |
|---|---|---|
| `c/util/ostream/t_ostream.c` | `tests/djinterp/c/util/ostream/ostream_tests.c` | `c_ostream_tests` |
| `c/text/t_ostream_template.c` | `tests/djinterp/c/text/ostream_template_tests.c` | `c_ostream_template_tests` |
| `c/text/t_text_template_overflow.c` | `tests/djinterp/c/text/text_template_overflow_tests.c` | `c_text_template_overflow_tests` |
| `c/util/pdf/t_pdf_stream.c` | `tests/djinterp/c/util/pdf/pdf_stream_tests.c` | `c_pdf_stream_tests` |
| `cpp/util/output/t_output_stream.cpp` | `tests/djinterp/cpp/util/output/output_stream_tests.cpp` | `output_stream_tests` |
| `cpp/util/pdf/t_pdf_stream.cpp` | `tests/djinterp/cpp/util/pdf/pdf_stream_tests.cpp` | `pdf_stream_tests` |
| `cpp/text/t_writer_stream.cpp` | `tests/djinterp/cpp/text/writer_stream_tests.cpp` | `writer_stream_tests` |

Each group of checks became a test, and each check is recorded against it,
so the counts are the programs' own. `scripts/ci/check_ostream.sh` runs all
seven at every language level.

## When one is ported

Delete it here, and repoint what runs it: its script in `scripts/ci`, its
CMake leaf under `build/cmake/config/testing/`, and its workflow under
`.github/workflows`. A port at another depth below the root changes the
relative includes too.
