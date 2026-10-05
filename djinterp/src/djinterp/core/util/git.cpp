/*******************************************************************************
* djinterp [core]                                                        git.cpp
*
*
* path:      /src/djinterp/core/util/git.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.01
*******************************************************************************/
// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
#include "../../../../inc/djinterp/core/util/git.hpp"

// std
#include <array>
#include <cstdio>
#include <limits>
#include <sstream>


NS_DJINTERP
NS_GIT

// =============================================================================
// I.   INTERNAL HELPERS
// =============================================================================

NS_INTERNAL

    /*
    shell_quote
      Wraps a single argument in single quotes so the shell treats it as one
    literal token, escaping any embedded single quotes.

    Parameter(s):
      _arg: the raw argument value.
    Return:
      The quoted argument, safe for inclusion in a shell command line.
    */
    static std::string
    shell_quote(
        const std::string& _arg
    )
    {
        std::string result;
        std::size_t i;

        result = "'";
        i      = 0;

        // escape each embedded single quote as the sequence '\''
        for (i = 0; i < _arg.size(); ++i)
        {
            if (_arg[i] == '\'')
            {
                result += "'\\''";
            }
            else
            {
                result += _arg[i];
            }
        }

        result += "'";

        return result;
    }

NS_END  // internal


// =============================================================================
// II.  COMMIT-LOG CONTAINER
// =============================================================================

const commit_log::size_type commit_log::max_size =
    std::numeric_limits<commit_log::size_type>::max();

commit_log::commit_log()
    : m_commits()
{}

commit_log::commit_log(
    const std::vector<commit>& _commits
)
    : m_commits(_commits)
{}

/*
commit_log::push_back
  Appends a commit to the end of the log.

Parameter(s):
  _commit: the commit record to append.
Return:
  none.
*/
void
commit_log::push_back(
    const commit& _commit
)
{
    m_commits.push_back(_commit);

    return;
}

/*
commit_log::clear
  Removes every commit from the log.

Parameter(s):
  none.
Return:
  none.
*/
void
commit_log::clear()
{
    m_commits.clear();

    return;
}

/*
commit_log::empty
  Reports whether the log holds no commits.

Parameter(s):
  none.
Return:
  true if the log is empty, false otherwise.
*/
bool
commit_log::empty() const
{
    return m_commits.empty();
}

/*
commit_log::size
  Reports the number of commits in the log.

Parameter(s):
  none.
Return:
  The commit count.
*/
commit_log::size_type
commit_log::size() const
{
    return m_commits.size();
}

/*
commit_log::front
  Accesses the first (newest) commit. Behaviour is undefined when the log is
empty.

Parameter(s):
  none.
Return:
  A reference to the first commit.
*/
commit_log::reference
commit_log::front()
{
    return m_commits.front();
}

commit_log::const_reference
commit_log::front() const
{
    return m_commits.front();
}

/*
commit_log::at
  Accesses the commit at `_index` with bounds checking.

Parameter(s):
  _index: the zero-based position of the commit.
Return:
  A reference to the requested commit.
*/
commit_log::reference
commit_log::at(
    size_type _index
)
{
    return m_commits.at(_index);
}

commit_log::const_reference
commit_log::at(
    size_type _index
) const
{
    return m_commits.at(_index);
}

commit_log::iterator
commit_log::begin()
{
    return m_commits.begin();
}

commit_log::iterator
commit_log::end()
{
    return m_commits.end();
}

commit_log::const_iterator
commit_log::begin() const
{
    return m_commits.begin();
}

commit_log::const_iterator
commit_log::end() const
{
    return m_commits.end();
}

commit_log::const_iterator
commit_log::cbegin() const
{
    return m_commits.cbegin();
}

commit_log::const_iterator
commit_log::cend() const
{
    return m_commits.cend();
}


// =============================================================================
// III. OPERATION RESULTS
// =============================================================================

template<typename Value>
result<Value>::result(
    status      _code,
    std::string _detail
)
    : m_code(_code),
      m_value(),
      m_detail(_detail)
{}

template<typename Value>
result<Value>::result(
    value_type  _value,
    std::string _detail
)
    : m_code(status::ok),
      m_value(_value),
      m_detail(_detail)
{}

template<typename Value>
result<Value>::operator bool() const
{
    return (m_code == status::ok);
}

template<typename Value>
status
result<Value>::code() const
{
    return m_code;
}

template<typename Value>
const typename result<Value>::value_type&
result<Value>::value() const
{
    return m_value;
}

template<typename Value>
const std::string&
result<Value>::detail() const
{
    return m_detail;
}

