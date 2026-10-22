/*******************************************************************************
* djinterp [test]                                       file_tests_recursive.cpp
*
*   X. RECURSION (file_recursive.hpp) -- walking a tree and removing one. Pure
* composition over directory, status and the operations, so what is tested here
* is the COMPOSITION's promises rather than any new syscall.
*
*   THREE PROMISES, EACH WITH A FAILURE MODE WORTH CATCHING:
*
*     PRE-ORDER. walk visits a directory BEFORE its contents. A visitor that
*   builds a tree, or creates mirror directories as it goes, breaks outright on
*   post-order -- so the ordering case records the visit sequence and checks a
*   parent's index precedes its children's rather than merely that both appear.
*
*     SYMLINKS ARE NOT FOLLOWED. A walk that follows links can loop forever on
*   a link to an ancestor. The case below builds exactly that -- a link back to
*   the tree root -- and requires the walk to terminate and to classify the
*   link as a non-directory. An implementation that regressed here would not
*   fail this case so much as hang in it, which is why the fixture is a real
*   cycle rather than a link to a sibling.
*
*     REMOVE_ALL IS IDEMPOTENT. Removing something already gone is SUCCESS, not
*   ENOENT, so cleanup code can run twice without special-casing. That is the
*   opposite of remove()'s behaviour on the same input, and both are asserted.
*
*
* path:      /tests/djinterp/core/filesys/file_tests_recursive.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.20
*******************************************************************************/

// std
#include <cerrno>
#include <string>
#include <vector>
// djinterp
#include "file_tests.hpp"


