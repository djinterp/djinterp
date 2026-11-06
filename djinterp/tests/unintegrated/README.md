# Tests not yet in DTest

Every test here was written before it could use DTest, djinterp's test
framework (`c/modules/test` and `cpp/modules/test`), and is kept as it is
until its lane ports it. Nothing here uses DTest yet. Each runs today as it
did before it moved here: the CI scripts in `scripts/ci`, CMake's test leaves
and the Windows kits point at this folder.

The folder is laid out as `tests/djinterp/` is: a C test under `c/`, a C++
one under `cpp/`, each below the module it tests. A test keeps its depth below
the root, so its relative includes (`../../../../../inc/...`) resolve from
here.

The database lane's tests moved here on 2026.10.26 (git keeps their history
as renames); the output stream's, its template engine's, the machine
environment's and the environment printer's joined them from the stream's
delivery of 2026.10.29, landed on the restructured layout by lanes D and E in
November.

## What is here

| Path | What | Run by |
|---|---|---|
| `c/db/<vendor>/t_<vendor>.c` | the C layer against a server (MariaDB, PostgreSQL, MongoDB) or SQLite: one program, its groups listed in `d_tests_<vendor>_all` | `scripts/ci/check_db_<vendor>.sh`; CMake `<vendor>_c_tests` |
| `c/db/<vendor>/t_<vendor>_stubs.c` | the C layer built without its client: every function's answer | the CI scripts; CMake `<vendor>_stubs_tests` |
| `c/db/sqlite/t_sqlite_extension.c` | a loadable extension `t_sqlite.c` loads | `scripts/ci/check_db_sqlite.sh` |
| `cpp/db/<vendor>/t_<vendor>_cpp.cpp` | the C headers from C++, C++98 to C++23 | the CI scripts |
| `cpp/db/<vendor>/t_<vendor>_face.cpp` | the C++ face (`face::<vendor>`), over the stubs and the client | the CI scripts |
| `cpp/db/<vendor>/t_<vendor>_core.cpp` | cpp/db over each vendor; the checks shared by all of them are in `cpp/db/t_db_core.hpp` | the CI scripts; CMake `<vendor>_core_tests` |
| `cpp/db/t_database.cpp` | cpp/db's vocabulary: values, types, the shared parts | the CI scripts |
| `cpp/db/t_db_face_cost.cpp` | the faces' cost against the C calls, at -O2 | `scripts/check_db_face_cost.py` |
| `cpp/db/database_tests*.{hpp,cpp}` | the older cpp/db suite, by sections | CMake `database_tests` (its runner is in `build/cmake/config/testing/djinterp/cpp/db/database`) |
| `c/util/ostream/t_ostream.c` | the output stream's kernel (`c/util/ostream/ostream.h`), its destinations and cursors, its doubles and printf (`c/text/ostream_format.h`), and a differential test of random operations on every kind of destination against a byte array | `scripts/ci/check_ostream.sh`; CMake `ostream_c_tests` |
| `c/text/t_ostream_template.c` | the template engine (`c/text/ostream_template.h`): values, one value's formatting, the conversions, every compile error, running templates; integers and reals against snprintf at random | `scripts/ci/check_ostream.sh`; CMake `ostream_template_c_tests` |
| `cpp/util/output/t_output_stream.cpp` | the stream's C++ face and its std adapters (`cpp/util/output`), with its doubles and printf (`cpp/text/output_format.hpp`), C++98 to C++26 | `scripts/ci/check_ostream.sh`; CMake `output_stream_cpp_tests` |
| `c/util/pdf/t_pdf_stream.c` | the PDF writer's numbers on the stream (the decimal-point ruling): `d_pdf_num` and `d_pdf_set_color` against snprintf at random and under `de_DE.UTF-8` and `ps_AF.UTF-8`, snprintf's contract, and `d_pdf_document_save_to` with a table from before the slot | `scripts/ci/check_ostream.sh` |
| `cpp/util/pdf/t_pdf_stream.cpp` | the C++ PDF writer against the C one at random under both locales, and `DPdfDocument::save_to`; C++17 and up | `scripts/ci/check_ostream.sh` |
| `cpp/text/t_writer_stream.cpp` | `writer.hpp`'s sinks and writers, `printer.hpp`'s `file_printer` and a document's `to_stream`/`to_file` on the stream: failed writes kept and reported by `ok()`; C++17 and up | `scripts/ci/check_ostream.sh` |
| `c/text/t_text_template_overflow.c` | `text_template.c`'s two allocation sizes, checked with `d_ckd_mul`: includes the unit to reach its static growth; run under ASan | proposed for `scripts/ci/check_ckd.sh` |
| `c/t_dstring_grow.c` | `d_string_internal_grow` with a request above `SIZE_MAX / 2`, which once looped for ever; run with `ASAN_OPTIONS=allocator_may_return_null=1` under ASan | proposed for `scripts/ci/check_ckd.sh` |

Lanes A and E add their own rows: the machine environment and the
environment printer (`c/option`, `cpp/option`, `c/env`, `cpp/env`), and the
checked-arithmetic and container overflow tests (`c/t_dckdint.c`,
`c/container/t_container_overflow.c`).

## How they behave, for whoever ports them

- Each is a standalone program with its own `main` (`database_tests`'s is
  its CMake runner): it prints a line per group and one per failed check,
  with the check's expression and line -- and, in the C harnesses, for a
  wrong value what came and what was expected -- ends with
  `N checks, M failed`, and exits 0 only when nothing failed.
- Their checks are their own macros: `D_TESTS_<VENDOR>_CHECK`, `_OK`,
  `_RESULT` in the database's C harnesses, `D_TESTS_<VENDOR>_FACE_CHECK` in
  the faces, `D_TESTS_DB_CORE_CHECK` and `_THROWS` in cpp/db
  (`t_db_core.hpp`), `D_DB_CHECK` in `database_tests`, and the stream's own
  in each of its three. A group is a function; the database's C harnesses
  list theirs in a table (`d_tests_<vendor>_all`), in the headers' order --
  a suite apiece, most naturally.
- The servers come from the environment, never from the tests:
  `DJINTERP_TEST_MARIADB_HOST`, `_PORT`, `_USER`, `_PASSWORD`, `_SOCKET`;
  `DJINTERP_TEST_POSTGRES_*` likewise, with `_DATABASE`. Without a server
  the C harnesses say so and fail; the faces and cpp/db run only the
  checks that need none, and fail too where
  `DJINTERP_TEST_<VENDOR>_REQUIRED=1`. Each makes and drops databases of
  its own, named for the run.
- `t_sqlite_core.cpp` keeps its files in the folder its first argument
  names (`.` without one); the CI scripts pass `.` to every cpp/db harness.
- The stream's harnesses write their temporary files into the working
  folder and remove them; `t_ostream.c` checks doubles under `de_DE.UTF-8`
  and `ps_AF.UTF-8` where those locales are installed, and says so where
  they are not.
- What each group tests is in the comment above it ("Tests the following:
  ...").

## When one is ported

Delete it here, and repoint what runs it: its script in `scripts/ci`, its
CMake leaf under `build/cmake/config/testing/`, and its workflow under
`.github/workflows`. A port at another depth below the root changes the
relative includes too.
