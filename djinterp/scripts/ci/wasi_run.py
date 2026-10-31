#!/usr/bin/env python3
"""wasi_run.py -- runs a WASI program under wasmtime, in a directory of the
host's that the program sees as its working directory.

  usage:  scripts/ci/wasi_run.py <program.wasm> <directory>

The program gets its own name as its only argument, this process's standard
output and error, and <directory> preopened as ".", which WASI's libc resolves
relative paths against; it reaches nothing else of the host's. The exit
status is the program's. wasmtime is pip's (pip install wasmtime), the
runtime with its Python bindings, so a runner needs no other install;
scripts/ci/check_filesys_platforms.sh runs c/filesys's WASI harnesses with it.
"""
import sys

import wasmtime


def main():
    if len(sys.argv) != 3:
        print("usage: wasi_run.py <program.wasm> <directory>", file=sys.stderr)
        return 2

    program, directory = sys.argv[1], sys.argv[2]
    engine = wasmtime.Engine()
    linker = wasmtime.Linker(engine)
    linker.define_wasi()
    module = wasmtime.Module.from_file(engine, program)
    store = wasmtime.Store(engine)
    config = wasmtime.WasiConfig()
    config.argv = [program]
    config.inherit_stdout()
    config.inherit_stderr()
    config.preopen_dir(directory, ".")
    store.set_wasi(config)
    instance = linker.instantiate(store, module)

    # a program that returns from main exits through proc_exit as well
    try:
        instance.exports(store)["_start"](store)
    except wasmtime.ExitTrap as status:
        return status.code

    return 0


if __name__ == "__main__":
    sys.exit(main())