// explicit instantiations
//   the result template is consumed only by the repository facade, so the
// instantiations are bounded and may be emitted from this translation unit.
template class result<bool>;
template class result<commit>;
template class result<commit_log>;


// =============================================================================
// IV.  REPOSITORY FACADE
// =============================================================================

repository::repository(
    std::string _work_tree
)
    : m_work_tree(_work_tree)
{}

/*
repository::m_run
  Executes the git client inside the configured work tree and captures its
standard output. The command is assembled as
`git -C <work_tree> <args...> 2>/dev/null`, with every token shell-quoted.

Parameter(s):
  _args:   the git sub-command and its arguments, e.g. {"add", "src/x.cpp"}.
  _output: receives the captured standard output of the process.
Return:
  The process exit code, or -1 if the process could not be started.
*/
int
repository::m_run(
    const std::vector<std::string>& _args,
    std::string&                    _output
) const
{
    std::string            command;
    std::array<char, 4096> buffer;
    std::FILE*             pipe;
    std::size_t            i;
    int                    exit_code;

    _output.clear();
    command = "git -C " + internal::shell_quote(m_work_tree);

    // append each quoted argument to the command line
    for (i = 0; i < _args.size(); ++i)
    {
        command += " ";
        command += internal::shell_quote(_args[i]);
    }

    command += " 2>/dev/null";

    pipe = ::popen(command.c_str(), "r");

    // fail cleanly if the process could not be launched
    if (!pipe)
    {
        return -1;
    }

    // drain standard output into the caller's buffer
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
    {
        _output += buffer.data();
    }

    exit_code = ::pclose(pipe);

    return exit_code;
}

/*
repository::is_repository
  Reports whether the configured work tree is inside a git repository.

Parameter(s):
  none.
Return:
  true if the work tree is part of a git repository, false otherwise.
*/
bool
repository::is_repository() const
{
    std::vector<std::string> args;
    std::string              output;
    int                      code;

    args.push_back("rev-parse");
    args.push_back("--is-inside-work-tree");
    output = "";

    code = m_run(args, output);

    return (code == 0);
}

/*
repository::stage
  Adds a path to the staging index via `git add`.

Parameter(s):
  _path: the file or directory to stage, relative to the work tree.
Return:
  A result whose status is:
  - ok,               on success;
  - not_a_repository, if the work tree is not a git repository;
  - backend_failure,  if git reported a non-zero exit code.
*/
result<bool>
repository::stage(
    const std::string& _path
)
{
    std::vector<std::string> args;
    std::string              output;
    int                      code;

    // verify the work tree is usable before mutating the index
    if (!is_repository())
    {
        return result<bool>(status::not_a_repository,
                             "work tree is not a git repository");
    }

    args.push_back("add");
    args.push_back("--");
    args.push_back(_path);
    output = "";

    code = m_run(args, output);

    // a non-zero exit code indicates git refused the operation
    if (code != 0)
    {
        return result<bool>(status::backend_failure,
                             "git add failed");
    }

    return result<bool>(true, "staged");
}

/*
repository::unstage
  Removes a path from the staging index via `git restore --staged`, leaving the
working-tree file untouched.

Parameter(s):
  _path: the file or directory to unstage, relative to the work tree.
Return:
  A result whose status is ok on success, not_a_repository if the work tree is
not a git repository, or backend_failure on a non-zero git exit code.
*/
result<bool>
repository::unstage(
    const std::string& _path
)
{
    std::vector<std::string> args;
    std::string              output;
    int                      code;

    // verify the work tree is usable before mutating the index
    if (!is_repository())
    {
        return result<bool>(status::not_a_repository,
                             "work tree is not a git repository");
    }

    args.push_back("restore");
    args.push_back("--staged");
    args.push_back("--");
    args.push_back(_path);
    output = "";

    code = m_run(args, output);

    // a non-zero exit code indicates git refused the operation
    if (code != 0)
    {
        return result<bool>(status::backend_failure,
                             "git restore --staged failed");
    }

    return result<bool>(true, "unstaged");
}

/*
repository::staged_paths
  Lists the paths currently present in the staging index via
`git diff --cached --name-only`.

Parameter(s):
  none.
Return:
  A vector of staged paths, empty when nothing is staged or the work tree is
not a git repository.
*/
std::vector<std::string>
repository::staged_paths() const
{
    std::vector<std::string> args;
    std::vector<std::string> paths;
    std::string              output;
    std::string              line;
    std::istringstream       stream;
    int                      code;

    args.push_back("diff");
    args.push_back("--cached");
    args.push_back("--name-only");
    output = "";

    code = m_run(args, output);

    // surface an empty list on any backend trouble
    if (code != 0)
    {
        return paths;
    }

    stream.str(output);

    // one path per line of git output
    while (std::getline(stream, line))
    {
        if (!line.empty())
        {
            paths.push_back(line);
        }
    }

    return paths;
}

