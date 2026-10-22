/*******************************************************************************
* djinterp [test]                                             file_tests_dir.cpp
*
*   V. DIRECTORY (file_dir.hpp) -- the RAII directory handle and the range-for
* built on it.
*
*   THE DOT ENTRIES ARE GONE, AND THAT IS A DIFFERENCE. The C layer hands "."
* and ".." to the caller (its SKIP_DOTS knob defaults off, which the c/filesys suite
* asserts). This layer removes them, because a C++ directory walk is expected
* to yield contents rather than bookkeeping. The two suites therefore assert
* OPPOSITE things about the same directory, on purpose -- and the case below
* names that, so a later "fix" that makes them agree fails loudly here.
*
*   THE READDIR TRAP, TESTED. d_dir_read's entry is borrowed and dies at the
* next call; directory_entry promises to COPY the name instead. That promise is
* invisible until you keep an entry and walk on, so the case below does exactly
* that -- reads one entry, advances the walk past it, and then reads the name
* it kept. Against a borrowing implementation that is a use-after-free; against
* this one it is just a name.
*
*   ENDING vs FAILING. read() returns false for both, and distinguishes them
* through _ec: clear on a clean end, set on a real error. A walk that treated
* an error as an ending would silently truncate. The clean-end case pins the
* error CLEAR, which is the half that a naive implementation gets wrong.
*
*
* path:      /tests/djinterp/core/filesys/file_tests_dir.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.20
*******************************************************************************/

// std
#include <cerrno>
#include <cstring>
#include <type_traits>
// djinterp
#include "file_tests.hpp"


