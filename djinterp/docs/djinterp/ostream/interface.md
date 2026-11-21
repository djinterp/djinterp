# The output stream, and text on it

The headers for output and text: what each promises, who uses what, and what
stays fixed until the freeze of Friday 2026-12-11. The headers hold the full
contracts; this is the map.

## The rule until the freeze

From the agreement of 2026-11-13 until 12-11:

- **Nothing named here changes in a way that breaks a caller** without the
  agreement of those who use it (the table under "Who uses what").
- **Additions are allowed**: a new function, a new constant, a new
  destination. No member is added to a struct that callers declare or
  copy, since that changes its layout: `dj_pack_text`, `dj_pack_bytes`,
  `dj_pack_sink`, `dj_pack_counting_sink`, `dj_pack_buffer_sink`,
  `dj_ostream`, `dj_ostream_value`, `dj_ostream_spec` and
  `dj_ostream_template`.
- **Every change is announced** with the delivery that makes it, to those
  who use what changed.

## The headers

| Header | What it is | Its unit |
|---|---|---|
| `c/util/sink_common.h` | the spans (`struct dj_pack_text` and `struct dj_pack_bytes`, with their makers and comparison), the sink (`struct dj_pack_sink`, a write function and its context), `d_sink_emit`, and the counting and buffer sinks | `sink_common.c`, in the runtime |
| `c/util/ostream/ostream.h` | the stream: a window of writable bytes and a slow path, transports and storage, a sticky status, integers, cursors, a fixed array, a `dj_string`, a fan-out | `ostream.c` in the runtime; `ostream_string.c`, listed by its users |
| `c/util/ostream/ostream_file.h` | a `FILE*`, lent or owned, through `c/filesys` | `ostream_file.c`, listed by its users |
| `c/util/ostream/ostream_console.h` | standard output and error, written as the platform needs | `ostream_console.c`, listed by its users |
| `c/text/ostream_format.h` | a double in the C form whatever the locale, a double in a locale the caller names, and printf | `ostream_format.c`, in the runtime |
| `c/text/ostream_template.h` | values, the format spec, and templates compiled once | `ostream_template.c`, listed by its users |
| `cpp/util/output/output_stream.hpp` | the C++ face: `DOutputStream`, `DOutputCursor`, `DBufferedOutputStream`, `DOutputStreamSink`, `DConsoleChannel` | header only |
| `cpp/util/output/output_stream_std.hpp` | the standard library both ways: a stream onto a `std::string` or a `std::ostream`, and `DOutputStreambuf` | header only |

The knobs, each fixed when compiling:

- `config/c/util/ostream/cfg_ostream.h`: the console's backend (POSIX,
  Windows' native console, ISO C, or none).
- `config/c/text/cfg_ostream_format.h`: how a locale is applied to one call
  (POSIX 2008 locale objects, the Windows CRT's, or rewriting the decimal
  point).
- `config/c/text/cfg_ostream_template.h`: how deeply blocks may nest, and
  whether JSON and XML replace text that isn't UTF-8 or refuse it.
- `config/c/memory/cfg_mem_source.h`: `D_CFG_MEM_SOURCE_DEFAULT`, where
  printf's rare allocation comes from.

## What each promises

### The spans

- `struct dj_pack_text` is `data` and `length`; `struct dj_pack_bytes` is
  `data` and `size`. Both are borrowed, own nothing, are passed by value,
  and have one layout in C and C++.
- **The length is what counts, not a NUL.** A text span may run past a NUL
  or have none at its end. Its encoding isn't implied.
- `D_PACK_TEXT_NONE` and `D_PACK_BYTES_NONE` are the empty spans, as brace
  initialisers.
- `d_pack_text_from_cstr` measures to the NUL; `d_pack_text_from_span` and
  `d_pack_bytes_from_span` take the length given. A null pointer is the
  empty span, whatever length comes with it. None allocates or fails.
- `d_pack_text_equal` compares the lengths, then the bytes, so an interior
  NUL counts. Two empty spans are equal whatever their pointers.
  `d_pack_text_is_empty` holds for a null pointer or a zero length.