/*
repository::commit
  Records the staged changes as a new commit. The author identity is passed to
git through the GIT_AUTHOR_* environment of the invoked process; here it is
forwarded via `--author` and the message via `-m`.

Parameter(s):
  _message: the commit message.
  _author:  the identity to record as the commit author.
Return:
  A result holding the newly created commit on success, or a status of
nothing_to_commit when the index is empty, not_a_repository when the work tree
is not a git repository, or backend_failure on a non-zero git exit code.
*/
result<struct commit>
repository::commit(
    const std::string& _message,
    const signature&   _author
)
{
    std::vector<std::string> args;
    std::string              author_spec;
    std::string              output;
    object_id                head;
    struct commit            record;
    int                      code;

    // verify the work tree is usable before attempting a commit
    if (!is_repository())
    {
        return result<struct commit>(status::not_a_repository,
                                     "work tree is not a git repository");
    }

    // refuse to create an empty commit
    if (staged_paths().empty())
    {
        return result<struct commit>(status::nothing_to_commit,
                                     "no staged changes to commit");
    }

    author_spec = _author.name + " <" + _author.email + ">";

    args.push_back("commit");
    args.push_back("--author");
    args.push_back(author_spec);
    args.push_back("-m");
    args.push_back(_message);
    output = "";

    code = m_run(args, output);

    // a non-zero exit code indicates git refused the commit
    if (code != 0)
    {
        return result<struct commit>(status::backend_failure,
                                     "git commit failed");
    }

    // resolve the hash of the commit just created
    args.clear();
    args.push_back("rev-parse");
    args.push_back("HEAD");
    output = "";

    code = m_run(args, output);

    // strip the trailing newline from the resolved hash
    if (!output.empty() && (output[output.size() - 1] == '\n'))
    {
        output.erase(output.size() - 1);
    }

    head = output;

    record.id        = head;
    record.author    = _author;
    record.committer = _author;
    record.message   = _message;

    return result<struct commit>(record, "committed");
}

/*
repository::log
  Walks history newest-first using a machine-readable `git log` format and
parses each record into a commit. Parent hashes and committer identity are not
populated by this minimal parser.

Parameter(s):
  _limit: the maximum number of commits to return; 0 means unlimited.
Return:
  A result holding the parsed commit_log on success, not_a_repository if the
work tree is not a git repository, or backend_failure on a non-zero git exit
code.
*/
result<commit_log>
repository::log(
    std::size_t _limit
) const
{
    std::vector<std::string> args;
    std::string              output;
    std::string              line;
    std::istringstream       stream;
    commit_log               history;
    int                      code;

    // verify the work tree is usable before walking history
    if (!is_repository())
    {
        return result<commit_log>(status::not_a_repository,
                                  "work tree is not a git repository");
    }

    args.push_back("log");

    // apply the caller's bound only when one was requested
    if (_limit > 0)
    {
        std::ostringstream limit_arg;

        limit_arg << "-n" << _limit;
        args.push_back(limit_arg.str());
    }

    // format: <hash>\x1f<author name>\x1f<author email>\x1f<subject>
    args.push_back("--pretty=format:%H%x1f%an%x1f%ae%x1f%s");
    output = "";

    code = m_run(args, output);

    // surface a backend failure to the caller
    if (code != 0)
    {
        return result<commit_log>(status::backend_failure,
                                  "git log failed");
    }

    stream.str(output);

    // one commit per line, fields separated by the unit-separator byte
    while (std::getline(stream, line))
    {
        struct commit      record;
        std::string        field;
        std::istringstream fields(line);

        if (std::getline(fields, field, '\x1f'))
        {
            record.id = field;
        }

        if (std::getline(fields, field, '\x1f'))
        {
            record.author.name = field;
        }

        if (std::getline(fields, field, '\x1f'))
        {
            record.author.email = field;
        }

        if (std::getline(fields, field, '\x1f'))
        {
            record.message = field;
        }

        history.push_back(record);
    }

    return result<commit_log>(history, "history walked");
}

/*
repository::work_tree
  Accesses the configured work-tree path.

Parameter(s):
  none.
Return:
  A reference to the work-tree path string.
*/
const std::string&
repository::work_tree() const
{
    return m_work_tree;
}

NS_END  // git
NS_END  // djinterp

#endif  // floor, for now