namespace djinterp
{
namespace testing
{

namespace
{

// make_walk_dir
//   function: build a scratch directory holding three known files, for the
// traversal cases. Removes any leftover first so a case is never confused by
// a previous run's debris.
// returns the directory's path, or an empty path on failure.
path
make_walk_dir(const char* _name)
{
    error ec;
    path  dir = fs_test_path(_name);

    remove_all(dir, ec);

    if (!create_directory(dir, ec))
    {
        return path();
    }

    if ( (!fs_test_write(dir / path("one.txt"), "1", 1)) ||
         (!fs_test_write(dir / path("two.txt"), "22", 2)) ||
         (!fs_test_write(dir / path("three.txt"), "333", 3)) )
    {
        return path();
    }

    return dir;
}

} // anonymous namespace


void
tests_fs_dir(suite_type& _suite)
{
    _suite.test_module("V. directory");

    // -----------------------------------------------------------------
    //  ownership
    // -----------------------------------------------------------------

    _suite.test_block("directory :: ownership");

    _suite.test("directory is NOT copyable")
          .assert_([]() -> bool
          {
              return ( (!::std::is_copy_constructible<directory>::value) &&
                       (!::std::is_copy_assignable<directory>::value) );
          });

    _suite.test("directory IS movable")
          .assert_([]() -> bool
          {
              return ( (::std::is_move_constructible<directory>::value) &&
                       (::std::is_move_assignable<directory>::value) );
          });

    _suite.test("a move transfers the handle and closes the source")
          .assert_([]() -> bool
          {
              error     ec;
              directory source(fs_test_root(), ec);
              directory sink(static_cast<directory&&>(source));

              return ( (sink.is_open()) &&
                       (!source.is_open()) );
          });

    // -----------------------------------------------------------------
    //  opening
    // -----------------------------------------------------------------

    _suite.test_block("directory :: opening");

    _suite.test("a default-constructed directory owns nothing")
          .assert_([]() -> bool
          {
              directory d;

              return ( (!d.is_open()) &&
                       (!static_cast<bool>(d)) );
          });

    _suite.test("an existing directory opens cleanly")
          .assert_([]() -> bool
          {
              error     ec(EIO);
              directory d(fs_test_root(), ec);

              return ( (d.is_open()) &&
                       (!ec.failed()) );
          });

    _suite.test("a missing directory fails to open, with a reason")
          .assert_([]() -> bool
          {
              error     ec;
              directory d(fs_test_path("no_such_directory"), ec);

              return ( (!d.is_open()) &&
                       (ec.failed()) );
          });

    _suite.test("a regular file is not a directory -- ENOTDIR")
          .assert_([]() -> bool
          {
              error     ec;
              directory d(fs_test_path(D_CF_TEST_FILE), ec);

              return ( (!d.is_open()) &&
                       (ec.failed()) );
          });

    _suite.test("an invalid path is EINVAL")
          .assert_([]() -> bool
          {
              error     ec;
              directory d(fs_test_invalid_path(), ec);

              return ( (!d.is_open()) &&
                       (ec.value() == EINVAL) );
          });

    // -----------------------------------------------------------------
    //  reading
    // -----------------------------------------------------------------

    _suite.test_block("directory :: read");

    _suite.test("every file in the directory is visited exactly once")
          .assert_([]() -> bool
          {
              error           ec;
              path            dir = make_walk_dir("dir_read");
              directory_entry entry;
              int             seen_one   = 0;
              int             seen_two   = 0;
              int             seen_three = 0;
              int             total      = 0;
              bool            ok;

              if (!dir.valid())
              {
                  return false;
              }

              {
                  directory d(dir, ec);

                  while (d.read(entry, ec))
                  {
                      ++total;

                      if (entry.name() == path("one.txt"))   { ++seen_one; }
                      if (entry.name() == path("two.txt"))   { ++seen_two; }
                      if (entry.name() == path("three.txt")) { ++seen_three; }
                  }

                  ok = ( (total == 3)     &&
                         (seen_one == 1)  &&
                         (seen_two == 1)  &&
                         (seen_three == 1) );
              }

              remove_all(dir, ec);

              return ok;
          });

    _suite.test("\".\" and \"..\" are SKIPPED -- unlike the C layer")
          .assert_([]() -> bool
          {
              error           ec;
              path            dir = make_walk_dir("dir_dots");
              directory_entry entry;
              bool            saw_dot = false;
              bool            ok;

              if (!dir.valid())
              {
                  return false;
              }

              {
                  directory d(dir, ec);

                  while (d.read(entry, ec))
                  {
                      if ( (entry.name() == path(".")) ||
                           (entry.name() == path("..")) )
                      {
                          saw_dot = true;
                      }
                  }
              }

              ok = !saw_dot;
              remove_all(dir, ec);

              return ok;
          });

    _suite.test("a clean end leaves the error CLEAR")
          .assert_([]() -> bool
          {
              error           ec(EIO);
              directory_entry entry;
              directory       d(fs_test_root(), ec);

              while (d.read(entry, ec))
              {
                  // drain
              }

              // reaching the end is not a failure, and must not look like one
              return (!ec.failed());
          });

    _suite.test("a clean walk leaves last_error clear and failed() false")
          .assert_([]() -> bool
          {
              error           ec;
              directory_entry entry;
              directory       d(fs_test_root(), ec);

              while (d.read(entry, ec))
              {
              }

              return ( (!d.failed()) &&
                       (!d.last_error().failed()) );
          });

    _suite.test("reading a CLOSED directory is EBADF")
          .assert_([]() -> bool
          {
              error           ec;
              directory_entry entry;
              directory       d;

              return ( (!d.read(entry, ec)) &&
                       (ec.value() == EBADF) );
          });

    _suite.test("rewind restarts the walk")
          .assert_([]() -> bool
          {
              error           ec;
              path            dir = make_walk_dir("dir_rewind");
              directory_entry entry;
              int             first_pass  = 0;
              int             second_pass = 0;
              bool            ok;

              if (!dir.valid())
              {
                  return false;
              }

              {
                  directory d(dir, ec);

                  while (d.read(entry, ec)) { ++first_pass; }

                  if (!d.rewind(ec))
                  {
                      remove_all(dir, ec);

                      return false;
                  }

                  while (d.read(entry, ec)) { ++second_pass; }

                  ok = (first_pass == 3) && (second_pass == 3);
              }

              remove_all(dir, ec);

              return ok;
          });

    _suite.test("rewinding a CLOSED directory is EBADF")
          .assert_([]() -> bool
          {
              error     ec;
              directory d;

              return ( (!d.rewind(ec)) &&
                       (ec.value() == EBADF) );
          });

    // -----------------------------------------------------------------
    //  the entry
    // -----------------------------------------------------------------

    _suite.test_block("directory_entry");

    _suite.test("an entry KEEPS its name after the walk moves on")
          .assert_([]() -> bool
          {
              error           ec;
              path            dir = make_walk_dir("dir_entry_copy");
              directory_entry first;
              directory_entry later;
              path            remembered;
              bool            ok;

              if (!dir.valid())
              {
                  return false;
              }

              {
                  directory d(dir, ec);

                  if (!d.read(first, ec))
                  {
                      remove_all(dir, ec);

                      return false;
                  }

                  remembered = first.name();

                  // advance past it -- against a BORROWED d_dirent_t this is
                  // exactly the point the earlier name would be overwritten
                  d.read(later, ec);

                  ok = ( (first.name() == remembered) &&
                         (!first.name().empty()) );
              }

              remove_all(dir, ec);

              return ok;
          });

    _suite.test("an entry names a file, not a full path")
          .assert_([]() -> bool
          {
              error           ec;
              path            dir = make_walk_dir("dir_entry_name");
              directory_entry entry;
              bool            ok = false;

              if (!dir.valid())
              {
                  return false;
              }

              {
                  directory d(dir, ec);

                  while (d.read(entry, ec))
                  {
                      if (entry.name() == path("one.txt"))
                      {
                          // a bare component: no separator, so joining it to
                          // the directory is the caller's job
                          ok = (entry.name().parent() == path("."));
                      }
                  }
              }

              remove_all(dir, ec);

              return ok;
          });

    _suite.test("a regular file entry classifies as one, however type is learned")
          .assert_([]() -> bool
          {
              error           ec;
              path            dir = make_walk_dir("dir_entry_type");
              directory_entry entry;
              bool            ok = false;

              if (!dir.valid())
              {
                  return false;
              }

              {
                  directory d(dir, ec);

                  while (d.read(entry, ec))
                  {
                      if (entry.name() == path("one.txt"))
                      {
                          // where d_type is unreported the entry says so
                          // rather than guessing; either answer is correct,
                          // but "known and wrong" is not
                          ok = (!entry.type_known()) ||
                               (entry.is_regular_file());
                      }
                  }
              }

              remove_all(dir, ec);

              return ok;
          });

    _suite.test("a default-constructed entry is inert")
          .assert_([]() -> bool
          {
              directory_entry entry;

              return ( (entry.name().empty()) &&
                       (!entry.is_regular_file()) &&
                       (!entry.is_directory()) );
          });

    // -----------------------------------------------------------------
    //  the range-for
    // -----------------------------------------------------------------

    _suite.test_block("directory :: range-for");

    _suite.test("a range-for visits the same entries read() does")
          .assert_([]() -> bool
          {
              error ec;
              path  dir = make_walk_dir("dir_rangefor");
              int   count = 0;
              bool  ok;

              if (!dir.valid())
              {
                  return false;
              }

              {
                  directory d(dir, ec);

                  for (const directory_entry& entry : d)
                  {
                      (void)entry;
                      ++count;
                  }

                  ok = (count == 3) && (!d.failed());
              }

              remove_all(dir, ec);

              return ok;
          });

    _suite.test("the loop reads each entry's name")
          .assert_([]() -> bool
          {
              error ec;
              path  dir = make_walk_dir("dir_rangefor_names");
              int   named = 0;
              bool  ok;

              if (!dir.valid())
              {
                  return false;
              }

              {
                  directory d(dir, ec);

                  for (const directory_entry& entry : d)
                  {
                      if (!entry.name().empty())
                      {
                          ++named;
                      }
                  }

                  ok = (named == 3);
              }

              remove_all(dir, ec);

              return ok;
          });

    _suite.test("an empty directory yields no iterations, and no error")
          .assert_([]() -> bool
          {
              error ec;
              path  dir = fs_test_path("dir_empty");
              int   count = 0;
              bool  ok;

              remove_all(dir, ec);

              if (!create_directory(dir, ec))
              {
                  return false;
              }

              {
                  directory d(dir, ec);

                  for (const directory_entry& entry : d)
                  {
                      (void)entry;
                      ++count;
                  }

                  // empty because the dot entries are filtered, not because
                  // the walk failed
                  ok = (count == 0) && (!d.failed());
              }

              remove_all(dir, ec);

              return ok;
          });

    // -----------------------------------------------------------------
    //  create / remove
    // -----------------------------------------------------------------

    _suite.test_block("create_directory / remove_directory");

    _suite.test("a directory is created and reported as one")
          .assert_([]() -> bool
          {
              error ec(EIO);
              path  p = fs_test_path("dir_created");
              bool  ok;

              remove_all(p, ec);

              ok = create_directory(p, ec) &&
                   (!ec.failed())          &&
                   fs_test_exists(p)       &&
                   is_directory(p, ec);

              remove_directory(p, ec);

              return ok;
          });

    _suite.test("creating one that already exists fails -- EEXIST")
          .assert_([]() -> bool
          {
              error ec;
              path  p = fs_test_path("dir_twice");
              bool  ok;

              remove_all(p, ec);
              create_directory(p, ec);

              ok = (!create_directory(p, ec)) &&
                   (ec.value() == EEXIST);

              remove_directory(p, ec);

              return ok;
          });

    _suite.test("creating with an invalid path is EINVAL")
          .assert_([]() -> bool
          {
              error ec;

              return ( (!create_directory(fs_test_invalid_path(), ec)) &&
                       (ec.value() == EINVAL) );
          });

    _suite.test("an empty directory is removed")
          .assert_([]() -> bool
          {
              error ec(EIO);
              path  p = fs_test_path("dir_removed");

              remove_all(p, ec);
              create_directory(p, ec);

              return ( (remove_directory(p, ec)) &&
                       (!ec.failed())            &&
                       (!fs_test_exists(p)) );
          });

    _suite.test("removing a missing directory fails")
          .assert_([]() -> bool
          {
              error ec;

              return ( (!remove_directory(fs_test_path("no_such_rmdir"), ec)) &&
                       (ec.failed()) );
          });

    _suite.test("a NON-empty directory is refused, not emptied")
          .assert_([]() -> bool
          {
              error ec;
              path  dir = make_walk_dir("dir_nonempty");
              bool  ok;

              if (!dir.valid())
              {
                  return false;
              }

              // the narrow promise: this cannot delete a tree by surprise
              ok = (!remove_directory(dir, ec)) &&
                   (ec.failed())                &&
                   fs_test_exists(dir / path("one.txt"));

              remove_all(dir, ec);

              return ok;
          });

    return;
}

} // namespace testing
} // namespace djinterp