### The sink

- `struct dj_pack_sink` is two members, `write` (a `dj_pack_sink_write_fn`)
  and `context`, passed by value. It owns nothing.
- `write(context, data, size)` returns the number of bytes it took. Fewer
  than `size` is a failure, and the sink may set `errno` to say why.
- `d_sink_emit` returns 1 when the sink took every byte, and 0 otherwise: a
  short write, or a sink with no write function. A zero-size write to a sink
  with one succeeds without calling it.
- **A counting sink** (`struct dj_pack_counting_sink`,
  `d_pack_counting_sink_init`, `d_pack_sink_from_counter`) takes everything
  and keeps the total in `total`.
- **A buffer sink** (`struct dj_pack_buffer_sink`, `d_pack_buffer_sink_init`,
  `d_pack_sink_from_buffer`) writes into memory the caller owns, `capacity`
  bytes of it. It takes every write and counts every byte offered in
  `needed`, which stops at `SIZE_MAX`. A write that fits whole is copied and
  added to `written`. One that doesn't is not copied, `overflow` is set, and
  nothing is copied after that, so the buffer always holds the output's
  first `written` bytes. With no buffer and no capacity it only measures.
- A sink over a counter or a buffer borrows it, so the counter or buffer
  must outlive the sink.

### The stream

- **A window and a slow path.** The stream exposes `[cur, end)`. A write
  that fits is a comparison and a `memcpy`, inline. Only a write that
  doesn't fit calls out of line, and only then is the destination touched.
- **Two kinds of destination.** A transport consumes bytes (a `FILE*`, the
  console, a fan-out, any sink); its window is a buffer the caller lends
  (`d_ostream_set_buffer`). Storage owns the memory the bytes end in (a
  `dj_string`, a fixed array), so a byte is copied once.
- **The status is sticky.** The first failure is kept, and every write after
  it returns 0 without reaching the destination: `D_OSTREAM_OK`, `_INVALID`,
  `_UNSUPPORTED`, `_FAILED` (with `d_ostream_last_errno`) and `_CLOSED`.
  `d_ostream_close` reports the first failure, as `fclose` does.
- **Nothing in `ostream.h` allocates, and it has no `<stdio.h>`.** A stream
  belongs to one thread at a time.
- **Every write returns 1 when every byte was accepted, and 0 otherwise.**
  `d_ostream_written` counts what the stream accepted, so a producer can use
  it as an offset without seeking.
- Every stream lowers back to a sink (`d_ostream_as_sink`), for producers
  that take only a sink.

### Appending to a file

`d_ostream_open_file` with `_append` set opens the file on a descriptor of
its own, opened to append, and hands the system each flush in one write.

**The guarantee:** a record no larger than the stream's buffer, written and
then flushed, reaches a local file whole, however many processes append to
the file at once. No other process's bytes land inside it.

- **Why it holds.** The stream drains its whole buffer in one call at a
  flush, and that call is one system write. The system appends each write
  whole: POSIX's `O_APPEND`, and on Windows a handle opened with
  `FILE_APPEND_DATA` and not `FILE_WRITE_DATA`.
- **Where it doesn't.**
  - A record larger than the buffer goes in more than one write.
  - A short write, which a full disk can cause, leaves part of a record;
    the stream writes the rest, but another process may come between.
  - Appends to a network file system such as NFS aren't atomic, whatever
    the program does.
  - A build whose file layer has no descriptors (the ISO C one) appends
    through stdio, and promises nothing.
- **On Windows,** the guarantee needs the file layer's `d_file_open` to open
  `O_APPEND` with `FILE_APPEND_DATA` alone. Until it does, Windows appends as
  the C runtime does: it moves to the end, then writes, and another process
  can come between the two.
- A file opened to append is created, when it doesn't exist, with
  `D_CFG_FILE_WRITE_CREATE_MODE` (0644 under the umask), as every file the
  file layer writes.

### Formatting

