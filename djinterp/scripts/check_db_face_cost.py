#!/usr/bin/env python3
"""check_db_face_cost.py -- check that the database faces cost nothing.

Compiles tests/unintegrated/cpp/db/t_db_face_cost.cpp at -O2 with each compiler
given, at each C++ level, disassembles it, and compares each pair of
functions in it: d_tests_db_face_cost_c_<name>, which makes a call through
a C layer, and d_tests_db_face_cost_face_<name>, which makes the same call
through the layer's C++ face. A pair passes when its halves are the same
instructions, or the same instructions in another order or other registers;
the padding between functions is not counted. A call the face adds, a
landing pad, a lost tail call or an instruction more fails the pair, and the
two listings are printed.

  usage:  scripts/check_db_face_cost.py [<c++ compiler> ...]
          (no compiler: g++ and clang++)
  env:    CXX_STD_LIST  the levels (default: "c++98 c++11 c++14 c++17 c++20
                        c++23 c++26", c++26 meaning -std=c++2c); one a
                        compiler does not have is skipped, and said to be
          OBJDUMP       the disassembler (default: objdump), GNU's or LLVM's
          EXTRA_FLAGS   added to every compile (default: none); -DD_NOTHROW=
                        shows what the faces would cost were the layers not
                        declared D_NOTHROW
  exit:   0 when every pair passes or is skipped; 1 when a pair fails or a
          build does; 2 when the check cannot run
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE = os.path.join(ROOT, 'tests', 'unintegrated', 'cpp', 'db',
                      't_db_face_cost.cpp')
LEVELS = 'c++98 c++11 c++14 c++17 c++20 c++23 c++26'

# a pair's two halves, by the name they share
HALF = re.compile(r'^(?:\S+ )?d_tests_db_face_cost_(c|face)_(\w+)\(')

# the padding between functions, which no call runs
PADDING = re.compile(r'^(nop|xchg\s+%ax,%ax|data16|cs nop|int3$)')


def flag(level):
    """the -std flag a level stands for"""
    return '-std=c++2c' if level == 'c++26' else '-std=' + level


def has_level(cxx, level):
    """whether a compiler takes a level's flag"""
    run = subprocess.run([cxx, flag(level), '-x', 'c++', '-fsyntax-only',
                          '-'], input='int main() { return 0; }\n',
                         capture_output=True, text=True)
    return run.returncode == 0


def listing(objdump, obj):
    """each function's instructions, by half and name: addresses,
    encodings and relocation offsets dropped, local labels made one"""
    run = subprocess.run([objdump, '-d', '-r', '-C', '--no-show-raw-insn',
                          obj], capture_output=True, text=True)
    if run.returncode != 0:
        raise SystemExit('%s failed: %s' % (objdump, run.stderr.strip()))
    halves, current = {}, None
    for line in run.stdout.splitlines():
        head = re.match(r'^[0-9a-f]+ <(.*)>:$', line)
        if head:
            pair = HALF.match(head.group(1))
            current = (pair.group(1), pair.group(2)) if pair else None
            if current:
                halves[current] = []
            continue
        if current is None or not line.strip() or \
                line.startswith('Disassembly'):
            continue
        text = re.sub(r'^\s*[0-9a-f]+:\s*', '', line).strip()
        text = re.sub(r'\b[0-9a-f]+ <', '<', text)
        text = re.sub(r'<[^>]*\+0x[0-9a-f]+>', '<local>', text)
        text = re.sub(r'\.L\.?str[.\w]*|\.LC\d+|\.rodata[.\w]*', '.str',
                      text)
        text = re.sub(r'R_\w+\s+', 'reloc ', text)
        text = re.sub(r'([+-]0x[0-9a-f]+)$', '', text)
        text = re.sub(r'\s+#.*$', '', text)
        if PADDING.match(text):
            continue
        # a call or a jump its relocation completes names whatever its
        # unrelocated address falls on -- often the next function, which
        # differs between the halves -- so its target is the relocation's
        if (text.startswith('reloc ') and halves[current] and
                re.match(r'^(call|jmp)q?\s+<[^>]*>$', halves[current][-1])):
            halves[current][-1] = re.sub(r'<[^>]*>$', '<reloc>',
                                         halves[current][-1])
        halves[current].append(text)
    return halves


def verdict(c_half, face_half):
    """'same', 'same, registers or order aside', or None"""
    if c_half == face_half:
        return 'same'
    plain = lambda half: sorted(re.sub(r'%[a-z0-9]+', '%reg', i)
                                for i in half)
    if plain(c_half) == plain(face_half):
        return 'same, registers or order aside'
    return None


def main(argv):
    compilers = argv or ['g++', 'clang++']
    levels = os.environ.get('CXX_STD_LIST', LEVELS).split()
    objdump = os.environ.get('OBJDUMP', 'objdump')
    extra = os.environ.get('EXTRA_FLAGS', '').split()
    if shutil.which(objdump) is None:
        print('no %s' % objdump)
        return 2
    failed = passed = skipped = 0
    work = tempfile.mkdtemp()
    try:
        for cxx in compilers:
            if shutil.which(cxx) is None:
                print('no %s' % cxx)
                return 2
            for level in levels:
                name = '%s %s' % (cxx, level)
                if not has_level(cxx, level):
                    print('SKIP %s (not in %s)' % (name, cxx))
                    skipped += 1
                    continue
                obj = os.path.join(work, 'cost.o')
                build = subprocess.run(
                    [cxx, flag(level), '-O2', '-D_XOPEN_SOURCE=700'] + extra +
                    ['-I' + os.path.join(ROOT, 'inc'), '-c', SOURCE, '-o',
                     obj], capture_output=True, text=True)
                if build.returncode != 0:
                    print('FAIL %s (the build)' % name)
                    print(build.stderr.strip())
                    failed += 1
                    continue
                halves = listing(objdump, obj)
                pairs = sorted(n for (half, n) in halves if half == 'c')
                faults = []
                for pair in pairs:
                    face_half = halves.get(('face', pair))
                    found = (None if face_half is None
                             else verdict(halves[('c', pair)], face_half))
                    if found is None:
                        faults.append(pair)
                        print('FAIL %s %s' % (name, pair))
                        print('    the layer:  ' +
                              '\n                '.join(halves[('c', pair)]))
                        print('    the face:   ' +
                              '\n                '.join(face_half or
                                                        ['(none)']))
                if faults or not pairs:
                    failed += 1
                    if not pairs:
                        print('FAIL %s (no pairs found)' % name)
                else:
                    passed += 1
                    print('PASS %s: %d pairs, %s' % (
                        name, len(pairs),
                        ', '.join('%s %s' % (p, verdict(halves[('c', p)],
                                                        halves[('face', p)]))
                                  for p in pairs)))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print('%d passed, %d failed, %d skipped' % (passed, failed, skipped))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
