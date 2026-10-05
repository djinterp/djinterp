/*******************************************************************************
* djinterp [core]                                                        git.hpp
*
*   git version-control module for the djinterp framework. Provides value
* types describing commit metadata, a commit-log container, and a repository
* facade exposing staging and commit operations. The repository talks to the
* installed git command-line client through a single, replaceable backend
* seam (`m_run`), so an alternative backend (e.g. libgit2) may be substituted
* without altering the public interface.
*
* This header contains declarations only; definitions live in git.cpp.
*
*
* path:      /inc/djinterp/core/util/git.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.30
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_UTIL_GIT_HPP
#define DJINTERP_UTIL_GIT_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <ctime>
#include <string>
#include <vector>
// djinterp
#include "../../djinterp.hpp"


// =============================================================================
// 0.   MODULE KEYWORD & NAMESPACE MACROS
// =============================================================================
// NOTE: djinterp.hpp owns the framework keyword/namespace macros but does not
// yet define a `git` keyword. These are guarded so that, should `git` later be
// promoted into djinterp.hpp, this block becomes a harmless no-op.

// D_KEYWORD_GIT
//   keyword: resolves to `git`.
// Used to specify that a unit of code pertains to the git version-control
// module.
#ifndef D_KEYWORD_GIT
    #define D_KEYWORD_GIT           git
#endif

// NS_GIT
//   namespace: the `git` namespace for git version-control types and
// operations.
#ifndef NS_GIT
    #define NS_GIT                  D_NAMESPACE(D_KEYWORD_GIT)
#endif


NS_DJINTERP
NS_GIT

// =============================================================================
// I.   VALUE TYPES
// =============================================================================

// object_id
//   type: a git object identifier (a hexadecimal SHA digest). Stored as a
// string so the type is agnostic to SHA-1 vs SHA-256 digest widths.
using object_id = std::string;

// signature
//   struct: identifies an author or committer together with the moment the
// corresponding action was recorded.
struct signature
{
    std::string name;
    std::string email;
    std::time_t when;
};

// commit
//   struct: an immutable record describing a single git commit. `parents` is
// empty for a root commit and holds two or more entries for a merge.
struct commit
{
    object_id              id;
    signature              author;
    signature              committer;
    std::string            message;
    std::vector<object_id> parents;
};


// =============================================================================
// II.  COMMIT-LOG CONTAINER
// =============================================================================
// NOTE: this is the portion governed by containers_howto.md, which is not
// present in the project. `commit_log` is therefore modelled to satisfy the
// container vocabulary the framework's own concepts test for (value_type,
// size_type, iterator, const_iterator, size(), max_size). If the howto
// requires deriving from a common container base (cf. fixed_array : base<...>
// in the style guide) or placement under djinterp::container, adjust here.

// commit_log
//   class: an ordered, read-mostly sequence of commits, newest first, as
// produced by a history walk. Models a minimal forward-iterable container.
class commit_log
{
private:
    using storage_type = std::vector<commit>;

public:
    using value_type      = commit;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;
    using iterator        = storage_type::iterator;
    using const_iterator  = storage_type::const_iterator;

    // max_size
    //   constant: the bounded-capacity marker the framework's has_max_size /
    // is_bounded traits probe for.
    static const size_type max_size;

    commit_log();

    explicit commit_log(
        const std::vector<commit>& _commits
    );

    void            push_back(const commit& _commit);
    void            clear();

    bool            empty() const;
    size_type       size() const;

    reference       front();
    const_reference front() const;
    reference       at(size_type _index);
    const_reference at(size_type _index) const;

    iterator        begin();
    iterator        end();
    const_iterator  begin() const;
    const_iterator  end() const;
    const_iterator  cbegin() const;
    const_iterator  cend() const;

private:
    storage_type m_commits;
};


// =============================================================================
// III. OPERATION RESULTS
// =============================================================================
// NOTE: error reporting is kept module-local pending integration with
// djinterp::error / djinterp::exception, whose definitions are not in the
// project. `status` plus `result<Value>` is a lightweight stand-in.

// status
//   enum: the outcome category of a repository operation.
enum class status
{
    ok,
    not_a_repository,
    nothing_to_commit,
    path_not_found,
    backend_failure
};

// result
//   class: pairs a status code with an optional produced value and a
// human-readable detail string. Contextually convertible to bool, true only
// when the status is `ok`.
template<typename Value>
class result
{
public:
    using value_type = Value;

    result(
        status      _code,
        std::string _detail
    );

    result(
        value_type  _value,
        std::string _detail
    );

    explicit         operator bool() const;

    status           code() const;
    const value_type& value() const;
    const std::string& detail() const;

private:
    status      m_code;
    value_type  m_value;
    std::string m_detail;
};


// =============================================================================
// IV.  REPOSITORY FACADE
// =============================================================================

// repository
//   class: a facade over a single git working tree. Operations are forwarded
// to the git command-line client via the private `m_run` backend seam.
class repository
{
public:
    explicit repository(
        std::string _work_tree
    );

    // is_repository
    //   queries whether the configured work tree is the root of a git
    // repository.
    bool                is_repository() const;

    // stage
    //   adds a path (file or directory, relative to the work tree) to the
    // staging index.
    result<bool>        stage(const std::string& _path);

    // unstage
    //   removes a path from the staging index, leaving the working tree file
    // untouched.
    result<bool>        unstage(const std::string& _path);

    // staged_paths
    //   lists the paths currently present in the staging index.
    std::vector<std::string> staged_paths() const;

    // commit
    //   records the staged changes as a new commit authored by `_author`.
    // Fails with status::nothing_to_commit when the index is empty.
    // NOTE: the return type is written `struct commit` because the member
    // function name `commit` otherwise shadows the `commit` struct in class
    // scope.
    result<struct commit>      commit(const std::string& _message,
                                      const signature&   _author);

    // log
    //   walks history newest-first, returning up to `_limit` commits (0 means
    // unlimited).
    result<commit_log>  log(std::size_t _limit = 0) const;

    const std::string&  work_tree() const;

private:
    // m_run
    //   backend seam: executes `git _args` inside the work tree, writing the
    // captured standard output into `_output` and returning the process exit
    // code. Replace this single method to target a non-CLI backend.
    int m_run(const std::vector<std::string>& _args,
              std::string&                    _output) const;

    std::string m_work_tree;
};

NS_END  // git
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_GIT_HPP