- **Numbers for machines don't follow the locale.**
  `d_ostream_write_double` writes the C form under every locale, and
  `d_ostream_write_double_l` writes in a locale the caller made
  (`d_ostream_locale_init`). The program's locale is never read or changed
  where the platform has locale objects.
- **printf is for people**, and follows the locale as printf does. It
  formats into the window when the window has room, otherwise into a
  256-byte stack buffer. It allocates only for output larger than both, and
  then from the default memory source. Where that is the null source, such
  output fails with `D_OSTREAM_FAILED` and `ENOMEM`.

### Templates and values

- **The language:**
  - `{name}`, `{name!json|csv|md|xml}` and
    `{name:[[fill]align][sign][#][0][width][.precision][type]}`, as
    `std::format` and Python have it;
  - `{?name}...{:}...{/}` and `{^name}...{/}`;
  - `{{` and `}}`.
  - Width and precision count UTF-8 code points.
- **Every name is checked when the template is compiled.** A misspelt field
  is an error with its offset. Running a template parses nothing and looks
  nothing up.
- **Nothing allocates.** A compiled template is an array of operations the
  caller provides, over source text it borrows.
- **A template is compiled once** and is the same wherever it runs. What
  differs between places is a field (stream 7, ruled 11-04).
- **A value is one of five kinds**: none, text, integer, boolean or real. An
  integer is a sign and a 64-bit magnitude held as two 32-bit halves, so a
  value has one layout in every language and at every level, ISO strict
  C++98 included.
- **The statuses** are `D_OSTREAM_TEMPLATE_OK`, `_INVALID`, `_SYNTAX`,
  `_UNKNOWN_FIELD`, `_BAD_SPEC`, `_BAD_CONVERSION`, `_NESTING` and
  `_CAPACITY`, each with its offset in the source.

### JSON

- **The entry point for machines** is `d_ostream_write_value` with a
  specification read from `"!json"` (`d_ostream_spec_parse`). A template's
  `{name!json}` writes the same. What a value writes as is part of the
  interface: changing it breaks a caller.
