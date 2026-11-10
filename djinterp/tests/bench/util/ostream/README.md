# The output stream's benchmarks

The stream's cost, measured, as the stream's delivery of 2026.10.28 left it
(ruled by the owner on 2026.11.08: kept here, behind a CMake option lane A
adds). Rerun them when the stream changes, and keep the results beside them.

| File | What it measures |
|---|---|
| `bench.c` | the C kernel: characters, cursors, strings, integers and printf into a discarding sink through a 64 KiB buffer, against a char array written by hand (the floor); `-DBENCH_OLD` builds the same loops against the 2026.10.07 kernel |
| `bench_cpp.cpp` | the C++ face against `std::ofstream`, `std::to_chars` and `std::format_to` (C++20) |
| `bench_stdio.c` | stdio's `fputc`, `putc_unlocked`, `fwrite` and `fprintf`, for scale |
| `bench_template.c` | the template engine against hand-written lines, and the environment printer |
| `results_2026.10.28.txt`, `results_template_2026.10.28.txt` | the figures of 2026.10.28 (GCC 13.3, -O2, 2 vCPUs) |

Each figure is the best of seven runs. Until lane A's CMake option lands,
build them from the tree's root:

```sh
U="src/djinterp/c/util"
C="$U/sink_common.c $U/ostream/ostream.c $U/ostream/ostream_string.c \
   $U/ostream/ostream_file.c $U/ostream/ostream_console.c \
   src/djinterp/c/text/ostream_format.c src/djinterp/c/dstring.c \
   src/djinterp/c/string_fn.c src/djinterp/c/djinterp.c \
   src/djinterp/c/memory/dmemory.c src/djinterp/c/filesys/file_open.c \
   src/djinterp/c/filesys/file_common.c"
E="src/djinterp/c/option/option_env.c src/djinterp/c/option/option_env_registry.c \
   src/djinterp/c/option/option_set_common.c src/djinterp/c/option/option_common.c \
   src/djinterp/env/env_printer.c src/djinterp/c/text/ostream_template.c"
B="tests/bench/util/ostream"
gcc -O2 -std=c11 -D_XOPEN_SOURCE=700 -Iinc $B/bench.c $C -o bench
gcc -O2 -std=c11 -D_XOPEN_SOURCE=700 $B/bench_stdio.c -o bench_stdio
gcc -O2 -std=c11 -D_XOPEN_SOURCE=700 -Iinc $B/bench_template.c $C $E -lm \
    -o bench_template
for u in $C; do gcc -O2 -std=c11 -D_XOPEN_SOURCE=700 -Iinc -c "$u" \
    -o "$(basename "$u" .c).o"; done
g++ -O2 -std=c++20 -D_XOPEN_SOURCE=700 -Iinc $B/bench_cpp.cpp *.o -o bench_cpp
```

On 2026.11.09 the C kernel's figures were within the 2026.10.28 ones or
below (write_char 0.63 ns, write_long 6.75 ns, printf 64.5 ns), on the same
kind of machine.