namespace djinterp
{
namespace testing
{

namespace
{

// make_tree
//   function: build a known nested tree under the scratch directory:
//
//     <name>/            depth 0 for its own children
//       a.txt
//       sub/
//         b.txt
//         deeper/
//           c.txt
//
// returns the root, or an invalid path on failure.
path
make_tree(const char* _name)
{
    error ec;
    path  root = fs_test_path(_name);
    path  sub;
    path  deeper;

    remove_all(root, ec);

    if (!create_directory(root, ec))
    {
        return path();
    }

    sub = root / path("sub");

    if (!create_directory(sub, ec))
    {
        return path();
    }

    deeper = sub / path("deeper");

    if (!create_directory(deeper, ec))
    {
        return path();
    }

    if ( (!fs_test_write(root / path("a.txt"), "a", 1))   ||
         (!fs_test_write(sub / path("b.txt"), "b", 1))    ||
         (!fs_test_write(deeper / path("c.txt"), "c", 1)) )
    {
        return path();
    }

    return root;
}

} // anonymous namespace


void
tests_fs_recursive(suite_type& _suite)
{
    _suite.test_module("X. recursion");

    // -----------------------------------------------------------------
    //  walk
    // -----------------------------------------------------------------

    _suite.test_block("walk");

    _suite.test("every entry in the tree is visited exactly once")
          .assert_([]() -> bool
          {
              error ec(EIO);
              path  root = make_tree("rec_walk_all");
              int   files = 0;
              int   dirs  = 0;
              bool  ok;

              if (!root.valid())
              {
                  return false;
              }

              ok = walk(root,
                        [&files, &dirs](const path&, unsigned, bool _is_dir)
                        {
                            if (_is_dir) { ++dirs; } else { ++files; }
                        },
                        ec);

              // three files (a, b, c) and two directories (sub, deeper);
              // the root itself is walked, not visited
              ok = ok && (!ec.failed()) && (files == 3) && (dirs == 2);

              remove_all(root, ec);

              return ok;
          });

    _suite.test("depth increases with nesting")
          .assert_([]() -> bool
          {
              error    ec;
              path     root = make_tree("rec_walk_depth");
              unsigned depth_a = 99;
              unsigned depth_b = 99;
              unsigned depth_c = 99;
              bool     ok;

              if (!root.valid())
              {
                  return false;
              }

              walk(root,
                   [&](const path& _p, unsigned _depth, bool)
                   {
                       if (_p.filename() == path("a.txt")) { depth_a = _depth; }
                       if (_p.filename() == path("b.txt")) { depth_b = _depth; }
                       if (_p.filename() == path("c.txt")) { depth_c = _depth; }
                   },
                   ec);

              ok = (depth_a < depth_b) && (depth_b < depth_c);

              remove_all(root, ec);

              return ok;
          });

    _suite.test("a directory is visited BEFORE its contents (pre-order)")
          .assert_([]() -> bool
          {
              error                     ec;
              path                      root = make_tree("rec_walk_order");
              ::std::vector<::std::string> order;
              int                       index_sub = -1;
              int                       index_b   = -1;
              ::std::size_t             i;
              bool                      ok;

              if (!root.valid())
              {
                  return false;
              }

              walk(root,
                   [&order](const path& _p, unsigned, bool)
                   {
                       order.push_back(::std::string(_p.filename().c_str()));
                   },
                   ec);

              for (i = 0; i < order.size(); ++i)
              {
                  if (order[i] == "sub")   { index_sub = static_cast<int>(i); }
                  if (order[i] == "b.txt") { index_b   = static_cast<int>(i); }
              }

              // the parent must be seen first: a visitor that mirrors the
              // tree cannot create b.txt before sub exists
              ok = (index_sub >= 0) && (index_b >= 0) && (index_sub < index_b);

              remove_all(root, ec);

              return ok;
          });

    _suite.test("the visitor receives full paths, not bare names")
          .assert_([]() -> bool
          {
              error ec;
              path  root = make_tree("rec_walk_paths");
              bool  all_absolute = true;
              bool  ok;

              if (!root.valid())
              {
                  return false;
              }

              walk(root,
                   [&all_absolute](const path& _p, unsigned, bool)
                   {
                       if (!_p.is_absolute())
                       {
                           all_absolute = false;
                       }
                   },
                   ec);

              // the scratch root is absolute, so every path built from it is
              // -- and a caller can act on what the visitor hands over
              ok = all_absolute;

              remove_all(root, ec);

              return ok;
          });

    _suite.test("is_directory is reported correctly per entry")
          .assert_([]() -> bool
          {
              error ec;
              path  root = make_tree("rec_walk_flags");
              bool  sub_is_dir  = false;
              bool  a_is_file   = false;
              bool  ok;

              if (!root.valid())
              {
                  return false;
              }

              walk(root,
                   [&](const path& _p, unsigned, bool _is_dir)
                   {
                       if (_p.filename() == path("sub"))   { sub_is_dir = _is_dir; }
                       if (_p.filename() == path("a.txt")) { a_is_file  = !_is_dir; }
                   },
                   ec);

              ok = sub_is_dir && a_is_file;

              remove_all(root, ec);

              return ok;
          });

    _suite.test("an empty directory walks cleanly, visiting nothing")
          .assert_([]() -> bool
          {
              error ec(EIO);
              path  root = fs_test_path("rec_walk_empty");
              int   count = 0;
              bool  ok;

              remove_all(root, ec);

              if (!create_directory(root, ec))
              {
                  return false;
              }

              ok = walk(root,
                        [&count](const path&, unsigned, bool) { ++count; },
                        ec);

              ok = ok && (!ec.failed()) && (count == 0);

              remove_all(root, ec);

              return ok;
          });

    _suite.test("an unreadable root fails fast, with a reason")
          .assert_([]() -> bool
          {
              error ec;
              int   count = 0;

              return ( (!walk(fs_test_path("no_such_walk_root"),
                              [&count](const path&, unsigned, bool) { ++count; },
                              ec)) &&
                       (ec.failed()) &&
                       (count == 0) );
          });

    _suite.test("an invalid root is EINVAL")
          .assert_([]() -> bool
          {
              error ec;

              return ( (!walk(fs_test_invalid_path(),
                              [](const path&, unsigned, bool) {},
                              ec)) &&
                       (ec.value() == EINVAL) );
          });

    // a walk that followed links would not fail this case -- it would hang in
    // it, so the fixture is a genuine cycle
    _suite.add_if(fs_test_symlinks_creatable(),
                  [](suite_type& _b)
                  {
                      _b.test("a symlink CYCLE terminates and is not descended")
                        .assert_([]() -> bool
                        {
                            error ec;
                            path  root = make_tree("rec_walk_cycle");
                            path  loop;
                            int   visits = 0;
                            bool  link_is_dir = true;
                            bool  ok;

                            if (!root.valid())
                            {
                                return false;
                            }

                            // point a link back at the tree root: following
                            // it would recurse without end
                            loop = root / path("sub") / path("loop.symlink");

                            if (!create_symlink(root, loop, ec))
                            {
                                remove_all(root, ec);

                                return false;
                            }

                            ok = walk(root,
                                      [&](const path& _p, unsigned, bool _is_dir)
                                      {
                                          ++visits;

                                          if (_p.filename() == path("loop.symlink"))
                                          {
                                              link_is_dir = _is_dir;
                                          }
                                      },
                                      ec);

                            // terminated, saw a bounded number of entries,
                            // and classified the link as a NON-directory so
                            // the recursion never entered it
                            ok = ok && (!link_is_dir) && (visits < 32);

                            remove_all(root, ec);

                            return ok;
                        });
                  });

    // -----------------------------------------------------------------
    //  remove_all
    // -----------------------------------------------------------------

    _suite.test_block("remove_all");

    _suite.test("a whole tree is removed, children and all")
          .assert_([]() -> bool
          {
              error ec(EIO);
              path  root = make_tree("rec_remove_tree");

              if (!root.valid())
              {
                  return false;
              }

              return ( (remove_all(root, ec)) &&
                       (!ec.failed())         &&
                       (!fs_test_exists(root)) );
          });

    _suite.test("a single file is removed")
          .assert_([]() -> bool
          {
              error ec;
              path  p = fs_test_path("rec_remove_file.txt");

              if (!fs_test_write(p, "x", 1))
              {
                  return false;
              }

              return ( (remove_all(p, ec)) &&
                       (!fs_test_exists(p)) );
          });

    _suite.test("an empty directory is removed")
          .assert_([]() -> bool
          {
              error ec;
              path  p = fs_test_path("rec_remove_empty");

              remove_all(p, ec);

              if (!create_directory(p, ec))
              {
                  return false;
              }

              return ( (remove_all(p, ec)) &&
                       (!fs_test_exists(p)) );
          });

    _suite.test("removing something ALREADY GONE is SUCCESS, not ENOENT")
          .assert_([]() -> bool
          {
              error ec(EIO);
              path  p = fs_test_path("rec_remove_absent");

              // idempotent by design, so cleanup can run twice
              return ( (remove_all(p, ec)) &&
                       (!ec.failed()) );
          });

    _suite.test("running it twice on a real tree also succeeds twice")
          .assert_([]() -> bool
          {
              error ec;
              path  root = make_tree("rec_remove_twice");

              if (!root.valid())
              {
                  return false;
              }

              return ( (remove_all(root, ec)) &&
                       (remove_all(root, ec)) &&
                       (!ec.failed()) );
          });

    _suite.test("plain remove() on an absent path DOES fail -- the contrast")
          .assert_([]() -> bool
          {
              error ec;

              // the same input, the opposite verdict: remove() reports the
              // absence, remove_all() forgives it
              return ( (!remove(fs_test_path("rec_contrast_absent"), ec)) &&
                       (ec.failed()) );
          });

    _suite.test("an invalid path is EINVAL")
          .assert_([]() -> bool
          {
              error ec;

              return ( (!remove_all(fs_test_invalid_path(), ec)) &&
                       (ec.value() == EINVAL) );
          });

    _suite.add_if(fs_test_symlinks_creatable(),
                  [](suite_type& _b)
                  {
                      _b.test("a symlink inside a tree is removed, its target is not")
                        .assert_([]() -> bool
                        {
                            error ec;
                            path  root   = make_tree("rec_remove_link");
                            path  target = fs_test_path(D_CF_TEST_FILE);
                            path  link;
                            bool  ok;

                            if (!root.valid())
                            {
                                return false;
                            }

                            link = root / path("outward.symlink");

                            if (!create_symlink(target, link, ec))
                            {
                                remove_all(root, ec);

                                return false;
                            }

                            // deleting through the link would destroy the
                            // fixture the rest of the suite depends on
                            ok = remove_all(root, ec)    &&
                                 (!fs_test_exists(root)) &&
                                 fs_test_exists(target);

                            return ok;
                        });
                  });

    return;
}

} // namespace testing
} // namespace djinterp