- **A value is one JSON value.** Text is a string, with `"`, `\` and every
  control character escaped; an integer is decimal with every digit, all 64
  bits of it; a boolean is `true` or `false`; none is `null`.
- **A real with no precision has the fewest significant digits that read
  back as the same double**, and of those the nearest: `0.1`, not
  `0.10000000000000001`. It is in the C form whatever the locale, written
  out in full or with an exponent, whichever is shorter: `1e+300`, and
  `36028797018963970` rather than `3.602879701896397e+16`. A precision the
  specification gives is kept: `"!json:.3"` writes two thirds as `0.667`.
- **NaN and the infinities are `null`.** JSON has no spelling for them. A
  writer that wants another spelling tests the value before writing it.
- **Text is UTF-8.** A sequence that isn't well formed (RFC 3629: no
  overlong forms, no surrogates, nothing past U+10FFFF) is written as
  U+FFFD, one for each maximal part, as Unicode recommends: `"x\xFF\xC3y"`
  is written as x, two U+FFFD and y. Built with `D_CFG_OSTREAM_TEMPLATE_UTF8`
  set to `D_CFG_OSTREAM_TEMPLATE_UTF8_REFUSE`, such a value writes nothing,
  padding included, and the stream fails with `D_OSTREAM_INVALID` and
  `EILSEQ`. XML's conversion does the same; raw text, CSV and Markdown pass
  every byte.
- **Padding is outside the value.** A width pads around it with the fill,
  so the output stays JSON only when the fill is whitespace.

### The C++ face

- **C++98 and up, adding only.** A member added above C++98 never replaces
  one below it, and the data members are the same at every level.
- **Every member is inline and forwards to the C stream.** Failure is a
  `bool`, with `status()` and `last_errno()` to say why, and nothing throws,
  not even across the C boundary from a `std::ostream`.
- **The objects can't be copied.** From C++11 they can be moved.

## Who uses what

On the tree of 2026-11-17:

| Where | What it uses |
|---|---|
| DTest's `test_event.h` and `test_object.h` | `struct dj_pack_sink` |
| the environment printer (`env_printer.h`, `.hpp`, `.c`) | the stream (`d_ostream_init_file`, `_set_buffer`, `_write`, `_write_char`, `_write_cstr`, `_fail`, `_close`, `_status`, `_is_ok`), templates (`_template_compile`, `_template_render`), values (`d_ostream_value_bool`, `_int64`, `_long`, `_none`, `_real`, `_size`, `_text`, `_text_n`, `_to_long`), the codes `D_OSTREAM_OK` and `D_OSTREAM_INVALID`, and `DOutputStream` |
| desc (`desc.h`, `desc_units.h`, `desc_units.c`, its machines, and option_env's `option_env_desc.c`) | the fixed stream (`d_ostream_init_fixed`, `_fixed_length`, `_fixed_truncated`), `d_ostream_fill`, `_write`, `_write_char`, `_write_cstr`, `_write_value`, `_is_ok`, `_close` and `_spec_init`; `struct dj_ostream_value`, `d_ostream_value_none`, `_text`, `_text_n`, `_int64`, `_real` and `_bool`, and `D_OSTREAM_VALUE_*`; `D_OSTREAM_CONVERT_JSON` and `_RAW` |
| the log | everything desc uses; the console (`d_ostream_init_console`, `D_OSTREAM_CONSOLE_ERR`), `d_ostream_open_file`, `_set_buffer` and `_flush`; templates (`d_ostream_template_compile`, `_template_render`, `D_OSTREAM_TEMPLATE_OK` and `_INVALID`), for its codes' templates, in the template language (`{name}`, `{name!json}`, `{name:spec}`, `{?name}...{:}...{/}`); the values `d_ostream_value_long`, `_ulong`, `_uint64` and `_size` |
| the argument parser, C and C++ | `struct dj_pack_sink`, `d_sink_emit`; `struct dj_pack_text` throughout its public headers, and `D_PACK_TEXT_NONE`; `d_pack_text_from_cstr` and `d_pack_text_equal` in its suites |
| vparse's command line | `struct dj_pack_sink`, `struct dj_pack_text` |
| the CLI kit (`tools/cli`): its headers and their C++ faces | `struct dj_pack_sink` |
| the CLI kit's sources and suites | `struct dj_pack_sink`, `dj_pack_sink_write_fn`, `struct dj_pack_text`, `D_PACK_TEXT_NONE`, `d_pack_text_from_cstr`, `d_pack_text_equal` |
| the djinterp program | `struct dj_pack_sink`, `d_sink_emit`, `struct dj_pack_text`; from Fri 11-20, for `describe --format=json` through desc, `struct dj_ostream`, `d_ostream_init` over a sink, `d_ostream_set_buffer`, `d_ostream_flush`, `d_ostream_close`, `d_ostream_is_ok` and `d_ostream_last_errno` |
| bitwisedom's command line | `struct dj_pack_sink` |
| the interpreter's `djin.c` and `djin.h` | `struct dj_pack_sink`, `d_sink_emit` |
| the interpreter's suite | `struct dj_pack_sink`, `struct dj_pack_buffer_sink`, `d_pack_buffer_sink_init`, `d_pack_sink_from_buffer` |
| net, C and C++, throughout (its root, `codec`, `curl`, `ftp`, `http`, `imap`, `pop`, `sftp`, `smtp`, `ssh`, `ssl`, `starttls`, `tcp`, `tls`) | `struct dj_pack_text`, `struct dj_pack_bytes`, `struct dj_pack_sink`, `d_pack_text_from_cstr`; `d_pack_text_from_span` in `imap`'s suite; `d_sink_emit` in `pop` and `imap` |
| compression and archiving | `struct dj_pack_sink`, `d_sink_emit`, the counting and buffer sinks |

Writers of JSON for machines, such as log records and descriptions, use the
entry point under "JSON".

## After 0.8

December's work, from the roadmap:

- the stream fully into text;
- the shortest double that reads back, which JSON writes now, offered to
  every caller;
- the renderers for the man pages and the wiki.

None of it changes what is fixed here before 12-11.
