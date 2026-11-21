# The output stream's benchmarks

The stream's cost, measured, as the stream's delivery of 2026.10.28 left it
(ruled on 2026.11.08: kept here, behind the CMake option
DJINTERP_BUILD_BENCHMARKS). Rerun them when the stream changes, and keep the
results beside them.

| File | What it measures |
|---|---|
| `bench.c` | the C kernel: characters, cursors, strings, integers and printf into a discarding sink through a 64 KiB buffer, against a char array written by hand (the floor); `-DBENCH_OLD` builds the same loops against the 2026.10.07 kernel |
| `bench_cpp.cpp` | the C++ face against `std::ofstream`, `std::to_chars` and `std::format_to` (C++20) |
| `bench_stdio.c` | stdio's `fputc`, `putc_unlocked`, `fwrite` and `fprintf`, for scale |
| `bench_template.c` | the template engine against hand-written lines, and the environment printer |
| `results_2026.10.28.txt`, `results_template_2026.10.28.txt` | the figures of 2026.10.28 (GCC 13.3, -O2, 2 vCPUs) |

Each figure is the best of seven runs. Build them with the tree's CMake, from
a build directory of their own, at -O2: RelWithDebInfo is GCC's and Clang's
-O2, where Release is -O3.

```sh
cmake -G Ninja <tree>/djinterp/build/cmake/config \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDJINTERP_BUILD_BENCHMARKS=ON
cmake --build . --target bench_ostream bench_cpp bench_stdio bench_template
```

The four programs land in the build directory's `bench/`.

On 2026.11.09 the C kernel's figures were within the 2026.10.28 ones or
below (write_char 0.63 ns, write_long 6.75 ns, printf 64.5 ns), on the same
kind of machine.
