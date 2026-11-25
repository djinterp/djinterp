/*******************************************************************************
* djinterp [cpp][container]                                  file_tree_tests.hpp
*
*   Declarations, fixtures, the per-suite assertion macro and the spec for the
* container/tree/file/ unit tests, the tree floor's file half: the core
* (file_tree_common.hpp, fs_os.hpp), the umbrella and every scanner
* (file_tree.hpp and its six backends), file_attributes.hpp,
* file_tree_path.hpp, file_tree_filter.hpp, file_tree_traits.hpp and
* file_tree_concepts.hpp. Two faces, selected by DTEST_SPEC_MODE: normal mode
* (the section TUs) pulls in the fixtures; spec mode (the runner) pulls in
* test_defaults.hpp and exposes file_tree_spec(). Each section includes its
* own headers and nothing else of the file tree, so each is compiled alone.
*
*   The scanners are run against a directory the suite builds under the
* system's temporary directory: on a POSIX host the posix, bsd and linux
* scanners each, and apple's portable path, by the allow knobs; on any host
* the native one. Windows's native code is compiled by the leaf's MinGW-w64
* check (file_tree_windows_probe.cpp), where a cross-compiler is installed.
*
*   The section .cpp files that DEFINE these predicates:
*     file_tree_tests_common.cpp      - file_tree_common.hpp, fs_os.hpp
*     file_tree_tests_scan.cpp        - file_tree.hpp, the POSIX scanners
*     file_tree_tests_foreign.cpp     - the apple, ios and windows scanners
*     file_tree_tests_attributes.cpp  - file_attributes.hpp
*     file_tree_tests_path.cpp        - file_tree_path.hpp
*     file_tree_tests_filter.cpp      - file_tree_filter.hpp
*     file_tree_tests_traits.cpp      - file_tree_traits.hpp and
*                                       file_tree_concepts.hpp
*
*
* path:      /tests/djinterp/cpp/container/tree/file/file_tree_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

#ifndef DJINTERP_TESTS_FILE_TREE_TESTS_
#define DJINTERP_TESTS_FILE_TREE_TESTS_ 1

// std
#include <cstdio>   // std::printf
#include <string>   // std::string
// djinterp
#include <djinterp/cpp/djinterp.hpp>  // framework root
#ifdef DTEST_SPEC_MODE
#include <djinterp/cpp/modules/test/test_defaults.hpp>  // module_spec (spec)
#else
#include <filesystem>  // the scanned directory
#include <fstream>     // its files
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest's entities (djinterp::test).
namespace dt = ::djinterp::test;


// file_tree_check
//   function: routes one D_FT_CHECK evaluation: prints the failing
// expression and its place, and hands the condition back.
inline
bool file_tree_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}

// D_FT_CHECK
//   macro: fail the current test unless the condition holds, printing the
// expression that failed. Variadic, so a condition may hold top-level commas.
#define D_FT_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::file_tree_check(                            \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// DFtScratch
//   fixture: a small directory under the system's temporary directory,
// removed when the fixture goes:
//     <root>/a.txt            5 bytes
//     <root>/sub/b.bin        3 bytes
//     <root>/sub/deeper/c     0 bytes
//     <root>/link             a symlink to a.txt, where the host allows one
struct DFtScratch
{
    std::filesystem::path root;
    bool                  has_link;
    bool                  ok;

    explicit DFtScratch(
        const char* _tag
    )
        : root(),
          has_link(false),
          ok(false)
    {
        std::error_code ec;

        root = std::filesystem::temp_directory_path(ec) /
               (std::string("djinterp_file_tree_") + _tag);
        std::filesystem::remove_all(root, ec);
        std::filesystem::create_directories(root / "sub" / "deeper", ec);

        if (ec)
        {
            return;
        }

        write(root / "a.txt", "hello");
        write(root / "sub" / "b.bin", "xyz");
        write(root / "sub" / "deeper" / "c", "");

        std::filesystem::create_symlink("a.txt", root / "link", ec);
        has_link = !ec;
        ok       = true;
    }

    ~DFtScratch()
    {
        std::error_code ec;

        std::filesystem::remove_all(root, ec);
    }

    DFtScratch(const DFtScratch&)            = delete;
    DFtScratch& operator=(const DFtScratch&) = delete;

    // path
    //   the root as the generic, '/'-separated string a scan is given.
    std::string path() const
    {
        return root.generic_string();
    }

    // nodes
    //   how many nodes a full scan holds: the root, a.txt, sub, b.bin,
    // deeper and c, and the link where there is one.
    std::size_t nodes() const
    {
        return (has_link ? 7u : 6u);
    }

private:
    static void write(
        const std::filesystem::path& _path,
        const char*                  _text
    )
    {
        std::ofstream out(_path, std::ios::binary);

        out << _text;
    }
};

#endif  // !DTEST_SPEC_MODE


// I.    file_tree_common.hpp, fs_os.hpp   (file_tree_tests_common.cpp)
bool tests_file_tree_common_values();
bool tests_file_tree_common_build_and_address();
bool tests_file_tree_common_separation();
bool tests_file_tree_common_filter_and_rename();
bool tests_file_tree_common_axes();

// II.   file_tree.hpp, the POSIX scanners   (file_tree_tests_scan.cpp)
bool tests_file_tree_scan_native();
bool tests_file_tree_scan_posix_family();
bool tests_file_tree_scan_null_and_missing();

// III.  the apple, ios and windows scanners   (file_tree_tests_foreign.cpp)
bool tests_file_tree_foreign_selectors();
bool tests_file_tree_foreign_off_platform();

// IV.   file_attributes.hpp   (file_tree_tests_attributes.cpp)
bool tests_file_tree_attributes_encoding();
bool tests_file_tree_attributes_metadata();
bool tests_file_tree_attributes_populate();

// V.    file_tree_path.hpp   (file_tree_tests_path.cpp)
bool tests_file_tree_path_resolve_and_build();
bool tests_file_tree_path_relations();

// VI.   file_tree_filter.hpp   (file_tree_tests_filter.cpp)
bool tests_file_tree_filter_predicates();
bool tests_file_tree_filter_query();

// VII.  file_tree_traits.hpp, file_tree_concepts.hpp
//       (file_tree_tests_traits.cpp)
bool tests_file_tree_traits_detection();
bool tests_file_tree_traits_concepts();


#ifdef DTEST_SPEC_MODE

// file_tree_spec
//   function: the suite's description: one block per section, one row per
// tests_* body.
inline
dt::module_spec file_tree_spec()
{
    return dt::module_spec{
        "file_tree",
        "The file tree's floor: the core, each scanner against a directory "
        "the suite builds, attributes, paths, filters, traits and concepts.",
        {
            dt::block_spec{
                "common",
                "file_tree_common.hpp and fs_os.hpp: values, building and "
                "addressing a tree, separation, filtering and renaming, and "
                "the container axes.",
                {
                    { "tests_file_tree_common_values",
                      "DFileType, DFileEntry's default, null_file_node and "
                      "DOperatingSystem's documented values.",
                      &tests_file_tree_common_values },
                    { "tests_file_tree_common_build_and_address",
                      "A tree built with add_child resolves each address to "
                      "its node, and each node's address back.",
                      &tests_file_tree_common_build_and_address },
                    { "tests_file_tree_common_separation",
                      "A second child of one name is not created: add_child "
                      "returns the first.",
                      &tests_file_tree_common_separation },
                    { "tests_file_tree_common_filter_and_rename",
                      "filter keeps the matches' ancestor closure; a rename "
                      "that collides two siblings is refused beforehand.",
                      &tests_file_tree_common_filter_and_rename },
                    { "tests_file_tree_common_axes",
                      "The core's container axes, as documented.",
                      &tests_file_tree_common_axes },
                }
            },
            dt::block_spec{
                "scan",
                "file_tree.hpp and the POSIX scanners, run against the "
                "suite's directory.",
                {
                    { "tests_file_tree_scan_native",
                      "The detected backend scans every entry with its type "
                      "and size, and each resolves and has its full path.",
                      &tests_file_tree_scan_native },
                    { "tests_file_tree_scan_posix_family",
                      "On a POSIX host the posix, bsd and linux scanners each "
                      "give the native scan's tree.",
                      &tests_file_tree_scan_posix_family },
                    { "tests_file_tree_scan_null_and_missing",
                      "The none backend, and a path that is not there, give "
                      "the root alone.",
                      &tests_file_tree_scan_null_and_missing },
                }
            },
            dt::block_spec{
                "foreign",
                "The apple, ios and windows scanners, allowed off their "
                "platforms.",
                {
                    { "tests_file_tree_foreign_selectors",
                      "Each operating system selects its scanner.",
                      &tests_file_tree_foreign_selectors },
                    { "tests_file_tree_foreign_off_platform",
                      "Off its platform, apple's scanner takes its portable "
                      "path and gives the native tree; windows's scans "
                      "nothing.",
                      &tests_file_tree_foreign_off_platform },
                }
            },
            dt::block_spec{
                "attributes",
                "file_attributes.hpp: encodings, metadata and populating "
                "from the suite's directory.",
                {
                    { "tests_file_tree_attributes_encoding",
                      "Each byte-order mark is detected, and each encoding's "
                      "name and Unicode-ness.",
                      &tests_file_tree_attributes_encoding },
                    { "tests_file_tree_attributes_metadata",
                      "DFileMetadata's default, and its flag queries.",
                      &tests_file_tree_attributes_metadata },
                    { "tests_file_tree_attributes_populate",
                      "The detected populator reads a file's size and a "
                      "directory's kind, and fails on a missing path.",
                      &tests_file_tree_attributes_populate },
                }
            },
            dt::block_spec{
                "path",
                "file_tree_path.hpp over a built tree.",
                {
                    { "tests_file_tree_path_resolve_and_build",
                      "resolve takes either separator; build, extension and "
                      "stem.",
                      &tests_file_tree_path_resolve_and_build },
                    { "tests_file_tree_path_relations",
                      "level, height, ancestors, lca and the relative path.",
                      &tests_file_tree_path_relations },
                }
            },
            dt::block_spec{
                "filter",
                "file_tree_filter.hpp's predicates and query over a built "
                "tree.",
                {
                    { "tests_file_tree_filter_predicates",
                      "Name, extension, glob, type, size and depth "
                      "predicates, collected and counted.",
                      &tests_file_tree_filter_predicates },
                    { "tests_file_tree_filter_query",
                      "DFileTreeQuery's files, directories, where, count "
                      "and names.",
                      &tests_file_tree_filter_query },
                }
            },
            dt::block_spec{
                "traits",
                "file_tree_traits.hpp and file_tree_concepts.hpp.",
                {
                    { "tests_file_tree_traits_detection",
                      "Each method probe and the payload traits, on the file "
                      "tree and on types that are not one.",
                      &tests_file_tree_traits_detection },
                    { "tests_file_tree_traits_concepts",
                      "The concepts agree with the traits (C++20).",
                      &tests_file_tree_traits_concepts },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_FILE_TREE_TESTS_
